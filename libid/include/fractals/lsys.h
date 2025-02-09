// SPDX-License-Identifier: GPL-3.0-only
//
//      Header file for L-system code.
//
#pragma once

#include <config/port.h>

enum
{
    MAX_LSYS_LINE_LEN = 255 // this limits line length to 255
};

struct LSysTurtleState
{
    char counter, angle, reverse;
    bool stack_overflow;
    // dmaxangle is maxangle - 1
    char max_angle, d_max_angle, curr_color, dummy;  // dummy ensures longword alignment
    LDouble size;
    LDouble real_angle;
    LDouble x_pos, y_pos;
    LDouble x_min, y_min, x_max, y_max;
    LDouble aspect; // aspect ratio of each pixel, ysize/xsize
    union
    {
        long n;
        LDouble nf;
    } param;
};

extern char g_max_angle;

struct LSysCmd;
LSysCmd *draw_lsysf(LSysCmd *command, LSysTurtleState *ts, LSysCmd **rules, int depth);
bool lsysf_find_scale(LSysCmd *command, LSysTurtleState *ts, LSysCmd **rules, int depth);
LSysCmd *lsysf_size_transform(char const *s, LSysTurtleState *ts);
LSysCmd *lsysf_draw_transform(char const *s, LSysTurtleState *ts);
void lsysf_do_sin_cos();
