// SPDX-License-Identifier: GPL-3.0-only
//
#include "create_minidump.h"
#include "instance.h"
#include "tos.h"

#include "io/special_dirs.h"
#include "ui/cmdfiles.h"
#include "ui/init_failure.h"

#include <config/port.h>

#include "win_defines.h"
#include <DbgHelp.h>
#include <Windows.h>

#include <cstdio>
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

using MiniDumpWriteDumpProc = BOOL(HANDLE process, DWORD pid, HANDLE file, MINIDUMP_TYPE dumpType,
    PMINIDUMP_EXCEPTION_INFORMATION exceptions, PMINIDUMP_USER_STREAM_INFORMATION user,
    PMINIDUMP_CALLBACK_INFORMATION callback);

namespace fs = std::filesystem;

void create_minidump(EXCEPTION_POINTERS *ep)
{
    HMODULE debug_hlp = LoadLibraryA("dbghelp.dll");
    if (debug_hlp == nullptr)
    {
        MessageBoxA(nullptr,
            "An unexpected error occurred while loading dbghelp.dll.\n" ID_PROGRAM_NAME " will now exit.",
            ID_PROGRAM_NAME ": Unexpected Error", MB_OK);
        return;
    }
    MiniDumpWriteDumpProc *dumper{(MiniDumpWriteDumpProc *) GetProcAddress(debug_hlp, "MiniDumpWriteDump")};
    if (dumper == nullptr)
    {
        MessageBoxA(
            nullptr, "Could not locate MiniDumpWriteDump", ID_PROGRAM_NAME ": Unexpected Error", MB_OK);
        ::FreeLibrary(debug_hlp);
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
        ::FreeLibrary(debug_hlp);
        return;
    }

    MINIDUMP_EXCEPTION_INFORMATION dump_info{GetCurrentThreadId(), ep, FALSE};
    BOOL status{(*dumper)(
        GetCurrentProcess(), GetCurrentProcessId(), dump_file, MiniDumpNormal, &dump_info, nullptr, nullptr)};
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
    status = FreeLibrary(debug_hlp);
    _ASSERTE(status);

    if (g_init_batch != BatchMode::NORMAL)
    {
        char msg[MAX_PATH * 2];
        std::sprintf(msg,
            "Unexpected error, crash dump saved to %s.\n"
            "Please include this file with your bug report.",
            minidump);
        MessageBoxA(nullptr, msg, ID_PROGRAM_NAME ": Unexpected Error", MB_OK);
    }
}

enum
{
    WIN32_STACK_SIZE = 1024 * 1024
};

// Return available stack space ... shouldn't be needed in Win32, should it?
long stack_avail()
{
    char junk{};
    return WIN32_STACK_SIZE - (long)(((char *) g_top_of_stack) - &junk);
}
