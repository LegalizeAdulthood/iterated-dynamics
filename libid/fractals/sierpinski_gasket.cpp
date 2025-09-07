// SPDX-License-Identifier: GPL-3.0-only
//
#include "fractals/sierpinski_gasket.h"

#include "engine/bailout_formula.h"
#include "engine/calcfrac.h"
#include "engine/fractals.h"

bool sierpinski_per_image()
{
    // sierpinski
    g_periodicity_check = 0;                // disable periodicity checks
    g_tmp_z.x = 1;
    g_tmp_z.y = 0.5;
    return true;
}

int sierpinski_orbit()
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

    return id::g_bailout_float();
}
