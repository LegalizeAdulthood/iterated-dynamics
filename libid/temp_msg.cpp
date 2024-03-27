#include "temp_msg.h"

#include "port.h"
#include "prototyp.h"

#include "cmdfiles.h"
#include "diskvid.h"
#include "drivers.h"
#include "editpal.h"
#include "find_special_colors.h"
#include "get_line.h"
#include "id_data.h"
#include "os.h"
#include "zoom.h"

#include <cstdio>
#include <cstring>
#include <vector>

static std::vector<BYTE> temptextsave;
static int textxdots;
static int textydots;

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
    temptextsave.clear();
}

bool showtempmsg(char const *msgparm)
{
    static long size = 0;
    char msg[41];
    int xrepeat = 0;
    int yrepeat = 0;
    int save_sxoffs, save_syoffs;

    std::strncpy(msg, msgparm, 40);
    msg[40] = 0; // ensure max message len of 40 chars
    if (driver_diskp())  // disk video, screen in text mode, easy
    {
        dvid_status(0, msg);
        return false;
    }
    if (g_first_init)      // & cmdfiles hasn't finished 1st try
    {
        std::printf("%s\n", msg);
        return false;
    }

    xrepeat = (g_screen_x_dots >= 640) ? 2 : 1;
    yrepeat = (g_screen_y_dots >= 300) ? 2 : 1;
    textxdots = (int) std::strlen(msg) * xrepeat * 8;
    textydots = yrepeat * 8;

    size = (long) textxdots * (long) textydots;
    if (temptextsave.size() != size)
    {
        temptextsave.clear();
    }
    save_sxoffs = g_logical_screen_x_offset;
    save_syoffs = g_logical_screen_y_offset;
    if (g_video_scroll)
    {
        g_logical_screen_x_offset = g_video_start_x;
        g_logical_screen_y_offset = g_video_start_y;
    }
    else
    {
        g_logical_screen_y_offset = 0;
        g_logical_screen_x_offset = g_logical_screen_y_offset;
    }
    if (temptextsave.empty()) // only save screen first time called
    {
        temptextsave.resize(textxdots*textydots);
        for (int i = 0; i < textydots; ++i)
        {
            get_line(i, 0, textxdots-1, &temptextsave[textxdots*i]);
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
    else if (!temptextsave.empty())
    {
        int save_sxoffs = g_logical_screen_x_offset;
        int save_syoffs = g_logical_screen_y_offset;
        if (g_video_scroll)
        {
            g_logical_screen_x_offset = g_video_start_x;
            g_logical_screen_y_offset = g_video_start_y;
        }
        else
        {
            g_logical_screen_y_offset = 0;
            g_logical_screen_x_offset = g_logical_screen_y_offset;
        }
        for (int i = 0; i < textydots; ++i)
        {
            put_line(i, 0, textxdots-1, &temptextsave[textxdots*i]);
        }
        if (!g_using_jiim)                // jiim frees memory with freetempmsg()
        {
            temptextsave.clear();
        }
        g_logical_screen_x_offset = save_sxoffs;
        g_logical_screen_y_offset = save_syoffs;
    }
}
