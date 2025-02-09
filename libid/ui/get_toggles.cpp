// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/get_toggles.h"

#include "engine/calcfrac.h"
#include "engine/id_data.h"
#include "engine/log_map.h"
#include "engine/perturbation.h"
#include "fractals/fractype.h"
#include "helpdefs.h"
#include "ui/cmdfiles.h"
#include "ui/full_screen_prompt.h"

#include <config/path_limits.h>
#include <config/string_lower.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <string>

/*
        get_toggles() is called whenever the 'x' key is pressed.
        This routine prompts for several options,
        sets the appropriate variables, and returns the following code
        to the calling routine:

        -1  routine was ESCAPEd - no need to re-generate the image.
         0  nothing changed, or minor variable such as "overwrite=".
            No need to re-generate the image.
         1  major variable changed (such as "inside=").  Re-generate
            the image.

        Finally, remember to insert variables in the list *and* check
        for them in the same order!!!
*/
int get_toggles()
{
    char const *choices[20];
    char prev_save_name[ID_FILE_MAX_DIR + 1];
    FullScreenValues values[25];
    int old_sound_flag;
    char const *calc_modes[] = {
        "1", "2", "3", "g", "g1", "g2", "g3", "g4", "g5", "g6", "b", "s", "t", "d", "o"};
    char const *sound_modes[5] = {"off", "beep", "x", "y", "z"};
    char const *inside_modes[] = {
        "numb", "maxiter", "zmag", "bof60", "bof61", "epsiloncross", "startrail", "period", "atan", "fmod"};
    char const *outside_modes[] = {"numb", "iter", "real", "imag", "mult", "summ", "atan", "fmod", "tdis"};
    char const *perturbation_modes[] = {"auto", "yes", "no"};

    int k = -1;

    choices[++k] = "Passes (1-3, g[es], b[ound], t[ess], d[iff], o[rbit])";
    values[k].type = 'l';
    values[k].uval.ch.vlen = 3;
    values[k].uval.ch.list_len = std::size(calc_modes);
    values[k].uval.ch.list = calc_modes;
    values[k].uval.ch.val =
        (g_user_std_calc_mode == '1') ? 0
        : (g_user_std_calc_mode == '2') ? 1
        : (g_user_std_calc_mode == '3') ? 2
        : (g_user_std_calc_mode == 'g' && g_stop_pass == 0) ? 3
        : (g_user_std_calc_mode == 'g' && g_stop_pass == 1) ? 4
        : (g_user_std_calc_mode == 'g' && g_stop_pass == 2) ? 5
        : (g_user_std_calc_mode == 'g' && g_stop_pass == 3) ? 6
        : (g_user_std_calc_mode == 'g' && g_stop_pass == 4) ? 7
        : (g_user_std_calc_mode == 'g' && g_stop_pass == 5) ? 8
        : (g_user_std_calc_mode == 'g' && g_stop_pass == 6) ? 9
        : (g_user_std_calc_mode == 'b') ? 10
        : (g_user_std_calc_mode == 's') ? 11
        : (g_user_std_calc_mode == 't') ? 12
        : (g_user_std_calc_mode == 'd') ? 13
        : /*(g_user_std_calc_mode == 'o') ?*/ 14;
//        :         "p"erturbation      15;
    char old_user_std_calc_mode = g_user_std_calc_mode;
    int old_stop_pass = g_stop_pass;
    choices[++k] = "Floating Point Algorithm";
    values[k].type = 'y';
    values[k].uval.ch.val = g_user_float_flag ? 1 : 0;
    choices[++k] = "Maximum Iterations (2 to 2,147,483,647)";
    values[k].type = 'L';
    long old_max_iterations = g_max_iterations;
    values[k].uval.Lval = old_max_iterations;

    choices[++k] = "Inside Color (0-# of colors, if Inside=numb)";
    values[k].type = 'i';
    if (g_inside_color >= COLOR_BLACK)
    {
        values[k].uval.ival = g_inside_color;
    }
    else
    {
        values[k].uval.ival = 0;
    }

    choices[++k] = "Inside (numb,maxit,zmag,bof60,bof61,epscr,star,per,atan,fmod)";
    values[k].type = 'l';
    values[k].uval.ch.vlen = 12;
    values[k].uval.ch.list_len = std::size(inside_modes);
    values[k].uval.ch.list = inside_modes;
    if (g_inside_color >= COLOR_BLACK)    // numb
    {
        values[k].uval.ch.val = 0;
    }
    else if (g_inside_color == ITER)
    {
        values[k].uval.ch.val = 1;
    }
    else if (g_inside_color == ZMAG)
    {
        values[k].uval.ch.val = 2;
    }
    else if (g_inside_color == BOF60)
    {
        values[k].uval.ch.val = 3;
    }
    else if (g_inside_color == BOF61)
    {
        values[k].uval.ch.val = 4;
    }
    else if (g_inside_color == EPS_CROSS)
    {
        values[k].uval.ch.val = 5;
    }
    else if (g_inside_color == STAR_TRAIL)
    {
        values[k].uval.ch.val = 6;
    }
    else if (g_inside_color == PERIOD)
    {
        values[k].uval.ch.val = 7;
    }
    else if (g_inside_color == ATANI)
    {
        values[k].uval.ch.val = 8;
    }
    else if (g_inside_color == FMODI)
    {
        values[k].uval.ch.val = 9;
    }
    int old_inside = g_inside_color;

    choices[++k] = "Outside Color (0-# of colors, if Outside=numb)";
    values[k].type = 'i';
    if (g_outside_color >= COLOR_BLACK)
    {
        values[k].uval.ival = g_outside_color;
    }
    else
    {
        values[k].uval.ival = 0;
    }

    choices[++k] = "Outside (numb,iter,real,imag,mult,summ,atan,fmod,tdis)";
    values[k].type = 'l';
    values[k].uval.ch.vlen = 4;
    values[k].uval.ch.list_len = std::size(outside_modes);
    values[k].uval.ch.list = outside_modes;
    if (g_outside_color >= COLOR_BLACK)    // numb
    {
        values[k].uval.ch.val = 0;
    }
    else
    {
        values[k].uval.ch.val = -g_outside_color;
    }
    int old_outside = g_outside_color;

    choices[++k] = "Savename (.GIF implied)";
    values[k].type = 's';
    std::strcpy(prev_save_name, g_save_filename.c_str());
    char const *save_name_ptr = std::strrchr(g_save_filename.c_str(), SLASH_CH);
    if (save_name_ptr == nullptr)
    {
        save_name_ptr = g_save_filename.c_str();
    }
    else
    {
        save_name_ptr++; // point past slash
    }
    std::strcpy(values[k].uval.sval, save_name_ptr);

    choices[++k] = "File Overwrite ('overwrite=')";
    values[k].type = 'y';
    values[k].uval.ch.val = g_overwrite_file ? 1 : 0;

    choices[++k] = "Sound (off, beep, x, y, z)";
    values[k].type = 'l';
    values[k].uval.ch.vlen = 4;
    values[k].uval.ch.list_len = 5;
    values[k].uval.ch.list = sound_modes;
    values[k].uval.ch.val = (old_sound_flag = g_sound_flag) & SOUNDFLAG_ORBIT_MASK;

    if (g_iteration_ranges_len == 0)
    {
        choices[++k] = "Log Palette (0=no,1=yes,-1=old,+n=cmprsd,-n=sqrt, 2=auto)";
        values[k].type = 'L';
    }
    else
    {
        choices[++k] = "Log Palette (n/a, ranges= parameter is in effect)";
        values[k].type = '*';
    }
    long old_log_map_flag = g_log_map_flag;
    values[k].uval.Lval = old_log_map_flag;

    choices[++k] = "Biomorph Color (-1 means OFF)";
    values[k].type = 'i';
    int old_biomorph = g_user_biomorph_value;
    values[k].uval.ival = old_biomorph;

    choices[++k] = "Decomp Option (2,4,8,..,256, 0=OFF)";
    values[k].type = 'i';
    int old_decomp = g_decomp[0];
    values[k].uval.ival = old_decomp;

    choices[++k] = "Fill Color (normal,#) (works with passes=t, b and d)";
    values[k].type = 's';
    if (g_fill_color < 0)
    {
        std::strcpy(values[k].uval.sval, "normal");
    }
    else
    {
        std::sprintf(values[k].uval.sval, "%d", g_fill_color);
    }
    int old_fill_color = g_fill_color;

    choices[++k] = "Proximity value for inside=epscross and fmod";
    values[k].type = 'f'; // should be 'd', but prompts get messed up
    double old_close_proximity = g_close_proximity;
    values[k].uval.dval = old_close_proximity;

    choices[++k] = "Use Perturbation (yes, no, auto - internal choice)";
    values[k].type = 'l';
    values[k].uval.ch.vlen = 4;
    values[k].uval.ch.list_len = std::size(perturbation_modes);
    values[k].uval.ch.list = perturbation_modes;
    PerturbationMode old_perturbation = g_perturbation;
    if (g_perturbation == PerturbationMode::AUTO)
    {
        values[k].uval.ch.val = 0;
    }
    else if (g_perturbation == PerturbationMode::YES)
    {
        values[k].uval.ch.val = 1;
    }
    else
    {
        values[k].uval.ch.val = 2;
    }

    choices[++k] = "Perturbation tolerance";
    values[k].type = 'd';
    double old_perturbation_tolerance = g_perturbation_tolerance;
    values[k].uval.dval = old_perturbation_tolerance;

    HelpLabels const old_help_mode = g_help_mode;
    g_help_mode = HelpLabels::HELP_X_OPTIONS;
    int i = full_screen_prompt(
        "Basic Options\n(not all combinations make sense)", k + 1, choices, values, 0, nullptr);
    g_help_mode = old_help_mode;
    if (i < 0)
    {
        return -1;
    }

    // now check out the results (*hopefully* in the same order <grin>)
    k = -1;
    int j = 0;   // return code

    g_user_std_calc_mode = calc_modes[values[++k].uval.ch.val][0];
    g_stop_pass = (int)calc_modes[values[k].uval.ch.val][1] - (int)'0';

    if (g_stop_pass < 0 || g_stop_pass > 6 || g_user_std_calc_mode != 'g')
    {
        g_stop_pass = 0;
    }

    if (g_user_std_calc_mode == 'o' && g_fractal_type == FractalType::LYAPUNOV)   // Oops,lyapunov type
    {
        // doesn't use 'new' & breaks orbits
        g_user_std_calc_mode = old_user_std_calc_mode;
    }

    if (old_user_std_calc_mode != g_user_std_calc_mode)
    {
        j++;
    }
    if (old_stop_pass != g_stop_pass)
    {
        j++;
    }
    if ((values[++k].uval.ch.val != 0) != g_user_float_flag)
    {
        g_user_float_flag = values[k].uval.ch.val != 0;
        j++;
    }
    ++k;
    g_max_iterations = values[k].uval.Lval;
    if (g_max_iterations < 0)
    {
        g_max_iterations = old_max_iterations;
    }
    g_max_iterations = std::max(g_max_iterations, 2L);

    if (g_max_iterations != old_max_iterations)
    {
        j++;
    }

    g_inside_color = values[++k].uval.ival;
    if (g_inside_color < COLOR_BLACK)
    {
        g_inside_color = -g_inside_color;
    }
    if (g_inside_color >= g_colors)
    {
        g_inside_color = (g_inside_color % g_colors) + (g_inside_color / g_colors);
    }

    {
        int tmp = values[++k].uval.ch.val;
        if (tmp > 0)
        {
            switch (tmp)
            {
            case 1:
                g_inside_color = ITER;
                break;
            case 2:
                g_inside_color = ZMAG;
                break;
            case 3:
                g_inside_color = BOF60;
                break;
            case 4:
                g_inside_color = BOF61;
                break;
            case 5:
                g_inside_color = EPS_CROSS;
                break;
            case 6:
                g_inside_color = STAR_TRAIL;
                break;
            case 7:
                g_inside_color = PERIOD;
                break;
            case 8:
                g_inside_color = ATANI;
                break;
            case 9:
                g_inside_color = FMODI;
                break;
            }
        }
    }
    if (g_inside_color != old_inside)
    {
        j++;
    }

    g_outside_color = values[++k].uval.ival;
    if (g_outside_color < COLOR_BLACK)
    {
        g_outside_color = -g_outside_color;
    }
    if (g_outside_color >= g_colors)
    {
        g_outside_color = (g_outside_color % g_colors) + (g_outside_color / g_colors);
    }

    {
        int tmp = values[++k].uval.ch.val;
        if (tmp > 0)
        {
            g_outside_color = -tmp;
        }
    }
    if (g_outside_color != old_outside)
    {
        j++;
    }

    g_save_filename = std::string{g_save_filename.c_str(), save_name_ptr} + values[++k].uval.sval;
    if (std::strcmp(g_save_filename.c_str(), prev_save_name) != 0)
    {
        g_resave_flag = 0;
        g_started_resaves = false; // forget pending increment
    }
    g_overwrite_file = values[++k].uval.ch.val != 0;

    g_sound_flag = ((g_sound_flag >> 3) << 3) | (values[++k].uval.ch.val);
    if (g_sound_flag != old_sound_flag && ((g_sound_flag & SOUNDFLAG_ORBIT_MASK) > SOUNDFLAG_BEEP || (old_sound_flag & SOUNDFLAG_ORBIT_MASK) > SOUNDFLAG_BEEP))
    {
        j++;
    }

    g_log_map_flag = values[++k].uval.Lval;
    if (g_log_map_flag != old_log_map_flag)
    {
        j++;
        g_log_map_auto_calculate = false;          // turn it off, use the supplied value
    }

    g_user_biomorph_value = values[++k].uval.ival;
    if (g_user_biomorph_value >= g_colors)
    {
        g_user_biomorph_value = (g_user_biomorph_value % g_colors) + (g_user_biomorph_value / g_colors);
    }
    if (g_user_biomorph_value != old_biomorph)
    {
        j++;
    }

    g_decomp[0] = values[++k].uval.ival;
    if (g_decomp[0] != old_decomp)
    {
        j++;
    }

    if (std::strncmp(string_lower(values[++k].uval.sval), "normal", 4) == 0)
    {
        g_fill_color = -1;
    }
    else
    {
        g_fill_color = std::atoi(values[k].uval.sval);
    }
    if (g_fill_color < 0)
    {
        g_fill_color = -1;
    }
    if (g_fill_color >= g_colors)
    {
        g_fill_color = (g_fill_color % g_colors) + (g_fill_color / g_colors);
    }
    if (g_fill_color != old_fill_color)
    {
        j++;
    }

    ++k;
    g_close_proximity = values[k].uval.dval;
    if (g_close_proximity != old_close_proximity)
    {
        j++;
    }

    int tmp = values[++k].uval.ch.val;
    if (tmp > 0)
    {
        switch (tmp)
        {
        case 0:
            g_perturbation = PerturbationMode::AUTO;
            break;
        case 1:
            g_perturbation = PerturbationMode::YES;
            break;
        case 2:
            g_perturbation = PerturbationMode::NO;
            break;
        }
    }

    if (old_perturbation != g_perturbation)
    {
        j++;
    }

    g_perturbation_tolerance = values[++k].uval.dval;
    if (old_perturbation_tolerance != g_perturbation_tolerance)
    {
        j++;
    }

    return j;
}
