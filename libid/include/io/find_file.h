// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <string>

enum
{
    SUB_DIR = 1
};

struct DirSearch
{
    std::string path;     // path and filespec
    char attribute;       // File attributes wanted
    std::string filename; // Filename and extension
};

extern DirSearch g_dta;

bool fr_find_first(const char *path);
bool fr_find_next();
