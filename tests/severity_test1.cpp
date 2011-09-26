#include "../clusters.h"
#include <stdio.h>

int main (void) {
    f_info fi;

    tuple ext1 = {1,1};
    tuple ext2 = {3,1};
    tuple ext3 = {5,1};
    tuple ext4 = {7,1};

    int param1 = 2*1024*1024 / 4096;    // window size (blocks)
    int param2 = 16*4096 / 4096;        // gap size (blocks)
    int param3 = 20;                    // hdd head reposition delay (ms)
    int param4 = 40e6 / 4096;           // raw read speed (blocks/s)

    fi.extents.push_back (ext1);
    printf ("severity = %f\n", get_file_severity (&fi, param1, param2, param3, param4));

    fi.extents.push_back (ext2);
    printf ("severity = %f\n", get_file_severity (&fi, param1, param2, param3, param4));

    fi.extents.push_back (ext3);
    printf ("severity = %f\n", get_file_severity (&fi, param1, param2, param3, param4));

    fi.extents.push_back (ext4);
    printf ("severity = %f\n", get_file_severity (&fi, param1, param2, param3, param4));

    return 0;
}
