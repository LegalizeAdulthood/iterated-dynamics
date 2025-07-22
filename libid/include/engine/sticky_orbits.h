// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

enum class OrbitDrawMode
{
    NONE = 0,
    RECTANGLE = 'r',
    LINE = 'l',
    FUNCTION = 'f',
};

extern OrbitDrawMode         g_draw_mode;

int sticky_orbits();
