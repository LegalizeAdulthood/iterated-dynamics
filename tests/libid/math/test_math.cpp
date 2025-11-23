// SPDX-License-Identifier: GPL-3.0-only
//
#include <math/fpu087.h>

#include <fractals/parser.h>
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

TEST(TestMath, nanExponent)
{
    const DComplex x{std::nan("1"), 0.0};
    DComplex z{};

    fpu_cmplx_exp(x, z); // z = e^x

    EXPECT_EQ((DComplex{1.0, 0.0}), z);
}

TEST(TestMath, nanRealMultiply)
{
    const DComplex lhs{std::nan("1"), 0.0};
    const DComplex rhs{1.0, 0.0};

    DComplex result;
    fpu_cmplx_mul(lhs, rhs, result);

    EXPECT_EQ((DComplex{ID_INFINITY, 0.0}), result);
}

TEST(TestMath, infRealMultiply)
{
    const DComplex lhs{INFINITY, 0.0};
    const DComplex rhs{1.0, 0.0};

    DComplex result;
    fpu_cmplx_mul(lhs, rhs, result);

    EXPECT_EQ((DComplex{ID_INFINITY, 0.0}), result);
}

TEST(TestMath, nanImagMultiply)
{
    const DComplex lhs{0.0, std::nan("1")};
    const DComplex rhs{1.0, 0.0};

    DComplex result;
    fpu_cmplx_mul(lhs, rhs, result);

    EXPECT_EQ((DComplex{0.0, ID_INFINITY}), result);
}

TEST(TestMath, infImagMultiply)
{
    const DComplex lhs{0.0, INFINITY};
    const DComplex rhs{1.0, 0.0};

    DComplex result;
    fpu_cmplx_mul(lhs, rhs, result);

    EXPECT_EQ((DComplex{0.0, ID_INFINITY}), result);
}

TEST(TestMath, zeroDivide)
{
    const DComplex lhs{1.0, 1.0};
    const DComplex rhs{};

    DComplex result;
    fpu_cmplx_div(lhs, rhs, result);

    EXPECT_EQ((DComplex{ID_INFINITY, ID_INFINITY}), result);
    EXPECT_TRUE(g_overflow);
}

TEST(TestMath, nanAngleSinCos)
{
    const double angle{std::nan("1")};

    double result_sin{999.0};
    double result_cos{999.0};
    sin_cos(angle, result_sin, result_cos);

    EXPECT_EQ(0.0, result_sin);
    EXPECT_EQ(1.0, result_cos);
}

TEST(TestMath, infAngleSinCos)
{
    const double angle{INFINITY};

    double result_sin{999.0};
    double result_cos{999.0};
    sin_cos(angle, result_sin, result_cos);

    EXPECT_EQ(0.0, result_sin);
    EXPECT_EQ(1.0, result_cos);
}

TEST(TestMath, nanAngleSinHCosH)
{
    const double angle{std::nan("1")};

    double result_sin{999.0};
    double result_cos{999.0};
    sinh_cosh(angle, result_sin, result_cos);

    EXPECT_EQ(1.0, result_sin);
    EXPECT_EQ(1.0, result_cos);
}

TEST(TestMath, infAngleSinHCosH)
{
    const double angle{INFINITY};

    double result_sin{999.0};
    double result_cos{999.0};
    sinh_cosh(angle, result_sin, result_cos);

    EXPECT_EQ(1.0, result_sin);
    EXPECT_EQ(1.0, result_cos);
}

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

TEST(TestMath, divideAliasing)
{
    const DComplex num{1.0, 2.0};
    const DComplex denom{2.0, 4.0};
    DComplex result;
    fpu_cmplx_div(num, denom, result);
    const std::complex<double> stdResult{std::complex<double>{1.0, 2.0}/std::complex<double>{2.0, 4.0}};

    DComplex alias_result1{1.0, 2.0};
    fpu_cmplx_div(alias_result1, denom, alias_result1);
    DComplex alias_result2{2.0, 4.0};
    fpu_cmplx_div(num, alias_result2, alias_result2);

    EXPECT_EQ(stdResult.real(), result.x);
    EXPECT_EQ(stdResult.imag(), result.y);
    EXPECT_EQ(stdResult.real(), alias_result1.x);
    EXPECT_EQ(stdResult.imag(), alias_result1.y);
    EXPECT_EQ(stdResult.real(), alias_result2.x);
    EXPECT_EQ(stdResult.imag(), alias_result2.y);
}

} // namespace id::test
