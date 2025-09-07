// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/diffusion.h"

#include "fractals/Diffusion.h"
#include "ui/check_key.h"

using namespace id::fractals;

namespace id::ui
{

int diffusion_type()
{
    Diffusion d;

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

} // namespace id::ui
