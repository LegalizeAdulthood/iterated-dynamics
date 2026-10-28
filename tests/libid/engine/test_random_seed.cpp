// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/random_seed.h>

#include <misc/ValueSaver.h>

#include <gtest/gtest.h>

#include <array>
#include <cstdlib>
#include <ostream>

using namespace id::engine;
using namespace id::misc;

namespace id::test
{
namespace
{

constexpr int SEED{4567};
constexpr std::array<int, 12> MSVC_RAND15_SEQUENCE{
    14952, 9227, 14589, 28486, 27900, 21076, 1617, 32765, 19715, 1795, 18494, 32654};

} // namespace

class TestRandomSeed : public testing::Test
{
private:
    ValueSaver<int> m_random_seed{g_random_seed, SEED};
    ValueSaver<bool> m_random_seed_flag{g_random_seed_flag, true};
};

TEST_F(TestRandomSeed, random15MatchesMsvcCRuntimeSequence)
{
    set_random_seed();

    for (const int value : MSVC_RAND15_SEQUENCE)
    {
        EXPECT_EQ(value, random15());
    }
    EXPECT_EQ(SEED, g_random_seed);
}

TEST_F(TestRandomSeed, nonFixedSeedAdvancesStoredSeedAfterSeeding)
{
    g_random_seed_flag = false;

    set_random_seed();

    EXPECT_EQ(SEED + 1, g_random_seed);
    EXPECT_EQ(MSVC_RAND15_SEQUENCE.front(), random15());
}

TEST_F(TestRandomSeed, explicitSeedDoesNotChangeStoredSeed)
{
    g_random_seed = 99;

    set_random_seed(SEED);

    EXPECT_EQ(99, g_random_seed);
    EXPECT_TRUE(g_random_seed_flag);
    EXPECT_EQ(MSVC_RAND15_SEQUENCE.front(), random15());
}

TEST_F(TestRandomSeed, sameFixedSeedRepeatsSequence)
{
    set_random_seed();
    const int first{random15()};
    const int second{random15()};

    set_random_seed();

    EXPECT_EQ(first, random15());
    EXPECT_EQ(second, random15());
    EXPECT_EQ(SEED, g_random_seed);
}

TEST_F(TestRandomSeed, differentFixedSeedsProduceDifferentFirstValue)
{
    set_random_seed(SEED);
    const int first{random15()};

    set_random_seed(SEED + 1);

    EXPECT_NE(first, random15());
}

TEST_F(TestRandomSeed, cRuntimeRandomCallsDoNotChangeImageSequence)
{
    set_random_seed(SEED);

    EXPECT_EQ(MSVC_RAND15_SEQUENCE[0], random15());

    std::srand(99);
    static_cast<void>(std::rand());
    static_cast<void>(std::rand());

    EXPECT_EQ(MSVC_RAND15_SEQUENCE[1], random15());

    std::srand(100);
    static_cast<void>(std::rand());

    EXPECT_EQ(MSVC_RAND15_SEQUENCE[2], random15());
}

struct RandomLimitInput
{
    const char *name;
    int limit;
    int position;
    int expected;
};

void PrintTo(const RandomLimitInput &value, std::ostream *str)
{
    *str << value.name;
}

class TestRandomSeedLimit : public TestRandomSeed, public testing::WithParamInterface<RandomLimitInput>
{
protected:
    void SetUp() override;
};

void TestRandomSeedLimit::SetUp()
{
    TestRandomSeed::SetUp();
    set_random_seed(SEED);
    for (int i = 0; i < GetParam().position; ++i)
    {
        static_cast<void>(random15());
    }
}

TEST_P(TestRandomSeedLimit, boundsValueAtSequencePosition)
{
    EXPECT_EQ(GetParam().expected, random_int(GetParam().limit));
}

INSTANTIATE_TEST_SUITE_P(LimitInputs, TestRandomSeedLimit,
    testing::Values(RandomLimitInput{"TenFirstValue", 10, 0, MSVC_RAND15_SEQUENCE[0] % 10},
        RandomLimitInput{"HundredSecondValue", 100, 1, MSVC_RAND15_SEQUENCE[1] % 100},
        RandomLimitInput{"OneAfterTwoValues", 1, 2, 0}, RandomLimitInput{"ZeroAfterTwoValues", 0, 2, 0}),
    [](const testing::TestParamInfo<RandomLimitInput> &info) { return info.param.name; });

TEST_F(TestRandomSeed, randomUnitScalesMsvcSequenceToUnitInterval)
{
    set_random_seed(SEED);

    EXPECT_DOUBLE_EQ(static_cast<double>(MSVC_RAND15_SEQUENCE.front()) / ID_RANDOM_MAX, random_unit());
}

} // namespace id::test
