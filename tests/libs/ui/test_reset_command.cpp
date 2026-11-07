// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <ui/CommandRouter.h>

#include <engine/calcfrac.h>
#include <engine/CommandProcessor.h>
#include <engine/OrbitPlot.h>
#include <engine/sound.h>
#include <engine/UserData.h>
#include <io/loadfile.h>
#include <misc/ValueSaver.h>
#include <misc/version.h>
#include <ui/big_while_loop.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <optional>
#include <ostream>
#include <string>

using namespace id::engine;
using namespace id::io;
using namespace id::misc;
using namespace id::ui;
using namespace testing;

namespace id::test
{

class TestResetCommand : public Test
{
protected:
    void exec_cmd_arg(const std::string &arg);

    ValueSaver<Version> m_saved_version{g_version, Version{1, 2, 3, 4, false}};
    ValueSaver<Version> m_saved_file_version{g_file_version, Version{5, 6, 7, 8, true}};
    ValueSaver<long> m_saved_iterations{g_max_iterations, 777L};
    ValueSaver<int> m_saved_delay{g_orbit_delay, 99};
    ValueSaver<int> m_saved_volume{g_fm_volume, 17};
    ValueSaver<bool> m_saved_escape_exit{g_escape_exit, true};
    ValueSaver<bool> m_saved_first_init{g_first_init, false};
    CmdArgFlags m_result{};
    CommandParseResult m_parsed;
};

void TestResetCommand::exec_cmd_arg(const std::string &arg)
{
    CommandProcessor processor{arg, CmdFile::AT_AFTER_STARTUP};
    const CommandRouter router;
    m_parsed = router.process(processor);
    m_result = m_parsed.flags;
}

class TestResetCommandError : public TestResetCommand
{
protected:
    void TearDown() override;
};

void TestResetCommandError::TearDown()
{
    EXPECT_EQ(1U, m_parsed.diagnostics.errors.size());
    EXPECT_TRUE(m_parsed.diagnostics.warnings.empty());
}

struct RejectedResetInput
{
    const char *name;
    const char *argument;
    Version initial_version;
    Version initial_file_version;
};

void PrintTo(const RejectedResetInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestResetCommandRejected : public TestResetCommandError, public WithParamInterface<RejectedResetInput>
{
protected:
    void SetUp() override;
};

void TestResetCommandRejected::SetUp()
{
    TestResetCommandError::SetUp();
    g_version = GetParam().initial_version;
    g_file_version = GetParam().initial_file_version;
}

TEST_P(TestResetCommandRejected, rejectsOnceWithoutChangingSettings)
{
    exec_cmd_arg(GetParam().argument);

    EXPECT_EQ(CmdArgFlags::BAD_ARG, m_result);
    EXPECT_EQ(GetParam().initial_version, g_version);
    EXPECT_EQ(GetParam().initial_file_version, g_file_version);
    EXPECT_EQ(777L, g_max_iterations);
    EXPECT_EQ(99, g_orbit_delay);
    EXPECT_EQ(17, g_fm_volume);
    EXPECT_TRUE(g_escape_exit);
    EXPECT_THAT(m_parsed.diagnostics.errors,
        ElementsAre(std::string{"Oops. I couldn't understand the argument:\n  "} + GetParam().argument));
}

INSTANTIATE_TEST_SUITE_P(Argument, TestResetCommandRejected,
    Values(RejectedResetInput{"Word", "reset=foo", {1, 2, 3, 4, false}, {5, 6, 7, 8, false}},
        RejectedResetInput{"Boolean", "reset=y", {1, 2, 3, 4, false}, {5, 6, 7, 8, false}},
        RejectedResetInput{"Garbage", "reset=garbage", {1, 2, 3, 4, false}, {5, 6, 7, 8, true}}),
    [](const TestParamInfo<RejectedResetInput> &info) { return info.param.name; });

struct ResetVersionInput
{
    const char *name;
    const char *argument;
    Version initial_version;
    Version initial_file_version;
    Version expected;
    std::optional<bool> legacy_float;
};

void PrintTo(const ResetVersionInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestResetCommandVersions : public TestResetCommand, public WithParamInterface<ResetVersionInput>
{
protected:
    void SetUp() override;

    ValueSaver<bool> m_saved_legacy_float{g_user.legacy_float};
};

void TestResetCommandVersions::SetUp()
{
    TestResetCommand::SetUp();
    g_version = GetParam().initial_version;
    g_file_version = GetParam().initial_file_version;
    if (GetParam().legacy_float.has_value())
    {
        g_user.legacy_float = *GetParam().legacy_float;
    }
}

TEST_P(TestResetCommandVersions, appliesDecodedVersion)
{
    exec_cmd_arg(GetParam().argument);

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::RESET, m_result);
    EXPECT_EQ(GetParam().expected, g_version);
    EXPECT_EQ(GetParam().expected, g_file_version);
    if (GetParam().legacy_float.has_value())
    {
        EXPECT_FALSE(g_user.legacy_float);
    }
}

INSTANTIATE_TEST_SUITE_P(Argument, TestResetCommandVersions,
    Values(ResetVersionInput{"NoArgument", "reset", {1, 2, 0, 0, false}, {9, 9, 9, 9, true}, parse_legacy_version(1730),
               true},
        ResetVersionInput{
            "Zero", "reset=0", {1, 2, 0, 0, false}, {9, 9, 9, 9, true}, parse_legacy_version(1730), std::nullopt},
        ResetVersionInput{"Version10", "reset=100", {2222, 3333, 4444, 5555, true}, {9, 9, 9, 9, true},
            {1, 0, 0, 0, false}, std::nullopt},
        ResetVersionInput{"Version11", "reset=101", {2222, 3333, 4444, 5555, true}, {9, 9, 9, 9, true},
            {1, 1, 0, 0, false}, std::nullopt},
        ResetVersionInput{"Version12", "reset=1/2", {2222, 3333, 4444, 5555, true}, {9, 9, 9, 9, true},
            {1, 2, 0, 0, false}, std::nullopt},
        ResetVersionInput{"Legacy", "reset=1906", {2222, 3333, 4444, 5555, false}, {9, 9, 9, 9, false},
            parse_legacy_version(1906), std::nullopt},
        ResetVersionInput{
            "AllFields", "reset=1/2/3/4", {1, 2, 3, 4, false}, {9, 9, 9, 9, true}, {1, 2, 3, 4, false}, std::nullopt},
        ResetVersionInput{"MajorMinorPatch", "reset=1/3/2", {1, 2, 3, 4, false}, {9, 9, 9, 9, true},
            {1, 3, 2, 0, false}, std::nullopt}),
    [](const TestParamInfo<ResetVersionInput> &info) { return info.param.name; });

struct ResetInsideInput
{
    const char *name;
    const char *argument;
    int expected;
};

void PrintTo(const ResetInsideInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestResetCommandInsideColors : public TestResetCommand, public WithParamInterface<ResetInsideInput>
{
protected:
    ValueSaver<int> m_saved_inside_color{g_inside_color, -9999};
};

TEST_P(TestResetCommandInsideColors, restoresInsideColor)
{
    exec_cmd_arg(GetParam().argument);

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::RESET, m_result);
    EXPECT_EQ(GetParam().expected, g_inside_color);
}

INSTANTIATE_TEST_SUITE_P(Argument, TestResetCommandInsideColors,
    Values(ResetInsideInput{"Default", "reset", 1}, ResetInsideInput{"Legacy", "reset=1960", 1},
        ResetInsideInput{"Modern", "reset=100", 1}),
    [](const TestParamInfo<ResetInsideInput> &info) { return info.param.name; });

TEST_F(TestResetCommand, acceptedCommandInitializesAndAppliesDecodedVersion)
{
    exec_cmd_arg("reset=100");

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::RESET, m_result);
    EXPECT_EQ((Version{1, 0, 0, 0, false}), g_version);
    EXPECT_EQ(g_version, g_file_version);
    EXPECT_EQ(INITIAL_MAX_ITERATIONS, g_max_iterations);
    EXPECT_EQ(0, g_orbit_delay);
    EXPECT_EQ(63, g_fm_volume);
    EXPECT_FALSE(g_escape_exit);
    EXPECT_TRUE(m_parsed.diagnostics.errors.empty());
}

} // namespace id::test
