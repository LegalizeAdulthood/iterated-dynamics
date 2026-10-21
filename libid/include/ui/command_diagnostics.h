// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

namespace id::engine
{
struct CommandDiagnostics;
}

namespace id::ui
{

void show_command_diagnostics(const engine::CommandDiagnostics &diagnostics);
bool show_startup_command_diagnostics(const engine::CommandDiagnostics &diagnostics);

} // namespace id::ui
