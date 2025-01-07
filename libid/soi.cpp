// SPDX-License-Identifier: GPL-3.0-only
//
/*
 *  Simultaneous Orbit Iteration Image Generation Method. Computes
 *      rectangular regions by tracking the orbits of only a few key points.
 *
 * Copyright (c) 1994-1997 Michael R. Ganss. All Rights Reserved.
 *
 * This file is distributed under the same conditions as
 * AlmondBread. For further information see
 * <http://www.cs.tu-berlin.de/~rms/AlmondBread>.
 *
 */
#include "soi.h"

#include "calcfrac.h"
#include "drivers.h"
#include "id_data.h"
#include "port.h"
#include "stack_avail.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iterator>

enum
{
    EVERY = 15,
    BASIN_COLOR = 0
};

int g_rhombus_stack[10]{};
int g_rhombus_depth{};
int g_max_rhombus_depth{};
int g_soi_min_stack_available{};
int g_soi_min_stack{2200}; // and this much stack to not crash when <tab> is pressed

namespace
{

struct LongDoubleComplex
{
    LDouble re;
    LDouble im;
};

struct SOILongDoubleState
{
    LongDoubleComplex z;
    LongDoubleComplex step;
    LDouble interleave_step;
    LDouble help_real;
    LongDoubleComplex scan_z;
    LongDoubleComplex b1[3];
    LongDoubleComplex b2[3];
    LongDoubleComplex b3[3];
    LongDoubleComplex limit;
    LongDoubleComplex rq[9];
    LongDoubleComplex corner[2];
    LongDoubleComplex tz[4];
    LongDoubleComplex tq[4];
};

} // namespace

inline LongDoubleComplex z_sqr(LongDoubleComplex z)
{
    return { z.re*z.re, z.im*z.im };
}

/* Newton Interpolation.
   It computes the value of the interpolation polynomial given by
   (x0,w0)..(x2,w2) at x:=t */
inline LDouble interpolate(
    LDouble x0, LDouble x1, LDouble x2,
    LDouble w0, LDouble w1, LDouble w2,
    LDouble t)
{
    const LDouble b = (w1 - w0)/(x1 - x0);
    return (((w2 - w1)/(x2 - x1) - b)/(x2 - x0)*(t - x1) + b)*(t - x0) + w0;
}

/* compute coefficients of Newton polynomial (b0,..,b2) from
   (x0,w0),..,(x2,w2). */
inline void interpolate(
    LDouble x0, LDouble x1, LDouble x2,
    LDouble w0, LDouble w1, LDouble w2,
    LDouble &b0, LDouble &b1, LDouble &b2)
{
    b0 = w0;
    b1 = (w1 - w0)/(x1 - x0);
    b2 = ((w2 - w1)/(x2 - x1) - b1)/(x2 - x0);
}

// evaluate Newton polynomial given by (x0,b0),(x1,b1) at x:=t
inline LDouble evaluate(
    LDouble x0, LDouble x1,
    LDouble b0, LDouble b1, LDouble b2,
    LDouble t)
{
    return (b2*(t - x1) + b1)*(t - x0) + b0;
}

static LongDoubleComplex s_zi[9]{};
static SOILongDoubleState s_state{};
static LDouble s_t_width{};
static LDouble s_equal{};
static bool s_ba_x_in_xx{};

static long iteration(
    LDouble cr, LDouble ci,
    LDouble re, LDouble im,
    long start)
{
    long iter;
    long offset = 0;
    LDouble ren;
    LDouble imn;
    LDouble mag;
    int exponent;

    if (s_ba_x_in_xx)
    {
        LDouble sre = re;
        LDouble sim = im;
        ren = re*re;
        imn = im*im;
        if (start != 0)
        {
            offset = g_max_iterations - start + 7;
            iter = offset >> 3;
            offset &= 7;
            offset = (8 - offset);
        }
        else
        {
            iter = g_max_iterations >> 3;
        }

        int k = 8;
        int n = 8;
        do
        {
            im = im*re;
            re = ren - imn;
            im += im;
            re += cr;
            im += ci;

            imn = im*re;
            ren = re + im;
            re = re - im;
            imn += imn;
            re = ren*re;
            im = imn + ci;
            re += cr;

            imn = im*re;
            ren = re + im;
            re = re - im;
            imn += imn;
            re = ren*re;
            im = imn + ci;
            re += cr;

            imn = im*re;
            ren = re + im;
            re = re - im;
            imn += imn;
            re = ren*re;
            im = imn + ci;
            re += cr;

            imn = im*re;
            ren = re + im;
            re = re - im;
            imn += imn;
            re = ren*re;
            im = imn + ci;
            re += cr;

            imn = im*re;
            ren = re + im;
            re = re - im;
            imn += imn;
            re = ren*re;
            im = imn + ci;
            re += cr;

            imn = im*re;
            ren = re + im;
            re = re - im;
            imn += imn;
            re = ren*re;
            im = imn + ci;
            re += cr;

            imn = im*re;
            ren = re + im;
            re = re - im;
            imn += imn;
            re = ren*re;
            im = imn + ci;
            re += cr;

            if (std::fabs(sre - re) < s_equal && std::fabs(sim - im) < s_equal)
            {
                return BASIN_COLOR;
            }

            k -= 8;
            if (k <= 0)
            {
                n <<= 1;
                sre = re;
                sim = im;
                k = n;
            }

            imn = im*im;
            ren = re*re;
            mag = ren + imn;
        }
        while (mag < 16.0 && --iter != 0);
    }
    else
    {
        ren = re*re;
        imn = im*im;
        if (start != 0)
        {
            offset = g_max_iterations - start + 7;
            iter = offset >> 3;
            offset &= 7;
            offset = (8 - offset);
        }
        else
        {
            iter = g_max_iterations >> 3;
        }

        do
        {
            im = im*re;
            re = ren - imn;
            im += im;
            re += cr;
            im += ci;

            imn = im*re;
            ren = re + im;
            re = re - im;
            imn += imn;
            re = ren*re;
            im = imn + ci;
            re += cr;

            imn = im*re;
            ren = re + im;
            re = re - im;
            imn += imn;
            re = ren*re;
            im = imn + ci;
            re += cr;

            imn = im*re;
            ren = re + im;
            re = re - im;
            imn += imn;
            re = ren*re;
            im = imn + ci;
            re += cr;

            imn = im*re;
            ren = re + im;
            re = re - im;
            imn += imn;
            re = ren*re;
            im = imn + ci;
            re += cr;

            imn = im*re;
            ren = re + im;
            re = re - im;
            imn += imn;
            re = ren*re;
            im = imn + ci;
            re += cr;

            imn = im*re;
            ren = re + im;
            re = re - im;
            imn += imn;
            re = ren*re;
            im = imn + ci;
            re += cr;

            imn = im*re;
            ren = re + im;
            re = re - im;
            imn += imn;
            re = ren*re;
            im = imn + ci;
            re += cr;

            imn = im*im;
            ren = re*re;
            mag = ren + imn;
        }
        while (mag < 16.0 && --iter != 0);
    }

    if (iter == 0)
    {
        s_ba_x_in_xx = true;
        return BASIN_COLOR;
    }
    static char adjust[256] =
    {
        0,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4,
        5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
        6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
        6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
        8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
        8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
        8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
        8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
        8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
        8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
        8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8
    };

    s_ba_x_in_xx = false;
    LDouble d = mag;
    frexpl(d, &exponent);
    return g_max_iterations + offset - (((iter - 1) << 3) + (long)adjust[exponent >> 3]);
}

static void put_hor_line(int x1, int y1, int x2, int color)
{
    for (int x = x1; x <= x2; x++)
    {
        (*g_plot)(x, y1, color);
    }
}

static void put_box(int x1, int y1, int x2, int y2, int color)
{
    for (; y1 <= y2; y1++)
    {
        put_hor_line(x1, y1, x2, color);
    }
}

/* maximum side length beyond which we start regular scanning instead of
   subdividing */
enum
{
    SCAN = 16
};

// pixel interleave used in scanning
enum
{
    INTERLEAVE = 4
};

// compute the value of the interpolation polynomial at (x,y)
#define GET_REAL(x, y) \
    interpolate(c_im1, mid_i, c_im2, \
        interpolate(c_re1, mid_r, c_re2, s_zi[0].re, s_zi[4].re, s_zi[1].re, x), \
        interpolate(c_re1, mid_r, c_re2, s_zi[5].re, s_zi[8].re, s_zi[6].re, x), \
        interpolate(c_re1, mid_r, c_re2, s_zi[2].re, s_zi[7].re, s_zi[3].re, x), y)
#define GET_IMAG(x, y) \
    interpolate(c_re1, mid_r, c_re2, \
        interpolate(c_im1, mid_i, c_im2, s_zi[0].im, s_zi[5].im, s_zi[2].im, y), \
        interpolate(c_im1, mid_i, c_im2, s_zi[4].im, s_zi[8].im, s_zi[7].im, y), \
        interpolate(c_im1, mid_i, c_im2, s_zi[1].im, s_zi[6].im, s_zi[3].im, y), x)

/* compute the value of the interpolation polynomial at (x,y)
   from saved values before interpolation failed to stay within tolerance */
#define GET_SAVED_REAL(x, y) \
    interpolate(c_im1, mid_i, c_im2, \
        interpolate(c_re1, mid_r, c_re2, s[0].re, s[4].re, s[1].re, x), \
        interpolate(c_re1, mid_r, c_re2, s[5].re, s[8].re, s[6].re, x), \
        interpolate(c_re1, mid_r, c_re2, s[2].re, s[7].re, s[3].re, x), y)
#define GET_SAVED_IMAG(x, y) \
    interpolate(c_re1, mid_r, c_re2, \
        interpolate(c_im1, mid_i, c_im2, s[0].im, s[5].im, s[2].im, y), \
        interpolate(c_im1, mid_i, c_im2, s[4].im, s[8].im, s[7].im, y), \
        interpolate(c_im1, mid_i, c_im2, s[1].im, s[6].im, s[3].im, y), x)

/* compute the value of the interpolation polynomial at (x,y)
   during scanning. Here, key values do not change, so we can precompute
   coefficients in one direction and simply evaluate the polynomial
   during scanning. */
#define GET_SCAN_REAL(x, y) \
    interpolate(c_im1, mid_i, c_im2, \
        evaluate(c_re1, mid_r, s_state.b1[0].re, s_state.b1[1].re, s_state.b1[2].re, x), \
        evaluate(c_re1, mid_r, s_state.b2[0].re, s_state.b2[1].re, s_state.b2[2].re, x), \
        evaluate(c_re1, mid_r, s_state.b3[0].re, s_state.b3[1].re, s_state.b3[2].re, x), y)
#define GET_SCAN_IMAG(x, y) \
    interpolate(c_re1, mid_r, c_re2, \
        evaluate(c_im1, mid_i, s_state.b1[0].im, s_state.b1[1].im, s_state.b1[2].im, y), \
        evaluate(c_im1, mid_i, s_state.b2[0].im, s_state.b2[1].im, s_state.b2[2].im, y), \
        evaluate(c_im1, mid_i, s_state.b3[0].im, s_state.b3[1].im, s_state.b3[2].im, y), x)

/* SOICompute - Perform simultaneous orbit iteration for a given rectangle

   Input: c_re1..c_im2 : values defining the four corners of the rectangle
          x1..y2     : corresponding pixel values
      zre1..zim9 : intermediate iterated values of the key points (key values)

      (c_re1,c_im1)               (c_re2,c_im1)
      (zre1,zim1)  (zre5,zim5)  (zre2,zim2)
           +------------+------------+
           |            |            |
           |            |            |
      (zre6,zim6)  (zre9,zim9)  (zre7,zim7)
           |            |            |
           |            |            |
           +------------+------------+
      (zre3,zim3)  (zre8,zim8)  (zre4,zim4)
      (c_re1,c_im2)               (c_re2,c_im2)

      iter       : current number of iterations
*/

/*
   The purpose of this macro is to reduce the number of parameters of the
   function rhombus(), since this is a recursive function, and stack space
   under DOS is extremely limited.
*/

#define RHOMBUS(CRE1, CRE2, CIM1, CIM2, X1, X2, Y1, Y2, ZRE1, ZIM1, ZRE2, ZIM2, ZRE3, ZIM3, ZRE4, ZIM4, \
    ZRE5, ZIM5, ZRE6, ZIM6, ZRE7, ZIM7, ZRE8, ZIM8, ZRE9, ZIM9, ITER)                                   \
    s_zi[0].re = (ZRE1);                                                                                \
    s_zi[0].im = (ZIM1);                                                                                \
    s_zi[1].re = (ZRE2);                                                                                \
    s_zi[1].im = (ZIM2);                                                                                \
    s_zi[2].re = (ZRE3);                                                                                \
    s_zi[2].im = (ZIM3);                                                                                \
    s_zi[3].re = (ZRE4);                                                                                \
    s_zi[3].im = (ZIM4);                                                                                \
    s_zi[4].re = (ZRE5);                                                                                \
    s_zi[4].im = (ZIM5);                                                                                \
    s_zi[5].re = (ZRE6);                                                                                \
    s_zi[5].im = (ZIM6);                                                                                \
    s_zi[6].re = (ZRE7);                                                                                \
    s_zi[6].im = (ZIM7);                                                                                \
    s_zi[7].re = (ZRE8);                                                                                \
    s_zi[7].im = (ZIM8);                                                                                \
    s_zi[8].re = (ZRE9);                                                                                \
    s_zi[8].im = (ZIM9);                                                                                \
    status = rhombus((CRE1), (CRE2), (CIM1), (CIM2), (X1), (X2), (Y1), (Y2), (ITER)) != 0;              \
    assert(status)

static int rhombus(
    LDouble c_re1, LDouble c_re2, LDouble c_im1, LDouble c_im2,
    int x1, int x2, int y1, int y2, long iter);

static int rhombus_aux(
    LDouble c_re1, LDouble c_re2, LDouble c_im1, LDouble c_im2,
    int x1, int x2, int y1, int y2, long iter)
{
    // The following variables do not need their values saved
    // used in scanning
    static long save_color;
    static long color;
    static long help_color;
    static int x;
    static int y;
    static int z;
    static int save_x;

    // number of iterations before SOI iteration cycle
    static long before;
    static int avail;

    // the variables below need to have local copies for recursive calls
    // center of rectangle
    LDouble mid_r = (c_re1 + c_re2)/2;
    LDouble mid_i = (c_im1 + c_im2)/2;

    LongDoubleComplex s[9];

    bool status = false;
    avail = stack_avail();
    g_soi_min_stack_available = std::min(avail, g_soi_min_stack_available);
    g_max_rhombus_depth = std::max(g_rhombus_depth, g_max_rhombus_depth);
    g_rhombus_stack[g_rhombus_depth] = avail;

    if (driver_key_pressed())
    {
        return 1;
    }
    if (iter > g_max_iterations)
    {
        put_box(x1, y1, x2, y2, 0);
        return 0;
    }

    if ((y2 - y1 <= SCAN) || (avail < g_soi_min_stack))
    {
        // finish up the image by scanning the rectangle
scan:
        interpolate(c_re1, mid_r, c_re2, s_zi[0].re, s_zi[4].re, s_zi[1].re, s_state.b1[0].re, s_state.b1[1].re, s_state.b1[2].re);
        interpolate(c_re1, mid_r, c_re2, s_zi[5].re, s_zi[8].re, s_zi[6].re, s_state.b2[0].re, s_state.b2[1].re, s_state.b2[2].re);
        interpolate(c_re1, mid_r, c_re2, s_zi[2].re, s_zi[7].re, s_zi[3].re, s_state.b3[0].re, s_state.b3[1].re, s_state.b3[2].re);

        interpolate(c_im1, mid_i, c_im2, s_zi[0].im, s_zi[5].im, s_zi[2].im, s_state.b1[0].im, s_state.b1[1].im, s_state.b1[2].im);
        interpolate(c_im1, mid_i, c_im2, s_zi[4].im, s_zi[8].im, s_zi[7].im, s_state.b2[0].im, s_state.b2[1].im, s_state.b2[2].im);
        interpolate(c_im1, mid_i, c_im2, s_zi[1].im, s_zi[6].im, s_zi[3].im, s_state.b3[0].im, s_state.b3[1].im, s_state.b3[2].im);

        s_state.step.re = (c_re2 - c_re1)/(x2 - x1);
        s_state.step.im = (c_im2 - c_im1)/(y2 - y1);
        s_state.interleave_step = INTERLEAVE*s_state.step.re;

        for (y = y1, s_state.z.im = c_im1; y < y2; y++, s_state.z.im += s_state.step.im)
        {
            if (driver_key_pressed())
            {
                return 1;
            }
            s_state.scan_z.re = GET_SCAN_REAL(c_re1, s_state.z.im);
            s_state.scan_z.im = GET_SCAN_IMAG(c_re1, s_state.z.im);
            save_color = iteration(c_re1, s_state.z.im, s_state.scan_z.re, s_state.scan_z.im, iter);
            if (save_color < 0)
            {
                return 1;
            }
            save_x = x1;
            for (x = x1 + INTERLEAVE, s_state.z.re = c_re1 + s_state.interleave_step; x < x2;
                    x += INTERLEAVE, s_state.z.re += s_state.interleave_step)
            {
                s_state.scan_z.re = GET_SCAN_REAL(s_state.z.re, s_state.z.im);
                s_state.scan_z.im = GET_SCAN_IMAG(s_state.z.re, s_state.z.im);

                color = iteration(s_state.z.re, s_state.z.im, s_state.scan_z.re, s_state.scan_z.im, iter);
                if (color < 0)
                {
                    return 1;
                }
                if (color == save_color)
                {
                    continue;
                }

                for (z = x - 1, s_state.help_real = s_state.z.re - s_state.step.re;
                        z > x - INTERLEAVE;
                        z--, s_state.help_real -= s_state.step.re)
                {
                    s_state.scan_z.re = GET_SCAN_REAL(s_state.help_real, s_state.z.im);
                    s_state.scan_z.im = GET_SCAN_IMAG(s_state.help_real, s_state.z.im);
                    help_color = iteration(s_state.help_real, s_state.z.im, s_state.scan_z.re, s_state.scan_z.im, iter);
                    if (help_color < 0)
                    {
                        return 1;
                    }
                    if (help_color == save_color)
                    {
                        break;
                    }
                    (*g_plot)(z, y, (int)(help_color&255));
                }

                if (save_x < z)
                {
                    put_hor_line(save_x, y, z, (int)(save_color&255));
                }
                else
                {
                    (*g_plot)(save_x, y, (int)(save_color&255));
                }

                save_x = x;
                save_color = color;
            }

            for (z = x2 - 1, s_state.help_real = c_re2 - s_state.step.re;
                z > save_x;
                z--, s_state.help_real -= s_state.step.re)
            {
                s_state.scan_z.re = GET_SCAN_REAL(s_state.help_real, s_state.z.im);
                s_state.scan_z.im = GET_SCAN_IMAG(s_state.help_real, s_state.z.im);
                help_color = iteration(s_state.help_real, s_state.z.im, s_state.scan_z.re, s_state.scan_z.im, iter);
                if (help_color < 0)
                {
                    return 1;
                }
                if (help_color == save_color)
                {
                    break;
                }

                (*g_plot)(z, y, (int)(help_color&255));
            }

            if (save_x < z)
            {
                put_hor_line(save_x, y, z, (int)(save_color&255));
            }
            else
            {
                (*g_plot)(save_x, y, (int)(save_color&255));
            }
        }
        return 0;
    }

    std::transform(std::begin(s_zi), std::end(s_zi), std::begin(s_state.rq), z_sqr);

    s_state.corner[0].re = 0.75*c_re1 + 0.25*c_re2;
    s_state.corner[0].im = 0.75*c_im1 + 0.25*c_im2;
    s_state.corner[1].re = 0.25*c_re1 + 0.75*c_re2;
    s_state.corner[1].im = 0.25*c_im1 + 0.75*c_im2;

    s_state.tz[0].re = GET_REAL(s_state.corner[0].re, s_state.corner[0].im);
    s_state.tz[0].im = GET_IMAG(s_state.corner[0].re, s_state.corner[0].im);

    s_state.tz[1].re = GET_REAL(s_state.corner[1].re, s_state.corner[0].im);
    s_state.tz[1].im = GET_IMAG(s_state.corner[1].re, s_state.corner[0].im);

    s_state.tz[2].re = GET_REAL(s_state.corner[0].re, s_state.corner[1].im);
    s_state.tz[2].im = GET_IMAG(s_state.corner[0].re, s_state.corner[1].im);

    s_state.tz[3].re = GET_REAL(s_state.corner[1].re, s_state.corner[1].im);
    s_state.tz[3].im = GET_IMAG(s_state.corner[1].re, s_state.corner[1].im);

    std::transform(std::begin(s_state.tz), std::end(s_state.tz), std::begin(s_state.tq), z_sqr);

    before = iter;

    while (true)
    {
        std::copy(std::begin(s_zi), std::end(s_zi), std::begin(s));

        // iterate key values
        s_zi[0].im = (s_zi[0].im + s_zi[0].im)*s_zi[0].re + c_im1;
        s_zi[0].re = s_state.rq[0].re - s_state.rq[0].im + c_re1;
        s_state.rq[0].re = s_zi[0].re*s_zi[0].re;
        s_state.rq[0].im = s_zi[0].im*s_zi[0].im;

        s_zi[1].im = (s_zi[1].im + s_zi[1].im)*s_zi[1].re + c_im1;
        s_zi[1].re = s_state.rq[1].re - s_state.rq[1].im + c_re2;
        s_state.rq[1].re = s_zi[1].re*s_zi[1].re;
        s_state.rq[1].im = s_zi[1].im*s_zi[1].im;

        s_zi[2].im = (s_zi[2].im + s_zi[2].im)*s_zi[2].re + c_im2;
        s_zi[2].re = s_state.rq[2].re - s_state.rq[2].im + c_re1;
        s_state.rq[2].re = s_zi[2].re*s_zi[2].re;
        s_state.rq[2].im = s_zi[2].im*s_zi[2].im;

        s_zi[3].im = (s_zi[3].im + s_zi[3].im)*s_zi[3].re + c_im2;
        s_zi[3].re = s_state.rq[3].re - s_state.rq[3].im + c_re2;
        s_state.rq[3].re = s_zi[3].re*s_zi[3].re;
        s_state.rq[3].im = s_zi[3].im*s_zi[3].im;

        s_zi[4].im = (s_zi[4].im + s_zi[4].im)*s_zi[4].re + c_im1;
        s_zi[4].re = s_state.rq[4].re - s_state.rq[4].im + mid_r;
        s_state.rq[4].re = s_zi[4].re*s_zi[4].re;
        s_state.rq[4].im = s_zi[4].im*s_zi[4].im;

        s_zi[5].im = (s_zi[5].im + s_zi[5].im)*s_zi[5].re + mid_i;
        s_zi[5].re = s_state.rq[5].re - s_state.rq[5].im + c_re1;
        s_state.rq[5].re = s_zi[5].re*s_zi[5].re;
        s_state.rq[5].im = s_zi[5].im*s_zi[5].im;

        s_zi[6].im = (s_zi[6].im + s_zi[6].im)*s_zi[6].re + mid_i;
        s_zi[6].re = s_state.rq[6].re - s_state.rq[6].im + c_re2;
        s_state.rq[6].re = s_zi[6].re*s_zi[6].re;
        s_state.rq[6].im = s_zi[6].im*s_zi[6].im;

        s_zi[7].im = (s_zi[7].im + s_zi[7].im)*s_zi[7].re + c_im2;
        s_zi[7].re = s_state.rq[7].re - s_state.rq[7].im + mid_r;
        s_state.rq[7].re = s_zi[7].re*s_zi[7].re;
        s_state.rq[7].im = s_zi[7].im*s_zi[7].im;

        s_zi[8].im = (s_zi[8].im + s_zi[8].im)*s_zi[8].re + mid_i;
        s_zi[8].re = s_state.rq[8].re - s_state.rq[8].im + mid_r;
        s_state.rq[8].re = s_zi[8].re*s_zi[8].re;
        s_state.rq[8].im = s_zi[8].im*s_zi[8].im;

        // iterate test point
        s_state.tz[0].im = (s_state.tz[0].im + s_state.tz[0].im)*s_state.tz[0].re + s_state.corner[0].im;
        s_state.tz[0].re = s_state.tq[0].re - s_state.tq[0].im + s_state.corner[0].re;
        s_state.tq[0].re = s_state.tz[0].re*s_state.tz[0].re;
        s_state.tq[0].im = s_state.tz[0].im*s_state.tz[0].im;

        s_state.tz[1].im = (s_state.tz[1].im + s_state.tz[1].im)*s_state.tz[1].re + s_state.corner[0].im;
        s_state.tz[1].re = s_state.tq[1].re - s_state.tq[1].im + s_state.corner[1].re;
        s_state.tq[1].re = s_state.tz[1].re*s_state.tz[1].re;
        s_state.tq[1].im = s_state.tz[1].im*s_state.tz[1].im;

        s_state.tz[2].im = (s_state.tz[2].im + s_state.tz[2].im)*s_state.tz[2].re + s_state.corner[1].im;
        s_state.tz[2].re = s_state.tq[2].re - s_state.tq[2].im + s_state.corner[0].re;
        s_state.tq[2].re = s_state.tz[2].re*s_state.tz[2].re;
        s_state.tq[2].im = s_state.tz[2].im*s_state.tz[2].im;

        s_state.tz[3].im = (s_state.tz[3].im + s_state.tz[3].im)*s_state.tz[3].re + s_state.corner[1].im;
        s_state.tz[3].re = s_state.tq[3].re - s_state.tq[3].im + s_state.corner[1].re;
        s_state.tq[3].re = s_state.tz[3].re*s_state.tz[3].re;
        s_state.tq[3].im = s_state.tz[3].im*s_state.tz[3].im;

        iter++;

        // if one of the iterated values bails out, subdivide
        if (std::find_if(std::begin(s_state.rq), std::end(s_state.rq),
            [](LongDoubleComplex z) { return z.re + z.im > 16.0; }) != std::end(s_state.rq))
        {
            break;
        }

        /* if maximum number of iterations is reached, the whole rectangle
        can be assumed part of M. This is of course best case behavior
        of SOI, we seldomly get there */
        if (iter > g_max_iterations)
        {
            put_box(x1, y1, x2, y2, 0);
            return 0;
        }

        /* now for all test points, check whether they exceed the
        allowed tolerance. if so, subdivide */
        s_state.limit.re = GET_REAL(s_state.corner[0].re, s_state.corner[0].im);
        s_state.limit.re = (s_state.tz[0].re == 0.0)?
           (s_state.limit.re == 0.0)?1.0:1000.0:
           s_state.limit.re/s_state.tz[0].re;
        if (std::fabs(1.0 - s_state.limit.re) > s_t_width)
        {
            break;
        }

        s_state.limit.im = GET_IMAG(s_state.corner[0].re, s_state.corner[0].im);
        s_state.limit.im = (s_state.tz[0].im == 0.0)?
           (s_state.limit.im == 0.0)?1.0:1000.0:
           s_state.limit.im/s_state.tz[0].im;
        if (std::fabs(1.0 - s_state.limit.im) > s_t_width)
        {
            break;
        }

        s_state.limit.re = GET_REAL(s_state.corner[1].re, s_state.corner[0].im);
        s_state.limit.re = (s_state.tz[1].re == 0.0)?
           (s_state.limit.re == 0.0)?1.0:1000.0:
           s_state.limit.re/s_state.tz[1].re;
        if (std::fabs(1.0 - s_state.limit.re) > s_t_width)
        {
            break;
        }

        s_state.limit.im = GET_IMAG(s_state.corner[1].re, s_state.corner[0].im);
        s_state.limit.im = (s_state.tz[1].im == 0.0)?
           (s_state.limit.im == 0.0)?1.0:1000.0:
           s_state.limit.im/s_state.tz[1].im;
        if (std::fabs(1.0 - s_state.limit.im) > s_t_width)
        {
            break;
        }

        s_state.limit.re = GET_REAL(s_state.corner[0].re, s_state.corner[1].im);
        s_state.limit.re = (s_state.tz[2].re == 0.0)?
           (s_state.limit.re == 0.0)?1.0:1000.0:
           s_state.limit.re/s_state.tz[2].re;
        if (std::fabs(1.0 - s_state.limit.re) > s_t_width)
        {
            break;
        }

        s_state.limit.im = GET_IMAG(s_state.corner[0].re, s_state.corner[1].im);
        s_state.limit.im = (s_state.tz[2].im == 0.0)?
           (s_state.limit.im == 0.0)?1.0:1000.0:
           s_state.limit.im/s_state.tz[2].im;
        if (std::fabs(1.0 - s_state.limit.im) > s_t_width)
        {
            break;
        }

        s_state.limit.re = GET_REAL(s_state.corner[1].re, s_state.corner[1].im);
        s_state.limit.re = (s_state.tz[3].re == 0.0)?
           (s_state.limit.re == 0.0)?1.0:1000.0:
           s_state.limit.re/s_state.tz[3].re;
        if (std::fabs(1.0 - s_state.limit.re) > s_t_width)
        {
            break;
        }

        s_state.limit.im = GET_IMAG(s_state.corner[1].re, s_state.corner[1].im);
        s_state.limit.im = (s_state.tz[3].im == 0.0)?
           (s_state.limit.im == 0.0)?1.0:1000.0:
           s_state.limit.im/s_state.tz[3].im;
        if (std::fabs(1.0 - s_state.limit.im) > s_t_width)
        {
            break;
        }
    }

    iter--;

    // this is a little heuristic I tried to improve performance.
    if (iter - before < 10)
    {
        std::copy(std::begin(s), std::end(s), std::begin(s_zi));
        goto scan;
    }

    // compute key values for subsequent rectangles

    LDouble re10 = interpolate(c_re1, mid_r, c_re2, s[0].re, s[4].re, s[1].re, s_state.corner[0].re);
    LDouble im10 = interpolate(c_re1, mid_r, c_re2, s[0].im, s[4].im, s[1].im, s_state.corner[0].re);

    LDouble re11 = interpolate(c_re1, mid_r, c_re2, s[0].re, s[4].re, s[1].re, s_state.corner[1].re);
    LDouble im11 = interpolate(c_re1, mid_r, c_re2, s[0].im, s[4].im, s[1].im, s_state.corner[1].re);

    LDouble re20 = interpolate(c_re1, mid_r, c_re2, s[2].re, s[7].re, s[3].re, s_state.corner[0].re);
    LDouble im20 = interpolate(c_re1, mid_r, c_re2, s[2].im, s[7].im, s[3].im, s_state.corner[0].re);

    LDouble re21 = interpolate(c_re1, mid_r, c_re2, s[2].re, s[7].re, s[3].re, s_state.corner[1].re);
    LDouble im21 = interpolate(c_re1, mid_r, c_re2, s[2].im, s[7].im, s[3].im, s_state.corner[1].re);

    LDouble re15 = interpolate(c_re1, mid_r, c_re2, s[5].re, s[8].re, s[6].re, s_state.corner[0].re);
    LDouble im15 = interpolate(c_re1, mid_r, c_re2, s[5].im, s[8].im, s[6].im, s_state.corner[0].re);

    LDouble re16 = interpolate(c_re1, mid_r, c_re2, s[5].re, s[8].re, s[6].re, s_state.corner[1].re);
    LDouble im16 = interpolate(c_re1, mid_r, c_re2, s[5].im, s[8].im, s[6].im, s_state.corner[1].re);

    LDouble re12 = interpolate(c_im1, mid_i, c_im2, s[0].re, s[5].re, s[2].re, s_state.corner[0].im);
    LDouble im12 = interpolate(c_im1, mid_i, c_im2, s[0].im, s[5].im, s[2].im, s_state.corner[0].im);

    LDouble re14 = interpolate(c_im1, mid_i, c_im2, s[1].re, s[6].re, s[3].re, s_state.corner[0].im);
    LDouble im14 = interpolate(c_im1, mid_i, c_im2, s[1].im, s[6].im, s[3].im, s_state.corner[0].im);

    LDouble re17 = interpolate(c_im1, mid_i, c_im2, s[0].re, s[5].re, s[2].re, s_state.corner[1].im);
    LDouble im17 = interpolate(c_im1, mid_i, c_im2, s[0].im, s[5].im, s[2].im, s_state.corner[1].im);

    LDouble re19 = interpolate(c_im1, mid_i, c_im2, s[1].re, s[6].re, s[3].re, s_state.corner[1].im);
    LDouble im19 = interpolate(c_im1, mid_i, c_im2, s[1].im, s[6].im, s[3].im, s_state.corner[1].im);

    LDouble re13 = interpolate(c_im1, mid_i, c_im2, s[4].re, s[8].re, s[7].re, s_state.corner[0].im);
    LDouble im13 = interpolate(c_im1, mid_i, c_im2, s[4].im, s[8].im, s[7].im, s_state.corner[0].im);

    LDouble re18 = interpolate(c_im1, mid_i, c_im2, s[4].re, s[8].re, s[7].re, s_state.corner[1].im);
    LDouble im18 = interpolate(c_im1, mid_i, c_im2, s[4].im, s[8].im, s[7].im, s_state.corner[1].im);

    LDouble re91 = GET_SAVED_REAL(s_state.corner[0].re, s_state.corner[0].im);
    LDouble im91 = GET_SAVED_IMAG(s_state.corner[0].re, s_state.corner[0].im);
    LDouble re92 = GET_SAVED_REAL(s_state.corner[1].re, s_state.corner[0].im);
    LDouble im92 = GET_SAVED_IMAG(s_state.corner[1].re, s_state.corner[0].im);
    LDouble re93 = GET_SAVED_REAL(s_state.corner[0].re, s_state.corner[1].im);
    LDouble im93 = GET_SAVED_IMAG(s_state.corner[0].re, s_state.corner[1].im);
    LDouble re94 = GET_SAVED_REAL(s_state.corner[1].re, s_state.corner[1].im);
    LDouble im94 = GET_SAVED_IMAG(s_state.corner[1].re, s_state.corner[1].im);

    RHOMBUS(c_re1, mid_r, c_im1, mid_i, x1, ((x1 + x2) >> 1), y1, ((y1 + y2) >> 1),
            s[0].re, s[0].im,
            s[4].re, s[4].im,
            s[5].re, s[5].im,
            s[8].re, s[8].im,
            re10, im10,
            re12, im12,
            re13, im13,
            re15, im15,
            re91, im91,
            iter);
    RHOMBUS(mid_r, c_re2, c_im1, mid_i, (x1 + x2) >> 1, x2, y1, (y1 + y2) >> 1,
            s[4].re, s[4].im,
            s[1].re, s[1].im,
            s[8].re, s[8].im,
            s[6].re, s[6].im,
            re11, im11,
            re13, im13,
            re14, im14,
            re16, im16,
            re92, im92,
            iter);
    RHOMBUS(c_re1, mid_r, mid_i, c_im2, x1, (x1 + x2) >> 1, (y1 + y2) >> 1, y2,
            s[5].re, s[5].im,
            s[8].re, s[8].im,
            s[2].re, s[2].im,
            s[7].re, s[7].im,
            re15, im15,
            re17, im17,
            re18, im18,
            re20, im20,
            re93, im93,
            iter);
    RHOMBUS(mid_r, c_re2, mid_i, c_im2, (x1 + x2) >> 1, x2, (y1 + y2) >> 1, y2,
            s[8].re, s[8].im,
            s[6].re, s[6].im,
            s[7].re, s[7].im,
            s[3].re, s[3].im,
            re16, im16,
            re18, im18,
            re19, im19,
            re21, im21,
            re94, im94,
            iter);

    return status ? 1 : 0;
}

static int rhombus(
    LDouble c_re1, LDouble c_re2, LDouble c_im1, LDouble c_im2,
    int x1, int x2, int y1, int y2, long iter)
{
    ++g_rhombus_depth;
    const int result = rhombus_aux(c_re1, c_re2, c_im1, c_im2, x1, x2, y1, y2, iter);
    --g_rhombus_depth;
    return result;
}

void soi_ldbl()
{
    // cppcheck-suppress unreadVariable
    bool status;
    LDouble tolerance = 0.1;
    LDouble xx_min_l;
    LDouble xx_max_l;
    LDouble yy_min_l;
    LDouble yy_max_l;
    g_soi_min_stack_available = 30000;
    g_rhombus_depth = -1;
    g_max_rhombus_depth = 0;
    if (g_bf_math != BFMathType::NONE)
    {
        xx_min_l = bf_to_float(g_bf_x_min);
        yy_min_l = bf_to_float(g_bf_y_min);
        xx_max_l = bf_to_float(g_bf_x_max);
        yy_max_l = bf_to_float(g_bf_y_max);
    }
    else
    {
        xx_min_l = g_x_min;
        yy_min_l = g_y_min;
        xx_max_l = g_x_max;
        yy_max_l = g_y_max;
    }
    s_t_width = tolerance/(g_logical_screen_x_dots - 1);
    LDouble step_x = (xx_max_l - xx_min_l) / g_logical_screen_x_dots;
    LDouble step_y = (yy_min_l - yy_max_l) / g_logical_screen_y_dots;
    s_equal = (step_x < step_y ? step_x : step_y);

    RHOMBUS(xx_min_l, xx_max_l, yy_max_l, yy_min_l,
            0, g_logical_screen_x_dots, 0, g_logical_screen_y_dots,
            xx_min_l, yy_max_l,
            xx_max_l, yy_max_l,
            xx_min_l, yy_min_l,
            xx_max_l, yy_min_l,
            (xx_max_l + xx_min_l)/2, yy_max_l,
            xx_min_l, (yy_max_l + yy_min_l)/2,
            xx_max_l, (yy_max_l + yy_min_l)/2,
            (xx_max_l + xx_min_l)/2, yy_min_l,
            (xx_min_l + xx_max_l)/2, (yy_max_l + yy_min_l)/2,
            1);
}
