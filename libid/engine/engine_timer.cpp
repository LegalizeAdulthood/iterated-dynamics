// SPDX-License-Identifier: GPL-3.0-only
//
#include "engine/engine_timer.h"

#include <ctime>

namespace id::engine
{

long g_engine_timer_start{}; // timer(...) start & total
long g_timer_interval{};     //

void engine_timer(int (*fn)())
{
    g_engine_timer_start = std::clock();
    fn();
    // next assumes CLOCKS_PER_SEC is 10^n, n>=2
    g_timer_interval = (std::clock() - g_engine_timer_start) / (CLOCKS_PER_SEC / 100);
}

} // namespace id::engine
