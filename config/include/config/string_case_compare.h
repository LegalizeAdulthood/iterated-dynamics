// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <cstddef>

namespace id::config
{

int string_case_compare(const char *s, const char *t, std::size_t ct);
int string_case_compare(const char *s, const char *t);

inline bool string_case_equal(const char *s, const char *t, std::size_t ct)
{
    return string_case_compare(s, t, ct) == 0;
}

inline bool string_case_equal(const char *s, const char *t)
{
    return string_case_compare(s, t) == 0;
}

} // namespace id::config
