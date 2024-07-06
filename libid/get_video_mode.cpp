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
        to 1 for every 2nd pixel, 2 for every 3rd, etc

*/
#include "port.h"
#include "prototyp.h"

#include "get_video_mode.h"

#include "cmdfiles.h"
#include "drivers.h"
#include "fractalp.h"
#include "full_screen_choice.h"
#include "helpdefs.h"
#include "id_data.h"
#include "loadfile.h"
#include "make_batch_file.h"
#include "stop_msg.h"
#include "trim_filename.h"
#include "value_saver.h"
#include "version.h"
#include "video_mode.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <string>

// routines in this module

static void   format_item(int, char *);
static int    check_modekey(int, int);
static void   format_vid_inf(int i, char const *err, char *buf);
static double vid_aspect(int tryxdots, int tryydots);

struct vidinf
{
    int entnum;     // g_video_entry subscript
    unsigned flags; // flags for sort's compare, defined below
};
/* defines for flags; done this way instead of bit union to ensure ordering;
   these bits represent the sort sequence for video mode list */
enum
{
    VI_EXACT = 0x8000, // unless the one and only exact match
    VI_DISK = 1024,    // if video mode is disk video
    VI_NOKEY = 512,    // if no function key assigned
    VI_SSMALL = 128,   // screen smaller than file's screen
    VI_SBIG = 64,      // screen bigger than file's screen
    VI_VSMALL = 32,    // screen smaller than file's view
    VI_VBIG = 16,      // screen bigger than file's view
    VI_CSMALL = 8,     // mode has too few colors
    VI_CBIG = 4,       // mode has excess colors
    VI_ASPECT = 1      // aspect ratio bad
};

static bool vidinf_less(const vidinf &lhs, const vidinf &rhs)
{
    if (lhs.flags < rhs.flags)
    {
        return true;
    }
    if (lhs.flags > rhs.flags)
    {
        return false;
    }
    if (g_video_table[lhs.entnum].keynum < g_video_table[rhs.entnum].keynum)
    {
        return true;
    }
    if (g_video_table[lhs.entnum].keynum > g_video_table[rhs.entnum].keynum)
    {
        return false;
    }
    if (lhs.entnum < rhs.entnum)
    {
        return true;
    }
    return false;
}

static void format_vid_inf(int i, char const *err, char *buf)
{
    char kname[5];
    std::memcpy((char *)&g_video_entry, (char *)&g_video_table[i],
           sizeof(g_video_entry));
    vidmode_keyname(g_video_entry.keynum, kname);
    std::sprintf(buf, "%-5s %-16s %-4s %5d %5d %3d %-25s",  // 67 chars
            kname, g_video_entry.driver->get_description().c_str(), err,
            g_video_entry.xdots, g_video_entry.ydots,
            g_video_entry.colors, g_video_entry.comment);
    g_video_entry.xdots = 0; // so tab_display knows to display nothing
}

static double vid_aspect(int tryxdots, int tryydots)
{
    // calc resulting aspect ratio for specified dots in current mode
    return (double)tryydots / (double)tryxdots
           * (double)g_video_entry.xdots / (double)g_video_entry.ydots
           * g_screen_aspect;
}

static std::vector<vidinf> s_video_info;

static std::string heading_detail(FRACTAL_INFO const *info, ext_blk_3 const *blk_3_info)
{
    std::ostringstream result;
    if (info->info_id[0] == 'G')
    {
        result << "      Non-fractal GIF";
    }
    else
    {
        char const *nameptr = g_cur_fractal_specific->name;
        if (*nameptr == '*')
        {
            ++nameptr;
        }
        if (g_display_3d != display_3d_modes::NONE)
        {
            nameptr = "3D Transform";
        }
        result << "Type: " << nameptr;
        if ((!std::strcmp(nameptr, "formula"))
            || (!std::strcmp(nameptr, "lsystem"))
            || (!std::strncmp(nameptr, "ifs", 3))) // for ifs and ifs3d
        {
            result << " -> " << blk_3_info->form_name;
        }
    }
    return result.str();
}

static std::string save_release_detail()
{
    char buff[80];
    std::snprintf(buff, std::size(buff), "v%d.%01d", g_release/100, (g_release%100)/10);
    if (g_release%100)
    {
        int i = (int) std::strlen(buff);
        buff[i] = (char)((g_release%10) + '0');
        buff[i+1] = 0;
    }
    return std::string(buff);
}

int get_video_mode(FRACTAL_INFO *info, ext_blk_3 *blk_3_info)
{
    bool gotrealmode;
    double ftemp;
    unsigned tmpflags;

    g_init_mode = -1;

    // try to find exact match for vid mode: first look for non-disk video
    const VIDEOINFO *begin = g_video_table;
    const VIDEOINFO *end = g_video_table + g_video_table_len;
    auto it = std::find_if(begin, end,
        [=](const VIDEOINFO &mode)
        {
            return info->xdots == mode.xdots    //
                && info->ydots == mode.ydots    //
                && g_file_colors == mode.colors //
                && mode.driver != nullptr       //
                && !mode.driver->diskp();
        });
    if (it == end)
    {
        it = std::find_if(begin, end,
            [=](const VIDEOINFO &mode)
            {
                return info->xdots == mode.xdots //
                    && info->ydots == mode.ydots //
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
    s_video_info.resize(g_video_table_len);
    for (int i = 0; i < g_video_table_len; ++i)
    {
        g_video_entry = g_video_table[i];
        tmpflags = VI_EXACT;
        if (g_video_entry.driver != nullptr && g_video_entry.driver->diskp())
        {
            tmpflags |= VI_DISK;
        }
        if (g_video_entry.keynum == 0)
        {
            tmpflags |= VI_NOKEY;
        }
        if (info->xdots > g_video_entry.xdots || info->ydots > g_video_entry.ydots)
        {
            tmpflags |= VI_SSMALL;
        }
        else if (info->xdots < g_video_entry.xdots || info->ydots < g_video_entry.ydots)
        {
            tmpflags |= VI_SBIG;
        }
        if (g_file_x_dots > g_video_entry.xdots || g_file_y_dots > g_video_entry.ydots)
        {
            tmpflags |= VI_VSMALL;
        }
        else if (g_file_x_dots < g_video_entry.xdots || g_file_y_dots < g_video_entry.ydots)
        {
            tmpflags |= VI_VBIG;
        }
        if (g_file_colors > g_video_entry.colors)
        {
            tmpflags |= VI_CSMALL;
        }
        if (g_file_colors < g_video_entry.colors)
        {
            tmpflags |= VI_CBIG;
        }
        if (i == g_init_mode)
        {
            tmpflags -= VI_EXACT;
        }
        if (g_file_aspect_ratio != 0.0f && (tmpflags & VI_VSMALL) == 0)
        {
            ftemp = vid_aspect(g_file_x_dots, g_file_y_dots);
            if (ftemp < g_file_aspect_ratio * 0.98
                || ftemp > g_file_aspect_ratio * 1.02)
            {
                tmpflags |= VI_ASPECT;
            }
        }
        s_video_info[i].entnum = i;
        // cppcheck-suppress unreadVariable
        s_video_info[i].flags  = tmpflags;
    }

    if (g_fast_restore  && !g_ask_video)
    {
        g_init_mode = g_adapter;
    }

    gotrealmode = false;
    if ((g_init_mode < 0 || (g_ask_video && (g_init_batch == batch_modes::NONE))) && !g_make_parameter_file)
    {
        // no exact match or (askvideo=yes and batch=no), and not in makepar mode, talk to user
        std::sort(s_video_info.begin(), s_video_info.end(), vidinf_less);

        std::vector<int> attributes(g_video_table_len, 1);

        // format heading
        char heading[256];  // big enough for more than a few lines
        std::snprintf(heading, std::size(heading), "File: %-44s  %d x %d x %d\n%-52s",
                trim_filename(g_read_filename, 44).c_str(), g_file_x_dots, g_file_y_dots, g_file_colors,
                heading_detail(info, blk_3_info).c_str());
        if (info->info_id[0] != 'G')
        {
            if (g_save_system)
            {
                std::strcat(heading, "Id       ");
            }
            std::strcat(heading, save_release_detail().c_str());
        }
        std::strcat(heading, "\n");
        if (info->info_id[0] != 'G' && g_save_system == 0)
        {
            if (g_init_mode < 0)
            {
                std::strcat(heading, "Saved in unknown video mode.");
            }
            else
            {
                char buff[80];
                format_vid_inf(g_init_mode, "", buff);
                std::strcat(heading, buff);
            }
        }
        if (g_file_aspect_ratio != 0 && g_file_aspect_ratio != g_screen_aspect)
        {
            std::strcat(heading,
                   "\nWARNING: non-standard aspect ratio; loading will change your <v>iew settings");
        }
        std::strcat(heading, "\n");
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
            ValueSaver saved_help_mode{g_help_mode, help_labels::HELP_LOADFILE};
            i = fullscreen_choice(0, heading,
                "key...name......................err...xdot..ydot.clr.comment..................",
                instructions.c_str(), g_video_table_len, nullptr, &attributes[0], 1, 13, 78, 0,
                format_item, nullptr, nullptr, check_modekey);
        }
        if (i == -1)
        {
            return -1;
        }
        if (i < 0)  // returned -100 - g_video_table entry number
        {
            g_init_mode = -100 - i;
            gotrealmode = true;
        }
        else
        {
            g_init_mode = s_video_info[i].entnum;
        }
    }

    if (!gotrealmode)  // translate from temp table to permanent
    {
        int i = g_init_mode;
        int j = g_video_table[i].keynum;
        if (j != 0)
        {
            for (g_init_mode = 0; g_init_mode < MAX_VIDEO_MODES-1; ++g_init_mode)
            {
                if (g_video_table[g_init_mode].keynum == j)
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
            std::memcpy((char *)&g_video_table[g_init_mode = MAX_VIDEO_MODES-1],
                   (char *)&g_video_table[i], sizeof(*g_video_table));
        }
    }

    // ok, we're going to return with a video mode
    std::memcpy((char *)&g_video_entry, (char *)&g_video_table[g_init_mode],
           sizeof(g_video_entry));

    if (g_view_window
        && g_file_x_dots == g_video_entry.xdots
        && g_file_y_dots == g_video_entry.ydots)
    {
        // pull image into a view window
        if (g_calc_status != calc_status_value::COMPLETED) // if not complete
        {
            g_calc_status = calc_status_value::PARAMS_CHANGED;  // can't resume anyway
        }
        if (g_view_x_dots)
        {
            g_view_reduction = (float)(g_video_entry.xdots / g_view_x_dots);
            g_view_y_dots = 0;
            g_view_x_dots = g_view_y_dots; // easier to use auto reduction
        }
        g_view_reduction = (float)((int)(g_view_reduction + 0.5)); // need integer value
        g_skip_y_dots = (short)(g_view_reduction - 1);
        g_skip_x_dots = g_skip_y_dots;
        return 0;
    }

    g_skip_y_dots = 0;
    g_skip_x_dots = g_skip_y_dots; // set for no reduction
    if (g_video_entry.xdots < g_file_x_dots || g_video_entry.ydots < g_file_y_dots)
    {
        // set up to load only every nth pixel to make image fit
        if (g_calc_status != calc_status_value::COMPLETED) // if not complete
        {
            g_calc_status = calc_status_value::PARAMS_CHANGED;  // can't resume anyway
        }
        g_skip_y_dots = 1;
        g_skip_x_dots = g_skip_y_dots;
        while (g_skip_x_dots * g_video_entry.xdots < g_file_x_dots)
        {
            ++g_skip_x_dots;
        }
        while (g_skip_y_dots * g_video_entry.ydots < g_file_y_dots)
        {
            ++g_skip_y_dots;
        }
        int i = 0;
        int j = 0;
        int tmpxdots;
        int tmpydots;
        while (true)
        {
            tmpxdots = (g_file_x_dots + g_skip_x_dots - 1) / g_skip_x_dots;
            tmpydots = (g_file_y_dots + g_skip_y_dots - 1) / g_skip_y_dots;
            // reduce further if that improves aspect
            ftemp = vid_aspect(tmpxdots, tmpydots);
            if (ftemp > g_file_aspect_ratio)
            {
                if (j)
                {
                    break; // already reduced x, don't reduce y
                }
                double const ftemp2 = vid_aspect(tmpxdots, (g_file_y_dots+g_skip_y_dots)/(g_skip_y_dots+1));
                if (ftemp2 < g_file_aspect_ratio
                    && ftemp/g_file_aspect_ratio *0.9 <= g_file_aspect_ratio/ftemp2)
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
                double const ftemp2 = vid_aspect((g_file_x_dots+g_skip_x_dots)/(g_skip_x_dots+1), tmpydots);
                if (ftemp2 > g_file_aspect_ratio
                    && g_file_aspect_ratio/ftemp *0.9 <= ftemp2/g_file_aspect_ratio)
                {
                    break; // further x reduction is worse
                }
                ++g_skip_x_dots;
                ++j;
            }
        }
        g_file_x_dots = tmpxdots;
        g_file_y_dots = tmpydots;
        --g_skip_x_dots;
        --g_skip_y_dots;
    }

    g_final_aspect_ratio = g_file_aspect_ratio;
    if (g_final_aspect_ratio == 0) // assume display correct
    {
        g_final_aspect_ratio = (float)vid_aspect(g_file_x_dots, g_file_y_dots);
    }
    if (g_final_aspect_ratio >= g_screen_aspect-0.02
        && g_final_aspect_ratio <= g_screen_aspect+0.02)
    {
        g_final_aspect_ratio = g_screen_aspect;
    }
    {
        int i = (int)(g_final_aspect_ratio * 1000.0 + 0.5);
        g_final_aspect_ratio = (float)(i/1000.0); // chop precision to 3 decimals
    }

    // setup view window stuff
    g_view_window = false;
    g_view_x_dots = 0;
    g_view_y_dots = 0;
    if (g_file_x_dots != g_video_entry.xdots || g_file_y_dots != g_video_entry.ydots)
    {
        // image not exactly same size as screen
        g_view_window = true;
        ftemp = g_final_aspect_ratio
                * (double)g_video_entry.ydots / (double)g_video_entry.xdots
                / g_screen_aspect;
        float tmpreduce;
        int i;
        int j;
        if (g_final_aspect_ratio <= g_screen_aspect)
        {
            i = (int)((double)g_video_entry.xdots / (double)g_file_x_dots * 20.0 + 0.5);
            tmpreduce = (float)(i/20.0); // chop precision to nearest .05
            i = (int)((double)g_video_entry.xdots / tmpreduce + 0.5);
            j = (int)((double)i * ftemp + 0.5);
        }
        else
        {
            i = (int)((double)g_video_entry.ydots / (double)g_file_y_dots * 20.0 + 0.5);
            tmpreduce = (float)(i/20.0); // chop precision to nearest .05
            j = (int)((double)g_video_entry.ydots / tmpreduce + 0.5);
            i = (int)((double)j / ftemp + 0.5);
        }
        if (i != g_file_x_dots || j != g_file_y_dots)  // too bad, must be explicit
        {
            g_view_x_dots = g_file_x_dots;
            g_view_y_dots = g_file_y_dots;
        }
        else
        {
            g_view_reduction = tmpreduce; // ok, this works
        }
    }
    if (!g_make_parameter_file
        && !g_fast_restore
        && (g_init_batch == batch_modes::NONE)
        && (std::fabs(g_final_aspect_ratio - g_screen_aspect) > .00001 || g_view_x_dots != 0))
    {
        stopmsg(stopmsg_flags::NO_BUZZER,
            "Warning: <V>iew parameters are being set to non-standard values.\n"
            "Remember to reset them when finished with this image!");
    }
    return 0;
}

static void format_item(int choice, char *buf)
{
    char errbuf[10];
    unsigned tmpflags;
    errbuf[0] = 0;
    tmpflags = s_video_info[choice].flags;
    if (tmpflags & (VI_VSMALL+VI_CSMALL+VI_ASPECT))
    {
        std::strcat(errbuf, "*");
    }
    if (tmpflags & VI_DISK)
    {
        std::strcat(errbuf, "D");
    }
    if (tmpflags & VI_VSMALL)
    {
        std::strcat(errbuf, "R");
    }
    if (tmpflags & VI_CSMALL)
    {
        std::strcat(errbuf, "C");
    }
    if (tmpflags & VI_ASPECT)
    {
        std::strcat(errbuf, "A");
    }
    if (tmpflags & VI_VBIG)
    {
        std::strcat(errbuf, "v");
    }
    if (tmpflags & VI_CBIG)
    {
        std::strcat(errbuf, "c");
    }
    format_vid_inf(s_video_info[choice].entnum, errbuf, buf);
}

static int check_modekey(int curkey, int /*choice*/)
{
    int i = check_vidmode_key(0, curkey);
    return i >= 0 ? -100-i : 0;
}
