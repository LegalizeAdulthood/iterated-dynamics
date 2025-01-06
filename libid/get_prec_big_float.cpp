// SPDX-License-Identifier: GPL-3.0-only
//
#include "get_prec_big_float.h"

#include "big.h"
#include "biginit.h"
#include "cmdfiles.h"
#include "convert_center_mag.h"
#include "id_data.h"
#include "pixel_limits.h"
#include "port.h"

#include <algorithm>
#include <cfloat>

int get_prec_bf_mag()
{
    double x_mag_factor;
    double rotation;
    double skew;
    LDouble magnification;

    int saved = save_stack();
    bf_t bXctr = alloc_stack(g_bf_length + 2);
    bf_t bYctr = alloc_stack(g_bf_length + 2);
    // this is just to find magnification
    cvt_center_mag_bf(bXctr, bYctr, magnification, x_mag_factor, rotation, skew);
    restore_stack(saved);

    // I don't know if this is portable, but something needs to
    // be used in case compiler's LDBL_MAX is not big enough
    if (magnification > LDBL_MAX || magnification < -LDBL_MAX)
    {
        return -1;
    }

    int dec = get_power10(magnification) + 4; // 4 digits of padding sounds good
    return dec;
}

/* This function calculates the precision needed to distinguish adjacent
   pixels at maximum resolution of MAX_PIXELS by MAX_PIXELS
   (if res==Resolution::MAX) or at current resolution (if res==Resolution::CURRENT)    */
int get_prec_bf(ResolutionFlag flag)
{
    int res;
    int saved = save_stack();
    bf_t del1 = alloc_stack(g_bf_length + 2);
    bf_t del2 = alloc_stack(g_bf_length + 2);
    bf_t one = alloc_stack(g_bf_length + 2);
    bf_t bfxxdel = alloc_stack(g_bf_length + 2);
    bf_t bfxxdel2 = alloc_stack(g_bf_length + 2);
    bf_t bfyydel = alloc_stack(g_bf_length + 2);
    bf_t bfyydel2 = alloc_stack(g_bf_length + 2);
    float_to_bf(one, 1.0);
    if (flag == ResolutionFlag::MAX)
    {
        res = OLD_MAX_PIXELS -1;
    }
    else
    {
        res = g_logical_screen_x_dots-1;
    }

    // bfxxdel = (bfxmax - bfx3rd)/(xdots-1)
    sub_bf(bfxxdel, g_bf_x_max, g_bf_x_3rd);
    div_a_bf_int(bfxxdel, (U16)res);

    // bfyydel2 = (bfy3rd - bfymin)/(xdots-1)
    sub_bf(bfyydel2, g_bf_y_3rd, g_bf_y_min);
    div_a_bf_int(bfyydel2, (U16)res);

    if (flag == ResolutionFlag::CURRENT)
    {
        res = g_logical_screen_y_dots-1;
    }

    // bfyydel = (bfymax - bfy3rd)/(ydots-1)
    sub_bf(bfyydel, g_bf_y_max, g_bf_y_3rd);
    div_a_bf_int(bfyydel, (U16)res);

    // bfxxdel2 = (bfx3rd - bfxmin)/(ydots-1)
    sub_bf(bfxxdel2, g_bf_x_3rd, g_bf_x_min);
    div_a_bf_int(bfxxdel2, (U16)res);

    abs_a_bf(add_bf(del1, bfxxdel, bfxxdel2));
    abs_a_bf(add_bf(del2, bfyydel, bfyydel2));
    if (cmp_bf(del2, del1) < 0)
    {
        copy_bf(del1, del2);
    }
    if (cmp_bf(del1, clear_bf(del2)) == 0)
    {
        restore_stack(saved);
        return -1;
    }
    int digits = 1;
    while (cmp_bf(del1, one) < 0)
    {
        digits++;
        mult_a_bf_int(del1, 10);
    }
    digits = std::max(digits, 3);
    restore_stack(saved);
    int dec = get_prec_bf_mag();
    return std::max(digits, dec);
}
