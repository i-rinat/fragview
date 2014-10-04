#ifndef FRAGVIEW_UTIL_HH
#define FRAGVIEW_UTIL_HH

#include <string>
#include <stdint.h>

class Util {
    public:
        static void format_filesize(uint64_t size, std::string &res);
    private:
        Util() {}
};

#endif // FRAGVIEW_UTIL_HH
