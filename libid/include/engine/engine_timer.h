// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

namespace id::engine
{

extern bool                  g_timer_flag;
extern long                  g_timer_interval;
extern long                  g_engine_timer_start;

void engine_timer(int (*fn)());

} // namespace id::engine
