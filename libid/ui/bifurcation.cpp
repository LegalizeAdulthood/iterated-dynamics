// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/bifurcation.h"

#include "engine/resume.h"
#include "fractals/Bifurcation.h"
#include "misc/Driver.h"

using namespace id::fractals;
using namespace id::misc;

namespace id::ui
{

int bifurcation_type()
{
    Bifurcation bif;
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

} // namespace id::ui
