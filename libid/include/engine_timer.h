#pragma once

enum class timer_type
{
    ENGINE = 0,
    DECODER = 1,
    ENCODER = 2
};

extern bool                  g_timer_flag;
extern long                  g_timer_interval;
extern long                  g_timer_start;

int timer(timer_type timertype, int(*subrtn)(), ...);
