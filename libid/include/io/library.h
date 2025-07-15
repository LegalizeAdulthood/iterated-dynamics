// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <filesystem>

namespace id::io
{

enum class FileType
{
    FORMULA,
    IFS,
    IMAGE,
    KEY,
    LSYSTEM,
    MAP,
    PARAMETER
};

void clear_search_path();
void add_library(std::filesystem::path path);

std::filesystem::path find_file(FileType kind, const std::filesystem::path &filename);

} // namespace id::io
