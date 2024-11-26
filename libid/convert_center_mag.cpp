// SPDX-License-Identifier: GPL-3.0-only
//
#include "convert_center_mag.h"

#include "biginit.h"
#include "convert_corners.h"
#include "debug_flags.h"
#include "fractalb.h"
#include "id.h"
#include "id_data.h"
#include "sign.h"
#include "sqr.h"

#include <cmath>

 // most people "think" in degrees
inline double rad_to_deg(double x)
{
    return x * (180.0 / PI);
}

/*
convert corners to center/mag
Rotation angles indicate how much the IMAGE has been rotated, not the
zoom box.  Same goes for the Skew angles
*/
void cvt_center_mag(double &ctr_x, double &ctr_y, LDBL &mag, double &x_mag_factor, double &rot, double &skew)
{
    // simple normal case first
    if (g_x_3rd == g_x_min && g_y_3rd == g_y_min)
    {
        // no rotation or skewing, but stretching is allowed
        const double width = g_x_max - g_x_min;
        const double height = g_y_max - g_y_min;
        ctr_x = (g_x_min + g_x_max)/2.0;
        ctr_y = (g_y_min + g_y_max)/2.0;
        mag  = 2.0/height;
        x_mag_factor =  height / (DEFAULT_ASPECT * width);
        rot = 0.0;
        skew = 0.0;
    }
    else
    {
        // set up triangle ABC, having sides abc
        // side a = bottom, b = left, c = diagonal not containing (x3rd,y3rd)
        double tmpx1 = g_x_max - g_x_min;
        double tmpy1 = g_y_max - g_y_min;
        const double c2 = tmpx1*tmpx1 + tmpy1*tmpy1;

        tmpx1 = g_x_max - g_x_3rd;
        tmpy1 = g_y_min - g_y_3rd;
        const double a2 = tmpx1 * tmpx1 + tmpy1 * tmpy1;
        const double a = std::sqrt(a2);
        rot = -rad_to_deg(std::atan2(tmpy1, tmpx1));   // negative for image rotation

        const double tmpx2 = g_x_min - g_x_3rd;
        const double tmpy2 = g_y_max - g_y_3rd;
        const double b2 = tmpx2 * tmpx2 + tmpy2 * tmpy2;
        const double b = std::sqrt(b2);

        const double tmpa = std::acos((a2+b2-c2)/(2*a*b)); // save tmpa for later use
        skew = 90.0 - rad_to_deg(tmpa);

        ctr_x = (g_x_min + g_x_max)*0.5;
        ctr_y = (g_y_min + g_y_max) * 0.5;

        const double height = b * std::sin(tmpa);

        mag  = 2.0/height; // 1/(h/2)
        x_mag_factor = height / (DEFAULT_ASPECT * a);

        // if vector_a cross vector_b is negative
        // then adjust for left-hand coordinate system
        if (tmpx1*tmpy2 - tmpx2*tmpy1 < 0
            && g_debug_flag != debug_flags::allow_negative_cross_product)
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
        const double txmin = g_x_min;
        const double txmax = g_x_max;
        const double tx3rd = g_x_3rd;
        const double tymin = g_y_min;
        const double tymax = g_y_max;
        const double ty3rd = g_y_3rd;
        cvtcorners(ctr_x, ctr_y, mag, x_mag_factor, rot, skew);
        const double error = sqr(txmin - g_x_min) //
            + sqr(txmax - g_x_max)                //
            + sqr(tx3rd - g_x_3rd)                //
            + sqr(tymin - g_y_min)                //
            + sqr(tymax - g_y_max)                //
            + sqr(ty3rd - g_y_3rd);               //
        if (error > .001)
        {
            show_corners_dbl("cvtcentermag problem");
        }
        g_x_min = txmin;
        g_x_max = txmax;
        g_x_3rd = tx3rd;
        g_y_min = tymin;
        g_y_max = tymax;
        g_y_3rd = ty3rd;
    }
#endif
}

// convert corners to center/mag using bf
void cvt_center_mag_bf(bf_t ctr_x, bf_t ctr_y, LDBL &mag, double &x_mag_factor, double &rot, double &skew)
{
    // needs to be LDBL or won't work past 307 (-DBL_MIN_10_EXP) or so digits
    BigStackSaver saved;

    // simple normal case first
    // if (g_x_3rd == g_x_min && g_y_3rd == g_y_min)
    if (!cmp_bf(g_bf_x_3rd, g_bf_x_min) && !cmp_bf(g_bf_y_3rd, g_bf_y_min))
    {
        // no rotation or skewing, but stretching is allowed
        const bf_t width_bf = alloc_stack(g_bf_length + 2);
        const bf_t height_bf = alloc_stack(g_bf_length+2);
        // width  = g_x_max - g_x_min;
        sub_bf(width_bf, g_bf_x_max, g_bf_x_min);
        const LDBL width = bftofloat(width_bf);
        // height = g_y_max - g_y_min;
        sub_bf(height_bf, g_bf_y_max, g_bf_y_min);
        const LDBL height = bftofloat(height_bf);
        // *ctr_x = (g_x_min + g_x_max)/2;
        add_bf(ctr_x, g_bf_x_min, g_bf_x_max);
        half_a_bf(ctr_x);
        // *ctr_y = (g_y_min + g_y_max)/2;
        add_bf(ctr_y, g_bf_y_min, g_bf_y_max);
        half_a_bf(ctr_y);
        mag  = 2/height;
        x_mag_factor = (double)(height / (DEFAULT_ASPECT * width));
        rot = 0.0;
        skew = 0.0;
    }
    else
    {
        const bf_t tmp_x_bf = alloc_stack(g_bf_length + 2);
        const bf_t tmp_y_bf = alloc_stack(g_bf_length+2);

        // set up triangle ABC, having sides abc
        // side a = bottom, b = left, c = diagonal not containing (x3rd,y3rd)
        // IMPORTANT: convert from bf AFTER subtracting

        // tmpx = g_x_max - g_x_min;
        sub_bf(tmp_x_bf, g_bf_x_max, g_bf_x_min);
        LDBL tmpx1 = bftofloat(tmp_x_bf);
        // tmpy = g_y_max - g_y_min;
        sub_bf(tmp_y_bf, g_bf_y_max, g_bf_y_min);
        LDBL tmpy1 = bftofloat(tmp_y_bf);
        const LDBL c2 = tmpx1*tmpx1 + tmpy1*tmpy1;

        // tmpx = g_x_max - g_x_3rd;
        sub_bf(tmp_x_bf, g_bf_x_max, g_bf_x_3rd);
        tmpx1 = bftofloat(tmp_x_bf);

        // tmpy = g_y_min - g_y_3rd;
        sub_bf(tmp_y_bf, g_bf_y_min, g_bf_y_3rd);
        tmpy1 = bftofloat(tmp_y_bf);
        const LDBL a2 = tmpx1 * tmpx1 + tmpy1 * tmpy1;
        const LDBL a = sqrtl(a2);

        // divide tmpx and tmpy by |tmpx| so that double version of atan2() can be used
        // atan2() only depends on the ratio, this puts it in double's range
        const int sign_x = sign(tmpx1);
        LDBL tmpy = 0.0;
        if (sign_x)
        {
            tmpy = tmpy1/tmpx1 * sign_x;    // tmpy = tmpy / |tmpx|
        }
        rot = (double)(-rad_to_deg(std::atan2((double)tmpy, sign_x)));   // negative for image rotation

        // tmpx = g_x_min - g_x_3rd;
        sub_bf(tmp_x_bf, g_bf_x_min, g_bf_x_3rd);
        const LDBL tmpx2 = bftofloat(tmp_x_bf);
        // tmpy = g_y_max - g_y_3rd;
        sub_bf(tmp_y_bf, g_bf_y_max, g_bf_y_3rd);
        const LDBL tmpy2 = bftofloat(tmp_y_bf);
        const LDBL b2 = tmpx2 * tmpx2 + tmpy2 * tmpy2;
        const LDBL b = sqrtl(b2);

        const double tmpa = std::acos((double)((a2+b2-c2)/(2*a*b))); // save tmpa for later use
        skew = 90 - rad_to_deg(tmpa);

        // these are the only two variables that must use big precision
        // *ctr_x = (g_x_min + g_x_max)/2;
        add_bf(ctr_x, g_bf_x_min, g_bf_x_max);
        half_a_bf(ctr_x);
        // *ctr_y = (g_y_min + g_y_max)/2;
        add_bf(ctr_y, g_bf_y_min, g_bf_y_max);
        half_a_bf(ctr_y);

        const LDBL height = b * std::sin(tmpa);
        mag  = 2/height; // 1/(h/2)
        x_mag_factor = (double)(height / (DEFAULT_ASPECT * a));

        // if vector_a cross vector_b is negative
        // then adjust for left-hand coordinate system
        if (tmpx1*tmpy2 - tmpx2*tmpy1 < 0
            && g_debug_flag != debug_flags::allow_negative_cross_product)
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
