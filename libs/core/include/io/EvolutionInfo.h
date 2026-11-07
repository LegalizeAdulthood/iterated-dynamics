// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include <cstdint>

namespace id::io
{

enum
{
    NUM_GENES = 21
};

/*
 * EvolutionInfo is serialized field by field in io/gif_file.cpp.
 * Keep those serializers and the 200-byte record length consistent with
 * changes here; native structure padding is not part of the file format.
 */
struct EvolutionInfo // for saving evolution data in a GIF file
{
    std::int16_t evolving;
    std::int16_t image_grid_size;
    std::uint16_t this_generation_random_seed;
    double max_random_mutation;
    double x_parameter_range;
    double y_parameter_range;
    double x_parameter_offset;
    double y_parameter_offset;
    std::int16_t discrete_x_parameter_offset;
    std::int16_t discrete_y_parameter_offset;
    std::int16_t px;
    std::int16_t py;
    std::int16_t screen_x_offset;
    std::int16_t screen_y_offset;
    std::uint16_t x_dots;
    std::uint16_t y_dots;
    std::int16_t mutate[NUM_GENES];
    std::int16_t count;                  // count of how many images have been calc'ed so far
    std::int16_t future[66 - NUM_GENES]; // reserved fields in the 200-byte record
};

bool operator==(const EvolutionInfo &lhs, const EvolutionInfo &rhs);

inline bool operator!=(const EvolutionInfo &lhs, const EvolutionInfo &rhs)
{
    return !(lhs == rhs);
}

} // namespace id::io
