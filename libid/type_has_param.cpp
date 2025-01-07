// SPDX-License-Identifier: GPL-3.0-only
//
#include "type_has_param.h"

#include "find_extra_param.h"
#include "fractalp.h"
#include "param_not_used.h"

#include <cstring>

/*
 *  Returns 1 if parameter number parm exists for type. If the
 *  parameter exists, the parameter prompt string is copied to buf.
 *  Pass in nullptr for buf if only the existence of the parameter is
 *  needed, and not the prompt string.
 */
bool type_has_param(FractalType type, int param, char *buf)
{
    char const *ret = nullptr;
    if (0 <= param && param < 4)
    {
        ret = g_fractal_specific[+type].param_names[param];
    }
    else if (param >= 4 && param < MAX_PARAMS)
    {
        int const extra = find_extra_param(type);
        if (extra > -1)
        {
            ret = g_more_fractal_params[extra].param_names[param-4];
        }
    }
    if (ret)
    {
        if (*ret == 0)
        {
            ret = nullptr;
        }
    }

    if (type == FractalType::FORMULA || type == FractalType::FORMULA_FP)
    {
        if (param_not_used(param))
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
