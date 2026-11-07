// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <math/fpu087.h>

#include <math/fixed_pt.h>

#include <gtest/gtest.h>

#include <cmath>
#include <complex>

using namespace id::math;

namespace id
{

inline std::ostream &operator<<(std::ostream &str, const DComplex &value)
{
    return str << '(' << value.x << ", " << value.y << ')';
}

} // namespace id

namespace id::test
{

struct ExceptionalComplexProduct
{
    const char *name;
    DComplex lhs;
    DComplex rhs;
    bool imaginary;
    bool nan;
};

class TestExceptionalComplexProduct : public testing::TestWithParam<ExceptionalComplexProduct>
{
};

TEST_P(TestExceptionalComplexProduct, preservesExceptionalComponent)
{
    DComplex result;

    fpu_cmplx_mul(GetParam().lhs, GetParam().rhs, result);

    const double exceptional{GetParam().imaginary ? result.y : result.x};
    const double zero{GetParam().imaginary ? result.x : result.y};
    EXPECT_EQ(0.0, zero);
    if (GetParam().nan)
    {
        EXPECT_TRUE(std::isnan(exceptional));
    }
    else
    {
        EXPECT_TRUE(std::isinf(exceptional));
    }
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestExceptionalComplexProduct,
    testing::Values(ExceptionalComplexProduct{"NanReal", {std::nan("1"), 0.0}, {1.0, 0.0}, false, true},
        ExceptionalComplexProduct{"InfiniteReal", {INFINITY, 0.0}, {1.0, 0.0}, false, false},
        ExceptionalComplexProduct{"NanImaginary", {0.0, std::nan("1")}, {1.0, 0.0}, true, true},
        ExceptionalComplexProduct{"InfiniteImaginary", {0.0, INFINITY}, {1.0, 0.0}, true, false}),
    [](const testing::TestParamInfo<ExceptionalComplexProduct> &info) { return info.param.name; });

TEST(TestMath, zeroDivide)
{
    const DComplex lhs{1.0, 1.0};
    const DComplex rhs{};

    DComplex result;
    fpu_cmplx_div(lhs, rhs, result);

    EXPECT_EQ((DComplex{ID_INFINITY, ID_INFINITY}), result);
    EXPECT_TRUE(g_overflow);
}

struct ExceptionalAngle
{
    const char *name;
    double value;
};

class TestExceptionalAngle : public testing::TestWithParam<ExceptionalAngle>
{
protected:
    double result_sin{999.0};
    double result_cos{999.0};
};

TEST_P(TestExceptionalAngle, sineAndCosineRetainFallback)
{
    sin_cos(GetParam().value, result_sin, result_cos);

    EXPECT_EQ(0.0, result_sin);
    EXPECT_EQ(1.0, result_cos);
}

TEST_P(TestExceptionalAngle, hyperbolicSineAndCosineRetainFallback)
{
    sinh_cosh(GetParam().value, result_sin, result_cos);

    EXPECT_EQ(1.0, result_sin);
    EXPECT_EQ(1.0, result_cos);
}

INSTANTIATE_TEST_SUITE_P(Angles, TestExceptionalAngle,
    testing::Values(ExceptionalAngle{"Nan", std::nan("1")}, ExceptionalAngle{"Infinite", INFINITY}),
    [](const testing::TestParamInfo<ExceptionalAngle> &info) { return info.param.name; });

TEST(TestMath, multiplyAliasing)
{
    const DComplex z{1.0, 2.0};
    DComplex result;
    fpu_cmplx_mul(z, z, result);
    const std::complex<double> stdResult{std::complex<double>{1.0, 2.0}*std::complex<double>{1.0, 2.0}};

    DComplex alias_result{1.0, 2.0};
    fpu_cmplx_mul(alias_result, alias_result, alias_result);

    EXPECT_EQ(stdResult.real(), result.x);
    EXPECT_EQ(stdResult.imag(), result.y);
    EXPECT_EQ(stdResult.real(), alias_result.x);
    EXPECT_EQ(stdResult.imag(), alias_result.y);
}

struct ComplexDivisionAliasInput
{
    const char *name;
    DComplex numerator;
    DComplex denominator;
    bool alias_numerator;
};

class TestComplexDivisionAlias : public testing::TestWithParam<ComplexDivisionAliasInput>
{
};

TEST_P(TestComplexDivisionAlias, matchesTheNonAliasedQuotient)
{
    DComplex numerator{GetParam().numerator};
    DComplex denominator{GetParam().denominator};
    DComplex baseline;
    fpu_cmplx_div(numerator, denominator, baseline);
    const std::complex<double> expected{
        std::complex<double>{numerator.x, numerator.y} / std::complex<double>{denominator.x, denominator.y}};
    DComplex &aliased{GetParam().alias_numerator ? numerator : denominator};

    fpu_cmplx_div(numerator, denominator, aliased);

    EXPECT_EQ(expected.real(), baseline.x);
    EXPECT_EQ(expected.imag(), baseline.y);
    EXPECT_EQ(expected.real(), aliased.x);
    EXPECT_EQ(expected.imag(), aliased.y);
}

INSTANTIATE_TEST_SUITE_P(Operands, TestComplexDivisionAlias,
    testing::Values(ComplexDivisionAliasInput{"CollinearNumerator", {1.0, 2.0}, {2.0, 4.0}, true},
        ComplexDivisionAliasInput{"CollinearDenominator", {1.0, 2.0}, {2.0, 4.0}, false},
        ComplexDivisionAliasInput{"RealDivisorNumerator", {1.0, 2.0}, {2.0, 0.0}, true},
        ComplexDivisionAliasInput{"RealDivisorDenominator", {1.0, 2.0}, {2.0, 0.0}, false}),
    [](const testing::TestParamInfo<ComplexDivisionAliasInput> &info) { return info.param.name; });

} // namespace id::test
