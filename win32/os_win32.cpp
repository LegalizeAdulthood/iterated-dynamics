#include "port.h"
#include "prototyp.h"

#include "calcfrac.h"
#include "cmdfiles.h"
#include "cmplx.h"
#include "diskvid.h"
#include "drivers.h"
#include "fractint.h"
#include "helpdefs.h"
#include "id_data.h"
#include "miscovl.h"
#include "mpmath.h"
#include "prompts2.h"
#include "realdos.h"
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

// External declarations
extern void check_same_name();

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
int g_color_dark = 0;       // darkest color in palette
int g_color_bright = 0;     // brightest color in palette
int g_color_medium = 0;     /* nearest to medbright grey in palette
                   Zoom-Box values (2K x 2K screens max) */
int dacnorm = 0;
int g_dac_count = 0;
bool fake_lut = false;
int g_fm_attack = 0;
int g_fm_decay = 0;
int g_fm_release = 0;
int g_fm_sustain = 0;
int g_fm_volume = 0;
int g_fm_wavetype = 0;
int g_hi_attenuation = 0;
long g_l_init_x = 0;
long g_l_init_y = 0;
int g_polyphony = 0;
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
/* g_video_table
 *
 *  |--Adapter/Mode-Name------|-------Comments-----------|
 *  |------INT 10H------|Dot-|--Resolution---|
 *  |key|--AX---BX---CX---DX|Mode|--X-|--Y-|Color|
 */
VIDEOINFO g_video_table[MAX_VIDEO_MODES] = { 0 };

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
; *************** Function find_special_colors ********************

;       Find the darkest and brightest colors in palette, and a medium
;       color which is reasonably bright and reasonably grey.
*/
void
find_special_colors()
{
    int maxb = 0;
    int minb = 9999;
    int med = 0;
    int maxgun, mingun;

    g_color_dark = 0;
    g_color_medium = 7;
    g_color_bright = 15;

    if (g_colors == 2)
    {
        g_color_medium = 1;
        g_color_bright = 1;
        return;
    }

    if (!(g_got_real_dac || fake_lut))
        return;

    for (int i = 0; i < g_colors; i++)
    {
        const int brt = (int) g_dac_box[i][0] + (int) g_dac_box[i][1] + (int) g_dac_box[i][2];
        if (brt > maxb)
        {
            maxb = brt;
            g_color_bright = i;
        }
        if (brt < minb)
        {
            minb = brt;
            g_color_dark = i;
        }
        if (brt < 150 && brt > 80)
        {
            mingun = (int) g_dac_box[i][0];
            maxgun = mingun;
            if ((int) g_dac_box[i][1] > (int) g_dac_box[i][0])
            {
                maxgun = (int) g_dac_box[i][1];
            }
            else
            {
                mingun = (int) g_dac_box[i][1];
            }
            if ((int) g_dac_box[i][2] > maxgun)
            {
                maxgun = (int) g_dac_box[i][2];
            }
            if ((int) g_dac_box[i][2] < mingun)
            {
                mingun = (int) g_dac_box[i][2];
            }
            if (brt - (maxgun - mingun) / 2 > med)
            {
                g_color_medium = i;
                med = brt - (maxgun - mingun) / 2;
            }
        }
    }
}

static HANDLE s_find_context = INVALID_HANDLE_VALUE;
static char s_find_base[MAX_PATH] = { 0 };
static WIN32_FIND_DATA s_find_data = { 0 };

/* fill_dta
 *
 * Use data in s_find_data to fill in DTA.filename, DTA.attribute and DTA.path
 */
#define DTA_FLAG(find_flag_, dta_flag_) \
    ((s_find_data.dwFileAttributes & find_flag_) ? dta_flag_ : 0)

static void fill_dta()
{
    DTA.path = s_find_base;
    DTA.path += s_find_data.cFileName;
    DTA.attribute = DTA_FLAG(FILE_ATTRIBUTE_DIRECTORY, SUBDIR) |
                    DTA_FLAG(FILE_ATTRIBUTE_SYSTEM, SYSTEM) |
                    DTA_FLAG(FILE_ATTRIBUTE_HIDDEN, HIDDEN);
    DTA.filename = s_find_data.cFileName;
}
#undef DTA_FLAG

/* fr_findfirst
 *
 * Fill in DTA.filename, DTA.path and DTA.attribute for the first file
 * matching the wildcard specification in path.  Return zero if a file
 * is found, or non-zero if a file was not found or an error occurred.
 */
int fr_findfirst(char const *path)       // Find 1st file (or subdir) meeting path/filespec
{
    if (s_find_context != INVALID_HANDLE_VALUE)
    {
        BOOL const result = FindClose(s_find_context);
        _ASSERTE(result);
    }
    SetLastError(0);
    s_find_context = FindFirstFile(path, &s_find_data);
    if (INVALID_HANDLE_VALUE == s_find_context)
    {
        DWORD const last_error = GetLastError();
        return last_error != 0 ? last_error : -1;
    }

    _ASSERTE(std::strlen(path) < NUM_OF(s_find_base));
    std::strcpy(s_find_base, path);
    {
        char *whack = std::strrchr(s_find_base, '\\');
        if (whack != nullptr)
        {
            whack[1] = 0;
        }
    }

    fill_dta();

    return 0;
}

/* fr_findnext
 *
 * Find the next file matching the wildcard search begun by fr_findfirst.
 * Fill in DTA.filename, DTA.path, and DTA.attribute
 */
int fr_findnext()
{
    _ASSERTE(INVALID_HANDLE_VALUE != s_find_context);
    if (!FindNextFile(s_find_context, &s_find_data))
    {
        DWORD code = GetLastError();
        _ASSERTE(ERROR_NO_MORE_FILES == code);
        return -1;
    }

    fill_dta();

    return 0;
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

/*
; *************** Function spindac(direction, rstep) ********************

;       Rotate the MCGA/VGA DAC in the (plus or minus) "direction"
;       in "rstep" increments - or, if "direction" is 0, just replace it.
*/
void spindac(int dir, int inc)
{
    if (g_colors < 16)
        return;
    if (g_is_true_color && g_true_mode != true_color_mode::default_color)
        return;
    if (dir != 0 && g_color_cycle_range_lo < g_colors && g_color_cycle_range_lo < g_color_cycle_range_hi)
    {
        int top = g_color_cycle_range_hi > g_colors ? g_colors - 1 : g_color_cycle_range_hi;
        unsigned char *dacbot = (unsigned char *) g_dac_box + 3*g_color_cycle_range_lo;
        int len = (top - g_color_cycle_range_lo)*3*sizeof(unsigned char);
        if (dir > 0)
        {
            for (int i = 0; i < inc; i++)
            {
                unsigned char tmp[3];
                std::memcpy(tmp, dacbot, 3*sizeof(unsigned char));
                std::memcpy(dacbot, dacbot + 3*sizeof(unsigned char), len);
                std::memcpy(dacbot + len, tmp, 3*sizeof(unsigned char));
            }
        }
        else
        {
            for (int i = 0; i < inc; i++)
            {
                unsigned char tmp[3];
                std::memcpy(tmp, dacbot + len, 3*sizeof(unsigned char));
                std::memcpy(dacbot + 3*sizeof(unsigned char), dacbot, len);
                std::memcpy(dacbot, tmp, 3*sizeof(unsigned char));
            }
        }
    }
    driver_write_palette();
    driver_delay(g_colors - g_dac_count - 1);
}

//; ************* function scroll_relative(bycol, byrow) ***********************
//
//; scroll_relative ------------------------------------------------------------
//; * relative screen center scrolling, arguments passed are signed deltas
//; ------------------------------------------------------------16-08-2002-ChCh-
//
//scroll_relative proc    bycol: word, byrow: word
//        cmp     g_video_scroll,0        ; is the scrolling on?
//        jne     okletsmove              ;  ok, lets move
//        jmp     staystill               ;  no, stay still
//okletsmove:
//        mov     cx,g_video_start_x         ; where we already are..
//        mov     dx,g_video_start_y
//        add     cx,video_cofs_x         ; find the screen center
//        add     dx,video_cofs_y
//        add     cx,bycol                ; add the relative shift
//        add     dx,byrow
//        call    VESAscroll              ; replace this later with a variable
//staystill:
//        ret
//scroll_relative endp

void scroll_relative(int bycol, int byrow)
{
    if (g_video_scroll)
    {
        _ASSERTE(0 && "scroll_relative called");
        // blt pixels around :-)
    }
}

/*
; **************** Function home()  ********************************

;       Home the cursor (called before printfs)
*/
void home()
{
    driver_move_cursor(0, 0);
    g_text_row = 0;
    g_text_col = 0;
}

bool isadirectory(char const *s)
{
    return PathIsDirectory(s) != 0;
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

/*
; ************* function scroll_center(tocol, torow) *************************

; scroll_center --------------------------------------------------------------
; * this is meant to be an universal scrolling redirection routine
;   (if scrolling will be coded for the other modes too, the VESAscroll
;   call should be replaced by a preset variable (like in proc newbank))
; * arguments passed are the coords of the screen center because
;   there is no universal way to determine physical screen resolution
; ------------------------------------------------------------12-08-2002-ChCh-
*/
void scroll_center(int tocol, int torow)
{
    // TODO
    _ASSERTE(FALSE);
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

/*
 * This routine returns a key, ignoring F1
 */
int getakeynohelp()
{
    int ch;
    do
    {
        ch = driver_get_key();
    }
    while (FIK_F1 == ch);
    return ch;
}

// converts relative path to absolute path
int expand_dirname(char *dirname, char *drive)
{
    if (PathIsRelative(dirname))
    {
        char relative[MAX_PATH];
        _ASSERTE(std::strlen(drive) < NUM_OF(relative));
        std::strcpy(relative, drive);
        _ASSERTE(std::strlen(relative) + std::strlen(dirname) < NUM_OF(relative));
        std::strcat(relative, dirname);
        char absolute[MAX_PATH];
        BOOL status = PathSearchAndQualify(relative, absolute, NUM_OF(absolute));
        _ASSERTE(status);
        if (':' == absolute[1])
        {
            drive[0] = absolute[0];
            drive[1] = absolute[1];
            drive[2] = 0;
            std::strcpy(dirname, &absolute[2]);
        }
        else
        {
            std::strcpy(dirname, absolute);
        }
    }
    fix_dirname(dirname);

    return 0;
}

bool abortmsg(char const *file, unsigned int line, int flags, char const *msg)
{
    char buffer[3*80];
    std::sprintf(buffer, "%s(%u):\n%s", file, line, msg);
    return stopmsg(flags, buffer);
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

/*
 *----------------------------------------------------------------------
 *
 * findpath --
 *
 *      Find where a file is.
 *  We return filename if it is an absolute path.
 *  Otherwise we first try FRACTDIR/filename, SRCDIR/filename,
 *      and then ./filename.
 *
 * Results:
 *      Returns full pathname in fullpathname.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */
void findpath(char const *filename, char *fullpathname) // return full pathnames
{
    fullpathname[0] = 0;                         // indicate none found

    char fname[FILE_MAX_FNAME];
    char ext[FILE_MAX_EXT];
    char temp_path[FILE_MAX_PATH];

    // check current directory if curdir= parameter set
    splitpath(filename, nullptr, nullptr, fname, ext);
    makepath(temp_path, "", "", fname, ext);
    if (g_check_cur_dir && access(temp_path, 0) == 0)   // file exists
    {
        std::strcpy(fullpathname, temp_path);
        return;
    }

    // check for absolute path
    std::strcpy(temp_path, filename);   // avoid side effect changes to filename
    if (temp_path[0] == SLASHC || (temp_path[0] && temp_path[1] == ':'))
    {
        if (access(temp_path, 0) == 0)   // file exists
        {
            std::strcpy(fullpathname, temp_path);
            return;
        }

        splitpath(temp_path, nullptr, nullptr, fname, ext);
        makepath(temp_path, "", "", fname, ext);
    }

    // check FRACTDIR
    makepath(temp_path, "", g_fractal_search_dir1, fname, ext);
    if (access(temp_path, 0) == 0)
    {
        std::strcpy(fullpathname, temp_path);
        return;
    }

    // check SRCDIR
    makepath(temp_path, "", g_fractal_search_dir2, fname, ext);
    if (access(temp_path, 0) == 0)
    {
        std::strcpy(fullpathname, temp_path);
        return;
    }

    // check PATH
    _searchenv(temp_path, "PATH", fullpathname);
    if (fullpathname[0] != 0)                    // found it!
    {
        if (std::strncmp(&fullpathname[2], SLASHSLASH, 2) == 0) // stupid klooge!
        {
            std::strcpy(&fullpathname[3], temp_path);
        }
    }
}

// case independent version of std::strncmp
int strncasecmp(char const *s, char const *t, int ct)
{
    for (; (std::tolower(*s) == std::tolower(*t)) && --ct ; s++, t++)
    {
        if (*s == '\0')
        {
            return 0;
        }
    }
    return std::tolower(*s) - std::tolower(*t);
}
