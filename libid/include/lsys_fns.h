// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "port.h"

inline bool is_pow2(int n)
{
    return n == (n & -n);
}

LDBL  get_number(char const **str);
int lsystem();
bool lsystem_load();
