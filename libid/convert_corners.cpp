// SPDX-License-Identifier: GPL-3.0-only
//
#include "convert_corners.h"

#include "biginit.h"
#include "id.h"
#include "id_data.h"

#include <cmath>

// most people "think" in degrees
inline double deg_to_rad(double x)
{
    return x * (PI / 180.0);
}

// convert center/mag to corners
void cvt_corners(double ctr_x, double ctr_y, LDBL mag, double x_mag_factor, double rot, double skew)
{
    if (x_mag_factor == 0.0)
    {
        x_mag_factor = 1.0;
    }

    // half height, width
    const double h = (double) (1 / mag);
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
    const double tanskew = std::tan(deg_to_rad(skew));
    g_x_min = -w + h*tanskew;
    g_x_max =  w - h*tanskew;
    g_x_3rd = -w - h*tanskew;
    g_y_max = h;
    g_y_min = -h;
    g_y_3rd = g_y_min;

    // rotate coord system and then translate it
    rot = deg_to_rad(rot);
    const double sinrot = std::sin(rot);
    const double cosrot = std::cos(rot);

    // top left
    double x = g_x_min * cosrot + g_y_max * sinrot;
    double y = -g_x_min * sinrot + g_y_max * cosrot;
    g_x_min = x + ctr_x;
    g_y_max = y + ctr_y;

    // bottom right
    x = g_x_max * cosrot + g_y_min *  sinrot;
    y = -g_x_max * sinrot + g_y_min *  cosrot;
    g_x_max = x + ctr_x;
    g_y_min = y + ctr_y;

    // bottom left
    x = g_x_3rd * cosrot + g_y_3rd *  sinrot;
    y = -g_x_3rd * sinrot + g_y_3rd *  cosrot;
    g_x_3rd = x + ctr_x;
    g_y_3rd = y + ctr_y;
}

// convert center/mag to corners using bf
void cvt_corners_bf(bf_t ctr_x, bf_t ctr_y, LDBL mag, double x_mag_factor, double rot, double skew)
{
    const int saved = save_stack();
    bf_t bfh = alloc_stack(g_bf_length + 2);
    bf_t bfw = alloc_stack(g_bf_length + 2);

    if (x_mag_factor == 0.0)
    {
        x_mag_factor = 1.0;
    }

    // half height, width
    const LDBL h = 1 / mag;
    float_to_bf(bfh, h);
    const LDBL w = h / (DEFAULT_ASPECT * x_mag_factor);
    float_to_bf(bfw, w);

    if (rot == 0.0 && skew == 0.0)
    {
        // simple, faster case
        // xx3rd = xxmin = Xctr - w;
        sub_bf(g_bf_x_min, ctr_x, bfw);
        copy_bf(g_bf_x_3rd, g_bf_x_min);
        // xxmax = Xctr + w;
        add_bf(g_bf_x_max, ctr_x, bfw);
        // yy3rd = yymin = Yctr - h;
        sub_bf(g_bf_y_min, ctr_y, bfh);
        copy_bf(g_bf_y_3rd, g_bf_y_min);
        // yymax = Yctr + h;
        add_bf(g_bf_y_max, ctr_y, bfh);
        restore_stack(saved);
        return;
    }

    bf_t g_bf_tmp = alloc_stack(g_bf_length + 2);
    // in unrotated, untranslated coordinate system
    const double tanskew = std::tan(deg_to_rad(skew));
    const LDBL xmin = -w + h * tanskew;
    const LDBL xmax = w - h * tanskew;
    const LDBL x3rd = -w - h * tanskew;
    const LDBL ymax = h;
    const LDBL ymin = -h;
    const LDBL y3rd = ymin;

    // rotate coord system and then translate it
    rot = deg_to_rad(rot);
    const double sinrot = std::sin(rot);
    const double cosrot = std::cos(rot);

    // top left
    LDBL x = xmin * cosrot + ymax * sinrot;
    LDBL y = -xmin * sinrot + ymax * cosrot;
    // xxmin = x + Xctr;
    float_to_bf(g_bf_tmp, x);
    add_bf(g_bf_x_min, g_bf_tmp, ctr_x);
    // yymax = y + Yctr;
    float_to_bf(g_bf_tmp, y);
    add_bf(g_bf_y_max, g_bf_tmp, ctr_y);

    // bottom right
    x =  xmax * cosrot + ymin *  sinrot;
    y = -xmax * sinrot + ymin *  cosrot;
    // xxmax = x + Xctr;
    float_to_bf(g_bf_tmp, x);
    add_bf(g_bf_x_max, g_bf_tmp, ctr_x);
    // yymin = y + Yctr;
    float_to_bf(g_bf_tmp, y);
    add_bf(g_bf_y_min, g_bf_tmp, ctr_y);

    // bottom left
    x =  x3rd * cosrot + y3rd *  sinrot;
    y = -x3rd * sinrot + y3rd *  cosrot;
    // xx3rd = x + Xctr;
    float_to_bf(g_bf_tmp, x);
    add_bf(g_bf_x_3rd, g_bf_tmp, ctr_x);
    // yy3rd = y + Yctr;
    float_to_bf(g_bf_tmp, y);
    add_bf(g_bf_y_3rd, g_bf_tmp, ctr_y);

    restore_stack(saved);
}
