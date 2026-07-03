// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <config/port.h>

#include <array>

namespace id::engine
{

class BoundaryTrace
{
public:
    bool iterate();

private:
    enum class Direction
    {
        NORTH,
        EAST,
        SOUTH,
        WEST
    };

    static Direction advance(Direction dir, int increment);

    void advance_scan_cursor();
    void initialize();
    bool process_current_pixel(int &last_fill_color_used, int &max_put_line_length);
    void record_progress() const;
    void reset_row_state();
    void save_resume_work() const;
    void step_col_row();

    Direction m_going_to{};
    int m_current_row{};
    int m_current_col{};
    int m_trail_row{};
    int m_trail_col{};
    bool m_done{};
    bool m_initialized{};
    std::array<Byte, 4096> m_stack{};
};

} // namespace id::engine
