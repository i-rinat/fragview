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

#define _XOPEN_SOURCE 500
#include "clusters.hh"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

void
print_usage()
{
    printf("Usage: fileseverity [-k] file [file] [file] ...\n");
}

int
main(int argc, char *argv[])
{
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
            exit(1);
            break;
        }
    }

    if (optind >= argc) {
        print_usage();
        exit(1);
    }

    Clusters clusters;

    for (int fileidx = optind; fileidx < argc; fileidx++) {
        struct stat64 st;
        if (-1 != stat64(argv[fileidx], &st)) {
            Clusters::f_info fi;
            if (clusters.get_file_extents(argv[fileidx], &st, &fi)) {
                if (flag_kilo) {
                    printf("%7d %s\n", (int)(fi.severity * 1000.0), argv[fileidx]);
                } else {
                    printf("%7.1f %s\n", fi.severity, argv[fileidx]);
                }
            }
        }
    }

    return 0;
}
