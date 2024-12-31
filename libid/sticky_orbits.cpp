// SPDX-License-Identifier: GPL-3.0-only
//
#include "sticky_orbits.h"

#include "calcfrac.h"
#include "convert_center_mag.h"
#include "id.h"
#include "id_data.h"
#include "lorenz.h"
#include "work_list.h"

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
                    add_work_list(g_xx_start, g_xx_stop, g_col, g_yy_start, g_yy_stop, g_row, 0, g_work_symmetry);
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
        int dX;
        int dY;    // vector components
        int final;
        int // final row or column number
            G;
        int // used to test for new row or column
            inc1;
        int       // G increment when row or column doesn't change
            inc2; // G increment when row or column changes
        char pos_slope;

        dX = g_i_x_stop - g_i_x_start;                   // find vector components
        dY = g_i_y_stop - g_i_y_start;
        pos_slope = (char)(dX > 0);                   // is slope positive?
        if (dY < 0)
        {
            pos_slope = (char)!pos_slope;
        }
        if (std::abs(dX) > std::abs(dY))                  // shallow line case
        {
            if (dX > 0)         // determine start point and last column
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
            inc1 = 2 * std::abs(dY);             // determine increments and initial G
            G = inc1 - std::abs(dX);
            inc2 = 2 * (std::abs(dY) - std::abs(dX));
            if (pos_slope)
            {
                while (g_col <= final)    // step through columns checking for new row
                {
                    if (plot_orbits2d_float() == -1)
                    {
                        add_work_list(g_xx_start, g_xx_stop, g_col, g_yy_start, g_yy_stop, g_row, 0, g_work_symmetry);
                        return -1; // interrupted
                    }
                    g_col++;
                    if (G >= 0)             // it's time to change rows
                    {
                        g_row++;      // positive slope so increment through the rows
                        G += inc2;
                    }
                    else                          // stay at the same row
                    {
                        G += inc1;
                    }
                }
            }
            else
            {
                while (g_col <= final)    // step through columns checking for new row
                {
                    if (plot_orbits2d_float() == -1)
                    {
                        add_work_list(g_xx_start, g_xx_stop, g_col, g_yy_start, g_yy_stop, g_row, 0, g_work_symmetry);
                        return -1; // interrupted
                    }
                    g_col++;
                    if (G > 0)              // it's time to change rows
                    {
                        g_row--;      // negative slope so decrement through the rows
                        G += inc2;
                    }
                    else                          // stay at the same row
                    {
                        G += inc1;
                    }
                }
            }
        }   // if |dX| > |dY|
        else                            // steep line case
        {
            if (dY > 0)             // determine start point and last row
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
            inc1 = 2 * std::abs(dX);             // determine increments and initial G
            G = inc1 - std::abs(dY);
            inc2 = 2 * (std::abs(dX) - std::abs(dY));
            if (pos_slope)
            {
                while (g_row <= final)    // step through rows checking for new column
                {
                    if (plot_orbits2d_float() == -1)
                    {
                        add_work_list(g_xx_start, g_xx_stop, g_col, g_yy_start, g_yy_stop, g_row, 0, g_work_symmetry);
                        return -1; // interrupted
                    }
                    g_row++;
                    if (G >= 0)                 // it's time to change columns
                    {
                        g_col++;  // positive slope so increment through the columns
                        G += inc2;
                    }
                    else                      // stay at the same column
                    {
                        G += inc1;
                    }
                }
            }
            else
            {
                while (g_row <= final)    // step through rows checking for new column
                {
                    if (plot_orbits2d_float() == -1)
                    {
                        add_work_list(g_xx_start, g_xx_stop, g_col, g_yy_start, g_yy_stop, g_row, 0, g_work_symmetry);
                        return -1; // interrupted
                    }
                    g_row++;
                    if (G > 0)                  // it's time to change columns
                    {
                        g_col--;  // negative slope so decrement through the columns
                        G += inc2;
                    }
                    else                      // stay at the same column
                    {
                        G += inc1;
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
        int angle;
        double factor = PI / 180.0;
        double theta;
        double xfactor = g_logical_screen_x_dots / 2.0;
        double yfactor = g_logical_screen_y_dots / 2.0;

        angle = g_xx_begin;  // save angle in x parameter

        cvt_center_mag(x_ctr, y_ctr, magnification, x_mag_factor, rotation, skew);
        if (rotation <= 0)
        {
            rotation += 360;
        }

        while (angle < rotation)
        {
            theta = (double)angle * factor;
            g_col = (int)(xfactor + (x_ctr + x_mag_factor * std::cos(theta)));
            g_row = (int)(yfactor + (y_ctr + x_mag_factor * std::sin(theta)));
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
