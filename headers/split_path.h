#pragma once

#include <string>

extern int splitpath(char const *file_template, char *drive, char *dir, char *fname, char *ext);
inline int split_fname_ext(const char *file_template, char *fname, char *ext)
{
    return splitpath(file_template, nullptr, nullptr, fname, ext);
}
inline int splitpath(const std::string &file_template, char *drive, char *dir, char *fname, char *ext)
{
    return splitpath(file_template.c_str(), drive, dir, fname, ext);
}
