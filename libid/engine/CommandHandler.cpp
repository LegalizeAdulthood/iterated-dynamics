// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "engine/CommandHandler.h"

namespace id::engine
{

std::optional<CmdArgFlags> handle_command(const CommandHandlerTables &tables, const Command &cmd)
{
    if (const std::optional<CmdArgFlags> result{handle_command(tables.startup, cmd, true)}; result.has_value())
    {
        return result;
    }
    return handle_command(tables.unrestricted, cmd);
}

} // namespace id::engine
