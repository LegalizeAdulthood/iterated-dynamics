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

struct LegacyLyapunovSequenceInput
{
    const char *name;
    int order;
    std::vector<int> expected;
};

class TestLegacyLyapunovSequence : public TestLyapunov, public testing::WithParamInterface<LegacyLyapunovSequenceInput>
{
protected:
    void SetUp() override;
};

void TestLegacyLyapunovSequence::SetUp()
{
    g_version = parse_legacy_version(1731);
}

TEST_P(TestLegacyLyapunovSequence, retainsLegacySequence)
{
    const LyapunovSequence sequence{build_lyapunov_sequence(GetParam().order)};

    ASSERT_EQ(GetParam().expected.size(), sequence.length);
    for (std::size_t i = 0; i < GetParam().expected.size(); ++i)
    {
        EXPECT_EQ(GetParam().expected[i], sequence.rxy[i]) << i;
    }
}

INSTANTIATE_TEST_SUITE_P(Compatibility, TestLegacyLyapunovSequence,
    testing::Values(LegacyLyapunovSequenceInput{"Inverted", 65, {0, 0, 1, 1, 1, 1, 1, 0, 1}},
        LegacyLyapunovSequenceInput{"MaskedToSixteenBits", 0x10001, {0, 0, 1}}),
    [](const testing::TestParamInfo<LegacyLyapunovSequenceInput> &info) { return info.param.name; });

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

struct LegacyLyapunovColorInput
{
    const char *name;
    int inside;
    int expected;
};

class TestLegacyLyapunovColor : public TestLyapunovSetup, public testing::WithParamInterface<LegacyLyapunovColorInput>
{
protected:
    void SetUp() override;
};

void TestLegacyLyapunovColor::SetUp()
{
    g_version = parse_legacy_version(1730);
    g_user.std_calc_mode = CalcMode::SOLID_GUESS;
    g_std_calc_mode = CalcMode::SOLID_GUESS;
    g_inside_color = GetParam().inside;
}

TEST_P(TestLegacyLyapunovColor, normalizesLegacySetup)
{
    EXPECT_TRUE(lyapunov_per_image());

    EXPECT_EQ(CalcMode::ONE_PASS, g_user.std_calc_mode);
    EXPECT_EQ(CalcMode::ONE_PASS, g_std_calc_mode);
    EXPECT_EQ(ColorMethod::COLOR, g_inside_method);
    EXPECT_EQ(GetParam().expected, g_inside_color);
}

INSTANTIATE_TEST_SUITE_P(Compatibility, TestLegacyLyapunovColor,
    testing::Values(LegacyLyapunovColorInput{"DefaultInside", 1, 0}, LegacyLyapunovColorInput{"ExplicitInside", 2, 2}),
    [](const testing::TestParamInfo<LegacyLyapunovColorInput> &info) { return info.param.name; });

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
    for (const ColorMethod method : {ColorMethod::ITER, ColorMethod::BOF60, ColorMethod::EPS_CROSS})
    {
        Lyapunov lyapunov;
        g_inside_method = method;

        EXPECT_EQ(
            (std::vector<std::string>{"Sorry, inside options other than inside=nnn are not supported by the lyapunov"}),
            lyapunov.resume());
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
    lyapunov.finish();

    EXPECT_TRUE(lyapunov.resume().empty());
}

} // namespace id::test
