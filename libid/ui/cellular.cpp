// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
//****************** standalone engine for "cellular" *******************

#include "ui/cellular.h"

#include "ui/CellularController.h"

using namespace id::engine;

namespace id::ui
{

// TODO: removed later
// Slice 52.12 replaces the synchronous entry with outer UI stepping.
int cellular_type(NonStandardFractal &calculation)
{
    CellularController controller{calculation};
    controller.start();
    while (!controller.done())
    {
        controller.iterate();
    }
    return controller.result();
}

} // namespace id::ui
