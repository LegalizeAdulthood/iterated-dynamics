#include <cassert>
#include <string>

#include <signal.h>
#include <string.h>
#include <sys/timeb.h>
#include <time.h>

#include <boost/format.hpp>

#define WIN32_LEAN_AND_MEAN
#define STRICT
#include <windows.h>
#include <shlwapi.h>
#include <dbghelp.h>

#include "port.h"
#include "prototyp.h"
#include "cmplx.h"
#include "id.h"
#include "externs.h"
#include "helpdefs.h"

#include "calcfrac.h"
#include "diskvid.h"
#include "drivers.h"
#include "fihelp.h"
#include "filesystem.h"
#include "mpmath.h"
#include "prompts2.h"
#include "realdos.h"
#include "StopMessage.h"

#include "frame.h"

// External declarations
extern void check_samename();

HINSTANCE g_instance = 0;

static void (*s_dot_write)(int, int, int) = 0;
static int (*s_dot_read)(int, int) = 0;
static void (*s_line_write)(int, int, int, BYTE const *) = 0;
static void (*s_line_read)(int, int, int, BYTE *) = 0;

typedef enum
{
	FE_UNKNOWN = -1,
	FE_IMAGE_INFO,					// TAB
	FE_RESTART,						// INSERT
	FE_SELECT_VIDEO_MODE,			// DELETE
	FE_EXECUTE_COMMANDS,			// @
	FE_COMMAND_SHELL,				// d
	FE_ORBITS_WINDOW,				// o
	FE_SELECT_FRACTAL_TYPE,			// t
	FE_TOGGLE_JULIA,				// IDK_SPACE
	FE_TOGGLE_INVERSE,				// j
	FE_PRIOR_IMAGE,					// h
	FE_REVERSE_HISTORY,				// ^H
	FE_BASIC_OPTIONS,				// x
	FE_EXTENDED_OPTIONS,			// y
	FE_TYPE_SPECIFIC_PARAMS,		// z
	FE_PASSES_OPTIONS,				// p
	FE_VIEW_WINDOW_OPTIONS,			// v
	FE_3D_PARAMS,					// i
	FE_BROWSE_PARAMS,				// ^B
	FE_EVOLVER_PARAMS,				// ^E
	FE_SOUND_PARAMS,				// ^F
	FE_SAVE_IMAGE,					// s
	FE_LOAD_IMAGE,					// r
	FE_3D_TRANSFORM,				// 3
	FE_3D_OVERLAY,					// #
	FE_SAVE_CURRENT_PARAMS,			// b
	FE_PRINT_IMAGE,					// ^P
	FE_GIVE_COMMAND_STRING,			// g
	FE_QUIT,						// ESC
	FE_COLOR_CYCLING_MODE,			// c
	FE_ROTATE_PALETTE_DOWN,			// -
	FE_ROTATE_PALETTE_UP,			// +
	FE_EDIT_PALETTE,				// e
	FE_MAKE_STARFIELD,				// a
	FE_ANT_AUTOMATON,				// ^A
	FE_STEREOGRAM,					// ^S
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
} fractint_event;

// Global variables (yuck!)
int g_overflow_mp = 0;
BYTE g_block[4096] = { 0 };
bool g_disk_flag = false;
bool g_disk_targa = false;
int g_is_true_color = 0;
long g_initial_x_l = 0;
long g_initial_y_l = 0;
bool g_overflow = false;
int g_polyphony = 0;
char g_rle_buffer[258] = { 0 };
int g_row_count = 0;
unsigned int g_string_location[10*1024] = { 0 };
int g_text_cbase = 0;
int g_text_col = 0;
int g_text_rbase = 0;
int g_text_row = 0;
char g_text_stack[4096] = { 0 };
int g_vx_dots = 0;

/* Global functions
 *
 * These were either copied from a .c file under unix, or
 * they have assembly language equivalents that we provide
 * here in a slower C form for portability.
 */

/* keyboard_event
**
** Map a keypress into an event id.
*/
static fractint_event keyboard_event(int key)
{
	struct
	{
		int key;
		fractint_event event;
	}
	mapping[] =
	{
		IDK_CTL_A,	FE_ANT_AUTOMATON,
		IDK_CTL_B,	FE_BROWSE_PARAMS,
		IDK_CTL_E,	FE_EVOLVER_PARAMS,
		IDK_CTL_F,	FE_SOUND_PARAMS,
		IDK_BACKSPACE,	FE_REVERSE_HISTORY,
		IDK_TAB,		FE_IMAGE_INFO,
		IDK_CTL_S,	FE_STEREOGRAM,
		IDK_ESC,		FE_QUIT,
		IDK_SPACE,	FE_TOGGLE_JULIA,
		IDK_INSERT,		FE_RESTART,
		DELETE,		FE_SELECT_VIDEO_MODE,
		'@',		FE_EXECUTE_COMMANDS,
		'#',		FE_3D_OVERLAY,
		'3',		FE_3D_TRANSFORM,
		'a',		FE_MAKE_STARFIELD,
		'b',		FE_SAVE_CURRENT_PARAMS,
		'c',		FE_COLOR_CYCLING_MODE,
		'd',		FE_COMMAND_SHELL,
		'e',		FE_EDIT_PALETTE,
		'g',		FE_GIVE_COMMAND_STRING,
		'h',		FE_PRIOR_IMAGE,
		'j',		FE_TOGGLE_INVERSE,
		'i',		FE_3D_PARAMS,
		'o',		FE_ORBITS_WINDOW,
		'p',		FE_PASSES_OPTIONS,
		'r',		FE_LOAD_IMAGE,
		's',		FE_SAVE_IMAGE,
		't',		FE_SELECT_FRACTAL_TYPE,
		'v',		FE_VIEW_WINDOW_OPTIONS,
		'x',		FE_BASIC_OPTIONS,
		'y',		FE_EXTENDED_OPTIONS,
		'z',		FE_TYPE_SPECIFIC_PARAMS,
		'-',		FE_ROTATE_PALETTE_DOWN,
		'+',		FE_ROTATE_PALETTE_UP
	};
	key = tolower(key);
	{
		int i;
		for (i = 0; i < NUM_OF(mapping); i++)
		{
			if (mapping[i].key == key)
			{
				return mapping[i].event;
			}
		}
	}

	return FE_UNKNOWN;
}

/*
;
;       32-bit integer divide routine with an 'n'-bit shift.
;       Overflow condition returns 0x7fffh with g_overflow = 1;
;
;       z = divide(x, y, n);       z = x/y;
*/
long divide(long x, long y, int n)
{
	return long((float(x))/(float(y))*float(1 << n));
}

/*
;
;       32-bit integer multiply routine with an 'n'-bit shift.
;       Overflow condition returns 0x7fffh with g_overflow = 1;
;
;       long x, y, z, multiply();
;       int n;
;
;       z = multiply(x, y, n)
;
*/

/*
 * 32 bit integer multiply with n bit shift.
 * Note that we fake integer multiplication with floating point
 * multiplication.
 */
long multiply(long x, long y, int n)
{
	register long l;
	l = long((float(x))*(float(y))/float(1 << n));
	if (l == 0x7fffffff)
	{
		g_overflow = true;
	}
	return l;
}

static HANDLE s_find_context = INVALID_HANDLE_VALUE;
static char s_find_base[MAX_PATH] = { 0 };
static WIN32_FIND_DATA s_find_data = { 0 };

/* fill_dta
 *
 * Use data in s_find_data to fill in g_dta.filename, g_dta.attribute and g_dta.path
 */
#define DTA_FLAG(find_flag_, dta_flag_) \
	((s_find_data.dwFileAttributes & find_flag_) ? dta_flag_ : 0)

static void fill_dta()
{
	g_dta.path = std::string(s_find_base) + s_find_data.cFileName;
	g_dta.attribute = DTA_FLAG(FILE_ATTRIBUTE_DIRECTORY, SUBDIR) |
		DTA_FLAG(FILE_ATTRIBUTE_SYSTEM, SYSTEM) |
		DTA_FLAG(FILE_ATTRIBUTE_HIDDEN, HIDDEN);
	g_dta.filename = s_find_data.cFileName;
}
#undef DTA_FLAG

/* fr_find_first
 *
 * Fill in g_dta.filename, g_dta.path and g_dta.attribute for the first file
 * matching the wildcard specification in path.  Return zero if a file
 * is found, or non-zero if a file was not found or an error occurred.
 */
int fr_find_first(char *path)       // Find 1st file (or subdir) meeting path/filespec
{
	if (s_find_context != INVALID_HANDLE_VALUE)
	{
		BOOL result = FindClose(s_find_context);
		assert(result || !"::FindClose failed");
	}
	SetLastError(0);
	s_find_context = FindFirstFile(path, &s_find_data);
	if (INVALID_HANDLE_VALUE == s_find_context)
	{
		return GetLastError() || -1;
	}

	assert(strlen(path) < NUM_OF(s_find_base) || !"path exceeds available storage");
	strcpy(s_find_base, path);
	{
		char *whack = strrchr(s_find_base, '\\');
		if (whack != 0)
		{
			whack[1] = 0;
		}
	}

	fill_dta();

	return 0;
}

/* fr_find_next
 *
 * Find the next file matching the wildcard search begun by fr_find_first.
 * Fill in g_dta.filename, g_dta.path, and g_dta.attribute
 */
int fr_find_next()
{
	assert(INVALID_HANDLE_VALUE != s_find_context || !"find context corrupted");
	BOOL result = FindNextFile(s_find_context, &s_find_data);
	if (result == 0)
	{
		DWORD code = GetLastError();
		assert(ERROR_NO_MORE_FILES == code || !"unexpected error from FindNextFile");
		return -1;
	}

	fill_dta();

	return 0;
}

/*
; long read_ticker() returns current bios ticker value
*/
long read_ticker()
{
	return long(GetTickCount());
}

/*
; *************** Function spin_dac(direction, rstep) ********************

;       Rotate the MCGA/VGA DAC in the (plus or minus) "direction"
;       in "rstep" increments - or, if "direction" is 0, just replace it.
*/
void spin_dac(int dir, int inc)
{
	if (g_is_true_color && g_true_mode_iterates)
	{
		return;
	}

	if ((dir != 0) && (g_rotate_lo < g_colors) && (g_rotate_lo < g_rotate_hi))
	{
		int top = (g_rotate_hi > g_colors) ? g_colors - 1 : g_rotate_hi;
		if (dir > 0)
		{
			for (int i = 0; i < inc; i++)
			{
				BYTE tmp[3];

				tmp[0] = g_.DAC().Red(g_rotate_lo);
				tmp[1] = g_.DAC().Green(g_rotate_lo);
				tmp[2] = g_.DAC().Blue(g_rotate_lo);
				for (int j = g_rotate_lo; j < top; j++)
				{
					g_.DAC().Set(j, g_.DAC().Red(j + 1), g_.DAC().Green(j + 1), g_.DAC().Blue(j + 1));
				}
				g_.DAC().Set(top, tmp[0], tmp[1], tmp[2]);
			}
		}
		else
		{
			for (int i = 0; i < inc; i++)
			{
				BYTE tmp[3];

				tmp[0] = g_.DAC().Red(top);
				tmp[1] = g_.DAC().Green(top);
				tmp[2] = g_.DAC().Blue(top);
				for (int j = top; j > g_rotate_lo; j--)
				{
					g_.DAC().Set(j, g_.DAC().Red(j-1), g_.DAC().Green(j-1), g_.DAC().Blue(j-1));
				}
				g_.DAC().Set(g_rotate_lo, tmp[0], tmp[1], tmp[2]);
			}
		}
	}
	driver_write_palette();
	driver_delay(g_colors - g_.DACSleepCount() - 1);
}

void load_dac()
{
	spin_dac(0, 1);
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
**  microseconds past midnight. Analogous to CLK_TCK is UCLK_TCK, the
**  number which a usec_clock() reading must be divided by to yield
**  a number of seconds.
*/
typedef unsigned long uclock_t;
uclock_t usec_clock()
{
	uclock_t result = 0;
	// TODO
	assert(!"usec_clock unexpectedly called");

	return result;
}

void showfreemem()
{
	// TODO
	assert(!"showfreemem unexpectedly called");
}

unsigned long get_disk_space()
{
	ULARGE_INTEGER space;
	unsigned long result = 0;
	if (GetDiskFreeSpaceEx(0, &space, 0, 0))
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
	MiniDumpWriteDumpProc *dumper = 0;
	HMODULE debughlp = LoadLibrary("dbghelp.dll");
	std::string minidump = "fractint.dmp";
	MINIDUMP_EXCEPTION_INFORMATION mdei =
	{
		GetCurrentThreadId(),
		ep,
		FALSE
	};
	HANDLE dump_file;
	BOOL status = 0;
	int i = 1;

	if (debughlp == 0)
	{
		MessageBox(0, "An unexpected error occurred.  FractInt will now exit.",
			"FractInt: Unexpected Error", MB_OK);
		return;
	}
	dumper = (MiniDumpWriteDumpProc *) GetProcAddress(debughlp, "MiniDumpWriteDump");

	while (PathFileExists(minidump.c_str()))
	{
		minidump = str(boost::format("fractint-%d.dmp") % i++);
	}
	dump_file = CreateFile(minidump.c_str(), GENERIC_READ | GENERIC_WRITE,
		0, 0, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, 0);
	assert(dump_file != INVALID_HANDLE_VALUE || !"CreateFile failed");

	status = (*dumper)(GetCurrentProcess(), GetCurrentProcessId(),
		dump_file, MiniDumpNormal, &mdei, 0, 0);
	assert(status || !"dumper failed");
	if (!status)
	{
		MessageBox(0,
			str(boost::format("MiniDumpWriteDump failed with %08x") % GetLastError()).c_str(),
			"Ugh", MB_OK);
	}
	else
	{
		status = CloseHandle(dump_file);
		assert(status || !"CloseHandle failed");
	}
	dumper = 0;
	status = FreeLibrary(debughlp);
	assert(status || !"FreeLibrary failed");

	{
		MessageBox(0, ("Unexpected error, crash dump saved to '" + minidump + "'.\n"
			"Please include this file with your bug report.").c_str(),
			"FractInt: Unexpected Error", MB_OK);
	}
}

int __stdcall WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR cmdLine, int show)
{
	int result = 0;

#if !defined(_DEBUG)
	__try
#endif
	{
		g_instance = instance;
		_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_LEAK_CHECK_DF);
		result = application_main(__argc, __argv);
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
int get_key_no_help()
{
	int old_help_mode = get_help_mode();
	set_help_mode(0);
	int ch;
	do
	{
		ch = driver_get_key();
	}
	while (IDK_F1 == ch);
	set_help_mode(old_help_mode);
	return ch;
}

// converts relative path to absolute path
int expand_dirname(char *dirname, char *drive)
{
	char relative[MAX_PATH];
	char absolute[MAX_PATH];
	BOOL status;

	if (PathIsRelative(dirname))
	{
		assert(strlen(drive) < NUM_OF(relative) || !"insufficient storage for filename");
		strcpy(relative, drive);
		assert(strlen(relative) + strlen(dirname) < NUM_OF(relative) || !"insufficient storage for filename");
		strcat(relative, dirname);
		status = PathSearchAndQualify(relative, absolute, NUM_OF(absolute));
		assert(status || !"PathSearchAndQualify failed");
		if (':' == absolute[1])
		{
			drive[0] = absolute[0];
			drive[1] = absolute[1];
			drive[2] = 0;
			strcpy(dirname, &absolute[2]);
		}
		else
		{
			strcpy(dirname, absolute);
		}
	}
	ensure_slash_on_directory(dirname);

	return 0;
}

int abort_message(const char *file, unsigned int line, int flags, const char *msg)
{
	return stop_message(flags, str(boost::format("%s(%d):\n%s") % file % line % msg));
}

/* ods
 *
 * varargs version of OutputDebugString with file and line markers.
 */
void ods(const char *file, unsigned int line, const char *format, ...)
{
	char full_msg[MAX_PATH + 1];
	char app_msg[MAX_PATH + 1];
	va_list args;

	va_start(args, format);
	_vsnprintf(app_msg, MAX_PATH, format, args);
	_snprintf(full_msg, MAX_PATH, "%s(%d): %s\n", file, line, app_msg);
	va_end(args);

	OutputDebugString(full_msg);
}

/*
; ***Function get_line(int row, int startcol, int stopcol, unsigned char *pixels)

;       This routine is a 'line' analog of 'get_color()', and gets a segment
;       of a line from the screen and stores it in pixels[] at one byte per
;       pixel
;       Called by the GIF decoder
*/

void get_line(int row, int startcol, int stopcol, BYTE *pixels)
{
	if (startcol + g_screen_x_offset >= g_screen_width || row + g_screen_y_offset >= g_screen_height)
	{
		return;
	}
	assert(s_line_read && "s_line_read function pointer not set");
	s_line_read(row + g_screen_y_offset, startcol + g_screen_x_offset, stopcol + g_screen_x_offset, pixels);
}

/*
; ***Function put_line(int row, int startcol, int stopcol, unsigned char *pixels)

;       This routine is a 'line' analog of 'g_plot_color_put_color()', and puts a segment
;       of a line from the screen and stores it in pixels[] at one byte per
;       pixel
;       Called by the GIF decoder
*/

void put_line(int row, int startcol, int stopcol, BYTE const *pixels)
{
	if (startcol + g_screen_x_offset >= g_screen_width || row + g_screen_y_offset > g_screen_height)
	{
		return;
	}
	assert(s_line_write && "s_line_write function pointer not set");
	s_line_write(row + g_screen_y_offset, startcol + g_screen_x_offset, stopcol + g_screen_x_offset, pixels);
}

/*
; **************** internal Read/Write-a-line routines *********************
;
;       These routines are called by out_line(), put_line() and get_line().
*/
static void normal_line_write(int y, int x, int lastx, BYTE const *pixels)
{
	int i, width;
	width = lastx - x + 1;
	assert(s_dot_write || !"s_dot_write function pointer not set");
	for (i = 0; i < width; i++)
	{
		s_dot_write(x + i, y, pixels[i]);
	}
}

static void normal_line_read(int y, int x, int lastx, BYTE *pixels)
{
	int i, width;
	width = lastx - x + 1;
	assert(s_dot_read || !"s_dot_read function pointer not set");
	for (i = 0; i < width; i++)
	{
		pixels[i] = s_dot_read(x + i, y);
	}
}

#if defined(USE_DRIVER_FUNCTIONS)
void set_normal_dot()
{
	s_dot_write = driver_write_pixel;
	s_dot_read = driver_read_pixel;
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
	s_dot_write = driver_dot_write;
	s_dot_read = driver_dot_read;
}
#endif

void set_disk_dot()
{
	s_dot_write = disk_write;
	s_dot_read = disk_read;
}

void set_normal_line()
{
	s_line_read = normal_line_read;
	s_line_write = normal_line_write;
}

static void null_write(int a, int b, int c)
{
	assert(!"null_write unexpectedly called");
}

static int null_read(int a, int b)
{
	assert(!"null_read unexpectedly called");
	return 0;
}

// from video.asm
void set_null_video()
{
	assert(!"setnullvideo unexpectedly called");
	s_dot_write = null_write;
	s_dot_read = null_read;
}

/*
; **************** Function get_color(xdot, ydot) *******************

;       Return the color on the screen at the (xdot, ydot) point
*/
int get_color(int xdot, int ydot)
{
	int x1, y1;
	x1 = xdot + g_screen_x_offset;
	y1 = ydot + g_screen_y_offset;
	assert(x1 >= 0 && x1 <= g_screen_width || !"x1 out of bounds");
	assert(y1 >= 0 && y1 <= g_screen_height || "y1 out of bounds");
	if (x1 < 0 || y1 < 0 || x1 >= g_screen_width || y1 >= g_screen_height)
	{
		return 0;
	}
	assert(s_dot_read || !"s_dot_read function pointer not set");
	return s_dot_read(x1, y1);
}

/*
; ************** Function putcolor_a(xdot, ydot, color) *******************

;       write the color on the screen at the (xdot, ydot) point
*/
void putcolor_a(int xdot, int ydot, int color)
{
	int x1 = xdot + g_screen_x_offset;
	int y1 = ydot + g_screen_y_offset;
	assert(x1 >= 0 && x1 <= g_screen_width || !"x1 out of bounds");
	assert(y1 >= 0 && y1 <= g_screen_height || !"y1 out of bounds");
	assert(s_dot_write || !"s_dot_write function pointer not set");
	s_dot_write(x1, y1, color & g_and_color);
}

/*
; ***************Function out_line(pixels, linelen) *********************

;       This routine is a 'line' analog of 'g_plot_color_put_color()', and sends an
;       entire line of pixels to the screen (0 <= xdot < g_x_dots) at a clip
;       Called by the GIF decoder
*/
int out_line(BYTE const *pixels, int linelen)
{
	assert(_CrtCheckMemory());
	if (g_row_count + g_screen_y_offset >= g_screen_height)
	{
		return 0;
	}
	assert(s_line_write || !"s_line_write function pointer not set");
	s_line_write(g_row_count + g_screen_y_offset, g_screen_x_offset, linelen + g_screen_x_offset - 1, pixels);
	g_row_count++;
	return 0;
}

void init_failure(std::string const &message)
{
	MessageBox(0, message.c_str(), "FractInt: Fatal Error", MB_OK);
}
