/*
	mount.c (22.10.09)
	exFAT file system implementation library.

	Copyright (C) 2010-2013  Andrew Nayenko

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "exfat.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <inttypes.h>

static uint64_t rootdir_size(const struct exfat* ef)
{
	uint64_t clusters = 0;
	cluster_t rootdir_cluster = le32_to_cpu(ef->sb->rootdir_cluster);

	while (!CLUSTER_INVALID(ef, rootdir_cluster))
	{
		clusters++;
		/* root directory cannot be contiguous because there is no flag
		   to indicate this */
		rootdir_cluster = exfat_next_cluster(ef, ef->root, rootdir_cluster);
	}
	return clusters * CLUSTER_SIZE(*ef->sb);
}

static const char* get_option(const char* options, const char* option_name)
{
	const char* p;
	size_t length = strlen(option_name);

	for (p = strstr(options, option_name); p; p = strstr(p + 1, option_name))
		if ((p == options || p[-1] == ',') && p[length] == '=')
			return p + length + 1;
	return NULL;
}

static int get_int_option(const char* options, const char* option_name,
		int base, int default_value)
{
	const char* p = get_option(options, option_name);

	if (p == NULL)
		return default_value;
	return strtol(p, NULL, base);
}

static bool match_option(const char* options, const char* option_name)
{
	const char* p;
	size_t length = strlen(option_name);

	for (p = strstr(options, option_name); p; p = strstr(p + 1, option_name))
		if ((p == options || p[-1] == ',') &&
				(p[length] == ',' || p[length] == '\0'))
			return true;
	return false;
}

static void parse_options(struct exfat* ef, const char* options)
{
	int sys_umask = umask(0);
	int opt_umask;

	umask(sys_umask); /* restore umask */
	opt_umask = get_int_option(options, "umask", 8, sys_umask);
	ef->dmask = get_int_option(options, "dmask", 8, opt_umask) & 0777;
	ef->fmask = get_int_option(options, "fmask", 8, opt_umask) & 0777;

	ef->uid = get_int_option(options, "uid", 10, geteuid());
	ef->gid = get_int_option(options, "gid", 10, getegid());

	ef->noatime = match_option(options, "noatime");
}

static int verify_vbr_checksum(struct exfat_dev* dev, void* sector,
		size_t sector_size, int backup)
{
	uint32_t vbr_checksum;
	int i, start;
	int j;

	start = backup ? 12 : 0;
	exfat_pread(dev, sector, sector_size, start * sector_size);
	vbr_checksum = exfat_vbr_start_checksum(sector, sector_size);
	for (i = 1; i < 11; i++)
	{
		exfat_pread(dev, sector, sector_size, (i + start) * sector_size);
		vbr_checksum = exfat_vbr_add_checksum(sector, sector_size,
				vbr_checksum);
	}
	exfat_pread(dev, sector, sector_size, (i + start) * sector_size);
	for (i = 0; i < sector_size / sizeof(vbr_checksum); i++)
		if (le32_to_cpu(((const le32_t*) sector)[i]) != vbr_checksum)
		{
			exfat_error("invalid VBR_%d checksum 0x%x (expected 0x%x)", backup,
					vbr_checksum, le32_to_cpu(((const le32_t*) sector)[i]));
			return -1;
		}
	return vbr_checksum;
}

static int commit_super_block(const struct exfat* ef)
{
	exfat_pwrite(ef->dev, ef->sb, sizeof(struct exfat_super_block), 0);
	return exfat_fsync(ef->dev);
}

static int prepare_super_block(const struct exfat* ef)
{
	if (le16_to_cpu(ef->sb->volume_state) & EXFAT_STATE_MOUNTED)
		exfat_warn("volume was not unmounted cleanly");

	if (ef->ro)
		return 0;

	ef->sb->volume_state = cpu_to_le16(
			le16_to_cpu(ef->sb->volume_state) | EXFAT_STATE_MOUNTED);
	return commit_super_block(ef);
}

/* compare boot region with the backup, merge conflict
 * to boot region and write back to disk. Open exfat
 * in 'RW' mode.
 */
static int check_boot_region(const struct exfat *ef)
{
	int rc = 0, r1, r2;
	void *sector;
	void *boot_region;
	int i;

	sector = malloc(SECTOR_SIZE(*ef->sb));
	boot_region = malloc(12 * SECTOR_SIZE(*ef->sb));

	r1 = verify_vbr_checksum(ef->dev, sector, SECTOR_SIZE(*ef->sb), 0);
	r2 = verify_vbr_checksum(ef->dev, sector, SECTOR_SIZE(*ef->sb), 1);

	if (exfat_recovered) {
		if (r1 != -1 && r2 == -1) {
			exfat_pread(ef->dev, boot_region, 12 * SECTOR_SIZE(*ef->sb), 0);
			exfat_pwrite(ef->dev, boot_region, 12 * SECTOR_SIZE(*ef->sb), 12 * SECTOR_SIZE(*ef->sb));
			if (exfat_fsync(ef->dev))
				rc = -1;
			else
				exfat_repair("Backup boot region");
		} else if (r1 == -1 && r2 != -1) {
			exfat_pread(ef->dev, boot_region, 12 * SECTOR_SIZE(*ef->sb), 12 * SECTOR_SIZE(*ef->sb));
			exfat_pwrite(ef->dev, boot_region, 12 * SECTOR_SIZE(*ef->sb), 0);
			if (exfat_fsync(ef->dev))
				rc = -1;
			else
				exfat_repair("Boot region flushed");
		} else if (r1 == -1 && r2 == -1) {
			printf("Checksum error.Need to format.\n");
			rc = -1;
		} else if (r1 != r2) {
			printf("VBR inconsistent.Need to format.\n");
			rc = -1;
		}
	}

	free(boot_region);
	free(sector);
	return rc;
}

int checkdisk(struct exfat *ef, const char* spec)
{
	int rc = -1, cnt = 0, flag = 0;
	int fd, i;
	size_t pgsize;
	s64 offset;
	void *buffer;

	fd = open(spec, O_RDWR | O_DIRECT);
	if (fd < 0) {
		printf("open %s failed while checking disk.\n", spec);
		return -1;
	}

	for (i = 0; i * 8 < le32_to_cpu(ef->sb->cluster_count); i++)
		if (ef->cmap.chunk[i] == 0x00)
			break;

	if (i * 8 >= le32_to_cpu(ef->sb->cluster_count)) {
		exfat_warn("No enough space on disk, check skipped");
		close(fd);
		return 0;
	}
	/* if one cluster address is not align with pgsize, all is not*/
	pgsize = getpagesize();
	exfat_debug("page size is %zu bytes", pgsize);
	if (posix_memalign(&buffer, pgsize, SECTOR_SIZE(*ef->sb))) {
		exfat_warn("No space for checking disk, check skipped");
		close(fd);
		return 0;
	}
	memset(buffer, 0, SECTOR_SIZE(*ef->sb));

	offset = (8 * i + 2) * ((s64)CLUSTER_SIZE(*ef->sb))
		+ le32_to_cpu(ef->sb->cluster_sector_start) * ((s64)SECTOR_SIZE(*ef->sb));
	if (offset & (pgsize - 1)) {
		exfat_warn("address of cluster isn't align with pgsize, check skipped");
		close(fd);
		return 0;
	}
	exfat_debug("write data to cluster(%d)", i * 8);

retry0:
	if (pread64(fd, buffer, SECTOR_SIZE(*ef->sb), offset) != SECTOR_SIZE(*ef->sb)) {
		if ((errno == EIO) && (cnt < 3)) {
			cnt++;
			goto retry0;
		}
		exfat_bug("read error while checking disk");
	}
	if (*((int *)buffer) == 0x77657264) {
		flag = 1;
		*((int *)buffer) = 0x00000000;
	} else {
		flag = 0;
		*((int *)buffer) = 0x77657264;
	}
	cnt = 0;

retry:
	if (pwrite64(fd, buffer, SECTOR_SIZE(*ef->sb), offset) != SECTOR_SIZE(*ef->sb)) {
		if ((errno == EIO) && (cnt < 3)) {
			cnt++;
			goto retry;
		}
		exfat_bug("write error at (%"PRIu64") while checking disk", offset);
	}
	memset(buffer, 0, SECTOR_SIZE(*ef->sb));
	cnt = 0;

retry1:
	if (pread64(fd, buffer, SECTOR_SIZE(*ef->sb), offset) != SECTOR_SIZE(*ef->sb)) {
		if ((errno == EIO) && (cnt < 3)) {
			cnt++;
			goto retry1;
		}
		exfat_bug("read error while checking disk");
	}
	cnt = 0;

	if ((*((int *)buffer) != 0x77657264 && flag == 0) || (*((int *)buffer) != 0x00000000 && flag == 1)) {
		printf("data inconsistent.\n");
		goto error;
	}

	rc = 0;
error:
	close(fd);
	free(buffer);
	return rc;
}

int exfat_mount(struct exfat* ef, const char* spec, enum exfat_mode mode)
{
	int rc = -EIO, tw;
	struct exfat_node* node, *cur_node = NULL;
	struct exfat_iterator it;

	exfat_tzset();
	memset(ef, 0, sizeof(struct exfat));

	if (mode == 0)
		parse_options(ef, "ro");

	ef->dev = exfat_open(spec, mode);
	if (ef->dev == NULL)
		return -ENODEV;
	if (exfat_get_mode(ef->dev) == EXFAT_MODE_RO)
	{
		if (mode == EXFAT_MODE_ANY)
			ef->ro = -1;
		else
			ef->ro = 1;
	}

	ef->sb = malloc(sizeof(struct exfat_super_block));
	if (ef->sb == NULL)
	{
		exfat_close(ef->dev);
		printf("failed to allocate memory for the super block.\n");
		return -ENOMEM;
	}
	memset(ef->sb, 0, sizeof(struct exfat_super_block));

	/* we check root dir structure here, in order to repair
	 * the error of root entry, which will compact the FAT checking */
	exfat_pread(ef->dev, ef->sb, sizeof(struct exfat_super_block), 0);
	printf("Checking boot region and root directory...\n");
	start_count(&fsck_p1_time);
	if (check_boot_region(ef)) {
		printf("Checking stoped.\n");
		exfat_close(ef->dev);
		free(ef->sb);
		return -EIO;
	}

	//Update boot sector after write.
	if (exfat_recovered)
		exfat_pread(ef->dev, ef->sb, sizeof(struct exfat_super_block), 0);
	if (memcmp(ef->sb->oem_name, "EXFAT   ", 8) != 0)
	{
		exfat_close(ef->dev);
		free(ef->sb);
		exfat_error("exFAT file system is not found");
		return -EIO;
	}
	if (le64_to_cpu(ef->sb->sector_start) != ef->dev->start_sec)
	{
		exfat_close(ef->dev);
		free(ef->sb);
		exfat_error("Formatted partition offset(%"PRIu64" bytes) "
			"does not match actual partition offset(%"PRIu64" bytes)",
			le64_to_cpu(ef->sb->sector_start), ef->dev->start_sec);
		return -EIO;
	}
	if (ef->sb->version.major != 1 || ef->sb->version.minor != 0)
	{
		exfat_close(ef->dev);
		exfat_error("unsupported exFAT version: %hhu.%hhu",
				ef->sb->version.major, ef->sb->version.minor);
		free(ef->sb);
		return -EIO;
	}
	if (ef->sb->fat_count != 1)
	{
		exfat_close(ef->dev);
		free(ef->sb);
		exfat_error("unsupported FAT count: %hhu", ef->sb->fat_count);
		return -EIO;
	}
	/* officially exFAT supports cluster size up to 32 MB */
	if ((int) ef->sb->sector_bits + (int) ef->sb->spc_bits > 25)
	{
		exfat_close(ef->dev);
		free(ef->sb);
		exfat_error("too big cluster size: 2^%d",
				(int) ef->sb->sector_bits + (int) ef->sb->spc_bits);
		return -EIO;
	}

	ef->zero_cluster = malloc(CLUSTER_SIZE(*ef->sb));
	if (ef->zero_cluster == NULL)
	{
		exfat_close(ef->dev);
		free(ef->sb);
		printf("failed to allocate zero sector.\n");
		return -ENOMEM;
	}
	memset(ef->zero_cluster, 0, CLUSTER_SIZE(*ef->sb));

	ef->root = malloc(sizeof(struct exfat_node));
	if (ef->root == NULL)
	{
		free(ef->zero_cluster);
		exfat_close(ef->dev);
		free(ef->sb);
		printf("failed to allocate root node.\n");
		return -ENOMEM;
	}
	memset(ef->root, 0, sizeof(struct exfat_node));
	ef->root->flags = EXFAT_ATTRIB_DIR;
	ef->root->start_cluster = le32_to_cpu(ef->sb->rootdir_cluster);
	ef->root->fptr_cluster = ef->root->start_cluster;
	ef->root->name[0] = cpu_to_le16('\0');
	ef->root->size = rootdir_size(ef);
	/* exFAT does not have time attributes for the root directory */
	ef->root->mtime = 0;
	ef->root->atime = 0;
	/* always keep at least 1 reference to the root node */
	exfat_get_node(ef->root);

	rc = exfat_cache_directory(ef, ef->root, 0);
	if (rc != 0)
		goto error;
	if (ef->upcase == NULL)
	{
		exfat_error("upcase table is not found");
		goto error;
	}
	if (ef->cmap.chunk == NULL)
	{
		exfat_error("clusters bitmap is not found");
		goto error;
	}

	/* check disk writable? */
	if (exfat_recovered) {
		if ((rc = checkdisk(ef, spec)) != 0)
			goto error;
	}

	if (ef->root->twins == NULL)
		ef->root->has_twin = 0;
	/* root directory check*/
	for (tw = 0; tw <= ef->root->has_twin; tw++) {
		it.parent = ef->root;
		it.current = NULL;
		while ((node = exfat_readdir(&it, cur_node, tw))) {
			char buffer[EXFAT_NAME_MAX + 1];
			exfat_get_name(node, buffer, EXFAT_NAME_MAX);
			//exfat_debug("%s: %s", buffer, IS_CONTIGUOUS(*node) ? "contiguous" : "fragmented");

			if (node->invalid_flag && exfat_recovered) {
				if (delete(ef, node) != 0)
					goto error;
				exfat_repair("delete dir/file '%s'", buffer);
			}
			cur_node = it.current->next;
			exfat_put_node(ef, node);
		}
		it.parent = NULL;
		it.current = NULL;
	}
	end_count("Checking boot region and root directory", &fsck_p1_time);

	if (prepare_super_block(ef) != 0)
		goto error;

	return 0;

error:
	exfat_put_node(ef, ef->root);
	exfat_reset_cache(ef);
	free(ef->root);
	free(ef->zero_cluster);
	exfat_close(ef->dev);
	free(ef->sb);
	return rc;
}

static void finalize_super_block(struct exfat* ef)
{
	if (ef->ro)
		return;

	ef->sb->volume_state = cpu_to_le16(
			le16_to_cpu(ef->sb->volume_state) & ~EXFAT_STATE_MOUNTED);

	/* Some implementations set the percentage of allocated space to 0xff
	   on FS creation and never update it. In this case leave it as is. */
	if (ef->sb->allocated_percent != 0xff)
	{
		uint32_t free, total;

		free = exfat_count_free_clusters(ef);
		total = le32_to_cpu(ef->sb->cluster_count);
		ef->sb->allocated_percent = ((total - free) * 100 + total / 2) / total;
	}

	commit_super_block(ef);
}

void exfat_unmount(struct exfat* ef)
{
	exfat_fsync(ef->dev);
	exfat_put_node(ef, ef->root);
	exfat_reset_cache(ef);
	free(ef->root);
	ef->root = NULL;
	finalize_super_block(ef);
	exfat_close(ef->dev);	/* close descriptor immediately after fsync */
	ef->dev = NULL;
	free(ef->zero_cluster);
	ef->zero_cluster = NULL;
	free(ef->cmap.chunk);
	ef->cmap.chunk = NULL;
	free(ef->sb);
	ef->sb = NULL;
	free(ef->upcase);
	ef->upcase = NULL;
	ef->upcase_chars = 0;
}
