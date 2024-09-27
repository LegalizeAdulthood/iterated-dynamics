// SPDX-License-Identifier: GPL-3.0-only
//
#include "type_has_param.h"

#include "port.h"
#include "prototyp.h"

#include "find_extra_param.h"
#include "fractalp.h"
#include "id.h"
#include "param_not_used.h"

#include <cstring>

/*
 *  Returns 1 if parameter number parm exists for type. If the
 *  parameter exists, the parameter prompt string is copied to buf.
 *  Pass in nullptr for buf if only the existence of the parameter is
 *  needed, and not the prompt string.
 */
bool typehasparm(fractal_type type, int parm, char *buf)
{
    char const *ret = nullptr;
    if (0 <= parm && parm < 4)
    {
        ret = g_fractal_specific[+type].param[parm];
    }
    else if (parm >= 4 && parm < MAX_PARAMS)
    {
        int const extra = find_extra_param(type);
        if (extra > -1)
        {
            ret = g_more_fractal_params[extra].param[parm-4];
        }
    }
    if (ret)
    {
        if (*ret == 0)
        {
            ret = nullptr;
        }
    }

    if (type == fractal_type::FORMULA || type == fractal_type::FFORMULA)
    {
        if (paramnotused(parm))
        {
            ret = nullptr;
        }
    }

    if (ret && buf != nullptr)
    {
        std::strcpy(buf, ret);
    }
    return ret != nullptr;
}
