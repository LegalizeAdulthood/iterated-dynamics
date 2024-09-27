// SPDX-License-Identifier: GPL-3.0-only
//
#include "param_not_used.h"

#include "port.h"
#include "prototyp.h"

#include "fractype.h"
#include "parser.h"

/*
 *  Returns 1 if the formula parameter is not used in the current
 *  formula.  If the parameter is used, or not a formula fractal,
 *  a 0 is returned.  Note: this routine only works for formula types.
 */
bool paramnotused(int parm)
{
    bool ret = false;

    // sanity check
    if (g_fractal_type != fractal_type::FORMULA && g_fractal_type != fractal_type::FFORMULA)
    {
        return false;
    }

    switch (parm/2)
    {
    case 0:
        if (!g_frm_uses_p1)
        {
            ret = true;
        }
        break;
    case 1:
        if (!g_frm_uses_p2)
        {
            ret = true;
        }
        break;
    case 2:
        if (!g_frm_uses_p3)
        {
            ret = true;
        }
        break;
    case 3:
        if (!g_frm_uses_p4)
        {
            ret = true;
        }
        break;
    case 4:
        if (!g_frm_uses_p5)
        {
            ret = true;
        }
        break;
    default:
        ret = false;
        break;
    }
    return ret;
}
