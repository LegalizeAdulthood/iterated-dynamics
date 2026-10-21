// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "engine/BoundaryTrace.h"

#include "engine/calcfrac.h"
#include "engine/pixel.h"
#include "engine/VideoInfo.h"
#include "engine/work_list.h"

#include <cstring>
#include <stdexcept>

namespace id::engine
{

// boundary trace method
constexpr int BK_COLOR{};
constexpr int BOUNDARY_TRACE_FILL_CHUNK{512};
constexpr int BOUNDARY_TRACE_OUTLINE_CHUNK{512};
constexpr int BOUNDARY_TRACE_SCAN_CHUNK{512};

bool BoundaryTrace::select_pixel()
{
    if (m_done)
    {
        return false;
    }
    if (m_pixel_selected)
    {
        g_row = m_pending_row;
        g_col = m_pending_col;
        return true;
    }
    if (!m_initialized)
    {
        initialize();
    }
    if (m_done)
    {
        return false;
    }

    for (; m_scan_steps < BOUNDARY_TRACE_SCAN_CHUNK && !m_done; ++m_scan_steps)
    {
        if (!process_current_pixel())
        {
            if (m_pixel_selected)
            {
                return true;
            }
            m_scan_steps = 0;
            return false;
        }
        advance_scan_cursor();
    }
    m_scan_steps = 0;
    return false;
}

math::Point2i BoundaryTrace::selected_pixel() const
{
    if (!m_pixel_selected)
    {
        throw std::logic_error{"No selected boundary-trace pixel"};
    }
    return {m_pending_col, m_pending_row};
}

void BoundaryTrace::complete_pixel(const int color)
{
    if (!m_pixel_selected)
    {
        throw std::logic_error{"No selected boundary-trace pixel to complete"};
    }
    g_col = m_pending_col;
    g_row = m_pending_row;
    g_color = color;
    m_pixel_color = color;
    m_color_ready = true;
    m_pixel_selected = false;
}

bool BoundaryTrace::done() const
{
    return m_done;
}

void BoundaryTrace::begin_chunk()
{
    m_scan_steps = 0;
    m_outline_steps = 0;
}

void BoundaryTrace::suspend()
{
    if (!m_done)
    {
        save_resume_work();
    }
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

void BoundaryTrace::begin_fill_trace()
{
    m_trail_row = m_seed_row;
    m_trail_col = m_seed_col;
    m_coming_from = Direction::WEST;
    m_going_to = Direction::EAST;
    m_last_fill_color_used = -1;
    m_max_put_line_length = 0;
    m_fill_started = false;
    m_pixel_phase = PixelPhase::FILL_TRACE;
}

void BoundaryTrace::begin_line_fill()
{
    m_fill_right = m_fill_match_col;
    m_pixel_phase = PixelPhase::FILL_RIGHT_EDGE;
}

void BoundaryTrace::begin_outline_trace(const int cur_row, const int cur_col, const int trail_color)
{
    m_outline_steps = 0;
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

bool BoundaryTrace::load_pixel_color()
{
    if (m_color_ready)
    {
        g_color = m_pixel_color;
        m_color_ready = false;
        return true;
    }
    m_pending_row = g_row;
    m_pending_col = g_col;
    m_pixel_selected = true;
    return false;
}

void BoundaryTrace::fill_line()
{
    if (m_fill_right == m_fill_left) // only one hole
    {
        g_plot(m_fill_left, m_fill_match_row, m_fill_color_used);
        return;
    }

    // fill the line to the left
    const int length = m_fill_right - m_fill_left + 1;
    if (m_fill_color_used != m_last_fill_color_used || length > m_max_put_line_length)
    {
        // only reset dstack if necessary
        std::memset(m_stack.data(), m_fill_color_used, length);
        m_last_fill_color_used = m_fill_color_used;
        m_max_put_line_length = length;
    }
    sym_fill_line(m_fill_match_row, m_fill_left, m_fill_right, m_stack.data());
}

bool BoundaryTrace::fill_line_left_step()
{
    if (get_color(--m_fill_left, m_fill_match_row) == BK_COLOR)
    {
        return true;
    }
    ++m_fill_left; // one pixel too far
    fill_line();
    finish_fill_match();
    return true;
}

bool BoundaryTrace::fill_line_right_step()
{
    --m_fill_right;
    if (m_fill_right >= g_i_start_pt.x)
    {
        g_color = get_color(m_fill_right, m_fill_match_row);
        if (g_color == m_trail_color)
        {
            return true;
        }
        if (g_color == BK_COLOR)
        {
            m_fill_left = m_fill_right;
            m_pixel_phase = PixelPhase::FILL_LEFT_EDGE;
            return true;
        }
    }
    finish_fill_match();
    return true;
}

bool BoundaryTrace::fill_region()
{
    for (int processed = 0; processed < BOUNDARY_TRACE_FILL_CHUNK; ++processed)
    {
        if (!fill_region_step())
        {
            return false;
        }
        if (m_fill_started && m_pixel_phase == PixelPhase::FILL_TRACE && m_trail_col == m_seed_col &&
            m_trail_row == m_seed_row)
        {
            return true;
        }
    }
    return false;
}

bool BoundaryTrace::fill_region_step()
{
    switch (m_pixel_phase)
    {
    case PixelPhase::FILL_TRACE:
        return fill_trace_step();

    case PixelPhase::FILL_RIGHT_EDGE:
        return fill_line_right_step();

    case PixelPhase::FILL_LEFT_EDGE:
        return fill_line_left_step();

    default:
        return true;
    }
}

bool BoundaryTrace::fill_trace_step()
{
    step_col_row();
    if (g_row >= m_seed_row                          //
        && g_col >= g_i_start_pt.x                   //
        && g_col <= g_i_stop_pt.x                    //
        && g_row <= g_i_stop_pt.y                    //
        && get_color(g_col, g_row) == m_trail_color) // getcolor() must be last
    {
        m_fill_match_row = g_row;
        m_fill_match_col = g_col;
        if (m_going_to == Direction::SOUTH || (m_going_to == Direction::WEST && m_coming_from != Direction::EAST))
        {
            begin_line_fill();
            return true;
        }
        finish_fill_match();
        return true;
    }

    m_going_to = advance(m_going_to, 1);
    if (m_going_to == m_coming_from)
    {
        // next one has to be a match
        step_col_row();
        m_fill_match_row = g_row;
        m_fill_match_col = g_col;
        finish_fill_match();
    }
    return true;
}

void BoundaryTrace::finish_fill_match()
{
    m_trail_row = m_fill_match_row;
    m_trail_col = m_fill_match_col;
    m_going_to = advance(m_going_to, -1);
    m_coming_from = advance(m_going_to, -1);
    m_fill_started = true;
    m_pixel_phase = PixelPhase::FILL_TRACE;
}

void BoundaryTrace::initialize()
{
    if ((g_inside_method == ColorMethod::COLOR && g_inside_color == 0) ||
        (g_outside_method == ColorMethod::COLOR && g_outside_color == 0))
    {
        throw std::runtime_error{"Boundary tracing cannot be used with inside=0 or outside=0"};
    }
    if (g_colors < 16)
    {
        throw std::runtime_error{"Boundary tracing cannot be used with < 16 colors"};
    }

    m_initialized = true;
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
    m_resume_saved = false;
    reset_row_state();
}

bool BoundaryTrace::process_current_pixel()
{
    if (m_pixel_phase == PixelPhase::SCAN)
    {
        record_progress();
        if (!m_color_ready)
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
        if (!load_pixel_color())
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

    if (m_pixel_phase == PixelPhase::TRACE_OUTLINE)
    {
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

        begin_fill_trace();
    }

    if (!fill_region())
    {
        return false;
    }
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

void BoundaryTrace::save_resume_work()
{
    if (m_resume_saved)
    {
        return;
    }
    if (g_i_stop_pt.y != g_stop_pt.y)
    {
        g_i_stop_pt.y = g_stop_pt.y - (m_current_row - g_start_pt.y); // allow for sym
    }
    add_work_list(g_start_pt.x, m_current_row, g_stop_pt.x, g_i_stop_pt.y, m_current_col, m_current_row, 0,
        g_work_symmetry);
    m_resume_saved = true;
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
    for (; m_outline_steps < BOUNDARY_TRACE_OUTLINE_CHUNK; ++m_outline_steps)
    {
        if (!trace_outline_step())
        {
            return false;
        }
        if (!m_continue_loop || (g_col == m_seed_col && g_row == m_seed_row))
        {
            m_outline_steps = 0;
            return true;
        }
    }
    m_outline_steps = 0;
    return false;
}

bool BoundaryTrace::trace_outline_step()
{
    bool candidate_in_bounds = true;
    if (m_color_ready)
    {
        g_row = m_pending_row;
        g_col = m_pending_col;
        if (!load_pixel_color())
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
            if (g_color == BK_COLOR && !load_pixel_color())
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
    return true;
}

} // namespace id::engine
