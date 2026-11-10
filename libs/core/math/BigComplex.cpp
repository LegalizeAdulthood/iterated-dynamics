// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <math/BigComplex.h>

#include <utility>

namespace id::math
{
BFComplex log(const BFComplex &value)
{
    if (value == BFComplex{BigFloat{0}, BigFloat{0}})
    {
        return {BigFloat{0}, BigFloat{0}};
    }
    BigFloat real = log(mag_squared(value)).half();
    BigFloat imag = atan2(value.y, value.x);
    return {std::move(real), std::move(imag)};
}

BFComplex pow(const BFComplex &base, const BFComplex &exponent)
{
    // Preserve the legacy convention, including zero and negative exponents.
    if (base == BFComplex{BigFloat{0}, BigFloat{0}})
    {
        return {BigFloat{0}, BigFloat{0}};
    }
    const BFComplex product = log(base) * exponent;
    const BigFloat magnitude = exp(product.x);
    const auto [sine, cosine] = sin_cos(product.y);
    return {magnitude * cosine, magnitude * sine};
}

} // namespace id::math
