// SPDX-License-Identifier: GPL-3.0-only
//
#include "engine/video_mode.h"

#include "engine/pixel_limits.h"
#include "engine/VideoInfo.h"
#include "ui/id_keys.h"

#include <cstdio>

using namespace id::ui;

namespace id::engine
{

int check_vid_mode_key(const int key)
{
    if (key == 1400)                // special value from select_vid_mode
    {
        return MAX_VIDEO_MODES-1; // for last entry with no key assigned
    }
    if (key != 0)
    {
        // check full g_video_table
        for (int i = 0; i < g_video_table_len; ++i)
        {
            if (g_video_table[i].key == key)
            {
                return i;
            }
        }
    }
    return -1;
}

int check_vid_mode_key_name(std::string_view key_name)
{
    int key_set = ID_KEY_F1 - 1;
    if (!key_name.empty() && (key_name.front() == 'S' || key_name.front() == 's'))
    {
        key_set = ID_KEY_SHF_F1 - 1;
        key_name.remove_prefix(1);
    }
    else if (!key_name.empty() && (key_name.front() == 'C' || key_name.front() == 'c'))
    {
        key_set = ID_KEY_CTL_F1 - 1;
        key_name.remove_prefix(1);
    }
    else if (!key_name.empty() && (key_name.front() == 'A' || key_name.front() == 'a'))
    {
        key_set = ID_KEY_ALT_F1 - 1;
        key_name.remove_prefix(1);
    }
    if (key_name.empty() || (key_name.front() != 'F' && key_name.front() != 'f'))
    {
        return 0;
    }
    key_name.remove_prefix(1);
    if (key_name.empty() || key_name.front() < '1' || key_name.front() > '9')
    {
        return 0;
    }
    int i = key_name.front() - '0';
    key_name.remove_prefix(1);
    if (!key_name.empty() && key_name.front() != ' ')
    {
        if (key_name.front() != '0' || i != 1)
        {
            return 0;
        }
        i = 10;
        key_name.remove_prefix(1);
    }
    while (!key_name.empty())
    {
        if (key_name.front() != ' ')
        {
            return 0;
        }
        key_name.remove_prefix(1);
    }
    if ((i += key_set) < 2)
    {
        i = 0;
    }
    return i;
}

void vid_mode_key_name(int key, char *buffer)
{
    *buffer = 0;
    if (key > 0)
    {
        if (key >= ID_KEY_ALT_F1 && key <= ID_KEY_ALT_F10)
        {
            *buffer++ = 'A';
            key -= ID_KEY_ALT_F1 - 1;
        }
        else if (key >= ID_KEY_CTL_F1 && key <= ID_KEY_CTL_F10)
        {
            *buffer++ = 'C';
            key -= ID_KEY_CTL_F1 - 1;
        }
        else if (key >= ID_KEY_SHF_F1 && key <= ID_KEY_SHF_F10)
        {
            *buffer++ = 'S';
            key -= ID_KEY_SHF_F1 - 1;
        }
        else if (key >= ID_KEY_F1 && key <= ID_KEY_F10)
        {
            key -= ID_KEY_F1 - 1;
        }
        else
        {
            return;
        }
        std::sprintf(buffer, "F%d", key);
    }
}

bool is_valid_disk_video_mode(const VideoInfo &mode)
{
    return mode.colors == 256              //
        && mode.x_dots > 0                 //
        && mode.y_dots > 0                 //
        && mode.x_dots <= GIF_MAX_PIXELS   //
        && mode.y_dots <= GIF_MAX_PIXELS;
}

bool is_valid_display_video_mode(const VideoInfo &mode, const int max_width, const int max_height)
{
    return mode.colors == 256         //
        && mode.x_dots > 0            //
        && mode.y_dots > 0            //
        && max_width > 0              //
        && max_height > 0             //
        && mode.x_dots <= max_width   //
        && mode.y_dots <= max_height;
}

} // namespace id::engine
