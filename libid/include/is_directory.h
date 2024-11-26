// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <filesystem>

inline bool is_a_directory(const char *s)
{
    return std::filesystem::is_directory(s);
}
