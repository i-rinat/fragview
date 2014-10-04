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

#define __STDC_LIMIT_MACROS
#include <sstream>
#include <iomanip>
#include "util.hh"

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
