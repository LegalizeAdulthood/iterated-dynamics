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
        StringCaseCountedComparisonInput{"prefixIsLessCount", "frac", "fractint", 8, -1}),
    [](const testing::TestParamInfo<StringCaseCountedComparisonInput> &info) { return info.param.name; });

struct StringCaseUnboundedComparisonInput
{
    const char *name;
    const char *left;
    const char *right;
    int expected;
};

void PrintTo(const StringCaseUnboundedComparisonInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestStringCaseCompare : public testing::Test,
                              public testing::WithParamInterface<StringCaseUnboundedComparisonInput>
{
};

TEST_P(TestStringCaseCompare, comparesOrdering)
{
    const int result{string_case_compare(GetParam().left, GetParam().right)};

    EXPECT_EQ(GetParam().expected, (result > 0) - (result < 0));
}

INSTANTIATE_TEST_SUITE_P(Operands, TestStringCaseCompare,
    testing::Values(StringCaseUnboundedComparisonInput{"firstStringEmptyEqual", "", "foo", -1},
        StringCaseUnboundedComparisonInput{"secondStringEmptyNotEqual", "foo", "", 1},
        StringCaseUnboundedComparisonInput{"firstStringExhaustedEqual", "foo", "foobar", -1},
        StringCaseUnboundedComparisonInput{"secondStringExhaustedGreater", "foobar", "foo", 1},
        StringCaseUnboundedComparisonInput{"zeroLengthAlwaysEqual", "", "", 0},
        StringCaseUnboundedComparisonInput{"differOnlyByCaseEqual", "foo", "FoO", 0},
        StringCaseUnboundedComparisonInput{"differNotZero", "foo", "bar", 1},
        StringCaseUnboundedComparisonInput{"lessNegative", "bar", "foo", -1}),
    [](const testing::TestParamInfo<StringCaseUnboundedComparisonInput> &info) { return info.param.name; });

struct StringCaseCountedEqualityInput
{
    const char *name;
    const char *left;
    const char *right;
    std::size_t count;
    bool expected;
};

void PrintTo(const StringCaseCountedEqualityInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestStringCaseEqualCount : public CountedStringOperands,
                                 public testing::WithParamInterface<StringCaseCountedEqualityInput>
{
protected:
    void SetUp() override;
};

void TestStringCaseEqualCount::SetUp()
{
    set_operands(GetParam().left, GetParam().right);
}

TEST_P(TestStringCaseEqualCount, comparesEquality)
{
    const bool result{string_case_equal(m_left.data(), m_right.data(), GetParam().count)};

    EXPECT_EQ(GetParam().expected, result);
}

INSTANTIATE_TEST_SUITE_P(Operands, TestStringCaseEqualCount,
    testing::Values(StringCaseCountedEqualityInput{"firstStringEmpty", "", "foo", 80, false},
        StringCaseCountedEqualityInput{"secondStringEmpty", "foo", "", 80, false},
        StringCaseCountedEqualityInput{"firstStringExhaustedEqual", "foo", "foobar", 3, true},
        StringCaseCountedEqualityInput{"secondStringExhaustedEqual", "foobar", "foo", 3, true},
        StringCaseCountedEqualityInput{"zeroLengthAlwaysEqual", "foo", "bar", 0, true},
        StringCaseCountedEqualityInput{"differOnlyByCaseEqual", "foo", "FoO", 3, true},
        StringCaseCountedEqualityInput{"differNotZero", "foo", "bar", 3, false},
        StringCaseCountedEqualityInput{"lessNegative", "bar", "foo", 3, false},
        StringCaseCountedEqualityInput{"prefixIsLessCount", "frac", "fractint", 8, false}),
    [](const testing::TestParamInfo<StringCaseCountedEqualityInput> &info) { return info.param.name; });

struct StringCaseUnboundedEqualityInput
{
    const char *name;
    const char *left;
    const char *right;
    bool expected;
};

void PrintTo(const StringCaseUnboundedEqualityInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestStringCaseEqual : public testing::Test, public testing::WithParamInterface<StringCaseUnboundedEqualityInput>
{
};

TEST_P(TestStringCaseEqual, comparesEquality)
{
    const bool result{string_case_equal(GetParam().left, GetParam().right)};

    EXPECT_EQ(GetParam().expected, result);
}

INSTANTIATE_TEST_SUITE_P(Operands, TestStringCaseEqual,
    testing::Values(StringCaseUnboundedEqualityInput{"firstStringEmptyEqual", "", "foo", false},
        StringCaseUnboundedEqualityInput{"secondStringEmptyNotEqual", "foo", "", false},
        StringCaseUnboundedEqualityInput{"firstStringExhaustedEqual", "foo", "foobar", false},
        StringCaseUnboundedEqualityInput{"secondStringExhaustedGreater", "foobar", "foo", false},
        StringCaseUnboundedEqualityInput{"zeroLengthAlwaysEqual", "", "", true},
        StringCaseUnboundedEqualityInput{"differOnlyByCaseEqual", "foo", "FoO", true},
        StringCaseUnboundedEqualityInput{"differNotZero", "foo", "bar", false},
        StringCaseUnboundedEqualityInput{"lessNegative", "bar", "foo", false}),
    [](const testing::TestParamInfo<StringCaseUnboundedEqualityInput> &info) { return info.param.name; });

} // namespace id::test
