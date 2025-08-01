// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <string>

enum
{
    MAX_VIDEO_MODES = 300       // maximum entries in id.cfg
};

class Driver;

struct VideoInfo
{                     // All we need to know about a video mode:
    int key;          // function key used to invoke this mode
    int x_dots;       // number of dots across the screen
    int y_dots;       // number of dots down the screen
    int colors;       // number of colors available
    Driver *driver;   // associated driver for this mode
    char comment[26]; // Comments (UNTESTED, etc)
};

extern VideoInfo             g_video_table[];
extern int                   g_video_table_len;

// returns g_video_table entry number if the passed keystroke is a
// function key currently assigned to a video mode, -1 otherwise
int check_vid_mode_key(int key);

// returns key number for the passed key name, 0 if not a key name
int check_vid_mode_key_name(const char *key_name);

// set buffer to name of passed key number
void vid_mode_key_name(int key, char *buffer);

inline std::string vid_mode_key_name(int key)
{
    char buffer[16];
    vid_mode_key_name(key, buffer);
    return buffer;
}
