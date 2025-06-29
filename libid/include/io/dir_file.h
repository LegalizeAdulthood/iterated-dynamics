// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <cstdio>
#include <filesystem>

inline void dir_remove(const std::filesystem::path &dir, const std::filesystem::path &filename)
{
    std::filesystem::remove(dir / filename);
}

inline std::FILE *dir_fopen(const std::filesystem::path &dir, const std::filesystem::path &filename, const char *mode)
{
    return std::fopen((dir / filename).string().c_str(), mode);
}
