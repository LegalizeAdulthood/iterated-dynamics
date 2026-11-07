// SPDX-License-Identifier: GPL-3.0-only
//
#include <ui/stereo.h>

#include <engine/random_seed.h>
#include <engine/VideoInfo.h>
#include <misc/ValueSaver.h>

#include <gtest/gtest.h>

#include <array>

using namespace id::engine;
using namespace id::misc;
using namespace id::ui;

namespace id::test
{

TEST(TestStereo, randomLineRepeatsWithFixedRandomSeed)
{
    constexpr int LINE_LEN{32};
    constexpr int SEED{363};
    std::array<Byte, LINE_LEN> first{};
    std::array<Byte, LINE_LEN> second{};
    ValueSaver saved_colors{g_colors, 16};
    ValueSaver saved_random_seed{g_random_seed, SEED};
    ValueSaver saved_random_seed_flag{g_random_seed_flag, true};

    set_random_seed();
    random_dot_line(first.data(), LINE_LEN);
    set_random_seed();
    random_dot_line(second.data(), LINE_LEN);

    EXPECT_EQ(first, second);
    EXPECT_EQ(SEED, g_random_seed);
}

} // namespace id::test
