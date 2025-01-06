// SPDX-License-Identifier: GPL-3.0-only
//
#include "lowerize_parameter.h"

#include <algorithm>
#include <cstring>
#include <string>

// don't convert these parameter values
static constexpr const char *const s_unchanged_params[] = {"autokeyname", "colors", "comment", "filename",
    "formulafile", "ifsfile", "lfile", "lightname", "makedoc", "makepar", "map", "orbitsavename", "orgfrmdir",
    "parmfile", "savename", "savedir", "tempdir", "workdir"};

void lowerize_parameter(char *cur_arg)
{
    char *arg_ptr = cur_arg;
    while (*arg_ptr)
    {
        // convert to lower case
        if (*arg_ptr >= 'A' && *arg_ptr <= 'Z')
        {
            *arg_ptr += 'a' - 'A';
        }
        else if (*arg_ptr == '=')
        {
            auto it = std::find_if(std::begin(s_unchanged_params), std::end(s_unchanged_params),
                [param = std::string(cur_arg, arg_ptr)](const char *str) { return param == str; });
            if (it != std::end(s_unchanged_params))
            {
                break;
            }
        }
        ++arg_ptr;
    }
}
