// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <ui/save_timer.h>

#include <engine/calcfrac.h>
#include <io/save_timer.h>
#include <ui/read_ticker.h>

#include <cstdlib>
#include <ctime>

using namespace id::engine;
using namespace id::io;

namespace id::ui
{

namespace
{

SaveTimer s_save_timer;

} // namespace

void SaveTimer::start(const int interval_minutes, const long now)
{
    m_save_start = now;
    m_save_ticks = std::abs(interval_minutes) * 60 * CLOCKS_PER_SEC;
    m_finish_row = -1;
}

void SaveTimer::stop()
{
    m_save_start = 0;
    m_save_ticks = 0;
    m_finish_row = -1;
}

bool SaveTimer::running() const
{
    return m_save_ticks != 0;
}

bool SaveTimer::auto_save_needed(const long now)
{
    if (!running())
    {
        return false;
    }

    if (now - m_save_start >= m_save_ticks)
    {
        if (m_finish_row == -1)
        {
            if (g_calc_status == CalcStatus::IN_PROGRESS)
            {
                if (g_passes == Passes::SEQUENTIAL_SCAN || g_passes == Passes::SOLID_GUESS)
                {
                    m_finish_row = g_row;
                }
            }
            else
            {
                g_timed_save = TimedSave::STARTED;
            }
        }
        else if (m_finish_row != g_row)
        {
            g_timed_save = TimedSave::STARTED;
        }
    }

    return g_timed_save == TimedSave::STARTED;
}

void start_save_timer()
{
    s_save_timer.start(g_save_time_interval, read_ticker());
}

void stop_save_timer()
{
    s_save_timer.stop();
}

bool auto_save_needed()
{
    return s_save_timer.running() && s_save_timer.auto_save_needed(read_ticker());
}

} // namespace id::ui
