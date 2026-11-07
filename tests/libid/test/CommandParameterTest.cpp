// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <CommandParameterTest.h>

#include <test_data.h>

#include <engine/Command.h>
#include <engine/CommandProcessor.h>
#include <io/library.h>

#include <cctype>
#include <filesystem>
#include <optional>

using namespace id::engine;
using namespace id::io;
using namespace id::test::data;

namespace id::test
{

std::string adjust_dir(std::string actual)
{
    if (actual[1] == ':')
    {
        actual[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(actual[0])));
    }
    std::filesystem::path sep;
    sep += std::filesystem::path::preferred_separator;
    if (actual.back() == sep.string().back())
    {
        actual.pop_back();
    }
    return actual;
}

CommandParseResult process_engine_commands(CommandProcessor &processor)
{
    while (processor.advance())
    {
        const Command &cmd{processor.command()};
        CmdArgFlags flags{cmd.status};
        if (flags == CmdArgFlags::NONE)
        {
            const std::optional<CmdArgFlags> handled{handle_engine_command(cmd)};
            flags = handled.has_value() ? *handled : cmd.bad_arg();
        }
        cmd.result.flags = flags;
    }
    return processor.result();
}

void TestParameterCommand::SetUp()
{
    testing::Test::SetUp();
    clear_read_library_path();
    add_read_library(ID_TEST_HOME_DIR);
}

void TestParameterCommand::TearDown()
{
    clear_read_library_path();
    testing::Test::TearDown();
}

void TestParameterCommand::exec_cmd_arg(const std::string &cur_arg, const CmdFile mode)
{
    CommandProcessor processor{cur_arg, mode};
    m_parsed = process_engine_commands(processor);
    m_result = m_parsed.flags;
}

void TestParameterCommandFile::SetUp()
{
    TestParameterCommand::SetUp();
    m_file.reset(std::tmpfile());
    ASSERT_NE(nullptr, m_file);
}

CommandParseResult TestParameterCommandFile::process_commands(const char *commands)
{
    std::fputs(commands, m_file.get());
    std::rewind(m_file.get());
    CommandProcessor processor{m_file.release(), CmdFile::AT_AFTER_STARTUP};
    return process_engine_commands(processor);
}

} // namespace id::test
