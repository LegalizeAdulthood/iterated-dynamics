// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "ui/request_memory.h"

#include "io/get_disk_space.h"
#include "misc/debug_flags.h"
#include "ui/stop_msg.h"

#include <fmt/format.h>

using namespace id::misc;

namespace id::ui
{

// TODO: removed later
// Slice 41 moves storage recovery to the outer requesting controller.
MemoryHandle request_memory(
    const U16 size, const std::uint64_t count, const MemoryLocation location, const std::filesystem::path &backing_file)
{
    for (;;)
    {
        try
        {
            const MemoryHandle handle{memory_alloc(size, count, location, backing_file)};
            const MemoryLocation actual{memory_type(handle)};
            if (location != actual && g_debug_flag == DebugFlags::DISPLAY_MEMORY_STATISTICS)
            {
                stop_msg(StopMsgFlags::INFO_ONLY | StopMsgFlags::NO_BUZZER,
                    fmt::format("Asked for {}, allocated {} bytes of {}, handle = {}.",
                        location == MemoryLocation::MEMORY ? "memory" : "disk", handle.size(),
                        actual == MemoryLocation::MEMORY ? "memory" : "disk", handle.index));
                stop_msg(
                    StopMsgFlags::INFO_ONLY | StopMsgFlags::NO_BUZZER, fmt::format("disk={}", io::get_disk_space()));
            }
            return handle;
        }
        catch (const MemoryError &error)
        {
            if (stop_msg(StopMsgFlags::CANCEL, std::string{error.what()} + "\nPress a key to retry."))
            {
                return {};
            }
        }
    }
}

} // namespace id::ui
