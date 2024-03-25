#include "make_path.h"

#include <cassert>
#include <cstring>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

void make_path(char *template_str, char const *drive, char const *dir, char const *fname, char const *ext)
{
    if (template_str == nullptr)
    {
        assert(template_str != nullptr);
        return;
    }

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
        result.replace_extension(ext);
    }
    std::strcpy(template_str, result.lexically_normal().make_preferred().string().c_str());
}
