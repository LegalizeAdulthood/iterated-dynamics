// SPDX-License-Identifier: GPL-3.0-only
//
#include "lowerize_parameter.h"

#include <algorithm>
#include <cstring>
#include <string>

// don't convert these parameter values
static constexpr const char *const s_unchanged_params[] = {"autokeyname", "colors", "comment", "filename",
    "formulafile", "ifsfile", "lfile", "lightname", "makedoc", "map", "orbitsavename", "orgfrmdir",
    "parmfile", "savename", "savedir", "tempdir", "workdir"};

void lowerize_parameter(char *curarg)
{
    char *argptr = curarg;
    while (*argptr)
    {
        // convert to lower case
        if (*argptr >= 'A' && *argptr <= 'Z')
        {
            *argptr += 'a' - 'A';
        }
        else if (*argptr == '=')
        {
            auto it = std::find_if(std::begin(s_unchanged_params), std::end(s_unchanged_params),
                [param = std::string(curarg, argptr)](const char *str) { return param == str; });
            if (it != std::end(s_unchanged_params))
            {
                break;
            }
        }
        ++argptr;
    }
}
