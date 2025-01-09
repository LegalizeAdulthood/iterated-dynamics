// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <string>

void split_path(const std::string &file_template, char *drive, char *dir, char *fname, char *ext);

inline void split_fname_ext(const std::string &file_template, char *fname, char *ext)
{
    split_path(file_template, nullptr, nullptr, fname, ext);
}

inline void split_drive_dir(const std::string &file_template, char *drive, char *dir)
{
    split_path(file_template, drive, dir, nullptr, nullptr);
}
