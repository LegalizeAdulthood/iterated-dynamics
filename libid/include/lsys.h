// SPDX-License-Identifier: GPL-3.0-only
//
//      Header file for L-system code.
//
#pragma once

#include "id.h"
#include "port.h"

/* Macro to take an FP number and turn it into a
 * 16/16-bit fixed-point number.
 */
#define FIXEDMUL        524288L
#define FIXEDPT(x)      ((long) (FIXEDMUL * (x)))
/* The number by which to multiply sines, cosines and other
 * values with magnitudes less than or equal to 1.
 * sins and coss are a 3/29 bit fixed-point scheme (so the
 * range is +/- 2, with good accuracy.  The range is to
 * avoid overflowing when the aspect ratio is taken into
 * account.
 */
#define FIXEDLT1        536870912.0
#define ANGLE2DOUBLE    (2.0*PI / 4294967296.0)
enum
{
    MAX_LSYS_LINE_LEN = 255 // this limits line length to 255
};

struct LSysTurtleStateI
{
    char counter, angle, reverse;
    bool stackoflow;
    // dmaxangle is maxangle - 1
    char maxangle, dmaxangle, curcolor, dummy;  // dummy ensures longword alignment
    long size;
    long realangle;
    long xpos, ypos; // xpos and ypos are long, not fixed point
    long xmin, ymin, xmax, ymax; // as are these
    long aspect; // aspect ratio of each pixel, ysize/xsize
    long num;
};

struct LSysTurtleStateF
{
    char counter, angle, reverse;
    bool stackoflow;
    // dmaxangle is maxangle - 1
    char maxangle, dmaxangle, curcolor, dummy;  // dummy ensures longword alignment
    LDouble size;
    LDouble realangle;
    LDouble xpos, ypos;
    LDouble xmin, ymin, xmax, ymax;
    LDouble aspect; // aspect ratio of each pixel, ysize/xsize
    union
    {
        long n;
        LDouble nf;
    } param;
};

extern char g_max_angle;

struct LSysFCmd;
LSysFCmd *draw_lsysf(LSysFCmd *command, LSysTurtleStateF *ts, LSysFCmd **rules, int depth);
bool lsysf_find_scale(LSysFCmd *command, LSysTurtleStateF *ts, LSysFCmd **rules, int depth);
LSysFCmd *lsysf_size_transform(char const *s, LSysTurtleStateF *ts);
LSysFCmd *lsysf_draw_transform(char const *s, LSysTurtleStateF *ts);
void lsysf_do_sin_cos();
