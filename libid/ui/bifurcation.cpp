// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "ui/bifurcation.h"

#include "engine/NonStandardFractal.h"
#include "misc/Driver.h"
#include "ui/diskvid.h"

using namespace id::engine;
using namespace id::misc;

namespace id::ui
{

int bifurcation_type(NonStandardFractal &calculation)
{
    while (!calculation.done())
    {
        calculation.iterate();
        if (!calculation.done())
        {
            display_disk_storage_progress();
            if (driver_key_pressed())
            {
                calculation.suspend();
                return -1;
            }
        }
    }
    return 0;
}

} // namespace id::ui
