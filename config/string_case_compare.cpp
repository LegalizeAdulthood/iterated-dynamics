// SPDX-License-Identifier: GPL-3.0-only
//
#include "config/string_case_compare.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>

namespace id::config
{

static std::string to_lower(const std::string_view src)
{
    std::string result;
    result.resize(src.size());
    std::transform(src.begin(), src.end(), result.begin(),
        [](const char c) -> char { return static_cast<char>(std::tolower(c)); });
    return result;
}

// case independent version of std::strncmp
int string_case_compare(const char *s, const char *t, const std::size_t ct)
{
    const std::string lhs{to_lower(std::string_view{s, ct})};
    const std::string rhs{to_lower(std::string_view{t, ct})};
    return lhs.compare(rhs);
}

int string_case_compare(const char *s, const char *t)
{
    const std::string lhs{to_lower(s)};
    const std::string rhs{to_lower(t)};
    return lhs.compare(rhs);
}

} // namespace id::config
