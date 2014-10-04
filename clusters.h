#ifndef __CLUSTERS_H__
#define __CLUSTERS_H__

#include <stdint.h>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <glibmm/ustring.h>
#include <pthread.h>
#include <boost/icl/interval_set.hpp>
#include <boost/icl/discrete_interval.hpp>

class Clusters {
    public:
        // type definitions
        class tuple {
            public:
            uint64_t start;
            uint64_t length;
            tuple(uint64_t s, uint64_t l) { start = s; length = l; };
            class compare {
                public:
                bool operator()(const tuple x, const tuple y) {
                    if (x.start == y.start) return x.length < y.length;
                    else return x.start < y.start;
                }
            };
        };

        typedef std::vector<tuple> tuple_list;

        enum FileType {
            TYPE_FILE = 0,
            TYPE_DIR = 1
        };

        typedef struct {
            tuple_list extents;
            std::string name;
            double severity;
            int fragmented;
            int filetype;
            uint64_t size;
        } f_info;

        typedef std::vector<f_info> file_list;
        typedef std::vector<unsigned int> file_p_list;

        class cluster_info {
            public:
            file_p_list files;
            int free;
            int fragmented;
            cluster_info () { free = 1; fragmented = 0; };
        };

        typedef std::map<int, cluster_info> cluster_list;

    public:
        Clusters();
        ~Clusters();

        void collect_fragments(const Glib::ustring& initial_dir);
        uint64_t get_device_size() const { return this->device_size; }
        void __fill_clusters(uint64_t m_start, uint64_t m_length);
        double get_file_severity(const f_info *fi, int64_t window, int shift, int penalty, double speed);
        int get_file_extents(const char *fname, const struct stat64 *sb, f_info *fi);
        void create_coarse_map(unsigned int granularity);

        int lock_clusters();
        int lock_files();
        int unlock_clusters();
        int unlock_files();

        cluster_info& at(unsigned int k) { return clusters[k]; }
        uint64_t get_count() { return cluster_count; }
        void set_desired_cluster_size(uint64_t ds);
        uint64_t get_actual_cluster_size(void);
        file_list& get_files();

    private:
        int fibmap_fallback(int fd, const char *fname, const struct stat64 *sb, struct fiemap *fiemap);
        void clear_caches();

    private:
        file_list files;
        cluster_list clusters;
        pthread_mutex_t clusters_mutex;
        pthread_mutex_t files_mutex;
        uint64_t device_size;
        uint64_t cluster_count;
        uint64_t cluster_size;
        bool hide_error_inaccessible_files;
        bool hide_error_no_fiemap;

        typedef boost::icl::discrete_interval<uint64_t> interval_t;
        typedef boost::icl::interval_set<uint64_t> interval_set_t;

        interval_set_t fill_cache;
        std::vector<std::vector<uint64_t> > coarse_map;
        unsigned int coarse_map_granularity;
};

#endif
