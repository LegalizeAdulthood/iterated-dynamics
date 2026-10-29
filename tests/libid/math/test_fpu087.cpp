// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <math/fpu087.h>

#include <gtest/gtest.h>

#include <cmath>

using namespace id::math;

namespace id::test
{

TEST(TestMath, complexLogZeroIsZero)
{
    const DComplex x{0.0, 0.0};
    DComplex z{1.0, 1.0};

    cmplx_log(x, z); // z = log(x)

    EXPECT_EQ(0.0, z.x);
    EXPECT_EQ(0.0, z.y);
}

struct ComplexLogInput
{
    const char *name;
    DComplex input;
    double expected_imaginary;
    double imaginary_tolerance;
};

class TestComplexLogInput : public testing::TestWithParam<ComplexLogInput>
{
};

TEST_P(TestComplexLogInput, retainsRealMagnitudeAndBranch)
{
    DComplex result{1.0, 1.0};

    cmplx_log(GetParam().input, result);

    EXPECT_NEAR(std::log(5.0), result.x, 1e-9);
    if (GetParam().imaginary_tolerance == 0.0)
    {
        EXPECT_EQ(GetParam().expected_imaginary, result.y);
    }
    else
    {
        EXPECT_NEAR(GetParam().expected_imaginary, result.y, GetParam().imaginary_tolerance);
    }
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestComplexLogInput,
    testing::Values(ComplexLogInput{"PositiveReal", {5.0, 0.0}, 0.0, 0.0},
        ComplexLogInput{"NegativeReal", {-5.0, 0.0}, std::acos(-1.0), 1e-9}),
    [](const testing::TestParamInfo<ComplexLogInput> &info) { return info.param.name; });

TEST(TestMath, complexLogPointerAlias)
{
    DComplex z{1.0, 2.0};

    cmplx_log(z, z); // z = log(z)

    EXPECT_NEAR(std::log(std::sqrt(5)), z.x, 1e-9);
    EXPECT_NEAR(std::atan2(2.0, 1.0), z.y, 1e-9);
}

enum class ExponentialRealResult
{
    ZERO,
    NAN_VALUE,
    INFINITE
};

struct ComplexExpInput
{
    const char *name;
    DComplex input;
    DComplex initial;
    ExponentialRealResult real_result;
};

class TestComplexExpInput : public testing::TestWithParam<ComplexExpInput>
{
};

TEST_P(TestComplexExpInput, retainsFractintExceptionalResult)
{
    DComplex result{GetParam().initial};

    cmplx_exp(GetParam().input, result);

    switch (GetParam().real_result)
    {
    case ExponentialRealResult::ZERO:
        EXPECT_EQ(0.0, result.x);
        break;
    case ExponentialRealResult::NAN_VALUE:
        EXPECT_TRUE(std::isnan(result.x));
        break;
    case ExponentialRealResult::INFINITE:
        EXPECT_TRUE(std::isinf(result.x));
        break;
    }
    EXPECT_EQ(0.0, result.y);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestComplexExpInput,
    testing::Values(ComplexExpInput{"Nan", {std::nan("1"), 0.0}, {}, ExponentialRealResult::NAN_VALUE},
        ComplexExpInput{"Underflow", {-691.0, 1.0}, {1.0, 1.0}, ExponentialRealResult::ZERO},
        ComplexExpInput{"Overflow", {1000.0, 0.0}, {1.0, 1.0}, ExponentialRealResult::INFINITE}),
    [](const testing::TestParamInfo<ComplexExpInput> &info) { return info.param.name; });

} // namespace id::test
