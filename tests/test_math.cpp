#include <fpu087.h>
#include <parser.h>

#include <gtest/gtest.h>

#include <cmath>

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
