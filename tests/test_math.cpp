#include <fpu087.h>
#include <parser.h>

#include <gtest/gtest.h>

#include <cmath>
#include <complex>
#include <fixed_pt.h>

namespace id
{

inline std::ostream &operator<<(std::ostream &str, const DComplex &value)
{
    return str << '(' << value.x << ", " << value.y << ')';
}

} // namespace id

TEST(TestMath, nanExponent)
{
    DComplex x{std::nan("1"), 0.0};
    DComplex z{};

    FPUcplxexp(&x, &z); // z = e^x

    EXPECT_EQ((DComplex{1.0, 0.0}), z);
}

TEST(TestMath, nanRealMultiply)
{
    DComplex lhs{std::nan("1"), 0.0};
    DComplex rhs{1.0, 0.0};

    DComplex result;
    FPUcplxmul(&lhs, &rhs, &result);

    EXPECT_EQ((DComplex{ID_INFINITY, 0.0}), result);
}

TEST(TestMath, infRealMultiply)
{
    DComplex lhs{INFINITY, 0.0};
    DComplex rhs{1.0, 0.0};

    DComplex result;
    FPUcplxmul(&lhs, &rhs, &result);

    EXPECT_EQ((DComplex{ID_INFINITY, 0.0}), result);
}

TEST(TestMath, nanImagMultiply)
{
    DComplex lhs{0.0, std::nan("1")};
    DComplex rhs{1.0, 0.0};

    DComplex result;
    FPUcplxmul(&lhs, &rhs, &result);

    EXPECT_EQ((DComplex{0.0, ID_INFINITY}), result);
}

TEST(TestMath, infImagMultiply)
{
    DComplex lhs{0.0, INFINITY};
    DComplex rhs{1.0, 0.0};

    DComplex result;
    FPUcplxmul(&lhs, &rhs, &result);

    EXPECT_EQ((DComplex{0.0, ID_INFINITY}), result);
}

TEST(TestMath, zeroDivide)
{
    DComplex lhs{1.0, 1.0};
    DComplex rhs{};

    DComplex result;
    FPUcplxdiv(&lhs, &rhs, &result);

    EXPECT_EQ((DComplex{ID_INFINITY, ID_INFINITY}), result);
    EXPECT_TRUE(g_overflow);
}

TEST(TestMath, nanAngleSinCos)
{
    const double angle{std::nan("1")};

    double result_sin{999.0};
    double result_cos{999.0};
    FPUsincos(&angle, &result_sin, &result_cos);

    EXPECT_EQ(0.0, result_sin);
    EXPECT_EQ(1.0, result_cos);
}

TEST(TestMath, infAngleSinCos)
{
    const double angle{INFINITY};

    double result_sin{999.0};
    double result_cos{999.0};
    FPUsincos(&angle, &result_sin, &result_cos);

    EXPECT_EQ(0.0, result_sin);
    EXPECT_EQ(1.0, result_cos);
}

TEST(TestMath, nanAngleSinHCosH)
{
    const double angle{std::nan("1")};

    double result_sin{999.0};
    double result_cos{999.0};
    FPUsinhcosh(&angle, &result_sin, &result_cos);

    EXPECT_EQ(1.0, result_sin);
    EXPECT_EQ(1.0, result_cos);
}

TEST(TestMath, infAngleSinHCosH)
{
    const double angle{INFINITY};

    double result_sin{999.0};
    double result_cos{999.0};
    FPUsinhcosh(&angle, &result_sin, &result_cos);

    EXPECT_EQ(1.0, result_sin);
    EXPECT_EQ(1.0, result_cos);
}

TEST(TestMath, multiplyAliasing)
{
    const DComplex z{1.0, 2.0};
    DComplex result;
    FPUcplxmul(&z, &z, &result);
    const std::complex<double> stdResult{std::complex<double>{1.0, 2.0}*std::complex<double>{1.0, 2.0}};

    DComplex aliasResult{1.0, 2.0};
    FPUcplxmul(&aliasResult, &aliasResult, &aliasResult);

    EXPECT_EQ(stdResult.real(), result.x);
    EXPECT_EQ(stdResult.imag(), result.y);
    EXPECT_EQ(stdResult.real(), aliasResult.x);
    EXPECT_EQ(stdResult.imag(), aliasResult.y);
}

TEST(TestMath, divideAliasing)
{
    const DComplex num{1.0, 2.0};
    const DComplex denom{2.0, 4.0};
    DComplex result;
    FPUcplxdiv(&num, &denom, &result);
    const std::complex<double> stdResult{std::complex<double>{1.0, 2.0}/std::complex<double>{2.0, 4.0}};

    DComplex aliasResult1{1.0, 2.0};
    FPUcplxdiv(&aliasResult1, &denom, &aliasResult1);
    DComplex aliasResult2{2.0, 4.0};
    FPUcplxdiv(&num, &aliasResult2, &aliasResult2);

    EXPECT_EQ(stdResult.real(), result.x);
    EXPECT_EQ(stdResult.imag(), result.y);
    EXPECT_EQ(stdResult.real(), aliasResult1.x);
    EXPECT_EQ(stdResult.imag(), aliasResult1.y);
    EXPECT_EQ(stdResult.real(), aliasResult2.x);
    EXPECT_EQ(stdResult.imag(), aliasResult2.y);
}
