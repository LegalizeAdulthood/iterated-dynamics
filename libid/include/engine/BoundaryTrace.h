// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include "math/Point.h"

#include <config/port.h>

#include <array>

namespace id::engine
{

class BoundaryTrace
{
public:
    void begin_chunk();
    // False ends this work chunk; done() distinguishes completion.
    bool select_pixel();
    math::Point2i selected_pixel() const;
    void complete_pixel(int color);
    bool done() const;
    void suspend();

private:
    enum class Direction
    {
        NORTH,
        EAST,
        SOUTH,
        WEST
    };

    enum class PixelPhase
    {
        SCAN,
        TRACE_OUTLINE,
        FILL_TRACE,
        FILL_RIGHT_EDGE,
        FILL_LEFT_EDGE
    };

    static Direction advance(Direction dir, int increment);

    void advance_scan_cursor();
    void begin_fill_trace();
    void begin_line_fill();
    void begin_outline_trace(int cur_row, int cur_col, int trail_color);
    bool load_pixel_color();
    void fill_line();
    bool fill_line_left_step();
    bool fill_line_right_step();
    bool fill_region();
    bool fill_region_step();
    bool fill_trace_step();
    void finish_fill_match();
    void initialize();
    bool process_current_pixel();
    void record_progress() const;
    void reset_row_state();
    void save_resume_work();
    void step_col_row();
    bool trace_outline();
    bool trace_outline_step();

    PixelPhase m_pixel_phase{PixelPhase::SCAN};
    Direction m_going_to{};
    Direction m_coming_from{};
    int m_current_row{};
    int m_current_col{};
    int m_seed_row{};
    int m_seed_col{};
    int m_seed_trail_color{};
    int m_pending_row{};
    int m_pending_col{};
    int m_pixel_color{};
    int m_scan_steps{};
    int m_outline_steps{};
    int m_fill_left{};
    int m_fill_right{};
    int m_fill_match_row{};
    int m_fill_match_col{};
    int m_trail_row{};
    int m_trail_col{};
    int m_trail_color{};
    int m_fill_color_used{};
    int m_last_fill_color_used{-1};
    int m_max_put_line_length{};
    unsigned int m_matches_found{};
    bool m_pixel_selected{};
    bool m_color_ready{};
    bool m_continue_loop{};
    bool m_done{};
    bool m_fill_started{};
    bool m_initialized{};
    bool m_resume_saved{};
    std::array<Byte, 4096> m_stack{};
};

} // namespace id::engine
