/*
	node.c (09.10.09)
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
#include <errno.h>
#include <string.h>
#include <inttypes.h>
#include <pthread.h>
#include <atomic.h>

/* threshold for multi-thread creating*/
#define ENTRY_THRESHOLD 200

/* on-disk nodes iterator */
struct iterator
{
	cluster_t cluster;
	s64 offset;
	int contiguous;
	char* chunk;
};

struct exfat_node* exfat_get_node(struct exfat_node* node)
{
	/* if we switch to multi-threaded mode we will need atomic
	   increment here and atomic decrement in exfat_put_node() */

	if (node->shared)
		android_atomic_inc(&node->references);
	else
		node->references++;
	return node;
}

int exfat_put_node(struct exfat* ef, struct exfat_node* node)
{
	int rc = 0;

	if (node->shared)
		android_atomic_dec(&node->references);
	else
		node->references--;
	if (node->references < 0)
	{
		char buffer[EXFAT_NAME_MAX + 1];
		exfat_get_name(node, buffer, EXFAT_NAME_MAX);
		exfat_bug("reference counter of `%s' is below zero", buffer);
	}

	if (node->references == 0)
	{
		if (node->flags & EXFAT_ATTRIB_DIRTY)
			exfat_flush_node(ef, node);
		if (node->flags & EXFAT_ATTRIB_UNLINKED)
		{
			/* free all clusters and node structure itself */
			rc = exfat_truncate(ef, node, 0, true);
			free(node);
		}
		if (ef->cmap.dirty)
			exfat_flush_cmap(ef);
	}

	return rc;
}

/**
 * Cluster + offset from the beginning of the directory to absolute offset.
 */
static s64 co2o(struct exfat* ef, cluster_t cluster, s64 offset)
{
	return exfat_c2o(ef, cluster) + offset % CLUSTER_SIZE(*ef->sb);
}

static int opendir(struct exfat* ef, const struct exfat_node* dir,
		struct iterator* it)
{
	if (!(dir->flags & EXFAT_ATTRIB_DIR))
		exfat_bug("not a directory");

	it->cluster = dir->start_cluster;
	it->offset = 0;
	it->contiguous = IS_CONTIGUOUS(*dir);
	it->chunk = malloc(CLUSTER_SIZE(*ef->sb));
	if (it->chunk == NULL)
	{
		printf("Failed to allocate memory while opening dir.\n");
		return -ENOMEM;
	}

	exfat_pread(ef->dev, it->chunk, CLUSTER_SIZE(*ef->sb),
			exfat_c2o(ef, it->cluster));
	return 0;
}

static void closedir(struct iterator* it)
{
	it->cluster = 0;
	it->offset = 0;
	it->contiguous = 0;
	free(it->chunk);
	it->chunk = NULL;
}

static int fetch_next_entry(struct exfat* ef, const struct exfat_node* parent,
		struct iterator* it)
{
	/* move iterator to the next entry in the directory */
	it->offset += sizeof(struct exfat_entry);
	/* fetch the next cluster if needed */
	if ((it->offset & (CLUSTER_SIZE(*ef->sb) - 1)) == 0)
	{
		/* reached the end of directory; the caller should check this
		   condition too */
		if (it->offset >= parent->size)
			return 0;
		it->cluster = exfat_next_cluster(ef, parent, it->cluster);
		if (CLUSTER_INVALID(ef, it->cluster))
		{
			exfat_error("invalid cluster 0x%x while reading directory",
					it->cluster);
			return 1;
		}
		exfat_pread(ef->dev, it->chunk, CLUSTER_SIZE(*ef->sb),
				exfat_c2o(ef, it->cluster));
	}
	return 0;
}

static struct exfat_node* allocate_node(void)
{
	struct exfat_node* node = malloc(sizeof(struct exfat_node));
	if (node == NULL)
	{
		printf("failed to allocate node.\n");
		return NULL;
	}
	memset(node, 0, sizeof(struct exfat_node));
	return node;
}

static void init_node_meta1(struct exfat_node* node,
		const struct exfat_entry_meta1* meta1)
{
	node->flags = le16_to_cpu(meta1->attrib);
	node->mtime = exfat_exfat2unix(meta1->mdate, meta1->mtime,
			meta1->mtime_cs);
	/* there is no centiseconds field for atime */
	node->atime = exfat_exfat2unix(meta1->adate, meta1->atime, 0);
}

static void init_node_meta2(struct exfat_node* node,
		const struct exfat_entry_meta2* meta2)
{
	node->size = le64_to_cpu(meta2->size);
	node->start_cluster = le32_to_cpu(meta2->start_cluster);
	node->fptr_cluster = node->start_cluster;
	node->name_hash = meta2->name_hash;
	if (meta2->flags & EXFAT_FLAG_CONTIGUOUS)
		node->flags |= EXFAT_ATTRIB_CONTIGUOUS;
}

static const struct exfat_entry* get_entry_ptr(const struct exfat* ef,
		const struct iterator* it)
{
	return (const struct exfat_entry*)
			(it->chunk + it->offset % CLUSTER_SIZE(*ef->sb));
}

static void flush_entry_type(struct exfat *ef, struct iterator *it, uint8_t type)
{
	exfat_pwrite(ef->dev, &type, 1, co2o(ef, it->cluster, it->offset));
	exfat_repair("Entry type repaired");
}

static int dir_scan(struct exfat* ef, const struct exfat_node* parent,
		struct exfat_node** node, struct iterator* it)
{
	int rc = -EIO;
	const struct exfat_entry* entry;
	const struct exfat_entry_meta1* meta1;
	const struct exfat_entry_meta2* meta2;
	const struct exfat_entry_name* file_name;
	const struct exfat_entry_bitmap* bitmap;
	uint8_t continuations = 0;
	le16_t* namep = NULL;
	uint64_t cluster_size;
	char buf[EXFAT_NAME_MAX + 1], buf_name[EXFAT_NAME_MAX + 1];
	int result, meta1_flag = 0, meta2_flag = 0, interval = 0;
	uint8_t type;

	exfat_get_name(parent, buf, EXFAT_NAME_MAX);

	cluster_size = (uint64_t)CLUSTER_SIZE(*ef->sb);
	*node = NULL;
	for (;;)
	{
		if (it->offset >= parent->size) {
			return -ENOENT; /* end of directory */
		}

		entry = get_entry_ptr(ef, it);
		switch (entry->type)
		{
		case EXFAT_ENTRY_ZERO:
			if (continuations != 0) {
				if (meta2_flag == 0) {
					free(*node);
					*node = NULL;
				}
			}
			return -ENOENT;/* end of directory too */

		case EXFAT_ENTRY_FILE:
			meta1_flag = 1;
			if (continuations != 0)
			{
				exfat_debug("previous expected %hhu continuations before new entry", continuations);
				if (meta2_flag == 0) {
					free(*node);
					*node = NULL;
				}
				return 0;/* end of last node here */
			}
			meta1 = (const struct exfat_entry_meta1*) entry;
			continuations = meta1->continuations;
			/* 3~19 */
			if (continuations < 2 || continuations > 18) {
				exfat_debug("File-continuations invalid(%hhu)", continuations);
				do {
					if (fetch_next_entry(ef, parent, it) != 0) {
						printf("fetch next entry failed.\n");
						goto error;
					}
					entry = get_entry_ptr(ef, it);
				} while ((entry->type != EXFAT_ENTRY_FILE) && (continuations-- > 0));
				return 0;/* node invalid, skip*/
			}
			if (*node == NULL)
				*node = allocate_node();
			if (*node == NULL) {
				printf("failed to allocate node.\n");
				return -ENOMEM;
			}
			/* new node has zero reference counter */
			(*node)->entry_cluster = it->cluster;
			(*node)->entry_offset = it->offset;
			init_node_meta1(*node, meta1);
			namep = (*node)->name;
			interval++;
			break;

		case EXFAT_ENTRY_FILE_INFO:
			if (!meta1_flag || interval == 0)
				break;

			interval = 0;
			meta2_flag = 1;
			meta2 = (const struct exfat_entry_meta2*) entry;
			/*we can not return here,otherwise we can not checking the chain*/
			if (meta2->flags & ~(EXFAT_FLAG_ALWAYS1 | EXFAT_FLAG_CONTIGUOUS)) {
				--continuations;
				break;
			}
			if (le32_to_cpu(meta2->start_cluster) == 0) {
				--continuations;
				break;
			}
			if (parent->start_cluster == le32_to_cpu(meta2->start_cluster)) {
				free(*node);
				*node = NULL;
				while(continuations-- > 0) {
					if (fetch_next_entry(ef, parent, it) != 0)
						goto error;
				}
				return 0;
			}
			if (CLUSTER_INVALID(ef, le32_to_cpu(meta2->start_cluster))) {
				free(*node);
				*node = NULL;
				while(continuations-- > 0) {
					if (fetch_next_entry(ef, parent, it) != 0)
						goto error;
				}
				return 0;
			}

			init_node_meta2(*node, meta2);
			if (!(meta2->flags & EXFAT_FLAG_CONTIGUOUS)) {
			cluster_t cl;
			for (cl = (*node)->start_cluster; cl != EXFAT_CLUSTER_END; cl = exfat_next_cluster(ef, (*node), cl)) {
				if (CLUSTER_INVALID(ef, cl)) {
					free(*node);
					*node = NULL;
					do {
						type = EXFAT_ENTRY_FILE & ~EXFAT_ENTRY_VALID;
						if (exfat_recovered)
							exfat_pwrite(ef->dev, &type, 1, co2o(ef, it->cluster, it->offset));
						if (fetch_next_entry(ef, parent, it) != 0) {
							printf("fetch next entry failed.\n");
							goto error;
						}
						entry = get_entry_ptr(ef, it);
					} while ((entry->type != EXFAT_ENTRY_FILE) && (continuations-- > 0));
					return 0;
				}
			}
                        }
			--continuations;
			break;

		case EXFAT_ENTRY_FILE_NAME:
			if (!meta1_flag || !meta2_flag)
				break;

			interval = 0;
			file_name = (const struct exfat_entry_name*) entry;
			memcpy(namep, file_name->name, EXFAT_ENAME_MAX * sizeof(le16_t));
			namep += EXFAT_ENAME_MAX;
			if (--continuations == 0) {
				if (fetch_next_entry(ef, parent, it) != 0) {
					printf("FILE_NAME-failed to fetch next entry.\n");
					goto error;
				}
				exfat_get_name(*node, buf_name, EXFAT_NAME_MAX);
				return 0; /* entry completed */
			}
			break;

		default:
			interval = 0;
			if (parent->child == NULL && it->cluster != ef->root->start_cluster) {
				if (entry->type == 0x05 || entry->type == 0x40 || entry->type == 0x41)
					break;
				exfat_debug("found invalid direcotry content.\n");
				return -ENOENT;
			}
			if (entry->type & EXFAT_ENTRY_VALID)
				exfat_debug("unknown entry type 0x%hhx at (%"PRIu64").\n", entry->type, co2o(ef, it->cluster, it->offset));
			break;
		}

		if (fetch_next_entry(ef, parent, it) != 0) {
			printf("failed to fetch next entry.\n");
			goto error;
		}
	}

error:
	free(*node);
	*node = NULL;
	return rc;
}

static void next_entry(struct exfat* ef, const struct exfat_node* parent,
		cluster_t* cluster, s64* offset);
/*
 * Reads one entry in directory at position pointed by iterator and fills
 * node structure.
 */
static int readdir(struct exfat* ef, const struct exfat_node* parent,
		struct exfat_node** node, struct iterator* it)
{
	int rc = -EIO, content_flag = 0;
	const struct exfat_entry* entry;
	const struct exfat_entry_meta1* meta1;
	struct exfat_entry_meta2* meta2;
	const struct exfat_entry_name* file_name;
	const struct exfat_entry_upcase* upcase;
	const struct exfat_entry_bitmap* bitmap;
	const struct exfat_entry_label* label;
	uint8_t continuations = 0;
	le16_t* namep = NULL;
	uint16_t reference_checksum = 0;
	uint16_t actual_checksum = 0;
	uint64_t real_size = 0;
	int meta1_flag = 0, meta2_flag = 0, interval = 0, cnt = 0;
	uint8_t type;
	s64 offset;
	cluster_t cluster;
	uint32_t upcase_checksum;
	le32_t checksum;

	*node = NULL;
	for (;;)
	{
		if (it->offset >= parent->size)
		{
			if (continuations != 0)
			{
				exfat_error("expected %hhu continuations at the end of parent dir", continuations);
				if (exfat_recovered) {
					while (cnt-- > 0) {
						type = EXFAT_ENTRY_FILE & ~EXFAT_ENTRY_VALID;
						exfat_pwrite(ef->dev, &type, 1, co2o(ef, cluster, offset));
						offset += sizeof(struct exfat_entry);
						if ((offset & (CLUSTER_SIZE(*ef->sb) - 1)) == 0)
							cluster = exfat_next_cluster(ef, parent, cluster);
					}
					exfat_repair("previous invalid entry repaired");
				} else
					goto error;
			}
			return -ENOENT; /* that's OK, means end of directory */
		}

		entry = get_entry_ptr(ef, it);
		switch (entry->type)
		{
		case EXFAT_ENTRY_ZERO:
			if (continuations != 0)
			{
				exfat_error("previous expected %hhu continuations before new entry", continuations);
				if (exfat_recovered) {
					while (cnt-- > 0) {
						type = EXFAT_ENTRY_FILE & ~EXFAT_ENTRY_VALID;
						exfat_pwrite(ef->dev, &type, 1, co2o(ef, cluster, offset));
						offset += sizeof(struct exfat_entry);
						if ((offset & (CLUSTER_SIZE(*ef->sb) - 1)) == 0)
							cluster = exfat_next_cluster(ef, parent, cluster);
					}
					exfat_repair("previous invalid entry repaired");
				} else
					goto error;
			}
			return -ENOENT;/* end of directory too */

		case EXFAT_ENTRY_FILE:
			meta1_flag = 1;
			if (continuations != 0)
			{
				/* may be previous only have one FILE ENTRY or incomplete FILE_NAME_ENTRY,
				   if we get here, thar means the previous entry need to be clear*/
				exfat_error("previous expected %hhu continuations before new entry", continuations);
				if (exfat_recovered) {
					while (cnt-- > 0) {
						type = EXFAT_ENTRY_FILE & ~EXFAT_ENTRY_VALID;
						exfat_pwrite(ef->dev, &type, 1, co2o(ef, cluster, offset));
						offset += sizeof(struct exfat_entry);
						if ((offset & (CLUSTER_SIZE(*ef->sb) - 1)) == 0)
							cluster = exfat_next_cluster(ef, parent, cluster);
					}
					exfat_repair("previous invalid entry repaired");
				} else
					goto error;
			}
			meta1 = (const struct exfat_entry_meta1*) entry;
			continuations = meta1->continuations;
			/* each file entry must have at least 2 continuations:
			   info and name, we check it before*/
			if (continuations < 2 || continuations > 18) {
				exfat_error("invalid continuations (%hhu) at (%"PRIu64")", continuations, co2o(ef, it->cluster, it->offset));
				if (exfat_recovered) {
					do {
						type = EXFAT_ENTRY_FILE & ~EXFAT_ENTRY_VALID;
						exfat_pwrite(ef->dev, &type, 1, co2o(ef, it->cluster, it->offset));
						if (fetch_next_entry(ef, parent, it) != 0) {
							printf("fetch next entry failed.\n");
							goto error;
						}
						entry = get_entry_ptr(ef, it);
					} while ((entry->type != EXFAT_ENTRY_FILE) && (continuations-- > 0));
					exfat_repair("invalid continuations repaired");
					return 0;/* the whole entries is invalid */
				}
				goto error;
			}

			/*store offset, we will use it while we did not have any other valid entry*/
			offset = it->offset;
			cluster = it->cluster;
			reference_checksum = le16_to_cpu(meta1->checksum);
			actual_checksum = exfat_start_checksum(meta1);
			/* if node is non-NULL, we can use it*/
			if (*node == NULL)
				*node = allocate_node();
			if (*node == NULL)
			{
				printf("failed to allocate node.\n");
				rc = -ENOMEM;
				goto error;
			}
			/* new node has zero reference counter */
			(*node)->entry_cluster = it->cluster;
			(*node)->entry_offset = it->offset;
			init_node_meta1(*node, meta1);
			namep = (*node)->name;
			interval++;
			cnt++;
			break;

		case EXFAT_ENTRY_FILE_INFO:
			if (!meta1_flag || interval == 0) {
				type = EXFAT_ENTRY_FILE_INFO & ~EXFAT_ENTRY_VALID;
				exfat_error("FILE ENTRY not exist");
				if (exfat_recovered)
					flush_entry_type(ef, it, type);
				break;
			}
			if (continuations < 2)
			{
				printf("unexpected continuation (%hhu).\n",
						continuations);
				goto error;
			}
			interval = 0;
			meta2_flag = 1;
			meta2 = (const struct exfat_entry_meta2*) entry;
			if (parent->start_cluster == le32_to_cpu(meta2->start_cluster)) {
				exfat_error("found cyclic entry");
				printf("Unrecovered errors!Checking stopped.\n");
				exit(3);
			}
			init_node_meta2(*node, meta2);
			actual_checksum = exfat_add_checksum(entry, actual_checksum);
			real_size = le64_to_cpu(meta2->real_size);
			if (meta2->flags & ~(EXFAT_FLAG_ALWAYS1 | EXFAT_FLAG_CONTIGUOUS))
			{
				exfat_error("unknown flags in meta2 (0x%hhx) at (%"PRIu64")", meta2->flags, co2o(ef, it->cluster, it->offset));
				if (exfat_recovered) {
					(*node)->invalid_flag = 1;
				} else
					goto error;
			}
			/* empty files must be marked as non-contiguous */
			if ((*node)->size == 0 && (meta2->flags & EXFAT_FLAG_CONTIGUOUS) && !((*node)->invalid_flag))
			{
				exfat_error("empty file marked as contiguous (0x%hhx)",
						meta2->flags);
				if (exfat_recovered) {
					(*node)->invalid_flag = 1;
				} else
					goto error;
			}
			/* directories must be aligned on at cluster boundary*/
			if (((*node)->flags & EXFAT_ATTRIB_DIR) && ((*node)->size == 0 ||
				(*node)->size % CLUSTER_SIZE(*ef->sb) != 0) && !((*node)->invalid_flag))
			{
				exfat_error("directory size(%"PRIu64") is not aligned with cluster size",
						(*node)->size);
				if (exfat_recovered) {
					(*node)->invalid_flag = 1;
				} else
					goto error;
			}
			/* zero size of fragmented file, we checked contiguous before */
			if (!((*node)->flags & EXFAT_ATTRIB_DIR) && (*node)->size == 0 && (*node)->start_cluster != 0 && !((*node)->invalid_flag)) {
				cluster_t cl, clusters = 0;
				for (cl = (*node)->start_cluster; cl != EXFAT_CLUSTER_END; cl = exfat_next_cluster(ef, (*node), cl)) {
					clusters++;
					if (CLUSTER_INVALID(ef, cl)) {
						exfat_debug("invalid cluster number of zero size file, cached skipped");
						free(*node);
						*node = NULL;
						do {
							type = EXFAT_ENTRY_FILE & ~EXFAT_ENTRY_VALID;
							if (exfat_recovered)
								exfat_pwrite(ef->dev, &type, 1, co2o(ef, it->cluster, it->offset));
							if (fetch_next_entry(ef, parent, it) != 0) {
								printf("fetch next entry failed.\n");
								goto error;
							}
							entry = get_entry_ptr(ef, it);
						} while ((entry->type != EXFAT_ENTRY_FILE) && (continuations-- > 0));

						return 0;
					}
				}
				exfat_error("zero node size of non-empty file");
				if (exfat_recovered) {
					(*node)->size = clusters * CLUSTER_SIZE(*ef->sb);
					(*node)->flags |= EXFAT_ATTRIB_DIRTY;
					exfat_repair("size of non-empty file repaired");
				} else
					goto error;
			}
			/* empty files start cluster must be zero */
			if ((*node)->size == 0 && (*node)->start_cluster != 0 && !((*node)->invalid_flag))
			{
				exfat_error("empty file start cluster is non-zero");
				if (exfat_recovered) {
					(*node)->invalid_flag = 1;
				} else
					goto error;
			}
			if (CLUSTER_INVALID(ef, (*node)->start_cluster) && !((*node)->invalid_flag))
			{
				if (!((*node)->flags & EXFAT_ATTRIB_DIR) && (*node)->size == 0 && (*node)->start_cluster == 0)
					exfat_debug("empty file");
				else {
					exfat_error("start_cluster invalid(%d)", (*node)->start_cluster);
					if (exfat_recovered) {
						(*node)->invalid_flag = 1;
					} else
						goto error;
				}
			}
			/*
				   There are two fields that contain file size. Maybe they
				   plan to add compression support in the future and one of
				   those fields is visible (uncompressed) size and the other
				   is real (compressed) size. Anyway, currently it looks like
				   exFAT does not support compression and both fields must be
				   equal.

				   There is an exception though: pagefile.sys (its real_size
				   is always 0).
			*/
			if (real_size != (*node)->size && !((*node)->invalid_flag))
			{
				exfat_error("real size does not equal to size "
						"(%"PRIu64" != %"PRIu64")",
						real_size, (*node)->size);
				if (exfat_recovered) {
					(*node)->invalid_flag = 1;
				} else
					goto error;
			}
			--continuations;
			cnt++;
			break;

		case EXFAT_ENTRY_FILE_NAME:
			if (!meta1_flag || !meta2_flag) {
				type = EXFAT_ENTRY_FILE_NAME & ~EXFAT_ENTRY_VALID;
				exfat_error("FILE/FILE_INFO ENTRY not exixt");
				if (exfat_recovered)
					flush_entry_type(ef, it, type);
				break;
			}
			if (continuations == 0)
			{
				printf("unexpected continuation.\n");
				goto error;
			}
			interval = 0;
			file_name = (const struct exfat_entry_name*) entry;
			actual_checksum = exfat_add_checksum(entry, actual_checksum);

			memcpy(namep, file_name->name, EXFAT_ENAME_MAX * sizeof(le16_t));
			namep += EXFAT_ENAME_MAX;
			cnt++;
			if (--continuations == 0)
			{
				/* if the node has errors before, we do not compare the checksum here*/
				if (actual_checksum != reference_checksum && !((*node)->invalid_flag))
				{
					char buffer[EXFAT_NAME_MAX + 1];

					exfat_get_name(*node, buffer, EXFAT_NAME_MAX);
					exfat_error("entry `%s' has invalid checksum (0x%hx != 0x%hx)",
							buffer, actual_checksum, reference_checksum);
					if (exfat_recovered) {
						(*node)->invalid_flag = 1;
					} else
						goto error;
				}
				if (fetch_next_entry(ef, parent, it) != 0) {
					goto error;
				}
				return 0; /* entry completed */
			}
			break;

		case EXFAT_ENTRY_UPCASE:
			if (it->cluster != ef->root->start_cluster) {
				exfat_error("found root-only entry in non-root dir(%x)", entry->type);
				if (exfat_recovered) {
					type = (entry->type) & ~EXFAT_ENTRY_VALID;
					flush_entry_type(ef, it, type);
				}
				break;
			}
			if (ef->upcase != NULL)
				break;
			interval = 0;
			upcase = (const struct exfat_entry_upcase*) entry;
			if (CLUSTER_INVALID(ef, le32_to_cpu(upcase->start_cluster)))
			{
				exfat_error("invalid cluster 0x%x in upcase table",
						le32_to_cpu(upcase->start_cluster));
				goto error;
			}
			if (le64_to_cpu(upcase->size) == 0 ||
				le64_to_cpu(upcase->size) > 0xffff * sizeof(uint16_t) ||
				le64_to_cpu(upcase->size) % sizeof(uint16_t) != 0)
			{
				exfat_error("bad upcase table size (%"PRIu64" bytes)",
						le64_to_cpu(upcase->size));
				goto error;
			}
			ef->upcase = malloc(le64_to_cpu(upcase->size));
			if (ef->upcase == NULL)
			{
				printf("failed to allocate upcase table (%"PRIu64" bytes)\n",
						le64_to_cpu(upcase->size));
				rc = -ENOMEM;
				goto error;
			}
			ef->upcase_chars = le64_to_cpu(upcase->size) / sizeof(le16_t);

			exfat_pread(ef->dev, ef->upcase, le64_to_cpu(upcase->size),
					exfat_c2o(ef, le32_to_cpu(upcase->start_cluster)));
			upcase_checksum = exfat_upcase_checksum(ef->upcase, le64_to_cpu(upcase->size));
			if (le32_to_cpu(upcase->checksum) != upcase_checksum) {
				exfat_error("Upcase table checksum(0x%X) does not match stored checksum(0x%X)", upcase_checksum, le32_to_cpu(upcase->checksum));
				/* upcase table is read only, checksum must be wrong*/
				if (exfat_recovered) {
					checksum = cpu_to_le32(upcase_checksum);
					exfat_pwrite(ef->dev, &checksum, 4, co2o(ef, it->cluster, it->offset) + 4);
					exfat_repair("update upcase checksum");
				} else
					goto error;
			}
			break;

		case EXFAT_ENTRY_BITMAP:
			if (it->cluster != ef->root->start_cluster) {
				exfat_error("found root-only entry in non-root dir(%x)", entry->type);
				if (exfat_recovered) {
					type = (entry->type) & ~EXFAT_ENTRY_VALID;
					flush_entry_type(ef, it, type);
				}
				break;
			}
			if (ef->cmap.chunk != NULL)
				break;
			interval = 0;
			bitmap = (const struct exfat_entry_bitmap*) entry;
			ef->cmap.start_cluster = le32_to_cpu(bitmap->start_cluster);
			if (CLUSTER_INVALID(ef, ef->cmap.start_cluster))
			{
				exfat_error("invalid cluster 0x%x in clusters bitmap",
						ef->cmap.start_cluster);
				goto error;
			}
			ef->cmap.size = le32_to_cpu(ef->sb->cluster_count);
			exfat_debug("cluster_count=0x%x", le32_to_cpu(ef->sb->cluster_count));
			if (le64_to_cpu(bitmap->size) < (ef->cmap.size + 7) / 8)
			{
				exfat_error("invalid clusters bitmap size: %"PRIu64" (expected at least %u)",
						le64_to_cpu(bitmap->size), (ef->cmap.size + 7) / 8);
				goto error;
			}
			/* FIXME bitmap can be rather big, up to 512 MB */
			ef->cmap.chunk_size = ef->cmap.size;
			ef->cmap.chunk = malloc(le64_to_cpu(bitmap->size));
			if (ef->cmap.chunk == NULL)
			{
				printf("failed to allocate clusters bitmap chunk "
						"(%"PRIu64" bytes)\n", le64_to_cpu(bitmap->size));
				rc = -ENOMEM;
				goto error;
			}

			exfat_debug("Read bitmap form device");
			exfat_pread(ef->dev, ef->cmap.chunk, (size_t)le64_to_cpu(bitmap->size),
					exfat_c2o(ef, ef->cmap.start_cluster));
			break;

		case EXFAT_ENTRY_LABEL:
			if (it->cluster != ef->root->start_cluster) {
				exfat_error("found root-only entry in non-root dir(%x)", entry->type);
				if (exfat_recovered) {
					type = (entry->type) & ~EXFAT_ENTRY_VALID;
					flush_entry_type(ef, it, type);
				}
				break;
			}
			interval = 0;
			label = (const struct exfat_entry_label*) entry;
			if (label->length > EXFAT_ENAME_MAX)
			{
				exfat_error("too long label (%hhu chars)", label->length);
				goto error;
			}
			if (utf16_to_utf8(ef->label, label->name,
						sizeof(ef->label), EXFAT_ENAME_MAX) != 0)
				goto error;
			break;

		default:
			interval = 0;
			/* we detect the beginning of dir entry.(fixed the situation of non-zero datas located in empty entry)*/
			if (parent->child == NULL && it->cluster != ef->root->start_cluster) {
				if (entry->type == 0x05 || entry->type == 0x40 || entry->type == 0x41)
					break;
				while (it->offset < parent->size) {
					if (entry->type & EXFAT_ENTRY_VALID) {
						content_flag++;
						if (exfat_recovered) {
							type = (entry->type) & ~EXFAT_ENTRY_VALID;
							exfat_pwrite(ef->dev, &type, 1, co2o(ef, it->cluster, it->offset));
						}
					}
					if (fetch_next_entry(ef, parent, it) != 0) {
						printf("fetch next entry failed.\n");
						goto error;
					}
					entry = get_entry_ptr(ef, it);
				}
				if (content_flag) {
					exfat_error("Invalid direcotry content(%d)", content_flag);
					if (exfat_recovered)
						exfat_repair("Invalid directory content repaired");
				}
				return -ENOENT;
			}

			/* unknow entry type in normal entry*/
			if (entry->type & EXFAT_ENTRY_VALID) {
				exfat_error("unknown entry type 0x%hhx at (%"PRIu64")", entry->type, co2o(ef, it->cluster, it->offset));
				type = (entry->type) & ~EXFAT_ENTRY_VALID;

				if (exfat_recovered)
					flush_entry_type(ef, it, type);
				else
					goto error;
			}
			break;
		}

		if (fetch_next_entry(ef, parent, it) != 0) {
			goto error;
		}
	}
	/* we never reach here */

error:
	free(*node);
	*node = NULL;
	return rc;
}

static int dir_count(struct exfat* ef, const struct exfat_node* parent, struct iterator* it, int *cnt)
{
	int rc = -EIO;
	const struct exfat_entry* entry;
	const struct exfat_entry_meta1* meta1;
	const struct exfat_entry_meta2* meta2;
	uint8_t continuations = 0;
	int result, meta1_flag = 0, meta2_flag = 0, interval = 0;


	*cnt = 0;
	for (;;)
	{
		if (it->offset >= parent->size) {
			return -ENOENT;
		}

		entry = get_entry_ptr(ef, it);
		switch (entry->type)
		{
		case EXFAT_ENTRY_ZERO:
			if (continuations != 0) {
				if (meta2_flag == 0)
					*cnt = 0;
			}
			return -ENOENT;

		case EXFAT_ENTRY_FILE:
			meta1_flag = 1;
			if (continuations != 0) {
				if (meta2_flag == 0)
					*cnt = 0;
				return 0;
			}
			meta1 = (const struct exfat_entry_meta1*) entry;
			continuations = meta1->continuations;
			if (continuations < 2 || continuations > 18) {
				do {
					if (fetch_next_entry(ef, parent, it) != 0)
						goto error;
					entry = get_entry_ptr(ef, it);
				} while ((entry->type != EXFAT_ENTRY_FILE) && (continuations-- > 0));
				return 0;
			}
			*cnt = 1;
			interval++;
			break;

		case EXFAT_ENTRY_FILE_INFO:
			if (!meta1_flag || interval == 0)
				break;

			interval = 0;
			meta2_flag = 1;
			meta2 = (const struct exfat_entry_meta2*) entry;
			if (meta2->flags & ~(EXFAT_FLAG_ALWAYS1 | EXFAT_FLAG_CONTIGUOUS)) {
				--continuations;
				break;
			}
			if (le32_to_cpu(meta2->start_cluster) == 0) {
				--continuations;
				break;
			}
			if (parent->start_cluster == le32_to_cpu(meta2->start_cluster)) {
				*cnt = 0;
				return 0;
			}
			if (CLUSTER_INVALID(ef, le32_to_cpu(meta2->start_cluster))) {
				*cnt = 0;
				return 0;
			}
			--continuations;
			break;

		case EXFAT_ENTRY_FILE_NAME:
			if (!meta1_flag || !meta2_flag)
				break;

			interval = 0;
			if (--continuations == 0) {
				if (fetch_next_entry(ef, parent, it) != 0)
					goto error;
				return 0;
			}
			break;

		default:
			interval = 0;
			if (parent->child == NULL && it->cluster != ef->root->start_cluster) {
				if (entry->type == 0x05 || entry->type == 0x40 || entry->type == 0x41)
					break;
				return -ENOENT;
			}
			break;
		}

		if (fetch_next_entry(ef, parent, it) != 0)
			goto error;
	}

error:
	*cnt = 0;
	return rc;
}

int exfat_cache_directory(struct exfat* ef, struct exfat_node* dir, int ck)
{
	struct iterator it, ite;
	int rc, odd = 0, cnt, entries = 0;
	struct exfat_node* node, *cur1 = NULL, *cur2 = NULL;
	struct exfat_node* current = NULL;
	uint64_t cl_size;

	if (dir->flags & EXFAT_ATTRIB_CACHED)
		return 0; /* already cached */
	rc = opendir(ef, dir, &it);
	if (rc == -ENOMEM) {
		printf("opendir failed.\n");
		return rc;
	}

	/* count entries will cost 100ms at the first time*/
	ite = it;
	ite.chunk = malloc(CLUSTER_SIZE(*ef->sb));
	memcpy(ite.chunk, it.chunk, CLUSTER_SIZE(*ef->sb));
	while ((rc = dir_count(ef, dir, &ite, &cnt)) == 0) {
		if (cnt == 1)
			entries++;
	}
	closedir(&ite);
	if (rc != -ENOENT)
		exfat_debug("count entry failed");
	if (threads == 0 && entries > ENTRY_THRESHOLD) {
		struct exfat_node *temp = dir;

		dir->has_twin = 1;
		while (temp) {
			temp->shared = 1;
			if (temp->prev)
				temp = temp->prev;
			else
				temp = temp->parent;
		}
		exfat_debug("lots of subdirs, begin to detach into two branches");
	}
	if (ck) {
		while ((rc = dir_scan(ef, dir, &node, &it)) == 0) {
			if (node != NULL) {
				odd++;
				node->parent = dir;
				if (current != NULL) {
					current->next = node;
					node->prev = current;
				} else if (dir->has_twin) {
					if (odd % 2)
						dir->child = node;
					else
						dir->twins = node;
				} else
					dir->child = node;

				if (dir->has_twin) {
					if (odd % 2) {
						cur1 = node;
						current = cur2;
					} else {
						cur2 = node;
						current = cur1;
					}
				} else
					current = node;
			}
		}
	} else {
		while ((rc = readdir(ef, dir, &node, &it)) == 0)
		{
			if (node != NULL) {
				odd++;
				node->parent = dir;
				if (current != NULL) {
					current->next = node;
					node->prev = current;
				} else if (dir->has_twin) {
					if (odd % 2)
						dir->child = node;
					else
						dir->twins = node;
				} else
					dir->child = node;

				if (dir->has_twin) {
					if (odd % 2) {
						cur1 = node;
						current = cur2;
					} else {
						cur2 = node;
						current = cur1;
					}
				} else
					current = node;
			}
		}
	}
	closedir(&it);

	if (rc != -ENOENT)
	{
		exfat_debug("Unrecovered errors found, interrupted before entry end");
		/* rollback */
		for (current = dir->child; current; current = node)
		{
			node = current->next;
			free(current);
		}
		dir->child = NULL;
		return rc;
	}

	dir->flags |= EXFAT_ATTRIB_CACHED;
	return 0;
}

static void tree_attach(struct exfat_node* dir, struct exfat_node* node)
{
	node->parent = dir;
	if (dir->child)
	{
		dir->child->prev = node;
		node->next = dir->child;
	}
	dir->child = node;
}

static void tree_detach(struct exfat_node* node, int tw)
{
	if (node->prev)
		node->prev->next = node->next;
	else { /* this is the first node in the list */
		if (!tw)
			node->parent->child = node->next;
		else
			node->parent->twins = node->next;
	}
	if (node->next)
		node->next->prev = node->prev;
	node->parent = NULL;
	node->prev = NULL;
/* we can not set 'node->next' to NULL,
 * because we need to jump to the next
 * node in exfat_readdir()
	node->next = NULL;
*/
}

static void reset_cache(struct exfat* ef, struct exfat_node* node)
{
	while (node->child) {
		struct exfat_node *p = node->child;
		reset_cache(ef, p);
		tree_detach(p, 0);
		free(p);
	}
	while (node->twins) {
		struct exfat_node *p = node->twins;
		reset_cache(ef, p);
		tree_detach(p, 1);
		free(p);
	}

	node->flags &= ~EXFAT_ATTRIB_CACHED;
	if (node->references != 0)
	{
		char buffer[EXFAT_NAME_MAX + 1];
		exfat_get_name(node, buffer, EXFAT_NAME_MAX);
		exfat_warn("non-zero reference counter (%d) for `%s'",
				node->references, buffer);
	}
	while (node->references)
		exfat_put_node(ef, node);
}

void exfat_reset_cache(struct exfat* ef)
{
	reset_cache(ef, ef->root);
}

static void next_entry(struct exfat* ef, const struct exfat_node* parent,
		cluster_t* cluster, s64* offset)
{
	*offset += sizeof(struct exfat_entry);
	if (*offset % CLUSTER_SIZE(*ef->sb) == 0)
		/* next cluster cannot be invalid */
		*cluster = exfat_next_cluster(ef, parent, *cluster);
}

void exfat_flush_node(struct exfat* ef, struct exfat_node* node)
{
	cluster_t cluster;
	s64 offset;
	s64 meta1_offset, meta2_offset;
	struct exfat_entry_meta1 meta1;
	struct exfat_entry_meta2 meta2;

	if (ef->ro)
		exfat_bug("unable to flush node to read-only FS");

	if (node->parent == NULL)
		return; /* do not flush unlinked node */

	cluster = node->entry_cluster;
	offset = node->entry_offset;
	meta1_offset = co2o(ef, cluster, offset);
	next_entry(ef, node->parent, &cluster, &offset);
	meta2_offset = co2o(ef, cluster, offset);

	exfat_pread(ef->dev, &meta1, sizeof(meta1), meta1_offset);
	if (meta1.type != EXFAT_ENTRY_FILE)
		exfat_bug("invalid type of meta1: 0x%hhx", meta1.type);
	meta1.attrib = cpu_to_le16(node->flags);
	exfat_unix2exfat(node->mtime, &meta1.mdate, &meta1.mtime, &meta1.mtime_cs);
	exfat_unix2exfat(node->atime, &meta1.adate, &meta1.atime, NULL);

	exfat_pread(ef->dev, &meta2, sizeof(meta2), meta2_offset);
	if (meta2.type != EXFAT_ENTRY_FILE_INFO)
		exfat_bug("invalid type of meta2: 0x%hhx", meta2.type);
	meta2.size = meta2.real_size = cpu_to_le64(node->size);
	meta2.start_cluster = cpu_to_le32(node->start_cluster);
	meta2.flags = EXFAT_FLAG_ALWAYS1;
	/* empty files must not be marked as contiguous */
	if (node->size != 0 && IS_CONTIGUOUS(*node))
		meta2.flags |= EXFAT_FLAG_CONTIGUOUS;
	/* name hash remains unchanged, no need to recalculate it */
	meta2.name_hash = node->name_hash;

	meta1.checksum = exfat_calc_checksum(&meta1, &meta2, node->name);

	exfat_pwrite(ef->dev, &meta1, sizeof(meta1), meta1_offset);
	exfat_pwrite(ef->dev, &meta2, sizeof(meta2), meta2_offset);

	node->flags &= ~EXFAT_ATTRIB_DIRTY;
}

void erase_entry(struct exfat* ef, struct exfat_node* node)
{
	cluster_t cluster = node->entry_cluster;
	s64 offset = node->entry_offset;
	int name_entries = DIV_ROUND_UP(utf16_length(node->name), EXFAT_ENAME_MAX);
	uint8_t entry_type;

	entry_type = EXFAT_ENTRY_FILE & ~EXFAT_ENTRY_VALID;
	exfat_pwrite(ef->dev, &entry_type, 1, co2o(ef, cluster, offset));

	next_entry(ef, node->parent, &cluster, &offset);
	entry_type = EXFAT_ENTRY_FILE_INFO & ~EXFAT_ENTRY_VALID;
	exfat_pwrite(ef->dev, &entry_type, 1, co2o(ef, cluster, offset));

	while (name_entries--)
	{
		next_entry(ef, node->parent, &cluster, &offset);
		entry_type = EXFAT_ENTRY_FILE_NAME & ~EXFAT_ENTRY_VALID;
		exfat_pwrite(ef->dev, &entry_type, 1, co2o(ef, cluster, offset));
	}
}

static int shrink_directory(struct exfat* ef, struct exfat_node* dir,
		s64 deleted_offset)
{
	const struct exfat_node* node;
	const struct exfat_node* last_node;
	uint64_t entries = 0;
	uint64_t new_size;
	int rc;

	if (!(dir->flags & EXFAT_ATTRIB_DIR))
		exfat_bug("attempted to shrink a file");
	if (!(dir->flags & EXFAT_ATTRIB_CACHED))
		exfat_bug("attempted to shrink uncached directory");

	for (last_node = node = dir->child; node; node = node->next)
	{
		if (deleted_offset < node->entry_offset)
		{
			/* there are other entries after the removed one, no way to shrink
			   this directory */
			return 0;
		}
		if (last_node->entry_offset < node->entry_offset)
			last_node = node;
	}

	if (last_node)
	{
		/* offset of the last entry */
		entries += last_node->entry_offset / sizeof(struct exfat_entry);
		/* two subentries with meta info */
		entries += 2;
		/* subentries with file name */
		entries += DIV_ROUND_UP(utf16_length(last_node->name),
				EXFAT_ENAME_MAX);
	}

	new_size = DIV_ROUND_UP(entries * sizeof(struct exfat_entry),
				 CLUSTER_SIZE(*ef->sb)) * CLUSTER_SIZE(*ef->sb);
	if (new_size == 0) /* directory always has at least 1 cluster */
		new_size = CLUSTER_SIZE(*ef->sb);
	if (new_size == dir->size)
		return 0;
	rc = exfat_truncate(ef, dir, new_size, true);
	if (rc != 0)
		return rc;
	return 0;
}

int delete(struct exfat* ef, struct exfat_node* node)
{
	struct exfat_node* parent = node->parent;
	s64 deleted_offset = node->entry_offset;
	int rc = 0;

	exfat_get_node(parent);
	erase_entry(ef, node);
	exfat_update_mtime(parent);
	if (node->prev == NULL) { /*first child*/
		if (node == node->parent->child)
			tree_detach(node, 0);
		else if (node == node->parent->twins)
			tree_detach(node, 1);
	} else
		tree_detach(node, 0);
	rc = shrink_directory(ef, parent, deleted_offset);
	exfat_put_node(ef, parent);
	/* file clusters will be freed when node reference counter becomes 0 */
	node->flags |= EXFAT_ATTRIB_UNLINKED;
	return rc;
}

int exfat_unlink(struct exfat* ef, struct exfat_node* node)
{
	if (node->flags & EXFAT_ATTRIB_DIR)
		return -EISDIR;
	return delete(ef, node);
}

int exfat_rmdir(struct exfat* ef, struct exfat_node* node)
{
	if (!(node->flags & EXFAT_ATTRIB_DIR))
		return -ENOTDIR;
	/* check that directory is empty */
	exfat_cache_directory(ef, node, 0);
	if (node->child)
		return -ENOTEMPTY;
	return delete(ef, node);
}

static int grow_directory(struct exfat* ef, struct exfat_node* dir,
		uint64_t asize, uint32_t difference)
{
	return exfat_truncate(ef, dir,
			DIV_ROUND_UP(asize + difference, CLUSTER_SIZE(*ef->sb))
				* CLUSTER_SIZE(*ef->sb), true);
}

static int find_slot(struct exfat* ef, struct exfat_node* dir,
		cluster_t* cluster, s64* offset, int subentries)
{
	struct iterator it;
	int rc;
	const struct exfat_entry* entry;
	int contiguous = 0;

	rc = opendir(ef, dir, &it);
	if (rc == -ENOMEM)
		return rc;
	for (;;)
	{
		if (contiguous == 0)
		{
			*cluster = it.cluster;
			*offset = it.offset;
		}
		entry = get_entry_ptr(ef, &it);
		if (entry->type & EXFAT_ENTRY_VALID)
			contiguous = 0;
		else
			contiguous++;
		if (contiguous == subentries)
			break;	/* suitable slot is found */
		if (it.offset + sizeof(struct exfat_entry) >= dir->size)
		{
			rc = grow_directory(ef, dir, dir->size,
					(subentries - contiguous) * sizeof(struct exfat_entry));
			if (rc != 0)
			{
				closedir(&it);
				return rc;
			}
		}
		if (fetch_next_entry(ef, dir, &it) != 0)
		{
			closedir(&it);
			return -EIO;
		}
	}
	closedir(&it);
	return 0;
}

static int write_entry(struct exfat* ef, struct exfat_node* dir,
		const le16_t* name, cluster_t cluster, s64 offset, uint16_t attrib,
		cluster_t start_cluster, uint64_t size)
{
	struct exfat_node* node;
	struct exfat_entry_meta1 meta1;
	struct exfat_entry_meta2 meta2;
	const size_t name_length = utf16_length(name);
	const int name_entries = DIV_ROUND_UP(name_length, EXFAT_ENAME_MAX);
	int i;

	node = allocate_node();
	if (node == NULL)
		return -ENOMEM;
	node->entry_cluster = cluster;
	node->entry_offset = offset;
	memcpy(node->name, name, name_length * sizeof(le16_t));

	memset(&meta1, 0, sizeof(meta1));
	meta1.type = EXFAT_ENTRY_FILE;
	meta1.continuations = 1 + name_entries;
	meta1.attrib = cpu_to_le16(attrib);
	exfat_unix2exfat(time(NULL), &meta1.crdate, &meta1.crtime,
			&meta1.crtime_cs);
	meta1.adate = meta1.mdate = meta1.crdate;
	meta1.atime = meta1.mtime = meta1.crtime;
	meta1.mtime_cs = meta1.crtime_cs; /* there is no atime_cs */

	memset(&meta2, 0, sizeof(meta2));
	meta2.type = EXFAT_ENTRY_FILE_INFO;
	meta2.flags = EXFAT_FLAG_ALWAYS1;
	meta2.name_length = name_length;
	meta2.name_hash = exfat_calc_name_hash(ef, node->name);
	meta2.start_cluster = cpu_to_le32(start_cluster);
	meta2.size = meta2.real_size = cpu_to_le64(size);

	meta1.checksum = exfat_calc_checksum(&meta1, &meta2, node->name);

	exfat_pwrite(ef->dev, &meta1, sizeof(meta1), co2o(ef, cluster, offset));
	next_entry(ef, dir, &cluster, &offset);
	exfat_pwrite(ef->dev, &meta2, sizeof(meta2), co2o(ef, cluster, offset));
	for (i = 0; i < name_entries; i++)
	{
		struct exfat_entry_name name_entry = {EXFAT_ENTRY_FILE_NAME, 0};
		memcpy(name_entry.name, node->name + i * EXFAT_ENAME_MAX,
				EXFAT_ENAME_MAX * sizeof(le16_t));
		next_entry(ef, dir, &cluster, &offset);
		exfat_pwrite(ef->dev, &name_entry, sizeof(name_entry),
				co2o(ef, cluster, offset));
	}

	init_node_meta1(node, &meta1);
	init_node_meta2(node, &meta2);

	tree_attach(dir, node);
	exfat_update_mtime(dir);
	return 0;
}

static int create(struct exfat* ef, const char* path, uint16_t attrib, cluster_t start_cluster, uint64_t size)
{
	struct exfat_node* dir;
	struct exfat_node* existing;
	cluster_t cluster = EXFAT_CLUSTER_BAD;
	s64 offset = -1;
	le16_t name[EXFAT_NAME_MAX + 1];
	int rc;

	rc = exfat_split(ef, &dir, &existing, name, path);
	if (rc != 0)
		return rc;
	if (existing != NULL)
	{
		exfat_put_node(ef, existing);
		exfat_put_node(ef, dir);
		return -EEXIST;
	}

	rc = find_slot(ef, dir, &cluster, &offset,
			2 + DIV_ROUND_UP(utf16_length(name), EXFAT_ENAME_MAX));
	if (rc != 0)
	{
		exfat_put_node(ef, dir);
		return rc;
	}
	rc = write_entry(ef, dir, name, cluster, offset, attrib, start_cluster, size);
	exfat_put_node(ef, dir);
	return rc;
}

int exfat_mknod(struct exfat* ef, const char* path, cluster_t start_cluster, uint64_t size)
{
	return create(ef, path, EXFAT_ATTRIB_ARCH, start_cluster, size);
}

/* additional_attrib use for special use, like create a hidden dir
 * Normally, additional_attrib=EXFAT_ATTRIB_NONE
 */
int exfat_mkdir(struct exfat* ef, const char* path, uint16_t additional_attrib)
{
	int rc;
	struct exfat_node* node;

	rc = create(ef, path, additional_attrib | EXFAT_ATTRIB_ARCH | EXFAT_ATTRIB_DIR, EXFAT_CLUSTER_FREE, 0);
	if (rc != 0)
		return rc;
	rc = exfat_lookup(ef, &node, path, 0);
	if (rc != 0)
		return 0;
	/* directories always have at least one cluster */
	rc = exfat_truncate(ef, node, CLUSTER_SIZE(*ef->sb), true);
	if (rc != 0)
	{
		delete(ef, node);
		exfat_put_node(ef, node);
		return rc;
	}
	exfat_put_node(ef, node);
	return 0;
}

static void rename_entry(struct exfat* ef, struct exfat_node* dir,
		struct exfat_node* node, const le16_t* name, cluster_t new_cluster,
		s64 new_offset)
{
	struct exfat_entry_meta1 meta1;
	struct exfat_entry_meta2 meta2;
	cluster_t old_cluster = node->entry_cluster;
	s64 old_offset = node->entry_offset;
	const size_t name_length = utf16_length(name);
	const int name_entries = DIV_ROUND_UP(name_length, EXFAT_ENAME_MAX);
	int i;

	exfat_pread(ef->dev, &meta1, sizeof(meta1),
			co2o(ef, old_cluster, old_offset));
	next_entry(ef, node->parent, &old_cluster, &old_offset);
	exfat_pread(ef->dev, &meta2, sizeof(meta2),
			co2o(ef, old_cluster, old_offset));
	meta1.continuations = 1 + name_entries;
	meta2.name_hash = exfat_calc_name_hash(ef, name);
	meta2.name_length = name_length;
	meta1.checksum = exfat_calc_checksum(&meta1, &meta2, name);

	erase_entry(ef, node);

	node->entry_cluster = new_cluster;
	node->entry_offset = new_offset;

	exfat_pwrite(ef->dev, &meta1, sizeof(meta1),
			co2o(ef, new_cluster, new_offset));
	next_entry(ef, dir, &new_cluster, &new_offset);
	exfat_pwrite(ef->dev, &meta2, sizeof(meta2),
			co2o(ef, new_cluster, new_offset));

	for (i = 0; i < name_entries; i++)
	{
		struct exfat_entry_name name_entry = {EXFAT_ENTRY_FILE_NAME, 0};
		memcpy(name_entry.name, name + i * EXFAT_ENAME_MAX,
				EXFAT_ENAME_MAX * sizeof(le16_t));
		next_entry(ef, dir, &new_cluster, &new_offset);
		exfat_pwrite(ef->dev, &name_entry, sizeof(name_entry),
				co2o(ef, new_cluster, new_offset));
	}

	memcpy(node->name, name, (EXFAT_NAME_MAX + 1) * sizeof(le16_t));
	if (node->prev == NULL) { /*first child*/
		if (node == node->parent->child)
			tree_detach(node, 0);
		else if (node == node->parent->twins)
			tree_detach(node, 1);
	} else
		tree_detach(node, 0);
	tree_attach(dir, node);
}

int exfat_rename(struct exfat* ef, const char* old_path, const char* new_path)
{
	struct exfat_node* node;
	struct exfat_node* existing;
	struct exfat_node* dir;
	cluster_t cluster = EXFAT_CLUSTER_BAD;
	s64 offset = -1;
	le16_t name[EXFAT_NAME_MAX + 1];
	int rc;

	rc = exfat_lookup(ef, &node, old_path, 0);
	if (rc != 0)
		return rc;

	rc = exfat_split(ef, &dir, &existing, name, new_path);
	if (rc != 0)
	{
		exfat_put_node(ef, node);
		return rc;
	}

	/* check that target is not a subdirectory of the source */
	if (node->flags & EXFAT_ATTRIB_DIR)
	{
		struct exfat_node* p;

		for (p = dir; p; p = p->parent)
			if (node == p)
			{
				if (existing != NULL)
					exfat_put_node(ef, existing);
				exfat_put_node(ef, dir);
				exfat_put_node(ef, node);
				return -EINVAL;
			}
	}

	if (existing != NULL)
	{
		/* remove target if it's not the same node as source */
		if (existing != node)
		{
			if (existing->flags & EXFAT_ATTRIB_DIR)
			{
				if (node->flags & EXFAT_ATTRIB_DIR)
					rc = exfat_rmdir(ef, existing);
				else
					rc = -ENOTDIR;
			}
			else
			{
				if (!(node->flags & EXFAT_ATTRIB_DIR))
					rc = exfat_unlink(ef, existing);
				else
					rc = -EISDIR;
			}
			exfat_put_node(ef, existing);
			if (rc != 0)
			{
				exfat_put_node(ef, dir);
				exfat_put_node(ef, node);
				return rc;
			}
		}
		else
			exfat_put_node(ef, existing);
	}

	rc = find_slot(ef, dir, &cluster, &offset,
			2 + DIV_ROUND_UP(utf16_length(name), EXFAT_ENAME_MAX));
	if (rc != 0)
	{
		exfat_put_node(ef, dir);
		exfat_put_node(ef, node);
		return rc;
	}
	rename_entry(ef, dir, node, name, cluster, offset);
	exfat_put_node(ef, dir);
	exfat_put_node(ef, node);
	return 0;
}

void exfat_utimes(struct exfat_node* node, const struct timespec tv[2])
{
	node->atime = tv[0].tv_sec;
	node->mtime = tv[1].tv_sec;
	node->flags |= EXFAT_ATTRIB_DIRTY;
}

void exfat_update_atime(struct exfat_node* node)
{
	node->atime = time(NULL);
	node->flags |= EXFAT_ATTRIB_DIRTY;
}

void exfat_update_mtime(struct exfat_node* node)
{
	node->mtime = time(NULL);
	node->flags |= EXFAT_ATTRIB_DIRTY;
}

const char* exfat_get_label(struct exfat* ef)
{
	return ef->label;
}

static int find_label(struct exfat* ef, cluster_t* cluster, s64* offset)
{
	struct iterator it;
	int rc;

	rc = opendir(ef, ef->root, &it);
	if (rc != 0)
		return rc;

	for (;;)
	{
		if (it.offset >= ef->root->size)
		{
			closedir(&it);
			return -ENOENT;
		}

		if (get_entry_ptr(ef, &it)->type == EXFAT_ENTRY_LABEL)
		{
			*cluster = it.cluster;
			*offset = it.offset;
			closedir(&it);
			return 0;
		}

		if (fetch_next_entry(ef, ef->root, &it) != 0)
		{
			closedir(&it);
			return -EIO;
		}
	}
}

int exfat_set_label(struct exfat* ef, const char* label)
{
	le16_t label_utf16[EXFAT_ENAME_MAX + 1];
	int rc;
	cluster_t cluster;
	s64 offset;
	struct exfat_entry_label entry;

	memset(label_utf16, 0, sizeof(label_utf16));
	rc = utf8_to_utf16(label_utf16, label, EXFAT_ENAME_MAX, strlen(label));
	if (rc != 0)
		return rc;

	rc = find_label(ef, &cluster, &offset);
	if (rc == -ENOENT)
		rc = find_slot(ef, ef->root, &cluster, &offset, 1);
	if (rc != 0)
		return rc;

	entry.type = EXFAT_ENTRY_LABEL;
	entry.length = utf16_length(label_utf16);
	memcpy(entry.name, label_utf16, sizeof(entry.name));
	if (entry.length == 0)
		entry.type ^= EXFAT_ENTRY_VALID;

	exfat_pwrite(ef->dev, &entry, sizeof(struct exfat_entry_label),
			co2o(ef, cluster, offset));
	return 0;
}
