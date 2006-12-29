#define _CRTDBG_MAP_ALLOC
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <crtdbg.h>

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include "WinText.h"

static int s_showing_cursor = FALSE;

#if defined(RT_VERBOSE)
int carrot_count = 0;
extern void ods(const char *file, unsigned int line, const char *format, ...);
#define ODS(fmt_)				ods(__FILE__, __LINE__, fmt_)
#define ODS1(fmt_, _1)			ods(__FILE__, __LINE__, fmt_, _1)
#define ODS2(fmt_, _1, _2)		ods(__FILE__, __LINE__, fmt_, _1, _2)
#define ODS3(fmt_, _1, _2, _3)	ods(__FILE__, __LINE__, fmt_, _1, _2, _3)
#else
#define ODS(fmt_)
#define ODS1(fmt_, _1)
#define ODS2(fmt_, _1, _2)
#define ODS3(fmt_, _1, _2, _3)
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
    2 = block cursor.  A cursor type of -1 means use whatever cursor
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
    with option !=0, it waits for a keystroke before returning.  Returns
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

        The 'g_wintext.textmode' flag tracks the current textmode status.
        Note that pressing Alt-F4 closes and destroys the window *and*
        resets this flag (to 1), so the main program should look at
        this flag whenever it is possible that Alt-F4 has been hit!
        ('wintext_getkeypress()' returns a 27 (ESCAPE) if this happens)
        (Note that you can use an 'WM_CLOSE' case to handle this situation.)
        The 'g_wintext.textmode' values are:
                0 = the initialization routine has never been called!
                1 = text mode is *not* active
                2 = text mode *is* active
        There is also a 'g_wintext.AltF4hit' flag that is non-zero if
        the window has been closed (by an Alt-F4, or a WM_CLOSE sequence)
        but the application program hasn't officially closed the window yet.
*/

struct tagWinText_Globals
{
	int textmode;
	int AltF4hit;
};
typedef struct tagWinText_Globals WinText_Globals;
static WinText_Globals g_wintext =
{
	0,
	0
};

/* function prototypes */

static LRESULT CALLBACK wintext_proc(HWND, UINT, WPARAM, LPARAM);

struct tagWinText_Instance
{
	/* Local copy of the "screen" characters and attributes */
	unsigned char chars[WINTEXT_MAX_ROW][WINTEXT_MAX_COL];
	unsigned char attrs[WINTEXT_MAX_ROW][WINTEXT_MAX_COL];
	int buffer_init;     /* zero if 'screen' is uninitialized */

	/* font information */

	HFONT hFont;
	int char_font;
	int char_width;
	int char_height;
	int char_xchars;
	int char_ychars;
	int max_width;
	int max_height;

	/* "cursor" variables (AKA the "caret" in Window-Speak) */
	int cursor_x;
	int cursor_y;
	int cursor_type;
	int cursor_owned;
	HBITMAP bitmap[3];
	short cursor_pattern[3][40];

	LPSTR title_text;         /* title-bar text */
};
typedef struct tagWinText_Instance WinText_Instance;
static WinText_Instance g_me = { 0 };


/* a few Windows variables we need to remember globally */

HWND wintext_hWndCopy;                /* a Global copy of hWnd */
static HWND wintext_hWndParent;              /* a Global copy of hWnd's Parent */
static HINSTANCE wintext_hInstance;             /* a global copy of hInstance */

/* the keypress buffer */

#define BUFMAX 80
static unsigned int  wintext_keypress_count;
static unsigned int  wintext_keypress_head;
static unsigned int  wintext_keypress_tail;
static unsigned char wintext_keypress_initstate;
static unsigned int  wintext_keypress_buffer[BUFMAX];
static unsigned char wintext_keypress_state[BUFMAX];

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

void invalidate(int left, int bot, int right, int top)
{
	RECT exposed =
	{
		left*g_me.char_width, top*g_me.char_height,
		(right+1)*g_me.char_width, (bot+1)*g_me.char_height			
	};
	InvalidateRect(wintext_hWndCopy, &exposed, FALSE);
}

/*
     Register the text window - a one-time function which perfomrs
     all of the neccessary registration and initialization
*/

BOOL wintext_initialize(HINSTANCE hInstance, HWND hWndParent, LPSTR titletext)
{
    WNDCLASS  wc;
    BOOL return_value;
    HDC hDC;
    HFONT hOldFont;
    TEXTMETRIC TextMetric;
    int i, j;

	ODS("wintext_initialize");
    wintext_hInstance = hInstance;
    g_me.title_text = titletext;
    wintext_hWndParent = hWndParent;

    wc.style = 0;
    wc.lpfnWndProc = wintext_proc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = GetStockObject(BLACK_BRUSH);
    wc.lpszMenuName =  titletext;
    wc.lpszClassName = "FractintForWindowsV0011";

    return_value = RegisterClass(&wc);

    /* set up the font characteristics */
    g_me.char_font = OEM_FIXED_FONT;
    g_me.hFont = GetStockObject(g_me.char_font);
    hDC=GetDC(hWndParent);
    hOldFont = SelectObject(hDC, g_me.hFont);
    GetTextMetrics(hDC, &TextMetric);
    SelectObject(hDC, hOldFont);
    ReleaseDC(hWndParent, hDC);
    g_me.char_width  = TextMetric.tmMaxCharWidth;
    g_me.char_height = TextMetric.tmHeight;
    g_me.char_xchars = WINTEXT_MAX_COL;
    g_me.char_ychars = WINTEXT_MAX_ROW;

	/* maximum screen width */
    g_me.max_width = g_me.char_xchars*g_me.char_width + GetSystemMetrics(SM_CXFRAME)*2;
    /* maximum screen height */
	g_me.max_height = g_me.char_ychars*g_me.char_height + GetSystemMetrics(SM_CYFRAME)*2
            - 1 + GetSystemMetrics(SM_CYCAPTION);

    /* set up the font and caret information */
    for (i = 0; i < 3; i++)
	{
		size_t count = NUM_OF(g_me.cursor_pattern[0])*sizeof(g_me.cursor_pattern[0][0]);
		memset(&g_me.cursor_pattern[i][0], 0, count);
	}
    for (j = g_me.char_height-2; j < g_me.char_height; j++)
	{
        g_me.cursor_pattern[1][j] = 0x00ff;
	}
    for (j = 0; j < g_me.char_height; j++)
	{
        g_me.cursor_pattern[2][j] = 0x00ff;
	}
    g_me.bitmap[0] = CreateBitmap(8, g_me.char_height, 1, 1, &g_me.cursor_pattern[0][0]);
    g_me.bitmap[1] = CreateBitmap(8, g_me.char_height, 1, 1, &g_me.cursor_pattern[1][0]);
    g_me.bitmap[2] = CreateBitmap(8, g_me.char_height, 1, 1, &g_me.cursor_pattern[2][0]);

    g_wintext.textmode = 1;
    g_wintext.AltF4hit = 0;

    return return_value;
}

/*
        clean-up routine
*/
void wintext_destroy(void)
{
	int i;

	ODS("wintext_destroy");

	if (g_wintext.textmode == 2)  /* text is still active! */
	{
        wintext_textoff();
	}
    if (g_wintext.textmode != 1)  /* not in the right mode */
	{
        return;
	}

    for (i = 0; i < 3; i++)
	{
        DeleteObject((HANDLE) g_me.bitmap[i]);
	}
    g_wintext.textmode = 0;
    g_wintext.AltF4hit = 0;
}


/*
     Set up the text window and clear it
*/
int wintext_texton(void)
{
    HWND hWnd;

	ODS("wintext_texton");

    if (g_wintext.textmode != 1)  /* not in the right mode */
	{
        return 0;
	}

    /* initialize the cursor */
    g_me.cursor_x    = 0;
    g_me.cursor_y    = 0;
    g_me.cursor_type = 0;
    g_me.cursor_owned = 0;
	s_showing_cursor = FALSE;

    /* clear the keyboard buffer */
    wintext_keypress_count = 0;
    wintext_keypress_head  = 0;
    wintext_keypress_tail  = 0;
    wintext_keypress_initstate = 0;
    g_me.buffer_init = 0;

    hWnd = CreateWindow("FractintForWindowsV0011",
        g_me.title_text,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,               /* default horizontal position */
        CW_USEDEFAULT,               /* default vertical position */
        g_me.max_width,
        g_me.max_height,
        wintext_hWndParent,
        NULL,
        wintext_hInstance,
        NULL);

    /* squirrel away a global copy of 'hWnd' for later */
    wintext_hWndCopy = hWnd;

    g_wintext.textmode = 2;
    g_wintext.AltF4hit = 0;

    ShowWindow(wintext_hWndCopy, SW_SHOWNORMAL);
    UpdateWindow(wintext_hWndCopy);
    InvalidateRect(wintext_hWndCopy, NULL, FALSE);

    return 0;
}

/*
     Remove the text window
*/

int wintext_textoff(void)
{
	ODS("wintext_textoff");
    g_wintext.AltF4hit = 0;
    if (g_wintext.textmode != 2)  /* not in the right mode */
	{
        return 0;
	}
    DestroyWindow(wintext_hWndCopy);
    g_wintext.textmode = 1;
    return 0;
}

static void wintext_OnClose(HWND window)
{
	ODS("wintext_OnClose");
	g_wintext.textmode = 1;
	g_wintext.AltF4hit = 1;
}

static void wintext_OnSetFocus(HWND window, HWND old_focus)
{
	ODS("wintext_OnSetFocus");
	/* get focus - display caret */
	/* create caret & display */
	if (TRUE == s_showing_cursor)
	{
		g_me.cursor_owned = 1;
		CreateCaret(wintext_hWndCopy, g_me.bitmap[g_me.cursor_type], g_me.char_width, g_me.char_height);
		SetCaretPos(g_me.cursor_x*g_me.char_width, g_me.cursor_y*g_me.char_height);
		//SetCaretBlinkTime(500);
		ODS3("======================== Show Caret %d #3 (%d,%d)", ++carrot_count, g_me.cursor_x*g_me.char_width, g_me.cursor_y*g_me.char_height);
		ShowCaret(wintext_hWndCopy);
	}
}

static void wintext_OnKillFocus(HWND window, HWND old_focus)
{
	/* kill focus - hide caret */
	ODS("wintext_OnKillFocus");
	if (TRUE == s_showing_cursor)
	{
		g_me.cursor_owned = 0;
		ODS1("======================== Hide Caret %d", --carrot_count);
		HideCaret(window);
		DestroyCaret();
	}
}

static void wintext_OnPaint(HWND window)
{
    PAINTSTRUCT ps;
    HDC hDC = BeginPaint(window, &ps);
    RECT tempRect;

	ODS("wintext_OnPaint");

	/* "Paint" routine - call the worker routine */
	GetUpdateRect(window, &tempRect, FALSE);
	ValidateRect(window, &tempRect);

	/* the routine below handles *all* window updates */
	wintext_paintscreen(0, g_me.char_xchars-1, 0, g_me.char_ychars-1);
	EndPaint(window, &ps);
}

static void wintext_OnKeyDown(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
{
	/* KEYUP, KEYDOWN, and CHAR msgs go to the 'keypressed' code */
	/* a key has been pressed - maybe ASCII, maybe not */
	/* if it's an ASCII key, 'WM_CHAR' will handle it  */
	unsigned int i, j, k;
	ODS("wintext_OnKeyDown");
	i = MapVirtualKey(vk, 0);
	j = MapVirtualKey(vk, 2);
	k = (i << 8) + j;
	if (vk == VK_SHIFT || vk == VK_CONTROL) /* shift or ctl key */
	{
		//j = 0;       /* send flag: special key down */
		k = 0xff00 + (unsigned int) vk;
	}
	if (j == 0)        /* use this call only for non-ASCII keys */
	{
		wintext_addkeypress(1000 + i);
	}
}

static void wintext_OnKeyUp(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
{
	/* KEYUP, KEYDOWN, and CHAR msgs go to the SG code */
	/* a key has been released - maybe ASCII, maybe not */
	/* Watch for Shift, Ctl keys  */
	unsigned int i, j, k;
	ODS("wintext_OnKeyUp");
	i = MapVirtualKey(vk, 0);
	j = MapVirtualKey(vk, 2);
	k = (i << 8) + j;
	j = 1;
	if (vk == VK_SHIFT || vk == VK_CONTROL) /* shift or ctl key */
	{
		//j = 0;       /* send flag: special key up */
		k = 0xfe00 + vk;
	}
	if (j == 0)        /* use this call only for non-ASCII keys */
	{
		wintext_addkeypress(k);
	}
}

static void wintext_OnChar(HWND hwnd, TCHAR ch, int cRepeat)
{
	/* KEYUP, KEYDOWN, and CHAR msgs go to the SG code */
	/* an ASCII key has been pressed */
	unsigned int i, j, k;
	ODS("wintext_OnChar");
	i = (unsigned int)((cRepeat & 0x00ff0000) >> 16);
	j = ch;
	k = (i << 8) + j;
	wintext_addkeypress(k);
}

static void wintext_OnSize(HWND window, UINT state, int cx, int cy)
{
	ODS("wintext_OnSize");
	if (cx > (WORD)g_me.max_width ||
		cy > (WORD)g_me.max_height)
	{
		SetWindowPos(window,
			GetNextWindow(window, GW_HWNDPREV),
			0, 0, g_me.max_width, g_me.max_height, SWP_NOMOVE);
	}
}

static void wintext_OnGetMinMaxInfo(HWND hwnd, LPMINMAXINFO lpMinMaxInfo)
{
	ODS("wintext_OnGetMinMaxInfo");
	lpMinMaxInfo->ptMaxSize.x = g_me.max_width;
	lpMinMaxInfo->ptMaxSize.y = g_me.max_height;
}

/*
        Window-handling procedure
*/
LRESULT CALLBACK wintext_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (wintext_hWndCopy == NULL)
	{
		wintext_hWndCopy = hWnd;
	}
	else if (hWnd != wintext_hWndCopy)  /* ??? not the text-mode window! */
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
	case WM_KEYDOWN:		HANDLE_WM_KEYDOWN(hWnd, wParam, lParam, wintext_OnKeyDown);		break;
	case WM_KEYUP:			HANDLE_WM_KEYUP(hWnd, wParam, lParam, wintext_OnKeyUp);			break;
	case WM_CHAR:			HANDLE_WM_CHAR(hWnd, wParam, lParam, wintext_OnChar);			break;
	default:				return DefWindowProc(hWnd, message, wParam, lParam);			break;
    }
    return 0;
}

/*
     simple keyboard logic capable of handling 80
     typed-ahead keyboard characters (more, if BUFMAX is changed)
          wintext_addkeypress() inserts a new keypress
          wintext_getkeypress(0) returns keypress-available info
          wintext_getkeypress(1) takes away the oldest keypress
*/

void wintext_addkeypress(unsigned int keypress)
{
	ODS("wintext_addkeypress");

	if (g_wintext.textmode != 2)  /* not in the right mode */
		return;

	if (wintext_keypress_count >= BUFMAX)
		/* no room */
		return;

	if ((keypress & 0xfe00) == 0xfe00)
	{
		if (keypress == 0xff10) wintext_keypress_initstate |= 0x01;
		if (keypress == 0xfe10) wintext_keypress_initstate &= 0xfe;
		if (keypress == 0xff11) wintext_keypress_initstate |= 0x02;
		if (keypress == 0xfe11) wintext_keypress_initstate &= 0xfd;
		return;
    }

	if (wintext_keypress_initstate != 0)              /* shift/ctl key down */
	{
		if ((wintext_keypress_initstate & 1) != 0)    /* shift key down */
		{
			if ((keypress & 0x00ff) == 9)             /* TAB key down */
			{
                keypress = (15 << 8);             /* convert to shift-tab */
			}
			if ((keypress & 0x00ff) == 0)           /* special character */
			{
				int i;
				i = (keypress >> 8) & 0xff;
				if (i >= 59 && i <= 68)               /* F1 thru F10 */
				{
					keypress = ((i + 25) << 8);       /* convert to Shifted-Fn */
				}
            }
        }
		else                                        /* control key down */
		{
			if ((keypress & 0x00ff) == 0)           /* special character */
			{
				int i;
				i = ((keypress & 0xff00) >> 8);
				if (i >= 59 && i <= 68)               /* F1 thru F10 */
				{
					keypress = ((i + 35) << 8);       /* convert to Ctl-Fn */
				}
				if (i == 71) keypress = (119 << 8);
				if (i == 73) keypress = (unsigned int)(132 << 8);
				if (i == 75) keypress = (115 << 8);
	            if (i == 77) keypress = (116 << 8);
				if (i == 79) keypress = (117 << 8);
				if (i == 81) keypress = (118 << 8);
            }
        }
    }

	wintext_keypress_buffer[wintext_keypress_head] = keypress;
	wintext_keypress_state[wintext_keypress_head] = wintext_keypress_initstate;
	if (++wintext_keypress_head >= BUFMAX)
	{
		wintext_keypress_head = 0;
	}
	wintext_keypress_count++;
}

unsigned int wintext_getkeypress(int option)
{
	int i;

	ODS("wintext_getkeypress");
	wintext_look_for_activity(option);

	if (g_wintext.textmode != 2)  /* not in the right mode */
	{
		return 27;
	}

	if (wintext_keypress_count == 0)
	{
		_ASSERTE(option == 0);
		return 0;
	}

	i = wintext_keypress_buffer[wintext_keypress_tail];

	if (option != 0)
	{
		if (++wintext_keypress_tail >= BUFMAX)
		{
			wintext_keypress_tail = 0;
		}
		wintext_keypress_count--;
	}

	return i;
}


/*
     simple event-handler and look-for-keyboard-activity process

     called with parameter:
          0 = tell me if a key was pressed
          1 = wait for a keypress
     returns:
          0 = no activity
          1 = key pressed
*/

int wintext_look_for_activity(int wintext_waitflag)
{
	MSG msg;

	ODS("wintext_look_for_activity");

	if (g_wintext.textmode != 2)  /* not in the right mode */
	{
		return 0;
	}

	for (;;)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE) == 0)
		{
			if (wintext_waitflag == 0 || wintext_keypress_count != 0 || g_wintext.textmode != 2)
			{
				return (wintext_keypress_count == 0) ? 0 : 1;
			}
		}

		if (GetMessage(&msg, NULL, 0, 0))
		{
			// translate accelerator here?
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return wintext_keypress_count == 0 ? 0 : 1;
}

/*
        general routine to send a string to the screen
*/

void wintext_putstring(int xpos, int ypos, int attrib, const char *string, int *end_row, int *end_col)
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
            if (j < WINTEXT_MAX_ROW-1) j++;
            k = xpos-1;
		}
        else
		{
            if ((++k) >= WINTEXT_MAX_COL)
			{
                if (j < WINTEXT_MAX_ROW-1) j++;
                k = xpos;
            }
            if (maxrow < j) maxrow = j;
            if (maxcol < k) maxcol = k;
            g_me.chars[j][k] = xc;
            g_me.attrs[j][k] = xa;
        }
    }
    if (i > 0)
	{
#if 0
		wintext_paintscreen(xpos, maxcol, ypos, maxrow);
#else
		invalidate(xpos, ypos, maxcol, maxrow);
		*end_row = j;
		*end_col = k+1;
#endif
    }
}

void wintext_scroll_up(int top, int bot)
{
	int row;
	for (row = top; row < bot; row++)
	{
		unsigned char *chars = &g_me.chars[row][0];
		unsigned char *attrs = &g_me.attrs[row][0];
		unsigned char *next_chars = &g_me.chars[row+1][0];
		unsigned char *next_attrs = &g_me.attrs[row+1][0];
		int col;

		for (col = 0; col < WINTEXT_MAX_COL; col++)
		{
			*chars++ = *next_chars++;
			*attrs++ = *next_attrs++;
		}
	}
	memset(&g_me.chars[bot][0], 0,  (size_t) WINTEXT_MAX_COL);
	memset(&g_me.attrs[bot][0], 0, (size_t) WINTEXT_MAX_COL);
#if 1
	invalidate(0, bot, WINTEXT_MAX_COL, top);
#else
	wintext_paintscreen(0, WINTEXT_MAX_COL, 0, WINTEXT_MAX_ROW);
#endif
}

/*
     general routine to repaint the screen
*/

void wintext_paintscreen(
    int xmin,       /* update this rectangular section */
    int xmax,       /* of the 'screen'                 */
    int ymin,
    int ymax)
{
	int i, j, k;
	int istart, jstart, length, foreground, background;
	unsigned char wintext_oldbk;
	unsigned char wintext_oldfg;
	HDC hDC;

	ODS("wintext_paintscreen");

	if (g_wintext.textmode != 2)  /* not in the right mode */
		return;

	/* first time through?  Initialize the 'screen' */
	if (g_me.buffer_init == 0)
	{
		g_me.buffer_init = 1;
		wintext_oldbk = 0x00;
		wintext_oldfg = 0x0f;
		k = (wintext_oldbk << 4) + wintext_oldfg;
		g_me.buffer_init = 1;
		for (i = 0; i < g_me.char_xchars; i++)
		{
			for (j = 0; j < g_me.char_ychars; j++)
			{
				g_me.chars[j][i] = ' ';
				g_me.attrs[j][i] = k;
            }
        }
    }

	if (xmin < 0) xmin = 0;
	if (xmax >= g_me.char_xchars) xmax = g_me.char_xchars-1;
	if (ymin < 0) ymin = 0;
	if (ymax >= g_me.char_ychars) ymax = g_me.char_ychars-1;

	hDC=GetDC(wintext_hWndCopy);
	SelectObject(hDC, g_me.hFont);
	SetBkMode(hDC, OPAQUE);
	SetTextAlign(hDC, TA_LEFT | TA_TOP);

	if (TRUE == s_showing_cursor)
	//if (g_me.cursor_owned != 0)
	{
		ODS1("======================== Hide Caret %d", --carrot_count);
		HideCaret( wintext_hWndCopy );
	}

	/*
	the following convoluted code minimizes the number of
	discrete calls to the Windows interface by locating
	'strings' of screen locations with common foreground
	and background colors
	*/
	for (j = ymin; j <= ymax; j++)
	{
		length = 0;
		wintext_oldbk = 99;
		wintext_oldfg = 99;
		for (i = xmin; i <= xmax+1; i++)
		{
			k = -1;
			if (i <= xmax)
			{
				k = g_me.attrs[j][i];
			}
			foreground = (k & 15);
			background = (k >> 4);
			if (i > xmax || foreground != (int)wintext_oldfg || background != (int)wintext_oldbk)
			{
				if (length > 0)
				{
					SetBkColor(hDC, wintext_color[wintext_oldbk]);
					SetTextColor(hDC, wintext_color[wintext_oldfg]);
					TextOut(hDC,
						istart*g_me.char_width,
						jstart*g_me.char_height,
						&g_me.chars[jstart][istart],
						length);
                }
				wintext_oldbk = background;
				wintext_oldfg = foreground;
				istart = i;
				jstart = j;
				length = 0;
			}
			length++;
		}
    }

	if (TRUE == s_showing_cursor)
	//if (g_me.cursor_owned != 0)
	{
		ODS1("======================== Show Caret %d", ++carrot_count);
		ShowCaret( wintext_hWndCopy );
	}

	ReleaseDC(wintext_hWndCopy, hDC);
}

void wintext_cursor(int xpos, int ypos, int cursor_type)
{
	int x, y;
	ODS("wintext_cursor");

	if (g_wintext.textmode != 2)  /* not in the right mode */
	{
		return;
	}

    g_me.cursor_x = xpos;
    g_me.cursor_y = ypos;
    if (cursor_type >= 0) g_me.cursor_type = cursor_type;
    if (g_me.cursor_type < 0) g_me.cursor_type = 0;
    if (g_me.cursor_type > 2) g_me.cursor_type = 2;
	if (FALSE == s_showing_cursor)
	{
		x = g_me.cursor_x*g_me.char_width;
		y = g_me.cursor_y*g_me.char_height;
        CreateCaret(wintext_hWndCopy, g_me.bitmap[g_me.cursor_type],
			g_me.char_width, g_me.char_height);
        SetCaretPos(x, y);
		ODS3("======================== Show Caret %d #2 (%d,%d)", ++carrot_count, x, y);
        ShowCaret(wintext_hWndCopy);
		s_showing_cursor = TRUE;
	}
	else
    //if (g_me.cursor_owned != 0)
    {
        /* CreateCaret(wintext_hWndCopy, g_me.bitmap[g_me.cursor_type],
			g_me.char_width, g_me.char_height); */
		x = g_me.cursor_x*g_me.char_width;
		y = g_me.cursor_y*g_me.char_height;
        SetCaretPos(x, y);
        /*SetCaretBlinkTime(500);*/
		ODS2("======================== Set Caret Pos #1 (%d,%d)", x, y);
        /*ShowCaret(wintext_hWndCopy);*/
	}
}

void wintext_set_attr(int row, int col, int attr, int count)
{
	int i;
	int xmin, xmax, ymin, ymax;
	xmin = xmax = col;
	ymin = ymax = row;
	for (i = 0; i < count; i++)
	{
		g_me.attrs[row][col+i] = (unsigned char) (attr & 0xFF);
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
#if 1
	invalidate(xmin, ymin, xmax, ymax);
#else
	wintext_paintscreen(xmin, xmax, ymin, ymax);
#endif
}

void wintext_clear(void)
{
	int y;
	for (y = 0; y < WINTEXT_MAX_ROW; y++)
	{
		memset(&g_me.chars[y][0], ' ', (size_t) WINTEXT_MAX_COL);
		memset(&g_me.attrs[y][0], 0xf0, (size_t) WINTEXT_MAX_COL);
	}
    InvalidateRect(wintext_hWndCopy, NULL, FALSE);
}

BYTE *wintext_screen_get(void)
{
	size_t count = sizeof(BYTE)*WINTEXT_MAX_ROW*WINTEXT_MAX_COL;
	BYTE *copy = (BYTE *) malloc(count*2);
	_ASSERTE(copy);
	memcpy(copy, g_me.chars, count);
	memcpy(copy + count, g_me.attrs, count);
	return copy;
}

void wintext_screen_set(const BYTE *copy)
{
	size_t count = sizeof(BYTE)*WINTEXT_MAX_ROW*WINTEXT_MAX_COL;
	memcpy(g_me.chars, copy, count);
	memcpy(g_me.attrs, copy + count, count);
	InvalidateRect(wintext_hWndCopy, NULL, FALSE);
}

void wintext_hide_cursor(void)
{
	if (TRUE == s_showing_cursor)
	{
		s_showing_cursor = FALSE;
		HideCaret(wintext_hWndCopy);
	}
}

VOID CALLBACK wintext_timer_redraw(HWND window, UINT msg, UINT_PTR idEvent, DWORD dwTime)
{
	InvalidateRect(window, NULL, FALSE);
}
