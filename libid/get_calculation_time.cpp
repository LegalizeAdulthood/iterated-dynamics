// SPDX-License-Identifier: GPL-3.0-only
//
#include "get_calculation_time.h"

#include <array>
#include <cstdio>
#include <string>

std::string get_calculation_time(long ctime)
{
    if (ctime < 0)
    {
        return "A long time! (> 24.855 days)";
    }

    char msg[80];
    std::snprintf(msg, std::size(msg), "%3ld:%02ld:%02ld.%02ld", ctime/360000L,
        (ctime%360000L)/6000, (ctime%6000)/100, ctime%100);
    return msg;
}
