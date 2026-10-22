// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
/*
   This file contains two 3-dimensional orbit-type fractal
   generators - IFS and LORENZ3D, along with code to generate
   red/blue 3D images.
*/
#include "ui/orbit2d.h"

#include "ui/OrbitController.h"

using namespace id::engine;

namespace id::ui
{

// TODO: removed later
// Slice 52.12 replaces the synchronous entry with outer UI stepping.
int orbit2d_type(NonStandardFractal &calculation, SoundSession &sound)
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
