// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "ui/dynamic2d.h"

#include "engine/NonStandardFractal.h"
#include "ui/diskvid.h"
#include "ui/KeyboardHandler.h"
#include "ui/SoundSession.h"

#include <memory>

using namespace id::engine;

namespace id::ui
{

int dynamic2d_type(NonStandardFractal &calculation, SoundSession &sound)
{
    reset_calc_interrupted();
    KeyboardHandlerPtr main_loop_handler{std::make_shared<MainLoopKeyboardHandler>()};
    ScopedKeyboardHandler main_loop_scope{main_loop_handler};
    while (!calculation.done())
    {
        if (!sound.wait_until_ready() || calc_interrupted())
        {
            sound.cancel();
            calculation.suspend();
            return -1;
        }

        calculation.iterate();
        display_disk_storage_progress();
    }

    return sound.finish() ? 0 : -1;
}

} // namespace id::ui
