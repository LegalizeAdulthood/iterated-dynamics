#pragma once

#include <string>

void splitpath(const std::string &file_template, char *drive, char *dir, char *fname, char *ext);

inline void split_fname_ext(const std::string &file_template, char *fname, char *ext)
{
    splitpath(file_template, nullptr, nullptr, fname, ext);
}

inline void split_drive_dir(const std::string &file_template, char *drive, char *dir)
{
    splitpath(file_template, drive, dir, nullptr, nullptr);
}
