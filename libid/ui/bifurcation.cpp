// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/bifurcation.h"

#include "engine/resume.h"
#include "fractals/Bifurcation.h"
#include "misc/Driver.h"

int bifurcation_type()
{
    id::fractals::Bifurcation bif;
    if (g_resuming)
    {
        bif.resume();
    }
    while (bif.iterate())
    {
        if (driver_key_pressed())
        {
            bif.suspend();
            return -1;
        }
    }
    return 0;
}
