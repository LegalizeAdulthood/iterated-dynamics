// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

namespace id::engine
{

extern long                  g_timer_interval;
extern long                  g_engine_timer_start;

void start_calculation_timer();
void update_calculation_timer();
void engine_timer(int (*fn)());

} // namespace id::engine
