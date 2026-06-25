// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

namespace id::engine
{

enum class OrbitDrawMode
{
    NONE = 0,
    RECTANGLE = 'r',
    LINE = 'l',
    FUNCTION = 'f',
};

extern OrbitDrawMode g_draw_mode;

class StickyOrbits
{
public:
    bool iterate();
    int scan();

private:
    int draw_function();
    int draw_line();
    int draw_rectangle();
};

int sticky_orbits();

} // namespace id::engine
