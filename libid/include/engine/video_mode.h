// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <string>

namespace id::engine
{

// returns g_video_table entry number if the passed keystroke is a
// function key currently assigned to a video mode, -1 otherwise
int check_vid_mode_key(int key);

// returns key number for the passed key name, 0 if not a key name
int check_vid_mode_key_name(const char *key_name);

// set buffer to name of passed key number
void vid_mode_key_name(int key, char *buffer);

inline std::string vid_mode_key_name(const int key)
{
    char buffer[16];
    vid_mode_key_name(key, buffer);
    return buffer;
}

} // namespace id::engine
