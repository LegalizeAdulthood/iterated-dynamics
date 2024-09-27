// SPDX-License-Identifier: GPL-3.0-only
//
#include "update_save_name.h"

#include <cstring>
#include <filesystem>

 // go to the next file name
std::string next_save_name(const std::string &filename)
{
    std::filesystem::path file_path{filename};
    std::string           stem{file_path.stem().string()};
    const auto            last_non_digit = stem.find_last_not_of("0123456789");
    if (last_non_digit == stem.length() - 1)
    {
        file_path.replace_filename(stem + "2" + file_path.extension().string());
    }
    else
    {
        const auto first_digit_pos = last_non_digit + 1;
        std::string next = std::to_string(std::stoi(stem.substr(first_digit_pos)) + 1);
        const auto  num_digits = stem.length() - first_digit_pos;
        if (num_digits > next.length())
        {
            next = std::string(num_digits - next.length(), '0') + next;
        }
        file_path.replace_filename(stem.substr(0, first_digit_pos) + next + file_path.extension().string());
    }
    return file_path.string();
}

void update_save_name(char *filename)
{
    const std::string next{next_save_name(filename)};
    std::strcpy(filename, next.c_str());
}
