// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "ui/lyapunov.h"

#include "engine/calcfrac.h"
#include "engine/StandardFractal.h"
#include "fractals/lyapunov.h"
#include "misc/Driver.h"
#include "ui/KeyboardHandler.h"
#include "ui/ShowDotController.h"
#include "ui/standard_orbit_plot.h"

#include <memory>

using namespace id::engine;
using namespace id::fractals;
using namespace id::misc;

namespace id::ui
{

namespace
{

class OrbitToggleKeyboardHandler : public KeyboardHandler
{
public:
    bool handle_key(int key) override
    {
        if (g_show_orbit)
        {
            scrub_orbit_display();
        }
        if (key != 'o' && key != 'O')
        {
            return false;
        }
        if (!driver_is_disk())
        {
            g_show_orbit = !g_show_orbit;
        }
        return true;
    }
};

} // namespace

int lyapunov_fractal()
{
    reset_calc_interrupted();
    auto main_loop_handler{std::make_shared<MainLoopKeyboardHandler>()};
    auto orbit_toggle_handler{std::make_shared<OrbitToggleKeyboardHandler>()};
    ScopedKeyboardHandler main_loop_scope{main_loop_handler};
    ScopedKeyboardHandler orbit_toggle_scope{orbit_toggle_handler};

    Lyapunov lyapunov;
    ShowDotController marker;
    lyapunov.resume();

    try
    {
        while (!lyapunov.done())
        {
            if (marker.enabled())
            {
                marker.iterate(lyapunov.calculation());
            }
            else
            {
                lyapunov.iterate();
            }
            if (!lyapunov.done() && calc_interrupted())
            {
                lyapunov.suspend();
                return -1;
            }
        }
    }
    catch (...)
    {
        marker.restore();
        scrub_orbit_display();
        lyapunov.calculation().abort();
        throw;
    }
    return 0;
}

} // namespace id::ui
