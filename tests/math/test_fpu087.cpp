#include <math/fpu087.h>

#include <gtest/gtest.h>

#include <cmath>

TEST(TestMath, complexDividePointerAliasX)
{
    DComplex y{2.0, 0.0};
    DComplex z{1.0, 2.0};

    fpu_cmplx_div(&z, &y, &z); // z = z/y

    EXPECT_EQ(1.0/2.0, z.x);
    EXPECT_EQ(1.0, z.y);
}

TEST(TestMath, complexDividePointerAliasY)
{
    DComplex x{1.0, 2.0};
    DComplex z{2.0, 0.0};

    fpu_cmplx_div(&x, &z, &z); // z = x/z

    EXPECT_EQ(1.0/2.0, z.x);
    EXPECT_EQ(1.0, z.y);
}

TEST(TestMath, complexLogZeroIsZero)
{
    DComplex x{0.0, 0.0};
    DComplex z{1.0, 1.0};

    fpu_cmplx_log(&x, &z); // z = log(x)

    EXPECT_EQ(0.0, z.x);
    EXPECT_EQ(0.0, z.y);
}

TEST(TestMath, complexLogReal)
{
    DComplex x{5.0, 0.0};
    DComplex z{1.0, 1.0};

    fpu_cmplx_log(&x, &z); // z = log(x)

    EXPECT_NEAR(std::log(5.0), z.x, 1e-9);
    EXPECT_EQ(0.0, z.y);
}

TEST(TestMath, complexLogPointerAlias)
{
    DComplex z{1.0, 2.0};

    fpu_cmplx_log(&z, &z); // z = log(z)

    EXPECT_NEAR(std::log(std::sqrt(5)), z.x, 1e-9);
    EXPECT_NEAR(std::atan2(2.0, 1.0), z.y, 1e-9);
}
