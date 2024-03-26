/*
FRACSUBR.C contains subroutines which belong primarily to CALCFRAC.C and
FRACTALS.C, i.e. which are non-fractal-specific fractal engine subroutines.
*/
#include "port.h"
#include "prototyp.h"

#include "wait_until.h"

#include "drivers.h"

#include <algorithm>
#include <iterator>
#include <sys/timeb.h>

// This is the only place where these are referenced from the OS layer.
// TODO: replace with <chrono> clock.
using uclock_t = unsigned long;
uclock_t usec_clock();
void restart_uclock();

void sleepms(long ms)
{
    uclock_t       now = usec_clock();
    const uclock_t next_time = now + ms * 100;
    while (now < next_time)
    {
        if (driver_key_pressed())
        {
            break;
        }
        now = usec_clock();
    }
}

/*
 * wait until wait_time microseconds from the
 * last call has elapsed.
 */
#define MAX_INDEX 2
static uclock_t s_next_time[MAX_INDEX];
void wait_until(int index, unsigned long wait_time)
{
    uclock_t now;
    while ((now = usec_clock()) < s_next_time[index])
    {
        if (driver_key_pressed())
        {
            break;
        }
    }
    s_next_time[index] = now + wait_time * 100; // wait until this time next call
}

void reset_clock()
{
    restart_uclock();
    std::fill(std::begin(s_next_time), std::end(s_next_time), 0);
}
