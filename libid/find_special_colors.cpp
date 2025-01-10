// SPDX-License-Identifier: GPL-3.0-only
//
#include "find_special_colors.h"

#include "engine/id_data.h"
#include "rotate.h"

#include <algorithm>

int g_color_dark{};   // darkest color in palette
int g_color_bright{}; // brightest color in palette
int g_color_medium{}; // nearest to medbright grey in palette

//*************** Function find_special_colors ********************
//
//      Find the darkest and brightest colors in palette, and a medium
//      color which is reasonably bright and reasonably grey.
//
void find_special_colors()
{
    int max_b = 0;
    int min_b = 9999;
    int med = 0;

    g_color_dark = 0;
    g_color_medium = 7;
    g_color_bright = 15;

    if (g_colors == 2)
    {
        g_color_medium = 1;
        g_color_bright = 1;
        return;
    }

    if (!g_got_real_dac)
    {
        return;
    }

    for (int i = 0; i < g_colors; i++)
    {
        const int brt = (int) g_dac_box[i][0] + (int) g_dac_box[i][1] + (int) g_dac_box[i][2];
        if (brt > max_b)
        {
            max_b = brt;
            g_color_bright = i;
        }
        if (brt < min_b)
        {
            min_b = brt;
            g_color_dark = i;
        }
        if (brt < 150 && brt > 80)
        {
            int min_gun = (int) g_dac_box[i][0];
            int max_gun = min_gun;
            if ((int) g_dac_box[i][1] > (int) g_dac_box[i][0])
            {
                max_gun = (int) g_dac_box[i][1];
            }
            else
            {
                min_gun = (int) g_dac_box[i][1];
            }
            max_gun = std::max((int) g_dac_box[i][2], max_gun);
            min_gun = std::min((int) g_dac_box[i][2], min_gun);
            if (brt - (max_gun - min_gun) / 2 > med)
            {
                g_color_medium = i;
                med = brt - (max_gun - min_gun) / 2;
            }
        }
    }
}
