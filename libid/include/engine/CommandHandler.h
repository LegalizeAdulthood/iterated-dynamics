// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include "engine/Command.h"
#include "misc/debug_flags.h"

#include <algorithm>
#include <functional>
#include <optional>
#include <string_view>
#include <vector>

namespace id::engine
{

struct CommandHandler
{
    std::string_view name;
    std::function<CmdArgFlags(const Command &)> handler;
};

struct CommandHandlerTables
{
    std::vector<CommandHandler> startup;
    std::vector<CommandHandler> unrestricted;
};

// Tables must be sorted by name. Only an unrecognized name permits delegation.
template <typename Container>
std::optional<CmdArgFlags> handle_command(
    const Container &handlers, const Command &cmd, const bool startup_only = false)
{
    if (cmd.status != CmdArgFlags::NONE)
    {
        return cmd.status;
    }
    const typename Container::const_iterator it{std::lower_bound(handlers.begin(), handlers.end(), cmd.variable,
        [](const CommandHandler &handler, const std::string_view name) { return handler.name < name; })};
    if (it == handlers.end() || it->name != cmd.variable)
    {
        return std::nullopt;
    }
    if (startup_only && cmd.mode == CmdFile::AT_AFTER_STARTUP &&
        misc::g_debug_flag != misc::DebugFlags::ALLOW_INIT_COMMANDS_ANYTIME)
    {
        return cmd.bad_arg();
    }
    return it->handler(cmd);
}

std::optional<CmdArgFlags> handle_command(const CommandHandlerTables &tables, const Command &cmd);
CommandHandlerTables engine_command_handlers();

} // namespace id::engine
