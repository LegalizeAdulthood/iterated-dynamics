// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "ui/standard_fractal.h"

#include "engine/SoundOutput.h"
#include "engine/StandardFractal.h"
#include "ui/SoundSession.h"
#include "ui/StandardFractalController.h"

using namespace id::engine;

namespace id::ui
{

// TODO: removed later
// Slice 52.12 replaces the synchronous compatibility entry with outer UI stepping.
int standard_fractal()
{
    SoundSession sound{g_sound, SoundSession::Ownership::BORROWED};
    return standard_fractal(sound);
}

// TODO: removed later
// Slice 52.12 makes the outer render context own this calculation and controller.
int standard_fractal(SoundSession &sound)
{
    StandardFractal calculation;
    StandardFractalController controller{calculation, sound};
    controller.start();
    while (!controller.done())
    {
        controller.iterate();
    }
    return controller.result();
}

} // namespace id::ui
