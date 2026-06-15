// SPDX-License-Identifier: GPL-3.0-only
//
#include "math/round_float_double.h"

#include <cstdlib>
#include <string>

#include <fmt/format.h>

namespace id::math
{

void round_float_double(double *x) // make double converted from float look ok
{
    const std::string buf{fmt::format("{:<10.7g}", *x)};
    *x = std::atof(buf.c_str());
}

} // namespace id::math
