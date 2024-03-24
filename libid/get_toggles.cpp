#include "get_toggles.h"

#include "port.h"
#include "prototyp.h"

#include "calcfrac.h"
#include "cmdfiles.h"
#include "fractype.h"
#include "full_screen_prompt.h"
#include "helpdefs.h"
#include "id.h"
#include "id_data.h"

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
    char prevsavename[FILE_MAX_DIR+1];
    char const *savenameptr;
    fullscreenvalues uvalues[25];
    int i, j, k;
    char old_usr_stdcalcmode;
    long old_maxit, old_logflag;
    int old_inside, old_outside, old_soundflag;
    int old_biomorph, old_decomp;
    int old_fillcolor;
    int old_stoppass;
    double old_closeprox;
    char const *calcmodes[] = {"1", "2", "3", "g", "g1", "g2", "g3", "g4", "g5", "g6", "b", "s", "t", "d", "o"};
    char const *soundmodes[5] = {"off", "beep", "x", "y", "z"};
    char const *insidemodes[] = {"numb", "maxiter", "zmag", "bof60", "bof61", "epsiloncross",
                          "startrail", "period", "atan", "fmod"
                         };
    char const *outsidemodes[] = {"numb", "iter", "real", "imag", "mult", "summ", "atan",
                           "fmod", "tdis"
                          };

    k = -1;

    choices[++k] = "Passes (1,2,3, g[uess], b[ound], t[ess], d[iffu], o[rbit])";
    uvalues[k].type = 'l';
    uvalues[k].uval.ch.vlen = 3;
    uvalues[k].uval.ch.llen = sizeof(calcmodes)/sizeof(*calcmodes);
    uvalues[k].uval.ch.list = calcmodes;
    uvalues[k].uval.ch.val =
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
        :        /* "o"rbits */      14;
    old_usr_stdcalcmode = g_user_std_calc_mode;
    old_stoppass = g_stop_pass;
    choices[++k] = "Floating Point Algorithm";
    uvalues[k].type = 'y';
    uvalues[k].uval.ch.val = g_user_float_flag ? 1 : 0;
    choices[++k] = "Maximum Iterations (2 to 2,147,483,647)";
    uvalues[k].type = 'L';
    old_maxit = g_max_iterations;
    uvalues[k].uval.Lval = old_maxit;

    choices[++k] = "Inside Color (0-# of colors, if Inside=numb)";
    uvalues[k].type = 'i';
    if (g_inside_color >= COLOR_BLACK)
    {
        uvalues[k].uval.ival = g_inside_color;
    }
    else
    {
        uvalues[k].uval.ival = 0;
    }

    choices[++k] = "Inside (numb,maxit,zmag,bof60,bof61,epscr,star,per,atan,fmod)";
    uvalues[k].type = 'l';
    uvalues[k].uval.ch.vlen = 12;
    uvalues[k].uval.ch.llen = sizeof(insidemodes)/sizeof(*insidemodes);
    uvalues[k].uval.ch.list = insidemodes;
    if (g_inside_color >= COLOR_BLACK)    // numb
    {
        uvalues[k].uval.ch.val = 0;
    }
    else if (g_inside_color == ITER)
    {
        uvalues[k].uval.ch.val = 1;
    }
    else if (g_inside_color == ZMAG)
    {
        uvalues[k].uval.ch.val = 2;
    }
    else if (g_inside_color == BOF60)
    {
        uvalues[k].uval.ch.val = 3;
    }
    else if (g_inside_color == BOF61)
    {
        uvalues[k].uval.ch.val = 4;
    }
    else if (g_inside_color == EPSCROSS)
    {
        uvalues[k].uval.ch.val = 5;
    }
    else if (g_inside_color == STARTRAIL)
    {
        uvalues[k].uval.ch.val = 6;
    }
    else if (g_inside_color == PERIOD)
    {
        uvalues[k].uval.ch.val = 7;
    }
    else if (g_inside_color == ATANI)
    {
        uvalues[k].uval.ch.val = 8;
    }
    else if (g_inside_color == FMODI)
    {
        uvalues[k].uval.ch.val = 9;
    }
    old_inside = g_inside_color;

    choices[++k] = "Outside Color (0-# of colors, if Outside=numb)";
    uvalues[k].type = 'i';
    if (g_outside_color >= COLOR_BLACK)
    {
        uvalues[k].uval.ival = g_outside_color;
    }
    else
    {
        uvalues[k].uval.ival = 0;
    }

    choices[++k] = "Outside (numb,iter,real,imag,mult,summ,atan,fmod,tdis)";
    uvalues[k].type = 'l';
    uvalues[k].uval.ch.vlen = 4;
    uvalues[k].uval.ch.llen = sizeof(outsidemodes)/sizeof(*outsidemodes);
    uvalues[k].uval.ch.list = outsidemodes;
    if (g_outside_color >= COLOR_BLACK)    // numb
    {
        uvalues[k].uval.ch.val = 0;
    }
    else
    {
        uvalues[k].uval.ch.val = -g_outside_color;
    }
    old_outside = g_outside_color;

    choices[++k] = "Savename (.GIF implied)";
    uvalues[k].type = 's';
    std::strcpy(prevsavename, g_save_filename.c_str());
    savenameptr = std::strrchr(g_save_filename.c_str(), SLASHC);
    if (savenameptr == nullptr)
    {
        savenameptr = g_save_filename.c_str();
    }
    else
    {
        savenameptr++; // point past slash
    }
    std::strcpy(uvalues[k].uval.sval, savenameptr);

    choices[++k] = "File Overwrite ('overwrite=')";
    uvalues[k].type = 'y';
    uvalues[k].uval.ch.val = g_overwrite_file ? 1 : 0;

    choices[++k] = "Sound (off, beep, x, y, z)";
    uvalues[k].type = 'l';
    uvalues[k].uval.ch.vlen = 4;
    uvalues[k].uval.ch.llen = 5;
    uvalues[k].uval.ch.list = soundmodes;
    uvalues[k].uval.ch.val = (old_soundflag = g_sound_flag) & SOUNDFLAG_ORBITMASK;

    if (g_iteration_ranges_len == 0)
    {
        choices[++k] = "Log Palette (0=no,1=yes,-1=old,+n=cmprsd,-n=sqrt, 2=auto)";
        uvalues[k].type = 'L';
    }
    else
    {
        choices[++k] = "Log Palette (n/a, ranges= parameter is in effect)";
        uvalues[k].type = '*';
    }
    old_logflag = g_log_map_flag;
    uvalues[k].uval.Lval = old_logflag;

    choices[++k] = "Biomorph Color (-1 means OFF)";
    uvalues[k].type = 'i';
    old_biomorph = g_user_biomorph_value;
    uvalues[k].uval.ival = old_biomorph;

    choices[++k] = "Decomp Option (2,4,8,..,256, 0=OFF)";
    uvalues[k].type = 'i';
    old_decomp = g_decomp[0];
    uvalues[k].uval.ival = old_decomp;

    choices[++k] = "Fill Color (normal,#) (works with passes=t, b and d)";
    uvalues[k].type = 's';
    if (g_fill_color < 0)
    {
        std::strcpy(uvalues[k].uval.sval, "normal");
    }
    else
    {
        std::sprintf(uvalues[k].uval.sval, "%d", g_fill_color);
    }
    old_fillcolor = g_fill_color;

    choices[++k] = "Proximity value for inside=epscross and fmod";
    uvalues[k].type = 'f'; // should be 'd', but prompts get messed up
    old_closeprox = g_close_proximity;
    uvalues[k].uval.dval = old_closeprox;

    help_labels const old_help_mode = g_help_mode;
    g_help_mode = help_labels::HELPXOPTS;
    i = fullscreen_prompt("Basic Options\n(not all combinations make sense)", k+1, choices, uvalues, 0, nullptr);
    g_help_mode = old_help_mode;
    if (i < 0)
    {
        return -1;
    }

    // now check out the results (*hopefully* in the same order <grin>)
    k = -1;
    j = 0;   // return code

    g_user_std_calc_mode = calcmodes[uvalues[++k].uval.ch.val][0];
    g_stop_pass = (int)calcmodes[uvalues[k].uval.ch.val][1] - (int)'0';

    if (g_stop_pass < 0 || g_stop_pass > 6 || g_user_std_calc_mode != 'g')
    {
        g_stop_pass = 0;
    }

    if (g_user_std_calc_mode == 'o' && g_fractal_type == fractal_type::LYAPUNOV)   // Oops,lyapunov type
    {
        // doesn't use 'new' & breaks orbits
        g_user_std_calc_mode = old_usr_stdcalcmode;
    }

    if (old_usr_stdcalcmode != g_user_std_calc_mode)
    {
        j++;
    }
    if (old_stoppass != g_stop_pass)
    {
        j++;
    }
    if ((uvalues[++k].uval.ch.val != 0) != g_user_float_flag)
    {
        g_user_float_flag = uvalues[k].uval.ch.val != 0;
        j++;
    }
    ++k;
    g_max_iterations = uvalues[k].uval.Lval;
    if (g_max_iterations < 0)
    {
        g_max_iterations = old_maxit;
    }
    if (g_max_iterations < 2)
    {
        g_max_iterations = 2;
    }

    if (g_max_iterations != old_maxit)
    {
        j++;
    }

    g_inside_color = uvalues[++k].uval.ival;
    if (g_inside_color < COLOR_BLACK)
    {
        g_inside_color = -g_inside_color;
    }
    if (g_inside_color >= g_colors)
    {
        g_inside_color = (g_inside_color % g_colors) + (g_inside_color / g_colors);
    }

    {
        int tmp;
        tmp = uvalues[++k].uval.ch.val;
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
                g_inside_color = EPSCROSS;
                break;
            case 6:
                g_inside_color = STARTRAIL;
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

    g_outside_color = uvalues[++k].uval.ival;
    if (g_outside_color < COLOR_BLACK)
    {
        g_outside_color = -g_outside_color;
    }
    if (g_outside_color >= g_colors)
    {
        g_outside_color = (g_outside_color % g_colors) + (g_outside_color / g_colors);
    }

    {
        int tmp;
        tmp = uvalues[++k].uval.ch.val;
        if (tmp > 0)
        {
            g_outside_color = -tmp;
        }
    }
    if (g_outside_color != old_outside)
    {
        j++;
    }

    g_save_filename = std::string{g_save_filename.c_str(), savenameptr} + uvalues[++k].uval.sval;
    if (std::strcmp(g_save_filename.c_str(), prevsavename))
    {
        g_resave_flag = 0;
        g_started_resaves = false; // forget pending increment
    }
    g_overwrite_file = uvalues[++k].uval.ch.val != 0;

    g_sound_flag = ((g_sound_flag >> 3) << 3) | (uvalues[++k].uval.ch.val);
    if (g_sound_flag != old_soundflag && ((g_sound_flag & SOUNDFLAG_ORBITMASK) > SOUNDFLAG_BEEP || (old_soundflag & SOUNDFLAG_ORBITMASK) > SOUNDFLAG_BEEP))
    {
        j++;
    }

    g_log_map_flag = uvalues[++k].uval.Lval;
    if (g_log_map_flag != old_logflag)
    {
        j++;
        g_log_map_auto_calculate = false;          // turn it off, use the supplied value
    }

    g_user_biomorph_value = uvalues[++k].uval.ival;
    if (g_user_biomorph_value >= g_colors)
    {
        g_user_biomorph_value = (g_user_biomorph_value % g_colors) + (g_user_biomorph_value / g_colors);
    }
    if (g_user_biomorph_value != old_biomorph)
    {
        j++;
    }

    g_decomp[0] = uvalues[++k].uval.ival;
    if (g_decomp[0] != old_decomp)
    {
        j++;
    }

    if (std::strncmp(strlwr(uvalues[++k].uval.sval), "normal", 4) == 0)
    {
        g_fill_color = -1;
    }
    else
    {
        g_fill_color = std::atoi(uvalues[k].uval.sval);
    }
    if (g_fill_color < 0)
    {
        g_fill_color = -1;
    }
    if (g_fill_color >= g_colors)
    {
        g_fill_color = (g_fill_color % g_colors) + (g_fill_color / g_colors);
    }
    if (g_fill_color != old_fillcolor)
    {
        j++;
    }

    ++k;
    g_close_proximity = uvalues[k].uval.dval;
    if (g_close_proximity != old_closeprox)
    {
        j++;
    }

    return j;
}
