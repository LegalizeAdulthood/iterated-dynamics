// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/OrbitDelayTimer.h"

#include <errno.h>
#include <time.h>

#include <algorithm>
#include <stdexcept>

namespace id::ui
{

namespace
{

constexpr long long NANOSECONDS_PER_SECOND = 1000000000LL;
constexpr long long NANOSECONDS_PER_MICROSECOND = 1000LL;
constexpr unsigned long SPIN_WINDOW_MICROSECONDS = 500UL;
constexpr long long MAX_SLEEP_NANOSECONDS = 1000000LL;

long long s_next_deadline{};

long long timespec_to_nanoseconds(const timespec &value)
{
    return static_cast<long long>(value.tv_sec) * NANOSECONDS_PER_SECOND + value.tv_nsec;
}

long long current_nanoseconds()
{
    timespec now{};
    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0)
    {
        throw std::logic_error{"clock_gettime(CLOCK_MONOTONIC) failed"};
    }
    return timespec_to_nanoseconds(now);
}

long long nanoseconds_for_microseconds(const unsigned long microseconds)
{
    const long long nanoseconds{static_cast<long long>(microseconds) * NANOSECONDS_PER_MICROSECOND};
    return microseconds == 0 || nanoseconds > 0 ? nanoseconds : 1;
}

long long nanoseconds_for_delay_units(const unsigned long delay_100us)
{
    const long long microseconds{
        static_cast<long long>(delay_100us) * static_cast<long long>(ORBIT_DELAY_UNIT_MICROSECONDS)};
    const long long nanoseconds{microseconds * NANOSECONDS_PER_MICROSECOND};
    return delay_100us == 0 || nanoseconds > 0 ? nanoseconds : 1;
}

void sleep_for_nanoseconds(const long long nanoseconds)
{
    timespec requested{};
    requested.tv_sec = nanoseconds / NANOSECONDS_PER_SECOND;
    requested.tv_nsec = nanoseconds % NANOSECONDS_PER_SECOND;

    while (nanosleep(&requested, &requested) != 0 && errno == EINTR)
    {
    }
}

bool input_is_pending(OrbitDelayInputPendingFn &input_pending)
{
    return input_pending();
}

} // namespace

void reset_orbit_delay_timer()
{
    s_next_deadline = 0;
}

bool wait_for_orbit_delay(const unsigned long delay_100us, OrbitDelayInputPendingFn &input_pending)
{
    bool interrupted = false;
    long long now = current_nanoseconds();
    const long long spin_window = nanoseconds_for_microseconds(SPIN_WINDOW_MICROSECONDS);

    if (s_next_deadline == 0)
    {
        s_next_deadline = now;
    }

    while (now < s_next_deadline)
    {
        if (input_is_pending(input_pending))
        {
            interrupted = true;
            break;
        }

        const long long remaining{s_next_deadline - now};
        if (remaining > spin_window)
        {
            sleep_for_nanoseconds(std::min(remaining - spin_window, MAX_SLEEP_NANOSECONDS));
        }
        now = current_nanoseconds();
    }

    s_next_deadline = now + nanoseconds_for_delay_units(delay_100us);
    return interrupted;
}

} // namespace id::ui
