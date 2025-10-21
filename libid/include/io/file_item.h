// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "misc/id.h"

#include <cstdio>
#include <filesystem>
#include <string>

namespace id::io
{

enum class ItemType
{
    PAR_SET = 0,
    FORMULA = 1,
    L_SYSTEM = 2,
    IFS = 3,
};

struct FileEntry
{
    char name[misc::ITEM_NAME_LEN + 2];
    long point; // points to the ( or the { following the name
};

int scan_entries(std::FILE *infile, FileEntry *choices);
bool search_for_entry(std::FILE *infile, const std::string &item_name);
bool find_file_item(
    std::filesystem::path &path, const std::string &item_name, std::FILE **file_ptr, ItemType item_type);

} // namespace id::io
