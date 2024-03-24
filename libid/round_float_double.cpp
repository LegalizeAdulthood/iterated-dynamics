#include "round_float_double.h"

#include <cstdio>
#include <cstdlib>

void roundfloatd(double *x) // make double converted from float look ok
{
    char buf[30];
    std::sprintf(buf, "%-10.7g", *x);
    *x = std::atof(buf);
}
