// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <cstdio>
#include <string>

enum class ItemType
{
    PAR_SET = 0,
    FORMULA = 1,
    L_SYSTEM = 2,
    IFS = 3,
};

bool find_file_item(std::string &filename, char const *itemname, std::FILE **fileptr, ItemType itemtype);

long get_file_entry(ItemType type, char const *title, char const *fmask, std::string &filename, std::string &entryname);

bool search_for_entry(std::FILE *infile, char const *itemname);
