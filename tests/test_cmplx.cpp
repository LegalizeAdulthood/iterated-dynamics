// SPDX-License-Identifier: GPL-3.0-only
//
#include <port.h>
#include <cmplx.h>

#include <gtest/gtest.h>

TEST(TestComplex, zeroConstruct)
{
    const DComplex zero{};

    EXPECT_EQ(0.0, zero.x);
    EXPECT_EQ(0.0, zero.y);
}

TEST(TestComplex, valueConstruct)
{
    const DComplex val{1.0, 2.0};

    EXPECT_EQ(1.0, val.x);
    EXPECT_EQ(2.0, val.y);
}

TEST(TestComplex, prefixPlus)
{
    const DComplex val{1.0, 2.0};

    const DComplex result = +val;

    EXPECT_EQ(val.x, result.x);
    EXPECT_EQ(val.y, result.y);
}

TEST(TestComplex, prefixMinus)
{
    const DComplex val{1.0, 2.0};

    const DComplex result = -val;

    EXPECT_EQ(-val.x, result.x);
    EXPECT_EQ(-val.y, result.y);
}

TEST(TestComplex, addAssign)
{
    DComplex lhs{1.0, 2.0};
    const DComplex rhs{2.0, 1.0};

    lhs += rhs;

    EXPECT_EQ(3.0, lhs.x);
    EXPECT_EQ(3.0, lhs.y);
}

TEST(TestComplex, subtractAssign)
{
    DComplex lhs{1.0, 2.0};
    const DComplex rhs{2.0, 1.0};

    lhs -= rhs;

    EXPECT_EQ(-1.0, lhs.x);
    EXPECT_EQ(1.0, lhs.y);
}

TEST(TestComplex, scalarMultiplyAssign)
{
    DComplex lhs{1.0, 2.0};

    lhs *= 2.0;

    EXPECT_EQ(2.0, lhs.x);
    EXPECT_EQ(4.0, lhs.y);
}

TEST(TestComplex, add)
{
    const DComplex lhs{1.0, 2.0};
    const DComplex rhs{2.0, 1.0};

    const DComplex result = lhs + rhs;

    EXPECT_EQ(3.0, result.x);
    EXPECT_EQ(3.0, result.y);
}

TEST(TestComplex, subtract)
{
    const DComplex lhs{1.0, 2.0};
    const DComplex rhs{2.0, 1.0};

    const DComplex result = lhs - rhs;

    EXPECT_EQ(-1.0, result.x);
    EXPECT_EQ(1.0, result.y);
}

TEST(TestComplex, scalarMultiplyRight)
{
    const DComplex lhs{1.0, 2.0};

    const DComplex result = lhs * 2.0;

    EXPECT_EQ(2.0, result.x);
    EXPECT_EQ(4.0, result.y);
}

TEST(TestComplex, scalarMultiplyLeft)
{
    const DComplex lhs{1.0, 2.0};

    const DComplex result = 2.0 * lhs;

    EXPECT_EQ(2.0, result.x);
    EXPECT_EQ(4.0, result.y);
}
