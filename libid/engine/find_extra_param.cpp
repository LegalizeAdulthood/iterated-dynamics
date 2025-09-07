// SPDX-License-Identifier: GPL-3.0-only
//
#include "engine/find_extra_param.h"

#include "fractals/fractalp.h"

using namespace id::fractals;

namespace id
{

int find_extra_param(FractalType type)
{
    int ret = -1;
    if (bit_set(get_fractal_specific(type)->flags, FractalFlags::MORE))
    {
        FractalType current_type;
        int i = -1;
        while ((current_type = g_more_fractal_params[++i].type) != type && current_type != FractalType::NO_FRACTAL)
        {
        }
        if (current_type == type)
        {
            ret = i;
        }
    }
    return ret;
}

} // namespace id
