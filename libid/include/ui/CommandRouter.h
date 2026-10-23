// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include "engine/CommandHandler.h"

#include <string>
#include <vector>

namespace id::engine
{
class CommandProcessor;
}

namespace id::ui
{

class CommandRouter
{
public:
    CommandRouter();

    // Validate ownership before processing any parameters.
    CommandRouter(engine::CommandHandlerTables ui, engine::CommandHandlerTables engine);

    engine::CommandParseResult process(engine::CommandProcessor &processor) const;

    engine::CommandParseResult process_startup(const std::vector<std::string> &args) const;

    // Restore defaults and configuration after abandoned entry selection.
    engine::CommandParseResult reset_startup() const;

private:
    engine::CommandHandlerTables m_ui;
    engine::CommandHandlerTables m_engine;
};

} // namespace id::ui
