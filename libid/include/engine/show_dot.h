// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

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
