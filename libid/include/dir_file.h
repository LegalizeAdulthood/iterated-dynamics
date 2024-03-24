#pragma once

#include <cstdio>
#include <string>

int dir_remove(char const *dir, char const *filename);
inline int dir_remove(const std::string &dir, const std::string &filename)
{
    return dir_remove(dir.c_str(), filename.c_str());
}
std::FILE *dir_fopen(char const *dir, char const *filename, char const *mode);
