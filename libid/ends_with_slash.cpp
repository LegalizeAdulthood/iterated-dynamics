// SPDX-License-Identifier: GPL-3.0-only
//
#include "ends_with_slash.h"

#include "port.h"

#include <cstring>

int ends_with_slash(char const *fl)
{
    int len;
    len = (int) std::strlen(fl);
    if (len)
    {
        if (fl[--len] == SLASH_CH)
        {
            return 1;
        }
    }
    return 0;
}
