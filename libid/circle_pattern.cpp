// SPDX-License-Identifier: GPL-3.0-only
//
#include "circle_pattern.h"

#include "calcfrac.h"
#include "fractals.h"
#include "id_data.h"

int circle_fp_fractal()
{
    long i;
    i = (long)(g_params[0]*(g_temp_sqr_x+g_temp_sqr_y));
    g_color_iter = i%g_colors;
    return 1;
}
