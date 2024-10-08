// SPDX-License-Identifier: GPL-3.0-only
//
#include "flip_image.h"

#include "port.h"
#include "prototyp.h"

#include "calcfrac.h"
#include "cmdfiles.h"
#include "drivers.h"
#include "fractalp.h"
#include "framain2.h"
#include "id.h"
#include "id_data.h"
#include "id_keys.h"
#include "video.h"
#include "zoom.h"

/* This routine copies the current screen to by flipping x-axis, y-axis,
   or both. Refuses to work if calculation in progress or if fractal
   non-resumable. Clears zoombox if any. Resets corners so resulting fractal
   is still valid. */
main_state flip_image(int &key, bool &, bool &, bool &)
{
    int ixhalf;
    int iyhalf;
    int tempdot;

    // fractal must be rotate-able and be finished
    if (bit_set(g_cur_fractal_specific->flags, fractal_flags::NOROTATE) //
        || g_calc_status == calc_status_value::IN_PROGRESS              //
        || g_calc_status == calc_status_value::RESUMABLE)
    {
        return main_state::NOTHING;
    }
    if (g_bf_math != bf_math_type::NONE)
    {
        clear_zoom_box(); // clear, don't copy, the zoombox
    }
    ixhalf = g_logical_screen_x_dots / 2;
    iyhalf = g_logical_screen_y_dots / 2;
    switch (key)
    {
    case ID_KEY_CTL_X:            // control-X - reverse X-axis
        for (int i = 0; i < ixhalf; i++)
        {
            if (driver_key_pressed())
            {
                break;
            }
            for (int j = 0; j < g_logical_screen_y_dots; j++)
            {
                tempdot = getcolor(i, j);
                g_put_color(i, j, getcolor(g_logical_screen_x_dots-1-i, j));
                g_put_color(g_logical_screen_x_dots-1-i, j, tempdot);
            }
        }
        g_save_x_min = g_x_max + g_x_min - g_x_3rd;
        g_save_y_max = g_y_max + g_y_min - g_y_3rd;
        g_save_x_max = g_x_3rd;
        g_save_y_min = g_y_3rd;
        g_save_x_3rd = g_x_max;
        g_save_y_3rd = g_y_min;
        if (g_bf_math != bf_math_type::NONE)
        {
            add_bf(g_bf_save_x_min, g_bf_x_max, g_bf_x_min); // sxmin = xxmax + xxmin - xx3rd;
            sub_a_bf(g_bf_save_x_min, g_bf_x_3rd);
            add_bf(g_bf_save_y_max, g_bf_y_max, g_bf_y_min); // symax = yymax + yymin - yy3rd;
            sub_a_bf(g_bf_save_y_max, g_bf_y_3rd);
            copy_bf(g_bf_save_x_max, g_bf_x_3rd);        // sxmax = xx3rd;
            copy_bf(g_bf_save_y_min, g_bf_y_3rd);        // symin = yy3rd;
            copy_bf(g_bf_save_x_3rd, g_bf_x_max);        // sx3rd = xxmax;
            copy_bf(g_bf_save_y_3rd, g_bf_y_min);        // sy3rd = yymin;
        }
        break;
    case ID_KEY_CTL_Y:            // control-Y - reverse Y-aXis
        for (int j = 0; j < iyhalf; j++)
        {
            if (driver_key_pressed())
            {
                break;
            }
            for (int i = 0; i < g_logical_screen_x_dots; i++)
            {
                tempdot = getcolor(i, j);
                g_put_color(i, j, getcolor(i, g_logical_screen_y_dots-1-j));
                g_put_color(i, g_logical_screen_y_dots-1-j, tempdot);
            }
        }
        g_save_x_min = g_x_3rd;
        g_save_y_max = g_y_3rd;
        g_save_x_max = g_x_max + g_x_min - g_x_3rd;
        g_save_y_min = g_y_max + g_y_min - g_y_3rd;
        g_save_x_3rd = g_x_min;
        g_save_y_3rd = g_y_max;
        if (g_bf_math != bf_math_type::NONE)
        {
            copy_bf(g_bf_save_x_min, g_bf_x_3rd);        // sxmin = xx3rd;
            copy_bf(g_bf_save_y_max, g_bf_y_3rd);        // symax = yy3rd;
            add_bf(g_bf_save_x_max, g_bf_x_max, g_bf_x_min); // sxmax = xxmax + xxmin - xx3rd;
            sub_a_bf(g_bf_save_x_max, g_bf_x_3rd);
            add_bf(g_bf_save_y_min, g_bf_y_max, g_bf_y_min); // symin = yymax + yymin - yy3rd;
            sub_a_bf(g_bf_save_y_min, g_bf_y_3rd);
            copy_bf(g_bf_save_x_3rd, g_bf_x_min);        // sx3rd = xxmin;
            copy_bf(g_bf_save_y_3rd, g_bf_y_max);        // sy3rd = yymax;
        }
        break;
    case ID_KEY_CTL_Z:            // control-Z - reverse X and Y aXis
        for (int i = 0; i < ixhalf; i++)
        {
            if (driver_key_pressed())
            {
                break;
            }
            for (int j = 0; j < g_logical_screen_y_dots; j++)
            {
                tempdot = getcolor(i, j);
                g_put_color(i, j, getcolor(g_logical_screen_x_dots-1-i, g_logical_screen_y_dots-1-j));
                g_put_color(g_logical_screen_x_dots-1-i, g_logical_screen_y_dots-1-j, tempdot);
            }
        }
        g_save_x_min = g_x_max;
        g_save_y_max = g_y_min;
        g_save_x_max = g_x_min;
        g_save_y_min = g_y_max;
        g_save_x_3rd = g_x_max + g_x_min - g_x_3rd;
        g_save_y_3rd = g_y_max + g_y_min - g_y_3rd;
        if (g_bf_math != bf_math_type::NONE)
        {
            copy_bf(g_bf_save_x_min, g_bf_x_max);        // sxmin = xxmax;
            copy_bf(g_bf_save_y_max, g_bf_y_min);        // symax = yymin;
            copy_bf(g_bf_save_x_max, g_bf_x_min);        // sxmax = xxmin;
            copy_bf(g_bf_save_y_min, g_bf_y_max);        // symin = yymax;
            add_bf(g_bf_save_x_3rd, g_bf_x_max, g_bf_x_min); // sx3rd = xxmax + xxmin - xx3rd;
            sub_a_bf(g_bf_save_x_3rd, g_bf_x_3rd);
            add_bf(g_bf_save_y_3rd, g_bf_y_max, g_bf_y_min); // sy3rd = yymax + yymin - yy3rd;
            sub_a_bf(g_bf_save_y_3rd, g_bf_y_3rd);
        }
        break;
    }
    reset_zoom_corners();
    g_calc_status = calc_status_value::PARAMS_CHANGED;
    return main_state::NOTHING;
}
