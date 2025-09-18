// SPDX-License-Identifier: GPL-3.0-only
//
#include "io/ends_with_slash.h"

#include <config/port.h>

#include <cstring>

namespace id::io
{

int ends_with_slash(const char *fl)
{
    if (int len = static_cast<int>(std::strlen(fl)); len)
    {
        if (fl[--len] == SLASH_CH)
        {
            return 1;
        }
    }
    return 0;
}

} // namespace id::io
