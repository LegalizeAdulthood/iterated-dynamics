// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <filesystem>
#include <string_view>

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
    DEBUG,
    DEBUG_JSON,
};

extern bool                  g_check_cur_dir;       // flag to check current dir for files

void clear_read_library_path();
void add_read_library(std::filesystem::path path);
void add_read_libraries(std::string_view path_list);
void init_default_read_libraries();

std::filesystem::path find_file(ReadFile kind, const std::filesystem::path &file_path);
std::filesystem::path find_file_in_read_library(const std::filesystem::path &file_path);

void clear_save_library();
void set_save_library(std::filesystem::path path);
std::filesystem::path get_save_path(WriteFile kind, const std::string &filename);

std::filesystem::path find_wildcard_first(ReadFile kind, const std::string &wildcard);
std::filesystem::path find_wildcard_next();

} // namespace id::io
