// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "misc/id.h"

#include <cstdio>
#include <string>

namespace id::io
{

struct FileEntry
{
    char name[ITEM_NAME_LEN + 2];
    long point; // points to the ( or the { following the name
};

int scan_entries(std::FILE *infile, FileEntry *choices);
bool search_for_entry(std::FILE *infile, const std::string &item_name);

} // namespace id::io
