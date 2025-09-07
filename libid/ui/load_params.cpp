// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/load_params.h"

#include "engine/find_extra_param.h"
#include "engine/id_data.h"
#include "fractals/fractalp.h"

using namespace id;
using namespace id::fractals;

void load_params(FractalType type)
{
    for (int i = 0; i < 4; ++i)
    {
        g_params[i] = get_fractal_specific(type)->params[i];
    }
    if (const int extra = find_extra_param(type); extra > -1)
    {
        for (int i = 0; i < MAX_PARAMS - 4; i++)
        {
            g_params[i + 4] = g_more_fractal_params[extra].params[i];
        }
    }
}
