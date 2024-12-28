// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

enum class TimerType
{
    ENGINE = 0,
    DECODER = 1,
    ENCODER = 2
};

extern bool                  g_timer_flag;
extern long                  g_timer_interval;
extern long                  g_timer_start;

int timer(TimerType timertype, int(*subrtn)(), ...);
