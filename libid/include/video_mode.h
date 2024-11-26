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

int check_vid_mode_key(int, int);
int check_vid_mode_key_name(char const *kname);
void vid_mode_key_name(int k, char *buf);
