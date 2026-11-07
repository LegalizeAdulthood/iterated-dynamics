// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <math/big.h>
#include <math/biginit.h>
#include <misc/ValueSaver.h>

#include <gtest/gtest.h>

#include <string>

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
        BigNum value{alloc_stack(g_bn_length)};
        str_to_bn(value, input);
        return format(value, decimals);
    }

    static std::string format(BigNum value, const int decimals)
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
    BigNum value{alloc_stack(g_bn_length)};
    int_to_bn(value, 0);

    EXPECT_EQ("0.", format(value, 5));
}

} // namespace id::test
