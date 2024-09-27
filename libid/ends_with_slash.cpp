// SPDX-License-Identifier: GPL-3.0-only
//
#include "ends_with_slash.h"

#include "port.h"

#include <cstring>

int endswithslash(char const *fl)
{
    int len;
    len = (int) std::strlen(fl);
    if (len)
    {
        if (fl[--len] == SLASHC)
        {
            return 1;
        }
    }
    return 0;
}
