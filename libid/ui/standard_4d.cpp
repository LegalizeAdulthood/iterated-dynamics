// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/standard_4d.h"

#include "fractals/jb.h"
#include "misc/Driver.h"

int standard_4d_type()
{
    id::fractals::Standard4D s4d;

    while (s4d.iterate())
    {
        if (driver_key_pressed())
        {
            return -1;
        }
    }

    return 0;
}
