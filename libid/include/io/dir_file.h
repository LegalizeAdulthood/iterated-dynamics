// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <cstdio>
#include <string>

int dir_remove(const char *dir, const char *filename);
inline int dir_remove(const std::string &dir, const std::string &filename)
{
    return dir_remove(dir.c_str(), filename.c_str());
}
std::FILE *dir_fopen(const char *dir, const char *filename, const char *mode);
