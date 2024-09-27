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
void cvtcorners(double Xctr, double Yctr, LDBL Magnification, double Xmagfactor, double Rotation, double Skew)
{
    if (Xmagfactor == 0.0)
    {
        Xmagfactor = 1.0;
    }

    // half height, width
    const double h = (double) (1 / Magnification);
    const double w = h / (DEFAULT_ASPECT * Xmagfactor);

    if (Rotation == 0.0 && Skew == 0.0)
    {
        // simple, faster case
        g_x_min = Xctr - w;
        g_x_3rd = g_x_min;
        g_x_max = Xctr + w;
        g_y_min = Yctr - h;
        g_y_3rd = g_y_min;
        g_y_max = Yctr + h;
        return;
    }

    // in unrotated, untranslated coordinate system
    const double tanskew = std::tan(deg_to_rad(Skew));
    g_x_min = -w + h*tanskew;
    g_x_max =  w - h*tanskew;
    g_x_3rd = -w - h*tanskew;
    g_y_max = h;
    g_y_min = -h;
    g_y_3rd = g_y_min;

    // rotate coord system and then translate it
    Rotation = deg_to_rad(Rotation);
    const double sinrot = std::sin(Rotation);
    const double cosrot = std::cos(Rotation);

    // top left
    double x = g_x_min * cosrot + g_y_max * sinrot;
    double y = -g_x_min * sinrot + g_y_max * cosrot;
    g_x_min = x + Xctr;
    g_y_max = y + Yctr;

    // bottom right
    x = g_x_max * cosrot + g_y_min *  sinrot;
    y = -g_x_max * sinrot + g_y_min *  cosrot;
    g_x_max = x + Xctr;
    g_y_min = y + Yctr;

    // bottom left
    x = g_x_3rd * cosrot + g_y_3rd *  sinrot;
    y = -g_x_3rd * sinrot + g_y_3rd *  cosrot;
    g_x_3rd = x + Xctr;
    g_y_3rd = y + Yctr;
}

// convert center/mag to corners using bf
void cvtcornersbf(bf_t Xctr, bf_t Yctr, LDBL Magnification, double Xmagfactor, double Rotation, double Skew)
{
    const int saved = save_stack();
    bf_t bfh = alloc_stack(g_bf_length + 2);
    bf_t bfw = alloc_stack(g_bf_length + 2);

    if (Xmagfactor == 0.0)
    {
        Xmagfactor = 1.0;
    }

    // half height, width
    const LDBL h = 1 / Magnification;
    floattobf(bfh, h);
    const LDBL w = h / (DEFAULT_ASPECT * Xmagfactor);
    floattobf(bfw, w);

    if (Rotation == 0.0 && Skew == 0.0)
    {
        // simple, faster case
        // xx3rd = xxmin = Xctr - w;
        sub_bf(g_bf_x_min, Xctr, bfw);
        copy_bf(g_bf_x_3rd, g_bf_x_min);
        // xxmax = Xctr + w;
        add_bf(g_bf_x_max, Xctr, bfw);
        // yy3rd = yymin = Yctr - h;
        sub_bf(g_bf_y_min, Yctr, bfh);
        copy_bf(g_bf_y_3rd, g_bf_y_min);
        // yymax = Yctr + h;
        add_bf(g_bf_y_max, Yctr, bfh);
        restore_stack(saved);
        return;
    }

    bf_t g_bf_tmp = alloc_stack(g_bf_length + 2);
    // in unrotated, untranslated coordinate system
    const double tanskew = std::tan(deg_to_rad(Skew));
    const LDBL xmin = -w + h * tanskew;
    const LDBL xmax = w - h * tanskew;
    const LDBL x3rd = -w - h * tanskew;
    const LDBL ymax = h;
    const LDBL ymin = -h;
    const LDBL y3rd = ymin;

    // rotate coord system and then translate it
    Rotation = deg_to_rad(Rotation);
    const double sinrot = std::sin(Rotation);
    const double cosrot = std::cos(Rotation);

    // top left
    LDBL x = xmin * cosrot + ymax * sinrot;
    LDBL y = -xmin * sinrot + ymax * cosrot;
    // xxmin = x + Xctr;
    floattobf(g_bf_tmp, x);
    add_bf(g_bf_x_min, g_bf_tmp, Xctr);
    // yymax = y + Yctr;
    floattobf(g_bf_tmp, y);
    add_bf(g_bf_y_max, g_bf_tmp, Yctr);

    // bottom right
    x =  xmax * cosrot + ymin *  sinrot;
    y = -xmax * sinrot + ymin *  cosrot;
    // xxmax = x + Xctr;
    floattobf(g_bf_tmp, x);
    add_bf(g_bf_x_max, g_bf_tmp, Xctr);
    // yymin = y + Yctr;
    floattobf(g_bf_tmp, y);
    add_bf(g_bf_y_min, g_bf_tmp, Yctr);

    // bottom left
    x =  x3rd * cosrot + y3rd *  sinrot;
    y = -x3rd * sinrot + y3rd *  cosrot;
    // xx3rd = x + Xctr;
    floattobf(g_bf_tmp, x);
    add_bf(g_bf_x_3rd, g_bf_tmp, Xctr);
    // yy3rd = y + Yctr;
    floattobf(g_bf_tmp, y);
    add_bf(g_bf_y_3rd, g_bf_tmp, Yctr);

    restore_stack(saved);
}
