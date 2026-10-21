// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include "misc/memory.h"

#include <config/port.h>

#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <vector>

namespace id::io
{

constexpr int DISK_STORAGE_BLOCK_SIZE{2048};
constexpr int DISK_STORAGE_CACHE_KB{4096};

enum class DiskStorageOperation
{
    NONE,
    READ,
    WRITE
};

struct DiskStorageProgress
{
    DiskStorageOperation operation{};
    int row{};
};

extern bool g_disk_16_bit;
extern bool g_disk_flag;
extern bool g_disk_targa;

bool disk_video_memory_blocks(
    std::int64_t row_size, std::int64_t col_size, int colors, int header_length, std::uint64_t &blocks);
misc::MemoryLocation disk_video_memory_type();
// Takes ownership of file, including on failure.
std::vector<Byte> read_targa_header(std::FILE *file, int length);
// Takes ownership of backing after the initialization preconditions pass.
void start_disk_storage(std::int64_t row_size, std::int64_t col_size, int colors, misc::MemoryHandle backing,
    const std::vector<Byte> &header = {});
void end_disk();
void set_targa_disk_path(const std::filesystem::path &path);
const std::filesystem::path &targa_disk_path();
DiskStorageProgress disk_storage_progress();
int read_disk_pixel(int col, int row);
void write_disk_pixel(int col, int row, int color);
void read_targa_pixel(unsigned int col, unsigned int row, Byte *red, Byte *green, Byte *blue);
void write_targa_pixel(unsigned int col, unsigned int row, Byte red, Byte green, Byte blue);
bool from_mem_disk(std::int64_t offset, int size, void *dest);
bool to_mem_disk(std::int64_t offset, int size, const void *src);

} // namespace id::io
