// SPDX-License-Identifier: GPL-3.0-only
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
};

extern std::shared_ptr<SpecialDirectories> create_special_directories();
extern std::shared_ptr<SpecialDirectories> g_special_dirs;
extern std::filesystem::path g_save_dir;

} // namespace id::io
