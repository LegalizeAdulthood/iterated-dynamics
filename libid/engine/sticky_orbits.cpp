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

OrbitDrawMode g_draw_mode{OrbitDrawMode::RECTANGLE};

static int orbit_draw_rectangle()
{
    // draw a rectangle
    g_row = g_begin_pt.y;
    g_col = g_begin_pt.x;

    while (g_row <= g_i_stop_pt.y)
    {
        g_current_row = g_row;
        while (g_col <= g_i_stop_pt.x)
        {
            if (plot_orbits2d() == -1)
            {
                add_work_list(
                    g_start_pt.x, g_start_pt.y, g_stop_pt.x, g_stop_pt.y, g_col, g_row, 0, g_work_symmetry);
                return -1; // interrupted
            }
            ++g_col;
        }
        g_col = g_i_start_pt.x;
        ++g_row;
    }

    return 0;
}

static int orbit_draw_line()
{
    int final;                               // final row or column number
    int g;                                   // used to test for new row or column
    int inc1;                                // G increment when row or column doesn't change
    int inc2;                                // G increment when row or column changes
    int dx = g_i_stop_pt.x - g_i_start_pt.x; // find vector components
    int dy = g_i_stop_pt.y - g_i_start_pt.y;
    bool pos_slope = dx > 0;                 // is slope positive?
    if (dy < 0)
    {
        pos_slope = !pos_slope;
    }
    if (std::abs(dx) > std::abs(dy)) // shallow line case
    {
        if (dx > 0)                  // determine start point and last column
        {
            g_col = g_begin_pt.x;
            g_row = g_begin_pt.y;
            final = g_i_stop_pt.x;
        }
        else
        {
            g_col = g_i_stop_pt.x;
            g_row = g_i_stop_pt.y;
            final = g_begin_pt.x;
        }
        inc1 = 2 * std::abs(dy); // determine increments and initial G
        g = inc1 - std::abs(dx);
        inc2 = 2 * (std::abs(dy) - std::abs(dx));
        if (pos_slope)
        {
            while (g_col <= final) // step through columns checking for new row
            {
                if (plot_orbits2d() == -1)
                {
                    add_work_list(g_start_pt.x, g_start_pt.y, g_stop_pt.x, g_stop_pt.y, g_col, g_row, 0,
                        g_work_symmetry);
                    return -1; // interrupted
                }
                g_col++;
                if (g >= 0)    // it's time to change rows
                {
                    g_row++;   // positive slope so increment through the rows
                    g += inc2;
                }
                else           // stay at the same row
                {
                    g += inc1;
                }
            }
        }
        else
        {
            while (g_col <= final) // step through columns checking for new row
            {
                if (plot_orbits2d() == -1)
                {
                    add_work_list(g_start_pt.x, g_start_pt.y, g_stop_pt.x, g_stop_pt.y, g_col, g_row, 0,
                        g_work_symmetry);
                    return -1; // interrupted
                }
                g_col++;
                if (g > 0)     // it's time to change rows
                {
                    g_row--;   // negative slope so decrement through the rows
                    g += inc2;
                }
                else           // stay at the same row
                {
                    g += inc1;
                }
            }
        }
    } // if |dX| > |dY|
    else            // steep line case
    {
        if (dy > 0) // determine start point and last row
        {
            g_col = g_begin_pt.x;
            g_row = g_begin_pt.y;
            final = g_i_stop_pt.y;
        }
        else
        {
            g_col = g_i_stop_pt.x;
            g_row = g_i_stop_pt.y;
            final = g_begin_pt.y;
        }
        inc1 = 2 * std::abs(dx); // determine increments and initial G
        g = inc1 - std::abs(dy);
        inc2 = 2 * (std::abs(dx) - std::abs(dy));
        if (pos_slope)
        {
            while (g_row <= final) // step through rows checking for new column
            {
                if (plot_orbits2d() == -1)
                {
                    add_work_list(g_start_pt.x, g_start_pt.y, g_stop_pt.x, g_stop_pt.y, g_col, g_row, 0,
                        g_work_symmetry);
                    return -1; // interrupted
                }
                g_row++;
                if (g >= 0)    // it's time to change columns
                {
                    g_col++;   // positive slope so increment through the columns
                    g += inc2;
                }
                else           // stay at the same column
                {
                    g += inc1;
                }
            }
        }
        else
        {
            while (g_row <= final) // step through rows checking for new column
            {
                if (plot_orbits2d() == -1)
                {
                    add_work_list(g_start_pt.x, g_start_pt.y, g_stop_pt.x, g_stop_pt.y, g_col, g_row, 0,
                        g_work_symmetry);
                    return -1; // interrupted
                }
                g_row++;
                if (g > 0)     // it's time to change columns
                {
                    g_col--;   // negative slope so decrement through the columns
                    g += inc2;
                }
                else           // stay at the same column
                {
                    g += inc1;
                }
            }
        }
    }
    return 0;
}

// this code does not yet work???
static int orbit_draw_function()
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

    int angle = g_begin_pt.x; // save angle in x parameter

    cvt_center_mag(x_ctr, y_ctr, magnification, x_mag_factor, rotation, skew);
    if (rotation <= 0)
    {
        rotation += 360;
    }

    while (angle < rotation)
    {
        double theta = (double) angle * factor;
        g_col = (int) (x_factor + (x_ctr + x_mag_factor * std::cos(theta)));
        g_row = (int) (y_factor + (y_ctr + x_mag_factor * std::sin(theta)));
        if (plot_orbits2d() == -1)
        {
            add_work_list(angle, 0, 0, 0, 0, 0, 0, g_work_symmetry);
            return -1; // interrupted
        }
        angle++;
    }
    return 0;
}

int sticky_orbits()
{
    g_passes = Passes::ORBITS; // for <tab> screen
    g_total_passes = 1;

    if (plot_orbits2d_setup() == -1)
    {
        g_std_calc_mode = CalcMode::SOLID_GUESS;
        return -1;
    }

    switch (g_draw_mode)
    {
    case OrbitDrawMode::RECTANGLE:
    default:
        return orbit_draw_rectangle();

    case OrbitDrawMode::LINE:
        return orbit_draw_line();

    case OrbitDrawMode::FUNCTION:
        return orbit_draw_function();
    }
}
