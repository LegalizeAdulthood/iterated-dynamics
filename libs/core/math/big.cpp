// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
// Wesley Loewer's Big Numbers. (C) 1994-95, Wesley B. Loewer
//
#include <math/big.h>

#include <cstring>

using namespace id::misc;

namespace id::math
{

// r = n
Byte *copy_bn(Byte *r, const Byte *n)
{
    std::memcpy(r, n, g_bn_length);
    return r;
}

// r = n1 + n2
Byte *add_bn(Byte *r, const Byte *n1, const Byte *n2)
{
    U32 sum = 0;

    // two bytes at a time
    for (int i = 0; i < g_bn_length; i += 2)
    {
        sum += static_cast<U32>(BIG_ACCESS16(n1 + i)) + static_cast<U32>(BIG_ACCESS16(n2 + i)); // add 'em up
        BIG_SET16(r + i, static_cast<U16>(sum)); // store the lower 2 bytes
        sum >>= 16;                              // shift the overflow for next time
    }
    return r;
}

// r = n1 - n2
Byte *sub_bn(Byte *r, const Byte *n1, const Byte *n2)
{
    U32 diff = 0;

    // two bytes at a time
    for (int i = 0; i < g_bn_length; i += 2)
    {
        diff = static_cast<U32>(BIG_ACCESS16(n1 + i)) -
            (static_cast<U32>(BIG_ACCESS16(n2 + i)) - static_cast<S32>(static_cast<S16>(diff))); // subtract with borrow
        BIG_SET16(r + i, static_cast<U16>(diff)); // store the lower 2 bytes
        diff >>= 16;                              // shift the underflow for next time
    }
    return r;
}

// r += n
Byte *add_a_bn(Byte *r, const Byte *n)
{
    return add_bn(r, r, n);
}

// r -= n
Byte *sub_a_bn(Byte *r, const Byte *n)
{
    return sub_bn(r, r, n);
}

// r = n * u
Byte *mult_bn_int(Byte *r, const Byte *n, const U16 u)
{
    U32 prod = 0;

    // two bytes at a time
    for (int i = 0; i < g_bn_length; i += 2)
    {
        prod += static_cast<U32>(BIG_ACCESS16(n + i)) * u; // n*u
        BIG_SET16(r + i, static_cast<U16>(prod));          // store the lower 2 bytes
        prod >>= 16;                                       // shift the overflow for next time
    }
    return r;
}

// r = |n|
Byte *abs_bn(Byte *r, const Byte *n)
{
    copy_bn(r, n);
    if (is_bn_neg(r))
    {
        neg_a_bn(r);
    }
    return r;
}

// r = |r|
Byte *abs_a_bn(Byte *r)
{
    if (is_bn_neg(r))
    {
        neg_a_bn(r);
    }
    return r;
}

} // namespace id::math
