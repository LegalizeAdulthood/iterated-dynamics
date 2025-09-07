// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/get_calculation_time.h"

#include <fmt/format.h>

#include <string>

namespace id::ui
{

std::string get_calculation_time(long calc_time)
{
    if (calc_time < 0)
    {
        return "A long time! (> 24.855 days)";
    }

    return fmt::format("{:3d}:{:02d}:{:02d}.{:02d}", //
        calc_time / 360000L,                         //
        (calc_time % 360000L) / 6000,                //
        (calc_time % 6000) / 100,                    //
        calc_time % 100);
}

} // namespace id::ui
