#pragma once

#include "port.h"

inline bool ispow2(int n)
{
    return n == (n & -n);
}

LDBL  getnumber(char const **str);
int Lsystem();
bool LLoad();
