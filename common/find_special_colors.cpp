#include "find_special_colors.h"

#include "port.h"

#include "id_data.h"
#include "rotate.h"

int g_color_dark = 0;       // darkest color in palette
int g_color_bright = 0;     // brightest color in palette
int g_color_medium = 0;     /* nearest to medbright grey in palette
                   Zoom-Box values (2K x 2K screens max) */

//*************** Function find_special_colors ********************
//
//      Find the darkest and brightest colors in palette, and a medium
//      color which is reasonably bright and reasonably grey.
//
void find_special_colors()
{
    int maxb = 0;
    int minb = 9999;
    int med = 0;
    int maxgun, mingun;

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
        return;

    for (int i = 0; i < g_colors; i++)
    {
        const int brt = (int) g_dac_box[i][0] + (int) g_dac_box[i][1] + (int) g_dac_box[i][2];
        if (brt > maxb)
        {
            maxb = brt;
            g_color_bright = i;
        }
        if (brt < minb)
        {
            minb = brt;
            g_color_dark = i;
        }
        if (brt < 150 && brt > 80)
        {
            mingun = (int) g_dac_box[i][0];
            maxgun = mingun;
            if ((int) g_dac_box[i][1] > (int) g_dac_box[i][0])
            {
                maxgun = (int) g_dac_box[i][1];
            }
            else
            {
                mingun = (int) g_dac_box[i][1];
            }
            if ((int) g_dac_box[i][2] > maxgun)
            {
                maxgun = (int) g_dac_box[i][2];
            }
            if ((int) g_dac_box[i][2] < mingun)
            {
                mingun = (int) g_dac_box[i][2];
            }
            if (brt - (maxgun - mingun) / 2 > med)
            {
                g_color_medium = i;
                med = brt - (maxgun - mingun) / 2;
            }
        }
    }
}
