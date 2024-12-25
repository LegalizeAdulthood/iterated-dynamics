// SPDX-License-Identifier: GPL-3.0-only
//
#include "memory.h"

#include "cmdfiles.h"
#include "debug_flags.h"
#include "dir_file.h"
#include "diskvid.h"
#include "drivers.h"
#include "get_disk_space.h"
#include "goodbye.h"
#include "id.h"
#include "line3d.h"
#include "stack_avail.h"
#include "stop_msg.h"

#include <array>
#include <climits>
#include <cstdio>
#include <cstring>

// for getpid
#ifdef WIN32
#include <process.h>
#else
#include <unistd.h>
#endif

// Memory allocation routines.

enum
{
                          // For disk memory:
    DISKWRITELEN = 2048L, // max # bytes transferred to/from disk mem at once
    MAXHANDLES = 256      // arbitrary #, suitably big
};

namespace
{

struct LinearMemory
{
    BYTE *memory;
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
static bool check_disk_space(long howmuch);
static MemoryLocation check_for_mem(MemoryLocation where, long size);
static U16 next_handle();
static int check_bounds(long start, long length, U16 handle);
static void which_disk_error(int);
static void display_error(MemoryLocation stored_at, long howmuch);
static void display_handle(U16 handle);

static int s_num_total_handles{};
static constexpr const char *const s_memory_names[3]{"nowhere", "memory", "disk"};
static Memory s_handles[MAXHANDLES];

// Memory handling support routines

static bool check_disk_space(long)
{
    return true;
}

static const char *memory_type(MemoryLocation where)
{
    return s_memory_names[static_cast<int>(where)];
}

static void which_disk_error(int I_O)
{
    // Set I_O == 1 after a file create, I_O == 2 after a file set value
    // Set I_O == 3 after a file write, I_O == 4 after a file read
    char buf[MSG_LEN];
    char const *pats[4] = {"Create file error %d:  %s", "Set file error %d:  %s", "Write file error %d:  %s",
        "Read file error %d:  %s"};
    std::snprintf(buf, std::size(buf), pats[(1 <= I_O && I_O <= 4) ? (I_O - 1) : 0], errno, strerror(errno));
    if (g_debug_flag == debug_flags::display_memory_statistics)
    {
        if (stop_msg(stopmsg_flags::CANCEL | stopmsg_flags::NO_BUZZER, buf))
        {
            goodbye(); // bailout if ESC
        }
    }
}

MemoryLocation memory_type(U16 handle)
{
    return s_handles[handle].stored_at;
}

static void display_error(MemoryLocation stored_at, long howmuch)
{
    // This routine is used to display an error message when the requested
    // memory type cannot be allocated due to insufficient memory, AND there
    // is also insufficient disk space to use as memory.

    char buf[MSG_LEN*2];
    std::snprintf(buf, std::size(buf),
        "Allocating %ld Bytes of %s memory failed.\n"
        "Alternate disk space is also insufficient. Goodbye",
        howmuch, memory_type(stored_at));
    stop_msg(buf);
}

static MemoryLocation check_for_mem(MemoryLocation where, long size)
{
    // This function returns an adjusted stored_at value.
    // This is where the memory requested can be allocated.

    long maxmem;
    BYTE *temp;
    MemoryLocation use_this_type;

    maxmem = (long)USHRT_MAX;

    if (g_debug_flag == debug_flags::force_memory_from_disk)
    {
        where = MemoryLocation::DISK;
    }
    if (g_debug_flag == debug_flags::force_memory_from_memory)
    {
        where = MemoryLocation::MEMORY;
    }

    switch (where)
    {
    case MemoryLocation::MEMORY: // check_for_mem
        if (maxmem > size)
        {
            temp = (BYTE *)malloc(size);
            if (temp != nullptr)
            {
                // minimum free space + requested amount
                free(temp);
                use_this_type = MemoryLocation::MEMORY;
                break;
            }
        }

    case MemoryLocation::DISK: // check_for_mem
    default: // just in case a nonsense number gets used
        if (check_disk_space(size))
        {
            use_this_type = MemoryLocation::DISK;
            break;
        }
        // failed, fall through, no memory available

    case MemoryLocation::NOWHERE: // check_for_mem
        use_this_type = MemoryLocation::NOWHERE;
        break;
    } // end of switch

    return use_this_type;
}

static U16 next_handle()
{
    U16 counter = 1; // don't use handle 0

    while (s_handles[counter].stored_at != MemoryLocation::NOWHERE &&
            counter < MAXHANDLES)
    {
        counter++;
    }
    return counter;
}

static int check_bounds(long start, long length, U16 handle)
{
    if (s_handles[handle].size < static_cast<std::uint64_t>(start - length))
    {
        stop_msg(stopmsg_flags::INFO_ONLY | stopmsg_flags::NO_BUZZER, "Memory reference out of bounds.");
        display_handle(handle);
        return 1;
    }
    if (length > (long)USHRT_MAX)
    {
        stop_msg(stopmsg_flags::INFO_ONLY | stopmsg_flags::NO_BUZZER, "Tried to move > 65,535 bytes.");
        display_handle(handle);
        return 1;
    }
    if (s_handles[handle].stored_at == MemoryLocation::DISK && (stack_avail() <= DISKWRITELEN))
    {
        stop_msg(stopmsg_flags::INFO_ONLY | stopmsg_flags::NO_BUZZER, "Stack space insufficient for disk memory.");
        display_handle(handle);
        return 1;
    }
    if (length <= 0)
    {
        stop_msg(stopmsg_flags::INFO_ONLY | stopmsg_flags::NO_BUZZER, "Zero or negative length.");
        display_handle(handle);
        return 1;
    }
    if (start < 0)
    {
        stop_msg(stopmsg_flags::INFO_ONLY | stopmsg_flags::NO_BUZZER, "Negative offset.");
        display_handle(handle);
        return 1;
    }
    return 0;
}

void display_memory()
{
    char buf[MSG_LEN];
    std::snprintf(buf, std::size(buf), "disk=%lu", get_disk_space());
    stop_msg(stopmsg_flags::INFO_ONLY | stopmsg_flags::NO_BUZZER, buf);
}

static void display_handle(U16 handle)
{
    char buf[MSG_LEN];
    std::snprintf(buf, std::size(buf), "Handle %u, type %s, size %llu", handle,
        memory_type(s_handles[handle].stored_at), s_handles[handle].size);
    if (stop_msg(stopmsg_flags::CANCEL | stopmsg_flags::NO_BUZZER, buf))
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
    for (U16 i = 1; i < MAXHANDLES; i++)
    {
        if (s_handles[i].stored_at == MemoryLocation::NOWHERE)
        {
            continue;
        }

        char buf[MSG_LEN];
        std::snprintf(buf, std::size(buf), "Memory type %s still allocated.  Handle = %u.",
            memory_type(s_handles[i].stored_at), i);
        stop_msg(buf);
        memory_release(i);
    }
}

static std::string mem_file_name(U16 handle)
{
    return "id." + std::to_string(::getpid()) + '.' + std::to_string(handle) + ".tmp";
}

// * * * *
// Memory handling routines

// Returns handle number if successful, 0 or nullptr if failure
U16 memory_alloc(U16 size, long count, MemoryLocation stored_at)
{
    std::uint64_t toallocate = count * size;

    /* check structure for requested memory type (add em up) to see if
       sufficient amount is available to grant request */

    MemoryLocation use_this_type = check_for_mem(stored_at, toallocate);
    if (use_this_type == MemoryLocation::NOWHERE)
    {
        display_error(stored_at, toallocate);
        goodbye();
    }

    // get next available handle

    U16 handle = next_handle();

    if (handle >= MAXHANDLES || handle == 0)
    {
        display_handle(handle);
        return 0U;
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
        s_handles[handle].linear.memory = (BYTE *)malloc(toallocate);
        s_handles[handle].size = toallocate;
        s_handles[handle].stored_at = MemoryLocation::MEMORY;
        s_num_total_handles++;
        success = true;
        break;

    case MemoryLocation::DISK: // MemoryAlloc
        if (g_disk_targa)
        {
            s_handles[handle].disk.file = dir_fopen(g_working_dir.c_str(), g_light_name.c_str(), "a+b");
        }
        else
        {
            s_handles[handle].disk.file = dir_fopen(g_temp_dir.c_str(), mem_file_name(handle).c_str(), "w+b");
        }
        std::rewind(s_handles[handle].disk.file);
        if (std::fseek(s_handles[handle].disk.file, toallocate, SEEK_SET) != 0)
        {
            s_handles[handle].disk.file = nullptr;
        }
        if (s_handles[handle].disk.file == nullptr)
        {
            s_handles[handle].stored_at = MemoryLocation::NOWHERE;
            use_this_type = MemoryLocation::NOWHERE;
            which_disk_error(1);
            display_memory();
            driver_buzzer(buzzer_codes::PROBLEM);
            break;
        }
        s_num_total_handles++;
        success = true;
        std::fclose(s_handles[handle].disk.file); // so clusters aren't lost if we crash while running
        s_handles[handle].disk.file = g_disk_targa ?
            dir_fopen(g_working_dir.c_str(), g_light_name.c_str(), "r+b") :
            dir_fopen(g_temp_dir.c_str(), mem_file_name(handle).c_str(), "r+b");
        // cppcheck-suppress useClosedFile
        std::rewind(s_handles[handle].disk.file);
        s_handles[handle].size = toallocate;
        s_handles[handle].stored_at = MemoryLocation::DISK;
        use_this_type = MemoryLocation::DISK;
        break;
    } // end of switch

    if (stored_at != use_this_type && g_debug_flag == debug_flags::display_memory_statistics)
    {
        char buf[MSG_LEN * 2];
        std::snprintf(buf, std::size(buf), "Asked for %s, allocated %llu bytes of %s, handle = %u.",
            memory_type(stored_at), toallocate, memory_type(use_this_type), handle);
        stop_msg(stopmsg_flags::INFO_ONLY | stopmsg_flags::NO_BUZZER, buf);
        display_memory();
    }

    return success ? handle : 0U;
}

void memory_release(U16 handle)
{
    switch (s_handles[handle].stored_at)
    {
    case MemoryLocation::NOWHERE: // MemoryRelease
        break;

    case MemoryLocation::MEMORY: // MemoryRelease
        free(s_handles[handle].linear.memory);
        s_handles[handle].linear.memory = nullptr;
        s_handles[handle].size = 0;
        s_handles[handle].stored_at = MemoryLocation::NOWHERE;
        s_num_total_handles--;
        break;

    case MemoryLocation::DISK: // MemoryRelease
        std::fclose(s_handles[handle].disk.file);
        dir_remove(g_temp_dir.c_str(), mem_file_name(handle));
        s_handles[handle].disk.file = nullptr;
        s_handles[handle].size = 0;
        s_handles[handle].stored_at = MemoryLocation::NOWHERE;
        s_num_total_handles--;
        break;
    } // end of switch
}

// buffer is a pointer to local memory
// Always start moving from the beginning of buffer
// offset is the number of units from the start of the allocated "Memory"
// to start moving the contents of buffer to
// size is the size of the unit, count is the number of units to move
// Returns true if successful, false if failure
bool copy_from_memory_to_handle(BYTE const *buffer, U16 size, long count, long offset, U16 handle)
{
    BYTE diskbuf[DISKWRITELEN];
    long start; // offset to first location to move to
    long tomove; // number of bytes to move
    U16 numwritten;

    start = (long)offset * size;
    tomove = (long)count * size;
    if (g_debug_flag == debug_flags::display_memory_statistics)
    {
        if (check_bounds(start, tomove, handle))
        {
            return false; // out of bounds, don't do it
        }
    }

    bool success = false;
    switch (s_handles[handle].stored_at)
    {
    case MemoryLocation::NOWHERE: // MoveToMemory
        display_handle(handle);
        break;

    case MemoryLocation::MEMORY: // MoveToMemory
#if defined(_WIN32)
        _ASSERTE(s_handles[handle].size >= size*count + start);
#endif
        std::memcpy(s_handles[handle].linear.memory + start, buffer, size*count);
        success = true; // No way to gauge success or failure
        break;

    case MemoryLocation::DISK: // MoveToMemory
        std::rewind(s_handles[handle].disk.file);
        std::fseek(s_handles[handle].disk.file, start, SEEK_SET);
        while (tomove > DISKWRITELEN)
        {
            std::memcpy(diskbuf, buffer, (U16)DISKWRITELEN);
            numwritten = (U16)write1(diskbuf, (U16)DISKWRITELEN, 1, s_handles[handle].disk.file);
            if (numwritten != 1)
            {
                which_disk_error(3);
                goto diskerror;
            }
            tomove -= DISKWRITELEN;
            buffer += DISKWRITELEN;
        }
        std::memcpy(diskbuf, buffer, (U16)tomove);
        numwritten = (U16)write1(diskbuf, (U16)tomove, 1, s_handles[handle].disk.file);
        if (numwritten != 1)
        {
            which_disk_error(3);
            break;
        }
        success = true;
diskerror:
        break;
    } // end of switch
    if (!success && g_debug_flag == debug_flags::display_memory_statistics)
    {
        display_handle(handle);
    }
    return success;
}

// buffer points is the location to move the data to
// offset is the number of units from the beginning of buffer to start moving
// size is the size of the unit, count is the number of units to move
// Returns true if successful, false if failure
bool copy_from_handle_to_memory(BYTE *buffer, U16 size, long count, long offset, U16 handle)
{
    BYTE diskbuf[DISKWRITELEN];
    long start; // first location to move
    long tomove; // number of bytes to move
    U16 numread;

    start = (long)offset * size;
    tomove = (long)count * size;
    if (g_debug_flag == debug_flags::display_memory_statistics)
    {
        if (check_bounds(start, tomove, handle))
        {
            return false; // out of bounds, don't do it
        }
    }

    bool success = false;
    switch (s_handles[handle].stored_at)
    {
    case MemoryLocation::NOWHERE: // MoveFromMemory
        display_handle(handle);
        break;

    case MemoryLocation::MEMORY: // MoveFromMemory
        for (int i = 0; i < size; i++)
        {
            std::memcpy(buffer, s_handles[handle].linear.memory+start, (U16)count);
            start += count;
            buffer += count;
        }
        success = true; // No way to gauge success or failure
        break;

    case MemoryLocation::DISK: // MoveFromMemory
        std::rewind(s_handles[handle].disk.file);
        std::fseek(s_handles[handle].disk.file, start, SEEK_SET);
        while (tomove > DISKWRITELEN)
        {
            numread = (U16)std::fread(diskbuf, (U16)DISKWRITELEN, 1, s_handles[handle].disk.file);
            if (numread != 1 && !std::feof(s_handles[handle].disk.file))
            {
                which_disk_error(4);
                goto diskerror;
            }
            std::memcpy(buffer, diskbuf, (U16)DISKWRITELEN);
            tomove -= DISKWRITELEN;
            buffer += DISKWRITELEN;
        }
        numread = (U16)std::fread(diskbuf, (U16)tomove, 1, s_handles[handle].disk.file);
        if (numread != 1 && !std::feof(s_handles[handle].disk.file))
        {
            which_disk_error(4);
            break;
        }
        std::memcpy(buffer, diskbuf, (U16)tomove);
        success = true;
diskerror:
        break;
    } // end of switch
    if (!success && g_debug_flag == debug_flags::display_memory_statistics)
    {
        display_handle(handle);
    }
    return success;
}

bool set_memory(int value, U16 size, long count, long offset, U16 handle)
{
    // value is the value to set memory to
    // offset is the number of units from the start of allocated memory
    // size is the size of the unit, count is the number of units to set
    // Returns true if successful, false if failure
    BYTE diskbuf[DISKWRITELEN];
    long start; // first location to set
    long tomove; // number of bytes to set
    U16 numwritten;

    start = (long)offset * size;
    tomove = (long)count * size;
    if (g_debug_flag == debug_flags::display_memory_statistics)
    {
        if (check_bounds(start, tomove, handle))
        {
            return false; // out of bounds, don't do it
        }
    }

    bool success = false;
    switch (s_handles[handle].stored_at)
    {
    case MemoryLocation::NOWHERE: // SetMemory
        display_handle(handle);
        break;

    case MemoryLocation::MEMORY: // SetMemory
        for (int i = 0; i < size; i++)
        {
            std::memset(s_handles[handle].linear.memory+start, value, (U16)count);
            start += count;
        }
        success = true; // No way to gauge success or failure
        break;

    case MemoryLocation::DISK: // SetMemory
        std::memset(diskbuf, value, (U16)DISKWRITELEN);
        std::rewind(s_handles[handle].disk.file);
        std::fseek(s_handles[handle].disk.file, start, SEEK_SET);
        while (tomove > DISKWRITELEN)
        {
            numwritten = (U16)write1(diskbuf, (U16)DISKWRITELEN, 1, s_handles[handle].disk.file);
            if (numwritten != 1)
            {
                which_disk_error(2);
                goto diskerror;
            }
            tomove -= DISKWRITELEN;
        }
        numwritten = (U16)write1(diskbuf, (U16)tomove, 1, s_handles[handle].disk.file);
        if (numwritten != 1)
        {
            which_disk_error(2);
            break;
        }
        success = true;
diskerror:
        break;
    } // end of switch
    if (!success && g_debug_flag == debug_flags::display_memory_statistics)
    {
        display_handle(handle);
    }
    return success;
}
