/* d_win32.c
 *
 * Routines for a Win32 GDI driver for fractint.
 */
#include <assert.h>
#include <stdio.h>

#define WIN32_LEAN_AND_MEAN
#define STRICT
#include <windows.h>

#include "port.h"
#include "prototyp.h"
#include "fractype.h"
#include "helpdefs.h"
#include "drivers.h"

#include "WinText.h"
#include "frame.h"
#include "plot.h"
#include "ods.h"

extern int windows_delay(int ms);

extern HINSTANCE g_instance;

#define MAXSCREENS 10
#define DRAW_INTERVAL 6
#define TIMER_ID 1

#define NUM_OF(ary_) (sizeof(ary_)/sizeof(ary_[0]))

#define DI(name_) Win32Driver *name_ = (Win32Driver *) drv

typedef struct tagWin32Driver Win32Driver;
struct tagWin32Driver
{
	Driver pub;

	Frame frame;
	Plot plot;
	WinText wintext;

	BOOL text_not_graphics;

	/* key_buffer
	*
	* When we peeked ahead and saw a keypress, stash it here for later
	* feeding to our caller.
	*/
	int key_buffer;

	int screen_count;
	BYTE *saved_screens[MAXSCREENS];
	int saved_cursor[MAXSCREENS+1];
	BOOL cursor_shown;
	int cursor_row;
	int cursor_col;
};

/* VIDEOINFO:															*/
/*         char    name[26];       Adapter name (IBM EGA, etc)          */
/*         char    comment[26];    Comments (UNTESTED, etc)             */
/*         int     keynum;         key number used to invoked this mode */
/*                                 2-10 = F2-10, 11-40 = S,C,A{F1-F10}  */
/*         int     videomodeax;    begin with INT 10H, AX=(this)        */
/*         int     videomodebx;                 ...and BX=(this)        */
/*         int     videomodecx;                 ...and CX=(this)        */
/*         int     videomodedx;                 ...and DX=(this)        */
/*                                 NOTE:  IF AX==BX==CX==0, SEE BELOW   */
/*         int     dotmode;        video access method used by asm code */
/*                                      1 == BIOS 10H, AH=12,13 (SLOW)  */
/*                                      2 == access like EGA/VGA        */
/*                                      3 == access like MCGA           */
/*                                      4 == Tseng-like  SuperVGA*256   */
/*                                      5 == P'dise-like SuperVGA*256   */
/*                                      6 == Vega-like   SuperVGA*256   */
/*                                      7 == "Tweaked" IBM-VGA ...*256  */
/*                                      8 == "Tweaked" SuperVGA ...*256 */
/*                                      9 == Targa Format               */
/*                                      10 = Hercules                   */
/*                                      11 = "disk video" (no screen)   */
/*                                      12 = 8514/A                     */
/*                                      13 = CGA 320x200x4, 640x200x2   */
/*                                      14 = Tandy 1000                 */
/*                                      15 = TRIDENT  SuperVGA*256      */
/*                                      16 = Chips&Tech SuperVGA*256    */
/*         int     xdots;          number of dots across the screen     */
/*         int     ydots;          number of dots down the screen       */
/*         int     colors;         number of colors available           */

#define DRIVER_MODE(name_, comment_, key_, width_, height_, mode_) \
	{ name_, comment_, key_, 0, 0, 0, 0, mode_, width_, height_, 256 }
#define MODE19(n_, c_, k_, w_, h_) DRIVER_MODE(n_, c_, k_, w_, h_, 19)
#define MODE27(n_, c_, k_, w_, h_) DRIVER_MODE(n_, c_, k_, w_, h_, 27)
#define MODE28(n_, c_, k_, w_, h_) DRIVER_MODE(n_, c_, k_, w_, h_, 28)
static VIDEOINFO modes[] =
{
	MODE19("Win32 Disk Video         ", "                        ", 0,  320,  200),
	MODE19("Win32 Disk Video         ", "                        ", 0,  320,  400),
	MODE19("Win32 Disk Video         ", "                        ", 0,  360,  480),
	MODE19("Win32 Disk Video         ", "                        ", 0,  640,  400),
	MODE19("Win32 Disk Video         ", "                        ", 0,  640,  480),
	MODE27("Win32 Disk Video         ", "                        ", 0,  800,  600),
	MODE27("Win32 Disk Video         ", "                        ", 0,  1024, 768),
	MODE28("Win32 Disk Video         ", "                        ", 0,  1280, 1024),
	MODE28("Win32 Disk Video         ", "                        ", 0, 1024, 1024),
	MODE28("Win32 Disk Video         ", "                        ", 0, 1600, 1200),
	MODE28("Win32 Disk Video         ", "                        ", 0, 2048, 2048),
	MODE28("Win32 Disk Video         ", "                        ", 0, 4096, 4096),
	MODE28("Win32 Disk Video         ", "                        ", 0, 8192, 8192)
};
#undef MODE28
#undef MODE27
#undef MODE19
#undef DRIVER_MODE

/* check_arg
 *
 *	See if we want to do something with the argument.
 *
 * Results:
 *	Returns 1 if we parsed the argument.
 *
 * Side effects:
 *	Increments i if we use more than 1 argument.
 */
static int
check_arg(Win32Driver *di, char *arg)
{
#if 0
	if (strcmp(arg, "-disk") == 0)
	{
		return 1;
	}
	else if (strcmp(arg, "-simple") == 0)
	{
		di->simple_input = 1;
		return 1;
	}
	else if (strcmp(arg, "-geometry") == 0 && *i+1 < argc)
	{
		di->Xgeometry = argv[(*i)+1];
		(*i)++;
		return 1;
	}
#endif

	return 0;
}

/* handle_help_tab
 *
 * Because we want context sensitive help to work everywhere, with the
 * help to display indicated by a non-zero value in helpmode, we need
 * to trap the F1 key at a very low level.  The same is true of the
 * TAB display.
 *
 * What we do here is check for these keys and invoke their displays.
 * To avoid a recursive invoke of help(), a static is used to avoid
 * recursing on ourselves as help will invoke get key!
 */
static int
handle_help_tab(int ch)
{
	static int inside_help = 0;

	if (FIK_F1 == ch && helpmode && !inside_help)
	{
		inside_help = 1;
		help(0);
		inside_help = 0;
		ch = 0;
	}
	else if (FIK_TAB == ch && tabmode)
	{
		int old_tab = tabmode;
		tabmode = 0;
		tab_display();
		tabmode = old_tab;
		ch = 0;
	}

	return ch;
}

static void
parse_geometry(const char *spec, int *x, int *y, int *width, int *height)
{
	/* do something like XParseGeometry() */
	if (2 == sscanf(spec, "%dx%d", width, height))
	{
		/* all we care about is width and height for disk output */
		*x = 0;
		*y = 0;
	}
}

static void
show_hide_windows(HWND show, HWND hide)
{
	ShowWindow(show, SW_NORMAL);
	ShowWindow(hide, SW_HIDE);
}

static void
max_size(Win32Driver *di, int *width, int *height, BOOL *center_graphics)
{
	*center_graphics = TRUE;
	*width = di->wintext.max_width;
	*height = di->wintext.max_height;
	if (xdots > *width)
	{
		*width = xdots;
		*center_graphics = FALSE;
	}
	if (ydots > *height)
	{
		*height = ydots;
		*center_graphics = FALSE;
	}
}

static void center_windows(Win32Driver *di, BOOL center_graphics)
{
	HWND center, zero;
	int width, height;
	BOOL status;

	if (center_graphics)
	{
		zero = di->wintext.hWndCopy;
		center = di->plot.window;
		width = di->plot.width;
		height = di->plot.height;
	}
	else
	{
		zero = di->plot.window;
		center = di->wintext.hWndCopy;
		width = di->wintext.max_width;
		height = di->wintext.max_height;
	}

	status = SetWindowPos(zero, NULL, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
	_ASSERTE(status);
	{
		int x = (g_frame.width - width)/2;
		int y = (g_frame.height - height)/2;
		status = SetWindowPos(center, NULL, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
	}
	_ASSERTE(status);
}

/***********************************************************************
////////////////////////////////////////////////////////////////////////
\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
***********************************************************************/

/*----------------------------------------------------------------------
*
* win32_terminate --
*
*	Cleanup windows and stuff.
*
* Results:
*	None.
*
* Side effects:
*	Cleans up.
*
*----------------------------------------------------------------------
*/
static void
win32_terminate(Driver *drv)
{
	DI(di);
	ODS("win32_terminate");

	plot_terminate(&di->plot);
	wintext_destroy(&di->wintext);
	{
		int i;
		for (i = 0; i < NUM_OF(di->saved_screens); i++)
		{
			if (NULL != di->saved_screens[i])
			{
				free(di->saved_screens[i]);
				di->saved_screens[i] = NULL;
			}
		}
	}
}

/*----------------------------------------------------------------------
*
* win32_init --
*
*	Initialize the windows and stuff.
*
* Results:
*	None.
*
* Side effects:
*	Initializes windows.
*
*----------------------------------------------------------------------
*/
static int
win32_init(Driver *drv, int *argc, char **argv)
{
	LPCSTR title = "FractInt for Windows";
	DI(di);

	ODS("win32_init");
	frame_init(g_instance, title);
	if (!wintext_initialize(&di->wintext, g_instance, NULL, "Text"))
	{
		return FALSE;
	}
	plot_init(&di->plot, g_instance, "Plot");

	/* filter out driver arguments */
	{
		int i;

		for (i = 0; i < *argc; i++)
		{
			if (check_arg(di, argv[i]))
			{
				int j;
				for (j = i; j < *argc-1; j++)
				{
					argv[j] = argv[j+1];
				}
				argv[j] = NULL;
				--*argc;
			}
		}
	}

	/* add default list of video modes */
	{
		int m;
		for (m = 0; m < NUM_OF(modes); m++)
		{
			add_video_mode(drv, &modes[m]);
		}
	}

	return TRUE;
}

/* win32_resize
 *
 * Check if we need resizing.  If no, return 0.
 * If yes, resize internal buffers and return 1.
 */
static int
win32_resize(Driver *drv)
{
	DI(di);
	int width, height;
	BOOL center_graphics;

	if ((xdots == di->plot.width) && (ydots == di->plot.height))
	{
		return 0;
	}

	max_size(di, &width, &height, &center_graphics);
	frame_resize(width, height);
	plot_resize(&di->plot);
	center_windows(di, center_graphics);
	return 1;
}


/*----------------------------------------------------------------------
* win32_read_palette
*
*	Reads the current video palette into g_dac_box.
*	
*
* Results:
*	None.
*
* Side effects:
*	Fills in g_dac_box.
*
*----------------------------------------------------------------------
*/
static int
win32_read_palette(Driver *drv)
{
	DI(di);
	return plot_read_palette(&di->plot);
}

/*
*----------------------------------------------------------------------
*
* win32_write_palette --
*	Writes g_dac_box into the video palette.
*	
*
* Results:
*	None.
*
* Side effects:
*	Changes the displayed colors.
*
*----------------------------------------------------------------------
*/
static int
win32_write_palette(Driver *drv)
{
	DI(di);
	return plot_write_palette(&di->plot);
}

/*
*----------------------------------------------------------------------
*
* win32_schedule_alarm --
*
*	Start the refresh alarm
*
* Results:
*	None.
*
* Side effects:
*	Starts the alarm.
*
*----------------------------------------------------------------------
*/
static void
win32_schedule_alarm(Driver *drv, int soon)
{
	DI(di);
	soon = (soon ? 1 : DRAW_INTERVAL)*1000;
	if (di->text_not_graphics)
	{
		wintext_schedule_alarm(&di->wintext, soon);
	}
	else
	{
		plot_schedule_alarm(&di->plot, soon);
	}
}

/*
*----------------------------------------------------------------------
*
* win32_write_pixel --
*
*	Write a point to the screen
*
* Results:
*	None.
*
* Side effects:
*	Draws point.
*
*----------------------------------------------------------------------
*/
static void 
win32_write_pixel(Driver *drv, int x, int y, int color)
{
	DI(di);
	plot_write_pixel(&di->plot, x, y, color);
}

/*
*----------------------------------------------------------------------
*
* win32_read_pixel --
*
*	Read a point from the screen
*
* Results:
*	Value of point.
*
* Side effects:
*	None.
*
*----------------------------------------------------------------------
*/
static int
win32_read_pixel(Driver *drv, int x, int y)
{
	DI(di);
	return plot_read_pixel(&di->plot, x, y);
}

/*
*----------------------------------------------------------------------
*
* win32_write_span --
*
*	Write a line of pixels to the screen.
*
* Results:
*	None.
*
* Side effects:
*	Draws pixels.
*
*----------------------------------------------------------------------
*/
static void
win32_write_span(Driver *drv, int y, int x, int lastx, BYTE *pixels)
{
	DI(di);
	plot_write_span(&di->plot, x, y, lastx, pixels);
}

/*
*----------------------------------------------------------------------
*
* win32_read_span --
*
*	Reads a line of pixels from the screen.
*
* Results:
*	None.
*
* Side effects:
*	Gets pixels
*
*----------------------------------------------------------------------
*/
static void
win32_read_span(Driver *drv, int y, int x, int lastx, BYTE *pixels)
{
	DI(di);
	plot_read_span(&di->plot, y, x, lastx, pixels);
}

static void
win32_set_line_mode(Driver *drv, int mode)
{
	DI(di);
	plot_set_line_mode(&di->plot, mode);
}

static void
win32_draw_line(Driver *drv, int x1, int y1, int x2, int y2, int color)
{
	DI(di);
	plot_draw_line(&di->plot, x1, y1, x2, y2, color);
}

/*
*----------------------------------------------------------------------
*
* win32_redraw --
*
*	Refresh the screen.
*
* Results:
*	None.
*
* Side effects:
*	Redraws the screen.
*
*----------------------------------------------------------------------
*/
static void
win32_redraw(Driver *drv)
{
	DI(di);
	ODS("win32_redraw");
	if (di->text_not_graphics)
	{
		wintext_paintscreen(&di->wintext, 0, 80, 0, 25);
	}
	else
	{
		plot_redraw(&di->plot);
	}
	frame_pump_messages(FALSE);
}

/* win32_key_pressed
 *
 * Return 0 if no key has been pressed, or the FIK value if it has.
 * driver_get_key() must still be called to eat the key; this routine
 * only peeks ahead.
 *
 * When a keystroke has been found by the underlying wintext_xxx
 * message pump, stash it in the one key buffer for later use by
 * get_key.
 */
static int
win32_key_pressed(Driver *drv)
{
	DI(di);
	int ch = di->key_buffer;

	plot_flush(&di->plot);
	frame_pump_messages(FALSE);
	if (ch)
	{
		return ch;
	}
	ch = frame_get_key_press(0);
	if (ch)
	{
		di->key_buffer = handle_help_tab(ch);
	}

	return ch;
}

/* win32_unget_key
 *
 * Unread a key!  The key buffer is only one character deep, so we
 * assert if its already full.  This should never happen in real life :-).
 */
void win32_unget_key(Driver *drv, int key)
{
	DI(di);
	_ASSERTE(0 == di->key_buffer);
	di->key_buffer = key;
}

/* win32_get_key
 *
 * Get a keystroke, blocking if necessary.  First, check the key buffer
 * and if that's empty ask the wintext window to pump a keystroke for us.
 * If we get it, pass it off to handle tab and help displays.  If those
 * displays ate the key, then get another one.
 */
static int
win32_get_key(Driver *drv)
{
	DI(di);
	int ch;
	
	plot_flush(&di->plot);
	do
	{
		if (di->key_buffer)
		{
			ch = di->key_buffer;
			di->key_buffer = 0;
		}
		else
		{
			ch = handle_help_tab(frame_get_key_press(1));
		}
	}
	while (ch == 0);

	return ch;
}

static void
win32_window(Driver *drv)
{
	DI(di);
	int width;
	int height;
	BOOL center_graphics;

	max_size(di, &width, &height, &center_graphics);
	frame_window(width, height);
	di->wintext.hWndParent = g_frame.window;
	wintext_texton(&di->wintext);
	plot_window(&di->plot, g_frame.window);
	center_windows(di, center_graphics);
}

/*
*----------------------------------------------------------------------
*
* shell_to_dos --
*
*	Exit to a unix shell.
*
* Results:
*	None.
*
* Side effects:
*	Goes to shell
*
*----------------------------------------------------------------------
*/
static void
win32_shell(Driver *drv)
{
	DI(di);
	windows_shell_to_dos();
}

static void
win32_hide_text_cursor(Driver *drv)
{
	DI(di);
	if (TRUE == di->cursor_shown)
	{
		di->cursor_shown = FALSE;
		wintext_hide_cursor(&di->wintext);
	}
	ODS("win32_hide_text_cursor");
}

static void
win32_set_for_text(Driver *drv)
{
	DI(di);
	di->text_not_graphics = TRUE;
	show_hide_windows(di->wintext.hWndCopy, di->plot.window);
}

static void
win32_set_for_graphics(Driver *drv)
{
	DI(di);
	di->text_not_graphics = FALSE;
	show_hide_windows(di->plot.window, di->wintext.hWndCopy);
	win32_hide_text_cursor(drv);
}

/* win32_set_clear
*/
static void
win32_set_clear(Driver *drv)
{
	DI(di);
	if (di->text_not_graphics)
	{
		wintext_clear(&di->wintext);
	}
	else
	{
		plot_clear(&di->plot);
	}
}

/* win32_set_video_mode
*/
static void
win32_set_video_mode(Driver *drv, VIDEOINFO *mode)
{
	extern void set_normal_dot(void);
	extern void set_normal_line(void);
	DI(di);

	/* initially, set the virtual line to be the scan line length */
	g_vxdots = sxdots;
	g_is_true_color = 0;				/* assume not truecolor */
	g_vesa_x_res = 0;					/* reset indicators used for */
	g_vesa_y_res = 0;					/* virtual screen limits estimation */
	g_ok_to_print = FALSE;
	g_good_mode = 1;
	if (dotmode !=0)
	{
		g_and_color = colors-1;
		boxcount = 0;
		g_dac_learn = 1;
		g_dac_count = cyclelimit;
		g_got_real_dac = TRUE;			/* we are "VGA" */

		driver_read_palette();
	}

	win32_resize(drv);

	if (g_disk_flag)
	{
		enddisk();
	}

	set_normal_dot();
	set_normal_line();

	win32_set_for_graphics(drv);
	win32_set_clear(drv);
}

static void
win32_put_string(Driver *drv, int row, int col, int attr, const char *msg)
{
	DI(di);
	_ASSERTE(di->text_not_graphics);
	if (-1 != row)
	{
		g_text_row = row;
	}
	if (-1 != col)
	{
		g_text_col = col;
	}
	{
		int abs_row = g_text_rbase + g_text_row;
		int abs_col = g_text_cbase + g_text_col;
		_ASSERTE(abs_row >= 0 && abs_row < WINTEXT_MAX_ROW);
		_ASSERTE(abs_col >= 0 && abs_col < WINTEXT_MAX_COL);
		wintext_putstring(&di->wintext, abs_col, abs_row, attr, msg, &g_text_row, &g_text_col);
	}
}

/************** Function scrollup(toprow, botrow) ******************
*
*       Scroll the screen up (from toprow to botrow)
*/
static void
win32_scroll_up(Driver *drv, int top, int bot)
{
	DI(di);
	_ASSERTE(di->text_not_graphics);
	wintext_scroll_up(&di->wintext, top, bot);
}

static BYTE *
win32_find_font(Driver *drv, int parm)
{
	ODS1("win32_find_font %d", parm);
	_ASSERTE(FALSE);
	return NULL;
}

static void
win32_move_cursor(Driver *drv, int row, int col)
{
	DI(di);

	_ASSERTE(di->text_not_graphics);
	ODS2("win32_move_cursor %d,%d", row, col);

	if (row != -1)
	{
		di->cursor_row = row;
		g_text_row = row;
	}
	if (col != -1)
	{
		di->cursor_col = col;
		g_text_col = col;
	}
	row = di->cursor_row;
	col = di->cursor_col;
	wintext_cursor(&di->wintext, g_text_cbase + col, g_text_rbase + row, 1);
	di->cursor_shown = TRUE;
}

static void
win32_set_attr(Driver *drv, int row, int col, int attr, int count)
{
	DI(di);

	_ASSERTE(di->text_not_graphics);
	if (-1 != row)
	{
		g_text_row = row;
	}
	if (-1 != col)
	{
		g_text_col = col;
	}
	wintext_set_attr(&di->wintext, g_text_rbase + g_text_row, g_text_cbase + g_text_col, attr, count);
}

/*
* Implement stack and unstack window functions by using multiple curses
* windows.
*/
static void
win32_stack_screen(Driver *drv)
{
	DI(di);

	ODS("win32_stack_screen");
	di->saved_cursor[di->screen_count+1] = g_text_row*80 + g_text_col;
	if (++di->screen_count)
	{
		/* already have some stacked */
		int i = di->screen_count - 1;

		_ASSERTE(di->text_not_graphics);
		_ASSERTE(i < MAXSCREENS);
		if (i >= MAXSCREENS)
		{
			/* bug, missing unstack? */
			stopmsg(STOPMSG_NO_STACK, "stackscreen overflow");
			exit(1);
		}
		di->saved_screens[i] = wintext_screen_get(&di->wintext);
		win32_set_clear(drv);
	}
	else
	{
		_ASSERTE(!di->text_not_graphics);
		win32_set_for_text(drv);
	}
}

static void
win32_unstack_screen(Driver *drv)
{
	DI(di);

	ODS("win32_unstack_screen");
	_ASSERTE(di->screen_count >= 0);
	g_text_row = di->saved_cursor[di->screen_count] / 80;
	g_text_col = di->saved_cursor[di->screen_count] % 80;
	if (--di->screen_count >= 0)
	{ /* unstack */
		_ASSERTE(di->text_not_graphics);
		wintext_screen_set(&di->wintext, di->saved_screens[di->screen_count]);
		free(di->saved_screens[di->screen_count]);
		di->saved_screens[di->screen_count] = NULL;
	}
	else
	{
		_ASSERTE(!di->text_not_graphics);
		win32_set_for_graphics(drv);
	}
	win32_move_cursor(drv, -1, -1);
}

static void
win32_discard_screen(Driver *drv)
{
	DI(di);

	if (--di->screen_count >= 0)
	{ /* unstack */
		_ASSERTE(di->text_not_graphics);
		if (di->saved_screens[di->screen_count])
		{
			free(di->saved_screens[di->screen_count]);
			di->saved_screens[di->screen_count] = NULL;
		}
	}
	else
	{
		win32_set_for_graphics(drv);
	}
}

static int
win32_init_fm(Driver *drv)
{
	ODS("win32_init_fm");
	_ASSERTE(0 && "win32_init_fm called");
	return 0;
}

static void
win32_buzzer(Driver *drv, int kind)
{
	ODS1("win32_buzzer %d", kind);
	MessageBeep(MB_OK);
}

static int
win32_sound_on(Driver *drv, int freq)
{
	ODS1("win32_sound_on %d", freq);
	return 0;
}

static void
win32_sound_off(Driver *drv)
{
	ODS("win32_sound_off");
}

static void
win32_mute(Driver *drv)
{
	ODS("win32_mute");
}

static int
win32_diskp(Driver *drv)
{
	return 0;
}

static int
win32_key_cursor(Driver *drv, int row, int col)
{
	DI(di);
	int result;

	ODS2("win32_key_cursor %d,%d", row, col);
	if (-1 != row)
	{
		di->cursor_row = row;
		g_text_row = row;
	}
	if (-1 != col)
	{
		di->cursor_col = col;
		g_text_col = col;
	}

	col = di->cursor_col;
	row = di->cursor_row;

	if (win32_key_pressed(drv))
	{
		result = win32_get_key(drv);
	}
	else
	{
		di->cursor_shown = TRUE;
		wintext_cursor(&di->wintext, col, row, 1);
		result = win32_get_key(drv);
		win32_hide_text_cursor(drv);
		di->cursor_shown = FALSE;
	}

	return result;
}

static int
win32_wait_key_pressed(Driver *drv, int timeout)
{
	int count = 10;
	while (!driver_key_pressed())
	{
		Sleep(25);
		if (timeout && (--count == 0))
		{
			break;
		}
	}

	return driver_key_pressed();
}

static int
win32_get_char_attr(Driver *drv)
{
	DI(di);
	return wintext_get_char_attr(&di->wintext, g_text_row, g_text_col);
}

static void
win32_put_char_attr(Driver *drv, int char_attr)
{
	DI(di);
	wintext_put_char_attr(&di->wintext, g_text_row, g_text_col, char_attr);
}

static int
win32_validate_mode(Driver *drv, VIDEOINFO *mode)
{
	return (mode->colors == 256) ? 1 : 0;
}

static void
win32_delay(Driver *drv, int ms)
{
	DI(di);
	windows_delay(ms);
}

static void
win32_pause(Driver *drv)
{
	DI(di);
	if (di->wintext.hWndCopy)
	{
		ShowWindow(di->wintext.hWndCopy, SW_HIDE);
	}
}

static void
win32_resume(Driver *drv)
{
	DI(di);
	if (!di->wintext.hWndCopy)
	{
		win32_window(drv);
	}

	ShowWindow(di->wintext.hWndCopy, SW_NORMAL);
}

static void
win32_get_truecolor(Driver *drv, int x, int y, int *r, int *g, int *b, int *a)
{
}

static void
win32_put_truecolor(Driver *drv, int x, int y, int r, int g, int b, int a)
{
}

static Win32Driver win32_driver_info =
{
	STD_DRIVER_STRUCT(win32, "A GDI driver for 32-bit Windows."),
	{ 0 },				/* Frame */
	{ 0 },				/* WinText */
	{ 0 },				/* Plot */
	TRUE,				/* text_not_graphics */
	0,					/* key_buffer */
	-1,					/* screen_count */
	{ NULL },			/* saved_screens */
	{ 0 },				/* saved_cursor */
	FALSE,				/* cursor_shown */
	0,					/* cursor_row */
	0					/* cursor_col */
};

Driver *win32_driver = &win32_driver_info.pub;
