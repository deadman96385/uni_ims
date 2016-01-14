/*
	main.c (02.09.09)
	exFAT file system checker.

	Copyright (C) 2011-2013  Andrew Nayenko

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

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <exfat.h>
#include <exfatfs.h>
#include <inttypes.h>
#include <unistd.h>
#include <pthread.h>
#include <atomic.h>

uint64_t files_count, directories_count;
int exfat_recovered = 0, exfat_info = 0, exfat_repair_num = 0;
cluster_t *fat_map = NULL;
char *bit_map = NULL;
int root_map = 0;
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
typedef struct {
	struct exfat ef;
	char *path;
	int tw;
} arg_type;
typedef int (*func)(void *);
int threads = 0;

/* we rebuild the bitmap while cluster of node not allocated
 * Note that we did this action only in directory checking!
 */
static int nodeck(struct exfat* ef, struct exfat_node* node)
{
	const cluster_t cluster_size = CLUSTER_SIZE(*ef->sb);
	cluster_t clusters = (node->size + cluster_size - 1) / cluster_size;
	cluster_t last, c = node->start_cluster;
	int rc = 0;
	char name[EXFAT_NAME_MAX + 1];

	if (node->invalid_flag) {
		exfat_debug("error node marked before");
		return rc;
	}

	exfat_get_name(node, name, EXFAT_NAME_MAX);
	while (clusters--) {
		if (c == EXFAT_CLUSTER_END) {
			exfat_error("End of file '%s', node size not equal to FAT clusters", name);
			if (exfat_recovered) {
				exfat_update_mtime(node);
				node->size = ((node->size + cluster_size - 1) / cluster_size - clusters - 1) * CLUSTER_SIZE(*ef->sb);
				node->flags |= EXFAT_ATTRIB_DIRTY;
				exfat_repair("size of '%s' repaired", name);
			}
			break;
		}
		if (CLUSTER_INVALID(ef, c)) {
			exfat_debug("file `%s' has invalid cluster 0x%x", name, c);
			rc = 1;
			break;
		}
		if (BMAP_GET(ef->cmap.chunk, c - EXFAT_FIRST_DATA_CLUSTER) == 0) {
			/* we rebuild the bitmap here*/
			exfat_error("cluster 0x%x of file '%s' is not allocated", c, name);
			if (exfat_recovered) {
				BMAP_SET(ef->cmap.chunk, c - EXFAT_FIRST_DATA_CLUSTER);
				exfat_repair("rebuild bitmap");
			}
		}
		last = c;
		c = exfat_next_cluster(ef, node, c);
		/* node size fit the FAT clusters */
		if (!IS_CONTIGUOUS(*node) && clusters == 0 && c != EXFAT_CLUSTER_END) {
			exfat_error("FAT allocated clusters not equal to the node size");
			exfat_debug("to set FAT num=%x, (%s)", last, name);
			if (exfat_recovered) {
				set_next_cluster(ef, 0, last, EXFAT_CLUSTER_END);
				exfat_repair("truncate FAT to fit the node size");
			}
		}
		if (node->start_cluster == ef->root->start_cluster)
			exfat_debug("root cluster=0x%x", c);
	}

	if (rc) {
		exfat_error("Invalid file %s", name);
		if (!exfat_recovered)
			exit(3);
	}
	return rc;
}

static int multi_thread(struct exfat *ef, const char *path, func fp)
{
	int s;
	pthread_t t1, t2;
	void *status1, *status2;
	arg_type arg1, arg2;

	arg1.ef = arg2.ef = *ef;
	arg1.path = arg2.path = path;
	arg1.tw = 0;
	arg2.tw = 1;

	exfat_debug("create multi-thread for current node");
	threads++;
	s = pthread_create(&t1, NULL, fp, &arg1);
	if (s != 0) {
		printf("pthread1 create failed\n");
		return -ENOMEM;
	}
	s = pthread_create(&t2, NULL, fp, &arg2);
	if (s != 0) {
		printf("pthread2 create failed\n");
		return -ENOMEM;
	}
	s = pthread_join(t1, &status1);
	if (s != 0) {
		printf("pthread1 join failed\n");
		return -ENOMEM;
	}
	s = pthread_join(t2, &status2);
	if (s != 0) {
		printf("pthread2 join failed\n");
		return -ENOMEM;
	}

	threads--;
	if ((long)status1 != 0 || (long)status2 != 0)
		return -1;

	return 0;
}

static int dirck(void *arg)
{
	struct exfat *ef;
	char *path;
	int rc = 0, tw;
	struct exfat_node* parent;
	struct exfat_node* node, *cur_node = NULL;
	struct exfat_iterator it;
	size_t path_length;
	char* entry_path;
	arg_type *arg_temp;

	arg_temp = (arg_type *)arg;
	ef = &(arg_temp->ef);
	path = arg_temp->path;
	tw = arg_temp->tw;
	if (path == NULL) {
		exfat_debug("pthread start root path");
		path = "";
	}
	if (exfat_lookup(ef, &parent, path, tw) != 0)
		exfat_bug("directory `%s' is not found", path);
	if (!(parent->flags & EXFAT_ATTRIB_DIR))
		exfat_bug("`%s' is not a directory (0x%x)", path, parent->flags);
	if (nodeck(ef, parent) != 0) {
		exfat_debug("Should not print here!");
		return -EIO;
	}

	path_length = strlen(path);
	entry_path = malloc(path_length + 1 + EXFAT_NAME_MAX);
	if (entry_path == NULL) {
		printf("Failed to allocate memory while checking dir.\n");
		return -ENOMEM;
	}
	memset(entry_path, 0, path_length + 1 + EXFAT_NAME_MAX);
	strcpy(entry_path, path);
	strcat(entry_path, "/");

	if (exfat_opendir(ef, parent, &it, 0) != 0) {
		free(entry_path);
		exfat_put_node(ef, parent);
		printf("Failed to open directory '%s'\n", path);
		return -EBUSY;
	}

	if (parent->has_twin) {
		int result;

		parent->has_twin = 0;/* clear for detach*/
		exfat_put_node(ef, parent);
		result = multi_thread(ef, path, dirck);
		parent->has_twin = 1;/* set for next scan*/
		free(entry_path);
		if (result != 0)
			return -1;
		else {
			exfat_put_node(ef, parent);
			return 0;
		}
	}
	while ((node = exfat_readdir(&it, cur_node, tw))) {
		le16_t hash;
		exfat_get_name(node, entry_path + path_length + 1, EXFAT_NAME_MAX);
		//exfat_debug("%s: %s, %"PRIu64" bytes, cluster %u", entry_path,
		//		IS_CONTIGUOUS(*node) ? "contiguous" : "fragmented",
		//		node->size, node->start_cluster);

		if (node->invalid_flag && exfat_recovered) {
			if (delete(ef, node) != 0)
				return -EIO;
			exfat_repair("delete entry '%s'", entry_path);
			cur_node = it.current->next;
			exfat_put_node(ef, node);
			continue;
		}
		/* checksum contained the change of name_hash.
		hash = exfat_calc_name_hash(ef, node->name);
		if (memcmp(&(node->name_hash), &hash, sizeof(le16_t))) {
			exfat_error("name hash inconsistent 0x%x(expected 0x%x)", node->name_hash, hash);
			if (exfat_recovered) {
				node->name_hash = hash;
				node->flags |= EXFAT_ATTRIB_DIRTY;
				exfat_repair("name hash updated");
			}
		}
		*/
		arg_temp->path = entry_path;
		if (node->flags & EXFAT_ATTRIB_DIR) {
			android_atomic_inc(&directories_count);
			if ((rc = dirck(arg_temp)) != 0) {
				free(entry_path);
				return rc;
			}
		} else {
			android_atomic_inc(&files_count);
			if (nodeck(ef, node) && exfat_recovered) {
				char name[EXFAT_NAME_MAX + 1];
				exfat_get_name(node, name, EXFAT_NAME_MAX);
				if (delete(ef, node) != 0)
					return -EIO;
				android_atomic_dec(&files_count);
				exfat_repair("errors of file '%s' repaired", name);
			}
		}
		cur_node = it.current->next;
		exfat_put_node(ef, node);
	}
	exfat_closedir(ef, &it);
	exfat_put_node(ef, parent);
	free(entry_path);

	return rc;
}

static uint32_t set_fat_map(struct exfat *ef, struct exfat_node *node)
{
	cluster_t cl, head, last, clusters = 0;

	last = head = cl = node->start_cluster;
	for (cl = node->start_cluster; ; cl = exfat_next_cluster(ef, node, cl)) {
		if (!IS_CONTIGUOUS(*node)) {
			if (cl == EXFAT_CLUSTER_END)
				break;
		} else {
			if (clusters >= (node->size + CLUSTER_SIZE(*ef->sb) - 1) / CLUSTER_SIZE(*ef->sb))
				break;
		}
		if (CLUSTER_INVALID(ef, cl)) {
			exfat_error("Invalid cluster number(%d)", cl);
			if (exfat_recovered) {
				set_next_cluster(ef, IS_CONTIGUOUS(*node), last, EXFAT_CLUSTER_END);
				exfat_update_mtime(node);
				node->size =  clusters * CLUSTER_SIZE(*ef->sb);
				node->flags |= EXFAT_ATTRIB_DIRTY;
				exfat_repair("truncate FAT chain and discard invalid cluster number");
				break;
			} else {
				printf("Cannot verify because of earlier errors.Use '-R' to recovery it\n");
				exit(3);
			}
		}
		if (android_atomic_release_cas(0, head, &fat_map[cl])) {
			exfat_error("Found crosslinked chain(at %d)", cl);
			return cl;
		}
		clusters++;
		last = cl;
	}

	node->cached = 1;
	//exfat_debug("Clean chain");
	return 0;
}

/*
 * we can truncate cluster chain if node present a file.
 * if not, this function need to be rewrited.
 * we did not rewrite because one cluster is large enough for the exfat dir.
 */
static int check_fat_chain(struct exfat *ef, struct exfat_node *node)
{
	char buffer[EXFAT_NAME_MAX + 1];
	cluster_t cl, last;
	uint32_t result;
	int clusters = 0;

	if (node->start_cluster == 0 || node->cached)
		return 0;

	exfat_get_name(node, buffer, EXFAT_NAME_MAX);
	//exfat_debug("%s:cluster chain head=%d", buffer, node->start_cluster);
	result = set_fat_map(ef, node);
	/* crosslinked at start cluster, mark to delete*/
	if (result > 0 && result == node->start_cluster) {
		if (exfat_recovered) {
			erase_entry(ef, node);
			exfat_repair("chain cross at start cluster, entry erased");
		}
		return 0;
	}
	if (result > 0 && exfat_recovered) {
		exfat_debug("chain cross cluster=%d", result);
		last = node->start_cluster;
		for (cl = node->start_cluster; cl != result; cl = exfat_next_cluster(ef, node, cl)) {
			last = cl;
			clusters++;
		}
		exfat_debug("truncate cluster=%d", last);
		/* we truncate the crosslinked chain into two chains*/
		set_next_cluster(ef, IS_CONTIGUOUS(*node), last, EXFAT_CLUSTER_END);
		exfat_update_mtime(node);
		node->size = clusters * CLUSTER_SIZE(*ef->sb);
		node->flags |= EXFAT_ATTRIB_DIRTY;
		/* crosslinked chain belongs to a directory, mark to delete */
		if (node->flags & EXFAT_ATTRIB_DIR) {
			exfat_debug("crosslinded directory");
			node->invalid_flag = 1;
		}

		exfat_repair("crosslinked chains repaired");
	}

	return 0;
}

static int check_fat(void *arg)
{
	struct exfat *ef;
	char *path;
	int rc = 0, s, tw;
	struct exfat_node *parent;
	struct exfat_node *node, *cur_node = NULL;
	struct exfat_iterator it;
	size_t path_length;
	char* entry_path;
	pthread_t t1, t2;
	void *status1, *status2;
	arg_type *arg_temp;

	arg_temp = (arg_type *)arg;
	ef = &(arg_temp->ef);
	path = arg_temp->path;
	tw = arg_temp->tw;
	if (path == NULL) {
		exfat_debug("pthread start root path");
		path = "";
	}
	if (exfat_lookup(ef, &parent, path, tw) != 0)
		printf("directory `%s' is not found\n", path);
	if (!(parent->flags & EXFAT_ATTRIB_DIR))
		printf("`%s' is not a directory (0x%x)\n", path, parent->flags);
	if (nodeck(ef, parent) != 0)
		return -EIO;
	/* set the root cluster chain map at the very beginning*/
	if (parent->start_cluster == ef->root->start_cluster && root_map == 0 && !parent->cached) {
		exfat_debug("setting root fat map");
		set_fat_map(ef, parent);
		android_atomic_release_cas(0, 1, &root_map);
	}

	path_length = strlen(path);
	entry_path = malloc(path_length + 1 + EXFAT_NAME_MAX);
	if (entry_path == NULL) {
		printf("failed to malloc entry_path while checking fat.\n");
		return -ENOMEM;
	}
	memset(entry_path, 0, path_length + 1 + EXFAT_NAME_MAX);
	strcpy(entry_path, path);
	strcat(entry_path, "/");

	if (parent->start_cluster == 0 || CLUSTER_INVALID(ef, parent->start_cluster)) {
		exfat_debug("directory start cluster is zero or invalid, skip it");
		return 0;
	}
	if (exfat_opendir(ef, parent, &it, 1) != 0) {
		free(entry_path);
		exfat_put_node(ef, parent);
		printf("failed to open directory '%s'\n", path);
		return -EBUSY;
	}

	if (parent->has_twin) {
		int result;

		parent->has_twin = 0;/* clear for detach*/
		exfat_put_node(ef, parent);
		result = multi_thread(ef, path, check_fat);
		parent->has_twin = 1;/* set for next scan*/
		free(entry_path);
		if (result != 0)
			return -1;
		else {
			exfat_put_node(ef, parent);
			return 0;
		}
	}
	while ((node = exfat_readdir(&it, cur_node, tw))) {
		/*
		if (node->invalid_flag) {
			cur_node = it.current->next;
			exfat_put_node(ef, node);
			continue;
		}
		*/
		exfat_get_name(node, entry_path + path_length + 1, EXFAT_NAME_MAX);
		if ((rc = check_fat_chain(ef, node)) < 0)
			break;
		//exfat_debug("%s(%s) flag=0x%x, references=%d", (node->flags & EXFAT_ATTRIB_DIR) ? "Dir" : "File",
		//	entry_path, node->flags, node->references);
		if (node->invalid_flag && exfat_recovered) {
			if (delete(ef, node) != 0)
				return -EIO;
			exfat_repair("delete entry '%s'", entry_path);
			cur_node = it.current->next;
			exfat_put_node(ef, node);
			continue;
		}
		arg_temp->path = entry_path;
		if (node->flags & EXFAT_ATTRIB_DIR) {
			if ((rc = check_fat(arg_temp)) != 0) {
				free(entry_path);
				return rc;
			}
		}
		cur_node = it.current->next;
		exfat_put_node(ef, node);
	}

	exfat_closedir(ef, &it);
	exfat_put_node(ef, parent);
	free(entry_path);

	return rc;
}

static int check_bitmap(void *arg)
{
	struct exfat *ef;
	char *path;
	int rc = 0, s, tw;
	struct exfat_node* parent;
	struct exfat_node* node, *cur_node = NULL;
	struct exfat_iterator it;
	size_t path_length;
	char* entry_path;
	cluster_t cl;
	uint32_t cluster_num, i;
	pthread_t t1, t2;
	void *status1, *status2;
	arg_type *arg_temp;

	arg_temp = (arg_type *)arg;
	ef = &(arg_temp->ef);
	path = arg_temp->path;
	tw = arg_temp->tw;
	if (path == NULL) {
		exfat_debug("pthread start root path");
		path = "";
	}
	if (exfat_lookup(ef, &parent, path, tw) != 0)
		printf("directory `%s' is not found\n", path);
	if (!(parent->flags & EXFAT_ATTRIB_DIR))
		printf("`%s' is not a directory (0x%x)\n", path, parent->flags);
	if (nodeck(ef, parent) != 0)
		return -EIO;

	if (parent->start_cluster == ef->root->start_cluster && root_map == 0) {
		exfat_debug("set bitmap of root directory");
		for (cl = parent->start_cluster; cl != EXFAT_CLUSTER_END; cl = exfat_next_cluster(ef, parent, cl)) {
			pthread_mutex_lock(&mtx);
			BMAP_SET(bit_map, cl - EXFAT_FIRST_DATA_CLUSTER);
			pthread_mutex_unlock(&mtx);
		}
		root_map = 1;
	}
	path_length = strlen(path);
	entry_path = malloc(path_length + 1 + EXFAT_NAME_MAX);
	if (entry_path == NULL)	{
		printf("out of memory while checking bitmap\n");
		return -ENOMEM;
	}
	memset(entry_path, 0, path_length + 1 + EXFAT_NAME_MAX);
	strcpy(entry_path, path);
	strcat(entry_path, "/");

	if (exfat_opendir(ef, parent, &it, 0) != 0) {
		free(entry_path);
		exfat_put_node(ef, parent);
		printf("failed to open directory '%s'\n", path);
		return -EBUSY;
	}

	if (parent->has_twin) {
		int result;

		parent->has_twin = 0;/* clear for detach*/
		exfat_put_node(ef, parent);
		result = multi_thread(ef, path, check_bitmap);
		parent->has_twin = 1;/* set for next scan*/
		free(entry_path);
		if (result != 0)
			return -1;
		else {
			exfat_put_node(ef, parent);
			return 0;
		}
	}
	while ((node = exfat_readdir(&it, cur_node, tw))) {
		/* empty file did not allocate any clusters, ignore*/
		if (!(node->flags & EXFAT_ATTRIB_DIR) && node->size == 0 && node->start_cluster == 0) {
			cur_node = it.current->next;
			exfat_put_node(ef, node);
			continue;
		}

		exfat_get_name(node, entry_path + path_length + 1, EXFAT_NAME_MAX);
		cluster_num = node->size / CLUSTER_SIZE(*ef->sb);
		if (node->size % CLUSTER_SIZE(*ef->sb))
			cluster_num++;
		//exfat_debug("%s: %s, %"PRIu64" bytes, cluster %u", entry_path,
		//		IS_CONTIGUOUS(*node) ? "contiguous" : "fragmented",
		//		node->size, node->start_cluster);
		if (IS_CONTIGUOUS(*node)) {
			for (i = 0, cl = node->start_cluster; i < cluster_num; cl++, i++) {
				pthread_mutex_lock(&mtx);
				BMAP_SET(bit_map, cl - EXFAT_FIRST_DATA_CLUSTER);
				pthread_mutex_unlock(&mtx);
			}
		} else {
			for (cl = node->start_cluster; cl != EXFAT_CLUSTER_END; cl = exfat_next_cluster(ef, node, cl)) {
				pthread_mutex_lock(&mtx);
				BMAP_SET(bit_map, cl - EXFAT_FIRST_DATA_CLUSTER);
				pthread_mutex_unlock(&mtx);
			}
		}
		arg_temp->path = entry_path;
		if (node->flags & EXFAT_ATTRIB_DIR) {
			if ((rc = check_bitmap(arg_temp)) != 0) {
				free(entry_path);
				return rc;
			}
		}
		cur_node = it.current->next;
		exfat_put_node(ef, node);
	}
	exfat_closedir(ef, &it);
	exfat_put_node(ef, parent);
	free(entry_path);

	return rc;
}

static void get_used_mem()
{
	int fd;
	FILE *info;
	char aux[256], path[256], name[256];
	unsigned long rss = 0;

	fd = getpid();
	sprintf(path, "/proc/%d/status", fd);
	if(!(info = fopen(path, "r"))) {
		printf("open /proc/%d/status failed!\n", fd);
		return;
	}

	while( !feof(info) && !fscanf(info, "Name: %s", name) )
            fgets(aux, sizeof(aux), info);
	while( !feof(info) && !fscanf(info, "VmRSS: %lu kB", &rss) )
            fgets(aux, sizeof(aux), info);
	fclose(info);

	exfat_debug("Process name: %s, VmRss=%lu kB", name, rss);
}

/*
 * Check boot region while 'mounting'.
 */
static int fsck(struct exfat *ef)
{
	char bitmap_rest, chunk_rest;
	uint32_t fat_size;
	int rc = -EIO, cnt = 0;
	uint32_t i, j, root_start, root_rest, rest;
	char path[EXFAT_NAME_MAX + 1];
	int result, temp, lost_flag = 0, lost_files = 0;
	uint64_t bits = 0;
	cluster_t start_cluster = 0, cur_cl = 0, pre_cl = 0;
	le32_t first, second;
	arg_type arg;

	fat_size = SECTOR_SIZE(*ef->sb) * le32_to_cpu(ef->sb->fat_sector_count);
	exfat_debug("exfat FAT size =%d bytes", fat_size);
	fat_map = (cluster_t *)malloc(fat_size);
	bit_map = (char *)malloc(ef->cmap.size);
	if (fat_map == NULL || bit_map == NULL) {
		printf("No memory. Checking stoped.\n");
		return -ENOMEM;
	}
	memset(bit_map, 0, ef->cmap.size);
	memset(fat_map, 0, fat_size);

	printf("Checking FAT...\n");
	start_count(&fsck_p2_time);
	/*First two relative locations should always have those same values in them.*/
	exfat_pread(ef->dev, &first, sizeof(le32_t), s2o(ef, le32_to_cpu(ef->sb->fat_sector_start)));
	exfat_pread(ef->dev, &second, sizeof(le32_t), s2o(ef, le32_to_cpu(ef->sb->fat_sector_start)) + sizeof(le32_t));
	if (le32_to_cpu(first) != 0xFFFFFFF8) {
		exfat_error("Invalid value for first FAT entry(0x%X)", le32_to_cpu(first));
		if (exfat_recovered) {
			first = cpu_to_le32(0xFFFFFFF8);
			exfat_pwrite(ef->dev, &first, sizeof(le32_t), s2o(ef, le32_to_cpu(ef->sb->fat_sector_start)));
			exfat_repair("Invalid value of first FAT entry repaired");
		}
	}
	if (le32_to_cpu(second) != 0xFFFFFFFF) {
		exfat_error("Second FAT entry is not set to 0xFFFFFFFF(0x%X)", le32_to_cpu(second));
		if (exfat_recovered) {
			second = cpu_to_le32(0xFFFFFFFF);
			exfat_pwrite(ef->dev, &second, sizeof(le32_t), s2o(ef, le32_to_cpu(ef->sb->fat_sector_start)) + sizeof(le32_t));
			exfat_repair("Invalid value of second FAT entry repaired");
		}
	}

	arg.ef = *ef;
	arg.path = "";
	arg.tw = 0;
	if (ef->root->twins) {
		int result;

		ef->root->has_twin = 0;
		result = multi_thread(ef, "", check_fat);
		ef->root->has_twin = 1;
		if (result != 0)
			goto error;
	} else
		if (check_fat(&arg) != 0)
			goto error;
	root_map = 0;
	get_used_mem();
	end_count("Checking FAT", &fsck_p2_time) ;

	printf("Checking directory structure...\n");
	start_count(&fsck_p3_time);
	/* After FAT check, we might have repaired some errors.
	 * So we recach the dir tree to check dir structure. */
	exfat_debug("root directory contiguous=%d", IS_CONTIGUOUS(*(ef->root)));
	exfat_put_node(ef, ef->root);
	exfat_reset_cache(ef);
	exfat_get_node(ef->root);
	if (exfat_cache_directory(ef, ef->root, 0) != 0) {
		exfat_debug("recached failed.Checking stopped");
		goto error;
	}
	if (ef->root->twins == NULL)
		ef->root->has_twin = 0;

	arg.ef = *ef;
	arg.path = "";
	arg.tw = 0;
	if (ef->root->twins) {
		int result;

		ef->root->has_twin = 0;
		result = multi_thread(ef, "", dirck);
		ef->root->has_twin = 1;
		if (result != 0)
			goto error;
	} else
		if (dirck(&arg) != 0)
			goto error;

	get_used_mem();
	end_count("Checking directory", &fsck_p3_time);

	printf("Checking bitmap...\n");
	start_count(&fsck_p4_time);
	bit_map[0] = 0x07;
	arg.ef = *ef;
	arg.path = "";
	arg.tw = 0;
	if (ef->root->twins) {
		int result;

		ef->root->has_twin = 0;
		result = multi_thread(ef, "", check_bitmap);
		ef->root->has_twin = 1;
		if (result != 0)
			goto error;
	} else
		if (check_bitmap(&arg) != 0)
			goto error;

	root_start = (ef->root->start_cluster - EXFAT_FIRST_DATA_CLUSTER) / 8;
	root_rest = (ef->root->start_cluster - EXFAT_FIRST_DATA_CLUSTER) % 8;
	/*
	for (i = 0; i < root_start; i++) {
		if (ef->cmap.chunk[i] != 0xff) {
			exfat_error("system allocation missed");
			if (exfat_recovered)
				ef->cmap.chunk[i] = 0xff;
		}
	}
	*/
	exfat_debug("root_start=%d, root_rest=%d", root_start, root_rest);
	if ((ef->cmap.chunk[root_start] & (0xff << root_rest)) != (bit_map[root_start] & (0xff << root_rest)))
		cnt++;
	if (cnt) {
		/* usually will not be here */
		exfat_debug("we have differences");
		if (exfat_recovered) {
			result = exfat_mkdir(ef, LOSTDIR, EXFAT_ATTRIB_HIDDEN);
			if (result == 0 || result == -EEXIST) {
				exfat_debug("cmap[root_start]=%x, bit_map[root_start]=%x", ef->cmap.chunk[root_start], bit_map[root_start]);
				temp = (ef->cmap.chunk[root_start] & (0xff << root_rest)) ^ (bit_map[root_start] & (0xff << root_rest));
				for (j = root_rest; j < 8; j++) {
					if (temp & (1 << j)) {
						cur_cl = ef->root->start_cluster + j - root_rest;
						if (start_cluster == 0)
							start_cluster = cur_cl;
						if (!CLUSTER_INVALID(ef, pre_cl))
							set_next_cluster(ef, 0, pre_cl, cur_cl);
						pre_cl = ef->root->start_cluster + j - root_rest;
						bits++;
					}
				}
				lost_flag = 1;
			}
		}
	}
	for (i = root_start + 1; i * 8 < le32_to_cpu(ef->sb->cluster_count); i++) {
		if ((le32_to_cpu(ef->sb->cluster_count) / 8 == i) && (rest = le32_to_cpu(ef->sb->cluster_count) % 8)) {
			exfat_debug("rest of cluster count(i=%d, rest=%d)", i, rest);
			bitmap_rest = bit_map[i] & ~(0xff << rest);
			chunk_rest = ef->cmap.chunk[i] & ~(0xff << rest);
		} else {
			bitmap_rest = bit_map[i];
			chunk_rest = ef->cmap.chunk[i];
		}
		if (chunk_rest != bitmap_rest) {
			exfat_debug("Invalid allocated bits, cmap[%d]=0x%02x(expected 0x%02x)", i, chunk_rest, bitmap_rest);
			cnt++;
			if (exfat_recovered) {
				result = exfat_mkdir(ef, LOSTDIR, EXFAT_ATTRIB_HIDDEN);
				if (result == 0 || result == -EEXIST) {
					temp = chunk_rest ^ bitmap_rest;
					for (j = 0; j < 8; j++) {
						if (temp & (1 << j)) {
							cur_cl = i * 8 + j + EXFAT_FIRST_DATA_CLUSTER;
							if (start_cluster == 0)
								start_cluster = cur_cl;
							if (!CLUSTER_INVALID(ef, pre_cl))
								set_next_cluster(ef, 0, pre_cl, cur_cl);
							pre_cl = i * 8 + j + EXFAT_FIRST_DATA_CLUSTER;
							bits++;
						}
					}
					lost_flag = 1;
				} else {
					exfat_debug("flush bitmap");
					if ((le32_to_cpu(ef->sb->cluster_count) / 8 == i) && (rest = le32_to_cpu(ef->sb->cluster_count) % 8))
						ef->cmap.chunk[i] = (ef->cmap.chunk[i] & (0xff << rest)) | bitmap_rest;
					else
						ef->cmap.chunk[i] = bit_map[i];
				}
			}
		}
	}

	if (exfat_recovered)
		exfat_flush_cmap(ef);
	if (lost_flag) {
		set_next_cluster(ef, 0, cur_cl, EXFAT_CLUSTER_END);
		exfat_debug("Put files(%"PRIu64" bytes, start cluster=0x%x) in %s", bits * CLUSTER_SIZE(*ef->sb), start_cluster, LOSTDIR);
		sprintf(path, LOSTDIR"Ownerless_file_%d", lost_files++);
		result = exfat_mknod(ef, path, start_cluster, bits * CLUSTER_SIZE(*ef->sb));
		while (result == -EEXIST) {
			sprintf(path, LOSTDIR"Ownerless_file_%02d", lost_files++);
			result = exfat_mknod(ef, path, start_cluster, bits * CLUSTER_SIZE(*ef->sb));
		}
		/* Extreme circumstances, there is no free cluster on disk*/
		if (result == -ENOSPC) {
			uint32_t difference;
			struct exfat_node node_temp;
			cluster_t previous, next;
			previous = start_cluster;
			difference = bits;

			exfat_debug("free the ownerless clusters");
			memset(&node_temp, 0, sizeof(struct exfat_node));
			while (difference--) {
				if (CLUSTER_INVALID(ef, previous)) {
					exfat_error("invalid cluster 0x%x while freeing after shrink", previous);
					return -EIO;
				}
				next = exfat_next_cluster(ef, &node_temp, previous);
				set_next_cluster(ef, 0, previous, EXFAT_CLUSTER_FREE);
				free_cluster(ef, previous);
				previous = next;
			}
			exfat_flush_cmap(ef);
		}
	}

	get_used_mem();
	end_count("Checking bitmap", &fsck_p4_time);
	if (cnt) {
		if (exfat_recovered) {
			exfat_error("Found %"PRIu64"(cnt=%d) invalid allocated bits", bits, cnt);
			if (lost_flag)
				exfat_repair("Put ownerless file in %s", LOSTDIR);
			else
				exfat_repair("bitmap flushed");
		} else
			exfat_error("Found %d invalid allocated bytes", cnt);
	}

	if (exfat_info)
		exfat_print_info(ef->sb, exfat_count_free_clusters(ef));

	rc = 0;
error:
	free(bit_map);
	free(fat_map);
	fat_map = NULL;
	bit_map = NULL;

	return rc;
}

static void usage(const char* prog)
{
	fprintf(stderr, "Usage: %s [-VRI] <device>\n", prog);
	exit(2);
}

/*
 * return -1 means that found unrecovered errors, need to format.
 */
int main(int argc, char* argv[])
{
	int opt, rc = 0;
	const char* spec = NULL;
	struct exfat ef;

	printf("exfatfsck %u.%u.%u\n",
			EXFAT_VERSION_MAJOR, EXFAT_VERSION_MINOR, EXFAT_VERSION_PATCH);

	while ((opt = getopt(argc, argv, "VRI")) != -1) {
		switch (opt) {
			case 'V':
				puts("SPRD Version of exFAT FSCK");
				return 0;
			case 'R':
				puts("File system automatic recovered");
				exfat_recovered = 1;
				break;
			case 'I':
				exfat_info = 1;
				break;
			default:
				usage(argv[0]);
				break;
		}
	}
	if (argc - optind != 1)
		usage(argv[0]);
	spec = argv[optind];

	start_count(&fsck_total_time);
	if ((rc = exfat_mount(&ef, spec, exfat_recovered ? EXFAT_MODE_RW : EXFAT_MODE_RO)) != 0) {
		if (rc == -EIO)
			return 3;
		else
			return 1;
	}
	get_used_mem();
	rc = fsck(&ef);
	end_count("Total Checking time", &fsck_total_time);
	exfat_unmount(&ef);
	if (rc != 0) {
		printf("Checking abort! %s\n", exfat_recovered ? "Need to format!" : "Use '-R' to recovery it.");
		if (rc == -EIO)
			return 3;
		else
			return 1;
	}

	printf("Totally %"PRIu64" directories and %"PRIu64" files.\n",
			directories_count, files_count);

	fputs("Checking finished. ", stdout);
	if (exfat_errors != 0 && !exfat_recovered) {
		printf("ERRORS FOUND: %d(at least).Use '-R' to recovery it\n", exfat_errors);
		rc = 3;
	} else if (exfat_repair_num != 0)
		printf("Found at lease %d errors, Fixed: %d.\n", exfat_errors, exfat_repair_num);
	else
		puts("File system is clean.");
	printf("\n");

	return rc;
}
