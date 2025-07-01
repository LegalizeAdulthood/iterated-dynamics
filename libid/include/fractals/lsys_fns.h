// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <config/port.h>

inline bool is_pow2(int n)
{
    return n == (n & -n);
}

LDouble  get_number(const char **str);
int lsystem_type();
bool lsystem_load();
