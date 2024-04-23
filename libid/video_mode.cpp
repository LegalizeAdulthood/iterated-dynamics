#include "video_mode.h"

#include "port.h"

#include "id.h"

#include <cstdio>

/* g_video_table
 *
 *  |--Adapter/Mode-Name------|-------Comments-----------|
 *  |------INT 10H------|Dot-|--Resolution---|
 *  |key|--AX---BX---CX---DX|Mode|--X-|--Y-|Color|
 */
VIDEOINFO g_video_table[MAX_VIDEO_MODES] = { 0 };

int g_video_table_len;                 // number of entries in above

int check_vidmode_key(int option, int k)
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

int check_vidmode_keyname(char const *kname)
{
    // returns key number for the passed keyname, 0 if not a keyname
    int i;
    int keyset;
    keyset = 1058;
    if (*kname == 'S' || *kname == 's')
    {
        keyset = 1083;
        ++kname;
    }
    else if (*kname == 'C' || *kname == 'c')
    {
        keyset = 1093;
        ++kname;
    }
    else if (*kname == 'A' || *kname == 'a')
    {
        keyset = 1103;
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
    i = *kname - '0';
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

void vidmode_keyname(int k, char *buf)
{
    // set buffer to name of passed key number
    *buf = 0;
    if (k > 0)
    {
        if (k > 1103)
        {
            *(buf++) = 'A';
            k -= 1103;
        }
        else if (k > 1093)
        {
            *(buf++) = 'C';
            k -= 1093;
        }
        else if (k > 1083)
        {
            *(buf++) = 'S';
            k -= 1083;
        }
        else
        {
            k -= 1058;
        }
        std::sprintf(buf, "F%d", k);
    }
}
