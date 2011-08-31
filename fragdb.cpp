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

    res = sqlite3_open ("fragmentator.db", &db);
    if (SQLITE_OK != res) {
        fprintf (stderr, "error: can't create/open database\n");
        exit (1);
    }

    res = sqlite3_exec (db, sql_create_tables, NULL, NULL, &errmsg);
    if (SQLITE_OK != res) {
        fprintf (stderr, "error: can't create table\n");
        fprintf (stderr, "sqlite3 said \"%s\"", errmsg);
        exit (1);
    }

    res = sqlite3_close (db);

}
