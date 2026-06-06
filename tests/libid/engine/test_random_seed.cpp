// SPDX-License-Identifier: GPL-3.0-only
//
#include <engine/random_seed.h>

#include <misc/ValueSaver.h>

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <cstdlib>

using namespace id::engine;
using namespace id::misc;

namespace id::test
{
namespace
{

constexpr int SEED{4567};
constexpr std::size_t SEQUENCE_LENGTH{12};

std::array<int, SEQUENCE_LENGTH> msvc_rand15_sequence(const int seed)
{
    std::array<int, SEQUENCE_LENGTH> result{};
    std::uint32_t state{static_cast<std::uint32_t>(seed)};
    for (std::size_t i{}; i < result.size(); ++i)
    {
        state = state * 214013U + 2531011U;
        result[i] = static_cast<int>((state >> 16U) & 0x7FFFU);
    }
    return result;
}

std::array<int, SEQUENCE_LENGTH> current_c_rand15_sequence(const int seed)
{
    std::array<int, SEQUENCE_LENGTH> result{};
    ValueSaver saved_random_seed{g_random_seed, seed};
    ValueSaver saved_random_seed_flag{g_random_seed_flag, true};

    set_random_seed();
    for (std::size_t i{}; i < result.size(); ++i)
    {
        result[i] = std::rand() & 0x7FFF;
    }
    return result;
}

} // namespace

TEST(TestRandomSeed, cRuntimeRand15SequenceIsMsvcSequenceOnlyOnWindows)
{
    const auto reference{msvc_rand15_sequence(SEED)};
    const auto actual{current_c_rand15_sequence(SEED)};

    // Existing gold images were generated with the MSVC C runtime sequence.
#if defined(_WIN32)
    EXPECT_EQ(reference, actual);
#else
    EXPECT_NE(reference, actual);
#endif
}

} // namespace id::test
