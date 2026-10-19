// SPDX-License-Identifier: GPL-3.0-only
//
/*
subroutines which belong primarily to calcfrac and
fractals, i.e. which are non-fractal-specific fractal engine subroutines.
*/
#include "engine/wait_until.h"

#include <chrono>
#include <thread>

namespace id::engine
{

namespace
{

using Clock = std::chrono::steady_clock;

Clock::time_point s_next_wait_time{};

} // namespace

void sleep_ms(const long ms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void sleep_orbit_delay(const unsigned long delay_100us)
{
    std::this_thread::sleep_for(std::chrono::microseconds(delay_100us * 100));
}

void reset_wait_until()
{
    s_next_wait_time = {};
}

/*
 * wait until wait_time 100-usec units from the
 * last call has elapsed.
 */
void wait_until(const unsigned long wait_time_100us)
{
    std::this_thread::sleep_until(s_next_wait_time);
    s_next_wait_time = Clock::now() + std::chrono::microseconds(wait_time_100us * 100);
}

} // namespace id::engine
