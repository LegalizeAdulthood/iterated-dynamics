// SPDX-License-Identifier: GPL-3.0-only
//
#include "check_key.h"

#include "calcfrac.h"
#include "drivers.h"
#include "orbit.h"

bool check_key()
{
    int key = driver_key_pressed();
    if (key != 0)
    {
        if (g_show_orbit)
        {
            scrub_orbit();
        }
        if (key != 'o' && key != 'O')
        {
            return true;
        }
        driver_get_key();
        if (!driver_is_disk())
        {
            g_show_orbit = !g_show_orbit;
        }
    }
    return false;
}
