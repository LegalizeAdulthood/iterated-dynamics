// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "ui/dynamic2d.h"

#include "ui/OrbitController.h"

using namespace id::engine;

namespace id::ui
{

// TODO: removed later
// Slice 52.12 replaces the synchronous entry with outer UI stepping.
int dynamic2d_type(NonStandardFractal &calculation, SoundSession &sound)
{
    OrbitController controller{calculation, sound};
    controller.start();
    while (!controller.done())
    {
        controller.iterate();
    }
    return controller.result();
}

} // namespace id::ui
