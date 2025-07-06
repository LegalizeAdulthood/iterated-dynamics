// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/frothy_basin.h"

#include "fractals/FrothyBasin.h"
#include "ui/check_key.h"

// Froth Fractal type
int froth_type()   // per pixel 1/2/g, called with row & col set
{
    if (check_key())
    {
        return -1;
    }

    const int calc = id::fractals::g_frothy_basin.calc();
    if (id::fractals::g_frothy_basin.keyboard_check())
    {
        if (check_key())
        {
            return -1;
        }
        id::fractals::g_frothy_basin.keyboard_reset();
    }

    return calc;
}

