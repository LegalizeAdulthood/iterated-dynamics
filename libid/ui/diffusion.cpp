// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/diffusion.h"

#include "engine/check_key.h"
#include "fractals/Diffusion.h"

int diffusion_type()
{
    id::fractals::Diffusion d;

    while (d.iterate())
    {
        if (check_key())
        {
            d.suspend();
            return 1;
        }
    }

    return 0;
}
