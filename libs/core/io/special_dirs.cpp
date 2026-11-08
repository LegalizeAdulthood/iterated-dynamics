// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <io/special_dirs.h>

#include <config/home_dir.h>

namespace id::io
{

std::shared_ptr<SpecialDirectories> g_special_dirs{create_special_directories()};
std::filesystem::path g_save_dir;
std::filesystem::path g_temp_dir;
std::filesystem::path g_working_dir;

std::filesystem::path SpecialDirectories::resource_dir() const
{
    return id::config::HOME_DIR;
}

} // namespace id::io
