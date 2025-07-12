// SPDX-License-Identifier: GPL-3.0-only
//
#include "io/save_timer.h"

#include "engine/calcfrac.h"
#include "engine/id_data.h"
#include "misc/Driver.h"
#include "ui/framain2.h"
#include "ui/read_ticker.h"

#include <cmath>
#include <ctime>

TimedSave g_resave_flag{};  // tells encoder not to incr filename
int g_save_time_interval{}; // autosave minutes
bool g_started_resaves{};   // but incr on first resave
TimedSave g_timed_save{};   // when doing a timed save

static long s_save_start{}; // base clock ticks
static long s_save_ticks{}; // save after this many ticks

void start_save_timer()
{
    s_save_start = read_ticker();                                        // calc's start time
    s_save_ticks = std::abs(g_save_time_interval) * 60 * CLOCKS_PER_SEC; // ticks/minute
}

void stop_save_timer()
{
    s_save_ticks = 0;
}

static bool save_timer_expired()
{
    const long elapsed = read_ticker() - s_save_start;
    return elapsed >= s_save_ticks;
}

bool auto_save_needed()
{
    if (s_save_ticks == 0)
    {
        return false;
    }

    if (save_timer_expired()) /* time to check */
    {
        /* time to save, check for end of row */
        if (g_finish_row == -1)                           /* end of row */
        {
            if (g_calc_status == CalcStatus::IN_PROGRESS) /* still calculating, check row */
            {
                if (g_passes == Passes::SEQUENTIAL_SCAN || g_passes == Passes::SOLID_GUESS)
                {
                    g_finish_row = g_row;
                }
            }
            else
            {
                g_timed_save = TimedSave::STARTED; /* do the save */
            }
        }
        else if (g_finish_row != g_row)            /* start of next row */
        {
            g_timed_save = TimedSave::STARTED;     /* do the save */
        }
    }

    if (g_timed_save == TimedSave::STARTED)
    {
        return true;
    }
    return false;
}
