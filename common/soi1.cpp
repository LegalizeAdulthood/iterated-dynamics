/*
 * soi.c --  SOI
 *
 *  Simultaneous Orbit Iteration Image Generation Method. Computes
 *      rectangular regions by tracking the orbits of only a few key points.
 *
 * Copyright (c) 1994-1997 Michael R. Ganss. All Rights Reserved.
 *
 * This file is distributed under the same conditions as
 * AlmondBread. For further information see
 * <URL:http://www.cs.tu-berlin.de/~rms/AlmondBread>.
 *
 */
#include <algorithm>
#include <cassert>

#include <float.h>
#include <time.h>
#if !defined(_WIN32)
#include <malloc.h>
#endif
#include "port.h"
#include "prototyp.h"
#include "drivers.h"
#include "fractype.h"

#define EVERY 15
#define BASIN_COLOR 0

extern int g_rhombus_stack[10];
extern int rhombus_depth;
extern int g_max_rhombus_depth;
extern int g_soi_min_stack_available;
extern int g_soi_min_stack; // need this much stack to recurse

namespace
{

struct double_complex
{
    double re;
    double im;
};

struct soi_double_state
{
    bool esc[9];
    bool tesc[4];

    double_complex z;
    double_complex step;
    double interstep;
    double helpre;
    double_complex scan_z;
    double_complex b1[3];
    double_complex b2[3];
    double_complex b3[3];
    double_complex limit;
    double_complex rq[9];
    double_complex corner[2];
    double_complex tz[4];
    double_complex tq[4];
};

inline double_complex zsqr(double_complex z)
{
    return { z.re*z.re, z.im*z.im };
}

double_complex zi[9];
soi_double_state state{};
double twidth;
double equal;

} // namespace

static long iteration(
    double cr, double ci,
    double re, double im,
    long start)
{
    g_old_z.x = re;
    g_old_z.y = im;
    g_temp_sqr_x = sqr(g_old_z.x);
    g_temp_sqr_y = sqr(g_old_z.y);
    g_float_param = &g_init;
    g_float_param->x = cr;
    g_float_param->y = ci;
    while (ORBITCALC() == 0 && start < g_max_iterations)
    {
        start++;
    }
    if (start >= g_max_iterations)
    {
        start = BASIN_COLOR;
    }
    return (start);
}

static void puthline(int x1, int y1, int x2, int color)
{
    int x;
    for (x = x1; x <= x2; x++)
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
        EVALUATE(cre1, midr, state.b1[0].re, state.b1[1].re, state.b1[2].re, x), \
        EVALUATE(cre1, midr, state.b2[0].re, state.b2[1].re, state.b2[2].re, x), \
        EVALUATE(cre1, midr, state.b3[0].re, state.b3[1].re, state.b3[2].re, x), y)
#define GET_SCAN_IMAG(x, y) \
    interpolate(cre1, midr, cre2, \
        EVALUATE(cim1, midi, state.b1[0].im, state.b1[1].im, state.b1[2].im, y), \
        EVALUATE(cim1, midi, state.b2[0].im, state.b2[1].im, state.b2[2].im, y), \
        EVALUATE(cim1, midi, state.b3[0].im, state.b3[1].im, state.b3[2].im, y), x)

/* compute coefficients of Newton polynomial (b0,..,b2) from
   (x0,w0),..,(x2,w2). */
#define INTERPOLATE(x0, x1, x2, w0, w1, w2, b0, b1, b2) \
    (b0) = (w0);                                        \
    (b1) = ((w1) - (w0))/((x1) - (x0));                 \
    (b2) = (((w2) - (w1))/((x2) - (x1)) - (b1))/((x2) - (x0))

// evaluate Newton polynomial given by (x0,b0),(x1,b1) at x:=t
#define EVALUATE(x0, x1, b0, b1, b2, t) \
    (((b2)*((t) - (x1)) + (b1))*((t) - (x0)) + (b0))

/* Newton Interpolation.
   It computes the value of the interpolation polynomial given by
   (x0,w0)..(x2,w2) at x:=t */
static inline double interpolate(double x0, double x1, double x2,
                        double w0, double w1, double w2,
                        double t)
{
    const double b = (w1 - w0)/(x1 - x0);
    return (((w2 - w1)/(x2 - x1) - b)/(x2 - x0)*(t - x1) + b)*(t - x0) + w0;
}

// SOICompute - Perform simultaneous orbit iteration for a given rectangle
//
// Input: cre1..cim2 : values defining the four corners of the rectangle
//        x1..y2     : corresponding pixel values
//    zi[0..8].re,im  : intermediate iterated values of the key points (key values)
//
// (cre1,cim1)                                    (cre2,cim1)
// (zi[0].re,zi[0].im)   (zi[4].re,zi[4].im)  (zi[1].re,zi[1].im)
//     +--------------------------+--------------------+
//     |                          |                    |
//     |                          |                    |
// (zi[5].re,zi[5].im)   (zi[8].re,zi[8].im)  (zi[6].re,zi[6].im)
//     |                          |                    |
//     |                          |                    |
//     +--------------------------+--------------------+
// (zi[2].re,zi[2].im)   (zi[7].re,zi[7].im)  (zi[3].re,zi[3].im)
// (cre1,cim2)                                    (cre2,cim2)
//
// iter       : current number of iterations

/*
   The purpose of this macro is to reduce the number of parameters of the
   function rhombus(), since this is a recursive function, and stack space
   under DOS is extremely limited.
*/

#define RHOMBUS(CRE1, CRE2, CIM1, CIM2, X1, X2, Y1, Y2, ZRE1, ZIM1, ZRE2, ZIM2, ZRE3, ZIM3, \
    ZRE4, ZIM4, ZRE5, ZIM5, ZRE6, ZIM6, ZRE7, ZIM7, ZRE8, ZIM8, ZRE9, ZIM9, ITER) \
    zi[0].re = (ZRE1);zi[0].im = (ZIM1);\
    zi[1].re = (ZRE2);zi[1].im = (ZIM2);\
    zi[2].re = (ZRE3);zi[2].im = (ZIM3);\
    zi[3].re = (ZRE4);zi[3].im = (ZIM4);\
    zi[4].re = (ZRE5);zi[4].im = (ZIM5);\
    zi[5].re = (ZRE6);zi[5].im = (ZIM6);\
    zi[6].re = (ZRE7);zi[6].im = (ZIM7);\
    zi[7].re = (ZRE8);zi[7].im = (ZIM8);\
    zi[8].re = (ZRE9);zi[8].im = (ZIM9);\
    status = rhombus((CRE1), (CRE2), (CIM1), (CIM2), (X1), (X2), (Y1), (Y2), (ITER)) != 0; \
    assert(status)

static int rhombus(double cre1, double cre2, double cim1, double cim2,
                   int x1, int x2, int y1, int y2, long iter)
{
    // The following variables do not need their values saved
    // used in scanning
    static long savecolor, color, helpcolor;
    static int x, y, z, savex;

    // number of iterations before SOI iteration cycle
    static long before;
    static int avail;

    // the variables below need to have local copies for recursive calls
    // center of rectangle
    double midr = (cre1 + cre2)/2;
    double midi = (cim1 + cim2)/2;

    double_complex s[9];
    double re10;
    double re11;
    double re12;
    double re13;
    double re14;
    double re15;
    double re16;
    double re17;
    double re18;
    double re19;
    double re20;
    double re21;
    double im10;
    double im11;
    double im12;
    double im13;
    double im14;
    double im15;
    double im16;
    double im17;
    double im18;
    double im19;
    double im20;
    double im21;
    double re91;
    double re92;
    double re93;
    double re94;
    double im91;
    double im92;
    double im93;
    double im94;

    bool status = false;
    rhombus_depth++;
    avail = stackavail();
    if (avail < g_soi_min_stack_available)
    {
        g_soi_min_stack_available = avail;
    }
    if (rhombus_depth > g_max_rhombus_depth)
    {
        g_max_rhombus_depth = rhombus_depth;
    }
    g_rhombus_stack[rhombus_depth] = avail;

    if (driver_key_pressed())
    {
        status = true;
        goto rhombus_done;
    }
    if (iter > g_max_iterations)
    {
        putbox(x1, y1, x2, y2, 0);
        status = false;
        goto rhombus_done;
    }

    if ((y2 - y1 <= SCAN) || (avail < g_soi_min_stack))
    {
        // finish up the image by scanning the rectangle
scan:
        INTERPOLATE(cre1, midr, cre2, zi[0].re, zi[4].re, zi[1].re, state.b1[0].re, state.b1[1].re, state.b1[2].re);
        INTERPOLATE(cre1, midr, cre2, zi[5].re, zi[8].re, zi[6].re, state.b2[0].re, state.b2[1].re, state.b2[2].re);
        INTERPOLATE(cre1, midr, cre2, zi[2].re, zi[7].re, zi[3].re, state.b3[0].re, state.b3[1].re, state.b3[2].re);

        INTERPOLATE(cim1, midi, cim2, zi[0].im, zi[5].im, zi[2].im, state.b1[0].im, state.b1[1].im, state.b1[2].im);
        INTERPOLATE(cim1, midi, cim2, zi[4].im, zi[8].im, zi[7].im, state.b2[0].im, state.b2[1].im, state.b2[2].im);
        INTERPOLATE(cim1, midi, cim2, zi[1].im, zi[6].im, zi[3].im, state.b3[0].im, state.b3[1].im, state.b3[2].im);

        state.step.re = (cre2 - cre1)/(x2 - x1);
        state.step.im = (cim2 - cim1)/(y2 - y1);
        state.interstep = INTERLEAVE*state.step.re;

        for (y = y1, state.z.im = cim1; y < y2; y++, state.z.im += state.step.im)
        {
            if (driver_key_pressed())
            {
                status = true;
                goto rhombus_done;
            }
            state.scan_z.re = GET_SCAN_REAL(cre1, state.z.im);
            state.scan_z.im = GET_SCAN_IMAG(cre1, state.z.im);
            savecolor = iteration(cre1, state.z.im, state.scan_z.re, state.scan_z.im, iter);
            if (savecolor < 0)
            {
                status = true;
                goto rhombus_done;
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
                    status = true;
                    goto rhombus_done;
                }
                if (color == savecolor)
                {
                    continue;
                }

                for (z = x - 1, state.helpre = state.z.re - state.step.re; z > x - INTERLEAVE; z--, state.helpre -= state.step.re)
                {
                    state.scan_z.re = GET_SCAN_REAL(state.helpre, state.z.im);
                    state.scan_z.im = GET_SCAN_IMAG(state.helpre, state.z.im);
                    helpcolor = iteration(state.helpre, state.z.im, state.scan_z.re, state.scan_z.im, iter);
                    if (helpcolor < 0)
                    {
                        status = true;
                        goto rhombus_done;
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

            for (z = x2 - 1, state.helpre = cre2 - state.step.re; z > savex; z--, state.helpre -= state.step.re)
            {
                state.scan_z.re = GET_SCAN_REAL(state.helpre, state.z.im);
                state.scan_z.im = GET_SCAN_IMAG(state.helpre, state.z.im);
                helpcolor = iteration(state.helpre, state.z.im, state.scan_z.re, state.scan_z.im, iter);
                if (helpcolor < 0)
                {
                    status = true;
                    goto rhombus_done;
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
        status = false;
        goto rhombus_done;
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

#define SOI_ORBIT1(zr, rq, zi, iq, cr, ci, esc) \
    tempsqrx = rq;                              \
    tempsqry = iq;                              \
    old.x = zr;                                 \
    old.y = zi;                                 \
    g_float_param->x = cr;                      \
    g_float_param->y = ci;                      \
    (esc) = ORBITCALC();                        \
    (rq) = tempsqrx;                            \
    (iq) = tempsqry;                            \
    (zr) = new.x;                               \
    (zi) = new.y

#define SOI_ORBIT(zr, rq, zi, iq, cr, ci, esc)  \
    (zi) = ((zi) + (zi))*(zr) + (ci);           \
    (zr) = (rq) - (iq) + (cr);                  \
    (rq) = (zr)*(zr);                           \
    (iq) = (zi)*(zi);                           \
    (esc) = (((rq) + (iq)) > 16.0)

        // iterate key values
        SOI_ORBIT(zi[0].re, state.rq[0].re, zi[0].im, state.rq[0].im, cre1, cim1, state.esc[0]);
        /*
              zim1=(zim1+zim1)*zre1+cim1;
              zre1=rq1-iq1+cre1;
              rq1=zre1*zre1;
              iq1=zim1*zim1;
        */
        SOI_ORBIT(zi[1].re, state.rq[1].re, zi[1].im, state.rq[1].im, cre2, cim1, state.esc[1]);
        /*
              zim2=(zim2+zim2)*zre2+cim1;
              zre2=rq2-iq2+cre2;
              rq2=zre2*zre2;
              iq2=zim2*zim2;
        */
        SOI_ORBIT(zi[2].re, state.rq[2].re, zi[2].im, state.rq[2].im, cre1, cim2, state.esc[2]);
        /*
              zim3=(zim3+zim3)*zre3+cim2;
              zre3=rq3-iq3+cre1;
              rq3=zre3*zre3;
              iq3=zim3*zim3;
        */
        SOI_ORBIT(zi[3].re, state.rq[3].re, zi[3].im, state.rq[3].im, cre2, cim2, state.esc[3]);
        /*
              zim4=(zim4+zim4)*zre4+cim2;
              zre4=rq4-iq4+cre2;
              rq4=zre4*zre4;
              iq4=zim4*zim4;
        */
        SOI_ORBIT(zi[4].re, state.rq[4].re, zi[4].im, state.rq[4].im, midr, cim1, state.esc[4]);
        /*
              zim5=(zim5+zim5)*zre5+cim1;
              zre5=rq5-iq5+midr;
              rq5=zre5*zre5;
              iq5=zim5*zim5;
        */
        SOI_ORBIT(zi[5].re, state.rq[5].re, zi[5].im, state.rq[5].im, cre1, midi, state.esc[5]);
        /*
              zim6=(zim6+zim6)*zre6+midi;
              zre6=rq6-iq6+cre1;
              rq6=zre6*zre6;
              iq6=zim6*zim6;
        */
        SOI_ORBIT(zi[6].re, state.rq[6].re, zi[6].im, state.rq[6].im, cre2, midi, state.esc[6]);
        /*
              zim7=(zim7+zim7)*zre7+midi;
              zre7=rq7-iq7+cre2;
              rq7=zre7*zre7;
              iq7=zim7*zim7;
        */
        SOI_ORBIT(zi[7].re, state.rq[7].re, zi[7].im, state.rq[7].im, midr, cim2, state.esc[7]);
        /*
              zim8=(zim8+zim8)*zre8+cim2;
              zre8=rq8-iq8+midr;
              rq8=zre8*zre8;
              iq8=zim8*zim8;
        */
        SOI_ORBIT(zi[8].re, state.rq[8].re, zi[8].im, state.rq[8].im, midr, midi, state.esc[8]);
        /*
              zim9=(zim9+zim9)*zre9+midi;
              zre9=rq9-iq9+midr;
              rq9=zre9*zre9;
              iq9=zim9*zim9;
        */
        // iterate test point
        SOI_ORBIT(state.tz[0].re, state.tq[0].re, state.tz[0].im, state.tq[0].im, state.corner[0].re, state.corner[0].im, state.tesc[0]);
        /*
              tzi1=(tzi1+tzi1)*tzr1+ci1;
              tzr1=trq1-tiq1+cr1;
              trq1=tzr1*tzr1;
              tiq1=tzi1*tzi1;
        */

        SOI_ORBIT(state.tz[1].re, state.tq[1].re, state.tz[1].im, state.tq[1].im, state.corner[1].re, state.corner[0].im, state.tesc[1]);
        /*
              tzi2=(tzi2+tzi2)*tzr2+ci1;
              tzr2=trq2-tiq2+cr2;
              trq2=tzr2*tzr2;
              tiq2=tzi2*tzi2;
        */
        SOI_ORBIT(state.tz[2].re, state.tq[2].re, state.tz[2].im, state.tq[2].im, state.corner[0].re, state.corner[1].im, state.tesc[2]);
        /*
              tzi3=(tzi3+tzi3)*tzr3+ci2;
              tzr3=trq3-tiq3+cr1;
              trq3=tzr3*tzr3;
              tiq3=tzi3*tzi3;
        */
        SOI_ORBIT(state.tz[3].re, state.tq[3].re, state.tz[3].im, state.tq[3].im, state.corner[1].re, state.corner[1].im, state.tesc[3]);
        /*
              tzi4=(tzi4+tzi4)*tzr4+ci2;
              tzr4=trq4-tiq4+cr2;
              trq4=tzr4*tzr4;
              tiq4=tzi4*tzi4;
        */
        iter++;

        // if one of the iterated values bails out, subdivide
        if (std::find(std::begin(state.esc), std::end(state.esc), true) != std::end(state.esc)
            || std::find(std::begin(state.tesc), std::end(state.tesc), true) != std::end(state.tesc))
        {
            break;
        }

        /* if maximum number of iterations is reached, the whole rectangle
        can be assumed part of M. This is of course best case behavior
        of SOI, we seldom get there */
        if (iter > g_max_iterations)
        {
            putbox(x1, y1, x2, y2, 0);
            status = false;
            goto rhombus_done;
        }

        /* now for all test points, check whether they exceed the
        allowed tolerance. if so, subdivide */
        state.limit.re = GET_REAL(state.corner[0].re, state.corner[0].im);
        state.limit.re = (state.tz[0].re == 0.0)?
           (state.limit.re == 0.0)?1.0:1000.0:
           state.limit.re/state.tz[0].re;
        if (fabs(1.0 - state.limit.re) > twidth)
        {
            break;
        }

        state.limit.im = GET_IMAG(state.corner[0].re, state.corner[0].im);
        state.limit.im = (state.tz[0].im == 0.0)?
           (state.limit.im == 0.0)?1.0:1000.0:
           state.limit.im/state.tz[0].im;
        if (fabs(1.0 - state.limit.im) > twidth)
        {
            break;
        }

        state.limit.re = GET_REAL(state.corner[1].re, state.corner[0].im);
        state.limit.re = (state.tz[1].re == 0.0)?
           (state.limit.re == 0.0)?1.0:1000.0:
           state.limit.re/state.tz[1].re;
        if (fabs(1.0 - state.limit.re) > twidth)
        {
            break;
        }

        state.limit.im = GET_IMAG(state.corner[1].re, state.corner[0].im);
        state.limit.im = (state.tz[1].im == 0.0)?
           (state.limit.im == 0.0)?1.0:1000.0:
           state.limit.im/state.tz[1].im;
        if (fabs(1.0 - state.limit.im) > twidth)
        {
            break;
        }

        state.limit.re = GET_REAL(state.corner[0].re, state.corner[1].im);
        state.limit.re = (state.tz[2].re == 0.0)?
           (state.limit.re == 0.0)?1.0:1000.0:
           state.limit.re/state.tz[2].re;
        if (fabs(1.0 - state.limit.re) > twidth)
        {
            break;
        }

        state.limit.im = GET_IMAG(state.corner[0].re, state.corner[1].im);
        state.limit.im = (state.tz[2].im == 0.0)?
           (state.limit.im == 0.0)?1.0:1000.0:
           state.limit.im/state.tz[2].im;
        if (fabs(1.0 - state.limit.im) > twidth)
        {
            break;
        }

        state.limit.re = GET_REAL(state.corner[1].re, state.corner[1].im);
        state.limit.re = (state.tz[3].re == 0.0)?
           (state.limit.re == 0.0)?1.0:1000.0:
           state.limit.re/state.tz[3].re;
        if (fabs(1.0 - state.limit.re) > twidth)
        {
            break;
        }

        state.limit.im = GET_IMAG(state.corner[1].re, state.corner[1].im);
        state.limit.im = (state.tz[3].im == 0.0)?
           (state.limit.im == 0.0)?1.0:1000.0:
           state.limit.im/state.tz[3].im;
        if (fabs(1.0 - state.limit.im) > twidth)
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

    re10 = interpolate(cre1, midr, cre2, s[0].re, s[4].re, s[1].re, state.corner[0].re);
    im10 = interpolate(cre1, midr, cre2, s[0].im, s[4].im, s[1].im, state.corner[0].re);

    re11 = interpolate(cre1, midr, cre2, s[0].re, s[4].re, s[1].re, state.corner[1].re);
    im11 = interpolate(cre1, midr, cre2, s[0].im, s[4].im, s[1].im, state.corner[1].re);

    re20 = interpolate(cre1, midr, cre2, s[2].re, s[7].re, s[3].re, state.corner[0].re);
    im20 = interpolate(cre1, midr, cre2, s[2].im, s[7].im, s[3].im, state.corner[0].re);

    re21 = interpolate(cre1, midr, cre2, s[2].re, s[7].re, s[3].re, state.corner[1].re);
    im21 = interpolate(cre1, midr, cre2, s[2].im, s[7].im, s[3].im, state.corner[1].re);

    re15 = interpolate(cre1, midr, cre2, s[5].re, s[8].re, s[6].re, state.corner[0].re);
    im15 = interpolate(cre1, midr, cre2, s[5].im, s[8].im, s[6].im, state.corner[0].re);

    re16 = interpolate(cre1, midr, cre2, s[5].re, s[8].re, s[6].re, state.corner[1].re);
    im16 = interpolate(cre1, midr, cre2, s[5].im, s[8].im, s[6].im, state.corner[1].re);

    re12 = interpolate(cim1, midi, cim2, s[0].re, s[5].re, s[2].re, state.corner[0].im);
    im12 = interpolate(cim1, midi, cim2, s[0].im, s[5].im, s[2].im, state.corner[0].im);

    re14 = interpolate(cim1, midi, cim2, s[1].re, s[6].re, s[3].re, state.corner[0].im);
    im14 = interpolate(cim1, midi, cim2, s[1].im, s[6].im, s[3].im, state.corner[0].im);

    re17 = interpolate(cim1, midi, cim2, s[0].re, s[5].re, s[2].re, state.corner[1].im);
    im17 = interpolate(cim1, midi, cim2, s[0].im, s[5].im, s[2].im, state.corner[1].im);

    re19 = interpolate(cim1, midi, cim2, s[1].re, s[6].re, s[3].re, state.corner[1].im);
    im19 = interpolate(cim1, midi, cim2, s[1].im, s[6].im, s[3].im, state.corner[1].im);

    re13 = interpolate(cim1, midi, cim2, s[4].re, s[8].re, s[7].re, state.corner[0].im);
    im13 = interpolate(cim1, midi, cim2, s[4].im, s[8].im, s[7].im, state.corner[0].im);

    re18 = interpolate(cim1, midi, cim2, s[4].re, s[8].re, s[7].re, state.corner[1].im);
    im18 = interpolate(cim1, midi, cim2, s[4].im, s[8].im, s[7].im, state.corner[1].im);

    re91 = GET_SAVED_REAL(state.corner[0].re, state.corner[0].im);
    re92 = GET_SAVED_REAL(state.corner[1].re, state.corner[0].im);
    re93 = GET_SAVED_REAL(state.corner[0].re, state.corner[1].im);
    re94 = GET_SAVED_REAL(state.corner[1].re, state.corner[1].im);

    im91 = GET_SAVED_IMAG(state.corner[0].re, state.corner[0].im);
    im92 = GET_SAVED_IMAG(state.corner[1].re, state.corner[0].im);
    im93 = GET_SAVED_IMAG(state.corner[0].re, state.corner[1].im);
    im94 = GET_SAVED_IMAG(state.corner[1].re, state.corner[1].im);

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
rhombus_done:
    rhombus_depth--;
    return status ? 1 : 0;
}

void soi()
{
    // cppcheck-suppress unreadVariable
    bool status;
    double tolerance = 0.1;
    double stepx, stepy;
    double xxminl, xxmaxl, yyminl, yymaxl;
    g_soi_min_stack_available = 30000;
    rhombus_depth = -1;
    g_max_rhombus_depth = 0;
    if (bf_math != bf_math_type::NONE)
    {
        xxminl = (double)bftofloat(g_bf_x_min);
        yyminl = (double)bftofloat(g_bf_y_min);
        xxmaxl = (double)bftofloat(g_bf_x_max);
        yymaxl = (double)bftofloat(g_bf_y_max);
    }
    else
    {
        xxminl = g_x_min;
        yyminl = g_y_min;
        xxmaxl = g_x_max;
        yymaxl = g_y_max;
    }
    twidth = tolerance/(g_logical_screen_x_dots - 1);
    stepx = (xxmaxl - xxminl)/g_logical_screen_x_dots;
    stepy = (yyminl - yymaxl)/g_logical_screen_y_dots;
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
