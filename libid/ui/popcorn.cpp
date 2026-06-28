// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/popcorn.h"

#include "fractals/popcorn.h"
#include "ui/KeyboardHandler.h"

#include <memory>

using namespace id::fractals;

namespace id::ui
{

int popcorn_type()
{
    reset_calc_interrupted();
    auto main_loop_handler{std::make_shared<MainLoopKeyboardHandler>()};
    ScopedKeyboardHandler main_loop_scope{main_loop_handler};

    Popcorn popcorn;
    popcorn.resume();

    while (!popcorn.done())
    {
        popcorn.iterate();
        if (!popcorn.done() && calc_interrupted())
        {
            popcorn.suspend();
            return -1;
        }
    }
    return 0;
}

} // namespace id::ui
