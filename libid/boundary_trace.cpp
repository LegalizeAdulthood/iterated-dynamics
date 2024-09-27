// SPDX-License-Identifier: GPL-3.0-only
//
#include "boundary_trace.h"

#include "calcfrac.h"
#include "cmdfiles.h"
#include "id_data.h"
#include "stop_msg.h"
#include "video.h"
#include "work_list.h"

#include <cstring>

enum class direction
{
    North,
    East,
    South,
    West
};
inline int operator+(direction value)
{
    return static_cast<int>(value);
}

static direction s_going_to{};
static int s_trail_row{};
static int s_trail_col{};
static BYTE s_stack[4096]{}; // common temp, two put_line calls

// boundary trace method
constexpr int BK_COLOR{};

inline direction advance(direction dir, int increment)
{
    return static_cast<direction>((+dir + increment) & 0x03);
}

// take one step in the direction of going_to
static void step_col_row()
{
    switch (s_going_to)
    {
    case direction::North:
        g_col = s_trail_col;
        g_row = s_trail_row - 1;
        break;
    case direction::East:
        g_col = s_trail_col + 1;
        g_row = s_trail_row;
        break;
    case direction::South:
        g_col = s_trail_col;
        g_row = s_trail_row + 1;
        break;
    case direction::West:
        g_col = s_trail_col - 1;
        g_row = s_trail_row;
        break;
    }
}

int boundary_trace()
{
    if (g_inside_color == COLOR_BLACK || g_outside_color == COLOR_BLACK)
    {
        stopmsg("Boundary tracing cannot be used with inside=0 or outside=0");
        return -1;
    }
    if (g_colors < 16)
    {
        stopmsg("Boundary tracing cannot be used with < 16 colors");
        return -1;
    }

    int last_fillcolor_used = -1;
    g_got_status = status_values::BOUNDARY_TRACE;
    int max_putline_length = 0; // reset max_putline_length
    for (int cur_row = g_i_y_start; cur_row <= g_i_y_stop; cur_row++)
    {
        g_reset_periodicity = true; // reset for a new row
        g_color = BK_COLOR;
        for (int cur_col = g_i_x_start; cur_col <= g_i_x_stop; cur_col++)
        {
            if (getcolor(cur_col, cur_row) != BK_COLOR)
            {
                continue;
            }

            int trail_color = g_color;
            g_row = cur_row;
            g_col = cur_col;
            if ((*g_calc_type)() == -1) // g_color, g_row, g_col are global
            {
                if (g_show_dot != BK_COLOR)   // remove show dot pixel
                {
                    (*g_plot)(g_col, g_row, BK_COLOR);
                }
                if (g_i_y_stop != g_yy_stop)
                {
                    g_i_y_stop = g_yy_stop - (cur_row - g_yy_start); // allow for sym
                }
                add_worklist(g_xx_start, g_xx_stop, cur_col, cur_row, g_i_y_stop, cur_row, 0, g_work_symmetry);
                return -1;
            }
            g_reset_periodicity = false; // normal periodicity checking

            // This next line may cause a few more pixels to be calculated,
            // but at the savings of quite a bit of overhead
            if (g_color != trail_color)
            {
                continue;
            }

            // sweep clockwise to trace outline
            s_trail_row = cur_row;
            s_trail_col = cur_col;
            trail_color = g_color;
            const int fill_color_used = g_fill_color > 0 ? g_fill_color : trail_color;
            direction coming_from = direction::West;
            s_going_to = direction::East;
            unsigned int matches_found = 0;
            bool continue_loop = true;
            do
            {
                step_col_row();
                if (g_row >= cur_row        //
                    && g_col >= g_i_x_start //
                    && g_col <= g_i_x_stop  //
                    && g_row <= g_i_y_stop)
                {
                    g_color = getcolor(g_col, g_row);
                    // g_color, g_row, g_col are global for (*g_calc_type)()
                    if (g_color == BK_COLOR && (*g_calc_type)()== -1)
                    {
                        if (g_show_dot != BK_COLOR)   // remove show dot pixel
                        {
                            (*g_plot)(g_col, g_row, BK_COLOR);
                        }
                        if (g_i_y_stop != g_yy_stop)
                        {
                            g_i_y_stop = g_yy_stop - (cur_row - g_yy_start); // allow for sym
                        }
                        add_worklist(g_xx_start, g_xx_stop, cur_col, cur_row, g_i_y_stop, cur_row, 0, g_work_symmetry);
                        return -1;
                    }
                    if (g_color == trail_color)
                    {
                        if (matches_found < 4)   // to keep it from overflowing
                        {
                            matches_found++;
                        }
                        s_trail_row = g_row;
                        s_trail_col = g_col;
                        s_going_to = advance(s_going_to, -1);
                        coming_from = advance(s_going_to, -1);
                    }
                    else
                    {
                        s_going_to = advance(s_going_to, 1);
                        continue_loop = s_going_to != coming_from || matches_found > 0;
                    }
                }
                else
                {
                    s_going_to = advance(s_going_to, 1);
                    continue_loop = s_going_to != coming_from || matches_found > 0;
                }
            } while (continue_loop && (g_col != cur_col || g_row != cur_row));

            if (matches_found <= 3)
            {
                // no hole
                g_color = BK_COLOR;
                g_reset_periodicity = true;
                continue;
            }

            // Fill in region by looping around again, filling lines to the left
            // whenever going_to is South or West
            s_trail_row = cur_row;
            s_trail_col = cur_col;
            coming_from = direction::West;
            s_going_to = direction::East;
            do
            {
                bool match_found = false;
                do
                {
                    step_col_row();
                    if (g_row >= cur_row                          //
                        && g_col >= g_i_x_start                   //
                        && g_col <= g_i_x_stop                    //
                        && g_row <= g_i_y_stop                    //
                        && getcolor(g_col, g_row) == trail_color) // getcolor() must be last
                    {
                        if (s_going_to == direction::South ||
                            (s_going_to == direction::West && coming_from != direction::East))
                        {
                            // fill a row, but only once
                            int right = g_col;
                            while (--right >= g_i_x_start && (g_color = getcolor(right,g_row)) == trail_color)
                            {
                                // do nothing
                            }
                            if (g_color == BK_COLOR) // check last color
                            {
                                int left = right;
                                while (getcolor(--left,g_row) == BK_COLOR)
                                {
                                    // Should NOT be possible for left < g_i_x_start
                                    // do nothing
                                }
                                left++; // one pixel too far
                                if (right == left)   // only one hole
                                {
                                    (*g_plot)(left,g_row,fill_color_used);
                                }
                                else
                                {
                                    // fill the line to the left
                                    const int length = right - left + 1;
                                    if (fill_color_used != last_fillcolor_used || length > max_putline_length)
                                    {
                                        // only reset dstack if necessary
                                        std::memset(s_stack, fill_color_used, length);
                                        last_fillcolor_used = fill_color_used;
                                        max_putline_length = length;
                                    }
                                    sym_fill_line(g_row, left, right, s_stack);
                                }
                            } // end of fill line
                        }
                        s_trail_row = g_row;
                        s_trail_col = g_col;
                        s_going_to = advance(s_going_to, -1);
                        coming_from = advance(s_going_to, -1);
                        match_found = true;
                    }
                    else
                    {
                        s_going_to = advance(s_going_to, 1);
                    }
                } while (!match_found && s_going_to != coming_from);

                if (!match_found)
                {
                    // next one has to be a match
                    step_col_row();
                    s_trail_row = g_row;
                    s_trail_col = g_col;
                    s_going_to = advance(s_going_to, -1);
                    coming_from = advance(s_going_to, -1);
                }
            } while (s_trail_col != cur_col || s_trail_row != cur_row);
            g_reset_periodicity = true; // reset after a trace/fill
            g_color = BK_COLOR;
        }
    }
    return 0;
}
