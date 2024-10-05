// SPDX-License-Identifier: GPL-3.0-only
//
#include "temp_msg.h"

#include "port.h"
#include "prototyp.h"

#include "cmdfiles.h"
#include "diskvid.h"
#include "drivers.h"
#include "editpal.h"
#include "find_special_colors.h"
#include "id_data.h"
#include "os.h"
#include "video.h"
#include "zoom.h"

#include <cstdio>
#include <cstring>
#include <vector>

static std::vector<BYTE> s_text_save;
static int s_text_x_dots{};
static int s_text_y_dots{};

/* texttempmsg(msg) displays a text message of up to 40 characters, waits
      for a key press, restores the prior display, and returns (without
      eating the key).
      It works in almost any video mode - does nothing in some very odd cases
      (HCGA hi-res with old bios), or when there isn't 10k of temp mem free. */
int texttempmsg(char const *msgparm)
{
    if (showtempmsg(msgparm))
    {
        return -1;
    }

    driver_wait_key_pressed(0); // wait for a keystroke but don't eat it
    cleartempmsg();
    return 0;
}

void freetempmsg()
{
    s_text_save.clear();
}

bool showtempmsg(char const *msgparm)
{
    static size_t size = 0;
    char msg[41];
    int save_sxoffs;
    int save_syoffs;

    std::strncpy(msg, msgparm, 40);
    msg[40] = 0; // ensure max message len of 40 chars
    if (driver_diskp())  // disk video, screen in text mode, easy
    {
        dvid_status(0, msg);
        return false;
    }
    if (g_first_init) // & cmdfiles hasn't finished 1st try
    {
        // TODO: handle this in an OS-specific way
        std::printf("%s\n", msg);
        return false;
    }

    const int xrepeat = (g_screen_x_dots >= 640) ? 2 : 1;
    const int yrepeat = (g_screen_y_dots >= 300) ? 2 : 1;
    s_text_x_dots = (int) std::strlen(msg) * xrepeat * 8;
    s_text_y_dots = yrepeat * 8;

    size = (long) s_text_x_dots * (long) s_text_y_dots;
    if (s_text_save.size() != size)
    {
        s_text_save.clear();
    }
    save_sxoffs = g_logical_screen_x_offset;
    save_syoffs = g_logical_screen_y_offset;
    g_logical_screen_y_offset = 0;
    g_logical_screen_x_offset = g_logical_screen_y_offset;
    if (s_text_save.empty()) // only save screen first time called
    {
        s_text_save.resize(s_text_x_dots*s_text_y_dots);
        for (int i = 0; i < s_text_y_dots; ++i)
        {
            read_span(i, 0, s_text_x_dots-1, &s_text_save[s_text_x_dots*i]);
        }
    }

    find_special_colors(); // get g_color_dark & g_color_medium set
    driver_display_string(0, 0, g_color_medium, g_color_dark, msg);
    g_logical_screen_x_offset = save_sxoffs;
    g_logical_screen_y_offset = save_syoffs;

    return false;
}

void cleartempmsg()
{
    if (driver_diskp()) // disk video, easy
    {
        dvid_status(0, "");
    }
    else if (!s_text_save.empty())
    {
        int save_sxoffs = g_logical_screen_x_offset;
        int save_syoffs = g_logical_screen_y_offset;
        g_logical_screen_y_offset = 0;
        g_logical_screen_x_offset = g_logical_screen_y_offset;
        for (int i = 0; i < s_text_y_dots; ++i)
        {
            write_span(i, 0, s_text_x_dots-1, &s_text_save[s_text_x_dots*i]);
        }
        if (!g_using_jiim)                // jiim frees memory with freetempmsg()
        {
            s_text_save.clear();
        }
        g_logical_screen_x_offset = save_sxoffs;
        g_logical_screen_y_offset = save_syoffs;
    }
}
