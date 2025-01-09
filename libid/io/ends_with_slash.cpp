// SPDX-License-Identifier: GPL-3.0-only
//
#include "io/ends_with_slash.h"

#include "port.h"

#include <cstring>

int ends_with_slash(char const *fl)
{
    if (int len = (int) std::strlen(fl); len)
    {
        if (fl[--len] == SLASH_CH)
        {
            return 1;
        }
    }
    return 0;
}
