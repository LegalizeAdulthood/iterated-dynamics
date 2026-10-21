// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include "misc/sized_types.h"

#include <config/port.h>

#include <cstdint>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

namespace id::misc
{

enum class MemoryLocation
{
    NOWHERE = 0,
    MEMORY = 1,
    DISK = 2
};

class MemoryError : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

struct MemoryHandle
{
    U16 index{};

    explicit operator bool() const
    {
        return index != 0;
    }

    void from_memory(const Byte *buffer, U16 size, std::uint64_t count, std::uint64_t offset) const;
    void to_memory(Byte *buffer, U16 size, std::uint64_t count, std::uint64_t offset) const;
    void set(int value, U16 size, std::uint64_t count, std::uint64_t offset) const;
    std::uint64_t size() const;
};

// TODO: Get rid of this and use regular memory routines;
// see about creating standard disk memory routines for disk video
MemoryLocation memory_type(MemoryHandle handle);
void init_memory();
std::vector<std::string> exit_check();
MemoryHandle memory_alloc(
    U16 size, std::uint64_t count, MemoryLocation stored_at, const std::filesystem::path &backing_file = {});
void memory_release(MemoryHandle handle);

} // namespace id::misc
