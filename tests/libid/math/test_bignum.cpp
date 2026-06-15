// SPDX-License-Identifier: GPL-3.0-only
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

TEST_F(TestBigNum, formatsIntegerWithDecimalPoint)
{
    char input[]{"42"};

    EXPECT_EQ("42.", round_trip(input, 5));
}

TEST_F(TestBigNum, formatsPositiveFractionalInput)
{
    char input[]{"12.5"};

    EXPECT_EQ("12.5", round_trip(input, 5));
}

TEST_F(TestBigNum, formatsZeroValue)
{
    BigNum value{alloc_stack(g_bn_length)};
    int_to_bn(value, 0);

    EXPECT_EQ("0.", format(value, 5));
}

TEST_F(TestBigNum, trimsTrailingFractionalZeros)
{
    char input[]{"12.500"};

    EXPECT_EQ("12.5", round_trip(input, 5));
}

} // namespace id::test
