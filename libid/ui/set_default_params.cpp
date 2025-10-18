// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/set_default_params.h"

#include "engine/calc_frac_init.h"
#include "engine/cmdfiles.h"
#include "engine/find_extra_param.h"
#include "engine/id_data.h"
#include "engine/ImageRegion.h"
#include "fractals/fractalp.h"
#include "fractals/fractype.h"
#include "math/round_float_double.h"
#include "misc/debug_flags.h"
#include "ui/zoom.h"

using namespace id::engine;
using namespace id::fractals;
using namespace id::math;
using namespace id::misc;

namespace id::ui
{

namespace
{

struct SavedParams
{
    double x_min{};
    double x_max{};
    double y_min{};
    double y_max{};
    double x_3rd{};
    double y_3rd{};              // selected screen corners
    double params[MAX_PARAMS]{}; // parameters
    CalcStatus calc_status{};
    // TODO: save bignum params?
};

} // namespace

static SavedParams s_saved_params{};

void set_default_params()
{
    g_image_region.m_min.x = g_cur_fractal_specific->x_min;
    g_image_region.m_max.x = g_cur_fractal_specific->x_max;
    g_image_region.m_min.y = g_cur_fractal_specific->y_min;
    g_image_region.m_max.y = g_cur_fractal_specific->y_max;
    g_image_region.m_3rd.x = g_image_region.m_min.x;
    g_image_region.m_3rd.y = g_image_region.m_min.y;

    if (g_view_crop && g_final_aspect_ratio != g_screen_aspect)
    {
        aspect_ratio_crop(g_screen_aspect, g_final_aspect_ratio);
    }
    for (int i = 0; i < 4; i++)
    {
        g_params[i] = g_cur_fractal_specific->params[i];
        if (g_fractal_type != FractalType::CELLULAR        //
            && g_fractal_type != FractalType::FROTHY_BASIN //
            && g_fractal_type != FractalType::ANT)
        {
            round_float_double(&g_params[i]); // don't round cellular, frothybasin or ant
        }
    }
    int extra = find_extra_param(g_fractal_type);
    if (extra > -1)
    {
        for (int i = 0; i < MAX_PARAMS - 4; i++)
        {
            g_params[i + 4] = g_more_fractal_params[extra].params[i];
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

void save_params()
{
    s_saved_params.x_min = g_image_region.m_min.x;
    s_saved_params.x_max = g_image_region.m_max.x;
    s_saved_params.y_min = g_image_region.m_min.y;
    s_saved_params.y_max = g_image_region.m_max.y;
    s_saved_params.x_3rd = g_image_region.m_3rd.x;
    s_saved_params.y_3rd = g_image_region.m_3rd.y; // selected screen corners
    for (int i = 0; i < MAX_PARAMS; ++i)
    {
        s_saved_params.params[i] = g_params[i]; // parameters}
    }
    s_saved_params.calc_status = g_calc_status;
}

void restore_params()
{
    g_image_region.m_min.x = s_saved_params.x_min;
    g_image_region.m_max.x = s_saved_params.x_max;
    g_image_region.m_min.y = s_saved_params.y_min;
    g_image_region.m_max.y = s_saved_params.y_max;
    g_image_region.m_3rd.x = s_saved_params.x_3rd;
    g_image_region.m_3rd.y = s_saved_params.y_3rd; // selected screen corners
    for (int i = 0; i < MAX_PARAMS; ++i)
    {
        g_params[i] = s_saved_params.params[i]; // parameters}
    }
    g_calc_status = s_saved_params.calc_status;
}

} // namespace id::ui
