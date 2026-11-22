// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include <config/port.h>
#include <math/MPMath.h>

namespace id::math
{

// Encode and decode legacy file records without native floating-point intermediates.
// Lossy conversions round to nearest, ties to even. Reverse conversions reject
// non-finite inputs and overflow; underflow rounds to zero. Signed zero is lost.
class MPConversions
{
public:
    // Legacy mantissa length excludes the two exponent bytes.
    static MPReal from_legacy(const Byte *bytes, int length, const MPPrecision &precision);
    static void to_legacy(const MPReal &value, Byte *bytes, int length);
};

} // namespace id::math
