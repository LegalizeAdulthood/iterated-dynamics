// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/ifs3d.h"

#include "fractals/lorenz.h"
#include "misc/Driver.h"

using namespace id::fractals;
using namespace id::misc;

namespace id::ui
{

int ifs3d_calc()
{
    IFS3D ifs;
    while (!ifs.done())
    {
        // keypress bails out
        if (driver_key_pressed())
        {
            return -1;
        }
        ifs.iterate();
    }
    return 0;
}

} // namespace id::ui
