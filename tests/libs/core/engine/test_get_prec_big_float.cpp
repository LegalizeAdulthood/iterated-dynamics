// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/get_prec_big_float.h>

#include <gtest/gtest.h>

#include <limits>
#include <ostream>

using namespace id::engine;

namespace id::test
{

struct MagnificationPrecisionInput
{
    const char *name;
    double magnification;
    int expected;
};

void PrintTo(const MagnificationPrecisionInput &value, std::ostream *str)
{
    *str << value.name;
}

class TestGetMagnificationPrecision : public testing::TestWithParam<MagnificationPrecisionInput>
{
};

TEST_P(TestGetMagnificationPrecision, computesRequiredPrecision)
{
    EXPECT_EQ(GetParam().expected, get_magnification_precision(GetParam().magnification));
}

INSTANTIATE_TEST_SUITE_P(Magnifications, TestGetMagnificationPrecision,
    testing::Values(MagnificationPrecisionInput{"One", 1.0, 4}, MagnificationPrecisionInput{"Ten", 10.0, 5},
        MagnificationPrecisionInput{"Hundred", 100.0, 6}, MagnificationPrecisionInput{"Tenth", 0.1, 3},
        MagnificationPrecisionInput{"Hundredth", 0.01, 2}, MagnificationPrecisionInput{"Two", 2.0, 4},
        MagnificationPrecisionInput{"Four", 4.0, 4}, MagnificationPrecisionInput{"Eight", 8.0, 4},
        MagnificationPrecisionInput{"Sixteen", 16.0, 5}, MagnificationPrecisionInput{"ThirtyTwo", 32.0, 5},
        MagnificationPrecisionInput{"Zero", 0.0, 4},
        MagnificationPrecisionInput{"MinimumNormal", std::numeric_limits<double>::min(), -304},
        MagnificationPrecisionInput{"Maximum", std::numeric_limits<double>::max(), 312},
        MagnificationPrecisionInput{"Epsilon", std::numeric_limits<double>::epsilon(), -12}),
    [](const testing::TestParamInfo<MagnificationPrecisionInput> &info) { return info.param.name; });

} // namespace id::test
