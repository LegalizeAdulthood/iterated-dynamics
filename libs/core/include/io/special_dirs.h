// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include <filesystem>
#include <memory>

namespace id::io
{

class SpecialDirectories
{
public:
    SpecialDirectories() = default;
    virtual ~SpecialDirectories() = default;

    virtual std::filesystem::path program_dir() const = 0;
    virtual std::filesystem::path documents_dir() const = 0;
    // Default resources supplied with this build.
    virtual std::filesystem::path resource_dir() const;
};

extern std::shared_ptr<SpecialDirectories> create_special_directories();
extern std::shared_ptr<SpecialDirectories> g_special_dirs;
extern std::filesystem::path g_save_dir;
extern std::filesystem::path g_temp_dir;    // name of temporary directory
extern std::filesystem::path g_working_dir; // name of directory for misc files

} // namespace id::io
