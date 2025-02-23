// SPDX-License-Identifier: GPL-3.0-only
//
#include "WinText.h"

#include "Frame.h"
#include "ods.h"

#include <windowsx.h>

#include <algorithm>
#include <array> // std::size
#include <cstring>
#include <stdexcept>
#include <string>

enum
{
    TIMER_ID = 1
};

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

static LRESULT CALLBACK wintext_proc(HWND window, UINT message, WPARAM wp, LPARAM lp);

static const char *const WINDOW_CLASS{"IdText"};
static WinText *s_me{};

static COLORREF s_wintext_color[]
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

void WinText::invalidate(int left, int bot, int right, int top)
{
    RECT exposed =
    {
        left*m_char_width, top*m_char_height,
        (right+1)*m_char_width, (bot+1)*m_char_height
    };
    InvalidateRect(m_window, &exposed, FALSE);
}

/*
     Register the text window - a one-time function which performs
     all the necessary registration and initialization
*/

bool WinText::initialize(HINSTANCE instance, HWND parent, LPCSTR title)
{
    ODS("WinText::initialize");

    TEXTMETRIC text_metric;
    WNDCLASS  wc;

    m_instance = instance;
    std::strcpy(m_title, title);
    m_parent = parent;

    bool return_value = GetClassInfoA(instance, WINDOW_CLASS, &wc) != 0;
    if (!return_value)
    {
        wc.style = CS_DBLCLKS;
        wc.lpfnWndProc = wintext_proc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = instance;
        wc.hIcon = nullptr;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
        wc.lpszMenuName =  m_title;
        wc.lpszClassName = WINDOW_CLASS;

        return_value = RegisterClassA(&wc) != 0;
    }

    // set up the font characteristics
    m_char_font = OEM_FIXED_FONT;
    m_font = static_cast<HFONT>(GetStockObject(m_char_font));
    HDC dc = GetDC(parent);
    HFONT old_font = static_cast<HFONT>(SelectObject(dc, m_font));
    GetTextMetricsA(dc, &text_metric);
    SelectObject(dc, old_font);
    ReleaseDC(parent, dc);
    m_char_width  = text_metric.tmMaxCharWidth;
    m_char_height = text_metric.tmHeight;
    m_char_x_chars = WINTEXT_MAX_COL;
    m_char_y_chars = WINTEXT_MAX_ROW;

    // maximum screen width
    m_max_width = m_char_x_chars*m_char_width;
    // maximum screen height
    m_max_height = m_char_y_chars*m_char_height;

    // set up the font and caret information
    for (int i = 0; i < 3; i++)  // NOLINT(modernize-loop-convert)
    {
        const size_t count{std::size(m_cursor_pattern[0]) * sizeof(m_cursor_pattern[0][0])};
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
}

/*
        clean-up routine
*/
void WinText::destroy()
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

    for (HBITMAP &bm : m_bitmap)
    {
        DeleteObject(bm);
    }
    m_text_mode = 0;
    m_alt_f4_hit = false;
}

/*
     Set up the text window and clear it
*/
int WinText::text_on()
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
    m_window = CreateWindowA(WINDOW_CLASS,                  //
        m_title,                                              //
        m_parent == nullptr ? WS_OVERLAPPEDWINDOW : WS_CHILD, //
        CW_USEDEFAULT,                                        //
        CW_USEDEFAULT,                                        //
        m_max_width,                                          //
        m_max_height,                                         //
        m_parent,                                             //
        nullptr,                                              //
        m_instance,                                           //
        nullptr);
    _ASSERTE(m_window);

    m_text_mode = 2;
    m_alt_f4_hit = false;

    ShowWindow(m_window, SW_SHOWNORMAL);
    UpdateWindow(m_window);
    InvalidateRect(m_window, nullptr, FALSE);

    return 0;
}

/*
     Remove the text window
*/

int WinText::text_off()
{
    ODS("WinText::text_off");
    m_alt_f4_hit = false;
    if (m_text_mode != 2)  // not in the right mode
    {
        return 0;
    }
    DestroyWindow(m_window);
    m_text_mode = 1;
    return 0;
}

void WinText::on_close(HWND window)
{
    ODS("wintext_OnClose");
    m_text_mode = 1;
    m_alt_f4_hit = true;
}

void WinText::on_set_focus(HWND window, HWND old_focus)
{
    ODS("wintext_OnSetFocus");
    // get focus - display caret
    // create caret & display
    if (TRUE == m_showing_cursor)
    {
        m_cursor_owned = true;
        CreateCaret(m_window, m_bitmap[m_cursor_type], m_char_width, m_char_height);
        SetCaretPos(m_cursor_x*m_char_width, m_cursor_y*m_char_height);
        //SetCaretBlinkTime(500);
        ODS3("======================== Show Caret %d #3 (%d,%d)", ++carrot_count, g_me->cursor_x*g_me->char_width, g_me->cursor_y*g_me->char_height);
        ShowCaret(m_window);
    }
}

void WinText::on_kill_focus(HWND window, HWND old_focus)
{
    // kill focus - hide caret
    ODS("wintext_OnKillFocus");
    if (TRUE == m_showing_cursor)
    {
        m_cursor_owned = false;
        ODS1("======================== Hide Caret %d", --carrot_count);
        HideCaret(window);
        DestroyCaret();
    }
}

void WinText::on_paint(HWND window)
{
    PAINTSTRUCT ps;
    BeginPaint(window, &ps);

    // the routine below handles *all* window updates
    int x_min = ps.rcPaint.left/m_char_width;
    int x_max = (ps.rcPaint.right + m_char_width - 1)/m_char_width;
    int y_min = ps.rcPaint.top/m_char_height;
    int y_max = (ps.rcPaint.bottom + m_char_height - 1)/m_char_height;

    ODS("wintext_OnPaint");

    paint_screen(x_min, x_max, y_min, y_max);
    EndPaint(window, &ps);
}

void WinText::on_size(HWND window, UINT state, int cx, int cy)
{
    ODS("wintext_OnSize");
    if (cx > (WORD)s_me->m_max_width ||
            cy > (WORD)s_me->m_max_height)
    {
        SetWindowPos(window,
                     GetNextWindow(window, GW_HWNDPREV),
                     0, 0, s_me->m_max_width, s_me->m_max_height, SWP_NOMOVE);
    }
}

void WinText::on_get_min_max_info(HWND hwnd, LPMINMAXINFO min_max_info)
{
    ODS("wintext_OnGetMinMaxInfo");
    min_max_info->ptMaxSize.x = s_me->m_max_width;
    min_max_info->ptMaxSize.y = s_me->m_max_height;
}

static void wintext_on_close(HWND window)
{
    s_me->on_close(window);
}

static void wintext_on_set_focus(HWND window, HWND old_focus)
{
    s_me->on_set_focus(window, old_focus);
}

static void wintext_on_kill_focus(HWND window, HWND old_focus)
{
    s_me->on_kill_focus(window, old_focus);
}

static void wintext_on_paint(HWND window)
{
    s_me->on_paint(window);
}

static void wintext_on_size(HWND window, UINT state, int cx, int cy)
{
    s_me->on_size(window, state, cx, cy);
}

static void wintext_on_get_min_max_info(HWND hwnd, LPMINMAXINFO min_max_info)
{
    s_me->on_get_min_max_info(hwnd, min_max_info);
}

static void wintext_on_primary_button_up(HWND window, int x, int y, UINT key_flags)
{
    g_frame.on_primary_button_up(window, x, y, key_flags);
}

static void wintext_on_secondary_button_up(HWND window, int x, int y, UINT key_flags)
{
    g_frame.on_secondary_button_up(window, x, y, key_flags);
}

static void wintext_on_middle_button_up(HWND window, int x, int y, UINT key_flags)
{
    g_frame.on_middle_button_up(window, x, y, key_flags);
}

LRESULT CALLBACK wintext_proc(HWND window, UINT message, WPARAM wp, LPARAM lp)
{
    switch (message)
    {
    case WM_GETMINMAXINFO:
        HANDLE_WM_GETMINMAXINFO(window, wp, lp, wintext_on_get_min_max_info);
        break;
    case WM_CLOSE:
        HANDLE_WM_CLOSE(window, wp, lp, wintext_on_close);
        break;
    case WM_SIZE:
        HANDLE_WM_SIZE(window, wp, lp, wintext_on_size);
        break;
    case WM_SETFOCUS:
        HANDLE_WM_SETFOCUS(window, wp, lp, wintext_on_set_focus);
        break;
    case WM_KILLFOCUS:
        HANDLE_WM_KILLFOCUS(window, wp, lp, wintext_on_kill_focus);
        break;
    case WM_PAINT:
        HANDLE_WM_PAINT(window, wp, lp, wintext_on_paint);
        break;
    case WM_LBUTTONUP:
        HANDLE_WM_LBUTTONUP(window, wp, lp, wintext_on_primary_button_up);
        break;
    case WM_RBUTTONUP:
        HANDLE_WM_RBUTTONUP(window, wp, lp, wintext_on_secondary_button_up);
        break;
    case WM_MBUTTONUP:
        HANDLE_WM_MBUTTONUP(window, wp, lp, wintext_on_middle_button_up);
        break;
    default:
        return DefWindowProc(window, message, wp, lp);
    }
    return 0;
}

/*
        general routine to send a string to the screen
*/

void WinText::put_string(int x_pos, int y_pos, int attrib, const char *string, int *end_row, int *end_col)
{
    ODS("WinText::put_string");

    char xc;

    char xa = (attrib & 0x0ff);
    int max_row = y_pos;
    int row = max_row;
    int max_col = x_pos - 1;
    int col = max_col;

    int i;
    for (i = 0; (xc = string[i]) != 0; i++)
    {
        if (xc == 13 || xc == 10)
        {
            if (row < WINTEXT_MAX_ROW-1)
            {
                row++;
            }
            col = x_pos-1;
        }
        else
        {
            if ((++col) >= WINTEXT_MAX_COL)
            {
                if (row < WINTEXT_MAX_ROW-1)
                {
                    row++;
                }
                col = x_pos;
            }
            max_row = std::max(max_row, row);
            max_col = std::max(max_col, col);
            m_screen.chars(row, col) = xc;
            m_screen.attrs(row, col) = xa;
        }
    }
    if (i > 0)
    {
        invalidate(x_pos, y_pos, max_col, max_row);
        *end_row = row;
        *end_col = col+1;
    }
}

void WinText::scroll_up(int top, int bot)
{
    for (int row = top; row < bot; row++)
    {
        char *chars = &m_screen.chars(row, 0);
        Byte *attrs = &m_screen.attrs(row, 0);
        char *next_chars = &m_screen.chars(row+1, 0);
        Byte *next_attrs = &m_screen.attrs(row+1, 0);
        for (int col = 0; col < WINTEXT_MAX_COL; col++)
        {
            *chars++ = *next_chars++;
            *attrs++ = *next_attrs++;
        }
    }
    std::memset(&m_screen.chars(bot, 0), 0, WINTEXT_MAX_COL);
    std::memset(&m_screen.attrs(bot, 0), 0, WINTEXT_MAX_COL);
    invalidate(0, bot, WINTEXT_MAX_COL, top);
}

/*
     general routine to repaint the screen
*/

void WinText::paint_screen(int x_min, int x_max, // update this rectangular section
    int y_min, int y_max)                        // of the 'screen'
{
    int i_start;
    int j_start;
    unsigned char old_bk;
    unsigned char old_fg;

    ODS("WinText::paint_screen");

    if (m_text_mode != 2)  // not in the right mode
    {
        return;
    }

    // first time through?  Initialize the 'screen'
    if (!m_buffer_init)
    {
        m_buffer_init = true;
        old_bk = 0x00;
        old_fg = 0x0f;
        int k = (old_bk << 4) + old_fg;
        m_buffer_init = true;
        for (int col = 0; col < m_char_x_chars; col++)
        {
            for (int row = 0; row < m_char_y_chars; row++)
            {
                m_screen.chars(row, col) = ' ';
                m_screen.attrs(row, col) = static_cast<Byte>(k);
            }
        }
    }

    x_min = std::max(x_min, 0);
    x_max = std::min(x_max, m_char_x_chars - 1);
    y_min = std::max(y_min, 0);
    y_max = std::min(y_max, m_char_y_chars - 1);

    HDC dc = GetDC(m_window);
    SelectObject(dc, m_font);
    SetBkMode(dc, OPAQUE);
    SetTextAlign(dc, TA_LEFT | TA_TOP);

    if (TRUE == m_showing_cursor)
    {
        ODS1("======================== Hide Caret %d", --carrot_count);
        HideCaret(m_window);
    }

    /*
    the following convoluted code minimizes the number of
    discrete calls to the Windows interface by locating
    'strings' of screen locations with common foreground
    and background colors
    */
    for (int j = y_min; j <= y_max; j++)
    {
        int length = 0;
        old_bk = 99;
        old_fg = 99;
        for (int i = x_min; i <= x_max+1; i++)
        {
            int k = -1;
            if (i <= x_max)
            {
                k = m_screen.attrs(j, i);
            }
            int foreground = (k & 15);
            int background = (k >> 4);
            if (i > x_max || foreground != (int)old_fg || background != (int)old_bk)
            {
                if (length > 0)
                {
                    SetBkColor(dc, s_wintext_color[old_bk]);
                    SetTextColor(dc, s_wintext_color[old_fg]);
                    TextOutA(dc,
                            i_start*m_char_width,
                            j_start*m_char_height,
                            &m_screen.chars(j_start, i_start),
                            length);
                }
                old_bk = background;
                old_fg = foreground;
                i_start = i;
                j_start = j;
                length = 0;
            }
            length++;
        }
    }

    if (TRUE == m_showing_cursor)
    {
        ODS1("======================== Show Caret %d", ++carrot_count);
        ShowCaret(m_window);
    }

    ReleaseDC(m_window, dc);
}

void WinText::cursor(int x_pos, int y_pos, int cursor_type)
{
    ODS("WinText::cursor");
    int x;
    int y;

    if (m_text_mode != 2)  // not in the right mode
    {
        return;
    }

    m_cursor_x = x_pos;
    m_cursor_y = y_pos;
    if (cursor_type >= 0)
    {
        m_cursor_type = cursor_type;
    }
    m_cursor_type = std::max(m_cursor_type, 0);
    m_cursor_type = std::min(m_cursor_type, 2);
    if (m_showing_cursor == FALSE)
    {
        x = m_cursor_x*m_char_width;
        y = m_cursor_y*m_char_height;
        CreateCaret(m_window, m_bitmap[m_cursor_type],
                    m_char_width, m_char_height);
        SetCaretPos(x, y);
        ODS3("======================== Show Caret %d #2 (%d,%d)", ++carrot_count, x, y);
        ShowCaret(m_window);
        m_showing_cursor = TRUE;
    }
    else
    {
        x = m_cursor_x*m_char_width;
        y = m_cursor_y*m_char_height;
        SetCaretPos(x, y);
        ODS2("======================== Set Caret Pos #1 (%d,%d)", x, y);
    }
}

void WinText::set_attr(int row, int col, int attr, int count)
{
    int x_max = col;
    int x_min = x_max;
    int y_max = row;
    int y_min = y_max;
    for (int i = 0; i < count; i++)
    {
        m_screen.attrs(row, col+i) = static_cast<Byte>(attr & 0xFF);
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
    std::fill(m_screen.m_chars.begin(), m_screen.m_chars.end(), ' ');
    std::fill(m_screen.m_attrs.begin(), m_screen.m_attrs.end(), 0xf0);
    InvalidateRect(m_window, nullptr, FALSE);
}

Screen WinText::get_screen() const
{
    return m_screen;
}

void WinText::set_screen(const Screen &screen)
{
    m_screen = screen;
    InvalidateRect(m_window, nullptr, FALSE);
}

void WinText::hide_cursor()
{
    if (TRUE == m_showing_cursor)
    {
        m_showing_cursor = FALSE;
        HideCaret(m_window);
    }
}

static VOID CALLBACK wintext_timer_redraw(HWND window, UINT msg, UINT_PTR /*idEvent*/, DWORD /*dwTime*/)
{
    InvalidateRect(window, nullptr, FALSE);
    KillTimer(window, TIMER_ID);
}

void WinText::schedule_alarm(int secs)
{
    UINT_PTR result = SetTimer(m_window, TIMER_ID, secs, wintext_timer_redraw);
    if (!result)
    {
        DWORD error = GetLastError();
        throw std::runtime_error("SetTimer failed: " + std::to_string(error));
    }
}

int WinText::get_char_attr(int row, int col)
{
    return (m_screen.chars(row, col) & 0xFF) << 8 | (m_screen.attrs(row, col) & 0xFF);
}

void WinText::put_char_attr(int row, int col, int char_attr)
{
    m_screen.chars(row, col) = static_cast<char>((char_attr >> 8) & 0xFF);
    m_screen.attrs(row, col) = static_cast<Byte>(char_attr & 0xFF);
}

void WinText::resume()
{
    s_me = this;
}
