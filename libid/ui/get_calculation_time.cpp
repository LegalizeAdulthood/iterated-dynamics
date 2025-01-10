// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/get_calculation_time.h"

#include <array>
#include <cstdio>
#include <string>

std::string get_calculation_time(long calc_time)
{
    if (calc_time < 0)
    {
        return "A long time! (> 24.855 days)";
    }

    char msg[80];
    std::snprintf(msg, std::size(msg), "%3ld:%02ld:%02ld.%02ld", calc_time/360000L,
        (calc_time%360000L)/6000, (calc_time%6000)/100, calc_time%100);
    return msg;
}
