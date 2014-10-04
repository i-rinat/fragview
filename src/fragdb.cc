/* fragdb is a command-line interface crawler with functionality
 * similar to frag.php, i.e. it can recursively walk directories
 * while collecting numbers of extents for each file using
 * sqlite3 database as storage. Then it is possible to obtain
 * list of most fragmented files from previous scan
 */

#include <sqlite3.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <iomanip>
#include "clusters.hh"

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

void
init_db(sqlite3 **db)
{
    int res = sqlite3_open("fragmentator.db", db);
    if (SQLITE_OK != res) {
        std::cerr << "error: can't create/open database" << std::endl;
        sqlite3_close(*db);
        _exit(1);
    }
}

void
scan(sqlite3 *db, const char *dir)
{
    int res;
    char *errmsg;
    Clusters clusters;

    res = sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, &errmsg);
    if (SQLITE_OK != res) {
        std::cerr << "error: can't start transaction (" << errmsg << ")" << std::endl;
        sqlite3_close(db);
        _exit(1);
    }

    res = sqlite3_exec(db, sql_create_tables, NULL, NULL, &errmsg);
    if (SQLITE_OK != res) {
        std::cerr << "error: can't create table (" << errmsg << ")" << std::endl;
        sqlite3_close(db);
        _exit(1);
    }

    std::cout << "scanning \"" << dir << "\" ... " << std::flush;
    clusters.collect_fragments(dir);
    std::cout << "done." << std::endl;

    std::cout << "inserting results to the database ... " << std::flush;

    sqlite3_stmt *stmt;
    const char *sql_insert =
        "INSERT INTO items (name, fragments, severity) VALUES (:n, :f, :s)";
    res = sqlite3_prepare(db, sql_insert, -1, &stmt, NULL);
    if (SQLITE_OK != res) {
        std::cerr << "error: insert prepare" << std::endl;
        sqlite3_close(db);
        _exit(1);
    }

    Clusters::file_list &files = clusters.get_files();

    for (Clusters::file_list::iterator item = files.begin(); item != files.end(); ++item) {
        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);
        sqlite3_bind_text(stmt, 1, item->name.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 2, item->extents.size());
        sqlite3_bind_double(stmt, 3, item->severity);
        res = sqlite3_step(stmt);
        if (SQLITE_DONE != res) {
            std::cerr << "error: something wrong with sqlite3_step" << std::endl;
            sqlite3_close(db);
            _exit(1);
        }
    }

    res = sqlite3_exec(db, "COMMIT", NULL, NULL, &errmsg);
    if (SQLITE_OK != res) {
        std::cerr << "error: can't commit (" << errmsg << ")" << std::endl;
        sqlite3_close(db);
        _exit(1);
    }

    std::cout << "done." << std::endl;
}

void
show_top(sqlite3 *db, int top_count)
{
    sqlite3_stmt *stmt;
    int res;

    sqlite3_prepare(db, "SELECT name, fragments FROM items ORDER BY fragments DESC LIMIT ?", -1,
                    &stmt, NULL);
    sqlite3_bind_int(stmt, 1, top_count);

    while (SQLITE_ROW == (res = sqlite3_step(stmt))) {
        const char unsigned *name = sqlite3_column_text(stmt, 0);
        int frag = sqlite3_column_int(stmt, 1);
        if (NULL != name) {
            std::cout << std::setw(7) << frag << " " << name << std::endl;
        } else {
            std::cerr << "error: can't fetch name from db" << std::endl;
            sqlite3_close(db);
            _exit(1);
        }
    }
    sqlite3_finalize(stmt);
}

void
show_top_severity(sqlite3 *db, int top_count)
{
    sqlite3_stmt *stmt;
    int res;

    sqlite3_prepare(db, "SELECT name, severity FROM items ORDER BY severity DESC LIMIT ?", -1,
                    &stmt, NULL);
    sqlite3_bind_int(stmt, 1, top_count);

    while (SQLITE_ROW == (res = sqlite3_step(stmt))) {
        const char unsigned *name = sqlite3_column_text(stmt, 0);
        double severity = sqlite3_column_double(stmt, 1);
        if (NULL != name) {
            std::cout << std::fixed << std::setw (7) << std::setprecision (1) << severity << " ";
            std::cout << name << std::endl;
        } else {
            std::cerr << "error: can't fetch name from db" << std::endl;
            sqlite3_close(db);
            _exit(1);
        }
    }
    sqlite3_finalize(stmt);
}

void
show_over(sqlite3 *db, int over_count)
{
    sqlite3_stmt *stmt;
    int res;

    sqlite3_prepare(db,
                    "SELECT name, fragments FROM items WHERE fragments>=? ORDER BY fragments DESC",
                    -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, over_count);

    while (SQLITE_ROW == (res = sqlite3_step(stmt))) {
        const char unsigned *name = sqlite3_column_text(stmt, 0);
        int frag = sqlite3_column_int(stmt, 1);
        if (NULL != name) {
            std::cout << std::setw(7) << frag << " " << name << std::endl;
        } else {
            std::cerr << "error: can't fetch name from db" << std::endl;
            sqlite3_close(db);
            _exit(1);
        }
    }
    sqlite3_finalize(stmt);
}

void
show_over_severity(sqlite3 *db, double over_count)
{
    sqlite3_stmt *stmt;
    int res;

    sqlite3_prepare(db, "SELECT name, severity FROM items WHERE severity>=? ORDER BY severity DESC",
                    -1, &stmt, NULL);
    sqlite3_bind_double(stmt, 1, over_count);

    while (SQLITE_ROW == (res = sqlite3_step(stmt))) {
        const char unsigned *name = sqlite3_column_text(stmt, 0);
        double severity = sqlite3_column_double(stmt, 1);
        if (NULL != name) {
            std::cout << std::fixed << std::setw (7) << std::setprecision (1) << severity << " " << name << std::endl;
        } else {
            std::cerr << "error: can't fetch name from db" << std::endl;
            sqlite3_close(db);
            _exit(1);
        }
    }
    sqlite3_finalize(stmt);
}

int
main(int argc, char *argv[])
{
    sqlite3 *db;

    const char *dir = 0;

    if (argc < 2) {
        std::cout <<
            "Usage:\n"
            "  fragdb scan <dir>       -- walk <dir> and store framentation information in db\n"
            "  fragdb top [<count>]    -- show <count> most fragmented files, 10 by default\n"
            "  fragdb over <threshold> -- show files with >= <threshold> fragments\n"
            "  fragdb tops [<count>]   -- show <count> most fragmented files (by severity metric),"
                                                                            " 10 by default\n"
            "  fragdb over <threshold> -- show files with severity >= <threshold>\n"
            ;
        _exit(0);
    }

    if (0 == strcmp(argv[1], "scan")) {
        if (argc < 3) {
            std::cerr << "error: no path specified" << std::endl;
            _exit(2);
        }
        dir = argv[2];

        init_db(&db);
        scan(db, dir);
        show_top(db, 10);
    } else if (0 == strcmp(argv[1], "top")) {
        int top_count = 10;
        if (argc >= 3)
            top_count = atoi(argv[2]);
        init_db(&db);
        show_top(db, top_count);
    } else if (0 == strcmp(argv[1], "tops")) {
        int top_count = 10;
        if (argc >= 3)
            top_count = atoi(argv[2]);
        init_db(&db);
        show_top_severity(db, top_count);
    } else if (0 == strcmp(argv[1], "over")) {
        if (argc < 3) {
            std::cerr << "error: no threshold specified" << std::endl;
            _exit(2);
        }
        int over_count = atoi(argv[2]);
        init_db(&db);
        show_over(db, over_count);
    } else if (0 == strcmp(argv[1], "overs")) {
        if (argc < 3) {
            std::cerr << "error: no threshold specified" << std::endl;
            _exit(2);
        }
        int over_count = atof(argv[2]);
        init_db(&db);
        show_over_severity(db, over_count);
    }

    sqlite3_close(db);
    return 0;
}
