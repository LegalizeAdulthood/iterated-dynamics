// SPDX-License-Identifier: GPL-3.0-only
//
#include "round_float_double.h"

#include <cstdio>
#include <cstdlib>

void round_float_double(double *x) // make double converted from float look ok
{
    char buf[30];
    std::sprintf(buf, "%-10.7g", *x);
    *x = std::atof(buf);
}
