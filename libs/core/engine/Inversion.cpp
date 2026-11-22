// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/Inversion.h>

#include <math/big_init.h>

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

void Inversion::apply(BFComplex &point) const
{
    BigFloat center_x{0};
    BigFloat center_y{0};
    BigFloat magnitude{0};
    BigFloat scale{0};
    center_x = center.x;
    center_y = center.y;
    point.x -= center_x;
    point.y -= center_y;
    magnitude = sqr(point.x);
    scale = sqr(point.y);
    magnitude += scale;
    scale = std::numeric_limits<float>::min();
    if (magnitude > scale)
    {
        scale = radius;
        scale /= magnitude;
    }
    else
    {
        scale = std::numeric_limits<float>::max();
    }
    point.x *= scale;
    point.y *= scale;
    point.x += center_x;
    point.y += center_y;
}

} // namespace id::engine
