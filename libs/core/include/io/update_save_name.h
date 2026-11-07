// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <filesystem>
#include <string>

namespace id::io
{

std::string next_save_name(const std::string &filename);

inline void update_save_name(std::string &filename)
{
    filename = next_save_name(filename);
}

inline void update_save_name(std::filesystem::path &filename)
{
    filename = next_save_name(filename.string());
}

} // namespace id::io
