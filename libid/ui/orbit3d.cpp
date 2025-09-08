// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/orbit3d.h"

#include "fractals/lorenz.h"
#include "misc/Driver.h"

using namespace id::fractals;
using namespace id::misc;

namespace id::ui
{

int orbit3d_calc()
{
    Orbit3D o3d;

    while (!o3d.done())
    {
        if (driver_key_pressed())
        {
            return -1;
        }

        o3d.iterate();
    }

    return 0;
}

} // namespace id::ui
