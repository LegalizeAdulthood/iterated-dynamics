// SPDX-License-Identifier: GPL-3.0-only
//
#include <ui/double_to_string.h>

#include <gtest/gtest.h>

namespace id::ui
{

TEST(TestDoubleToString, usesSixteenSignificantDigitsWhenShortEnough)
{
    EXPECT_EQ("0.3333333333333333", double_to_string(1.0 / 3.0));
}

TEST(TestDoubleToString, keepsCellularPrecision)
{
    EXPECT_EQ("1234567890123456", double_to_string(1234567890123456.0));
}

TEST(TestDoubleToString, shortensLongExponentOutput)
{
    const std::string text{double_to_string(1.2345678901234567e+30)};

    EXPECT_LE(text.length(), 20U);
    EXPECT_EQ("1.23456789012346e+30", text);
}

} // namespace id::ui
