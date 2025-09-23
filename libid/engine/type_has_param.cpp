// SPDX-License-Identifier: GPL-3.0-only
//
#include "engine/type_has_param.h"

#include "engine/find_extra_param.h"
#include "engine/param_not_used.h"
#include "fractals/fractalp.h"

#include <cstring>

using namespace id::fractals;

namespace id::engine
{

/*
 *  Returns 1 if parameter number parm exists for type. If the
 *  parameter exists, the parameter prompt string is copied to buf.
 *  Pass in nullptr for buf if only the existence of the parameter is
 *  needed, and not the prompt string.
 */
bool type_has_param(const FractalType type, const int param, const char **prompt)
{
    const char *ret = nullptr;
    if (0 <= param && param < 4)
    {
        ret = get_fractal_specific(type)->param_names[param];
    }
    else if (param >= 4 && param < MAX_PARAMS)
    {
        if (const int extra = find_extra_param(type); extra > -1)
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

    if (type == FractalType::FORMULA)
    {
        if (param_not_used(param))
        {
            ret = nullptr;
        }
    }

    if (ret && prompt != nullptr)
    {
        *prompt = ret;
    }
    return ret != nullptr;
}

} // namespace id::engine
