// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "ui/CommandRouter.h"

#include "engine/CommandProcessor.h"

#include <fmt/format.h>

#include <optional>
#include <set>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

using namespace id::engine;

namespace id::ui
{

// TODO: removed later
static const CommandHandlerTables s_ui_commands{};

static void add_names(std::set<std::string_view> &names, const std::vector<CommandHandler> &handlers)
{
    for (const CommandHandler &handler : handlers)
    {
        if (!names.insert(handler.name).second)
        {
            throw std::logic_error(fmt::format("Duplicate parameter owner: {}", handler.name));
        }
    }
}

CommandRouter::CommandRouter() :
    CommandRouter(s_ui_commands, engine_command_handlers())
{
}

CommandRouter::CommandRouter(CommandHandlerTables ui, CommandHandlerTables engine) :
    m_ui(std::move(ui)),
    m_engine(std::move(engine))
{
    std::set<std::string_view> names;
    add_names(names, m_ui.startup);
    add_names(names, m_ui.unrestricted);
    add_names(names, m_engine.startup);
    add_names(names, m_engine.unrestricted);
}

CommandParseResult CommandRouter::process(CommandProcessor &processor) const
{
    while (processor.advance())
    {
        const Command &cmd{processor.command()};
        CmdArgFlags flags{cmd.status};
        if (flags == CmdArgFlags::NONE)
        {
            std::optional<CmdArgFlags> handled{handle_command(m_ui, cmd)};
            if (!handled.has_value())
            {
                handled = handle_command(m_engine, cmd);
            }
            flags = handled.has_value() ? *handled : cmd.bad_arg();
        }
        cmd.result.flags = flags;
    }
    return processor.result();
}

} // namespace id::ui
