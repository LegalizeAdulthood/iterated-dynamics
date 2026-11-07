// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

namespace id::engine
{
struct CommandParseResult;
}

namespace id::ui
{

// Reports action failures as diagnostics; true means the caller should exit.
bool apply_command_effects(engine::CommandParseResult &result);

} // namespace id::ui
