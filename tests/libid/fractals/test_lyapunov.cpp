// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <fractals/lyapunov.h>

#include <engine/calcfrac.h>
#include <engine/UserData.h>
#include <misc/debug_flags.h>
#include <misc/Driver.h>
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

struct TestLyapunovSetup : TestLyapunov
{
    ValueSaver<Driver *> saved_driver{g_driver, nullptr};
    ValueSaver<long> saved_iterations{g_max_iterations, 25L};
    ValueSaver<ColorMethod> saved_method{g_inside_method, ColorMethod::COLOR};
    ValueSaver<int> saved_color{g_inside_color, 5};
    ValueSaver<CalcMode> saved_user_mode{g_user.std_calc_mode, CalcMode::ONE_PASS};
    ValueSaver<CalcMode> saved_mode{g_std_calc_mode, CalcMode::ONE_PASS};
    ValueSaver<DebugFlags> saved_debug{g_debug_flag, DebugFlags::NONE};
    ValueSaver<double> saved_order{g_params[0], 65.0};
    ValueSaver<double> saved_seed{g_params[1], 0.05};
    ValueSaver<double> saved_filter{g_params[2], 0.0};
};

TEST_F(TestLyapunovSetup, numericInsideNeedsNoWarning)
{
    Lyapunov lyapunov;

    EXPECT_TRUE(lyapunov.resume().empty());
    EXPECT_TRUE(lyapunov_per_image());

    EXPECT_EQ(ColorMethod::COLOR, g_inside_method);
    EXPECT_EQ(5, g_inside_color);
}

TEST_F(TestLyapunovSetup, unsupportedInsideReturnsWarningWithoutUI)
{
    Lyapunov lyapunov;
    for (const ColorMethod method : {ColorMethod::ITER, ColorMethod::BOF60, ColorMethod::EPS_CROSS})
    {
        g_inside_method = method;

        EXPECT_EQ("Sorry, inside options other than inside=nnn are not supported by the lyapunov", lyapunov.resume());
        EXPECT_EQ(method, g_inside_method);
        EXPECT_TRUE(lyapunov_per_image());

        EXPECT_EQ(ColorMethod::COLOR, g_inside_method);
        EXPECT_EQ(1, g_inside_color);
    }
}

TEST_F(TestLyapunovSetup, legacyUnsupportedInsideStillFallsBackToOne)
{
    g_version = parse_legacy_version(1730);
    g_inside_method = ColorMethod::BOF60;
    Lyapunov lyapunov;

    EXPECT_FALSE(lyapunov.resume().empty());
    EXPECT_TRUE(lyapunov_per_image());

    EXPECT_EQ(ColorMethod::COLOR, g_inside_method);
    EXPECT_EQ(1, g_inside_color);
}

TEST_F(TestLyapunovSetup, correctedInsideDoesNotRepeatWarning)
{
    g_inside_method = ColorMethod::BOF60;
    Lyapunov lyapunov;
    ASSERT_FALSE(lyapunov.resume().empty());
    ASSERT_TRUE(lyapunov_per_image());

    EXPECT_TRUE(lyapunov.resume().empty());
}

} // namespace id::test
