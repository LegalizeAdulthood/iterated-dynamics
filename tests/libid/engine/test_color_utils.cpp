// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/color_utils.h>

#include <gtest/gtest.h>

#include <ostream>

using namespace id::engine;

namespace id::test
{

struct ColorExpansionInput
{
    const char *name;
    Byte input;
    int expected;
};

void PrintTo(const ColorExpansionInput &value, std::ostream *str)
{
    *str << value.name;
}

class TestColorUtils6Bit : public testing::TestWithParam<ColorExpansionInput>
{
};

TEST_P(TestColorUtils6Bit, expandsChannelToFullRange)
{
    EXPECT_EQ(GetParam().expected, expand_6bit_color(GetParam().input));
}

INSTANTIATE_TEST_SUITE_P(ChannelInputs, TestColorUtils6Bit,
    testing::Values(ColorExpansionInput{"Zero", 0, 0}, ColorExpansionInput{"FirstStep", 1, 4},
        ColorExpansionInput{"OneThird", 21, 85}, ColorExpansionInput{"TwoThirds", 42, 170},
        ColorExpansionInput{"Maximum", 63, 255}),
    [](const testing::TestParamInfo<ColorExpansionInput> &info) { return info.param.name; });

class TestColorUtils8Bit : public testing::TestWithParam<ColorExpansionInput>
{
};

TEST_P(TestColorUtils8Bit, expandsQuantizedChannelToFullRange)
{
    EXPECT_EQ(GetParam().expected, expand_8bit_color(GetParam().input));
}

INSTANTIATE_TEST_SUITE_P(ChannelInputs, TestColorUtils8Bit,
    testing::Values(ColorExpansionInput{"Zero", 0, 0}, ColorExpansionInput{"FirstStep", 4, 4},
        ColorExpansionInput{"OneThird", 84, 85}, ColorExpansionInput{"TwoThirds", 168, 170},
        ColorExpansionInput{"Maximum", 252, 255}),
    [](const testing::TestParamInfo<ColorExpansionInput> &info) { return info.param.name; });

} // namespace id::test
