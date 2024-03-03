#pragma once

#include <string>

void splitpath(char const *file_template, char *drive, char *dir, char *fname, char *ext);

inline void split_fname_ext(const char *file_template, char *fname, char *ext)
{
    splitpath(file_template, nullptr, nullptr, fname, ext);
}

inline void splitpath(const std::string &file_template, char *drive, char *dir, char *fname, char *ext)
{
    splitpath(file_template.c_str(), drive, dir, fname, ext);
}
