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

#if defined(RT_VERBOSE)
static int carrot_count = 0;
#endif

/*
		WINTEXT.C handles the character-based "prompt screens",
		using a 24x80 character-window driver that I wrote originally
		for the Windows port of the DOS-based "Screen Generator"
		commercial package - Bert Tyler

		Modified for Win32 by Richard Thomson

		the subroutines and their functions are:

BOOL WinText::initialize(HANDLE hInstance, LPSTR title);
	Registers and initializes the text window - must be called
	once (and only once).  Its parameters are the handle of the application
	instance and a pointer to a string containing the title of the window.
void WinText::destroy();
	Destroys items like bitmaps that the initialization routine has
	created.  Should be called once (and only once) as your program exits.

int WinText::texton();
	Brings up and blanks out the text window.  No parameters.
int WinText::textoff();
	Removes the text window.  No parameters.

void WinText::putstring(int xpos, int ypos, int attrib, const char *string);
	Sends a character string to the screen starting at (xpos, ypos)
	using the (CGA-style and, yes, it should be a 'char') specified attribute.
void WinText::paintscreen(int x_min, int x_max, int y_min, int y_max);
	Repaints the rectangular portion of the text screen specified by
	the four parameters, which are in character co-ordinates.  This
	routine is called automatically by 'WinText::putstring()' as well as
	other internal routines whenever Windows uncovers the window.  It can
	also be called  manually by your program when it wants a portion
	of the screen updated (the actual data is kept in two arrays, which
	your program has presumably updated:)
		unsigned char m_chars[WINTEXT_MAX_ROW][80]  holds the text
		unsigned char m_attrs[WINTEXT_MAX_ROW][80]  holds the (CGA-style) attributes

void WinText::cursor(int xpos, int ypos, int m_cursor_type);
	Sets the cursor to character position (xpos, ypos) and switches to
	a cursor type specified by 'm_cursor_type': 0 = none, 1 = underline,
	2 = g_block cursor.  A cursor type of -1 means use whatever cursor
	type (0, 1, or 2) was already active.

unsigned int WinText::getkeypress(int option);
	A simple keypress-retriever that, based on the parameter, either checks
	for any keypress activity (option = 0) or waits for a keypress before
	returning (option != 0).  Returns a 0 to indicate no keystroke, or the
	keystroke itself.  Up to 80 keystrokes are queued in an internal buffer.
	If the text window is not open, returns an ESCAPE keystroke (27).
	The keystroke is returned as an integer value identical to that a
	DOS program receives in AX when it invokes INT 16H, AX = 0 or 1.

int WinText::look_for_activity(int option);
	An internal routine that handles buffered keystrokes and lets
	Windows messaging (multitasking) take place.  Called with option=0,
	it returns without waiting for the presence of a keystroke.  Called
	with option != 0, it waits for a keystroke before returning.  Returns
	1 if a keystroke is pending, 0 if none pending.  Called internally
	(and automatically) by 'WinText::getkeypress()'.
void WinText::addkeypress(unsigned int);
	An internal routine, called by 'WinText::look_for_activity()' and
	'WinText::proc()', that adds keystrokes to an internal buffer.
	Never called directly by the applications program.
long FAR PASCAL WinText::proc(HANDLE, UINT, WPARAM, LPARAM);
	An internal routine that handles all message functions while
	the text window is on the screen.  Never called directly by
	the applications program, but must be referenced as a call-back
	routine by your ".DEF" file.

		The 'm_text_mode' flag tracks the current m_text_mode status.
		Note that pressing Alt-F4 closes and destroys the window *and*
		resets this flag (to 1), so the main program should look at
		this flag whenever it is possible that Alt-F4 has been hit!
		('WinText::getkeypress()' returns a 27 (ESCAPE) if this happens)
		(Note that you can use an 'WM_CLOSE' case to handle this situation.)
		The 'm_text_mode' values are:
			0 = the initialization routine has never been called!
			1 = text mode is *not* active
			2 = text mode *is* active
		There is also a 'm_alt_f4_hit' flag that is non-zero if
		the window has been closed (by an Alt-F4, or a WM_CLOSE sequence)
		but the application program hasn't officially closed the window yet.
*/

LPCTSTR WinText::s_window_class = "FractIntText";
bool WinText::s_showing_cursor = false;
WinText *WinText::s_me = NULL;

WinText::WinText() :
	m_text_mode(0),
	m_alt_f4_hit(0),
	m_showing_cursor(0),
	m_buffer_init(0),
	m_font(0),
	m_char_font(0),
	m_char_width(0),
	m_char_height(0),
	m_char_xchars(0),
	m_char_ychars(0),
	m_max_width(0),
	m_max_height(0),
	m_cursor_x(0),
	m_cursor_y(0),
	m_cursor_type(0),
	m_cursor_owned(0),
	m_window(0),
	m_parent_window(0),
	m_instance(0)
{
	for (int i = 0; i < WINTEXT_MAX_ROW; i++)
	{
		for (int j = 0; i < WINTEXT_MAX_COL; i++)
		{
			m_chars[i][j] = 0;
			m_attrs[i][j] = 0;
		}
	}
	for (int i = 0; i < NUM_OF(m_bitmap); i++)
	{
		m_bitmap[i] = 0;
		for (int j = 0; j < 40; j++)
		{
			m_cursor_pattern[i][j] = 0;
		}
	}
	for (int i = 0; i < NUM_OF(m_title_text); i++)
	{
		m_title_text[i] = 0;
	}
}

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

void WinText::invalidate(int left, int bot, int right, int top)
{
	RECT exposed =
	{
		left*m_char_width, top*m_char_height,
		(right + 1)*m_char_width, (bot + 1)*m_char_height
	};
	if (m_window)
	{
		::InvalidateRect(m_window, &exposed, FALSE);
	}
}

/*
	Register the text window - a one-time function which perfomrs
	all of the neccessary registration and initialization
*/

BOOL WinText::initialize(HINSTANCE hInstance, HWND hWndParent, LPCSTR titletext)
{
	BOOL return_value;
	HDC hDC;
	HFONT hOldFont;
	TEXTMETRIC TextMetric;
	WNDCLASS  wc;

	ODS("wintext_initialize");
	m_instance = hInstance;
	::strcpy(m_title_text, titletext);
	m_parent_window = hWndParent;

	return_value = ::GetClassInfo(hInstance, s_window_class, &wc);
	if (!return_value)
	{
		wc.style = 0;
		wc.lpfnWndProc = proc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = hInstance;
		wc.hIcon = NULL;
		wc.hCursor = ::LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = static_cast<HBRUSH>(::GetStockObject(BLACK_BRUSH));
		wc.lpszMenuName =  m_title_text;
		wc.lpszClassName = s_window_class;

		return_value = ::RegisterClass(&wc);
	}

	/* set up the font characteristics */
	m_char_font = OEM_FIXED_FONT;
	m_font = static_cast<HFONT>(::GetStockObject(m_char_font));
	hDC = ::GetDC(hWndParent);
	hOldFont = static_cast<HFONT>(::SelectObject(hDC, m_font));
	::GetTextMetrics(hDC, &TextMetric);
	::SelectObject(hDC, hOldFont);
	::ReleaseDC(hWndParent, hDC);
	m_char_width  = TextMetric.tmMaxCharWidth;
	m_char_height = TextMetric.tmHeight;
	m_char_xchars = WINTEXT_MAX_COL;
	m_char_ychars = WINTEXT_MAX_ROW;

	/* maximum screen width */
	m_max_width = m_char_xchars*m_char_width;
	/* maximum screen height */
	m_max_height = m_char_ychars*m_char_height;

	/* set up the font and caret information */
	for (int i = 0; i < 3; i++)
	{
		size_t count = NUM_OF(m_cursor_pattern[0])*sizeof(m_cursor_pattern[0][0]);
		::memset(&m_cursor_pattern[i][0], 0, count);
	}
	for (int i = m_char_height-2; i < m_char_height; i++)
	{
		m_cursor_pattern[1][i] = 0x00ff;
	}
	for (int i = 0; i < m_char_height; i++)
	{
		m_cursor_pattern[2][i] = 0x00ff;
	}
	m_bitmap[0] = ::CreateBitmap(8, m_char_height, 1, 1, &m_cursor_pattern[0][0]);
	m_bitmap[1] = ::CreateBitmap(8, m_char_height, 1, 1, &m_cursor_pattern[1][0]);
	m_bitmap[2] = ::CreateBitmap(8, m_char_height, 1, 1, &m_cursor_pattern[2][0]);

	m_text_mode = 1;
	m_alt_f4_hit = 0;

	return return_value;
}

/*
		clean-up routine
*/
void WinText::destroy()
{
	int i;

	ODS("wintext_destroy");

	if (m_text_mode == 2)  /* text is still active! */
	{
		textoff();
	}
	if (m_text_mode != 1)  /* not in the right mode */
	{
		return;
	}

	for (i = 0; i < 3; i++)
	{
		::DeleteObject(static_cast<HGDIOBJ>(m_bitmap[i]));
	}
	m_text_mode = 0;
	m_alt_f4_hit = 0;
}


/*
	Set up the text window and clear it
*/
void WinText::create(HWND parent)
{
	ODS("wintext_create");

	if (m_text_mode != 1)  /* not in the right mode */
	{
		return;
	}

	/* initialize the cursor */
	m_cursor_x    = 0;
	m_cursor_y    = 0;
	m_cursor_type = 0;
	m_cursor_owned = 0;
	m_showing_cursor = FALSE;
	m_parent_window = parent;

	if (m_window)
	{
		::DestroyWindow(m_window);
		m_window = 0;
	}

	/* make sure s_me points to me because CreateWindow
	 * is going to call the window procedure.
	 */
	s_me = this;
	HWND hWnd = ::CreateWindow(s_window_class,
		m_title_text,
		(NULL == m_parent_window) ? WS_OVERLAPPEDWINDOW : WS_CHILD,
		CW_USEDEFAULT,               /* default horizontal position */
		CW_USEDEFAULT,               /* default vertical position */
		m_max_width,
		m_max_height,
		m_parent_window,
		NULL,
		m_instance,
		NULL);
	_ASSERTE(hWnd);

	/* squirrel away a global copy of 'hWnd' for later */
	m_window = hWnd;

	m_text_mode = 2;
	m_alt_f4_hit = 0;

	::ShowWindow(m_window, SW_SHOWNORMAL);
	::UpdateWindow(m_window);
	::InvalidateRect(m_window, NULL, FALSE);
}

/*
	Remove the text window
*/

int WinText::textoff()
{
	ODS("wintext_textoff");
	m_alt_f4_hit = 0;
	if (m_text_mode != 2)  /* not in the right mode */
	{
		return 0;
	}
	::DestroyWindow(m_window);
	m_text_mode = 1;
	return 0;
}

void WinText::OnClose(HWND window)
{
	ODS("wintext_OnClose");
	s_me->m_text_mode = 1;
	s_me->m_alt_f4_hit = 1;
}

void WinText::OnSetFocus(HWND window, HWND old_focus)
{
	ODS("wintext_OnSetFocus");
	/* get focus - display caret */
	/* create caret & display */
	if (TRUE == s_me->m_showing_cursor)
	{
		s_me->m_cursor_owned = 1;
		::CreateCaret(s_me->m_window, s_me->m_bitmap[s_me->m_cursor_type], s_me->m_char_width, s_me->m_char_height);
		::SetCaretPos(s_me->m_cursor_x*s_me->m_char_width, s_me->m_cursor_y*s_me->m_char_height);
		//::SetCaretBlinkTime(500);
		::ShowCaret(s_me->m_window);
	}
}

void WinText::OnKillFocus(HWND window, HWND old_focus)
{
	/* kill focus - hide caret */
	ODS("wintext_OnKillFocus");
	if (TRUE == s_me->m_showing_cursor)
	{
		s_me->m_cursor_owned = 0;
		::HideCaret(window);
		::DestroyCaret();
	}
}

void WinText::set_focus(void)
{
	OnSetFocus(NULL, NULL);
}

void WinText::kill_focus(void)
{
	OnKillFocus(NULL, NULL);
}

void WinText::OnPaint(HWND window)
{
	PAINTSTRUCT ps;
	HDC hDC = ::BeginPaint(window, &ps);

	/* the routine below handles *all* window updates */
	int x_min = ps.rcPaint.left/s_me->m_char_width;
	int x_max = (ps.rcPaint.right + s_me->m_char_width - 1)/s_me->m_char_width;
	int y_min = ps.rcPaint.top/s_me->m_char_height;
	int y_max = (ps.rcPaint.bottom + s_me->m_char_height - 1)/s_me->m_char_height;

	ODS("wintext_OnPaint");

	s_me->paintscreen(x_min, x_max, y_min, y_max);
	::EndPaint(window, &ps);
}

void WinText::OnSize(HWND window, UINT state, int cx, int cy)
{
	ODS("wintext_OnSize");
	if (cx > (WORD)s_me->m_max_width ||
		cy > (WORD)s_me->m_max_height)
	{
		::SetWindowPos(window,
			::GetNextWindow(window, GW_HWNDPREV),
			0, 0, s_me->m_max_width, s_me->m_max_height, SWP_NOMOVE);
	}
}

void WinText::OnGetMinMaxInfo(HWND hwnd, LPMINMAXINFO lpMinMaxInfo)
{
	ODS("wintext_OnGetMinMaxInfo");
	lpMinMaxInfo->ptMaxSize.x = s_me->m_max_width;
	lpMinMaxInfo->ptMaxSize.y = s_me->m_max_height;
}

/*
		Window-handling procedure
*/
LRESULT CALLBACK WinText::proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (s_me->m_window == NULL)
	{
		s_me->m_window = hWnd;
	}
	else if (hWnd != s_me->m_window)  /* ??? not the text-mode window! */
	{
		return ::DefWindowProc(hWnd, message, wParam, lParam);
	}

	switch (message)
	{
	case WM_GETMINMAXINFO:	HANDLE_WM_GETMINMAXINFO(hWnd, wParam, lParam, OnGetMinMaxInfo); break;
	case WM_CLOSE:			HANDLE_WM_CLOSE(hWnd, wParam, lParam, OnClose);			break;
	case WM_SIZE:			HANDLE_WM_SIZE(hWnd, wParam, lParam, OnSize);			break;
	case WM_SETFOCUS:		HANDLE_WM_SETFOCUS(hWnd, wParam, lParam, OnSetFocus);	break;
	case WM_KILLFOCUS:		HANDLE_WM_KILLFOCUS(hWnd, wParam, lParam, OnKillFocus); break;
	case WM_PAINT:			HANDLE_WM_PAINT(hWnd, wParam, lParam, OnPaint);			break;
	default:				return DefWindowProc(hWnd, message, wParam, lParam);			break;
	}
	return 0;
}

/*
		general routine to send a string to the screen
*/

void WinText::putstring(int xpos, int ypos, int attrib, const char *text, int *end_row, int *end_col)
{
	int j, k, maxrow, maxcol;
	char xc, xa;

	ODS("wintext_putstring");

	xa = (attrib & 0x0ff);
	j = maxrow = ypos;
	k = maxcol = xpos-1;

	for (int i = 0; (xc = text[i]) != 0; i++)
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
			m_chars[j][k] = xc;
			m_attrs[j][k] = xa;
		}
	}
	if (text[0] != 0)
	{
		invalidate(xpos, ypos, maxcol, maxrow);
		*end_row = j;
		*end_col = k + 1;
	}
}

void WinText::scroll_up(int top, int bot)
{
	for (int row = top; row < bot; row++)
	{
		char *chars = &m_chars[row][0];
		char *attrs = &m_attrs[row][0];
		char *next_chars = &m_chars[row + 1][0];
		char *next_attrs = &m_attrs[row + 1][0];
		for (int col = 0; col < WINTEXT_MAX_COL; col++)
		{
			*chars++ = *next_chars++;
			*attrs++ = *next_attrs++;
		}
	}
	::memset(&m_chars[bot][0], 0,  (size_t) WINTEXT_MAX_COL);
	::memset(&m_attrs[bot][0], 0, (size_t) WINTEXT_MAX_COL);
	invalidate(0, bot, WINTEXT_MAX_COL, top);
}

/*
	general routine to repaint the screen
*/

void WinText::paintscreen(
	int x_min,       /* update this rectangular section */
	int x_max,       /* of the 'screen'                 */
	int y_min,
	int y_max)
{
	int k;
	int istart, jstart, length, foreground, background;
	unsigned char oldbk;
	unsigned char oldfg;
	HDC hDC;

	ODS("wintext_paintscreen");

	if (m_text_mode != 2)  /* not in the right mode */
	{
		return;
	}

	/* first time through?  Initialize the 'screen' */
	if (m_buffer_init == 0)
	{
		m_buffer_init = 1;
		oldbk = 0x00;
		oldfg = 0x0f;
		k = (oldbk << 4) + oldfg;
		m_buffer_init = 1;
		for (int i = 0; i < m_char_xchars; i++)
		{
			for (int j = 0; j < m_char_ychars; j++)
			{
				m_chars[j][i] = ' ';
				m_attrs[j][i] = k;
			}
		}
	}

	if (x_min < 0)
	{
		x_min = 0;
	}
	if (x_max >= m_char_xchars)
	{
		x_max = m_char_xchars-1;
	}
	if (y_min < 0)
	{
		y_min = 0;
	}
	if (y_max >= m_char_ychars)
	{
		y_max = m_char_ychars-1;
	}

	hDC = ::GetDC(m_window);
	::SelectObject(hDC, m_font);
	::SetBkMode(hDC, OPAQUE);
	::SetTextAlign(hDC, TA_LEFT | TA_TOP);

	if (TRUE == m_showing_cursor)
	{
		::HideCaret(m_window);
	}

	/*
	the following convoluted code minimizes the number of
	discrete calls to the Windows interface by locating
	'strings' of screen locations with common foreground
	and background g_colors
	*/
	for (int j = y_min; j <= y_max; j++)
	{
		length = 0;
		oldbk = 99;
		oldfg = 99;
		for (int i = x_min; i <= x_max + 1; i++)
		{
			k = -1;
			if (i <= x_max)
			{
				k = m_attrs[j][i];
			}
			foreground = (k & 15);
			background = (k >> 4);
			if (i > x_max || foreground != (int)oldfg || background != (int)oldbk)
			{
				if (length > 0)
				{
					::SetBkColor(hDC, wintext_color[oldbk]);
					::SetTextColor(hDC, wintext_color[oldfg]);
					::TextOut(hDC,
						istart*m_char_width,
						jstart*m_char_height,
						&m_chars[jstart][istart],
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

	if (TRUE == m_showing_cursor)
	{
		::ShowCaret(m_window);
	}

	::ReleaseDC(m_window, hDC);
}

void WinText::cursor(int xpos, int ypos, int m_cursor_type)
{
	int x, y;
	ODS("wintext_cursor");

	if (m_text_mode != 2)  /* not in the right mode */
	{
		return;
	}

	m_cursor_x = xpos;
	m_cursor_y = ypos;
	if (m_cursor_type >= 0)
	{
		m_cursor_type = m_cursor_type;
	}
	if (m_cursor_type < 0)
	{
		m_cursor_type = 0;
	}
	else if (m_cursor_type > 2)
	{
		m_cursor_type = 2;
	}
	if (FALSE == m_showing_cursor)
	{
		x = m_cursor_x*m_char_width;
		y = m_cursor_y*m_char_height;
		::CreateCaret(m_window, m_bitmap[m_cursor_type],
			m_char_width, m_char_height);
		::SetCaretPos(x, y);
		::ShowCaret(m_window);
		m_showing_cursor = TRUE;
	}
	else
	{
		/* CreateCaret(m_window, m_bitmap[m_cursor_type],
			m_char_width, m_char_height); */
		x = m_cursor_x*m_char_width;
		y = m_cursor_y*m_char_height;
		::SetCaretPos(x, y);
	}
}

void WinText::set_attr(int row, int col, int attr, int count)
{
	int i;
	int x_min, x_max, y_min, y_max;
	x_min = x_max = col;
	y_min = y_max = row;
	for (i = 0; i < count; i++)
	{
		m_attrs[row][col + i] = (unsigned char) (attr & 0xFF);
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
	invalidate(x_min, y_min, x_max, y_max);
}

void WinText::clear()
{
	for (int y = 0; y < WINTEXT_MAX_ROW; y++)
	{
		::memset(&m_chars[y][0], ' ', (size_t) WINTEXT_MAX_COL);
		::memset(&m_attrs[y][0], ' ', (size_t) WINTEXT_MAX_COL);
	}
	::InvalidateRect(m_window, NULL, FALSE);
}

BYTE *WinText::screen_get()
{
	size_t count = sizeof(BYTE)*WINTEXT_MAX_ROW*WINTEXT_MAX_COL;
	BYTE *copy = (BYTE *) malloc(count*2);
	_ASSERTE(copy);
	::memcpy(copy, m_chars, count);
	::memcpy(copy + count, m_attrs, count);
	return copy;
}

void WinText::screen_set(const BYTE *copy)
{
	size_t count = sizeof(BYTE)*WINTEXT_MAX_ROW*WINTEXT_MAX_COL;
	::memcpy(m_chars, copy, count);
	::memcpy(m_attrs, copy + count, count);
	::InvalidateRect(m_window, NULL, FALSE);
}

void WinText::hide_cursor()
{
	if (TRUE == m_showing_cursor)
	{
		m_showing_cursor = FALSE;
		::HideCaret(m_window);
	}
}

VOID CALLBACK WinText::timer_redraw(HWND window, UINT msg, UINT_PTR idEvent, DWORD dwTime)
{
	::InvalidateRect(window, NULL, FALSE);
	::KillTimer(window, TIMER_ID);
}

void WinText::schedule_alarm(int delay)
{
	UINT_PTR result = ::SetTimer(m_window, TIMER_ID, delay, timer_redraw);
	if (!result)
	{
		DWORD error = ::GetLastError();
		_ASSERTE(result);
	}
}

int WinText::get_char_attr(int row, int col)
{
	return ((m_chars[row][col] & 0xFF) << 8) | (m_attrs[row][col] & 0xFF);
}

void WinText::put_char_attr(int row, int col, int char_attr)
{
	_ASSERTE(WINTEXT_MAX_ROW > row);
	_ASSERTE(WINTEXT_MAX_COL > col);
	m_chars[row][col] = (char_attr >> 8) & 0xFF;
	m_attrs[row][col] = (char_attr & 0xFF);
	invalidate(col, row, col, row);
}

void WinText::pause()
{
	::ShowWindow(m_window, SW_HIDE);
	s_me = NULL;
}

void WinText::resume()
{
	s_me = this;
	::ShowWindow(m_window, SW_SHOW);
}
