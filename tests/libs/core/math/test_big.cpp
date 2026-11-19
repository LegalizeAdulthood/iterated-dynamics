// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <math/big.h>

#include <engine/bailout_formula.h>
#include <engine/UserData.h>
#include <fractals/fractype.h>
#include <math/big_init.h>
#include <math/BigArena.h>
#include <math/BigFloat.h>
#include <math/BigNum.h>
#include <misc/ValueSaver.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iterator>
#include <limits>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

using namespace id::math;
using namespace id::misc;

namespace id::test
{
namespace
{

static_assert(std::is_same_v<BFConstPtrComplex, Complex<BigFloatConstPtr>>);
static_assert(!std::is_convertible_v<BFPtrComplex, BFComplex>);
static_assert(!std::is_convertible_v<BNPtrComplex, BNComplex>);

void keep_first_block(Byte *)
{
}

} // namespace

class TestBigNumConversionBase : public testing::Test
{
protected:
    void SetUp() override;
    void TearDown() override;

    ValueSaver<int> m_digits{g_bf_digits, 0};
};

void TestBigNumConversionBase::SetUp()
{
    init_bf_dec(20);
}

void TestBigNumConversionBase::TearDown()
{
    free_bf_vars();
}

enum class BigFloatByteProcedure
{
    COPY,
    CLEAR,
    MAXIMUM
};

class TestBigFloatByteProcedures : public TestBigNumConversionBase,
                                   public testing::WithParamInterface<std::tuple<int, BigFloatByteProcedure>>
{
};

TEST_P(TestBigFloatByteProcedures, preservesExactExtentSourceReturnAddressAndGuards)
{
    ValueSaver<int> bf_length{g_bf_length, std::get<0>(GetParam())};
    const std::size_t size = static_cast<std::size_t>(g_bf_length + 2);
    const std::array<int, 7> lengths{
        g_bn_length, g_bf_length, g_int_length, g_padding, g_r_length, g_r_bf_length, g_shift_factor};
    const std::vector<Byte> source(size, 0x35);
    std::vector<Byte> destination(size + 16, 0xa5);
    std::vector<Byte> expected(size + 16, 0xa5);
    Byte *const output = destination.data() + 8;

    switch (std::get<1>(GetParam()))
    {
    case BigFloatByteProcedure::COPY:
        std::copy(source.begin(), source.end(), expected.begin() + 8);
        EXPECT_EQ(output, copy_bf(output, source.data()));
        break;
    case BigFloatByteProcedure::CLEAR:
        std::fill_n(expected.begin() + 8, size, Byte{0});
        EXPECT_EQ(output, clear_bf(output));
        break;
    case BigFloatByteProcedure::MAXIMUM:
    {
        constexpr int exponent = std::numeric_limits<long double>::max_exponent / 8;
        std::fill_n(expected.begin() + 8, size, Byte{0});
        expected[8 + size - 4] = 1;
        expected[8 + size - 2] = static_cast<Byte>(exponent % 256);
        expected[8 + size - 1] = static_cast<Byte>(exponent / 256);
        EXPECT_EQ(output, max_bf(output));
        break;
    }
    }

    EXPECT_EQ(expected, destination);
    EXPECT_EQ(std::vector<Byte>(size, 0x35), source);
    EXPECT_EQ(lengths,
        (std::array<int, 7>{
            g_bn_length, g_bf_length, g_int_length, g_padding, g_r_length, g_r_bf_length, g_shift_factor}));
}

INSTANTIATE_TEST_SUITE_P(LengthsAndOperations, TestBigFloatByteProcedures,
    testing::Combine(testing::Values(8, 12, 64, 712),
        testing::Values(BigFloatByteProcedure::COPY, BigFloatByteProcedure::CLEAR, BigFloatByteProcedure::MAXIMUM)));

struct BigFloatByteSignInput
{
    Byte first;
    Byte last;
    int sign;
};

class TestBigFloatByteSign : public testing::TestWithParam<BigFloatByteSignInput>
{
};

TEST_P(TestBigFloatByteSign, observesOnlyTheBorrowedMantissa)
{
    constexpr int length = 8;
    const ValueSaver<int> bf_length{g_bf_length, length};
    const BigFloatByteSignInput &input = GetParam();
    const std::array<Byte, length + 4> encoded{0x80, input.first, 0, 0, 0, 0, 0, 0, input.last, 0x7f, 0xff, 0xa5};
    const Byte *const mantissa = encoded.data() + 1;

    EXPECT_EQ(input.sign, sign_bf(mantissa));
    EXPECT_EQ(input.sign == 0, is_bf_zero(mantissa));
    EXPECT_EQ(input.sign < 0, is_bf_neg(mantissa));
    EXPECT_EQ(input.sign != 0, is_bf_not_zero(mantissa));
}

INSTANTIATE_TEST_SUITE_P(Mantissas, TestBigFloatByteSign,
    testing::Values(BigFloatByteSignInput{0, 0, 0}, BigFloatByteSignInput{0x80, 0, 1},
        BigFloatByteSignInput{0, 0x01, 1}, BigFloatByteSignInput{0, 0x80, -1}));

struct BigFloatByteComparisonInput
{
    std::array<Byte, 5> left_tail;
    std::array<Byte, 5> right_tail;
    int expected;
};

class TestBigFloatByteComparison : public testing::TestWithParam<BigFloatByteComparisonInput>
{
};

TEST_P(TestBigFloatByteComparison, returnsTheSignedMismatchPosition)
{
    constexpr int length = 8;
    const ValueSaver<int> bf_length{g_bf_length, length};
    const BigFloatByteComparisonInput &input = GetParam();
    alignas(U16) std::array<Byte, length + 2> left{};
    alignas(U16) std::array<Byte, length + 2> right{};
    std::copy(input.left_tail.begin(), input.left_tail.end(), left.end() - 5);
    std::copy(input.right_tail.begin(), input.right_tail.end(), right.end() - 5);
    const std::array<Byte, length + 2> original_left = left;
    const std::array<Byte, length + 2> original_right = right;

    EXPECT_EQ(input.expected, cmp_bf(left.data(), right.data()));
    EXPECT_EQ(-input.expected, cmp_bf(right.data(), left.data()));
    EXPECT_EQ(original_left, left);
    EXPECT_EQ(original_right, right);
}

INSTANTIATE_TEST_SUITE_P(Layouts, TestBigFloatByteComparison,
    testing::Values(BigFloatByteComparisonInput{{0x40, 1, 0, 0, 0}, {0x40, 1, 0, 0, 0}, 0},
        BigFloatByteComparisonInput{{0, 0, 0, 0, 0}, {0xc0, 0xfe, 0xff, 0, 0}, 8},
        BigFloatByteComparisonInput{{0, 0x80, 0, 0xff, 0xff}, {0x40, 1, 0, 0, 0}, -8},
        BigFloatByteComparisonInput{{0, 0xfe, 0xff, 1, 0}, {0xc0, 0xfe, 0xff, 0, 0}, -8},
        BigFloatByteComparisonInput{{0x40, 1, 0, 0, 0}, {0x40, 2, 0, 0, 0}, -7},
        BigFloatByteComparisonInput{{0x40, 1, 0, 0, 0}, {0x80, 1, 0, 0, 0}, -6}));

struct BigFloatByteNegationInput
{
    std::array<Byte, 5> source_tail;
    std::array<Byte, 5> result_tail;
};

class TestBigFloatByteNegation : public testing::TestWithParam<BigFloatByteNegationInput>
{
};

TEST_P(TestBigFloatByteNegation, preservesExponentNormalizationAliasesAndPadding)
{
    constexpr int length = 8;
    constexpr int size = length + 2;
    const ValueSaver<int> bf_length{g_bf_length, length};
    const ValueSaver<int> bn_length{g_bn_length, 4};
    const BigFloatByteNegationInput &input = GetParam();
    alignas(U16) std::array<Byte, size + 2> source{};
    std::copy(input.source_tail.begin(), input.source_tail.end(), source.begin() + size - 5);
    source[size] = 0xa5;
    source[size + 1] = 0x5a;
    const std::array<Byte, size + 2> original = source;
    std::array<Byte, size + 2> expected{};
    std::copy(input.result_tail.begin(), input.result_tail.end(), expected.begin() + size - 5);
    expected[size] = 0xa5;
    expected[size + 1] = 0x5a;
    alignas(U16) std::array<Byte, size + 2> result;
    result.fill(0x3c);
    result[size] = 0xa5;
    result[size + 1] = 0x5a;

    EXPECT_EQ(result.data(), neg_bf(result.data(), source.data()));
    EXPECT_EQ(expected, result);
    EXPECT_EQ(original, source);
    EXPECT_EQ(4, g_bn_length);

    EXPECT_EQ(source.data(), neg_bf(source.data(), source.data()));
    EXPECT_EQ(expected, source);
    EXPECT_EQ(4, g_bn_length);

    source = original;
    EXPECT_EQ(source.data(), neg_a_bf(source.data()));
    EXPECT_EQ(expected, source);
    EXPECT_EQ(4, g_bn_length);
}

INSTANTIATE_TEST_SUITE_P(Layouts, TestBigFloatByteNegation,
    testing::Values(BigFloatByteNegationInput{{0, 0, 0, 0x12, 0}, {}},
        BigFloatByteNegationInput{{0x40, 1, 0, 0, 0}, {0xc0, 0xfe, 0xff, 0, 0}},
        BigFloatByteNegationInput{{0, 1, 0, 0, 0}, {0, 0, 0xff, 0xff, 0xff}},
        BigFloatByteNegationInput{{0, 0, 0xff, 0xff, 0xff}, {0, 1, 0, 0, 0}}));

struct BigFloatByteAbsInput
{
    std::array<Byte, 5> source_tail;
    std::array<Byte, 5> result_tail;
};

class TestBigFloatByteAbs : public testing::TestWithParam<BigFloatByteAbsInput>
{
};

TEST_P(TestBigFloatByteAbs, preservesDestinationAndCanonicalExtent)
{
    constexpr int length = 8;
    constexpr int size = length + 2;
    const ValueSaver<int> bf_length{g_bf_length, length};
    const ValueSaver<int> bn_length{g_bn_length, 4};
    const BigFloatByteAbsInput &input = GetParam();
    alignas(U16) std::array<Byte, size + 1> source{};
    std::copy(input.source_tail.begin(), input.source_tail.end(), source.begin() + size - 5);
    source.back() = 0xa5;
    const std::array<Byte, size + 1> original = source;
    std::array<Byte, size + 1> expected{};
    std::copy(input.result_tail.begin(), input.result_tail.end(), expected.begin() + size - 5);
    expected.back() = 0xa5;
    alignas(U16) std::array<Byte, size + 1> result;
    result.fill(0xa5);

    EXPECT_EQ(result.data(), abs_bf(result.data(), source.data()));
    EXPECT_EQ(expected, result);
    EXPECT_EQ(original, source);
    EXPECT_EQ(4, g_bn_length);

    EXPECT_EQ(source.data(), abs_a_bf(source.data()));
    EXPECT_EQ(expected, source);
    EXPECT_EQ(4, g_bn_length);
}

INSTANTIATE_TEST_SUITE_P(Layouts, TestBigFloatByteAbs,
    testing::Values(BigFloatByteAbsInput{{0, 0, 0, 0x12, 0}, {0, 0, 0, 0x12, 0}},
        BigFloatByteAbsInput{{0x40, 1, 0, 0, 0}, {0x40, 1, 0, 0, 0}},
        BigFloatByteAbsInput{{0, 0, 0xff, 0xff, 0xff}, {0, 1, 0, 0, 0}}));

struct BigFloatByteAdditionInput
{
    std::array<Byte, 5> left_tail;
    std::array<Byte, 5> right_tail;
    std::array<Byte, 5> aligned_right_tail;
    std::array<Byte, 5> result_tail;
};

class TestBigFloatByteAddition : public testing::TestWithParam<BigFloatByteAdditionInput>
{
};

TEST_P(TestBigFloatByteAddition, inPlaceKernelAlignsMantissasAndPreservesPadding)
{
    constexpr int length = 8;
    constexpr int size = length + 2;
    const ValueSaver<int> bf_length{g_bf_length, length};
    const ValueSaver<int> bn_length{g_bn_length, 4};
    const BigFloatByteAdditionInput &input = GetParam();
    alignas(U16) std::array<Byte, size + 2> left{};
    alignas(U16) std::array<Byte, size + 2> right{};
    std::copy(input.left_tail.begin(), input.left_tail.end(), left.begin() + size - 5);
    std::copy(input.right_tail.begin(), input.right_tail.end(), right.begin() + size - 5);
    left.back() = 0x5a;
    right.back() = 0xa5;
    std::array<Byte, size + 2> expected_left{};
    std::array<Byte, size + 2> expected_right{};
    std::copy(input.result_tail.begin(), input.result_tail.end(), expected_left.begin() + size - 5);
    std::copy(input.aligned_right_tail.begin(), input.aligned_right_tail.end(), expected_right.begin() + size - 5);
    expected_left.back() = 0x5a;
    expected_right.back() = 0xa5;

    EXPECT_EQ(left.data(), unsafe_add_a_bf(left.data(), right.data()));

    EXPECT_EQ(expected_left, left);
    EXPECT_EQ(expected_right, right);
    EXPECT_EQ(4, g_bn_length);
    EXPECT_EQ(length, g_bf_length);
}

INSTANTIATE_TEST_SUITE_P(Alignment, TestBigFloatByteAddition,
    testing::Values(BigFloatByteAdditionInput{{0x40, 1, 0, 0, 0}, {0, 0x80, 0xff, 0xff, 0xff}, {0x80, 0xff, 0xff, 0, 0},
                        {0, 0xc0, 0, 0xff, 0xff}},
        BigFloatByteAdditionInput{{0, 0x80, 0, 0xff, 0xff}, {0x40, 1, 0, 0, 0}, {0x40, 1, 0, 0, 0}, {0xc0, 1, 0, 0, 0}},
        BigFloatByteAdditionInput{{0x40, 1, 0, 0, 0}, {0, 1, 0, 0xf8, 0xff}, {}, {0x40, 1, 0, 0, 0}}));

struct BigFloatByteSubtractionInput
{
    std::array<Byte, 5> left_tail;
    std::array<Byte, 5> right_tail;
    std::array<Byte, 5> aligned_right_tail;
    std::array<Byte, 5> result_tail;
};

class TestBigFloatByteSubtraction : public testing::TestWithParam<BigFloatByteSubtractionInput>
{
};

TEST_P(TestBigFloatByteSubtraction, preservesExtentAndAlignsScratchOperands)
{
    constexpr int length = 8;
    constexpr int size = length + 2;
    const ValueSaver<int> bf_length{g_bf_length, length};
    const ValueSaver<int> bn_length{g_bn_length, 4};
    const BigFloatByteSubtractionInput &input = GetParam();
    alignas(U16) std::array<Byte, size + 2> left{};
    alignas(U16) std::array<Byte, size + 2> right{};
    std::copy(input.left_tail.begin(), input.left_tail.end(), left.begin() + size - 5);
    std::copy(input.right_tail.begin(), input.right_tail.end(), right.begin() + size - 5);
    left.back() = 0x5a;
    right.back() = 0xa5;
    const std::array<Byte, size + 2> original_left = left;
    const std::array<Byte, size + 2> original_right = right;
    std::array<Byte, size + 2> expected{};
    std::array<Byte, size + 2> expected_right{};
    std::copy(input.result_tail.begin(), input.result_tail.end(), expected.begin() + size - 5);
    std::copy(input.aligned_right_tail.begin(), input.aligned_right_tail.end(), expected_right.begin() + size - 5);
    expected.back() = 0x5a;
    expected_right.back() = 0xa5;
    alignas(U16) std::array<Byte, size + 2> result{};
    result.back() = 0x5a;

    EXPECT_EQ(result.data(), unsafe_sub_bf(result.data(), left.data(), right.data()));
    EXPECT_EQ(expected, result);
    EXPECT_EQ(original_left, left);
    EXPECT_EQ(expected_right, right);
    EXPECT_EQ(4, g_bn_length);

    right = original_right;
    EXPECT_EQ(left.data(), unsafe_sub_a_bf(left.data(), right.data()));
    EXPECT_EQ(expected, left);
    EXPECT_EQ(expected_right, right);
    EXPECT_EQ(4, g_bn_length);
    EXPECT_EQ(length, g_bf_length);
}

INSTANTIATE_TEST_SUITE_P(Operations, TestBigFloatByteSubtraction,
    testing::Values(BigFloatByteSubtractionInput{{0x40, 1, 0, 0, 0}, {0, 0x80, 0xff, 0xff, 0xff},
                        {0x80, 0xff, 0xff, 0, 0}, {0xc0, 1, 0, 0, 0}},
        BigFloatByteSubtractionInput{{}, {0, 1, 0, 0, 0}, {0, 1, 0, 0, 0}, {0, 0, 0xff, 0xff, 0xff}},
        BigFloatByteSubtractionInput{{0x40, 1, 0, 0, 0}, {}, {}, {0x40, 1, 0, 0, 0}}));

struct BigFloatByteMultiplyInput
{
    int left;
    int right;
    int left_after;
    int right_after;
    int product;
    int product_exponent;
};

class TestBigFloatBytePrecision : public TestBigNumConversionBase
{
protected:
    ValueSaver<fractals::FractalType> m_fractal_type{fractals::g_fractal_type, fractals::FractalType::MANDEL_Z_POWER};
};

class TestBigFloatByteMultiply : public TestBigFloatBytePrecision,
                                 public testing::WithParamInterface<std::tuple<int, BigFloatByteMultiplyInput>>
{
protected:
    void SetUp() override
    {
        init_bf_length(std::get<0>(GetParam()));
    }
};

TEST_P(TestBigFloatByteMultiply, writesCompleteWideProductAndRestoresLengths)
{
    const BigFloatByteMultiplyInput &input = std::get<1>(GetParam());
    const std::array<int, 7> lengths{
        g_bn_length, g_bf_length, g_int_length, g_padding, g_r_length, g_r_bf_length, g_shift_factor};
    const auto encode = [](int length, int mantissa, int exponent)
    {
        std::vector<Byte> bytes(length + 2);
        bytes[length - 2] = static_cast<Byte>(mantissa);
        bytes[length - 1] = mantissa < 0 ? 0xff : 0;
        bytes[length] = static_cast<Byte>(exponent);
        bytes[length + 1] = static_cast<Byte>(static_cast<unsigned>(exponent) >> 8);
        return bytes;
    };
    std::vector<Byte> left = encode(g_bf_length, input.left, 2);
    std::vector<Byte> right = encode(g_bf_length, input.right, -1);
    const std::vector<Byte> product = encode(g_r_bf_length, input.product, input.product_exponent);
    std::vector<Byte> guarded(product.size() + 4, 0xa5);
    std::vector<Byte> expected = guarded;
    std::copy(product.begin(), product.end(), expected.begin() + 2);
    Byte *const result = guarded.data() + 2;

    EXPECT_EQ(result, multiply_wide(result, left.data(), right.data()));

    EXPECT_EQ(expected, guarded);
    EXPECT_EQ(encode(g_bf_length, input.left_after, 2), left);
    EXPECT_EQ(encode(g_bf_length, input.right_after, -1), right);
    EXPECT_EQ(lengths,
        (std::array<int, 7>{
            g_bn_length, g_bf_length, g_int_length, g_padding, g_r_length, g_r_bf_length, g_shift_factor}));
}

INSTANTIATE_TEST_SUITE_P(PrecisionsSignsAndZero, TestBigFloatByteMultiply,
    testing::Combine(testing::Values(4, 12, 52, 56, 64, 708),
        testing::Values(BigFloatByteMultiplyInput{2, 3, 2, 3, 6, 1}, BigFloatByteMultiplyInput{-2, 3, 2, 3, -6, 1},
            BigFloatByteMultiplyInput{2, -3, 2, 3, -6, 1}, BigFloatByteMultiplyInput{-2, -3, 2, 3, 6, 1},
            BigFloatByteMultiplyInput{0, -3, 0, -3, 0, 0})));

struct BigFloatByteSqrInput
{
    int source;
    int source_after;
    int exponent;
    int squared;
    int squared_exponent;
};

class TestBigFloatByteSqr : public TestBigFloatBytePrecision,
                            public testing::WithParamInterface<std::tuple<int, BigFloatByteSqrInput>>
{
protected:
    void SetUp() override
    {
        init_bf_length(std::get<0>(GetParam()));
    }
};

TEST_P(TestBigFloatByteSqr, writesCompleteWideProductAndRestoresLengths)
{
    const BigFloatByteSqrInput &input = std::get<1>(GetParam());
    const std::array<int, 7> lengths{
        g_bn_length, g_bf_length, g_int_length, g_padding, g_r_length, g_r_bf_length, g_shift_factor};
    std::vector<Byte> source(g_bf_length + 2);
    source[g_bf_length - 2] = static_cast<Byte>(input.source);
    source[g_bf_length - 1] = input.source < 0 ? 0xff : 0;
    source[g_bf_length] = static_cast<Byte>(input.exponent);
    source[g_bf_length + 1] = static_cast<Byte>(static_cast<unsigned>(input.exponent) >> 8);
    std::vector<Byte> expected_source = source;
    expected_source[g_bf_length - 2] = static_cast<Byte>(input.source_after);
    expected_source[g_bf_length - 1] = 0;
    std::vector<Byte> guarded(g_r_bf_length + 6, 0xa5);
    std::vector<Byte> expected = guarded;
    std::fill_n(expected.begin() + 2, g_r_bf_length + 2, Byte{0});
    expected[g_r_bf_length] = static_cast<Byte>(input.squared);
    expected[g_r_bf_length + 2] = static_cast<Byte>(input.squared_exponent);
    expected[g_r_bf_length + 3] = static_cast<Byte>(static_cast<unsigned>(input.squared_exponent) >> 8);
    Byte *const result = guarded.data() + 2;

    EXPECT_EQ(result, square_wide(result, source.data()));

    EXPECT_EQ(expected, guarded);
    EXPECT_EQ(expected_source, source);
    EXPECT_EQ(lengths,
        (std::array<int, 7>{
            g_bn_length, g_bf_length, g_int_length, g_padding, g_r_length, g_r_bf_length, g_shift_factor}));
}

INSTANTIATE_TEST_SUITE_P(PrecisionsSignsAndExponents, TestBigFloatByteSqr,
    testing::Combine(testing::Values(4, 12, 52, 56, 64, 708),
        testing::Values(BigFloatByteSqrInput{3, 3, -700, 9, -1400}, BigFloatByteSqrInput{-2, 2, 500, 4, 1000},
            BigFloatByteSqrInput{0, 0, -1, 0, 0})));

namespace
{

std::vector<Byte> integer_bf_bytes(const int length, const int mantissa, const int exponent)
{
    std::vector<Byte> bytes(length + 2);
    bytes[length - 2] = static_cast<Byte>(mantissa);
    bytes[length - 1] = mantissa < 0 ? 0xff : 0;
    bytes[length] = static_cast<Byte>(exponent);
    bytes[length + 1] = static_cast<Byte>(static_cast<unsigned>(exponent) >> 8);
    return bytes;
}

struct BigFloatByteProductInput
{
    int left;
    int right;
    int left_exponent;
    int right_exponent;
    int product;
    int product_exponent;
    bool shared{};
    bool sqr{};
};

} // namespace

class TestBigFloatByteCanonicalProduct : public TestBigFloatBytePrecision,
                                         public testing::WithParamInterface<std::tuple<int, BigFloatByteProductInput>>
{
protected:
    void SetUp() override
    {
        init_bf_length(std::get<0>(GetParam()));
    }
};

TEST_P(TestBigFloatByteCanonicalProduct, preservesOutputBoundsAndRecordsDestructiveOperandEffects)
{
    const BigFloatByteProductInput &input = std::get<1>(GetParam());
    std::vector<Byte> left = integer_bf_bytes(g_bf_length, input.left, input.left_exponent);
    std::vector<Byte> right = integer_bf_bytes(g_bf_length, input.right, input.right_exponent);
    const bool nonzero = input.left != 0 && (input.sqr || input.shared || input.right != 0);
    std::vector<Byte> expected_left =
        integer_bf_bytes(g_bf_length, nonzero ? std::abs(input.left) : input.left, input.left_exponent);
    std::vector<Byte> expected_right = integer_bf_bytes(g_bf_length,
        nonzero && !input.shared && !input.sqr ? std::abs(input.right) : input.right, input.right_exponent);
    left.push_back(0x5a);
    right.push_back(0x6c);
    expected_left.push_back(0x5a);
    expected_right.push_back(0x6c);
    std::vector<Byte> output(g_r_bf_length + 6, 0xa5);
    std::vector<Byte> expected_output = output;
    const std::vector<Byte> canonical = integer_bf_bytes(g_bf_length, input.product, input.product_exponent);
    std::copy(canonical.begin(), canonical.end(), expected_output.begin() + 2);
    BigArena &temporary = bf_arena(BigLifetime::TEMPORARY);
    BigStackSaver saved;
    Byte *const borrowed = bf_stack().allocate_single();
    std::fill_n(borrowed, temporary.block_size(), Byte{0x69});
    BigArena::Allocation reservation = temporary.allocate_double();
    Byte *const product_address = reservation.data();
    std::fill_n(product_address, 2 * temporary.block_size(), Byte{0xa5});
    reservation.reset();
    const std::vector<Byte> free_bytes(product_address, product_address + 2 * temporary.block_size());
    const std::size_t high_water = temporary.high_water_mark();
    const BigStackMark mark = save_stack();
    Byte *const result = output.data() + 2;

    Byte *const returned = input.sqr ? unsafe_square_bf(result, left.data())
                                     : unsafe_mult_bf(result, left.data(), input.shared ? left.data() : right.data());

    EXPECT_EQ(result, returned);
    EXPECT_EQ(expected_output, output);
    EXPECT_EQ(expected_left, left);
    EXPECT_EQ(expected_right, right);
    EXPECT_EQ(mark, save_stack());
    EXPECT_EQ(high_water, temporary.high_water_mark());
    EXPECT_EQ(std::vector<Byte>(temporary.block_size(), 0x69),
        (std::vector<Byte>{borrowed, borrowed + temporary.block_size()}));
    const BigArena::Allocation reused = temporary.allocate_double();
    EXPECT_EQ(product_address, reused.data());
    EXPECT_EQ(high_water, temporary.high_water_mark());
    if (!nonzero)
    {
        EXPECT_EQ(free_bytes, (std::vector<Byte>{reused.data(), reused.data() + 2 * temporary.block_size()}));
    }
}

INSTANTIATE_TEST_SUITE_P(LayoutsAndOperations, TestBigFloatByteCanonicalProduct,
    testing::Combine(testing::Values(4, 12, 52, 56, 64, 708),
        testing::Values(BigFloatByteProductInput{2, 3, 0, 0, 6, 0}, BigFloatByteProductInput{-2, 3, 0, 0, -6, 0},
            BigFloatByteProductInput{2, -3, 0, 0, -6, 0}, BigFloatByteProductInput{-2, -3, 0, 0, 6, 0},
            BigFloatByteProductInput{2, 3, 500, 700, 6, 1200}, BigFloatByteProductInput{-2, 3, -500, -700, -6, -1200},
            BigFloatByteProductInput{0, 3, 0, 700, 0, 0}, BigFloatByteProductInput{-2, 0, -500, 0, 0, 0},
            BigFloatByteProductInput{-3, -3, 1000, 1000, 9, 2000, true},
            BigFloatByteProductInput{2, 0, 0, 0, 4, 0, false, true},
            BigFloatByteProductInput{-3, 0, 0, 0, 9, 0, false, true},
            BigFloatByteProductInput{-2, 0, 500, 0, 4, 1000, false, true},
            BigFloatByteProductInput{3, 0, -700, 0, 9, -1400, false, true},
            BigFloatByteProductInput{0, 0, 0, 0, 0, 0, false, true})));

class TestBigFloatByteReduction : public TestBigFloatBytePrecision, public testing::WithParamInterface<int>
{
protected:
    void SetUp() override
    {
        init_bf_length(GetParam());
    }
};

TEST_P(TestBigFloatByteReduction, copiesOnlyTheCanonicalWindowIncludingTheExponent)
{
    std::vector<Byte> guarded(g_r_bf_length + 6, 0xa5);
    for (int i = 0; i < g_r_bf_length + 2; ++i)
    {
        guarded[2 + i] = static_cast<Byte>(37 * i + 19);
    }
    std::vector<Byte> expected = guarded;
    std::copy_n(guarded.begin() + 2 + g_padding, g_bf_length + 2, expected.begin() + 2);

    reduce_bf_product(guarded.data() + 2);

    EXPECT_EQ(expected, guarded);
}

INSTANTIATE_TEST_SUITE_P(Precisions, TestBigFloatByteReduction, testing::Values(4, 12, 52, 56, 64, 708));

namespace
{

// Encoded pi independently verified with Chudnovsky and Gauss-Legendre.
// 710 fractional bytes, two integer bytes, and a two-byte zero exponent.
constexpr Byte PI_BYTES[] = {
    0x4F, 0x8C, 0xC6, 0x32, 0x67, 0xAC, 0xC3, 0xFF, 0x7E, 0x28, 0xF6, 0xA8, 0x42, 0xD5, 0xDB, 0x69, 0x17, 0xDF, 0x2E,
    0x56, 0x87, 0x1A, 0xA0, 0x8C, 0x6F, 0xCA, 0xBB, 0x57, 0x5C, 0x9E, 0x82, 0xDF, 0x00, 0x3E, 0x48, 0x7B, 0x31, 0x53,
    0x60, 0x87, 0x23, 0xFD, 0xFA, 0xB5, 0x3D, 0x32, 0xAB, 0x52, 0x05, 0xAD, 0xC8, 0x1E, 0x50, 0x2F, 0x15, 0x6B, 0x61,
    0xFD, 0xDF, 0x16, 0x75, 0x3C, 0xF8, 0x22, 0x32, 0xDB, 0xF8, 0xE9, 0xA5, 0x8E, 0xCC, 0xA3, 0x1F, 0xFB, 0xFE, 0x25,
    0x9F, 0x67, 0x79, 0x72, 0x2C, 0x40, 0xC6, 0x00, 0xA1, 0xD6, 0x0A, 0x32, 0x60, 0x1A, 0xBD, 0xC0, 0x79, 0x55, 0xDB,
    0xFB, 0xD3, 0xB9, 0x39, 0x5F, 0x0B, 0xD2, 0x0F, 0x74, 0xC8, 0x45, 0x57, 0xA8, 0xCB, 0xC0, 0xB3, 0x4B, 0x2E, 0x19,
    0x07, 0x28, 0x0F, 0x66, 0xFD, 0x4A, 0x33, 0xDE, 0x04, 0xD0, 0xE3, 0xBE, 0x09, 0xBD, 0x5E, 0xAF, 0x44, 0x45, 0x81,
    0xCC, 0x2C, 0x95, 0x30, 0x9B, 0x1F, 0x51, 0xFC, 0x6D, 0x6F, 0xEC, 0x52, 0x3B, 0xEB, 0xB2, 0x39, 0x13, 0xB5, 0x53,
    0x6C, 0x3E, 0xAF, 0x6F, 0xFB, 0x68, 0x63, 0x24, 0x6A, 0x19, 0xC2, 0x9E, 0x5C, 0x5E, 0xC4, 0x60, 0x9F, 0x40, 0xB6,
    0x4F, 0xA9, 0xC1, 0xBA, 0x06, 0xC0, 0x04, 0xBD, 0xE0, 0x6C, 0x97, 0x3B, 0x4C, 0x79, 0xB6, 0x1A, 0x50, 0xFE, 0xE3,
    0xF7, 0xDE, 0xE8, 0xF6, 0xD8, 0x79, 0xD4, 0x25, 0x7B, 0x1B, 0x99, 0x80, 0xC9, 0x72, 0x53, 0x07, 0x9B, 0xC0, 0xF1,
    0x49, 0xD3, 0xEA, 0x0F, 0xDB, 0x48, 0x12, 0x0A, 0xD0, 0x24, 0xD7, 0xD0, 0x37, 0x3D, 0x02, 0x9B, 0x42, 0x72, 0xDF,
    0xFE, 0x1B, 0x06, 0x77, 0x3F, 0x36, 0x62, 0xAA, 0xD3, 0x4E, 0xA6, 0x6A, 0xC1, 0x56, 0x9F, 0x44, 0x1A, 0x40, 0x73,
    0x20, 0xC1, 0x85, 0xD8, 0x75, 0x6F, 0xE0, 0xBE, 0x5E, 0x8B, 0x3B, 0xC3, 0xA5, 0x84, 0x7D, 0xB4, 0x9F, 0x6F, 0x45,
    0x19, 0x86, 0xEE, 0x8C, 0x88, 0x0E, 0x43, 0x82, 0x3E, 0x59, 0xCA, 0x66, 0x76, 0x01, 0xAF, 0x39, 0x1D, 0x65, 0xF1,
    0xA1, 0x98, 0x2A, 0xFB, 0x7E, 0x50, 0xF0, 0x3B, 0xBA, 0xE4, 0x3B, 0x7A, 0x13, 0x6C, 0x0B, 0xEF, 0x6E, 0xA3, 0x33,
    0x51, 0xAB, 0x28, 0xA7, 0x0F, 0x96, 0x68, 0x2F, 0x54, 0xD8, 0xD2, 0xA0, 0x51, 0x6A, 0xF0, 0x88, 0xD3, 0xAB, 0x61,
    0x9C, 0x0C, 0x67, 0x9A, 0x6C, 0xE9, 0xF6, 0x42, 0x68, 0xC6, 0x21, 0x5E, 0x9B, 0x1F, 0x9E, 0x4A, 0xF0, 0xC8, 0x69,
    0x04, 0x20, 0x84, 0xA4, 0x82, 0x44, 0x0B, 0x2E, 0x39, 0x42, 0xF4, 0x83, 0xF3, 0x6F, 0x6D, 0x0F, 0xC5, 0xAC, 0x96,
    0xD3, 0x81, 0x3E, 0x89, 0x23, 0x88, 0x1B, 0x65, 0xEB, 0x02, 0x23, 0x26, 0xDC, 0xB1, 0x75, 0x85, 0xE9, 0x5D, 0x5D,
    0x84, 0xEF, 0x32, 0x80, 0xEC, 0x5D, 0x60, 0xAC, 0x7C, 0x48, 0x91, 0xA9, 0x21, 0xFB, 0xCC, 0x09, 0xD8, 0x61, 0x93,
    0x21, 0x28, 0x66, 0x1B, 0xE8, 0xBF, 0xC4, 0xAF, 0xB9, 0x4B, 0x6B, 0x98, 0x48, 0x8F, 0x3B, 0x77, 0x86, 0x95, 0x28,
    0x81, 0x53, 0x32, 0x7A, 0x5C, 0xCF, 0x24, 0x6C, 0x33, 0xBA, 0xD6, 0xAF, 0x1E, 0x93, 0x87, 0x9B, 0x16, 0x3E, 0x5C,
    0xCE, 0xF6, 0x31, 0x18, 0x74, 0x5D, 0xC5, 0xA9, 0x2B, 0x2A, 0xBC, 0x6F, 0x63, 0x11, 0x14, 0xEE, 0xB3, 0x93, 0xE9,
    0x72, 0x7C, 0xAF, 0x86, 0x54, 0xA1, 0xCE, 0xE8, 0x41, 0x11, 0x34, 0x5C, 0xCC, 0xB4, 0xB6, 0x10, 0xAB, 0x2A, 0x6A,
    0x39, 0xCA, 0x55, 0x40, 0x14, 0xE8, 0x63, 0x62, 0x98, 0x48, 0x57, 0x94, 0xAB, 0x55, 0xAA, 0xF3, 0x25, 0x55, 0xE6,
    0x60, 0x5C, 0x60, 0x55, 0xDA, 0x2F, 0xAF, 0x78, 0x27, 0x4B, 0x31, 0xBD, 0xC1, 0x77, 0x15, 0xD7, 0x3E, 0x8A, 0x1E,
    0xB0, 0x8B, 0x0E, 0x9E, 0x6C, 0x0E, 0x18, 0x3A, 0x60, 0xB0, 0xDC, 0x79, 0x8E, 0xEF, 0x38, 0xDB, 0xB8, 0x18, 0x79,
    0x41, 0xCA, 0xF0, 0x85, 0x60, 0x28, 0x23, 0xB0, 0xD1, 0xC5, 0x13, 0x60, 0xF2, 0x2A, 0x39, 0xD5, 0x30, 0x9C, 0xB5,
    0x59, 0x5A, 0xC2, 0x1D, 0xA4, 0x54, 0x7B, 0xEE, 0x4A, 0x15, 0x82, 0x58, 0xCD, 0x8B, 0x71, 0x58, 0xB6, 0x8E, 0x72,
    0x8F, 0x74, 0x95, 0x0D, 0x7E, 0x3D, 0x93, 0xF4, 0xA3, 0xFE, 0x58, 0xA4, 0x69, 0x4E, 0x57, 0x71, 0xD8, 0x20, 0x69,
    0x63, 0x16, 0xFC, 0x8E, 0x85, 0xE2, 0xF2, 0x01, 0x08, 0xF7, 0x6C, 0x91, 0xB3, 0x47, 0x99, 0xA1, 0x24, 0x99, 0x7F,
    0x2C, 0xF1, 0x45, 0x90, 0x7C, 0xBA, 0x96, 0x7E, 0x26, 0x6A, 0xED, 0xAF, 0xE1, 0xB8, 0xB7, 0xDF, 0x1A, 0xD0, 0xDB,
    0x72, 0xFD, 0x2F, 0xAC, 0xB5, 0xDF, 0x98, 0xA6, 0x0B, 0x31, 0xD1, 0x1B, 0xFB, 0x79, 0x89, 0xD9, 0xD5, 0x16, 0x92,
    0x17, 0x09, 0x47, 0xB5, 0xB5, 0xD5, 0x84, 0x3F, 0xDD, 0x50, 0x7C, 0xC9, 0xB7, 0x29, 0xAC, 0xC0, 0x6C, 0x0C, 0xE9,
    0x34, 0xCF, 0x66, 0x54, 0xBE, 0x77, 0x13, 0xD0, 0x38, 0xE6, 0x21, 0x28, 0x45, 0x89, 0x6C, 0x4E, 0xEC, 0x98, 0xFA,
    0x2E, 0x08, 0xD0, 0x31, 0x9F, 0x29, 0x22, 0x38, 0x09, 0xA4, 0x44, 0x73, 0x70, 0x03, 0x2E, 0x8A, 0x19, 0x13, 0xD3,
    0x08, 0xA3, 0x85, 0x88, 0x6A, 0x3F, 0x24,
    /* . */ 0x03, 0x00, 0x00, 0x00
    //  <- up to g_int_length 4 ->
    // or BigFloat int length of 2 + 2 byte exp
};

} // namespace

class TestBigFloatBytePi : public TestBigFloatBytePrecision, public testing::WithParamInterface<int>
{
protected:
    void SetUp() override
    {
        init_bf_length(GetParam());
    }
};

TEST_P(TestBigFloatBytePi, copiesTheRequestedTableWindowAndPreservesGuards)
{
    const int size = g_bf_length + 2;
    ASSERT_LE(size, static_cast<int>(std::size(PI_BYTES)));
    std::vector<Byte> guarded(size + 4, 0xa5);
    std::vector<Byte> expected = guarded;
    std::copy_n(std::end(PI_BYTES) - size, size, expected.begin() + 2);

    copy_bf_pi(guarded.data() + 2);

    EXPECT_EQ(expected, guarded);
}

INSTANTIATE_TEST_SUITE_P(Precisions, TestBigFloatBytePi, testing::Values(4, 12, 56, 64, 708));

TEST(TestBigFloatByteAdditionBounds, writesOnlyTheBorrowedByteWindow)
{
    const ValueSaver<int> bf_length{g_bf_length, 8};
    const ValueSaver<int> bn_length{g_bn_length, 4};
    std::vector<Byte> left = integer_bf_bytes(g_bf_length, 2, 0);
    std::vector<Byte> right = integer_bf_bytes(g_bf_length, 1, 0);
    left[g_bf_length - 3] = 0x80;
    right[g_bf_length - 3] = 0x40;
    std::vector<Byte> expected = integer_bf_bytes(g_bf_length, 3, 0);
    expected[g_bf_length - 3] = 0xc0;
    std::vector<Byte> guarded(expected.size() + 4, 0xa5);
    const std::vector<Byte> original_left = left;
    const std::vector<Byte> original_right = right;
    std::vector<Byte> expected_output = guarded;
    std::copy(expected.begin(), expected.end(), expected_output.begin() + 2);
    Byte *const result = guarded.data() + 2;

    EXPECT_EQ(result, unsafe_add_bf(result, left.data(), right.data()));

    EXPECT_EQ(expected_output, guarded);
    EXPECT_EQ(original_left, left);
    EXPECT_EQ(original_right, right);
}

struct BigFloatByteScalarMultiplyInput
{
    bool negative;
    U16 factor;
    std::array<Byte, 6> result_tail;
};

class TestBigFloatByteScalarMultiply : public testing::TestWithParam<BigFloatByteScalarMultiplyInput>
{
};

TEST_P(TestBigFloatByteScalarMultiply, preservesExtentAndLegacyNormalization)
{
    constexpr int length = 8;
    constexpr int size = length + 2;
    const ValueSaver<int> bf_length{g_bf_length, length};
    const ValueSaver<int> bn_length{g_bn_length, 4};
    const BigFloatByteScalarMultiplyInput &input = GetParam();
    alignas(U16) std::array<Byte, size + 2> source{};
    source[length - 2] = input.negative ? 0xfe : 2;
    source[length - 1] = input.negative ? 0xff : 0;
    source.back() = 0xa5;
    const std::array<Byte, size + 2> original = source;
    std::array<Byte, size + 2> expected_source = original;
    if (input.factor > 255)
    {
        expected_source[length - 3] = original[length - 2];
        expected_source[length - 2] = original[length - 1];
    }
    alignas(U16) std::array<Byte, size + 2> result{};
    result.back() = 0xa5;
    std::array<Byte, size + 2> expected{};
    std::copy(input.result_tail.begin(), input.result_tail.end(), expected.begin() + size - 6);
    expected.back() = 0xa5;

    EXPECT_EQ(result.data(), unsafe_mult_bf_int(result.data(), source.data(), input.factor));
    EXPECT_EQ(expected, result);
    EXPECT_EQ(expected_source, source);
    EXPECT_EQ(4, g_bn_length);

    source = original;
    EXPECT_EQ(source.data(), mult_a_bf_int(source.data(), input.factor));
    EXPECT_EQ(expected, source);
    EXPECT_EQ(4, g_bn_length);
    EXPECT_EQ(length, g_bf_length);
}

INSTANTIATE_TEST_SUITE_P(Factors, TestBigFloatByteScalarMultiply,
    testing::Values(BigFloatByteScalarMultiplyInput{false, 0, {}},
        BigFloatByteScalarMultiplyInput{true, 0, {0, 0, 0, 0xff, 0, 0}},
        BigFloatByteScalarMultiplyInput{false, 255, {0, 0xfe, 1, 0, 1, 0}},
        BigFloatByteScalarMultiplyInput{true, 256, {0, 0, 0xfe, 0xff, 1, 0}},
        BigFloatByteScalarMultiplyInput{false, 65535, {0xfe, 0xff, 1, 0, 2, 0}}));

struct BigFloatByteDoublingInput
{
    std::array<Byte, 5> source_tail;
    std::array<Byte, 5> result_tail;
};

class TestBigFloatByteDoubling : public testing::TestWithParam<BigFloatByteDoublingInput>
{
};

TEST_P(TestBigFloatByteDoubling, preservesExponentNormalizationAndCanonicalExtent)
{
    constexpr int length = 8;
    constexpr int size = length + 2;
    const ValueSaver<int> bf_length{g_bf_length, length};
    const ValueSaver<int> bn_length{g_bn_length, 4};
    const BigFloatByteDoublingInput &input = GetParam();
    alignas(U16) std::array<Byte, size + 2> source{};
    std::copy(input.source_tail.begin(), input.source_tail.end(), source.begin() + size - 5);
    source.back() = 0xa5;
    const std::array<Byte, size + 2> original = source;
    std::array<Byte, size + 2> expected{};
    std::copy(input.result_tail.begin(), input.result_tail.end(), expected.begin() + size - 5);
    expected.back() = 0xa5;
    alignas(U16) std::array<Byte, size + 2> result{};
    result.back() = 0xa5;

    EXPECT_EQ(result.data(), double_bf(result.data(), source.data()));
    EXPECT_EQ(expected, result);
    EXPECT_EQ(original, source);
    EXPECT_EQ(4, g_bn_length);

    EXPECT_EQ(source.data(), double_bf(source.data(), source.data()));
    EXPECT_EQ(expected, source);
    EXPECT_EQ(4, g_bn_length);

    source = original;
    EXPECT_EQ(source.data(), double_a_bf(source.data()));
    EXPECT_EQ(expected, source);
    EXPECT_EQ(4, g_bn_length);
    EXPECT_EQ(length, g_bf_length);
}

INSTANTIATE_TEST_SUITE_P(Layouts, TestBigFloatByteDoubling,
    testing::Values(BigFloatByteDoublingInput{{0, 0, 0, 0x12, 0}, {}},
        BigFloatByteDoublingInput{{0x40, 1, 0, 0, 0}, {0x80, 2, 0, 0, 0}},
        BigFloatByteDoublingInput{{0, 0x80, 0, 0, 0}, {0, 1, 0, 1, 0}},
        BigFloatByteDoublingInput{{0, 0x7f, 0xff, 0, 0}, {0xfe, 0xfe, 0xff, 1, 0}}));

struct BigFloatByteHalvingInput
{
    std::array<Byte, 5> source_tail;
    std::array<Byte, 5> result_tail;
    Byte source_low{};
    Byte result_low{};
};

class TestBigFloatByteHalving : public testing::TestWithParam<BigFloatByteHalvingInput>
{
};

TEST_P(TestBigFloatByteHalving, preservesRoundingNormalizationAndCanonicalExtent)
{
    constexpr int length = 8;
    constexpr int size = length + 2;
    const ValueSaver<int> bf_length{g_bf_length, length};
    const ValueSaver<int> bn_length{g_bn_length, 4};
    const BigFloatByteHalvingInput &input = GetParam();
    alignas(U16) std::array<Byte, size + 2> source{};
    std::copy(input.source_tail.begin(), input.source_tail.end(), source.begin() + size - 5);
    source.front() = input.source_low;
    source.back() = 0xa5;
    const std::array<Byte, size + 2> original = source;
    std::array<Byte, size + 2> expected{};
    std::copy(input.result_tail.begin(), input.result_tail.end(), expected.begin() + size - 5);
    expected.front() = input.result_low;
    expected.back() = 0xa5;
    alignas(U16) std::array<Byte, size + 2> result{};
    result.back() = 0xa5;

    EXPECT_EQ(result.data(), half_bf(result.data(), source.data()));
    EXPECT_EQ(expected, result);
    EXPECT_EQ(original, source);
    EXPECT_EQ(4, g_bn_length);

    EXPECT_EQ(source.data(), half_bf(source.data(), source.data()));
    EXPECT_EQ(expected, source);
    EXPECT_EQ(4, g_bn_length);

    source = original;
    EXPECT_EQ(source.data(), half_a_bf(source.data()));
    EXPECT_EQ(expected, source);
    EXPECT_EQ(4, g_bn_length);
    EXPECT_EQ(length, g_bf_length);
}

INSTANTIATE_TEST_SUITE_P(Layouts, TestBigFloatByteHalving,
    testing::Values(BigFloatByteHalvingInput{{0, 0, 0, 0x12, 0}, {}},
        BigFloatByteHalvingInput{{0, 1, 0, 0, 0}, {0, 0x80, 0, 0xff, 0xff}},
        BigFloatByteHalvingInput{{0, 0xfe, 0xff, 0, 0}, {0, 0, 0xff, 0xff, 0xff}},
        BigFloatByteHalvingInput{{0, 0xfd, 0xff, 0, 0}, {0x80, 0xfe, 0xff, 0, 0}, 3, 1}));

class TestBigFloatByteReciprocal : public TestBigNumConversionBase, public testing::WithParamInterface<int>
{
};

TEST_P(TestBigFloatByteReciprocal, restoresPrecisionAndKeepsDestructiveWindowEffects)
{
    init_bf_length(GetParam());
    const int size = g_bf_length + 2;
    const std::array<int, 6> lengths{g_bn_length, g_bf_length, g_padding, g_r_length, g_r_bf_length, g_shift_factor};
    constexpr std::array<Byte, 4> source_tail{0xfe, 0xff, 1, 0}; // -2 * 256
    constexpr std::array<Byte, 4> result_tail{0x80, 0xff, 0xfe, 0xff};
    std::vector<Byte> source(size + 2);
    std::copy(source_tail.begin(), source_tail.end(), source.begin() + size - 4);
    source.back() = 0xa5;
    std::vector<Byte> result(size + 2);
    std::fill(result.begin() + size, result.end(), Byte{0xa5});

    std::vector<Byte> tmp1(size + 2, 0xa5);
    std::vector<Byte> tmp2(size + 2, 0xa5);

    EXPECT_EQ(result.data(), unsafe_inv_bf(result.data(), source.data(), tmp1.data(), tmp2.data()));

    std::vector<Byte> expected_source(size + 2);
    expected_source[size - 4] = 2;
    expected_source.back() = 0xa5;
    std::vector<Byte> expected_result(size + 2);
    std::copy(result_tail.begin(), result_tail.end(), expected_result.begin() + size - 4);
    std::fill(expected_result.begin() + size, expected_result.end(), Byte{0xa5});
    EXPECT_EQ(expected_source, source);
    EXPECT_EQ(expected_result, result);
    std::vector<Byte> expected_tmp(size + 2);
    expected_tmp[size - 4] = 1;
    std::fill(expected_tmp.begin() + size, expected_tmp.end(), Byte{0xa5});
    EXPECT_EQ(expected_tmp, tmp1);
    EXPECT_EQ(expected_tmp, tmp2);
    EXPECT_EQ(
        lengths, (std::array<int, 6>{g_bn_length, g_bf_length, g_padding, g_r_length, g_r_bf_length, g_shift_factor}));
}

INSTANTIATE_TEST_SUITE_P(Precisions, TestBigFloatByteReciprocal, testing::Values(4, 64));

class TestBigFloatByteDivision : public TestBigNumConversionBase, public testing::WithParamInterface<int>
{
};

TEST_P(TestBigFloatByteDivision, reappliesExponentsAndKeepsDestructiveOperandEffects)
{
    init_bf_length(GetParam());
    const int size = g_bf_length + 2;
    const std::array<int, 6> lengths{g_bn_length, g_bf_length, g_padding, g_r_length, g_r_bf_length, g_shift_factor};
    std::vector<Byte> numerator(size + 2);
    std::vector<Byte> denominator(size + 2);
    std::vector<Byte> result(size + 2);
    constexpr std::array<Byte, 6> numerator_tail{0xf4, 0xff, 2, 0, 0xa5, 0xa5};   // -12 * 256^2
    constexpr std::array<Byte, 6> denominator_tail{0xfc, 0xff, 1, 0, 0xa5, 0xa5}; // -4 * 256
    std::copy(numerator_tail.begin(), numerator_tail.end(), numerator.end() - 6);
    std::copy(denominator_tail.begin(), denominator_tail.end(), denominator.end() - 6);
    std::fill(result.begin() + size, result.end(), Byte{0xa5});

    std::vector<Byte> tmp1(size + 2, 0xa5);
    std::vector<Byte> tmp2(size + 2, 0xa5);

    EXPECT_EQ(
        result.data(), unsafe_div_bf(result.data(), numerator.data(), denominator.data(), tmp1.data(), tmp2.data()));

    std::vector<Byte> expected(size + 2);
    constexpr std::array<Byte, 6> result_tail{3, 0, 1, 0, 0xa5, 0xa5};
    std::copy(result_tail.begin(), result_tail.end(), expected.end() - 6);
    EXPECT_EQ(expected, result);
    expected[size - 2] = 0;
    EXPECT_EQ(expected, tmp1);
    expected[size - 4] = 1;
    EXPECT_EQ(expected, tmp2);
    expected[size - 4] = 12;
    EXPECT_EQ(expected, numerator);
    expected[size - 4] = 4;
    EXPECT_EQ(expected, denominator);
    EXPECT_EQ(
        lengths, (std::array<int, 6>{g_bn_length, g_bf_length, g_padding, g_r_length, g_r_bf_length, g_shift_factor}));
}

INSTANTIATE_TEST_SUITE_P(Precisions, TestBigFloatByteDivision, testing::Values(4, 64));

struct BigFloatByteScalarDivisionInput
{
    std::array<Byte, 4> source_tail;
    U16 divisor;
    std::array<Byte, 4> result_tail;
};

class TestBigFloatByteScalarDivision : public testing::TestWithParam<BigFloatByteScalarDivisionInput>
{
};

TEST_P(TestBigFloatByteScalarDivision, preservesShortenedExtentAndMakesTheSourcePositive)
{
    constexpr int length = 8;
    constexpr int size = length + 2;
    const ValueSaver<int> bf_length{g_bf_length, length};
    const ValueSaver<int> bn_length{g_bn_length, 4};
    const BigFloatByteScalarDivisionInput &input = GetParam();
    alignas(U16) std::array<Byte, size + 2> source{};
    std::copy(input.source_tail.begin(), input.source_tail.end(), source.begin() + length - 2);
    source.back() = 0xa5;
    const std::array<Byte, size + 2> original = source;
    std::array<Byte, size + 2> expected_source = original;
    expected_source[length - 2] = 6;
    expected_source[length - 1] = 0;
    alignas(U16) std::array<Byte, size + 2> result{};
    result.back() = 0xa5;
    std::array<Byte, size + 2> expected{};
    std::copy(input.result_tail.begin(), input.result_tail.end(), expected.begin() + length - 2);
    expected.back() = 0xa5;

    EXPECT_EQ(result.data(), unsafe_div_bf_int(result.data(), source.data(), input.divisor));
    EXPECT_EQ(expected, result);
    EXPECT_EQ(expected_source, source);
    EXPECT_EQ(4, g_bn_length);

    source = original;
    EXPECT_EQ(source.data(), unsafe_div_bf_int(source.data(), source.data(), input.divisor));
    EXPECT_EQ(expected, source);
    EXPECT_EQ(4, g_bn_length);

    source = original;
    EXPECT_EQ(source.data(), div_a_bf_int(source.data(), input.divisor));
    EXPECT_EQ(expected, source);
    EXPECT_EQ(4, g_bn_length);
    EXPECT_EQ(length, g_bf_length);
}

INSTANTIATE_TEST_SUITE_P(Divisors, TestBigFloatByteScalarDivision,
    testing::Values(BigFloatByteScalarDivisionInput{{0xfa, 0xff, 2, 0}, 2, {0xfd, 0xff, 2, 0}},
        BigFloatByteScalarDivisionInput{{6, 0, 2, 0}, 32768, {12, 0, 0, 0}}));

TEST(TestBigFloatByteScalarDivisionZero, distinguishesAliasedAndInPlaceSaturation)
{
    constexpr int length = 8;
    constexpr int size = length + 2;
    constexpr int max_exp = std::numeric_limits<LDouble>::max_exponent / 8;
    const ValueSaver<int> bf_length{g_bf_length, length};
    const ValueSaver<int> bn_length{g_bn_length, 4};
    alignas(U16) std::array<Byte, size + 2> source{0, 0, 0, 0, 0, 0, 0xfa, 0xff, 2, 0, 0, 0xa5};
    const std::array<Byte, size + 2> original = source;
    alignas(U16) std::array<Byte, size + 2> result{};
    result.back() = 0xa5;
    const std::array<Byte, size + 2> positive{
        0, 0, 0, 0, 0, 0, 1, 0, static_cast<Byte>(max_exp), static_cast<Byte>(max_exp >> 8), 0, 0xa5};
    const std::array<Byte, size + 2> negative{
        0, 0, 0, 0, 0, 0, 0, 0xff, static_cast<Byte>(max_exp - 1), static_cast<Byte>((max_exp - 1) >> 8), 0, 0xa5};

    EXPECT_EQ(result.data(), unsafe_div_bf_int(result.data(), source.data(), 0));
    EXPECT_EQ(negative, result);
    EXPECT_EQ(original, source);

    EXPECT_EQ(source.data(), unsafe_div_bf_int(source.data(), source.data(), 0));
    EXPECT_EQ(positive, source);

    source = original;
    EXPECT_EQ(source.data(), div_a_bf_int(source.data(), 0));
    EXPECT_EQ(negative, source);
    EXPECT_EQ(4, g_bn_length);
    EXPECT_EQ(length, g_bf_length);
}

class TestBigFloatByteExponential : public TestBigNumConversionBase
{
};

TEST_F(TestBigFloatByteExponential, truncatesTheFirstTinyTermWithinTheBorrowedWindows)
{
    init_bf_length(4);
    constexpr int length = 8;
    constexpr int size = length + 2;
    constexpr int offset = 4;
    ASSERT_EQ(length, g_bf_length);
    alignas(U16) std::array<Byte, offset + size + 2> source{};
    std::fill_n(source.begin(), offset, Byte{0xa5});
    source[offset + length - 2] = 1;
    source[offset + length] = 0xf8; // exponent -8
    source[offset + length + 1] = 0xff;
    source.back() = 0xa5;
    const std::array<Byte, offset + size + 2> original = source;
    alignas(U16) std::array<Byte, offset + size + 2> result{};
    result.fill(0xa5);
    Byte *const output = result.data() + offset;
    alignas(U16) std::array<Byte, size + 2> tmp1{};
    alignas(U16) std::array<Byte, size + 2> tmp2{};
    alignas(U16) std::array<Byte, size + 2> tmp3{};
    tmp1.fill(0xa5);
    tmp2.fill(0xa5);
    tmp3.fill(0xa5);
    const BigStackMark mark = save_stack();

    EXPECT_EQ(output, exp_bf(output, source.data() + offset, tmp1.data(), tmp2.data(), tmp3.data()));

    std::array<Byte, offset + size + 2> expected{};
    expected.fill(0xa5);
    std::fill_n(expected.begin() + offset, size, Byte{0});
    expected[offset + length - 2] = 1;
    EXPECT_EQ(expected, result);
    EXPECT_EQ(original, source);
    std::array<Byte, size + 2> term{};
    term.fill(0xa5);
    std::copy_n(original.begin() + offset, size, term.begin());
    EXPECT_EQ(term, tmp1);
    EXPECT_EQ(term, tmp2);
    EXPECT_EQ(term, tmp3);
    EXPECT_EQ(4, g_bn_length);
    EXPECT_EQ(length, g_bf_length);
    EXPECT_EQ(mark, save_stack());
}

class TestBigFloatByteLogarithm : public TestBigNumConversionBase
{
};

TEST_F(TestBigFloatByteLogarithm, confinesThePrecisionRampToTheBorrowedWindows)
{
    init_bf_length(64);
    const int size = g_bf_length + 2;
    const std::array<int, 6> lengths{g_bn_length, g_bf_length, g_padding, g_r_length, g_r_bf_length, g_shift_factor};
    std::vector<Byte> source(size + 4, 0xa5);
    std::fill_n(source.begin() + 2, size, Byte{0});
    source[size - 2] = 1;
    const std::vector<Byte> original = source;
    std::vector<Byte> result(size + 4, 0xa5);
    std::fill_n(result.begin() + 2, size, Byte{0});
    const std::vector<Byte> expected = result;
    Byte *const output = result.data() + 2;
    std::vector<Byte> expected_scratch(size + 2);
    expected_scratch[size] = 0xa5;
    expected_scratch[size + 1] = 0xa5;
    std::vector<Byte> tmp5 = expected_scratch;
    const BigStackMark mark = save_stack();

    EXPECT_EQ(output,
        unsafe_ln_bf(output, source.data() + 2, g_bf_tmp1.data(), g_bf_tmp2.data(), g_bf_tmp3.data(), g_bf_tmp4.data(),
            tmp5.data(), g_bf_tmp6.data()));

    EXPECT_EQ(expected, result);
    EXPECT_EQ(original, source);
    EXPECT_EQ(expected_scratch, tmp5);
    EXPECT_EQ(
        lengths, (std::array<int, 6>{g_bn_length, g_bf_length, g_padding, g_r_length, g_r_bf_length, g_shift_factor}));
    EXPECT_EQ(mark, save_stack());
}

class TestBigFloatByteSineCosine : public TestBigNumConversionBase
{
};

TEST_F(TestBigFloatByteSineCosine, reducesTheBorrowedHalfPiWindowAndPreservesGuards)
{
    init_bf_length(4);
    constexpr int length = 8;
    constexpr int size = length + 2;
    constexpr int offset = 4;
    ASSERT_EQ(length, g_bf_length);
    // Use an exact pi substitute of 3.125 so the half-pi branch is exact.
    alignas(U16) std::array<Byte, offset + size + 2> pi{
        0xa5, 0xa5, 0xa5, 0xa5, 0, 0, 0, 0, 0, 0x20, 3, 0, 0, 0, 0xa5, 0xa5};
    const std::array<Byte, offset + size + 2> original_pi = pi;
    alignas(U16) std::array<Byte, offset + size + 2> input{
        0xa5, 0xa5, 0xa5, 0xa5, 0, 0, 0, 0, 0, 0x70, 0xfe, 0xff, 0, 0, 0xa5, 0xa5};
    alignas(U16) std::array<Byte, size + 2> sine{};
    alignas(U16) std::array<Byte, size + 2> cosine{};
    sine.fill(0xa5);
    cosine.fill(0xa5);
    int_to_bf(g_bf_pi.data(), 4);
    alignas(U16) std::array<Byte, size + 2> tmp1{};
    alignas(U16) std::array<Byte, size + 2> tmp2{};
    tmp1.fill(0xa5);
    tmp2.fill(0xa5);
    const BigStackMark mark = save_stack();

    EXPECT_EQ(sine.data(),
        unsafe_sin_cos_bf(
            sine.data(), cosine.data(), input.data() + offset, pi.data() + offset, tmp1.data(), tmp2.data()));

    std::array<Byte, offset + size + 2> reduced{};
    reduced.fill(0xa5);
    std::fill_n(reduced.begin() + offset, size, Byte{0});
    constexpr std::array<Byte, size + 2> zero{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xa5, 0xa5};
    constexpr std::array<Byte, size + 2> one{0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0xa5, 0xa5};
    constexpr std::array<Byte, size + 2> half_pi{0, 0, 0, 0, 0, 0x90, 1, 0, 0, 0, 0xa5, 0xa5};
    EXPECT_EQ(reduced, input);
    EXPECT_EQ(zero, sine);
    EXPECT_EQ(one, cosine);
    EXPECT_EQ(original_pi, pi);
    EXPECT_EQ(half_pi, tmp1);
    EXPECT_EQ(half_pi, tmp2);
    EXPECT_EQ(4, g_bn_length);
    EXPECT_EQ(length, g_bf_length);
    EXPECT_EQ(mark, save_stack());
}

class TestBigFloatByteArctangent : public TestBigNumConversionBase
{
};

TEST_F(TestBigFloatByteArctangent, restoresPrecisionAndKeepsLargeArgumentScratchEffects)
{
    init_bf_length(64);
    init_big_pi();
    const int size = g_bf_length + 2;
    const std::array<int, 6> lengths{g_bn_length, g_bf_length, g_padding, g_r_length, g_r_bf_length, g_shift_factor};
    std::vector<Byte> source(size + 4, 0xa5);
    Byte *const input = source.data() + 2;
    std::fill_n(input, size, Byte{0});
    input[size - 4] = 2;
    std::vector<Byte> result(size + 4, 0xa5);
    Byte *const output = result.data() + 2;
    std::fill_n(output, size, Byte{0});
    std::vector<Byte> tmp3(size + 2);
    std::fill(tmp3.begin() + size, tmp3.end(), Byte{0xa5});
    std::vector<Byte> copy1(size + 2, 0xa5);
    const BigStackMark mark = save_stack();

    EXPECT_EQ(output,
        arctangent(output, input, g_bf_pi.data(), g_bf_tmp1.data(), g_bf_tmp2.data(), tmp3.data(), g_bf_tmp4.data(),
            g_bf_tmp5.data(), copy1.data()));

    std::vector<Byte> reciprocal(size + 4, 0xa5);
    std::fill_n(reciprocal.begin() + 2, size, Byte{0});
    reciprocal[size - 2] = 0x80;
    reciprocal[size] = 0xff;
    reciprocal[size + 1] = 0xff;
    EXPECT_EQ(reciprocal, source);
    EXPECT_EQ(std::vector<Byte>(output, output + size + 2), tmp3);
    EXPECT_EQ(0, BIG_ACCESS16(copy1.data() + g_bf_length));
    EXPECT_EQ(0xa5, copy1[size]);
    EXPECT_EQ(0xa5, copy1[size + 1]);
    EXPECT_EQ(0xa5, result.front());
    EXPECT_EQ(0xa5, result.back());
    EXPECT_EQ(
        lengths, (std::array<int, 6>{g_bn_length, g_bf_length, g_padding, g_r_length, g_r_bf_length, g_shift_factor}));
    EXPECT_EQ(mark, save_stack());
    EXPECT_NEAR(1.1071487177940905L, bf_to_float(output), 1e-11L);
}

class TestBigFloatByteArctangent2 : public TestBigNumConversionBase,
                                    public testing::WithParamInterface<std::tuple<long, long>>
{
};

TEST_P(TestBigFloatByteArctangent2, preservesQuadrantReductionAndScratchWrites)
{
    init_bf_length(64);
    init_big_pi();
    const long y = std::get<0>(GetParam());
    const long x = std::get<1>(GetParam());
    const bool subtract_from_pi = x < 0 && y != 0;
    const int size = g_bf_length + 2;
    constexpr int offset = 2;
    std::vector<Byte> pi(size + 2 * offset, 0xa5);
    std::copy_n(g_bf_pi.data(), size, pi.begin() + offset);
    const std::vector<Byte> original_pi = pi;
    int_to_bf(g_bf_pi.data(), 4);
    std::vector<Byte> y_storage(size + 2 * offset, 0xa5);
    std::vector<Byte> x_storage(size + 2 * offset, 0xa5);
    std::vector<Byte> result(size + 2 * offset, 0xa5);
    Byte *const ny = y_storage.data() + offset;
    Byte *const nx = x_storage.data() + offset;
    Byte *const output = result.data() + offset;
    int_to_bf(ny, y);
    int_to_bf(nx, x);
    clear_bf(output);
    std::vector<Byte> copy1(size + offset, 0xa5);
    std::vector<Byte> copy2(size + offset, 0xa5);
    const BigStackMark mark = save_stack();

    EXPECT_EQ(output,
        unsafe_atan2_bf(output, ny, nx, pi.data() + offset, g_bf_tmp1.data(), g_bf_tmp2.data(), g_bf_tmp3.data(),
            g_bf_tmp4.data(), g_bf_tmp5.data(), g_bf_tmp6.data(), copy1.data(), copy2.data()));

    ASSERT_EQ(size, g_bf_length + 2);
    std::vector<Byte> expected_y(size + 2 * offset, 0xa5);
    std::vector<Byte> expected_x(size + 2 * offset, 0xa5);
    int_to_bf(expected_y.data() + offset, subtract_from_pi ? std::abs(y) : y);
    int_to_bf(expected_x.data() + offset, subtract_from_pi ? -x : x);
    EXPECT_EQ(expected_y, y_storage);
    EXPECT_EQ(expected_x, x_storage);
    EXPECT_EQ(original_pi, pi);
    EXPECT_EQ(0xa5, result.front());
    EXPECT_EQ(0xa5, result.back());
    EXPECT_EQ(mark, save_stack());
    EXPECT_NEAR(std::atan2(static_cast<LDouble>(y), static_cast<LDouble>(x)), bf_to_float(output), 1e-11L);
    std::vector<Byte> expected_copy1(size + offset, 0xa5);
    if (subtract_from_pi)
    {
        std::copy_n(pi.begin() + offset, size, expected_copy1.begin());
        EXPECT_EQ(0, BIG_ACCESS16(copy2.data() + g_bf_length));
        EXPECT_NEAR(0.4636476090008061L, bf_to_float(copy2.data()), 1e-11L);
        EXPECT_EQ(0xa5, copy2[size]);
        EXPECT_EQ(0xa5, copy2[size + 1]);
    }
    else
    {
        EXPECT_EQ(expected_copy1, copy2);
    }
    EXPECT_EQ(expected_copy1, copy1);
    if (y == 0)
    {
        EXPECT_EQ(std::vector<Byte>(pi.begin() + offset, pi.begin() + offset + size),
            std::vector<Byte>(output, output + size));
    }
}

INSTANTIATE_TEST_SUITE_P(QuadrantsAndAxes, TestBigFloatByteArctangent2,
    testing::Values(
        std::make_tuple(1L, -2L), std::make_tuple(-1L, -2L), std::make_tuple(0L, -2L), std::make_tuple(-1L, 0L)));

namespace
{

std::vector<Byte> fixed_sample(const int length, const int fractional_bytes, const int whole, const Byte fraction = 0)
{
    std::vector<Byte> result(length, whole < 0 ? Byte{0xff} : Byte{0});
    std::fill_n(result.begin(), fractional_bytes, Byte{0});
    if (fractional_bytes != 0)
    {
        result[fractional_bytes - 1] = fraction;
    }
    result[fractional_bytes] = static_cast<Byte>(whole);
    return result;
}

} // namespace

class TestBigNumByteKernel : public TestBigNumConversionBase
{
protected:
    void init_layout(const int length, const int integer_bytes)
    {
        engine::g_bailout_test = integer_bytes == 2 ? engine::Bailout::REAL : engine::Bailout::MOD;
        engine::g_user.bailout_value = integer_bytes == 4 ? 100 : 0;
        init_bf_length(length);
    }

private:
    ValueSaver<long> m_bailout_value{engine::g_user.bailout_value, 0};
    ValueSaver<engine::Bailout> m_bailout_test{engine::g_bailout_test, engine::Bailout::MOD};
    ValueSaver<fractals::FractalType> m_fractal_type{fractals::g_fractal_type, fractals::FractalType::MANDEL};
};

namespace
{

struct BigNumIntegerInput
{
    const char *name;
    std::uint32_t integer_bits;
    bool fractional;
    std::array<long, 3> expected;
};

void PrintTo(const BigNumIntegerInput &input, std::ostream *out)
{
    *out << input.name;
}

} // namespace

class TestBigNumByteIntegerObservation : public TestBigNumByteKernel,
                                         public testing::WithParamInterface<std::tuple<int, int, BigNumIntegerInput>>
{
protected:
    void SetUp() override;
};

void TestBigNumByteIntegerObservation::SetUp()
{
    const int integer_length = std::get<1>(GetParam());
    init_layout(std::get<0>(GetParam()), integer_length);
    ASSERT_EQ(integer_length, g_int_length);
}

TEST_P(TestBigNumByteIntegerObservation, extractsIntegerBitsWithoutChangingSourceBytes)
{
    const BigNumIntegerInput &input = std::get<2>(GetParam());
    std::vector<Byte> bytes(g_bn_length, input.fractional ? Byte{0x80} : Byte{0});
    for (int byte = 0; byte < g_int_length; ++byte)
    {
        bytes[g_bn_length - g_int_length + byte] = static_cast<Byte>(input.integer_bits >> (8 * byte));
    }
    bytes.push_back(0xa5);
    const std::vector<Byte> original = bytes;
    const int expected_index = g_int_length == 1 ? 0 : g_int_length == 2 ? 1 : 2;

    EXPECT_EQ(input.expected[expected_index], bn_to_int(bytes.data()));
    EXPECT_EQ(original, bytes);
}

INSTANTIATE_TEST_SUITE_P(LayoutsAndInputs, TestBigNumByteIntegerObservation,
    testing::Combine(testing::Values(4, 8, 12, 60, 64, 672), testing::Values(1, 2, 4),
        testing::Values(BigNumIntegerInput{"Zero", 0, false, {0, 0, 0}}, BigNumIntegerInput{"One", 1, false, {1, 1, 1}},
            BigNumIntegerInput{"BelowSign", 0x7fffffff, false, {255, 65535, 2147483647}},
            BigNumIntegerInput{"AtSign", 0x80000000, false, {0, 0, static_cast<long>(std::uint32_t{0x80000000})}},
            BigNumIntegerInput{
                "AllOnes", 0xffffffff, false, {255, 65535, static_cast<long>(std::uint32_t{0xffffffff})}},
            BigNumIntegerInput{"PositiveFraction", 1, true, {1, 1, 1}},
            BigNumIntegerInput{
                "NegativeFraction", 0xfffffffe, true, {254, 65534, static_cast<long>(std::uint32_t{0xfffffffe})}})));

namespace
{

enum class BigNumScalarCall
{
    PARSE,
    INTEGER,
    FLOAT
};

struct BigNumScalarInput
{
    const char *text;
    long magnitude;
    bool half;
    bool negative;
    const char *fractional_text;
    const char *integer_text;
};

void PrintTo(const BigNumScalarInput &input, std::ostream *out)
{
    *out << input.text;
}

std::vector<Byte> scalar_bytes(const long magnitude, const bool half, const bool negative)
{
    std::vector<Byte> bytes(g_bn_length);
    for (int byte = 0; byte < g_int_length; ++byte)
    {
        bytes[g_bn_length - g_int_length + byte] =
            static_cast<Byte>(static_cast<std::uint64_t>(magnitude) >> (8 * byte));
    }
    if (half && g_bn_length > g_int_length)
    {
        bytes[g_bn_length - g_int_length - 1] = 0x80;
    }
    if (negative)
    {
        unsigned carry = 1;
        for (Byte &byte : bytes)
        {
            const unsigned sum = (byte ^ 0xffU) + carry;
            byte = static_cast<Byte>(sum);
            carry = sum >> 8;
        }
    }
    return bytes;
}

} // namespace

class TestBigNumByteScalarInterfaces
    : public TestBigNumByteKernel,
      public testing::WithParamInterface<std::tuple<int, int, BigNumScalarInput, BigNumScalarCall>>
{
};

TEST_P(TestBigNumByteScalarInterfaces, preservesExactEncodingReturnAddressAndGuards)
{
    const int integer_length = std::get<1>(GetParam());
    init_layout(std::get<0>(GetParam()), integer_length);
    ASSERT_EQ(integer_length, g_int_length);
    const BigNumScalarInput &input = std::get<2>(GetParam());
    const BigNumScalarCall call = std::get<3>(GetParam());
    const bool half = call != BigNumScalarCall::INTEGER && input.half;
    const bool has_fraction = half && g_bn_length > g_int_length;
    const LDouble native = (input.negative ? -1 : 1) * (input.magnitude + (input.half ? 0.5L : 0.0L));
    std::vector<Byte> expected = scalar_bytes(input.magnitude, half, input.negative);
    expected.push_back(0xa5);
    std::vector<Byte> value(g_bn_length + 1, 0xa5);

    if (call == BigNumScalarCall::PARSE)
    {
        EXPECT_EQ(value.data(), str_to_bn(value.data(), input.text));
    }
    else if (call == BigNumScalarCall::INTEGER)
    {
        EXPECT_EQ(value.data(), int_to_bn(value.data(), input.negative ? -input.magnitude : input.magnitude));
    }
    else
    {
        EXPECT_EQ(value.data(), float_to_bn(value.data(), native));
    }
    EXPECT_EQ(expected, value);

    EXPECT_EQ(has_fraction ? input.fractional_text : input.integer_text, unsafe_bn_to_string(value.data(), 8));
    EXPECT_EQ(0xa5, value.back());
}

INSTANTIATE_TEST_SUITE_P(LayoutsInputsAndCalls, TestBigNumByteScalarInterfaces,
    testing::Combine(testing::Values(4, 12, 60, 64, 672), testing::Values(1, 2, 4),
        testing::Values(BigNumScalarInput{"0", 0, false, false, "0.", "0."},
            BigNumScalarInput{"42", 42, false, false, "42.", "42."},
            BigNumScalarInput{"12.5", 12, true, false, "12.5", "12."},
            BigNumScalarInput{"-12.5", 12, true, true, "-12.5", "-12."},
            BigNumScalarInput{"-0.5", 0, true, true, "-0.5", "0."},
            BigNumScalarInput{"+12.5", 12, true, false, "12.5", "12."},
            BigNumScalarInput{"12.500", 12, true, false, "12.5", "12."},
            BigNumScalarInput{"12.5x", 0, false, false, "0.", "0."},
            BigNumScalarInput{"12e3", 12, false, false, "12.", "12."}),
        testing::Values(BigNumScalarCall::PARSE, BigNumScalarCall::INTEGER, BigNumScalarCall::FLOAT)));

struct BigNumByteMachineInput
{
    Byte fill;
    Byte low_byte;
    Byte fraction_byte;
    Byte integer_byte;
    LDouble whole;
    int low_units;
};

class TestBigNumByteMachineObservation : public TestBigNumByteKernel,
                                         public testing::WithParamInterface<std::tuple<int, BigNumByteMachineInput>>
{
};

TEST_P(TestBigNumByteMachineObservation, observesSignedBoundariesAndLowBitsAndRestoresTheSource)
{
    const auto [length, input] = GetParam();
    init_layout(length, 1);
    const int fraction_bits = 8 * (g_bn_length - g_int_length);
    std::vector<Byte> bytes(g_bn_length + 2, input.fill);
    bytes[0] = 0x5a;
    bytes[1] = input.low_byte;
    bytes[g_bn_length - 1] = input.fraction_byte;
    bytes[g_bn_length] = input.integer_byte;
    bytes.back() = 0xa5;
    const std::vector<Byte> original = bytes;
    const LDouble low = std::ldexp(static_cast<LDouble>(input.low_units), -fraction_bits);
    const LDouble expected = input.whole == 0 ? low : input.whole + low;

    const LDouble result = bn_to_float(bytes.data() + 1);

    EXPECT_EQ(expected, result);
    EXPECT_EQ(std::signbit(expected), std::signbit(result));
    EXPECT_EQ(original, bytes);
}

INSTANTIATE_TEST_SUITE_P(PrecisionsAndInputs, TestBigNumByteMachineObservation,
    testing::Combine(testing::Values(4, 8, 12, 672),
        testing::Values(BigNumByteMachineInput{0, 0, 0, 0, 0, 0}, BigNumByteMachineInput{0, 0, 0x40, 1, 1.25L, 0},
            BigNumByteMachineInput{0, 0, 0xc0, 0xfe, -1.25L, 0}, BigNumByteMachineInput{0, 1, 0, 0, 0, 1},
            BigNumByteMachineInput{0xff, 0xff, 0xff, 0xff, 0, -1}, BigNumByteMachineInput{0, 0, 0, 0x80, -128, 0},
            BigNumByteMachineInput{0xff, 0xff, 0xff, 0x7f, 128, -1})));

class TestBigNumByteHexDump : public TestBigNumByteKernel, public testing::WithParamInterface<int>
{
protected:
    void SetUp() override;
    static void expect_hex_dump(const std::vector<Byte> &bytes, const std::string &expected);
};

void TestBigNumByteHexDump::SetUp()
{
    init_layout(GetParam(), 1);
    ASSERT_EQ(GetParam(), g_bn_length);
    ASSERT_EQ(1, g_int_length);
}

void TestBigNumByteHexDump::expect_hex_dump(const std::vector<Byte> &bytes, const std::string &expected)
{
    const std::vector<Byte> original = bytes;
    testing::internal::CaptureStdout();

    bn_hex_dump(bytes.data());

    const std::string output = testing::internal::GetCapturedStdout();
    EXPECT_EQ(expected + '\n', output);
    EXPECT_EQ(original, bytes);
}

TEST_P(TestBigNumByteHexDump, hexDumpPrintsZeroAndPreservesInput)
{
    const int length = GetParam();
    const std::vector<Byte> bytes(static_cast<std::size_t>(length));
    std::string expected;
    for (int i = 0; i < length; ++i)
    {
        expected += "00 ";
    }

    expect_hex_dump(bytes, expected);
}

TEST_P(TestBigNumByteHexDump, hexDumpPrintsNegativeValueAndPreservesInput)
{
    const int length = GetParam();
    std::vector<Byte> bytes(static_cast<std::size_t>(length));
    bytes[bytes.size() - 2] = 0xc0;
    bytes.back() = 0xfe; // -1.25 in the active fixed-point layout.
    std::string expected;
    for (int i = 0; i < length - 2; ++i)
    {
        expected += "00 ";
    }
    expected += "C0 FE ";

    expect_hex_dump(bytes, expected);
}

TEST_P(TestBigNumByteHexDump, hexDumpPrintsHighBitBytesInStorageOrder)
{
    const std::array<Byte, 4> pattern{0x00, 0x09, 0x8a, 0xff};
    const int length = GetParam();
    std::vector<Byte> bytes(static_cast<std::size_t>(length));
    std::string expected;
    for (int i = 0; i < length; i += 4)
    {
        std::copy(pattern.begin(), pattern.end(), bytes.begin() + i);
        expected += "00 09 8A FF ";
    }

    expect_hex_dump(bytes, expected);
}

INSTANTIATE_TEST_SUITE_P(Precisions, TestBigNumByteHexDump, testing::Values(4, 672));

class TestBigNumByteLayout : public TestBigNumByteKernel, public testing::WithParamInterface<int>
{
};

TEST_P(TestBigNumByteLayout, integerScalarMaximumRetainsLowIntegerBytes)
{
    init_layout(32, GetParam());
    ASSERT_EQ(GetParam(), g_int_length);
    const long native = std::numeric_limits<long>::max();
    std::vector<Byte> expected(g_bn_length);
    for (int byte = 0; byte < g_int_length; ++byte)
    {
        expected[g_bn_length - g_int_length + byte] =
            static_cast<Byte>(static_cast<std::uint64_t>(native) >> (8 * byte));
    }
    std::vector<Byte> value(g_bn_length + 1, 0xa5);
    expected.push_back(0xa5);

    EXPECT_EQ(value.data(), int_to_bn(value.data(), native));

    EXPECT_EQ(expected, value);
}

TEST_P(TestBigNumByteLayout, integerScalarMinimumRetainsLowIntegerBytes)
{
    init_layout(32, GetParam());
    ASSERT_EQ(GetParam(), g_int_length);
    const long native = std::numeric_limits<long>::min();
    std::vector<Byte> expected(g_bn_length);
    for (int byte = 0; byte < g_int_length; ++byte)
    {
        expected[g_bn_length - g_int_length + byte] =
            static_cast<Byte>(static_cast<std::uint64_t>(native) >> (8 * byte));
    }
    std::vector<Byte> value(g_bn_length + 1, 0xa5);
    expected.push_back(0xa5);

    EXPECT_EQ(value.data(), int_to_bn(value.data(), native));

    EXPECT_EQ(expected, value);
}

TEST_P(TestBigNumByteLayout, nativeAndDecimalEncodingUseTheActiveLayout)
{
    init_layout(32, GetParam());
    ASSERT_EQ(GetParam(), g_int_length);
    const std::size_t ones_byte = static_cast<std::size_t>(g_bn_length - g_int_length);
    std::vector<Byte> expected(g_bn_length + 1);
    expected[ones_byte] = 1;
    expected[ones_byte - 1] = 0x40;
    expected.back() = 0xa5;
    std::vector<Byte> native(g_bn_length + 1, 0xa5);
    std::vector<Byte> decimal(g_bn_length + 1, 0xa5);

    EXPECT_EQ(native.data(), float_to_bn(native.data(), 1.25L));
    EXPECT_EQ(decimal.data(), str_to_bn(decimal.data(), "1.25"));
    EXPECT_EQ(expected, native);
    EXPECT_EQ(expected, decimal);
}

TEST_P(TestBigNumByteLayout, roundTripPreservesEncodedValuesAndInputs)
{
    init_layout(32, GetParam());
    ASSERT_EQ(GetParam(), g_int_length);
    const int fractional_bytes = g_bn_length - g_int_length;
    std::vector<Byte> fixed = fixed_sample(g_bn_length, fractional_bytes, -2, 0xc3);
    for (int i = 0; i < fractional_bytes - 1; ++i)
    {
        fixed[i] = static_cast<Byte>(0x11 + i * 0x6f);
    }
    const std::vector<Byte> original_fixed = fixed;
    std::vector<Byte> floating(g_bf_length + 2);
    std::vector<Byte> restored(g_bn_length);
    std::vector<Byte> expected_float(g_bf_length + 2);
    std::copy_n(fixed.begin(), fractional_bytes + 1, expected_float.begin() + g_bf_length - fractional_bytes - 2);
    expected_float[g_bf_length - 1] = 0xff;

    EXPECT_EQ(floating.data(), bn_to_bf(floating.data(), fixed.data()));
    EXPECT_EQ(expected_float, floating);
    EXPECT_EQ(restored.data(), bf_to_bn(restored.data(), floating.data()));

    EXPECT_EQ(original_fixed, fixed);
    EXPECT_EQ(expected_float, floating);
    EXPECT_EQ(original_fixed, restored);
}

TEST_P(TestBigNumByteLayout, arithmeticSamplesPreserveDistinctOperands)
{
    init_layout(32, GetParam());
    ASSERT_EQ(GetParam(), g_int_length);
    const int fractional_bytes = g_bn_length - g_int_length;
    std::vector<Byte> left = fixed_sample(g_bn_length, fractional_bytes, -8, 0x80);
    std::vector<Byte> right = fixed_sample(g_bn_length, fractional_bytes, 2, 0x80);
    const std::vector<Byte> original_left = left;
    const std::vector<Byte> original_right = right;
    std::vector<Byte> result(g_r_length);

    EXPECT_EQ(result.data(), add_bn(result.data(), left.data(), right.data()));
    EXPECT_EQ(fixed_sample(g_bn_length, fractional_bytes, -5),
        std::vector<Byte>(result.begin(), result.begin() + g_bn_length));
    EXPECT_EQ(result.data(), sub_bn(result.data(), left.data(), right.data()));
    EXPECT_EQ(fixed_sample(g_bn_length, fractional_bytes, -10),
        std::vector<Byte>(result.begin(), result.begin() + g_bn_length));
    EXPECT_EQ(result.data(), mult_bn(result.data(), left.data(), right.data()));
    EXPECT_EQ(fixed_sample(g_r_length, g_r_length - 2 * g_int_length, -19, 0x40), result);
    EXPECT_EQ(original_left, left);
    EXPECT_EQ(original_right, right);
}

INSTANTIATE_TEST_SUITE_P(IntegerBytes, TestBigNumByteLayout, testing::Values(1, 2, 4));

class TestBigNumByteConversionLimits : public TestBigNumByteKernel,
                                       public testing::WithParamInterface<std::tuple<int, bool, bool>>
{
};

TEST_P(TestBigNumByteConversionLimits, preservesFloatConversionOverflowAndUnderflowRules)
{
    const int integer_bytes = std::get<0>(GetParam());
    init_layout(32, integer_bytes);
    ASSERT_EQ(integer_bytes, g_int_length);
    const bool overflow = std::get<1>(GetParam());
    const bool negative = std::get<2>(GetParam());
    std::vector<Byte> source(g_bf_length + 2);
    source[g_bf_length - 2] = negative ? 0xfe : 2;
    source[g_bf_length - 1] = negative ? 0xff : 0;
    // Base-256 exponents of 166 and -167 exceed the fixed-point range.
    source[g_bf_length] = overflow ? 166 : 0x59;
    source[g_bf_length + 1] = overflow ? 0 : 0xff;
    const std::vector<Byte> original = source;
    std::vector<Byte> result(g_bn_length);
    std::vector<Byte> expected(g_bn_length, overflow && !negative ? Byte{0xff} : Byte{0});
    if (overflow)
    {
        expected.front() = negative ? 1 : 0xff;
        expected.back() = negative ? 0x80 : 0x7f;
    }

    EXPECT_EQ(result.data(), bf_to_bn(result.data(), source.data()));

    EXPECT_EQ(expected, result);
    EXPECT_EQ(original, source);
}

INSTANTIATE_TEST_SUITE_P(LayoutsAndLimits, TestBigNumByteConversionLimits,
    testing::Combine(testing::Values(1, 2, 4), testing::Bool(), testing::Bool()));

class TestBigNumByteProducts : public TestBigNumByteKernel, public testing::WithParamInterface<int>
{
};

TEST_P(TestBigNumByteProducts, legacyProductsUseTheirDocumentedOffsets)
{
    init_layout(GetParam(), 1);
    const int fractional_bytes = g_bn_length - g_int_length;
    std::vector<Byte> value = fixed_sample(g_bn_length, fractional_bytes, -8, 0x80);
    std::vector<Byte> other = value;
    const std::vector<Byte> original = value;
    std::vector<Byte> partial(g_r_length);
    std::vector<Byte> full(2 * g_bn_length);
    const std::vector<Byte> expected_partial = fixed_sample(g_r_length, g_r_length - 2 * g_int_length, 56, 0x40);
    const std::vector<Byte> expected_full = fixed_sample(2 * g_bn_length, 2 * fractional_bytes, 56, 0x40);

    EXPECT_EQ(partial.data(), mult_bn(partial.data(), value.data(), other.data()));
    EXPECT_EQ(expected_partial, partial);
    EXPECT_EQ(partial.data(), square_bn(partial.data(), value.data()));
    EXPECT_EQ(expected_partial, partial);
    EXPECT_EQ(full.data(), full_mult_bn(full.data(), value.data(), other.data()));
    EXPECT_EQ(expected_full, full);
    EXPECT_EQ(full.data(), full_square_bn(full.data(), value.data()));
    EXPECT_EQ(expected_full, full);
    EXPECT_EQ(original, value);
    EXPECT_EQ(original, other);
}

TEST_P(TestBigNumByteProducts, legacyProductTruncationKeepsTheSignedLowBit)
{
    init_layout(GetParam(), 1);
    std::vector<Byte> quantum(g_bn_length);
    quantum.front() = 1;
    std::vector<Byte> half = fixed_sample(g_bn_length, g_bn_length - g_int_length, 0, 0x80);
    std::vector<Byte> product(g_r_length);
    const std::vector<Byte> zero(g_bn_length);

    mult_bn(product.data(), quantum.data(), half.data());
    EXPECT_EQ(zero, std::vector<Byte>(product.data() + g_shift_factor, product.data() + g_shift_factor + g_bn_length));

    std::fill(quantum.begin(), quantum.end(), Byte{0xff});
    const std::vector<Byte> negative_quantum = quantum;
    mult_bn(product.data(), quantum.data(), half.data());
    EXPECT_EQ(negative_quantum,
        std::vector<Byte>(product.data() + g_shift_factor, product.data() + g_shift_factor + g_bn_length));
    EXPECT_EQ(negative_quantum, quantum);
}

INSTANTIATE_TEST_SUITE_P(Precisions, TestBigNumByteProducts, testing::Values(4, 8, 12, 672));

TEST(TestBigNumByteProcedures, saturationAndNegationWriteOnlyTheCanonicalBytes)
{
    const ValueSaver<int> length{g_bn_length, 4};
    alignas(U32) std::array<Byte, 8> value{0, 0, 0, 0, 0xa5, 0xa5, 0xa5, 0xa5};
    constexpr std::array<Byte, 8> maximum{0xff, 0xff, 0xff, 0x7f, 0xa5, 0xa5, 0xa5, 0xa5};
    constexpr std::array<Byte, 8> negative_maximum{1, 0, 0, 0x80, 0xa5, 0xa5, 0xa5, 0xa5};

    EXPECT_EQ(value.data(), max_bn(value.data()));
    EXPECT_EQ(maximum, value);
    EXPECT_EQ(value.data(), neg_a_bn(value.data()));
    EXPECT_EQ(negative_maximum, value);
}

enum class KernelProduct
{
    SAFE_DISTINCT,
    SAFE_SAME,
    UNSAFE_DISTINCT,
    UNSAFE_SAME,
    FULL_SAME,
    SAFE_SQR,
    UNSAFE_SQR
};

class TestBigNumKernelProducts : public TestBigNumByteKernel,
                                 public testing::WithParamInterface<std::tuple<int, KernelProduct>>
{
protected:
    void SetUp() override;
};

void TestBigNumKernelProducts::SetUp()
{
    init_layout(std::get<0>(GetParam()), 1);
}

TEST_P(TestBigNumKernelProducts, retainsFullBytesReturnAliasAndLegacyOperandMutation)
{
    const KernelProduct kind = std::get<1>(GetParam());
    const int fractional_bytes = g_bn_length - g_int_length;
    std::vector<Byte> left = fixed_sample(g_bn_length, fractional_bytes, -2);
    std::vector<Byte> right = fixed_sample(g_bn_length, fractional_bytes, 3);
    const bool squared = kind != KernelProduct::SAFE_DISTINCT && kind != KernelProduct::UNSAFE_DISTINCT;
    const int extent = kind == KernelProduct::FULL_SAME ? 2 * g_bn_length : g_r_length;
    std::vector<Byte> guarded(extent + 2, 0xa5);
    Byte *const result = guarded.data() + 1;
    std::vector<Byte> expected(extent, squared ? 0 : 0xff);
    // The retained product has two integer bytes in this one-byte layout.
    expected[extent - 2] = squared ? 4 : 0xfa;
    if (!squared)
    {
        std::fill(expected.begin(), expected.end() - 2, Byte{0});
    }
    const std::array<int, 9> lengths{g_bn_step, g_bn_length, g_int_length, g_r_length, g_padding, g_shift_factor,
        g_bf_length, g_r_bf_length, g_bf_decimals};

    switch (kind)
    {
    case KernelProduct::SAFE_DISTINCT:
        EXPECT_EQ(result, mult_bn(result, left.data(), right.data()));
        break;
    case KernelProduct::SAFE_SAME:
        EXPECT_EQ(result, mult_bn(result, left.data(), left.data()));
        break;
    case KernelProduct::UNSAFE_DISTINCT:
        EXPECT_EQ(result, unsafe_mult_bn(result, left.data(), right.data()));
        break;
    case KernelProduct::UNSAFE_SAME:
        EXPECT_EQ(result, unsafe_mult_bn(result, left.data(), left.data()));
        break;
    case KernelProduct::FULL_SAME:
        EXPECT_EQ(result, full_mult_bn(result, left.data(), left.data()));
        break;
    case KernelProduct::SAFE_SQR:
        EXPECT_EQ(result, square_bn(result, left.data()));
        break;
    case KernelProduct::UNSAFE_SQR:
        EXPECT_EQ(result, unsafe_square_bn(result, left.data()));
        break;
    }

    EXPECT_EQ(expected, std::vector<Byte>(result, result + extent));
    EXPECT_EQ(0xa5, guarded.front());
    EXPECT_EQ(0xa5, guarded.back());
    const bool restored = kind == KernelProduct::SAFE_DISTINCT || kind == KernelProduct::SAFE_SQR;
    // The safe binary adapters restore a shared negative input twice.
    // Preserve this pre-existing behavior separately from value operators.
    EXPECT_EQ(fixed_sample(g_bn_length, fractional_bytes, restored ? -2 : 2), left);
    EXPECT_EQ(fixed_sample(g_bn_length, fractional_bytes, 3), right);
    EXPECT_EQ(lengths,
        (std::array<int, 9>{g_bn_step, g_bn_length, g_int_length, g_r_length, g_padding, g_shift_factor, g_bf_length,
            g_r_bf_length, g_bf_decimals}));
}

INSTANTIATE_TEST_SUITE_P(PrecisionsAndAliasing, TestBigNumKernelProducts,
    testing::Combine(testing::Values(4, 8, 12, 672),
        testing::Values(KernelProduct::SAFE_DISTINCT, KernelProduct::SAFE_SAME, KernelProduct::UNSAFE_DISTINCT,
            KernelProduct::UNSAFE_SAME, KernelProduct::FULL_SAME, KernelProduct::SAFE_SQR, KernelProduct::UNSAFE_SQR)));

struct BigNumByteRetainedProductInput
{
    int left;
    int right;
    bool shared{};
    bool sqr{};
};

class TestBigNumByteRetainedProduct
    : public TestBigNumByteKernel,
      public testing::WithParamInterface<std::tuple<int, int, BigNumByteRetainedProductInput>>
{
};

TEST_P(TestBigNumByteRetainedProduct, preservesExactProductBytesOperandEffectsAndPadding)
{
    init_layout(std::get<0>(GetParam()), std::get<1>(GetParam()));
    const BigNumByteRetainedProductInput &input = std::get<2>(GetParam());
    const int fraction = g_bn_length - g_int_length;
    const int product_fraction = g_r_length - 2 * g_int_length;
    // The square cases use +/-2.25 where the source has fractional bytes.
    const bool fractional_sqr = input.sqr && fraction != 0;
    const int left_whole = fractional_sqr && input.left < 0 ? -3 : input.left;
    const Byte left_fraction = fractional_sqr ? (input.left < 0 ? 0xc0 : 0x40) : 0;
    std::vector<Byte> left = fixed_sample(g_bn_length, fraction, left_whole, left_fraction);
    std::vector<Byte> right = fixed_sample(g_bn_length, fraction, input.right);
    const std::vector<Byte> original_right = right;
    const std::vector<Byte> expected_left = input.shared ? fixed_sample(g_bn_length, fraction, 2) : left;
    const int whole = fractional_sqr ? 5 : input.left * (input.shared || input.sqr ? input.left : input.right);
    const std::vector<Byte> expected = fixed_sample(g_r_length, product_fraction, whole, fractional_sqr ? 0x10 : 0);
    constexpr int GUARD_SIZE = 4;
    const std::size_t stride = bn_arena(BigLifetime::WORKING).block_size();
    std::vector<Byte> guarded(2 * stride + 2 * GUARD_SIZE, 0xa5);
    std::vector<Byte> expected_output = guarded;
    std::copy(expected.begin(), expected.end(), expected_output.begin() + GUARD_SIZE);
    Byte *const result = guarded.data() + GUARD_SIZE;

    Byte *const returned = input.sqr ? square_bn(result, left.data())
                                     : mult_bn(result, left.data(), input.shared ? left.data() : right.data());

    EXPECT_EQ(result, returned);
    EXPECT_EQ(expected_output, guarded);
    EXPECT_EQ(expected_left, left);
    EXPECT_EQ(original_right, right);
}

INSTANTIATE_TEST_SUITE_P(LayoutsSignsAndAliasing, TestBigNumByteRetainedProduct,
    testing::Combine(testing::Values(4, 8, 12, 56, 60, 64, 672), testing::Values(1, 2, 4),
        testing::Values(BigNumByteRetainedProductInput{2, 3}, BigNumByteRetainedProductInput{2, -3},
            BigNumByteRetainedProductInput{-2, 3}, BigNumByteRetainedProductInput{-2, -3},
            BigNumByteRetainedProductInput{2, 3, true}, BigNumByteRetainedProductInput{-2, 3, true},
            BigNumByteRetainedProductInput{2, 0, false, true}, BigNumByteRetainedProductInput{-2, 0, false, true})));

TEST_P(TestBigNumByteProducts, reductionCopiesOnlyTheCanonicalWindow)
{
    init_layout(GetParam(), 1);
    std::vector<Byte> guarded(g_r_length + 8, 0xa5);
    for (int i = 0; i < g_r_length; ++i)
    {
        guarded[4 + i] = static_cast<Byte>(37 * i + 19);
    }
    std::vector<Byte> expected = guarded;
    std::copy_n(guarded.begin() + 4 + g_shift_factor, g_bn_length, expected.begin() + 4);

    reduce_bn_product(guarded.data() + 4);

    EXPECT_EQ(expected, guarded);
}

class TestBigNumByteProductRelocation
    : public TestBigNumByteKernel,
      public testing::WithParamInterface<std::tuple<int, std::pair<int, int>, bool, BigLifetime>>
{
};

TEST_P(TestBigNumByteProductRelocation, relocatesRetainedBytesRatherThanTreatingTheirBaseAsCanonical)
{
    const int old_length = std::get<0>(GetParam());
    const std::pair<int, int> integers = std::get<1>(GetParam());
    const bool negative = std::get<2>(GetParam());
    init_layout(old_length, integers.first);
    BigArena &arena = bn_arena(std::get<3>(GetParam()));
    EXPECT_EQ(static_cast<std::size_t>((old_length + 63) / 64 * 64), arena.block_size());
    const std::size_t capacity = arena.capacity();
    BigArena::Allocation product = arena.allocate_double();
    const int old_fraction = g_r_length - 2 * g_int_length;
    const int guard = g_shift_factor - 1;
    std::fill_n(product.data(), 2 * arena.block_size(), Byte{0});
    std::fill(product.data() + old_fraction, product.data() + g_r_length, negative ? Byte{0xff} : Byte{0});
    product.data()[old_fraction] = negative ? 0xee : 18;
    if (guard >= 0)
    {
        product.data()[guard] = 0x5a;
    }
    const Byte *const address = product.data();
    const BigArena::Index index = product.index();

    init_layout(old_length <= 12 ? 64 : 12, integers.second);

    ASSERT_EQ(integers.second, g_int_length);
    EXPECT_EQ(static_cast<std::size_t>((g_bn_length + 63) / 64 * 64), arena.block_size());
    EXPECT_EQ(capacity, arena.capacity());
    const int fraction = g_r_length - 2 * g_int_length;
    std::vector<Byte> expected(g_r_length);
    std::fill(expected.begin() + fraction, expected.end(), negative ? Byte{0xff} : Byte{0});
    expected[fraction] = negative ? 0xee : 18;
    const int relocated_guard = guard + fraction - old_fraction;
    if (guard >= 0 && relocated_guard >= 0)
    {
        expected[relocated_guard] = 0x5a;
    }
    EXPECT_NE(address, product.data());
    EXPECT_EQ(&arena, &product.arena());
    EXPECT_EQ(index, product.index());
    EXPECT_EQ(2U, product.blocks());
    EXPECT_EQ(expected, std::vector<Byte>(product.data(), product.data() + g_r_length));
    EXPECT_EQ(std::vector<Byte>(2 * arena.block_size() - g_r_length),
        std::vector<Byte>(product.data() + g_r_length, product.data() + 2 * arena.block_size()));
    const Byte *const relocated = product.data();

    product.reduce();

    EXPECT_EQ(relocated, product.data());
    EXPECT_EQ(1U, product.blocks());
    EXPECT_EQ(std::vector<Byte>(expected.begin() + g_shift_factor, expected.begin() + g_shift_factor + g_bn_length),
        std::vector<Byte>(product.data(), product.data() + g_bn_length));
    BigArena::Allocation companion = arena.allocate_single();
    EXPECT_EQ(relocated + arena.block_size(), companion.data());
}

INSTANTIATE_TEST_SUITE_P(LayoutsSignsAndLifetimes, TestBigNumByteProductRelocation,
    testing::Combine(testing::Values(4, 12, 64, 672),
        testing::Values(
            std::pair<int, int>{1, 1}, std::pair<int, int>{1, 2}, std::pair<int, int>{2, 4}, std::pair<int, int>{4, 1}),
        testing::Bool(), testing::Values(BigLifetime::PERSISTENT, BigLifetime::WORKING, BigLifetime::TEMPORARY)));

class TestBigNumByteProductWindows : public TestBigNumByteKernel,
                                     public testing::WithParamInterface<std::tuple<int, int>>
{
};

TEST_P(TestBigNumByteProductWindows, canonicalSlotAndPairedProductHaveSeparateExtents)
{
    init_layout(std::get<0>(GetParam()), std::get<1>(GetParam()));
    BigArena &arena = bn_arena(BigLifetime::WORKING);
    const std::size_t stride = (g_bn_length + 63) / 64 * 64;
    EXPECT_EQ(stride, arena.block_size());
    BigArena::Allocation canonical = arena.allocate_single();
    BigArena::Allocation product = arena.allocate_double();
    ASSERT_EQ(1U, canonical.blocks());
    ASSERT_EQ(2U, product.blocks());
    ASSERT_LE(static_cast<std::size_t>(g_r_length), 2 * arena.block_size());
    std::fill_n(canonical.data(), arena.block_size(), Byte{0x5a});
    std::fill_n(product.data(), 2 * arena.block_size(), Byte{0xa5});
    std::vector<Byte> input = fixed_sample(g_bn_length, g_bn_length - g_int_length, -3);
    const std::vector<Byte> original = input;

    square_bn(product.data(), input.data());

    const std::vector<Byte> expected = fixed_sample(g_r_length, g_r_length - 2 * g_int_length, 9);
    EXPECT_EQ(expected, std::vector<Byte>(product.data(), product.data() + g_r_length));
    EXPECT_EQ(std::vector<Byte>(2 * arena.block_size() - g_r_length, 0xa5),
        std::vector<Byte>(product.data() + g_r_length, product.data() + 2 * arena.block_size()));
    EXPECT_EQ(std::vector<Byte>(arena.block_size(), 0x5a),
        std::vector<Byte>(canonical.data(), canonical.data() + arena.block_size()));
    EXPECT_EQ(1U, canonical.blocks());
    EXPECT_EQ(2U, product.blocks());
    EXPECT_EQ(original, input);
}

TEST_P(TestBigNumByteProductWindows, shiftedSubtractionAndDoublingPreserveTheOtherProductBytes)
{
    init_layout(std::get<0>(GetParam()), std::get<1>(GetParam()));
    BigStackSaver saved;
    BigArena::Allocation product = bn_arena(BigLifetime::TEMPORARY).allocate_double();
    const int fraction = g_bn_length - g_int_length;
    const int wide_fraction = g_r_length - 2 * g_int_length;
    std::vector<Byte> x = fixed_sample(g_bn_length, fraction, 3);
    std::vector<Byte> y = fixed_sample(g_bn_length, fraction, -2);
    const std::vector<Byte> original_x = x;
    const std::vector<Byte> original_y = y;
    std::vector<Byte> cache_x(g_r_length), cache_y(g_r_length);
    square_bn(cache_x.data(), x.data());
    square_bn(cache_y.data(), y.data());
    std::vector<Byte> expected_x = fixed_sample(g_r_length, wide_fraction, 9);
    std::vector<Byte> expected_product = fixed_sample(g_r_length, wide_fraction, -6);
    const std::vector<Byte> difference = fixed_sample(g_bn_length, fraction, 5);
    const std::vector<Byte> doubled = fixed_sample(g_bn_length, fraction, -12);
    std::copy(difference.begin(), difference.end(), expected_x.begin() + g_shift_factor);
    std::copy(doubled.begin(), doubled.end(), expected_product.begin() + g_shift_factor);

    sub_a_bn(cache_x.data() + g_shift_factor, cache_y.data() + g_shift_factor);
    mult_bn(product.data(), x.data(), y.data());
    double_a_bn(product.data() + g_shift_factor);

    EXPECT_EQ(expected_x, cache_x);
    EXPECT_EQ(fixed_sample(g_r_length, wide_fraction, 4), cache_y);
    EXPECT_EQ(expected_product, std::vector<Byte>(product.data(), product.data() + g_r_length));
    EXPECT_EQ(5, bn_to_int(cache_x.data() + g_shift_factor));
    const std::int64_t modulus = std::int64_t{1} << (8 * g_int_length);
    const std::int64_t extracted =
        g_int_length == 4 ? static_cast<long>(static_cast<std::uint32_t>(-12)) : modulus - 12;
    EXPECT_EQ(extracted, bn_to_int(product.data() + g_shift_factor));
    EXPECT_EQ(original_x, x);
    EXPECT_EQ(original_y, y);
}

INSTANTIATE_TEST_SUITE_P(Layouts, TestBigNumByteProductWindows,
    testing::Combine(testing::Values(4, 8, 12, 56, 60, 64, 68, 672), testing::Values(1, 2, 4)));

struct BigNumConversionInput
{
    const char *name;
    int length;
    int integer_length;
    std::vector<Byte> expected;
};

class TestBigNumByteConversion : public TestBigNumConversionBase,
                                 public testing::WithParamInterface<BigNumConversionInput>
{
};

TEST_P(TestBigNumByteConversion, conversionPreservesFixedPointBytePositionsAndTruncation)
{
    const int active_length = g_bn_length;
    const int active_integer_length = g_int_length;
    const std::vector<Byte> source{0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0xfe};
    const std::vector<Byte> original = source;
    const BigNumConversionInput &input = GetParam();
    std::vector<Byte> destination(input.expected.size() + 2, 0xcc);

    EXPECT_EQ(0, convert_bn(destination.data() + 1, source.data(), input.length, input.integer_length, 8, 2));

    EXPECT_EQ(input.expected, (std::vector<Byte>{destination.begin() + 1, destination.end() - 1}));
    EXPECT_EQ(0xcc, destination.front());
    EXPECT_EQ(0xcc, destination.back());
    EXPECT_EQ(original, source);
    EXPECT_EQ(active_length, g_bn_length);
    EXPECT_EQ(active_integer_length, g_int_length);
}

INSTANTIATE_TEST_SUITE_P(Layouts, TestBigNumByteConversion,
    testing::Values(
        BigNumConversionInput{"WidenFraction", 12, 2, {0, 0, 0, 0, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0xfe}},
        BigNumConversionInput{"NarrowFraction", 4, 2, {0x55, 0x66, 0x77, 0xfe}},
        BigNumConversionInput{"SameLayout", 8, 2, {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0xfe}},
        BigNumConversionInput{"WidenInteger", 12, 4, {0, 0, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0xfe, 0, 0}},
        BigNumConversionInput{"NarrowInteger", 8, 1, {0, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77}}),
    [](const testing::TestParamInfo<BigNumConversionInput> &info) { return info.param.name; });

class TestBigNumSignedConversion : public TestBigNumConversionBase, public testing::WithParamInterface<bool>
{
};

TEST_P(TestBigNumSignedConversion, integerWideningPreservesTheSignWithoutChangingTheSourceOrGuards)
{
    const bool negative = GetParam();
    const Byte high_byte = negative ? 0xfe : 0x7e;
    const Byte extension = negative ? 0xff : 0;
    const std::vector<Byte> source{0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, high_byte};
    const std::vector<Byte> original = source;
    const std::vector<Byte> expected{0, 0, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, high_byte, extension, extension};
    std::vector<Byte> destination(14, 0xcc);

    convert_bn_signed(destination.data() + 1, source.data(), 12, 4, 8, 2);

    EXPECT_EQ(expected, (std::vector<Byte>{destination.begin() + 1, destination.end() - 1}));
    EXPECT_EQ(0xcc, destination.front());
    EXPECT_EQ(0xcc, destination.back());
    EXPECT_EQ(original, source);
}

INSTANTIATE_TEST_SUITE_P(Signs, TestBigNumSignedConversion, testing::Bool());

struct BigFloatToBigNumInput
{
    const char *name;
    int integer_length;
    std::vector<Byte> source;
    std::array<Byte, 4> expected;
};

class TestBigFloatToBigNumBytes : public TestBigNumConversionBase,
                                  public testing::WithParamInterface<BigFloatToBigNumInput>
{
};

TEST_P(TestBigFloatToBigNumBytes, preservesScalingSaturationSourceReturnIdentityAndDestinationBounds)
{
    const BigFloatToBigNumInput &input = GetParam();
    ValueSaver<int> bn_length{g_bn_length, 4};
    ValueSaver<int> bf_length{g_bf_length, static_cast<int>(input.source.size()) - 2};
    ValueSaver<int> integer_length{g_int_length, input.integer_length};
    const std::array<int, 7> lengths{
        g_bn_length, g_bf_length, g_int_length, g_padding, g_r_length, g_r_bf_length, g_shift_factor};
    std::vector<Byte> source{0xaa};
    source.insert(source.end(), input.source.begin(), input.source.end());
    source.push_back(0xbb);
    const std::vector<Byte> original = source;
    std::array<Byte, 8> destination;
    destination.fill(0xcc);
    std::array<Byte, 8> expected_destination = destination;
    std::copy(input.expected.begin(), input.expected.end(), expected_destination.begin() + 1);

    EXPECT_EQ(destination.data() + 1, bf_to_bn(destination.data() + 1, source.data() + 1));

    EXPECT_EQ(expected_destination, destination);
    EXPECT_EQ(original, source);
    EXPECT_EQ(lengths,
        (std::array<int, 7>{
            g_bn_length, g_bf_length, g_int_length, g_padding, g_r_length, g_r_bf_length, g_shift_factor}));
}

INSTANTIATE_TEST_SUITE_P(Layouts, TestBigFloatToBigNumBytes,
    testing::Values(BigFloatToBigNumInput{"Zero", 1, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {}},
        BigFloatToBigNumInput{
            "Positive", 1, {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0, 0, 0}, {0x44, 0x55, 0x66, 0x77}},
        BigFloatToBigNumInput{
            "Negative", 1, {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0xfe, 0xff, 0, 0}, {0x44, 0x55, 0x66, 0xfe}},
        BigFloatToBigNumInput{
            "TwoIntegerBytes", 2, {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0, 0, 0}, {0x55, 0x66, 0x77, 0}},
        BigFloatToBigNumInput{"NegativeIntegerTruncation", 4, {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0xfe, 0xff, 0, 0},
            {0xfe, 0xff, 0xff, 0xff}},
        BigFloatToBigNumInput{
            "NegativeExponent", 1, {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0, 0xff, 0xff}, {0x55, 0x66, 0x77, 0}},
        BigFloatToBigNumInput{"SmallestFraction", 1, {0, 0, 0, 0, 0, 0, 1, 0, 0xfd, 0xff}, {1, 0, 0, 0}},
        BigFloatToBigNumInput{"PositiveUnderflow", 1, {0, 0, 0, 0, 0, 0, 1, 0, 0xfc, 0xff}, {}},
        BigFloatToBigNumInput{"NegativeUnderflow", 1, {0, 0, 0, 0, 0, 0, 0xfe, 0xff, 0xfc, 0xff}, {}},
        BigFloatToBigNumInput{"PositiveOverflow", 1, {0, 0, 0, 0, 0, 0, 1, 0, 1, 0}, {0xff, 0xff, 0xff, 0x7f}},
        BigFloatToBigNumInput{"NegativeOverflow", 1, {0, 0, 0, 0, 0, 0, 0xfe, 0xff, 1, 0}, {1, 0, 0, 0x80}},
        BigFloatToBigNumInput{
            "TwoIntegerBytesPositiveOverflow", 2, {0, 0, 0, 0, 0, 0, 1, 0, 2, 0}, {0xff, 0xff, 0xff, 0x7f}},
        BigFloatToBigNumInput{
            "TwoIntegerBytesNegativeOverflow", 2, {0, 0, 0, 0, 0, 0, 0xfe, 0xff, 2, 0}, {1, 0, 0, 0x80}},
        BigFloatToBigNumInput{
            "FourIntegerBytesPositiveOverflow", 4, {0, 0, 0, 0, 0, 0, 1, 0, 4, 0}, {0xff, 0xff, 0xff, 0x7f}},
        BigFloatToBigNumInput{
            "FourIntegerBytesNegativeOverflow", 4, {0, 0, 0, 0, 0, 0, 0xfe, 0xff, 4, 0}, {1, 0, 0, 0x80}},
        BigFloatToBigNumInput{"TwoIntegerBytesPositiveUnderflow", 2, {0, 0, 0, 0, 0, 0, 1, 0, 0xfd, 0xff}, {}},
        BigFloatToBigNumInput{"TwoIntegerBytesNegativeUnderflow", 2, {0, 0, 0, 0, 0, 0, 0xfe, 0xff, 0xfd, 0xff}, {}},
        BigFloatToBigNumInput{"FourIntegerBytesPositiveUnderflow", 4, {0, 0, 0, 0, 0, 0, 1, 0, 0xff, 0xff}, {}},
        BigFloatToBigNumInput{"FourIntegerBytesNegativeUnderflow", 4, {0, 0, 0, 0, 0, 0, 0xfe, 0xff, 0xff, 0xff}, {}},
        BigFloatToBigNumInput{
            "ShortenedMantissa", 1, {0x11, 0x22, 0x33, 0x44, 0x55, 0, 0, 0}, {0x22, 0x33, 0x44, 0x55}}),
    [](const testing::TestParamInfo<BigFloatToBigNumInput> &info) { return info.param.name; });

struct BigNumToBigFloatInput
{
    const char *name;
    int integer_length;
    int float_length;
    std::array<Byte, 4> source;
    std::vector<Byte> expected;
};

class TestBigNumToBigFloatBytes : public TestBigNumConversionBase,
                                  public testing::WithParamInterface<BigNumToBigFloatInput>
{
};

TEST_P(TestBigNumToBigFloatBytes, preservesScalingExponentSourceReturnIdentityAndDestinationBounds)
{
    const BigNumToBigFloatInput &input = GetParam();
    ValueSaver<int> bn_length{g_bn_length, 4};
    ValueSaver<int> bf_length{g_bf_length, input.float_length};
    ValueSaver<int> integer_length{g_int_length, input.integer_length};
    const std::array<int, 7> lengths{
        g_bn_length, g_bf_length, g_int_length, g_padding, g_r_length, g_r_bf_length, g_shift_factor};
    std::array<Byte, 6> source{0xaa, 0, 0, 0, 0, 0xbb};
    std::copy(input.source.begin(), input.source.end(), source.begin() + 1);
    const std::array<Byte, 6> original = source;
    std::vector<Byte> destination(input.expected.size() + 4, 0xcc);
    std::vector<Byte> expected_destination = destination;
    std::copy(input.expected.begin(), input.expected.end(), expected_destination.begin() + 1);

    EXPECT_EQ(destination.data() + 1, bn_to_bf(destination.data() + 1, source.data() + 1));

    EXPECT_EQ(expected_destination, destination);
    EXPECT_EQ(original, source);
    EXPECT_EQ(lengths,
        (std::array<int, 7>{
            g_bn_length, g_bf_length, g_int_length, g_padding, g_r_length, g_r_bf_length, g_shift_factor}));
}

INSTANTIATE_TEST_SUITE_P(Layouts, TestBigNumToBigFloatBytes,
    testing::Values(BigNumToBigFloatInput{"Zero", 1, 8, {}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
        BigNumToBigFloatInput{"Positive", 1, 8, {0, 0, 0x40, 1}, {0, 0, 0, 0, 0, 0x40, 1, 0, 0, 0}},
        BigNumToBigFloatInput{"Negative", 1, 8, {0, 0, 0xc0, 0xfe}, {0, 0, 0, 0, 0, 0xc0, 0xfe, 0xff, 0, 0}},
        BigNumToBigFloatInput{"TwoIntegerBytes", 2, 8, {0, 0, 0x40, 1}, {0, 0, 0, 0, 0, 0x40, 1, 0, 1, 0}},
        BigNumToBigFloatInput{"FourIntegerBytes", 4, 8, {0, 0, 0xc0, 0xfe}, {0, 0, 0, 0, 0, 0xc0, 0xfe, 0xff, 3, 0}},
        BigNumToBigFloatInput{"Half", 1, 8, {0, 0, 0x80, 0}, {0, 0, 0, 0, 0, 0, 0x80, 0, 0xff, 0xff}},
        BigNumToBigFloatInput{"LowestFractionBit", 1, 8, {1, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 1, 0, 0xfd, 0xff}},
        BigNumToBigFloatInput{"MostNegativeInteger", 4, 8, {0, 0, 0, 0x80}, {0, 0, 0, 0, 0, 0, 0x80, 0xff, 3, 0}},
        BigNumToBigFloatInput{"ShortenedMantissa", 1, 6, {0, 0, 0x40, 1}, {0, 0, 0, 0x40, 1, 0, 0, 0}}),
    [](const testing::TestParamInfo<BigNumToBigFloatInput> &info) { return info.param.name; });

struct BigFloatConversionInput
{
    const char *name;
    std::vector<Byte> expected;
};

class TestBigFloatByteConversion : public testing::TestWithParam<BigFloatConversionInput>
{
protected:
    void SetUp() override;
    void TearDown() override;

    ValueSaver<long> m_saved_bailout_value{engine::g_user.bailout_value, 0};
    ValueSaver<engine::Bailout> m_saved_bailout_test{engine::g_bailout_test, engine::Bailout::MOD};
    ValueSaver<int> m_saved_bf_digits{g_bf_digits, 0};
    ValueSaver<fractals::FractalType> m_saved_fractal_type{fractals::g_fractal_type, fractals::FractalType::MANDEL};
};

void TestBigFloatByteConversion::SetUp()
{
    init_bf_dec(20);
}

void TestBigFloatByteConversion::TearDown()
{
    free_bf_vars();
}

TEST_P(TestBigFloatByteConversion, conversionRelocatesTheExponentAndTruncatesLowMantissaBytes)
{
    const int active_length = g_bf_length;
    const std::vector<Byte> source{0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0xfe, 0x34, 0x12};
    const std::vector<Byte> original = source;
    const BigFloatConversionInput &input = GetParam();
    std::vector<Byte> destination(input.expected.size() + 2, 0xcc);

    EXPECT_EQ(0, convert_bf(destination.data() + 1, source.data(), static_cast<int>(input.expected.size()) - 2, 8));

    EXPECT_EQ(input.expected, (std::vector<Byte>{destination.begin() + 1, destination.end() - 1}));
    EXPECT_EQ(0xcc, destination.front());
    EXPECT_EQ(0xcc, destination.back());
    EXPECT_EQ(original, source);
    EXPECT_EQ(active_length, g_bf_length);
}

TEST_P(TestBigFloatByteConversion, unpackConvertsAtCurrentLengthAndReturnsTheNextStoredCursor)
{
    const BigFloatConversionInput &input = GetParam();
    ValueSaver<int> length{g_bf_length, static_cast<int>(input.expected.size()) - 2};
    const std::array<int, 7> lengths{
        g_bn_length, g_bf_length, g_int_length, g_padding, g_r_length, g_r_bf_length, g_shift_factor};
    std::array<Byte, 12> source{0xaa, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0xfe, 0x34, 0x12, 0xbb};
    const std::array<Byte, 12> original = source;
    std::vector<Byte> destination(input.expected.size() + 4, 0xcc);
    std::vector<Byte> expected_destination = destination;
    std::copy(input.expected.begin(), input.expected.end(), expected_destination.begin() + 1);

    const Byte *const cursor = unpack_bf(destination.data() + 1, source.data() + 1, 8);

    EXPECT_EQ(source.data() + 11, cursor);
    EXPECT_EQ(expected_destination, destination);
    EXPECT_EQ(original, source);
    EXPECT_EQ(lengths,
        (std::array<int, 7>{
            g_bn_length, g_bf_length, g_int_length, g_padding, g_r_length, g_r_bf_length, g_shift_factor}));
}

INSTANTIATE_TEST_SUITE_P(Layouts, TestBigFloatByteConversion,
    testing::Values(BigFloatConversionInput{"WidenMantissa",
                        {0, 0, 0, 0, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0xfe, 0x34, 0x12}},
        BigFloatConversionInput{"NarrowMantissa", {0x55, 0x66, 0x77, 0xfe, 0x34, 0x12}},
        BigFloatConversionInput{"SameLength", {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0xfe, 0x34, 0x12}}),
    [](const testing::TestParamInfo<BigFloatConversionInput> &info) { return info.param.name; });

TEST(TestBigFloatDiagnosticBytes, hexDumpPrintsShiftedShortenedWorkspace)
{
    const int original_length = 28;
    std::vector<Byte> workspace(static_cast<std::size_t>(original_length + 2), 0xcc);
    const ValueSaver<int> active_length{g_bf_length, 8};
    Byte *const shortened = workspace.data() + original_length - g_bf_length;
    const std::array<Byte, 10> bytes{0x00, 0x09, 0x8a, 0xff, 0x12, 0x34, 0x80, 0xfe, 0xfe, 0xff};
    std::copy(bytes.begin(), bytes.end(), shortened);
    const std::vector<Byte> original = workspace;
    testing::internal::CaptureStdout();

    bf_hex_dump_snapshot(shortened);

    const std::string output = testing::internal::GetCapturedStdout();
    EXPECT_EQ("00 09 8A FF 12 34 80 FE  e FFFE\n", output);
    EXPECT_EQ(original, workspace);
    EXPECT_EQ(8, g_bf_length);
}

struct BigFloatMigrationInput
{
    const char *name;
    std::size_t blocks;
    int old_length;
    int old_padding;
    int new_length;
    int new_padding;
    std::vector<Byte> source;
    std::vector<Byte> expected;
};

class TestBigFloatByteMigration : public testing::TestWithParam<BigFloatMigrationInput>
{
};

TEST_P(TestBigFloatByteMigration, preservesCanonicalOrWideExtentExponentSourceAndGuards)
{
    const BigFloatMigrationInput &input = GetParam();
    const std::vector<Byte> original = input.source;
    std::vector<Byte> destination(input.expected.size() + 2, 0xcc);

    migrate_bf(destination.data() + 1, input.source.data(), input.blocks, input.new_length, input.new_padding,
        input.old_length, input.old_padding);

    EXPECT_EQ(input.expected, (std::vector<Byte>{destination.begin() + 1, destination.end() - 1}));
    EXPECT_EQ(0xcc, destination.front());
    EXPECT_EQ(0xcc, destination.back());
    EXPECT_EQ(original, input.source);
}

INSTANTIATE_TEST_SUITE_P(Layouts, TestBigFloatByteMigration,
    testing::Values(BigFloatMigrationInput{"CanonicalGrowth", 1, 8, 4, 12, 8,
                        {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0xfe, 0x34, 0x12},
                        {0, 0, 0, 0, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0xfe, 0x34, 0x12}},
        BigFloatMigrationInput{"CanonicalShrink", 1, 12, 8, 8, 4,
            {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xfe, 0xfe, 0xff},
            {0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xfe, 0xfe, 0xff}},
        BigFloatMigrationInput{"WideGrowth", 2, 8, 4, 12, 8,
            {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xfe, 0x78, 0x56},
            {0, 0, 0, 0, 0, 0, 0, 0, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xfe, 0x78,
                0x56}},
        BigFloatMigrationInput{"WideShrink", 2, 12, 8, 8, 4,
            {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x91, 0xa2, 0xb3, 0xc4, 0xd5, 0xe6, 0xf7, 0x08, 0x19, 0x2a,
                0x3b, 0xfe, 0x80, 0xff},
            {0x91, 0xa2, 0xb3, 0xc4, 0xd5, 0xe6, 0xf7, 0x08, 0x19, 0x2a, 0x3b, 0xfe, 0x80, 0xff}}),
    [](const testing::TestParamInfo<BigFloatMigrationInput> &info) { return info.param.name; });

namespace
{

std::vector<Byte> wide_float_fixture(const int length, const bool negative, const int guard)
{
    std::vector<Byte> bytes(length + 2);
    bytes[length - 2] = negative ? 0xee : 0x12;
    bytes[length - 1] = negative ? 0xff : 0;
    bytes[length] = negative ? 0xdc : 0x34;
    bytes[length + 1] = negative ? 0xfe : 0x12;
    if (guard >= 0 && guard < length - 2)
    {
        bytes[guard] = 0x5a;
    }
    return bytes;
}

} // namespace

class TestBigFloatByteWideRelocation : public TestBigFloatBytePrecision,
                                       public testing::WithParamInterface<std::tuple<int, bool>>
{
};

TEST_P(TestBigFloatByteWideRelocation, preservesSignedMantissaExponentAndFractionalGuard)
{
    init_bf_length(std::get<0>(GetParam()));
    const int old_length = g_bf_length;
    const int old_padding = g_padding;
    const int old_wide_length = g_r_bf_length;
    const int guard = old_padding - 1;
    const bool negative = std::get<1>(GetParam());
    const std::vector<Byte> source = wide_float_fixture(old_wide_length, negative, guard);

    init_bf_length(old_length == 56 ? 64 : 56);

    const int new_wide_length = g_r_bf_length;
    const std::vector<Byte> expected =
        wide_float_fixture(new_wide_length, negative, guard + new_wide_length - old_wide_length);
    std::vector<Byte> destination(expected.size() + 4, 0xa5);

    migrate_bf(destination.data() + 2, source.data(), 2, g_bf_length, g_padding, old_length, old_padding);

    EXPECT_EQ(expected, (std::vector<Byte>{destination.begin() + 2, destination.end() - 2}));
    EXPECT_EQ(0xa5, destination[0]);
    EXPECT_EQ(0xa5, destination[1]);
    EXPECT_EQ(0xa5, destination[destination.size() - 2]);
    EXPECT_EQ(0xa5, destination.back());
}

INSTANTIATE_TEST_SUITE_P(LayoutsAndSigns, TestBigFloatByteWideRelocation,
    testing::Combine(testing::Values(4, 12, 52, 56, 64, 708), testing::Bool()));

class TestBigArenaNumericMigration : public testing::TestWithParam<bool>
{
};

TEST_P(TestBigArenaNumericMigration, repeatedGrowthAndReductionPreserveEncodedValues)
{
    const int exponent_bytes = GetParam() ? 2 : 0;
    int old_length = 28;
    BigArena arena{static_cast<std::size_t>(old_length + exponent_bytes), 8, keep_first_block};
    std::vector<BigArena::Allocation> values;
    values.push_back(arena.allocate_single());
    values.push_back(arena.allocate_single());
    values.push_back(arena.allocate_single());
    values.push_back(arena.allocate_double());
    const std::vector<BigArena::Index> indices{0, 1, 2, 4};
    const std::vector<int> widths{1, 1, 1, 2};
    std::vector<std::vector<Byte>> expected;
    for (std::size_t i = 0; i < values.size(); ++i)
    {
        const int length = widths[i] * old_length;
        std::vector<Byte> encoded(length + exponent_bytes);
        if (i != 2)
        {
            encoded[0] = 0x35;
            encoded[1] = 0x7b;
            encoded[length - 2] = i == 0 ? 0x7f : 0x80;
            encoded[length - 1] = i == 0 ? 0 : 0xff;
            if (GetParam())
            {
                encoded[length] = 0x19;
                encoded[length + 1] = i == 0 ? 0 : 0xff;
            }
        }
        std::copy(encoded.begin(), encoded.end(), values[i].data());
        expected.push_back(std::move(encoded));
    }

    // The first growth keeps the same aligned stride; later changes cross it.
    for (const int new_length : {60, 124, 20, 28})
    {
        SCOPED_TRACE(new_length);
        arena.relocate(static_cast<std::size_t>(new_length + exponent_bytes),
            [&](Byte *destination, const Byte *source, std::size_t width)
            {
                const int blocks = static_cast<int>(width);
                if (GetParam())
                {
                    convert_bf(destination, source, blocks * new_length, blocks * old_length);
                }
                else
                {
                    convert_bn(destination, source, blocks * new_length, blocks * 2, blocks * old_length, blocks * 2);
                }
            });

        for (std::size_t i = 0; i < values.size(); ++i)
        {
            const int old_mantissa = widths[i] * old_length;
            const int new_mantissa = widths[i] * new_length;
            const int retained = std::min(old_mantissa, new_mantissa);
            std::vector<Byte> converted(new_mantissa + exponent_bytes);
            std::copy_n(expected[i].data() + old_mantissa - retained, retained + exponent_bytes,
                converted.data() + new_mantissa - retained);
            expected[i] = std::move(converted);
            EXPECT_EQ(expected[i], (std::vector<Byte>{values[i].data(), values[i].data() + expected[i].size()}));
            EXPECT_EQ(indices[i], values[i].index());
            EXPECT_EQ(&arena, &values[i].arena());
            EXPECT_EQ(0, reinterpret_cast<std::uintptr_t>(values[i].data()) % 64);
        }
        EXPECT_EQ(6, arena.high_water_mark());
        EXPECT_EQ(8, arena.capacity());
        EXPECT_EQ(0, arena.block_size() % 64);
        old_length = new_length;
    }
    values.clear();
    const BigArena::Allocation reused = arena.allocate_double();
    EXPECT_EQ(6, arena.high_water_mark());
    EXPECT_EQ(0, reused.index() % 2);
}

INSTANTIATE_TEST_SUITE_P(Representations, TestBigArenaNumericMigration, testing::Bool());

} // namespace id::test
