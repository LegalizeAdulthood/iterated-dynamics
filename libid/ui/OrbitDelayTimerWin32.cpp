// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/OrbitDelayTimer.h"

#include <Windows.h>

#include <stdexcept>

namespace id::ui
{

namespace
{

constexpr double MICROSECONDS_PER_SECOND = 1000000.0;
constexpr unsigned long SPIN_WINDOW_MICROSECONDS = 500UL;
constexpr DWORD MESSAGE_WAIT_MILLISECONDS = 1;

LARGE_INTEGER s_frequency{};
long long s_next_deadline{};
bool s_clock_initialized{};

void initialize_clock()
{
    if (!s_clock_initialized)
    {
        if (!QueryPerformanceFrequency(&s_frequency))
        {
            throw std::logic_error{"QueryPerformanceFrequency failed"};
        }
        s_clock_initialized = true;
    }
}

long long current_ticks()
{
    initialize_clock();
    LARGE_INTEGER now{};
    QueryPerformanceCounter(&now);
    return now.QuadPart;
}

long long ticks_for_microseconds(const unsigned long microseconds)
{
    initialize_clock();
    const long long ticks = static_cast<long long>(
        static_cast<double>(microseconds) * static_cast<double>(s_frequency.QuadPart) / MICROSECONDS_PER_SECOND);
    return microseconds == 0 || ticks > 0 ? ticks : 1;
}

long long ticks_for_delay_units(const unsigned long delay_100us)
{
    initialize_clock();
    const double microseconds{static_cast<double>(delay_100us) * static_cast<double>(ORBIT_DELAY_UNIT_MICROSECONDS)};
    const long long ticks =
        static_cast<long long>(microseconds * static_cast<double>(s_frequency.QuadPart) / MICROSECONDS_PER_SECOND);
    return delay_100us == 0 || ticks > 0 ? ticks : 1;
}

bool input_is_pending(OrbitDelayInputPendingFn &input_pending)
{
    return input_pending();
}

} // namespace

void reset_orbit_delay_timer()
{
    initialize_clock();
    s_next_deadline = 0;
}

bool wait_for_orbit_delay(const unsigned long delay_100us, OrbitDelayInputPendingFn &input_pending)
{
    initialize_clock();

    bool interrupted = false;
    long long now = current_ticks();
    const long long spin_window = ticks_for_microseconds(SPIN_WINDOW_MICROSECONDS);

    if (s_next_deadline == 0)
    {
        s_next_deadline = now;
    }

    while (now < s_next_deadline)
    {
        const long long remaining{s_next_deadline - now};
        if (remaining <= spin_window)
        {
            do
            {
                YieldProcessor();
                now = current_ticks();
            } while (now < s_next_deadline);
            break;
        }

        if (input_is_pending(input_pending))
        {
            interrupted = true;
            break;
        }

        MsgWaitForMultipleObjectsEx(0, nullptr, MESSAGE_WAIT_MILLISECONDS, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
        now = current_ticks();
    }

    s_next_deadline = now + ticks_for_delay_units(delay_100us);
    return interrupted;
}

} // namespace id::ui
