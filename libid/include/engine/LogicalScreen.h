// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "math/Point.h"

namespace id::engine
{

struct LogicalScreen
{
    int x_dots;         // # of dots on the logical screen
    int y_dots;
    int x_offset;       // physical top left of logical screen
    int y_offset;
    double x_size_dots; // xdots-1, ydots-1
    double y_size_dots;
};

extern LogicalScreen g_logical_screen;

} // namespace id::engine
