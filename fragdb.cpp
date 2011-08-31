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

const char *sql_create_tables =
"DROP TABLE IF EXISTS items; "
"CREATE TABLE items( "
"id INTEGER PRIMARY KEY, "
"name TEXT, "
"fragments INTEGER "
"); "
"CREATE INDEX name_idx ON items (name); "
"CREATE INDEX frag_idx ON items (fragments); ";


int main (int argc, char *argv[]) {

    sqlite3 *db;
    int res;
    char *errmsg;
    const char *dir = "/home";

    file_list files;
    pthread_mutex_t useless_mutex = PTHREAD_MUTEX_INITIALIZER;


    res = sqlite3_open ("fragmentator.db", &db);
    if (SQLITE_OK != res) {
        fprintf (stderr, "error: can't create/open database\n");
        sqlite3_close (db);
        exit (1);
    }

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
    collect_fragments (dir, &files, &useless_mutex);
    printf ("done.\n");

    printf ("inserting results to the database ... "); fflush (stdout);
    for (file_list::iterator item = files.begin(); item != files.end(); ++item) {
        sqlite3_stmt *stmt;
        // printf ("%s\n", item->name.c_str());
        const char *sql_insert =
            "INSERT INTO items (name, fragments) VALUES (:n, :f)";
        res = sqlite3_prepare (db, sql_insert, -1, &stmt, NULL);
        if (SQLITE_OK != res) {
            fprintf (stderr, "error: insert prepare\n");
            sqlite3_close (db);
            exit (2);
        }
        sqlite3_bind_text (stmt, 1, item->name.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int  (stmt, 2, item->extents.size());
        res = sqlite3_step (stmt);
        if (SQLITE_DONE != res) {
            fprintf (stderr, "error: something wrong with sqlite3_step\n");
            sqlite3_close (db);
            exit (2);
        }
    }

    res = sqlite3_exec (db, "COMMIT", NULL, NULL, &errmsg);
    if (SQLITE_OK != res) {
        fprintf (stderr, "error: can't commit (%s)\n", errmsg);
        sqlite3_close (db);
        exit (1);
    }

    printf ("done.\n");

    res = sqlite3_close (db);

}
