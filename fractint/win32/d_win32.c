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

/*
; New (Apr '90) mouse code by Pieter Branderhorst follows.
; The variable lookatmouse controls it all.  Callers of keypressed and
; getakey should set lookatmouse to:
;      0  ignore the mouse entirely
;     <0  only test for left button click; if it occurs return fake key
;           number 0-lookatmouse
;      1  return enter key for left button, arrow keys for mouse movement,
;           mouse sensitivity is suitable for graphics cursor
;      2  same as 1 but sensitivity is suitable for text cursor
;      3  specials for zoombox, left/right double-clicks generate fake
;           keys, mouse movement generates a variety of fake keys
;           depending on state of buttons
; Mouse movement is accumulated & saved across calls.  Eg if mouse has been
; moved up-right quickly, the next few calls to getakey might return:
;      right,right,up,right,up
; Minor jiggling of the mouse generates no keystroke, and is forgotten (not
; accumulated with additional movement) if no additional movement in the
; same direction occurs within a short interval.
; Movements on angles near horiz/vert are treated as horiz/vert; the nearness
; tolerated varies depending on mode.
; Any movement not picked up by calling routine within a short time of mouse
; stopping is forgotten.  (This does not apply to button pushes in modes<3.)
; Mouseread would be more accurate if interrupt-driven, but with the usage
; in fractint (tight getakey loops while mouse active) there's no need.

; translate table for mouse movement -> fake keys
mousefkey dw   1077,1075,1080,1072  ; right,left,down,up     just movement
        dw        0,   0,1081,1073  ;            ,pgdn,pgup  + left button
        dw    1144,1142,1147,1146  ; kpad+,kpad-,cdel,cins  + rt   button
        dw    1117,1119,1118,1132  ; ctl-end,home,pgdn,pgup + mid/multi
*/
int mousefkey[16] =
{
	FIK_RIGHT_ARROW,	FIK_LEFT_ARROW,	FIK_DOWN_ARROW,		FIK_UP_ARROW,
	0,					0,				FIK_PAGE_DOWN,		FIK_PAGE_UP,
	FIK_CTL_PLUS,		FIK_CTL_MINUS,	FIK_CTL_DEL,		FIK_CTL_INSERT,
	FIK_CTL_END,		FIK_CTL_HOME,	FIK_CTL_PAGE_DOWN,	FIK_CTL_PAGE_UP
};

int lookatmouse = LOOK_MOUSE_NONE;
static int previous_look_mouse = LOOK_MOUSE_NONE;
static int mousetime = 0;				/* time of last mouseread call */
static int mlbtimer = 0;				/* time of left button 1st click */
static int mrbtimer = 0;				/* time of right button 1st click */
static int mhtimer = 0;					/* time of last horiz move */
static int mvtimer = 0;					/* time of last vert  move */
static int mhmickeys = 0;				/* pending horiz movement */
static int mvmickeys = 0;				/* pending vert  movement */
static int mbstatus = 0;				/* status of mouse buttons */
static int mbclicks = 0;				/* had 1 click so far? &1 mlb, &2 mrb */
#define MOUSE_LEFT_CLICK 1
#define MOUSE_RIGHT_CLICK 2

/* timed save variables, handled by readmouse: */
static int savechktime = 0;				/* time of last autosave check */
long savebase = 0;						/* base clock ticks */
long saveticks = 0;						/* save after this many ticks */
int finishrow = 0;						/* save when this row is finished */

/*
DclickTime    equ 9   ; ticks within which 2nd click must occur
JitterTime    equ 6   ; idle ticks before turfing unreported mickeys
TextHSens     equ 22  ; horizontal sensitivity in text mode
TextVSens     equ 44  ; vertical sensitivity in text mode
GraphSens     equ 5   ; sensitivity in graphics mode; gets lower @ higher res
ZoomSens      equ 20  ; sensitivity for zoom box sizing/rotation
TextVHLimit   equ 6   ; treat angles < 1:6  as straight
GraphVHLimit  equ 14  ; treat angles < 1:14 as straight
ZoomVHLimit   equ 1   ; treat angles < 1:1  as straight
JitterMickeys equ 3   ; mickeys to ignore before noticing motion
*/
#define DclickTime 9
#define JitterTime 6
#define TextHSens 22
#define TextVSens 44
#define GraphSens 5
#define ZoomSens 20
#define TextVHLimit 6
#define GraphVHLimit 14
#define ZoomVHLimit 1
#define JitterMickeys 3

int left_button_pressed(void)
{
	return 0;
}

int left_button_released(void)
{
	return 0;
}

int right_button_pressed(void)
{
	return 0;
}

int right_button_released(void)
{
	return 0;
}

int button_states(void)
{
	return 0;
}

int get_mouse_motion(void)
{
	return 0;
}

int mouse_x = 0;
int mouse_y = 0;

int mouseread(void)
{
	int ax, bx, cx, dx;
	int moveaxis = 0;
	int ticker = readticker();

	if (saveticks && (ticker != savechktime))
	{
		savechktime = ticker;
		ticker -= savebase;
		if (ticker > saveticks)
		{
			if (finishrow == 1)
			{
				if (calc_status != CALCSTAT_IN_PROGRESS)
				{
					if ((got_status != GOT_STATUS_12PASS) && (got_status != GOT_STATUS_GUESSING))
					{
						finishrow = currow;
						goto mouse0;
					}
				}
			}
			else if (currow == finishrow)
			{
				goto mouse0;
			}
			timedsave = TRUE;
			return 9999;
		}
	}

mouse0:
	if (lookatmouse != previous_look_mouse)
	{
		/* lookatmouse changed, reset everything */
		previous_look_mouse = lookatmouse;
		mbclicks = 0;
		mbstatus = 0;
		mhmickeys = 0;
		mvmickeys = 0;
	}
	if (lookatmouse != LOOK_MOUSE_NONE)
	{
		if (readticker() != mousetime)
		{
			goto mnewtick;
		}
		if (lookatmouse >= 0)
		{
			goto mouse5;
		}
	}
	return 0;

mnewtick:
	mousetime = readticker();
	if (LOOK_MOUSE_ZOOM_BOX == lookatmouse)
	{
		goto mouse2;
	}
	if (left_button_pressed())
	{
		goto mleftb;
	}
	if (lookatmouse < 0)
	{
		return 0;
	}
	goto mouse3;

mleftb:
	if (lookatmouse > LOOK_MOUSE_NONE)
	{
		return FIK_ENTER;
	}
	return -lookatmouse;

mouse2:
	if (left_button_released())
	{
		if (mbclicks & MOUSE_LEFT_CLICK)
		{
			goto mslbgo;
		}

		mlbtimer = mousetime;
		mbclicks |= MOUSE_LEFT_CLICK;
		goto mousrb;
	}
	else
	{
		goto msnolb;
	}

mslbgo:
	mbclicks = ~MOUSE_LEFT_CLICK;
	return FIK_ENTER;

msnolb:
	if (mousetime - mlbtimer > DclickTime)
	{
		mbclicks = ~MOUSE_LEFT_CLICK;
	}

mousrb:
	if (right_button_pressed())
	{
		if (mbclicks & MOUSE_RIGHT_CLICK)
		{
			goto msrbgo;
		}
		mrbtimer = mousetime;
		mbclicks |= MOUSE_RIGHT_CLICK;
		goto mouse3;
	}
	else
	{
		goto msnorb;
	}

msrbgo:
	mbclicks &= ~MOUSE_RIGHT_CLICK;
	return FIK_CTL_ENTER;

msnorb:
	if (mousetime - mrbtimer > DclickTime)
	{
		mbclicks |= ~MOUSE_LEFT_CLICK;
	}

mouse3:
	if (button_states() != mbstatus)
	{
		mbstatus = button_states();
		mhmickeys = 0;
		mvmickeys = 0;
	}
	get_mouse_motion();
	if (mouse_x > 0)
	{
		goto moushm;
	}
	if (mousetime - mhtimer > JitterTime)
	{
		goto mousev;
	}
	mhmickeys = 0;
	goto mousev;

moushm:
	mhmickeys += mouse_x;

mousev:
	if (mouse_y > 0)
	{
		goto mousvm;
	}
	if (mousetime - mvtimer > JitterTime)
	{
		goto mouse5;
	}
	mvmickeys = 0;
	goto mouse5;

mousvm:
	mvtimer = mousetime;
	mvmickeys += mouse_y;

mouse5:
	bx = (mhmickeys > 0) ? mhmickeys : -mhmickeys;
	cx = (mvmickeys > 0) ? mvmickeys : -mvmickeys;
	moveaxis = 0;
	if (bx < cx)
	{
		int tmp = bx;
		bx = cx;
		cx = tmp;
		moveaxis = 1;
	}

	if (LOOK_MOUSE_TEXT == lookatmouse)
	{
		ax = TextVHLimit;
		goto mangl2;
	}
	if (LOOK_MOUSE_ZOOM_BOX != lookatmouse)
	{
		ax = GraphVHLimit;
		goto mangl2;
	}
	if (mbstatus == 0)
	{
		ax = ZoomVHLimit;
		goto mangl2;
	}

mangl2:
	ax *= cx;
	if (ax > bx)
	{
		goto mchkmv;
	}
	if (moveaxis != 0)
	{
		goto mzeroh;
	}
	mvmickeys = 0;
	goto mchkmv;

mzeroh:
	mhmickeys = 0;

mchkmv:
	if (LOOK_MOUSE_TEXT == lookatmouse)
	{
		goto mchkmt;
	}
	dx = ZoomSens + JitterMickeys;
	if (LOOK_MOUSE_ZOOM_BOX != lookatmouse)
	{
		goto mchkmg;
	}
	if (mbstatus != 0)
	{
		goto mchkm2;
	}

mchkmg:
	dx = GraphSens;
	cx = sxdots;

mchkg2:
	if (cx < 400)
	{
		goto mchkg3;
	}
	cx >>= 1;
	dx >>= 1;
	dx++;
	goto mchkg2;

mchkg3:
	dx += JitterMickeys;
	goto mchkm2;

mchkmt:
	dx = TextVSens + JitterMickeys;
	if (moveaxis != 0)
	{
		goto mchkm2;
	}
	dx = TextHSens + JitterMickeys;

mchkm2:
	if (bx >= dx)
	{
		goto mmove;
	}
	return 0;

mmove:
	dx -= JitterMickeys;
	if (moveaxis != 0)
	{
		goto mmovev;
	}
	if (mhmickeys < 0)
	{
		goto mmovh2;
	}
	mhmickeys -= dx;
	bx = 0;
	goto mmoveb;

mmovh2:
	mhmickeys += dx;
	bx = 2;
	goto mmoveb;

mmovev:
	if (mvmickeys < 0)
	{
		goto mmovv2;
	}
	mvmickeys -= dx;
	bx = 4;
	goto mmoveb;

mmovv2:
	mvmickeys += dx;
	bx = 6;

mmoveb:
	if (LOOK_MOUSE_ZOOM_BOX == lookatmouse)
	{
		goto mmovek;
	}
	if (mbstatus != 1)
	{
		goto mmovb2;
	}
	bx += 8;
	goto mmovek;

mmovb2:
	if (mbstatus != 2)
	{
		goto mmovb3;
	}
	bx += 16;
	goto mmovek;

mmovb3:
	if (mbstatus == 0)
	{
		goto mmovek;
	}
	bx += 24;

mmovek:
	ax = mousefkey[bx];

	return ax;
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

	di->saved_cursor[di->screen_count+1] = g_text_row*80 + g_text_col;
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
