#pragma once

#include <cassert>
#include <cstring>
#include <string>

std::string make_path(char const *drive, char const *dir, char const *fname, char const *ext);

inline void make_path(char *template_str, char const *drive, char const *dir, char const *fname, char const *ext)
{
    if (template_str == nullptr)
    {
        assert(template_str != nullptr);
        return;
    }

    const std::string result{make_path(drive, dir, fname, ext)};
    std::strcpy(template_str, result.c_str()); 
}

inline std::string make_fname_ext(char const *fname, char const *ext)
{
    return make_path(nullptr, nullptr, fname, ext);
}

inline void make_fname_ext(char *template_str, char const *fname, char const *ext)
{
    make_path(template_str, nullptr, nullptr, fname, ext);
}

inline std::string make_drive_dir(const char *drive, const char *dir)
{
    return make_path(drive, dir, nullptr, nullptr);
}

inline void make_drive_dir(char *template_str, const char *drive, const char *dir)
{
    make_path(template_str, drive, dir, nullptr, nullptr);
}
