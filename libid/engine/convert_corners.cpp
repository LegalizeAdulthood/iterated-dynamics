// SPDX-License-Identifier: GPL-3.0-only
//
#include "engine/convert_corners.h"

#include "engine/id_data.h"
#include "math/biginit.h"
#include "misc/id.h"

#include <cmath>

using namespace id::math;

namespace id::engine
{

// most people "think" in degrees
static double deg_to_rad(double x)
{
    return x * (PI / 180.0);
}

// convert center/mag to corners
void cvt_corners(double ctr_x, double ctr_y, LDouble mag, double x_mag_factor, double rot, double skew)
{
    if (x_mag_factor == 0.0)
    {
        x_mag_factor = 1.0;
    }

    // half height, width
    const double h = static_cast<double>(1.0 / mag);
    const double w = h / (DEFAULT_ASPECT * x_mag_factor);

    if (rot == 0.0 && skew == 0.0)
    {
        // simple, faster case
        g_x_min = ctr_x - w;
        g_x_3rd = g_x_min;
        g_x_max = ctr_x + w;
        g_y_min = ctr_y - h;
        g_y_3rd = g_y_min;
        g_y_max = ctr_y + h;
        return;
    }

    // in unrotated, untranslated coordinate system
    const double tan_skew = std::tan(deg_to_rad(skew));
    g_x_min = -w + h*tan_skew;
    g_x_max =  w - h*tan_skew;
    g_x_3rd = -w - h*tan_skew;
    g_y_max = h;
    g_y_min = -h;
    g_y_3rd = g_y_min;

    // rotate coord system and then translate it
    rot = deg_to_rad(rot);
    const double sin_rot = std::sin(rot);
    const double cos_rot = std::cos(rot);

    // top left
    double x = g_x_min * cos_rot + g_y_max * sin_rot;
    double y = -g_x_min * sin_rot + g_y_max * cos_rot;
    g_x_min = x + ctr_x;
    g_y_max = y + ctr_y;

    // bottom right
    x = g_x_max * cos_rot + g_y_min *  sin_rot;
    y = -g_x_max * sin_rot + g_y_min *  cos_rot;
    g_x_max = x + ctr_x;
    g_y_min = y + ctr_y;

    // bottom left
    x = g_x_3rd * cos_rot + g_y_3rd *  sin_rot;
    y = -g_x_3rd * sin_rot + g_y_3rd *  cos_rot;
    g_x_3rd = x + ctr_x;
    g_y_3rd = y + ctr_y;
}

// convert center/mag to corners using bf
void cvt_corners_bf(BigFloat ctr_x, BigFloat ctr_y, LDouble mag, double x_mag_factor, double rot, double skew)
{
    int saved = save_stack();
    BigFloat bfh = alloc_stack(g_bf_length + 2);
    BigFloat bfw = alloc_stack(g_bf_length + 2);

    if (x_mag_factor == 0.0)
    {
        x_mag_factor = 1.0;
    }

    // half height, width
    const LDouble h = 1 / mag;
    float_to_bf(bfh, h);
    const LDouble w = h / (DEFAULT_ASPECT * x_mag_factor);
    float_to_bf(bfw, w);

    if (rot == 0.0 && skew == 0.0)
    {
        // simple, faster case
        // xx3rd = xxmin = x_ctr - w;
        sub_bf(g_bf_x_min, ctr_x, bfw);
        copy_bf(g_bf_x_3rd, g_bf_x_min);
        // xxmax = x_ctr + w;
        add_bf(g_bf_x_max, ctr_x, bfw);
        // yy3rd = yymin = y_ctr - h;
        sub_bf(g_bf_y_min, ctr_y, bfh);
        copy_bf(g_bf_y_3rd, g_bf_y_min);
        // yymax = y_ctr + h;
        add_bf(g_bf_y_max, ctr_y, bfh);
        restore_stack(saved);
        return;
    }

    BigFloat bf_tmp = alloc_stack(g_bf_length + 2);
    // in unrotated, untranslated coordinate system
    const double tan_skew = std::tan(deg_to_rad(skew));
    const LDouble x_min = -w + h * tan_skew;
    const LDouble x_max = w - h * tan_skew;
    const LDouble x_3rd = -w - h * tan_skew;
    const LDouble y_max = h;
    const LDouble y_min = -h;
    const LDouble y_3rd = y_min;

    // rotate coord system and then translate it
    rot = deg_to_rad(rot);
    const double sin_rot = std::sin(rot);
    const double cos_rot = std::cos(rot);

    // top left
    LDouble x = x_min * cos_rot + y_max * sin_rot;
    LDouble y = -x_min * sin_rot + y_max * cos_rot;
    // xxmin = x + x_ctr;
    float_to_bf(bf_tmp, x);
    add_bf(g_bf_x_min, bf_tmp, ctr_x);
    // yymax = y + y_ctr;
    float_to_bf(bf_tmp, y);
    add_bf(g_bf_y_max, bf_tmp, ctr_y);

    // bottom right
    x =  x_max * cos_rot + y_min *  sin_rot;
    y = -x_max * sin_rot + y_min *  cos_rot;
    // xxmax = x + x_ctr;
    float_to_bf(bf_tmp, x);
    add_bf(g_bf_x_max, bf_tmp, ctr_x);
    // yymin = y + y_ctr;
    float_to_bf(bf_tmp, y);
    add_bf(g_bf_y_min, bf_tmp, ctr_y);

    // bottom left
    x =  x_3rd * cos_rot + y_3rd *  sin_rot;
    y = -x_3rd * sin_rot + y_3rd *  cos_rot;
    // xx3rd = x + x_ctr;
    float_to_bf(bf_tmp, x);
    add_bf(g_bf_x_3rd, bf_tmp, ctr_x);
    // yy3rd = y + y_ctr;
    float_to_bf(bf_tmp, y);
    add_bf(g_bf_y_3rd, bf_tmp, ctr_y);

    restore_stack(saved);
}

} // namespace id::engine
