// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "ui/frothy_basin.h"

#include "engine/SoundOutput.h"
#include "ui/FrothyBasinController.h"
#include "ui/SoundSession.h"

using namespace id::engine;

namespace id::ui
{

// TODO: removed later
// Slice 52.12 moves sound-session ownership to the outer UI context.
int froth_type(NonStandardFractal &calculation)
{
    SoundSession sound{g_sound, SoundSession::Ownership::BORROWED};
    return froth_type(calculation, sound);
}

// TODO: removed later
// Slice 52.12 replaces the synchronous entry with outer UI stepping.
int froth_type(NonStandardFractal &calculation, SoundSession &sound)
{
    FrothyBasinController controller{calculation, sound};
    controller.start();
    while (!controller.done())
    {
        controller.iterate();
    }
    return controller.result();
}

} // namespace id::ui
