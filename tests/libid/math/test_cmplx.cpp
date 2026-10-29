// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <math/cmplx.h>

#include <gtest/gtest.h>

#include <cmath>

using namespace id::math;

namespace id::test
{

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

class TestComplexOperands : public testing::Test
{
protected:
    DComplex lhs{1.0, 2.0};
    const DComplex rhs{2.0, 1.0};
    const double scalar{2.0};
};

TEST_F(TestComplexOperands, prefixPlus)
{
    const DComplex result = +lhs;

    EXPECT_EQ(lhs.x, result.x);
    EXPECT_EQ(lhs.y, result.y);
}

TEST_F(TestComplexOperands, prefixMinus)
{
    const DComplex result = -lhs;

    EXPECT_EQ(-lhs.x, result.x);
    EXPECT_EQ(-lhs.y, result.y);
}

TEST_F(TestComplexOperands, addAssign)
{
    lhs += rhs;

    EXPECT_EQ(3.0, lhs.x);
    EXPECT_EQ(3.0, lhs.y);
}

TEST_F(TestComplexOperands, subtractAssign)
{
    lhs -= rhs;

    EXPECT_EQ(-1.0, lhs.x);
    EXPECT_EQ(1.0, lhs.y);
}

TEST_F(TestComplexOperands, scalarMultiplyAssign)
{
    lhs *= scalar;

    EXPECT_EQ(2.0, lhs.x);
    EXPECT_EQ(4.0, lhs.y);
}

TEST_F(TestComplexOperands, add)
{
    const DComplex result = lhs + rhs;

    EXPECT_EQ(3.0, result.x);
    EXPECT_EQ(3.0, result.y);
}

TEST_F(TestComplexOperands, subtract)
{
    const DComplex result = lhs - rhs;

    EXPECT_EQ(-1.0, result.x);
    EXPECT_EQ(1.0, result.y);
}

TEST_F(TestComplexOperands, scalarMultiplyRight)
{
    const DComplex result = lhs * scalar;

    EXPECT_EQ(2.0, result.x);
    EXPECT_EQ(4.0, result.y);
}

TEST_F(TestComplexOperands, scalarMultiplyLeft)
{
    const DComplex result = scalar * lhs;

    EXPECT_EQ(2.0, result.x);
    EXPECT_EQ(4.0, result.y);
}

TEST(TestComplex, powerPropagatesInfiniteLog)
{
    const DComplex result = pow({INFINITY, 1.0}, {1.0, 0.0});

    EXPECT_TRUE(std::isinf(result.x));
    EXPECT_EQ(0.0, result.y);
}

} // namespace id::test
