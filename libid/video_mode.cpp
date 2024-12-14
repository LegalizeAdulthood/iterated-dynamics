// SPDX-License-Identifier: GPL-3.0-only
//
#include "video_mode.h"

#include "port.h"

#include "id.h"
#include "id_keys.h"

#include <cstdio>

VideoInfo g_video_table[MAX_VIDEO_MODES]{};

int g_video_table_len{};                 // number of entries in above

int check_vid_mode_key(int option, int k)
{
    // returns g_video_table entry number if the passed keystroke is a
    // function key currently assigned to a video mode, -1 otherwise
    if (k == 1400)                // special value from select_vid_mode
    {
        return MAX_VIDEO_MODES-1; // for last entry with no key assigned
    }
    if (k != 0)
    {
        if (option == 0)
        {
            // check resident video mode table
            for (int i = 0; i < MAX_VIDEO_MODES; ++i)
            {
                if (g_video_table[i].keynum == k)
                {
                    return i;
                }
            }
        }
        else
        {
            // check full g_video_table
            for (int i = 0; i < g_video_table_len; ++i)
            {
                if (g_video_table[i].keynum == k)
                {
                    return i;
                }
            }
        }
    }
    return -1;
}

// returns key number for the passed keyname, 0 if not a keyname
int check_vid_mode_key_name(char const *kname)
{
    int keyset = ID_KEY_F1 - 1;
    if (*kname == 'S' || *kname == 's')
    {
        keyset = ID_KEY_SF1 - 1;
        ++kname;
    }
    else if (*kname == 'C' || *kname == 'c')
    {
        keyset = ID_KEY_CTL_F1 - 1;
        ++kname;
    }
    else if (*kname == 'A' || *kname == 'a')
    {
        keyset = ID_KEY_ALT_F1 - 1;
        ++kname;
    }
    if (*kname != 'F' && *kname != 'f')
    {
        return 0;
    }
    if (*++kname < '1' || *kname > '9')
    {
        return 0;
    }
    int i = *kname - '0';
    if (*++kname != 0 && *kname != ' ')
    {
        if (*kname != '0' || i != 1)
        {
            return 0;
        }
        i = 10;
        ++kname;
    }
    while (*kname)
    {
        if (*(kname++) != ' ')
        {
            return 0;
        }
    }
    if ((i += keyset) < 2)
    {
        i = 0;
    }
    return i;
}

void vid_mode_key_name(int k, char *buf)
{
    // set buffer to name of passed key number
    *buf = 0;
    if (k > 0)
    {
        if (k >= ID_KEY_ALT_F1)
        {
            *(buf++) = 'A';
            k -= ID_KEY_ALT_F1 - 1;
        }
        else if (k >= ID_KEY_CTL_F1)
        {
            *(buf++) = 'C';
            k -= ID_KEY_CTL_F1 - 1;
        }
        else if (k > ID_KEY_SF1 - 1)
        {
            *(buf++) = 'S';
            k -= ID_KEY_SF1 - 1;
        }
        else
        {
            k -= ID_KEY_F1 - 1;
        }
        std::sprintf(buf, "F%d", k);
    }
}
