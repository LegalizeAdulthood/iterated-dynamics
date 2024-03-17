#pragma once

#include <string>

std::string next_save_name(const std::string &filename);

void update_save_name(char *filename);

inline void update_save_name(std::string &filename)
{
    filename = next_save_name(filename);
}
