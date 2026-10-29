// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <math/round_float_double.h>

#include <gtest/gtest.h>

namespace id::math
{

struct FloatRoundingInput
{
    const char *name;
    double input;
    double expected;
};

class TestFloatRounding : public testing::TestWithParam<FloatRoundingInput>
{
};

TEST_P(TestFloatRounding, roundsToSevenSignificantDigits)
{
    double value{GetParam().input};

    round_float_double(&value);

    EXPECT_DOUBLE_EQ(GetParam().expected, value);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestFloatRounding,
    testing::Values(
        FloatRoundingInput{"FloatConversionNoise", static_cast<double>(static_cast<float>(1.0 / 3.0)), 0.3333333},
        FloatRoundingInput{"SevenSignificantDigits", 12345.678901234, 12345.68}),
    [](const testing::TestParamInfo<FloatRoundingInput> &info) { return info.param.name; });

} // namespace id::math
