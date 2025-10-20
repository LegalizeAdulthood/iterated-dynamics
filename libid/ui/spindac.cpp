// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/spindac.h"

#include "engine/cmdfiles.h"
#include "engine/VideoInfo.h"
#include "misc/Driver.h"
#include "ui/rotate.h"

#include <cstring>

using namespace id::engine;
using namespace id::misc;

namespace id::ui
{

int g_dac_count{};
bool g_is_true_color{};

/*
; *************** Function spindac(direction, rstep) ********************

;       Rotate the MCGA/VGA DAC in the (plus or minus) "direction"
;       in "rstep" increments - or, if "direction" is 0, just replace it.
*/
void spin_dac(const int dir, const int inc)
{
    if (g_colors < 16)
    {
        return;
    }
    if (g_is_true_color && g_true_mode != TrueColorMode::DEFAULT_COLOR)
    {
        return;
    }
    if (dir != 0 && g_color_cycle_range_lo < g_colors && g_color_cycle_range_lo < g_color_cycle_range_hi)
    {
        const int top = g_color_cycle_range_hi > g_colors ? g_colors - 1 : g_color_cycle_range_hi;
        unsigned char *dac_bot = reinterpret_cast<unsigned char *>(g_dac_box) + 3 * g_color_cycle_range_lo;
        const int len = (top - g_color_cycle_range_lo)*3*sizeof(unsigned char);
        if (dir > 0)
        {
            for (int i = 0; i < inc; i++)
            {
                unsigned char tmp[3];
                std::memcpy(tmp, dac_bot, 3*sizeof(unsigned char));
                std::memcpy(dac_bot, dac_bot + 3*sizeof(unsigned char), len);
                std::memcpy(dac_bot + len, tmp, 3*sizeof(unsigned char));
            }
        }
        else
        {
            for (int i = 0; i < inc; i++)
            {
                unsigned char tmp[3];
                std::memcpy(tmp, dac_bot + len, 3*sizeof(unsigned char));
                std::memcpy(dac_bot + 3*sizeof(unsigned char), dac_bot, len);
                std::memcpy(dac_bot, tmp, 3*sizeof(unsigned char));
            }
        }
    }
    driver_write_palette();
    driver_delay(g_colors - g_dac_count - 1);
}

} // namespace id::ui
