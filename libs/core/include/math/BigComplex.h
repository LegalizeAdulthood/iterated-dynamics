// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include <math/BigFloat.h>
#include <math/BigNum.h>
#include <math/cmplx.h>

namespace id::math
{

using BFComplex = Complex<BigFloat>;

BFComplex log(const BFComplex &value);
BFComplex pow(const BFComplex &base, const BFComplex &exponent);

} // namespace id::math
