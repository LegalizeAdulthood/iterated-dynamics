// SPDX-License-Identifier: GPL-3.0-only
//
#include <engine/bailout_formula.h>
#include <engine/UserData.h>
#include <fractals/fractype.h>
#include <math/big.h>
#include <math/biginit.h>
#include <misc/ValueSaver.h>

#include <gtest/gtest.h>

#include <array>
#include <string>

using namespace id::engine;
using namespace id::fractals;
using namespace id::math;
using namespace id::misc;

namespace id::test
{

class TestBigFloat : public testing::Test
{
protected:
    TestBigFloat() :
        m_saved_bailout_value(g_user.bailout_value, 0),
        m_saved_bailout_test(g_bailout_test, Bailout::MOD),
        m_saved_bf_digits(g_bf_digits, 0),
        m_saved_fractal_type(g_fractal_type, FractalType::MANDEL)
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

    static std::string to_string(char *(*format)(char *, BigFloat, int), BigFloat value, const int decimals)
    {
        std::array<char, 128> text{};

        format(text.data(), value, decimals);

        return text.data();
    }

    ValueSaver<long> m_saved_bailout_value;
    ValueSaver<Bailout> m_saved_bailout_test;
    ValueSaver<int> m_saved_bf_digits;
    ValueSaver<FractalType> m_saved_fractal_type;
};

TEST_F(TestBigFloat, parsesPlainDecimal)
{
    BigFloat value{alloc_stack(g_bf_length + 2)};

    str_to_bf(value, "1.25");

    EXPECT_EQ(1.25L, bf_to_float(value));
}

TEST_F(TestBigFloat, parsesExponent)
{
    BigFloat value{alloc_stack(g_bf_length + 2)};

    str_to_bf(value, "1.25e2");

    EXPECT_EQ(125.0L, bf_to_float(value));
}

TEST_F(TestBigFloat, formatsZero)
{
    BigFloat value{alloc_stack(g_bf_length + 2)};
    clear_bf(value);

    EXPECT_EQ("0.0", to_string(bf_to_str, value, 5));
    EXPECT_EQ("0.0", to_string(bf_to_str_e, value, 5));
    EXPECT_EQ("0.0", to_string(bf_to_str_f, value, 5));
}

TEST_F(TestBigFloat, formatsFixedDecimal)
{
    BigFloat value{alloc_stack(g_bf_length + 2)};
    str_to_bf(value, "1.25");

    EXPECT_EQ("1.25", to_string(bf_to_str_f, value, 5));
}

TEST_F(TestBigFloat, formatsScientificDecimal)
{
    BigFloat value{alloc_stack(g_bf_length + 2)};
    str_to_bf(value, "1.25");

    EXPECT_EQ("1.25e0", to_string(bf_to_str_e, value, 5));
}

} // namespace id::test
