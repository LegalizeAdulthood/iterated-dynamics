// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

extern bool                  g_timer_flag;
extern long                  g_timer_interval;
extern long                  g_engine_timer_start;

void engine_timer(int (*fn)());
int encoder_timer();
int decoder_timer(int width);
