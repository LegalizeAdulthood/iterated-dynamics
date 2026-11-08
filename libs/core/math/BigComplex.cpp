// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <math/BigComplex.h>

#include <engine/fractalb.h>
#include <math/complex_fn.h>

#include <utility>

namespace id::math
{
namespace
{

BFComplex import_bf(const BFConstPtrComplex &source)
{
    const auto size = static_cast<std::size_t>(g_bf_length + 2);
    return {BigFloat::from_legacy(source.x, size), BigFloat::from_legacy(source.y, size)};
}

void export_bf(const BFComplex &value, const BFPtrComplex &destination)
{
    const auto size = static_cast<std::size_t>(g_bf_length + 2);
    value.x.copy_to_legacy(destination.x, size);
    value.y.copy_to_legacy(destination.y, size);
}

} // namespace

BFComplex log(const BFComplex &value)
{
    if (value == BFComplex{})
    {
        return {};
    }
    BigFloat real = log(mag_squared(value)).half();
    BigFloat imag = atan2(value.y, value.x);
    return {std::move(real), std::move(imag)};
}

BFComplex pow(const BFComplex &base, const BFComplex &exponent)
{
    // Preserve the legacy convention, including zero and negative exponents.
    if (base == BFComplex{})
    {
        return {};
    }
    const BFComplex product = log(base) * exponent;
    const BigFloat magnitude = exp(product.x);
    const auto [sine, cosine] = sin_cos(product.y);
    return {magnitude * cosine, magnitude * sine};
}

void cube(BFPtrComplex &out, const BFPtrComplex &in)
{
    const BFComplex input = import_bf({in.x, in.y});
    export_bf(cube(input), out);
}

void power(BFPtrComplex &result, const BFPtrComplex &z, const int degree)
{
    const BFComplex input = import_bf({z.x, z.y});
    export_bf(power(input, degree), result);
}

} // namespace id::math

// Historical engine entry points remain until their consumers migrate.
namespace id::engine
{

math::BFPtrComplex *cmplx_log_bf(math::BFPtrComplex *t, math::BFPtrComplex *s)
{
    const math::BFComplex input = math::import_bf({s->x, s->y});
    math::export_bf(math::log(input), *t);
    return t;
}

math::BFPtrComplex *cmplx_mul_bf(math::BFPtrComplex *t, math::BFPtrComplex *x, math::BFPtrComplex *y)
{
    const math::BFComplex left = math::import_bf({x->x, x->y});
    const math::BFComplex right = math::import_bf({y->x, y->y});
    math::export_bf(left * right, *t);
    return t;
}

math::BFPtrComplex *cmplx_div_bf(math::BFPtrComplex *t, math::BFPtrComplex *x, math::BFPtrComplex *y)
{
    const math::BFComplex left = math::import_bf({x->x, x->y});
    const math::BFComplex right = math::import_bf({y->x, y->y});
    math::export_bf(left / right, *t);
    return t;
}

math::BFPtrComplex *cmplx_pow_bf(math::BFPtrComplex *t, math::BFPtrComplex *xx, math::BFPtrComplex *yy)
{
    const math::BFComplex base = math::import_bf({xx->x, xx->y});
    const math::BFComplex exponent = math::import_bf({yy->x, yy->y});
    math::export_bf(math::pow(base, exponent), *t);
    return t;
}

} // namespace id::engine
