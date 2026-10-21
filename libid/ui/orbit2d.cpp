// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
/*
   This file contains two 3-dimensional orbit-type fractal
   generators - IFS and LORENZ3D, along with code to generate
   red/blue 3D images.
*/
#include "ui/orbit2d.h"

#include "engine/resume.h"
#include "fractals/lorenz.h"
#include "ui/diskvid.h"
#include "ui/KeyboardHandler.h"
#include "ui/SoundSession.h"

#include <memory>

using namespace id::engine;
using namespace id::fractals;

namespace id::ui
{

int orbit2d_type()
{
    SoundSession sound{g_sound, SoundSession::Ownership::BORROWED};
    return orbit2d_type(sound);
}

int orbit2d_type(SoundSession &sound)
{
    reset_calc_interrupted();
    KeyboardHandlerPtr main_loop_handler{std::make_shared<MainLoopKeyboardHandler>()};
    ScopedKeyboardHandler main_loop_scope{main_loop_handler};
    Orbit2D o2d;

    if (g_resuming)
    {
        o2d.resume();
    }

    while (!o2d.done())
    {
        if (!sound.wait_until_ready() || calc_interrupted())
        {
            sound.cancel();
            o2d.suspend();
            return -1;
        }

        o2d.iterate();
        display_disk_storage_progress();
    }
    return sound.finish() ? 0 : -1;
}

} // namespace id::ui
