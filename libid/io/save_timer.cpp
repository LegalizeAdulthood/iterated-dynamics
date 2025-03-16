#include "io/save_timer.h"

#include "engine/calcfrac.h"
#include "engine/id_data.h"
#include "misc/Driver.h"
#include "ui/framain2.h"
#include "ui/id_keys.h"
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

bool auto_save_needed()
{
    if (s_save_ticks == 0)
    {
        return false;
    }

    /* get current timer value */
    long elapsed = read_ticker() - s_save_start;
    if (elapsed >= s_save_ticks) /* time to check */
    {
        /* time to save, check for end of row */
        if (g_finish_row == -1)                           /* end of row */
        {
            if (g_calc_status == CalcStatus::IN_PROGRESS) /* still calculating, check row */
            {
                if (g_got_status == StatusValues::ONE_OR_TWO_PASS ||
                    g_got_status == StatusValues::SOLID_GUESS)
                {
                    g_finish_row = g_row;
                }
            }
            else
            {
                g_timed_save = TimedSave::STARTED; /* do the save */
            }
        }
        else                                       /* not end of row */
        {
            if (g_finish_row != g_row)             /* start of next row */
            {
                g_timed_save = TimedSave::STARTED; /* do the save */
            }
        }
    }

    if (g_timed_save == TimedSave::STARTED)
    {
        driver_unget_key(ID_KEY_FAKE_AUTOSAVE);
        return true;
    }
    return false;
}
