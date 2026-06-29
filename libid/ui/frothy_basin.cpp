// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/frothy_basin.h"

#include "engine/calcfrac.h"
#include "engine/orbit.h"
#include "fractals/FrothyBasin.h"
#include "misc/Driver.h"
#include "ui/KeyboardHandler.h"
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

void drive_pending_orbit_point()
{
    const auto point{g_frothy_basin.pending_overlay_orbit_point()};
    orbit_plot().reset_overlay(point.x, point.y);
    if (drive_orbit_plot(orbit_plot()))
    {
        g_frothy_basin.complete_pending_overlay_orbit_point();
    }
}

} // namespace

int froth_type()
{
    reset_calc_interrupted();
    reset_orbit_delay();
    auto main_loop_handler{std::make_shared<MainLoopKeyboardHandler>()};
    auto orbit_toggle_handler{std::make_shared<OrbitToggleKeyboardHandler>()};
    ScopedKeyboardHandler main_loop_scope{main_loop_handler};
    ScopedKeyboardHandler orbit_toggle_scope{orbit_toggle_handler};

    if (!g_frothy_basin.per_image())
    {
        return -1;
    }

    g_frothy_basin.resume();
    while (!g_frothy_basin.done())
    {
        if (g_frothy_basin.overlay_orbit_point_pending())
        {
            drive_pending_orbit_point();
        }
        else if (g_frothy_basin.overlay_scrub_pending())
        {
            scrub_orbit();
            g_frothy_basin.complete_overlay_scrub();
        }
        else
        {
            g_frothy_basin.iterate();
        }
        if (!g_frothy_basin.done() && !g_frothy_basin.overlay_orbit_point_pending() &&
            !g_frothy_basin.overlay_scrub_pending() && calc_interrupted())
        {
            g_frothy_basin.suspend();
            return -1;
        }
    }

    return 0;
}

} // namespace id::ui
