// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <cstdio>
#include <filesystem>
#include <string>

namespace id::ui
{

enum class ItemType
{
    PAR_SET = 0,
    FORMULA = 1,
    L_SYSTEM = 2,
    IFS = 3,
};

bool find_file_item(
    std::filesystem::path &path, const std::string &item_name, std::FILE **file_ptr, ItemType item_type);

long get_file_entry(ItemType type, std::filesystem::path &path, std::string &entry_name);

} // namespace id::ui
