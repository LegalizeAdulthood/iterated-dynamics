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
#include <tuple>
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

void PrintTo(const DComplex &value, std::ostream *out)
{
    *out << '(' << value.x << ", " << value.y << ')';
}

} // namespace id::math

namespace id::test
{
namespace
{

static_assert(std::is_same_v<BFConstPtrComplex, Complex<BigFloatConstPtr>>);
static_assert(!std::is_convertible_v<BFPtrComplex, BFComplex>);
static_assert(!std::is_convertible_v<BNPtrComplex, BNComplex>);

template <typename T>
void check_owning_arithmetic()
{
    const Complex<T> left{T{1}, T{2}};
    const Complex<T> right{T{2}, T{-1}};
    EXPECT_EQ((Complex<T>{T{4}, T{3}}), left * right);
    // Preserve scalar division rounding, including fixed-point 5/5.
    EXPECT_EQ((Complex<T>{T{0}, T{5} / T{5}}), left / right);
    EXPECT_EQ(T{5}, mag_squared(left));
    EXPECT_EQ((Complex<T>{T{2}, T{4}}), left * 2.0);
    EXPECT_EQ(left * 2.0, 2.0 * left);

    Complex<T> copy = left;
    EXPECT_EQ(&copy, &(copy *= copy));
    EXPECT_EQ((Complex<T>{T{-3}, T{4}}), copy);
    EXPECT_EQ(&copy, &(copy /= copy));
    EXPECT_EQ((Complex<T>{T{25} / T{25}, T{0}}), copy);
    copy = left;
    copy += copy;
    EXPECT_EQ(left * 2.0, copy);
    copy -= copy;
    EXPECT_EQ((Complex<T>{T{0}, T{0}}), copy);
    EXPECT_EQ((Complex<T>{T{1}, T{2}}), left);

    g_overflow = false;
    EXPECT_EQ((Complex<T>{T{0}, T{0}}), (left / Complex<T>{T{0}, T{0}}));
    EXPECT_TRUE(g_overflow);
    EXPECT_EQ((Complex<T>{T{-11}, T{-2}}), cube(left));
    EXPECT_EQ(cube(left), power(left, 3));
    EXPECT_EQ((Complex<T>{T{1}, T{0}}), power(left, -2));
}

} // namespace

class TestBigComplexBase : public testing::Test
{
protected:
    TestBigComplexBase();
    void initialize_precision(int precision);
    void TearDown() override;
    BFComplex value(LDouble real, LDouble imag);
    void expect_value(const BFComplex &value, double real, double imag);
    void expect_precision(const BigFloat &expected, const BigFloat &actual);
    void expect_precision(const BFComplex &expected, const BFComplex &actual);

    ValueSaver<long> m_saved_bailout_value;
    ValueSaver<Bailout> m_saved_bailout_test;
    ValueSaver<int> m_saved_bf_digits;
    ValueSaver<FractalType> m_saved_fractal_type;
    ValueSaver<bool> m_saved_overflow;
};

TestBigComplexBase::TestBigComplexBase() :
    m_saved_bailout_value(g_user.bailout_value, 0),
    m_saved_bailout_test(g_bailout_test, Bailout::MOD),
    m_saved_bf_digits(g_bf_digits, 0),
    m_saved_fractal_type(g_fractal_type, FractalType::MANDEL_Z_POWER),
    m_saved_overflow(g_overflow, false)
{
}

void TestBigComplexBase::initialize_precision(const int precision)
{
    init_bf_dec(precision);
    init_big_pi();
}

void TestBigComplexBase::TearDown()
{
    free_bf_vars();
}

BFComplex TestBigComplexBase::value(const LDouble real, const LDouble imag)
{
    BFComplex result{BigFloat{bf_arena(BigLifetime::TEMPORARY)}, BigFloat{bf_arena(BigLifetime::TEMPORARY)}};
    result.x = real;
    result.y = imag;
    return result;
}

void TestBigComplexBase::expect_value(const BFComplex &value, const double real, const double imag)
{
    const DComplex actual = cmplx_bf_to_float(value);
    EXPECT_NEAR(real, actual.x, 1e-14);
    EXPECT_NEAR(imag, actual.y, 1e-14);
}

void TestBigComplexBase::expect_precision(const BigFloat &expected, const BigFloat &actual)
{
    EXPECT_EQ(expected.to_string(g_decimals), actual.to_string(g_decimals));
}

void TestBigComplexBase::expect_precision(const BFComplex &expected, const BFComplex &actual)
{
    expect_precision(expected.x, actual.x);
    expect_precision(expected.y, actual.y);
}

class TestBigComplex : public TestBigComplexBase, public testing::WithParamInterface<int>
{
protected:
    void SetUp() override;
};

class TestBigComplexLogBranch : public TestBigComplexBase, public testing::WithParamInterface<std::tuple<int, DComplex>>
{
protected:
    void SetUp() override;
};

class TestBigComplexZeroPower : public TestBigComplexBase, public testing::WithParamInterface<std::tuple<int, DComplex>>
{
protected:
    void SetUp() override;
};

class TestBigComplexPower : public TestBigComplexBase, public testing::WithParamInterface<std::tuple<int, DComplex>>
{
protected:
    void SetUp() override;
};

class TestBigComplexIntegerPower : public TestBigComplexBase, public testing::WithParamInterface<std::tuple<int, int>>
{
protected:
    void SetUp() override;
};

enum class ComplexBinaryOperation
{
    MULTIPLY,
    DIVIDE,
    POWER
};

class TestBigComplexBinaryAlias : public TestBigComplexBase,
                                  public testing::WithParamInterface<std::tuple<int, ComplexBinaryOperation, bool>>
{
protected:
    void SetUp() override;
};

void TestBigComplex::SetUp()
{
    initialize_precision(GetParam());
}

void TestBigComplexLogBranch::SetUp()
{
    initialize_precision(std::get<0>(GetParam()));
}

void TestBigComplexZeroPower::SetUp()
{
    initialize_precision(std::get<0>(GetParam()));
}

void TestBigComplexPower::SetUp()
{
    initialize_precision(std::get<0>(GetParam()));
}

void TestBigComplexIntegerPower::SetUp()
{
    initialize_precision(std::get<0>(GetParam()));
}

void TestBigComplexBinaryAlias::SetUp()
{
    initialize_precision(std::get<0>(GetParam()));
}

TEST_P(TestBigComplex, productAndQuotient)
{
    BFComplex left = value(1.25L, -0.5L);
    BFComplex right = value(-0.75L, 2);
    BFComplex result = value(0, 0);

    result = left * right;
    expect_value(result, 0.0625, 2.875);
    result = left / right;
    expect_value(result, -31.0 / 73.0, -34.0 / 73.0);
    EXPECT_FALSE(g_overflow);
    expect_value(left, 1.25, -0.5);
    expect_value(right, -0.75, 2);
}

TEST_P(TestBigComplex, divisionByZeroSetsOverflowAndReturnsZero)
{
    BFComplex left = value(1, -2);
    BFComplex zero = value(0, 0);
    BFComplex result = value(7, 9);

    result = left / zero;

    EXPECT_TRUE(g_overflow);
    expect_value(result, 0, 0);
}

TEST_P(TestBigComplexLogBranch, logarithmBranches)
{
    BFComplex result = value(0, 0);
    const DComplex base = std::get<1>(GetParam());
    BFComplex input = value(base.x, base.y);
    result = log(input);
    const bool zero = base == DComplex{};
    expect_value(result, zero ? 0 : std::log(std::hypot(base.x, base.y)), zero ? 0 : std::atan2(base.y, base.x));
    expect_value(input, base.x, base.y);
}

TEST_P(TestBigComplexZeroPower, zeroBasePowerAlwaysReturnsZero)
{
    BFComplex base = value(0, 0);
    BFComplex result = value(7, 9);
    const DComplex exponent = std::get<1>(GetParam());
    BFComplex exp = value(exponent.x, exponent.y);
    result = pow(base, exp);
    expect_value(result, 0, 0);
}

TEST_P(TestBigComplexPower, powerHandlesNegativeAndComplexExponents)
{
    BFComplex result = value(0, 0);
    const DComplex exponent = std::get<1>(GetParam());
    BFComplex base = value(-1, 0);
    BFComplex exp = value(exponent.x, exponent.y);
    result = pow(base, exp);
    const std::complex<double> expected =
        std::pow(std::complex<double>{-1, 0}, std::complex<double>{exponent.x, exponent.y});
    expect_value(result, expected.real(), expected.imag());
    expect_value(base, -1, 0);
    expect_value(exp, exponent.x, exponent.y);
}

TEST_P(TestBigComplex, powerPreservesTheExactHalfPiSineCosineQuirk)
{
    BFComplex base = value(-1, 0);
    BFComplex exponent = value(0.5L, 0);
    BFComplex result = value(0, 0);

    result = pow(base, exponent);

    // Range reduction returns early before applying the quadrant change.
    expect_value(result, 1, 0);
}

TEST_P(TestBigComplex, cube)
{
    BFComplex base = value(0.75L, -0.5L);
    BFComplex result = value(0, 0);
    result = cube(base);
    expect_value(result, -0.140625, -0.71875);
    expect_value(base, 0.75, -0.5);
}

TEST_P(TestBigComplexIntegerPower, integerPower)
{
    BFComplex base = value(0.75L, -0.5L);
    BFComplex result = value(0, 0);
    const int degree = std::get<1>(GetParam());
    result = power(base, degree);
    const auto expected = std::pow(std::complex<double>{0.75, -0.5}, std::max(degree, 0));
    expect_value(result, expected.real(), expected.imag());
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

TEST_P(TestBigComplex, realValueFunctionsPreserveTheirOperands)
{
    BFComplex raw = value(0.375L, -0.625L);
    BFComplex result = value(0, 0);
    const BFComplex input = raw;
    square_bf(result.x, raw.x);
    EXPECT_EQ(result.x, sqr(input.x));
    half_bf(result.x, raw.x);
    EXPECT_EQ(result.x, input.x.half());
    exp_bf(result.x.data(), raw.y.data());
    EXPECT_EQ(result.x, exp(input.y));
    ln_bf(result.x.data(), raw.x.data());
    expect_precision(result.x, log(input.x));
    atan2_bf(result.x.data(), raw.y.data(), raw.x.data());
    expect_precision(result.x, atan2(input.y, input.x));
    sin_cos_bf(result.x.data(), result.y.data(), raw.y.data());
    const auto [sine, cosine] = sin_cos(input.y);
    EXPECT_EQ(result, (BFComplex{sine, cosine}));
    EXPECT_EQ(input, raw);
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

TEST_P(TestBigComplexBinaryAlias, binaryOperationsCaptureBothInputsBeforeWriting)
{
    const auto kind = std::get<1>(GetParam());
    const bool alias_left = std::get<2>(GetParam());
    BigStackSaver saved;
    BFComplex left = value(-0.75L, 0.25L);
    BFComplex right = value(0.375L, -0.125L);
    BFComplex &output = alias_left ? left : right;
    const BFComplex x = left;
    const BFComplex y = right;
    const BFComplex expected = kind == ComplexBinaryOperation::MULTIPLY ? x * y
        : kind == ComplexBinaryOperation::DIVIDE                        ? x / y
                                                                        : pow(x, y);

    output = kind == ComplexBinaryOperation::MULTIPLY ? left * right
        : kind == ComplexBinaryOperation::DIVIDE      ? left / right
                                                      : pow(left, right);

    EXPECT_EQ(expected, output);
    EXPECT_EQ(alias_left ? y : x, (alias_left ? right : left));
}

TEST_P(TestBigComplex, logAndCubeAllowInPlaceResults)
{
    BFComplex raw = value(-0.75L, 0.25L);
    BFComplex expected = log(raw);
    raw = log(raw);
    EXPECT_EQ(expected, raw);

    raw = value(-0.75L, 0.25L);
    expected = cube(raw);
    raw = cube(raw);
    EXPECT_EQ(expected, raw);
}

TEST_P(TestBigComplexIntegerPower, integerPowerAllowsInPlaceResult)
{
    BFComplex raw = value(-0.75L, 0.25L);
    const int degree = std::get<1>(GetParam());
    const BFComplex expected = power(raw, degree);
    raw = power(raw, degree);
    EXPECT_EQ(expected, raw);
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestBigComplexLogBranch,
    testing::Combine(
        testing::Values(20, 80), testing::Values(DComplex{}, DComplex{-1, 0}, DComplex{0, -1}, DComplex{-1, -1})));
INSTANTIATE_TEST_SUITE_P(Inputs, TestBigComplexZeroPower,
    testing::Combine(testing::Values(20, 80),
        testing::Values(DComplex{}, DComplex{-2, 0}, DComplex{1.25, -0.5}, DComplex{0.25, -0.5})));
INSTANTIATE_TEST_SUITE_P(Inputs, TestBigComplexPower,
    testing::Combine(testing::Values(20, 80), testing::Values(DComplex{}, DComplex{-2, 0}, DComplex{0.25, -0.5})));
INSTANTIATE_TEST_SUITE_P(Inputs, TestBigComplexIntegerPower,
    testing::Combine(testing::Values(20, 80), testing::Values(-3, -2, 0, 1, 2, 3, 6, 13)));
INSTANTIATE_TEST_SUITE_P(Inputs, TestBigComplexBinaryAlias,
    testing::Combine(testing::Values(20, 80),
        testing::Values(
            ComplexBinaryOperation::MULTIPLY, ComplexBinaryOperation::DIVIDE, ComplexBinaryOperation::POWER),
        testing::Bool()));

INSTANTIATE_TEST_SUITE_P(Precisions, TestBigComplex, testing::Values(20, 80));

} // namespace id::test
