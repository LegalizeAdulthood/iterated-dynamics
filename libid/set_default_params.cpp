// SPDX-License-Identifier: GPL-3.0-only
//
#include "set_default_params.h"

#include "debug_flags.h"
#include "engine/calc_frac_init.h"
#include "engine/find_extra_param.h"
#include "engine/id_data.h"
#include "fractals/fractalp.h"
#include "fractals/fractype.h"
#include "math/round_float_double.h"
#include "ui/cmdfiles.h"
#include "ui/zoom.h"

void set_default_params()
{
    g_x_min = g_cur_fractal_specific->x_min;
    g_x_max = g_cur_fractal_specific->x_max;
    g_y_min = g_cur_fractal_specific->y_min;
    g_y_max = g_cur_fractal_specific->y_max;
    g_x_3rd = g_x_min;
    g_y_3rd = g_y_min;

    if (g_view_crop && g_final_aspect_ratio != g_screen_aspect)
    {
        aspect_ratio_crop(g_screen_aspect, g_final_aspect_ratio);
    }
    for (int i = 0; i < 4; i++)
    {
        g_params[i] = g_cur_fractal_specific->params[i];
        if (g_fractal_type != FractalType::CELLULAR
            && g_fractal_type != FractalType::FROTH
            && g_fractal_type != FractalType::FROTH_FP
            && g_fractal_type != FractalType::ANT)
        {
            round_float_double(&g_params[i]); // don't round cellular, frothybasin or ant
        }
    }
    int extra = find_extra_param(g_fractal_type);
    if (extra > -1)
    {
        for (int i = 0; i < MAX_PARAMS-4; i++)
        {
            g_params[i+4] = g_more_fractal_params[extra].params[i];
        }
    }
    if (g_debug_flag != DebugFlags::FORCE_ARBITRARY_PRECISION_MATH)
    {
        g_bf_math = BFMathType::NONE;
    }
    else if (g_bf_math != BFMathType::NONE)
    {
        fractal_float_to_bf();
    }
}
