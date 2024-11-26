// SPDX-License-Identifier: GPL-3.0-only
//
#include "calcmand.h"

#include "stop_msg.h"

long calc_mand_asm()
{
    static bool been_here = false;
    if (!been_here)
    {
        stop_msg("This integer fractal type is unimplemented;\n"
                "Use float=yes to get a real image.");
        been_here = true;
    }
    return 0;
}
