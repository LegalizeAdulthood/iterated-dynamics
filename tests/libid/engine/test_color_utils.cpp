// SPDX-License-Identifier: GPL-3.0-only
//
#include <engine/color_utils.h>

#include <gtest/gtest.h>

using namespace id::engine;

namespace id::test
{

TEST(TestColorUtils, expand6BitColor)
{
    EXPECT_EQ(0, expand_6bit_color(0));
    EXPECT_EQ(4, expand_6bit_color(1));
    EXPECT_EQ(85, expand_6bit_color(21));
    EXPECT_EQ(170, expand_6bit_color(42));
    EXPECT_EQ(255, expand_6bit_color(63));
}

TEST(TestColorUtils, expand8BitColor)
{
    EXPECT_EQ(0, expand_8bit_color(0));
    EXPECT_EQ(4, expand_8bit_color(4));
    EXPECT_EQ(85, expand_8bit_color(84));
    EXPECT_EQ(170, expand_8bit_color(168));
    EXPECT_EQ(255, expand_8bit_color(252));
}

} // namespace id::test
