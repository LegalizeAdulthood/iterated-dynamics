// SPDX-License-Identifier: GPL-3.0-only
//
/*
subroutines which belong primarily to calcfrac and
fractals, i.e. which are non-fractal-specific fractal engine subroutines.
*/
#include "engine/wait_until.h"

#include "misc/Driver.h"

#include <chrono>
#include <thread>

void sleep_ms(long ms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

/*
 * wait until wait_time microseconds from the
 * last call has elapsed.
 */
void wait_until(unsigned long wait_time_us)
{
    std::this_thread::sleep_for(std::chrono::microseconds(wait_time_us));
}
