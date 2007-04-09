#define _CRTDBG_MAP_ALLOC
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <crtdbg.h>

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>

#include "port.h"
#include "prototyp.h"
#include "fractint.h"

#include "WinText.h"
#include "ods.h"

#define TIMER_ID 1

static int s_showing_cursor = FALSE;

#if defined(RT_VERBOSE)
static int carrot_count = 0;
#endif

#define NUM_OF(ary_) (sizeof(ary_)/sizeof(ary_[0]))

/*
		WINTEXT.C handles the character-based "prompt screens",
		using a 24x80 character-window driver that I wrote originally
		for the Windows port of the DOS-based "Screen Generator"
		commercial package - Bert Tyler

		Modified for Win32 by Richard Thomson

		the subroutines and their functions are:

BOOL wintext_initialize(HANDLE hInstance, LPSTR title);
	Registers and initializes the text window - must be called
	once (and only once).  Its parameters are the handle of the application
	instance and a pointer to a string containing the title of the window.
void wintext_destroy();
	Destroys items like bitmaps that the initialization routine has
	created.  Should be called once (and only once) as your program exits.

int wintext_texton();
	Brings up and blanks out the text window.  No parameters.
int wintext_textoff();
	Removes the text window.  No parameters.

void wintext_putstring(int xpos, int ypos, int attrib, const char *string);
	Sends a character string to the screen starting at (xpos, ypos)
	using the (CGA-style and, yes, it should be a 'char') specified attribute.
void wintext_paintscreen(int xmin, int xmax, int ymin, int ymax);
	Repaints the rectangular portion of the text screen specified by
	the four parameters, which are in character co-ordinates.  This
	routine is called automatically by 'wintext_putstring()' as well as
	other internal routines whenever Windows uncovers the window.  It can
	also be called  manually by your program when it wants a portion
	of the screen updated (the actual data is kept in two arrays, which
	your program has presumably updated:)
		unsigned char chars[WINTEXT_MAX_ROW][80]  holds the text
		unsigned char attrs[WINTEXT_MAX_ROW][80]  holds the (CGA-style) attributes

void wintext_cursor(int xpos, int ypos, int cursor_type);
	Sets the cursor to character position (xpos, ypos) and switches to
	a cursor type specified by 'cursor_type': 0 = none, 1 = underline,
	2 = g_block cursor.  A cursor type of -1 means use whatever cursor
	type (0, 1, or 2) was already active.

unsigned int wintext_getkeypress(int option);
	A simple keypress-retriever that, based on the parameter, either checks
	for any keypress activity (option = 0) or waits for a keypress before
	returning (option != 0).  Returns a 0 to indicate no keystroke, or the
	keystroke itself.  Up to 80 keystrokes are queued in an internal buffer.
	If the text window is not open, returns an ESCAPE keystroke (27).
	The keystroke is returned as an integer value identical to that a
	DOS program receives in AX when it invokes INT 16H, AX = 0 or 1.

int wintext_look_for_activity(int option);
	An internal routine that handles buffered keystrokes and lets
	Windows messaging (multitasking) take place.  Called with option=0,
	it returns without waiting for the presence of a keystroke.  Called
	with option != 0, it waits for a keystroke before returning.  Returns
	1 if a keystroke is pending, 0 if none pending.  Called internally
	(and automatically) by 'wintext_getkeypress()'.
void wintext_addkeypress(unsigned int);
	An internal routine, called by 'wintext_look_for_activity()' and
	'wintext_proc()', that adds keystrokes to an internal buffer.
	Never called directly by the applications program.
long FAR PASCAL wintext_proc(HANDLE, UINT, WPARAM, LPARAM);
	An internal routine that handles all message functions while
	the text window is on the screen.  Never called directly by
	the applications program, but must be referenced as a call-back
	routine by your ".DEF" file.

		The 'me->textmode' flag tracks the current textmode status.
		Note that pressing Alt-F4 closes and destroys the window *and*
		resets this flag (to 1), so the main program should look at
		this flag whenever it is possible that Alt-F4 has been hit!
		('wintext_getkeypress()' returns a 27 (ESCAPE) if this happens)
		(Note that you can use an 'WM_CLOSE' case to handle this situation.)
		The 'me->textmode' values are:
			0 = the initialization routine has never been called!
			1 = text mode is *not* active
			2 = text mode *is* active
		There is also a 'me->AltF4hit' flag that is non-zero if
		the window has been closed (by an Alt-F4, or a WM_CLOSE sequence)
		but the application program hasn't officially closed the window yet.
*/

/* function prototypes */

static LRESULT CALLBACK wintext_proc(HWND, UINT, WPARAM, LPARAM);

static LPCSTR s_window_class = "FractIntText";
static WinText *g_me = NULL;


/* EGA/VGA 16-color palette (which doesn't match Windows palette exactly) */
/*
COLORREF wintext_color[] =
{
	RGB(0, 0, 0),
	RGB(0, 0, 168),
	RGB(0, 168, 0),
	RGB(0, 168, 168),
	RGB(168, 0, 0),
	RGB(168, 0, 168),
	RGB(168, 84, 0),
	RGB(168, 168, 168),
	RGB(84, 84, 84),
	RGB(84, 84, 255),
	RGB(84, 255, 84),
	RGB(84, 255, 255),
	RGB(255, 84, 84),
	RGB(255, 84, 255),
	RGB(255, 255, 84),
	RGB(255, 255, 255)
};
*/
/* 16-color Windows Palette */

static COLORREF wintext_color[] =
{
	RGB(0, 0, 0),
	RGB(0, 0, 128),
	RGB(0, 128, 0),
	RGB(0, 128, 128),
	RGB(128, 0, 0),
	RGB(128, 0, 128),
	RGB(128, 128, 0),
	RGB(192, 192, 192),
/*  RGB(128, 128, 128),  This looks lousy - make it black */
	RGB(0, 0, 0),
	RGB(0, 0, 255),
	RGB(0, 255, 0),
	RGB(0, 255, 255),
	RGB(255, 0, 0),
	RGB(255, 0, 255),
	RGB(255, 255, 0),
	RGB(255, 255, 255)
};

void invalidate(WinText *me, int left, int bot, int right, int top)
{
	RECT exposed =
	{
		left*me->char_width, top*me->char_height,
		(right + 1)*me->char_width, (bot + 1)*me->char_height
	};
	if (me->hWndCopy)
	{
		InvalidateRect(me->hWndCopy, &exposed, FALSE);
	}
}

/*
	Register the text window - a one-time function which perfomrs
	all of the neccessary registration and initialization
*/

BOOL wintext_initialize(WinText *me, HINSTANCE hInstance, HWND hWndParent, LPCSTR titletext)
{
	BOOL return_value;
	HDC hDC;
	HFONT hOldFont;
	TEXTMETRIC TextMetric;
	int i, j;
	WNDCLASS  wc;

	ODS("wintext_initialize");
	me->hInstance = hInstance;
	strcpy(me->title_text, titletext);
	me->hWndParent = hWndParent;

	return_value = GetClassInfo(hInstance, s_window_class, &wc);
	if (!return_value)
	{
		wc.style = 0;
		wc.lpfnWndProc = wintext_proc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = hInstance;
		wc.hIcon = NULL;
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = GetStockObject(BLACK_BRUSH);
		wc.lpszMenuName =  me->title_text;
		wc.lpszClassName = s_window_class;

		return_value = RegisterClass(&wc);
	}

	/* set up the font characteristics */
	me->char_font = OEM_FIXED_FONT;
	me->hFont = GetStockObject(me->char_font);
	hDC = GetDC(hWndParent);
	hOldFont = SelectObject(hDC, me->hFont);
	GetTextMetrics(hDC, &TextMetric);
	SelectObject(hDC, hOldFont);
	ReleaseDC(hWndParent, hDC);
	me->char_width  = TextMetric.tmMaxCharWidth;
	me->char_height = TextMetric.tmHeight;
	me->char_xchars = WINTEXT_MAX_COL;
	me->char_ychars = WINTEXT_MAX_ROW;

	/* maximum screen width */
	me->max_width = me->char_xchars*me->char_width;
	/* maximum screen height */
	me->max_height = me->char_ychars*me->char_height;

	/* set up the font and caret information */
	for (i = 0; i < 3; i++)
	{
		size_t count = NUM_OF(me->cursor_pattern[0])*sizeof(me->cursor_pattern[0][0]);
		memset(&me->cursor_pattern[i][0], 0, count);
	}
	for (j = me->char_height-2; j < me->char_height; j++)
	{
		me->cursor_pattern[1][j] = 0x00ff;
	}
	for (j = 0; j < me->char_height; j++)
	{
		me->cursor_pattern[2][j] = 0x00ff;
	}
	me->bitmap[0] = CreateBitmap(8, me->char_height, 1, 1, &me->cursor_pattern[0][0]);
	me->bitmap[1] = CreateBitmap(8, me->char_height, 1, 1, &me->cursor_pattern[1][0]);
	me->bitmap[2] = CreateBitmap(8, me->char_height, 1, 1, &me->cursor_pattern[2][0]);

	me->textmode = 1;
	me->AltF4hit = 0;

	return return_value;
}

/*
		clean-up routine
*/
void wintext_destroy(WinText *me)
{
	int i;

	ODS("wintext_destroy");

	if (me->textmode == 2)  /* text is still active! */
	{
		wintext_textoff(me);
	}
	if (me->textmode != 1)  /* not in the right mode */
	{
		return;
	}

	for (i = 0; i < 3; i++)
	{
		DeleteObject((HANDLE) me->bitmap[i]);
	}
	me->textmode = 0;
	me->AltF4hit = 0;
}


/*
	Set up the text window and clear it
*/
int wintext_texton(WinText *me)
{
	HWND hWnd;

	ODS("wintext_texton");

	if (me->textmode != 1)  /* not in the right mode */
	{
		return 0;
	}

	/* initialize the cursor */
	me->cursor_x    = 0;
	me->cursor_y    = 0;
	me->cursor_type = 0;
	me->cursor_owned = 0;
	me->showing_cursor = FALSE;

	/* make sure g_me points to me because CreateWindow
	 * is going to call the window procedure.
	 */
	g_me = me;
	hWnd = CreateWindow(s_window_class,
		me->title_text,
		(NULL == me->hWndParent) ? WS_OVERLAPPEDWINDOW : WS_CHILD,
		CW_USEDEFAULT,               /* default horizontal position */
		CW_USEDEFAULT,               /* default vertical position */
		me->max_width,
		me->max_height,
		me->hWndParent,
		NULL,
		me->hInstance,
		NULL);
	_ASSERTE(hWnd);

	/* squirrel away a global copy of 'hWnd' for later */
	me->hWndCopy = hWnd;

	me->textmode = 2;
	me->AltF4hit = 0;

	ShowWindow(me->hWndCopy, SW_SHOWNORMAL);
	UpdateWindow(me->hWndCopy);
	InvalidateRect(me->hWndCopy, NULL, FALSE);

	return 0;
}

/*
	Remove the text window
*/

int wintext_textoff(WinText *me)
{
	ODS("wintext_textoff");
	me->AltF4hit = 0;
	if (me->textmode != 2)  /* not in the right mode */
	{
		return 0;
	}
	DestroyWindow(me->hWndCopy);
	me->textmode = 1;
	return 0;
}

static void wintext_OnClose(HWND window)
{
	ODS("wintext_OnClose");
	g_me->textmode = 1;
	g_me->AltF4hit = 1;
}

static void wintext_OnSetFocus(HWND window, HWND old_focus)
{
	ODS("wintext_OnSetFocus");
	/* get focus - display caret */
	/* create caret & display */
	if (TRUE == g_me->showing_cursor)
	{
		g_me->cursor_owned = 1;
		CreateCaret(g_me->hWndCopy, g_me->bitmap[g_me->cursor_type], g_me->char_width, g_me->char_height);
		SetCaretPos(g_me->cursor_x*g_me->char_width, g_me->cursor_y*g_me->char_height);
		//SetCaretBlinkTime(500);
		ODS3("======================== Show Caret %d #3 (%d,%d)", ++carrot_count, g_me->cursor_x*g_me->char_width, g_me->cursor_y*g_me->char_height);
		ShowCaret(g_me->hWndCopy);
	}
}

static void wintext_OnKillFocus(HWND window, HWND old_focus)
{
	/* kill focus - hide caret */
	ODS("wintext_OnKillFocus");
	if (TRUE == g_me->showing_cursor)
	{
		g_me->cursor_owned = 0;
		ODS1("======================== Hide Caret %d", --carrot_count);
		HideCaret(window);
		DestroyCaret();
	}
}

void wintext_set_focus(void)
{
	wintext_OnSetFocus(NULL, NULL);
}

void wintext_kill_focus(void)
{
	wintext_OnKillFocus(NULL, NULL);
}

static void wintext_OnPaint(HWND window)
{
	PAINTSTRUCT ps;
	HDC hDC = BeginPaint(window, &ps);

	/* the routine below handles *all* window updates */
	int xmin = ps.rcPaint.left/g_me->char_width;
	int xmax = (ps.rcPaint.right + g_me->char_width - 1)/g_me->char_width;
	int ymin = ps.rcPaint.top/g_me->char_height;
	int ymax = (ps.rcPaint.bottom + g_me->char_height - 1)/g_me->char_height;

	ODS("wintext_OnPaint");

	wintext_paintscreen(g_me, xmin, xmax, ymin, ymax);
	EndPaint(window, &ps);
}

static void wintext_OnSize(HWND window, UINT state, int cx, int cy)
{
	ODS("wintext_OnSize");
	if (cx > (WORD)g_me->max_width ||
		cy > (WORD)g_me->max_height)
	{
		SetWindowPos(window,
			GetNextWindow(window, GW_HWNDPREV),
			0, 0, g_me->max_width, g_me->max_height, SWP_NOMOVE);
	}
}

static void wintext_OnGetMinMaxInfo(HWND hwnd, LPMINMAXINFO lpMinMaxInfo)
{
	ODS("wintext_OnGetMinMaxInfo");
	lpMinMaxInfo->ptMaxSize.x = g_me->max_width;
	lpMinMaxInfo->ptMaxSize.y = g_me->max_height;
}

/*
		Window-handling procedure
*/
LRESULT CALLBACK wintext_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (g_me->hWndCopy == NULL)
	{
		g_me->hWndCopy = hWnd;
	}
	else if (hWnd != g_me->hWndCopy)  /* ??? not the text-mode window! */
	{
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	switch (message)
	{
	case WM_GETMINMAXINFO:	HANDLE_WM_GETMINMAXINFO(hWnd, wParam, lParam, wintext_OnGetMinMaxInfo); break;
	case WM_CLOSE:			HANDLE_WM_CLOSE(hWnd, wParam, lParam, wintext_OnClose);			break;
	case WM_SIZE:			HANDLE_WM_SIZE(hWnd, wParam, lParam, wintext_OnSize);			break;
	case WM_SETFOCUS:		HANDLE_WM_SETFOCUS(hWnd, wParam, lParam, wintext_OnSetFocus);	break;
	case WM_KILLFOCUS:		HANDLE_WM_KILLFOCUS(hWnd, wParam, lParam, wintext_OnKillFocus); break;
	case WM_PAINT:			HANDLE_WM_PAINT(hWnd, wParam, lParam, wintext_OnPaint);			break;
	default:				return DefWindowProc(hWnd, message, wParam, lParam);			break;
	}
	return 0;
}

/*
		general routine to send a string to the screen
*/

void wintext_putstring(WinText *me, int xpos, int ypos, int attrib, const char *string, int *end_row, int *end_col)
{
	int i, j, k, maxrow, maxcol;
	char xc, xa;

	ODS("wintext_putstring");

	xa = (attrib & 0x0ff);
	j = maxrow = ypos;
	k = maxcol = xpos-1;

	for (i = 0; (xc = string[i]) != 0; i++)
	{
		if (xc == 13 || xc == 10)
		{
			if (j < WINTEXT_MAX_ROW-1)
			{
				j++;
			}
			k = xpos-1;
		}
		else
		{
			if ((++k) >= WINTEXT_MAX_COL)
			{
				if (j < WINTEXT_MAX_ROW-1)
				{
					j++;
				}
				k = xpos;
			}
			if (maxrow < j)
			{
				maxrow = j;
			}
			if (maxcol < k)
			{
				maxcol = k;
			}
			me->chars[j][k] = xc;
			me->attrs[j][k] = xa;
		}
	}
	if (i > 0)
	{
		invalidate(me, xpos, ypos, maxcol, maxrow);
		*end_row = j;
		*end_col = k + 1;
	}
}

void wintext_scroll_up(WinText *me, int top, int bot)
{
	int row;
	for (row = top; row < bot; row++)
	{
		unsigned char *chars = &me->chars[row][0];
		unsigned char *attrs = &me->attrs[row][0];
		unsigned char *next_chars = &me->chars[row + 1][0];
		unsigned char *next_attrs = &me->attrs[row + 1][0];
		int col;

		for (col = 0; col < WINTEXT_MAX_COL; col++)
		{
			*chars++ = *next_chars++;
			*attrs++ = *next_attrs++;
		}
	}
	memset(&me->chars[bot][0], 0,  (size_t) WINTEXT_MAX_COL);
	memset(&me->attrs[bot][0], 0, (size_t) WINTEXT_MAX_COL);
	invalidate(me, 0, bot, WINTEXT_MAX_COL, top);
}

/*
	general routine to repaint the screen
*/

void wintext_paintscreen(WinText *me,
	int xmin,       /* update this rectangular section */
	int xmax,       /* of the 'screen'                 */
	int ymin,
	int ymax)
{
	int i, j, k;
	int istart, jstart, length, foreground, background;
	unsigned char oldbk;
	unsigned char oldfg;
	HDC hDC;

	ODS("wintext_paintscreen");

	if (me->textmode != 2)  /* not in the right mode */
	{
		return;
	}

	/* first time through?  Initialize the 'screen' */
	if (me->buffer_init == 0)
	{
		me->buffer_init = 1;
		oldbk = 0x00;
		oldfg = 0x0f;
		k = (oldbk << 4) + oldfg;
		me->buffer_init = 1;
		for (i = 0; i < me->char_xchars; i++)
		{
			for (j = 0; j < me->char_ychars; j++)
			{
				me->chars[j][i] = ' ';
				me->attrs[j][i] = k;
			}
		}
	}

	if (xmin < 0)
	{
		xmin = 0;
	}
	if (xmax >= me->char_xchars)
	{
		xmax = me->char_xchars-1;
	}
	if (ymin < 0)
	{
		ymin = 0;
	}
	if (ymax >= me->char_ychars)
	{
		ymax = me->char_ychars-1;
	}

	hDC = GetDC(me->hWndCopy);
	SelectObject(hDC, me->hFont);
	SetBkMode(hDC, OPAQUE);
	SetTextAlign(hDC, TA_LEFT | TA_TOP);

	if (TRUE == me->showing_cursor)
	//if (me->cursor_owned != 0)
	{
		ODS1("======================== Hide Caret %d", --carrot_count);
		HideCaret(me->hWndCopy);
	}

	/*
	the following convoluted code minimizes the number of
	discrete calls to the Windows interface by locating
	'strings' of screen locations with common foreground
	and background g_colors
	*/
	for (j = ymin; j <= ymax; j++)
	{
		length = 0;
		oldbk = 99;
		oldfg = 99;
		for (i = xmin; i <= xmax + 1; i++)
		{
			k = -1;
			if (i <= xmax)
			{
				k = me->attrs[j][i];
			}
			foreground = (k & 15);
			background = (k >> 4);
			if (i > xmax || foreground != (int)oldfg || background != (int)oldbk)
			{
				if (length > 0)
				{
					SetBkColor(hDC, wintext_color[oldbk]);
					SetTextColor(hDC, wintext_color[oldfg]);
					TextOut(hDC,
						istart*me->char_width,
						jstart*me->char_height,
						&me->chars[jstart][istart],
						length);
				}
				oldbk = background;
				oldfg = foreground;
				istart = i;
				jstart = j;
				length = 0;
			}
			length++;
		}
	}

	if (TRUE == me->showing_cursor)
	//if (me->cursor_owned != 0)
	{
		ODS1("======================== Show Caret %d", ++carrot_count);
		ShowCaret(me->hWndCopy);
	}

	ReleaseDC(me->hWndCopy, hDC);
}

void wintext_cursor(WinText *me, int xpos, int ypos, int cursor_type)
{
	int x, y;
	ODS("wintext_cursor");

	if (me->textmode != 2)  /* not in the right mode */
	{
		return;
	}

	me->cursor_x = xpos;
	me->cursor_y = ypos;
	if (cursor_type >= 0)
	{
		me->cursor_type = cursor_type;
	}
	if (me->cursor_type < 0)
	{
		me->cursor_type = 0;
	}
	else if (me->cursor_type > 2)
	{
		me->cursor_type = 2;
	}
	if (FALSE == me->showing_cursor)
	{
		x = me->cursor_x*me->char_width;
		y = me->cursor_y*me->char_height;
		CreateCaret(me->hWndCopy, me->bitmap[me->cursor_type],
			me->char_width, me->char_height);
		SetCaretPos(x, y);
		ODS3("======================== Show Caret %d #2 (%d,%d)", ++carrot_count, x, y);
		ShowCaret(me->hWndCopy);
		me->showing_cursor = TRUE;
	}
	else
	//if (me->cursor_owned != 0)
	{
		/* CreateCaret(me->hWndCopy, me->bitmap[me->cursor_type],
			me->char_width, me->char_height); */
		x = me->cursor_x*me->char_width;
		y = me->cursor_y*me->char_height;
		SetCaretPos(x, y);
		/*SetCaretBlinkTime(500); */
		ODS2("======================== Set Caret Pos #1 (%d,%d)", x, y);
		/*ShowCaret(me->hWndCopy); */
	}
}

void wintext_set_attr(WinText *me, int row, int col, int attr, int count)
{
	int i;
	int xmin, xmax, ymin, ymax;
	xmin = xmax = col;
	ymin = ymax = row;
	for (i = 0; i < count; i++)
	{
		me->attrs[row][col + i] = (unsigned char) (attr & 0xFF);
	}
	if (xmin + count >= WINTEXT_MAX_COL)
	{
		xmin = 0;
		xmax = WINTEXT_MAX_COL-1;
		ymax = (count + WINTEXT_MAX_COL - 1)/WINTEXT_MAX_COL;
	}
	else
	{
		xmax = xmin + count;
	}
	invalidate(me, xmin, ymin, xmax, ymax);
}

void wintext_clear(WinText *me)
{
	int y;
	for (y = 0; y < WINTEXT_MAX_ROW; y++)
	{
		memset(&me->chars[y][0], ' ', (size_t) WINTEXT_MAX_COL);
		memset(&me->attrs[y][0], ' ', (size_t) WINTEXT_MAX_COL);
	}
	InvalidateRect(me->hWndCopy, NULL, FALSE);
}

BYTE *wintext_screen_get(WinText *me)
{
	size_t count = sizeof(BYTE)*WINTEXT_MAX_ROW*WINTEXT_MAX_COL;
	BYTE *copy = (BYTE *) malloc(count*2);
	_ASSERTE(copy);
	memcpy(copy, me->chars, count);
	memcpy(copy + count, me->attrs, count);
	return copy;
}

void wintext_screen_set(WinText *me, const BYTE *copy)
{
	size_t count = sizeof(BYTE)*WINTEXT_MAX_ROW*WINTEXT_MAX_COL;
	memcpy(me->chars, copy, count);
	memcpy(me->attrs, copy + count, count);
	InvalidateRect(me->hWndCopy, NULL, FALSE);
}

void wintext_hide_cursor(WinText *me)
{
	if (TRUE == me->showing_cursor)
	{
		me->showing_cursor = FALSE;
		HideCaret(me->hWndCopy);
	}
}

static VOID CALLBACK wintext_timer_redraw(HWND window, UINT msg, UINT_PTR idEvent, DWORD dwTime)
{
	InvalidateRect(window, NULL, FALSE);
	KillTimer(window, TIMER_ID);
}

void wintext_schedule_alarm(WinText *me, int delay)
{
	UINT_PTR result = SetTimer(me->hWndCopy, TIMER_ID, delay, wintext_timer_redraw);
	if (!result)
	{
		DWORD error = GetLastError();
		_ASSERTE(result);
	}
}

int wintext_get_char_attr(WinText *me, int row, int col)
{
	return ((me->chars[row][col] & 0xFF) << 8) | (me->attrs[row][col] & 0xFF);
}

void wintext_put_char_attr(WinText *me, int row, int col, int char_attr)
{
	_ASSERTE(WINTEXT_MAX_ROW > row);
	_ASSERTE(WINTEXT_MAX_COL > col);
	me->chars[row][col] = (char_attr >> 8) & 0xFF;
	me->attrs[row][col] = (char_attr & 0xFF);
	invalidate(me, col, row, col, row);
}

void wintext_resume(WinText *me)
{
	g_me = me;
}
