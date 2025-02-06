// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/temp_msg.h"

#include "engine/id_data.h"
#include "misc/Driver.h"
#include "ui/cmdfiles.h"
#include "ui/diskvid.h"
#include "ui/editpal.h"
#include "ui/find_special_colors.h"
#include "ui/video.h"

#include <config/port.h>

#include <cstdio>
#include <cstring>
#include <vector>

static std::vector<Byte> s_text_save;
static int s_text_x_dots{};
static int s_text_y_dots{};

int text_temp_msg(char const *msg)
{
    if (show_temp_msg(msg))
    {
        return -1;
    }

    driver_wait_key_pressed(false); // wait for a keystroke but don't eat it
    clear_temp_msg();
    return 0;
}

void free_temp_msg()
{
    s_text_save.clear();
}

bool show_temp_msg(char const *msg)
{
    static size_t size = 0;
    char buffer[41];

    std::strncpy(buffer, msg, 40);
    buffer[40] = 0; // ensure max message len of 40 chars
    if (driver_is_disk())  // disk video, screen in text mode, easy
    {
        dvid_status(0, buffer);
        return false;
    }
    if (g_first_init) // & cmdfiles hasn't finished 1st try
    {
        // TODO: handle this in an OS-specific way
        std::printf("%s\n", buffer);
        return false;
    }

    const int x_repeat = (g_screen_x_dots >= 640) ? 2 : 1;
    const int y_repeat = (g_screen_y_dots >= 300) ? 2 : 1;
    s_text_x_dots = (int) std::strlen(buffer) * x_repeat * 8;
    s_text_y_dots = y_repeat * 8;

    size = (long) s_text_x_dots * (long) s_text_y_dots;
    if (s_text_save.size() != size)
    {
        s_text_save.clear();
    }
    int save_screen_x_offset = g_logical_screen_x_offset;
    int save_screen_y_offset = g_logical_screen_y_offset;
    g_logical_screen_y_offset = 0;
    g_logical_screen_x_offset = 0;
    if (s_text_save.empty()) // only save screen first time called
    {
        s_text_save.resize(s_text_x_dots*s_text_y_dots);
        for (int i = 0; i < s_text_y_dots; ++i)
        {
            read_span(i, 0, s_text_x_dots-1, &s_text_save[s_text_x_dots*i]);
        }
    }

    find_special_colors(); // get g_color_dark & g_color_medium set
    driver_display_string(0, 0, g_color_medium, g_color_dark, buffer);
    g_logical_screen_x_offset = save_screen_x_offset;
    g_logical_screen_y_offset = save_screen_y_offset;

    return false;
}

void clear_temp_msg()
{
    if (driver_is_disk()) // disk video, easy
    {
        dvid_status(0, "");
    }
    else if (!s_text_save.empty())
    {
        int save_screen_x_offset = g_logical_screen_x_offset;
        int save_screen_y_offset = g_logical_screen_y_offset;
        g_logical_screen_y_offset = 0;
        g_logical_screen_x_offset = 0;
        for (int i = 0; i < s_text_y_dots; ++i)
        {
            write_span(i, 0, s_text_x_dots-1, &s_text_save[s_text_x_dots*i]);
        }
        if (!g_using_jiim)                // jiim frees memory with freetempmsg()
        {
            s_text_save.clear();
        }
        g_logical_screen_x_offset = save_screen_x_offset;
        g_logical_screen_y_offset = save_screen_y_offset;
    }
}
