// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/standard_fractal.h"

#include "engine/calcfrac.h"
#include "engine/orbit.h"
#include "engine/StandardFractal.h"
#include "misc/Driver.h"
#include "ui/KeyboardHandler.h"
#include "ui/standard_orbit_plot.h"

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

} // namespace

int standard_fractal()
{
    reset_calc_interrupted();
    reset_orbit_delay();
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
            if (drive_orbit_plot(standard_fractal.pending_orbit_plot()))
            {
                standard_fractal.complete_pending_orbit_plot();
            }
        }
        else
        {
            standard_fractal.iterate();
        }
        if (!standard_fractal.done() && !standard_fractal.orbit_plot_pending() && calc_interrupted())
        {
            standard_fractal.suspend();
            return -1;
        }
    }
    return 0;
}

} // namespace id::ui
