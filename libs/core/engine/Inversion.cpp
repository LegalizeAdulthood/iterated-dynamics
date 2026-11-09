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
    BigStackSaver saved;
    BigFloat center_x{bf_arena(BigLifetime::TEMPORARY)};
    BigFloat center_y{bf_arena(BigLifetime::TEMPORARY)};
    BigFloat magnitude{bf_arena(BigLifetime::TEMPORARY)};
    BigFloat scale{bf_arena(BigLifetime::TEMPORARY)};
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
