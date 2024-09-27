// SPDX-License-Identifier: GPL-3.0-only
//
#include <mpmath.h>

#include <value_saver.h>

#include <gtest/gtest.h>

#include <complex>

using namespace testing;

inline long fixed_point(long value)
{
    return value << g_bit_shift;
}

namespace
{

class TestMathLComplex : public Test
{
protected:
    ValueSaver<int> m_saved_bit_shift{g_bit_shift, 16};
    Arg arg1{};
    Arg arg2{};
    ValueSaver<Arg *> m_saved_arg1{g_arg1, &arg1};
    ValueSaver<Arg *> m_saved_arg2{g_arg2, &arg2};
};

} // namespace

TEST_F(TestMathLComplex, add)
{
    const LComplex lhs{fixed_point(1), fixed_point(2)};
    const LComplex rhs{fixed_point(3), fixed_point(4)};

    const LComplex result{lhs + rhs};

    EXPECT_EQ(fixed_point(4), result.x);
    EXPECT_EQ(fixed_point(6), result.y);
}

TEST_F(TestMathLComplex, subtract)
{
    const LComplex lhs{fixed_point(1), fixed_point(2)};
    const LComplex rhs{fixed_point(3), fixed_point(4)};

    const LComplex result{lhs - rhs};

    EXPECT_EQ(-fixed_point(2), result.x);
    EXPECT_EQ(-fixed_point(2), result.y);
}

TEST_F(TestMathLComplex, multiplyReal)
{
    const LComplex lhs{fixed_point(1), fixed_point(2)};

    LComplex result{};
    LCMPLXtimesreal(lhs, fixed_point(3), result);

    EXPECT_EQ(fixed_point(3), result.x);
    EXPECT_EQ(fixed_point(6), result.y);
}

TEST_F(TestMathLComplex, multiplyOperator)
{
    const LComplex lhs{fixed_point(1), fixed_point(2)};
    const LComplex rhs{fixed_point(3), fixed_point(4)};

    LComplex result{lhs * rhs};

    const std::complex<float> expected{std::complex<float>(1.0f, 2.0f) * std::complex<float>(3.0f, 4.0f)};
    EXPECT_EQ(expected.real(), static_cast<float>(result.x >> g_bit_shift));
    EXPECT_EQ(expected.imag(), static_cast<float>(result.y >> g_bit_shift));
}

TEST_F(TestMathLComplex, multiplyStackFunction)
{
    const LComplex lhs{fixed_point(1), fixed_point(2)};
    const LComplex rhs{fixed_point(3), fixed_point(4)};

    g_arg1->l = lhs;
    g_arg2->l = rhs;
    lStkMul();
    g_arg2++;
    const LComplex result{g_arg2->l};

    const std::complex<float> expected{std::complex<float>(1.0f, 2.0f) * std::complex<float>(3.0f, 4.0f)};
    EXPECT_EQ(expected.real(), static_cast<float>(result.x >> g_bit_shift));
    EXPECT_EQ(expected.imag(), static_cast<float>(result.y >> g_bit_shift));
}
