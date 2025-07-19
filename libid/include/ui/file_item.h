// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <cstdio>
#include <filesystem>
#include <string>

enum class ItemType
{
    PAR_SET = 0,
    FORMULA = 1,
    L_SYSTEM = 2,
    IFS = 3,
};

bool find_file_item(
    std::string &filename, const std::string &item_name, std::FILE **file_ptr, ItemType item_type);

inline bool find_file_item(
    std::filesystem::path &path, const std::string &item_name, std::FILE **file_ptr, ItemType item_type)
{
    std::string filename{path.string()};
    const bool result{find_file_item(filename, item_name, file_ptr, item_type)};
    path = filename;
    return result;
}

long get_file_entry(ItemType type, std::string &filename, std::string &entry_name);

inline long get_file_entry(ItemType type, std::filesystem::path &path, std::string &entry_name)
{
    std::string filename{path.string()};
    const long result{get_file_entry(type, filename, entry_name)};
    path = filename;
    return result;
}

bool search_for_entry(std::FILE *infile, const char *item_name);
