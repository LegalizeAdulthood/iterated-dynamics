// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <string>

int text_temp_msg(char const *);
bool show_temp_msg(char const *);
inline bool show_temp_msg(const std::string &msg)
{
    return show_temp_msg(msg.c_str());
}
void clear_temp_msg();
void free_temp_msg();
