// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include "misc/version.h"

#include <optional>

namespace id::engine
{

class Command;

// Invalid input returns nullopt without changing settings or diagnostics.
std::optional<misc::Version> parse_reset_version(const Command &cmd);

// Apply compatibility versions without resetting other settings.
void apply_reset_version(const misc::Version &version);

} // namespace id::engine
