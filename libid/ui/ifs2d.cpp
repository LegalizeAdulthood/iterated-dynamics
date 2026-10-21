// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "ui/ifs2d.h"

#include "engine/NonStandardFractal.h"
#include "misc/Driver.h"
#include "ui/diskvid.h"

using namespace id::engine;
using namespace id::misc;

namespace id::ui
{

int ifs2d(NonStandardFractal &calculation)
{
    while (!calculation.done())
    {
        if (driver_key_pressed())
        {
            calculation.suspend();
            return -1;
        }

        calculation.iterate();
        display_disk_storage_progress();
    }
    return 0;
}

} // namespace id::ui
