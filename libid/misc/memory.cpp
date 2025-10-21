// SPDX-License-Identifier: GPL-3.0-only
//
// Memory allocation routines.
//
#include "misc/memory.h"

#include "geometry/line3d.h"
#include "io/dir_file.h"
#include "io/get_disk_space.h"
#include "io/special_dirs.h"
#include "misc/debug_flags.h"
#include "misc/Driver.h"
#include "misc/stack_avail.h"
#include "ui/diskvid.h"
#include "ui/goodbye.h"
#include "ui/stop_msg.h"

#include <config/getpid.h>

#include <fmt/format.h>

#include <cinttypes>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>

namespace fs = std::filesystem;

using namespace id::engine;
using namespace id::geometry;
using namespace id::io;
using namespace id::ui;

namespace id::misc
{

enum
{
    DISK_WRITE_LEN = 2048L, // disk memory: max # bytes transferred to/from disk mem at once
    MAX_HANDLES = 256       // disk memory: arbitrary #, suitably big
};

namespace
{

struct LinearMemory
{
    Byte *memory;
};

struct Disk
{
    std::FILE *file;
};

struct Memory
{
    MemoryLocation stored_at;
    std::uint64_t size;
    LinearMemory linear;
    Disk disk;
};

} // namespace

// Routines in this module
static bool check_disk_space(std::uint64_t size);
static MemoryLocation check_for_mem(MemoryLocation where, std::uint64_t size);
static U16 next_handle();
static int check_bounds(long start, long length, U16 handle);
static void which_disk_error(int status);
static void display_error(MemoryLocation stored_at, long how_much);
static void display_handle(U16 handle);

static int s_num_total_handles{};
static constexpr const char *const MEMORY_NAMES[3]{"nowhere", "memory", "disk"};
static Memory s_handles[MAX_HANDLES];

// Memory handling support routines

static bool check_disk_space(const std::uint64_t size)
{
    const std::filesystem::space_info space{std::filesystem::space(g_temp_dir)};
    return space.free >= size;
}

static const char *memory_type(MemoryLocation where)
{
    return MEMORY_NAMES[static_cast<int>(where)];
}

static void which_disk_error(const int status)
{
    // Set status == 1 after a file create, status == 2 after a file set value
    // Set status == 3 after a file write, status == 4 after a file read
    if (g_debug_flag == DebugFlags::DISPLAY_MEMORY_STATISTICS)
    {
        const char *names[4]{"Create", "Set", "Write", "Read"};
        if (stop_msg(StopMsgFlags::CANCEL | StopMsgFlags::NO_BUZZER,
                fmt::format("{:s} file error {:d}:  {:s}",
                    names[status >= 1 && status <= 4 ? status - 1 : 0], errno, std::strerror(errno))))
        {
            goodbye(); // bailout if ESC
        }
    }
}

// buffer is a pointer to local memory
// Always start moving from the beginning of buffer
// offset is the number of units from the start of the allocated "Memory"
// to start moving the contents of buffer to
// size is the size of the unit, count is the number of units to move
// Returns true if successful, false if failure
bool MemoryHandle::from_memory(const Byte *buffer, const U16 size, const long count, const long offset)
{
    Byte disk_buff[DISK_WRITE_LEN];
    U16 num_written;
    const long start = offset * size; // offset to first location to move to
    long to_move = count * size; // number of bytes to move
    if (g_debug_flag == DebugFlags::DISPLAY_MEMORY_STATISTICS)
    {
        if (check_bounds(start, to_move, index))
        {
            return false; // out of bounds, don't do it
        }
    }
    bool success = false;
    switch (s_handles[index].stored_at)
    {
    case MemoryLocation::NOWHERE: // MoveToMemory
        display_handle(index);
        break;

    case MemoryLocation::MEMORY: // MoveToMemory
#if defined(_WIN32)
        _ASSERTE(s_handles[index].size >= size * count + start);
#endif
        std::memcpy(s_handles[index].linear.memory + start, buffer, size * count);
        success = true; // No way to gauge success or failure
        break;

    case MemoryLocation::DISK: // MoveToMemory
        std::fseek(s_handles[index].disk.file, start, SEEK_SET);
        while (to_move > DISK_WRITE_LEN)
        {
            std::memcpy(disk_buff, buffer, DISK_WRITE_LEN);
            num_written = static_cast<U16>(std::fwrite(disk_buff, DISK_WRITE_LEN, 1, s_handles[index].disk.file));
            if (num_written != 1)
            {
                which_disk_error(3);
                goto disk_error;
            }
            to_move -= DISK_WRITE_LEN;
            buffer += DISK_WRITE_LEN;
        }
        std::memcpy(disk_buff, buffer, static_cast<U16>(to_move));
        num_written = static_cast<U16>(std::fwrite(disk_buff, static_cast<U16>(to_move), 1, s_handles[index].disk.file));
        if (num_written != 1)
        {
            which_disk_error(3);
            break;
        }
        success = true;
disk_error:
        break;
    } // end of switch
    if (!success && g_debug_flag == DebugFlags::DISPLAY_MEMORY_STATISTICS)
    {
        display_handle(index);
    }
    return success;
}

// buffer points is the location to move the data to
// offset is the number of units from the beginning of buffer to start moving
// size is the size of the unit, count is the number of units to move
// Returns true if successful, false if failure
bool MemoryHandle::to_memory(Byte *buffer, const U16 size, const long count, const long offset)
{
    Byte disk_buff[DISK_WRITE_LEN];
    U16 num_read;
    long start = offset * size;// first location to move
    long to_move = count * size; // number of bytes to move
    if (g_debug_flag == DebugFlags::DISPLAY_MEMORY_STATISTICS)
    {
        if (check_bounds(start, to_move, index))
        {
            return false; // out of bounds, don't do it
        }
    }
    bool success = false;
    switch (s_handles[index].stored_at)
    {
    case MemoryLocation::NOWHERE: // MoveFromMemory
        display_handle(index);
        break;

    case MemoryLocation::MEMORY: // MoveFromMemory
        for (int i = 0; i < size; i++)
        {
            std::memcpy(buffer, s_handles[index].linear.memory + start, static_cast<U16>(count));
            start += count;
            buffer += count;
        }
        success = true; // No way to gauge success or failure
        break;

    case MemoryLocation::DISK: // MoveFromMemory
        std::fseek(s_handles[index].disk.file, start, SEEK_SET);
        while (to_move > DISK_WRITE_LEN)
        {
            num_read = static_cast<U16>(std::fread(disk_buff, DISK_WRITE_LEN, 1, s_handles[index].disk.file));
            if (num_read != 1 && !std::feof(s_handles[index].disk.file))
            {
                which_disk_error(4);
                goto disk_error;
            }
            std::memcpy(buffer, disk_buff, DISK_WRITE_LEN);
            to_move -= DISK_WRITE_LEN;
            buffer += DISK_WRITE_LEN;
        }
        num_read = static_cast<U16>(std::fread(disk_buff, static_cast<U16>(to_move), 1, s_handles[index].disk.file));
        if (num_read != 1 && !std::feof(s_handles[index].disk.file))
        {
            which_disk_error(4);
            break;
        }
        std::memcpy(buffer, disk_buff, static_cast<U16>(to_move));
        success = true;
disk_error:
        break;
    } // end of switch
    if (!success && g_debug_flag == DebugFlags::DISPLAY_MEMORY_STATISTICS)
    {
        display_handle(index);
    }
    return success;
}


// value is the value to set memory to
// offset is the number of units from the start of allocated memory
// size is the size of the unit, count is the number of units to set
// Returns true if successful, false if failure
bool MemoryHandle::set(const int value, const U16 size, const long count, const long offset)
{
    Byte disk_buff[DISK_WRITE_LEN];
    U16 num_written;
    long start = offset * size; // first location to set
    long to_move = count * size; // number of bytes to set
    if (g_debug_flag == DebugFlags::DISPLAY_MEMORY_STATISTICS)
    {
        if (check_bounds(start, to_move, index))
        {
            return false; // out of bounds, don't do it
        }
    }
    bool success = false;
    switch (s_handles[index].stored_at)
    {
    case MemoryLocation::NOWHERE: // SetMemory
        display_handle(index);
        break;

    case MemoryLocation::MEMORY: // SetMemory
        for (int i = 0; i < size; i++)
        {
            std::memset(s_handles[index].linear.memory + start, value, static_cast<U16>(count));
            start += count;
        }
        success = true; // No way to gauge success or failure
        break;

    case MemoryLocation::DISK: // SetMemory
        std::memset(disk_buff, value, DISK_WRITE_LEN);
        std::fseek(s_handles[index].disk.file, start, SEEK_SET);
        while (to_move > DISK_WRITE_LEN)
        {
            num_written = static_cast<U16>(std::fwrite(disk_buff, DISK_WRITE_LEN, 1, s_handles[index].disk.file));
            if (num_written != 1)
            {
                which_disk_error(2);
                goto disk_error;
            }
            to_move -= DISK_WRITE_LEN;
        }
        num_written = static_cast<U16>(std::fwrite(disk_buff, static_cast<U16>(to_move), 1, s_handles[index].disk.file));
        if (num_written != 1)
        {
            which_disk_error(2);
            break;
        }
        success = true;
disk_error:
        break;
    } // end of switch
    if (!success && g_debug_flag == DebugFlags::DISPLAY_MEMORY_STATISTICS)
    {
        display_handle(index);
    }
    return success;
}

MemoryLocation memory_type(const MemoryHandle handle)
{
    return s_handles[handle.index].stored_at;
}

static void display_error(const MemoryLocation stored_at, long how_much)
{
    // This routine is used to display an error message when the requested
    // memory type cannot be allocated due to insufficient memory, AND there
    // is also insufficient disk space to use as memory.
    stop_msg(fmt::format("Allocating {:d} Bytes of {:s} memory failed.\n"
                         "Alternate disk space is also insufficient. Goodbye",
        how_much, memory_type(stored_at)));
}

// This function returns an adjusted stored_at value.
// This is where the memory requested can be allocated.
static MemoryLocation check_for_mem(MemoryLocation where, const std::uint64_t size)
{
    if (g_debug_flag == DebugFlags::FORCE_MEMORY_FROM_DISK)
    {
        where = MemoryLocation::DISK;
    }
    if (g_debug_flag == DebugFlags::FORCE_MEMORY_FROM_MEMORY)
    {
        where = MemoryLocation::MEMORY;
    }

    MemoryLocation result{MemoryLocation::NOWHERE};
    switch (where)
    {
    case MemoryLocation::MEMORY:
        if (void *temp = std::malloc(size); temp != nullptr)
        {
            std::free(temp);
            result = MemoryLocation::MEMORY;
            break;
        }
        // failed, fall through, not enough heap available

    case MemoryLocation::DISK: // NOLINT(clang-diagnostic-implicit-fallthrough)
        if (check_disk_space(size))
        {
            result = MemoryLocation::DISK;
            break;
        }
        // failed, fall through, no memory available

    case MemoryLocation::NOWHERE:  // NOLINT(clang-diagnostic-implicit-fallthrough)
        result = MemoryLocation::NOWHERE;
        break;
    }

    return result;
}

static U16 next_handle()
{
    U16 counter = 1; // don't use handle 0

    while (s_handles[counter].stored_at != MemoryLocation::NOWHERE && counter < MAX_HANDLES)
    {
        counter++;
    }
    return counter;
}

static int check_bounds(const long start, const long length, const U16 handle)
{
    if (s_handles[handle].size < static_cast<std::uint64_t>(start - length))
    {
        stop_msg(StopMsgFlags::INFO_ONLY | StopMsgFlags::NO_BUZZER, "Memory reference out of bounds.");
        display_handle(handle);
        return 1;
    }
    if (length > static_cast<long>(USHRT_MAX))
    {
        stop_msg(StopMsgFlags::INFO_ONLY | StopMsgFlags::NO_BUZZER, "Tried to move > 65,535 bytes.");
        display_handle(handle);
        return 1;
    }
    if (s_handles[handle].stored_at == MemoryLocation::DISK && stack_avail() <= DISK_WRITE_LEN)
    {
        stop_msg(StopMsgFlags::INFO_ONLY | StopMsgFlags::NO_BUZZER, "Stack space insufficient for disk memory.");
        display_handle(handle);
        return 1;
    }
    if (length <= 0)
    {
        stop_msg(StopMsgFlags::INFO_ONLY | StopMsgFlags::NO_BUZZER, "Zero or negative length.");
        display_handle(handle);
        return 1;
    }
    if (start < 0)
    {
        stop_msg(StopMsgFlags::INFO_ONLY | StopMsgFlags::NO_BUZZER, "Negative offset.");
        display_handle(handle);
        return 1;
    }
    return 0;
}

static void display_memory()
{
    stop_msg(StopMsgFlags::INFO_ONLY | StopMsgFlags::NO_BUZZER, fmt::format("disk={:d}", get_disk_space()));
}

static void display_handle(U16 handle)
{
    if (stop_msg(StopMsgFlags::CANCEL | StopMsgFlags::NO_BUZZER,
            fmt::format("Handle {:d}, type {:s}, size {:d}", //
                handle, memory_type(s_handles[handle].stored_at), s_handles[handle].size)))
    {
        goodbye(); // bailout if ESC, it's messy, but should work
    }
}

void init_memory()
{
    s_num_total_handles = 0;
    for (Memory &elem : s_handles)
    {
        elem.stored_at = MemoryLocation::NOWHERE;
        elem.size = 0;
    }
}

void exit_check()
{
    if (s_num_total_handles == 0)
    {
        return;
    }

    stop_msg("Error - not all memory released, I'll get it.");
    for (U16 i = 1; i < MAX_HANDLES; i++)
    {
        if (s_handles[i].stored_at == MemoryLocation::NOWHERE)
        {
            continue;
        }

        stop_msg(fmt::format(
            "Memory type {:s} still allocated.  Handle = {:d}.", memory_type(s_handles[i].stored_at), i));
        memory_release(MemoryHandle{i});
    }
}

static std::string mem_filename(U16 handle)
{
    return fmt::format("id.{:d}.{:d}.tmp", getpid(), handle);
}

// * * * *
// Memory handling routines

// Returns handle number if successful, 0 or nullptr if failure
MemoryHandle memory_alloc(const U16 size, const long count, const MemoryLocation stored_at)
{
    std::uint64_t to_allocate = count * size;

    /* check structure for requested memory type (add em up) to see if
       sufficient amount is available to grant request */

    MemoryLocation use_this_type = check_for_mem(stored_at, to_allocate);
    if (use_this_type == MemoryLocation::NOWHERE)
    {
        display_error(stored_at, to_allocate);
        goodbye();
    }

    // get next available handle

    U16 handle = next_handle();

    if (handle >= MAX_HANDLES || handle == 0)
    {
        display_handle(handle);
        return {};
        // Oops, do something about this! ?????
    }

    // attempt to allocate requested memory type
    bool success = false;
    switch (use_this_type)
    {
    default:
    case MemoryLocation::NOWHERE: // MemoryAlloc
        use_this_type = MemoryLocation::NOWHERE; // in case nonsense value is passed
        break;

    case MemoryLocation::MEMORY: // MemoryAlloc
        // Availability of memory checked in check_for_mem()
        s_handles[handle].linear.memory = static_cast<Byte *>(malloc(to_allocate));
        s_handles[handle].size = to_allocate;
        s_handles[handle].stored_at = MemoryLocation::MEMORY;
        s_num_total_handles++;
        success = true;
        break;

    case MemoryLocation::DISK: // MemoryAlloc
        if (g_disk_targa)
        {
            s_handles[handle].disk.file = dir_fopen(g_working_dir, g_light_name, "a+b");
        }
        else
        {
            s_handles[handle].disk.file = dir_fopen(g_temp_dir, mem_filename(handle), "w+b");
        }
        std::fseek(s_handles[handle].disk.file, 0, SEEK_SET);
        if (std::fseek(s_handles[handle].disk.file, to_allocate, SEEK_SET) != 0)
        {
            s_handles[handle].disk.file = nullptr;
        }
        if (s_handles[handle].disk.file == nullptr)
        {
            s_handles[handle].stored_at = MemoryLocation::NOWHERE;
            use_this_type = MemoryLocation::NOWHERE;
            which_disk_error(1);
            display_memory();
            driver_buzzer(Buzzer::PROBLEM);
            break;
        }
        s_num_total_handles++;
        success = true;
        std::fclose(s_handles[handle].disk.file); // so clusters aren't lost if we crash while running
        s_handles[handle].disk.file = g_disk_targa ?
            dir_fopen(g_working_dir, g_light_name, "r+b") :
            dir_fopen(g_temp_dir, mem_filename(handle), "r+b");
        // cppcheck-suppress useClosedFile
        std::fseek(s_handles[handle].disk.file, 0, SEEK_SET);
        s_handles[handle].size = to_allocate;
        s_handles[handle].stored_at = MemoryLocation::DISK;
        use_this_type = MemoryLocation::DISK;
        break;
    } // end of switch

    if (stored_at != use_this_type && g_debug_flag == DebugFlags::DISPLAY_MEMORY_STATISTICS)
    {
        stop_msg(StopMsgFlags::INFO_ONLY | StopMsgFlags::NO_BUZZER,
            fmt::format("Asked for {:s}, allocated {:d} bytes of {:s}, handle = {:d}.", //
                memory_type(stored_at), to_allocate, memory_type(use_this_type), handle));
        display_memory();
    }

    if (success)
    {
        return {handle};
    }
    return {};
}

void memory_release(const MemoryHandle handle)
{
    const U16 index{handle.index};
    switch (s_handles[index].stored_at)
    {
    case MemoryLocation::NOWHERE: // MemoryRelease
        break;

    case MemoryLocation::MEMORY: // MemoryRelease
        free(s_handles[index].linear.memory);
        s_handles[index].linear.memory = nullptr;
        s_handles[index].size = 0;
        s_handles[index].stored_at = MemoryLocation::NOWHERE;
        s_num_total_handles--;
        break;

    case MemoryLocation::DISK: // MemoryRelease
        std::fclose(s_handles[index].disk.file);
        dir_remove(g_temp_dir, mem_filename(index));
        s_handles[index].disk.file = nullptr;
        s_handles[index].size = 0;
        s_handles[index].stored_at = MemoryLocation::NOWHERE;
        s_num_total_handles--;
        break;
    } // end of switch
}

} // namespace id::misc
