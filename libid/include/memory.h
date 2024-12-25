// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "port.h"

enum class stored_at_values
{
    NOWHERE = 0,
    MEMORY = 1,
    DISK = 2
};

// TODO: Get rid of this and use regular memory routines;
// see about creating standard disk memory routines for disk video
stored_at_values memory_type(U16 handle);
void init_memory();
void exit_check();
U16 memory_alloc(U16 size, long count, stored_at_values stored_at);
void memory_release(U16 handle);
bool copy_from_memory_to_handle(BYTE const *buffer, U16 size, long count, long offset, U16 handle);
bool copy_from_handle_to_memory(BYTE *buffer, U16 size, long count, long offset, U16 handle);
bool set_memory(int value, U16 size, long count, long offset, U16 handle);
