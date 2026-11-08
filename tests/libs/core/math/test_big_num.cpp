// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <math/big.h>
#include <math/big_init.h>
#include <misc/ValueSaver.h>

#include <gtest/gtest.h>

#include <string>
#include <vector>

using namespace id::math;
using namespace id::misc;

namespace id::test
{

class TestBigNum : public testing::Test
{
protected:
    TestBigNum() :
        m_saved_bf_digits(g_bf_digits, 0)
    {
    }

    void SetUp() override
    {
        init_bf_dec(20);
    }

    void TearDown() override
    {
        free_bf_vars();
    }

    std::string round_trip(char *input, const int decimals)
    {
        BigNumPtr value{bn_stack().allocate_single()};
        str_to_bn(value, input);
        return format(value, decimals);
    }

    static std::string format(BigNumPtr value, const int decimals)
    {
        return bn_to_string(value, decimals);
    }

    ValueSaver<int> m_saved_bf_digits;
};

struct BigNumFormatInput
{
    const char *name;
    const char *input;
    int decimals;
    const char *expected;
};

class TestBigNumFormatting : public TestBigNum, public testing::WithParamInterface<BigNumFormatInput>
{
};

TEST_P(TestBigNumFormatting, formatsParsedValue)
{
    std::string input{GetParam().input};

    EXPECT_EQ(GetParam().expected, round_trip(input.data(), GetParam().decimals));
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestBigNumFormatting,
    testing::Values(BigNumFormatInput{"Integer", "42", 5, "42."},
        BigNumFormatInput{"PositiveFraction", "12.5", 5, "12.5"},
        BigNumFormatInput{"TrailingFractionalZeros", "12.500", 5, "12.5"}),
    [](const testing::TestParamInfo<BigNumFormatInput> &info) { return info.param.name; });

TEST_F(TestBigNum, formatsZeroValue)
{
    BigNumPtr value{bn_stack().allocate_single()};
    int_to_bn(value, 0);

    EXPECT_EQ("0.", format(value, 5));
}

TEST_F(TestBigNum, conversionPreservesFixedPointBytePositionsAndTruncation)
{
    const int active_length = g_bn_length;
    const int active_integer_length = g_int_length;
    std::vector<Byte> source{0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0xfe};
    const auto original = source;

    struct Case
    {
        int length;
        int integer_length;
        std::vector<Byte> expected;
    };

    const Case cases[]{
        {12, 2, {0, 0, 0, 0, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0xfe}},
        {4, 2, {0x55, 0x66, 0x77, 0xfe}},
        {8, 2, original},
        {12, 4, {0, 0, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0xfe, 0, 0}},
        {8, 1, {0, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77}},
    };
    for (const auto &test : cases)
    {
        SCOPED_TRACE(test.length);
        SCOPED_TRACE(test.integer_length);
        std::vector<Byte> destination(test.expected.size() + 2, 0xcc);

        EXPECT_EQ(0, convert_bn(destination.data() + 1, source.data(), test.length, test.integer_length, 8, 2));

        EXPECT_EQ(test.expected, (std::vector<Byte>{destination.begin() + 1, destination.end() - 1}));
        EXPECT_EQ(0xcc, destination.front());
        EXPECT_EQ(0xcc, destination.back());
        EXPECT_EQ(original, source);
        EXPECT_EQ(active_length, g_bn_length);
        EXPECT_EQ(active_integer_length, g_int_length);
    }
}

} // namespace id::test
