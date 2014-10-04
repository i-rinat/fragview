#define __STDC_LIMIT_MACROS
#include "util.h"
#include <sstream>
#include <iomanip>

void
Util::format_filesize(uint64_t size, std::string &res)
{
    std::stringstream ss;

    if (size < 1024) {
        ss << size << " B";
        res = ss.str();
        return;
    }

    ss << std::fixed << std::setprecision(1);

    if (size < __UINT64_C(1048576)) {
        ss << (double)size/__UINT64_C(1024) << " kiB";
    } else if (size < __UINT64_C(1073741824)) {
        ss << (double)size/__UINT64_C(1048576) << " MiB";
    } else if (size < __UINT64_C(1099511627776)) {
        ss << (double)size/__UINT64_C(1073741824) << " GiB";
    } else {
        ss << (double)size/__UINT64_C(1099511627776) << " TiB";
    }

    res = ss.str();
}
