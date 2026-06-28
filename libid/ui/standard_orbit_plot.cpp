// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/standard_orbit_plot.h"

#include "engine/calcfrac.h"
#include "engine/orbit.h"
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

bool drive_orbit_plot(OrbitPlot &orbit_plot)
{
    orbit_plot.iterate_without_delay();
    if (orbit_plot.consume_delay_pending())
    {
        wait_for_orbit_delay();
    }
    return orbit_plot.done();
}

void reset_orbit_delay()
{
    s_next_orbit_delay_time = {};
}

} // namespace id::ui
