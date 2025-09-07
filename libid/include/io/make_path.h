// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <cassert>
#include <cstring>
#include <string>

namespace id::io
{

std::string make_path(const char *drive, const char *dir, const char *fname, const char *ext);

inline void make_path(char *template_str, const char *drive, const char *dir, const char *fname, const char *ext)
{
    if (template_str == nullptr)
    {
        assert(template_str != nullptr);
        return;
    }

    const std::string result{make_path(drive, dir, fname, ext)};
    std::strcpy(template_str, result.c_str());
}

inline void make_fname_ext(char *template_str, const char *fname, const char *ext)
{
    make_path(template_str, nullptr, nullptr, fname, ext);
}

inline void make_drive_dir(char *template_str, const char *drive, const char *dir)
{
    make_path(template_str, drive, dir, nullptr, nullptr);
}

} // namespace id::io
