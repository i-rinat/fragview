#define _XOPEN_SOURCE 500
#define _LARGEFILE64_SOURCE

#ifndef _GNU_SOURCE
	#define _GNU_SOURCE
#endif

#include <ftw.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <linux/fiemap.h>
#include "clusters.h"



#define IGNORE_INACCESSIBLE_FILES
#define IGNORE_UNAVAILABLE_FIEMAP

cluster_list clusters2;
file_list files2;
pthread_mutex_t clusters2_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t files2_mutex = PTHREAD_MUTEX_INITIALIZER;

static file_list *worker_files;
static pthread_mutex_t *worker_files_mutex;

// emulate FIEMAP by means of FIBMAP
static int fibmap_fallback(int fd, const char *fname, const struct stat64 *sb,
							struct fiemap *fiemap)
{
	__u64 block;
	__u64 block_count = (0 == sb->st_size) ? 0 :
						(sb->st_size - 1) / sb->st_blksize + 1;
	__u64 block_start = fiemap->fm_start / sb->st_blksize;
	fiemap->fm_mapped_extents = 0;
	struct fiemap_extent *extents = &fiemap->fm_extents[0];
	int idx = 0; // index for extents array

	for (__u64 k = block_start;
	     k < block_count && idx < fiemap->fm_extent_count;
         k ++)
    {
		block = k;
		int ret = ioctl(fd, FIBMAP, &block);
		if (0 != ret) {
			if (ENOTTY != errno) {
				printf("%s: FIBMAP unavailable, %d, %s\n", fname, errno, strerror(errno));
			}
			return ret;
		}
		if (0 == block) {
			// '0' here means block is not allocated, there is hole in file.
			// we should try next block, skipping current
			continue;
		}

		fiemap->fm_start += sb->st_blksize;

		int flag_very_first_extent = (k == block_start);
		if (!flag_very_first_extent &&
		    extents[idx].fe_logical + extents[idx].fe_length == block * sb->st_blksize)
		{
			extents[idx].fe_length += sb->st_blksize;
		} else {
			if (!flag_very_first_extent) idx ++;
			if (idx >= fiemap->fm_extent_count)
				break;	// there is no room left in the extent table
			extents[idx].fe_logical  = k * sb->st_blksize;
			extents[idx].fe_physical = block * sb->st_blksize;
			extents[idx].fe_length = sb->st_blksize;
			extents[idx].fe_flags = FIEMAP_EXTENT_MERGED;
		}
		if (k == block_count - 1) {
			extents[idx].fe_flags |= FIEMAP_EXTENT_LAST;
		}
	}
	fiemap->fm_mapped_extents = idx;

	return 0;
}

static int worker_fiemap(const char *fname, const struct stat64 *sb,
				  int typeflag, struct FTW *ftw_struct) {
	f_info fi;
	static char fiemap_buffer[16*1024];
	
	fi.name = fname;
	if (!S_ISREG(sb->st_mode) && !S_ISDIR(sb->st_mode) &&
		!S_ISLNK(sb->st_mode) )
	{
		//printf("not regular: %s\n", fname);
		return 0;
	}
	//printf("file: %s\n", fname);
	
	int fd = open(fname, O_RDONLY | O_NOFOLLOW | O_LARGEFILE);
	if (-1 == fd) {
#ifndef IGNORE_INACCESSIBLE_FILES
		fprintf(stderr, "can't open file/dir: %s\n", fname);
#endif
		return 0;
	}

	struct fiemap *fiemap = (struct fiemap *)fiemap_buffer;
	int max_count = (sizeof(fiemap_buffer) - sizeof(struct fiemap)) / 
						sizeof(struct fiemap_extent);
	
	int extent_number = 0;
	memset(fiemap, 0, sizeof(struct fiemap) );
	fiemap->fm_start = 0;
	do {
		fiemap->fm_extent_count = max_count;
		fiemap->fm_length = ~0ULL;

		int ret = ioctl(fd, FS_IOC_FIEMAP, fiemap);
		if (ret == -1) {
#ifndef IGNORE_UNAVAILABLE_FIEMAP
			fprintf(stderr, "%s, fiemap get count error %d: \"%s\"\n", fname, 
				errno, strerror(errno));
#endif
			// there is no FIEMAP or it's inaccessible, trying to emulate
			if (-1 == fibmap_fallback(fd, fname, sb, fiemap)) {
				if (ENOTTY != errno) {
					fprintf(stderr, "%s, fibmap fallback failed.\n", fname);
				}
				close(fd);
				return 0;
			}
		}
		
		if (0 == fiemap->fm_mapped_extents) break; // there are no more left
		
		int last_entry;
		for (int k = 0; k < (int)fiemap->fm_mapped_extents; ++k) {
			tuple tempt = { fiemap->fm_extents[k].fe_physical / sb->st_blksize,
							fiemap->fm_extents[k].fe_length / sb->st_blksize };

			if (fi.extents.size() > 0) {
				tuple *last = &fi.extents.back();
				if (last->start + last->length == tempt.start) {
					last->length += tempt.length;	// extent continuation
				} else {
					fi.extents.push_back(tempt);
				}
			} else {
				fi.extents.push_back(tempt);
			}
			last_entry = k;
		}
		fiemap->fm_start = fiemap->fm_extents[last_entry].fe_logical +
							fiemap->fm_extents[last_entry].fe_length;
		
	} while (1);
	
	pthread_mutex_lock(worker_files_mutex);
	worker_files->push_back(fi);
	pthread_mutex_unlock(worker_files_mutex);

	close(fd);
	return 0;
}

__u64 get_device_size_in_blocks(const char *initial_dir) {
	// get info about device
	char cmd[128];
	struct stat64 ss64;
	int res = lstat64(initial_dir, &ss64);
	if (-1 == res) {
		printf("error stat'ing %s: %s\n", initial_dir, strerror(errno));
		fflush(stdout);
		exit(1);
	}
	blksize_t device_block_size = ss64.st_blksize;

	sprintf(cmd, "awk '($1==%d && $2==%d){print $3}' /proc/partitions",
		 major(ss64.st_dev), minor(ss64.st_dev));
	FILE *pp = popen(cmd, "r");
	if (NULL == pp) {
		fprintf(stderr, "can't get blockdev size\n");
		fflush(stderr); fflush(stdout);
		_exit(1);
	}

	char ssize[128];
	fgets(ssize, 127, pp);
	fclose(pp);
	
	// '/proc/partitions' reports blockdevice size in 1K units
	return atoll(ssize) * 1024 / device_block_size;	
}


void collect_fragments(const char *initial_dir, file_list *files, pthread_mutex_t *files_mutex) {
	// walk directory tree, don't cross mount borders
	worker_files = files;
	worker_files_mutex = files_mutex;
	nftw64(initial_dir, worker_fiemap, 40, FTW_MOUNT | FTW_PHYS);
	printf("files.size = %lu\n", files->size());
}

void __fill_clusters(file_list *files, __u64 device_size_in_blocks, 
					cluster_list *clusters, __u64 cluster_count,
					int frag_limit)
{
	
	if (cluster_count == 0) cluster_count = 1;
	// divide whole disk to clusters of blocks
	__u64 cluster_size = (device_size_in_blocks-1) / cluster_count + 1;
	
	clusters->resize(cluster_count);
	
	int files_per_cluster_mean = files->size() / cluster_count;
	
	int k, k2;
	for (k = 0; k < clusters->size(); ++k) {
		clusters->at(k).free = 1;
		clusters->at(k).fragmented = 0;
		clusters->at(k).files.resize(0);
		//clusters->at(k).files.reserve(files_per_cluster_mean);
	}
	printf("cluster_size = %llu\n", cluster_size);

	unsigned int files_count = files->size();
	
	file_list::iterator item;
	for (item = files->begin(); item != files->end(); ++item) {

		for (k2 = 0; k2 < item->extents.size(); k2 ++) {
			__u64 estart_c, eend_c;
			__u64 estart_b, eend_b;

			estart_c = item->extents[k2].start / cluster_size;
			eend_c = (item->extents[k2].start +
					  item->extents[k2].length - 1);
			eend_c = eend_c / cluster_size;
			// printf("span: %llu-%llu\n", estart_c, eend_c);
			
			for (__u64 k3 = estart_c; k3 <= eend_c; k3 ++ ) {
				// N-th cluster start: cluster_size * N
				// N-th cluster end:   (cluster_size+1)*N-1
				clusters->at(k3).files.push_back(&(*item));
				clusters->at(k3).free = 0;
				if (item->extents.size() > frag_limit)
					clusters->at(k3).fragmented = 1;
			}
		}
	}
}

