// SPDX-License-Identifier: GPL-3.0-only
//
#include "find_extra_param.h"

#include "fractalp.h"

int find_extra_param(FractalType type)
{
    int ret = -1;
    if (bit_set(g_fractal_specific[+type].flags, fractal_flags::MORE))
    {
        FractalType curtyp;
        int i = -1;
        while ((curtyp = g_more_fractal_params[++i].type) != type && curtyp != FractalType::NOFRACTAL);
        if (curtyp == type)
        {
            ret = i;
        }
    }
    return ret;
}
