// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "ui/lsystem.h"

#include "ui/LSystemController.h"

using namespace id::engine;

namespace id::ui
{

// TODO: removed later
// Slice 52.12 replaces the synchronous entry with outer UI stepping.
int lsystem_type(NonStandardFractal &calculation)
{
    LSystemController controller{calculation};
    controller.start();
    while (!controller.done())
    {
        controller.iterate();
    }
    return controller.result();
}

} // namespace id::ui
