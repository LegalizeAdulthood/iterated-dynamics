// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

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
};

} // namespace id::math
