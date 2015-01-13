#include <algorithm>
#include <cassert>

#include "port.h"
#include "x11_text.h"

x11_text_window::x11_text_window()
        : dpy_{},
        font_{},
        parent_{},
        window_{},
        char_width_{},
        char_height_{},
        char_xchars_{},
        char_ychars_{},
        max_width_{},
        max_height_{},
        x_{},
        y_{},
        colormap_{},
        buffer_init_{}
{
#if 0
    bool return_value = GetClassInfo(hInstance, s_window_class, &wc) == TRUE;
    if (!return_value)
    {
        wc.style = 0;
        wc.lpfnWndProc = wintext_proc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = hInstance;
        wc.hIcon = nullptr;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
        wc.lpszMenuName =  me->title_text;
        wc.lpszClassName = s_window_class;

        return_value = RegisterClass(&wc) != 0;
    }

    // set up the font characteristics
    me->char_font = OEM_FIXED_FONT;
    me->hFont = static_cast<HFONT>(GetStockObject(me->char_font));
    hDC=GetDC(hWndParent);
    hOldFont = static_cast<HFONT>(SelectObject(hDC, me->hFont));
    GetTextMetrics(hDC, &TextMetric);
    SelectObject(hDC, hOldFont);
    ReleaseDC(hWndParent, hDC);
    me->char_width  = TextMetric.tmMaxCharWidth;
    me->char_height = TextMetric.tmHeight;
    me->char_xchars = WINTEXT_MAX_COL;
    me->char_ychars = WINTEXT_MAX_ROW;

    // maximum screen width
    me->max_width = me->char_xchars*me->char_width;
    // maximum screen height
    me->max_height = me->char_ychars*me->char_height;

    // set up the font and caret information
    for (int i = 0; i < 3; i++)
    {
        size_t count = NUM_OF(me->cursor_pattern[0])*sizeof(me->cursor_pattern[0][0]);
        memset(&me->cursor_pattern[i][0], 0, count);
    }
    for (int j = me->char_height-2; j < me->char_height; j++)
    {
        me->cursor_pattern[1][j] = 0x00ff;
    }
    for (int j = 0; j < me->char_height; j++)
    {
        me->cursor_pattern[2][j] = 0x00ff;
    }
    me->bitmap[0] = CreateBitmap(8, me->char_height, 1, 1, &me->cursor_pattern[0][0]);
    me->bitmap[1] = CreateBitmap(8, me->char_height, 1, 1, &me->cursor_pattern[1][0]);
    me->bitmap[2] = CreateBitmap(8, me->char_height, 1, 1, &me->cursor_pattern[2][0]);

    me->textmode = 1;
    me->AltF4hit = false;

    return return_value;
#endif
}

x11_text_window::~x11_text_window()
{
    if (font_)
    {
        XUnloadFont(dpy_, font_->fid);
        XFreeFont(nullptr, const_cast<XFontStruct*>(font_));
        font_ = nullptr;
    }
}

namespace
{
inline XColor xcolor_from_rgb(int red, int green, int blue)
{
    return XColor{ 0UL,
            static_cast<unsigned short>(red*256),
            static_cast<unsigned short>(green*256),
            static_cast<unsigned short>(blue*256),
            DoRed | DoGreen | DoBlue
    };
}

std::vector<XColor> text_colors =
{
    xcolor_from_rgb(0, 0, 0),
    xcolor_from_rgb(0, 0, 128),
    xcolor_from_rgb(0, 128, 0),
    xcolor_from_rgb(0, 128, 128),
    xcolor_from_rgb(128, 0, 0),
    xcolor_from_rgb(128, 0, 128),
    xcolor_from_rgb(128, 128, 0),
    xcolor_from_rgb(192, 192, 192),
    xcolor_from_rgb(0, 0, 0),
    xcolor_from_rgb(0, 0, 255),
    xcolor_from_rgb(0, 255, 0),
    xcolor_from_rgb(0, 255, 255),
    xcolor_from_rgb(255, 0, 0),
    xcolor_from_rgb(255, 0, 255),
    xcolor_from_rgb(255, 255, 0),
    xcolor_from_rgb(255, 255, 255)
};
}

void x11_text_window::initialize(Display *dpy, int screen_num, Window parent)
{
    dpy_ = dpy;
    screen_num_ = screen_num;
    parent_ = parent;
    font_ = XLoadQueryFont(dpy_, "6x12");
    char_width_ = font_->max_bounds.width;
    char_height_ = font_->max_bounds.ascent + font_->max_bounds.descent;
    char_xchars_ = X11_TEXT_MAX_COL;
    char_ychars_ = X11_TEXT_MAX_ROW;
    max_width_ = static_cast<unsigned>(char_xchars_*char_width_);
    max_height_ = static_cast<unsigned>(char_ychars_*char_height_);
    text_mode_ = 1;
    alt_f4_hit_ = false;
    colormap_ = DefaultColormap(dpy, screen_num);
    for (std::size_t i = 0; i < text_colors.size(); ++i)
    {
        Status const status = XAllocColor(dpy_, colormap_, &text_colors[i]);
        assert(status);
    }
}

int x11_text_window::text_on()
{
    if (text_mode_ != 1)
    {
        return 0;
    }

    cursor_x_ = 0;
    cursor_y_ = 0;
    cursor_type_ = 0;
    cursor_owned_ = false;
    showing_cursor_ = false;

    if (window_ == 0)
    {
        XSetWindowAttributes attrs;
        Screen *screen = ScreenOfDisplay(dpy_, screen_num_);
        attrs.background_pixel = BlackPixelOfScreen(screen);
        attrs.bit_gravity = StaticGravity;
        attrs.backing_store = DoesBackingStore(screen) != 0 ? Always : NotUseful;
        window_ = XCreateWindow(dpy_, parent_,
                x_, y_, max_width_, max_height_, 0, DefaultDepth(dpy_, screen_num_),
                InputOutput, CopyFromParent,
                CWBackPixel | CWBitGravity | CWBackingStore, &attrs);
    }
    assert(window_ != 0);

    text_mode_ = 2;
    alt_f4_hit_ = false;
    XMapWindow(dpy_, window_);

    return 0;
}

int x11_text_window::text_off()
{
    alt_f4_hit_ = false;
    XUnmapWindow(dpy_, window_);
    text_mode_ = 1;
    return 0;
}


void x11_text_window::clear()
{
    for (int y = 0; y < X11_TEXT_MAX_ROW; ++y)
    {
        std::fill(text_[y].begin(), text_[y].end(), ' ');
        std::fill(attributes_[y].begin(), attributes_[y].end(), 0xf0);
    }
    // TODO: repaint
}

void x11_text_window::set_position(int x, int y)
{
    x_ = x;
    y_ = y;
    XMoveWindow(dpy_, window_, x, y);
}

#if 0

// EGA/VGA 16-color palette (which doesn't match Windows palette exactly)
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
// 16-color Windows Palette

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

void invalidate(WinText *me, int left, int bot, int right, int top)
{
    RECT exposed =
    {
        left*me->char_width, top*me->char_height,
        (right+1)*me->char_width, (bot+1)*me->char_height
    };
    InvalidateRect(me->hWndCopy, &exposed, FALSE);
}

/*
     Register the text window - a one-time function which perfomrs
     all of the neccessary registration and initialization
*/

/*
        clean-up routine
*/
void wintext_destroy(WinText *me)
{
    ODS("wintext_destroy");

    if (me->textmode == 2)  // text is still active!
    {
        wintext_textoff(me);
    }
    if (me->textmode != 1)  // not in the right mode
    {
        return;
    }

    for (int i = 0; i < 3; i++)
    {
        DeleteObject((HANDLE) me->bitmap[i]);
    }
    me->textmode = 0;
    me->AltF4hit = false;
}


/*
     Set up the text window and clear it
*/
int wintext_texton(WinText *me)
{
    HWND hWnd;

    ODS("wintext_texton");

    if (me->textmode != 1)  // not in the right mode
    {
        return 0;
    }

    // initialize the cursor
    me->cursor_x    = 0;
    me->cursor_y    = 0;
    me->cursor_type = 0;
    me->cursor_owned = false;
    me->showing_cursor = FALSE;

    /* make sure g_me points to me because CreateWindow
     * is going to call the window procedure.
     */
    g_me = me;
    hWnd = CreateWindow(s_window_class,
                        me->title_text,
                        (nullptr == me->hWndParent) ? WS_OVERLAPPEDWINDOW : WS_CHILD,
                        CW_USEDEFAULT,               // default horizontal position
                        CW_USEDEFAULT,               // default vertical position
                        me->max_width,
                        me->max_height,
                        me->hWndParent,
                        nullptr,
                        me->hInstance,
                        nullptr);
    _ASSERTE(hWnd);

    // squirrel away a global copy of 'hWnd' for later
    me->hWndCopy = hWnd;

    me->textmode = 2;
    me->AltF4hit = false;

    ShowWindow(me->hWndCopy, SW_SHOWNORMAL);
    UpdateWindow(me->hWndCopy);
    InvalidateRect(me->hWndCopy, nullptr, FALSE);

    return 0;
}

/*
     Remove the text window
*/

int wintext_textoff(WinText *me)
{
    ODS("wintext_textoff");
    me->AltF4hit = false;
    if (me->textmode != 2)  // not in the right mode
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
    g_me->AltF4hit = true;
}

static void wintext_OnSetFocus(HWND window, HWND old_focus)
{
    ODS("wintext_OnSetFocus");
    // get focus - display caret
    // create caret & display
    if (TRUE == g_me->showing_cursor)
    {
        g_me->cursor_owned = true;
        CreateCaret(g_me->hWndCopy, g_me->bitmap[g_me->cursor_type], g_me->char_width, g_me->char_height);
        SetCaretPos(g_me->cursor_x*g_me->char_width, g_me->cursor_y*g_me->char_height);
        //SetCaretBlinkTime(500);
        ODS3("======================== Show Caret %d #3 (%d,%d)", ++carrot_count, g_me->cursor_x*g_me->char_width, g_me->cursor_y*g_me->char_height);
        ShowCaret(g_me->hWndCopy);
    }
}

static void wintext_OnKillFocus(HWND window, HWND old_focus)
{
    // kill focus - hide caret
    ODS("wintext_OnKillFocus");
    if (TRUE == g_me->showing_cursor)
    {
        g_me->cursor_owned = false;
        ODS1("======================== Hide Caret %d", --carrot_count);
        HideCaret(window);
        DestroyCaret();
    }
}

void wintext_set_focus()
{
    wintext_OnSetFocus(nullptr, nullptr);
}

void wintext_kill_focus()
{
    wintext_OnKillFocus(nullptr, nullptr);
}

static void wintext_OnPaint(HWND window)
{
    PAINTSTRUCT ps;
    HDC hDC = BeginPaint(window, &ps);

    // the routine below handles *all* window updates
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
    if (g_me->hWndCopy == nullptr)
    {
        g_me->hWndCopy = hWnd;
    }
    else if (hWnd != g_me->hWndCopy)  // ??? not the text-mode window!
    {
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    switch (message)
    {
    case WM_GETMINMAXINFO:
        HANDLE_WM_GETMINMAXINFO(hWnd, wParam, lParam, wintext_OnGetMinMaxInfo);
        break;
    case WM_CLOSE:
        HANDLE_WM_CLOSE(hWnd, wParam, lParam, wintext_OnClose);
        break;
    case WM_SIZE:
        HANDLE_WM_SIZE(hWnd, wParam, lParam, wintext_OnSize);
        break;
    case WM_SETFOCUS:
        HANDLE_WM_SETFOCUS(hWnd, wParam, lParam, wintext_OnSetFocus);
        break;
    case WM_KILLFOCUS:
        HANDLE_WM_KILLFOCUS(hWnd, wParam, lParam, wintext_OnKillFocus);
        break;
    case WM_PAINT:
        HANDLE_WM_PAINT(hWnd, wParam, lParam, wintext_OnPaint);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
        break;
    }
    return 0;
}

/*
        general routine to send a string to the screen
*/

void wintext_putstring(WinText *me, int xpos, int ypos, int attrib, const char *string, int *end_row, int *end_col)
{
    int j, k, maxrow, maxcol;
    char xc, xa;

    ODS("wintext_putstring");

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
            me->chars[j][k] = xc;
            me->attrs[j][k] = xa;
        }
    }
    if (i > 0)
    {
        invalidate(me, xpos, ypos, maxcol, maxrow);
        *end_row = j;
        *end_col = k+1;
    }
}

void wintext_scroll_up(WinText *me, int top, int bot)
{
    for (int row = top; row < bot; row++)
    {
        char *chars = &me->chars[row][0];
        unsigned char *attrs = &me->attrs[row][0];
        char *next_chars = &me->chars[row+1][0];
        unsigned char *next_attrs = &me->attrs[row+1][0];
        for (int col = 0; col < WINTEXT_MAX_COL; col++)
        {
            *chars++ = *next_chars++;
            *attrs++ = *next_attrs++;
        }
    }
    memset(&me->chars[bot][0], 0, (size_t) WINTEXT_MAX_COL);
    memset(&me->attrs[bot][0], 0, (size_t) WINTEXT_MAX_COL);
    invalidate(me, 0, bot, WINTEXT_MAX_COL, top);
}

/*
     general routine to repaint the screen
*/

void wintext_paintscreen(WinText *me,
                         int xmin,       // update this rectangular section
                         int xmax,       // of the 'screen'
                         int ymin,
                         int ymax)
{
    int istart, jstart, length, foreground, background;
    unsigned char oldbk;
    unsigned char oldfg;
    HDC hDC;

    ODS("wintext_paintscreen");

    if (me->textmode != 2)  // not in the right mode
        return;

    // first time through?  Initialize the 'screen'
    if (!me->buffer_init)
    {
        me->buffer_init = true;
        oldbk = 0x00;
        oldfg = 0x0f;
        int k = (oldbk << 4) + oldfg;
        me->buffer_init = true;
        for (int i = 0; i < me->char_xchars; i++)
        {
            for (int j = 0; j < me->char_ychars; j++)
            {
                me->chars[j][i] = ' ';
                me->attrs[j][i] = k;
            }
        }
    }

    if (xmin < 0)
        xmin = 0;
    if (xmax >= me->char_xchars)
        xmax = me->char_xchars-1;
    if (ymin < 0)
        ymin = 0;
    if (ymax >= me->char_ychars)
        ymax = me->char_ychars-1;

    hDC=GetDC(me->hWndCopy);
    SelectObject(hDC, me->hFont);
    SetBkMode(hDC, OPAQUE);
    SetTextAlign(hDC, TA_LEFT | TA_TOP);

    if (TRUE == me->showing_cursor)
    {
        ODS1("======================== Hide Caret %d", --carrot_count);
        HideCaret(me->hWndCopy);
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

    if (me->textmode != 2)  // not in the right mode
    {
        return;
    }

    me->cursor_x = xpos;
    me->cursor_y = ypos;
    if (cursor_type >= 0) me->cursor_type = cursor_type;
    if (me->cursor_type < 0) me->cursor_type = 0;
    if (me->cursor_type > 2) me->cursor_type = 2;
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
    {
        x = me->cursor_x*me->char_width;
        y = me->cursor_y*me->char_height;
        SetCaretPos(x, y);
        ODS2("======================== Set Caret Pos #1 (%d,%d)", x, y);
    }
}

void wintext_set_attr(WinText *me, int row, int col, int attr, int count)
{
    int xmin, xmax, ymin, ymax;
    xmax = col;
    xmin = xmax;
    ymax = row;
    ymin = ymax;
    for (int i = 0; i < count; i++)
    {
        me->attrs[row][col+i] = (unsigned char)(attr & 0xFF);
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
    for (int y = 0; y < WINTEXT_MAX_ROW; y++)
    {
        memset(&me->chars[y][0], ' ', (size_t) WINTEXT_MAX_COL);
        memset(&me->attrs[y][0], 0xf0, (size_t) WINTEXT_MAX_COL);
    }
    InvalidateRect(me->hWndCopy, nullptr, FALSE);
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
    InvalidateRect(me->hWndCopy, nullptr, FALSE);
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
    InvalidateRect(window, nullptr, FALSE);
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
    me->chars[row][col] = (char_attr >> 8) & 0xFF;
    me->attrs[row][col] = (char_attr & 0xFF);
}

void wintext_resume(WinText *me)
{
    g_me = me;
}
#endif

void x11_text_window::put_string(int xpos, int ypos, int attrib, std::string const &text, int *end_row, int *end_col)
{
    unsigned char const xa = static_cast<unsigned char>(attrib & 0x0ff);
    int maxrow = ypos;
    int j = maxrow;
    int maxcol = xpos-1;
    int k = maxcol;

    bool stored = false;
    for (char xc : text)
    {
        if (xc == '\n')
        {
            if (j < X11_TEXT_MAX_ROW-1)
            {
                j++;
            }
            k = xpos-1;
        }
        else
        {
            if ((++k) >= X11_TEXT_MAX_COL)
            {
                if (j < X11_TEXT_MAX_ROW-1)
                {
                    j++;
                }
                k = xpos;
            }
            maxrow = std::max(maxrow, j);
            maxcol = std::max(maxcol, k);
            text_[j][k] = xc;
            attributes_[j][k] = xa;
            stored = true;
        }
    }
    if (stored)
    {
        repaint(xpos, ypos, maxcol, maxrow);
        *end_row = j;
        *end_col = k+1;
    }
}

void x11_text_window::repaint(int xmin, int xmax, int ymin, int ymax)
{
    unsigned char oldbk;
    unsigned char oldfg;

    if (text_mode_ != 2)  // not in the right mode
        return;

    // first time through?  Initialize the 'screen'
    if (!buffer_init_)
    {
        buffer_init_ = true;
        oldbk = 0x00;
        oldfg = 0x0f;
        int k = (oldbk << 4) + oldfg;
        for (int i = 0; i < char_xchars_; i++)
        {
            for (int j = 0; j < char_ychars_; j++)
            {
                text_[j][i] = ' ';
                attributes_[j][i] = k;
            }
        }
    }

    if (xmin < 0)
        xmin = 0;
    if (xmax >= char_xchars_)
        xmax = char_xchars_-1;
    if (ymin < 0)
        ymin = 0;
    if (ymax >= char_ychars_)
        ymax = char_ychars_-1;

    XGCValues values;
    unsigned long values_mask = 0;
    GC gc = XCreateGC(dpy_, window_, values_mask, &values);

//    SelectObject(hDC, hFont);
//    SetBkMode(hDC, OPAQUE);
//    SetTextAlign(hDC, TA_LEFT | TA_TOP);

    if (showing_cursor_)
    {
//        ODS1("======================== Hide Caret %d", --carrot_count);
//        HideCaret(hWndCopy);
    }

    /*
    the following convoluted code minimizes the number of
    discrete calls to the Windows interface by locating
    'strings' of screen locations with common foreground
    and background colors
    */
    int istart = 0;
    int jstart = 0;
    unsigned char foreground = 0;
    unsigned char background = 0;
    for (int j = ymin; j <= ymax; j++)
    {
        int length = 0;
        oldbk = 99;
        oldfg = 99;
        for (int i = xmin; i <= xmax+1; i++)
        {
            int k = -1;
            if (i <= xmax)
            {
                k = attributes_[j][i];
            }
            foreground = static_cast<unsigned char>(k & 15);
            background = static_cast<unsigned char>(k >> 4);
            if (i > xmax || foreground != (int)oldfg || background != (int)oldbk)
            {
                if (length > 0)
                {
                    XSetBackground(dpy_, gc, text_colors[oldbk].pixel);
                    XSetForeground(dpy_, gc, text_colors[oldfg].pixel);
                    XDrawImageString(dpy_, window_, gc, istart*char_width_, jstart*char_height_, &text_[jstart][istart], length);
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

    if (showing_cursor_)
    {
//        ODS1("======================== Show Caret %d", ++carrot_count);
//        ShowCaret(hWndCopy);
    }

    XFreeGC(dpy_, gc);
}
