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

TEST_F(TestResetCommandError, resetBadArg)
{
    ValueSaver saved_version{g_version, Version{1, 2, 3, 4, false}};
    ValueSaver saved_file_version{g_file_version, Version{5, 6, 7, 8, false}};

    exec_cmd_arg("reset=foo");

    EXPECT_EQ(CmdArgFlags::BAD_ARG, m_result);
    EXPECT_EQ((Version{1, 2, 3, 4, false}), g_version);
    EXPECT_EQ((Version{5, 6, 7, 8, false}), g_file_version);
}

TEST_F(TestResetCommand, resetNoArg)
{
    ValueSaver saved_version{g_version, Version{1, 2, 0, 0, false}};
    ValueSaver saved_file_version{g_file_version, Version{9, 9, 9, 9, true}};
    ValueSaver saved_legacy_float{g_user.legacy_float, true};

    exec_cmd_arg("reset");

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::RESET, m_result);
    EXPECT_EQ(17, g_version.major);
    EXPECT_EQ(30, g_version.minor);
    EXPECT_EQ(0, g_version.patch);
    EXPECT_EQ(0, g_version.tweak);
    EXPECT_TRUE(g_version.legacy);
    EXPECT_EQ(17, g_file_version.major);
    EXPECT_EQ(30, g_file_version.minor);
    EXPECT_EQ(0, g_file_version.patch);
    EXPECT_EQ(0, g_file_version.tweak);
    EXPECT_TRUE(g_file_version.legacy);
    EXPECT_FALSE(g_user.legacy_float);
}

TEST_F(TestResetCommand, resetZero)
{
    ValueSaver saved_version{g_version, Version{1, 2, 0, 0, false}};
    ValueSaver saved_file_version{g_file_version, Version{9, 9, 9, 9, true}};

    exec_cmd_arg("reset=0");

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::RESET, m_result);
    EXPECT_EQ(parse_legacy_version(1730), g_version);
    EXPECT_EQ(parse_legacy_version(1730), g_file_version);
}

TEST_F(TestResetCommand, resetVersion10)
{
    ValueSaver saved_version{g_version, Version{2222, 3333, 4444, 5555, true}};
    ValueSaver saved_file_version{g_file_version, Version{9, 9, 9, 9, true}};

    exec_cmd_arg("reset=100");

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::RESET, m_result);
    EXPECT_EQ(1, g_version.major);
    EXPECT_EQ(0, g_version.minor);
    EXPECT_EQ(0, g_version.patch);
    EXPECT_EQ(0, g_version.tweak);
    EXPECT_FALSE(g_version.legacy);
    EXPECT_EQ((Version{1, 0, 0, 0, false}), g_file_version);
}

TEST_F(TestResetCommand, resetVersion11)
{
    ValueSaver saved_version{g_version, Version{2222, 3333, 4444, 5555, true}};
    ValueSaver saved_file_version{g_file_version, Version{9, 9, 9, 9, true}};

    exec_cmd_arg("reset=101");

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::RESET, m_result);
    EXPECT_EQ(1, g_version.major);
    EXPECT_EQ(1, g_version.minor);
    EXPECT_EQ(0, g_version.patch);
    EXPECT_EQ(0, g_version.tweak);
    EXPECT_FALSE(g_version.legacy);
    EXPECT_EQ((Version{1, 1, 0, 0, false}), g_file_version);
}

TEST_F(TestResetCommand, resetVersion12)
{
    ValueSaver saved_version{g_version, Version{2222, 3333, 4444, 5555, true}};
    ValueSaver saved_file_version{g_file_version, Version{9, 9, 9, 9, true}};

    exec_cmd_arg("reset=1/2");

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::RESET, m_result);
    EXPECT_EQ(1, g_version.major);
    EXPECT_EQ(2, g_version.minor);
    EXPECT_EQ(0, g_version.patch);
    EXPECT_EQ(0, g_version.tweak);
    EXPECT_FALSE(g_version.legacy);
    EXPECT_EQ((Version{1, 2, 0, 0, false}), g_file_version);
}

TEST_F(TestResetCommand, resetLegacyVersion)
{
    ValueSaver saved_version{g_version, Version{2222, 3333, 4444, 5555, false}};
    ValueSaver saved_file_version{g_file_version, Version{9, 9, 9, 9, false}};

    exec_cmd_arg("reset=1906");

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::RESET, m_result);
    EXPECT_EQ(19, g_version.major);
    EXPECT_EQ(6, g_version.minor);
    EXPECT_EQ(0, g_version.patch);
    EXPECT_EQ(0, g_version.tweak);
    EXPECT_TRUE(g_version.legacy);
    EXPECT_EQ(19, g_file_version.major);
    EXPECT_EQ(6, g_file_version.minor);
    EXPECT_EQ(0, g_file_version.patch);
    EXPECT_EQ(0, g_file_version.tweak);
    EXPECT_TRUE(g_file_version.legacy);
}

TEST_F(TestResetCommand, resetVersionAllFields)
{
    ValueSaver saved_file_version{g_file_version, Version{9, 9, 9, 9, true}};

    exec_cmd_arg("reset=1/2/3/4");

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::RESET, m_result);
    EXPECT_EQ(1, g_version.major);
    EXPECT_EQ(2, g_version.minor);
    EXPECT_EQ(3, g_version.patch);
    EXPECT_EQ(4, g_version.tweak);
    EXPECT_FALSE(g_version.legacy);
    EXPECT_EQ(1, g_file_version.major);
    EXPECT_EQ(2, g_file_version.minor);
    EXPECT_EQ(3, g_file_version.patch);
    EXPECT_EQ(4, g_file_version.tweak);
    EXPECT_FALSE(g_file_version.legacy);
}

TEST_F(TestResetCommand, resetVersionMajorMinorPatch)
{
    ValueSaver saved_file_version{g_file_version, Version{9, 9, 9, 9, true}};

    exec_cmd_arg("reset=1/3/2");

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::RESET, m_result);
    EXPECT_EQ((Version{1, 3, 2, 0, false}), g_version);
    EXPECT_EQ((Version{1, 3, 2, 0, false}), g_file_version);
}

TEST_F(TestResetCommandError, resetYesBadArg)
{
    ValueSaver saved_version{g_version, Version{1, 2, 3, 4, false}};
    ValueSaver saved_file_version{g_file_version, Version{5, 6, 7, 8, false}};

    exec_cmd_arg("reset=y");

    EXPECT_EQ(CmdArgFlags::BAD_ARG, m_result);
    EXPECT_EQ((Version{1, 2, 3, 4, false}), g_version);
    EXPECT_EQ((Version{5, 6, 7, 8, false}), g_file_version);
}

TEST_F(TestResetCommand, resetInsideOne)
{
    ValueSaver saved_inside_color{g_inside_color, -9999};

    exec_cmd_arg("reset");

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::RESET, m_result);
    EXPECT_EQ(1, g_inside_color);
}

TEST_F(TestResetCommand, resetLegacyInsideOne)
{
    ValueSaver saved_inside_color{g_inside_color, -9999};

    exec_cmd_arg("reset=1960");

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::RESET, m_result);
    EXPECT_EQ(1, g_inside_color);
}

TEST_F(TestResetCommand, resetIdInsideOne)
{
    ValueSaver saved_inside_color{g_inside_color, -9999};

    exec_cmd_arg("reset=100");

    EXPECT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::RESET, m_result);
    EXPECT_EQ(1, g_inside_color);
}

TEST_F(TestResetCommand, rejectedCommandKeepsSettingsAndDiagnosesOnce)
{
    exec_cmd_arg("reset=garbage");

    EXPECT_EQ(CmdArgFlags::BAD_ARG, m_result);
    EXPECT_EQ((Version{1, 2, 3, 4, false}), g_version);
    EXPECT_EQ((Version{5, 6, 7, 8, true}), g_file_version);
    EXPECT_EQ(777L, g_max_iterations);
    EXPECT_EQ(99, g_orbit_delay);
    EXPECT_EQ(17, g_fm_volume);
    EXPECT_TRUE(g_escape_exit);
    EXPECT_THAT(m_parsed.diagnostics.errors, ElementsAre("Oops. I couldn't understand the argument:\n  reset=garbage"));
}

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
