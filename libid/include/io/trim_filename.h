// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <filesystem>
#include <string>

std::string trim_filename(const std::string &filename, int length);
inline std::string trim_filename(const std::filesystem::path &filename, int length)
{
    return trim_filename(filename.string(), length);
}
