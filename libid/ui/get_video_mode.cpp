// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
// Select a pending image's video mode and scaling without changing it.
#include "ui/get_video_mode.h"

#include "engine/calc_frac_init.h"
#include "engine/cmdfiles.h"
#include "engine/video_mode.h"
#include "engine/VideoInfo.h"
#include "engine/Viewport.h"
#include "fractals/fractalp.h"
#include "fractals/lorenz.h"
#include "io/gif_extensions.h"
#include "io/loadfile.h"
#include "io/trim_filename.h"
#include "misc/Driver.h"
#include "misc/ValueSaver.h"
#include "misc/version.h"
#include "ui/big_while_loop.h"
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
static double video_aspect(const VideoInfo &video, int try_x_dots, int try_y_dots);

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
    const VideoInfo &video{g_video_table[i]};
    std::string key_name = vid_mode_key_name(video.key);
    *fmt::format_to(buf, "{:<5s} {:<16s} {:<4s} {:5d} {:5d} {:3d} {:<25s}", // 67 chars
        key_name, video.driver->get_description(), err,                     //
        video.x_dots, video.y_dots,                                         //
        video.colors, video.comment) = '\0';
}

static double video_aspect(const VideoInfo &video, const int try_x_dots, const int try_y_dots)
{
    // calc resulting aspect ratio for specified dots in current mode
    return static_cast<double>(try_y_dots) / static_cast<double>(try_x_dots)
           * static_cast<double>(video.x_dots) / static_cast<double>(video.y_dots)
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
        const char *name_ptr = get_fractal_specific(migrate_integer_types(info->fractal_type))->name;
        if (info->info_version > 4 && info->display_3d > 0)
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

std::optional<ImageVideoMode> get_video_mode(const ImageMetadata &metadata, const Viewport &viewport)
{
    const FractalInfo *info{&metadata.fractal};
    const ExtBlock3 *blk_3_info{&metadata.formula};
    ImageVideoMode setup;
    setup.viewport = viewport;
    setup.width = metadata.width;
    setup.height = metadata.height;
    setup.aspect_ratio = metadata.aspect_ratio;
    setup.calc_status =
        info->info_version > 3 ? static_cast<CalcStatus>(info->calc_status) : CalcStatus::PARAMS_CHANGED;
    if (info->info_version > 4)
    {
        if (info->pot16bit)
        {
            setup.width >>= 1;
        }
        setup.aspect_ratio = info->final_aspect_ratio < 0.01f ? g_screen_aspect : info->final_aspect_ratio;
    }
    double f_temp;

    // try to find exact match for vid mode: first look for non-disk video
    const VideoInfo *begin = g_video_table;
    const VideoInfo *end = g_video_table + g_video_table_len;
    const VideoInfo *it = std::find_if(begin, end,
        [=](const VideoInfo &mode)
        {
            return info->x_dots == mode.x_dots    //
                && info->y_dots == mode.y_dots    //
                && metadata.colors == mode.colors //
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
                    && metadata.colors == mode.colors;
            });
    }
    if (it != end)
    {
        setup.mode = static_cast<int>(it - begin);
    }

    // exit in makepar mode if no exact match of video mode in file
    if (g_make_parameter_file && setup.mode == -1)
    {
        return setup;
    }

    // setup table entry for each vid mode, flagged for how well it matches
    s_video_choices.resize(g_video_table_len);
    for (int i = 0; i < g_video_table_len; ++i)
    {
        setup.video = g_video_table[i];
        unsigned tmp_flags = VI_EXACT;
        if (setup.video.driver != nullptr && setup.video.driver->is_disk())
        {
            tmp_flags |= VI_DISK;
        }
        if (setup.video.key == 0)
        {
            tmp_flags |= VI_NO_KEY;
        }
        if (info->x_dots > setup.video.x_dots || info->y_dots > setup.video.y_dots)
        {
            tmp_flags |= VI_SCREEN_SMALLER;
        }
        else if (info->x_dots < setup.video.x_dots || info->y_dots < setup.video.y_dots)
        {
            tmp_flags |= VI_SCREEN_BIGGER;
        }
        if (setup.width > setup.video.x_dots || setup.height > setup.video.y_dots)
        {
            tmp_flags |= VI_VIEW_SMALLER;
        }
        else if (setup.width < setup.video.x_dots || setup.height < setup.video.y_dots)
        {
            tmp_flags |= VI_VIEW_BIGGER;
        }
        if (metadata.colors > setup.video.colors)
        {
            tmp_flags |= VI_COLORS_SMALLER;
        }
        if (metadata.colors < setup.video.colors)
        {
            tmp_flags |= VI_COLORS_BIGGER;
        }
        if (i == setup.mode)
        {
            tmp_flags -= VI_EXACT;
        }
        if (setup.aspect_ratio != 0.0f && (tmp_flags & VI_VIEW_SMALLER) == 0)
        {
            f_temp = video_aspect(setup.video, setup.width, setup.height);
            if (f_temp < setup.aspect_ratio * 0.98 || f_temp > setup.aspect_ratio * 1.02)
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
        setup.mode = g_adapter;
    }

    bool got_real_mode = false;
    if ((setup.mode < 0 || (g_ask_video && g_init_batch == BatchMode::NONE)) && !g_make_parameter_file)
    {
        // no exact match or (askvideo=yes and batch=no), and not in makepar mode, talk to user
        std::sort(s_video_choices.begin(), s_video_choices.end(), video_choice_less);

        std::vector attributes(g_video_table_len, 1);

        // format heading
        std::string heading{fmt::format("File: {:<44s}  {:d} x {:d} x {:d}\n"
                                        "{:<52s}",
            trim_filename(g_read_filename, 44),          //
            setup.width, setup.height, metadata.colors, //
            heading_detail(info, blk_3_info))};
        if (info->info_id[0] != 'G')
        {
            if (info->system)
            {
                heading += "Id       ";
            }
            heading += to_display_string(fractal_info_version(*info));
        }
        heading += '\n';
        if (info->info_id[0] != 'G' && info->system == +SaveSystem::DOS)
        {
            if (setup.mode < 0)
            {
                heading += "Saved in unknown video mode.";
            }
            else
            {
                char buff[80];
                format_video_choice(setup.mode, "", buff);
                heading += buff;
            }
        }
        if (setup.aspect_ratio != 0 && setup.aspect_ratio != g_screen_aspect)
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
            return std::nullopt;
        }
        if (i < 0)  // returned -100 - g_video_table entry number
        {
            setup.mode = -100 - i;
            got_real_mode = true;
        }
        else
        {
            setup.mode = s_video_choices[i].entry_num;
        }
    }

    const VideoInfo selected{g_video_table[setup.mode]};
    if (!got_real_mode)  // translate from temp table to permanent
    {
        int i = setup.mode;
        int j = g_video_table[i].key;
        if (j != 0)
        {
            for (setup.mode = 0; setup.mode < MAX_VIDEO_MODES-1; ++setup.mode)
            {
                if (g_video_table[setup.mode].key == j)
                {
                    break;
                }
            }
            if (setup.mode >= MAX_VIDEO_MODES-1)
            {
                j = 0;
            }
        }
        if (j == 0) // mode has no key, add to reserved slot at end
        {
            setup.mode = MAX_VIDEO_MODES - 1;
        }
    }

    // ok, we're going to return with a video mode
    setup.video = selected;

    if (setup.viewport.enabled               //
        && setup.width == setup.video.x_dots //
        && setup.height == setup.video.y_dots)
    {
        // pull image into a view window
        if (setup.calc_status != CalcStatus::COMPLETED) // if not complete
        {
            setup.calc_status = CalcStatus::PARAMS_CHANGED;  // can't resume anyway
        }
        if (setup.viewport.x_dots)
        {
            setup.viewport.reduction = static_cast<float>(setup.video.x_dots / setup.viewport.x_dots);
            setup.viewport.y_dots = 0;
            setup.viewport.x_dots = 0;                                   // easier to use auto reduction
        }
        setup.viewport.reduction = std::round(setup.viewport.reduction); // need integer value
        setup.skip_y = static_cast<int>(setup.viewport.reduction - 1);
        setup.skip_x = static_cast<int>(setup.viewport.reduction - 1);
        return setup;
    }

    setup.skip_y = 0;
    setup.skip_x = 0; // set for no reduction
    if (setup.aspect_ratio == 0)
    {
        setup.aspect_ratio = static_cast<float>(video_aspect(setup.video, setup.width, setup.height));
    }
    if (setup.video.x_dots < setup.width || setup.video.y_dots < setup.height)
    {
        // set up to load only every nth pixel to make image fit
        if (setup.calc_status != CalcStatus::COMPLETED) // if not complete
        {
            setup.calc_status = CalcStatus::PARAMS_CHANGED;  // can't resume anyway
        }
        setup.skip_y = 1;
        setup.skip_x = 1;
        while (setup.skip_x * setup.video.x_dots < setup.width)
        {
            ++setup.skip_x;
        }
        while (setup.skip_y * setup.video.y_dots < setup.height)
        {
            ++setup.skip_y;
        }
        int i = 0;
        int j = 0;
        int tmp_x_dots;
        int tmp_y_dots;
        while (true)
        {
            tmp_x_dots = (setup.width + setup.skip_x - 1) / setup.skip_x;
            tmp_y_dots = (setup.height + setup.skip_y - 1) / setup.skip_y;
            // reduce further if that improves aspect
            f_temp = video_aspect(setup.video, tmp_x_dots, tmp_y_dots);
            if (f_temp > setup.aspect_ratio)
            {
                if (j)
                {
                    break; // already reduced x, don't reduce y
                }
                const double f_temp2 =
                    video_aspect(setup.video, tmp_x_dots, (setup.height + setup.skip_y) / (setup.skip_y + 1));
                if (f_temp2 < setup.aspect_ratio && f_temp / setup.aspect_ratio * 0.9 <= setup.aspect_ratio / f_temp2)
                {
                    break; // further y reduction is worse
                }
                ++setup.skip_y;
                ++i;
            }
            else
            {
                if (i)
                {
                    break; // already reduced y, don't reduce x
                }
                const double f_temp2 =
                    video_aspect(setup.video, (setup.width + setup.skip_x) / (setup.skip_x + 1), tmp_y_dots);
                if (f_temp2 > setup.aspect_ratio && setup.aspect_ratio / f_temp * 0.9 <= f_temp2 / setup.aspect_ratio)
                {
                    break; // further x reduction is worse
                }
                ++setup.skip_x;
                ++j;
            }
        }
        setup.width = tmp_x_dots;
        setup.height = tmp_y_dots;
        --setup.skip_x;
        --setup.skip_y;
    }

    setup.viewport.final_aspect_ratio = setup.aspect_ratio;
    if (setup.viewport.final_aspect_ratio == 0) // assume display correct
    {
        setup.viewport.final_aspect_ratio = static_cast<float>(video_aspect(setup.video, setup.width, setup.height));
    }
    if (setup.viewport.final_aspect_ratio >= g_screen_aspect - 0.02 &&
        setup.viewport.final_aspect_ratio <= g_screen_aspect + 0.02)
    {
        setup.viewport.final_aspect_ratio = g_screen_aspect;
    }
    {
        int i = static_cast<int>(setup.viewport.final_aspect_ratio * 1000.0 + 0.5);
        setup.viewport.final_aspect_ratio = static_cast<float>(i / 1000.0); // chop precision to 3 decimals
    }

    // setup view window stuff
    setup.viewport.enabled = false;
    setup.viewport.x_dots = 0;
    setup.viewport.y_dots = 0;
    if (setup.width != setup.video.x_dots || setup.height != setup.video.y_dots)
    {
        // image not exactly same size as screen
        setup.viewport.enabled = true;
        f_temp = setup.viewport.final_aspect_ratio * static_cast<double>(setup.video.y_dots) /
            static_cast<double>(setup.video.x_dots) / g_screen_aspect;
        float tmp_reduce;
        int i;
        int j;
        if (setup.viewport.final_aspect_ratio <= g_screen_aspect)
        {
            i = static_cast<int>(std::lround(
                static_cast<double>(setup.video.x_dots) / static_cast<double>(setup.width) * 20.0));
            tmp_reduce = static_cast<float>(i / 20.0); // chop precision to nearest .05
            i = static_cast<int>(std::lround(static_cast<double>(setup.video.x_dots) / tmp_reduce));
            j = static_cast<int>(std::lround(static_cast<double>(i) * f_temp));
        }
        else
        {
            i = static_cast<int>(std::lround(
                static_cast<double>(setup.video.y_dots) / static_cast<double>(setup.height) * 20.0));
            tmp_reduce = static_cast<float>(i / 20.0); // chop precision to nearest .05
            j = static_cast<int>(std::lround(static_cast<double>(setup.video.y_dots) / tmp_reduce));
            i = static_cast<int>(std::lround(static_cast<double>(j) / f_temp));
        }
        if (i != setup.width || j != setup.height)  // too bad, must be explicit
        {
            setup.viewport.x_dots = setup.width;
            setup.viewport.y_dots = setup.height;
        }
        else
        {
            setup.viewport.reduction = tmp_reduce; // ok, this works
        }
    }
    if (!g_make_parameter_file
        && !g_fast_restore
        && g_init_batch == BatchMode::NONE
        && (std::abs(setup.viewport.final_aspect_ratio - g_screen_aspect) > .00001 || setup.viewport.x_dots != 0))
    {
        stop_msg(StopMsgFlags::NO_BUZZER,
            "Warning: <V>iew parameters are being set to non-standard values.\n"
            "Remember to reset them when finished with this image!");
    }
    return setup;
}

static std::string video_choice_error_flags(const unsigned flags)
{
    std::string result;
    result.reserve(7);
    if (flags & (VI_VIEW_SMALLER | VI_COLORS_SMALLER | VI_BAD_ASPECT))
    {
        result += '*';
    }
    if (flags & VI_DISK)
    {
        result += 'D';
    }
    if (flags & VI_VIEW_SMALLER)
    {
        result += 'R';
    }
    if (flags & VI_COLORS_SMALLER)
    {
        result += 'C';
    }
    if (flags & VI_BAD_ASPECT)
    {
        result += 'A';
    }
    if (flags & VI_VIEW_BIGGER)
    {
        result += 'v';
    }
    if (flags & VI_COLORS_BIGGER)
    {
        result += 'c';
    }
    return result;
}

static void format_item(const int choice, char *buf)
{
    const std::string err{video_choice_error_flags(s_video_choices[choice].flags)};
    format_video_choice(s_video_choices[choice].entry_num, err.c_str(), buf);
}

static int check_mode_key(const int key, int /*choice*/)
{
    const int i = check_vid_mode_key(key);
    return i >= 0 ? -100-i : 0;
}

} // namespace id::ui
