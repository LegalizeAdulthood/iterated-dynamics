#include "get_prec_big_float.h"

#include "port.h"
#include "prototyp.h"

#include "big.h"
#include "biginit.h"
#include "cmdfiles.h"
#include "convert_center_mag.h"
#include "id.h"
#include "id_data.h"
#include "pixel_limits.h"

#include <algorithm>
#include <cfloat>

int getprecbf_mag()
{
    double Xmagfactor;
    double Rotation;
    double Skew;
    LDBL Magnification;
    bf_t bXctr;
    bf_t bYctr;
    int saved;
    int dec;

    saved = save_stack();
    bXctr            = alloc_stack(g_bf_length+2);
    bYctr            = alloc_stack(g_bf_length+2);
    // this is just to find Magnification
    cvtcentermagbf(bXctr, bYctr, &Magnification, &Xmagfactor, &Rotation, &Skew);
    restore_stack(saved);

    // I don't know if this is portable, but something needs to
    // be used in case compiler's LDBL_MAX is not big enough
    if (Magnification > LDBL_MAX || Magnification < -LDBL_MAX)
    {
        return -1;
    }

    dec = getpower10(Magnification) + 4; // 4 digits of padding sounds good
    return dec;
}

/* This function calculates the precision needed to distiguish adjacent
   pixels at maximum resolution of MAX_PIXELS by MAX_PIXELS
   (if rez==MAXREZ) or at current resolution (if rez==CURRENTREZ)    */
int getprecbf(int rezflag)
{
    bf_t del1;
    bf_t del2;
    bf_t one;
    bf_t bfxxdel;
    bf_t bfxxdel2;
    bf_t bfyydel;
    bf_t bfyydel2;
    int digits;
    int dec;
    int saved;
    int rez;
    saved    = save_stack();
    del1     = alloc_stack(g_bf_length+2);
    del2     = alloc_stack(g_bf_length+2);
    one      = alloc_stack(g_bf_length+2);
    bfxxdel   = alloc_stack(g_bf_length+2);
    bfxxdel2  = alloc_stack(g_bf_length+2);
    bfyydel   = alloc_stack(g_bf_length+2);
    bfyydel2  = alloc_stack(g_bf_length+2);
    floattobf(one, 1.0);
    if (rezflag == MAXREZ)
    {
        rez = OLD_MAX_PIXELS -1;
    }
    else
    {
        rez = g_logical_screen_x_dots-1;
    }

    // bfxxdel = (bfxmax - bfx3rd)/(xdots-1)
    sub_bf(bfxxdel, g_bf_x_max, g_bf_x_3rd);
    div_a_bf_int(bfxxdel, (U16)rez);

    // bfyydel2 = (bfy3rd - bfymin)/(xdots-1)
    sub_bf(bfyydel2, g_bf_y_3rd, g_bf_y_min);
    div_a_bf_int(bfyydel2, (U16)rez);

    if (rezflag == CURRENTREZ)
    {
        rez = g_logical_screen_y_dots-1;
    }

    // bfyydel = (bfymax - bfy3rd)/(ydots-1)
    sub_bf(bfyydel, g_bf_y_max, g_bf_y_3rd);
    div_a_bf_int(bfyydel, (U16)rez);

    // bfxxdel2 = (bfx3rd - bfxmin)/(ydots-1)
    sub_bf(bfxxdel2, g_bf_x_3rd, g_bf_x_min);
    div_a_bf_int(bfxxdel2, (U16)rez);

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
    digits = 1;
    while (cmp_bf(del1, one) < 0)
    {
        digits++;
        mult_a_bf_int(del1, 10);
    }
    digits = std::max(digits, 3);
    restore_stack(saved);
    dec = getprecbf_mag();
    return std::max(digits, dec);
}
