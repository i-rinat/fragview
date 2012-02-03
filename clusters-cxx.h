#ifndef __CLUSTERS_H__
#define __CLUSTERS_H__

#include <stdint.h>
#include <vector>
#include <string>


class Clusters {
    public:
        // internal type definitions
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

        void collect_fragments (const char *initial_dir);
        uint64_t get_device_size_in_blocks (const char *initial_dir);
        void __fill_clusters ();
        double get_file_severity (const f_info *fi, int64_t window, int shift, int penalty, double speed);
        int get_file_extents (const char *fname, const struct stat64 *sb, f_info *fi);

    private:
        file_list files2;
        cluster_list clusters2;
        pthread_mutex_t clusters2_mutex;
        pthread_mutex_t files2_mutex;
};

#endif
