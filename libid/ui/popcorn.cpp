// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/popcorn.h"

#include "engine/StandardFractal.h"
#include "fractals/popcorn.h"
#include "ui/KeyboardHandler.h"
#include "ui/standard_orbit_plot.h"

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
    reset_orbit_delay();
    popcorn.resume();

    while (!popcorn.done())
    {
        if (popcorn.standard_fractal().orbit_plot_pending())
        {
            drive_pending_orbit_plot(popcorn.standard_fractal());
        }
        else
        {
            popcorn.iterate();
        }
        if (!popcorn.done() && !popcorn.standard_fractal().orbit_plot_pending() && calc_interrupted())
        {
            popcorn.suspend();
            return -1;
        }
    }
    return 0;
}

} // namespace id::ui
