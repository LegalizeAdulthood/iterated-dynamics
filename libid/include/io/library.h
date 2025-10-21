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
    ID_CONFIG,
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
    RAYTRACE,
    ID_CONFIG,
    ORBIT,
    SOUND,
};

extern bool                  g_check_cur_dir;       // flag to check current dir for files
extern std::filesystem::path g_fractal_search_dir1;
extern std::filesystem::path g_fractal_search_dir2;

void clear_read_library_path();
void add_read_library(std::filesystem::path path);

std::filesystem::path find_file(ReadFile kind, const std::filesystem::path &file_path);

void clear_save_library();
void set_save_library(std::filesystem::path path);
std::filesystem::path get_save_path(WriteFile kind, const std::string &filename);

std::filesystem::path find_wildcard_first(ReadFile kind, const std::string &wildcard);
std::filesystem::path find_wildcard_next();

} // namespace id::io
