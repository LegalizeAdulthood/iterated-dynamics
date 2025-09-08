// SPDX-License-Identifier: GPL-3.0-only
//
#include <cctype>

namespace id::config
{

// string_lower -- Convert string to lower case.
char *string_lower(char *s)
{
    for (char *sptr = s; *sptr; ++sptr)
    {
        *sptr = std::tolower(*sptr);
    }
    return s;
}

} // namespace id
