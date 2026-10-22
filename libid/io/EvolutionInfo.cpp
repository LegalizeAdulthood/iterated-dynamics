// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "io/EvolutionInfo.h"

#include <algorithm>
#include <cmath>
#include <iterator>

namespace id::io
{

template <int N>
static bool equal(const std::int16_t (&lhs)[N], const std::int16_t (&rhs)[N])
{
    return std::equal(std::begin(lhs), std::end(lhs), std::begin(rhs));
}

static bool within_eps(const double lhs, const double rhs)
{
    return std::abs(lhs - rhs) < 1.0e-6f;
}

bool operator==(const EvolutionInfo &lhs, const EvolutionInfo &rhs)
{
    return lhs.evolving == rhs.evolving                                       //
        && lhs.image_grid_size == rhs.image_grid_size                         //
        && lhs.this_generation_random_seed == rhs.this_generation_random_seed //
        && lhs.max_random_mutation == rhs.max_random_mutation                 //
        && within_eps(lhs.x_parameter_range, rhs.x_parameter_range)           //
        && within_eps(lhs.y_parameter_range, rhs.y_parameter_range)           //
        && within_eps(lhs.x_parameter_offset, rhs.x_parameter_offset)         //
        && within_eps(lhs.y_parameter_offset, rhs.y_parameter_offset)         //
        && lhs.discrete_x_parameter_offset == rhs.discrete_x_parameter_offset //
        && lhs.discrete_y_parameter_offset == rhs.discrete_y_parameter_offset //
        && lhs.px == rhs.px                                                   //
        && lhs.py == rhs.py                                                   //
        && lhs.screen_x_offset == rhs.screen_x_offset                         //
        && lhs.screen_y_offset == rhs.screen_y_offset                         //
        && lhs.x_dots == rhs.x_dots                                           //
        && lhs.y_dots == rhs.y_dots                                           //
        && equal(lhs.mutate, rhs.mutate)                                      //
        && lhs.count == rhs.count;                                            //
}

} // namespace id::io
