#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "clusters.h"

void print_usage() {
    printf ("Usage: fileseverity [-k] file [file] [file] ...\n");
}

int main (int argc, char *argv[]) {
    int opt;
    int flag_kilo;

    flag_kilo = 0;
    while ((opt = getopt(argc, argv, "k")) != -1) {
        switch (opt) {
            case 'k':
                flag_kilo = 1;
                break;
            default:
                print_usage();
                exit (1);
                break;
        }
    }

    if (optind >= argc) {
        print_usage();
        exit (1);
    }

    for (int fileidx = optind; fileidx < argc; fileidx ++) {
        struct stat64 st;
        if (-1 != stat64 (argv[fileidx], &st)) {
            f_info fi;
            if (get_file_extents (argv[fileidx], &st, &fi)) {
                if (flag_kilo) {
                    printf ("%7d %s\n", (int)(fi.severity * 1000.0), fi.name.c_str());
                } else {
                    printf ("%7.1f %s\n", fi.severity, fi.name.c_str());
                }
            }
        }
    }


    return 0;
}
