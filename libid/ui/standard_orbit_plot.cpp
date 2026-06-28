// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/standard_orbit_plot.h"

#include "engine/calcfrac.h"
#include "engine/orbit.h"
#include "engine/StandardFractal.h"
#include "ui/KeyboardHandler.h"

#include <chrono>
#include <thread>

using namespace id::engine;

namespace id::ui
{

namespace
{

using Clock = std::chrono::steady_clock;

constexpr auto KEY_POLL_INTERVAL = std::chrono::milliseconds{1};

Clock::time_point s_next_orbit_delay_time{};

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

} // namespace

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

void reset_orbit_delay()
{
    s_next_orbit_delay_time = {};
}

} // namespace id::ui
