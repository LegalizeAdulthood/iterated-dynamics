// SPDX-License-Identifier: GPL-3.0-only
//

#include <array>
#include <cassert>
#include <cstdlib>
#include <cstring>

#include "port.h"
#include "id.h"

#include "frame.h"
#include "text_screen.h"

#include "drivers.h"

#include <string>

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

bool wintext_initialize(HANDLE hInstance, LPSTR title);
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

void wintext_putstring(int xpos, int ypos, int attrib, char const *string);
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
       char chars[WINTEXT_MAX_ROW][80]  holds the text
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

// function prototypes
using LRESULT = long;
#define CALLBACK
using WPARAM = int;
using LPARAM = long;
struct COLORREF
{
    int red;
    int green;
    int blue;
};
struct RECT
{
    long left;
    long top;
    long right;
    long bottom;
};
struct DC;
using HDC = DC *;
struct TEXTMETRIC;
using LONG = long;
struct TEXTMETRIC
{
    LONG        tmHeight;
    LONG        tmAscent;
    LONG        tmDescent;
    LONG        tmInternalLeading;
    LONG        tmExternalLeading;
    LONG        tmAveCharWidth;
    LONG        tmMaxCharWidth;
    LONG        tmWeight;
    LONG        tmOverhang;
    LONG        tmDigitizedAspectX;
    LONG        tmDigitizedAspectY;
    BYTE        tmFirstChar;
    BYTE        tmLastChar;
    BYTE        tmDefaultChar;
    BYTE        tmBreakChar;
    BYTE        tmItalic;
    BYTE        tmUnderlined;
    BYTE        tmStruckOut;
    BYTE        tmPitchAndFamily;
    BYTE        tmCharSet;
};
using LPTEXTMETRICA = TEXTMETRIC*;
struct ICON;
using HICON = ICON *;
struct BRUSH;
using HBRUSH = BRUSH *;
struct CURSOR;
using HCURSOR = CURSOR *;
using WNDPROC = LRESULT (*)(HWND, UINT, WPARAM, LPARAM);
struct WNDCLASS
{
    UINT        style;
    WNDPROC     lpfnWndProc;
    int         cbClsExtra;
    int         cbWndExtra;
    HINSTANCE   hInstance;
    HICON       hIcon;
    HCURSOR     hCursor;
    HBRUSH      hbrBackground;
    LPCSTR      lpszMenuName;
    LPCSTR      lpszClassName;
};
using LPWNDCLASSA = WNDCLASS*;
const char *IDC_ARROW{};
enum
{
    BLACK_BRUSH = 4,
    OEM_FIXED_FONT = 10,
    SW_SHOWNORMAL = 1,
};
#define CW_USEDEFAULT       ((int)0x80000000)
#define WS_OVERLAPPEDWINDOW ((int)0x40000000)
#define WS_CHILD            0x40000000L
using HGDIOBJ = void*;
using ATOM = int;
using BOOL = int;
using HANDLE = void *;
using LPVOID = void*;
struct MENU;
using HMENU = MENU *;
using DWORD = int;

inline COLORREF RGB(int red, int green, int blue)
{
    return COLORREF{red, green, blue};
}
#define WINUSERAPI
#define WINGDIAPI
#define WINAPI
#define CONST const
#define VOID void

#define ODS(arg_)

static LRESULT CALLBACK wintext_proc(HWND, UINT, WPARAM, LPARAM);

static const char *const s_window_class{"IdText"};
static TextScreen *s_me{};

static COLORREF wintext_color[]
{
    RGB(0, 0, 0),
    RGB(0, 0, 128),
    RGB(0, 128, 0),
    RGB(0, 128, 128),
    RGB(128, 0, 0),
    RGB(128, 0, 128),
    RGB(128, 128, 0),
    RGB(192, 192, 192),
    //  RGB(128, 128, 128),  This looks lousy - make it black
    RGB(0, 0, 0),
    RGB(0, 0, 255),
    RGB(0, 255, 0),
    RGB(0, 255, 255),
    RGB(255, 0, 0),
    RGB(255, 0, 255),
    RGB(255, 255, 0),
    RGB(255, 255, 255)
};

void TextScreen::invalidate(int left, int bot, int right, int top)
{
    RECT exposed =
    {
        left*m_char_width, top*m_char_height,
        (right+1)*m_char_width, (bot+1)*m_char_height
    };
    //InvalidateRect(m_window, &exposed, FALSE);
}

/*
     Register the text window - a one-time function which performs
     all the necessary registration and initialization
*/

bool TextScreen::initialize(HINSTANCE instance, HWND parent, LPCSTR title)
{
#if 1
    return false;
#else
    ODS("WinText::initialize");

    HDC hDC;
    HFONT hOldFont;
    TEXTMETRIC TextMetric;
    WNDCLASS  wc;

    m_instance = instance;
    std::strcpy(m_title, title);
    m_parent = parent;

    bool return_value = GetClassInfoA(instance, s_window_class, &wc) != 0;
    if (!return_value)
    {
        wc.style = 0;
        wc.lpfnWndProc = wintext_proc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = instance;
        wc.hIcon = nullptr;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
        wc.lpszMenuName =  m_title;
        wc.lpszClassName = s_window_class;

        return_value = RegisterClassA(&wc) != 0;
    }

    // set up the font characteristics
    m_char_font = OEM_FIXED_FONT;
    m_font = static_cast<HFONT>(GetStockObject(m_char_font));
    hDC = GetDC(parent);
    hOldFont = static_cast<HFONT>(SelectObject(hDC, m_font));
    GetTextMetricsA(hDC, &TextMetric);
    SelectObject(hDC, hOldFont);
    ReleaseDC(parent, hDC);
    m_char_width  = TextMetric.tmMaxCharWidth;
    m_char_height = TextMetric.tmHeight;
    m_char_xchars = WINTEXT_MAX_COL;
    m_char_ychars = WINTEXT_MAX_ROW;

    // maximum screen width
    m_max_width = m_char_xchars*m_char_width;
    // maximum screen height
    m_max_height = m_char_ychars*m_char_height;

    // set up the font and caret information
    for (int i = 0; i < 3; i++)
    {
        size_t count = std::size(m_cursor_pattern[0])*sizeof(m_cursor_pattern[0][0]);
        std::memset(&m_cursor_pattern[i][0], 0, count);
    }
    for (int j = m_char_height-2; j < m_char_height; j++)
    {
        m_cursor_pattern[1][j] = 0x00ff;
    }
    for (int j = 0; j < m_char_height; j++)
    {
        m_cursor_pattern[2][j] = 0x00ff;
    }
    m_bitmap[0] = CreateBitmap(8, m_char_height, 1, 1, &m_cursor_pattern[0][0]);
    m_bitmap[1] = CreateBitmap(8, m_char_height, 1, 1, &m_cursor_pattern[1][0]);
    m_bitmap[2] = CreateBitmap(8, m_char_height, 1, 1, &m_cursor_pattern[2][0]);

    m_text_mode = 1;
    m_alt_f4_hit = false;

    return return_value;
#endif
}

/*
        clean-up routine
*/
void TextScreen::destroy()
{
    ODS("WinText::destroy");

    if (m_text_mode == 2)  // text is still active!
    {
        text_off();
    }
    if (m_text_mode != 1)  // not in the right mode
    {
        return;
    }

    for (int i = 0; i < 3; i++)
    {
        //DeleteObject((HANDLE) m_bitmap[i]);
    }
    m_text_mode = 0;
    m_alt_f4_hit = false;
}

/*
     Set up the text window and clear it
*/
int TextScreen::text_on()
{
    ODS("WinText::text_on");

    if (m_text_mode != 1)  // not in the right mode
    {
        return 0;
    }

    // initialize the cursor
    m_cursor_x    = 0;
    m_cursor_y    = 0;
    m_cursor_type = 0;
    m_cursor_owned = false;
    m_showing_cursor = FALSE;

    /* make sure g_me points to me because CreateWindow
     * is going to call the window procedure.
     */
    s_me = this;
    //m_window = CreateWindowA(s_window_class,                  //
    //    m_title,                                              //
    //    nullptr == m_parent ? WS_OVERLAPPEDWINDOW : WS_CHILD, //
    //    CW_USEDEFAULT,                                        //
    //    CW_USEDEFAULT,                                        //
    //    m_max_width,                                          //
    //    m_max_height,                                         //
    //    m_parent,                                             //
    //    nullptr,                                              //
    //    m_instance,                                           //
    //    nullptr);
    //_ASSERTE(m_window);

    m_text_mode = 2;
    m_alt_f4_hit = false;

    //ShowWindow(m_window, SW_SHOWNORMAL);
    //UpdateWindow(m_window);
    //InvalidateRect(m_window, nullptr, FALSE);

    return 0;
}

/*
     Remove the text window
*/

int TextScreen::text_off()
{
    ODS("WinText::text_off");
    m_alt_f4_hit = false;
    if (m_text_mode != 2)  // not in the right mode
    {
        return 0;
    }
    //DestroyWindow(m_window);
    m_text_mode = 1;
    return 0;
}

void TextScreen::on_close(HWND window)
{
    ODS("wintext_OnClose");
    m_text_mode = 1;
    m_alt_f4_hit = true;
}

void TextScreen::on_set_focus(HWND window, HWND old_focus)
{
    ODS("wintext_OnSetFocus");
    // get focus - display caret
    // create caret & display
    if (TRUE == m_showing_cursor)
    {
        m_cursor_owned = true;
        //CreateCaret(m_window, m_bitmap[m_cursor_type], m_char_width, m_char_height);
        //SetCaretPos(m_cursor_x*m_char_width, m_cursor_y*m_char_height);
        ////SetCaretBlinkTime(500);
        //ODS3("======================== Show Caret %d #3 (%d,%d)", ++carrot_count, g_me->cursor_x*g_me->char_width, g_me->cursor_y*g_me->char_height);
        //ShowCaret(m_window);
    }
}

void TextScreen::on_kill_focus(HWND window, HWND old_focus)
{
    // kill focus - hide caret
    ODS("wintext_OnKillFocus");
    if (TRUE == m_showing_cursor)
    {
        m_cursor_owned = false;
        //ODS1("======================== Hide Caret %d", --carrot_count);
        //HideCaret(window);
        //DestroyCaret();
    }
}

void TextScreen::on_paint(HWND window)
{
    //PAINTSTRUCT ps;
    //HDC hDC = BeginPaint(window, &ps);
    
    // the routine below handles *all* window updates
    //int xmin = ps.rcPaint.left/m_char_width;
    //int xmax = (ps.rcPaint.right + m_char_width - 1)/m_char_width;
    //int ymin = ps.rcPaint.top/m_char_height;
    //int ymax = (ps.rcPaint.bottom + m_char_height - 1)/m_char_height;

    ODS("wintext_OnPaint");

    //paint_screen(xmin, xmax, ymin, ymax);
    //EndPaint(window, &ps);
}

using WORD = int;
void TextScreen::on_size(HWND window, UINT state, int cx, int cy)
{
    ODS("wintext_OnSize");
    if (cx > (WORD)s_me->m_max_width ||
            cy > (WORD)s_me->m_max_height)
    {
        //SetWindowPos(window,
        //             GetNextWindow(window, GW_HWNDPREV),
        //             0, 0, s_me->m_max_width, s_me->m_max_height, SWP_NOMOVE);
    }
}

void TextScreen::on_get_min_max_info(HWND hwnd, LPMINMAXINFO lpMinMaxInfo)
{
    ODS("wintext_OnGetMinMaxInfo");
    //lpMinMaxInfo->ptMaxSize.x = s_me->m_max_width;
    //lpMinMaxInfo->ptMaxSize.y = s_me->m_max_height;
}

static void wintext_OnClose(HWND window)
{
    s_me->on_close(window);
}

static void wintext_OnSetFocus(HWND window, HWND old_focus)
{
    s_me->on_set_focus(window, old_focus);
}

static void wintext_OnKillFocus(HWND window, HWND old_focus)
{
    s_me->on_kill_focus(window, old_focus);
}

static void wintext_OnPaint(HWND window)
{
    s_me->on_paint(window);
}

static void wintext_OnSize(HWND window, UINT state, int cx, int cy)
{
    s_me->on_size(window, state, cx, cy);
}

static void wintext_OnGetMinMaxInfo(HWND hwnd, LPMINMAXINFO lpMinMaxInfo)
{
    s_me->on_get_min_max_info(hwnd,lpMinMaxInfo);
}

static void wintext_OnLButtonUp(HWND window, int x, int y, UINT key_flags)
{
    //g_frame.on_left_button_up(window, x, y, key_flags);
}

static void wintext_OnRButtonUp(HWND window, int x, int y, UINT key_flags)
{
    //g_frame.on_right_button_up(window, x, y, key_flags);
}

static void wintext_OnMButtonUp(HWND window, int x, int y, UINT key_flags)
{
    //g_frame.on_middle_button_up(window, x, y, key_flags);
}

LRESULT CALLBACK wintext_proc(HWND window, UINT message, WPARAM wp, LPARAM lp)
{
    //switch (message)
    //{
    //case WM_GETMINMAXINFO:
    //    HANDLE_WM_GETMINMAXINFO(window, wp, lp, wintext_OnGetMinMaxInfo);
    //    break;
    //case WM_CLOSE:
    //    HANDLE_WM_CLOSE(window, wp, lp, wintext_OnClose);
    //    break;
    //case WM_SIZE:
    //    HANDLE_WM_SIZE(window, wp, lp, wintext_OnSize);
    //    break;
    //case WM_SETFOCUS:
    //    HANDLE_WM_SETFOCUS(window, wp, lp, wintext_OnSetFocus);
    //    break;
    //case WM_KILLFOCUS:
    //    HANDLE_WM_KILLFOCUS(window, wp, lp, wintext_OnKillFocus);
    //    break;
    //case WM_PAINT:
    //    HANDLE_WM_PAINT(window, wp, lp, wintext_OnPaint);
    //    break;
    //case WM_LBUTTONUP:
    //    HANDLE_WM_LBUTTONUP(window, wp, lp, wintext_OnLButtonUp);
    //    break;
    //case WM_RBUTTONUP:
    //    HANDLE_WM_RBUTTONUP(window, wp, lp, wintext_OnRButtonUp);
    //    break;
    //case WM_MBUTTONUP:
    //    HANDLE_WM_MBUTTONUP(window, wp, lp, wintext_OnMButtonUp);
    //    break;
    //default:
    //    return DefWindowProc(window, message, wp, lp);
    //}
    return 0;
}

/*
        general routine to send a string to the screen
*/

void TextScreen::put_string(int xpos, int ypos, int attrib, char const *string, int *end_row, int *end_col)
{
    ODS("WinText::put_string");

    int j;
    int k;
    int maxrow;
    int maxcol;
    char xc;
    char xa;

    xa = (attrib & 0x0ff);
    maxrow = ypos;
    j = maxrow;
    maxcol = xpos-1;
    k = maxcol;

    int i;
    for (i = 0; (xc = string[i]) != 0; i++)
    {
        if (xc == 13 || xc == 10)
        {
            if (j < WINTEXT_MAX_ROW-1)
                j++;
            k = xpos-1;
        }
        else
        {
            if ((++k) >= WINTEXT_MAX_COL)
            {
                if (j < WINTEXT_MAX_ROW-1)
                    j++;
                k = xpos;
            }
            if (maxrow < j)
                maxrow = j;
            if (maxcol < k)
                maxcol = k;
            m_chars[j][k] = xc;
            m_attrs[j][k] = xa;
        }
    }
    if (i > 0)
    {
        invalidate(xpos, ypos, maxcol, maxrow);
        *end_row = j;
        *end_col = k+1;
    }
}

void TextScreen::scroll_up(int top, int bot)
{
    for (int row = top; row < bot; row++)
    {
        char *chars = &m_chars[row][0];
        unsigned char *attrs = &m_attrs[row][0];
        char *next_chars = &m_chars[row+1][0];
        unsigned char *next_attrs = &m_attrs[row+1][0];
        for (int col = 0; col < WINTEXT_MAX_COL; col++)
        {
            *chars++ = *next_chars++;
            *attrs++ = *next_attrs++;
        }
    }
    std::memset(&m_chars[bot][0], 0, (size_t) WINTEXT_MAX_COL);
    std::memset(&m_attrs[bot][0], 0, (size_t) WINTEXT_MAX_COL);
    invalidate(0, bot, WINTEXT_MAX_COL, top);
}

/*
     general routine to repaint the screen
*/

void TextScreen::paint_screen(int xmin, int xmax, // update this rectangular section
    int ymin, int ymax)                        // of the 'screen'
{
    int istart;
    int jstart;
    int length;
    int foreground;
    int background;
    unsigned char oldbk;
    unsigned char oldfg;
    HDC hDC;

    ODS("WinText::paint_screen");

    if (m_text_mode != 2)  // not in the right mode
        return;

    // first time through?  Initialize the 'screen'
    if (!m_buffer_init)
    {
        m_buffer_init = true;
        oldbk = 0x00;
        oldfg = 0x0f;
        int k = (oldbk << 4) + oldfg;
        m_buffer_init = true;
        for (int i = 0; i < m_char_xchars; i++)
        {
            for (int j = 0; j < m_char_ychars; j++)
            {
                m_chars[j][i] = ' ';
                m_attrs[j][i] = k;
            }
        }
    }

    if (xmin < 0)
        xmin = 0;
    if (xmax >= m_char_xchars)
        xmax = m_char_xchars-1;
    if (ymin < 0)
        ymin = 0;
    if (ymax >= m_char_ychars)
        ymax = m_char_ychars-1;

    //hDC = GetDC(m_window);
    //SelectObject(hDC, m_font);
    //SetBkMode(hDC, OPAQUE);
    //SetTextAlign(hDC, TA_LEFT | TA_TOP);

    if (TRUE == m_showing_cursor)
    {
        //ODS1("======================== Hide Caret %d", --carrot_count);
        //HideCaret(m_window);
    }

    /*
    the following convoluted code minimizes the number of
    discrete calls to the Windows interface by locating
    'strings' of screen locations with common foreground
    and background colors
    */
    for (int j = ymin; j <= ymax; j++)
    {
        length = 0;
        oldbk = 99;
        oldfg = 99;
        for (int i = xmin; i <= xmax+1; i++)
        {
            int k = -1;
            if (i <= xmax)
            {
                k = m_attrs[j][i];
            }
            foreground = (k & 15);
            background = (k >> 4);
            if (i > xmax || foreground != (int)oldfg || background != (int)oldbk)
            {
                if (length > 0)
                {
                    //SetBkColor(hDC, wintext_color[oldbk]);
                    //SetTextColor(hDC, wintext_color[oldfg]);
                    //TextOutA(hDC,
                    //        istart*m_char_width,
                    //        jstart*m_char_height,
                    //        &m_chars[jstart][istart],
                    //        length);
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
        //ODS1("======================== Show Caret %d", ++carrot_count);
        //ShowCaret(m_window);
    }

    //ReleaseDC(m_window, hDC);
}

void TextScreen::cursor(int xpos, int ypos, int cursor_type)
{
    ODS("WinText::cursor");
    int x;
    int y;

    if (m_text_mode != 2)  // not in the right mode
    {
        return;
    }

    m_cursor_x = xpos;
    m_cursor_y = ypos;
    if (cursor_type >= 0)
        m_cursor_type = cursor_type;
    if (m_cursor_type < 0)
        m_cursor_type = 0;
    if (m_cursor_type > 2)
        m_cursor_type = 2;
    if (m_showing_cursor == FALSE)
    {
        x = m_cursor_x*m_char_width;
        y = m_cursor_y*m_char_height;
        //CreateCaret(m_window, m_bitmap[m_cursor_type],
        //            m_char_width, m_char_height);
        //SetCaretPos(x, y);
        //ODS3("======================== Show Caret %d #2 (%d,%d)", ++carrot_count, x, y);
        //ShowCaret(m_window);
        m_showing_cursor = TRUE;
    }
    else
    {
        x = m_cursor_x*m_char_width;
        y = m_cursor_y*m_char_height;
        //SetCaretPos(x, y);
        //ODS2("======================== Set Caret Pos #1 (%d,%d)", x, y);
    }
}

void TextScreen::set_attr(int row, int col, int attr, int count)
{
    int xmin;
    int xmax;
    int ymin;
    int ymax;
    xmax = col;
    xmin = xmax;
    ymax = row;
    ymin = ymax;
    for (int i = 0; i < count; i++)
    {
        m_attrs[row][col+i] = (unsigned char)(attr & 0xFF);
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
    invalidate(xmin, ymin, xmax, ymax);
}

void TextScreen::clear()
{
    for (int y = 0; y < WINTEXT_MAX_ROW; y++)
    {
        std::memset(&m_chars[y][0], ' ', (size_t) WINTEXT_MAX_COL);
        std::memset(&m_attrs[y][0], 0xf0, (size_t) WINTEXT_MAX_COL);
    }
    //InvalidateRect(m_window, nullptr, FALSE);
}

Screen TextScreen::get_screen() const
{
    constexpr size_t count{WINTEXT_MAX_ROW * WINTEXT_MAX_COL};
    Screen result;
    result.chars.resize(count);
    std::memcpy(result.chars.data(), m_chars, count);
    result.attrs.resize(count);
    std::memcpy(result.attrs.data(), m_attrs, count);
    return result;
}

void TextScreen::set_screen(const Screen &screen)
{
    constexpr size_t count{WINTEXT_MAX_ROW * WINTEXT_MAX_COL};
    std::memcpy(m_chars, screen.chars.data(), count);
    std::memcpy(m_attrs, screen.attrs.data(), count);
    //InvalidateRect(m_window, nullptr, FALSE);
}

void TextScreen::hide_cursor()
{
    if (TRUE == m_showing_cursor)
    {
        m_showing_cursor = FALSE;
        //HideCaret(m_window);
    }
}

using UINT_PTR = unsigned int *;

static VOID CALLBACK wintext_timer_redraw(HWND window, UINT msg, UINT_PTR idEvent, DWORD dwTime)
{
    //InvalidateRect(window, nullptr, FALSE);
    //KillTimer(window, TIMER_ID);
}

void TextScreen::schedule_alarm(int secs)
{
    //UINT_PTR result = SetTimer(m_window, TIMER_ID, secs, wintext_timer_redraw);
    //if (!result)
    {
        //DWORD error = GetLastError();
        //_ASSERTE(result);
    }
}

int TextScreen::get_char_attr(int row, int col)
{
    return ((m_chars[row][col] & 0xFF) << 8) | (m_attrs[row][col] & 0xFF);
}

void TextScreen::put_char_attr(int row, int col, int char_attr)
{
    m_chars[row][col] = (char_attr >> 8) & 0xFF;
    m_attrs[row][col] = (char_attr & 0xFF);
}

void TextScreen::resume()
{
    s_me = this;
}
