// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <config/port.h>

namespace id::fractals
{

inline bool is_pow2(const int n)
{
    return n == (n & -n);
}

LDouble  get_number(const char **str);
int lsystem_type();
bool lsystem_load();

} // namespace id::Fractals
