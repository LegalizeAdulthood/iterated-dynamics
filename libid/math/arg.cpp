// SPDX-License-Identifier: GPL-3.0-only
//
#include "math/arg.h"

using namespace id::fractals;

namespace id::math
{

Arg *g_arg1{};
Arg *g_arg2{};
void (*g_d_trig0)(){d_stk_sin};
void (*g_d_trig1)(){d_stk_sqr};
void (*g_d_trig2)(){d_stk_sinh};
void (*g_d_trig3)(){d_stk_cosh};

} // namespace id::math
