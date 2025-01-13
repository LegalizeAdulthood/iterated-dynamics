// SPDX-License-Identifier: GPL-3.0-only
//`
#include <config/string_case_compare.h>

#include <gtest/gtest.h>

TEST(TestStringCaseCompare, firstStringEmpty)
{
    char buffer[80]{"foo"};
    
    EXPECT_NE(0, string_case_compare("", buffer, sizeof(buffer)));
}

TEST(TestStringCaseCompare, secondStringEmpty)
{
    char buffer[80]{"foo"};
    
    EXPECT_NE(0, string_case_compare(buffer, "", sizeof(buffer)));
}

TEST(TestStringCaseCompare, firstStringExhaustedEqual)
{
    char buffer[]{"foo"};
    
    EXPECT_EQ(0, string_case_compare(buffer, "foobar", sizeof(buffer) - 1));
}

TEST(TestStringCaseCompare, secondStringExhaustedEqual)
{
    char buffer[]{"foo"};
    
    EXPECT_EQ(0, string_case_compare("foobar", buffer, sizeof(buffer) - 1));
}

TEST(TestStringCaseCompare, zeroLengthAlwaysEqual)
{
    EXPECT_EQ(0, string_case_compare("foo", "bar", 0));
}

TEST(TestStringCaseCompare, differOnlyByCaseEqual)
{
    EXPECT_EQ(0, string_case_compare("foo", "FoO", 3));
}

TEST(TestStringCaseCompare, differNotZero)
{
    EXPECT_NE(0, string_case_compare("foo", "bar", 3));
}

TEST(TestStringCaseCompare, lessNegative)
{
    EXPECT_GT(0, string_case_compare("bar", "foo", 3));
}

TEST(TestStringCaseCompare, greaterPositive)
{
    EXPECT_LT(0, string_case_compare("foo", "bar", 3));
}
