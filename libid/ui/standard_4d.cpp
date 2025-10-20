// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/standard_4d.h"

#include "fractals/julibrot.h"
#include "misc/Driver.h"

using namespace id::fractals;
using namespace id::misc;

namespace id::ui
{

int standard_4d_type()
{
    Standard4D s4d;

    while (s4d.iterate())
    {
        if (driver_key_pressed())
        {
            return -1;
        }
    }

    return 0;
}

} // namespace id::ui
