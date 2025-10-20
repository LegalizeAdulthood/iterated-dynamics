// SPDX-License-Identifier: GPL-3.0-only
//
/*
    subroutine of which sets up video (mode, screen size).

    get_video_mode should return with:
      return code 0 for ok, -1 for error or cancelled by user
      video parameters setup for the mainline, in the dos case this means
        setting g_init_mode to video mode, based on this code will set up
        for and call setvideomode
      set viewwindow true if file going to be loaded into a view smaller than
        physical screen, in this case also set viewreduction, viewxdots,
        viewydots, and finalaspectratio
      set skipxdots and skipydots, to 0 if all pixels are to be loaded,
        to 1 for every 2nd pixel, 2 for every 3rd, etc.

*/
#include "ui/get_video_mode.h"

#include "engine/cmdfiles.h"
#include "engine/video_mode.h"
#include "engine/VideoInfo.h"
#include "engine/Viewport.h"
#include "fractals/fractalp.h"
#include "io/loadfile.h"
#include "io/trim_filename.h"
#include "misc/Driver.h"
#include "misc/ValueSaver.h"
#include "misc/version.h"
#include "ui/full_screen_choice.h"
#include "ui/help.h"
#include "ui/make_batch_file.h"
#include "ui/stop_msg.h"

#include <fmt/format.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

using namespace id::engine;
using namespace id::fractals;
using namespace id::help;
using namespace id::io;
using namespace id::misc;

namespace id::ui
{

/* defines for flags; done this way instead of bit union to ensure ordering;
   these bits represent the sort sequence for video mode list */
enum
{
    VI_EXACT = 0x8000,       // unless the one and only exact match
    VI_DISK = 1024,          // if video mode is disk video
    VI_NO_KEY = 512,         // if no function key assigned
    VI_SCREEN_SMALLER = 128, // screen smaller than file's screen
    VI_SCREEN_BIGGER = 64,   // screen bigger than file's screen
    VI_VIEW_SMALLER = 32,    // screen smaller than file's view
    VI_VIEW_BIGGER = 16,     // screen bigger than file's view
    VI_COLORS_SMALLER = 8,   // mode has too few colors
    VI_COLORS_BIGGER = 4,    // mode has excess colors
    VI_BAD_ASPECT = 1        // aspect ratio bad
};

namespace
{

struct VideoModeChoice
{
    int entry_num;  // g_video_entry subscript
    unsigned flags; // flags for sort's compare, defined below
};

} // namespace

static std::vector<VideoModeChoice> s_video_choices;

static void format_item(int choice, char *buf);
static int    check_mode_key(int key, int /*choice*/);
static void   format_video_choice(int i, const char *err, char *buf);
static double video_aspect(int try_x_dots, int try_y_dots);

static bool video_choice_less(const VideoModeChoice &lhs, const VideoModeChoice &rhs)
{
    if (lhs.flags < rhs.flags)
    {
        return true;
    }
    if (lhs.flags > rhs.flags)
    {
        return false;
    }
    if (g_video_table[lhs.entry_num].key < g_video_table[rhs.entry_num].key)
    {
        return true;
    }
    if (g_video_table[lhs.entry_num].key > g_video_table[rhs.entry_num].key)
    {
        return false;
    }
    if (lhs.entry_num < rhs.entry_num)
    {
        return true;
    }
    return false;
}

static void format_video_choice(const int i, const char *err, char *buf)
{
    g_video_entry = g_video_table[i];
    std::string key_name = vid_mode_key_name(g_video_entry.key);
    *fmt::format_to(buf, "{:<5s} {:<16s} {:<4s} {:5d} {:5d} {:3d} {:<25s}", // 67 chars
        key_name, g_video_entry.driver->get_description(), err,             //
        g_video_entry.x_dots, g_video_entry.y_dots,                         //
        g_video_entry.colors, g_video_entry.comment) = '\0';
    g_video_entry.x_dots = 0;                                               // so tab_display knows to display nothing
}

static double video_aspect(const int try_x_dots, const int try_y_dots)
{
    // calc resulting aspect ratio for specified dots in current mode
    return static_cast<double>(try_y_dots) / static_cast<double>(try_x_dots)
           * static_cast<double>(g_video_entry.x_dots) / static_cast<double>(g_video_entry.y_dots)
           * g_screen_aspect;
}

static std::string heading_detail(const FractalInfo *info, const ExtBlock3 *blk_3_info)
{
    std::ostringstream result;
    if (info->info_id[0] == 'G')
    {
        result << "      Non-fractal GIF";
    }
    else
    {
        const char *name_ptr = g_cur_fractal_specific->name;
        if (g_display_3d != Display3DMode::NONE)
        {
            name_ptr = "3D Transform";
        }
        result << "Type: " << name_ptr;
        if (std::strcmp(name_ptr, "formula") == 0     //
            || std::strcmp(name_ptr, "lsystem") == 0  //
            || std::strncmp(name_ptr, "ifs", 3) == 0) // for ifs and ifs3d
        {
            result << " -> " << blk_3_info->form_name;
        }
    }
    return result.str();
}

int get_video_mode(FractalInfo *info, ExtBlock3 *blk_3_info)
{
    double f_temp;

    g_init_mode = -1;

    // try to find exact match for vid mode: first look for non-disk video
    const VideoInfo *begin = g_video_table;
    const VideoInfo *end = g_video_table + g_video_table_len;
    auto it = std::find_if(begin, end,
        [=](const VideoInfo &mode)
        {
            return info->x_dots == mode.x_dots    //
                && info->y_dots == mode.y_dots    //
                && g_file_colors == mode.colors //
                && mode.driver != nullptr       //
                && !mode.driver->is_disk();
        });
    if (it == end)
    {
        it = std::find_if(begin, end,
            [=](const VideoInfo &mode)
            {
                return info->x_dots == mode.x_dots //
                    && info->y_dots == mode.y_dots //
                    && g_file_colors == mode.colors;
            });
    }
    if (it != end)
    {
        g_init_mode = static_cast<int>(it - begin);
    }

    // exit in makepar mode if no exact match of video mode in file
    if (g_make_parameter_file && g_init_mode == -1)
    {
        return 0;
    }

    // setup table entry for each vid mode, flagged for how well it matches
    s_video_choices.resize(g_video_table_len);
    for (int i = 0; i < g_video_table_len; ++i)
    {
        g_video_entry = g_video_table[i];
        unsigned tmp_flags = VI_EXACT;
        if (g_video_entry.driver != nullptr && g_video_entry.driver->is_disk())
        {
            tmp_flags |= VI_DISK;
        }
        if (g_video_entry.key == 0)
        {
            tmp_flags |= VI_NO_KEY;
        }
        if (info->x_dots > g_video_entry.x_dots || info->y_dots > g_video_entry.y_dots)
        {
            tmp_flags |= VI_SCREEN_SMALLER;
        }
        else if (info->x_dots < g_video_entry.x_dots || info->y_dots < g_video_entry.y_dots)
        {
            tmp_flags |= VI_SCREEN_BIGGER;
        }
        if (g_file_x_dots > g_video_entry.x_dots || g_file_y_dots > g_video_entry.y_dots)
        {
            tmp_flags |= VI_VIEW_SMALLER;
        }
        else if (g_file_x_dots < g_video_entry.x_dots || g_file_y_dots < g_video_entry.y_dots)
        {
            tmp_flags |= VI_VIEW_BIGGER;
        }
        if (g_file_colors > g_video_entry.colors)
        {
            tmp_flags |= VI_COLORS_SMALLER;
        }
        if (g_file_colors < g_video_entry.colors)
        {
            tmp_flags |= VI_COLORS_BIGGER;
        }
        if (i == g_init_mode)
        {
            tmp_flags -= VI_EXACT;
        }
        if (g_file_aspect_ratio != 0.0f && (tmp_flags & VI_VIEW_SMALLER) == 0)
        {
            f_temp = video_aspect(g_file_x_dots, g_file_y_dots);
            if (f_temp < g_file_aspect_ratio * 0.98
                || f_temp > g_file_aspect_ratio * 1.02)
            {
                tmp_flags |= VI_BAD_ASPECT;
            }
        }
        s_video_choices[i].entry_num = i;
        // cppcheck-suppress unreadVariable
        s_video_choices[i].flags  = tmp_flags;
    }

    if (g_fast_restore  && !g_ask_video)
    {
        g_init_mode = g_adapter;
    }

    bool got_real_mode = false;
    if ((g_init_mode < 0 || (g_ask_video && g_init_batch == BatchMode::NONE)) && !g_make_parameter_file)
    {
        // no exact match or (askvideo=yes and batch=no), and not in makepar mode, talk to user
        std::sort(s_video_choices.begin(), s_video_choices.end(), video_choice_less);

        std::vector attributes(g_video_table_len, 1);

        // format heading
        std::string heading{fmt::format("File: {:<44s}  {:d} x {:d} x {:d}\n"
                                        "{:<52s}",
            trim_filename(g_read_filename, 44),          //
            g_file_x_dots, g_file_y_dots, g_file_colors, //
            heading_detail(info, blk_3_info))};
        if (info->info_id[0] != 'G')
        {
            if (info->system)
            {
                heading += "Id       ";
            }
            heading += to_display_string(g_file_version);
        }
        heading += '\n';
        if (info->info_id[0] != 'G' && info->system == +SaveSystem::DOS)
        {
            if (g_init_mode < 0)
            {
                heading += "Saved in unknown video mode.";
            }
            else
            {
                char buff[80];
                format_video_choice(g_init_mode, "", buff);
                heading += buff;
            }
        }
        if (g_file_aspect_ratio != 0 && g_file_aspect_ratio != g_screen_aspect)
        {
            heading += "\n"
                       "WARNING: non-standard aspect ratio; loading will change your <v>iew settings";
        }
        heading += '\n';
        // set up instructions
        std::string instructions{"Select a video mode.  Use the cursor keypad to move the pointer.\n"
               "Press ENTER for selected mode, or use a video mode function key.\n"
               "Press F1 for help, "};
        if (info->info_id[0] != 'G')
        {
            instructions += "TAB for fractal information, ";
        }
        instructions += "ESCAPE to back out.";

        int i;
        {
            ValueSaver saved_help_mode{g_help_mode, HelpLabels::HELP_LOAD_FILE};
            i = full_screen_choice(ChoiceFlags::NONE, heading.c_str(),
                "key...name......................err...xdot..ydot.clr.comment..................",
                instructions.c_str(), g_video_table_len, nullptr, attributes.data(), 1, 13, 78, 0, format_item,
                nullptr, nullptr, check_mode_key);
        }
        if (i == -1)
        {
            return -1;
        }
        if (i < 0)  // returned -100 - g_video_table entry number
        {
            g_init_mode = -100 - i;
            got_real_mode = true;
        }
        else
        {
            g_init_mode = s_video_choices[i].entry_num;
        }
    }

    if (!got_real_mode)  // translate from temp table to permanent
    {
        int i = g_init_mode;
        int j = g_video_table[i].key;
        if (j != 0)
        {
            for (g_init_mode = 0; g_init_mode < MAX_VIDEO_MODES-1; ++g_init_mode)
            {
                if (g_video_table[g_init_mode].key == j)
                {
                    break;
                }
            }
            if (g_init_mode >= MAX_VIDEO_MODES-1)
            {
                j = 0;
            }
        }
        if (j == 0) // mode has no key, add to reserved slot at end
        {
            std::memcpy(
                &g_video_table[g_init_mode = MAX_VIDEO_MODES - 1],
                   &g_video_table[i], sizeof(*g_video_table));
        }
    }

    // ok, we're going to return with a video mode
    g_video_entry = g_video_table[g_init_mode];

    if (g_viewport.enabled                       //
        && g_file_x_dots == g_video_entry.x_dots //
        && g_file_y_dots == g_video_entry.y_dots)
    {
        // pull image into a view window
        if (g_calc_status != CalcStatus::COMPLETED) // if not complete
        {
            g_calc_status = CalcStatus::PARAMS_CHANGED;  // can't resume anyway
        }
        if (g_viewport.x_dots)
        {
            g_viewport.reduction = static_cast<float>(g_video_entry.x_dots / g_viewport.x_dots);
            g_viewport.y_dots = 0;
            g_viewport.x_dots = 0; // easier to use auto reduction
        }
        g_viewport.reduction = std::round(g_viewport.reduction); // need integer value
        g_skip_y_dots = static_cast<short>(g_viewport.reduction - 1);
        g_skip_x_dots = static_cast<short>(g_viewport.reduction - 1);
        return 0;
    }

    g_skip_y_dots = 0;
    g_skip_x_dots = 0; // set for no reduction
    if (g_video_entry.x_dots < g_file_x_dots || g_video_entry.y_dots < g_file_y_dots)
    {
        // set up to load only every nth pixel to make image fit
        if (g_calc_status != CalcStatus::COMPLETED) // if not complete
        {
            g_calc_status = CalcStatus::PARAMS_CHANGED;  // can't resume anyway
        }
        g_skip_y_dots = 1;
        g_skip_x_dots = 1;
        while (g_skip_x_dots * g_video_entry.x_dots < g_file_x_dots)
        {
            ++g_skip_x_dots;
        }
        while (g_skip_y_dots * g_video_entry.y_dots < g_file_y_dots)
        {
            ++g_skip_y_dots;
        }
        int i = 0;
        int j = 0;
        int tmp_x_dots;
        int tmp_y_dots;
        while (true)
        {
            tmp_x_dots = (g_file_x_dots + g_skip_x_dots - 1) / g_skip_x_dots;
            tmp_y_dots = (g_file_y_dots + g_skip_y_dots - 1) / g_skip_y_dots;
            // reduce further if that improves aspect
            f_temp = video_aspect(tmp_x_dots, tmp_y_dots);
            if (f_temp > g_file_aspect_ratio)
            {
                if (j)
                {
                    break; // already reduced x, don't reduce y
                }
                const double f_temp2 = video_aspect(tmp_x_dots, (g_file_y_dots+g_skip_y_dots)/(g_skip_y_dots+1));
                if (f_temp2 < g_file_aspect_ratio
                    && f_temp/g_file_aspect_ratio *0.9 <= g_file_aspect_ratio/f_temp2)
                {
                    break; // further y reduction is worse
                }
                ++g_skip_y_dots;
                ++i;
            }
            else
            {
                if (i)
                {
                    break; // already reduced y, don't reduce x
                }
                const double f_temp2 = video_aspect((g_file_x_dots+g_skip_x_dots)/(g_skip_x_dots+1), tmp_y_dots);
                if (f_temp2 > g_file_aspect_ratio
                    && g_file_aspect_ratio/f_temp *0.9 <= f_temp2/g_file_aspect_ratio)
                {
                    break; // further x reduction is worse
                }
                ++g_skip_x_dots;
                ++j;
            }
        }
        g_file_x_dots = tmp_x_dots;
        g_file_y_dots = tmp_y_dots;
        --g_skip_x_dots;
        --g_skip_y_dots;
    }

    g_viewport.final_aspect_ratio = g_file_aspect_ratio;
    if (g_viewport.final_aspect_ratio == 0) // assume display correct
    {
        g_viewport.final_aspect_ratio = static_cast<float>(video_aspect(g_file_x_dots, g_file_y_dots));
    }
    if (g_viewport.final_aspect_ratio >= g_screen_aspect-0.02
        && g_viewport.final_aspect_ratio <= g_screen_aspect+0.02)
    {
        g_viewport.final_aspect_ratio = g_screen_aspect;
    }
    {
        int i = static_cast<int>(g_viewport.final_aspect_ratio * 1000.0 + 0.5);
        g_viewport.final_aspect_ratio = static_cast<float>(i / 1000.0); // chop precision to 3 decimals
    }

    // setup view window stuff
    g_viewport.enabled = false;
    g_viewport.x_dots = 0;
    g_viewport.y_dots = 0;
    if (g_file_x_dots != g_video_entry.x_dots || g_file_y_dots != g_video_entry.y_dots)
    {
        // image not exactly same size as screen
        g_viewport.enabled = true;
        f_temp = g_viewport.final_aspect_ratio
                * static_cast<double>(g_video_entry.y_dots) / static_cast<double>(g_video_entry.x_dots)
                / g_screen_aspect;
        float tmp_reduce;
        int i;
        int j;
        if (g_viewport.final_aspect_ratio <= g_screen_aspect)
        {
            i = static_cast<int>(std::lround(
                static_cast<double>(g_video_entry.x_dots) / static_cast<double>(g_file_x_dots) * 20.0));
            tmp_reduce = static_cast<float>(i / 20.0); // chop precision to nearest .05
            i = static_cast<int>(std::lround(static_cast<double>(g_video_entry.x_dots) / tmp_reduce));
            j = static_cast<int>(std::lround(static_cast<double>(i) * f_temp));
        }
        else
        {
            i = static_cast<int>(std::lround(
                static_cast<double>(g_video_entry.y_dots) / static_cast<double>(g_file_y_dots) * 20.0));
            tmp_reduce = static_cast<float>(i / 20.0); // chop precision to nearest .05
            j = static_cast<int>(std::lround(static_cast<double>(g_video_entry.y_dots) / tmp_reduce));
            i = static_cast<int>(std::lround(static_cast<double>(j) / f_temp));
        }
        if (i != g_file_x_dots || j != g_file_y_dots)  // too bad, must be explicit
        {
            g_viewport.x_dots = g_file_x_dots;
            g_viewport.y_dots = g_file_y_dots;
        }
        else
        {
            g_viewport.reduction = tmp_reduce; // ok, this works
        }
    }
    if (!g_make_parameter_file
        && !g_fast_restore
        && g_init_batch == BatchMode::NONE
        && (std::abs(g_viewport.final_aspect_ratio - g_screen_aspect) > .00001 || g_viewport.x_dots != 0))
    {
        stop_msg(StopMsgFlags::NO_BUZZER,
            "Warning: <V>iew parameters are being set to non-standard values.\n"
            "Remember to reset them when finished with this image!");
    }
    return 0;
}

static void format_item(const int choice, char *buf)
{
    char err_buf[10];
    err_buf[0] = 0;
    const unsigned tmp_flags = s_video_choices[choice].flags;
    if (tmp_flags & VI_VIEW_SMALLER + VI_COLORS_SMALLER + VI_BAD_ASPECT)
    {
        std::strcat(err_buf, "*");
    }
    if (tmp_flags & VI_DISK)
    {
        std::strcat(err_buf, "D");
    }
    if (tmp_flags & VI_VIEW_SMALLER)
    {
        std::strcat(err_buf, "R");
    }
    if (tmp_flags & VI_COLORS_SMALLER)
    {
        std::strcat(err_buf, "C");
    }
    if (tmp_flags & VI_BAD_ASPECT)
    {
        std::strcat(err_buf, "A");
    }
    if (tmp_flags & VI_VIEW_BIGGER)
    {
        std::strcat(err_buf, "v");
    }
    if (tmp_flags & VI_COLORS_BIGGER)
    {
        std::strcat(err_buf, "c");
    }
    format_video_choice(s_video_choices[choice].entry_num, err_buf, buf);
}

static int check_mode_key(const int key, int /*choice*/)
{
    const int i = check_vid_mode_key(key);
    return i >= 0 ? -100-i : 0;
}

} // namespace id::ui
