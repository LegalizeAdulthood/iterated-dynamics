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
#include "engine/SynchronousOrbit.h"

#include "engine/calcfrac.h"
#include "engine/fractals.h"
#include "engine/ImageRegion.h"
#include "engine/LogicalScreen.h"
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

int g_rhombus_stack[10]{};
int g_max_rhombus_depth{};
int g_soi_min_stack_available{};
int g_soi_min_stack{2200}; // and this much stack to not crash when <tab> is pressed

static DComplex z_sqr(const DComplex z)
{
    return { z.x*z.x, z.y*z.y };
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
//    m_zi[0..8].x,im  : intermediate iterated values of the key points (key values)
//
// (c_re1,c_im1)                                    (c_re2,c_im1)
// (m_zi[0].x,m_zi[0].y)   (m_zi[4].x,m_zi[4].y)  (m_zi[1].x,m_zi[1].y)
//     +--------------------------+--------------------+
//     |                          |                    |
//     |                          |                    |
// (m_zi[5].x,m_zi[5].y)   (m_zi[8].x,m_zi[8].y)  (m_zi[6].x,m_zi[6].y)
//     |                          |                    |
//     |                          |                    |
//     +--------------------------+--------------------+
// (m_zi[2].x,m_zi[2].y)   (m_zi[7].x,m_zi[7].y)  (m_zi[3].x,m_zi[3].y)
// (c_re1,c_im2)                                    (c_re2,c_im2)
//
// iter       : current number of iterations

SynchronousOrbit::RhombusInput SynchronousOrbit::make_rhombus_input(                               //
    const double c_re1, const double c_re2, const double c_im1, const double c_im2,                //
    const int x1, const int x2, const int y1, const int y2,                                        //
    const DComplex z1, const DComplex z2, const DComplex z3, const DComplex z4,                    //
    const DComplex z5, const DComplex z6, const DComplex z7, const DComplex z8, const DComplex z9, //
    const long iter, const int depth)
{
    RhombusInput input{};
    input.c_re1 = c_re1;
    input.c_re2 = c_re2;
    input.c_im1 = c_im1;
    input.c_im2 = c_im2;
    input.x1 = x1;
    input.x2 = x2;
    input.y1 = y1;
    input.y2 = y2;
    input.iter = iter;
    input.depth = depth;
    input.zi = {z1, z2, z3, z4, z5, z6, z7, z8, z9};
    return input;
}

void SynchronousOrbit::push_rhombus(const RhombusInput &input)
{
    RhombusFrame frame{};
    frame.input = input;
    m_frame_stack.push_back(frame);
}

SynchronousOrbit::FrameResult SynchronousOrbit::process_next_frame()
{
    if (m_frame_stack.empty())
    {
        return FrameResult::DONE;
    }

    RhombusFrame &frame = m_frame_stack.back();
    if (frame.phase == RhombusPhase::SCAN)
    {
        if (process_scan(frame) == RhombusStep::COMPLETE)
        {
            m_frame_stack.pop_back();
            return m_frame_stack.empty() ? FrameResult::DONE : FrameResult::CONTINUE;
        }
        return FrameResult::YIELDED;
    }

    if (frame.phase == RhombusPhase::CHILDREN)
    {
        if (frame.child_index >= static_cast<int>(frame.children.size()))
        {
            m_frame_stack.pop_back();
            return m_frame_stack.empty() ? FrameResult::DONE : FrameResult::CONTINUE;
        }

        const RhombusInput child{frame.children[frame.child_index++]};
        push_rhombus(child);
        return FrameResult::CONTINUE;
    }

    const RhombusStep step{process_rhombus(frame)};
    if (step == RhombusStep::COMPLETE)
    {
        m_frame_stack.pop_back();
        return m_frame_stack.empty() ? FrameResult::DONE : FrameResult::CONTINUE;
    }
    if (step == RhombusStep::INCOMPLETE)
    {
        return FrameResult::YIELDED;
    }
    return FrameResult::CONTINUE;
}

void SynchronousOrbit::orbit(DComplex &z, DComplex &rq, const double cr, const double ci, bool &esc)
{
    z.y = (z.y + z.y) * z.x + ci;
    z.x = rq.x - rq.y + cr;
    rq.x = z.x * z.x;
    rq.y = z.y * z.y;
    esc = rq.x + rq.y > 16.0;
}

void SynchronousOrbit::begin_scan(RhombusFrame &frame)
{
    const RhombusInput &input = frame.input;
    ScanState &scan = frame.scan;
    const double mid_r = (input.c_re1 + input.c_re2) / 2;
    const double mid_i = (input.c_im1 + input.c_im2) / 2;

    interpolate(input.c_re1, mid_r, input.c_re2, input.zi[0].x, input.zi[4].x, input.zi[1].x, scan.b1[0].x,
        scan.b1[1].x, scan.b1[2].x);
    interpolate(input.c_re1, mid_r, input.c_re2, input.zi[5].x, input.zi[8].x, input.zi[6].x, scan.b2[0].x,
        scan.b2[1].x, scan.b2[2].x);
    interpolate(input.c_re1, mid_r, input.c_re2, input.zi[2].x, input.zi[7].x, input.zi[3].x, scan.b3[0].x,
        scan.b3[1].x, scan.b3[2].x);

    interpolate(input.c_im1, mid_i, input.c_im2, input.zi[0].y, input.zi[5].y, input.zi[2].y, scan.b1[0].y,
        scan.b1[1].y, scan.b1[2].y);
    interpolate(input.c_im1, mid_i, input.c_im2, input.zi[4].y, input.zi[8].y, input.zi[7].y, scan.b2[0].y,
        scan.b2[1].y, scan.b2[2].y);
    interpolate(input.c_im1, mid_i, input.c_im2, input.zi[1].y, input.zi[6].y, input.zi[3].y, scan.b3[0].y,
        scan.b3[1].y, scan.b3[2].y);

    scan.step.x = (input.c_re2 - input.c_re1) / (input.x2 - input.x1);
    scan.step.y = (input.c_im2 - input.c_im1) / (input.y2 - input.y1);
    scan.interleave_step = INTERLEAVE * scan.step.x;
    scan.y = input.y1;
    scan.z_value.y = input.c_im1;
    scan.phase = ScanPhase::ROW_START;
    frame.phase = RhombusPhase::SCAN;
}

double SynchronousOrbit::scan_real(const RhombusFrame &frame, const double re, const double im) const
{
    const RhombusInput &input = frame.input;
    const ScanState &scan = frame.scan;
    const double mid_i = (input.c_im1 + input.c_im2) / 2;
    return interpolate(input.c_im1, mid_i, input.c_im2,
        evaluate(input.c_re1, (input.c_re1 + input.c_re2) / 2, scan.b1[0].x, scan.b1[1].x, scan.b1[2].x, re),
        evaluate(input.c_re1, (input.c_re1 + input.c_re2) / 2, scan.b2[0].x, scan.b2[1].x, scan.b2[2].x, re),
        evaluate(input.c_re1, (input.c_re1 + input.c_re2) / 2, scan.b3[0].x, scan.b3[1].x, scan.b3[2].x, re), im);
}

double SynchronousOrbit::scan_imag(const RhombusFrame &frame, const double re, const double im) const
{
    const RhombusInput &input = frame.input;
    const ScanState &scan = frame.scan;
    const double mid_r = (input.c_re1 + input.c_re2) / 2;
    return interpolate(input.c_re1, mid_r, input.c_re2,
        evaluate(input.c_im1, (input.c_im1 + input.c_im2) / 2, scan.b1[0].y, scan.b1[1].y, scan.b1[2].y, im),
        evaluate(input.c_im1, (input.c_im1 + input.c_im2) / 2, scan.b2[0].y, scan.b2[1].y, scan.b2[2].y, im),
        evaluate(input.c_im1, (input.c_im1 + input.c_im2) / 2, scan.b3[0].y, scan.b3[1].y, scan.b3[2].y, im), re);
}

SynchronousOrbit::RhombusStep SynchronousOrbit::process_scan(RhombusFrame &frame)
{
    const RhombusInput &input = frame.input;
    ScanState &scan = frame.scan;
    while (scan.y < input.y2)
    {
        switch (scan.phase)
        {
        case ScanPhase::ROW_START:
            if (driver_key_pressed())
            {
                return RhombusStep::COMPLETE;
            }
            scan.scan_z.x = scan_real(frame, input.c_re1, scan.z_value.y);
            scan.scan_z.y = scan_imag(frame, input.c_re1, scan.z_value.y);
            scan.save_color = iteration(input.c_re1, scan.z_value.y, scan.scan_z.x, scan.scan_z.y, input.iter);
            if (scan.save_color < 0)
            {
                return RhombusStep::COMPLETE;
            }
            scan.save_x = input.x1;
            scan.x = input.x1 + INTERLEAVE;
            scan.z_value.x = input.c_re1 + scan.interleave_step;
            scan.phase = ScanPhase::INTERLEAVE;
            break;

        case ScanPhase::INTERLEAVE:
            if (scan.x >= input.x2)
            {
                scan.z = input.x2 - 1;
                scan.help_real = input.c_re2 - scan.step.x;
                scan.phase = ScanPhase::ROW_END_BACKFILL;
                break;
            }
            scan.scan_z.x = scan_real(frame, scan.z_value.x, scan.z_value.y);
            scan.scan_z.y = scan_imag(frame, scan.z_value.x, scan.z_value.y);
            scan.color = iteration(scan.z_value.x, scan.z_value.y, scan.scan_z.x, scan.scan_z.y, input.iter);
            if (scan.color < 0)
            {
                return RhombusStep::COMPLETE;
            }
            if (scan.color == scan.save_color)
            {
                scan.x += INTERLEAVE;
                scan.z_value.x += scan.interleave_step;
                break;
            }
            scan.z = scan.x - 1;
            scan.help_real = scan.z_value.x - scan.step.x;
            scan.phase = ScanPhase::INTERLEAVE_BACKFILL;
            break;

        case ScanPhase::INTERLEAVE_BACKFILL:
            while (scan.z > scan.x - INTERLEAVE)
            {
                scan.scan_z.x = scan_real(frame, scan.help_real, scan.z_value.y);
                scan.scan_z.y = scan_imag(frame, scan.help_real, scan.z_value.y);
                scan.help_color = iteration(scan.help_real, scan.z_value.y, scan.scan_z.x, scan.scan_z.y, input.iter);
                if (scan.help_color < 0)
                {
                    return RhombusStep::COMPLETE;
                }
                if (scan.help_color == scan.save_color)
                {
                    break;
                }
                g_plot(scan.z, scan.y, static_cast<int>(scan.help_color & 255));
                --scan.z;
                scan.help_real -= scan.step.x;
            }
            if (scan.save_x < scan.z)
            {
                put_hor_line(scan.save_x, scan.y, scan.z, static_cast<int>(scan.save_color & 255));
            }
            else
            {
                g_plot(scan.save_x, scan.y, static_cast<int>(scan.save_color & 255));
            }
            scan.save_x = scan.x;
            scan.save_color = scan.color;
            scan.x += INTERLEAVE;
            scan.z_value.x += scan.interleave_step;
            scan.phase = ScanPhase::INTERLEAVE;
            break;

        case ScanPhase::ROW_END_BACKFILL:
            while (scan.z > scan.save_x)
            {
                scan.scan_z.x = scan_real(frame, scan.help_real, scan.z_value.y);
                scan.scan_z.y = scan_imag(frame, scan.help_real, scan.z_value.y);
                scan.help_color = iteration(scan.help_real, scan.z_value.y, scan.scan_z.x, scan.scan_z.y, input.iter);
                if (scan.help_color < 0)
                {
                    return RhombusStep::COMPLETE;
                }
                if (scan.help_color == scan.save_color)
                {
                    break;
                }
                g_plot(scan.z, scan.y, static_cast<int>(scan.help_color & 255));
                --scan.z;
                scan.help_real -= scan.step.x;
            }
            if (scan.save_x < scan.z)
            {
                put_hor_line(scan.save_x, scan.y, scan.z, static_cast<int>(scan.save_color & 255));
            }
            else
            {
                g_plot(scan.save_x, scan.y, static_cast<int>(scan.save_color & 255));
            }
            ++scan.y;
            scan.z_value.y += scan.step.y;
            scan.phase = ScanPhase::ROW_START;
            return scan.y >= input.y2 ? RhombusStep::COMPLETE : RhombusStep::INCOMPLETE;
        }
    }
    return RhombusStep::COMPLETE;
}

SynchronousOrbit::RhombusStep SynchronousOrbit::process_rhombus(RhombusFrame &frame)
{
    const RhombusInput &input = frame.input;
    const double c_re1 = input.c_re1;
    const double c_re2 = input.c_re2;
    const double c_im1 = input.c_im1;
    const double c_im2 = input.c_im2;
    const int x1 = input.x1;
    const int x2 = input.x2;
    const int y1 = input.y1;
    const int y2 = input.y2;
    long iter = input.iter;
    m_rhombus_depth = input.depth;
    std::copy(input.zi.begin(), input.zi.end(), std::begin(m_zi));

    // number of iterations before SOI iteration cycle
    long before{};
    int avail{};

    // the variables below need to have local copies for recursive calls
    // center of rectangle
    const double mid_r = (c_re1 + c_re2) / 2;
    const double mid_i = (c_im1 + c_im2) / 2;

    DComplex s[9];

    avail = stack_avail();
    g_soi_min_stack_available = std::min(avail, g_soi_min_stack_available);
    g_max_rhombus_depth = std::max(m_rhombus_depth, g_max_rhombus_depth);
    if (m_rhombus_depth >= 0 && m_rhombus_depth < static_cast<int>(std::size(g_rhombus_stack)))
    {
        g_rhombus_stack[m_rhombus_depth] = avail;
    }

    if (driver_key_pressed())
    {
        return RhombusStep::COMPLETE;
    }
    if (iter > g_max_iterations)
    {
        put_box(x1, y1, x2, y2, 0);
        return RhombusStep::COMPLETE;
    }

    if (y2 - y1 <= SCAN || avail < g_soi_min_stack)
    {
        begin_scan(frame);
        return process_scan(frame);
    }

    std::transform(std::begin(m_zi), std::end(m_zi), std::begin(m_rq), z_sqr);

    m_corner[0].x = 0.75 * c_re1 + 0.25 * c_re2;
    m_corner[0].y = 0.75 * c_im1 + 0.25 * c_im2;
    m_corner[1].x = 0.25 * c_re1 + 0.75 * c_re2;
    m_corner[1].y = 0.25 * c_im1 + 0.75 * c_im2;

    // compute the value of the interpolation polynomial at (x,y)
    const auto get_real{[=](const double re, const double im)
        {
            return interpolate(c_im1, mid_i, c_im2,
                interpolate(c_re1, mid_r, c_re2, m_zi[0].x, m_zi[4].x, m_zi[1].x, re),
                interpolate(c_re1, mid_r, c_re2, m_zi[5].x, m_zi[8].x, m_zi[6].x, re),
                interpolate(c_re1, mid_r, c_re2, m_zi[2].x, m_zi[7].x, m_zi[3].x, re), im);
        }};
    const auto get_imag{[=](const double re, const double im)
        {
            return interpolate(c_re1, mid_r, c_re2,
                interpolate(c_im1, mid_i, c_im2, m_zi[0].y, m_zi[5].y, m_zi[2].y, im),
                interpolate(c_im1, mid_i, c_im2, m_zi[4].y, m_zi[8].y, m_zi[7].y, im),
                interpolate(c_im1, mid_i, c_im2, m_zi[1].y, m_zi[6].y, m_zi[3].y, im), re);
        }};
    m_tz[0].x = get_real(m_corner[0].x, m_corner[0].y);
    m_tz[0].y = get_imag(m_corner[0].x, m_corner[0].y);

    m_tz[1].x = get_real(m_corner[1].x, m_corner[0].y);
    m_tz[1].y = get_imag(m_corner[1].x, m_corner[0].y);

    m_tz[2].x = get_real(m_corner[0].x, m_corner[1].y);
    m_tz[2].y = get_imag(m_corner[0].x, m_corner[1].y);

    m_tz[3].x = get_real(m_corner[1].x, m_corner[1].y);
    m_tz[3].y = get_imag(m_corner[1].x, m_corner[1].y);

    std::transform(std::begin(m_tz), std::end(m_tz), std::begin(m_tq), z_sqr);

    before = iter;

    while (true)
    {
        std::copy(std::begin(m_zi), std::end(m_zi), std::begin(s));

        // iterate key values
        orbit(m_zi[0], m_rq[0], c_re1, c_im1, m_esc[0]);
        /*
              zim1=(zim1+zim1)*zre1+c_im1;
              zre1=rq1-iq1+c_re1;
              rq1=zre1*zre1;
              iq1=zim1*zim1;
        */
        orbit(m_zi[1], m_rq[1], c_re2, c_im1, m_esc[1]);
        /*
              zim2=(zim2+zim2)*zre2+c_im1;
              zre2=rq2-iq2+c_re2;
              rq2=zre2*zre2;
              iq2=zim2*zim2;
        */
        orbit(m_zi[2], m_rq[2], c_re1, c_im2, m_esc[2]);
        /*
              zim3=(zim3+zim3)*zre3+c_im2;
              zre3=rq3-iq3+c_re1;
              rq3=zre3*zre3;
              iq3=zim3*zim3;
        */
        orbit(m_zi[3], m_rq[3], c_re2, c_im2, m_esc[3]);
        /*
              zim4=(zim4+zim4)*zre4+c_im2;
              zre4=rq4-iq4+c_re2;
              rq4=zre4*zre4;
              iq4=zim4*zim4;
        */
        orbit(m_zi[4], m_rq[4], mid_r, c_im1, m_esc[4]);
        /*
              zim5=(zim5+zim5)*zre5+c_im1;
              zre5=rq5-iq5+mid_r;
              rq5=zre5*zre5;
              iq5=zim5*zim5;
        */
        orbit(m_zi[5], m_rq[5], c_re1, mid_i, m_esc[5]);
        /*
              zim6=(zim6+zim6)*zre6+mid_i;
              zre6=rq6-iq6+c_re1;
              rq6=zre6*zre6;
              iq6=zim6*zim6;
        */
        orbit(m_zi[6], m_rq[6], c_re2, mid_i, m_esc[6]);
        /*
              zim7=(zim7+zim7)*zre7+mid_i;
              zre7=rq7-iq7+c_re2;
              rq7=zre7*zre7;
              iq7=zim7*zim7;
        */
        orbit(m_zi[7], m_rq[7], mid_r, c_im2, m_esc[7]);
        /*
              zim8=(zim8+zim8)*zre8+c_im2;
              zre8=rq8-iq8+mid_r;
              rq8=zre8*zre8;
              iq8=zim8*zim8;
        */
        orbit(m_zi[8], m_rq[8], mid_r, mid_i, m_esc[8]);
        /*
              zim9=(zim9+zim9)*zre9+mid_i;
              zre9=rq9-iq9+mid_r;
              rq9=zre9*zre9;
              iq9=zim9*zim9;
        */
        // iterate test point
        orbit(m_tz[0], m_tq[0], m_corner[0].x, m_corner[0].y, m_t_esc[0]);
        /*
              tzi1=(tzi1+tzi1)*tzr1+ci1;
              tzr1=trq1-tiq1+cr1;
              trq1=tzr1*tzr1;
              tiq1=tzi1*tzi1;
        */

        orbit(m_tz[1], m_tq[1], m_corner[1].x, m_corner[0].y, m_t_esc[1]);
        /*
              tzi2=(tzi2+tzi2)*tzr2+ci1;
              tzr2=trq2-tiq2+cr2;
              trq2=tzr2*tzr2;
              tiq2=tzi2*tzi2;
        */
        orbit(m_tz[2], m_tq[2], m_corner[0].x, m_corner[1].y, m_t_esc[2]);
        /*
              tzi3=(tzi3+tzi3)*tzr3+ci2;
              tzr3=trq3-tiq3+cr1;
              trq3=tzr3*tzr3;
              tiq3=tzi3*tzi3;
        */
        orbit(m_tz[3], m_tq[3], m_corner[1].x, m_corner[1].y, m_t_esc[3]);
        /*
              tzi4=(tzi4+tzi4)*tzr4+ci2;
              tzr4=trq4-tiq4+cr2;
              trq4=tzr4*tzr4;
              tiq4=tzi4*tzi4;
        */
        iter++;

        // if one of the iterated values bails out, subdivide
        if (std::find(std::begin(m_esc), std::end(m_esc), true) != std::end(m_esc) ||
            std::find(std::begin(m_t_esc), std::end(m_t_esc), true) != std::end(m_t_esc))
        {
            break;
        }

        /* if maximum number of iterations is reached, the whole rectangle
        can be assumed part of M. This is of course best case behavior
        of SOI, we seldom get there */
        if (iter > g_max_iterations)
        {
            put_box(x1, y1, x2, y2, 0);
            return RhombusStep::COMPLETE;
        }

        /* now for all test points, check whether they exceed the
        allowed tolerance. if so, subdivide */
        m_limit.x = get_real(m_corner[0].x, m_corner[0].y);
        m_limit.x = m_tz[0].x == 0.0 ? m_limit.x == 0.0 ? 1.0 : 1000.0 : m_limit.x / m_tz[0].x;
        if (std::abs(1.0 - m_limit.x) > m_t_width)
        {
            break;
        }

        m_limit.y = get_imag(m_corner[0].x, m_corner[0].y);
        m_limit.y = m_tz[0].y == 0.0 ? m_limit.y == 0.0 ? 1.0 : 1000.0 : m_limit.y / m_tz[0].y;
        if (std::abs(1.0 - m_limit.y) > m_t_width)
        {
            break;
        }

        m_limit.x = get_real(m_corner[1].x, m_corner[0].y);
        m_limit.x = m_tz[1].x == 0.0 ? m_limit.x == 0.0 ? 1.0 : 1000.0 : m_limit.x / m_tz[1].x;
        if (std::abs(1.0 - m_limit.x) > m_t_width)
        {
            break;
        }

        m_limit.y = get_imag(m_corner[1].x, m_corner[0].y);
        m_limit.y = m_tz[1].y == 0.0 ? m_limit.y == 0.0 ? 1.0 : 1000.0 : m_limit.y / m_tz[1].y;
        if (std::abs(1.0 - m_limit.y) > m_t_width)
        {
            break;
        }

        m_limit.x = get_real(m_corner[0].x, m_corner[1].y);
        m_limit.x = m_tz[2].x == 0.0 ? m_limit.x == 0.0 ? 1.0 : 1000.0 : m_limit.x / m_tz[2].x;
        if (std::abs(1.0 - m_limit.x) > m_t_width)
        {
            break;
        }

        m_limit.y = get_imag(m_corner[0].x, m_corner[1].y);
        m_limit.y = m_tz[2].y == 0.0 ? m_limit.y == 0.0 ? 1.0 : 1000.0 : m_limit.y / m_tz[2].y;
        if (std::abs(1.0 - m_limit.y) > m_t_width)
        {
            break;
        }

        m_limit.x = get_real(m_corner[1].x, m_corner[1].y);
        m_limit.x = m_tz[3].x == 0.0 ? m_limit.x == 0.0 ? 1.0 : 1000.0 : m_limit.x / m_tz[3].x;
        if (std::abs(1.0 - m_limit.x) > m_t_width)
        {
            break;
        }

        m_limit.y = get_imag(m_corner[1].x, m_corner[1].y);
        m_limit.y = m_tz[3].y == 0.0 ? m_limit.y == 0.0 ? 1.0 : 1000.0 : m_limit.y / m_tz[3].y;
        if (std::abs(1.0 - m_limit.y) > m_t_width)
        {
            break;
        }
    }

    iter--;

    // this is a little heuristic I tried to improve performance.
    if (iter - before < 10)
    {
        std::copy(std::begin(s), std::end(s), frame.input.zi.begin());
        frame.input.iter = iter;
        begin_scan(frame);
        return process_scan(frame);
    }

    // compute key values for subsequent rectangles

    const double re10 = interpolate(c_re1, mid_r, c_re2, s[0].x, s[4].x, s[1].x, m_corner[0].x);
    const double im10 = interpolate(c_re1, mid_r, c_re2, s[0].y, s[4].y, s[1].y, m_corner[0].x);

    const double re11 = interpolate(c_re1, mid_r, c_re2, s[0].x, s[4].x, s[1].x, m_corner[1].x);
    const double im11 = interpolate(c_re1, mid_r, c_re2, s[0].y, s[4].y, s[1].y, m_corner[1].x);

    const double re20 = interpolate(c_re1, mid_r, c_re2, s[2].x, s[7].x, s[3].x, m_corner[0].x);
    const double im20 = interpolate(c_re1, mid_r, c_re2, s[2].y, s[7].y, s[3].y, m_corner[0].x);

    const double re21 = interpolate(c_re1, mid_r, c_re2, s[2].x, s[7].x, s[3].x, m_corner[1].x);
    const double im21 = interpolate(c_re1, mid_r, c_re2, s[2].y, s[7].y, s[3].y, m_corner[1].x);

    const double re15 = interpolate(c_re1, mid_r, c_re2, s[5].x, s[8].x, s[6].x, m_corner[0].x);
    const double im15 = interpolate(c_re1, mid_r, c_re2, s[5].y, s[8].y, s[6].y, m_corner[0].x);

    const double re16 = interpolate(c_re1, mid_r, c_re2, s[5].x, s[8].x, s[6].x, m_corner[1].x);
    const double im16 = interpolate(c_re1, mid_r, c_re2, s[5].y, s[8].y, s[6].y, m_corner[1].x);

    const double re12 = interpolate(c_im1, mid_i, c_im2, s[0].x, s[5].x, s[2].x, m_corner[0].y);
    const double im12 = interpolate(c_im1, mid_i, c_im2, s[0].y, s[5].y, s[2].y, m_corner[0].y);

    const double re14 = interpolate(c_im1, mid_i, c_im2, s[1].x, s[6].x, s[3].x, m_corner[0].y);
    const double im14 = interpolate(c_im1, mid_i, c_im2, s[1].y, s[6].y, s[3].y, m_corner[0].y);

    const double re17 = interpolate(c_im1, mid_i, c_im2, s[0].x, s[5].x, s[2].x, m_corner[1].y);
    const double im17 = interpolate(c_im1, mid_i, c_im2, s[0].y, s[5].y, s[2].y, m_corner[1].y);

    const double re19 = interpolate(c_im1, mid_i, c_im2, s[1].x, s[6].x, s[3].x, m_corner[1].y);
    const double im19 = interpolate(c_im1, mid_i, c_im2, s[1].y, s[6].y, s[3].y, m_corner[1].y);

    const double re13 = interpolate(c_im1, mid_i, c_im2, s[4].x, s[8].x, s[7].x, m_corner[0].y);
    const double im13 = interpolate(c_im1, mid_i, c_im2, s[4].y, s[8].y, s[7].y, m_corner[0].y);

    const double re18 = interpolate(c_im1, mid_i, c_im2, s[4].x, s[8].x, s[7].x, m_corner[1].y);
    const double im18 = interpolate(c_im1, mid_i, c_im2, s[4].y, s[8].y, s[7].y, m_corner[1].y);

    // compute the value of the interpolation polynomial at (x,y)
    // from saved values before interpolation failed to stay within tolerance
    const auto get_saved_real{[=](const double re, const double im)
        {
            return interpolate(c_im1, mid_i, c_im2, interpolate(c_re1, mid_r, c_re2, s[0].x, s[4].x, s[1].x, re),
                interpolate(c_re1, mid_r, c_re2, s[5].x, s[8].x, s[6].x, re),
                interpolate(c_re1, mid_r, c_re2, s[2].x, s[7].x, s[3].x, re), im);
        }};
    const auto get_saved_imag{[=](const double re, const double im)
        {
            return interpolate(c_re1, mid_r, c_re2, interpolate(c_im1, mid_i, c_im2, s[0].y, s[5].y, s[2].y, im),
                interpolate(c_im1, mid_i, c_im2, s[4].y, s[8].y, s[7].y, im),
                interpolate(c_im1, mid_i, c_im2, s[1].y, s[6].y, s[3].y, im), re);
        }};
    const double re91 = get_saved_real(m_corner[0].x, m_corner[0].y);
    const double im91 = get_saved_imag(m_corner[0].x, m_corner[0].y);
    const double re92 = get_saved_real(m_corner[1].x, m_corner[0].y);
    const double im92 = get_saved_imag(m_corner[1].x, m_corner[0].y);
    const double re93 = get_saved_real(m_corner[0].x, m_corner[1].y);
    const double im93 = get_saved_imag(m_corner[0].x, m_corner[1].y);
    const double re94 = get_saved_real(m_corner[1].x, m_corner[1].y);
    const double im94 = get_saved_imag(m_corner[1].x, m_corner[1].y);

    const int child_depth = m_rhombus_depth + 1;
    frame.children[0] = make_rhombus_input(c_re1, mid_r,                                        //
        c_im1, mid_i,                                                                           //
        x1, (x1 + x2) >> 1, y1, (y1 + y2) >> 1,                                                 //
        s[0], s[4], s[5], s[8],                                                                 //
        DComplex{re10, im10}, DComplex{re12, im12}, DComplex{re13, im13}, DComplex{re15, im15}, //
        DComplex{re91, im91}, iter, child_depth);
    frame.children[1] = make_rhombus_input(mid_r, c_re2,                                        //
        c_im1, mid_i,                                                                           //
        (x1 + x2) >> 1, x2, y1, (y1 + y2) >> 1,                                                 //
        s[4], s[1], s[8], s[6],                                                                 //
        DComplex{re11, im11}, DComplex{re13, im13}, DComplex{re14, im14}, DComplex{re16, im16}, //
        DComplex{re92, im92}, iter, child_depth);
    frame.children[2] = make_rhombus_input(c_re1, mid_r,                                        //
        mid_i, c_im2,                                                                           //
        x1, (x1 + x2) >> 1, (y1 + y2) >> 1, y2,                                                 //
        s[5], s[8], s[2], s[7],                                                                 //
        DComplex{re15, im15}, DComplex{re17, im17}, DComplex{re18, im18}, DComplex{re20, im20}, //
        DComplex{re93, im93}, iter, child_depth);
    frame.children[3] = make_rhombus_input(mid_r, c_re2,                                        //
        mid_i, c_im2,                                                                           //
        (x1 + x2) >> 1, x2, (y1 + y2) >> 1, y2,                                                 //
        s[8], s[6], s[7], s[3],                                                                 //
        DComplex{re16, im16}, DComplex{re18, im18}, DComplex{re19, im19}, DComplex{re21, im21}, //
        DComplex{re94, im94}, iter, child_depth);
    frame.child_index = 0;
    frame.phase = RhombusPhase::CHILDREN;
    return RhombusStep::CHILDREN;
}

void SynchronousOrbit::start()
{
    // cppcheck-suppress unreadVariable
    constexpr double TOLERANCE = 0.1;
    double xx_min_l;
    double xx_max_l;
    double yy_min_l;
    double yy_max_l;
    g_soi_min_stack_available = 30000;
    m_frame_stack.clear();
    g_max_rhombus_depth = 0;
    m_done = false;
    m_started = true;
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
    m_t_width = TOLERANCE / (g_logical_screen.x_dots - 1);
    const double step_x = (xx_max_l - xx_min_l) / g_logical_screen.x_dots;
    const double step_y = (yy_min_l - yy_max_l) / g_logical_screen.y_dots;
    m_equal = step_x < step_y ? step_x : step_y;

    push_rhombus(make_rhombus_input(xx_min_l, xx_max_l,                 //
        yy_max_l, yy_min_l,                                             //
        0, g_logical_screen.x_dots, 0, g_logical_screen.y_dots,         //
        DComplex{xx_min_l, yy_max_l}, DComplex{xx_max_l, yy_max_l},     //
        DComplex{xx_min_l, yy_min_l}, DComplex{xx_max_l, yy_min_l},     //
        DComplex{(xx_max_l + xx_min_l) / 2, yy_max_l},                  //
        DComplex{xx_min_l, (yy_max_l + yy_min_l) / 2},                  //
        DComplex{xx_max_l, (yy_max_l + yy_min_l) / 2},                  //
        DComplex{(xx_max_l + xx_min_l) / 2, yy_min_l},                  //
        DComplex{(xx_min_l + xx_max_l) / 2, (yy_max_l + yy_min_l) / 2}, //
        1, 0));
}

bool SynchronousOrbit::iterate()
{
    if (!m_started)
    {
        start();
    }
    while (!m_done)
    {
        const FrameResult result{process_next_frame()};
        if (result == FrameResult::DONE)
        {
            m_done = true;
            break;
        }
        if (result == FrameResult::YIELDED)
        {
            break;
        }
    }
    return m_done;
}

} // namespace id::engine
