// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <MemoryDiskStorage.h>

#include <io/disk_storage.h>
#include <misc/memory.h>

#include <cstdint>
#include <stdexcept>

namespace id::test
{

void init_memory_disk_storage()
{
    io::end_disk();
    misc::init_memory();
}

void open_memory_disk_storage(const int width, const int height, const int colors)
{
    std::uint64_t blocks{};
    if (io::g_disk_flag || !io::disk_video_memory_blocks(width, height, colors, 0, blocks))
    {
        throw std::logic_error("Invalid memory-backed disk storage setup");
    }
    io::start_disk_storage(
        width, height, colors, misc::memory_alloc(io::DISK_STORAGE_BLOCK_SIZE, blocks, misc::MemoryLocation::MEMORY));
}

} // namespace id::test
