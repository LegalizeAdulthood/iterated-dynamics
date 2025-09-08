// SPDX-License-Identifier: GPL-3.0-only
//
#include <engine/get_prec_big_float.h>

#include <gtest/gtest.h>

#include <limits>

using namespace id::engine;

namespace id::test
{

TEST(TestGetMagnificationPrecision, basicAssertions)
{
    EXPECT_EQ(get_magnification_precision(1.0), 4);
    EXPECT_EQ(get_magnification_precision(10.0), 5);
    EXPECT_EQ(get_magnification_precision(100.0), 6);
    EXPECT_EQ(get_magnification_precision(0.1), 3);
    EXPECT_EQ(get_magnification_precision(0.01), 2);
}

TEST(TestGetMagnificationPrecision, powersOfTwo)
{
    EXPECT_EQ(get_magnification_precision(2.0), 4);
    EXPECT_EQ(get_magnification_precision(4.0), 4);
    EXPECT_EQ(get_magnification_precision(8.0), 4);
    EXPECT_EQ(get_magnification_precision(16.0), 5);
    EXPECT_EQ(get_magnification_precision(32.0), 5);
}

TEST(TestGetMagnificationPrecision, edgeCases)
{
    EXPECT_EQ(get_magnification_precision(0.0), 4);
    EXPECT_EQ(get_magnification_precision(std::numeric_limits<double>::min()), -304);
    EXPECT_EQ(get_magnification_precision(std::numeric_limits<double>::max()), 312);
    EXPECT_EQ(get_magnification_precision(std::numeric_limits<double>::epsilon()), -12);
}

} // namespace id::test
