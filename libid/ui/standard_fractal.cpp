// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/standard_fractal.h"

#include "engine/calcfrac.h"
#include "engine/orbit.h"
#include "engine/StandardFractal.h"
#include "misc/Driver.h"
#include "ui/KeyboardHandler.h"

#include <memory>

using namespace id::engine;
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
            scrub_orbit();
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

void drive_pending_orbit_plot(StandardFractal &standard_fractal)
{
    if (!standard_fractal.orbit_plot_pending())
    {
        return;
    }

    OrbitPlot &orbit_plot{standard_fractal.pending_orbit_plot()};
    orbit_plot.iterate();
    if (orbit_plot.done())
    {
        standard_fractal.complete_pending_orbit_plot();
    }
}

} // namespace

int standard_fractal()
{
    reset_calc_interrupted();
    auto main_loop_handler{std::make_shared<MainLoopKeyboardHandler>()};
    auto orbit_toggle_handler{std::make_shared<OrbitToggleKeyboardHandler>()};
    ScopedKeyboardHandler main_loop_scope{main_loop_handler};
    ScopedKeyboardHandler orbit_toggle_scope{orbit_toggle_handler};

    StandardFractal standard_fractal;
    standard_fractal.resume();

    while (!standard_fractal.done())
    {
        if (standard_fractal.orbit_plot_pending())
        {
            drive_pending_orbit_plot(standard_fractal);
            continue;
        }
        standard_fractal.iterate();
        if (standard_fractal.orbit_plot_pending())
        {
            continue;
        }
        if (!standard_fractal.done() && calc_interrupted())
        {
            standard_fractal.suspend();
            return -1;
        }
    }
    return 0;
}

} // namespace id::ui
