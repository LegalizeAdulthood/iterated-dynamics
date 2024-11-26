// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <filesystem>
#include <functional>
#include <string>

enum
{
    SUB_DIR = 1
};

struct DirSearch
{
    std::string path;           // path and filespec
    char attribute;             // File attributes wanted
    int  ftime;                 // File creation time
    int  fdate;                 // File creation date
    long size;                  // File size in bytes
    std::string filename;       // Filename and extension
};

extern DirSearch g_dta;

int fr_find_first(char const *path);
int fr_find_next();
