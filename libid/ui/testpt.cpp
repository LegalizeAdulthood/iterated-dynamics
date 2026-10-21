// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "ui/testpt.h"

#include "engine/NonStandardFractal.h"
#include "misc/Driver.h"
#include "ui/diskvid.h"

using namespace id::engine;
using namespace id::misc;

namespace id::ui
{

// standalone engine for "test"
int test_type(NonStandardFractal &calculation)
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
