// SPDX-License-Identifier: GPL-3.0-only
//
/*
   This file contains two 3-dimensional orbit-type fractal
   generators - IFS and LORENZ3D, along with code to generate
   red/blue 3D images.
*/
#include "ui/orbit2d.h"

#include "engine/resume.h"
#include "fractals/lorenz.h"
#include "misc/Driver.h"

using namespace id;

int orbit2d_type()
{
    id::fractals::Orbit2D o2d;

    if (g_resuming)
    {
        o2d.resume();
    }

    while (!o2d.done())
    {
        if (driver_key_pressed())
        {
            o2d.suspend();
            return -1;
        }

        o2d.iterate();
    }
    return 0;
}
