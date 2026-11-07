// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include <engine/calcfrac.h>
#include <engine/cmdfiles.h>
#include <misc/Driver.h>
#include <misc/ValueSaver.h>

#include <gtest/gtest.h>

#include <cstdio>
#include <memory>
#include <string>

namespace id::engine
{
class CommandProcessor;
}

namespace id::test
{

engine::CommandParseResult process_engine_commands(engine::CommandProcessor &processor);
std::string adjust_dir(std::string actual);

class TestParameterCommand : public testing::Test
{
public:
    ~TestParameterCommand() override = default;

protected:
    void SetUp() override;
    void TearDown() override;
    void exec_cmd_arg(const std::string &cur_arg, engine::CmdFile mode = engine::CmdFile::AT_AFTER_STARTUP);

    engine::CmdArgFlags m_result{};
    engine::CommandParseResult m_parsed;
};

class TestParameterCommandFile : public TestParameterCommand
{
protected:
    void SetUp() override;
    engine::CommandParseResult process_commands(const char *commands);

private:
    misc::ValueSaver<misc::Driver *> m_saved_driver{misc::g_driver, nullptr};
    misc::ValueSaver<long> m_saved_iterations{engine::g_max_iterations, 150L};
    std::unique_ptr<std::FILE, decltype(&std::fclose)> m_file{nullptr, std::fclose};
};

} // namespace id::test
