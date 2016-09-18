/*
 * Copyright © 2011-2014  Rinat Ibragimov
 *
 * This file is part of "fragview" software suite.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include <boost/icl/discrete_interval.hpp>
#include <boost/icl/interval_set.hpp>
#include <glibmm/ustring.h>
#include <map>
#include <pthread.h>
#include <set>
#include <stdint.h>
#include <string>
#include <vector>

class Clusters
{
public:
    // type definitions
    class tuple
    {
    public:
        uint64_t start;
        uint64_t length;
        tuple(uint64_t s, uint64_t l)
            : start(s)
            , length(l){};
        class compare
        {
        public:
            bool
            operator()(const tuple x, const tuple y)
            {
                if (x.start == y.start)
                    return x.length < y.length;
                else
                    return x.start < y.start;
            }
        };
    };

    typedef std::vector<tuple> tuple_list;

    enum FileType { TYPE_FILE = 0, TYPE_DIR = 1 };

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

    class cluster_info
    {
    public:
        file_p_list files;
        int free;
        int fragmented;
        cluster_info()
            : free(1)
            , fragmented(0){};
    };

    typedef std::map<int, cluster_info> cluster_list;

public:
    Clusters();

    ~Clusters();

    void
    collect_fragments(const Glib::ustring &initial_dir);

    uint64_t
    get_device_size() const
    {
        return device_size_;
    }

    uint64_t
    get_device_block_size() const
    {
        return device_block_size_;
    }

    void
    __fill_clusters(uint64_t m_start, uint64_t m_length);

    double
    get_file_severity(const f_info *fi, int64_t window, int shift, int penalty, double speed);

    int
    get_file_extents(const char *fname, const struct stat64 *sb, f_info *fi);

    void
    create_coarse_map(unsigned int granularity);

    int
    lock_clusters();

    int
    lock_files();

    int
    unlock_clusters();

    int
    unlock_files();

    cluster_info &
    at(unsigned int k)
    {
        return clusters_[k];
    }

    uint64_t
    get_count()
    {
        return cluster_count_;
    }

    void
    set_desired_cluster_size(uint64_t ds);

    uint64_t
    get_actual_cluster_size();

    file_list &
    get_files();

private:
    int
    fibmap_fallback(int fd, const char *fname, const struct stat64 *sb, struct fiemap *fiemap);

    void
    clear_caches();

private:
    file_list files_;
    cluster_list clusters_;
    pthread_mutex_t clusters_mutex_;
    pthread_mutex_t files_mutex_;
    uint64_t device_size_;
    uint64_t device_block_size_;
    uint64_t cluster_count_;
    uint64_t cluster_size_;
    bool hide_error_inaccessible_files_;
    bool hide_error_no_fiemap_;

    typedef boost::icl::discrete_interval<uint64_t> interval_t;
    typedef boost::icl::interval_set<uint64_t> interval_set_t;

    interval_set_t fill_cache_;
    unsigned int coarse_map_granularity_;
    std::vector<std::vector<uint64_t>> coarse_map_;
};
