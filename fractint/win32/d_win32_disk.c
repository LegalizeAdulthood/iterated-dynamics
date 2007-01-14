/* d_win32_disk.c
 *
 * Routines for a Win32 disk video mode driver for fractint.
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
#include "..\win32\frame.h"

/* read/write-a-dot/line routines */
typedef void t_dotwriter(int, int, int);
typedef int  t_dotreader(int, int);
typedef void t_linewriter(int y, int x, int lastx, BYTE *pixels);
typedef void t_linereader(int y, int x, int lastx, BYTE *pixels);

extern void windows_delay(int ms);

extern HINSTANCE g_instance;

#define MAXSCREENS 10
#define DRAW_INTERVAL 6
#define TIMER_ID 1

#define NUM_OF(ary_) (sizeof(ary_)/sizeof(ary_[0]))

#define DI(name_) Win32DiskDriver *name_ = (Win32DiskDriver *) drv

#if RT_VERBOSE
#define ODS(text_)						ods(__FILE__, __LINE__, text_)
#define ODS1(fmt_, arg_)				ods(__FILE__, __LINE__, fmt_, arg_)
#define ODS2(fmt_, a1_, a2_)			ods(__FILE__, __LINE__, fmt_, a1_, a2_)
#define ODS3(fmt_, a1_, a2_, a3_)		ods(__FILE__, __LINE__, fmt_, a1_, a2_, a3_)
#define ODS4(fmt_, _1, _2, _3, _4)		ods(__FILE__, __LINE__, fmt_, _1, _2, _3, _4)
#define ODS5(fmt_, _1, _2, _3, _4, _5)	ods(__FILE__, __LINE__, fmt_, _1, _2, _3, _4, _5)
#else
#define ODS(text_)
#define ODS1(fmt_, arg_)
#define ODS2(fmt_, a1_, a2_)
#define ODS3(fmt_, a1_, a2_, a3_)
#define ODS4(fmt_, _1, _2, _3, _4)
#define ODS5(fmt_, _1, _2, _3, _4, _5)
#endif

static t_dotwriter win32_dot_writer;
static t_dotreader win32_dot_reader;
static t_linewriter win32_line_writer;
static t_linereader win32_line_reader;

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
	MODE19("Win32 Disk Video         ", "                        ", FIK_F2,  320,  200),
	MODE19("Win32 Disk Video         ", "                        ", FIK_F3,  320,  400),
	MODE19("Win32 Disk Video         ", "                        ", FIK_F4,  360,  480),
	MODE19("Win32 Disk Video         ", "                        ", FIK_F5,  640,  400),
	MODE19("Win32 Disk Video         ", "                        ", FIK_F6,  640,  480),
	MODE27("Win32 Disk Video         ", "                        ", FIK_F7,  800,  600),
	MODE27("Win32 Disk Video         ", "                        ", FIK_F8,  1024, 768),
	MODE28("Win32 Disk Video         ", "                        ", FIK_F9,  1280, 1024),
	MODE28("Win32 Disk Video         ", "                        ", FIK_SF1, 1024, 1024),
	MODE28("Win32 Disk Video         ", "                        ", FIK_SF2, 1600, 1200),
	MODE28("Win32 Disk Video         ", "                        ", FIK_SF3, 2048, 2048),
	MODE28("Win32 Disk Video         ", "                        ", FIK_SF4, 4096, 4096),
	MODE28("Win32 Disk Video         ", "                        ", FIK_SF5, 8192, 8192)
};
#undef MODE28
#undef MODE27
#undef MODE19
#undef DRIVER_MODE

typedef struct tagWin32DiskDriver Win32DiskDriver;
struct tagWin32DiskDriver
{
	Driver pub;

	WinText wintext;

	/* key_buffer
	*
	* When we peeked ahead and saw a keypress, stash it here for later
	* feeding to our caller.
	*/
	int key_buffer;

	int video_flag;

	int width;
	int height;
	unsigned char cols[256][3];

	int screen_count;
	BYTE *saved_screens[MAXSCREENS];
	int saved_cursor[MAXSCREENS+1];
	BOOL cursor_shown;
	int cursor_row;
	int cursor_col;
};

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
check_arg(Win32DiskDriver *di, char *arg)
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

/*----------------------------------------------------------------------
*
* initdacbox --
*
* Put something nice in the dac.
*
* The conditions are:
*	Colors 1 and 2 should be bright so ifs fractals show up.
*	Color 15 should be bright for lsystem.
*	Color 1 should be bright for bifurcation.
*	Colors 1, 2, 3 should be distinct for periodicity.
*	The color map should look good for mandelbrot.
*
* Results:
*	None.
*
* Side effects:
*	Loads the dac.
*
*----------------------------------------------------------------------
*/
static void
initdacbox()
{
	int i;
	for (i=0;i < 256;i++)
	{
		g_dac_box[i][0] = (i >> 5)*8+7;
		g_dac_box[i][1] = (((i+16) & 28) >> 2)*8+7;
		g_dac_box[i][2] = (((i+2) & 3))*16+15;
	}
	g_dac_box[0][0] = g_dac_box[0][1] = g_dac_box[0][2] = 0;
	g_dac_box[1][0] = g_dac_box[1][1] = g_dac_box[1][2] = 255;
	g_dac_box[2][0] = 190; g_dac_box[2][1] = g_dac_box[2][2] = 255;
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

static void win32_dot_writer(int x, int y, int color)
{
	driver_write_pixel(x, y, color);
}
static int win32_dot_reader(int x, int y)
{
	return driver_read_pixel(x, y);
}
static void win32_line_writer(int row, int col, int lastcol, BYTE *pixels)
{
	driver_write_span(row, col, lastcol, pixels);
}
static void win32_line_reader(int row, int col, int lastcol, BYTE *pixels)
{
	driver_read_span(row, col, lastcol, pixels);
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

/***********************************************************************
////////////////////////////////////////////////////////////////////////
\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
***********************************************************************/

/*----------------------------------------------------------------------
*
* win32_disk_terminate --
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
win32_disk_terminate(Driver *drv)
{
	DI(di);
	int i;
	ODS("win32_disk_terminate");

	for (i = 0; i < NUM_OF(di->saved_screens); i++)
	{
		if (NULL != di->saved_screens[i])
		{
			free(di->saved_screens[i]);
			di->saved_screens[i] = NULL;
		}
	}
}

/*----------------------------------------------------------------------
*
* win32_disk_init --
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
win32_disk_init(Driver *drv, int *argc, char **argv)
{
	LPCSTR title = "FractInt for Windows";
	DI(di);

	frame_init(g_instance, title);
	if (!wintext_initialize(&di->wintext, g_instance, NULL, title))
	{
		return FALSE;
	}

	initdacbox();

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

/* win32_disk_resize
 *
 * Check if we need resizing.  If no, return 0.
 * If yes, resize internal buffers and return 1.
 */
static int
win32_disk_resize(Driver *drv)
{
	DI(di);

	if ((sxdots == di->width) && (sydots == di->height)) 
	{
		return 0;
	}

	return !0;
}


/*----------------------------------------------------------------------
* win32_disk_read_palette
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
win32_disk_read_palette(Driver *drv)
{
	DI(di);
	int i;

	ODS("win32_disk_read_palette");
	if (g_got_real_dac == 0)
	{
		return -1;
	}
	for (i = 0; i < 256; i++)
	{
		g_dac_box[i][0] = di->cols[i][0];
		g_dac_box[i][1] = di->cols[i][1];
		g_dac_box[i][2] = di->cols[i][2];
	}
	return 0;
}

/*
*----------------------------------------------------------------------
*
* win32_disk_write_palette --
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
win32_disk_write_palette(Driver *drv)
{
	DI(di);
	int i;

	ODS("win32_disk_write_palette");
	for (i = 0; i < 256; i++)
	{
		di->cols[i][0] = g_dac_box[i][0];
		di->cols[i][1] = g_dac_box[i][1];
		di->cols[i][2] = g_dac_box[i][2];
	}

	return 0;
}

/*
*----------------------------------------------------------------------
*
* win32_disk_schedule_alarm --
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
win32_disk_schedule_alarm(Driver *drv, int soon)
{
	DI(di);
	wintext_schedule_alarm(&di->wintext, (soon ? 1 : DRAW_INTERVAL)*1000);
}

/*
*----------------------------------------------------------------------
*
* win32_disk_write_pixel --
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
win32_disk_write_pixel(Driver *drv, int x, int y, int color)
{
	putcolor_a(x, y, color);
}

/*
*----------------------------------------------------------------------
*
* win32_disk_read_pixel --
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
win32_disk_read_pixel(Driver *drv, int x, int y)
{
	return getcolor(x, y);
}

/*
*----------------------------------------------------------------------
*
* win32_disk_write_span --
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
win32_disk_write_span(Driver *drv, int y, int x, int lastx, BYTE *pixels)
{
	int i;
	int width = lastx-x+1;
	ODS3("win32_disk_write_span (%d,%d,%d)", y, x, lastx);

	for (i = 0; i < width; i++)
	{
		win32_disk_write_pixel(drv, x+i, y, pixels[i]);
	}
}

/*
*----------------------------------------------------------------------
*
* win32_disk_read_span --
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
win32_disk_read_span(Driver *drv, int y, int x, int lastx, BYTE *pixels)
{
	int i, width;
	ODS3("win32_disk_read_span (%d,%d,%d)", y, x, lastx);
	width = lastx-x+1;
	for (i = 0; i < width; i++)
	{
		pixels[i] = win32_disk_read_pixel(drv, x+i, y);
	}
}

static void
win32_disk_set_line_mode(Driver *drv, int mode)
{
	ODS1("win32_disk_set_line_mode %d", mode);
}

static void
win32_disk_draw_line(Driver *drv, int x1, int y1, int x2, int y2, int color)
{
	ODS5("win32_disk_draw_line (%d,%d) (%d,%d) %d", x1, y1, x2, y2, color);
	draw_line(x1, y1, x2, y2, color);
}

/*
*----------------------------------------------------------------------
*
* win32_disk_redraw --
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
win32_disk_redraw(Driver *drv)
{
	DI(di);
	ODS("win32_disk_redraw");
	wintext_paintscreen(&di->wintext, 0, 80, 0, 25);
}

/* win32_disk_key_pressed
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
win32_disk_key_pressed(Driver *drv)
{
	DI(di);
	int ch = di->key_buffer;
	if (ch)
	{
		return ch;
	}
	ch = frame_get_key_press(0);
	if (ch)
	{
		ch = handle_help_tab(ch);
		di->key_buffer = ch;
	}

	return ch;
}

/* win32_disk_unget_key
 *
 * Unread a key!  The key buffer is only one character deep, so we
 * assert if its already full.  This should never happen in real life :-).
 */
void win32_disk_unget_key(Driver *drv, int key)
{
	DI(di);
	_ASSERTE(0 == di->key_buffer);
	di->key_buffer = key;
}

/* win32_disk_get_key
 *
 * Get a keystroke, blocking if necessary.  First, check the key buffer
 * and if that's empty ask the wintext window to pump a keystroke for us.
 * If we get it, pass it off to handle tab and help displays.  If those
 * displays ate the key, then get another one.
 */
static int
win32_disk_get_key(Driver *drv)
{
	DI(di);
	int ch;
	
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
win32_disk_window(Driver *drv)
{
	DI(di);
	frame_window(di->wintext.max_width, di->wintext.max_height);
	di->wintext.hWndParent = g_frame.window;
	wintext_texton(&di->wintext);
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
win32_disk_shell(Driver *drv)
{
	DI(di);
	windows_shell_to_dos();
}

/*
; **************** Function setvideomode(ax, bx, cx, dx) ****************
;       This function sets the (alphanumeric or graphic) video mode
;       of the monitor.   Called with the proper values of AX thru DX.
;       No returned values, as there is no particular standard to
;       adhere to in this case.

;       (SPECIAL "TWEAKED" VGA VALUES:  if AX==BX==CX==0, assume we have a
;       genuine VGA or register compatable adapter and program the registers
;       directly using the coded value in DX)

; Unix: We ignore ax,bx,cx,dx.  dotmode is the "mode" field in the video
; table.  We use mode 19 for the X window.
*/
static void
win32_disk_set_video_mode(Driver *drv, VIDEOINFO *mode)
{
	extern void set_disk_dot(void);
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
		g_got_real_dac = TRUE;

		driver_read_palette();
	}

	win32_disk_resize(drv);

	if (g_disk_flag)
	{
		enddisk();
	}

	startdisk();
	set_disk_dot();
	set_normal_line();
}

/*
; PUTSTR.asm puts a string directly to video display memory. Called from C by:
;    putstring(row, col, attr, string) where
;         row, col = row and column to start printing.
;         attr = color attribute.
;         string = pointer to the null terminated string to print.
;    Written for the A86 assembler (which has much less 'red tape' than MASM)
;    by Bob Montgomery, Orlando, Fla.             7-11-88
;    Adapted for MASM 5.1 by Tim Wegner          12-11-89
;    Furthur mucked up to handle graphics
;       video modes by Bert Tyler                 1-07-90
;    Reworked for:  row,col update/inherit;
;       620x200x2 inverse video;  ptr to string;
;       fix to avoid scrolling when last posn chgd;
;       divider removed;  newline ctl chars;  PB  9-25-90
*/
static void
win32_disk_put_string(Driver *drv, int row, int col, int attr, const char *msg)
{
	DI(di);
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

static void
win32_disk_set_clear(Driver *drv)
{
	DI(di);
	wintext_clear(&di->wintext);
}

/************** Function scrollup(toprow, botrow) ******************
*
*       Scroll the screen up (from toprow to botrow)
*/
static void
win32_disk_scroll_up(Driver *drv, int top, int bot)
{
	DI(di);
	ODS2("win32_disk_scroll_up %d, %d", top, bot);
	wintext_scroll_up(&di->wintext, top, bot);
}

static BYTE *
win32_disk_find_font(Driver *drv, int parm)
{
	ODS1("win32_disk_find_font %d", parm);
	return NULL;
}

static void
win32_disk_move_cursor(Driver *drv, int row, int col)
{
	DI(di);
	ODS2("win32_disk_move_cursor %d,%d", row, col);

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
win32_disk_set_attr(Driver *drv, int row, int col, int attr, int count)
{
	DI(di);
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

static void
win32_disk_hide_text_cursor(Driver *drv)
{
	DI(di);
	if (TRUE == di->cursor_shown)
	{
		di->cursor_shown = FALSE;
		wintext_hide_cursor(&di->wintext);
	}
	ODS("win32_disk_hide_text_cursor");
}

static void
win32_disk_set_for_text(Driver *drv)
{
}

static void
win32_disk_set_for_graphics(Driver *drv)
{
	win32_disk_hide_text_cursor(drv);
}

/*
* Implement stack and unstack window functions by using multiple curses
* windows.
*/
static void
win32_disk_stack_screen(Driver *drv)
{
	Win32DiskDriver *di = (Win32DiskDriver *) drv;
	ODS("win32_disk_stack_screen");

	di->saved_cursor[di->screen_count+1] = g_text_row*80 + g_text_col;
	if (++di->screen_count)
	{
		/* already have some stacked */
		int i = di->screen_count - 1;

		_ASSERTE(i < MAXSCREENS);
		if (i >= MAXSCREENS)
		{
			/* bug, missing unstack? */
			stopmsg(STOPMSG_NO_STACK, "stackscreen overflow");
			exit(1);
		}
		di->saved_screens[i] = wintext_screen_get(&di->wintext);
		win32_disk_set_clear(drv);
	}
	else
	{
		win32_disk_set_for_text(drv);
	}
}

static void
win32_disk_unstack_screen(Driver *drv)
{
	Win32DiskDriver *di = (Win32DiskDriver *) drv;

	ODS("win32_disk_unstack_screen");
	_ASSERTE(di->screen_count >= 0);
	g_text_row = di->saved_cursor[di->screen_count] / 80;
	g_text_col = di->saved_cursor[di->screen_count] % 80;
	if (--di->screen_count >= 0)
	{ /* unstack */
		wintext_screen_set(&di->wintext, di->saved_screens[di->screen_count]);
		free(di->saved_screens[di->screen_count]);
		di->saved_screens[di->screen_count] = NULL;
	}
	else
	{
		win32_disk_set_for_graphics(drv);
	}
	win32_disk_move_cursor(drv, -1, -1);
}

static void
win32_disk_discard_screen(Driver *drv)
{
	Win32DiskDriver *di = (Win32DiskDriver *) drv;

	_ASSERTE(di->screen_count > 0);
	if (--di->screen_count >= 0)
	{ /* unstack */
		if (di->saved_screens[di->screen_count])
		{
			free(di->saved_screens[di->screen_count]);
			di->saved_screens[di->screen_count] = NULL;
		}
	}
}

static int
win32_disk_init_fm(Driver *drv)
{
	ODS("win32_disk_init_fm");
	return 0;
}

static void
win32_disk_buzzer(Driver *drv, int kind)
{
	ODS1("win32_disk_buzzer %d", kind);
	MessageBeep(MB_OK);
}

static int
win32_disk_sound_on(Driver *drv, int freq)
{
	ODS1("win32_disk_sound_on %d", freq);
	return 0;
}

static void
win32_disk_sound_off(Driver *drv)
{
	ODS("win32_disk_sound_off");
}

static void
win32_disk_mute(Driver *drv)
{
	ODS("win32_disk_mute");
}

static int
win32_disk_diskp(Driver *drv)
{
	return 1;
}

static int
win32_disk_key_cursor(Driver *drv, int row, int col)
{
	DI(di);
	int result;

	ODS2("win32_disk_key_cursor %d,%d", row, col);
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

	if (win32_disk_key_pressed(drv))
	{
		result = win32_disk_get_key(drv);
	}
	else
	{
		di->cursor_shown = TRUE;
		wintext_cursor(&di->wintext, col, row, 1);
		result = win32_disk_get_key(drv);
		win32_disk_hide_text_cursor(drv);
		di->cursor_shown = FALSE;
	}

	return result;
}

static int
win32_disk_wait_key_pressed(Driver *drv, int timeout)
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
win32_disk_get_char_attr(Driver *drv)
{
	DI(di);
	return wintext_get_char_attr(&di->wintext, g_text_row, g_text_col);
}

static void
win32_disk_put_char_attr(Driver *drv, int char_attr)
{
	DI(di);
	wintext_put_char_attr(&di->wintext, g_text_row, g_text_col, char_attr);
}

static int
win32_disk_validate_mode(Driver *drv, VIDEOINFO *mode)
{
	return (mode->colors == 256) ? 1 : 0;
}

static void
win32_disk_delay(Driver *drv, int ms)
{
	DI(di);
	windows_delay(ms);
}

static void
win32_disk_pause(Driver *drv)
{
	DI(di);
	if (di->wintext.hWndCopy)
	{
		ShowWindow(di->wintext.hWndCopy, SW_HIDE);
	}
}

static void
win32_disk_resume(Driver *drv)
{
	DI(di);
	if (!di->wintext.hWndCopy)
	{
		win32_disk_window(drv);
	}

	ShowWindow(di->wintext.hWndCopy, SW_NORMAL);
}

static void
win32_disk_get_truecolor(Driver *drv, int x, int y, int *r, int *g, int *b, int *a)
{
}

static void
win32_disk_put_truecolor(Driver *drv, int x, int y, int r, int g, int b, int a)
{
}


static Win32DiskDriver win32_disk_driver_info =
{
	STD_DRIVER_STRUCT(win32_disk, "A disk video driver for 32-bit Windows.")
};

Driver *win32_disk_driver = &win32_disk_driver_info.pub;
