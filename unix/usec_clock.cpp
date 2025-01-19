// SPDX-License-Identifier: GPL-3.0-only
//
#include "misc/usec_clock.h"

#include <sys/time.h>

// tenths of millisecond timer routine
static struct timeval tv_start;

void restart_uclock()
{
    gettimeofday(&tv_start, nullptr);
}

uclock_t usec_clock()
{
    uclock_t result;

    struct timeval tv, elapsed;
    gettimeofday(&tv, nullptr);

    elapsed.tv_usec  = tv.tv_usec -  tv_start.tv_sec;
    elapsed.tv_sec   = tv.tv_sec -   tv_start.tv_sec;

    if (elapsed.tv_usec < 0)
    {
        // "borrow
        elapsed.tv_usec += 1000000;
        elapsed.tv_sec--;
    }
    result  = (unsigned long)(elapsed.tv_sec*10000 +  elapsed.tv_usec/100);
    return result;
}
