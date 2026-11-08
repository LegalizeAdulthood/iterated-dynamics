// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/fractalb.h>

#include <engine/bailout_formula.h>
#include <engine/UserData.h>
#include <fractals/fractype.h>
#include <math/big_init.h>
#include <math/BigComplex.h>
#include <math/complex_fn.h>
#include <math/fixed_pt.h>
#include <misc/ValueSaver.h>

#include <gtest/gtest.h>

#include <cmath>
#include <ostream>
#include <type_traits>
#include <utility>

using namespace id::engine;
using namespace id::fractals;
using namespace id::math;
using namespace id::misc;

namespace id::math
{

void PrintTo(const BigFloat &value, std::ostream *out)
{
    *out << value.to_string(g_bf_decimals);
}

void PrintTo(const BigNum &value, std::ostream *out)
{
    *out << value.to_string(g_decimals);
}

template <typename T>
void PrintTo(const Complex<T> &value, std::ostream *out)
{
    *out << '(' << value.x.to_string(g_bf_decimals) << ", " << value.y.to_string(g_bf_decimals) << ')';
}

} // namespace id::math

namespace id::test
{
namespace
{

static_assert(std::is_same_v<BFConstPtrComplex, Complex<BigFloatConstPtr>>);
static_assert(!std::is_convertible_v<BFPtrComplex, BFComplex>);
static_assert(!std::is_convertible_v<BNPtrComplex, BNComplex>);

BFComplex owned(const BFPtrComplex &value)
{
    const auto size = static_cast<std::size_t>(g_bf_length + 2);
    return {BigFloat::from_legacy(value.x, size), BigFloat::from_legacy(value.y, size)};
}

template <typename T>
void check_owning_arithmetic()
{
    const Complex<T> left{T{1}, T{2}};
    const Complex<T> right{T{2}, T{-1}};
    EXPECT_EQ((Complex<T>{T{4}, T{3}}), left * right);
    // Preserve scalar division rounding, including fixed-point 5/5.
    EXPECT_EQ((Complex<T>{T{}, T{5} / T{5}}), left / right);
    EXPECT_EQ(T{5}, mag_squared(left));
    EXPECT_EQ((Complex<T>{T{2}, T{4}}), left * 2.0);
    EXPECT_EQ(left * 2.0, 2.0 * left);

    Complex<T> copy = left;
    EXPECT_EQ(&copy, &(copy *= copy));
    EXPECT_EQ((Complex<T>{T{-3}, T{4}}), copy);
    EXPECT_EQ(&copy, &(copy /= copy));
    EXPECT_EQ((Complex<T>{T{25} / T{25}, T{}}), copy);
    copy = left;
    copy += copy;
    EXPECT_EQ(left * 2.0, copy);
    copy -= copy;
    EXPECT_EQ(Complex<T>{}, copy);
    EXPECT_EQ((Complex<T>{T{1}, T{2}}), left);

    g_overflow = false;
    EXPECT_EQ(Complex<T>{}, left / Complex<T>{});
    EXPECT_TRUE(g_overflow);
    EXPECT_EQ((Complex<T>{T{-11}, T{-2}}), cube(left));
    EXPECT_EQ(cube(left), power(left, 3));
    EXPECT_EQ((Complex<T>{T{1}, T{0}}), power(left, -2));
}

} // namespace

class TestBigComplex : public testing::TestWithParam<int>
{
protected:
    TestBigComplex();
    void SetUp() override;
    void TearDown() override;
    BFPtrComplex legacy(LDouble real, LDouble imag);
    void expect_value(const BFPtrComplex &value, double real, double imag);
    void expect_precision(const BigFloat &expected, const BigFloat &actual);
    void expect_precision(const BFComplex &expected, const BFComplex &actual);

    ValueSaver<long> m_saved_bailout_value;
    ValueSaver<Bailout> m_saved_bailout_test;
    ValueSaver<int> m_saved_bf_digits;
    ValueSaver<FractalType> m_saved_fractal_type;
    ValueSaver<bool> m_saved_overflow;
};

TestBigComplex::TestBigComplex() :
    m_saved_bailout_value(g_user.bailout_value, 0),
    m_saved_bailout_test(g_bailout_test, Bailout::MOD),
    m_saved_bf_digits(g_bf_digits, 0),
    m_saved_fractal_type(g_fractal_type, FractalType::MANDEL_Z_POWER),
    m_saved_overflow(g_overflow, false)
{
}

void TestBigComplex::SetUp()
{
    init_bf_dec(GetParam());
    init_big_pi();
}

void TestBigComplex::TearDown()
{
    free_bf_vars();
}

BFPtrComplex TestBigComplex::legacy(const LDouble real, const LDouble imag)
{
    BFPtrComplex result{bf_stack().allocate_single(), bf_stack().allocate_single()};
    float_to_bf(result.x, real);
    float_to_bf(result.y, imag);
    return result;
}

void TestBigComplex::expect_value(const BFPtrComplex &value, const double real, const double imag)
{
    const DComplex actual = cmplx_bf_to_float(value);
    EXPECT_NEAR(real, actual.x, 1e-14);
    EXPECT_NEAR(imag, actual.y, 1e-14);
}

void TestBigComplex::expect_precision(const BigFloat &expected, const BigFloat &actual)
{
    EXPECT_EQ(expected.to_string(GetParam()), actual.to_string(GetParam()));
}

void TestBigComplex::expect_precision(const BFComplex &expected, const BFComplex &actual)
{
    expect_precision(expected.x, actual.x);
    expect_precision(expected.y, actual.y);
}

TEST_P(TestBigComplex, legacyProductAndQuotient)
{
    BFPtrComplex left = legacy(1.25L, -0.5L);
    BFPtrComplex right = legacy(-0.75L, 2);
    BFPtrComplex result = legacy(0, 0);

    EXPECT_EQ(&result, cmplx_mul_bf(&result, &left, &right));
    expect_value(result, 0.0625, 2.875);
    EXPECT_EQ(&result, cmplx_div_bf(&result, &left, &right));
    expect_value(result, -31.0 / 73.0, -34.0 / 73.0);
    EXPECT_FALSE(g_overflow);
    expect_value(left, 1.25, -0.5);
    expect_value(right, -0.75, 2);
}

TEST_P(TestBigComplex, legacyDivisionByZeroSetsOverflowAndReturnsZero)
{
    BFPtrComplex left = legacy(1, -2);
    BFPtrComplex zero = legacy(0, 0);
    BFPtrComplex result = legacy(7, 9);

    cmplx_div_bf(&result, &left, &zero);

    EXPECT_TRUE(g_overflow);
    expect_value(result, 0, 0);
}

TEST_P(TestBigComplex, legacyLogarithmBranches)
{
    BFPtrComplex result = legacy(0, 0);
    for (const DComplex base : {DComplex{}, DComplex{-1, 0}, DComplex{0, -1}, DComplex{-1, -1}})
    {
        BFPtrComplex input = legacy(base.x, base.y);
        cmplx_log_bf(&result, &input);
        const bool zero = base == DComplex{};
        expect_value(result, zero ? 0 : std::log(std::hypot(base.x, base.y)), zero ? 0 : std::atan2(base.y, base.x));
        expect_value(input, base.x, base.y);
    }
}

TEST_P(TestBigComplex, legacyZeroBasePowerAlwaysReturnsZero)
{
    BFPtrComplex base = legacy(0, 0);
    BFPtrComplex result = legacy(7, 9);
    for (const DComplex exponent : {DComplex{}, DComplex{-2, 0}, DComplex{1.25, -0.5}})
    {
        BFPtrComplex exp = legacy(exponent.x, exponent.y);
        cmplx_pow_bf(&result, &base, &exp);
        expect_value(result, 0, 0);
    }
}

TEST_P(TestBigComplex, legacyPowerHandlesNegativeAndComplexExponents)
{
    BFPtrComplex result = legacy(0, 0);
    for (const DComplex exponent : {DComplex{}, DComplex{-2, 0}, DComplex{0.25, -0.5}})
    {
        BFPtrComplex base = legacy(-1, 0);
        BFPtrComplex exp = legacy(exponent.x, exponent.y);
        cmplx_pow_bf(&result, &base, &exp);
        const std::complex<double> expected =
            std::pow(std::complex<double>{-1, 0}, std::complex<double>{exponent.x, exponent.y});
        expect_value(result, expected.real(), expected.imag());
        expect_value(base, -1, 0);
        expect_value(exp, exponent.x, exponent.y);
    }
}

TEST_P(TestBigComplex, legacyPowerPreservesTheExactHalfPiSineCosineQuirk)
{
    BFPtrComplex base = legacy(-1, 0);
    BFPtrComplex exponent = legacy(0.5L, 0);
    BFPtrComplex result = legacy(0, 0);

    cmplx_pow_bf(&result, &base, &exponent);

    // Range reduction returns early before applying the quadrant change.
    expect_value(result, 1, 0);
}

TEST_P(TestBigComplex, legacyCubeAndIntegerPower)
{
    BFPtrComplex base = legacy(0.75L, -0.5L);
    BFPtrComplex result = legacy(0, 0);
    cube(result, base);
    expect_value(result, -0.140625, -0.71875);
    for (const int degree : {-3, 0, 1, 2, 3, 6, 13})
    {
        power(result, base, degree);
        const auto expected = std::pow(std::complex<double>{0.75, -0.5}, std::max(degree, 0));
        expect_value(result, expected.real(), expected.imag());
    }
    expect_value(base, 0.75, -0.5);
}

TEST_P(TestBigComplex, owningBigFloatArithmeticPreservesOperandsAndHandlesSelfAssignment)
{
    check_owning_arithmetic<BigFloat>();
}

TEST_P(TestBigComplex, owningBigNumArithmeticPreservesOperandsAndHandlesSelfAssignment)
{
    check_owning_arithmetic<BigNum>();
}

TEST_P(TestBigComplex, valueMathAgreesWithLegacyCalculations)
{
    BFPtrComplex left = legacy(0, 0);
    BFPtrComplex right = legacy(0, 0);
    str_to_bf(left.x, "-1.2345678901234567890123456789012345");
    str_to_bf(left.y, "0.8765432109876543210987654321098765");
    str_to_bf(right.x, "-0.375");
    str_to_bf(right.y, "0.0625");
    const BFComplex x = owned(left);
    const BFComplex y = owned(right);
    BFPtrComplex result = legacy(0, 0);

    cmplx_mul_bf(&result, &left, &right);
    EXPECT_EQ(owned(result), x * y);
    cmplx_div_bf(&result, &left, &right);
    // Newton iterations retain guard-digit differences from reused outputs.
    expect_precision(owned(result), x / y);
    cmplx_log_bf(&result, &left);
    expect_precision(owned(result), log(x));
    cmplx_pow_bf(&result, &left, &right);
    expect_precision(owned(result), pow(x, y));
    cube(result, left);
    EXPECT_EQ(owned(result), cube(x));
    for (const int degree : {-2, 0, 1, 2, 3, 6, 13})
    {
        power(result, left, degree);
        EXPECT_EQ(owned(result), power(x, degree)) << degree;
    }
    EXPECT_EQ(x, owned(left));
    EXPECT_EQ(y, owned(right));
}

TEST_P(TestBigComplex, owningLogAndPowerKeepZeroAndBranchConventions)
{
    const BFComplex zero;
    EXPECT_EQ(zero, log(zero));
    for (const BFComplex exponent :
        {zero, BFComplex{BigFloat{-2}, BigFloat{}}, BFComplex{BigFloat{0.25L}, BigFloat{-0.5L}}})
    {
        EXPECT_EQ(zero, pow(zero, exponent));
    }
    const BFComplex negative{BigFloat{-1}, BigFloat{}};
    EXPECT_EQ((BFComplex{BigFloat{1}, BigFloat{}}), pow(negative, BFComplex{BigFloat{0.5L}, BigFloat{}}));
    BFPtrComplex raw = legacy(-1, -1);
    BFPtrComplex result = legacy(0, 0);
    cmplx_log_bf(&result, &raw);
    EXPECT_EQ(owned(result), log(owned(raw)));
}

TEST_P(TestBigComplex, realValueFunctionsPreserveTheirOperands)
{
    BFPtrComplex raw = legacy(0.375L, -0.625L);
    BFPtrComplex result = legacy(0, 0);
    const BFComplex input = owned(raw);
    square_bf(result.x, raw.x);
    EXPECT_EQ(owned(result).x, sqr(input.x));
    half_bf(result.x, raw.x);
    EXPECT_EQ(owned(result).x, input.x.half());
    exp_bf(result.x, raw.y);
    EXPECT_EQ(owned(result).x, exp(input.y));
    ln_bf(result.x, raw.x);
    expect_precision(owned(result).x, log(input.x));
    atan2_bf(result.x, raw.y, raw.x);
    expect_precision(owned(result).x, atan2(input.y, input.x));
    sin_cos_bf(result.x, result.y, raw.y);
    const auto [sine, cosine] = sin_cos(input.y);
    EXPECT_EQ(owned(result), (BFComplex{sine, cosine}));
    EXPECT_EQ(input, owned(raw));
}

TEST_P(TestBigComplex, owningCalculationsDoNotAllocateArenaStorage)
{
    const auto marker = save_stack();
    const BFComplex base{BigFloat{-0.75L}, BigFloat{0.125L}};

    const BFComplex result = pow(base, BFComplex{BigFloat{0.375L}, BigFloat{-0.25L}});

    EXPECT_EQ(marker, save_stack());
    restore_stack(marker);
    EXPECT_EQ(result, pow(base, BFComplex{BigFloat{0.375L}, BigFloat{-0.25L}}));
}

TEST_P(TestBigComplex, logarithmAndPowerMatchRecordedLegacySamples)
{
    const BFComplex base{
        BigFloat{"-1.2345678901234567890123456789012345"}, BigFloat{"0.8765432109876543210987654321098765"}};
    const BFComplex exponent{BigFloat{"-0.375"}, BigFloat{"0.0625"}};
    const BFComplex expected_log{
        BigFloat{
            "0.41481838209636195491412400727387571094698275934638562083728302194169551879987897419073488647326469154"},
        BigFloat{
            "2.5241867569914683338918055707442912395903265966985097956410058648532659713973426178526831026139757135"}};
    const BFComplex expected_power{
        BigFloat{
            "0.44249025840474456578197006003211083958237182607596657080637012998361909661261066797451841254153611171"},
        BigFloat{
            "-0.5818833418008393071307465141849960483809995114255372252408727113952518171147202941938611692922067732"}};

    expect_precision(expected_log, log(base));
    expect_precision(expected_power, pow(base, exponent));
}

TEST_P(TestBigComplex, legacyBinaryAdaptersCaptureBothInputsBeforeWriting)
{
    for (const auto operation : {cmplx_mul_bf, cmplx_div_bf, cmplx_pow_bf})
    {
        for (const bool alias_left : {false, true})
        {
            BigStackSaver saved;
            BFPtrComplex left = legacy(-0.75L, 0.25L);
            BFPtrComplex right = legacy(0.375L, -0.125L);
            BFPtrComplex output = alias_left ? left : right;
            const BFComplex x = owned(left);
            const BFComplex y = owned(right);
            const BFComplex expected = operation == cmplx_mul_bf ? x * y
                : operation == cmplx_div_bf                      ? x / y
                                                                 : pow(x, y);

            EXPECT_EQ(&output, operation(&output, &left, &right));

            EXPECT_EQ(expected, owned(output));
            EXPECT_EQ(alias_left ? y : x, owned(alias_left ? right : left));
        }
    }
}

TEST_P(TestBigComplex, legacyLogCubeAndIntegerPowerAllowInPlaceResults)
{
    BFPtrComplex raw = legacy(-0.75L, 0.25L);
    BFComplex expected = log(owned(raw));
    cmplx_log_bf(&raw, &raw);
    EXPECT_EQ(expected, owned(raw));

    raw = legacy(-0.75L, 0.25L);
    expected = cube(owned(raw));
    cube(raw, raw);
    EXPECT_EQ(expected, owned(raw));
    for (const int degree : {-2, 0, 1, 3, 6})
    {
        raw = legacy(-0.75L, 0.25L);
        expected = power(owned(raw), degree);
        power(raw, raw, degree);
        EXPECT_EQ(expected, owned(raw));
    }
}

INSTANTIATE_TEST_SUITE_P(Precisions, TestBigComplex, testing::Values(20, 80));

} // namespace id::test
