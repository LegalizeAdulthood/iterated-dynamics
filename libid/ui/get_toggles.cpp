// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/get_toggles.h"

#include "engine/calcfrac.h"
#include "engine/log_map.h"
#include "engine/solid_guess.h"
#include "engine/sound.h"
#include "engine/UserData.h"
#include "engine/VideoInfo.h"
#include "fractals/fractype.h"
#include "helpdefs.h"
#include "io/check_write_file.h"
#include "io/encoder.h"
#include "io/save_timer.h"
#include "ui/ChoiceBuilder.h"
#include "ui/help.h"

#include <algos/string_algorithms.h>

#include <fmt/format.h>

#include <algorithm>
#include <cstdlib>
#include <iterator>
#include <string>

using namespace id::engine;
using namespace id::fractals;
using namespace id::help;
using namespace id::io;

namespace id::ui
{

/*
        get_toggles() is called whenever the 'x' key is pressed.
        This routine prompts for several options,
        sets the appropriate variables, and returns the following code
        to the calling routine:

        -1  routine was ESCAPEd - no need to re-generate the image.
         0  nothing changed, or minor variable such as "overwrite=".
            No need to re-generate the image.
        >0  major variable changed (such as "inside=").  Re-generate
            the image.

        Finally, remember to insert variables in the list *and* check
        for them in the same order!!!
*/
int get_toggles()
{
    ChoiceBuilder<14> choices;
    const char *calc_modes[] = {"1", "2", "3", "g", "g1", "g2", "g3", "g4", "g5", "g6", "b", "s", "t", "d", "o", "p"};
    const char *sound_modes[5] = {"off", "beep", "x", "y", "z"};
    const char *inside_modes[] = {
        "numb", "maxiter", "zmag", "bof60", "bof61", "epsiloncross", "startrail", "period", "atan", "fmod"};
    const char *outside_modes[] = {"numb", "iter", "real", "imag", "mult", "summ", "atan", "fmod", "tdis"};

    const int calc_mode = g_user.std_calc_mode == CalcMode::ONE_PASS        ? 0
        : g_user.std_calc_mode == CalcMode::TWO_PASS                        ? 1
        : g_user.std_calc_mode == CalcMode::THREE_PASS                      ? 2
        : g_user.std_calc_mode == CalcMode::SOLID_GUESS && g_stop_pass == 0 ? 3
        : g_user.std_calc_mode == CalcMode::SOLID_GUESS && g_stop_pass == 1 ? 4
        : g_user.std_calc_mode == CalcMode::SOLID_GUESS && g_stop_pass == 2 ? 5
        : g_user.std_calc_mode == CalcMode::SOLID_GUESS && g_stop_pass == 3 ? 6
        : g_user.std_calc_mode == CalcMode::SOLID_GUESS && g_stop_pass == 4 ? 7
        : g_user.std_calc_mode == CalcMode::SOLID_GUESS && g_stop_pass == 5 ? 8
        : g_user.std_calc_mode == CalcMode::SOLID_GUESS && g_stop_pass == 6 ? 9
        : g_user.std_calc_mode == CalcMode::BOUNDARY_TRACE                  ? 10
        : g_user.std_calc_mode == CalcMode::SYNCHRONOUS_ORBIT               ? 11
        : g_user.std_calc_mode == CalcMode::TESSERAL                        ? 12
        : g_user.std_calc_mode == CalcMode::DIFFUSION                       ? 13
        : g_user.std_calc_mode == CalcMode::ORBIT                           ? 14
                                                                            : /* "p"erturbation */ 15;
    CalcMode old_user_std_calc_mode = g_user.std_calc_mode;
    int old_stop_pass = g_stop_pass;
    long old_max_iterations = g_max_iterations;

    int inside_color;
    if (g_inside_method >= ColorMethod::COLOR)
    {
        inside_color = g_inside_color;
    }
    else
    {
        inside_color = 0;
    }

    int inside_mode = 0;
    if (g_inside_method >= ColorMethod::COLOR) // numb
    {
        inside_mode = 0;
    }
    else if (g_inside_method == ColorMethod::ITER)
    {
        inside_mode = 1;
    }
    else if (g_inside_method == ColorMethod::ZMAG)
    {
        inside_mode = 2;
    }
    else if (g_inside_method == ColorMethod::BOF60)
    {
        inside_mode = 3;
    }
    else if (g_inside_method == ColorMethod::BOF61)
    {
        inside_mode = 4;
    }
    else if (g_inside_method == ColorMethod::EPS_CROSS)
    {
        inside_mode = 5;
    }
    else if (g_inside_method == ColorMethod::STAR_TRAIL)
    {
        inside_mode = 6;
    }
    else if (g_inside_method == ColorMethod::PERIOD)
    {
        inside_mode = 7;
    }
    else if (g_inside_method == ColorMethod::ATANI)
    {
        inside_mode = 8;
    }
    else if (g_inside_method == ColorMethod::FMODI)
    {
        inside_mode = 9;
    }
    const ColorMethod old_inside_method{g_inside_method};
    const int old_inside = g_inside_color;

    int outside_color;
    if (g_outside_method >= ColorMethod::COLOR)
    {
        outside_color = g_outside_color;
    }
    else
    {
        outside_color = 0;
    }

    int outside_mode;
    if (g_outside_method >= ColorMethod::COLOR) // numb
    {
        outside_mode = 0;
    }
    else
    {
        outside_mode = -+g_outside_method;
    }
    ColorMethod old_outside_method{g_outside_method};
    int old_outside = g_outside_color;

    std::filesystem::path prev_save_name = g_save_filename;

    const int old_sound_flag = g_sound_flag;

    const long old_log_map_flag = g_log_map_flag;
    const int old_biomorph = g_user.biomorph_value;
    const int old_decomp = g_decomp[0];
    const std::string fill_color{g_fill_color < 0 ? "normal" : fmt::format("{:d}", g_fill_color)};
    const int old_fill_color = g_fill_color;
    const double old_close_proximity = g_close_proximity;

    choices
        .list("Passes (1-3, g[es], b[ound], t[ess], d[iff], o[rbit], p[ert])", std::size(calc_modes), 3, calc_modes,
            calc_mode)
        .long_number("Maximum Iterations (2 to 2,147,483,647)", old_max_iterations)
        .int_number("Inside Color (0-# of colors, if Inside=numb)", inside_color)
        .list("Inside (numb,maxit,zmag,bof60,bof61,epscr,star,per,atan,fmod)", std::size(inside_modes), 12,
            inside_modes, inside_mode)
        .int_number("Outside Color (0-# of colors, if Outside=numb)", outside_color)
        .list("Outside (numb,iter,real,imag,mult,summ,atan,fmod,tdis)", std::size(outside_modes), 4, outside_modes,
            outside_mode)
        .string("Savename (.GIF implied)", g_save_filename.filename().string().c_str())
        .yes_no("File Overwrite ('overwrite=')", g_overwrite_file)
        .list("Sound (off, beep, x, y, z)", std::size(sound_modes), 4, sound_modes,
            old_sound_flag & SOUNDFLAG_ORBIT_MASK);
    if (g_iteration_ranges.empty())
    {
        choices.long_number("Log Palette (0=no,1=yes,-1=old,+n=cmprsd,-n=sqrt, 2=auto)", old_log_map_flag);
    }
    else
    {
        choices.comment("Log Palette (n/a, ranges= parameter is in effect)");
    }
    choices.int_number("Biomorph Color (-1 means OFF)", old_biomorph)
        .int_number("Decomp Option (2,4,8,..,256, 0=OFF)", old_decomp)
        .string("Fill Color (normal,#) (works with passes=t, b and d)", fill_color.c_str())
        .double_number("Proximity value for inside=epscross and fmod", old_close_proximity, 14);

    const HelpLabels old_help_mode = g_help_mode;
    g_help_mode = HelpLabels::HELP_X_OPTIONS;
    const int i = choices.prompt("Basic Options\n(not all combinations make sense)");
    g_help_mode = old_help_mode;
    if (i < 0)
    {
        return -1;
    }

    // now check out the results (*hopefully* in the same order <grin>)
    int j = 0; // return code

    const int calc_choice{choices.read_list()};
    g_user.std_calc_mode = static_cast<CalcMode>(calc_modes[calc_choice][0]);
    g_stop_pass = static_cast<int>(calc_modes[calc_choice][1]) - static_cast<int>('0');

    if (g_stop_pass < 0 || g_stop_pass > 6 || g_user.std_calc_mode != CalcMode::SOLID_GUESS)
    {
        g_stop_pass = 0;
    }

    if (g_user.std_calc_mode == CalcMode::ORBIT && g_fractal_type == FractalType::LYAPUNOV) // Oops,lyapunov type
    {
        // doesn't use 'new' & breaks orbits
        g_user.std_calc_mode = old_user_std_calc_mode;
    }

    if (old_user_std_calc_mode != g_user.std_calc_mode)
    {
        j++;
    }
    if (old_stop_pass != g_stop_pass)
    {
        j++;
    }
    g_max_iterations = choices.read_long_number();
    if (g_max_iterations < 0)
    {
        g_max_iterations = old_max_iterations;
    }
    g_max_iterations = std::max(g_max_iterations, 2L);

    if (g_max_iterations != old_max_iterations)
    {
        j++;
    }

    g_inside_color = choices.read_int_number();
    if (g_inside_color < 0)
    {
        g_inside_color = -g_inside_color;
    }
    if (g_inside_color >= g_colors)
    {
        g_inside_color = g_inside_color % g_colors + g_inside_color / g_colors;
    }

    if (int tmp = choices.read_list(); tmp > 0)
    {
        switch (tmp)
        {
        case 0:
            g_inside_method = ColorMethod::COLOR;
            break;
        case 1:
            g_inside_method = ColorMethod::ITER;
            g_inside_color = 0;
            break;
        case 2:
            g_inside_method = ColorMethod::ZMAG;
            g_inside_color = 0;
            break;
        case 3:
            g_inside_method = ColorMethod::BOF60;
            g_inside_color = 0;
            break;
        case 4:
            g_inside_method = ColorMethod::BOF61;
            g_inside_color = 0;
            break;
        case 5:
            g_inside_method = ColorMethod::EPS_CROSS;
            g_inside_color = 0;
            break;
        case 6:
            g_inside_method = ColorMethod::STAR_TRAIL;
            g_inside_color = 0;
            break;
        case 7:
            g_inside_method = ColorMethod::PERIOD;
            g_inside_color = 0;
            break;
        case 8:
            g_inside_method = ColorMethod::ATANI;
            g_inside_color = 0;
            break;
        case 9:
            g_inside_method = ColorMethod::FMODI;
            g_inside_color = 0;
            break;
        }
    }
    if (g_inside_color != old_inside || g_inside_method != old_inside_method)
    {
        j++;
    }

    g_outside_color = choices.read_int_number();
    if (g_outside_color < 0)
    {
        g_outside_color = -g_outside_color;
    }
    if (g_outside_color >= g_colors)
    {
        g_outside_color = g_outside_color % g_colors + g_outside_color / g_colors;
    }

    if (int tmp = choices.read_list(); tmp > 0)
    {
        g_outside_method = static_cast<ColorMethod>(-tmp);
        g_outside_color = -1;
    }
    if (g_outside_color != old_outside || g_outside_method != old_outside_method)
    {
        j++;
    }

    g_save_filename = g_save_filename.parent_path() / choices.read_string();
    if (g_save_filename != prev_save_name)
    {
        g_resave_flag = TimedSave::NONE;
        g_started_resaves = false; // forget pending increment
    }
    g_overwrite_file = choices.read_yes_no();

    g_sound_flag = (g_sound_flag >> 3) << 3 | choices.read_list();
    if (g_sound_flag != old_sound_flag &&
        ((g_sound_flag & SOUNDFLAG_ORBIT_MASK) > SOUNDFLAG_BEEP ||
            (old_sound_flag & SOUNDFLAG_ORBIT_MASK) > SOUNDFLAG_BEEP))
    {
        j++;
    }

    if (g_iteration_ranges.empty())
    {
        g_log_map_flag = choices.read_long_number();
        if (g_log_map_flag != old_log_map_flag)
        {
            j++;
            g_log_map_auto_calculate = false; // turn it off, use the supplied value
        }
    }
    else
    {
        choices.read_comment();
    }

    g_user.biomorph_value = choices.read_int_number();
    if (g_user.biomorph_value >= g_colors)
    {
        g_user.biomorph_value = g_user.biomorph_value % g_colors + g_user.biomorph_value / g_colors;
    }
    if (g_user.biomorph_value != old_biomorph)
    {
        j++;
    }

    g_decomp[0] = choices.read_int_number();
    if (g_decomp[0] != old_decomp)
    {
        j++;
    }

    const std::string new_fill_color{id::algos::ascii_to_lower_copy(choices.read_string())};
    if (new_fill_color.compare(0, 4, "norm") == 0)
    {
        g_fill_color = -1;
    }
    else
    {
        g_fill_color = std::atoi(new_fill_color.c_str());
    }
    if (g_fill_color < 0)
    {
        g_fill_color = -1;
    }
    if (g_fill_color >= g_colors)
    {
        g_fill_color = g_fill_color % g_colors + g_fill_color / g_colors;
    }
    if (g_fill_color != old_fill_color)
    {
        j++;
    }

    g_close_proximity = choices.read_double_number();
    if (g_close_proximity != old_close_proximity)
    {
        j++;
    }

    return j;
}

} // namespace id::ui
