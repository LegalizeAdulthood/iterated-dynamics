// SPDX-License-Identifier: GPL-3.0-only
//
#include <engine/random_seed.h>

#include <misc/ValueSaver.h>

#include <gtest/gtest.h>

#include <array>

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

TEST(TestRandomSeed, random15MatchesMsvcCRuntimeSequence)
{
    ValueSaver saved_random_seed{g_random_seed, SEED};
    ValueSaver saved_random_seed_flag{g_random_seed_flag, true};

    set_random_seed();

    for (const int value : MSVC_RAND15_SEQUENCE)
    {
        EXPECT_EQ(value, random15());
    }
    EXPECT_EQ(SEED, g_random_seed);
}

TEST(TestRandomSeed, nonFixedSeedAdvancesStoredSeedAfterSeeding)
{
    ValueSaver saved_random_seed{g_random_seed, SEED};
    ValueSaver saved_random_seed_flag{g_random_seed_flag, false};

    set_random_seed();

    EXPECT_EQ(SEED + 1, g_random_seed);
    EXPECT_EQ(MSVC_RAND15_SEQUENCE.front(), random15());
}

TEST(TestRandomSeed, explicitSeedDoesNotChangeStoredSeed)
{
    ValueSaver saved_random_seed{g_random_seed, 99};
    ValueSaver saved_random_seed_flag{g_random_seed_flag, true};

    set_random_seed(SEED);

    EXPECT_EQ(99, g_random_seed);
    EXPECT_TRUE(g_random_seed_flag);
    EXPECT_EQ(MSVC_RAND15_SEQUENCE.front(), random15());
}

TEST(TestRandomSeed, randomIntUsesMsvcSequenceModuloLimit)
{
    set_random_seed(SEED);

    EXPECT_EQ(MSVC_RAND15_SEQUENCE[0] % 10, random_int(10));
    EXPECT_EQ(MSVC_RAND15_SEQUENCE[1] % 100, random_int(100));
    EXPECT_EQ(0, random_int(1));
    EXPECT_EQ(0, random_int(0));
}

TEST(TestRandomSeed, randomUnitScalesMsvcSequenceToUnitInterval)
{
    set_random_seed(SEED);

    EXPECT_DOUBLE_EQ(static_cast<double>(MSVC_RAND15_SEQUENCE.front()) / RANDOM_MAX, random_unit());
}

} // namespace id::test
