// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <string>

// text_temp_msg(msg) displays a text message of up to 40 characters, waits
// for a key press, restores the prior display, and returns (without eating the key).
int text_temp_msg(const char *msg);
bool show_temp_msg(const char *msg);
inline bool show_temp_msg(const std::string &msg)
{
    return show_temp_msg(msg.c_str());
}
void clear_temp_msg();
void free_temp_msg();
