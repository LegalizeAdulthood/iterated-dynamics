// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "fractals/fractalp.h"

namespace id::engine
{

enum class AutoShowDot
{
    NONE = 0,        // no auto show dot
    AUTOMATIC = 'a', // automatic color
    DARK = 'd',      // dark dot
    MEDIUM = 'm',    // medium dot
    BRIGHT = 'b'     // bright dot
};

extern AutoShowDot           g_auto_show_dot;
extern int                   g_show_dot;
extern int                   g_size_dot;

fractals::CalcType wrap_show_dot_calc_type(int show_dot_width, int show_dot_color);

} // namespace id::engine
