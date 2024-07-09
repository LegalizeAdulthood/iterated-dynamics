#include "port.h"
#include "prototyp.h"

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

// Memory allocation routines.

enum
{
    FAR_RESERVE = 8192L,  // amount of far mem we will leave avail.
                          // For disk memory:
    DISKWRITELEN = 2048L, // max # bytes transferred to/from disk mem at once
    MAXHANDLES = 256      // arbitrary #, suitably big
};

struct nowhere
{
    stored_at_values stored_at; // first 2 entries must be the same
    long size;                       // for each of these data structures
};

struct linearmem
{
    stored_at_values stored_at;
    long size;
    BYTE *memory;
};

struct disk
{
    stored_at_values stored_at;
    long size;
    std::FILE *file;
};

union mem
{
    nowhere Nowhere;
    linearmem Linearmem;
    disk Disk;
};

// Routines in this module
static bool CheckDiskSpace(long howmuch);
static int check_for_mem(int stored_at, long howmuch);
static U16 next_handle();
static int CheckBounds(long start, long length, U16 handle);
static void WhichDiskError(int);
static void DisplayError(int stored_at, long howmuch);
static void DisplayHandle(U16 handle);

static int s_num_total_handles{};
static constexpr const char *const s_memory_names[3]{"nowhere", "memory", "disk"};
static mem handletable[MAXHANDLES];

int MemoryType(U16 handle);
void InitMemory();
void ExitCheck();
U16 MemoryAlloc(U16 size, long count, int stored_at);
void MemoryRelease(U16 handle);
bool CopyFromMemoryToHandle(BYTE *buffer, U16 size, long count, long offset, U16 handle);
bool CopyFromHandleToMemory(BYTE *buffer, U16 size, long count, long offset, U16 handle);
bool SetMemory(int value, U16 size, long count, long offset, U16 handle);

// Memory handling support routines

static bool CheckDiskSpace(long)
{
    return true;
}

static void WhichDiskError(int I_O)
{
    // Set I_O == 1 after a file create, I_O == 2 after a file set value
    // Set I_O == 3 after a file write, I_O == 4 after a file read
    char buf[MSG_LEN];
    char const *pats[4] =
    {
        "Create file error %d:  %s",
        "Set file error %d:  %s",
        "Write file error %d:  %s",
        "Read file error %d:  %s"
    };
    std::snprintf(buf, std::size(buf), pats[(1 <= I_O && I_O <= 4) ? (I_O-1) : 0], errno, strerror(errno));
    if (g_debug_flag == debug_flags::display_memory_statistics)
    {
        if (stopmsg(stopmsg_flags::CANCEL | stopmsg_flags::NO_BUZZER, buf))
        {
            goodbye(); // bailout if ESC
        }
    }
}

int MemoryType(U16 handle)
{
    return handletable[handle].Nowhere.stored_at;
}

static void DisplayError(int stored_at, long howmuch)
{
    // This routine is used to display an error message when the requested
    // memory type cannot be allocated due to insufficient memory, AND there
    // is also insufficient disk space to use as memory.

    char buf[MSG_LEN*2];
    std::snprintf(buf, std::size(buf), "Allocating %ld Bytes of %s memory failed.\n"
            "Alternate disk space is also insufficient. Goodbye",
            howmuch, s_memory_names[stored_at]);
    stopmsg(buf);
}

static int check_for_mem(int stored_at, long howmuch)
{
    // This function returns an adjusted stored_at value.
    // This is where the memory requested can be allocated.

    long maxmem;
    BYTE *temp;
    int use_this_type;

    maxmem = (long)USHRT_MAX;

    if (g_debug_flag == debug_flags::force_memory_from_disk)
    {
        stored_at = DISK;
    }
    if (g_debug_flag == debug_flags::force_memory_from_memory)
    {
        stored_at = MEMORY;
    }

    switch (stored_at)
    {
    case MEMORY: // check_for_mem
        if (maxmem > howmuch)
        {
            temp = (BYTE *)malloc(howmuch + FAR_RESERVE);
            if (temp != nullptr)
            {
                // minimum free space + requested amount
                free(temp);
                use_this_type = MEMORY;
                break;
            }
        }

    case DISK: // check_for_mem
    default: // just in case a nonsense number gets used
        if (CheckDiskSpace(howmuch))
        {
            use_this_type = DISK;
            break;
        }
        // failed, fall through, no memory available

    case NOWHERE: // check_for_mem
        use_this_type = NOWHERE;
        break;

    } // end of switch

    return use_this_type;
}

static U16 next_handle()
{
    U16 counter = 1; // don't use handle 0

    while (handletable[counter].Nowhere.stored_at != NOWHERE &&
            counter < MAXHANDLES)
    {
        counter++;
    }
    return counter;
}

static int CheckBounds(long start, long length, U16 handle)
{
    if (handletable[handle].Nowhere.size - start - length < 0)
    {
        stopmsg(stopmsg_flags::INFO_ONLY | stopmsg_flags::NO_BUZZER, "Memory reference out of bounds.");
        DisplayHandle(handle);
        return 1;
    }
    if (length > (long)USHRT_MAX)
    {
        stopmsg(stopmsg_flags::INFO_ONLY | stopmsg_flags::NO_BUZZER, "Tried to move > 65,535 bytes.");
        DisplayHandle(handle);
        return 1;
    }
    if (handletable[handle].Nowhere.stored_at == DISK
        && (stackavail() <= DISKWRITELEN))
    {
        stopmsg(stopmsg_flags::INFO_ONLY | stopmsg_flags::NO_BUZZER, "Stack space insufficient for disk memory.");
        DisplayHandle(handle);
        return 1;
    }
    if (length <= 0)
    {
        stopmsg(stopmsg_flags::INFO_ONLY | stopmsg_flags::NO_BUZZER, "Zero or negative length.");
        DisplayHandle(handle);
        return 1;
    }
    if (start < 0)
    {
        stopmsg(stopmsg_flags::INFO_ONLY | stopmsg_flags::NO_BUZZER, "Negative offset.");
        DisplayHandle(handle);
        return 1;
    }
    return 0;
}

void DisplayMemory()
{
    char buf[MSG_LEN];

    std::snprintf(buf, std::size(buf), "disk=%lu", get_disk_space());
    stopmsg(stopmsg_flags::INFO_ONLY | stopmsg_flags::NO_BUZZER, buf);
}

static void DisplayHandle(U16 handle)
{
    char buf[MSG_LEN];

    std::snprintf(buf, std::size(buf), "Handle %u, type %s, size %li", handle, s_memory_names[handletable[handle].Nowhere.stored_at],
            handletable[handle].Nowhere.size);
    if (stopmsg(stopmsg_flags::CANCEL | stopmsg_flags::NO_BUZZER, buf))
    {
        goodbye(); // bailout if ESC, it's messy, but should work
    }
}

void InitMemory()
{
    s_num_total_handles = 0;
    for (auto &elem : handletable)
    {
        elem.Nowhere.stored_at = NOWHERE;
        elem.Nowhere.size = 0;
    }
}

void ExitCheck()
{
    if (s_num_total_handles != 0)
    {
        stopmsg("Error - not all memory released, I'll get it.");
        for (U16 i = 1; i < MAXHANDLES; i++)
        {
            if (handletable[i].Nowhere.stored_at != NOWHERE)
            {
                char buf[MSG_LEN];
                std::snprintf(buf, std::size(buf), "Memory type %s still allocated.  Handle = %u.",
                        s_memory_names[handletable[i].Nowhere.stored_at], i);
                stopmsg(buf);
                MemoryRelease(i);
            }
        }
    }
}

static std::string mem_file_name(U16 handle)
{
    return "id" + std::to_string(handle) + ".$$$";
}

// * * * *
// Memory handling routines

U16 MemoryAlloc(U16 size, long count, int stored_at)
{
    // Returns handle number if successful, 0 or nullptr if failure

    long toallocate = count * size;
    if (toallocate <= 0)      // we failed, can't allocate > 2,147,483,647
    {
        return 0U;          // or it wraps around to negative
    }

    /* check structure for requested memory type (add em up) to see if
       sufficient amount is available to grant request */

    int use_this_type = check_for_mem(stored_at, toallocate);
    if (use_this_type == NOWHERE)
    {
        DisplayError(stored_at, toallocate);
        goodbye();
    }

    // get next available handle

    U16 handle = next_handle();

    if (handle >= MAXHANDLES || handle == 0)
    {
        DisplayHandle(handle);
        return 0U;
        // Oops, do something about this! ?????
    }

    // attempt to allocate requested memory type
    bool success = false;
    switch (use_this_type)
    {
    default:
    case NOWHERE: // MemoryAlloc
        use_this_type = NOWHERE; // in case nonsense value is passed
        break;

    case MEMORY: // MemoryAlloc
        // Availability of memory checked in check_for_mem()
        handletable[handle].Linearmem.memory = (BYTE *)malloc(toallocate);
        handletable[handle].Linearmem.size = toallocate;
        handletable[handle].Linearmem.stored_at = MEMORY;
        s_num_total_handles++;
        success = true;
        break;

    case DISK: // MemoryAlloc
        if (g_disk_targa)
        {
            handletable[handle].Disk.file = dir_fopen(g_working_dir.c_str(), g_light_name.c_str(), "a+b");
        }
        else
        {
            handletable[handle].Disk.file = dir_fopen(g_temp_dir.c_str(), mem_file_name(handle).c_str(), "w+b");
        }
        rewind(handletable[handle].Disk.file);
        if (fseek(handletable[handle].Disk.file, toallocate, SEEK_SET) != 0)
        {
            handletable[handle].Disk.file = nullptr;
        }
        if (handletable[handle].Disk.file == nullptr)
        {
            handletable[handle].Disk.stored_at = NOWHERE;
            use_this_type = NOWHERE;
            WhichDiskError(1);
            DisplayMemory();
            driver_buzzer(buzzer_codes::PROBLEM);
            break;
        }
        s_num_total_handles++;
        success = true;
        std::fclose(handletable[handle].Disk.file); // so clusters aren't lost if we crash while running
        handletable[handle].Disk.file = g_disk_targa ?
            dir_fopen(g_working_dir.c_str(), g_light_name.c_str(), "r+b") :
            dir_fopen(g_temp_dir.c_str(), mem_file_name(handle).c_str(), "r+b");
        // cppcheck-suppress useClosedFile
        rewind(handletable[handle].Disk.file);
        handletable[handle].Disk.size = toallocate;
        handletable[handle].Disk.stored_at = DISK;
        use_this_type = DISK;
        break;
    } // end of switch

    if (stored_at != use_this_type && g_debug_flag == debug_flags::display_memory_statistics)
    {
        char buf[MSG_LEN * 2];
        std::snprintf(buf, std::size(buf), "Asked for %s, allocated %ld bytes of %s, handle = %u.",
            s_memory_names[stored_at], toallocate, s_memory_names[use_this_type], handle);
        stopmsg(stopmsg_flags::INFO_ONLY | stopmsg_flags::NO_BUZZER, buf);
        DisplayMemory();
    }

    if (success)
    {
        return handle;
    }
    else          // return 0 if failure
    {
        return 0U;
    }
}

void MemoryRelease(U16 handle)
{
    switch (handletable[handle].Nowhere.stored_at)
    {
    case NOWHERE: // MemoryRelease
        break;

    case MEMORY: // MemoryRelease
        free(handletable[handle].Linearmem.memory);
        handletable[handle].Linearmem.memory = nullptr;
        handletable[handle].Linearmem.size = 0;
        handletable[handle].Linearmem.stored_at = NOWHERE;
        s_num_total_handles--;
        break;

    case DISK: // MemoryRelease
        std::fclose(handletable[handle].Disk.file);
        dir_remove(g_temp_dir.c_str(), mem_file_name(handle));
        handletable[handle].Disk.file = nullptr;
        handletable[handle].Disk.size = 0;
        handletable[handle].Disk.stored_at = NOWHERE;
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
bool CopyFromMemoryToHandle(BYTE const *buffer, U16 size, long count, long offset, U16 handle)
{
    BYTE diskbuf[DISKWRITELEN];
    long start; // offset to first location to move to
    long tomove; // number of bytes to move
    U16 numwritten;

    start = (long)offset * size;
    tomove = (long)count * size;
    if (g_debug_flag == debug_flags::display_memory_statistics)
    {
        if (CheckBounds(start, tomove, handle))
        {
            return false; // out of bounds, don't do it
        }
    }

    bool success = false;
    switch (handletable[handle].Nowhere.stored_at)
    {
    case NOWHERE: // MoveToMemory
        DisplayHandle(handle);
        break;

    case MEMORY: // MoveToMemory
#if defined(_WIN32)
        _ASSERTE(handletable[handle].Linearmem.size >= size*count + start);
#endif
        std::memcpy(handletable[handle].Linearmem.memory + start, buffer, size*count);
        success = true; // No way to gauge success or failure
        break;

    case DISK: // MoveToMemory
        rewind(handletable[handle].Disk.file);
        fseek(handletable[handle].Disk.file, start, SEEK_SET);
        while (tomove > DISKWRITELEN)
        {
            std::memcpy(diskbuf, buffer, (U16)DISKWRITELEN);
            numwritten = (U16)write1(diskbuf, (U16)DISKWRITELEN, 1, handletable[handle].Disk.file);
            if (numwritten != 1)
            {
                WhichDiskError(3);
                goto diskerror;
            }
            tomove -= DISKWRITELEN;
            buffer += DISKWRITELEN;
        }
        std::memcpy(diskbuf, buffer, (U16)tomove);
        numwritten = (U16)write1(diskbuf, (U16)tomove, 1, handletable[handle].Disk.file);
        if (numwritten != 1)
        {
            WhichDiskError(3);
            break;
        }
        success = true;
diskerror:
        break;
    } // end of switch
    if (!success && g_debug_flag == debug_flags::display_memory_statistics)
    {
        DisplayHandle(handle);
    }
    return success;
}

// buffer points is the location to move the data to
// offset is the number of units from the beginning of buffer to start moving
// size is the size of the unit, count is the number of units to move
// Returns true if successful, false if failure
bool CopyFromHandleToMemory(BYTE *buffer, U16 size, long count, long offset, U16 handle)
{
    BYTE diskbuf[DISKWRITELEN];
    long start; // first location to move
    long tomove; // number of bytes to move
    U16 numread;

    start = (long)offset * size;
    tomove = (long)count * size;
    if (g_debug_flag == debug_flags::display_memory_statistics)
    {
        if (CheckBounds(start, tomove, handle))
        {
            return false; // out of bounds, don't do it
        }
    }

    bool success = false;
    switch (handletable[handle].Nowhere.stored_at)
    {
    case NOWHERE: // MoveFromMemory
        DisplayHandle(handle);
        break;

    case MEMORY: // MoveFromMemory
        for (int i = 0; i < size; i++)
        {
            std::memcpy(buffer, handletable[handle].Linearmem.memory+start, (U16)count);
            start += count;
            buffer += count;
        }
        success = true; // No way to gauge success or failure
        break;

    case DISK: // MoveFromMemory
        rewind(handletable[handle].Disk.file);
        fseek(handletable[handle].Disk.file, start, SEEK_SET);
        while (tomove > DISKWRITELEN)
        {
            numread = (U16)std::fread(diskbuf, (U16)DISKWRITELEN, 1, handletable[handle].Disk.file);
            if (numread != 1 && !std::feof(handletable[handle].Disk.file))
            {
                WhichDiskError(4);
                goto diskerror;
            }
            std::memcpy(buffer, diskbuf, (U16)DISKWRITELEN);
            tomove -= DISKWRITELEN;
            buffer += DISKWRITELEN;
        }
        numread = (U16)std::fread(diskbuf, (U16)tomove, 1, handletable[handle].Disk.file);
        if (numread != 1 && !std::feof(handletable[handle].Disk.file))
        {
            WhichDiskError(4);
            break;
        }
        std::memcpy(buffer, diskbuf, (U16)tomove);
        success = true;
diskerror:
        break;
    } // end of switch
    if (!success && g_debug_flag == debug_flags::display_memory_statistics)
    {
        DisplayHandle(handle);
    }
    return success;
}

bool SetMemory(int value, U16 size, long count, long offset, U16 handle)
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
        if (CheckBounds(start, tomove, handle))
        {
            return false; // out of bounds, don't do it
        }
    }

    bool success = false;
    switch (handletable[handle].Nowhere.stored_at)
    {
    case NOWHERE: // SetMemory
        DisplayHandle(handle);
        break;

    case MEMORY: // SetMemory
        for (int i = 0; i < size; i++)
        {
            std::memset(handletable[handle].Linearmem.memory+start, value, (U16)count);
            start += count;
        }
        success = true; // No way to gauge success or failure
        break;

    case DISK: // SetMemory
        std::memset(diskbuf, value, (U16)DISKWRITELEN);
        rewind(handletable[handle].Disk.file);
        fseek(handletable[handle].Disk.file, start, SEEK_SET);
        while (tomove > DISKWRITELEN)
        {
            numwritten = (U16)write1(diskbuf, (U16)DISKWRITELEN, 1, handletable[handle].Disk.file);
            if (numwritten != 1)
            {
                WhichDiskError(2);
                goto diskerror;
            }
            tomove -= DISKWRITELEN;
        }
        numwritten = (U16)write1(diskbuf, (U16)tomove, 1, handletable[handle].Disk.file);
        if (numwritten != 1)
        {
            WhichDiskError(2);
            break;
        }
        success = true;
diskerror:
        break;
    } // end of switch
    if (!success && g_debug_flag == debug_flags::display_memory_statistics)
    {
        DisplayHandle(handle);
    }
    return success;
}
