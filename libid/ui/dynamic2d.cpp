// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/dynamic2d.h"

#include "engine/resume.h"
#include "fractals/lorenz.h"
#include "misc/Driver.h"

int dynamic2d_type()
{
    id::fractals::Dynamic2D d2d;

    if (g_resuming)
    {
        d2d.resume();
    }

    while (!d2d.done())
    {
        if (driver_key_pressed())
        {
            d2d.suspend();
            return -1;
        }

        d2d.iterate();
    }

    return 0;
}
