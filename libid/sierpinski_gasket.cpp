// SPDX-License-Identifier: GPL-3.0-only
//
#include "sierpinski_gasket.h"

#include "port.h"
#include "prototyp.h"

#include "bailout_formula.h"
#include "calcfrac.h"
#include "fixed_pt.h"
#include "fractals.h"
#include "id_data.h"

bool sierpinski_setup()
{
    // sierpinski
    g_periodicity_check = 0;                // disable periodicity checks
    g_l_temp.x = 1;
    g_l_temp.x = g_l_temp.x << g_bit_shift; // ltmp.x = 1
    g_l_temp.y = g_l_temp.x >> 1;           // ltmp.y = .5
    return true;
}

bool sierpinski_fp_setup()
{
    // sierpinski
    g_periodicity_check = 0;                // disable periodicity checks
    g_tmp_z.x = 1;
    g_tmp_z.y = 0.5;
    return true;
}

int sierpinski_fractal()
{
    /* following code translated from basic - see "Fractals
    Everywhere" by Michael Barnsley, p. 251, Program 7.1.1 */
    g_l_new_z.x = (g_l_old_z.x << 1); // new.x = 2 * old.x
    g_l_new_z.y = (g_l_old_z.y << 1); // new.y = 2 * old.y
    if (g_l_old_z.y > g_l_temp.y)     // if old.y > .5
    {
        g_l_new_z.y = g_l_new_z.y - g_l_temp.x; // new.y = 2 * old.y - 1
    }
    else if (g_l_old_z.x > g_l_temp.y) // if old.x > .5
    {
        g_l_new_z.x = g_l_new_z.x - g_l_temp.x; // new.x = 2 * old.x - 1
    }

    return g_bailout_long();
}

int sierpinski_fp_fractal()
{
    /* following code translated from basic - see "Fractals
    Everywhere" by Michael Barnsley, p. 251, Program 7.1.1 */

    g_new_z.x = g_old_z.x + g_old_z.x;
    g_new_z.y = g_old_z.y + g_old_z.y;
    if (g_old_z.y > .5)
    {
        g_new_z.y = g_new_z.y - 1;
    }
    else if (g_old_z.x > .5)
    {
        g_new_z.x = g_new_z.x - 1;
    }

    return g_bailout_float();
}
