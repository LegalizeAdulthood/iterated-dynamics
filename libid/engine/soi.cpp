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
#include "engine/soi.h"

#include "engine/calcfrac.h"
#include "engine/fractals.h"
#include "engine/id_data.h"
#include "engine/ImageRegion.h"
#include "fractals/fractalp.h"
#include "misc/Driver.h"
#include "misc/stack_avail.h"

#include <algorithm>
#include <cmath>
#include <iterator>

using namespace id::fractals;
using namespace id::math;
using namespace id::misc;

namespace id::engine
{

enum
{
    EVERY = 15,
    BASIN_COLOR = 0
};

namespace
{

struct DoubleComplex
{
    double re;
    double im;
};

struct SOIDoubleState
{
    bool esc[9];
    bool t_esc[4];

    DoubleComplex z;
    DoubleComplex step;
    double interleave_step;
    double help_real;
    DoubleComplex scan_z;
    DoubleComplex b1[3];
    DoubleComplex b2[3];
    DoubleComplex b3[3];
    DoubleComplex limit;
    DoubleComplex rq[9];
    DoubleComplex corner[2];
    DoubleComplex tz[4];
    DoubleComplex tq[4];
};

} // namespace

int g_rhombus_stack[10]{};
static int g_rhombus_depth{};
int g_max_rhombus_depth{};
int g_soi_min_stack_available{};
int g_soi_min_stack{2200}; // and this much stack to not crash when <tab> is pressed

static bool rhombus(double c_re1, double c_re2, double c_im1, double c_im2, //
    int x1, int x2, int y1, int y2, long iter);

static DoubleComplex z_sqr(const DoubleComplex z)
{
    return { z.re*z.re, z.im*z.im };
}

/* compute coefficients of Newton polynomial (b0,..,b2) from
   (x0,w0),..,(x2,w2). */
static void interpolate(const double x0, const double x1, const double x2, //
    const double w0, const double w1, const double w2,                     //
    double &b0, double &b1, double &b2)
{
    b0 = w0;
    b1 = (w1 - w0)/(x1 - x0);
    b2 = ((w2 - w1)/(x2 - x1) - b1)/(x2 - x0);
}

// evaluate Newton polynomial given by (x0,b0),(x1,b1) at x:=t
static double evaluate(const double x0, const double x1, //
    const double b0, const double b1, const double b2,   //
    const double t)
{
    return (b2*(t - x1) + b1)*(t - x0) + b0;
}

static DoubleComplex s_zi[9]{};
static SOIDoubleState s_state{};
static double s_t_width{};
static double s_equal{};

static long iteration(const double cr, const double ci, //
    const double re, const double im,                   //
    long start)
{
    g_old_z.x = re;
    g_old_z.y = im;
    g_temp_sqr_x = sqr(g_old_z.x);
    g_temp_sqr_y = sqr(g_old_z.y);
    g_float_param = &g_init;
    g_float_param->x = cr;
    g_float_param->y = ci;
    while (orbit_calc() == 0 && start < g_max_iterations)
    {
        start++;
    }
    if (start >= g_max_iterations)
    {
        start = BASIN_COLOR;
    }
    return start;
}

static void put_hor_line(const int x1, const int y1, const int x2, const int color)
{
    for (int x = x1; x <= x2; x++)
    {
        g_plot(x, y1, color);
    }
}

static void put_box(const int x1, int y1, const int x2, const int y2, const int color)
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

/* Newton Interpolation.
   It computes the value of the interpolation polynomial given by
   (x0,w0)..(x2,w2) at x:=t */
static inline double interpolate(const double x0, const double x1, const double x2, //
    const double w0, const double w1, const double w2,                              //
    const double t)
{
    const double b = (w1 - w0)/(x1 - x0);
    return (((w2 - w1)/(x2 - x1) - b)/(x2 - x0)*(t - x1) + b)*(t - x0) + w0;
}

// SOICompute - Perform simultaneous orbit iteration for a given rectangle
//
// Input: c_re1..c_im2 : values defining the four corners of the rectangle
//        x1..y2     : corresponding pixel values
//    s_zi[0..8].re,im  : intermediate iterated values of the key points (key values)
//
// (c_re1,c_im1)                                    (c_re2,c_im1)
// (s_zi[0].re,s_zi[0].im)   (s_zi[4].re,s_zi[4].im)  (s_zi[1].re,s_zi[1].im)
//     +--------------------------+--------------------+
//     |                          |                    |
//     |                          |                    |
// (s_zi[5].re,s_zi[5].im)   (s_zi[8].re,s_zi[8].im)  (s_zi[6].re,s_zi[6].im)
//     |                          |                    |
//     |                          |                    |
//     +--------------------------+--------------------+
// (s_zi[2].re,s_zi[2].im)   (s_zi[7].re,s_zi[7].im)  (s_zi[3].re,s_zi[3].im)
// (c_re1,c_im2)                                    (c_re2,c_im2)
//
// iter       : current number of iterations

static bool rhombus2(const double cre1, const double cre2, const double cim1, const double cim2, //
    const int x1, const int x2, const int y1, const int y2,                                      //
    const double zre1, const double zim1, const double zre2, const double zim2,                  //
    const double zre3, const double zim3, const double zre4, const double zim4,                  //
    const double zre5, const double zim5, const double zre6, const double zim6,                  //
    const double zre7, const double zim7, const double zre8, const double zim8,                  //
    const double zre9, const double zim9,                                                        //
    const long iter)
{
    s_zi[0].re = zre1;
    s_zi[0].im = zim1;
    s_zi[1].re = zre2;
    s_zi[1].im = zim2;
    s_zi[2].re = zre3;
    s_zi[2].im = zim3;
    s_zi[3].re = zre4;
    s_zi[3].im = zim4;
    s_zi[4].re = zre5;
    s_zi[4].im = zim5;
    s_zi[5].re = zre6;
    s_zi[5].im = zim6;
    s_zi[6].re = zre7;
    s_zi[6].im = zim7;
    s_zi[7].re = zre8;
    s_zi[7].im = zim8;
    s_zi[8].re = zre9;
    s_zi[8].im = zim9;
    return rhombus(cre1, cre2, cim1, cim2, x1, x2, y1, y2, iter);
}

static bool rhombus2(const double cre1, const double cre2, const double cim1, const double cim2, //
    const int x1, const int x2, const int y1, const int y2,                                      //
    const DoubleComplex z1, const DoubleComplex z2,                                              //
    const DoubleComplex z3, const DoubleComplex z4,                                              //
    const double zre5, const double zim5, const double zre6, const double zim6,                  //
    const double zre7, const double zim7, const double zre8, const double zim8,                  //
    const double zre9, const double zim9,                                                        //
    const long iter)
{
    return rhombus2(cre1, cre2, cim1, cim2,                              //
        x1, x2, y1, y2,                                                  //
        z1.re, z1.im, z2.re, z2.im,                                      //
        z3.re, z3.im, z4.re, z4.im,                                      //
        zre5, zim5, zre6, zim6,                                          //
        zre7, zim7, zre8, zim8,                                          //
        zre9, zim9,                                                      //
        iter);
}

static void soi_orbit(DoubleComplex &z, DoubleComplex &rq, const double cr, const double ci, bool &esc)
{
    z.im = (z.im + z.im) * z.re + ci;
    z.re = rq.re - rq.im + cr;
    rq.re = z.re * z.re;
    rq.im = z.im * z.im;
    esc = rq.re + rq.im > 16.0;
}

static bool rhombus_aux(const double c_re1, const double c_re2, const double c_im1, const double c_im2, //
    const int x1, const int x2, const int y1, const int y2, long iter)
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
    const double mid_r = (c_re1 + c_re2) / 2;
    const double mid_i = (c_im1 + c_im2)/2;

    DoubleComplex s[9];

    bool status{};
    avail = stack_avail();
    g_soi_min_stack_available = std::min(avail, g_soi_min_stack_available);
    g_max_rhombus_depth = std::max(g_rhombus_depth, g_max_rhombus_depth);
    g_rhombus_stack[g_rhombus_depth] = avail;

    if (driver_key_pressed())
    {
        return true;
    }
    if (iter > g_max_iterations)
    {
        put_box(x1, y1, x2, y2, 0);
        return false;
    }

    if (y2 - y1 <= SCAN || avail < g_soi_min_stack)
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

        /* compute the value of the interpolation polynomial at (x,y)
           during scanning. Here, key values do not change, so we can precompute
           coefficients in one direction and simply evaluate the polynomial
           during scanning. */
        const auto get_scan_real{[=](const double re, const double im)
            {
                return interpolate(c_im1, mid_i, c_im2,
                    evaluate(c_re1, mid_r, s_state.b1[0].re, s_state.b1[1].re, s_state.b1[2].re, re),
                    evaluate(c_re1, mid_r, s_state.b2[0].re, s_state.b2[1].re, s_state.b2[2].re, re),
                    evaluate(c_re1, mid_r, s_state.b3[0].re, s_state.b3[1].re, s_state.b3[2].re, re), im);
            }};
        const auto get_scan_imag{[=](const double re, const double im)
            {
                return interpolate(c_re1, mid_r, c_re2,
                    evaluate(c_im1, mid_i, s_state.b1[0].im, s_state.b1[1].im, s_state.b1[2].im, im),
                    evaluate(c_im1, mid_i, s_state.b2[0].im, s_state.b2[1].im, s_state.b2[2].im, im),
                    evaluate(c_im1, mid_i, s_state.b3[0].im, s_state.b3[1].im, s_state.b3[2].im, im), re);
            }};
        for (y = y1, s_state.z.im = c_im1; y < y2; y++, s_state.z.im += s_state.step.im)
        {
            if (driver_key_pressed())
            {
                return true;
            }
            s_state.scan_z.re = get_scan_real(c_re1, s_state.z.im);
            s_state.scan_z.im = get_scan_imag(c_re1, s_state.z.im);
            save_color = iteration(c_re1, s_state.z.im, s_state.scan_z.re, s_state.scan_z.im, iter);
            if (save_color < 0)
            {
                return true;
            }
            save_x = x1;
            for (x = x1 + INTERLEAVE, s_state.z.re = c_re1 + s_state.interleave_step; x < x2;
                    x += INTERLEAVE, s_state.z.re += s_state.interleave_step)
            {
                s_state.scan_z.re = get_scan_real(s_state.z.re, s_state.z.im);
                s_state.scan_z.im = get_scan_imag(s_state.z.re, s_state.z.im);

                color = iteration(s_state.z.re, s_state.z.im, s_state.scan_z.re, s_state.scan_z.im, iter);
                if (color < 0)
                {
                    return true;
                }
                if (color == save_color)
                {
                    continue;
                }

                for (z = x - 1, s_state.help_real = s_state.z.re - s_state.step.re; z > x - INTERLEAVE; z--, s_state.help_real -= s_state.step.re)
                {
                    s_state.scan_z.re = get_scan_real(s_state.help_real, s_state.z.im);
                    s_state.scan_z.im = get_scan_imag(s_state.help_real, s_state.z.im);
                    help_color = iteration(s_state.help_real, s_state.z.im, s_state.scan_z.re, s_state.scan_z.im, iter);
                    if (help_color < 0)
                    {
                        return true;
                    }
                    if (help_color == save_color)
                    {
                        break;
                    }
                    g_plot(z, y, static_cast<int>(help_color & 255));
                }

                if (save_x < z)
                {
                    put_hor_line(save_x, y, z, static_cast<int>(save_color & 255));
                }
                else
                {
                    g_plot(save_x, y, static_cast<int>(save_color & 255));
                }

                save_x = x;
                save_color = color;
            }

            for (z = x2 - 1, s_state.help_real = c_re2 - s_state.step.re; z > save_x; z--, s_state.help_real -= s_state.step.re)
            {
                s_state.scan_z.re = get_scan_real(s_state.help_real, s_state.z.im);
                s_state.scan_z.im = get_scan_imag(s_state.help_real, s_state.z.im);
                help_color = iteration(s_state.help_real, s_state.z.im, s_state.scan_z.re, s_state.scan_z.im, iter);
                if (help_color < 0)
                {
                    return true;
                }
                if (help_color == save_color)
                {
                    break;
                }

                g_plot(z, y, static_cast<int>(help_color & 255));
            }

            if (save_x < z)
            {
                put_hor_line(save_x, y, z, static_cast<int>(save_color & 255));
            }
            else
            {
                g_plot(save_x, y, static_cast<int>(save_color & 255));
            }
        }

        return false;
    }

    std::transform(std::begin(s_zi), std::end(s_zi), std::begin(s_state.rq), z_sqr);

    s_state.corner[0].re = 0.75*c_re1 + 0.25*c_re2;
    s_state.corner[0].im = 0.75*c_im1 + 0.25*c_im2;
    s_state.corner[1].re = 0.25*c_re1 + 0.75*c_re2;
    s_state.corner[1].im = 0.25*c_im1 + 0.75*c_im2;

    // compute the value of the interpolation polynomial at (x,y)
    const auto get_real{[=](const double re, const double im)
        {
            return interpolate(c_im1, mid_i, c_im2,
                interpolate(c_re1, mid_r, c_re2, s_zi[0].re, s_zi[4].re, s_zi[1].re, re),
                interpolate(c_re1, mid_r, c_re2, s_zi[5].re, s_zi[8].re, s_zi[6].re, re),
                interpolate(c_re1, mid_r, c_re2, s_zi[2].re, s_zi[7].re, s_zi[3].re, re), im);
        }};
    const auto get_imag{[=](const double re, const double im)
        {
            return interpolate(c_re1, mid_r, c_re2,
                interpolate(c_im1, mid_i, c_im2, s_zi[0].im, s_zi[5].im, s_zi[2].im, im),
                interpolate(c_im1, mid_i, c_im2, s_zi[4].im, s_zi[8].im, s_zi[7].im, im),
                interpolate(c_im1, mid_i, c_im2, s_zi[1].im, s_zi[6].im, s_zi[3].im, im), re);
        }};
    s_state.tz[0].re = get_real(s_state.corner[0].re, s_state.corner[0].im);
    s_state.tz[0].im = get_imag(s_state.corner[0].re, s_state.corner[0].im);

    s_state.tz[1].re = get_real(s_state.corner[1].re, s_state.corner[0].im);
    s_state.tz[1].im = get_imag(s_state.corner[1].re, s_state.corner[0].im);

    s_state.tz[2].re = get_real(s_state.corner[0].re, s_state.corner[1].im);
    s_state.tz[2].im = get_imag(s_state.corner[0].re, s_state.corner[1].im);

    s_state.tz[3].re = get_real(s_state.corner[1].re, s_state.corner[1].im);
    s_state.tz[3].im = get_imag(s_state.corner[1].re, s_state.corner[1].im);

    std::transform(std::begin(s_state.tz), std::end(s_state.tz), std::begin(s_state.tq), z_sqr);

    before = iter;

    while (true)
    {
        std::copy(std::begin(s_zi), std::end(s_zi), std::begin(s));

        // iterate key values
        soi_orbit(s_zi[0], s_state.rq[0], c_re1, c_im1, s_state.esc[0]);
        /*
              zim1=(zim1+zim1)*zre1+c_im1;
              zre1=rq1-iq1+c_re1;
              rq1=zre1*zre1;
              iq1=zim1*zim1;
        */
        soi_orbit(s_zi[1], s_state.rq[1], c_re2, c_im1, s_state.esc[1]);
        /*
              zim2=(zim2+zim2)*zre2+c_im1;
              zre2=rq2-iq2+c_re2;
              rq2=zre2*zre2;
              iq2=zim2*zim2;
        */
        soi_orbit(s_zi[2], s_state.rq[2], c_re1, c_im2, s_state.esc[2]);
        /*
              zim3=(zim3+zim3)*zre3+c_im2;
              zre3=rq3-iq3+c_re1;
              rq3=zre3*zre3;
              iq3=zim3*zim3;
        */
        soi_orbit(s_zi[3], s_state.rq[3], c_re2, c_im2, s_state.esc[3]);
        /*
              zim4=(zim4+zim4)*zre4+c_im2;
              zre4=rq4-iq4+c_re2;
              rq4=zre4*zre4;
              iq4=zim4*zim4;
        */
        soi_orbit(s_zi[4], s_state.rq[4], mid_r, c_im1, s_state.esc[4]);
        /*
              zim5=(zim5+zim5)*zre5+c_im1;
              zre5=rq5-iq5+mid_r;
              rq5=zre5*zre5;
              iq5=zim5*zim5;
        */
        soi_orbit(s_zi[5], s_state.rq[5], c_re1, mid_i, s_state.esc[5]);
        /*
              zim6=(zim6+zim6)*zre6+mid_i;
              zre6=rq6-iq6+c_re1;
              rq6=zre6*zre6;
              iq6=zim6*zim6;
        */
        soi_orbit(s_zi[6], s_state.rq[6], c_re2, mid_i, s_state.esc[6]);
        /*
              zim7=(zim7+zim7)*zre7+mid_i;
              zre7=rq7-iq7+c_re2;
              rq7=zre7*zre7;
              iq7=zim7*zim7;
        */
        soi_orbit(s_zi[7], s_state.rq[7], mid_r, c_im2, s_state.esc[7]);
        /*
              zim8=(zim8+zim8)*zre8+c_im2;
              zre8=rq8-iq8+mid_r;
              rq8=zre8*zre8;
              iq8=zim8*zim8;
        */
        soi_orbit(s_zi[8], s_state.rq[8], mid_r, mid_i, s_state.esc[8]);
        /*
              zim9=(zim9+zim9)*zre9+mid_i;
              zre9=rq9-iq9+mid_r;
              rq9=zre9*zre9;
              iq9=zim9*zim9;
        */
        // iterate test point
        soi_orbit(s_state.tz[0], s_state.tq[0], s_state.corner[0].re, s_state.corner[0].im, s_state.t_esc[0]);
        /*
              tzi1=(tzi1+tzi1)*tzr1+ci1;
              tzr1=trq1-tiq1+cr1;
              trq1=tzr1*tzr1;
              tiq1=tzi1*tzi1;
        */

        soi_orbit(s_state.tz[1], s_state.tq[1], s_state.corner[1].re, s_state.corner[0].im, s_state.t_esc[1]);
        /*
              tzi2=(tzi2+tzi2)*tzr2+ci1;
              tzr2=trq2-tiq2+cr2;
              trq2=tzr2*tzr2;
              tiq2=tzi2*tzi2;
        */
        soi_orbit(s_state.tz[2], s_state.tq[2], s_state.corner[0].re, s_state.corner[1].im, s_state.t_esc[2]);
        /*
              tzi3=(tzi3+tzi3)*tzr3+ci2;
              tzr3=trq3-tiq3+cr1;
              trq3=tzr3*tzr3;
              tiq3=tzi3*tzi3;
        */
        soi_orbit(s_state.tz[3], s_state.tq[3], s_state.corner[1].re, s_state.corner[1].im, s_state.t_esc[3]);
        /*
              tzi4=(tzi4+tzi4)*tzr4+ci2;
              tzr4=trq4-tiq4+cr2;
              trq4=tzr4*tzr4;
              tiq4=tzi4*tzi4;
        */
        iter++;

        // if one of the iterated values bails out, subdivide
        if (std::find(std::begin(s_state.esc), std::end(s_state.esc), true) != std::end(s_state.esc)
            || std::find(std::begin(s_state.t_esc), std::end(s_state.t_esc), true) != std::end(s_state.t_esc))
        {
            break;
        }

        /* if maximum number of iterations is reached, the whole rectangle
        can be assumed part of M. This is of course best case behavior
        of SOI, we seldom get there */
        if (iter > g_max_iterations)
        {
            put_box(x1, y1, x2, y2, 0);
            return false;
        }

        /* now for all test points, check whether they exceed the
        allowed tolerance. if so, subdivide */
        s_state.limit.re = get_real(s_state.corner[0].re, s_state.corner[0].im);
        s_state.limit.re = s_state.tz[0].re == 0.0 ?
           s_state.limit.re == 0.0 ?1.0:1000.0:
           s_state.limit.re/s_state.tz[0].re;
        if (std::abs(1.0 - s_state.limit.re) > s_t_width)
        {
            break;
        }

        s_state.limit.im = get_imag(s_state.corner[0].re, s_state.corner[0].im);
        s_state.limit.im = s_state.tz[0].im == 0.0 ?
           s_state.limit.im == 0.0 ?1.0:1000.0:
           s_state.limit.im/s_state.tz[0].im;
        if (std::abs(1.0 - s_state.limit.im) > s_t_width)
        {
            break;
        }

        s_state.limit.re = get_real(s_state.corner[1].re, s_state.corner[0].im);
        s_state.limit.re = s_state.tz[1].re == 0.0 ?
           s_state.limit.re == 0.0 ?1.0:1000.0:
           s_state.limit.re/s_state.tz[1].re;
        if (std::abs(1.0 - s_state.limit.re) > s_t_width)
        {
            break;
        }

        s_state.limit.im = get_imag(s_state.corner[1].re, s_state.corner[0].im);
        s_state.limit.im = s_state.tz[1].im == 0.0 ?
           s_state.limit.im == 0.0 ?1.0:1000.0:
           s_state.limit.im/s_state.tz[1].im;
        if (std::abs(1.0 - s_state.limit.im) > s_t_width)
        {
            break;
        }

        s_state.limit.re = get_real(s_state.corner[0].re, s_state.corner[1].im);
        s_state.limit.re = s_state.tz[2].re == 0.0 ?
           s_state.limit.re == 0.0 ?1.0:1000.0:
           s_state.limit.re/s_state.tz[2].re;
        if (std::abs(1.0 - s_state.limit.re) > s_t_width)
        {
            break;
        }

        s_state.limit.im = get_imag(s_state.corner[0].re, s_state.corner[1].im);
        s_state.limit.im = s_state.tz[2].im == 0.0 ?
           s_state.limit.im == 0.0 ?1.0:1000.0:
           s_state.limit.im/s_state.tz[2].im;
        if (std::abs(1.0 - s_state.limit.im) > s_t_width)
        {
            break;
        }

        s_state.limit.re = get_real(s_state.corner[1].re, s_state.corner[1].im);
        s_state.limit.re = s_state.tz[3].re == 0.0 ?
           s_state.limit.re == 0.0 ?1.0:1000.0:
           s_state.limit.re/s_state.tz[3].re;
        if (std::abs(1.0 - s_state.limit.re) > s_t_width)
        {
            break;
        }

        s_state.limit.im = get_imag(s_state.corner[1].re, s_state.corner[1].im);
        s_state.limit.im = s_state.tz[3].im == 0.0 ?
           s_state.limit.im == 0.0 ?1.0:1000.0:
           s_state.limit.im/s_state.tz[3].im;
        if (std::abs(1.0 - s_state.limit.im) > s_t_width)
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

    const double re10 = interpolate(c_re1, mid_r, c_re2, s[0].re, s[4].re, s[1].re, s_state.corner[0].re);
    const double im10 = interpolate(c_re1, mid_r, c_re2, s[0].im, s[4].im, s[1].im, s_state.corner[0].re);

    const double re11 = interpolate(c_re1, mid_r, c_re2, s[0].re, s[4].re, s[1].re, s_state.corner[1].re);
    const double im11 = interpolate(c_re1, mid_r, c_re2, s[0].im, s[4].im, s[1].im, s_state.corner[1].re);

    const double re20 = interpolate(c_re1, mid_r, c_re2, s[2].re, s[7].re, s[3].re, s_state.corner[0].re);
    const double im20 = interpolate(c_re1, mid_r, c_re2, s[2].im, s[7].im, s[3].im, s_state.corner[0].re);

    const double re21 = interpolate(c_re1, mid_r, c_re2, s[2].re, s[7].re, s[3].re, s_state.corner[1].re);
    const double im21 = interpolate(c_re1, mid_r, c_re2, s[2].im, s[7].im, s[3].im, s_state.corner[1].re);

    const double re15 = interpolate(c_re1, mid_r, c_re2, s[5].re, s[8].re, s[6].re, s_state.corner[0].re);
    const double im15 = interpolate(c_re1, mid_r, c_re2, s[5].im, s[8].im, s[6].im, s_state.corner[0].re);

    const double re16 = interpolate(c_re1, mid_r, c_re2, s[5].re, s[8].re, s[6].re, s_state.corner[1].re);
    const double im16 = interpolate(c_re1, mid_r, c_re2, s[5].im, s[8].im, s[6].im, s_state.corner[1].re);

    const double re12 = interpolate(c_im1, mid_i, c_im2, s[0].re, s[5].re, s[2].re, s_state.corner[0].im);
    const double im12 = interpolate(c_im1, mid_i, c_im2, s[0].im, s[5].im, s[2].im, s_state.corner[0].im);

    const double re14 = interpolate(c_im1, mid_i, c_im2, s[1].re, s[6].re, s[3].re, s_state.corner[0].im);
    const double im14 = interpolate(c_im1, mid_i, c_im2, s[1].im, s[6].im, s[3].im, s_state.corner[0].im);

    const double re17 = interpolate(c_im1, mid_i, c_im2, s[0].re, s[5].re, s[2].re, s_state.corner[1].im);
    const double im17 = interpolate(c_im1, mid_i, c_im2, s[0].im, s[5].im, s[2].im, s_state.corner[1].im);

    const double re19 = interpolate(c_im1, mid_i, c_im2, s[1].re, s[6].re, s[3].re, s_state.corner[1].im);
    const double im19 = interpolate(c_im1, mid_i, c_im2, s[1].im, s[6].im, s[3].im, s_state.corner[1].im);

    const double re13 = interpolate(c_im1, mid_i, c_im2, s[4].re, s[8].re, s[7].re, s_state.corner[0].im);
    const double im13 = interpolate(c_im1, mid_i, c_im2, s[4].im, s[8].im, s[7].im, s_state.corner[0].im);

    const double re18 = interpolate(c_im1, mid_i, c_im2, s[4].re, s[8].re, s[7].re, s_state.corner[1].im);
    const double im18 = interpolate(c_im1, mid_i, c_im2, s[4].im, s[8].im, s[7].im, s_state.corner[1].im);

    // compute the value of the interpolation polynomial at (x,y)
    // from saved values before interpolation failed to stay within tolerance
    const auto get_saved_real{[=](const double re, const double im)
        {
            return interpolate(c_im1, mid_i, c_im2,
                interpolate(c_re1, mid_r, c_re2, s[0].re, s[4].re, s[1].re, re),
                interpolate(c_re1, mid_r, c_re2, s[5].re, s[8].re, s[6].re, re),
                interpolate(c_re1, mid_r, c_re2, s[2].re, s[7].re, s[3].re, re), im);
        }};
    const auto get_saved_imag{[=](const double re, const double im)
        {
            return interpolate(c_re1, mid_r, c_re2,
                interpolate(c_im1, mid_i, c_im2, s[0].im, s[5].im, s[2].im, im),
                interpolate(c_im1, mid_i, c_im2, s[4].im, s[8].im, s[7].im, im),
                interpolate(c_im1, mid_i, c_im2, s[1].im, s[6].im, s[3].im, im), re);
        }};
    const double re91 = get_saved_real(s_state.corner[0].re, s_state.corner[0].im);
    const double im91 = get_saved_imag(s_state.corner[0].re, s_state.corner[0].im);
    const double re92 = get_saved_real(s_state.corner[1].re, s_state.corner[0].im);
    const double im92 = get_saved_imag(s_state.corner[1].re, s_state.corner[0].im);
    const double re93 = get_saved_real(s_state.corner[0].re, s_state.corner[1].im);
    const double im93 = get_saved_imag(s_state.corner[0].re, s_state.corner[1].im);
    const double re94 = get_saved_real(s_state.corner[1].re, s_state.corner[1].im);
    const double im94 = get_saved_imag(s_state.corner[1].re, s_state.corner[1].im);

    status = rhombus2(c_re1, mid_r,                 //
        c_im1, mid_i,                               //
        x1, (x1 + x2) >> 1, y1, (y1 + y2) >> 1,     //
        s[0], s[4], s[5], s[8],                     //
        re10, im10, re12, im12,                     //
        re13, im13, re15, im15,                     //
        re91, im91,                                 //
        iter);
    status = rhombus2(mid_r, c_re2,                 //
        c_im1, mid_i,                               //
        (x1 + x2) >> 1, x2, y1, (y1 + y2) >> 1,     //
        s[4], s[1], s[8], s[6],                     //
        re11, im11, re13, im13,                     //
        re14, im14, re16, im16,                     //
        re92, im92,                                 //
        iter) && status;
    status = rhombus2(c_re1, mid_r,                 //
        mid_i, c_im2,                               //
        x1, (x1 + x2) >> 1, (y1 + y2) >> 1, y2,     //
        s[5], s[8], s[2], s[7],                     //
        re15, im15, re17, im17,                     //
        re18, im18, re20, im20,                     //
        re93, im93,                                 //
        iter) && status;
    status = rhombus2(mid_r, c_re2,                 //
        mid_i, c_im2,                               //
        (x1 + x2) >> 1, x2, (y1 + y2) >> 1, y2,     //
        s[8], s[6], s[7], s[3],                     //
        re16, im16, re18, im18,                     //
        re19, im19, re21, im21,                     //
        re94, im94,                                 //
        iter) && status;

    return status;
}

static bool rhombus(const double c_re1, const double c_re2, const double c_im1, const double c_im2, //
    const int x1, const int x2, const int y1, const int y2,                                         //
    const long iter)
{
    ++g_rhombus_depth;
    const bool result = rhombus_aux(c_re1, c_re2, c_im1, c_im2, x1, x2, y1, y2, iter);
    --g_rhombus_depth;
    return result;
}

void soi()
{
    // cppcheck-suppress unreadVariable
    constexpr double TOLERANCE = 0.1;
    double xx_min_l;
    double xx_max_l;
    double yy_min_l;
    double yy_max_l;
    g_soi_min_stack_available = 30000;
    g_rhombus_depth = -1;
    g_max_rhombus_depth = 0;
    if (g_bf_math != BFMathType::NONE)
    {
        xx_min_l = static_cast<double>(bf_to_float(g_bf_x_min));
        yy_min_l = static_cast<double>(bf_to_float(g_bf_y_min));
        xx_max_l = static_cast<double>(bf_to_float(g_bf_x_max));
        yy_max_l = static_cast<double>(bf_to_float(g_bf_y_max));
    }
    else
    {
        xx_min_l = g_image_region.m_min.x;
        yy_min_l = g_image_region.m_min.y;
        xx_max_l = g_image_region.m_max.x;
        yy_max_l = g_image_region.m_max.y;
    }
    s_t_width = TOLERANCE / (g_logical_screen_x_dots - 1);
    const double step_x = (xx_max_l - xx_min_l) / g_logical_screen_x_dots;
    const double step_y = (yy_min_l - yy_max_l) / g_logical_screen_y_dots;
    s_equal = step_x < step_y ? step_x : step_y;

    rhombus2(xx_min_l, xx_max_l,                                //
        yy_max_l, yy_min_l,                                     //
        0, g_logical_screen_x_dots, 0, g_logical_screen_y_dots, //
        xx_min_l, yy_max_l,                                     //
        xx_max_l, yy_max_l,                                     //
        xx_min_l, yy_min_l,                                     //
        xx_max_l, yy_min_l,                                     //
        (xx_max_l + xx_min_l) / 2, yy_max_l,                    //
        xx_min_l, (yy_max_l + yy_min_l) / 2,                    //
        xx_max_l, (yy_max_l + yy_min_l) / 2,                    //
        (xx_max_l + xx_min_l) / 2, yy_min_l,                    //
        (xx_min_l + xx_max_l) / 2, (yy_max_l + yy_min_l) / 2,   //
        1);
}

} // namespace id::engine
