// SPDX-License-Identifier: GPL-3.0-only
//
#include "engine/boundary_trace.h"

#include "engine/calcfrac.h"
#include "engine/cmdfiles.h"
#include "engine/id_data.h"
#include "engine/show_dot.h"
#include "engine/work_list.h"
#include "ui/stop_msg.h"
#include "ui/video.h"

#include <cstring>

enum class Direction
{
    NORTH,
    EAST,
    SOUTH,
    WEST
};
inline int operator+(Direction value)
{
    return static_cast<int>(value);
}

static Direction s_going_to{};
static int s_trail_row{};
static int s_trail_col{};
static Byte s_stack[4096]{}; // common temp, two put_line calls

// boundary trace method
constexpr int BK_COLOR{};

static Direction advance(Direction dir, int increment)
{
    return static_cast<Direction>((+dir + increment) & 0x03);
}

// take one step in the direction of going_to
static void step_col_row()
{
    switch (s_going_to)
    {
    case Direction::NORTH:
        g_col = s_trail_col;
        g_row = s_trail_row - 1;
        break;
    case Direction::EAST:
        g_col = s_trail_col + 1;
        g_row = s_trail_row;
        break;
    case Direction::SOUTH:
        g_col = s_trail_col;
        g_row = s_trail_row + 1;
        break;
    case Direction::WEST:
        g_col = s_trail_col - 1;
        g_row = s_trail_row;
        break;
    }
}

int boundary_trace()
{
    if (g_inside_color == COLOR_BLACK || g_outside_color == COLOR_BLACK)
    {
        stop_msg("Boundary tracing cannot be used with inside=0 or outside=0");
        return -1;
    }
    if (g_colors < 16)
    {
        stop_msg("Boundary tracing cannot be used with < 16 colors");
        return -1;
    }

    int last_fill_color_used = -1;
    g_got_status = StatusValues::BOUNDARY_TRACE;
    int max_put_line_length = 0; // reset max_putline_length
    for (int cur_row = g_i_start_pt.y; cur_row <= g_i_stop_pt.y; cur_row++)
    {
        g_reset_periodicity = true; // reset for a new row
        g_color = BK_COLOR;
        for (int cur_col = g_i_start_pt.x; cur_col <= g_i_stop_pt.x; cur_col++)
        {
            if (get_color(cur_col, cur_row) != BK_COLOR)
            {
                continue;
            }

            int trail_color = g_color;
            g_row = cur_row;
            g_col = cur_col;
            if (g_calc_type() == -1) // g_color, g_row, g_col are global
            {
                if (g_show_dot != BK_COLOR)   // remove show dot pixel
                {
                    g_plot(g_col, g_row, BK_COLOR);
                }
                if (g_i_stop_pt.y != g_stop_pt.y)
                {
                    g_i_stop_pt.y = g_stop_pt.y - (cur_row - g_start_pt.y); // allow for sym
                }
                add_work_list(
                    g_start_pt.x, cur_row, g_stop_pt.x, g_i_stop_pt.y, cur_col, cur_row, 0, g_work_symmetry);
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
            Direction coming_from = Direction::WEST;
            s_going_to = Direction::EAST;
            unsigned int matches_found = 0;
            bool continue_loop = true;
            do
            {
                step_col_row();
                if (g_row >= cur_row        //
                    && g_col >= g_i_start_pt.x //
                    && g_col <= g_i_stop_pt.x  //
                    && g_row <= g_i_stop_pt.y)
                {
                    g_color = get_color(g_col, g_row);
                    // g_color, g_row, g_col are global for g_calc_type()
                    if (g_color == BK_COLOR && g_calc_type()== -1)
                    {
                        if (g_show_dot != BK_COLOR)   // remove show dot pixel
                        {
                            g_plot(g_col, g_row, BK_COLOR);
                        }
                        if (g_i_stop_pt.y != g_stop_pt.y)
                        {
                            g_i_stop_pt.y = g_stop_pt.y - (cur_row - g_start_pt.y); // allow for sym
                        }
                        add_work_list(
                            g_start_pt.x, cur_row, g_stop_pt.x, g_i_stop_pt.y, cur_col, cur_row, 0, g_work_symmetry);
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
            coming_from = Direction::WEST;
            s_going_to = Direction::EAST;
            do
            {
                bool match_found = false;
                do
                {
                    step_col_row();
                    if (g_row >= cur_row                          //
                        && g_col >= g_i_start_pt.x                   //
                        && g_col <= g_i_stop_pt.x                    //
                        && g_row <= g_i_stop_pt.y                    //
                        && get_color(g_col, g_row) == trail_color) // getcolor() must be last
                    {
                        if (s_going_to == Direction::SOUTH ||
                            (s_going_to == Direction::WEST && coming_from != Direction::EAST))
                        {
                            // fill a row, but only once
                            int right = g_col;
                            while (--right >= g_i_start_pt.x && (g_color = get_color(right,g_row)) == trail_color)
                            {
                                // do nothing
                            }
                            if (g_color == BK_COLOR) // check last color
                            {
                                int left = right;
                                while (get_color(--left,g_row) == BK_COLOR)
                                {
                                    // Should NOT be possible for left < g_i_start_pt.x
                                    // do nothing
                                }
                                left++; // one pixel too far
                                if (right == left)   // only one hole
                                {
                                    g_plot(left,g_row,fill_color_used);
                                }
                                else
                                {
                                    // fill the line to the left
                                    const int length = right - left + 1;
                                    if (fill_color_used != last_fill_color_used || length > max_put_line_length)
                                    {
                                        // only reset dstack if necessary
                                        std::memset(s_stack, fill_color_used, length);
                                        last_fill_color_used = fill_color_used;
                                        max_put_line_length = length;
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
