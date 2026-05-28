// SPDX-License-Identifier: GPL-3.0-only
//
#include <fractals/lyapunov.h>

#include <engine/calcfrac.h>
#include <engine/UserData.h>
#include <misc/debug_flags.h>
#include <misc/ValueSaver.h>
#include <misc/version.h>

#include <gtest/gtest.h>

using namespace id::engine;
using namespace id::fractals;
using namespace id::misc;

namespace id::test
{

struct TestLyapunov : testing::Test
{
    ValueSaver<Version> saved_version{g_version, parse_legacy_version(2000)};
};

TEST_F(TestLyapunov, legacy1731InvertsLyapunovSequence)
{
    g_version = parse_legacy_version(1731);

    const LyapunovSequence sequence{build_lyapunov_sequence(65)};

    EXPECT_EQ(9, sequence.length);
    EXPECT_EQ(0, sequence.rxy[0]);
    EXPECT_EQ(0, sequence.rxy[1]);
    EXPECT_EQ(1, sequence.rxy[2]);
    EXPECT_EQ(1, sequence.rxy[3]);
    EXPECT_EQ(1, sequence.rxy[4]);
    EXPECT_EQ(1, sequence.rxy[5]);
    EXPECT_EQ(1, sequence.rxy[6]);
    EXPECT_EQ(0, sequence.rxy[7]);
    EXPECT_EQ(1, sequence.rxy[8]);
}

TEST_F(TestLyapunov, legacy1731MasksOrderToSixteenBits)
{
    g_version = parse_legacy_version(1731);

    const LyapunovSequence sequence{build_lyapunov_sequence(0x10001)};

    EXPECT_EQ(3, sequence.length);
    EXPECT_EQ(0, sequence.rxy[0]);
    EXPECT_EQ(0, sequence.rxy[1]);
    EXPECT_EQ(1, sequence.rxy[2]);
}

TEST_F(TestLyapunov, legacy1730ForcesOnePassAndDefaultInsideZero)
{
    ValueSaver saved_max_iterations{g_max_iterations, 25L};
    ValueSaver saved_user_calc_mode{g_user.std_calc_mode, CalcMode::SOLID_GUESS};
    ValueSaver saved_calc_mode{g_std_calc_mode, CalcMode::SOLID_GUESS};
    ValueSaver saved_inside_method{g_inside_method, ColorMethod::COLOR};
    ValueSaver saved_inside_color{g_inside_color, 1};
    ValueSaver saved_debug_flag{g_debug_flag, DebugFlags::NONE};
    ValueSaver saved_param_0{g_params[0], 65.0};
    ValueSaver saved_param_1{g_params[1], 0.05};
    ValueSaver saved_param_2{g_params[2], 0.0};
    g_version = parse_legacy_version(1730);

    EXPECT_TRUE(lyapunov_per_image());

    EXPECT_EQ(CalcMode::ONE_PASS, g_user.std_calc_mode);
    EXPECT_EQ(CalcMode::ONE_PASS, g_std_calc_mode);
    EXPECT_EQ(ColorMethod::COLOR, g_inside_method);
    EXPECT_EQ(0, g_inside_color);
}

TEST_F(TestLyapunov, legacy1730KeepsExplicitInsideColor)
{
    ValueSaver saved_max_iterations{g_max_iterations, 25L};
    ValueSaver saved_user_calc_mode{g_user.std_calc_mode, CalcMode::SOLID_GUESS};
    ValueSaver saved_calc_mode{g_std_calc_mode, CalcMode::SOLID_GUESS};
    ValueSaver saved_inside_method{g_inside_method, ColorMethod::COLOR};
    ValueSaver saved_inside_color{g_inside_color, 2};
    ValueSaver saved_debug_flag{g_debug_flag, DebugFlags::NONE};
    ValueSaver saved_param_0{g_params[0], 65.0};
    ValueSaver saved_param_1{g_params[1], 0.05};
    ValueSaver saved_param_2{g_params[2], 0.0};
    g_version = parse_legacy_version(1730);

    EXPECT_TRUE(lyapunov_per_image());

    EXPECT_EQ(2, g_inside_color);
}

} // namespace id::test
