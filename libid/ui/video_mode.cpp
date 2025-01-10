// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/video_mode.h"

#include "id_keys.h"

#include <cstdio>

VideoInfo g_video_table[MAX_VIDEO_MODES]{};

int g_video_table_len{};                 // number of entries in above

int check_vid_mode_key(int key)
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

int check_vid_mode_key_name(char const *key_name)
{
    int key_set = ID_KEY_F1 - 1;
    if (*key_name == 'S' || *key_name == 's')
    {
        key_set = ID_KEY_SHF_F1 - 1;
        ++key_name;
    }
    else if (*key_name == 'C' || *key_name == 'c')
    {
        key_set = ID_KEY_CTL_F1 - 1;
        ++key_name;
    }
    else if (*key_name == 'A' || *key_name == 'a')
    {
        key_set = ID_KEY_ALT_F1 - 1;
        ++key_name;
    }
    if (*key_name != 'F' && *key_name != 'f')
    {
        return 0;
    }
    if (*++key_name < '1' || *key_name > '9')
    {
        return 0;
    }
    int i = *key_name - '0';
    if (*++key_name != 0 && *key_name != ' ')
    {
        if (*key_name != '0' || i != 1)
        {
            return 0;
        }
        i = 10;
        ++key_name;
    }
    while (*key_name)
    {
        if (*(key_name++) != ' ')
        {
            return 0;
        }
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
