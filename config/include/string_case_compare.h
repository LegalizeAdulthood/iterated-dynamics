// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "port_config.h"

#ifdef ID_HAVE_STRNCASECMP
#include <strings.h>

#else
#include <cctype>

// case independent version of std::strncmp
inline int strncasecmp(char const *s, char const *t, int ct)
{
    for (; (std::tolower(*s) == std::tolower(*t)) && --ct ; s++, t++)
    {
        if (*s == '\0')
        {
            return 0;
        }
    }
    return std::tolower(*s) - std::tolower(*t);
}

#endif
