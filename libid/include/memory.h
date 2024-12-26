// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "port.h"

enum class MemoryLocation
{
    NOWHERE = 0,
    MEMORY = 1,
    DISK = 2
};

struct MemoryHandle
{
    U16 index{};

    operator bool() const
    {
        return index != 0;
    }

    bool from_memory(BYTE const *buffer, U16 size, long count, long offset);
    bool to_memory(BYTE *buffer, U16 size, long count, long offset);
    bool set(int value, U16 size, long count, long offset);
};

// TODO: Get rid of this and use regular memory routines;
// see about creating standard disk memory routines for disk video
MemoryLocation memory_type(MemoryHandle handle);
void init_memory();
void exit_check();
MemoryHandle memory_alloc(U16 size, long count, MemoryLocation stored_at);
void memory_release(MemoryHandle handle);
