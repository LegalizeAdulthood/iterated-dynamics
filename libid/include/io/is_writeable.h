// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <filesystem>

inline bool is_writeable(const std::filesystem::path &path)
{
    namespace fs = std::filesystem;
    constexpr fs::perms read_write{fs::perms::owner_read | fs::perms::owner_write};
    return (status(path).permissions() & read_write) == read_write;
}
