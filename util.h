#ifndef FRAGVIEW_UTIL_H
#define FRAGVIEW_UTIL_H

#include <string>
#include <stdint.h>

class Util {
    public:
        static void format_filesize(uint64_t size, std::string &res);
    private:
        Util() {}
};

#endif // FRAGVIEW_UTIL_H
