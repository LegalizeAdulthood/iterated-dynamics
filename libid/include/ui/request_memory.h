// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include "misc/memory.h"

namespace id::ui
{

// TODO: removed later
// Slice 41 moves storage recovery to the outer requesting controller.
misc::MemoryHandle request_memory(
    misc::U16 size, std::uint64_t count, misc::MemoryLocation location, const std::filesystem::path &backing_file = {});

} // namespace id::ui
