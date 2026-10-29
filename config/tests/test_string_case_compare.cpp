// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <config/string_case_compare.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <ostream>
#include <string_view>

using namespace id::config;

namespace id::test
{

class CountedStringOperands : public testing::Test
{
protected:
    void set_operands(std::string_view left, std::string_view right);

    std::array<char, 80> m_left{};
    std::array<char, 80> m_right{};
};

void CountedStringOperands::set_operands(const std::string_view left, const std::string_view right)
{
    std::copy(left.begin(), left.end(), m_left.begin());
    std::copy(right.begin(), right.end(), m_right.begin());
}

struct StringCaseCountedComparisonInput
{
    const char *name;
    const char *left;
    const char *right;
    std::size_t count;
    int expected;
};

void PrintTo(const StringCaseCountedComparisonInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestStringCaseCompareCount : public CountedStringOperands,
                                   public testing::WithParamInterface<StringCaseCountedComparisonInput>
{
protected:
    void SetUp() override;
};

void TestStringCaseCompareCount::SetUp()
{
    set_operands(GetParam().left, GetParam().right);
}

TEST_P(TestStringCaseCompareCount, comparesOrdering)
{
    const int result{string_case_compare(m_left.data(), m_right.data(), GetParam().count)};

    EXPECT_EQ(GetParam().expected, (result > 0) - (result < 0));
}

INSTANTIATE_TEST_SUITE_P(Operands, TestStringCaseCompareCount,
    testing::Values(StringCaseCountedComparisonInput{"firstStringEmpty", "", "foo", 80, -1},
        StringCaseCountedComparisonInput{"secondStringEmpty", "foo", "", 80, 1},
        StringCaseCountedComparisonInput{"firstStringExhaustedEqual", "foo", "foobar", 3, 0},
        StringCaseCountedComparisonInput{"secondStringExhaustedEqual", "foobar", "foo", 3, 0},
        StringCaseCountedComparisonInput{"zeroLengthAlwaysEqual", "foo", "bar", 0, 0},
        StringCaseCountedComparisonInput{"differOnlyByCaseEqual", "foo", "FoO", 3, 0},
        StringCaseCountedComparisonInput{"differNotZero", "foo", "bar", 3, 1},
        StringCaseCountedComparisonInput{"lessNegative", "bar", "foo", 3, -1},
        StringCaseCountedComparisonInput{"greaterPositive", "foo", "bar", 3, 1},
        StringCaseCountedComparisonInput{"prefixIsLessCount", "frac", "fractint", 8, -1}),
    [](const testing::TestParamInfo<StringCaseCountedComparisonInput> &info) { return info.param.name; });

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
