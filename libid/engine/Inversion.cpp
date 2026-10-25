// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "engine/Inversion.h"

#include "math/biginit.h"

#include <cmath>
#include <limits>

using namespace id::math;

namespace id::engine
{

Inversion g_inversion;

double Inversion::apply(DComplex &point) const
{
    point -= center;
    const double magnitude{sqr(point.x) + sqr(point.y)};
    const double scale{std::abs(magnitude) > std::numeric_limits<float>::min() ? radius / magnitude
                                                                               : std::numeric_limits<float>::max()};
    point.x *= scale;
    point.y *= scale;
    point += center;
    return scale;
}

void Inversion::apply(const BFComplex point) const
{
    BigStackSaver saved;
    const BigFloat center_x{alloc_stack(g_r_bf_length + 2)};
    const BigFloat center_y{alloc_stack(g_r_bf_length + 2)};
    const BigFloat magnitude{alloc_stack(g_r_bf_length + 2)};
    const BigFloat scale{alloc_stack(g_r_bf_length + 2)};
    float_to_bf(center_x, center.x);
    float_to_bf(center_y, center.y);
    sub_a_bf(point.x, center_x);
    sub_a_bf(point.y, center_y);
    square_bf(magnitude, point.x);
    square_bf(scale, point.y);
    add_a_bf(magnitude, scale);
    float_to_bf(scale, std::numeric_limits<float>::min());
    if (cmp_bf(magnitude, scale) > 0)
    {
        float_to_bf(scale, radius);
        div_bf(scale, scale, magnitude);
    }
    else
    {
        float_to_bf(scale, std::numeric_limits<float>::max());
    }
    mult_bf(point.x, point.x, scale);
    mult_bf(point.y, point.y, scale);
    add_a_bf(point.x, center_x);
    add_a_bf(point.y, center_y);
}

} // namespace id::engine
