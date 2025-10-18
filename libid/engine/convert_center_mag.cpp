// SPDX-License-Identifier: GPL-3.0-only
//
#include "engine/convert_center_mag.h"

#include "engine/convert_corners.h"
#include "engine/fractalb.h"
#include "engine/id_data.h"
#include "engine/ImageRegion.h"
#include "math/biginit.h"
#include "math/sign.h"
#include "math/sqr.h"
#include "misc/debug_flags.h"
#include "misc/id.h"

#include <cmath>

using namespace id::math;
using namespace id::misc;

namespace id::engine
{

// most people "think" in degrees
static double rad_to_deg(const double x)
{
    return x * (180.0 / PI);
}

/*
convert corners to center/mag
Rotation angles indicate how much the IMAGE has been rotated, not the
zoom box.  Same goes for the skew angles
*/
void cvt_center_mag(double &ctr_x, double &ctr_y, LDouble &mag, double &x_mag_factor, double &rot, double &skew)
{
    // simple normal case first
    if (g_image_region.m_3rd.x == g_image_region.m_min.x && g_image_region.m_3rd.y == g_image_region.m_min.y)
    {
        // no rotation or skewing, but stretching is allowed
        const double width = g_image_region.m_max.x - g_image_region.m_min.x;
        const double height = g_image_region.m_max.y - g_image_region.m_min.y;
        ctr_x = (g_image_region.m_min.x + g_image_region.m_max.x)/2.0;
        ctr_y = (g_image_region.m_min.y + g_image_region.m_max.y)/2.0;
        mag  = 2.0/height;
        x_mag_factor =  height / (DEFAULT_ASPECT * width);
        rot = 0.0;
        skew = 0.0;
    }
    else
    {
        // set up triangle ABC, having sides abc
        // side a = bottom, b = left, c = diagonal not containing (x3rd,y3rd)
        double tmp_x1 = g_image_region.m_max.x - g_image_region.m_min.x;
        double tmp_y1 = g_image_region.m_max.y - g_image_region.m_min.y;
        const double c2 = tmp_x1*tmp_x1 + tmp_y1*tmp_y1;

        tmp_x1 = g_image_region.m_max.x - g_image_region.m_3rd.x;
        tmp_y1 = g_image_region.m_min.y - g_image_region.m_3rd.y;
        const double a2 = tmp_x1 * tmp_x1 + tmp_y1 * tmp_y1;
        const double a = std::sqrt(a2);
        rot = -rad_to_deg(std::atan2(tmp_y1, tmp_x1));   // negative for image rotation

        const double tmp_x2 = g_image_region.m_min.x - g_image_region.m_3rd.x;
        const double tmp_y2 = g_image_region.m_max.y - g_image_region.m_3rd.y;
        const double b2 = tmp_x2 * tmp_x2 + tmp_y2 * tmp_y2;
        const double b = std::sqrt(b2);

        const double tmp_a = std::acos((a2+b2-c2)/(2*a*b)); // save tmpa for later use
        skew = 90.0 - rad_to_deg(tmp_a);

        ctr_x = (g_image_region.m_min.x + g_image_region.m_max.x)*0.5;
        ctr_y = (g_image_region.m_min.y + g_image_region.m_max.y) * 0.5;

        const double height = b * std::sin(tmp_a);

        mag  = 2.0/height; // 1/(h/2)
        x_mag_factor = height / (DEFAULT_ASPECT * a);

        // if vector_a cross vector_b is negative
        // then adjust for left-hand coordinate system
        if (tmp_x1*tmp_y2 - tmp_x2*tmp_y1 < 0
            && g_debug_flag != DebugFlags::ALLOW_NEGATIVE_CROSS_PRODUCT)
        {
            skew = -skew;
            x_mag_factor = -x_mag_factor;
            mag = -mag;
        }
    }
    // just to make par file look nicer
    if (mag < 0)
    {
        mag = -mag;
        rot += 180;
    }
#ifndef NDEBUG
    {
        const double t_x_min = g_image_region.m_min.x;
        const double t_x_max = g_image_region.m_max.x;
        const double t_x_3rd = g_image_region.m_3rd.x;
        const double t_y_min = g_image_region.m_min.y;
        const double t_y_max = g_image_region.m_max.y;
        const double t_y_3rd = g_image_region.m_3rd.y;
        cvt_corners(ctr_x, ctr_y, mag, x_mag_factor, rot, skew);
        const double error = sqr(t_x_min - g_image_region.m_min.x) //
            + sqr(t_x_max - g_image_region.m_max.x)                //
            + sqr(t_x_3rd - g_image_region.m_3rd.x)                //
            + sqr(t_y_min - g_image_region.m_min.y)                //
            + sqr(t_y_max - g_image_region.m_max.y)                //
            + sqr(t_y_3rd - g_image_region.m_3rd.y);               //
        if (error > .001)
        {
            show_corners_dbl("cvtcentermag problem");
        }
        g_image_region.m_min.x = t_x_min;
        g_image_region.m_max.x = t_x_max;
        g_image_region.m_3rd.x = t_x_3rd;
        g_image_region.m_min.y = t_y_min;
        g_image_region.m_max.y = t_y_max;
        g_image_region.m_3rd.y = t_y_3rd;
    }
#endif
}

// convert corners to center/mag using bf
void cvt_center_mag_bf(BigFloat ctr_x, BigFloat ctr_y, LDouble &mag, double &x_mag_factor, double &rot, double &skew)
{
    // needs to be LDouble or won't work past 307 (-DBL_MIN_10_EXP) or so digits
    BigStackSaver saved;

    // simple normal case first
    // if (g_image_region.m_3rd.x == g_image_region.m_min.x && g_image_region.m_3rd.y == g_image_region.m_min.y)
    if (!cmp_bf(g_bf_x_3rd, g_bf_x_min) && !cmp_bf(g_bf_y_3rd, g_bf_y_min))
    {
        // no rotation or skewing, but stretching is allowed
        const BigFloat width_bf = alloc_stack(g_bf_length + 2);
        const BigFloat height_bf = alloc_stack(g_bf_length+2);
        // width  = x_max - x_min;
        sub_bf(width_bf, g_bf_x_max, g_bf_x_min);
        const LDouble width = bf_to_float(width_bf);
        // height = y_max - y_min;
        sub_bf(height_bf, g_bf_y_max, g_bf_y_min);
        const LDouble height = bf_to_float(height_bf);
        // *ctr_x = (x_min + x_max)/2;
        add_bf(ctr_x, g_bf_x_min, g_bf_x_max);
        half_a_bf(ctr_x);
        // *ctr_y = (y_min + y_max)/2;
        add_bf(ctr_y, g_bf_y_min, g_bf_y_max);
        half_a_bf(ctr_y);
        mag  = 2/height;
        x_mag_factor = static_cast<double>(height / (DEFAULT_ASPECT * width));
        rot = 0.0;
        skew = 0.0;
    }
    else
    {
        const BigFloat tmp_x_bf = alloc_stack(g_bf_length + 2);
        const BigFloat tmp_y_bf = alloc_stack(g_bf_length+2);

        // set up triangle ABC, having sides abc
        // side a = bottom, b = left, c = diagonal not containing (x3rd,y3rd)
        // IMPORTANT: convert from bf AFTER subtracting

        // tmpx = x_max - x_min;
        sub_bf(tmp_x_bf, g_bf_x_max, g_bf_x_min);
        LDouble tmp_x1 = bf_to_float(tmp_x_bf);
        // tmpy = y_max - y_min;
        sub_bf(tmp_y_bf, g_bf_y_max, g_bf_y_min);
        LDouble tmp_y1 = bf_to_float(tmp_y_bf);
        const LDouble c2 = tmp_x1*tmp_x1 + tmp_y1*tmp_y1;

        // tmpx = x_max - x_3rd;
        sub_bf(tmp_x_bf, g_bf_x_max, g_bf_x_3rd);
        tmp_x1 = bf_to_float(tmp_x_bf);

        // tmpy = y_min - y_3rd;
        sub_bf(tmp_y_bf, g_bf_y_min, g_bf_y_3rd);
        tmp_y1 = bf_to_float(tmp_y_bf);
        const LDouble a2 = tmp_x1 * tmp_x1 + tmp_y1 * tmp_y1;
        const LDouble a = std::sqrt(a2);

        // divide tmpx and tmpy by |tmpx| so that double version of atan2() can be used
        // atan2() only depends on the ratio, this puts it in double's range
        const int sign_x = sign(tmp_x1);
        LDouble tmp_y = 0.0;
        if (sign_x)
        {
            tmp_y = tmp_y1/tmp_x1 * sign_x;    // tmpy = tmpy / |tmpx|
        }
        rot = -rad_to_deg(std::atan2(static_cast<double>(tmp_y), sign_x));   // negative for image rotation

        // tmpx = x_min - x_3rd;
        sub_bf(tmp_x_bf, g_bf_x_min, g_bf_x_3rd);
        const LDouble tmp_x2 = bf_to_float(tmp_x_bf);
        // tmpy = y_max - y_3rd;
        sub_bf(tmp_y_bf, g_bf_y_max, g_bf_y_3rd);
        const LDouble tmp_y2 = bf_to_float(tmp_y_bf);
        const LDouble b2 = tmp_x2 * tmp_x2 + tmp_y2 * tmp_y2;
        const LDouble b = std::sqrt(b2);

        const double tmp_a = std::acos(static_cast<double>((a2 + b2 - c2) / (2 * a * b))); // save tmpa for later use
        skew = 90 - rad_to_deg(tmp_a);

        // these are the only two variables that must use big precision
        // *ctr_x = (x_min + x_max)/2;
        add_bf(ctr_x, g_bf_x_min, g_bf_x_max);
        half_a_bf(ctr_x);
        // *ctr_y = (y_min + y_max)/2;
        add_bf(ctr_y, g_bf_y_min, g_bf_y_max);
        half_a_bf(ctr_y);

        const LDouble height = b * std::sin(tmp_a);
        mag  = 2/height; // 1/(h/2)
        x_mag_factor = static_cast<double>(height / (DEFAULT_ASPECT * a));

        // if vector_a cross vector_b is negative
        // then adjust for left-hand coordinate system
        if (tmp_x1*tmp_y2 - tmp_x2*tmp_y1 < 0
            && g_debug_flag != DebugFlags::ALLOW_NEGATIVE_CROSS_PRODUCT)
        {
            skew = -skew;
            x_mag_factor = -x_mag_factor;
            mag = -mag;
        }
    }
    if (mag < 0)
    {
        mag = -mag;
        rot += 180;
    }
}

} // namespace id::engine
