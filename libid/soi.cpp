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
#include "port.h"
#include "prototyp.h"

#include "soi.h"

#include "calcfrac.h"
#include "drivers.h"
#include "id_data.h"
#include "stack_avail.h"

#include <algorithm>
#include <cassert>
#include <cmath>

#define EVERY 15
#define BASIN_COLOR 0

int g_rhombus_stack[10];
int g_rhombus_depth = 0;
int g_max_rhombus_depth;
int g_soi_min_stack_available;
int g_soi_min_stack = 2200; // and this much stack to not crash when <tab> is pressed

namespace
{

struct long_double_complex
{
    LDBL re;
    LDBL im;
};

struct soi_long_double_state
{
    long_double_complex z;
    long_double_complex step;
    LDBL interstep;
    LDBL helpre;
    long_double_complex scan_z;
    long_double_complex b1[3];
    long_double_complex b2[3];
    long_double_complex b3[3];
    long_double_complex limit;
    long_double_complex rq[9];
    long_double_complex corner[2];
    long_double_complex tz[4];
    long_double_complex tq[4];
};

} // namespace

inline long_double_complex zsqr(long_double_complex z)
{
    return { z.re*z.re, z.im*z.im };
}

/* Newton Interpolation.
   It computes the value of the interpolation polynomial given by
   (x0,w0)..(x2,w2) at x:=t */
inline LDBL interpolate(
    LDBL x0, LDBL x1, LDBL x2,
    LDBL w0, LDBL w1, LDBL w2,
    LDBL t)
{
    const LDBL b = (w1 - w0)/(x1 - x0);
    return (((w2 - w1)/(x2 - x1) - b)/(x2 - x0)*(t - x1) + b)*(t - x0) + w0;
}

/* compute coefficients of Newton polynomial (b0,..,b2) from
   (x0,w0),..,(x2,w2). */
inline void interpolate(
    LDBL x0, LDBL x1, LDBL x2,
    LDBL w0, LDBL w1, LDBL w2,
    LDBL &b0, LDBL &b1, LDBL &b2)
{
    b0 = w0;
    b1 = (w1 - w0)/(x1 - x0);
    b2 = ((w2 - w1)/(x2 - x1) - b1)/(x2 - x0);
}

// evaluate Newton polynomial given by (x0,b0),(x1,b1) at x:=t
inline LDBL evaluate(
    LDBL x0, LDBL x1,
    LDBL b0, LDBL b1, LDBL b2,
    LDBL t)
{
    return (b2*(t - x1) + b1)*(t - x0) + b0;
}

static long_double_complex zi[9];
static soi_long_double_state state{};
static LDBL twidth;
static LDBL equal;
static bool baxinxx = false;

static long iteration(
    LDBL cr, LDBL ci,
    LDBL re, LDBL im,
    long start)
{
    long iter;
    long offset = 0;
    LDBL ren;
    LDBL imn;
    LDBL mag;
    int exponent;

    if (baxinxx)
    {
        LDBL sre = re;
        LDBL sim = im;
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

            if (std::fabs(sre - re) < equal && std::fabs(sim - im) < equal)
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
        baxinxx = true;
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

    baxinxx = false;
    LDBL d = mag;
    frexpl(d, &exponent);
    return g_max_iterations + offset - (((iter - 1) << 3) + (long)adjust[exponent >> 3]);
}

static void puthline(int x1, int y1, int x2, int color)
{
    for (int x = x1; x <= x2; x++)
    {
        (*g_plot)(x, y1, color);
    }
}

static void putbox(int x1, int y1, int x2, int y2, int color)
{
    for (; y1 <= y2; y1++)
    {
        puthline(x1, y1, x2, color);
    }
}

/* maximum side length beyond which we start regular scanning instead of
   subdividing */
#define SCAN 16

// pixel interleave used in scanning
#define INTERLEAVE 4

// compute the value of the interpolation polynomial at (x,y)
#define GET_REAL(x, y) \
    interpolate(cim1, midi, cim2, \
        interpolate(cre1, midr, cre2, zi[0].re, zi[4].re, zi[1].re, x), \
        interpolate(cre1, midr, cre2, zi[5].re, zi[8].re, zi[6].re, x), \
        interpolate(cre1, midr, cre2, zi[2].re, zi[7].re, zi[3].re, x), y)
#define GET_IMAG(x, y) \
    interpolate(cre1, midr, cre2, \
        interpolate(cim1, midi, cim2, zi[0].im, zi[5].im, zi[2].im, y), \
        interpolate(cim1, midi, cim2, zi[4].im, zi[8].im, zi[7].im, y), \
        interpolate(cim1, midi, cim2, zi[1].im, zi[6].im, zi[3].im, y), x)

/* compute the value of the interpolation polynomial at (x,y)
   from saved values before interpolation failed to stay within tolerance */
#define GET_SAVED_REAL(x, y) \
    interpolate(cim1, midi, cim2, \
        interpolate(cre1, midr, cre2, s[0].re, s[4].re, s[1].re, x), \
        interpolate(cre1, midr, cre2, s[5].re, s[8].re, s[6].re, x), \
        interpolate(cre1, midr, cre2, s[2].re, s[7].re, s[3].re, x), y)
#define GET_SAVED_IMAG(x, y) \
    interpolate(cre1, midr, cre2, \
        interpolate(cim1, midi, cim2, s[0].im, s[5].im, s[2].im, y), \
        interpolate(cim1, midi, cim2, s[4].im, s[8].im, s[7].im, y), \
        interpolate(cim1, midi, cim2, s[1].im, s[6].im, s[3].im, y), x)

/* compute the value of the interpolation polynomial at (x,y)
   during scanning. Here, key values do not change, so we can precompute
   coefficients in one direction and simply evaluate the polynomial
   during scanning. */
#define GET_SCAN_REAL(x, y) \
    interpolate(cim1, midi, cim2, \
        evaluate(cre1, midr, state.b1[0].re, state.b1[1].re, state.b1[2].re, x), \
        evaluate(cre1, midr, state.b2[0].re, state.b2[1].re, state.b2[2].re, x), \
        evaluate(cre1, midr, state.b3[0].re, state.b3[1].re, state.b3[2].re, x), y)
#define GET_SCAN_IMAG(x, y) \
    interpolate(cre1, midr, cre2, \
        evaluate(cim1, midi, state.b1[0].im, state.b1[1].im, state.b1[2].im, y), \
        evaluate(cim1, midi, state.b2[0].im, state.b2[1].im, state.b2[2].im, y), \
        evaluate(cim1, midi, state.b3[0].im, state.b3[1].im, state.b3[2].im, y), x)

/* SOICompute - Perform simultaneous orbit iteration for a given rectangle

   Input: cre1..cim2 : values defining the four corners of the rectangle
          x1..y2     : corresponding pixel values
      zre1..zim9 : intermediate iterated values of the key points (key values)

      (cre1,cim1)               (cre2,cim1)
      (zre1,zim1)  (zre5,zim5)  (zre2,zim2)
           +------------+------------+
           |            |            |
           |            |            |
      (zre6,zim6)  (zre9,zim9)  (zre7,zim7)
           |            |            |
           |            |            |
           +------------+------------+
      (zre3,zim3)  (zre8,zim8)  (zre4,zim4)
      (cre1,cim2)               (cre2,cim2)

      iter       : current number of iterations
*/

/*
   The purpose of this macro is to reduce the number of parameters of the
   function rhombus(), since this is a recursive function, and stack space
   under DOS is extremely limited.
*/

#define RHOMBUS(CRE1, CRE2, CIM1, CIM2, X1, X2, Y1, Y2, ZRE1, ZIM1, ZRE2, ZIM2, ZRE3, ZIM3,  \
    ZRE4, ZIM4, ZRE5, ZIM5, ZRE6, ZIM6, ZRE7, ZIM7, ZRE8, ZIM8, ZRE9, ZIM9, ITER)    \
    zi[0].re = (ZRE1);zi[0].im = (ZIM1);                                                       \
    zi[1].re = (ZRE2);zi[1].im = (ZIM2);                                                       \
    zi[2].re = (ZRE3);zi[2].im = (ZIM3);                                                       \
    zi[3].re = (ZRE4);zi[3].im = (ZIM4);                                                       \
    zi[4].re = (ZRE5);zi[4].im = (ZIM5);                                                       \
    zi[5].re = (ZRE6);zi[5].im = (ZIM6);                                                       \
    zi[6].re = (ZRE7);zi[6].im = (ZIM7);                                                       \
    zi[7].re = (ZRE8);zi[7].im = (ZIM8);                                                       \
    zi[8].re = (ZRE9);zi[8].im = (ZIM9);                                                       \
    status = rhombus((CRE1), (CRE2), (CIM1), (CIM2), (X1), (X2), (Y1), (Y2), (ITER)) != 0; \
    assert(status)

static int rhombus(
    LDBL cre1, LDBL cre2, LDBL cim1, LDBL cim2,
    int x1, int x2, int y1, int y2, long iter);

static int rhombus_aux(
    LDBL cre1, LDBL cre2, LDBL cim1, LDBL cim2,
    int x1, int x2, int y1, int y2, long iter)
{
    // The following variables do not need their values saved
    // used in scanning
    static long savecolor;
    static long color;
    static long helpcolor;
    static int x;
    static int y;
    static int z;
    static int savex;

    // number of iterations before SOI iteration cycle
    static long before;
    static int avail;

    // the variables below need to have local copies for recursive calls
    // center of rectangle
    LDBL midr = (cre1 + cre2)/2;
    LDBL midi = (cim1 + cim2)/2;

    long_double_complex s[9];

    bool status = false;
    avail = stackavail();
    if (avail < g_soi_min_stack_available)
    {
        g_soi_min_stack_available = avail;
    }
    if (g_rhombus_depth > g_max_rhombus_depth)
    {
        g_max_rhombus_depth = g_rhombus_depth;
    }
    g_rhombus_stack[g_rhombus_depth] = avail;

    if (driver_key_pressed())
    {
        return 1;
    }
    if (iter > g_max_iterations)
    {
        putbox(x1, y1, x2, y2, 0);
        return 0;
    }

    if ((y2 - y1 <= SCAN) || (avail < g_soi_min_stack))
    {
        // finish up the image by scanning the rectangle
scan:
        interpolate(cre1, midr, cre2, zi[0].re, zi[4].re, zi[1].re, state.b1[0].re, state.b1[1].re, state.b1[2].re);
        interpolate(cre1, midr, cre2, zi[5].re, zi[8].re, zi[6].re, state.b2[0].re, state.b2[1].re, state.b2[2].re);
        interpolate(cre1, midr, cre2, zi[2].re, zi[7].re, zi[3].re, state.b3[0].re, state.b3[1].re, state.b3[2].re);

        interpolate(cim1, midi, cim2, zi[0].im, zi[5].im, zi[2].im, state.b1[0].im, state.b1[1].im, state.b1[2].im);
        interpolate(cim1, midi, cim2, zi[4].im, zi[8].im, zi[7].im, state.b2[0].im, state.b2[1].im, state.b2[2].im);
        interpolate(cim1, midi, cim2, zi[1].im, zi[6].im, zi[3].im, state.b3[0].im, state.b3[1].im, state.b3[2].im);

        state.step.re = (cre2 - cre1)/(x2 - x1);
        state.step.im = (cim2 - cim1)/(y2 - y1);
        state.interstep = INTERLEAVE*state.step.re;

        for (y = y1, state.z.im = cim1; y < y2; y++, state.z.im += state.step.im)
        {
            if (driver_key_pressed())
            {
                return 1;
            }
            state.scan_z.re = GET_SCAN_REAL(cre1, state.z.im);
            state.scan_z.im = GET_SCAN_IMAG(cre1, state.z.im);
            savecolor = iteration(cre1, state.z.im, state.scan_z.re, state.scan_z.im, iter);
            if (savecolor < 0)
            {
                return 1;
            }
            savex = x1;
            for (x = x1 + INTERLEAVE, state.z.re = cre1 + state.interstep; x < x2;
                    x += INTERLEAVE, state.z.re += state.interstep)
            {
                state.scan_z.re = GET_SCAN_REAL(state.z.re, state.z.im);
                state.scan_z.im = GET_SCAN_IMAG(state.z.re, state.z.im);

                color = iteration(state.z.re, state.z.im, state.scan_z.re, state.scan_z.im, iter);
                if (color < 0)
                {
                    return 1;
                }
                if (color == savecolor)
                {
                    continue;
                }

                for (z = x - 1, state.helpre = state.z.re - state.step.re;
                        z > x - INTERLEAVE;
                        z--, state.helpre -= state.step.re)
                {
                    state.scan_z.re = GET_SCAN_REAL(state.helpre, state.z.im);
                    state.scan_z.im = GET_SCAN_IMAG(state.helpre, state.z.im);
                    helpcolor = iteration(state.helpre, state.z.im, state.scan_z.re, state.scan_z.im, iter);
                    if (helpcolor < 0)
                    {
                        return 1;
                    }
                    if (helpcolor == savecolor)
                    {
                        break;
                    }
                    (*g_plot)(z, y, (int)(helpcolor&255));
                }

                if (savex < z)
                {
                    puthline(savex, y, z, (int)(savecolor&255));
                }
                else
                {
                    (*g_plot)(savex, y, (int)(savecolor&255));
                }

                savex = x;
                savecolor = color;
            }

            for (z = x2 - 1, state.helpre = cre2 - state.step.re;
                z > savex;
                z--, state.helpre -= state.step.re)
            {
                state.scan_z.re = GET_SCAN_REAL(state.helpre, state.z.im);
                state.scan_z.im = GET_SCAN_IMAG(state.helpre, state.z.im);
                helpcolor = iteration(state.helpre, state.z.im, state.scan_z.re, state.scan_z.im, iter);
                if (helpcolor < 0)
                {
                    return 1;
                }
                if (helpcolor == savecolor)
                {
                    break;
                }

                (*g_plot)(z, y, (int)(helpcolor&255));
            }

            if (savex < z)
            {
                puthline(savex, y, z, (int)(savecolor&255));
            }
            else
            {
                (*g_plot)(savex, y, (int)(savecolor&255));
            }
        }
        return 0;
    }

    std::transform(std::begin(zi), std::end(zi), std::begin(state.rq), zsqr);

    state.corner[0].re = 0.75*cre1 + 0.25*cre2;
    state.corner[0].im = 0.75*cim1 + 0.25*cim2;
    state.corner[1].re = 0.25*cre1 + 0.75*cre2;
    state.corner[1].im = 0.25*cim1 + 0.75*cim2;

    state.tz[0].re = GET_REAL(state.corner[0].re, state.corner[0].im);
    state.tz[0].im = GET_IMAG(state.corner[0].re, state.corner[0].im);

    state.tz[1].re = GET_REAL(state.corner[1].re, state.corner[0].im);
    state.tz[1].im = GET_IMAG(state.corner[1].re, state.corner[0].im);

    state.tz[2].re = GET_REAL(state.corner[0].re, state.corner[1].im);
    state.tz[2].im = GET_IMAG(state.corner[0].re, state.corner[1].im);

    state.tz[3].re = GET_REAL(state.corner[1].re, state.corner[1].im);
    state.tz[3].im = GET_IMAG(state.corner[1].re, state.corner[1].im);

    std::transform(std::begin(state.tz), std::end(state.tz), std::begin(state.tq), zsqr);

    before = iter;

    while (true)
    {
        std::copy(std::begin(zi), std::end(zi), std::begin(s));

        // iterate key values
        zi[0].im = (zi[0].im + zi[0].im)*zi[0].re + cim1;
        zi[0].re = state.rq[0].re - state.rq[0].im + cre1;
        state.rq[0].re = zi[0].re*zi[0].re;
        state.rq[0].im = zi[0].im*zi[0].im;

        zi[1].im = (zi[1].im + zi[1].im)*zi[1].re + cim1;
        zi[1].re = state.rq[1].re - state.rq[1].im + cre2;
        state.rq[1].re = zi[1].re*zi[1].re;
        state.rq[1].im = zi[1].im*zi[1].im;

        zi[2].im = (zi[2].im + zi[2].im)*zi[2].re + cim2;
        zi[2].re = state.rq[2].re - state.rq[2].im + cre1;
        state.rq[2].re = zi[2].re*zi[2].re;
        state.rq[2].im = zi[2].im*zi[2].im;

        zi[3].im = (zi[3].im + zi[3].im)*zi[3].re + cim2;
        zi[3].re = state.rq[3].re - state.rq[3].im + cre2;
        state.rq[3].re = zi[3].re*zi[3].re;
        state.rq[3].im = zi[3].im*zi[3].im;

        zi[4].im = (zi[4].im + zi[4].im)*zi[4].re + cim1;
        zi[4].re = state.rq[4].re - state.rq[4].im + midr;
        state.rq[4].re = zi[4].re*zi[4].re;
        state.rq[4].im = zi[4].im*zi[4].im;

        zi[5].im = (zi[5].im + zi[5].im)*zi[5].re + midi;
        zi[5].re = state.rq[5].re - state.rq[5].im + cre1;
        state.rq[5].re = zi[5].re*zi[5].re;
        state.rq[5].im = zi[5].im*zi[5].im;

        zi[6].im = (zi[6].im + zi[6].im)*zi[6].re + midi;
        zi[6].re = state.rq[6].re - state.rq[6].im + cre2;
        state.rq[6].re = zi[6].re*zi[6].re;
        state.rq[6].im = zi[6].im*zi[6].im;

        zi[7].im = (zi[7].im + zi[7].im)*zi[7].re + cim2;
        zi[7].re = state.rq[7].re - state.rq[7].im + midr;
        state.rq[7].re = zi[7].re*zi[7].re;
        state.rq[7].im = zi[7].im*zi[7].im;

        zi[8].im = (zi[8].im + zi[8].im)*zi[8].re + midi;
        zi[8].re = state.rq[8].re - state.rq[8].im + midr;
        state.rq[8].re = zi[8].re*zi[8].re;
        state.rq[8].im = zi[8].im*zi[8].im;

        // iterate test point
        state.tz[0].im = (state.tz[0].im + state.tz[0].im)*state.tz[0].re + state.corner[0].im;
        state.tz[0].re = state.tq[0].re - state.tq[0].im + state.corner[0].re;
        state.tq[0].re = state.tz[0].re*state.tz[0].re;
        state.tq[0].im = state.tz[0].im*state.tz[0].im;

        state.tz[1].im = (state.tz[1].im + state.tz[1].im)*state.tz[1].re + state.corner[0].im;
        state.tz[1].re = state.tq[1].re - state.tq[1].im + state.corner[1].re;
        state.tq[1].re = state.tz[1].re*state.tz[1].re;
        state.tq[1].im = state.tz[1].im*state.tz[1].im;

        state.tz[2].im = (state.tz[2].im + state.tz[2].im)*state.tz[2].re + state.corner[1].im;
        state.tz[2].re = state.tq[2].re - state.tq[2].im + state.corner[0].re;
        state.tq[2].re = state.tz[2].re*state.tz[2].re;
        state.tq[2].im = state.tz[2].im*state.tz[2].im;

        state.tz[3].im = (state.tz[3].im + state.tz[3].im)*state.tz[3].re + state.corner[1].im;
        state.tz[3].re = state.tq[3].re - state.tq[3].im + state.corner[1].re;
        state.tq[3].re = state.tz[3].re*state.tz[3].re;
        state.tq[3].im = state.tz[3].im*state.tz[3].im;

        iter++;

        // if one of the iterated values bails out, subdivide
        if (std::find_if(std::begin(state.rq), std::end(state.rq),
            [](long_double_complex z) { return z.re + z.im > 16.0; }) != std::end(state.rq))
        {
            break;
        }

        /* if maximum number of iterations is reached, the whole rectangle
        can be assumed part of M. This is of course best case behavior
        of SOI, we seldomly get there */
        if (iter > g_max_iterations)
        {
            putbox(x1, y1, x2, y2, 0);
            return 0;
        }

        /* now for all test points, check whether they exceed the
        allowed tolerance. if so, subdivide */
        state.limit.re = GET_REAL(state.corner[0].re, state.corner[0].im);
        state.limit.re = (state.tz[0].re == 0.0)?
           (state.limit.re == 0.0)?1.0:1000.0:
           state.limit.re/state.tz[0].re;
        if (std::fabs(1.0 - state.limit.re) > twidth)
        {
            break;
        }

        state.limit.im = GET_IMAG(state.corner[0].re, state.corner[0].im);
        state.limit.im = (state.tz[0].im == 0.0)?
           (state.limit.im == 0.0)?1.0:1000.0:
           state.limit.im/state.tz[0].im;
        if (std::fabs(1.0 - state.limit.im) > twidth)
        {
            break;
        }

        state.limit.re = GET_REAL(state.corner[1].re, state.corner[0].im);
        state.limit.re = (state.tz[1].re == 0.0)?
           (state.limit.re == 0.0)?1.0:1000.0:
           state.limit.re/state.tz[1].re;
        if (std::fabs(1.0 - state.limit.re) > twidth)
        {
            break;
        }

        state.limit.im = GET_IMAG(state.corner[1].re, state.corner[0].im);
        state.limit.im = (state.tz[1].im == 0.0)?
           (state.limit.im == 0.0)?1.0:1000.0:
           state.limit.im/state.tz[1].im;
        if (std::fabs(1.0 - state.limit.im) > twidth)
        {
            break;
        }

        state.limit.re = GET_REAL(state.corner[0].re, state.corner[1].im);
        state.limit.re = (state.tz[2].re == 0.0)?
           (state.limit.re == 0.0)?1.0:1000.0:
           state.limit.re/state.tz[2].re;
        if (std::fabs(1.0 - state.limit.re) > twidth)
        {
            break;
        }

        state.limit.im = GET_IMAG(state.corner[0].re, state.corner[1].im);
        state.limit.im = (state.tz[2].im == 0.0)?
           (state.limit.im == 0.0)?1.0:1000.0:
           state.limit.im/state.tz[2].im;
        if (std::fabs(1.0 - state.limit.im) > twidth)
        {
            break;
        }

        state.limit.re = GET_REAL(state.corner[1].re, state.corner[1].im);
        state.limit.re = (state.tz[3].re == 0.0)?
           (state.limit.re == 0.0)?1.0:1000.0:
           state.limit.re/state.tz[3].re;
        if (std::fabs(1.0 - state.limit.re) > twidth)
        {
            break;
        }

        state.limit.im = GET_IMAG(state.corner[1].re, state.corner[1].im);
        state.limit.im = (state.tz[3].im == 0.0)?
           (state.limit.im == 0.0)?1.0:1000.0:
           state.limit.im/state.tz[3].im;
        if (std::fabs(1.0 - state.limit.im) > twidth)
        {
            break;
        }
    }

    iter--;

    // this is a little heuristic I tried to improve performance.
    if (iter - before < 10)
    {
        std::copy(std::begin(s), std::end(s), std::begin(zi));
        goto scan;
    }

    // compute key values for subsequent rectangles

    LDBL re10 = interpolate(cre1, midr, cre2, s[0].re, s[4].re, s[1].re, state.corner[0].re);
    LDBL im10 = interpolate(cre1, midr, cre2, s[0].im, s[4].im, s[1].im, state.corner[0].re);

    LDBL re11 = interpolate(cre1, midr, cre2, s[0].re, s[4].re, s[1].re, state.corner[1].re);
    LDBL im11 = interpolate(cre1, midr, cre2, s[0].im, s[4].im, s[1].im, state.corner[1].re);

    LDBL re20 = interpolate(cre1, midr, cre2, s[2].re, s[7].re, s[3].re, state.corner[0].re);
    LDBL im20 = interpolate(cre1, midr, cre2, s[2].im, s[7].im, s[3].im, state.corner[0].re);

    LDBL re21 = interpolate(cre1, midr, cre2, s[2].re, s[7].re, s[3].re, state.corner[1].re);
    LDBL im21 = interpolate(cre1, midr, cre2, s[2].im, s[7].im, s[3].im, state.corner[1].re);

    LDBL re15 = interpolate(cre1, midr, cre2, s[5].re, s[8].re, s[6].re, state.corner[0].re);
    LDBL im15 = interpolate(cre1, midr, cre2, s[5].im, s[8].im, s[6].im, state.corner[0].re);

    LDBL re16 = interpolate(cre1, midr, cre2, s[5].re, s[8].re, s[6].re, state.corner[1].re);
    LDBL im16 = interpolate(cre1, midr, cre2, s[5].im, s[8].im, s[6].im, state.corner[1].re);

    LDBL re12 = interpolate(cim1, midi, cim2, s[0].re, s[5].re, s[2].re, state.corner[0].im);
    LDBL im12 = interpolate(cim1, midi, cim2, s[0].im, s[5].im, s[2].im, state.corner[0].im);

    LDBL re14 = interpolate(cim1, midi, cim2, s[1].re, s[6].re, s[3].re, state.corner[0].im);
    LDBL im14 = interpolate(cim1, midi, cim2, s[1].im, s[6].im, s[3].im, state.corner[0].im);

    LDBL re17 = interpolate(cim1, midi, cim2, s[0].re, s[5].re, s[2].re, state.corner[1].im);
    LDBL im17 = interpolate(cim1, midi, cim2, s[0].im, s[5].im, s[2].im, state.corner[1].im);

    LDBL re19 = interpolate(cim1, midi, cim2, s[1].re, s[6].re, s[3].re, state.corner[1].im);
    LDBL im19 = interpolate(cim1, midi, cim2, s[1].im, s[6].im, s[3].im, state.corner[1].im);

    LDBL re13 = interpolate(cim1, midi, cim2, s[4].re, s[8].re, s[7].re, state.corner[0].im);
    LDBL im13 = interpolate(cim1, midi, cim2, s[4].im, s[8].im, s[7].im, state.corner[0].im);

    LDBL re18 = interpolate(cim1, midi, cim2, s[4].re, s[8].re, s[7].re, state.corner[1].im);
    LDBL im18 = interpolate(cim1, midi, cim2, s[4].im, s[8].im, s[7].im, state.corner[1].im);

    LDBL re91 = GET_SAVED_REAL(state.corner[0].re, state.corner[0].im);
    LDBL im91 = GET_SAVED_IMAG(state.corner[0].re, state.corner[0].im);
    LDBL re92 = GET_SAVED_REAL(state.corner[1].re, state.corner[0].im);
    LDBL im92 = GET_SAVED_IMAG(state.corner[1].re, state.corner[0].im);
    LDBL re93 = GET_SAVED_REAL(state.corner[0].re, state.corner[1].im);
    LDBL im93 = GET_SAVED_IMAG(state.corner[0].re, state.corner[1].im);
    LDBL re94 = GET_SAVED_REAL(state.corner[1].re, state.corner[1].im);
    LDBL im94 = GET_SAVED_IMAG(state.corner[1].re, state.corner[1].im);

    RHOMBUS(cre1, midr, cim1, midi, x1, ((x1 + x2) >> 1), y1, ((y1 + y2) >> 1),
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
    RHOMBUS(midr, cre2, cim1, midi, (x1 + x2) >> 1, x2, y1, (y1 + y2) >> 1,
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
    RHOMBUS(cre1, midr, midi, cim2, x1, (x1 + x2) >> 1, (y1 + y2) >> 1, y2,
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
    RHOMBUS(midr, cre2, midi, cim2, (x1 + x2) >> 1, x2, (y1 + y2) >> 1, y2,
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
    LDBL cre1, LDBL cre2, LDBL cim1, LDBL cim2,
    int x1, int x2, int y1, int y2, long iter)
{
    ++g_rhombus_depth;
    const int result = rhombus_aux(cre1, cre2, cim1, cim2, x1, x2, y1, y2, iter);
    --g_rhombus_depth;
    return result;
}

void soi_ldbl()
{
    // cppcheck-suppress unreadVariable
    bool status;
    LDBL tolerance = 0.1;
    LDBL stepx;
    LDBL stepy;
    LDBL xxminl;
    LDBL xxmaxl;
    LDBL yyminl;
    LDBL yymaxl;
    g_soi_min_stack_available = 30000;
    g_rhombus_depth = -1;
    g_max_rhombus_depth = 0;
    if (g_bf_math != bf_math_type::NONE)
    {
        xxminl = bftofloat(g_bf_x_min);
        yyminl = bftofloat(g_bf_y_min);
        xxmaxl = bftofloat(g_bf_x_max);
        yymaxl = bftofloat(g_bf_y_max);
    }
    else
    {
        xxminl = g_x_min;
        yyminl = g_y_min;
        xxmaxl = g_x_max;
        yymaxl = g_y_max;
    }
    twidth = tolerance/(g_logical_screen_x_dots - 1);
    stepx = (xxmaxl - xxminl) / g_logical_screen_x_dots;
    stepy = (yyminl - yymaxl) / g_logical_screen_y_dots;
    equal = (stepx < stepy ? stepx : stepy);

    RHOMBUS(xxminl, xxmaxl, yymaxl, yyminl,
            0, g_logical_screen_x_dots, 0, g_logical_screen_y_dots,
            xxminl, yymaxl,
            xxmaxl, yymaxl,
            xxminl, yyminl,
            xxmaxl, yyminl,
            (xxmaxl + xxminl)/2, yymaxl,
            xxminl, (yymaxl + yyminl)/2,
            xxmaxl, (yymaxl + yyminl)/2,
            (xxmaxl + xxminl)/2, yyminl,
            (xxminl + xxmaxl)/2, (yymaxl + yyminl)/2,
            1);
}
