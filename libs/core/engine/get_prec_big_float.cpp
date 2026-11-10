// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/get_prec_big_float.h>

#include <engine/convert_center_mag.h>
#include <engine/LogicalScreen.h>
#include <engine/pixel_limits.h>
#include <math/big.h>
#include <math/big_init.h>
#include <math/BigFloat.h>

#include <config/port.h>

#include <algorithm>
#include <cfloat>
#include <cmath>

using namespace id::math;
using namespace id::misc;

namespace id::engine
{

int get_prec_bf_mag()
{
    double x_mag_factor;
    double rotation;
    double skew;
    LDouble magnification;

    BigFloat b_x_ctr{bf_arena(BigLifetime::TEMPORARY)};
    BigFloat b_y_ctr{bf_arena(BigLifetime::TEMPORARY)};
    // this is just to find magnification
    cvt_center_mag_bf(b_x_ctr, b_y_ctr, magnification, x_mag_factor, rotation, skew);

    if (std::isnan(magnification) || std::isinf(magnification))
    {
        return -1;
    }

    return get_magnification_precision(magnification);
}

/* This function calculates the precision needed to distinguish adjacent
   pixels at maximum resolution of MAX_PIXELS by MAX_PIXELS
   (if res==Resolution::MAX) or at current resolution (if res==Resolution::CURRENT)    */
int get_prec_bf(const ResolutionFlag flag)
{
    int res;
    BigFloat del1{bf_arena(BigLifetime::TEMPORARY)};
    BigFloat del2{bf_arena(BigLifetime::TEMPORARY)};
    BigFloat one{bf_arena(BigLifetime::TEMPORARY)};
    BigFloat bf_x_delta{bf_arena(BigLifetime::TEMPORARY)};
    BigFloat bf_x_delta2{bf_arena(BigLifetime::TEMPORARY)};
    BigFloat bf_y_delta{bf_arena(BigLifetime::TEMPORARY)};
    BigFloat bf_y_delta2{bf_arena(BigLifetime::TEMPORARY)};
    one = 1.0;
    if (flag == ResolutionFlag::MAX)
    {
        res = OLD_MAX_PIXELS -1;
    }
    else
    {
        res = g_logical_screen.x_dots-1;
    }

    // bfxxdel = (bfxmax - bfx3rd)/(xdots-1)
    sub_bf(bf_x_delta.data(), g_bf_x_max.data(), g_bf_x_3rd.data());
    bf_x_delta /= static_cast<U16>(res);

    // bfyydel2 = (bfy3rd - bfymin)/(xdots-1)
    sub_bf(bf_y_delta2.data(), g_bf_y_3rd.data(), g_bf_y_min.data());
    bf_y_delta2 /= static_cast<U16>(res);

    if (flag == ResolutionFlag::CURRENT)
    {
        res = g_logical_screen.y_dots-1;
    }

    // bfyydel = (bfymax - bfy3rd)/(ydots-1)
    sub_bf(bf_y_delta.data(), g_bf_y_max.data(), g_bf_y_3rd.data());
    bf_y_delta /= static_cast<U16>(res);

    // bfxxdel2 = (bfx3rd - bfxmin)/(ydots-1)
    sub_bf(bf_x_delta2.data(), g_bf_x_3rd.data(), g_bf_x_min.data());
    bf_x_delta2 /= static_cast<U16>(res);

    del1 = abs(del1 = bf_x_delta + bf_x_delta2);
    del2 = abs(del2 = bf_y_delta + bf_y_delta2);
    if (del2 < del1)
    {
        del1 = del2;
    }
    if (del1 == (del2 = 0))
    {
        return -1;
    }
    int digits = 1;
    while (del1 < one)
    {
        digits++;
        del1 = del1 * 10;
    }
    digits = std::max(digits, 3);
    return std::max(digits, get_prec_bf_mag());
}

// get_power10(x) returns the magnitude of x.
static int get_power10(const LDouble x)
{
    // Special case for zero
    if (x == 0.0)
    {
        return 0;
    }
    return static_cast<int>(std::floor(std::log10(std::abs(x))));
}

int get_magnification_precision(const LDouble magnification)
{
    return get_power10(magnification) + 4; // 4 digits of padding sounds good
}

} // namespace id::engine
