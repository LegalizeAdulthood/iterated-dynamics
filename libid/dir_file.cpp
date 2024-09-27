// SPDX-License-Identifier: GPL-3.0-only
//
#include "dir_file.h"

#include "port.h"

#include "id.h"

#include <cstdio>
#include <cstring>

static void dir_name(char *target, char const *dir, char const *name)
{
    *target = 0;
    if (*dir != 0)
    {
        std::strcpy(target, dir);
    }
    std::strcat(target, name);
}

// removes file in dir directory
int dir_remove(char const *dir, char const *filename)
{
    char tmp[FILE_MAX_PATH];
    dir_name(tmp, dir, filename);
    return std::remove(tmp);
}

// fopens file in dir directory
std::FILE *dir_fopen(char const *dir, char const *filename, char const *mode)
{
    char tmp[FILE_MAX_PATH];
    dir_name(tmp, dir, filename);
    return std::fopen(tmp, mode);
}
