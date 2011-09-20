/* fragdb is a command-line interface crawler with functionality
 * similar to frag.php, i.e. it can recursively walk directories
 * while collecting numbers of extents for each file using
 * sqlite3 database as storage. Then it is possible to obtain
 * list of most fragmented files from previous scan
 */

#include "clusters.h"
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

const char *sql_create_tables =
    "DROP TABLE IF EXISTS items; "
    "CREATE TABLE items( "
    "   id INTEGER PRIMARY KEY, "
    "   name TEXT, "
    "   fragments INTEGER, "
    "   severity REAL "
    "); "
    "CREATE INDEX name_idx ON items (name); "
    "CREATE INDEX frag_idx ON items (fragments); "
    "CREATE INDEX sev_idx ON items (severity); ";

void init_db (sqlite3 **db) {
    int res = sqlite3_open ("fragmentator.db", db);
    if (SQLITE_OK != res) {
        fprintf (stderr, "error: can't create/open database\n");
        sqlite3_close (*db);
        exit (1);
    }
}

void scan (sqlite3 *db, const char *dir) {
    int res;
    char *errmsg;
    file_list files;

    res = sqlite3_exec (db, "BEGIN TRANSACTION", NULL, NULL, &errmsg);
    if (SQLITE_OK != res) {
        fprintf (stderr, "error: can't start transaction (%s)\n", errmsg);
        sqlite3_close (db);
        exit (1);
    }

    res = sqlite3_exec (db, sql_create_tables, NULL, NULL, &errmsg);
    if (SQLITE_OK != res) {
        fprintf (stderr, "error: can't create table (%s)\n", errmsg);
        sqlite3_close (db);
        exit (1);
    }

    printf ("scanning \"%s\" ... ", dir); fflush (stdout);
    pthread_mutex_t useless_mutex = PTHREAD_MUTEX_INITIALIZER;
    collect_fragments (dir, &files, &useless_mutex);
    printf ("done.\n");

    printf ("inserting results to the database ... "); fflush (stdout);

    sqlite3_stmt *stmt;
    const char *sql_insert =
        "INSERT INTO items (name, fragments, severity) VALUES (:n, :f, :s)";
    res = sqlite3_prepare (db, sql_insert, -1, &stmt, NULL);
    if (SQLITE_OK != res) {
        fprintf (stderr, "error: insert prepare\n");
        sqlite3_close (db);
        exit (1);
    }

    for (file_list::iterator item = files.begin(); item != files.end(); ++item) {
        sqlite3_reset (stmt);
        sqlite3_clear_bindings (stmt);
        sqlite3_bind_text (stmt, 1, item->name.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int  (stmt, 2, item->extents.size());
        sqlite3_bind_double (stmt, 3, item->severity);
        res = sqlite3_step (stmt);
        if (SQLITE_DONE != res) {
            fprintf (stderr, "error: something wrong with sqlite3_step\n");
            sqlite3_close (db);
            exit (1);
        }
    }

    res = sqlite3_exec (db, "COMMIT", NULL, NULL, &errmsg);
    if (SQLITE_OK != res) {
        fprintf (stderr, "error: can't commit (%s)\n", errmsg);
        sqlite3_close (db);
        exit (1);
    }

    printf("done.\n");
}

void show_top (sqlite3 *db, int top_count) {
    sqlite3_stmt *stmt;
    int res;

    sqlite3_prepare (db, "SELECT name, fragments FROM items ORDER BY fragments DESC LIMIT ?", -1, &stmt, NULL);
    sqlite3_bind_int (stmt, 1, top_count);

    while (SQLITE_ROW == (res = sqlite3_step (stmt))) {
        const char unsigned *name = sqlite3_column_text (stmt, 0);
        int frag = sqlite3_column_int (stmt, 1);
        if (NULL != name) {
            printf ("%7d %s\n", frag, name);
        } else {
            fprintf (stderr, "error: can't fetch name from db\n");
            sqlite3_close (db);
            exit (1);
        }
    }
    sqlite3_finalize (stmt);
}

void show_top_severity (sqlite3 *db, int top_count) {
    sqlite3_stmt *stmt;
    int res;

    sqlite3_prepare (db, "SELECT name, severity FROM items ORDER BY severity DESC LIMIT ?", -1, &stmt, NULL);
    sqlite3_bind_int (stmt, 1, top_count);

    while (SQLITE_ROW == (res = sqlite3_step (stmt))) {
        const char unsigned *name = sqlite3_column_text (stmt, 0);
        double severity = sqlite3_column_double (stmt, 1);
        if (NULL != name) {
            printf ("%7.1f %s\n", severity, name);
        } else {
            fprintf (stderr, "error: can't fetch name from db\n");
            sqlite3_close (db);
            exit (1);
        }
    }
    sqlite3_finalize (stmt);
}

void show_over (sqlite3 *db, int over_count) {
    sqlite3_stmt *stmt;
    int res;

    sqlite3_prepare (db, "SELECT name, fragments FROM items WHERE fragments>=? ORDER BY fragments DESC", -1, &stmt, NULL);
    sqlite3_bind_int (stmt, 1, over_count);

    while (SQLITE_ROW == (res = sqlite3_step (stmt))) {
        const char unsigned *name = sqlite3_column_text (stmt, 0);
        int frag = sqlite3_column_int (stmt, 1);
        if (NULL != name) {
            printf ("%7d %s\n", frag, name);
        } else {
            fprintf (stderr, "error: can't fetch name from db\n");
            sqlite3_close (db);
            exit (1);
        }
    }
    sqlite3_finalize (stmt);
}

void show_over_severity (sqlite3 *db, double over_count) {
    sqlite3_stmt *stmt;
    int res;

    sqlite3_prepare (db, "SELECT name, severity FROM items WHERE severity>=? ORDER BY severity DESC", -1, &stmt, NULL);
    sqlite3_bind_double (stmt, 1, over_count);

    while (SQLITE_ROW == (res = sqlite3_step (stmt))) {
        const char unsigned *name = sqlite3_column_text (stmt, 0);
        double severity = sqlite3_column_double (stmt, 1);
        if (NULL != name) {
            printf ("%7.1f %s\n", severity, name);
        } else {
            fprintf (stderr, "error: can't fetch name from db\n");
            sqlite3_close (db);
            exit (1);
        }
    }
    sqlite3_finalize (stmt);
}

int main (int argc, char *argv[]) {

    sqlite3 *db;
    int res;

    const char *dir = "/home";

    if (argc < 2) {
        printf ("Usage:\n"
                "  fragdb scan <dir>       -- walk <dir> and store framentation information in db\n"
                "  fragdb top [<count>]    -- show <count> most fragmented files, 10 by default\n"
                "  fragdb over <threshold> -- show files with >= <threshold> fragments\n");
        exit (0);
    }

    if (0 == strcmp (argv[1], "scan")) {
        if (argc < 3) {
            printf ("error: no path specified\n");
            exit (2);
        }
        dir = argv[2];

        init_db (&db);
        scan (db, dir);
        show_top (db, 10);
    } else if (0 == strcmp (argv[1], "top")) {
        int top_count = 10;
        if (argc >= 3) top_count = atoi (argv[2]);
        init_db (&db);
        show_top (db, top_count);
    } else if (0 == strcmp (argv[1], "tops")) {
        int top_count = 10;
        if (argc >= 3) top_count = atoi (argv[2]);
        init_db (&db);
        show_top_severity (db, top_count);
    } else if (0 == strcmp (argv[1], "over")) {
        if (argc < 3) {
            printf ("error: no threshold specified\n");
            exit (2);
        }
        int over_count = atoi (argv[2]);
        init_db (&db);
        show_over (db, over_count);
    } else if (0 == strcmp (argv[1], "overs")) {
        if (argc < 3) {
            printf ("error: no threshold specified\n");
            exit (2);
        }
        int over_count = atof (argv[2]);
        init_db (&db);
        show_over_severity (db, over_count);
    }

    sqlite3_close (db);
}
