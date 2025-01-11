// SPDX-License-Identifier: GPL-3.0-only
//
#include "math/cmplx.h"

#include "math/fixed_pt.h"
#include "math/fpu087.h"

#include <cmath>

LComplex complex_sqrt_long(long x, long y)
{
    double theta;
    long      mag_long, theta_long;
    LComplex    result;

    double mag = std::sqrt(std::sqrt(((double) multiply(x, x, g_bit_shift))/g_fudge_factor +
                           ((double) multiply(y, y, g_bit_shift))/ g_fudge_factor));
    mag_long   = (long)(mag * g_fudge_factor);
    theta     = std::atan2((double) y/g_fudge_factor, (double) x/g_fudge_factor)/2;
    constexpr long SIN_COS_FUDGE{0x10000L};
    theta_long = (long)(theta * SIN_COS_FUDGE);
    sin_cos(theta_long, &result.y, &result.x);
    result.x  = multiply(result.x << (g_bit_shift - 16), mag_long, g_bit_shift);
    result.y  = multiply(result.y << (g_bit_shift - 16), mag_long, g_bit_shift);
    return result;
}

DComplex complex_sqrt_float(double x, double y)
{
    DComplex  result;

    if (x == 0.0 && y == 0.0)
    {
        result.x = 0.0;
        result.y = 0.0;
    }
    else
    {
        double mag = std::sqrt(std::sqrt(x*x + y*y));
        double theta = std::atan2(y, x) / 2;
        sin_cos(&theta, &result.y, &result.x);
        result.x *= mag;
        result.y *= mag;
    }
    return result;
}
