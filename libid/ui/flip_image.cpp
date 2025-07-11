// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/flip_image.h"

#include "engine/cmdfiles.h"
#include "engine/id_data.h"
#include "fractals/fractalp.h"
#include "misc/Driver.h"
#include "ui/id_keys.h"
#include "ui/video.h"
#include "ui/zoom.h"

/* This routine copies the current screen to by flipping the x-axis, y-axis,
   or both. Refuses to work if calculation in progress or if fractal
   non-resumable. Clears zoombox if any. Resets corners so resulting fractal
   is still valid. */
MainState flip_image(MainContext &context)
{
    int temp_dot;

    // fractal must be rotate-able and be finished
    if (bit_set(g_cur_fractal_specific->flags, FractalFlags::NO_ROTATE) //
        || g_calc_status == CalcStatus::IN_PROGRESS              //
        || g_calc_status == CalcStatus::RESUMABLE)
    {
        return MainState::NOTHING;
    }
    if (g_bf_math != BFMathType::NONE)
    {
        clear_zoom_box(); // clear, don't copy, the zoombox
    }
    int x_half = g_logical_screen_x_dots / 2;
    int y_half = g_logical_screen_y_dots / 2;
    switch (context.key)
    {
    case ID_KEY_CTL_X:            // control-X - reverse X-axis
        for (int i = 0; i < x_half; i++)
        {
            if (driver_key_pressed())
            {
                break;
            }
            for (int j = 0; j < g_logical_screen_y_dots; j++)
            {
                temp_dot = get_color(i, j);
                g_put_color(i, j, get_color(g_logical_screen_x_dots-1-i, j));
                g_put_color(g_logical_screen_x_dots-1-i, j, temp_dot);
            }
        }
        g_save_x_min = g_x_max + g_x_min - g_x_3rd;
        g_save_y_max = g_y_max + g_y_min - g_y_3rd;
        g_save_x_max = g_x_3rd;
        g_save_y_min = g_y_3rd;
        g_save_x_3rd = g_x_max;
        g_save_y_3rd = g_y_min;
        if (g_bf_math != BFMathType::NONE)
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
        for (int j = 0; j < y_half; j++)
        {
            if (driver_key_pressed())
            {
                break;
            }
            for (int i = 0; i < g_logical_screen_x_dots; i++)
            {
                temp_dot = get_color(i, j);
                g_put_color(i, j, get_color(i, g_logical_screen_y_dots-1-j));
                g_put_color(i, g_logical_screen_y_dots-1-j, temp_dot);
            }
        }
        g_save_x_min = g_x_3rd;
        g_save_y_max = g_y_3rd;
        g_save_x_max = g_x_max + g_x_min - g_x_3rd;
        g_save_y_min = g_y_max + g_y_min - g_y_3rd;
        g_save_x_3rd = g_x_min;
        g_save_y_3rd = g_y_max;
        if (g_bf_math != BFMathType::NONE)
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
        for (int i = 0; i < x_half; i++)
        {
            if (driver_key_pressed())
            {
                break;
            }
            for (int j = 0; j < g_logical_screen_y_dots; j++)
            {
                temp_dot = get_color(i, j);
                g_put_color(i, j, get_color(g_logical_screen_x_dots-1-i, g_logical_screen_y_dots-1-j));
                g_put_color(g_logical_screen_x_dots-1-i, g_logical_screen_y_dots-1-j, temp_dot);
            }
        }
        g_save_x_min = g_x_max;
        g_save_y_max = g_y_min;
        g_save_x_max = g_x_min;
        g_save_y_min = g_y_max;
        g_save_x_3rd = g_x_max + g_x_min - g_x_3rd;
        g_save_y_3rd = g_y_max + g_y_min - g_y_3rd;
        if (g_bf_math != BFMathType::NONE)
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
    g_calc_status = CalcStatus::PARAMS_CHANGED;
    return MainState::NOTHING;
}
