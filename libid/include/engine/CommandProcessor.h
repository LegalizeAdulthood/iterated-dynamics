// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include "engine/cmdfiles.h"

#include <cstddef>
#include <cstdio>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace id::engine
{

class CommandProcessor
{
public:
    // Read startup configuration only, without defaults or entry selection.
    CommandProcessor();

    // Adopt an open file at its existing source position.
    CommandProcessor(std::FILE *file, CmdFile mode);

    // Process startup sources without initializing their settings.
    explicit CommandProcessor(std::vector<std::string> args);

    // Interpret one parameter without initialization or file completion.
    CommandProcessor(std::string_view text, CmdFile mode);

    ~CommandProcessor();
    CommandProcessor(const CommandProcessor &) = delete;
    CommandProcessor(CommandProcessor &&) = delete;
    CommandProcessor &operator=(const CommandProcessor &) = delete;
    CommandProcessor &operator=(CommandProcessor &&) = delete;

    // Collect the previous outcome and expose the next command, if any.
    bool advance();

    // The command remains valid until the next advance.
    const Command &command() const;

    const CommandParseResult &result() const
    {
        return m_result;
    }

private:
    using FilePtr = std::unique_ptr<std::FILE, decltype(&std::fclose)>;

    enum class Source
    {
        FILE,
        SINGLE_COMMAND,
        CONFIGURATION,
        ARGUMENTS,
        DONE
    };

    bool advance_single();
    bool advance_startup();
    void finish_file();

    FilePtr m_file{nullptr, std::fclose};
    CmdFile m_mode{CmdFile::AT_CMD_LINE};
    std::string m_text;
    std::string m_line_buf;
    std::string_view m_line;
    CommandParseResult m_result;
    CommandParseResult m_command_result;
    std::unique_ptr<Command> m_command;
    Source m_source{Source::FILE};
    std::vector<std::string> m_args;
    std::size_t m_arg_index{1};
    std::unique_ptr<CommandProcessor> m_source_file;
};

} // namespace id::engine
