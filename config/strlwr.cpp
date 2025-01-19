// SPDX-License-Identifier: GPL-3.0-only
//
#include <cctype>

// strlwr -- Convert string to lower case.
char *strlwr(char *s)
{
    for (char *sptr = s; *sptr; ++sptr)
    {
        *sptr = std::tolower(*sptr);
    }
    return s;
}
