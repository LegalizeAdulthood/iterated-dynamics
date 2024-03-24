#include "port.h"
#include "prototyp.h"

#include "calcfrac.h"
#include "cmdfiles.h"
#include "cmplx.h"
#include "diskvid.h"
#include "drivers.h"
#include "find_file.h"
#include "id.h"
#include "helpdefs.h"
#include "id_data.h"
#include "make_path.h"
#include "miscovl.h"
#include "mpmath.h"
#include "prompts2.h"
#include "rotate.h"
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

#include "frame.h"

HINSTANCE g_instance = nullptr;

static void (*dotwrite)(int, int, int) = nullptr;
static int (*dotread)(int, int) = nullptr;
static void (*linewrite)(int, int, int, BYTE const *) = nullptr;
static void (*lineread)(int, int, int, BYTE *) = nullptr;

enum fractint_event
{
    FE_UNKNOWN = -1,
    FE_IMAGE_INFO,                  // TAB
    FE_RESTART,                     // INSERT
    FE_SELECT_VIDEO_MODE,           // DELETE
    FE_EXECUTE_COMMANDS,            // @
    FE_COMMAND_SHELL,               // d
    FE_ORBITS_WINDOW,               // o
    FE_SELECT_FRACTAL_TYPE,         // t
    FE_TOGGLE_JULIA,                // FIK_SPACE
    FE_TOGGLE_INVERSE,              // j
    FE_PRIOR_IMAGE,                 // h
    FE_REVERSE_HISTORY,             // ^H
    FE_BASIC_OPTIONS,               // x
    FE_EXTENDED_OPTIONS,            // y
    FE_TYPE_SPECIFIC_PARAMS,        // z
    FE_PASSES_OPTIONS,              // p
    FE_VIEW_WINDOW_OPTIONS,         // v
    FE_3D_PARAMS,                   // i
    FE_BROWSE_PARAMS,               // ^B
    FE_EVOLVER_PARAMS,              // ^E
    FE_SOUND_PARAMS,                // ^F
    FE_SAVE_IMAGE,                  // s
    FE_LOAD_IMAGE,                  // r
    FE_3D_TRANSFORM,                // 3
    FE_3D_OVERLAY,                  // #
    FE_SAVE_CURRENT_PARAMS,         // b
    FE_PRINT_IMAGE,                 // ^P
    FE_GIVE_COMMAND_STRING,         // g
    FE_QUIT,                        // ESC
    FE_COLOR_CYCLING_MODE,          // c
    FE_ROTATE_PALETTE_DOWN,         // -
    FE_ROTATE_PALETTE_UP,           // +
    FE_EDIT_PALETTE,                // e
    FE_MAKE_STARFIELD,              // a
    FE_ANT_AUTOMATON,               // ^A
    FE_STEREOGRAM,                  // ^S
    FE_VIDEO_F1,
    FE_VIDEO_F2,
    FE_VIDEO_F3,
    FE_VIDEO_F4,
    FE_VIDEO_F5,
    FE_VIDEO_F6,
    FE_VIDEO_F7,
    FE_VIDEO_F8,
    FE_VIDEO_F9,
    FE_VIDEO_F10,
    FE_VIDEO_F11,
    FE_VIDEO_F12,
    FE_VIDEO_AF1,
    FE_VIDEO_AF2,
    FE_VIDEO_AF3,
    FE_VIDEO_AF4,
    FE_VIDEO_AF5,
    FE_VIDEO_AF6,
    FE_VIDEO_AF7,
    FE_VIDEO_AF8,
    FE_VIDEO_AF9,
    FE_VIDEO_AF10,
    FE_VIDEO_AF11,
    FE_VIDEO_AF12,
    FE_VIDEO_CF1,
    FE_VIDEO_CF2,
    FE_VIDEO_CF3,
    FE_VIDEO_CF4,
    FE_VIDEO_CF5,
    FE_VIDEO_CF6,
    FE_VIDEO_CF7,
    FE_VIDEO_CF8,
    FE_VIDEO_CF9,
    FE_VIDEO_CF10,
    FE_VIDEO_CF11,
    FE_VIDEO_CF12
};

// Global variables (yuck!)
int dacnorm = 0;
long g_l_init_x = 0;
long g_l_init_y = 0;
int g_row_count = 0;
long g_save_base = 0;              // base clock ticks
long g_save_ticks = 0;             // save after this many ticks
int g_text_cbase = 0;
int g_text_col = 0;
int g_text_rbase = 0;
int g_text_row = 0;
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

int get_sound_params()
{
    // TODO
    _ASSERTE(FALSE);
    return 0;
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

unsigned long get_disk_space()
{
    ULARGE_INTEGER space;
    unsigned long result = 0;
    if (GetDiskFreeSpaceEx(nullptr, &space, nullptr, nullptr))
    {
        if (space.HighPart)
        {
            result = ~0UL;
        }
        else
        {
            result = space.LowPart;
        }
    }
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
    char minidump[MAX_PATH] = "fractint.dmp";
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
        MessageBox(nullptr, "An unexpected error occurred.  FractInt will now exit.",
                   "FractInt: Unexpected Error", MB_OK);
        return;
    }
    dumper = (MiniDumpWriteDumpProc *) GetProcAddress(debughlp, "MiniDumpWriteDump");

    while (PathFileExists(minidump))
    {
        std::sprintf(minidump, "fractint-%d.dmp", i++);
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
        MessageBox(nullptr, msg, "FractInt: Unexpected Error", MB_OK);
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
        extern int id_main(int argc, char *argv[]);
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

#if defined(USE_DRIVER_FUNCTIONS)
void set_normal_dot()
{
    dotwrite = driver_write_pixel;
    dotread = driver_read_pixel;
}
#else
static void driver_dot_write(int x, int y, int color)
{
    driver_write_pixel(x, y, color);
}

static int driver_dot_read(int x, int y)
{
    return driver_read_pixel(x, y);
}

void set_normal_dot()
{
    dotwrite = driver_dot_write;
    dotread = driver_dot_read;
}
#endif

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
    MessageBox(nullptr, message, "FractInt: Fatal Error", MB_OK);
}
