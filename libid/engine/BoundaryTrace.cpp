// SPDX-License-Identifier: GPL-3.0-only
//
#include "engine/BoundaryTrace.h"

#include "engine/calcfrac.h"
#include "engine/show_dot.h"
#include "engine/VideoInfo.h"
#include "engine/work_list.h"
#include "fractals/fractalp.h"
#include "ui/stop_msg.h"
#include "ui/video.h"

#include <cstring>

using namespace id::ui;
using namespace id::fractals;

namespace id::engine
{

// boundary trace method
constexpr int BK_COLOR{};
constexpr int BOUNDARY_TRACE_SCAN_CHUNK{512};

bool BoundaryTrace::iterate()
{
    if (m_done)
    {
        return true;
    }
    if (!m_initialized)
    {
        initialize();
    }
    if (m_done)
    {
        return true;
    }

    int last_fill_color_used = -1;
    int max_put_line_length = 0; // reset max_putline_length
    for (int processed = 0; processed < BOUNDARY_TRACE_SCAN_CHUNK && !m_done; ++processed)
    {
        if (!process_current_pixel(last_fill_color_used, max_put_line_length))
        {
            return false;
        }
        advance_scan_cursor();
    }
    return m_done;
}

BoundaryTrace::Direction BoundaryTrace::advance(const Direction dir, const int increment)
{
    return static_cast<Direction>(static_cast<int>(dir) + increment & 0x03);
}

void BoundaryTrace::advance_scan_cursor()
{
    ++m_current_col;
    if (m_current_col <= g_i_stop_pt.x)
    {
        return;
    }

    ++m_current_row;
    if (m_current_row > g_i_stop_pt.y)
    {
        m_done = true;
        return;
    }

    m_current_col = g_i_start_pt.x;
    reset_row_state();
}

void BoundaryTrace::initialize()
{
    m_initialized = true;
    if ((g_inside_method == ColorMethod::COLOR && g_inside_color == 0) ||
        (g_outside_method == ColorMethod::COLOR && g_outside_color == 0))
    {
        stop_msg("Boundary tracing cannot be used with inside=0 or outside=0");
        m_done = true;
        return;
    }
    if (g_colors < 16)
    {
        stop_msg("Boundary tracing cannot be used with < 16 colors");
        m_done = true;
        return;
    }

    g_passes = Passes::BOUNDARY_TRACE;
    if (g_i_start_pt.x > g_i_stop_pt.x || g_i_start_pt.y > g_i_stop_pt.y)
    {
        m_done = true;
        return;
    }

    m_current_row = g_begin_pt.y;
    m_current_col = g_begin_pt.x;
    if (m_current_row < g_i_start_pt.y || m_current_row > g_i_stop_pt.y)
    {
        m_current_row = g_i_start_pt.y;
    }
    if (m_current_col < g_i_start_pt.x || m_current_col > g_i_stop_pt.x)
    {
        m_current_col = g_i_start_pt.x;
    }
    reset_row_state();
}

bool BoundaryTrace::process_current_pixel(int &last_fill_color_used, int &max_put_line_length)
{
    record_progress();
    const int cur_row = m_current_row;
    const int cur_col = m_current_col;
    if (get_color(cur_col, cur_row) != BK_COLOR)
    {
        return true;
    }

    int trail_color = g_color;
    g_row = cur_row;
    g_col = cur_col;
    if (calc_type() == -1) // g_color, g_row, g_col are global
    {
        save_resume_work();
        return false;
    }
    g_reset_periodicity = false; // normal periodicity checking

    // This next line may cause a few more pixels to be calculated,
    // but at the savings of quite a bit of overhead
    if (g_color != trail_color)
    {
        return true;
    }

    // sweep clockwise to trace outline
    m_trail_row = cur_row;
    m_trail_col = cur_col;
    trail_color = g_color;
    const int fill_color_used = g_fill_color > 0 ? g_fill_color : trail_color;
    Direction coming_from = Direction::WEST;
    m_going_to = Direction::EAST;
    unsigned int matches_found = 0;
    bool continue_loop = true;
    do
    {
        step_col_row();
        if (g_row >= cur_row           //
            && g_col >= g_i_start_pt.x //
            && g_col <= g_i_stop_pt.x  //
            && g_row <= g_i_stop_pt.y)
        {
            g_color = get_color(g_col, g_row);
            // g_color, g_row, g_col are global for calc_type()
            if (g_color == BK_COLOR && calc_type() == -1)
            {
                save_resume_work();
                return false;
            }
            if (g_color == trail_color)
            {
                if (matches_found < 4) // to keep it from overflowing
                {
                    ++matches_found;
                }
                m_trail_row = g_row;
                m_trail_col = g_col;
                m_going_to = advance(m_going_to, -1);
                coming_from = advance(m_going_to, -1);
            }
            else
            {
                m_going_to = advance(m_going_to, 1);
                continue_loop = m_going_to != coming_from || matches_found > 0;
            }
        }
        else
        {
            m_going_to = advance(m_going_to, 1);
            continue_loop = m_going_to != coming_from || matches_found > 0;
        }
    } while (continue_loop && (g_col != cur_col || g_row != cur_row));

    if (matches_found <= 3)
    {
        // no hole
        reset_row_state();
        return true;
    }

    // Fill in region by looping around again, filling lines to the left
    // whenever going_to is South or West
    m_trail_row = cur_row;
    m_trail_col = cur_col;
    coming_from = Direction::WEST;
    m_going_to = Direction::EAST;
    do
    {
        bool match_found = false;
        do
        {
            step_col_row();
            if (g_row >= cur_row                           //
                && g_col >= g_i_start_pt.x                 //
                && g_col <= g_i_stop_pt.x                  //
                && g_row <= g_i_stop_pt.y                  //
                && get_color(g_col, g_row) == trail_color) // getcolor() must be last
            {
                if (m_going_to == Direction::SOUTH || (m_going_to == Direction::WEST && coming_from != Direction::EAST))
                {
                    // fill a row, but only once
                    int right = g_col;
                    while (--right >= g_i_start_pt.x && (g_color = get_color(right, g_row)) == trail_color)
                    {
                        // do nothing
                    }
                    if (g_color == BK_COLOR) // check last color
                    {
                        int left = right;
                        while (get_color(--left, g_row) == BK_COLOR)
                        {
                            // Should NOT be possible for left < g_i_start_pt.x
                            // do nothing
                        }
                        left++;            // one pixel too far
                        if (right == left) // only one hole
                        {
                            g_plot(left, g_row, fill_color_used);
                        }
                        else
                        {
                            // fill the line to the left
                            if (const int length = right - left + 1;
                                fill_color_used != last_fill_color_used || length > max_put_line_length)
                            {
                                // only reset dstack if necessary
                                std::memset(m_stack.data(), fill_color_used, length);
                                last_fill_color_used = fill_color_used;
                                max_put_line_length = length;
                            }
                            sym_fill_line(g_row, left, right, m_stack.data());
                        }
                    } // end of fill line
                }
                m_trail_row = g_row;
                m_trail_col = g_col;
                m_going_to = advance(m_going_to, -1);
                coming_from = advance(m_going_to, -1);
                match_found = true;
            }
            else
            {
                m_going_to = advance(m_going_to, 1);
            }
        } while (!match_found && m_going_to != coming_from);

        if (!match_found)
        {
            // next one has to be a match
            step_col_row();
            m_trail_row = g_row;
            m_trail_col = g_col;
            m_going_to = advance(m_going_to, -1);
            coming_from = advance(m_going_to, -1);
        }
    } while (m_trail_col != cur_col || m_trail_row != cur_row);
    reset_row_state();
    return true;
}

void BoundaryTrace::record_progress() const
{
    g_current_row = m_current_row;
    g_current_column = m_current_col;
}

void BoundaryTrace::reset_row_state()
{
    g_reset_periodicity = true;
    g_color = BK_COLOR;
    record_progress();
}

void BoundaryTrace::save_resume_work() const
{
    if (g_show_dot != BK_COLOR) // remove show dot pixel
    {
        g_plot(g_col, g_row, BK_COLOR);
    }
    if (g_i_stop_pt.y != g_stop_pt.y)
    {
        g_i_stop_pt.y = g_stop_pt.y - (m_current_row - g_start_pt.y); // allow for sym
    }
    add_work_list(g_start_pt.x, m_current_row, g_stop_pt.x, g_i_stop_pt.y, m_current_col, m_current_row, 0,
        g_work_symmetry);
}

// take one step in the direction of going_to
void BoundaryTrace::step_col_row()
{
    switch (m_going_to)
    {
    case Direction::NORTH:
        g_col = m_trail_col;
        g_row = m_trail_row - 1;
        break;
    case Direction::EAST:
        g_col = m_trail_col + 1;
        g_row = m_trail_row;
        break;
    case Direction::SOUTH:
        g_col = m_trail_col;
        g_row = m_trail_row + 1;
        break;
    case Direction::WEST:
        g_col = m_trail_col - 1;
        g_row = m_trail_row;
        break;
    }
}

} // namespace id::engine
