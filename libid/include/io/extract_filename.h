// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <filesystem>
#include <string>

inline std::string extract_file_name(const char *source)
{
    return std::filesystem::path(source).filename().string();
}
