// SPDX-License-Identifier: GPL-3.0-only
//
#include "engine/BoundaryTrace.h"

#include "engine/calcfrac.h"
#include "engine/show_dot.h"
#include "engine/StandardFractal.h"
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
        if (StandardFractal *standard_fractal = active_standard_fractal();
            standard_fractal != nullptr && standard_fractal->consume_standard_pixel_yield())
        {
            return false;
        }
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

void BoundaryTrace::begin_outline_trace(const int cur_row, const int cur_col, const int trail_color)
{
    m_seed_row = cur_row;
    m_seed_col = cur_col;
    m_trail_row = cur_row;
    m_trail_col = cur_col;
    m_trail_color = trail_color;
    m_fill_color_used = g_fill_color > 0 ? g_fill_color : trail_color;
    m_coming_from = Direction::WEST;
    m_going_to = Direction::EAST;
    m_matches_found = 0;
    m_continue_loop = true;
    m_pixel_phase = PixelPhase::TRACE_OUTLINE;
}

bool BoundaryTrace::calculate_current_pixel()
{
    if (calc_type() != -1) // g_color, g_row, g_col are global
    {
        m_calculation_pending = false;
        return true;
    }
    return stop_for_calculation_return();
}

bool BoundaryTrace::stop_for_calculation_return()
{
    StandardFractal *standard_fractal{active_standard_fractal()};
    if (standard_fractal != nullptr && standard_fractal->consume_incomplete_standard_pixel_yield())
    {
        m_pending_row = g_row;
        m_pending_col = g_col;
        m_calculation_pending = true;
        return false;
    }
    m_calculation_pending = false;
    save_resume_work();
    return false;
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
    if (m_pixel_phase == PixelPhase::SCAN)
    {
        record_progress();
        if (!m_calculation_pending)
        {
            m_seed_row = m_current_row;
            m_seed_col = m_current_col;
            if (get_color(m_seed_col, m_seed_row) != BK_COLOR)
            {
                return true;
            }
            m_seed_trail_color = g_color;
        }

        g_row = m_seed_row;
        g_col = m_seed_col;
        if (!calculate_current_pixel())
        {
            return false;
        }
        g_reset_periodicity = false; // normal periodicity checking

        // This next line may cause a few more pixels to be calculated,
        // but at the savings of quite a bit of overhead
        if (g_color != m_seed_trail_color)
        {
            return true;
        }

        begin_outline_trace(m_seed_row, m_seed_col, g_color);
    }

    if (!trace_outline())
    {
        return false;
    }

    if (m_matches_found <= 3)
    {
        // no hole
        reset_row_state();
        m_pixel_phase = PixelPhase::SCAN;
        return true;
    }

    // Fill in region by looping around again, filling lines to the left
    // whenever going_to is South or West
    m_trail_row = m_seed_row;
    m_trail_col = m_seed_col;
    Direction coming_from = Direction::WEST;
    m_going_to = Direction::EAST;
    do
    {
        bool match_found = false;
        do
        {
            step_col_row();
            if (g_row >= m_seed_row                          //
                && g_col >= g_i_start_pt.x                   //
                && g_col <= g_i_stop_pt.x                    //
                && g_row <= g_i_stop_pt.y                    //
                && get_color(g_col, g_row) == m_trail_color) // getcolor() must be last
            {
                if (m_going_to == Direction::SOUTH || (m_going_to == Direction::WEST && coming_from != Direction::EAST))
                {
                    // fill a row, but only once
                    int right = g_col;
                    while (--right >= g_i_start_pt.x && (g_color = get_color(right, g_row)) == m_trail_color)
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
                            g_plot(left, g_row, m_fill_color_used);
                        }
                        else
                        {
                            // fill the line to the left
                            if (const int length = right - left + 1;
                                m_fill_color_used != last_fill_color_used || length > max_put_line_length)
                            {
                                // only reset dstack if necessary
                                std::memset(m_stack.data(), m_fill_color_used, length);
                                last_fill_color_used = m_fill_color_used;
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
    } while (m_trail_col != m_seed_col || m_trail_row != m_seed_row);
    reset_row_state();
    m_pixel_phase = PixelPhase::SCAN;
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

bool BoundaryTrace::trace_outline()
{
    do
    {
        bool candidate_in_bounds = true;
        if (m_calculation_pending)
        {
            g_row = m_pending_row;
            g_col = m_pending_col;
            if (!calculate_current_pixel())
            {
                return false;
            }
        }
        else
        {
            step_col_row();
            candidate_in_bounds =
                g_row >= m_seed_row && g_col >= g_i_start_pt.x && g_col <= g_i_stop_pt.x && g_row <= g_i_stop_pt.y;
            if (candidate_in_bounds)
            {
                g_color = get_color(g_col, g_row);
                if (g_color == BK_COLOR && !calculate_current_pixel())
                {
                    return false;
                }
            }
        }

        if (candidate_in_bounds)
        {
            if (g_color == m_trail_color)
            {
                if (m_matches_found < 4) // to keep it from overflowing
                {
                    ++m_matches_found;
                }
                m_trail_row = g_row;
                m_trail_col = g_col;
                m_going_to = advance(m_going_to, -1);
                m_coming_from = advance(m_going_to, -1);
            }
            else
            {
                m_going_to = advance(m_going_to, 1);
                m_continue_loop = m_going_to != m_coming_from || m_matches_found > 0;
            }
        }
        else
        {
            m_going_to = advance(m_going_to, 1);
            m_continue_loop = m_going_to != m_coming_from || m_matches_found > 0;
        }
    } while (m_continue_loop && (g_col != m_seed_col || g_row != m_seed_row));
    return true;
}

} // namespace id::engine
