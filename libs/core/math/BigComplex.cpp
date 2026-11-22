// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <math/BigComplex.h>

#include <algorithm>

namespace id::math
{
namespace
{

MPComplex complex_value(const MPReal &real, const MPReal &imag)
{
    return MPComplex{real, imag, std::max(real.precision(), imag.precision())};
}

} // namespace

BFComplex log(const BFComplex &value)
{
    if (value.x.is_zero() && value.y.is_zero())
    {
        return {BigFloat{0}, BigFloat{0}};
    }
    const MPComplex input = complex_value(*value.x.m_value, *value.y.m_value);
    const MPComplex output = boost::multiprecision::log(input);
    return {BigFloat::from_value(output.real()), BigFloat::from_value(output.imag())};
}

BFComplex pow(const BFComplex &base, const BFComplex &exponent)
{
    // Preserve the legacy convention, including zero and negative exponents.
    if (base.x.is_zero() && base.y.is_zero())
    {
        return {BigFloat{0}, BigFloat{0}};
    }
    const MPComplex input = complex_value(*base.x.m_value, *base.y.m_value);
    const MPComplex power = complex_value(*exponent.x.m_value, *exponent.y.m_value);
    const MPComplex output = boost::multiprecision::pow(input, power);
    return {BigFloat::from_value(output.real()), BigFloat::from_value(output.imag())};
}

} // namespace id::math
