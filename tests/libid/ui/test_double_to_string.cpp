// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <ui/double_to_string.h>

#include <gtest/gtest.h>

#include <ostream>

namespace id::ui
{

struct DoubleToStringInput
{
    const char *name;
    double value;
    const char *expected;
    std::size_t max_length;
};

void PrintTo(const DoubleToStringInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestDoubleToString : public testing::TestWithParam<DoubleToStringInput>
{
};

TEST_P(TestDoubleToString, formatsWithinDisplayWidth)
{
    const std::string text{double_to_string(GetParam().value)};

    EXPECT_EQ(GetParam().expected, text);
    EXPECT_LE(text.length(), GetParam().max_length);
}

INSTANTIATE_TEST_SUITE_P(Value, TestDoubleToString,
    testing::Values(DoubleToStringInput{"Fraction", 1.0 / 3.0, "0.3333333333333333", 20U},
        DoubleToStringInput{"Cellular", 1234567890123456.0, "1234567890123456", 20U},
        DoubleToStringInput{"Exponent", 1.2345678901234567e+30, "1.23456789012346e+30", 20U}),
    [](const testing::TestParamInfo<DoubleToStringInput> &info) { return info.param.name; });

} // namespace id::ui
