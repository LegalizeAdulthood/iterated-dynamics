// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

enum
{
    MAX_VIDEO_MODES = 300       // maximum entries in id.cfg
};

class Driver;
struct VideoInfo
{                     // All we need to know about a video mode:
    int keynum;       // key number used to invoke this mode: 2-10 = F2-10, 11-40 = Shift,Ctrl,Alt{F1-F10}
    int xdots;        // number of dots across the screen
    int ydots;        // number of dots down the screen
    int colors;       // number of colors available
    Driver *driver;   //
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
