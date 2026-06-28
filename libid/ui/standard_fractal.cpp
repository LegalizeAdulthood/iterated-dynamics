// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/standard_fractal.h"

#include "engine/calcfrac.h"
#include "engine/orbit.h"
#include "engine/StandardFractal.h"
#include "misc/Driver.h"
#include "ui/KeyboardHandler.h"

#include <chrono>
#include <memory>
#include <thread>

using namespace id::engine;
using namespace id::misc;

namespace id::ui
{

namespace
{

using Clock = std::chrono::steady_clock;

constexpr auto KEY_POLL_INTERVAL = std::chrono::milliseconds{1};

Clock::time_point s_next_orbit_delay_time{};

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

void reset_orbit_delay()
{
    s_next_orbit_delay_time = {};
}

void wait_for_orbit_delay()
{
    auto now = Clock::now();
    while (now < s_next_orbit_delay_time)
    {
        if (dispatch_pending_keyboard_input())
        {
            break;
        }
        const auto remaining = s_next_orbit_delay_time - now;
        std::this_thread::sleep_for(remaining < KEY_POLL_INTERVAL ? remaining : Clock::duration{KEY_POLL_INTERVAL});
        now = Clock::now();
    }
    s_next_orbit_delay_time = now + std::chrono::microseconds(g_orbit_delay * 100);
}

void drive_pending_orbit_plot(StandardFractal &standard_fractal)
{
    if (!standard_fractal.orbit_plot_pending())
    {
        return;
    }

    OrbitPlot &orbit_plot{standard_fractal.pending_orbit_plot()};
    orbit_plot.iterate_without_delay();
    if (orbit_plot.consume_delay_pending())
    {
        wait_for_orbit_delay();
    }
    if (orbit_plot.done())
    {
        standard_fractal.complete_pending_orbit_plot();
    }
}

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
            drive_pending_orbit_plot(standard_fractal);
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
