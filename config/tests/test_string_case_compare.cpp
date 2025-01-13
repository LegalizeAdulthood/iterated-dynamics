// SPDX-License-Identifier: GPL-3.0-only
//`
#include <config/string_case_compare.h>

#include <gtest/gtest.h>

TEST(TestStringCaseCompare, firstStringEmpty)
{
    char buffer[80]{"foo"};
    
    EXPECT_NE(0, strncasecmp("", buffer, sizeof(buffer)));
}

TEST(TestStringCaseCompare, secondStringEmpty)
{
    char buffer[80]{"foo"};
    
    EXPECT_NE(0, strncasecmp(buffer, "", sizeof(buffer)));
}

TEST(TestStringCaseCompare, firstStringExhaustedEqual)
{
    char buffer[]{"foo"};
    
    EXPECT_EQ(0, strncasecmp(buffer, "foobar", sizeof(buffer) - 1));
}

TEST(TestStringCaseCompare, secondStringExhaustedEqual)
{
    char buffer[]{"foo"};
    
    EXPECT_EQ(0, strncasecmp("foobar", buffer, sizeof(buffer) - 1));
}

TEST(TestStringCaseCompare, zeroLengthAlwaysEqual)
{
    EXPECT_EQ(0, strncasecmp("foo", "bar", 0));
}

TEST(TestStringCaseCompare, differOnlyByCaseEqual)
{
    EXPECT_EQ(0, strncasecmp("foo", "FoO", 3));
}

TEST(TestStringCaseCompare, differNotZero)
{
    EXPECT_NE(0, strncasecmp("foo", "bar", 3));
}

TEST(TestStringCaseCompare, lessNegative)
{
    EXPECT_GT(0, strncasecmp("bar", "foo", 3));
}

TEST(TestStringCaseCompare, greaterPositive)
{
    EXPECT_LT(0, strncasecmp("foo", "bar", 3));
}
