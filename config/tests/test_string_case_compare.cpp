// SPDX-License-Identifier: GPL-3.0-only
//
#include <config/string_case_compare.h>

#include <gtest/gtest.h>

using namespace id::config;

namespace id::test
{

TEST(TestStringCaseCompareCount, firstStringEmpty)
{
    constexpr char buffer[80]{"foo"};

    EXPECT_GT(0, string_case_compare("", buffer, sizeof(buffer)));
}

TEST(TestStringCaseCompareCount, secondStringEmpty)
{
    constexpr char buffer[80]{"foo"};

    EXPECT_LT(0, string_case_compare(buffer, "", sizeof(buffer)));
}

TEST(TestStringCaseCompareCount, firstStringExhaustedEqual)
{
    constexpr char buffer[]{"foo"};

    EXPECT_EQ(0, string_case_compare(buffer, "foobar", sizeof(buffer) - 1));
}

TEST(TestStringCaseCompareCount, secondStringExhaustedEqual)
{
    constexpr char buffer[]{"foo"};

    EXPECT_EQ(0, string_case_compare("foobar", buffer, sizeof(buffer) - 1));
}

TEST(TestStringCaseCompareCount, zeroLengthAlwaysEqual)
{
    EXPECT_EQ(0, string_case_compare("foo", "bar", 0));
}

TEST(TestStringCaseCompareCount, differOnlyByCaseEqual)
{
    EXPECT_EQ(0, string_case_compare("foo", "FoO", 3));
}

TEST(TestStringCaseCompareCount, differNotZero)
{
    EXPECT_LT(0, string_case_compare("foo", "bar", 3));
}

TEST(TestStringCaseCompareCount, lessNegative)
{
    EXPECT_GT(0, string_case_compare("bar", "foo", 3));
}

TEST(TestStringCaseCompareCount, greaterPositive)
{
    EXPECT_LT(0, string_case_compare("foo", "bar", 3));
}

TEST(TestStringCaseCompareCount, prefixIsLessCount)
{
    EXPECT_GT(0, string_case_compare("frac", "fractint", 8));
}

TEST(TestStringCaseCompare, firstStringEmptyEqual)
{
    EXPECT_GT(0, string_case_compare("", "foo"));
}

TEST(TestStringCaseCompare, secondStringEmptyNotEqual)
{
    EXPECT_LT(0, string_case_compare("foo", ""));
}

TEST(TestStringCaseCompare, firstStringExhaustedEqual)
{
    EXPECT_GT(0, string_case_compare("foo", "foobar"));
}

TEST(TestStringCaseCompare, secondStringExhaustedGreater)
{
    EXPECT_LT(0, string_case_compare("foobar", "foo"));
}

TEST(TestStringCaseCompare, zeroLengthAlwaysEqual)
{
    EXPECT_EQ(0, string_case_compare("", ""));
}

TEST(TestStringCaseCompare, differOnlyByCaseEqual)
{
    EXPECT_EQ(0, string_case_compare("foo", "FoO"));
}

TEST(TestStringCaseCompare, differNotZero)
{
    EXPECT_LT(0, string_case_compare("foo", "bar"));
}

TEST(TestStringCaseCompare, lessNegative)
{
    EXPECT_GT(0, string_case_compare("bar", "foo"));
}

TEST(TestStringCaseCompare, greaterPositive)
{
    EXPECT_LT(0, string_case_compare("foo", "bar"));
}

TEST(TestStringCaseCompare, prefixIsLess)
{
    EXPECT_GT(0, string_case_compare("frac", "fractint"));
}

TEST(TestStringCaseEqualCount, firstStringEmpty)
{
    constexpr char buffer[80]{"foo"};

    EXPECT_FALSE(string_case_equal("", buffer, sizeof(buffer)));
}

TEST(TestStringCaseEqualCount, secondStringEmpty)
{
    constexpr char buffer[80]{"foo"};

    EXPECT_FALSE(string_case_equal(buffer, "", sizeof(buffer)));
}

TEST(TestStringCaseEqualCount, firstStringExhaustedEqual)
{
    constexpr char buffer[]{"foo"};

    EXPECT_TRUE(string_case_equal(buffer, "foobar", sizeof(buffer) - 1));
}

TEST(TestStringCaseEqualCount, secondStringExhaustedEqual)
{
    constexpr char buffer[]{"foo"};

    EXPECT_TRUE(string_case_equal("foobar", buffer, sizeof(buffer) - 1));
}

TEST(TestStringCaseEqualCount, zeroLengthAlwaysEqual)
{
    EXPECT_TRUE(string_case_equal("foo", "bar", 0));
}

TEST(TestStringCaseEqualCount, differOnlyByCaseEqual)
{
    EXPECT_TRUE(string_case_equal("foo", "FoO", 3));
}

TEST(TestStringCaseEqualCount, differNotZero)
{
    EXPECT_FALSE(string_case_equal("foo", "bar", 3));
}

TEST(TestStringCaseEqualCount, lessNegative)
{
    EXPECT_FALSE(string_case_equal("bar", "foo", 3));
}

TEST(TestStringCaseEqualCount, greaterPositive)
{
    EXPECT_FALSE(string_case_equal("foo", "bar", 3));
}

TEST(TestStringCaseEqualCount, prefixIsLessCount)
{
    EXPECT_FALSE(string_case_equal("frac", "fractint", 8));
}

TEST(TestStringCaseEqual, firstStringEmptyEqual)
{
    EXPECT_FALSE(string_case_equal("", "foo"));
}

TEST(TestStringCaseEqual, secondStringEmptyNotEqual)
{
    EXPECT_FALSE(string_case_equal("foo", ""));
}

TEST(TestStringCaseEqual, firstStringExhaustedEqual)
{
    EXPECT_FALSE(string_case_equal("foo", "foobar"));
}

TEST(TestStringCaseEqual, secondStringExhaustedGreater)
{
    EXPECT_FALSE(string_case_equal("foobar", "foo"));
}

TEST(TestStringCaseEqual, zeroLengthAlwaysEqual)
{
    EXPECT_TRUE(string_case_equal("", ""));
}

TEST(TestStringCaseEqual, differOnlyByCaseEqual)
{
    EXPECT_TRUE(string_case_equal("foo", "FoO"));
}

TEST(TestStringCaseEqual, differNotZero)
{
    EXPECT_FALSE(string_case_equal("foo", "bar"));
}

TEST(TestStringCaseEqual, lessNegative)
{
    EXPECT_FALSE(string_case_equal("bar", "foo"));
}

TEST(TestStringCaseEqual, greaterPositive)
{
    EXPECT_FALSE(string_case_equal("foo", "bar"));
}

TEST(TestStringCaseEqual, prefixIsLess)
{
    EXPECT_FALSE(string_case_equal("frac", "fractint"));
}

} // namespace id::test
