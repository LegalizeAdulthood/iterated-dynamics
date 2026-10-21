// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "ui/ifs2d.h"

#include "fractals/lorenz.h"
#include "misc/Driver.h"
#include "ui/diskvid.h"

using namespace id::fractals;
using namespace id::misc;

namespace id::ui
{

int ifs2d()
{
    IFS2D ifs;

    while (!ifs.done())
    {
        if (driver_key_pressed())
        {
            return -1;
        }

        ifs.iterate();
        display_disk_storage_progress();
    }
    return 0;
}

} // namespace id::ui
