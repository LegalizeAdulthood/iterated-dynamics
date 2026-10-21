// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
// Memory allocation routines.
//
#include "misc/memory.h"

#include "io/special_dirs.h"
#include "misc/debug_flags.h"

#include <config/getpid.h>

#include <fmt/format.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#if !defined(_WIN32)
#include <sys/types.h>
#endif

using namespace id::io;

namespace id::misc
{

namespace
{

constexpr U16 MAX_HANDLES{256};
constexpr size_t DISK_WRITE_LEN{2048};

struct Memory
{
    MemoryLocation stored_at{MemoryLocation::NOWHERE};
    std::uint64_t size{};
    Byte *memory{};
    std::FILE *file{};
    std::filesystem::path path;
    bool temporary{};
};

std::array<Memory, MAX_HANDLES> s_handles;

const char *memory_name(const MemoryLocation location)
{
    switch (location)
    {
    case MemoryLocation::NOWHERE:
        return "nowhere";
    case MemoryLocation::MEMORY:
        return "memory";
    case MemoryLocation::DISK:
        return "disk";
    }
    throw std::logic_error("Invalid memory location");
}

Memory &get_memory(const MemoryHandle handle)
{
    if (handle.index == 0 || handle.index >= s_handles.size() ||
        s_handles[handle.index].stored_at == MemoryLocation::NOWHERE)
    {
        throw std::logic_error(fmt::format("Invalid memory handle {}", handle.index));
    }
    return s_handles[handle.index];
}

bool seek_file(std::FILE *file, const std::uint64_t offset)
{
#if defined(_WIN32)
    if (offset > static_cast<std::uint64_t>(std::numeric_limits<__int64>::max()))
    {
        return false;
    }
    return ::_fseeki64(file, static_cast<__int64>(offset), SEEK_SET) == 0;
#else
    if (offset > static_cast<std::uint64_t>(std::numeric_limits<off_t>::max()))
    {
        return false;
    }
    return ::fseeko(file, static_cast<off_t>(offset), SEEK_SET) == 0;
#endif
}

MemoryError disk_error(const char *operation, const std::filesystem::path &path, const int error)
{
    return MemoryError{fmt::format(
        "{} file error: {}: {}", operation, path.string(), error == 0 ? "Incomplete transfer" : std::strerror(error))};
}

std::uint64_t transfer_start(
    const Memory &memory, const U16 size, const std::uint64_t count, const std::uint64_t offset)
{
    if (size == 0 || count == 0 || count > std::numeric_limits<std::uint64_t>::max() / size ||
        offset > std::numeric_limits<std::uint64_t>::max() / size)
    {
        throw std::logic_error("Invalid memory transfer size");
    }
    const std::uint64_t start{offset * size};
    if (start > memory.size || count * size > memory.size - start)
    {
        throw std::logic_error("Memory reference out of bounds");
    }
    return start;
}

void seek_transfer(Memory &memory, const std::uint64_t start)
{
    std::clearerr(memory.file);
    errno = 0;
    if (!seek_file(memory.file, start))
    {
        throw disk_error("Seek", memory.path, errno);
    }
}

void write_disk(Memory &memory, const Byte *buffer, const size_t length)
{
    errno = 0;
    if (std::fwrite(buffer, 1, length, memory.file) != length)
    {
        throw disk_error("Write", memory.path, errno);
    }
}

} // namespace

void MemoryHandle::from_memory(
    const Byte *buffer, const U16 unit_size, const std::uint64_t count, const std::uint64_t offset) const
{
    Memory &memory{get_memory(*this)};
    const std::uint64_t start{transfer_start(memory, unit_size, count, offset)};
    std::uint64_t remaining{count * unit_size};
    if (buffer == nullptr)
    {
        throw std::logic_error("Null memory transfer buffer");
    }
    if (memory.stored_at == MemoryLocation::MEMORY)
    {
        std::memcpy(memory.memory + start, buffer, static_cast<size_t>(remaining));
        return;
    }
    seek_transfer(memory, start);
    while (remaining != 0)
    {
        const size_t length{static_cast<size_t>(std::min<std::uint64_t>(remaining, DISK_WRITE_LEN))};
        write_disk(memory, buffer, length);
        buffer += length;
        remaining -= length;
    }
}

void MemoryHandle::to_memory(
    Byte *buffer, const U16 unit_size, const std::uint64_t count, const std::uint64_t offset) const
{
    Memory &memory{get_memory(*this)};
    const std::uint64_t start{transfer_start(memory, unit_size, count, offset)};
    std::uint64_t remaining{count * unit_size};
    if (buffer == nullptr)
    {
        throw std::logic_error("Null memory transfer buffer");
    }
    if (memory.stored_at == MemoryLocation::MEMORY)
    {
        std::memcpy(buffer, memory.memory + start, static_cast<size_t>(remaining));
        return;
    }
    seek_transfer(memory, start);
    while (remaining != 0)
    {
        const size_t length{static_cast<size_t>(std::min<std::uint64_t>(remaining, DISK_WRITE_LEN))};
        errno = 0;
        if (std::fread(buffer, 1, length, memory.file) != length)
        {
            throw disk_error("Read", memory.path, errno);
        }
        buffer += length;
        remaining -= length;
    }
}

void MemoryHandle::set(
    const int value, const U16 unit_size, const std::uint64_t count, const std::uint64_t offset) const
{
    Memory &memory{get_memory(*this)};
    const std::uint64_t start{transfer_start(memory, unit_size, count, offset)};
    std::uint64_t remaining{count * unit_size};
    if (memory.stored_at == MemoryLocation::MEMORY)
    {
        std::memset(memory.memory + start, value, static_cast<size_t>(remaining));
        return;
    }
    std::array<Byte, DISK_WRITE_LEN> buffer;
    buffer.fill(static_cast<Byte>(value));
    seek_transfer(memory, start);
    while (remaining != 0)
    {
        const size_t length{static_cast<size_t>(std::min<std::uint64_t>(remaining, buffer.size()))};
        write_disk(memory, buffer.data(), length);
        remaining -= length;
    }
}

std::uint64_t MemoryHandle::size() const
{
    return get_memory(*this).size;
}

MemoryLocation memory_type(const MemoryHandle handle)
{
    return handle ? get_memory(handle).stored_at : MemoryLocation::NOWHERE;
}

void init_memory()
{
    for (U16 i = 1; i < MAX_HANDLES; ++i)
    {
        memory_release(MemoryHandle{i});
    }
}

std::vector<std::string> exit_check()
{
    std::vector<std::string> diagnostics;
    for (U16 i = 1; i < MAX_HANDLES; ++i)
    {
        if (s_handles[i].stored_at == MemoryLocation::NOWHERE)
        {
            continue;
        }
        if (diagnostics.empty())
        {
            diagnostics.emplace_back("Error - not all memory released, I'll get it.");
        }
        diagnostics.push_back(
            fmt::format("Memory type {} still allocated.  Handle = {}.", memory_name(s_handles[i].stored_at), i));
        memory_release(MemoryHandle{i});
    }
    return diagnostics;
}

MemoryHandle memory_alloc(
    const U16 size, const std::uint64_t count, MemoryLocation requested, const std::filesystem::path &backing_file)
{
    if (size == 0 || count == 0 || count > std::numeric_limits<std::uint64_t>::max() / size)
    {
        throw std::logic_error("Invalid memory allocation size");
    }
    const std::uint64_t bytes{count * size};
    if (requested == MemoryLocation::NOWHERE)
    {
        throw std::logic_error("Cannot allocate memory at nowhere");
    }
    memory_name(requested);
    if (g_debug_flag == DebugFlags::FORCE_MEMORY_FROM_DISK || !backing_file.empty())
    {
        requested = MemoryLocation::DISK;
    }
    else if (g_debug_flag == DebugFlags::FORCE_MEMORY_FROM_MEMORY)
    {
        requested = MemoryLocation::MEMORY;
    }
    U16 index{1};
    while (index < MAX_HANDLES && s_handles[index].stored_at != MemoryLocation::NOWHERE)
    {
        ++index;
    }
    if (index == MAX_HANDLES)
    {
        throw MemoryError("No memory handles available");
    }
    Memory &memory{s_handles[index]};
    if (requested == MemoryLocation::MEMORY && bytes <= std::numeric_limits<size_t>::max())
    {
        memory.memory = static_cast<Byte *>(std::calloc(static_cast<size_t>(bytes), 1));
        if (memory.memory != nullptr)
        {
            memory.size = bytes;
            memory.stored_at = MemoryLocation::MEMORY;
            return {index};
        }
    }
    const bool temporary{backing_file.empty()};
    std::filesystem::path path{temporary ? g_temp_dir / fmt::format("id.{}.{}.tmp", getpid(), index) : backing_file};
    std::error_code error;
    const std::filesystem::space_info space{
        std::filesystem::space(path.has_parent_path() ? path.parent_path() : std::filesystem::path{"."}, error)};
    if (error || space.available < bytes)
    {
        throw MemoryError(fmt::format("Allocating {} bytes of {} failed: {}", bytes, memory_name(requested),
            error ? error.message() : "insufficient disk space"));
    }
    std::FILE *file{std::fopen(path.string().c_str(), temporary ? "w+b" : "r+b")};
    if (file == nullptr)
    {
        throw disk_error("Create", path, errno);
    }
    std::error_code size_error;
    const bool extend{temporary || std::filesystem::file_size(path, size_error) < bytes};
    if (size_error)
    {
        std::fclose(file);
        throw MemoryError(fmt::format("Cannot inspect backing file {}: {}", path.string(), size_error.message()));
    }
    errno = 0;
    if ((extend && (!seek_file(file, bytes - 1) || std::fputc(0, file) == EOF || std::fflush(file) != 0)) ||
        !seek_file(file, 0))
    {
        const int failure{errno};
        std::fclose(file);
        if (temporary)
        {
            std::filesystem::remove(path, error);
        }
        throw disk_error("Create", path, failure);
    }
    memory.path.swap(path);
    memory.file = file;
    memory.temporary = temporary;
    memory.size = bytes;
    memory.stored_at = MemoryLocation::DISK;
    return {index};
}

void memory_release(const MemoryHandle handle)
{
    if (handle.index >= MAX_HANDLES)
    {
        throw std::logic_error("Invalid memory handle");
    }
    Memory &memory{s_handles[handle.index]};
    if (memory.stored_at == MemoryLocation::MEMORY)
    {
        std::free(memory.memory);
    }
    else if (memory.stored_at == MemoryLocation::DISK)
    {
        std::fclose(memory.file);
        if (memory.temporary)
        {
            std::error_code ignore;
            std::filesystem::remove(memory.path, ignore);
        }
    }
    memory = Memory{};
}

} // namespace id::misc
