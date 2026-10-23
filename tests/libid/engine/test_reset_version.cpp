// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/reset_version.h>

#include <engine/calcfrac.h>
#include <engine/Command.h>
#include <engine/OrbitPlot.h>
#include <engine/sound.h>
#include <io/loadfile.h>
#include <misc/ValueSaver.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <optional>
#include <ostream>
#include <string>

using namespace id::engine;
using namespace id::io;
using namespace id::misc;
using namespace testing;

namespace id::test
{

class TestResetVersion : public Test
{
protected:
    void expect_render_settings_unchanged() const;

    static constexpr Version ACTIVE_VERSION{1, 2, 3, 4, false};
    static constexpr Version FILE_VERSION{5, 6, 7, 8, true};
    ValueSaver<Version> m_saved_version{g_version, ACTIVE_VERSION};
    ValueSaver<Version> m_saved_file_version{g_file_version, FILE_VERSION};
    ValueSaver<long> m_saved_iterations{g_max_iterations, 777L};
    ValueSaver<int> m_saved_delay{g_orbit_delay, 99};
    ValueSaver<int> m_saved_volume{g_fm_volume, 17};
    ValueSaver<bool> m_saved_escape_exit{g_escape_exit, true};
    ValueSaver<bool> m_saved_first_init{g_first_init, false};
    CommandParseResult m_result;
};

void TestResetVersion::expect_render_settings_unchanged() const
{
    EXPECT_EQ(777L, g_max_iterations);
    EXPECT_EQ(99, g_orbit_delay);
    EXPECT_EQ(17, g_fm_volume);
    EXPECT_TRUE(g_escape_exit);
}

struct ResetVersionCase
{
    const char *name;
    const char *argument;
    std::optional<Version> expected;
};

void PrintTo(const ResetVersionCase &input, std::ostream *str)
{
    *str << input.name;
}

class TestParseResetVersion : public TestResetVersion, public WithParamInterface<ResetVersionCase>
{
};

TEST_P(TestParseResetVersion, decodesWithoutChangingSettingsOrDiagnostics)
{
    const ResetVersionCase &input{GetParam()};
    const Command cmd{input.argument, CmdFile::AT_AFTER_STARTUP, m_result};
    ASSERT_EQ(CmdArgFlags::NONE, cmd.status);

    EXPECT_EQ(input.expected, parse_reset_version(cmd));

    EXPECT_EQ(ACTIVE_VERSION, g_version);
    EXPECT_EQ(FILE_VERSION, g_file_version);
    expect_render_settings_unchanged();
    EXPECT_EQ(CmdArgFlags::NONE, m_result.flags);
    EXPECT_TRUE(m_result.diagnostics.errors.empty());
    EXPECT_TRUE(m_result.diagnostics.warnings.empty());
}

INSTANTIATE_TEST_SUITE_P(ResetValues, TestParseResetVersion,
    Values(ResetVersionCase{"Omitted", "reset", Version{17, 30, 0, 0, true}},
        ResetVersionCase{"Empty", "reset=", Version{17, 30, 0, 0, true}},
        ResetVersionCase{"Zero", "reset=0", Version{17, 30, 0, 0, true}},
        ResetVersionCase{"Legacy", "reset=1960", Version{19, 60, 0, 0, true}},
        ResetVersionCase{"Id100", "reset=100", Version{1, 0, 0, 0, false}},
        ResetVersionCase{"Id101", "reset=101", Version{1, 1, 0, 0, false}},
        ResetVersionCase{"Legacy102", "reset=102", Version{1, 2, 0, 0, true}},
        ResetVersionCase{"Major", "reset=2", Version{2, 0, 0, 0, false}},
        ResetVersionCase{"Minor", "reset=1/5", Version{1, 5, 0, 0, false}},
        ResetVersionCase{"ZeroMajor", "reset=0/1", Version{0, 1, 0, 0, false}},
        ResetVersionCase{"Patch", "reset=1/5/3", Version{1, 5, 3, 0, false}},
        ResetVersionCase{"Tweak", "reset=1/5/3/9", Version{1, 5, 3, 9, false}},
        ResetVersionCase{"ExtraFields", "reset=1/2/3/4/99", Version{1, 2, 3, 4, false}},
        ResetVersionCase{"NonnumericLaterField", "reset=1/garbage/2", Version{1, 0, 0, 0, false}},
        ResetVersionCase{"NegativeLaterField", "reset=1/-2", Version{1, -2, 0, 0, false}},
        ResetVersionCase{"NegativeFirstField", "reset=-1", std::nullopt},
        ResetVersionCase{"NonnumericFirstField", "reset=foo", std::nullopt},
        ResetVersionCase{"FloatingFirstField", "reset=1.5", std::nullopt}),
    [](const TestParamInfo<ResetVersionCase> &info) { return std::string{info.param.name}; });

TEST_F(TestResetVersion, syntaxFailureDoesNotRepeatDiagnosticOrChangeSettings)
{
    const Command cmd{std::string(33, 'a') + "=1", CmdFile::AT_AFTER_STARTUP, m_result};
    ASSERT_EQ(CmdArgFlags::BAD_ARG, cmd.status);
    ASSERT_EQ(1U, m_result.diagnostics.errors.size());

    EXPECT_FALSE(parse_reset_version(cmd).has_value());

    EXPECT_EQ(ACTIVE_VERSION, g_version);
    EXPECT_EQ(FILE_VERSION, g_file_version);
    expect_render_settings_unchanged();
    EXPECT_EQ(1U, m_result.diagnostics.errors.size());
}

TEST_F(TestResetVersion, applyingVersionDoesNotResetSettings)
{
    const Version version{2, 6, 7, 8, false};

    apply_reset_version(version);

    EXPECT_EQ(version, g_version);
    EXPECT_EQ(version, g_file_version);
    expect_render_settings_unchanged();
}

TEST_F(TestResetVersion, rejectedCommandKeepsSettingsAndDiagnosesOnce)
{
    const CommandParseResult result{cmd_arg("reset=garbage", CmdFile::AT_AFTER_STARTUP)};

    EXPECT_EQ(CmdArgFlags::BAD_ARG, result.flags);
    EXPECT_EQ(ACTIVE_VERSION, g_version);
    EXPECT_EQ(FILE_VERSION, g_file_version);
    expect_render_settings_unchanged();
    EXPECT_THAT(result.diagnostics.errors, ElementsAre("Oops. I couldn't understand the argument:\n  reset=garbage"));
}

TEST_F(TestResetVersion, acceptedCommandInitializesAndAppliesDecodedVersion)
{
    const CommandParseResult result{cmd_arg("reset=100", CmdFile::AT_AFTER_STARTUP)};

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::RESET, result.flags);
    EXPECT_EQ((Version{1, 0, 0, 0, false}), g_version);
    EXPECT_EQ(g_version, g_file_version);
    EXPECT_EQ(INITIAL_MAX_ITERATIONS, g_max_iterations);
    EXPECT_EQ(0, g_orbit_delay);
    EXPECT_EQ(63, g_fm_volume);
    EXPECT_FALSE(g_escape_exit);
    EXPECT_TRUE(result.diagnostics.errors.empty());
}

} // namespace id::test
