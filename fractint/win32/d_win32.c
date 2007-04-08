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

#define DI(name_) Win32BaseDriver *name_ = (Win32BaseDriver *) drv

int lookatmouse = LOOK_MOUSE_NONE;
static int previous_look_mouse = LOOK_MOUSE_NONE;
static int mousetime = 0;				/* time of last mouseread call */
static int mlbtimer = 0;				/* time of left button 1st click */
static int mrbtimer = 0;				/* time of right button 1st click */
static int mhtimer = 0;					/* time of last horiz move */
static int mvtimer = 0;					/* time of last vert  move */
static int mhmickeys = 0;				/* pending horiz movement */
static int mvmickeys = 0;				/* pending vert  movement */
static int mbstatus = 0;				/* status of mouse buttons: MOUSE_CLICK_{NONE, LEFT, RIGHT} */
static int mbclicks = 0;				/* had 1 click so far? &1 mlb, &2 mrb */
#define MOUSE_CLICK_NONE 0
#define MOUSE_CLICK_LEFT 1
#define MOUSE_CLICK_RIGHT 2
int mouse_x = 0;
int mouse_y = 0;

/* timed save variables, handled by readmouse: */
static int savechktime = 0;				/* time of last autosave check */
long savebase = 0;						/* base clock ticks */
long saveticks = 0;						/* save after this many ticks */
int finishrow = 0;						/* save when this row is finished */

int handle_timed_save(int ch)
{
	int ticker;

	if (ch != 0)
	{
		return ch;
	}

	/* now check for automatic/periodic saving... */
	ticker = readticker();
	if (saveticks && (ticker != savechktime))
	{
		savechktime = ticker;
		ticker -= savebase;
		if (ticker > saveticks)
		{
			if (1 == finishrow)
			{
				if (calc_status != CALCSTAT_IN_PROGRESS)
				{
					if ((g_got_status != GOT_STATUS_12PASS) && (g_got_status != GOT_STATUS_GUESSING))
					{
						finishrow = g_current_row;
					}
				}
			}
			else if (g_current_row != finishrow)
			{
				timedsave = TRUE;
				return FIK_SAVE_TIME;
			}
		}
	}

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

	ch = handle_timed_save(ch);
	if (ch != FIK_SAVE_TIME)
	{
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
	}
	if (DEBUGFLAG_EDIT_TEXT_COLORS == g_debug_flag)
	{
		if ('~' == ch)
		{
			edit_text_colors();
			ch = 0;
		}
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

static void flush_output(void)
{
	static time_t start = 0;
	static long ticks_per_second = 0;
	static long last = 0;
	static long frames_per_second = 10;

	if (!ticks_per_second)
	{
		if (!start)
		{
			time(&start);
			last = readticker();
		}
		else
		{
			time_t now = time(NULL);
			long now_ticks = readticker();
			if (now > start)
			{
				ticks_per_second = (now_ticks - last)/((long) (now - start));
			}
		}
	}
	else
	{
		long now = readticker();
		if ((now - last)*frames_per_second > ticks_per_second)
		{
			driver_flush();
			frame_pump_messages(FALSE);
			last = now;
		}
	}
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
void
win32_terminate(Driver *drv)
{
	DI(di);
	ODS("win32_terminate");

	/* plot_terminate(&di->plot); */
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
int
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

	return TRUE;
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
int
win32_key_pressed(Driver *drv)
{
	DI(di);
	int ch = di->key_buffer;

	if (ch)
	{
		return ch;
	}
	flush_output();
	ch = handle_special_keys(frame_get_key_press(0));
	_ASSERTE(di->key_buffer == 0);
	di->key_buffer = ch;

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
int
win32_get_key(Driver *drv)
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
			ch = handle_special_keys(frame_get_key_press(1));
		}
	}
	while (ch == 0);

	return ch;
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
void
win32_shell(Driver *drv)
{
	DI(di);
	STARTUPINFO si =
	{
		sizeof(si)
	};
	PROCESS_INFORMATION pi = { 0 };
	char *comspec = getenv("COMSPEC");

	if (NULL == comspec)
	{
		comspec = "cmd.exe";
	}
	if (CreateProcess(NULL, comspec, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi))
	{
		DWORD status = WaitForSingleObject(pi.hProcess, 1000);
		while (WAIT_TIMEOUT == status)
		{
			frame_pump_messages(0);
			status = WaitForSingleObject(pi.hProcess, 1000);
		}
		CloseHandle(pi.hProcess);
	}
}

void
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

/* win32_set_video_mode
*/
void
win32_set_video_mode(Driver *drv, VIDEOINFO *mode)
{
	extern void set_normal_dot(void);
	extern void set_normal_line(void);
	DI(di);

	/* initially, set the virtual line to be the scan line length */
	g_vxdots = sxdots;
	g_is_true_color = 0;				/* assume not truecolor */
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

	driver_resize();

	if (g_disk_flag)
	{
		enddisk();
	}

	set_normal_dot();
	set_normal_line();

	driver_set_for_graphics();
	driver_set_clear();
}

void
win32_put_string(Driver *drv, int row, int col, int attr, const char *msg)
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

/************** Function scrollup(toprow, botrow) ******************
*
*       Scroll the screen up (from toprow to botrow)
*/
void
win32_scroll_up(Driver *drv, int top, int bot)
{
	DI(di);

	wintext_scroll_up(&di->wintext, top, bot);
}

void
win32_move_cursor(Driver *drv, int row, int col)
{
	DI(di);

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

void
win32_set_attr(Driver *drv, int row, int col, int attr, int count)
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

/*
* Implement stack and unstack window functions by using multiple curses
* windows.
*/
void
win32_stack_screen(Driver *drv)
{
	DI(di);

	di->saved_cursor[di->screen_count + 1] = g_text_row*80 + g_text_col;
	if (++di->screen_count)
	{
		/* already have some stacked */
		int i = di->screen_count - 1;

		_ASSERTE(i < WIN32_MAXSCREENS);
		if (i >= WIN32_MAXSCREENS)
		{
			/* bug, missing unstack? */
			stopmsg(STOPMSG_NO_STACK, "stackscreen overflow");
			exit(1);
		}
		di->saved_screens[i] = wintext_screen_get(&di->wintext);
		driver_set_clear();
	}
	else
	{
		driver_set_for_text();
		driver_set_clear();
	}
}

void
win32_unstack_screen(Driver *drv)
{
	DI(di);

	_ASSERTE(di->screen_count >= 0);
	g_text_row = di->saved_cursor[di->screen_count] / 80;
	g_text_col = di->saved_cursor[di->screen_count] % 80;
	if (--di->screen_count >= 0)
	{
		/* unstack */
		wintext_screen_set(&di->wintext, di->saved_screens[di->screen_count]);
		free(di->saved_screens[di->screen_count]);
		di->saved_screens[di->screen_count] = NULL;
		win32_move_cursor(drv, -1, -1);
	}
	else
	{
		driver_set_for_graphics();
	}
}

void
win32_discard_screen(Driver *drv)
{
	DI(di);

	if (--di->screen_count >= 0)
	{
		/* unstack */
		if (di->saved_screens[di->screen_count])
		{
			free(di->saved_screens[di->screen_count]);
			di->saved_screens[di->screen_count] = NULL;
		}
	}
	else
	{
		driver_set_for_graphics();
	}
}

int
win32_init_fm(Driver *drv)
{
	ODS("win32_init_fm");
	return 0;
}

void
win32_buzzer(Driver *drv, int kind)
{
	ODS1("win32_buzzer %d", kind);
	MessageBeep(MB_OK);
}

int
win32_sound_on(Driver *drv, int freq)
{
	ODS1("win32_sound_on %d", freq);
	return 0;
}

void
win32_sound_off(Driver *drv)
{
	ODS("win32_sound_off");
}

void
win32_mute(Driver *drv)
{
	ODS("win32_mute");
}

int
win32_diskp(Driver *drv)
{
	return 0;
}

int
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

int
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

int
win32_get_char_attr(Driver *drv)
{
	DI(di);
	return wintext_get_char_attr(&di->wintext, g_text_row, g_text_col);
}

void
win32_put_char_attr(Driver *drv, int char_attr)
{
	DI(di);
	wintext_put_char_attr(&di->wintext, g_text_row, g_text_col, char_attr);
}

int
win32_get_char_attr_rowcol(Driver *drv, int row, int col)
{
	DI(di);
	return wintext_get_char_attr(&di->wintext, row, col);
}

void
win32_put_char_attr_rowcol(Driver *drv, int row, int col, int char_attr)
{
	DI(di);
	wintext_put_char_attr(&di->wintext, row, col, char_attr);
}

void
win32_delay(Driver *drv, int ms)
{
	DI(di);

	frame_pump_messages(FALSE);
	if (ms >= 0)
	{
		Sleep(ms);
	}
}

void
win32_get_truecolor(Driver *drv, int x, int y, int *r, int *g, int *b, int *a)
{
	_ASSERTE(0 && "win32_get_truecolor called.");
}

void
win32_put_truecolor(Driver *drv, int x, int y, int r, int g, int b, int a)
{
	_ASSERTE(0 && "win32_put_truecolor called.");
}

void
win32_set_keyboard_timeout(Driver *drv, int ms)
{
	frame_set_keyboard_timeout(ms);
}
