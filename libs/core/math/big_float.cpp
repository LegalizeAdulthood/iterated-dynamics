// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
// C routines for big floating point numbers

/*
Wesley Loewer's Big Numbers.        (C) 1994-95, Wesley B. Loewer
*/
#include <math/big.h>
#include <math/BigFloat.h>

#include <cstdio>

using namespace id::misc;

namespace id::math
{

/********************************************************************/
// bf_hexdump() - for debugging, dumps to stdout

void bf_hex_dump(const BigFloat &r)
{
    for (int i = 0; i < g_bf_length; i++)
    {
        std::printf("%02X ", *(r.data() + i));
    }
    std::printf(" e %04hX\n", static_cast<S16>(BIG_ACCESS16(r.data() + g_bf_length)));
}

/**********************************************************************/
// The rest of the functions are "safe" versions of the routines that
// have side effects which alter the parameters.
// Most bf routines change values of parameters, not just the sign.
/**********************************************************************/


/**********************************************************************/
BigFloatPtr ln_bf(BigFloatPtr r, BigFloatPtr n)
{
    copy_bf(g_bf_tmp_copy1.data(), n);
    unsafe_ln_bf(r, g_bf_tmp_copy1.data());
    return r;
}

/**********************************************************************/
BigFloatPtr sin_cos_bf(BigFloatPtr s, BigFloatPtr c, BigFloatPtr n)
{
    copy_bf(g_bf_tmp_copy1.data(), n);
    return unsafe_sin_cos_bf(s, c, g_bf_tmp_copy1.data());
}

/**********************************************************************/
BigFloatPtr atan2_bf(BigFloatPtr r, BigFloatPtr ny, BigFloatPtr nx)
{
    copy_bf(g_bf_tmp_copy1.data(), ny);
    copy_bf(g_bf_tmp_copy2.data(), nx);
    unsafe_atan2_bf(r, g_bf_tmp_copy1.data(), g_bf_tmp_copy2.data());
    return r;
}

/********************************************************************/
// extracts the mantissa and exponent of f
// finds m and n such that 1<=|m|<10 and f = m*10^n
// n is stored in *exp_ptr and m is returned, sort of like frexp()
LDouble extract_10(LDouble f, int *exp_ptr)
{
    return extract_value(f, 10, exp_ptr);
}

/********************************************************************/
// calculates and returns the value of f*10^n
// sort of like ldexp()
LDouble scale_10(LDouble f, const int n)
{
    return scale_value(f, 10, n);
}

} // namespace id::math
