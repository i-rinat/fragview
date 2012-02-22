#include "clusters.h"
#include <map>
#include <iostream>
#include <fstream>
#include <algorithm>
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

    hide_error_inaccessible_files = true;
    hide_error_no_fiemap = true;
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
Clusters::create_coarse_map (int granularity)
{
    this->coarse_map_granularity = granularity;
    int map_size = (get_device_size () - 1) / coarse_map_granularity + 1;
    coarse_map.resize (map_size);

    for (int k = 0; k < files.size(); ++ k) {
        for (int k2 = 0; k2 < files[k].extents.size(); k2 ++) {
            uint64_t estart_c, eend_c;

            estart_c = files[k].extents[k2].start / coarse_map_granularity;
            eend_c = (files[k].extents[k2].start + files[k].extents[k2].length - 1);
            eend_c = eend_c / coarse_map_granularity;

            for (uint64_t k3 = estart_c; k3 <= eend_c; k3 ++ ) {
                coarse_map[k3].insert (k);
            }
        }
    }

    std::cout << "I've done creating coarse_map and here is some statistics:" << std::endl;
    unsigned int max = 0;
    int max_idx = -1;
    int min = 2000000000;
    double mean = 0;
    for (int k = 0; k < map_size; k++ ) {
        if (coarse_map[k].size() > max) {
            max = coarse_map[k].size();
            max_idx = k;
        }
        min = std::min (min, (int)coarse_map[k].size());
        mean += coarse_map[k].size();
    }
    std::cout << "coarse_map.size() == " << coarse_map.size() << std::endl;
    std::cout << "max_depth of map: " << max << " files in one cluster idx=" << max_idx << std::endl;
    std::cout << "min_depth of map: " << min << " files in one cluster" << std::endl;
    std::cout << "mean depth of map: " << mean/map_size << " files in one cluster" << std::endl;
}

void
Clusters::collect_fragments (const Glib::ustring & initial_dir)
{
    struct stat64 sb_root;

    this->device_size = 0;

    if (0 == stat64 (initial_dir.c_str(), &sb_root)) {
        std::string partition_name;
        std::fstream fp("/proc/partitions", std::ios_base::in);
        fp.ignore (1024, '\n'); // header
        fp.ignore (1024, '\n'); // empty line

        while (! fp.eof ()){
            unsigned int major, minor;
            uint64_t blocks;

            fp >> major >> minor >> blocks >> partition_name;
            if (sb_root.st_dev == (major << 8) + minor) {
                this->device_size = blocks * 1024 / sb_root.st_blksize;
                break;
            }
        }
        fp.close ();
    }

    // walk directory tree, don't cross mount borders
    char *dirs[2] = {const_cast<char *>(initial_dir.c_str()), 0};
    FTS *fts_handle = fts_open (dirs, FTS_PHYSICAL | FTS_XDEV | FTS_NOCHDIR | FTS_NOSTAT, NULL);

    while (1) {
        FTSENT *ent = fts_read (fts_handle);
        if (NULL == ent) break;
        switch (ent->fts_info) {
            case FTS_NSOK:
            case FTS_NS:
            case FTS_D:
                {
                    f_info fi;
                    struct stat64 sb_ent;
                    if (0 != lstat64 (ent->fts_path, &sb_ent)) break; // something wrong, skip
                    if (sb_ent.st_dev != sb_root.st_dev) break; // another device, skip
                    if (get_file_extents (ent->fts_path, &sb_ent, &fi)) {
                        this->lock_files ();
                        files.push_back (fi);
                        this->unlock_files ();
                    }
                }
                break;
            default:
                break;
        }
    }

    fts_close (fts_handle);

    std::cout << "files.size = " << files.size() << std::endl;
}

void
Clusters::allocate (uint64_t cluster_count)
{
    if (cluster_count == 0) cluster_count = 1;
    this->cluster_count = cluster_count;

    clusters.clear ();
    fill_cache.clear ();
}

void
Clusters::__fill_clusters (uint64_t m_start, uint64_t m_length)
{
    // parameters are in cluster units, so compute cluster size
    uint64_t cluster_size = (this->device_size - 1) / cluster_count + 1;

    // get starting and ending items in coarse map
    uint64_t c_start = m_start * cluster_size / coarse_map_granularity;
    uint64_t c_end = ((m_start + m_length - 1) * cluster_size - 1) / coarse_map_granularity;
    c_end = std::min (c_end, (uint64_t)coarse_map.size() - 1);

    // convert to blocks
    c_start = c_start * coarse_map_granularity;
    c_end = (c_end + 1) * coarse_map_granularity - 1;
    c_end = std::min (c_end, get_device_size ());

    interval_t requested_interval (c_start, c_end);
    interval_set_t scan_intervals;
    scan_intervals += requested_interval;
    scan_intervals -= fill_cache;

    fill_cache += requested_interval;

    for (interval_set_t::iterator i_iter = scan_intervals.begin(); i_iter != scan_intervals.end(); ++ i_iter) {
        // get block
        uint64_t c_start2 = i_iter->lower();
        uint64_t c_end2   = i_iter->upper();
        c_start2 /= coarse_map_granularity;
        c_end2 /= coarse_map_granularity;

        for (uint64_t c_block = c_start2; c_block < c_end2; c_block ++) {
            std::set<uint64_t>::iterator f_iter = coarse_map[c_block].begin ();
            for (; f_iter != coarse_map[c_block].end (); ++ f_iter) {
                unsigned int item_idx = *f_iter;
                f_info &fi = files[item_idx];
                fi.fragmented = (fi.severity >= 2.0);
                for (unsigned int k2 = 0; k2 < fi.extents.size(); k2 ++) {
                    uint64_t estart_c, eend_c;

                    estart_c = fi.extents[k2].start / cluster_size;
                    eend_c = (fi.extents[k2].start + fi.extents[k2].length - 1) / cluster_size;

                    if (estart_c > m_start + m_length - 1) continue;
                    if (eend_c < m_start) continue;
                    if (estart_c < m_start) estart_c = m_start;
                    if (eend_c > m_start + m_length - 1) eend_c = m_start + m_length - 1;

                    for (uint64_t k3 = estart_c; k3 <= eend_c; k3 ++ ) {
                        clusters[k3].files.push_back (item_idx);
                        clusters[k3].free = 0;
                        if (fi.fragmented) clusters[k3].fragmented = 1;
                    } // k3
                } // k2
            }
        }
    }

    for (int k3 = m_start; k3 < m_start + m_length; k3 ++) {
        file_p_list &filesp = clusters[k3].files;
        std::sort(filesp.begin(), filesp.end());
        filesp.erase(std::unique(filesp.begin(), filesp.end()), filesp.end());
    }
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
        if (! hide_error_inaccessible_files) {
            std::cerr << "can't open file/dir: " << fname << std::endl;
        }
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
            if (! hide_error_no_fiemap) {
                std::cerr << fname << ", fiemap get count error " << errno << ":\"" << strerror (errno) << "\"" << std::endl;
            }
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
            tuple tempt ( fiemap->fm_extents[k].fe_physical / sb->st_blksize,
                          fiemap->fm_extents[k].fe_length / sb->st_blksize );

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
