// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include <config/port.h>
#include <math/MPMath.h>

namespace id::math
{

class BigArena;
class BigFloat;

// Bridge the active legacy layout without native floating-point intermediates.
// Lossy conversions round to nearest, ties to even. Reverse conversions reject
// non-finite inputs and overflow; underflow rounds to zero. Signed zero is lost.
// Owners and selected arenas must match the current legacy precision layout.
class MPConversions
{
public:
    static MPReal from_big_float(const BigFloat &value, const MPPrecision &precision);
    static BigFloat to_big_float(const MPReal &value, BigArena &arena);
    // Legacy mantissa length excludes the two exponent bytes.
    static MPReal from_legacy(const Byte *bytes, int length, const MPPrecision &precision);
    static void to_legacy(const MPReal &value, Byte *bytes, int length);
};

} // namespace id::math
