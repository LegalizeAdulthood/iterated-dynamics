/* d_win32.c
 *
 * Routines for a Win32 GDI driver for fractint.
 */
#include <assert.h>
#include <stdio.h>
#include <time.h>

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
#include "d_win32.h"
#include "ods.h"

extern HINSTANCE g_instance;

#define DRAW_INTERVAL 6
#define TIMER_ID 1

#define NUM_OF(ary_) (sizeof(ary_)/sizeof(ary_[0]))

#define DI(name_) GDIDriver *name_ = (GDIDriver *) drv

typedef struct tagGDIDriver GDIDriver;
struct tagGDIDriver
{
	Win32BaseDriver base;

	Plot plot;
	BOOL text_not_graphics;
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
static VIDEOINFO modes[] =
{
	MODE19("Win32 GDI Video          ", "                        ", 0,  320,  240),
	MODE19("Win32 GDI Video          ", "                        ", 0,  400,  300),
	MODE19("Win32 GDI Video          ", "                        ", 0,  480,  360),
	MODE19("Win32 GDI Video          ", "                        ", 0,  600,  450),
	MODE19("Win32 GDI Video          ", "                        ", 0,  640,  480),
	MODE19("Win32 GDI Video          ", "                        ", 0,  800,  600),
	MODE19("Win32 GDI Video          ", "                        ", 0, 1024,  768),
	MODE19("Win32 GDI Video          ", "                        ", 0, 1200,  900),
	MODE19("Win32 GDI Video          ", "                        ", 0, 1280,  960),
	MODE19("Win32 GDI Video          ", "                        ", 0, 1400, 1050),
	MODE19("Win32 GDI Video          ", "                        ", 0, 1500, 1125),
	MODE19("Win32 GDI Video          ", "                        ", 0, 1600, 1200)
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
check_arg(GDIDriver *di, char *arg)
{
	return 0;
}

/* handle_special_keys
 *
 * First, do some slideshow processing.  Then handle F1 and TAB display.
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
handle_special_keys(int ch)
{
	static int inside_help = 0;

	if (SLIDES_PLAY == g_slides)
	{
		if (ch == FIK_ESC)
		{
			stopslideshow();
			ch = 0;
		}
		else if (!ch)
		{
			ch = slideshw();
		}
	}
	else if ((SLIDES_RECORD == g_slides) && ch)
	{
		recordshw(ch);
	}

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
max_size(GDIDriver *di, int *width, int *height, BOOL *center_x, BOOL *center_y)
{
	*width = di->base.wintext.max_width;
	*height = di->base.wintext.max_height;
	if (g_video_table[g_adapter].xdots > *width)
	{
		*width = g_video_table[g_adapter].xdots;
		*center_x = FALSE;
	}
	if (g_video_table[g_adapter].ydots > *height)
	{
		*height = g_video_table[g_adapter].ydots;
		*center_y = FALSE;
	}
}

static void center_windows(GDIDriver *di, BOOL center_x, BOOL center_y)
{
	POINT text_pos = { 0 }, plot_pos = { 0 };
	BOOL status;

	if (center_x)
	{
		plot_pos.x = (g_frame.width - di->plot.width)/2;
	}
	else
	{
		text_pos.x = (g_frame.width - di->base.wintext.max_width)/2;
	}

	if (center_y)
	{
		plot_pos.y = (g_frame.height - di->plot.height)/2;
	}
	else
	{
		text_pos.y = (g_frame.height - di->base.wintext.max_height)/2;
	}

	status = SetWindowPos(di->plot.window, NULL,
		plot_pos.x, plot_pos.y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
	_ASSERTE(status);
	status = SetWindowPos(di->base.wintext.hWndCopy, NULL,
		text_pos.x, text_pos.y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
	_ASSERTE(status);
}

/***********************************************************************
////////////////////////////////////////////////////////////////////////
\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
***********************************************************************/

/*----------------------------------------------------------------------
*
* gdi_terminate --
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
gdi_terminate(Driver *drv)
{
	DI(di);
	ODS("gdi_terminate");

	plot_terminate(&di->plot);
	win32_terminate(drv);
}

static void
gdi_get_max_screen(Driver *drv, int *xmax, int *ymax)
{
	RECT desktop;
	GetClientRect(GetDesktopWindow(), &desktop);
	desktop.right -= GetSystemMetrics(SM_CXFRAME)*2;
	desktop.bottom -= GetSystemMetrics(SM_CYFRAME)*2
		+ GetSystemMetrics(SM_CYCAPTION) - 1;

	if (xmax != NULL)
	{
		*xmax = desktop.right;
	}
	if (ymax != NULL)
	{
		*ymax = desktop.bottom;
	}
}

/*----------------------------------------------------------------------
*
* gdi_init --
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
gdi_init(Driver *drv, int *argc, char **argv)
{
	LPCSTR title = "FractInt for Windows";
	DI(di);

	ODS("gdi_init");
	frame_init(g_instance, title);
	if (!wintext_initialize(&di->base.wintext, g_instance, NULL, "Text"))
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
					argv[j] = argv[j + 1];
				}
				argv[j] = NULL;
				--*argc;
			}
		}
	}

	/* add default list of video modes */
	{
		int width, height;
		int m;

		gdi_get_max_screen(drv, &width, &height);

		for (m = 0; m < NUM_OF(modes); m++)
		{
			if ((modes[m].xdots <= width) &&
				(modes[m].ydots <= height))
			{
				add_video_mode(drv, &modes[m]);
			}
		}
	}

	return TRUE;
}

/* gdi_resize
 *
 * Check if we need resizing.  If no, return 0.
 * If yes, resize internal buffers and return 1.
 */
static int
gdi_resize(Driver *drv)
{
	DI(di);
	int width, height;
	BOOL center_graphics_x, center_graphics_y;

	max_size(di, &width, &height, &center_graphics_x, &center_graphics_y);
	if ((g_video_table[g_adapter].xdots == di->plot.width)
		&& (g_video_table[g_adapter].ydots == di->plot.height)
		&& (width == g_frame.width)
		&& (height == g_frame.height))
	{
		return 0;
	}

	frame_resize(width, height);
	plot_resize(&di->plot);
	center_windows(di, center_graphics_x, center_graphics_y);
	return 1;
}


/*----------------------------------------------------------------------
* gdi_read_palette
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
gdi_read_palette(Driver *drv)
{
	DI(di);
	return plot_read_palette(&di->plot);
}

/*
*----------------------------------------------------------------------
*
* gdi_write_palette --
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
gdi_write_palette(Driver *drv)
{
	DI(di);
	return plot_write_palette(&di->plot);
}

/*
*----------------------------------------------------------------------
*
* gdi_schedule_alarm --
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
gdi_schedule_alarm(Driver *drv, int soon)
{
	DI(di);
	soon = (soon ? 1 : DRAW_INTERVAL)*1000;
	if (di->text_not_graphics)
	{
		wintext_schedule_alarm(&di->base.wintext, soon);
	}
	else
	{
		plot_schedule_alarm(&di->plot, soon);
	}
}

/*
*----------------------------------------------------------------------
*
* gdi_write_pixel --
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
gdi_write_pixel(Driver *drv, int x, int y, int color)
{
	DI(di);
	plot_write_pixel(&di->plot, x, y, color);
}

/*
*----------------------------------------------------------------------
*
* gdi_read_pixel --
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
gdi_read_pixel(Driver *drv, int x, int y)
{
	DI(di);
	return plot_read_pixel(&di->plot, x, y);
}

/*
*----------------------------------------------------------------------
*
* gdi_write_span --
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
gdi_write_span(Driver *drv, int y, int x, int lastx, BYTE *pixels)
{
	DI(di);
	plot_write_span(&di->plot, x, y, lastx, pixels);
}

/*
*----------------------------------------------------------------------
*
* gdi_read_span --
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
gdi_read_span(Driver *drv, int y, int x, int lastx, BYTE *pixels)
{
	DI(di);
	plot_read_span(&di->plot, y, x, lastx, pixels);
}

static void
gdi_set_line_mode(Driver *drv, int mode)
{
	DI(di);
	plot_set_line_mode(&di->plot, mode);
}

static void
gdi_draw_line(Driver *drv, int x1, int y1, int x2, int y2, int color)
{
	DI(di);
	plot_draw_line(&di->plot, x1, y1, x2, y2, color);
}

/*
*----------------------------------------------------------------------
*
* gdi_redraw --
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
gdi_redraw(Driver *drv)
{
	DI(di);
	ODS("gdi_redraw");
	if (di->text_not_graphics)
	{
		wintext_paintscreen(&di->base.wintext, 0, 80, 0, 25);
	}
	else
	{
		plot_redraw(&di->plot);
	}
	frame_pump_messages(FALSE);
}

static void
gdi_window(Driver *drv)
{
	DI(di);
	int width;
	int height;
	BOOL center_x, center_y;

	max_size(di, &width, &height, &center_x, &center_y);
	frame_window(width, height);
	di->base.wintext.hWndParent = g_frame.window;
	wintext_texton(&di->base.wintext);
	plot_window(&di->plot, g_frame.window);
	center_windows(di, center_x, center_y);
}

static void
gdi_set_for_text(Driver *drv)
{
	DI(di);
	di->text_not_graphics = TRUE;
	show_hide_windows(di->base.wintext.hWndCopy, di->plot.window);
}

static void
gdi_set_for_graphics(Driver *drv)
{
	DI(di);
	di->text_not_graphics = FALSE;
	show_hide_windows(di->plot.window, di->base.wintext.hWndCopy);
	win32_hide_text_cursor(drv);
}

/* gdi_set_clear
*/
static void
gdi_set_clear(Driver *drv)
{
	DI(di);
	if (di->text_not_graphics)
	{
		wintext_clear(&di->base.wintext);
	}
	else
	{
		plot_clear(&di->plot);
	}
}

/* gdi_set_video_mode
*/
static void
gdi_set_video_mode(Driver *drv, VIDEOINFO *mode)
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
	if (dotmode != 0)
	{
		g_and_color = colors-1;
		boxcount = 0;
		g_dac_learn = 1;
		g_dac_count = cyclelimit;
		g_got_real_dac = TRUE;			/* we are "VGA" */

		driver_read_palette();
	}

	gdi_resize(drv);
	plot_clear(&di->plot);

	if (g_disk_flag)
	{
		enddisk();
	}

	set_normal_dot();
	set_normal_line();

	gdi_set_for_graphics(drv);
	gdi_set_clear(drv);
}

static void
gdi_put_string(Driver *drv, int row, int col, int attr, const char *msg)
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
		wintext_putstring(&di->base.wintext, abs_col, abs_row, attr, msg, &g_text_row, &g_text_col);
	}
}

/************** Function scrollup(toprow, botrow) ******************
*
*       Scroll the screen up (from toprow to botrow)
*/
static void
gdi_scroll_up(Driver *drv, int top, int bot)
{
	DI(di);
	_ASSERTE(di->text_not_graphics);
	wintext_scroll_up(&di->base.wintext, top, bot);
}

static void
gdi_move_cursor(Driver *drv, int row, int col)
{
	DI(di);

	_ASSERTE(di->text_not_graphics);
	ODS2("gdi_move_cursor %d,%d", row, col);

	if (row != -1)
	{
		di->base.cursor_row = row;
		g_text_row = row;
	}
	if (col != -1)
	{
		di->base.cursor_col = col;
		g_text_col = col;
	}
	row = di->base.cursor_row;
	col = di->base.cursor_col;
	wintext_cursor(&di->base.wintext, g_text_cbase + col, g_text_rbase + row, 1);
	di->base.cursor_shown = TRUE;
}

static void
gdi_set_attr(Driver *drv, int row, int col, int attr, int count)
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
	wintext_set_attr(&di->base.wintext, g_text_rbase + g_text_row, g_text_cbase + g_text_col, attr, count);
}

/*
* Implement stack and unstack window functions by using multiple curses
* windows.
*/
static void
gdi_stack_screen(Driver *drv)
{
	DI(di);

	ODS("gdi_stack_screen");
	di->base.saved_cursor[di->base.screen_count + 1] = g_text_row*80 + g_text_col;
	if (++di->base.screen_count)
	{
		/* already have some stacked */
		int i = di->base.screen_count - 1;

		_ASSERTE(di->text_not_graphics);
		_ASSERTE(i < WIN32_MAXSCREENS);
		if (i >= WIN32_MAXSCREENS)
		{
			/* bug, missing unstack? */
			stopmsg(STOPMSG_NO_STACK, "stackscreen overflow");
			exit(1);
		}
		di->base.saved_screens[i] = wintext_screen_get(&di->base.wintext);
		gdi_set_clear(drv);
	}
	else
	{
		gdi_set_for_text(drv);
		gdi_set_clear(drv);
	}
}

static void
gdi_unstack_screen(Driver *drv)
{
	DI(di);

	ODS("gdi_unstack_screen");
	_ASSERTE(di->base.screen_count >= 0);
	g_text_row = di->base.saved_cursor[di->base.screen_count] / 80;
	g_text_col = di->base.saved_cursor[di->base.screen_count] % 80;
	if (--di->base.screen_count >= 0)
	{ /* unstack */
		_ASSERTE(di->text_not_graphics);
		wintext_screen_set(&di->base.wintext, di->base.saved_screens[di->base.screen_count]);
		free(di->base.saved_screens[di->base.screen_count]);
		di->base.saved_screens[di->base.screen_count] = NULL;
		gdi_move_cursor(drv, -1, -1);
	}
	else
	{
		gdi_set_for_graphics(drv);
	}
}

static void
gdi_discard_screen(Driver *drv)
{
	DI(di);

	if (--di->base.screen_count >= 0)
	{ /* unstack */
		_ASSERTE(di->text_not_graphics);
		if (di->base.saved_screens[di->base.screen_count])
		{
			free(di->base.saved_screens[di->base.screen_count]);
			di->base.saved_screens[di->base.screen_count] = NULL;
		}
	}
	else
	{
		gdi_set_for_graphics(drv);
	}
}

static int
gdi_init_fm(Driver *drv)
{
	ODS("gdi_init_fm");
	_ASSERTE(0 && "gdi_init_fm called");
	return 0;
}

static void
gdi_buzzer(Driver *drv, int kind)
{
	ODS1("gdi_buzzer %d", kind);
	MessageBeep(MB_OK);
}

static int
gdi_sound_on(Driver *drv, int freq)
{
	ODS1("gdi_sound_on %d", freq);
	return 0;
}

static void
gdi_sound_off(Driver *drv)
{
	ODS("gdi_sound_off");
}

static void
gdi_mute(Driver *drv)
{
	ODS("gdi_mute");
}

static int
gdi_validate_mode(Driver *drv, VIDEOINFO *mode)
{
	int width, height;
	gdi_get_max_screen(drv, &width, &height);

	/* allow modes <= size of screen with 256 colors and dotmode=19
	   ax/bx/cx/dx must be zero. */
	return (mode->xdots <= width) &&
		(mode->ydots <= height) &&
		(mode->colors == 256) &&
		(mode->videomodeax == 0) &&
		(mode->videomodebx == 0) &&
		(mode->videomodecx == 0) &&
		(mode->videomodedx == 0) &&
		(mode->dotmode == 19);
}

static void
gdi_pause(Driver *drv)
{
	DI(di);
	if (di->base.wintext.hWndCopy)
	{
		ShowWindow(di->base.wintext.hWndCopy, SW_HIDE);
	}
	if (di->plot.window)
	{
		ShowWindow(di->plot.window, SW_HIDE);
	}
}

static void
gdi_resume(Driver *drv)
{
	DI(di);
	if (!di->base.wintext.hWndCopy)
	{
		gdi_window(drv);
	}

	ShowWindow(di->base.wintext.hWndCopy, SW_NORMAL);
	wintext_resume(&di->base.wintext);
}

static void
gdi_display_string(Driver *drv, int x, int y, int fg, int bg, const char *text)
{
	DI(di);
	_ASSERTE(!di->text_not_graphics);
	plot_display_string(&di->plot, x, y, fg, bg, text);
}

static void
gdi_save_graphics(Driver *drv)
{
	DI(di);
	plot_save_graphics(&di->plot);
}

static void
gdi_restore_graphics(Driver *drv)
{
	DI(di);
	plot_restore_graphics(&di->plot);
}

static void
gdi_flush(Driver *drv)
{
	DI(di);
	plot_flush(&di->plot);
}

static GDIDriver gdi_driver_info =
{
	{
		{
			"gdi", "A GDI driver for 32-bit Windows.",
			gdi_init,
			gdi_validate_mode,
			gdi_get_max_screen,
			gdi_terminate,
			gdi_pause, gdi_resume,
			gdi_schedule_alarm,
			gdi_window, gdi_resize, gdi_redraw,
			gdi_read_palette, gdi_write_palette,
			gdi_read_pixel, gdi_write_pixel,
			gdi_read_span, gdi_write_span,
			win32_get_truecolor, win32_put_truecolor,
			gdi_set_line_mode, gdi_draw_line,
			gdi_display_string,
			gdi_save_graphics, gdi_restore_graphics,
			win32_get_key, win32_key_cursor, win32_key_pressed, win32_wait_key_pressed, win32_unget_key,
			win32_shell,
			gdi_set_video_mode,
			win32_put_string,
			gdi_set_for_text, gdi_set_for_graphics,
			gdi_set_clear,
			win32_move_cursor, win32_hide_text_cursor,
			win32_set_attr,
			win32_scroll_up,
			win32_stack_screen, win32_unstack_screen, win32_discard_screen,
			win32_init_fm, win32_buzzer, win32_sound_on, win32_sound_off, win32_mute,
			win32_diskp,
			win32_get_char_attr, win32_put_char_attr,
			win32_get_char_attr_rowcol, win32_put_char_attr_rowcol,
			win32_delay,
			win32_set_keyboard_timeout,
			gdi_flush
		},
		{ 0 },				/* Frame */
		{ 0 },				/* WinText */
		0,					/* key_buffer */
		-1,					/* screen_count */
		{ NULL },			/* saved_screens */
		{ 0 },				/* saved_cursor */
		FALSE,				/* cursor_shown */
		0,					/* cursor_row */
		0					/* cursor_col */
	},
	{ 0 },				/* Plot */
	TRUE				/* text_not_graphics */
};

Driver *gdi_driver = &gdi_driver_info.base.pub;
