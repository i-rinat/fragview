#include "clusters.h"
#include <map>
#include <iostream>
#include <fstream>
#include <fts.h>
#include <fcntl.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/ioctl.h>
#include <linux/fiemap.h>
#include <linux/fs.h>

Clusters::Clusters ()
{
    pthread_mutex_init (&files_mutex, NULL);
    pthread_mutex_init (&clusters_mutex, NULL);
}

Clusters::~Clusters ()
{

}

int
Clusters::lock_clusters ()
{
    return pthread_mutex_lock (&clusters_mutex);
}

int
Clusters::lock_files ()
{
    return pthread_mutex_lock (&files_mutex);
}

int
Clusters::unlock_clusters ()
{
    return pthread_mutex_unlock (&clusters_mutex);
}

int
Clusters::unlock_files ()
{
    return pthread_mutex_unlock (&files_mutex);
}

Clusters::file_list&
Clusters::get_files ()
{
    return files;
}

void
Clusters::collect_fragments (const Glib::ustring & initial_dir)
{
    struct stat64 sb;

    this->device_size = 0;

    if (0 == stat64 (initial_dir.c_str(), &sb)) {
        std::string partition_name;
        std::fstream fp("/proc/partitions", std::ios_base::in);
        fp.ignore (1024, '\n'); // header
        fp.ignore (1024, '\n'); // empty line

        while (! fp.eof ()){
            unsigned int major, minor;
            uint64_t blocks;

            fp >> major >> minor >> blocks >> partition_name;
            if (sb.st_dev == (major << 8) + minor) {
                this->device_size = blocks * 1024 / sb.st_blksize;
                break;
            }
        }
        fp.close ();
    }

    // walk directory tree, don't cross mount borders
    char *dirs[2] = {const_cast<char *>(initial_dir.c_str()), 0};
    FTS *fts_handle = fts_open (dirs, FTS_PHYSICAL | FTS_XDEV | FTS_NOCHDIR, NULL);

    while (1) {
        FTSENT *ent = fts_read (fts_handle);
        if (NULL == ent) break;
        switch (ent->fts_info) {
            case FTS_D:
            case FTS_F:
                {
                    f_info fi;
                    struct stat64 sb;
                    if (0 != lstat64 (ent->fts_path, &sb)) // something wrong
                        break;
                    if (get_file_extents (ent->fts_path, &sb, &fi)) {
                        this->lock_files ();
                        files.push_back (fi);
                        this->unlock_files ();
                    }
                }
                break;
            default:
                continue;
        }
    }

    fts_close (fts_handle);

    std::cout << "files.size = " << files.size() << std::endl;
}

uint64_t
Clusters::get_device_size ()
{
    return this->device_size;
}

void
Clusters::__fill_clusters (uint64_t cluster_count, int frag_limit)
{
    if (cluster_count == 0) cluster_count = 1;
    // divide whole disk to clusters of blocks
    uint64_t cluster_size = (this->device_size - 1) / cluster_count + 1;

    clusters.resize(cluster_count);

    int k, k2;
    for (k = 0; k < clusters.size(); ++k) {
        clusters.at(k).free = 1;
        clusters.at(k).fragmented = 0;
        clusters.at(k).files.resize (0);
    }
    std::cout << "cluster_size = " << cluster_size << std::endl;

    typedef std::map<int, int> onecopy_t;
    onecopy_t *entry_exist = new onecopy_t[clusters.size()];

    file_list::iterator item;
    int item_idx;
    for (item = files.begin(), item_idx = 0; item != files.end(); ++item, ++item_idx) {
        item->fragmented = (item->severity >= 2.0);
        for (k2 = 0; k2 < item->extents.size(); k2 ++) {
            uint64_t estart_c, eend_c;

            estart_c = item->extents[k2].start / cluster_size;
            eend_c = (item->extents[k2].start +
                      item->extents[k2].length - 1);
            eend_c = eend_c / cluster_size;

            for (uint64_t k3 = estart_c; k3 <= eend_c; k3 ++ ) {
                // N-th cluster start: cluster_size * N
                // N-th cluster end:   (cluster_size+1)*N-1
                if (entry_exist[k3].count(item_idx) == 0) {
                    clusters.at(k3).files.push_back(item_idx);
                    entry_exist[k3][item_idx] = 1;
                }
                clusters.at(k3).free = 0;
                if (item->fragmented)
                    clusters.at(k3).fragmented = 1;
            }
        }
    }

    delete [] entry_exist;
}

double
Clusters::get_file_severity (const f_info *fi, int64_t window, int shift, int penalty, double speed)
{
    double overall_severity = 1.0; // continuous files have severity equal to 1.0

    for (int k1 = 0; k1 < fi->extents.size(); k1 ++) {
        int64_t span = window;
        double read_time = 1e-3 * penalty;
        for (int k2 = k1; k2 < fi->extents.size() && span > 0; k2 ++) {
            if (fi->extents[k2].length <= span) {
                span -= fi->extents[k2].length;
                read_time += fi->extents[k2].length / speed;
                if (k2 + 1 < fi->extents.size()) { // for all but last extent
                    // compute head "jump"
                    int64_t delta = fi->extents[k2+1].start -
                        (fi->extents[k2].start + fi->extents[k2].length);
                    if (delta >= 0 && delta <= shift) { // small gaps
                        read_time += delta / speed;     // are likely to be read
                    } else {                            // while large ones
                        read_time += penalty * 1.0e-3;  // will cause head jump
                    }
                }
            } else {
                read_time += span / speed;
                span = 0;
            }
        }
        double raw_read_time = (window - span) / speed + penalty * 1e-3;
        double local_severity = read_time / raw_read_time;
        // we need the worst value
        overall_severity = std::max(local_severity, overall_severity);
    }

    return overall_severity;
}

// emulate FIEMAP by means of FIBMAP
int
Clusters::fibmap_fallback(int fd, const char *fname, const struct stat64 *sb, struct fiemap *fiemap)
{
    uint64_t block;
    uint64_t block_count = (0 == sb->st_size) ? 0 :
                        (sb->st_size - 1) / sb->st_blksize + 1;
    uint64_t block_start = fiemap->fm_start / sb->st_blksize;
    fiemap->fm_mapped_extents = 0;
    struct fiemap_extent *extents = &fiemap->fm_extents[0];
    int idx = 0; // index for extents array

    for (uint64_t k = block_start;
         k < block_count && idx < fiemap->fm_extent_count;
         k ++)
    {
        block = k;
        int ret = ioctl(fd, FIBMAP, &block);
        if (0 != ret) {
            if (ENOTTY != errno) {
                std::cout << fname << " FIBMAP unavailable, " << errno << ", " << strerror (errno) << std::endl;
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
                break;    // there is no room left in the extent table
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

int
Clusters::get_file_extents (const char *fname, const struct stat64 *sb, f_info *fi)
{
    static char fiemap_buffer[16*1024];

    fi->name = fname;

    if (!S_ISREG(sb->st_mode) && !S_ISDIR(sb->st_mode) && !S_ISLNK(sb->st_mode)) {
        return 0; // not regular file or directory
    }

    int fd = open (fname, O_RDONLY | O_NOFOLLOW | O_LARGEFILE);
    if (-1 == fd) {
#ifndef IGNORE_INACCESSIBLE_FILES
        std::cerr << "can't open file/dir: " << fname << std::endl;
#endif
        return 0;
    }

    struct fiemap *fiemap = (struct fiemap *)fiemap_buffer;
    int max_count = (sizeof(fiemap_buffer) - sizeof(struct fiemap)) /
                        sizeof(struct fiemap_extent);

    memset (fiemap, 0, sizeof(struct fiemap));
    fiemap->fm_start = 0;
    do {
        fiemap->fm_extent_count = max_count;
        fiemap->fm_length = ~0ULL;

        int ret = ioctl (fd, FS_IOC_FIEMAP, fiemap);
        if (ret == -1) {
#ifndef IGNORE_UNAVAILABLE_FIEMAP
            std::cerr << fname << ", fiemap get count error " << errno << ":\"" << strerror (errno) << "\"" << std::endl;
#endif
            // there is no FIEMAP or it's inaccessible, trying to emulate
            if (-1 == fibmap_fallback (fd, fname, sb, fiemap)) {
                if (ENOTTY != errno) {
                    std::cerr << fname << ", fibmap fallback failed." << std::endl;
                }
                close (fd);
                return 0;
            }
        }

        if (0 == fiemap->fm_mapped_extents) break; // there are no more left

        int last_entry;
        for (int k = 0; k < (int)fiemap->fm_mapped_extents; ++k) {
            tuple tempt = { fiemap->fm_extents[k].fe_physical / sb->st_blksize,
                            fiemap->fm_extents[k].fe_length / sb->st_blksize };

            if (fi->extents.size() > 0) {
                tuple *last = &fi->extents.back();
                if (last->start + last->length == tempt.start) {
                    last->length += tempt.length;    // extent continuation
                } else {
                    fi->extents.push_back (tempt);
                }
            } else {
                fi->extents.push_back (tempt);
            }
            last_entry = k;
        }
        fiemap->fm_start = fiemap->fm_extents[last_entry].fe_logical +
                            fiemap->fm_extents[last_entry].fe_length;

    } while (1);

    // calculate linear read performance degradation
    fi->severity = get_file_severity (fi,
        2*1024*1024 / sb->st_blksize,    // window size (blocks)
        16*4096 / sb->st_blksize,        // gap size (blocks)
        20,                              // hdd head reposition delay (ms)
        40e6 / sb->st_blksize);          // raw read speed (blocks/s)

    close (fd);
    return 1;
}
