// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "ui/orbit3d.h"

#include "fractals/lorenz.h"
#include "ui/stereo.h"
#include "ui/KeyboardHandler.h"
#include "ui/SoundSession.h"

#include <memory>

using namespace id::engine;
using namespace id::fractals;

namespace id::ui
{

int orbit3d_calc()
{
    SoundSession sound{g_sound, SoundSession::Ownership::BORROWED};
    return orbit3d_calc(sound);
}

int orbit3d_calc(SoundSession &sound)
{
    reset_calc_interrupted();
    KeyboardHandlerPtr main_loop_handler{std::make_shared<MainLoopKeyboardHandler>()};
    ScopedKeyboardHandler main_loop_scope{main_loop_handler};
    setup_stereo_palette();
    Orbit3D o3d;

    while (!o3d.done())
    {
        if (!sound.wait_until_ready() || calc_interrupted())
        {
            sound.cancel();
            return -1;
        }

        o3d.iterate();
    }

    return sound.finish() ? 0 : -1;
}

} // namespace id::ui
