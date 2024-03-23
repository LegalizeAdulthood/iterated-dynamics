#include "sierpinski_gasket.h"

#include "port.h"
#include "prototyp.h"

#include "bailout_formula.h"
#include "calcfrac.h"
#include "fractals.h"

int SierpinskiFractal()
{
    /* following code translated from basic - see "Fractals
    Everywhere" by Michael Barnsley, p. 251, Program 7.1.1 */
    g_l_new_z.x = (g_l_old_z.x << 1);              // new.x = 2 * old.x
    g_l_new_z.y = (g_l_old_z.y << 1);              // new.y = 2 * old.y
    if (g_l_old_z.y > g_l_temp.y)  // if old.y > .5
    {
        g_l_new_z.y = g_l_new_z.y - g_l_temp.x;    // new.y = 2 * old.y - 1
    }
    else if (g_l_old_z.x > g_l_temp.y)     // if old.x > .5
    {
        g_l_new_z.x = g_l_new_z.x - g_l_temp.x;    // new.x = 2 * old.x - 1
    }
    // end barnsley code
    return g_bailout_long();
}

int SierpinskiFPFractal()
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

    // end barnsley code
    return g_bailout_float();
}
