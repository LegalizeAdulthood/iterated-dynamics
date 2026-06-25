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
    int run();

private:
    enum class Direction
    {
        NORTH,
        EAST,
        SOUTH,
        WEST
    };

    static Direction advance(Direction dir, int increment);

    void step_col_row();

    Direction m_going_to{};
    int m_trail_row{};
    int m_trail_col{};
    std::array<Byte, 4096> m_stack{};
};

int boundary_trace();

} // namespace id::engine
