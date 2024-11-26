// SPDX-License-Identifier: GPL-3.0-only
//
#include "port.h"

#include "calcfrac.h"
#include "cmdfiles.h"
#include "cmplx.h"
#include "diskvid.h"
#include "drivers.h"
#include "find_file.h"
#include "id.h"
#include "helpdefs.h"
#include "id_data.h"
#include "id_main.h"
#include "init_failure.h"
#include "make_path.h"
#include "mpmath.h"
#include "read_ticker.h"
#include "rotate.h"
#include "special_dirs.h"
#include "stack_avail.h"
#include "zoom.h"

#include "create_minidump.h"
#include "instance.h"
#include "tos.h"

#include "win_defines.h"
#include <direct.h>
#include <io.h>
#include <Windows.h>
#include <Shlwapi.h>
#include <DbgHelp.h>

#include <cassert>
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <filesystem>

HINSTANCE g_instance{};

// Global variables (yuck!)
char *g_top_of_stack{};

/* Global functions
 *
 * These were either copied from a .c file under unix, or
 * they have assembly language equivalents that we provide
 * here in a slower C form for portability.
 */

/*
; long readticker() returns current bios ticker value
*/
long read_ticker()
{
    return (long) GetTickCount();
}

// tenths of millisecond timewr routine
// static struct timeval tv_start;

void restart_uclock()
{
    // TODO
}

/*
**  usec_clock()
**
**  An analog of the clock() function, usec_clock() returns a number of
**  type uclock_t (defined in UCLOCK.H) which represents the number of
**  microseconds past midnight. Analogous to CLOCKS_PER_SEC is UCLK_TCK, the
**  number which a usec_clock() reading must be divided by to yield
**  a number of seconds.
*/
using uclock_t = unsigned long;
uclock_t usec_clock()
{
    uclock_t result{};
    // TODO
    _ASSERTE(FALSE);

    return result;
}

using MiniDumpWriteDumpProc = BOOL(HANDLE process, DWORD pid, HANDLE file, MINIDUMP_TYPE dumpType,
    PMINIDUMP_EXCEPTION_INFORMATION exceptions, PMINIDUMP_USER_STREAM_INFORMATION user,
    PMINIDUMP_CALLBACK_INFORMATION callback);

namespace fs = std::filesystem;

void create_minidump(EXCEPTION_POINTERS *ep)
{
    HMODULE debughlp = LoadLibraryA("dbghelp.dll");
    if (debughlp == nullptr)
    {
        MessageBoxA(nullptr,
            "An unexpected error occurred while loading dbghelp.dll.\n" ID_PROGRAM_NAME " will now exit.",
            ID_PROGRAM_NAME ": Unexpected Error", MB_OK);
        return;
    }
    MiniDumpWriteDumpProc *dumper{(MiniDumpWriteDumpProc *) GetProcAddress(debughlp, "MiniDumpWriteDump")};
    if (dumper == nullptr)
    {
        MessageBoxA(
            nullptr, "Could not locate MiniDumpWriteDump", ID_PROGRAM_NAME ": Unexpected Error", MB_OK);
        ::FreeLibrary(debughlp);
        return;
    }

    char minidump[MAX_PATH]{"id-" ID_GIT_HASH ".dmp"};
    int i{1};
    fs::path path{g_save_dir};
    while (exists(path / minidump))
    {
        std::sprintf(minidump, "id-" ID_GIT_HASH "-%d.dmp", i++);
    }
    path /= minidump;
    HANDLE dump_file{CreateFileA(path.string().c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_NEW,
        FILE_ATTRIBUTE_NORMAL, nullptr)};
    _ASSERTE(dump_file != INVALID_HANDLE_VALUE);
    if (dump_file == INVALID_HANDLE_VALUE)
    {
        MessageBoxA(nullptr, ("Could not open dump file " + path.string() + " for writing.").c_str(),
            ID_PROGRAM_NAME ": Unexpected Error", MB_OK);
        ::FreeLibrary(debughlp);
        return;
    }

    MINIDUMP_EXCEPTION_INFORMATION mdei{GetCurrentThreadId(), ep, FALSE};
    BOOL status{(*dumper)(
        GetCurrentProcess(), GetCurrentProcessId(), dump_file, MiniDumpNormal, &mdei, nullptr, nullptr)};
    _ASSERTE(status);
    if (!status)
    {
        char msg[100];
        std::sprintf(msg, "MiniDumpWriteDump failed with %08lx", GetLastError());
        MessageBoxA(nullptr, msg, ID_PROGRAM_NAME ": Unexpected Error", MB_OK);
    }
    else
    {
        status = CloseHandle(dump_file);
        _ASSERTE(status);
    }
    dumper = nullptr;
    status = FreeLibrary(debughlp);
    _ASSERTE(status);

    if (g_init_batch != batch_modes::NORMAL)
    {
        char msg[MAX_PATH * 2];
        std::sprintf(msg,
            "Unexpected error, crash dump saved to %s.\n"
            "Please include this file with your bug report.",
            minidump);
        MessageBoxA(nullptr, msg, ID_PROGRAM_NAME ": Unexpected Error", MB_OK);
    }
}

/* ods
 *
 * varargs version of OutputDebugString with file and line markers.
 */
void ods(char const *file, unsigned int line, char const *format, ...)
{
    char full_msg[MAX_PATH+1];
    char app_msg[MAX_PATH+1];
    std::va_list args;

    va_start(args, format);
    std::vsnprintf(app_msg, MAX_PATH, format, args);
    std::snprintf(full_msg, MAX_PATH, "%s(%u): %s\n", file, line, app_msg);
    va_end(args);

    OutputDebugStringA(full_msg);
}

void init_failure(char const *message)
{
    MessageBoxA(nullptr, message, "Id: Fatal Error", MB_OK);
}

#define WIN32_STACK_SIZE 1024*1024

// Return available stack space ... shouldn't be needed in Win32, should it?
long stack_avail()
{
    char junk{};
    return WIN32_STACK_SIZE - (long)(((char *) g_top_of_stack) - &junk);
}
