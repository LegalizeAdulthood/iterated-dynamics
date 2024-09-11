#include "port.h"

#include "calcfrac.h"
#include "cmdfiles.h"
#include "cmplx.h"
#include "diskvid.h"
#include "drivers.h"
#include "find_file.h"
#include "get_color.h"
#include "get_line.h"
#include "id.h"
#include "helpdefs.h"
#include "id_data.h"
#include "id_main.h"
#include "init_failure.h"
#include "make_path.h"
#include "mpmath.h"
#include "out_line.h"
#include "put_color_a.h"
#include "read_ticker.h"
#include "rotate.h"
#include "special_dirs.h"
#include "stack_avail.h"
#include "zoom.h"

#include "create_minidump.h"
#include "instance.h"
#include "tos.h"

#include <direct.h>
#include <io.h>
#define WIN32_LEAN_AND_MEAN
#define STRICT
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

static void (*dotwrite)(int, int, int){};
static int (*dotread)(int, int){};
static void (*linewrite)(int, int, int, BYTE const *){};
static void (*lineread)(int, int, int, BYTE *){};

// Global variables (yuck!)
int g_row_count{};
int g_vesa_detect{};
int g_vesa_x_res{};
int g_vesa_y_res{};
int g_video_start_x{};
int g_video_start_y{};
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
long readticker()
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
void
ods(char const *file, unsigned int line, char const *format, ...)
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

/*
; ***Function get_line(int row, int startcol, int stopcol, unsigned char *pixels)

;       This routine is a 'line' analog of 'getcolor()', and gets a segment
;       of a line from the screen and stores it in pixels[] at one byte per
;       pixel
;       Called by the GIF decoder
*/

void get_line(int row, int startcol, int stopcol, BYTE *pixels)
{
    if (startcol + g_logical_screen_x_offset >= g_screen_x_dots || row + g_logical_screen_y_offset >= g_screen_y_dots)
        return;
    _ASSERTE(lineread);
    (*lineread)(row + g_logical_screen_y_offset, startcol + g_logical_screen_x_offset, stopcol + g_logical_screen_x_offset, pixels);
}

/*
; ***Function put_line(int row, int startcol, int stopcol, unsigned char *pixels)

;       This routine is a 'line' analog of 'putcolor()', and puts a segment
;       of a line from the screen and stores it in pixels[] at one byte per
;       pixel
;       Called by the GIF decoder
*/

void put_line(int row, int startcol, int stopcol, BYTE const *pixels)
{
    if (startcol + g_logical_screen_x_offset >= g_screen_x_dots || row + g_logical_screen_y_offset > g_screen_y_dots)
    {
        return;
    }
    _ASSERTE(linewrite);
    (*linewrite)(row + g_logical_screen_y_offset, startcol + g_logical_screen_x_offset, stopcol + g_logical_screen_x_offset, pixels);
}

/*
; **************** internal Read/Write-a-line routines *********************
;
;       These routines are called by out_line(), put_line() and get_line().
*/
void normaline(int y, int x, int lastx, BYTE const *pixels)
{
    int width = lastx - x + 1;
    _ASSERTE(dotwrite);
    for (int i = 0; i < width; i++)
    {
        (*dotwrite)(x + i, y, pixels[i]);
    }
}

void normalineread(int y, int x, int lastx, BYTE *pixels)
{
    int width = lastx - x + 1;
    _ASSERTE(dotread);
    for (int i = 0; i < width; i++)
    {
        pixels[i] = (*dotread)(x + i, y);
    }
}

void set_normal_dot()
{
    dotwrite = driver_write_pixel;
    dotread = driver_read_pixel;
}

void set_disk_dot()
{
    dotwrite = disk_write_pixel;
    dotread = disk_read_pixel;
}

void set_normal_line()
{
    lineread = normalineread;
    linewrite = normaline;
}

static void nullwrite(int a, int b, int c)
{
    _ASSERTE(FALSE);
}

static int nullread(int a, int b)
{
    _ASSERTE(FALSE);
    return 0;
}

// from video.asm
void setnullvideo()
{
    _ASSERTE(0 && "setnullvideo called");
    dotwrite = nullwrite;
    dotread = nullread;
}

/*
; **************** Function getcolor(xdot, ydot) *******************

;       Return the color on the screen at the (xdot, ydot) point
*/
int getcolor(int xdot, int ydot)
{
    const int x1 = xdot + g_logical_screen_x_offset;
    const int y1 = ydot + g_logical_screen_y_offset;
    if (x1 < 0 || y1 < 0 || x1 >= g_screen_x_dots || y1 >= g_screen_y_dots)
    {
        // this can happen in boundary trace
        return 0;
    }
    _ASSERTE(dotread);
    return (*dotread)(x1, y1);
}

/*
; ************** Function putcolor_a(xdot, ydot, color) *******************

;       write the color on the screen at the (xdot, ydot) point
*/
void putcolor_a(int xdot, int ydot, int color)
{
    int x1 = xdot + g_logical_screen_x_offset;
    int y1 = ydot + g_logical_screen_y_offset;
    _ASSERTE(x1 >= 0 && x1 <= g_screen_x_dots);
    _ASSERTE(y1 >= 0 && y1 <= g_screen_y_dots);
    _ASSERTE(dotwrite);
    (*dotwrite)(x1, y1, color & g_and_color);
}

/*
; ***************Function out_line(pixels, linelen) *********************

;       This routine is a 'line' analog of 'putcolor()', and sends an
;       entire line of pixels to the screen (0 <= xdot < xdots) at a clip
;       Called by the GIF decoder
*/
int out_line(BYTE *pixels, int linelen)
{
    _ASSERTE(_CrtCheckMemory());
    if (g_row_count + g_logical_screen_y_offset >= g_screen_y_dots)
    {
        return 0;
    }
    _ASSERTE(linewrite);
    (*linewrite)(g_row_count + g_logical_screen_y_offset, g_logical_screen_x_offset, linelen + g_logical_screen_x_offset - 1, pixels);
    g_row_count++;
    return 0;
}

void init_failure(char const *message)
{
    MessageBoxA(nullptr, message, "Id: Fatal Error", MB_OK);
}

#define WIN32_STACK_SIZE 1024*1024

// Return available stack space ... shouldn't be needed in Win32, should it?
long stackavail()
{
    char junk{};
    return WIN32_STACK_SIZE - (long)(((char *) g_top_of_stack) - &junk);
}
