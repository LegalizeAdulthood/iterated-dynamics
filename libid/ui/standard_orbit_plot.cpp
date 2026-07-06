// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/standard_orbit_plot.h"

#include "engine/calcfrac.h"
#include "engine/OrbitPlot.h"
#include "ui/KeyboardHandler.h"
#include "ui/OrbitDelayTimer.h"

#include <chrono>

using namespace id::engine;

namespace id::ui
{

namespace
{

constexpr unsigned long DISPLAY_REFRESH_HZ = 72;
constexpr unsigned long MICROSECONDS_PER_SECOND = 1000000;
constexpr unsigned long MIN_DISPLAY_DELAY_UNITS =
    (MICROSECONDS_PER_SECOND + DISPLAY_REFRESH_HZ * ORBIT_DELAY_UNIT_MICROSECONDS - 1) /
    (DISPLAY_REFRESH_HZ * ORBIT_DELAY_UNIT_MICROSECONDS);

using Clock = std::chrono::steady_clock;

constexpr std::chrono::microseconds INPUT_POLL_INTERVAL{MIN_DISPLAY_DELAY_UNITS * ORBIT_DELAY_UNIT_MICROSECONDS};

Clock::time_point s_next_input_poll_time{};

bool dispatch_orbit_input_if_due()
{
    const Clock::time_point now{Clock::now()};
    if (s_next_input_poll_time != Clock::time_point{} && now < s_next_input_poll_time)
    {
        return false;
    }

    s_next_input_poll_time = now + INPUT_POLL_INTERVAL;
    return dispatch_pending_keyboard_input();
}

} // namespace

unsigned long display_orbit_delay_units(const unsigned long delay_100us)
{
    return delay_100us < MIN_DISPLAY_DELAY_UNITS ? 0 : delay_100us;
}

void pace_orbit_display()
{
    if (g_orbit_delay <= 0)
    {
        return;
    }

    const unsigned long delay_100us{display_orbit_delay_units(static_cast<unsigned long>(g_orbit_delay))};
    if (delay_100us != 0)
    {
        wait_for_orbit_delay(delay_100us, dispatch_orbit_input_if_due);
    }
    else
    {
        dispatch_orbit_input_if_due();
    }
}

bool drive_orbit_plot(OrbitPlot &orbit_plot)
{
    orbit_plot.iterate();
    if (orbit_plot.flush_delay())
    {
        pace_orbit_display();
    }
    return orbit_plot.done();
}

void reset_orbit_delay()
{
    reset_orbit_delay_timer();
    s_next_input_poll_time = {};
}

} // namespace id::ui
