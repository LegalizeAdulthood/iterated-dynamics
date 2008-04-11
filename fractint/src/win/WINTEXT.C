
#define STRICT

#include <windows.h>
#include <string.h>
#include <stdlib.h>

/*
        WINTEXT.C handles the character-based "prompt screens",
        using a 24x80 character-window driver that I wrote originally
        for the Windows port of the DOS-based "Screen Generator"
        commercial package - Bert Tyler

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

void wintext_putstring(int xpos, int ypos, int attrib, char *string);
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
       unsigned char wintext_chars[25][80]  holds the text
       unsigned char wintext_attrs[25][80]  holds the (CGA-style) attributes

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

        The 'wintext_textmode' flag tracks the current textmode status.
        Note that pressing Alt-F4 closes and destroys the window *and*
        resets this flag (to 1), so the main program should look at
        this flag whenever it is possible that Alt-F4 has been hit!
        ('wintext_getkeypress()' returns a 27 (ESCAPE) if this happens)
        (Note that you can use an 'WM_CLOSE' case to handle this situation.)
        The 'wintext_textmode' values are:
                0 = the initialization routine has never been called!
                1 = text mode is *not* active
                2 = text mode *is* active
        There is also a 'wintext_AltF4hit' flag that is non-zero if
        the window has been closed (by an Alt-F4, or a WM_CLOSE sequence)
        but the application program hasn't officially closed the window yet.
*/

int wintext_textmode = 0;
int wintext_AltF4hit = 0;

/* function prototypes */

BOOL wintext_initialize(HANDLE, HWND, LPSTR);
void wintext_destroy(void);
long FAR PASCAL wintext_proc(HWND, UINT, WPARAM, LPARAM);
int wintext_texton(void);
int wintext_textoff(void);
void wintext_putstring(int, int, int, char *);
void wintext_paintscreen(int, int, int, int);
void wintext_cursor(int, int, int);
int wintext_look_for_activity(int);
void wintext_addkeypress(unsigned int);
unsigned int wintext_getkeypress(int);

/* Local copy of the "screen" characters and attributes */

unsigned char wintext_chars[25][80];
unsigned char wintext_attrs[25][80];
int wintext_buffer_init;     /* zero if 'screen' is uninitialized */

/* font information */

HFONT wintext_hFont;
int wintext_char_font;
int wintext_char_width;
int wintext_char_height;
int wintext_char_xchars;
int wintext_char_ychars;
int wintext_max_width;
int wintext_max_height;

/* "cursor" variables (AKA the "caret" in Window-Speak) */
int wintext_cursor_x;
int wintext_cursor_y;
int wintext_cursor_type;
int wintext_cursor_owned;
HBITMAP wintext_bitmap[3];
short wintext_cursor_pattern[3][40];

LPSTR wintext_title_text;         /* title-bar text */

/* a few Windows variables we need to remember globally */

HWND wintext_hWndCopy;                /* a Global copy of hWnd */
HWND wintext_hWndParent;              /* a Global copy of hWnd's Parent */
HANDLE wintext_hInstance;             /* a global copy of hInstance */

/* the keypress buffer */

#define BUFMAX 80
unsigned int  wintext_keypress_count;
unsigned int  wintext_keypress_head;
unsigned int  wintext_keypress_tail;
unsigned char wintext_keypress_initstate;
unsigned int  wintext_keypress_buffer[BUFMAX];
unsigned char wintext_keypress_state[BUFMAX];

/* EGA/VGA 16-color palette (which doesn't match Windows palette exactly) */
/*
COLORREF wintext_color[] = {
    RGB(0,0,0),
    RGB(0,0,168),
    RGB(0,168,0),
    RGB(0,168,168),
    RGB(168,0,0),
    RGB(168,0,168),
    RGB(168,84,0),
    RGB(168,168,168),
    RGB(84,84,84),
    RGB(84,84,255),
    RGB(84,255,84),
    RGB(84,255,255),
    RGB(255,84,84),
    RGB(255,84,255),
    RGB(255,255,84),
    RGB(255,255,255)
    };
*/
/* 16-color Windows Palette */

COLORREF wintext_color[] = {
    RGB(0,0,0),
    RGB(0,0,128),
    RGB(0,128,0),
    RGB(0,128,128),
    RGB(128,0,0),
    RGB(128,0,128),
    RGB(128,128,0),
    RGB(192,192,192),
/*  RGB(128,128,128),  This looks lousy - make it black */  RGB(0,0,0),
    RGB(0,0,255),
    RGB(0,255,0),
    RGB(0,255,255),
    RGB(255,0,0),
    RGB(255,0,255),
    RGB(255,255,0),
    RGB(255,255,255)
    };

/*
     Register the text window - a one-time function which perfomrs
     all of the neccessary registration and initialization
*/

BOOL wintext_initialize(HANDLE hInstance, HWND hWndParent, LPSTR titletext)
{
    WNDCLASS  wc;
    BOOL return_value;
    HDC hDC;
    HFONT hOldFont;
    TEXTMETRIC TextMetric;
    int i, j;

    wintext_hInstance = hInstance;
    wintext_title_text = titletext;
    wintext_hWndParent = hWndParent;

    wc.style = NULL;
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
    wintext_char_font = OEM_FIXED_FONT;
    wintext_hFont = GetStockObject(wintext_char_font);
    hDC=GetDC(hWndParent);
    hOldFont = SelectObject(hDC, wintext_hFont);
    GetTextMetrics(hDC, &TextMetric);
    SelectObject(hDC, hOldFont);
    ReleaseDC(hWndParent, hDC);
    wintext_char_width  = TextMetric.tmMaxCharWidth;
    wintext_char_height = TextMetric.tmHeight;
    wintext_char_xchars = 80;
    wintext_char_ychars = 25;

    wintext_max_width =                           /* maximum screen width */
        wintext_char_xchars*wintext_char_width+
            (GetSystemMetrics(SM_CXFRAME) * 2);
    wintext_max_height =                           /* maximum screen height */
        wintext_char_ychars*wintext_char_height+
            (GetSystemMetrics(SM_CYFRAME) * 2)
            -1
             + GetSystemMetrics(SM_CYCAPTION);

    /* set up the font and caret information */
    for (i = 0; i < 3; i++)
        for (j = 0; j < wintext_char_height; j++)
            wintext_cursor_pattern[i][j] = 0;
    for (j = wintext_char_height-2; j < wintext_char_height; j++)
        wintext_cursor_pattern[1][j] = 0x00ff;
    for (j = 0; j < wintext_char_height; j++)
        wintext_cursor_pattern[2][j] = 0x00ff;
    wintext_bitmap[0] = CreateBitmap(8, wintext_char_height, 1, 1,
        (LPSTR)wintext_cursor_pattern[0]);
    wintext_bitmap[1] = CreateBitmap(8, wintext_char_height, 1, 1,
        (LPSTR)wintext_cursor_pattern[1]);
    wintext_bitmap[2] = CreateBitmap(8, wintext_char_height, 1, 1,
        (LPSTR)wintext_cursor_pattern[2]);

    wintext_textmode = 1;
    wintext_AltF4hit = 0;

    return(return_value);
}

/*
        clean-up routine
*/
void wintext_destroy(void)
{
int i;

    if (wintext_textmode == 2)  /* text is still active! */
        wintext_textoff();
    if (wintext_textmode != 1)  /* not in the right mode */
        return;

/*
    DeleteObject((HANDLE)wintext_hFont);
*/
    for (i = 0; i < 3; i++)
        DeleteObject((HANDLE)wintext_bitmap[i]);
    wintext_textmode = 0;
    wintext_AltF4hit = 0;
}


/*
     Set up the text window and clear it
*/

int wintext_texton(void)
{
    HWND hWnd;

    if (wintext_textmode != 1)  /* not in the right mode */
        return(0);

    /* initialize the cursor */
    wintext_cursor_x    = 0;
    wintext_cursor_y    = 0;
    wintext_cursor_type = 0;
    wintext_cursor_owned = 0;

    /* clear the keyboard buffer */
    wintext_keypress_count = 0;
    wintext_keypress_head  = 0;
    wintext_keypress_tail  = 0;
    wintext_keypress_initstate = 0;
    wintext_buffer_init = 0;

    hWnd = CreateWindow(
        "FractintForWindowsV0011",
        wintext_title_text,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,               /* default horizontal position */
        CW_USEDEFAULT,               /* default vertical position */
        wintext_max_width,
        wintext_max_height,
        wintext_hWndParent,
        NULL,
        wintext_hInstance,
        NULL);

    /* squirrel away a global copy of 'hWnd' for later */
    wintext_hWndCopy = hWnd;

    wintext_textmode = 2;
    wintext_AltF4hit = 0;

    ShowWindow(wintext_hWndCopy, SW_SHOWNORMAL);
    UpdateWindow(wintext_hWndCopy);

    InvalidateRect(wintext_hWndCopy,NULL,FALSE);

    return(0);
}

/*
     Remove the text window
*/

int wintext_textoff(void)
{

    wintext_AltF4hit = 0;
    if (wintext_textmode != 2)  /* not in the right mode */
        return(0);
    DestroyWindow(wintext_hWndCopy);
    wintext_textmode = 1;
    return(0);
}

/*
        Window-handling procedure
*/


long FAR PASCAL wintext_proc(hWnd, message, wParam, lParam)
HWND hWnd;
UINT message;
WPARAM wParam;
LPARAM lParam;
{
    RECT tempRect;
    PAINTSTRUCT ps;
    HDC hDC;

    if (hWnd != wintext_hWndCopy)  /* ??? not the text-mode window! */
         return (DefWindowProc(hWnd, message, wParam, lParam));

    switch (message) {

        case WM_INITMENU:
           /* first time through*/
           /* someday we might want to do something special here */
           return (TRUE);

     case WM_COMMAND:
         /* if we added menu items, they would go here */
         return (DefWindowProc(hWnd, message, wParam, lParam));

     case WM_CLOSE:
         wintext_textmode = 1;
         wintext_AltF4hit = 1;
         return (DefWindowProc(hWnd, message, wParam, lParam));

        case WM_SIZE:
            /* code to prevent the window from exceeding a "full text page" */
            if (LOWORD(lParam) > (WORD)wintext_max_width ||
                HIWORD(lParam) > (WORD)wintext_max_height)
                SetWindowPos(wintext_hWndCopy,
                   GetNextWindow(wintext_hWndCopy, GW_HWNDPREV),
                   0, 0, wintext_max_width, wintext_max_height, SWP_NOMOVE);
            break;

     case WM_SETFOCUS : /* get focus - display caret */
          /* create caret & display */
          wintext_cursor_owned = 1;
          CreateCaret( wintext_hWndCopy, wintext_bitmap[wintext_cursor_type],
              wintext_char_width, wintext_char_height);
          SetCaretPos( wintext_cursor_x*wintext_char_width,
              wintext_cursor_y*wintext_char_height);
                SetCaretBlinkTime(500);
                ShowCaret( wintext_hWndCopy );
          break;

     case WM_KILLFOCUS : /* kill focus - hide caret */
          wintext_cursor_owned = 0;
          DestroyCaret();
          break;

       case WM_PAINT:  /* "Paint" routine - call the worker routine */
            hDC = BeginPaint(hWnd, &ps);
            GetUpdateRect(hWnd, &tempRect, FALSE);
            ValidateRect(hWnd, &tempRect);
            /* the routine below handles *all* window updates */
            wintext_paintscreen(0, wintext_char_xchars-1, 0, wintext_char_ychars-1);
            EndPaint(hWnd, &ps);
            break;

       case WM_KEYDOWN:   /* KEYUP, KEYDOWN, and CHAR msgs go to the 'keypressed' code */
            /* a key has been pressed - maybe ASCII, maybe not */
            /* if it's an ASCII key, 'WM_CHAR' will handle it  */
            {
            unsigned int i, j, k;
            i = (MapVirtualKey(wParam,0));
            j = (MapVirtualKey(wParam,2));
            k = (i << 8) + j;
            if (wParam == 0x10 || wParam == 0x11) { /* shift or ctl key */
                j = 0;       /* send flag: special key down */
                k = 0xff00 + wParam;
                }
            if (j == 0)        /* use this call only for non-ASCII keys */
                wintext_addkeypress(k);
            }
            break;

       case WM_KEYUP:   /* KEYUP, KEYDOWN, and CHAR msgs go to the SG code */
            /* a key has been released - maybe ASCII, maybe not */
            /* Watch for Shift, Ctl keys  */
            {
            unsigned int i, j, k;
            i = (MapVirtualKey(wParam,0));
            j = (MapVirtualKey(wParam,2));
            k = (i << 8) + j;
            j = 1;
            if (wParam == 0x10 || wParam == 0x11) { /* shift or ctl key */
                j = 0;       /* send flag: special key up */
                k = 0xfe00 + wParam;
                }
            if (j == 0)        /* use this call only for non-ASCII keys */
                wintext_addkeypress(k);
            }
            break;

       case WM_CHAR:   /* KEYUP, KEYDOWN, and CHAR msgs go to the SG code */
            /* an ASCII key has been pressed */
            {
            unsigned int i, j, k;
            i = (unsigned int)((lParam & 0x00ff0000) >> 16);
            j = wParam;
            k = (i << 8) + j;
            wintext_addkeypress(k);
            }
            break;

     default:
         return (DefWindowProc(hWnd, message, wParam, lParam));
    }
    return (NULL);
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
	if (wintext_textmode != 2)  /* not in the right mode */
	{
		return;
	}

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
		else
		{                                        /* control key down */
			if ((keypress & 0x00ff) == 0)
			{           /* special character */
				int i;
				i = ((keypress & 0xff00) >> 8);
				if (i >= 59 && i <= 68)               /* F1 thru F10 */
				{
					keypress = ((i + 35) << 8);       /* convert to Ctl-Fn */
				}
				if (i == 71)
				{
					keypress = (119 << 8);
				}
				if (i == 73)
				{
					keypress = (unsigned int)(132 << 8);
				}
				if (i == 75)
				{
					keypress = (115 << 8);
				}
				if (i == 77)
				{
					keypress = (116 << 8);
				}
				if (i == 79)
				{
					keypress = (117 << 8);
				}
				if (i == 81)
				{
					keypress = (118 << 8);
				}
			}
		}
	}

wintext_keypress_buffer[wintext_keypress_head] = keypress;
wintext_keypress_state[wintext_keypress_head] = wintext_keypress_initstate;
if (++wintext_keypress_head >= BUFMAX)
    wintext_keypress_head = 0;
wintext_keypress_count++;
}

unsigned int wintext_getkeypress(int option)
{
int i;

wintext_look_for_activity(option);

if (wintext_textmode != 2)  /* not in the right mode */
    return(27);

if (wintext_keypress_count == 0)
    return(0);

i = wintext_keypress_buffer[wintext_keypress_tail];

if (option != 0) {
    if (++wintext_keypress_tail >= BUFMAX)
        wintext_keypress_tail = 0;
    wintext_keypress_count--;
    }

return(i);

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

if (wintext_textmode != 2)  /* not in the right mode */
    return(0);

for (;;) {
    if (PeekMessage(&msg, NULL, NULL, NULL, PM_NOREMOVE) == 0)
        if (wintext_waitflag == 0 || wintext_keypress_count != 0
            || wintext_textmode != 2)
            return(wintext_keypress_count == 0 ? 0 : 1);

    if (GetMessage(&msg, NULL, NULL, NULL)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        }
    }

return(wintext_keypress_count == 0 ? 0 : 1);

}

/*
        general routine to send a string to the screen
*/

void wintext_putstring(int xpos, int ypos, int attrib, char *string)
{
    int i, j, k, maxrow, maxcol;
    char xc, xa;

    xa = (attrib & 0x0ff);
    j = maxrow = ypos;
    k = maxcol = xpos-1;

    for (i = 0; (xc = string[i]) != 0; i++) {
        if (xc == 13 || xc == 10) {
            if (j < 24) j++;
            k = -1;
            }
        else {
            if ((++k) >= 80) {
                if (j < 24) j++;
                k = 0;
                }
            if (maxrow < j) maxrow = j;
            if (maxcol < k) maxcol = k;
            wintext_chars[j][k] = xc;
            wintext_attrs[j][k] = xa;
            }
        }
    if (i > 0) {
        wintext_paintscreen(xpos, maxcol, ypos, maxrow);
        }
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

if (wintext_textmode != 2)  /* not in the right mode */
    return;

/* first time through?  Initialize the 'screen' */
if (wintext_buffer_init == 0) {
    wintext_buffer_init = 1;
    wintext_oldbk = 0x00;
    wintext_oldfg = 0x0f;
    k = (wintext_oldbk << 4) + wintext_oldfg;
    wintext_buffer_init = 1;
    for (i = 0; i < wintext_char_xchars; i++) {
        for (j = 0; j < wintext_char_ychars; j++) {
            wintext_chars[j][i] = ' ';
            wintext_attrs[j][i] = k;
            }
        }
    }

if (xmin < 0) xmin = 0;
if (xmax >= wintext_char_xchars) xmax = wintext_char_xchars-1;
if (ymin < 0) ymin = 0;
if (ymax >= wintext_char_ychars) ymax = wintext_char_ychars-1;

hDC=GetDC(wintext_hWndCopy);
SelectObject(hDC, wintext_hFont);
SetBkMode(hDC, OPAQUE);
SetTextAlign(hDC, TA_LEFT | TA_TOP);

if (wintext_cursor_owned != 0)
    HideCaret( wintext_hWndCopy );

/*
   the following convoluted code minimizes the number of
   discrete calls to the Windows interface by locating
   'strings' of screen locations with common foreground
   and background colors
*/

for (j = ymin; j <= ymax; j++) {
    length = 0;
    wintext_oldbk = 99;
    wintext_oldfg = 99;
    for (i = xmin; i <= xmax+1; i++) {
        k = -1;
        if (i <= xmax)
            k = wintext_attrs[j][i];
        foreground = (k & 15);
        background = (k >> 4);
        if (i > xmax || foreground != (int)wintext_oldfg || background != (int)wintext_oldbk) {
            if (length > 0) {
                SetBkColor(hDC,wintext_color[wintext_oldbk]);
                SetTextColor(hDC,wintext_color[wintext_oldfg]);
                TextOut(hDC,
                    istart*wintext_char_width,
                    jstart*wintext_char_height,
                    &wintext_chars[jstart][istart],
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

if (wintext_cursor_owned != 0)
    ShowCaret( wintext_hWndCopy );

ReleaseDC(wintext_hWndCopy,hDC);

}

void wintext_cursor(int xpos, int ypos, int cursor_type)
{

if (wintext_textmode != 2)  /* not in the right mode */
    return;

    wintext_cursor_x = xpos;
    wintext_cursor_y = ypos;
    if (cursor_type >= 0) wintext_cursor_type = cursor_type;
    if (wintext_cursor_type < 0) wintext_cursor_type = 0;
    if (wintext_cursor_type > 2) wintext_cursor_type = 2;
    if (wintext_cursor_owned != 0)
        {
        CreateCaret( wintext_hWndCopy, wintext_bitmap[wintext_cursor_type],
              wintext_char_width, wintext_char_height);
              SetCaretPos( wintext_cursor_x*wintext_char_width,
                  wintext_cursor_y*wintext_char_height);
              SetCaretBlinkTime(500);
              ShowCaret( wintext_hWndCopy );
        }

}
