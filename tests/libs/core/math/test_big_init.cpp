// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/bailout_formula.h>
#include <engine/UserData.h>
#include <fractals/fractype.h>
#include <math/big_init.h>
#include <math/BigComplex.h>
#include <math/BigFloat.h>
#include <math/BigNum.h>
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
static_assert(std::is_same_v<decltype(g_bf_pi), BigFloat>);

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

class TestBigFloatPiStorage : public TestBigFloat, public testing::WithParamInterface<int>
{
protected:
    void SetUp() override;
    static void expect_pi_value();
};

void TestBigFloatPiStorage::SetUp()
{
    g_fractal_type = FractalType::JULIA_Z_POWER;
    init_bf_length(GetParam());
    ASSERT_TRUE(g_bf_pi);
    EXPECT_EQ(&bf_arena(BigLifetime::WORKING), &g_bf_pi.arena());
}

void TestBigFloatPiStorage::expect_pi_value()
{
    EXPECT_NEAR(3.141592653589793238L, g_bf_pi.to_float(), 0.00002L);
}

TEST_P(TestBigFloatPiStorage, initializationUsesTheExistingWorkingSlotAndLeavesPaddingUntouched)
{
    BigArena &working = bf_arena(BigLifetime::WORKING);
    Byte *const original = g_bf_pi.data();
    std::fill_n(original, working.block_size(), Byte{0xa5});
    const std::size_t high_water = working.high_water_mark();
    const BigStackMark stack = save_stack();

    init_big_pi();

    expect_pi_value();
    EXPECT_EQ(original, g_bf_pi.data());
    EXPECT_EQ(high_water, working.high_water_mark());
    EXPECT_EQ(stack, save_stack());
    EXPECT_EQ(std::vector<Byte>(working.block_size() - g_bf_length - 2, Byte{0xa5}),
        (std::vector<Byte>{g_bf_pi.data() + g_bf_length + 2, g_bf_pi.data() + working.block_size()}));
}

TEST_P(TestBigFloatPiStorage, repeatedBindingClearsTheWholeReservationWithoutAllocatingAgain)
{
    init_big_pi();
    BigArena &working = bf_arena(BigLifetime::WORKING);
    Byte *const original = g_bf_pi.data();
    std::fill_n(original, working.block_size(), Byte{0xa5});
    const std::size_t high_water = working.high_water_mark();

    init_bf_length(GetParam());

    EXPECT_EQ(original, g_bf_pi.data());
    EXPECT_EQ(high_water, working.high_water_mark());
    EXPECT_EQ(
        std::vector<Byte>(working.block_size()), (std::vector<Byte>{g_bf_pi.data(), g_bf_pi.data() + working.block_size()}));
    init_big_pi();
    expect_pi_value();
}

TEST_P(TestBigFloatPiStorage, precisionChangesClearPiAndTableCopiesUseTheFreshSlot)
{
    init_big_pi();
    std::fill_n(g_bf_pi.data(), bf_arena(BigLifetime::WORKING).block_size(), Byte{0xa5});

    init_bf_length(GetParam() == 708 ? 4 : 708);

    const std::size_t other_slot_size = bf_arena(BigLifetime::WORKING).block_size();
    EXPECT_EQ(
        std::vector<Byte>(other_slot_size), (std::vector<Byte>{g_bf_pi.data(), g_bf_pi.data() + other_slot_size}));
    init_big_pi();
    expect_pi_value();
    std::fill_n(g_bf_pi.data(), other_slot_size, Byte{0x5a});

    init_bf_length(GetParam());

    const std::size_t original_slot_size = bf_arena(BigLifetime::WORKING).block_size();
    EXPECT_EQ(
        std::vector<Byte>(original_slot_size), (std::vector<Byte>{g_bf_pi.data(), g_bf_pi.data() + original_slot_size}));
    init_big_pi();
    expect_pi_value();
}

TEST_P(TestBigFloatPiStorage, temporaryRewindsPreservePiStorage)
{
    init_big_pi();
    Byte *const original = g_bf_pi.data();
    const BigStackMark stack = save_stack();
    const std::vector<Byte> before(original, original + g_bf_length + 2);

    bf_stack().allocate_double();
    restore_stack(stack);

    EXPECT_EQ(original, g_bf_pi.data());
    EXPECT_EQ(before, (std::vector<Byte>{g_bf_pi.data(), g_bf_pi.data() + g_bf_length + 2}));
}

TEST_P(TestBigFloatPiStorage, repeatedCleanupUnbindsPiAndReinitializationRestoresItsBytes)
{
    init_big_pi();

    free_bf_vars();

    EXPECT_FALSE(g_bf_pi);

    free_bf_vars();

    EXPECT_FALSE(g_bf_pi);

    init_bf_length(GetParam());
    init_big_pi();

    expect_pi_value();
}

INSTANTIATE_TEST_SUITE_P(Precisions, TestBigFloatPiStorage, testing::Values(4, 12, 56, 64, 708));

class TestActivePrecisionArena : public TestBigFloat
{
protected:
    void prepare_active_arena(FractalType type);
    void expect_preserved_arena();

    int m_length{};
    int m_decimals{};
    BigStackMark m_stack{};
    BigFloatPtr m_corner{};
};

void TestActivePrecisionArena::prepare_active_arena(const FractalType type)
{
    g_fractal_type = type;
    init_bf_dec(20);
    float_to_bf(g_bf_x_min, 1.25);
    m_length = g_bf_length;
    m_decimals = g_decimals;
    m_stack = save_stack();
    m_corner = g_bf_x_min.data();
}

void TestActivePrecisionArena::expect_preserved_arena()
{
    EXPECT_EQ(m_length, g_bf_length);
    EXPECT_EQ(m_decimals, g_decimals);
    EXPECT_EQ(m_stack, save_stack());
    EXPECT_EQ(m_corner, g_bf_x_min.data());
    EXPECT_EQ(1.25L, bf_to_float(g_bf_x_min));
}

struct RejectedDecimalPrecisionInput
{
    const char *name;
    FractalType type;
    BFMathType math;
    int request;
};

class TestRejectedDecimalPrecision : public TestActivePrecisionArena,
                                     public testing::WithParamInterface<RejectedDecimalPrecisionInput>
{
protected:
    void SetUp() override;
};

void TestRejectedDecimalPrecision::SetUp()
{
    TestBigFloat::SetUp();
    prepare_active_arena(GetParam().type);
    ASSERT_EQ(GetParam().math, g_bf_math);
}

TEST_P(TestRejectedDecimalPrecision, preservesActiveArenaAndAllowsCorrection)
{
    const int integer_length{g_bn_length};

    EXPECT_THROW(init_bf_dec(GetParam().request), PrecisionError);

    expect_preserved_arena();
    EXPECT_EQ(integer_length, g_bn_length);
    EXPECT_NO_THROW(init_bf_dec(40));
    EXPECT_EQ(1.25L, bf_to_float(g_bf_x_min));
}

INSTANTIATE_TEST_SUITE_P(Requests, TestRejectedDecimalPrecision,
    testing::Values(RejectedDecimalPrecisionInput{"BigNumUnsupported", FractalType::MANDEL, BFMathType::BIG_NUM, 2000},
        RejectedDecimalPrecisionInput{
            "BigNumMaximumInteger", FractalType::MANDEL, BFMathType::BIG_NUM, std::numeric_limits<int>::max()},
        RejectedDecimalPrecisionInput{"BigFloatUnsupported", FractalType::DIVIDE_BROT5, BFMathType::BIG_FLT, 2000}),
    [](const testing::TestParamInfo<RejectedDecimalPrecisionInput> &info) { return info.param.name; });

class TestRejectedBytePrecision : public TestActivePrecisionArena, public testing::WithParamInterface<int>
{
protected:
    void SetUp() override;
};

void TestRejectedBytePrecision::SetUp()
{
    TestBigFloat::SetUp();
    prepare_active_arena(FractalType::MANDEL);
}

TEST_P(TestRejectedBytePrecision, preservesActiveArenaAndAllowsCorrection)
{
    EXPECT_THROW(init_bf_length(GetParam()), PrecisionError);

    expect_preserved_arena();
    init_bf_dec(40);
    EXPECT_EQ(1.25L, bf_to_float(g_bf_x_min));
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
    float_to_bf(g_bf_x_min, 1.25);
}

TEST_P(TestArenaPrecisionLimit, rejectsRequestsPastTheBackendLimit)
{
    const auto stack{save_stack()};

    EXPECT_THROW(init_bf_length(GetParam().length + 1), PrecisionError);

    EXPECT_EQ(GetParam().length, g_bn_length);
    EXPECT_EQ(stack, save_stack());
    EXPECT_EQ(1.25L, bf_to_float(g_bf_x_min));
}

INSTANTIATE_TEST_SUITE_P(Backends, TestArenaPrecisionLimit,
    testing::Values(ArenaPrecisionLimit{"BigNum", FractalType::MANDEL, BFMathType::BIG_NUM, 672},
        ArenaPrecisionLimit{"BigFloat", FractalType::DIVIDE_BROT5, BFMathType::BIG_FLT, 708}),
    [](const testing::TestParamInfo<ArenaPrecisionLimit> &info) { return info.param.name; });

TEST_F(TestBigFloat, stackStorageCrossesSixtyFourKilobytes)
{
    constexpr std::size_t OLD_ARENA_SIZE{64 * 1024};
    const auto stack = save_stack();
    float_to_bf(g_bf_x_min, 1.25);
    {
        BigStackSaver saved;
        const std::size_t stride = bf_arena(BigLifetime::TEMPORARY).block_size();
        for (std::size_t bytes = 0; bytes < OLD_ARENA_SIZE; bytes += stride)
        {
            bf_stack().allocate_single();
        }
        Byte *const value = bf_stack().allocate_single();
        const std::size_t size = static_cast<std::size_t>(g_bf_length + 2);
        const std::vector<Byte> bytes(size, 0x5a);
        std::copy(bytes.begin(), bytes.end(), value);
        EXPECT_GT(big_memory_stats().temporaries, OLD_ARENA_SIZE);
        EXPECT_EQ(bytes, (std::vector<Byte>{value, value + size}));
        EXPECT_EQ(1.25L, bf_to_float(g_bf_x_min));
    }
    EXPECT_EQ(stack, save_stack());
}

class TestArenaStack : public TestBigFloat
{
protected:
    void SetUp() override;
    void fill_stack();

    BigStackMark m_stack{};
    std::size_t m_available{};
};

void TestArenaStack::SetUp()
{
    TestBigFloat::SetUp();
    m_stack = save_stack();
    m_available = bf_arena(BigLifetime::TEMPORARY).capacity();
}

void TestArenaStack::fill_stack()
{
    for (std::size_t i = 0; i < m_available; ++i)
    {
        bf_stack().allocate_single();
    }
}

TEST_F(TestArenaStack, exactExhaustionAllocatesEveryRemainingBlock)
{
    EXPECT_NO_THROW(fill_stack());
    EXPECT_EQ(m_available, save_stack().big_float);
}

class TestExhaustedArenaStack : public TestArenaStack
{
protected:
    void SetUp() override;
};

void TestExhaustedArenaStack::SetUp()
{
    TestArenaStack::SetUp();
    fill_stack();
}

TEST_F(TestExhaustedArenaStack, rejectionPreservesAllocationStateForBothWidths)
{
    const auto before = save_stack();
    const auto high_water = bf_arena(BigLifetime::TEMPORARY).high_water_mark();

    EXPECT_THROW(bf_stack().allocate_single(), PrecisionError);
    EXPECT_THROW(bf_stack().allocate_double(), PrecisionError);

    EXPECT_EQ(before, save_stack());
    EXPECT_EQ(high_water, bf_arena(BigLifetime::TEMPORARY).high_water_mark());
    EXPECT_NO_THROW(bn_stack().allocate_double());
}

TEST_F(TestExhaustedArenaStack, restoringStackAfterRejectionAllowsReuse)
{
    EXPECT_THROW(bf_stack().allocate_single(), PrecisionError);
    restore_stack(m_stack);
    BigFloatPtr value = bf_stack().allocate_single();
    BigFloat{"1.25"}.copy_to_legacy(value, static_cast<std::size_t>(g_bf_length + 2));
    BigFloat observed_float = BigFloat::from_legacy(value, static_cast<std::size_t>(g_bf_length + 2));
    EXPECT_EQ(1.25L, bf_to_float(observed_float));
}

TEST_F(TestBigFloat, stackSaverUnwindsAfterExhaustion)
{
    const auto stack = save_stack();
    EXPECT_THROW(
        {
            BigStackSaver saved;
            for (std::size_t i = 0; i < bf_arena(BigLifetime::TEMPORARY).capacity(); ++i)
            {
                bf_stack().allocate_single();
            }
            bf_stack().allocate_double();
        },
        PrecisionError);
    EXPECT_EQ(stack, save_stack());
    EXPECT_NO_THROW(bf_stack().allocate_single());
}

class TestFreedArena : public TestBigFloat
{
protected:
    void SetUp() override;
};

void TestFreedArena::SetUp()
{
    TestBigFloat::SetUp();
    float_to_bf(g_bf_x_min, 1.25);
    bf_stack().allocate_single();
    free_bf_vars();
}

TEST_F(TestFreedArena, releasesActiveArenaBindings)
{
    EXPECT_EQ(BFMathType::NONE, g_bf_math);
    EXPECT_EQ(BigStackMark{}, save_stack());
    EXPECT_FALSE(g_bf_x_min);
    EXPECT_FALSE(g_bn_tmp2);
    EXPECT_FALSE(g_bf_tmp1);
}

TEST_F(TestFreedArena, changingMathFlagDoesNotReactivateFreedBindings)
{
    g_bf_math = BFMathType::BIG_FLT;

    EXPECT_THROW(bf_stack().allocate_single(), std::logic_error);
    EXPECT_THROW(bn_stack().allocate_double(), std::logic_error);
    EXPECT_EQ(BigStackMark{}, save_stack());
}

TEST_F(TestFreedArena, initializesFreshStorageAfterRejectedOperations)
{
    EXPECT_THROW(bf_stack().allocate_single(), std::logic_error);
    EXPECT_THROW(bn_stack().allocate_double(), std::logic_error);
    EXPECT_THROW(init_bf_dec(2000), PrecisionError);
    EXPECT_EQ(BFMathType::NONE, g_bf_math);

    init_bf_dec(20);

    EXPECT_EQ(0.0L, bf_to_float(g_bf_x_min));
    BigFloatPtr value = bf_stack().allocate_single();
    BigFloat{"1.25"}.copy_to_legacy(value, static_cast<std::size_t>(g_bf_length + 2));
    BigFloat observed_float = BigFloat::from_legacy(value, static_cast<std::size_t>(g_bf_length + 2));
    EXPECT_EQ(1.25L, bf_to_float(observed_float));
}

TEST_F(TestBigFloat, loweringPrecisionPreservesCornersWithoutOverrunningArena)
{
    init_bf_dec(1200);
    float_to_bf(g_bf_x_min, 1.25);
    init_bf_dec(20);
    EXPECT_EQ(1.25L, bf_to_float(g_bf_x_min));
    EXPECT_EQ(BigStackMark{}, save_stack());
    EXPECT_NO_THROW(bf_stack().allocate_single());
}

TEST_F(TestBigFloat, changingPrecisionRebuildsJuliaParametersAndTemporaryStorage)
{
    g_fractal_type = FractalType::JULIA;
    init_bf_length(12);
    const ValueSaver real{g_params[0], -0.75};
    const ValueSaver imag{g_params[1], 0.125};
    g_bf_params[0] = 1.5L;
    g_bf_params[1] = -2.0L;
    bn_stack().allocate_single();
    bf_stack().allocate_double();

    init_bf_length(64);

    EXPECT_EQ(-0.75L, g_bf_params[0].to_float());
    EXPECT_EQ(0.125L, g_bf_params[1].to_float());
    EXPECT_EQ(BigStackMark{}, save_stack());
    EXPECT_EQ(0U, bn_arena(BigLifetime::TEMPORARY).high_water_mark());
    EXPECT_EQ(0U, bf_arena(BigLifetime::TEMPORARY).high_water_mark());
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
        EXPECT_EQ(&bf_arena(BigLifetime::PERSISTENT), &corners[i]->arena());
    }
}

INSTANTIATE_TEST_SUITE_P(Layouts, TestBigFloatCornerPrecision, testing::Values(std::pair{12, 64}, std::pair{64, 12}));

TEST_F(TestBigFloat, nestedStackRewindsPreserveOuterValuesAndPersistentCorners)
{
    const auto start = save_stack();
    float_to_bf(g_bf_x_min, -1.25);
    {
        BigStackSaver outer;
        const std::size_t fixed_block_size = bn_arena(BigLifetime::TEMPORARY).block_size();
        const std::vector<Byte> fixed_sentinel(fixed_block_size, 0x5a);
        Byte *const fixed = bn_stack().allocate_single();
        BigFloatPtr floating = bf_stack().allocate_single();
        std::copy(fixed_sentinel.begin(), fixed_sentinel.end(), fixed);
        BigFloat{-3.75}.copy_to_legacy(floating, static_cast<std::size_t>(g_bf_length + 2));
        const auto middle = save_stack();
        {
            BigStackSaver inner;
            std::fill_n(bn_stack().allocate_double(), 2 * fixed_block_size, Byte{0xa5});
            BigFloat{8.25}.copy_to_legacy(bf_stack().allocate_double(), static_cast<std::size_t>(g_bf_length + 2));
            EXPECT_GT(save_stack(), middle);
        }
        EXPECT_EQ(middle, save_stack());
        EXPECT_TRUE(std::equal(fixed_sentinel.begin(), fixed_sentinel.end(), fixed));
        BigFloat observed_float = BigFloat::from_legacy(floating, static_cast<std::size_t>(g_bf_length + 2));
        EXPECT_EQ(-3.75L, bf_to_float(observed_float));
        EXPECT_EQ(-1.25L, bf_to_float(g_bf_x_min));
    }
    EXPECT_EQ(start, save_stack());
    EXPECT_EQ(-1.25L, bf_to_float(g_bf_x_min));
}

TEST_F(TestBigFloat, eachRepresentationAndLifetimeHasItsOwnAlignedArena)
{
    std::vector<BigArena::Allocation> values;
    std::vector<const BigArena *> arenas;
    for (BigLifetime lifetime : {BigLifetime::PERSISTENT, BigLifetime::WORKING, BigLifetime::TEMPORARY})
    {
        for (BigArena *arena : {&bn_arena(lifetime), &bf_arena(lifetime)})
        {
            EXPECT_EQ(arenas.end(), std::find(arenas.begin(), arenas.end(), arena));
            arenas.push_back(arena);
            values.push_back(arena->allocate_single());
            EXPECT_EQ(0, reinterpret_cast<std::uintptr_t>(values.back().data()) % 64);
            EXPECT_EQ(0, arena->block_size() % 64);
        }
    }
}

TEST_F(TestBigFloat, bothStackAdaptersRewindWhileOwnedPersistentAndWorkingValuesSurvive)
{
    auto persistent = bf_arena(BigLifetime::PERSISTENT).allocate_single();
    const BigNum working{bn_arena(BigLifetime::WORKING), 3.75L};
    BigFloat{-2.5}.copy_to_legacy(persistent.data(), static_cast<std::size_t>(g_bf_length + 2));
    const auto before = save_stack();
    {
        BigStackSaver scope;
        bn_stack().allocate_single();
        BigFloat{6.25}.copy_to_legacy(bf_stack().allocate_double(), static_cast<std::size_t>(g_bf_length + 2));
        const auto middle = save_stack();
        EXPECT_GT(middle.big_num, before.big_num);
        EXPECT_GT(middle.big_float, before.big_float);
        {
            BigStackSaver inner;
            bn_stack().allocate_double();
            bf_stack().allocate_single();
        }
        EXPECT_EQ(middle, save_stack());
    }
    EXPECT_EQ(before, save_stack());
    BigFloat observed_float = BigFloat::from_legacy(persistent.data(), static_cast<std::size_t>(g_bf_length + 2));
    EXPECT_EQ(-2.5L, bf_to_float(observed_float));
    EXPECT_EQ(BigNum{3.75L}, working);
}

TEST_F(TestBigFloat, stackRewindCoalescesAReleasedNarrowedAllocationWithItsBorrowedHalf)
{
    const auto before = save_stack();
    auto &arena = bf_arena(BigLifetime::TEMPORARY);
    BigArena::Index index{};
    {
        BigStackSaver scope;
        auto wide = arena.allocate_double();
        index = wide.index();
        BigFloat{-2.5}.copy_to_legacy(wide.data() + g_padding, static_cast<std::size_t>(g_bf_length + 2));
        wide.reduce();
        const auto middle = save_stack();
        {
            BigStackSaver inner;
            BigFloatPtr half = bf_stack().allocate_single();
            EXPECT_EQ(wide.data() + arena.block_size(), half);
            BigFloat{3.75}.copy_to_legacy(half, static_cast<std::size_t>(g_bf_length + 2));
            bn_stack().allocate_double();
        }
        EXPECT_EQ(middle, save_stack());
        BigFloat observed_float = BigFloat::from_legacy(wide.data(), static_cast<std::size_t>(g_bf_length + 2));
        EXPECT_EQ(-2.5L, bf_to_float(observed_float));
        bf_stack().allocate_single();
        // The owning first half is released before the scope rewinds the second.
    }
    EXPECT_EQ(before, save_stack());
    const auto reused = arena.allocate_double();
    EXPECT_EQ(index, reused.index());
}

TEST_F(TestBigFloat, ownedArenaValuesSurviveContextResetAtTheSamePrecision)
{
    constexpr int LENGTH = 12;
    init_bf_length(LENGTH);
    auto floating = bf_arena(BigLifetime::WORKING).allocate_single();
    BigArena &fixed_arena = bn_arena(BigLifetime::PERSISTENT);
    const BigNum fixed{fixed_arena, 3.75L};
    BigFloat{-2.5}.copy_to_legacy(floating.data(), static_cast<std::size_t>(g_bf_length + 2));
    const auto floating_index = floating.index();

    free_bf_vars();
    EXPECT_NE(nullptr, floating.data());
    EXPECT_TRUE(fixed);
    init_bf_length(LENGTH);

    EXPECT_EQ(floating_index, floating.index());
    EXPECT_EQ(&fixed_arena, &fixed.arena());
    BigFloat observed_float = BigFloat::from_legacy(floating.data(), static_cast<std::size_t>(g_bf_length + 2));
    EXPECT_EQ(-2.5L, bf_to_float(observed_float));
    EXPECT_EQ(BigNum{3.75L}, fixed);
    EXPECT_EQ(0.0L, bf_to_float(g_bf_x_min));
}

TEST_F(TestBigFloat, arenaReducesProductsThroughItsConfiguredFunction)
{
    BigArena::Allocation floating = bf_arena(BigLifetime::TEMPORARY).allocate_double();
    std::vector<Byte> expected_float(g_bf_length + 2);
    expected_float.front() = 0x59;
    expected_float[expected_float.size() - 2] = 0x34;
    expected_float.back() = 0xfe;
    std::copy(expected_float.begin(), expected_float.end(), floating.data() + g_padding);
    const BigArena::Index floating_index = floating.index();

    floating.reduce();

    EXPECT_EQ(expected_float, (std::vector<Byte>{floating.data(), floating.data() + expected_float.size()}));
    EXPECT_EQ(floating_index, floating.index());
    const BigArena::Allocation floating_half = bf_arena(BigLifetime::TEMPORARY).allocate_single();
    EXPECT_EQ(floating_index + 1, floating_half.index());
}

TEST_F(TestBigFloat, parameterPackingPreservesOpaqueBytesAndRejectsIncompletePayloads)
{
    std::vector<Byte> expected(22 * (g_bf_length + 2));
    for (std::size_t i = 0; i < expected.size(); ++i)
    {
        expected[i] = static_cast<Byte>(i * 53 + 157);
    }
    unpack_bf_parameters(expected);

    EXPECT_EQ(expected, pack_bf_parameters());
    const Byte *const first = g_bf_x_min.data();
    const Byte *const second = g_bf_x_max.data();
    EXPECT_TRUE(std::equal(expected.begin(), expected.begin() + g_bf_length + 2, first));
    EXPECT_TRUE(std::equal(expected.begin() + g_bf_length + 2, expected.begin() + 2 * (g_bf_length + 2), second));
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
    const std::array<int, 10> expected_lengths{g_bn_length, g_bf_length, g_int_length, g_padding, g_r_length,
        g_r_bf_length, g_shift_factor, g_bn_step, g_decimals, g_bf_decimals};
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
    EXPECT_EQ(expected_lengths,
        (std::array<int, 10>{g_bn_length, g_bf_length, g_int_length, g_padding, g_r_length, g_r_bf_length,
            g_shift_factor, g_bn_step, g_decimals, g_bf_decimals}));
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
    const std::array<BigFloat *, 22> values{&g_bf_x_min, &g_bf_x_max, &g_bf_y_min, &g_bf_y_max, &g_bf_x_3rd,
        &g_bf_y_3rd, &g_bf_params[0], &g_bf_params[1], &g_bf_params[2], &g_bf_params[3], &g_bf_params[4],
        &g_bf_params[5], &g_bf_params[6], &g_bf_params[7], &g_bf_params[8], &g_bf_params[9], &g_bf_save_x_min,
        &g_bf_save_x_max, &g_bf_save_y_min, &g_bf_save_y_max, &g_bf_save_x_3rd, &g_bf_save_y_3rd};
    std::vector<Byte> expected(values.size() * width);
    for (std::size_t field = 0; field < values.size(); ++field)
    {
        for (std::size_t byte = 0; byte < width; ++byte)
        {
            const Byte value = static_cast<Byte>(field * 17 + byte * 53);
            values[field]->data()[byte] = value;
            expected[field * width + byte] = value;
        }
    }

    EXPECT_EQ(expected, pack_bf_parameters());
    const std::vector<Byte> zeros(expected.size());
    const Byte *const address = g_bf_x_min.data();
    BigArena &arena = bf_arena(BigLifetime::PERSISTENT);
    const std::size_t high_water = arena.high_water_mark();
    std::fill(g_bf_x_min.data() + width, g_bf_x_min.data() + arena.block_size(), Byte{0xa5});
    unpack_bf_parameters(zeros);
    EXPECT_EQ(zeros, pack_bf_parameters());
    unpack_bf_parameters(expected);
    EXPECT_EQ(expected, pack_bf_parameters());
    auto incomplete = expected;
    incomplete.pop_back();
    EXPECT_THROW(unpack_bf_parameters(incomplete), std::invalid_argument);
    EXPECT_EQ(expected, pack_bf_parameters());
    EXPECT_EQ(address, g_bf_x_min.data());
    EXPECT_EQ(high_water, arena.high_water_mark());
    EXPECT_EQ(std::vector<Byte>(arena.block_size() - width, 0xa5),
        std::vector<Byte>(g_bf_x_min.data() + width, g_bf_x_min.data() + arena.block_size()));
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
        float_to_bf(*values[i], -static_cast<LDouble>(i + 1));
    }
    init_bf_length(GetParam());
    for (std::size_t i = 0; i < values.size(); ++i)
    {
        EXPECT_EQ(-static_cast<LDouble>(i + 1), bf_to_float(*values[i]));
    }
    const auto high_water = bf_arena(BigLifetime::PERSISTENT).high_water_mark();
    free_bf_vars();
    init_bf_length(GetParam());
    EXPECT_EQ(std::vector<Byte>(22 * (g_bf_length + 2)), pack_bf_parameters());
    EXPECT_EQ(high_water, bf_arena(BigLifetime::PERSISTENT).high_water_mark());
}

TEST_P(TestPersistentBigFloatParameters, ownsPersistentSlotsAndBecomesNullAtCleanup)
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
        EXPECT_EQ(&bf_arena(BigLifetime::PERSISTENT), &value->arena());
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
        EXPECT_EQ(&bf_arena(BigLifetime::PERSISTENT), &value->arena());
    }
}

INSTANTIATE_TEST_SUITE_P(Precision, TestPersistentBigFloatParameters, testing::Values(4, 708),
    [](const testing::TestParamInfo<int> &info) { return info.param == 4 ? "Minimum" : "Maximum"; });

namespace
{

enum class KernelSlot
{
    TMP1,
    TMP2,
    TMP3,
    TMP4,
    TMP5,
    TMP6,
    COPY1,
    COPY2
};

} // namespace

class TestBigFloatKernelStorage : public TestBigFloat, public testing::WithParamInterface<std::tuple<int, KernelSlot>>
{
protected:
    void SetUp() override;
    Byte *buffer() const;
    bool allocated() const;
    std::size_t reservation_size() const;
    void expect_owner() const;
};

void TestBigFloatKernelStorage::SetUp()
{
    g_fractal_type = FractalType::JULIA_Z_POWER;
    init_bf_length(std::get<0>(GetParam()));
}

Byte *TestBigFloatKernelStorage::buffer() const
{
    switch (std::get<1>(GetParam()))
    {
    case KernelSlot::TMP1:
        return g_bf_tmp1.data();
    case KernelSlot::TMP2:
        return g_bf_tmp2.data();
    case KernelSlot::TMP3:
        return g_bf_tmp3.data();
    case KernelSlot::TMP4:
        return g_bf_tmp4.data();
    case KernelSlot::TMP5:
        return g_bf_tmp5.data();
    case KernelSlot::TMP6:
        return g_bf_tmp6.data();
    case KernelSlot::COPY1:
        return g_bf_tmp_copy1.data();
    case KernelSlot::COPY2:
        return g_bf_tmp_copy2.data();
    }
    return nullptr;
}

bool TestBigFloatKernelStorage::allocated() const
{
    switch (std::get<1>(GetParam()))
    {
    case KernelSlot::TMP1:
        return static_cast<bool>(g_bf_tmp1);
    case KernelSlot::TMP2:
        return static_cast<bool>(g_bf_tmp2);
    case KernelSlot::TMP3:
        return static_cast<bool>(g_bf_tmp3);
    case KernelSlot::TMP4:
        return static_cast<bool>(g_bf_tmp4);
    case KernelSlot::TMP5:
        return static_cast<bool>(g_bf_tmp5);
    case KernelSlot::TMP6:
        return static_cast<bool>(g_bf_tmp6);
    case KernelSlot::COPY1:
        return static_cast<bool>(g_bf_tmp_copy1);
    case KernelSlot::COPY2:
        return static_cast<bool>(g_bf_tmp_copy2);
    }
    return false;
}

std::size_t TestBigFloatKernelStorage::reservation_size() const
{
    const auto slot = std::get<1>(GetParam());
    const std::size_t width = slot == KernelSlot::COPY1 || slot == KernelSlot::COPY2 ? 2 : 1;
    return width * bf_arena(BigLifetime::WORKING).block_size();
}

void TestBigFloatKernelStorage::expect_owner() const
{
    static_assert(std::is_same_v<decltype(g_bf_tmp1), BigFloat>);
    static_assert(std::is_same_v<decltype(g_bf_tmp2), BigFloat>);
    static_assert(std::is_same_v<decltype(g_bf_tmp3), BigFloat>);
    static_assert(std::is_same_v<decltype(g_bf_tmp4), BigFloat>);
    static_assert(std::is_same_v<decltype(g_bf_tmp5), BigFloat>);
    static_assert(std::is_same_v<decltype(g_bf_tmp6), BigFloat>);
    static_assert(std::is_same_v<decltype(g_bf_tmp_copy1), BigArena::Allocation>);
    static_assert(std::is_same_v<decltype(g_bf_tmp_copy2), BigArena::Allocation>);
    auto &working = bf_arena(BigLifetime::WORKING);
    switch (std::get<1>(GetParam()))
    {
    case KernelSlot::TMP1:
        EXPECT_EQ(&working, &g_bf_tmp1.arena());
        break;
    case KernelSlot::TMP2:
        EXPECT_EQ(&working, &g_bf_tmp2.arena());
        break;
    case KernelSlot::TMP3:
        EXPECT_EQ(&working, &g_bf_tmp3.arena());
        break;
    case KernelSlot::TMP4:
        EXPECT_EQ(&working, &g_bf_tmp4.arena());
        break;
    case KernelSlot::TMP5:
        EXPECT_EQ(&working, &g_bf_tmp5.arena());
        break;
    case KernelSlot::TMP6:
        EXPECT_EQ(&working, &g_bf_tmp6.arena());
        break;
    case KernelSlot::COPY1:
        EXPECT_EQ(&working, &g_bf_tmp_copy1.arena());
        EXPECT_EQ(2U, g_bf_tmp_copy1.blocks());
        break;
    case KernelSlot::COPY2:
        EXPECT_EQ(&working, &g_bf_tmp_copy2.arena());
        EXPECT_EQ(2U, g_bf_tmp_copy2.blocks());
        break;
    }
}

TEST_P(TestBigFloatKernelStorage, rebindingClearsTheWholeReservationWithoutAllocatingAgain)
{
    ASSERT_TRUE(allocated());
    expect_owner();
    auto *const original = buffer();
    std::fill_n(original, reservation_size(), Byte{0xa5});
    const auto high_water = bf_arena(BigLifetime::WORKING).high_water_mark();

    init_bf_length(std::get<0>(GetParam()));

    ASSERT_TRUE(allocated());
    expect_owner();
    EXPECT_EQ(original, buffer());
    EXPECT_EQ(high_water, bf_arena(BigLifetime::WORKING).high_water_mark());
    EXPECT_EQ(std::vector<Byte>(reservation_size()), std::vector<Byte>(buffer(), buffer() + reservation_size()));
}

TEST_P(TestBigFloatKernelStorage, precisionChangesDiscardTheWholeScratchReservation)
{
    ASSERT_TRUE(allocated());
    expect_owner();
    std::fill_n(buffer(), reservation_size(), Byte{0xa5});
    const int original_length = std::get<0>(GetParam());

    init_bf_length(original_length == 708 ? 4 : 708);

    ASSERT_TRUE(allocated());
    expect_owner();
    EXPECT_EQ(std::vector<Byte>(reservation_size()), std::vector<Byte>(buffer(), buffer() + reservation_size()));
    std::fill_n(buffer(), reservation_size(), Byte{0x5a});

    init_bf_length(original_length);

    ASSERT_TRUE(allocated());
    expect_owner();
    EXPECT_EQ(std::vector<Byte>(reservation_size()), std::vector<Byte>(buffer(), buffer() + reservation_size()));
}

TEST_P(TestBigFloatKernelStorage, repeatedCleanupUnbindsAndReinitializationClearsScratch)
{
    ASSERT_TRUE(allocated());
    expect_owner();
    std::fill_n(buffer(), reservation_size(), Byte{0xa5});

    free_bf_vars();

    EXPECT_FALSE(allocated());

    free_bf_vars();

    EXPECT_FALSE(allocated());

    init_bf_length(std::get<0>(GetParam()));

    ASSERT_TRUE(allocated());
    expect_owner();
    EXPECT_EQ(std::vector<Byte>(reservation_size()), std::vector<Byte>(buffer(), buffer() + reservation_size()));
}

TEST_P(TestBigFloatKernelStorage, temporaryRewindsLeaveWorkingScratchUntouched)
{
    std::fill_n(buffer(), reservation_size(), Byte{0xa5});
    const auto *const original = buffer();
    const auto high_water = bf_arena(BigLifetime::WORKING).high_water_mark();
    const auto stack = save_stack();

    bf_stack().allocate_double();
    restore_stack(stack);

    EXPECT_EQ(original, buffer());
    EXPECT_EQ(high_water, bf_arena(BigLifetime::WORKING).high_water_mark());
    EXPECT_EQ(
        std::vector<Byte>(reservation_size(), Byte{0xa5}), std::vector<Byte>(buffer(), buffer() + reservation_size()));
}

INSTANTIATE_TEST_SUITE_P(PrecisionsAndSlots, TestBigFloatKernelStorage,
    testing::Combine(testing::Values(4, 12, 56, 64, 708),
        testing::Values(KernelSlot::TMP1, KernelSlot::TMP2, KernelSlot::TMP3, KernelSlot::TMP4, KernelSlot::TMP5,
            KernelSlot::TMP6, KernelSlot::COPY1, KernelSlot::COPY2)));

class TestBigNumProductLifetime : public TestBigFloat, public testing::WithParamInterface<std::tuple<int, BigLifetime>>
{
};

TEST_P(TestBigNumProductLifetime, temporaryRewindsPreserveLivePairedAllocations)
{
    init_bf_length(std::get<0>(GetParam()));
    BigArena &arena = bn_arena(std::get<1>(GetParam()));
    BigArena::Allocation product = arena.allocate_double();
    const std::vector<Byte> bytes(2 * arena.block_size(), 0x5a);
    std::copy(bytes.begin(), bytes.end(), product.data());
    const BigStackMark mark = save_stack();
    {
        BigStackSaver saved;
        bn_stack().allocate_double();
        const BigArena::Allocation local = product.clone(bn_arena(BigLifetime::TEMPORARY));
        EXPECT_EQ(bytes, std::vector<Byte>(local.data(), local.data() + bytes.size()));
    }

    EXPECT_EQ(mark, save_stack());
    EXPECT_EQ(bytes, std::vector<Byte>(product.data(), product.data() + bytes.size()));
}

INSTANTIATE_TEST_SUITE_P(PrecisionsAndLifetimes, TestBigNumProductLifetime,
    testing::Combine(testing::Values(4, 12, 64, 672),
        testing::Values(BigLifetime::PERSISTENT, BigLifetime::WORKING, BigLifetime::TEMPORARY)));

namespace
{

template <typename Value>
BigArena &value_arena(const BigLifetime lifetime)
{
    if constexpr (std::is_same_v<Value, BigNum>)
    {
        return bn_arena(lifetime);
    }
    else
    {
        return bf_arena(lifetime);
    }
}

std::vector<Byte> encoded(const BigFloat &value)
{
    std::vector<Byte> result(value.arena().value_size());
    value.copy_to_legacy(result.data(), result.size());
    return result;
}

void unused_reducer(Byte *)
{
}

} // namespace

template <typename Value>
class TestBigValueStorage : public testing::Test
{
protected:
    void SetUp() override;
    void TearDown() override;

    ValueSaver<long> m_bailout{g_user.bailout_value, 0};
    ValueSaver<Bailout> m_test{g_bailout_test, Bailout::MOD};
    ValueSaver<int> m_digits{g_bf_digits, 0};
    ValueSaver<FractalType> m_type{
        g_fractal_type, std::is_same_v<Value, BigNum> ? FractalType::MANDEL : FractalType::JULIA_Z_POWER};
};

template <typename Value>
void TestBigValueStorage<Value>::SetUp()
{
    init_bf_dec(40);
    init_big_pi();
}

template <typename Value>
void TestBigValueStorage<Value>::TearDown()
{
    free_bf_vars();
}

using ValueTypes = testing::Types<BigNum, BigFloat>;
TYPED_TEST_SUITE(TestBigValueStorage, ValueTypes);

template <typename Value>
class TestBigNullValue : public testing::Test
{
};

TYPED_TEST_SUITE(TestBigNullValue, ValueTypes);

TYPED_TEST(TestBigNullValue, defaultConstructionAndNullLifetimeOperationsNeedNoArena)
{
    EXPECT_TRUE(std::is_nothrow_default_constructible_v<TypeParam>);
    TypeParam value;
    EXPECT_FALSE(value);
    const TypeParam copy{value};
    EXPECT_FALSE(copy);
    TypeParam moved{std::move(value)};
    EXPECT_FALSE(value);
    EXPECT_FALSE(moved);
    value = copy;
    EXPECT_FALSE(value);
    value = std::move(moved);
    EXPECT_FALSE(value);
    EXPECT_FALSE(moved);
    const Complex<TypeParam> complex;
    EXPECT_FALSE(complex.x);
    EXPECT_FALSE(complex.y);
}

TYPED_TEST(TestBigValueStorage, assignmentInitializesNullValuesInTheWorkingAllocator)
{
    auto &working = value_arena<TypeParam>(BigLifetime::WORKING);
    TypeParam source{value_arena<TypeParam>(BigLifetime::PERSISTENT), -1.25L};
    TypeParam working_source{2.5L};
    TypeParam copied;
    TypeParam moved;
    TypeParam moved_from_working;
    TypeParam scalar;

    copied = source;
    moved = std::move(source);
    moved_from_working = std::move(working_source);
    scalar = 0;

    EXPECT_TRUE(copied);
    EXPECT_TRUE(moved);
    EXPECT_TRUE(moved_from_working);
    EXPECT_FALSE(working_source);
    EXPECT_TRUE(scalar);
    EXPECT_EQ(&working, &copied.arena());
    EXPECT_EQ(&working, &moved.arena());
    EXPECT_EQ(&working, &moved_from_working.arena());
    EXPECT_EQ(&working, &scalar.arena());
    EXPECT_EQ(TypeParam{-1.25L}, copied);
    EXPECT_EQ(TypeParam{-1.25L}, moved);
    EXPECT_EQ(TypeParam{2.5L}, moved_from_working);
    EXPECT_EQ(TypeParam{0}, scalar);
}

TYPED_TEST(TestBigValueStorage, copyingNullWithASelectedAllocatorDoesNotAllocate)
{
    auto &working = value_arena<TypeParam>(BigLifetime::WORKING);
    BigArena arena{working.value_size(), working.block_size(), 2, unused_reducer};
    const TypeParam null;

    const TypeParam selected{arena, null};
    const TypeParam copy{selected};

    EXPECT_FALSE(selected);
    EXPECT_FALSE(copy);
    EXPECT_EQ(&arena, &selected.arena());
    EXPECT_EQ(&arena, &copy.arena());
    EXPECT_EQ(0, arena.high_water_mark());
}

TYPED_TEST(TestBigValueStorage, assigningNullReleasesStorageAndRetainsTheDestinationAllocator)
{
    auto &working = value_arena<TypeParam>(BigLifetime::WORKING);
    BigArena arena{working.value_size(), working.block_size(), 2, unused_reducer};
    TypeParam value{arena, 1.25L};
    const TypeParam occupied{arena, 2.5L};
    TypeParam null;

    value = null;
    EXPECT_FALSE(value);
    EXPECT_EQ(&arena, &value.arena());
    value = -1.25L;
    EXPECT_EQ(TypeParam{-1.25L}, value);
    value = std::move(null);
    EXPECT_FALSE(value);
    EXPECT_FALSE(null);
    EXPECT_EQ(&arena, &value.arena());
    value = 0;
    EXPECT_TRUE(value);
    EXPECT_EQ(TypeParam{0}, value);
    EXPECT_EQ(TypeParam{2.5L}, occupied);
}

TYPED_TEST(TestBigValueStorage, constructionAndCopiesUseTheSelectedAllocator)
{
    auto &persistent = value_arena<TypeParam>(BigLifetime::PERSISTENT);
    auto &temporary = value_arena<TypeParam>(BigLifetime::TEMPORARY);
    TypeParam source{persistent, -1.25L};
    const TypeParam text{persistent, "-2.5"};
    const TypeParam copy{source};
    const TypeParam selected_copy{temporary, source};
    const TypeParam zero{temporary};

    source = 3.75L;

    EXPECT_EQ(&persistent, &source.arena());
    EXPECT_EQ(&persistent, &copy.arena());
    EXPECT_EQ(&temporary, &selected_copy.arena());
    EXPECT_EQ(&temporary, &zero.arena());
    EXPECT_EQ("-2.5", text.to_string());
    EXPECT_EQ("-1.25", copy.to_string());
    EXPECT_EQ("-1.25", selected_copy.to_string());
    EXPECT_EQ(TypeParam{0}, zero);
}

TYPED_TEST(TestBigValueStorage, conversionsAndImportsUseDestinationRepresentationAndAllocator)
{
    using Other = std::conditional_t<std::is_same_v<TypeParam, BigNum>, BigFloat, BigNum>;
    auto &persistent = value_arena<TypeParam>(BigLifetime::PERSISTENT);
    const Other source{-1.25L};
    const TypeParam converted{persistent, source};
    EXPECT_EQ(&persistent, &converted.arena());
    EXPECT_EQ(TypeParam{-1.25L}, converted);

    if constexpr (std::is_same_v<TypeParam, BigFloat>)
    {
        const std::vector<Byte> bytes = encoded(converted);
        const BigFloat imported = BigFloat::from_legacy(persistent, bytes.data(), bytes.size());
        EXPECT_EQ(&persistent, &imported.arena());
        EXPECT_EQ(bytes, encoded(imported));
    }
}

TYPED_TEST(TestBigValueStorage, assignmentRetainsTheDestinationAllocator)
{
    auto &persistent = value_arena<TypeParam>(BigLifetime::PERSISTENT);
    auto &temporary = value_arena<TypeParam>(BigLifetime::TEMPORARY);
    TypeParam destination{persistent, 2.5L};
    TypeParam source{temporary, -1.25L};

    destination = source;
    EXPECT_EQ(&persistent, &destination.arena());
    EXPECT_EQ(TypeParam{-1.25L}, destination);
    source = 3.75L;
    destination = std::move(source);
    EXPECT_EQ(&persistent, &destination.arena());
    EXPECT_EQ(TypeParam{3.75L}, destination);
    EXPECT_EQ(&temporary, &source.arena());
    source = -2.5L;
    EXPECT_EQ(TypeParam{3.75L}, destination);
}

TYPED_TEST(TestBigValueStorage, movedFromValuesRetainTheirAllocatorForReassignment)
{
    auto &temporary = value_arena<TypeParam>(BigLifetime::TEMPORARY);
    TypeParam source{temporary, -1.25L};
    TypeParam moved{std::move(source)};

    EXPECT_EQ(&temporary, &source.arena());
    EXPECT_EQ(&temporary, &moved.arena());
    source = 2.5L;
    EXPECT_EQ(TypeParam{-1.25L}, moved);
    source = moved;
    EXPECT_EQ(TypeParam{-1.25L}, source);
}

TYPED_TEST(TestBigValueStorage, aFullArenaPermitsMovesAndExistingDestinationAssignment)
{
    auto &working = value_arena<TypeParam>(BigLifetime::WORKING);
    BigArena arena{working.value_size(), working.block_size(), 2, unused_reducer};
    TypeParam first{arena, -1.25L};
    TypeParam second{arena, 2.5L};
    const TypeParam outside{3.75L};

    EXPECT_THROW((TypeParam{first}), PrecisionError);
    TypeParam moved{std::move(first)};
    first = std::move(second);
    moved = outside;
    first = outside;
    EXPECT_EQ(&arena, &moved.arena());
    EXPECT_EQ(&arena, &first.arena());
    EXPECT_EQ(outside, first);
    EXPECT_EQ(outside, moved);
}

TYPED_TEST(TestBigValueStorage, failedCrossArenaAssignmentToAnEmptyValuePreservesTheSource)
{
    auto &working = value_arena<TypeParam>(BigLifetime::WORKING);
    BigArena arena{working.value_size(), working.block_size(), 2, unused_reducer};
    TypeParam destination{arena, -1.25L};
    const TypeParam occupied{arena, 2.5L};
    const TypeParam moved{std::move(destination)};
    TypeParam outside{3.75L};

    EXPECT_THROW(destination = std::move(outside), PrecisionError);
    EXPECT_EQ(&arena, &destination.arena());
    EXPECT_EQ(TypeParam{3.75L}, outside);
    EXPECT_EQ(TypeParam{-1.25L}, moved);
    EXPECT_EQ(TypeParam{2.5L}, occupied);
}

TYPED_TEST(TestBigValueStorage, eagerScalarAndComplexResultsUseTheWorkingAllocator)
{
    auto &persistent = value_arena<TypeParam>(BigLifetime::PERSISTENT);
    auto &working = value_arena<TypeParam>(BigLifetime::WORKING);
    const TypeParam left{persistent, 1.25L};
    const TypeParam right{persistent, 0.5L};
    for (const auto &result : {left + right, left - right, left * right, left / right, -left, left.sqr()})
    {
        EXPECT_EQ(&working, &result.arena());
    }
    const Complex<TypeParam> value{left, right};
    for (const auto &result : {+value, -value, value + value, value - value, value * value, value / value, value * 0.5,
             0.5 * value, power(value, 1), power(value, 3), cube(value)})
    {
        EXPECT_EQ(&working, &result.x.arena());
        EXPECT_EQ(&working, &result.y.arena());
    }
    if constexpr (std::is_same_v<TypeParam, BigFloat>)
    {
        EXPECT_EQ(&working, &left.half().arena());
        EXPECT_EQ(&working, &left.log().arena());
        EXPECT_EQ(&working, &right.exp().arena());
        EXPECT_EQ(&working, &left.atan2(right).arena());
        const auto trigonometric = right.sin_cos();
        EXPECT_EQ(&working, &trigonometric.first.arena());
        EXPECT_EQ(&working, &trigonometric.second.arena());
    }
    EXPECT_EQ(&persistent, &left.arena());
    EXPECT_EQ(&persistent, &right.arena());
}

TYPED_TEST(TestBigValueStorage, copiesMovesAndExportsRemainValidAfterContextReset)
{
    auto &persistent = value_arena<TypeParam>(BigLifetime::PERSISTENT);
    const TypeParam source{persistent, -1.25L};
    TypeParam assigned{2.5L};
    std::vector<Byte> expected;
    if constexpr (std::is_same_v<TypeParam, BigFloat>)
    {
        expected = encoded(source);
    }
    free_bf_vars();

    const TypeParam copy{source};
    TypeParam moving{source};
    const TypeParam moved{std::move(moving)};
    assigned = copy;

    if constexpr (std::is_same_v<TypeParam, BigNum>)
    {
        init_bf_dec(40);
        const BigNum value{-1.25L};
        EXPECT_EQ(value, source);
        EXPECT_EQ(value, copy);
        EXPECT_EQ(value, moved);
        EXPECT_EQ(value, assigned);
    }
    else
    {
        EXPECT_EQ(expected, encoded(source));
        EXPECT_EQ(expected, encoded(copy));
        EXPECT_EQ(expected, encoded(moved));
        EXPECT_EQ(expected, encoded(assigned));
    }
}

TYPED_TEST(TestBigValueStorage, temporaryRewindsLeavePersistentAndWorkingValuesIndependent)
{
    auto &persistent = value_arena<TypeParam>(BigLifetime::PERSISTENT);
    TypeParam destination{persistent, 1.25L};
    const TypeParam working{2.5L};
    const auto before = save_stack();
    {
        BigStackSaver scope;
        TypeParam temporary{value_arena<TypeParam>(BigLifetime::TEMPORARY), 1.25L};
        bf_stack().allocate_double();
        bn_stack().allocate_single();
        destination = temporary + working;
    }
    EXPECT_EQ(before, save_stack());
    EXPECT_EQ(&persistent, &destination.arena());
    EXPECT_EQ(TypeParam{3.75L}, destination);
    EXPECT_EQ(TypeParam{2.5L}, working);
}

TYPED_TEST(TestBigValueStorage, compoundAssignmentsRetainTheDestinationAllocator)
{
    auto &persistent = value_arena<TypeParam>(BigLifetime::PERSISTENT);
    const TypeParam right{value_arena<TypeParam>(BigLifetime::TEMPORARY), 0.5L};
    TypeParam destination{persistent, 1.25L};
    destination += right;
    EXPECT_EQ(&persistent, &destination.arena());
    EXPECT_EQ(TypeParam{1.75L}, destination);
    destination -= right;
    EXPECT_EQ(&persistent, &destination.arena());
    EXPECT_EQ(TypeParam{1.25L}, destination);
    destination *= right;
    EXPECT_EQ(&persistent, &destination.arena());
    EXPECT_EQ(TypeParam{0.625L}, destination);
    destination /= right;
    EXPECT_EQ(&persistent, &destination.arena());
    EXPECT_EQ(TypeParam{1.25L}, destination);
}

TYPED_TEST(TestBigValueStorage, constructionInitializesRecycledStorage)
{
    auto &working = value_arena<TypeParam>(BigLifetime::WORKING);
    BigArena arena{working.value_size(), working.block_size(), 2, unused_reducer};
    auto previous = arena.allocate_single();
    std::fill_n(previous.data(), arena.block_size(), Byte{0xff});
    previous = {};

    const TypeParam zero{arena};

    if constexpr (std::is_same_v<TypeParam, BigNum>)
    {
        EXPECT_EQ(BigNum{0}, zero);
    }
    else
    {
        EXPECT_EQ(std::vector<Byte>(arena.value_size()), encoded(zero));
    }
}

} // namespace id::test
