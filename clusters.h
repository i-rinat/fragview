#ifndef __CLUSTERS_H__
#define __CLUSTERS_H__

#include <stdint.h>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <glibmm/ustring.h>
#include <pthread.h>


class Clusters {
    public:
        // type definitions
        class tuple {
            public:
            uint64_t start;
            uint64_t length;
            tuple (uint64_t s, uint64_t l) { start = s; length = l; };
            class compare {
                public:
                bool operator () (const tuple x, const tuple y) {
                    if (x.start == y.start) return x.length < y.length;
                    else return x.start < y.start;
                }
            };
        };

        typedef std::vector<tuple> tuple_list;

        typedef struct {
            tuple_list extents;
            std::string name;
            double severity;
            int fragmented;
        } f_info;

        typedef std::vector<f_info> file_list;
        typedef std::vector<int> file_p_list;

        class cluster_info {
            public:
            file_p_list files;
            int free;
            int fragmented;
            cluster_info () { free = 1; fragmented = 0; };
        };

        typedef std::map<int, cluster_info> cluster_list;

    public:
        Clusters ();
        ~Clusters ();

        void collect_fragments (const Glib::ustring& initial_dir);
        uint64_t get_device_size () const { return this->device_size; }
        void allocate (uint64_t cluster_count);
        void __fill_clusters (uint64_t start, uint64_t length);
        double get_file_severity (const f_info *fi, int64_t window, int shift, int penalty, double speed);
        int get_file_extents (const char *fname, const struct stat64 *sb, f_info *fi);
        void create_coarse_map (int granularity);

        int lock_clusters ();
        int lock_files ();
        int unlock_clusters ();
        int unlock_files ();

        cluster_info& at (int k) { return clusters[k]; }
        uint64_t size () { return cluster_count; }
        file_list& get_files ();

    private:
        int fibmap_fallback (int fd, const char *fname, const struct stat64 *sb, struct fiemap *fiemap);


    private:
        file_list files;
        cluster_list clusters;
        pthread_mutex_t clusters_mutex;
        pthread_mutex_t files_mutex;
        uint64_t device_size;
        uint64_t cluster_count;
        bool hide_error_inaccessible_files;
        bool hide_error_no_fiemap;

        std::map<tuple, bool, tuple::compare> fill_cache;
        std::vector<std::set<uint64_t> > coarse_map;
        int coarse_map_granularity;
};

#endif
