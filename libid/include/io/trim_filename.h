// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <filesystem>
#include <string>

std::string trim_file_name(const std::string &file_name, int length);
inline std::string trim_file_name(const std::filesystem::path &file_name, int length)
{
    return trim_file_name(file_name.string(), length);
}
