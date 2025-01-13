// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#ifdef ID_HAVE_STRNCASECMP
#include <strings.h>

#else
#include <cctype>
#include <cstring>

// case independent version of std::strncmp
inline int strncasecmp(char const *s, char const *t, std::size_t ct)
{
    for (; *t && (std::tolower(*s) == std::tolower(*t)) && ct > 0 ; --ct, s++, t++)
    {
        if (*s == '\0')
        {
            return 0;
        }
    }
    if (ct == 0)
    {
        return 0;
    }
    return std::tolower(*s) - std::tolower(*t);
}

inline int strcasecmp(const char *s, const char *t)
{
    return strncasecmp(s, t, std::strlen(s));
}

#endif
