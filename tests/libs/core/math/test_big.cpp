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

static_assert(std::is_same_v<BFConstPtrComplex, Complex<const Byte *>>);
static_assert(!std::is_convertible_v<BFPtrComplex, BFComplex>);

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
    testing::Combine(testing::Values(8, 12, 16, 64, 68, 712),
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

class TestBigFloatByteComparison : public testing::TestWithParam<std::tuple<int, BigFloatByteComparisonInput>>
{
};

TEST_P(TestBigFloatByteComparison, returnsTheSignedMismatchPosition)
{
    const auto [length, input] = GetParam();
    const ValueSaver<int> bf_length{g_bf_length, length};
    std::vector<Byte> left(length + 2);
    std::vector<Byte> right(length + 2);
    std::copy(input.left_tail.begin(), input.left_tail.end(), left.end() - 5);
    std::copy(input.right_tail.begin(), input.right_tail.end(), right.end() - 5);
    const std::vector<Byte> original_left = left;
    const std::vector<Byte> original_right = right;
    int expected = input.expected;
    if (expected != 0)
    {
        // The expected mismatch positions in the cases use an eight-byte mantissa.
        const int extra_bytes = length - 8;
        expected += expected < 0 ? -extra_bytes : extra_bytes;
    }

    EXPECT_EQ(expected, cmp_bf(left.data(), right.data()));
    EXPECT_EQ(-expected, cmp_bf(right.data(), left.data()));
    EXPECT_EQ(0, cmp_bf(left.data(), left.data()));
    EXPECT_EQ(original_left, left);
    EXPECT_EQ(original_right, right);
}

INSTANTIATE_TEST_SUITE_P(Layouts, TestBigFloatByteComparison,
    testing::Combine(testing::Values(8, 68, 712),
        testing::Values(BigFloatByteComparisonInput{{0x40, 1, 0, 0, 0}, {0x40, 1, 0, 0, 0}, 0},
            BigFloatByteComparisonInput{{0, 0, 0, 0, 0}, {0xc0, 0xfe, 0xff, 0, 0}, 8},
            BigFloatByteComparisonInput{{0, 0x80, 0, 0xff, 0xff}, {0x40, 1, 0, 0, 0}, -8},
            BigFloatByteComparisonInput{{0, 0xfe, 0xff, 1, 0}, {0xc0, 0xfe, 0xff, 0, 0}, -8},
            BigFloatByteComparisonInput{{0x40, 1, 0, 0, 0}, {0x40, 2, 0, 0, 0}, -7},
            BigFloatByteComparisonInput{{0x40, 1, 0, 0, 0}, {0x80, 1, 0, 0, 0}, -6},
            BigFloatByteComparisonInput{{0x80, 0xfe, 0xff, 0, 0}, {0xc0, 0xfe, 0xff, 0, 0}, -6})));

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

class TestBigFloatByteSelfAddSubtract : public testing::TestWithParam<std::tuple<int, bool>>
{
};

TEST_P(TestBigFloatByteSelfAddSubtract, preservesExactExtentWithSameOperand)
{
    const int length = std::get<0>(GetParam());
    const bool subtract = std::get<1>(GetParam());
    const ValueSaver<int> bf_length{g_bf_length, length};
    const ValueSaver<int> bn_length{g_bn_length, 4};
    const std::size_t size = static_cast<std::size_t>(length + 2);
    std::vector<Byte> guarded(size + 4);
    guarded.front() = 0x5a;
    guarded.back() = 0xa5;
    std::vector<Byte> expected = guarded;
    Byte *const value = guarded.data() + 2;
    value[length - 3] = 0x40;
    value[length - 2] = 1;

    if (subtract)
    {
        EXPECT_EQ(value, unsafe_sub_a_bf(value, value));
    }
    else
    {
        expected[2 + length - 3] = 0x80;
        expected[2 + length - 2] = 2;
        EXPECT_EQ(value, unsafe_add_a_bf(value, value));
    }

    EXPECT_EQ(expected, guarded);
    EXPECT_EQ(4, g_bn_length);
    EXPECT_EQ(length, g_bf_length);
}

INSTANTIATE_TEST_SUITE_P(LengthsAndOperations, TestBigFloatByteSelfAddSubtract,
    testing::Combine(testing::Values(8, 68, 712), testing::Bool()));

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

struct BigFloatByteParseCase
{
    std::string_view text;
    std::array<Byte, 5> tail;
};

class TestBigFloatByteParseInterface : public TestBigFloatBytePrecision,
                                       public testing::WithParamInterface<std::tuple<int, BigFloatByteParseCase>>
{
};

TEST_P(TestBigFloatByteParseInterface, writesExactBytesAndPreservesDestinationAndScratchGuards)
{
    init_bf_length(std::get<0>(GetParam()));
    const BigFloatByteParseCase &input = std::get<1>(GetParam());
    const std::size_t size = static_cast<std::size_t>(g_bf_length) + 2;
    std::vector<Byte> result(size + 4, 0xa5);
    std::vector<Byte> scratch(size + 4, 0x3c);
    std::vector<Byte> expected = result;
    std::fill_n(expected.begin() + 2, size, Byte{0});
    std::copy(input.tail.begin(), input.tail.end(), expected.end() - 2 - input.tail.size());
    const int bn_length = g_bn_length;
    const int bf_length = g_bf_length;

    EXPECT_EQ(result.data() + 2, str_to_bf(result.data() + 2, input.text, scratch.data() + 2));

    EXPECT_EQ(expected, result);
    EXPECT_EQ(std::vector<Byte>(2, 0x3c), std::vector<Byte>(scratch.begin(), scratch.begin() + 2));
    EXPECT_EQ(std::vector<Byte>(2, 0x3c), std::vector<Byte>(scratch.end() - 2, scratch.end()));
    EXPECT_EQ(bn_length, g_bn_length);
    EXPECT_EQ(bf_length, g_bf_length);
}

INSTANTIATE_TEST_SUITE_P(LayoutsAndInputs, TestBigFloatByteParseInterface,
    testing::Combine(testing::Values(4, 12, 60, 64, 708),
        testing::Values(BigFloatByteParseCase{"1.25", {0x40, 0x01, 0x00, 0x00, 0x00}},
            BigFloatByteParseCase{"-1.25", {0xc0, 0xfe, 0xff, 0x00, 0x00}},
            BigFloatByteParseCase{"+1.25", {0x40, 0x01, 0x00, 0x00, 0x00}},
            BigFloatByteParseCase{std::string_view{"1.25x", 4}, {0x40, 0x01, 0x00, 0x00, 0x00}},
            BigFloatByteParseCase{"1.25e2", {0x00, 0x7d, 0x00, 0x00, 0x00}},
            BigFloatByteParseCase{"1.25E+2tail", {0x00, 0x7d, 0x00, 0x00, 0x00}},
            BigFloatByteParseCase{"125e-2", {0x40, 0x01, 0x00, 0x00, 0x00}},
            BigFloatByteParseCase{"0.5", {0x00, 0x80, 0x00, 0xff, 0xff}},
            BigFloatByteParseCase{"1.00390625", {0x01, 0x01, 0x00, 0x00, 0x00}},
            BigFloatByteParseCase{"junk1.25", {0x40, 0x01, 0x00, 0x00, 0x00}},
            BigFloatByteParseCase{"1.25e+", {0x40, 0x01, 0x00, 0x00, 0x00}}, BigFloatByteParseCase{"0", {}},
            BigFloatByteParseCase{"-0", {}}, BigFloatByteParseCase{"", {}}, BigFloatByteParseCase{"+", {}},
            BigFloatByteParseCase{"-", {}}, BigFloatByteParseCase{".", {}}, BigFloatByteParseCase{"garbage", {}},
            BigFloatByteParseCase{"1.25x", {}})));

struct BigFloatByteIntegerInput
{
    long value;
    std::array<Byte, 7> tail;
};

class TestBigFloatByteIntegerInterface : public TestBigFloatBytePrecision,
                                         public testing::WithParamInterface<std::tuple<int, BigFloatByteIntegerInput>>
{
};

TEST_P(TestBigFloatByteIntegerInterface, encodesIntegersAndPreservesDestinationGuards)
{
    init_bf_length(std::get<0>(GetParam()));
    const BigFloatByteIntegerInput &input = std::get<1>(GetParam());
    const std::size_t size = static_cast<std::size_t>(g_bf_length) + 2;
    std::vector<Byte> result(size + 4, 0xa5);
    std::vector<Byte> expected = result;
    std::fill_n(expected.begin() + 2, size, Byte{0});
    std::copy(input.tail.begin(), input.tail.end(), expected.end() - 2 - input.tail.size());
    const int bn_length = g_bn_length;
    const int bf_length = g_bf_length;

    EXPECT_EQ(result.data() + 2, int_to_bf(result.data() + 2, input.value));

    EXPECT_EQ(expected, result);
    EXPECT_EQ(bn_length, g_bn_length);
    EXPECT_EQ(bf_length, g_bf_length);
}

INSTANTIATE_TEST_SUITE_P(LayoutsAndIntegers, TestBigFloatByteIntegerInterface,
    testing::Combine(testing::Values(4, 12, 60, 64, 708),
        testing::Values(BigFloatByteIntegerInput{0, {}},
            BigFloatByteIntegerInput{1, {0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00}},
            BigFloatByteIntegerInput{-1, {0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff}},
            BigFloatByteIntegerInput{127, {0x00, 0x00, 0x00, 0x7f, 0x00, 0x00, 0x00}},
            BigFloatByteIntegerInput{-127, {0x00, 0x00, 0x00, 0x81, 0xff, 0x00, 0x00}},
            BigFloatByteIntegerInput{128, {0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00}},
            BigFloatByteIntegerInput{-128, {0x00, 0x00, 0x00, 0x80, 0xff, 0x00, 0x00}},
            BigFloatByteIntegerInput{255, {0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00}},
            BigFloatByteIntegerInput{-255, {0x00, 0x00, 0x00, 0x01, 0xff, 0x00, 0x00}},
            BigFloatByteIntegerInput{256, {0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00}},
            BigFloatByteIntegerInput{-256, {0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00}},
            BigFloatByteIntegerInput{65535, {0x00, 0x00, 0xff, 0xff, 0x00, 0x01, 0x00}},
            BigFloatByteIntegerInput{-65535, {0x00, 0x00, 0x01, 0x00, 0xff, 0x01, 0x00}},
            BigFloatByteIntegerInput{65536, {0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00}},
            BigFloatByteIntegerInput{-65536, {0x00, 0x00, 0x00, 0x00, 0xff, 0x01, 0x00}},
            BigFloatByteIntegerInput{std::numeric_limits<S32>::max(), {0xff, 0xff, 0xff, 0x7f, 0x00, 0x03, 0x00}},
            BigFloatByteIntegerInput{std::numeric_limits<S32>::min(), {0x00, 0x00, 0x00, 0x80, 0xff, 0x03, 0x00}})));

struct BigFloatByteFloatingInput
{
    LDouble value;
    std::array<Byte, 7> tail;
    int fractional_bit{};
};

class TestBigFloatByteFloatingInput
    : public TestBigFloatBytePrecision,
      public testing::WithParamInterface<std::tuple<int, int, BigFloatByteFloatingInput>>
{
};

TEST_P(TestBigFloatByteFloatingInput, encodesNativeValuesAndRestoresSharedLengths)
{
    init_bf_length(std::get<0>(GetParam()));
    const int int_length = std::get<1>(GetParam());
    const ValueSaver<int> integer_length{g_int_length, int_length};
    const BigFloatByteFloatingInput &input = std::get<2>(GetParam());
    const std::size_t size = static_cast<std::size_t>(g_bf_length) + 2;
    std::vector<Byte> result(size + 4, 0xa5);
    std::vector<Byte> expected = result;
    std::fill_n(expected.begin() + 2, size, Byte{0});
    std::copy(input.tail.begin(), input.tail.end(), expected.end() - 2 - input.tail.size());
    if (input.fractional_bit != 0)
    {
        const int position = g_bf_length - 2 - (input.fractional_bit + 7) / 8;
        if (position >= 0)
        {
            expected[2 + position] |= static_cast<Byte>(1U << ((8 - input.fractional_bit % 8) % 8));
        }
    }
    const int bn_length = g_bn_length;
    const int bf_length = g_bf_length;

    EXPECT_EQ(result.data() + 2, float_to_bf(result.data() + 2, input.value));

    EXPECT_EQ(expected, result);
    EXPECT_EQ(bn_length, g_bn_length);
    EXPECT_EQ(bf_length, g_bf_length);
    EXPECT_EQ(int_length, g_int_length);
}

INSTANTIATE_TEST_SUITE_P(LayoutsIntegerLengthsAndInputs, TestBigFloatByteFloatingInput,
    testing::Combine(testing::Values(4, 12, 60, 64, 708), testing::Values(1, 2, 4),
        testing::Values(BigFloatByteFloatingInput{0, {}}, BigFloatByteFloatingInput{-0.0L, {}},
            BigFloatByteFloatingInput{1, {0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00}},
            BigFloatByteFloatingInput{-1, {0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00}},
            BigFloatByteFloatingInput{1.25L, {0x00, 0x00, 0x40, 0x01, 0x00, 0x00, 0x00}},
            BigFloatByteFloatingInput{-1.25L, {0x00, 0x00, 0xc0, 0xfe, 0xff, 0x00, 0x00}},
            BigFloatByteFloatingInput{0.5L, {0x00, 0x00, 0x00, 0x80, 0x00, 0xff, 0xff}},
            BigFloatByteFloatingInput{-0.5L, {0x00, 0x00, 0x00, 0x80, 0xff, 0xff, 0xff}},
            BigFloatByteFloatingInput{255, {0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00}},
            BigFloatByteFloatingInput{256, {0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00}},
            BigFloatByteFloatingInput{257, {0x00, 0x00, 0x01, 0x01, 0x00, 0x01, 0x00}},
            BigFloatByteFloatingInput{65536, {0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00}},
            BigFloatByteFloatingInput{1.0L / 256, {0x00, 0x00, 0x00, 0x01, 0x00, 0xff, 0xff}},
            BigFloatByteFloatingInput{1.0L / 65536, {0x00, 0x00, 0x00, 0x01, 0x00, 0xfe, 0xff}},
            BigFloatByteFloatingInput{std::ldexp(1.0L, 1023), {0x00, 0x00, 0x00, 0x80, 0x00, 0x7f, 0x00}},
            BigFloatByteFloatingInput{-std::ldexp(1.0L, 1023), {0x00, 0x00, 0x00, 0x80, 0xff, 0x7f, 0x00}},
            BigFloatByteFloatingInput{std::ldexp(1.0L, -1022), {0x00, 0x00, 0x00, 0x04, 0x00, 0x80, 0xff}},
            BigFloatByteFloatingInput{1.0L + std::ldexp(1.0L, -52), {0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00}, 52})));

struct BigFloatByteOutputInput
{
    std::array<Byte, 7> tail;
    LDouble expected;
};

class TestBigFloatByteOutputInterface
    : public TestBigFloatBytePrecision,
      public testing::WithParamInterface<std::tuple<int, int, BigFloatByteOutputInput>>
{
};

TEST_P(TestBigFloatByteOutputInterface, observesSignedValuesAndRestoresInputAndLengths)
{
    init_bf_length(std::get<0>(GetParam()));
    const int int_length = std::get<1>(GetParam());
    const ValueSaver<int> integer_length{g_int_length, int_length};
    const BigFloatByteOutputInput &input = std::get<2>(GetParam());
    const std::size_t size = static_cast<std::size_t>(g_bf_length) + 2;
    std::vector<Byte> value(size + 4, 0xa5);
    std::fill_n(value.begin() + 2, size, Byte{0});
    std::copy(input.tail.begin(), input.tail.end(), value.end() - 2 - input.tail.size());
    const std::vector<Byte> original = value;
    const int bn_length = g_bn_length;
    const int bf_length = g_bf_length;

    const LDouble actual = bf_to_float(value.data() + 2);

    EXPECT_EQ(input.expected, actual);
    EXPECT_EQ(std::signbit(input.expected), std::signbit(actual));
    EXPECT_EQ(original, value);
    EXPECT_EQ(bn_length, g_bn_length);
    EXPECT_EQ(bf_length, g_bf_length);
    EXPECT_EQ(int_length, g_int_length);
}

INSTANTIATE_TEST_SUITE_P(LayoutsIntegerLengthsAndInputs, TestBigFloatByteOutputInterface,
    testing::Combine(testing::Values(4, 12, 60, 64, 708), testing::Values(1, 2, 4),
        testing::Values(BigFloatByteOutputInput{{}, 0},
            BigFloatByteOutputInput{{0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00}, 1},
            BigFloatByteOutputInput{{0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00}, -1},
            BigFloatByteOutputInput{{0x00, 0x00, 0x40, 0x01, 0x00, 0x00, 0x00}, 1.25L},
            BigFloatByteOutputInput{{0x00, 0x00, 0xc0, 0xfe, 0xff, 0x00, 0x00}, -1.25L},
            BigFloatByteOutputInput{{0x00, 0x00, 0x00, 0x80, 0x00, 0xff, 0xff}, 0.5L},
            BigFloatByteOutputInput{{0x00, 0x00, 0x00, 0x80, 0xff, 0xff, 0xff}, -0.5L},
            BigFloatByteOutputInput{{0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00}, 255},
            BigFloatByteOutputInput{{0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00}, 256},
            BigFloatByteOutputInput{{0x00, 0x00, 0x00, 0xff, 0x7f, 0x00, 0x00}, 32767},
            BigFloatByteOutputInput{{0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00}, -32768},
            BigFloatByteOutputInput{{0x00, 0x00, 0x40, 0x01, 0x00, 0x0c, 0x00}, std::ldexp(1.25L, 96)},
            BigFloatByteOutputInput{{0x00, 0x00, 0xc0, 0xfe, 0xff, 0xf4, 0xff}, -std::ldexp(1.25L, -96)},
            BigFloatByteOutputInput{
                {0x00, 0x00, 0x00, 0x01, 0x00, 0xb8, 0x0b}, std::numeric_limits<LDouble>::infinity()},
            BigFloatByteOutputInput{
                {0x00, 0x00, 0x00, 0xff, 0xff, 0xb8, 0x0b}, -std::numeric_limits<LDouble>::infinity()},
            BigFloatByteOutputInput{{0x00, 0x00, 0x00, 0x01, 0x00, 0x48, 0xf4}, 0.0L},
            BigFloatByteOutputInput{{0x00, 0x00, 0x00, 0xff, 0xff, 0x48, 0xf4}, -0.0L})));

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

TEST_P(TestBigFloatByteReduction, arenaReductionUsesTheCanonicalWindowAndReleasesTheCompanion)
{
    BigArena &arena = bf_arena(BigLifetime::TEMPORARY);
    BigArena::Allocation product = arena.allocate_double();
    std::fill_n(product.data(), 2 * arena.block_size(), Byte{0xa5});
    std::vector<Byte> expected(g_bf_length + 2);
    expected.front() = 0x59;
    expected[expected.size() - 2] = 0x34;
    expected.back() = 0xfe;
    std::copy(expected.begin(), expected.end(), product.data() + g_padding);
    const BigArena::Index index = product.index();
    const std::size_t high_water = arena.high_water_mark();

    product.reduce();

    EXPECT_EQ(expected, (std::vector<Byte>{product.data(), product.data() + expected.size()}));
    EXPECT_EQ(index, product.index());
    ASSERT_EQ(1U, product.blocks());
    const BigArena::Allocation companion = arena.allocate_single();
    EXPECT_EQ(index + 1, companion.index());
    EXPECT_EQ(high_water, arena.high_water_mark());
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

TEST_P(TestBigFloatByteReciprocal, zeroSaturatesWithoutTouchingScratch)
{
    init_bf_length(GetParam());
    constexpr int exponent = std::numeric_limits<LDouble>::max_exponent / 8;
    std::vector<Byte> source = integer_bf_bytes(g_bf_length, 0, 0);
    std::vector<Byte> expected = integer_bf_bytes(g_bf_length, 1, exponent);
    source.push_back(0xa5);
    expected.push_back(0xa5);
    const std::vector<Byte> original = source;
    std::vector<Byte> result(expected.size(), 0xa5);
    const std::vector<Byte> scratch(expected.size(), 0x5a);
    std::vector<Byte> tmp1 = scratch;
    std::vector<Byte> tmp2 = scratch;

    EXPECT_EQ(result.data(), unsafe_inv_bf(result.data(), source.data(), tmp1.data(), tmp2.data()));

    EXPECT_EQ(expected, result);
    EXPECT_EQ(original, source);
    EXPECT_EQ(scratch, tmp1);
    EXPECT_EQ(scratch, tmp2);
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

TEST_P(TestBigFloatByteDivision, nonzeroNumeratorWithZeroDenominatorSaturates)
{
    init_bf_length(GetParam());
    constexpr int exponent = std::numeric_limits<LDouble>::max_exponent / 8;
    std::vector<Byte> numerator = integer_bf_bytes(g_bf_length, -8, 1);
    std::vector<Byte> denominator = integer_bf_bytes(g_bf_length, 0, 0);
    std::vector<Byte> expected = integer_bf_bytes(g_bf_length, 1, exponent);
    numerator.push_back(0xa5);
    denominator.push_back(0xa5);
    expected.push_back(0xa5);
    std::vector<Byte> expected_numerator = integer_bf_bytes(g_bf_length, -8, 0);
    expected_numerator.push_back(0xa5);
    const std::vector<Byte> original_denominator = denominator;
    std::vector<Byte> result(expected.size(), 0xa5);
    const std::vector<Byte> scratch(expected.size(), 0x5a);
    std::vector<Byte> tmp1 = scratch;
    std::vector<Byte> tmp2 = scratch;

    EXPECT_EQ(
        result.data(), unsafe_div_bf(result.data(), numerator.data(), denominator.data(), tmp1.data(), tmp2.data()));

    EXPECT_EQ(expected, result);
    EXPECT_EQ(expected_numerator, numerator);
    EXPECT_EQ(original_denominator, denominator);
    EXPECT_EQ(scratch, tmp1);
    EXPECT_EQ(scratch, tmp2);
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

struct BigFloatByteFormatInput
{
    std::string (*format)(Byte *source, int digits, Byte *decimal);
    const char *suffix;
};

class TestBigFloatByteFormatting : public TestBigFloatBytePrecision,
                                   public testing::WithParamInterface<std::tuple<int, BigFloatByteFormatInput, bool>>
{
};

TEST_P(TestBigFloatByteFormatting, writesExactDecimalBytesWithinBorrowedWindows)
{
    init_bf_length(std::get<0>(GetParam()));
    const BigFloatByteFormatInput &input = std::get<1>(GetParam());
    const bool negative = std::get<2>(GetParam());
    constexpr int offset = 2;
    constexpr int digits = 12;
    constexpr int decimal_size = digits + 4;
    const int size = g_bf_length + 2;
    const std::array<int, 3> lengths{g_bn_length, g_bf_length, g_int_length};
    std::vector<Byte> value = integer_bf_bytes(g_bf_length, negative ? -2 : 1, 0);
    value[g_bf_length - 3] = negative ? 0xc0 : 0x40;
    std::vector<Byte> source(2 * bf_arena(BigLifetime::WORKING).block_size() + 2 * offset, 0x5a);
    std::copy(value.begin(), value.end(), source.begin() + offset);
    std::vector<Byte> expected_source = source;
    std::fill_n(expected_source.begin() + offset, size, Byte{0});
    alignas(U16) std::array<Byte, decimal_size + 2 * offset> decimal{};
    decimal.fill(0xa5);
    std::array<Byte, decimal_size + 2 * offset> expected_decimal = decimal;
    std::fill_n(expected_decimal.begin() + offset, decimal_size, Byte{0});
    expected_decimal[offset] = negative ? 1 : 0;
    expected_decimal[offset + 1] = 1;
    expected_decimal[offset + 2] = 2;
    expected_decimal[offset + 3] = 5;
    const std::string expected = std::string{negative ? "-1.25" : "1.25"} + input.suffix;
    const BigStackMark mark = save_stack();

    EXPECT_EQ(expected, input.format(source.data() + offset, digits, decimal.data() + offset));

    EXPECT_EQ(expected_source, source);
    EXPECT_EQ(expected_decimal, decimal);
    EXPECT_EQ(lengths, (std::array<int, 3>{g_bn_length, g_bf_length, g_int_length}));
    EXPECT_EQ(mark, save_stack());
}

INSTANTIATE_TEST_SUITE_P(LayoutsModesAndSigns, TestBigFloatByteFormatting,
    testing::Combine(testing::Values(4, 12, 64, 708),
        testing::Values(BigFloatByteFormatInput{unsafe_bf_to_string, ""},
            BigFloatByteFormatInput{unsafe_bf_to_string_e, "e0"}, BigFloatByteFormatInput{unsafe_bf_to_string_f, ""}),
        testing::Bool()));

struct BigFloatByteFixedFormatInput
{
    const char *name;
    const char *text;
    int digits;
    const char *expected;
    bool maximum_precision{};
};

class TestBigFloatByteFixedFormat : public TestBigFloatBytePrecision,
                                    public testing::WithParamInterface<BigFloatByteFixedFormatInput>
{
};

TEST_P(TestBigFloatByteFixedFormat, formatsFixedDecimal)
{
    const BigFloatByteFixedFormatInput &input = GetParam();
    if (input.maximum_precision)
    {
        init_bf_length(708);
    }
    const int digits = input.maximum_precision ? g_bf_decimals : input.digits;
    std::vector<Byte> source(g_bf_length + 2);
    std::vector<Byte> scratch(g_bf_length + 2);
    std::vector<Byte> decimal((digits == 0 ? g_decimals : digits) + 4);
    str_to_bf(source.data(), input.text, scratch.data());

    EXPECT_EQ(input.expected, unsafe_bf_to_string_f(source.data(), digits, decimal.data()));
}

INSTANTIATE_TEST_SUITE_P(Inputs, TestBigFloatByteFixedFormat,
    testing::Values(BigFloatByteFixedFormatInput{"Zero", "0", 5, "0.0"},
        BigFloatByteFixedFormatInput{"PositiveDecimal", "1.25", 5, "1.25"},
        BigFloatByteFixedFormatInput{"Negative", "-1.25", 5, "-1.25"},
        BigFloatByteFixedFormatInput{"RoundDown", "1.2344", 4, "1.234"},
        BigFloatByteFixedFormatInput{"RoundUp", "1.2346", 4, "1.235"},
        BigFloatByteFixedFormatInput{"LegacyCarryTruncation", "9.9996", 4, "9.999"},
        BigFloatByteFixedFormatInput{"SmallExponent", "0.000125", 8, "0.000125"},
        BigFloatByteFixedFormatInput{"LargeExponent", "1250000", 8, "1250000.0"},
        BigFloatByteFixedFormatInput{"FewDigitsLargeExponent", "1250000", 3, "1250000.0"},
        BigFloatByteFixedFormatInput{"SingleDigit", "1.25", 1, "1.0"},
        BigFloatByteFixedFormatInput{"DefaultPrecision", "1.234567890123456789", 0, "1.234567890123456789"},
        BigFloatByteFixedFormatInput{"NegativeRounded", "-1.234567890123456789", 5, "-1.2346"},
        BigFloatByteFixedFormatInput{"MaximumPrecision", "-1.25", 0, "-1.25", true}),
    [](const testing::TestParamInfo<BigFloatByteFixedFormatInput> &info) { return info.param.name; });

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

namespace
{

void expect_bf_window_guards(const std::vector<Byte> &buffer)
{
    EXPECT_EQ(0xa5, buffer.front());
    EXPECT_EQ(0xa5, buffer[1]);
    EXPECT_EQ(0xa5, buffer[buffer.size() - 2]);
    EXPECT_EQ(0xa5, buffer.back());
}

} // namespace

class TestBigFloatByteLogarithm : public TestBigFloatBytePrecision,
                                  public testing::WithParamInterface<std::tuple<int, long>>
{
};

TEST_P(TestBigFloatByteLogarithm, confinesThePrecisionRampToTheBorrowedWindows)
{
    init_bf_length(std::get<0>(GetParam()));
    const long input = std::get<1>(GetParam());
    const int size = g_bf_length + 2;
    constexpr int offset = 2;
    const std::array<int, 6> lengths{g_bn_length, g_bf_length, g_padding, g_r_length, g_r_bf_length, g_shift_factor};
    std::vector<Byte> window(size + 2 * offset, 0xa5);
    std::fill_n(window.begin() + offset, size, Byte{0});
    std::vector<Byte> source = window;
    int_to_bf(source.data() + offset, input);
    const std::vector<Byte> original = source;
    std::vector<Byte> result = window;
    std::vector<Byte> expected = window;
    if (input <= 0)
    {
        constexpr int exponent = std::numeric_limits<LDouble>::max_exponent / 8 - 1;
        expected[size - 1] = 0xff;
        expected[size] = static_cast<Byte>(exponent);
        expected[size + 1] = static_cast<Byte>(exponent >> 8);
    }
    Byte *const output = result.data() + offset;
    std::array<std::vector<Byte>, 6> scratch{window, window, window, window, window, window};
    const BigStackMark mark = save_stack();

    EXPECT_EQ(output,
        unsafe_ln_bf(output, source.data() + offset, scratch[0].data() + offset, scratch[1].data() + offset,
            scratch[2].data() + offset, scratch[3].data() + offset, scratch[4].data() + offset,
            scratch[5].data() + offset));

    EXPECT_EQ(expected, result);
    EXPECT_EQ(original, source);
    EXPECT_EQ(window, scratch[4]);
    for (const std::vector<Byte> &buffer : scratch)
    {
        expect_bf_window_guards(buffer);
        if (input <= 0)
        {
            EXPECT_EQ(window, buffer);
        }
    }
    EXPECT_EQ(
        lengths, (std::array<int, 6>{g_bn_length, g_bf_length, g_padding, g_r_length, g_r_bf_length, g_shift_factor}));
    EXPECT_EQ(mark, save_stack());
}

INSTANTIATE_TEST_SUITE_P(PrecisionsAndInputs, TestBigFloatByteLogarithm,
    testing::Combine(testing::Values(32, 64, 708), testing::Values(1L, 0L, -1L)));

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

class TestBigFloatByteArctangent : public TestBigFloatBytePrecision, public testing::WithParamInterface<int>
{
};

TEST_P(TestBigFloatByteArctangent, restoresPrecisionAndKeepsLargeArgumentScratchEffects)
{
    init_bf_length(GetParam());
    const int size = g_bf_length + 2;
    constexpr int offset = 2;
    const std::array<int, 6> lengths{g_bn_length, g_bf_length, g_padding, g_r_length, g_r_bf_length, g_shift_factor};
    std::vector<Byte> window(size + 2 * offset, 0xa5);
    std::fill_n(window.begin() + offset, size, Byte{0});
    std::vector<Byte> source = window;
    Byte *const input = source.data() + offset;
    input[size - 4] = 2;
    std::vector<Byte> result = window;
    Byte *const output = result.data() + offset;
    std::vector<Byte> pi = window;
    copy_bf_pi(pi.data() + offset);
    const std::vector<Byte> original_pi = pi;
    std::array<std::vector<Byte>, 5> scratch{window, window, window, window, window};
    std::vector<Byte> copy1(size + 2 * offset, 0xa5);
    const BigStackMark mark = save_stack();

    EXPECT_EQ(output,
        arctangent(output, input, pi.data() + offset, scratch[0].data() + offset, scratch[1].data() + offset,
            scratch[2].data() + offset, scratch[3].data() + offset, scratch[4].data() + offset, copy1.data() + offset));

    std::vector<Byte> reciprocal = window;
    reciprocal[size - 2] = 0x80;
    reciprocal[size] = 0xff;
    reciprocal[size + 1] = 0xff;
    EXPECT_EQ(reciprocal, source);
    EXPECT_EQ(original_pi, pi);
    EXPECT_EQ(result, scratch[2]);
    EXPECT_EQ(0, BIG_ACCESS16(copy1.data() + offset + g_bf_length));
    expect_bf_window_guards(copy1);
    expect_bf_window_guards(result);
    for (const std::vector<Byte> &buffer : scratch)
    {
        expect_bf_window_guards(buffer);
    }
    EXPECT_EQ(
        lengths, (std::array<int, 6>{g_bn_length, g_bf_length, g_padding, g_r_length, g_r_bf_length, g_shift_factor}));
    EXPECT_EQ(mark, save_stack());
    EXPECT_NEAR(1.1071487177940905L, bf_to_float(output), 1e-11L);
}

INSTANTIATE_TEST_SUITE_P(Precisions, TestBigFloatByteArctangent, testing::Values(32, 64, 708));

class TestBigFloatByteArctangent2 : public TestBigFloatBytePrecision,
                                    public testing::WithParamInterface<std::tuple<int, std::tuple<long, long>>>
{
};

TEST_P(TestBigFloatByteArctangent2, preservesQuadrantReductionAndScratchWrites)
{
    init_bf_length(std::get<0>(GetParam()));
    const std::tuple<long, long> &coordinates = std::get<1>(GetParam());
    const long y = std::get<0>(coordinates);
    const long x = std::get<1>(coordinates);
    const bool subtract_from_pi = x < 0 && y != 0;
    const int size = g_bf_length + 2;
    constexpr int offset = 2;
    const std::array<int, 6> lengths{g_bn_length, g_bf_length, g_padding, g_r_length, g_r_bf_length, g_shift_factor};
    std::vector<Byte> pi(size + 2 * offset, 0xa5);
    copy_bf_pi(pi.data() + offset);
    const std::vector<Byte> original_pi = pi;
    std::vector<Byte> y_storage(size + 2 * offset, 0xa5);
    std::vector<Byte> x_storage(size + 2 * offset, 0xa5);
    std::vector<Byte> result(size + 2 * offset, 0xa5);
    Byte *const ny = y_storage.data() + offset;
    Byte *const nx = x_storage.data() + offset;
    Byte *const output = result.data() + offset;
    int_to_bf(ny, y);
    int_to_bf(nx, x);
    clear_bf(output);
    std::array<std::vector<Byte>, 6> scratch{result, result, result, result, result, result};
    std::vector<Byte> copy1(size + 2 * offset, 0xa5);
    std::vector<Byte> copy2(size + 2 * offset, 0xa5);
    const BigStackMark mark = save_stack();

    EXPECT_EQ(output,
        unsafe_atan2_bf(output, ny, nx, pi.data() + offset, scratch[0].data() + offset, scratch[1].data() + offset,
            scratch[2].data() + offset, scratch[3].data() + offset, scratch[4].data() + offset,
            scratch[5].data() + offset, copy1.data() + offset, copy2.data() + offset));

    EXPECT_EQ(
        lengths, (std::array<int, 6>{g_bn_length, g_bf_length, g_padding, g_r_length, g_r_bf_length, g_shift_factor}));
    std::vector<Byte> expected_y(size + 2 * offset, 0xa5);
    std::vector<Byte> expected_x(size + 2 * offset, 0xa5);
    int_to_bf(expected_y.data() + offset, subtract_from_pi ? std::abs(y) : y);
    int_to_bf(expected_x.data() + offset, subtract_from_pi ? -x : x);
    EXPECT_EQ(expected_y, y_storage);
    EXPECT_EQ(expected_x, x_storage);
    EXPECT_EQ(original_pi, pi);
    expect_bf_window_guards(result);
    for (const std::vector<Byte> &buffer : scratch)
    {
        expect_bf_window_guards(buffer);
    }
    EXPECT_EQ(mark, save_stack());
    EXPECT_NEAR(std::atan2(static_cast<LDouble>(y), static_cast<LDouble>(x)), bf_to_float(output), 1e-11L);
    std::vector<Byte> expected_copy1(size + 2 * offset, 0xa5);
    if (subtract_from_pi)
    {
        std::copy_n(pi.begin() + offset, size, expected_copy1.begin() + offset);
        EXPECT_EQ(0, BIG_ACCESS16(copy2.data() + offset + g_bf_length));
        EXPECT_NEAR(0.4636476090008061L, bf_to_float(copy2.data() + offset), 1e-11L);
        expect_bf_window_guards(copy2);
    }
    else
    {
        EXPECT_EQ(expected_copy1, copy2);
    }
    EXPECT_EQ(expected_copy1, copy1);
    if (y == 0)
    {
        EXPECT_EQ(pi, result);
    }
}

INSTANTIATE_TEST_SUITE_P(PrecisionsQuadrantsAndAxes, TestBigFloatByteArctangent2,
    testing::Combine(testing::Values(32, 64, 708),
        testing::Values(
            std::make_tuple(1L, -2L), std::make_tuple(-1L, -2L), std::make_tuple(0L, -2L), std::make_tuple(-1L, 0L))));

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
    BigNumScalarCall call{BigNumScalarCall::PARSE};
};

void PrintTo(const BigNumScalarInput &input, std::ostream *out)
{
    *out << static_cast<int>(input.call) << ' ' << input.text;
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

class TestBigNumByteLayout : public TestBigNumByteKernel, public testing::WithParamInterface<int>
{
};

INSTANTIATE_TEST_SUITE_P(IntegerBytes, TestBigNumByteLayout, testing::Values(1, 2, 4));

namespace
{

std::vector<Byte> bn_saturation_bytes(const bool negative)
{
    std::vector<Byte> result(g_bn_length, negative ? Byte{0} : Byte{0xff});
    result.front() = negative ? Byte{1} : Byte{0xff};
    result.back() = negative ? Byte{0x80} : Byte{0x7f};
    return result;
}

} // namespace

namespace
{

enum class LayoutScalarInput
{
    NEGATIVE,
    ZERO,
    POSITIVE,
    MINIMUM,
    QUANTUM,
    NEGATIVE_QUANTUM
};

std::vector<Byte> scalar_input_bytes(const LayoutScalarInput input)
{
    std::vector<Byte> parsed(g_bn_length);
    switch (input)
    {
    case LayoutScalarInput::NEGATIVE:
        parsed = fixed_sample(g_bn_length, g_bn_length - g_int_length, -2, 0xc0);
        return parsed;
    case LayoutScalarInput::ZERO:
        return std::vector<Byte>(g_bn_length);
    case LayoutScalarInput::POSITIVE:
        parsed = fixed_sample(g_bn_length, g_bn_length - g_int_length, 1, 0x40);
        return parsed;
    case LayoutScalarInput::MINIMUM:
    {
        std::vector<Byte> result(g_bn_length);
        result.back() = 0x80;
        return result;
    }
    case LayoutScalarInput::QUANTUM:
    {
        std::vector<Byte> result(g_bn_length);
        result.front() = 1;
        return result;
    }
    case LayoutScalarInput::NEGATIVE_QUANTUM:
        return std::vector<Byte>(g_bn_length, 0xff);
    }
    throw std::logic_error("Unexpected scalar input");
}

void negate_encoded_bytes(std::vector<Byte> &value)
{
    unsigned carry = 1;
    for (Byte &byte : value)
    {
        const unsigned digit = (byte ^ 0xff) + carry;
        byte = static_cast<Byte>(digit);
        carry = digit >> 8;
    }
}

std::vector<Byte> scalar_quotient_bytes(std::vector<Byte> value, const U16 divisor)
{
    const bool negative = (value.back() & 0x80) != 0;
    if (divisor == 0)
    {
        return bn_saturation_bytes(negative);
    }
    if (negative)
    {
        negate_encoded_bytes(value);
    }
    unsigned remainder = 0;
    for (std::size_t i = value.size(); i > 0; --i)
    {
        Byte &byte = value[i - 1];
        const unsigned digit = (remainder << 8) | byte;
        byte = static_cast<Byte>(digit / divisor);
        remainder = digit % divisor;
    }
    if (negative)
    {
        negate_encoded_bytes(value);
    }
    return value;
}

} // namespace

class TestBigNumByteScalar : public TestBigNumByteKernel,
                             public testing::WithParamInterface<std::tuple<int, LayoutScalarInput, U16>>
{
protected:
    void SetUp() override
    {
        const int integer_length = std::get<0>(GetParam());
        init_layout(32, integer_length);
        ASSERT_EQ(integer_length, g_int_length);
    }
};

class TestBigNumByteScaling : public TestBigNumByteScalar
{
};

TEST_P(TestBigNumByteScaling, unsignedScalingPreservesAllOperandBitsAndFixedWidthOverflow)
{
    const U16 factor = std::get<2>(GetParam());
    std::vector<Byte> source = scalar_input_bytes(std::get<1>(GetParam()));
    const std::vector<Byte> original = source;
    std::vector<Byte> expected = original;
    unsigned carry = 0;
    for (Byte &byte : expected)
    {
        const unsigned digit = static_cast<unsigned>(byte) * factor + carry;
        byte = static_cast<Byte>(digit);
        carry = digit >> 8;
    }

    std::vector<Byte> result(g_bn_length + 1, 0xa5);
    expected.push_back(0xa5);

    EXPECT_EQ(result.data(), mult_bn_int(result.data(), source.data(), factor));
    EXPECT_EQ(expected, result);
    EXPECT_EQ(original, source);
}

INSTANTIATE_TEST_SUITE_P(LayoutsInputsAndFactors, TestBigNumByteScaling,
    testing::Combine(testing::Values(1, 2, 4),
        testing::Values(LayoutScalarInput::NEGATIVE, LayoutScalarInput::ZERO, LayoutScalarInput::POSITIVE),
        testing::Values<U16>(0, 1, 2, 255, 256, 32768, 65535)));

class TestBigNumByteScalarDivision : public TestBigNumByteScalar
{
};

TEST_P(TestBigNumByteScalarDivision, unsignedDivisionAssignmentPreservesRoundingAndZeroDivisorSaturation)
{
    const U16 divisor = std::get<2>(GetParam());
    std::vector<Byte> value = scalar_input_bytes(std::get<1>(GetParam()));
    std::vector<Byte> expected = scalar_quotient_bytes(value, divisor);
    value.push_back(0xa5);
    expected.push_back(0xa5);

    EXPECT_EQ(value.data(), div_a_bn_int(value.data(), divisor));
    EXPECT_EQ(expected, value);
}

INSTANTIATE_TEST_SUITE_P(LayoutsInputsAndDivisors, TestBigNumByteScalarDivision,
    testing::Combine(testing::Values(1, 2, 4),
        testing::Values(LayoutScalarInput::NEGATIVE, LayoutScalarInput::ZERO, LayoutScalarInput::POSITIVE,
            LayoutScalarInput::MINIMUM, LayoutScalarInput::QUANTUM, LayoutScalarInput::NEGATIVE_QUANTUM),
        testing::Values<U16>(0, 1, 2, 255, 256, 32768, 65535)));

namespace
{

enum class UnaryOperation
{
    CLEAR,
    MAX,
    NEGATE,
    NEGATE_ASSIGN,
    DOUBLE,
    DOUBLE_ASSIGN,
    HALF,
    HALF_ASSIGN,
    MULT_ASSIGN
};

struct EncodedPattern
{
    U16 low;
    Byte fill;
    Byte high;
};

std::vector<Byte> encoded_pattern(const EncodedPattern &pattern)
{
    std::vector<Byte> bytes(static_cast<std::size_t>(g_bn_length), pattern.fill);
    bytes[0] = static_cast<Byte>(pattern.low);
    bytes[1] = static_cast<Byte>(pattern.low >> 8);
    bytes.back() = pattern.high;
    return bytes;
}

struct UnaryInput
{
    const char *name;
    UnaryOperation operation;
    EncodedPattern source;
    EncodedPattern expected;
    U16 factor{};
    bool alias{};
};

void PrintTo(const UnaryInput &input, std::ostream *out)
{
    *out << input.name;
}

struct ObservationInput
{
    const char *name;
    EncodedPattern source;
    int mismatch;
    bool high_byte;
};

void PrintTo(const ObservationInput &input, std::ostream *out)
{
    *out << input.name;
}

} // namespace

class TestBigNumByteUnary : public TestBigNumByteKernel, public testing::WithParamInterface<std::tuple<int, UnaryInput>>
{
protected:
    void SetUp() override;
};

void TestBigNumByteUnary::SetUp()
{
    init_layout(std::get<0>(GetParam()), 1);
}

TEST_P(TestBigNumByteUnary, preservesDestinationAddressGuardsAndAliases)
{
    const UnaryInput &input = std::get<1>(GetParam());
    const bool self = input.alias;
    std::vector<Byte> source_bytes = encoded_pattern(input.source);
    source_bytes.push_back(0x5a);
    const std::vector<Byte> original_source = source_bytes;
    std::vector<Byte> separate = source_bytes;
    separate.back() = 0xa5;
    Byte *const source = source_bytes.data();
    Byte *const destination = self ? source : separate.data();
    std::vector<Byte> expected = encoded_pattern(input.expected);
    expected.push_back(self ? 0x5a : 0xa5);

    switch (input.operation)
    {
    case UnaryOperation::CLEAR:
        EXPECT_EQ(destination, clear_bn(destination));
        break;
    case UnaryOperation::MAX:
        EXPECT_EQ(destination, max_bn(destination));
        break;
    case UnaryOperation::NEGATE:
        EXPECT_EQ(destination, neg_bn(destination, source));
        break;
    case UnaryOperation::NEGATE_ASSIGN:
        EXPECT_EQ(destination, neg_a_bn(destination));
        break;
    case UnaryOperation::DOUBLE:
        EXPECT_EQ(destination, double_bn(destination, source));
        break;
    case UnaryOperation::DOUBLE_ASSIGN:
        EXPECT_EQ(destination, double_a_bn(destination));
        break;
    case UnaryOperation::HALF:
        EXPECT_EQ(destination, half_bn(destination, source));
        break;
    case UnaryOperation::HALF_ASSIGN:
        EXPECT_EQ(destination, half_a_bn(destination));
        break;
    case UnaryOperation::MULT_ASSIGN:
        EXPECT_EQ(destination, mult_a_bn_int(destination, input.factor));
        break;
    }

    EXPECT_EQ(expected, self ? source_bytes : separate);
    if (!self)
    {
        EXPECT_EQ(original_source, source_bytes);
    }
}

INSTANTIATE_TEST_SUITE_P(LayoutsOperationsAndAliases, TestBigNumByteUnary,
    testing::Combine(testing::Values(4, 12, 64, 672),
        testing::Values(UnaryInput{"Clear", UnaryOperation::CLEAR, {0xfffd, 0xff, 0xff}, {0, 0, 0}},
            UnaryInput{"Maximum", UnaryOperation::MAX, {3, 0, 0}, {0xffff, 0xff, 0x7f}},
            UnaryInput{"NegatePositive", UnaryOperation::NEGATE, {3, 0, 0}, {0xfffd, 0xff, 0xff}},
            UnaryInput{"NegateNegative", UnaryOperation::NEGATE, {0xfffd, 0xff, 0xff}, {3, 0, 0}},
            UnaryInput{"NegateMinimum", UnaryOperation::NEGATE, {0, 0, 0x80}, {0, 0, 0x80}},
            UnaryInput{"NegateInPlace", UnaryOperation::NEGATE_ASSIGN, {0xfffd, 0xff, 0xff}, {3, 0, 0}},
            UnaryInput{"DoublePositive", UnaryOperation::DOUBLE, {3, 0, 0}, {6, 0, 0}},
            UnaryInput{"DoubleNegative", UnaryOperation::DOUBLE, {0xfffd, 0xff, 0xff}, {0xfffa, 0xff, 0xff}},
            UnaryInput{"DoubleOverflow", UnaryOperation::DOUBLE, {0xffff, 0xff, 0x7f}, {0xfffe, 0xff, 0xff}},
            UnaryInput{"DoubleInPlace", UnaryOperation::DOUBLE_ASSIGN, {0xffff, 0xff, 0x7f}, {0xfffe, 0xff, 0xff}},
            UnaryInput{"HalfPositiveOdd", UnaryOperation::HALF, {3, 0, 0}, {1, 0, 0}},
            UnaryInput{"HalfNegativeOdd", UnaryOperation::HALF, {0xfffd, 0xff, 0xff}, {0xfffe, 0xff, 0xff}},
            UnaryInput{"HalfMinimum", UnaryOperation::HALF, {0, 0, 0x80}, {0, 0, 0xc0}},
            UnaryInput{"HalfInPlace", UnaryOperation::HALF_ASSIGN, {0xfffd, 0xff, 0xff}, {0xfffe, 0xff, 0xff}},
            UnaryInput{"MultiplyZero", UnaryOperation::MULT_ASSIGN, {0xfffd, 0xff, 0xff}, {0, 0, 0}, 0},
            UnaryInput{"MultiplyOne", UnaryOperation::MULT_ASSIGN, {0xffff, 0xff, 0x7f}, {0xffff, 0xff, 0x7f}, 1},
            UnaryInput{"MultiplyUnsigned", UnaryOperation::MULT_ASSIGN, {0xffff, 0xff, 0xff}, {1, 0xff, 0xff}, 65535},
            UnaryInput{"MultiplyOverflow", UnaryOperation::MULT_ASSIGN, {0, 0, 0x80}, {0, 0, 0}, 2},
            UnaryInput{"NegatePositiveAliased", UnaryOperation::NEGATE, {3, 0, 0}, {0xfffd, 0xff, 0xff}, 0, true},
            UnaryInput{"NegateNegativeAliased", UnaryOperation::NEGATE, {0xfffd, 0xff, 0xff}, {3, 0, 0}, 0, true},
            UnaryInput{"NegateMinimumAliased", UnaryOperation::NEGATE, {0, 0, 0x80}, {0, 0, 0x80}, 0, true},
            UnaryInput{"DoublePositiveAliased", UnaryOperation::DOUBLE, {3, 0, 0}, {6, 0, 0}, 0, true},
            UnaryInput{
                "DoubleNegativeAliased", UnaryOperation::DOUBLE, {0xfffd, 0xff, 0xff}, {0xfffa, 0xff, 0xff}, 0, true},
            UnaryInput{
                "DoubleOverflowAliased", UnaryOperation::DOUBLE, {0xffff, 0xff, 0x7f}, {0xfffe, 0xff, 0xff}, 0, true},
            UnaryInput{"HalfPositiveOddAliased", UnaryOperation::HALF, {3, 0, 0}, {1, 0, 0}, 0, true},
            UnaryInput{
                "HalfNegativeOddAliased", UnaryOperation::HALF, {0xfffd, 0xff, 0xff}, {0xfffe, 0xff, 0xff}, 0, true},
            UnaryInput{"HalfMinimumAliased", UnaryOperation::HALF, {0, 0, 0x80}, {0, 0, 0xc0}, 0, true})));

namespace
{

enum class CanonicalProcedure
{
    COPY,
    COPY_SELF,
    ADD,
    ADD_SELF,
    SUB,
    SUB_SELF,
    ABS_NEGATIVE,
    ABS_POSITIVE,
    MULT_TO,
    MULT_TO_SELF,
    ABS_TO,
    ABS_TO_SELF,
    ABS_TO_POSITIVE
};

} // namespace

enum class IntegerDivisionCall
{
    DISTINCT,
    ALIASED,
    IN_PLACE
};

class TestBigNumByteIntegerDivision
    : public TestBigNumByteKernel,
      public testing::WithParamInterface<std::tuple<int, int, U16, bool, IntegerDivisionCall>>
{
protected:
    void SetUp() override;
};

void TestBigNumByteIntegerDivision::SetUp()
{
    const int integer_length = std::get<1>(GetParam());
    init_layout(std::get<0>(GetParam()), integer_length);
    ASSERT_EQ(integer_length, g_int_length);
}

TEST_P(TestBigNumByteIntegerDivision, preservesResultAddressSourceMutationAliasesAndGuards)
{
    const U16 divisor = std::get<2>(GetParam());
    const bool negative = std::get<3>(GetParam());
    const IntegerDivisionCall call = std::get<4>(GetParam());
    const int fractional_bytes = g_bn_length - g_int_length;
    std::vector<Byte> source = fixed_sample(g_bn_length, fractional_bytes, negative ? -6 : 6);
    std::vector<Byte> destination = fixed_sample(g_bn_length, fractional_bytes, -17);
    std::vector<Byte> expected = scalar_quotient_bytes(source, divisor);
    std::vector<Byte> expected_source =
        call == IntegerDivisionCall::DISTINCT ? fixed_sample(g_bn_length, fractional_bytes, 6) : expected;
    source.push_back(0x5a);
    destination.push_back(0xa5);
    expected_source.push_back(0x5a);
    const std::vector<Byte> untouched = destination;
    std::vector<Byte> &result = call == IntegerDivisionCall::DISTINCT ? destination : source;
    expected.push_back(result.back());

    if (call == IntegerDivisionCall::IN_PLACE)
    {
        EXPECT_EQ(result.data(), div_a_bn_int(result.data(), divisor));
    }
    else
    {
        EXPECT_EQ(result.data(), unsafe_div_bn_int(result.data(), source.data(), divisor));
    }

    EXPECT_EQ(expected, result);
    EXPECT_EQ(expected_source, source);
    if (call != IntegerDivisionCall::DISTINCT)
    {
        EXPECT_EQ(untouched, destination);
    }
}

INSTANTIATE_TEST_SUITE_P(LayoutsDivisorsSignsAndAliases, TestBigNumByteIntegerDivision,
    testing::Combine(testing::Values(12, 32, 672), testing::Values(1, 2, 4), testing::Values<U16>(0, 1, 2, 256),
        testing::Bool(),
        testing::Values(IntegerDivisionCall::DISTINCT, IntegerDivisionCall::ALIASED, IntegerDivisionCall::IN_PLACE)));

namespace
{

struct BigNumByteDivisionScratch
{
    std::vector<Byte> product = std::vector<Byte>(g_r_length);
    std::vector<Byte> tmp2 = std::vector<Byte>(g_bn_length);
    std::vector<Byte> tmp3 = std::vector<Byte>(g_bn_length);
    // The legacy conversion scans one preceding byte for a zero input.
    std::vector<Byte> left_copy = std::vector<Byte>(g_bn_length + 1);
    std::vector<Byte> right_copy = std::vector<Byte>(g_bn_length + 1);
};

std::array<int, 9> big_num_kernel_lengths()
{
    return {g_bn_step, g_bn_length, g_int_length, g_r_length, g_padding, g_shift_factor, g_bf_length, g_r_bf_length,
        g_bf_decimals};
}

} // namespace

TEST_P(TestBigNumByteLayout, arithmeticWrapsAtTheFixedWidth)
{
    init_layout(32, GetParam());
    std::vector<Byte> maximum = bn_saturation_bytes(false);
    std::vector<Byte> minimum(g_bn_length);
    minimum.back() = 0x80;
    std::vector<Byte> quantum(g_bn_length);
    quantum.front() = 1;
    std::vector<Byte> expected_product(g_bn_length, 0xff);
    expected_product.front() = 0xfe;
    std::vector<Byte> result(g_bn_length);

    add_bn(result.data(), maximum.data(), quantum.data());
    EXPECT_EQ(minimum, result);
    sub_bn(result.data(), minimum.data(), quantum.data());
    EXPECT_EQ(maximum, result);
    neg_bn(result.data(), minimum.data());
    EXPECT_EQ(minimum, result);
    mult_bn_int(result.data(), maximum.data(), 2);
    EXPECT_EQ(expected_product, result);
}

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

struct BigNumByteRetainedProductInput
{
    int left;
    int right;
    bool shared{};
    bool sqr{};
};

struct BigNumConversionInput
{
    const char *name;
    int length;
    int integer_length;
    std::vector<Byte> expected;
};

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

struct BigNumToBigFloatInput
{
    const char *name;
    int integer_length;
    int float_length;
    std::array<Byte, 4> source;
    std::vector<Byte> expected;
};

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

struct BigFloatByteHexDumpInput
{
    std::array<Byte, 4> pattern;
    const char *pattern_dump;
    std::array<Byte, 6> tail;
    const char *tail_dump;
};

class TestBigFloatByteHexDump : public testing::TestWithParam<std::tuple<int, BigFloatByteHexDumpInput>>
{
};

TEST_P(TestBigFloatByteHexDump, printsMantissaAndExponentAsStored)
{
    const int length = std::get<0>(GetParam());
    const BigFloatByteHexDumpInput &input = std::get<1>(GetParam());
    const ValueSaver<int> bf_length{g_bf_length, length};
    std::vector<Byte> bytes(length + 2);
    std::string expected;
    for (int i = 0; i < length - 4; i += 4)
    {
        std::copy(input.pattern.begin(), input.pattern.end(), bytes.begin() + i);
        expected += input.pattern_dump;
    }
    std::copy(input.tail.begin(), input.tail.end(), bytes.end() - input.tail.size());
    expected += input.tail_dump;
    const std::vector<Byte> original = bytes;

    testing::internal::CaptureStdout();
    bf_hex_dump_snapshot(bytes.data());
    const std::string output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(expected, output);
    EXPECT_EQ(original, bytes);
}

INSTANTIATE_TEST_SUITE_P(PrecisionsAndInputs, TestBigFloatByteHexDump,
    testing::Combine(testing::Values(8, 712),
        testing::Values(BigFloatByteHexDumpInput{{0x00, 0x09, 0x8a, 0xff}, "00 09 8A FF ",
                            {0x00, 0x09, 0x8a, 0xff, 0x34, 0x12}, "00 09 8A FF  e 1234\n"},
            BigFloatByteHexDumpInput{{0x00, 0x09, 0x8a, 0xff}, "00 09 8A FF ", {0x00, 0x09, 0x8a, 0xff, 0xfe, 0xff},
                "00 09 8A FF  e FFFE\n"},
            BigFloatByteHexDumpInput{{}, "00 00 00 00 ", {0, 0, 0xc0, 0xfe, 0, 0}, "00 00 C0 FE  e 0000\n"})));

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

} // namespace id::test
