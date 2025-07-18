// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <filesystem>

namespace id::io
{

enum class ReadFile
{
    FORMULA,
    IFS,
    IMAGE,
    KEY,
    LSYSTEM,
    MAP,
    PARAMETER,
};

enum class WriteFile
{
    FORMULA,
    IFS,
    IMAGE,
    KEY,
    LSYSTEM,
    MAP,
    PARAMETER,
    ROOT,
};

void clear_read_library_path();
void add_read_library(std::filesystem::path path);

std::filesystem::path find_file(ReadFile kind, const std::filesystem::path &filename);

void clear_save_library();
void set_save_library(std::filesystem::path path);
std::filesystem::path get_save_path(WriteFile file, const std::string &filename);

} // namespace id::io
