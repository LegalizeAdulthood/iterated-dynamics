// SPDX-License-Identifier: GPL-3.0-only
//
#include "misc/usec_clock.h"

#include <cassert>

// tenths of millisecond timewr routine
// static struct timeval tv_start;

void restart_uclock()
{
    // TODO
}

/*
**  usec_clock()
**
**  An analog of the clock() function, usec_clock() returns a number of
**  type uclock_t (defined in UCLOCK.H) which represents the number of
**  microseconds past midnight. Analogous to CLOCKS_PER_SEC is UCLK_TCK, the
**  number which a usec_clock() reading must be divided by to yield
**  a number of seconds.
*/
using uclock_t = unsigned long;
uclock_t usec_clock()
{
    uclock_t result{};
    // TODO
    assert(false);

    return result;
}
