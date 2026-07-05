// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/OrbitDelayTimer.h"

#include <errno.h>
#include <mach/mach_time.h>
#include <time.h>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <stdexcept>

namespace id::ui
{

namespace
{

constexpr long long NANOSECONDS_PER_SECOND = 1000000000LL;
constexpr long long NANOSECONDS_PER_MICROSECOND = 1000LL;
constexpr unsigned long SPIN_WINDOW_MICROSECONDS = 500UL;
constexpr long long MAX_SLEEP_NANOSECONDS = 1000000LL;

std::uint64_t s_timebase_numer{};
std::uint64_t s_timebase_denom{};
std::uint64_t s_next_deadline{};
bool s_timebase_initialized{};

void initialize_timebase()
{
    if (!s_timebase_initialized)
    {
        mach_timebase_info_data_t timebase{};
        if (mach_timebase_info(&timebase) != 0 || timebase.denom == 0)
        {
            throw std::logic_error{"mach_timebase_info failed"};
        }
        s_timebase_numer = timebase.numer;
        s_timebase_denom = timebase.denom;
        s_timebase_initialized = true;
    }
}

std::uint64_t current_ticks()
{
    initialize_timebase();
    return mach_absolute_time();
}

std::uint64_t ticks_for_nanoseconds(const long long nanoseconds)
{
    initialize_timebase();
    const double ticks{static_cast<double>(nanoseconds) * static_cast<double>(s_timebase_denom) /
        static_cast<double>(s_timebase_numer)};
    return nanoseconds == 0 || ticks >= 1.0 ? static_cast<std::uint64_t>(ticks) : 1ULL;
}

std::uint64_t ticks_for_microseconds(const unsigned long microseconds)
{
    const long long nanoseconds{static_cast<long long>(microseconds) * NANOSECONDS_PER_MICROSECOND};
    return ticks_for_nanoseconds(nanoseconds);
}

std::uint64_t ticks_for_delay_units(const unsigned long delay_100us)
{
    const long long microseconds{
        static_cast<long long>(delay_100us) * static_cast<long long>(ORBIT_DELAY_UNIT_MICROSECONDS)};
    const long long nanoseconds{microseconds * NANOSECONDS_PER_MICROSECOND};
    return ticks_for_nanoseconds(nanoseconds);
}

long long nanoseconds_for_ticks(const std::uint64_t ticks)
{
    initialize_timebase();
    const double nanoseconds{
        static_cast<double>(ticks) * static_cast<double>(s_timebase_numer) / static_cast<double>(s_timebase_denom)};
    const double max_nanoseconds{static_cast<double>(std::numeric_limits<long long>::max())};
    if (nanoseconds >= max_nanoseconds)
    {
        return std::numeric_limits<long long>::max();
    }
    return ticks == 0 || nanoseconds >= 1.0 ? static_cast<long long>(nanoseconds) : 1;
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
    std::uint64_t now = current_ticks();
    const std::uint64_t spin_window{ticks_for_microseconds(SPIN_WINDOW_MICROSECONDS)};

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

        const std::uint64_t remaining{s_next_deadline - now};
        if (remaining > spin_window)
        {
            const long long sleep_nanoseconds{
                std::min(nanoseconds_for_ticks(remaining - spin_window), MAX_SLEEP_NANOSECONDS)};
            sleep_for_nanoseconds(sleep_nanoseconds);
        }
        now = current_ticks();
    }

    s_next_deadline = now + ticks_for_delay_units(delay_100us);
    return interrupted;
}

} // namespace id::ui
