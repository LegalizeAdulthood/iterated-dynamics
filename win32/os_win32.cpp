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
#include "stack_avail.h"
#include "zoom.h"

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

#include "instance.h"

HINSTANCE g_instance{};

static void (*dotwrite)(int, int, int) = nullptr;
static int (*dotread)(int, int) = nullptr;
static void (*linewrite)(int, int, int, BYTE const *) = nullptr;
static void (*lineread)(int, int, int, BYTE *) = nullptr;

// Global variables (yuck!)
int dacnorm = 0;
int g_row_count = 0;
int g_vesa_detect = 0;
int g_vesa_x_res = 0;
int g_vesa_y_res = 0;
int g_video_start_x = 0;
int g_video_start_y = 0;

/* Global functions
 *
 * These were either copied from a .c file under unix, or
 * they have assembly language equivalents that we provide
 * here in a slower C form for portability.
 */

static char *g_tos = nullptr;
#define WIN32_STACK_SIZE 1024*1024
// Return available stack space ... shouldn't be needed in Win32, should it?
long stackavail()
{
    char junk = 0;
    return WIN32_STACK_SIZE - (long)(((char *) g_tos) - &junk);
}

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
typedef unsigned long uclock_t;
uclock_t usec_clock()
{
    uclock_t result = 0;
    // TODO
    _ASSERTE(FALSE);

    return result;
}

typedef BOOL MiniDumpWriteDumpProc(HANDLE process, DWORD pid, HANDLE file, MINIDUMP_TYPE dumpType,
                                   PMINIDUMP_EXCEPTION_INFORMATION exceptions,
                                   PMINIDUMP_USER_STREAM_INFORMATION user,
                                   PMINIDUMP_CALLBACK_INFORMATION callback);

static void CreateMiniDump(EXCEPTION_POINTERS *ep)
{
    MiniDumpWriteDumpProc *dumper = nullptr;
    HMODULE debughlp = LoadLibrary("dbghelp.dll");
    char minidump[MAX_PATH] = "id.dmp";
    MINIDUMP_EXCEPTION_INFORMATION mdei =
    {
        GetCurrentThreadId(),
        ep,
        FALSE
    };
    HANDLE dump_file;
    int i = 1;

    if (debughlp == nullptr)
    {
        MessageBox(nullptr, "An unexpected error occurred.  Iterated Dynamics will now exit.",
                   "Id: Unexpected Error", MB_OK);
        return;
    }
    dumper = (MiniDumpWriteDumpProc *) GetProcAddress(debughlp, "MiniDumpWriteDump");

    while (PathFileExists(minidump))
    {
        std::sprintf(minidump, "id-%d.dmp", i++);
    }
    dump_file = CreateFile(minidump, GENERIC_READ | GENERIC_WRITE,
                           0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    _ASSERTE(dump_file != INVALID_HANDLE_VALUE);

    BOOL status = (*dumper)(GetCurrentProcess(), GetCurrentProcessId(),
                       dump_file, MiniDumpNormal, &mdei, nullptr, nullptr);
    _ASSERTE(status);
    if (!status)
    {
        char msg[100];
        std::sprintf(msg, "MiniDumpWriteDump failed with %08lx", GetLastError());
        MessageBox(nullptr, msg, "Ugh", MB_OK);
    }
    else
    {
        status = CloseHandle(dump_file);
        _ASSERTE(status);
    }
    dumper = nullptr;
    status = FreeLibrary(debughlp);
    _ASSERTE(status);

    {
        char msg[MAX_PATH*2];
        std::sprintf(msg, "Unexpected error, crash dump saved to %s.\n"
                "Please include this file with your bug report.", minidump);
        MessageBox(nullptr, msg, "Id: Unexpected Error", MB_OK);
    }
}

int __stdcall WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR cmdLine, int show)
{
    int result = 0;

#if !defined(_DEBUG)
    __try
#endif
    {
        g_tos = (char *) &result;
        g_instance = instance;
        _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_LEAK_CHECK_DF);
        result = id_main(__argc, __argv);
    }
#if !defined(_DEBUG)
    __except (CreateMiniDump(GetExceptionInformation()), EXCEPTION_EXECUTE_HANDLER)
    {
        result = -1;
    }
#endif

    return result;
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

    OutputDebugString(full_msg);
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
    dotwrite = writedisk;
    dotread = readdisk;
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
    int x1, y1;
    x1 = xdot + g_logical_screen_x_offset;
    y1 = ydot + g_logical_screen_y_offset;
    _ASSERTE(x1 >= 0 && x1 <= g_screen_x_dots);
    _ASSERTE(y1 >= 0 && y1 <= g_screen_y_dots);
    if (x1 < 0 || y1 < 0 || x1 >= g_screen_x_dots || y1 >= g_screen_y_dots)
    {
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
    MessageBox(nullptr, message, "Id: Fatal Error", MB_OK);
}
