// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/bailout_formula.h>
#include <engine/UserData.h>
#include <fractals/fractype.h>
#include <math/big_init.h>
#include <math/BigComplex.h>
#include <math/BigFloat.h>
#include <math/complex_fn.h>
#include <misc/ValueSaver.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

using namespace id::engine;
using namespace id::fractals;
using namespace id::math;
using namespace id::misc;

namespace id::test
{

static_assert(std::is_same_v<decltype(g_bf_x_min), BigFloat>);
static_assert(std::is_same_v<decltype(g_bf_x_max), BigFloat>);
static_assert(std::is_same_v<decltype(g_bf_y_min), BigFloat>);
static_assert(std::is_same_v<decltype(g_bf_y_max), BigFloat>);
static_assert(std::is_same_v<decltype(g_bf_x_3rd), BigFloat>);
static_assert(std::is_same_v<decltype(g_bf_y_3rd), BigFloat>);
static_assert(std::is_same_v<decltype(g_bf_save_x_min), BigFloat>);
static_assert(std::is_same_v<decltype(g_bf_save_x_max), BigFloat>);
static_assert(std::is_same_v<decltype(g_bf_save_y_min), BigFloat>);
static_assert(std::is_same_v<decltype(g_bf_save_y_max), BigFloat>);
static_assert(std::is_same_v<decltype(g_bf_save_x_3rd), BigFloat>);
static_assert(std::is_same_v<decltype(g_bf_save_y_3rd), BigFloat>);
static_assert(std::is_same_v<decltype(g_bf_params), BigFloat[10]>);

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

    ValueSaver<long> m_saved_bailout_value;
    ValueSaver<Bailout> m_saved_bailout_test;
    ValueSaver<int> m_saved_bf_digits;
    ValueSaver<FractalType> m_saved_fractal_type;
};

class TestActivePrecision : public TestBigFloat
{
protected:
    void prepare_active_precision(FractalType type);
    void expect_preserved_values();

    int m_length{};
    int m_decimals{};
};

void TestActivePrecision::prepare_active_precision(const FractalType type)
{
    g_fractal_type = type;
    init_bf_dec(20);
    g_bf_x_min = 1.25L;
    m_length = g_bf_length;
    m_decimals = g_decimals;
}

void TestActivePrecision::expect_preserved_values()
{
    EXPECT_EQ(m_length, g_bf_length);
    EXPECT_EQ(m_decimals, g_decimals);
    ASSERT_TRUE(g_bf_x_min);
    EXPECT_EQ(BigFloat{1.25L}, g_bf_x_min);
}

struct RejectedDecimalPrecisionInput
{
    const char *name;
    FractalType type;
    BFMathType math;
    int request;
};

class TestRejectedDecimalPrecision : public TestActivePrecision,
                                     public testing::WithParamInterface<RejectedDecimalPrecisionInput>
{
protected:
    void SetUp() override;
};

void TestRejectedDecimalPrecision::SetUp()
{
    TestBigFloat::SetUp();
    prepare_active_precision(GetParam().type);
    ASSERT_EQ(GetParam().math, g_bf_math);
}

TEST_P(TestRejectedDecimalPrecision, preservesActiveValuesAndAllowsCorrection)
{
    const int integer_length{g_bn_length};

    EXPECT_THROW(init_bf_dec(GetParam().request), PrecisionError);

    expect_preserved_values();
    EXPECT_EQ(integer_length, g_bn_length);
    EXPECT_NO_THROW(init_bf_dec(40));
    EXPECT_EQ(BigFloat{1.25L}, g_bf_x_min);
}

INSTANTIATE_TEST_SUITE_P(
    Requests, TestRejectedDecimalPrecision,
    testing::Values(RejectedDecimalPrecisionInput{"BigFloatUnsupported", FractalType::MANDEL, BFMathType::BIG_FLT,
                                                  2000},
                    RejectedDecimalPrecisionInput{"BigFloatMaximumInteger", FractalType::MANDEL, BFMathType::BIG_FLT,
                                                  std::numeric_limits<int>::max()},
                    RejectedDecimalPrecisionInput{"Negative", FractalType::MANDEL, BFMathType::BIG_FLT, -100}),
    [](const testing::TestParamInfo<RejectedDecimalPrecisionInput> &info) { return info.param.name; });

class TestRejectedBytePrecision : public TestActivePrecision, public testing::WithParamInterface<int>
{
protected:
    void SetUp() override;
};

void TestRejectedBytePrecision::SetUp()
{
    TestBigFloat::SetUp();
    prepare_active_precision(FractalType::MANDEL);
}

TEST_P(TestRejectedBytePrecision, preservesActiveValuesAndAllowsCorrection)
{
    EXPECT_THROW(init_bf_length(GetParam()), PrecisionError);

    expect_preserved_values();
    init_bf_dec(40);
    EXPECT_EQ(BigFloat{1.25L}, g_bf_x_min);
}

INSTANTIATE_TEST_SUITE_P(Requests, TestRejectedBytePrecision,
    testing::Values(4096, std::numeric_limits<int>::max(), std::numeric_limits<int>::min()));

TEST_F(TestBigFloat, rejectedForcedPrecisionAllowsCorrectedRequest)
{
    g_bf_digits = 2000;

    EXPECT_THROW(init_bf_dec(20), PrecisionError);

    g_bf_digits = 40;
    EXPECT_NO_THROW(init_bf_dec(20));
    EXPECT_EQ(40, g_decimals);
}

struct ArenaPrecisionLimit
{
    const char *name;
    FractalType type;
    BFMathType math;
    int length;
};

class TestArenaPrecisionLimit : public TestBigFloat, public testing::WithParamInterface<ArenaPrecisionLimit>
{
protected:
    void SetUp() override;
};

void TestArenaPrecisionLimit::SetUp()
{
    TestBigFloat::SetUp();
    g_fractal_type = GetParam().type;
    init_bf_length(GetParam().length);
    ASSERT_EQ(GetParam().math, g_bf_math);
    ASSERT_EQ(GetParam().length, g_bn_length);
    g_bf_x_min = 1.25;
}

TEST_P(TestArenaPrecisionLimit, rejectsRequestsPastTheBackendLimit)
{

    EXPECT_THROW(init_bf_length(GetParam().length + 1), PrecisionError);

    EXPECT_EQ(GetParam().length, g_bn_length);
    EXPECT_EQ(1.25L, g_bf_x_min.to_float());
}

INSTANTIATE_TEST_SUITE_P(Backends, TestArenaPrecisionLimit,
                         testing::Values(ArenaPrecisionLimit{"BigFloat", FractalType::MANDEL, BFMathType::BIG_FLT,
                                                             708}),
                         [](const testing::TestParamInfo<ArenaPrecisionLimit> &info) { return info.param.name; });

TEST_F(TestBigFloat, loweringPrecisionPreservesCorners)
{
    init_bf_dec(1200);
    g_bf_x_min = 1.25;
    init_bf_dec(20);
    EXPECT_EQ(1.25L, g_bf_x_min.to_float());
}

TEST_F(TestBigFloat, changingLayoutRebuildsJuliaParameters)
{
    g_fractal_type = FractalType::JULIA;
    init_bf_length(12);
    const ValueSaver real{g_params[0], -0.75};
    const ValueSaver imag{g_params[1], 0.125};
    g_bf_params[0] = 1.5L;
    g_bf_params[1] = -2.0L;

    init_bf_length(64);

    EXPECT_EQ(-0.75L, g_bf_params[0].to_float());
    EXPECT_EQ(0.125L, g_bf_params[1].to_float());
}

class TestBigFloatCornerPrecision : public TestBigFloat, public testing::WithParamInterface<std::pair<int, int>>
{
};

TEST_P(TestBigFloatCornerPrecision, preservesCurrentAndSavedCorners)
{
    const auto [source_length, destination_length] = GetParam();
    init_bf_length(source_length);
    const std::array<BigFloat *, 12> corners{&g_bf_x_min, &g_bf_x_max, &g_bf_y_min, &g_bf_y_max, &g_bf_x_3rd,
        &g_bf_y_3rd, &g_bf_save_x_min, &g_bf_save_x_max, &g_bf_save_y_min, &g_bf_save_y_max, &g_bf_save_x_3rd,
        &g_bf_save_y_3rd};
    const std::array<const char *, 12> expected{"-1.000000000000000000867361737988403547205962240695953369140625",
        "0.75", "-1.25", "1.5", "-0.875", "-1.125", "-2.5", "2.25", "-1.875", "1.625", "-2.125",
        "-1.999999999999999999132638262011596452794037759304046630859375"};
    for (std::size_t i = 0; i < corners.size(); ++i)
    {
        *corners[i] = BigFloat{std::string_view{expected[i]}};
    }

    init_bf_length(destination_length);

    for (std::size_t i = 0; i < corners.size(); ++i)
    {
        SCOPED_TRACE(i);
        EXPECT_EQ(BigFloat{std::string_view{expected[i]}}, *corners[i]);
    }
}

INSTANTIATE_TEST_SUITE_P(Layouts, TestBigFloatCornerPrecision, testing::Values(std::pair{12, 64}, std::pair{64, 12}));

TEST_F(TestBigFloat, owningValuesSurviveContextReset)
{
    constexpr int LENGTH = 12;
    init_bf_length(LENGTH);
    const BigFloat floating{-2.5L};
    const BigFloat fixed{3.75L};

    free_bf_vars();
    EXPECT_TRUE(floating);
    EXPECT_TRUE(fixed);
    init_bf_length(LENGTH);

    EXPECT_EQ(BigFloat{-2.5L}, floating);
    EXPECT_EQ(BigFloat{3.75L}, fixed);
    EXPECT_EQ(BigFloat{0}, g_bf_x_min);
}

TEST_F(TestBigFloat, parameterPackingCanonicalizesLegacyBytesAndRejectsIncompletePayloads)
{
    std::vector<Byte> input(22 * (g_bf_length + 2));
    for (std::size_t i = 0; i < input.size(); ++i)
    {
        input[i] = static_cast<Byte>(i * 53 + 157);
    }
    unpack_bf_parameters(input);
    const auto expected = pack_bf_parameters();
    unpack_bf_parameters(expected);

    EXPECT_EQ(expected, pack_bf_parameters());
    auto incomplete = expected;
    incomplete.pop_back();
    EXPECT_THROW(unpack_bf_parameters(incomplete), std::invalid_argument);
    EXPECT_EQ(expected, pack_bf_parameters());
}

class TestBigFloatParameterPrecision : public TestBigFloat, public testing::WithParamInterface<std::tuple<int, int>>
{
};

TEST_P(TestBigFloatParameterPrecision, derivesTheSavedPrecisionBeforeUnpacking)
{
    g_fractal_type = FractalType::MANDEL_Z_POWER;
    init_bf_length(std::get<0>(GetParam()));
    const int stored_bn_length = g_bn_length;
    init_bf_length(stored_bn_length);
    const std::array<int, 5> expected_lengths{g_bn_length, g_bf_length, g_int_length, g_decimals, g_bf_decimals};
    g_bf_x_min = 1.25L;
    g_bf_params[9] = -2.5L;
    g_bf_save_y_3rd = 3.75L;
    std::vector<Byte> payload = pack_bf_parameters();
    const std::vector<Byte> expected = payload;
    init_bf_length(std::get<1>(GetParam()));

    ASSERT_NO_THROW(unpack_bf_parameters(payload));

    EXPECT_EQ(expected, pack_bf_parameters());
    EXPECT_EQ(1.25L, g_bf_x_min.to_float());
    EXPECT_EQ(-2.5L, g_bf_params[9].to_float());
    EXPECT_EQ(3.75L, g_bf_save_y_3rd.to_float());
    EXPECT_EQ(
        expected_lengths, (std::array<int, 5>{g_bn_length, g_bf_length, g_int_length, g_decimals, g_bf_decimals}));
    EXPECT_EQ(expected, payload);
}

INSTANTIATE_TEST_SUITE_P(StoredAndActivePrecisions, TestBigFloatParameterPrecision,
    testing::Combine(testing::Values(4, 5, 12, 65, 708), testing::Values(4, 64)));

class TestBigFloatParameterPayload : public TestBigFloat, public testing::WithParamInterface<int>
{
};

TEST_P(TestBigFloatParameterPayload, rejectsMalformedSizesWithoutChangingPrecisionOrParameters)
{
    g_bf_x_min = 1.25L;
    const std::vector<Byte> expected = pack_bf_parameters();
    const std::array<int, 4> lengths{g_bn_length, g_bf_length, g_decimals, g_bf_decimals};
    const std::vector<Byte> malformed(GetParam(), 0);

    EXPECT_THROW(unpack_bf_parameters(malformed), std::invalid_argument);

    EXPECT_EQ(expected, pack_bf_parameters());
    EXPECT_EQ(lengths, (std::array<int, 4>{g_bn_length, g_bf_length, g_decimals, g_bf_decimals}));
}

INSTANTIATE_TEST_SUITE_P(Sizes, TestBigFloatParameterPayload, testing::Values(0, 1, 132, 154, 219, 221, 242, 15730));

class TestPersistentBigFloatParameters : public TestBigFloat, public testing::WithParamInterface<int>
{
};

TEST_P(TestPersistentBigFloatParameters, preservesEverySerializedFieldAndRejectsIncompletePayload)
{
    g_fractal_type = FractalType::MANDEL_Z_POWER;
    init_bf_length(GetParam());
    const std::size_t width = static_cast<std::size_t>(g_bf_length + 2);
    std::vector<Byte> expected(22 * width);
    for (std::size_t field = 0; field < 22; ++field)
    {
        for (std::size_t byte = 0; byte < width - 2; ++byte)
        {
            expected[field * width + byte] = static_cast<Byte>(field * 17 + byte * 53);
        }
        // Canonical signed significands with distinct values and exponent bytes.
        expected[(field + 1) * width - 4] = static_cast<Byte>(0x31 + field);
        expected[(field + 1) * width - 3] = field % 2 == 0 ? 0x00 : 0xff;
        const auto exponent = static_cast<std::uint16_t>(static_cast<int>(field) - 11);
        expected[(field + 1) * width - 2] = static_cast<Byte>(exponent);
        expected[(field + 1) * width - 1] = static_cast<Byte>(exponent >> 8);
    }

    const std::vector<Byte> zeros(expected.size());
    unpack_bf_parameters(zeros);
    EXPECT_EQ(zeros, pack_bf_parameters());
    unpack_bf_parameters(expected);
    EXPECT_EQ(expected, pack_bf_parameters());
    auto incomplete = expected;
    incomplete.pop_back();
    EXPECT_THROW(unpack_bf_parameters(incomplete), std::invalid_argument);
    EXPECT_EQ(expected, pack_bf_parameters());
    ASSERT_TRUE(g_bf_x_min);
}

TEST_P(TestPersistentBigFloatParameters, samePrecisionBindingPreservesValuesAndCleanupClearsThem)
{
    g_fractal_type = FractalType::MANDEL_Z_POWER;
    init_bf_length(GetParam());
    const std::array<BigFloat *, 22> values{&g_bf_x_min, &g_bf_x_max, &g_bf_y_min, &g_bf_y_max, &g_bf_x_3rd, &g_bf_y_3rd,
        &g_bf_params[0], &g_bf_params[1], &g_bf_params[2], &g_bf_params[3], &g_bf_params[4], &g_bf_params[5],
        &g_bf_params[6], &g_bf_params[7], &g_bf_params[8], &g_bf_params[9], &g_bf_save_x_min, &g_bf_save_x_max,
        &g_bf_save_y_min, &g_bf_save_y_max, &g_bf_save_x_3rd, &g_bf_save_y_3rd};
    for (std::size_t i = 0; i < values.size(); ++i)
    {
        const int value = -static_cast<int>(i + 1);
        *values[i] = static_cast<LDouble>(value);
    }
    const auto expected = pack_bf_parameters();

    init_bf_length(GetParam());

    EXPECT_EQ(expected, pack_bf_parameters());
    for (std::size_t i = 0; i < values.size(); ++i)
    {
        EXPECT_EQ(BigFloat{-static_cast<LDouble>(i + 1)}, *values[i]);
    }
    free_bf_vars();
    init_bf_length(GetParam());
    EXPECT_EQ(std::vector<Byte>(expected.size()), pack_bf_parameters());
}

TEST_P(TestPersistentBigFloatParameters, initializationAndCleanupSetValuePresence)
{
    g_fractal_type = FractalType::MANDEL_Z_POWER;
    init_bf_length(GetParam());
    const std::array<BigFloat *, 22> values{&g_bf_x_min, &g_bf_x_max, &g_bf_y_min, &g_bf_y_max, &g_bf_x_3rd,
        &g_bf_y_3rd, &g_bf_params[0], &g_bf_params[1], &g_bf_params[2], &g_bf_params[3], &g_bf_params[4],
        &g_bf_params[5], &g_bf_params[6], &g_bf_params[7], &g_bf_params[8], &g_bf_params[9], &g_bf_save_x_min,
        &g_bf_save_x_max, &g_bf_save_y_min, &g_bf_save_y_max, &g_bf_save_x_3rd, &g_bf_save_y_3rd};
    for (const BigFloat *value : values)
    {
        EXPECT_TRUE(*value);
    }

    free_bf_vars();

    for (const BigFloat *value : values)
    {
        EXPECT_FALSE(*value);
    }
    init_bf_length(GetParam());
    for (const BigFloat *value : values)
    {
        EXPECT_TRUE(*value);
    }
}

INSTANTIATE_TEST_SUITE_P(Precision, TestPersistentBigFloatParameters, testing::Values(4, 708),
    [](const testing::TestParamInfo<int> &info) { return info.param == 4 ? "Minimum" : "Maximum"; });

TEST(TestBigFloatNullValue, copiesAndMovesNeedNoNumericContext)
{
    free_bf_vars();
    BigFloat value;
    const BigFloat copy{value};
    BigFloat moved{std::move(value)};
    value = copy;
    moved = std::move(value);
    EXPECT_FALSE(value);
    EXPECT_FALSE(copy);
    EXPECT_FALSE(moved);
    const BFComplex complex;
    EXPECT_FALSE(complex.x);
    EXPECT_FALSE(complex.y);
}

} // namespace id::test
