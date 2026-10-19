// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/dynamic2d.h"

#include "engine/resume.h"
#include "fractals/lorenz.h"
#include "ui/KeyboardHandler.h"
#include "ui/SoundSession.h"

#include <memory>

using namespace id::engine;
using namespace id::fractals;

namespace id::ui
{

int dynamic2d_type()
{
    SoundSession sound{DriverSoundOutput::Source::ORBIT};
    return dynamic2d_type(sound);
}

int dynamic2d_type(SoundSession &sound)
{
    reset_calc_interrupted();
    KeyboardHandlerPtr main_loop_handler{std::make_shared<MainLoopKeyboardHandler>()};
    ScopedKeyboardHandler main_loop_scope{main_loop_handler};
    Dynamic2D d2d;

    if (g_resuming)
    {
        d2d.resume();
    }

    while (!d2d.done())
    {
        if (!sound.wait_until_ready() || calc_interrupted())
        {
            sound.cancel();
            d2d.suspend();
            return -1;
        }

        d2d.iterate();
    }

    return sound.finish() ? 0 : -1;
}

} // namespace id::ui
