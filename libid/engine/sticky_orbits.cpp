// SPDX-License-Identifier: GPL-3.0-only
//
#include "engine/sticky_orbits.h"

#include "engine/calcfrac.h"
#include "engine/convert_center_mag.h"
#include "engine/id_data.h"
#include "engine/work_list.h"
#include "fractals/lorenz.h"
#include "misc/id.h"

#include <cmath>

char g_draw_mode{'r'};

int sticky_orbits()
{
    g_got_status = StatusValues::ORBITS; // for <tab> screen
    g_total_passes = 1;

    if (plot_orbits2d_setup() == -1)
    {
        g_std_calc_mode = 'g';
        return -1;
    }

    switch (g_draw_mode)
    {
    case 'r':
    default:
        // draw a rectangle
        g_row = g_yy_begin;
        g_col = g_xx_begin;

        while (g_row <= g_i_y_stop)
        {
            g_current_row = g_row;
            while (g_col <= g_i_x_stop)
            {
                if (plot_orbits2d_float() == -1)
                {
                    add_work_list(
                        g_xx_start, g_yy_start, g_xx_stop, g_yy_stop, g_col, g_row, 0, g_work_symmetry);
                    return -1; // interrupted
                }
                ++g_col;
            }
            g_col = g_i_x_start;
            ++g_row;
        }
        break;
    case 'l':
    {
        int final; // final row or column number
        int g;     // used to test for new row or column
        int inc1;  // G increment when row or column doesn't change
        int inc2;  // G increment when row or column changes
        int dx = g_i_x_stop - g_i_x_start;                   // find vector components
        int dy = g_i_y_stop - g_i_y_start;
        bool pos_slope = dx > 0;                   // is slope positive?
        if (dy < 0)
        {
            pos_slope = !pos_slope;
        }
        if (std::abs(dx) > std::abs(dy))                  // shallow line case
        {
            if (dx > 0)         // determine start point and last column
            {
                g_col = g_xx_begin;
                g_row = g_yy_begin;
                final = g_i_x_stop;
            }
            else
            {
                g_col = g_i_x_stop;
                g_row = g_i_y_stop;
                final = g_xx_begin;
            }
            inc1 = 2 * std::abs(dy);             // determine increments and initial G
            g = inc1 - std::abs(dx);
            inc2 = 2 * (std::abs(dy) - std::abs(dx));
            if (pos_slope)
            {
                while (g_col <= final)    // step through columns checking for new row
                {
                    if (plot_orbits2d_float() == -1)
                    {
                        add_work_list(
                            g_xx_start, g_yy_start, g_xx_stop, g_yy_stop, g_col, g_row, 0, g_work_symmetry);
                        return -1; // interrupted
                    }
                    g_col++;
                    if (g >= 0)             // it's time to change rows
                    {
                        g_row++;      // positive slope so increment through the rows
                        g += inc2;
                    }
                    else                          // stay at the same row
                    {
                        g += inc1;
                    }
                }
            }
            else
            {
                while (g_col <= final)    // step through columns checking for new row
                {
                    if (plot_orbits2d_float() == -1)
                    {
                        add_work_list(
                            g_xx_start, g_yy_start, g_xx_stop, g_yy_stop, g_col, g_row, 0, g_work_symmetry);
                        return -1; // interrupted
                    }
                    g_col++;
                    if (g > 0)              // it's time to change rows
                    {
                        g_row--;      // negative slope so decrement through the rows
                        g += inc2;
                    }
                    else                          // stay at the same row
                    {
                        g += inc1;
                    }
                }
            }
        }   // if |dX| > |dY|
        else                            // steep line case
        {
            if (dy > 0)             // determine start point and last row
            {
                g_col = g_xx_begin;
                g_row = g_yy_begin;
                final = g_i_y_stop;
            }
            else
            {
                g_col = g_i_x_stop;
                g_row = g_i_y_stop;
                final = g_yy_begin;
            }
            inc1 = 2 * std::abs(dx);             // determine increments and initial G
            g = inc1 - std::abs(dy);
            inc2 = 2 * (std::abs(dx) - std::abs(dy));
            if (pos_slope)
            {
                while (g_row <= final)    // step through rows checking for new column
                {
                    if (plot_orbits2d_float() == -1)
                    {
                        add_work_list(
                            g_xx_start, g_yy_start, g_xx_stop, g_yy_stop, g_col, g_row, 0, g_work_symmetry);
                        return -1; // interrupted
                    }
                    g_row++;
                    if (g >= 0)                 // it's time to change columns
                    {
                        g_col++;  // positive slope so increment through the columns
                        g += inc2;
                    }
                    else                      // stay at the same column
                    {
                        g += inc1;
                    }
                }
            }
            else
            {
                while (g_row <= final)    // step through rows checking for new column
                {
                    if (plot_orbits2d_float() == -1)
                    {
                        add_work_list(
                            g_xx_start, g_yy_start, g_xx_stop, g_yy_stop, g_col, g_row, 0, g_work_symmetry);
                        return -1; // interrupted
                    }
                    g_row++;
                    if (g > 0)                  // it's time to change columns
                    {
                        g_col--;  // negative slope so decrement through the columns
                        g += inc2;
                    }
                    else                      // stay at the same column
                    {
                        g += inc1;
                    }
                }
            }
        }
        break;
    } // end case 'l'

    case 'f':  // this code does not yet work???
    {
        double x_ctr;
        double y_ctr;
        LDouble magnification; // LDouble not really needed here, but used to match function parameters
        double x_mag_factor;
        double rotation;
        double skew;
        double factor = PI / 180.0;
        double x_factor = g_logical_screen_x_dots / 2.0;
        double y_factor = g_logical_screen_y_dots / 2.0;

        int angle = g_xx_begin;  // save angle in x parameter

        cvt_center_mag(x_ctr, y_ctr, magnification, x_mag_factor, rotation, skew);
        if (rotation <= 0)
        {
            rotation += 360;
        }

        while (angle < rotation)
        {
            double theta = (double) angle * factor;
            g_col = (int)(x_factor + (x_ctr + x_mag_factor * std::cos(theta)));
            g_row = (int)(y_factor + (y_ctr + x_mag_factor * std::sin(theta)));
            if (plot_orbits2d_float() == -1)
            {
                add_work_list(angle, 0, 0, 0, 0, 0, 0, g_work_symmetry);
                return -1; // interrupted
            }
            angle++;
        }
        break;
    }  // end case 'f'
    }  // end switch

    return 0;
}
