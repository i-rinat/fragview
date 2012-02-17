#ifndef __CLUSTERS_H__
#define __CLUSTERS_H__

#include <stdint.h>
#include <vector>
#include <string>
#include <glibmm/ustring.h>
#include <pthread.h>


class Clusters {
    public:
        // type definitions
        typedef struct {
            int64_t start;
            int64_t length;
        } tuple;

        typedef std::vector<tuple> tuple_list;

        typedef struct {
            tuple_list extents;
            std::string name;
            double severity;
            int fragmented;
        } f_info;

        typedef std::vector<f_info> file_list;
        typedef std::vector<int> file_p_list;

        typedef struct {
            file_p_list files;
            int free;
            int fragmented;
        } cluster_info;

        typedef std::vector<cluster_info> cluster_list;

    public:
        Clusters ();
        ~Clusters ();

        void collect_fragments (const Glib::ustring& initial_dir);
        uint64_t get_device_size ();
        void __fill_clusters (uint64_t cluster_count, int frag_limit);
        double get_file_severity (const f_info *fi, int64_t window, int shift, int penalty, double speed);
        int get_file_extents (const char *fname, const struct stat64 *sb, f_info *fi);

        int lock_clusters ();
        int lock_files ();
        int unlock_clusters ();
        int unlock_files ();

        cluster_info& at (int k) { return clusters.at(k); }
        size_t size () { return clusters.size(); }
        file_list& get_files ();

    private:
        int fibmap_fallback (int fd, const char *fname, const struct stat64 *sb, struct fiemap *fiemap);


    private:
        file_list files;
        cluster_list clusters;
        pthread_mutex_t clusters_mutex;
        pthread_mutex_t files_mutex;
        uint64_t device_size;
};

#endif
