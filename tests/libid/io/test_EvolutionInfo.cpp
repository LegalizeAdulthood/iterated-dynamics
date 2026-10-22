// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <io/EvolutionInfo.h>

#include <gtest/gtest.h>

using namespace id::io;

namespace id::test
{

TEST(TestEvolutionInfo, comparesDiscreteFields)
{
    const EvolutionInfo original{};
    EXPECT_EQ(original, original);
    EXPECT_FALSE(original != original);
    constexpr std::int16_t EvolutionInfo::*signed_members[]{
        &EvolutionInfo::evolving,
        &EvolutionInfo::image_grid_size,
        &EvolutionInfo::discrete_x_parameter_offset,
        &EvolutionInfo::discrete_y_parameter_offset,
        &EvolutionInfo::px,
        &EvolutionInfo::py,
        &EvolutionInfo::screen_x_offset,
        &EvolutionInfo::screen_y_offset,
        &EvolutionInfo::count,
    };
    for (std::int16_t EvolutionInfo::*member : signed_members)
    {
        EvolutionInfo changed{original};
        changed.*member = -1;
        EXPECT_NE(original, changed);
        EXPECT_FALSE(original == changed);
    }
    constexpr std::uint16_t EvolutionInfo::*unsigned_members[]{
        &EvolutionInfo::this_generation_random_seed,
        &EvolutionInfo::x_dots,
        &EvolutionInfo::y_dots,
    };
    for (std::uint16_t EvolutionInfo::*member : unsigned_members)
    {
        EvolutionInfo changed{original};
        changed.*member = 65535;
        EXPECT_NE(original, changed);
    }
    for (int i = 0; i < NUM_GENES; ++i)
    {
        EvolutionInfo changed{original};
        changed.mutate[i] = 1;
        EXPECT_NE(original, changed) << i;
    }
}

TEST(TestEvolutionInfo, preservesCoordinateToleranceAndExactMutationMagnitude)
{
    const EvolutionInfo original{};
    constexpr double EvolutionInfo::*coordinates[]{
        &EvolutionInfo::x_parameter_range,
        &EvolutionInfo::y_parameter_range,
        &EvolutionInfo::x_parameter_offset,
        &EvolutionInfo::y_parameter_offset,
    };
    for (double EvolutionInfo::*member : coordinates)
    {
        EvolutionInfo changed{original};
        changed.*member = 1.0e-7;
        EXPECT_EQ(original, changed);
        changed.*member = 1.0e-5;
        EXPECT_NE(original, changed);
    }
    EvolutionInfo changed{original};
    changed.max_random_mutation = 1.0e-7;
    EXPECT_NE(original, changed);
}

TEST(TestEvolutionInfo, ignoresReservedFields)
{
    const EvolutionInfo original{};
    EvolutionInfo changed{original};
    for (std::int16_t &value : changed.future)
    {
        value = -1;
    }

    EXPECT_EQ(original, changed);
    EXPECT_FALSE(original != changed);
}

} // namespace id::test
