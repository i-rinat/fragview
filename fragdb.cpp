/* fragdb is a command-line interface crawler with functionality
 * similar to frag.php, i.e. it can recursively walk directories
 * while collecting numbers of extents for each file using
 * sqlite3 database as storage. Then it is possible to obtain
 * list of most fragmented files from previous scan
 */

#include "clusters.h"
#include <sqlite3.h>

int main (int argc, char *argv[]) {

    sqlite3 *db;
    int res;

    res = sqlite3_open("fragmentator.db", &db);

    res = sqlite3_close(db);

}
