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
void wintext_paintscreen(int x_min, int x_max, int y_min, int y_max);
	Repaints the rectangular portion of the text screen specified by
	the four parameters, which are in character co-ordinates.  This
	routine is called automatically by 'wintext_putstring()' as well as
	other internal routines whenever Windows uncovers the window.  It can
	also be called  manually by your program when it wants a portion
	of the screen updated (the actual data is kept in two arrays, which
	your program has presumably updated:)
		unsigned char m_chars[WINTEXT_MAX_ROW][80]  holds the text
		unsigned char m_attrs[WINTEXT_MAX_ROW][80]  holds the (CGA-style) attributes

void wintext_cursor(int xpos, int ypos, int m_cursor_type);
	Sets the cursor to character position (xpos, ypos) and switches to
	a cursor type specified by 'm_cursor_type': 0 = none, 1 = underline,
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

		The 'me->m_text_mode' flag tracks the current m_text_mode status.
		Note that pressing Alt-F4 closes and destroys the window *and*
		resets this flag (to 1), so the main program should look at
		this flag whenever it is possible that Alt-F4 has been hit!
		('wintext_getkeypress()' returns a 27 (ESCAPE) if this happens)
		(Note that you can use an 'WM_CLOSE' case to handle this situation.)
		The 'me->m_text_mode' values are:
			0 = the initialization routine has never been called!
			1 = text mode is *not* active
			2 = text mode *is* active
		There is also a 'me->m_alt_f4_hit' flag that is non-zero if
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
		left*me->m_char_width, top*me->m_char_height,
		(right + 1)*me->m_char_width, (bot + 1)*me->m_char_height
	};
	if (me->m_window)
	{
		InvalidateRect(me->m_window, &exposed, FALSE);
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
	me->m_instance = hInstance;
	strcpy(me->m_title_text, titletext);
	me->m_parent_window = hWndParent;

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
		wc.hbrBackground = static_cast<HBRUSH>(::GetStockObject(BLACK_BRUSH));
		wc.lpszMenuName =  me->m_title_text;
		wc.lpszClassName = s_window_class;

		return_value = RegisterClass(&wc);
	}

	/* set up the font characteristics */
	me->m_char_font = OEM_FIXED_FONT;
	me->m_font = static_cast<HFONT>(::GetStockObject(me->m_char_font));
	hDC = GetDC(hWndParent);
	hOldFont = static_cast<HFONT>(::SelectObject(hDC, me->m_font));
	GetTextMetrics(hDC, &TextMetric);
	SelectObject(hDC, hOldFont);
	ReleaseDC(hWndParent, hDC);
	me->m_char_width  = TextMetric.tmMaxCharWidth;
	me->m_char_height = TextMetric.tmHeight;
	me->m_char_xchars = WINTEXT_MAX_COL;
	me->m_char_ychars = WINTEXT_MAX_ROW;

	/* maximum screen width */
	me->m_max_width = me->m_char_xchars*me->m_char_width;
	/* maximum screen height */
	me->m_max_height = me->m_char_ychars*me->m_char_height;

	/* set up the font and caret information */
	for (i = 0; i < 3; i++)
	{
		size_t count = NUM_OF(me->m_cursor_pattern[0])*sizeof(me->m_cursor_pattern[0][0]);
		memset(&me->m_cursor_pattern[i][0], 0, count);
	}
	for (j = me->m_char_height-2; j < me->m_char_height; j++)
	{
		me->m_cursor_pattern[1][j] = 0x00ff;
	}
	for (j = 0; j < me->m_char_height; j++)
	{
		me->m_cursor_pattern[2][j] = 0x00ff;
	}
	me->m_bitmap[0] = CreateBitmap(8, me->m_char_height, 1, 1, &me->m_cursor_pattern[0][0]);
	me->m_bitmap[1] = CreateBitmap(8, me->m_char_height, 1, 1, &me->m_cursor_pattern[1][0]);
	me->m_bitmap[2] = CreateBitmap(8, me->m_char_height, 1, 1, &me->m_cursor_pattern[2][0]);

	me->m_text_mode = 1;
	me->m_alt_f4_hit = 0;

	return return_value;
}

/*
		clean-up routine
*/
void wintext_destroy(WinText *me)
{
	int i;

	ODS("wintext_destroy");

	if (me->m_text_mode == 2)  /* text is still active! */
	{
		wintext_textoff(me);
	}
	if (me->m_text_mode != 1)  /* not in the right mode */
	{
		return;
	}

	for (i = 0; i < 3; i++)
	{
		DeleteObject((HANDLE) me->m_bitmap[i]);
	}
	me->m_text_mode = 0;
	me->m_alt_f4_hit = 0;
}


/*
	Set up the text window and clear it
*/
int wintext_texton(WinText *me)
{
	HWND hWnd;

	ODS("wintext_texton");

	if (me->m_text_mode != 1)  /* not in the right mode */
	{
		return 0;
	}

	/* initialize the cursor */
	me->m_cursor_x    = 0;
	me->m_cursor_y    = 0;
	me->m_cursor_type = 0;
	me->m_cursor_owned = 0;
	me->m_showing_cursor = FALSE;

	/* make sure g_me points to me because CreateWindow
	 * is going to call the window procedure.
	 */
	g_me = me;
	hWnd = CreateWindow(s_window_class,
		me->m_title_text,
		(NULL == me->m_parent_window) ? WS_OVERLAPPEDWINDOW : WS_CHILD,
		CW_USEDEFAULT,               /* default horizontal position */
		CW_USEDEFAULT,               /* default vertical position */
		me->m_max_width,
		me->m_max_height,
		me->m_parent_window,
		NULL,
		me->m_instance,
		NULL);
	_ASSERTE(hWnd);

	/* squirrel away a global copy of 'hWnd' for later */
	me->m_window = hWnd;

	me->m_text_mode = 2;
	me->m_alt_f4_hit = 0;

	ShowWindow(me->m_window, SW_SHOWNORMAL);
	UpdateWindow(me->m_window);
	InvalidateRect(me->m_window, NULL, FALSE);

	return 0;
}

/*
	Remove the text window
*/

int wintext_textoff(WinText *me)
{
	ODS("wintext_textoff");
	me->m_alt_f4_hit = 0;
	if (me->m_text_mode != 2)  /* not in the right mode */
	{
		return 0;
	}
	DestroyWindow(me->m_window);
	me->m_text_mode = 1;
	return 0;
}

static void wintext_OnClose(HWND window)
{
	ODS("wintext_OnClose");
	g_me->m_text_mode = 1;
	g_me->m_alt_f4_hit = 1;
}

static void wintext_OnSetFocus(HWND window, HWND old_focus)
{
	ODS("wintext_OnSetFocus");
	/* get focus - display caret */
	/* create caret & display */
	if (TRUE == g_me->m_showing_cursor)
	{
		g_me->m_cursor_owned = 1;
		CreateCaret(g_me->m_window, g_me->m_bitmap[g_me->m_cursor_type], g_me->m_char_width, g_me->m_char_height);
		SetCaretPos(g_me->m_cursor_x*g_me->m_char_width, g_me->m_cursor_y*g_me->m_char_height);
		//SetCaretBlinkTime(500);
		ODS3("======================== Show Caret %d #3 (%d,%d)", ++carrot_count, g_me->m_cursor_x*g_me->m_char_width, g_me->m_cursor_y*g_me->m_char_height);
		ShowCaret(g_me->m_window);
	}
}

static void wintext_OnKillFocus(HWND window, HWND old_focus)
{
	/* kill focus - hide caret */
	ODS("wintext_OnKillFocus");
	if (TRUE == g_me->m_showing_cursor)
	{
		g_me->m_cursor_owned = 0;
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
	int x_min = ps.rcPaint.left/g_me->m_char_width;
	int x_max = (ps.rcPaint.right + g_me->m_char_width - 1)/g_me->m_char_width;
	int y_min = ps.rcPaint.top/g_me->m_char_height;
	int y_max = (ps.rcPaint.bottom + g_me->m_char_height - 1)/g_me->m_char_height;

	ODS("wintext_OnPaint");

	wintext_paintscreen(g_me, x_min, x_max, y_min, y_max);
	EndPaint(window, &ps);
}

static void wintext_OnSize(HWND window, UINT state, int cx, int cy)
{
	ODS("wintext_OnSize");
	if (cx > (WORD)g_me->m_max_width ||
		cy > (WORD)g_me->m_max_height)
	{
		SetWindowPos(window,
			GetNextWindow(window, GW_HWNDPREV),
			0, 0, g_me->m_max_width, g_me->m_max_height, SWP_NOMOVE);
	}
}

static void wintext_OnGetMinMaxInfo(HWND hwnd, LPMINMAXINFO lpMinMaxInfo)
{
	ODS("wintext_OnGetMinMaxInfo");
	lpMinMaxInfo->ptMaxSize.x = g_me->m_max_width;
	lpMinMaxInfo->ptMaxSize.y = g_me->m_max_height;
}

/*
		Window-handling procedure
*/
LRESULT CALLBACK wintext_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (g_me->m_window == NULL)
	{
		g_me->m_window = hWnd;
	}
	else if (hWnd != g_me->m_window)  /* ??? not the text-mode window! */
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
			me->m_chars[j][k] = xc;
			me->m_attrs[j][k] = xa;
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
		char *chars = &me->m_chars[row][0];
		char *attrs = &me->m_attrs[row][0];
		char *next_chars = &me->m_chars[row + 1][0];
		char *next_attrs = &me->m_attrs[row + 1][0];
		int col;

		for (col = 0; col < WINTEXT_MAX_COL; col++)
		{
			*chars++ = *next_chars++;
			*attrs++ = *next_attrs++;
		}
	}
	memset(&me->m_chars[bot][0], 0,  (size_t) WINTEXT_MAX_COL);
	memset(&me->m_attrs[bot][0], 0, (size_t) WINTEXT_MAX_COL);
	invalidate(me, 0, bot, WINTEXT_MAX_COL, top);
}

/*
	general routine to repaint the screen
*/

void wintext_paintscreen(WinText *me,
	int x_min,       /* update this rectangular section */
	int x_max,       /* of the 'screen'                 */
	int y_min,
	int y_max)
{
	int i, j, k;
	int istart, jstart, length, foreground, background;
	unsigned char oldbk;
	unsigned char oldfg;
	HDC hDC;

	ODS("wintext_paintscreen");

	if (me->m_text_mode != 2)  /* not in the right mode */
	{
		return;
	}

	/* first time through?  Initialize the 'screen' */
	if (me->m_buffer_init == 0)
	{
		me->m_buffer_init = 1;
		oldbk = 0x00;
		oldfg = 0x0f;
		k = (oldbk << 4) + oldfg;
		me->m_buffer_init = 1;
		for (i = 0; i < me->m_char_xchars; i++)
		{
			for (j = 0; j < me->m_char_ychars; j++)
			{
				me->m_chars[j][i] = ' ';
				me->m_attrs[j][i] = k;
			}
		}
	}

	if (x_min < 0)
	{
		x_min = 0;
	}
	if (x_max >= me->m_char_xchars)
	{
		x_max = me->m_char_xchars-1;
	}
	if (y_min < 0)
	{
		y_min = 0;
	}
	if (y_max >= me->m_char_ychars)
	{
		y_max = me->m_char_ychars-1;
	}

	hDC = GetDC(me->m_window);
	SelectObject(hDC, me->m_font);
	SetBkMode(hDC, OPAQUE);
	SetTextAlign(hDC, TA_LEFT | TA_TOP);

	if (TRUE == me->m_showing_cursor)
	//if (me->m_cursor_owned != 0)
	{
		ODS1("======================== Hide Caret %d", --carrot_count);
		HideCaret(me->m_window);
	}

	/*
	the following convoluted code minimizes the number of
	discrete calls to the Windows interface by locating
	'strings' of screen locations with common foreground
	and background g_colors
	*/
	for (j = y_min; j <= y_max; j++)
	{
		length = 0;
		oldbk = 99;
		oldfg = 99;
		for (i = x_min; i <= x_max + 1; i++)
		{
			k = -1;
			if (i <= x_max)
			{
				k = me->m_attrs[j][i];
			}
			foreground = (k & 15);
			background = (k >> 4);
			if (i > x_max || foreground != (int)oldfg || background != (int)oldbk)
			{
				if (length > 0)
				{
					SetBkColor(hDC, wintext_color[oldbk]);
					SetTextColor(hDC, wintext_color[oldfg]);
					TextOut(hDC,
						istart*me->m_char_width,
						jstart*me->m_char_height,
						&me->m_chars[jstart][istart],
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

	if (TRUE == me->m_showing_cursor)
	//if (me->m_cursor_owned != 0)
	{
		ODS1("======================== Show Caret %d", ++carrot_count);
		ShowCaret(me->m_window);
	}

	ReleaseDC(me->m_window, hDC);
}

void wintext_cursor(WinText *me, int xpos, int ypos, int m_cursor_type)
{
	int x, y;
	ODS("wintext_cursor");

	if (me->m_text_mode != 2)  /* not in the right mode */
	{
		return;
	}

	me->m_cursor_x = xpos;
	me->m_cursor_y = ypos;
	if (m_cursor_type >= 0)
	{
		me->m_cursor_type = m_cursor_type;
	}
	if (me->m_cursor_type < 0)
	{
		me->m_cursor_type = 0;
	}
	else if (me->m_cursor_type > 2)
	{
		me->m_cursor_type = 2;
	}
	if (FALSE == me->m_showing_cursor)
	{
		x = me->m_cursor_x*me->m_char_width;
		y = me->m_cursor_y*me->m_char_height;
		CreateCaret(me->m_window, me->m_bitmap[me->m_cursor_type],
			me->m_char_width, me->m_char_height);
		SetCaretPos(x, y);
		ODS3("======================== Show Caret %d #2 (%d,%d)", ++carrot_count, x, y);
		ShowCaret(me->m_window);
		me->m_showing_cursor = TRUE;
	}
	else
	{
		/* CreateCaret(me->m_window, me->m_bitmap[me->m_cursor_type],
			me->m_char_width, me->m_char_height); */
		x = me->m_cursor_x*me->m_char_width;
		y = me->m_cursor_y*me->m_char_height;
		SetCaretPos(x, y);
		/*SetCaretBlinkTime(500); */
		ODS2("======================== Set Caret Pos #1 (%d,%d)", x, y);
		/*ShowCaret(me->m_window); */
	}
}

void wintext_set_attr(WinText *me, int row, int col, int attr, int count)
{
	int i;
	int x_min, x_max, y_min, y_max;
	x_min = x_max = col;
	y_min = y_max = row;
	for (i = 0; i < count; i++)
	{
		me->m_attrs[row][col + i] = (unsigned char) (attr & 0xFF);
	}
	if (x_min + count >= WINTEXT_MAX_COL)
	{
		x_min = 0;
		x_max = WINTEXT_MAX_COL-1;
		y_max = (count + WINTEXT_MAX_COL - 1)/WINTEXT_MAX_COL;
	}
	else
	{
		x_max = x_min + count;
	}
	invalidate(me, x_min, y_min, x_max, y_max);
}

void wintext_clear(WinText *me)
{
	int y;
	for (y = 0; y < WINTEXT_MAX_ROW; y++)
	{
		memset(&me->m_chars[y][0], ' ', (size_t) WINTEXT_MAX_COL);
		memset(&me->m_attrs[y][0], ' ', (size_t) WINTEXT_MAX_COL);
	}
	InvalidateRect(me->m_window, NULL, FALSE);
}

BYTE *wintext_screen_get(WinText *me)
{
	size_t count = sizeof(BYTE)*WINTEXT_MAX_ROW*WINTEXT_MAX_COL;
	BYTE *copy = (BYTE *) malloc(count*2);
	_ASSERTE(copy);
	memcpy(copy, me->m_chars, count);
	memcpy(copy + count, me->m_attrs, count);
	return copy;
}

void wintext_screen_set(WinText *me, const BYTE *copy)
{
	size_t count = sizeof(BYTE)*WINTEXT_MAX_ROW*WINTEXT_MAX_COL;
	memcpy(me->m_chars, copy, count);
	memcpy(me->m_attrs, copy + count, count);
	InvalidateRect(me->m_window, NULL, FALSE);
}

void wintext_hide_cursor(WinText *me)
{
	if (TRUE == me->m_showing_cursor)
	{
		me->m_showing_cursor = FALSE;
		HideCaret(me->m_window);
	}
}

static VOID CALLBACK wintext_timer_redraw(HWND window, UINT msg, UINT_PTR idEvent, DWORD dwTime)
{
	InvalidateRect(window, NULL, FALSE);
	KillTimer(window, TIMER_ID);
}

void wintext_schedule_alarm(WinText *me, int delay)
{
	UINT_PTR result = SetTimer(me->m_window, TIMER_ID, delay, wintext_timer_redraw);
	if (!result)
	{
		DWORD error = GetLastError();
		_ASSERTE(result);
	}
}

int wintext_get_char_attr(WinText *me, int row, int col)
{
	return ((me->m_chars[row][col] & 0xFF) << 8) | (me->m_attrs[row][col] & 0xFF);
}

void wintext_put_char_attr(WinText *me, int row, int col, int char_attr)
{
	_ASSERTE(WINTEXT_MAX_ROW > row);
	_ASSERTE(WINTEXT_MAX_COL > col);
	me->m_chars[row][col] = (char_attr >> 8) & 0xFF;
	me->m_attrs[row][col] = (char_attr & 0xFF);
	invalidate(me, col, row, col, row);
}

void wintext_resume(WinText *me)
{
	g_me = me;
}
