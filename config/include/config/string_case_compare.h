// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <cstring>

#ifdef ID_HAVE_STRNCASECMP
#include <strings.h>

#else
#include <cctype>

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

inline int string_case_compare(char const *s, char const *t, std::size_t ct)
{
    return strncasecmp(s, t, ct);
}

inline int string_case_compare(const char *s, const char *t)
{
    return strncasecmp(s, t, std::strlen(s));
}
