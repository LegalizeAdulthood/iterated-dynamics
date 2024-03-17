#include "spindac.h"

#include "port.h"
#include "prototyp.h"

#include "cmdfiles.h"
#include "drivers.h"
#include "id_data.h"
#include "miscovl.h"
#include "rotate.h"

#include <cstring>

int g_dac_count = 0;

/*
; *************** Function spindac(direction, rstep) ********************

;       Rotate the MCGA/VGA DAC in the (plus or minus) "direction"
;       in "rstep" increments - or, if "direction" is 0, just replace it.
*/
void spindac(int dir, int inc)
{
    if (g_colors < 16)
        return;
    if (g_is_true_color && g_true_mode != true_color_mode::default_color)
        return;
    if (dir != 0 && g_color_cycle_range_lo < g_colors && g_color_cycle_range_lo < g_color_cycle_range_hi)
    {
        int top = g_color_cycle_range_hi > g_colors ? g_colors - 1 : g_color_cycle_range_hi;
        unsigned char *dacbot = (unsigned char *) g_dac_box + 3*g_color_cycle_range_lo;
        int len = (top - g_color_cycle_range_lo)*3*sizeof(unsigned char);
        if (dir > 0)
        {
            for (int i = 0; i < inc; i++)
            {
                unsigned char tmp[3];
                std::memcpy(tmp, dacbot, 3*sizeof(unsigned char));
                std::memcpy(dacbot, dacbot + 3*sizeof(unsigned char), len);
                std::memcpy(dacbot + len, tmp, 3*sizeof(unsigned char));
            }
        }
        else
        {
            for (int i = 0; i < inc; i++)
            {
                unsigned char tmp[3];
                std::memcpy(tmp, dacbot + len, 3*sizeof(unsigned char));
                std::memcpy(dacbot + 3*sizeof(unsigned char), dacbot, len);
                std::memcpy(dacbot, tmp, 3*sizeof(unsigned char));
            }
        }
    }
    driver_write_palette();
    driver_delay(g_colors - g_dac_count - 1);
}
