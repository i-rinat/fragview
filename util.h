#pragma once
#ifndef __UTILS_H__
#define __UTILS_H__

#include <string>
#include <stdint.h>

class Util {
    public:
        static void format_filesize (uint64_t size, std::string& res);
    private:
        Util () { }
};

#endif
