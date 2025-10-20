// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <cstring>

namespace id::misc
{
class Driver;
}

namespace id::engine
{

enum
{
    MAX_VIDEO_MODES = 300       // maximum entries in id.cfg
};

struct VideoInfo
{                         // All we need to know about a video mode:
    int key;              // function key used to invoke this mode
    int x_dots;           // number of dots across the screen
    int y_dots;           // number of dots down the screen
    int colors;           // number of colors available
    misc::Driver *driver; // associated driver for this mode
    char comment[26];     // Comments (UNTESTED, etc)
};

inline bool operator==(const VideoInfo &lhs, const VideoInfo &rhs)
{
    return lhs.key == rhs.key       //
        && lhs.x_dots == rhs.x_dots //
        && lhs.y_dots == rhs.y_dots //
        && lhs.colors == rhs.colors //
        && lhs.driver == rhs.driver //
        && std::strcmp(lhs.comment, rhs.comment) == 0;
}

inline bool operator!=(const VideoInfo &lhs, const VideoInfo &rhs)
{
    return !(lhs == rhs);
}

extern int g_adapter;             // index into g_video_table[]
extern int g_colors;              // maximum colors available
extern int g_screen_x_dots;       // # of dots on the physical screen
extern int g_screen_y_dots;       //
extern VideoInfo g_video_entry;   //
extern VideoInfo g_video_table[]; //
extern int g_video_table_len;     //

} // namespace id::ui
