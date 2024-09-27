// SPDX-License-Identifier: GPL-3.0-only
//
#include "make_path.h"

#include <filesystem>
#include <string>

namespace fs = std::filesystem;

std::string make_path(char const *drive, char const *dir, char const *fname, char const *ext)
{
    fs::path result;
    auto not_empty = [](const char *ptr) { return ptr != nullptr && ptr[0] != 0; };
#ifndef XFRACT
    if (not_empty(drive))
    {
        result = drive;
    }
#endif
    if (not_empty(dir))
    {
        result /= dir;
        result.make_preferred();
        if (result.string().back() != fs::path::preferred_separator)
        {
            result += '/';
        }
    }
    if (not_empty(fname))
    {
        result /= fname;
    }
    if (not_empty(ext))
    {
        result += ext;
    }
    return result.lexically_normal().make_preferred().string();
}
