// SPDX-License-Identifier: GPL-3.0-only
//
#include "io/dir_file.h"

#include <config/path_limits.h>

#include <cstdio>
#include <cstring>

static void dir_name(char *target, const char *dir, const char *name)
{
    *target = 0;
    if (*dir != 0)
    {
        std::strcpy(target, dir);
    }
    std::strcat(target, name);
}

// removes file in dir directory
int dir_remove(const char *dir, const char *filename)
{
    char tmp[ID_FILE_MAX_PATH];
    dir_name(tmp, dir, filename);
    return std::remove(tmp);
}

// fopens file in dir directory
std::FILE *dir_fopen(const char *dir, const char *filename, const char *mode)
{
    char tmp[ID_FILE_MAX_PATH];
    dir_name(tmp, dir, filename);
    return std::fopen(tmp, mode);
}
