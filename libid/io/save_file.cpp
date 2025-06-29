// SPDX-License-Identifier: GPL-3.0-only
//
#include "io/save_file.h"

#include "io/special_dirs.h"

#include <filesystem>

namespace fs = std::filesystem;

std::filesystem::path get_save_name(const std::string &name)
{
    fs::path path{name};
    if (path.is_relative())
    {
        path = g_save_dir / path;
    }
    return path;
}

std::FILE *open_save_file(const std::string &name, const std::string &mode)
{
    return std::fopen(get_save_name(name).string().c_str(), mode.c_str());
}
