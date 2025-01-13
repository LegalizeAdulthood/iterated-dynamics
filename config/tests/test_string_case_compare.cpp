// SPDX-License-Identifier: GPL-3.0-only
//`
#include <config/string_case_compare.h>

#include <gtest/gtest.h>

TEST(TestStringCaseCompareCount, firstStringEmpty)
{
    char buffer[80]{"foo"};
    
    EXPECT_GT(0, string_case_compare("", buffer, sizeof(buffer)));
}

TEST(TestStringCaseCompareCount, secondStringEmpty)
{
    char buffer[80]{"foo"};
    
    EXPECT_LT(0, string_case_compare(buffer, "", sizeof(buffer)));
}

TEST(TestStringCaseCompareCount, firstStringExhaustedEqual)
{
    char buffer[]{"foo"};
    
    EXPECT_EQ(0, string_case_compare(buffer, "foobar", sizeof(buffer) - 1));
}

TEST(TestStringCaseCompareCount, secondStringExhaustedEqual)
{
    char buffer[]{"foo"};
    
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
