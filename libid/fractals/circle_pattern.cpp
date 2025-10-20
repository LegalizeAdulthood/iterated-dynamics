// SPDX-License-Identifier: GPL-3.0-only
//
#include "fractals/circle_pattern.h"

#include "engine/calcfrac.h"
#include "engine/fractals.h"
#include "engine/VideoInfo.h"

using namespace id::engine;

namespace id::fractals
{

int circle_orbit()
{
    const long i = static_cast<long>(g_params[0] * (g_temp_sqr_x + g_temp_sqr_y));
    g_color_iter = i % g_colors;
    return 1;
}

} // namespace id::fractals
