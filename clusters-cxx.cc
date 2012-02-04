#include "clusters-cxx.h"
#include <assert.h>
#include <map>
#include <iostream>
#include <fts.h>
#include <sys/stat.h>

Clusters::Clusters ()
{

}

Clusters::~Clusters ()
{

}

/*
int
Clusters::worker_fiemap (const char *fname, const struct stat64 *sb, int typeflag, struct FTW *ftw_struct)
{
    f_info fi;

    if (get_file_extents (fname, sb, &fi)) {
        pthread_mutex_lock(worker_files_mutex);
        worker_files->push_back(fi);
        pthread_mutex_unlock(worker_files_mutex);
    }

    return 0;
}
*/

void
Clusters::collect_fragments (const Glib::ustring & initial_dir)
{
    // walk directory tree, don't cross mount borders
    //nftw64 (initial_dir.c_str(), worker_fiemap, 40, FTW_MOUNT | FTW_PHYS);

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
                    lstat64 (ent->fts_path, &sb);
                    if (get_file_extents (ent->fts_path, &sb, &fi)) {
                        pthread_mutex_lock (&files_mutex);
                        files.push_back (fi);
                        pthread_mutex_unlock (&files_mutex);
                    }
                }
                break;
            default:
                continue;
        }
        std::cout << ent->fts_path << std::endl;
    }

    fts_close (fts_handle);

    std::cout << "files.size = " << files.size() << std::endl;
}

uint64_t
Clusters::get_device_size_in_blocks (Glib::ustring & initial_dir)
{
    assert (0);
}

void
Clusters::__fill_clusters (uint64_t device_size_in_blocks, uint64_t cluster_count, int frag_limit)
{
    if (cluster_count == 0) cluster_count = 1;
    // divide whole disk to clusters of blocks
    uint64_t cluster_size = (device_size_in_blocks-1) / cluster_count + 1;

    clusters.resize(cluster_count);

    int files_per_cluster_mean = files.size() / cluster_count;

    int k, k2;
    for (k = 0; k < clusters.size(); ++k) {
        clusters.at(k).free = 1;
        clusters.at(k).fragmented = 0;
        clusters.at(k).files.resize(0);
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
            uint64_t estart_b, eend_b;

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
    assert (0);
}

int
Clusters::get_file_extents (const char *fname, const struct stat64 *sb, f_info *fi)
{
    assert (0);
}
