// SPDX-License-Identifier: GPL-3.0-only
//
#include "port.h"

#include "drivers.h"
#include "goodbye.h"
#include "id.h"
#include "id_keys.h"
#include "mouse.h"

#include "win_defines.h"
#include <tchar.h>
#include <Windows.h>
#include <windowsx.h>

#include "frame.h"
#include "mouse.h"
#include "resource.h"
#include "win_text.h"

#include <atlbase.h>
#include <tchar.h>

#include <cctype>
#include <stdexcept>
#include <string>

Frame g_frame{};

constexpr const TCHAR *const LEFT_POS{_T("Left")};
constexpr const TCHAR *const TOP_POS{_T("Top")};
constexpr const TCHAR *const WINDOW_POS_KEY{_T("SOFTWARE\\" ID_VENDOR_NAME "\\" ID_PROGRAM_NAME "\\Settings\\Window")};

static void forget_frame_position(CRegKey &key)
{
    key.DeleteValue(LEFT_POS);
    key.DeleteValue(TOP_POS);
}

static void save_frame_position(HWND window)
{
    RECT rect{};
    GetWindowRect(window, &rect);
    CRegKey key;
    if (key.Create(HKEY_CURRENT_USER, WINDOW_POS_KEY) != ERROR_SUCCESS)
    {
        return;
    }
    if (key.SetDWORDValue(LEFT_POS, rect.left) != ERROR_SUCCESS)
    {
        return;
    }
    key.SetDWORDValue(TOP_POS, rect.top);
}

static POINT get_saved_frame_position()
{
    POINT pos;
    CRegKey key;
    if (key.Open(HKEY_CURRENT_USER, WINDOW_POS_KEY) == ERROR_SUCCESS)
    {
        auto get_value = [&](const TCHAR *name)
        {
            DWORD value;
            if (const LONG status = key.QueryDWORDValue(name, value); status != ERROR_SUCCESS)
            {
                if (status == ERROR_FILE_NOT_FOUND)
                {
                    return CW_USEDEFAULT;
                }
                throw std::runtime_error(std::string{"QueryDWORDValue "} + name + " failed: " + std::to_string(status));
            }
            return static_cast<int>(value);
        };
        pos.x = get_value(LEFT_POS);
        pos.y = pos.x != CW_USEDEFAULT ? get_value(TOP_POS) : CW_USEDEFAULT;

        if (pos.x != CW_USEDEFAULT && pos.y != CW_USEDEFAULT)
        {
            const HMONITOR monitor{MonitorFromPoint(pos, MONITOR_DEFAULTTONULL)};
            if (monitor == nullptr)
            {
                forget_frame_position(key);
                return {CW_USEDEFAULT, CW_USEDEFAULT};
            }
        }
    }
    else
    {
        pos.x = CW_USEDEFAULT;
        pos.y = CW_USEDEFAULT;
    }

    return pos;
}

void Frame::on_close(HWND window)
{
    if (window != m_window)
    {
        return;
    }

    save_frame_position(window);
    PostQuitMessage(0);
}

void Frame::on_paint(HWND window)
{
    if (window != m_window)
    {
        return;
    }

    PAINTSTRUCT ps;
    HDC hDC = BeginPaint(window, &ps);
    EndPaint(window, &ps);
}

void Frame::add_key_press(unsigned int key)
{
    if (key_buffer_full())
    {
        _ASSERTE(m_key_press_count < KEY_BUF_MAX);
        // no room
        return;
    }

    m_key_press_buffer[g_frame.m_key_press_head] = key;
    if (++m_key_press_head >= KEY_BUF_MAX)
    {
        m_key_press_head = 0;
    }
    m_key_press_count++;
}

inline bool has_mod(int modifier)
{
    return (GetKeyState(modifier) & 0x8000) != 0;
}

inline unsigned int mod_key(int modifier, int code, int id_key, unsigned int *j = nullptr)
{
    if (has_mod(modifier))
    {
        if (j != nullptr)
        {
            *j = 0;
        }
        return id_key;
    }
    return 1000 + code;
}

#undef ID_DEBUG_KEYSTROKES
#ifdef ID_DEBUG_KEYSTROKES
inline void debug_key_strokes(const std::string &text)
{
    driver_debug_line(text);
}
#else
inline void debug_key_strokes(const std::string &text)
{
}
#endif

void Frame::on_key_down(HWND window, UINT vk, BOOL down, int repeat_count, UINT flags)
{
    if (window != m_window)
    {
        return;
    }

    // KEYUP, KEYDOWN, and CHAR msgs go to the 'key pressed' code
    // a key has been pressed - maybe ASCII, maybe not
    // if it's an ASCII key, 'WM_CHAR' will handle it
    unsigned int i = MapVirtualKey(vk, MAPVK_VK_TO_VSC);
    unsigned int j = MapVirtualKey(vk, MAPVK_VK_TO_CHAR);
    const bool alt = has_mod(VK_MENU);
    debug_key_strokes("OnKeyDown vk: " + std::to_string(vk) + ", vsc: " + std::to_string(i) + ", char: " + std::to_string(j));

    // handle modifier keys on the non-WM_CHAR keys
    if (VK_F1 <= vk && vk <= VK_F10)
    {
        if (has_mod(VK_SHIFT))
        {
            i = mod_key(VK_SHIFT, i, ID_KEY_SF1 + (vk - 0x70));
        }
        else if (has_mod(VK_CONTROL))
        {
            i = mod_key(VK_CONTROL, i, ID_KEY_CTL_F1 + (vk - 0x70));
        }
        else if (alt)
        {
            i = mod_key(VK_MENU, i, ID_KEY_ALT_F1 + (vk - 0x70));
        }
        else
        {
            i = ID_KEY_F1 + vk - VK_F1;
        }
    }
    else
    {
        switch (vk)
        {
        // sorted in ID_KEY_xxx order
        case VK_DELETE:
            i = mod_key(VK_CONTROL, i, ID_KEY_CTL_DEL);
            break;
        case VK_DOWN:
            i = mod_key(VK_CONTROL, i, ID_KEY_CTL_DOWN_ARROW);
            break;
        case VK_END:
            i = mod_key(VK_CONTROL, i, ID_KEY_CTL_END);
            break;
        case VK_RETURN:
            i = mod_key(VK_CONTROL, i, ID_KEY_CTL_ENTER);
            break;
        case VK_HOME:
            i = mod_key(VK_CONTROL, i, ID_KEY_CTL_HOME);
            break;
        case VK_INSERT:
            i = mod_key(VK_CONTROL, i, ID_KEY_CTL_INSERT);
            break;
        case VK_LEFT:
            i = mod_key(VK_CONTROL, i, ID_KEY_CTL_LEFT_ARROW);
            break;
        case VK_PRIOR:
            i = mod_key(VK_CONTROL, i, ID_KEY_CTL_PAGE_UP);
            break;
        case VK_NEXT:
            i = mod_key(VK_CONTROL, i, ID_KEY_CTL_PAGE_DOWN);
            break;
        case VK_RIGHT:
            i = mod_key(VK_CONTROL, i, ID_KEY_CTL_RIGHT_ARROW);
            break;
        case VK_UP:
            i = mod_key(VK_CONTROL, i, ID_KEY_CTL_UP_ARROW);
            break;
        case VK_TAB:
            i = mod_key(VK_CONTROL, i, ID_KEY_CTL_TAB, &j);
            break;
        case VK_ADD:
            i = mod_key(VK_CONTROL, i, ID_KEY_CTL_PLUS, &j);
            break;
        case VK_SUBTRACT:
            i = mod_key(VK_CONTROL, i, ID_KEY_CTL_MINUS, &j);
            break;

        default:
            if (0 == j)
            {
                i += 1000;
            }
            break;
        }
    }

    if (alt && j != 0)
    {
        // handle Alt+1 through Alt+7
        if (j >= '1' && j <= '7')
        {
            i = ID_KEY_ALT_1 + (j - '1');
            add_key_press(i);
            debug_key_strokes("OnKeyDown key: " + std::to_string(i));
        }
        else if (std::tolower(j) == 'a')
        {
            i = ID_KEY_ALT_A;
            add_key_press(i);
            debug_key_strokes("OnKeyDown key: " + std::to_string(i));
        }
        else if (std::tolower(j) == 's')
        {
            i = ID_KEY_ALT_S;
            add_key_press(i);
            debug_key_strokes("OnKeyDown key: " + std::to_string(i));
        }
    }

    // use this call only for non-ASCII keys
    if (!(vk == VK_SHIFT || vk == VK_CONTROL || vk == VK_MENU) && (j == 0))
    {
        add_key_press(i);
        debug_key_strokes("OnKeyDown key: " + std::to_string(i));
    }
    debug_key_strokes("OnKeyDown exit vk: " + std::to_string(vk) + ", vsc: " + std::to_string(i) + ", char: " + std::to_string(j));
}

void Frame::on_char(HWND window, TCHAR ch, int num_repeat)
{
    if (window != m_window)
    {
        return;
    }

    // KEYUP, KEYDOWN, and CHAR msgs go to the SG code
    // an ASCII key has been pressed
    unsigned int i, j, k;
    i = (unsigned int)((num_repeat & 0x00ff0000) >> 16);
    j = ch;
    k = (i << 8) + j;
    if (k == '\t' && has_mod(VK_SHIFT))
    {
        k = ID_KEY_SHF_TAB;
    }
    add_key_press(k);
    debug_key_strokes("OnChar " + std::to_string(k));
}

void Frame::on_get_min_max_info(HWND hwnd, LPMINMAXINFO info)
{
    if (hwnd != m_window)
    {
        return;
    }

    info->ptMaxSize.x = m_nc_width;
    info->ptMaxSize.y = m_nc_height;
    info->ptMaxTrackSize = info->ptMaxSize;
    info->ptMinTrackSize = info->ptMaxSize;
}

void Frame::on_timer(HWND window, UINT id)
{
    if (window != m_window)
    {
        return;
    }

    _ASSERTE(FRAME_TIMER_ID == id);
    m_timed_out = true;
}

static void frame_OnClose(HWND window)
{
    g_frame.on_close(window);
}

static void frame_OnSetFocus(HWND window, HWND old_focus)
{
    g_frame.on_set_focus(window, old_focus);
}

static void frame_OnKillFocus(HWND window, HWND old_focus)
{
    g_frame.on_kill_focus(window, old_focus);
}

static void frame_OnPaint(HWND window)
{
    g_frame.on_paint(window);
}

static void frame_OnKeyDown(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
{
    g_frame.on_key_down(hwnd, vk, fDown, cRepeat, flags);
}

static void frame_OnChar(HWND hwnd, TCHAR ch, int cRepeat)
{
    g_frame.on_char(hwnd, ch, cRepeat);
}

static void frame_OnGetMinMaxInfo(HWND hwnd, LPMINMAXINFO info)
{
    g_frame.on_get_min_max_info(hwnd, info);
}

static void frame_OnTimer(HWND window, UINT id)
{
    g_frame.on_timer(window, id);
}

static void frame_OnLButtonUp(HWND window, int x, int y, UINT key_flags)
{
    g_frame.on_left_button_up(window, x, y, key_flags);
}

static void frame_OnRButtonUp(HWND window, int x, int y, UINT key_flags)
{
    g_frame.on_right_button_up(window, x, y, key_flags);
}

static void frame_OnMButtonUp(HWND window, int x, int y, UINT key_flags)
{
    g_frame.on_middle_button_up(window, x, y, key_flags);
}

static void frame_OnMouseMove(HWND window, int x, int y, UINT key_flags)
{
    g_frame.on_mouse_move(window,x, y, key_flags);
}

static void frame_OnLButtonDown(HWND window, BOOL double_click, int x, int y, UINT key_flags)
{
    g_frame.on_left_button_down(window, double_click, x, y, key_flags);
}

static void frame_OnRButtonDown(HWND window, BOOL double_click, int x, int y, UINT key_flags)
{
    g_frame.on_right_button_down(window, double_click, x, y, key_flags);
}

static void frame_OnMButtonDown(HWND window, BOOL double_click, int x, int y, UINT key_flags)
{
    g_frame.on_middle_button_down(window, double_click, x, y, key_flags);
}

static LRESULT CALLBACK frame_window_proc(HWND window, UINT message, WPARAM wp, LPARAM lp)
{
    switch (message)
    {
    case WM_CLOSE:
        HANDLE_WM_CLOSE(window, wp, lp, frame_OnClose);
        break;
    case WM_GETMINMAXINFO:
        HANDLE_WM_GETMINMAXINFO(window, wp, lp, frame_OnGetMinMaxInfo);
        break;
    case WM_SETFOCUS:
        HANDLE_WM_SETFOCUS(window, wp, lp, frame_OnSetFocus);
        break;
    case WM_KILLFOCUS:
        HANDLE_WM_KILLFOCUS(window, wp, lp, frame_OnKillFocus);
        break;
    case WM_PAINT:
        HANDLE_WM_PAINT(window, wp, lp, frame_OnPaint);
        break;
    case WM_KEYDOWN:
        HANDLE_WM_KEYDOWN(window, wp, lp, frame_OnKeyDown);
        break;
    case WM_SYSKEYDOWN:
        HANDLE_WM_SYSKEYDOWN(window, wp, lp, frame_OnKeyDown);
        break;
    case WM_CHAR:
        HANDLE_WM_CHAR(window, wp, lp, frame_OnChar);
        break;
    case WM_TIMER:
        HANDLE_WM_TIMER(window, wp, lp, frame_OnTimer);
        break;
    case WM_LBUTTONUP:
        HANDLE_WM_LBUTTONUP(window, wp, lp, frame_OnLButtonUp);
        break;
    case WM_RBUTTONUP:
        HANDLE_WM_RBUTTONUP(window, wp, lp, frame_OnRButtonUp);
        break;
    case WM_MBUTTONUP:
        HANDLE_WM_MBUTTONUP(window, wp, lp, frame_OnMButtonUp);
        break;
    case WM_MOUSEMOVE:
        HANDLE_WM_MOUSEMOVE(window, wp, lp, frame_OnMouseMove);
        break;
    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
        HANDLE_WM_LBUTTONDOWN(window, wp, lp, frame_OnLButtonDown);
        break;
    case WM_RBUTTONDOWN:
    case WM_RBUTTONDBLCLK:
        HANDLE_WM_RBUTTONDOWN(window, wp, lp, frame_OnRButtonDown);
        break;
    case WM_MBUTTONDOWN:
    case WM_MBUTTONDBLCLK:
        HANDLE_WM_MBUTTONDOWN(window, wp, lp, frame_OnMButtonDown);
        break;
    default:
        return DefWindowProc(window, message, wp, lp);
    }
    return 0;
}

void Frame::init(HINSTANCE instance, LPCSTR title)
{
    LPCTSTR windowClass = _T("IdFrame");
    WNDCLASS  wc;

    bool status = GetClassInfo(instance, windowClass, &wc) != 0;
    if (!status)
    {
        m_instance = instance;
        m_title = title;

        wc.style = 0;
        wc.lpfnWndProc = frame_window_proc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = m_instance;
        wc.hIcon = LoadIcon(instance, MAKEINTRESOURCE(IDI_ITERATED_DYNAMICS));
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_BACKGROUND+1);
        wc.lpszMenuName = m_title.c_str();
        wc.lpszClassName = windowClass;

        status = RegisterClass(&wc) != 0;
    }
    _ASSERTE(status);

    m_key_press_count = 0;
    m_key_press_head  = 0;
    m_key_press_tail  = 0;
}

void Frame::terminate()
{
    save_frame_position(m_window);
}

void Frame::pump_messages(bool wait_flag)
{
    MSG msg;
    bool quitting = false;
    m_timed_out = false;

    while (!quitting)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_NOREMOVE) == 0)
        {
            // no messages waiting
            if (!wait_flag                     //
                || m_key_press_count != 0      //
                || (wait_flag && m_timed_out)) //
            {
                return;
            }
        }

        if (int result = GetMessage(&msg, nullptr, 0, 0); result > 0)
        {
            // translate accelerator here?
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else if (0 == result)
        {
            quitting = true;
        }
    }

    if (quitting)
    {
        goodbye();
    }
}

int Frame::get_key_press(bool wait_for_key)
{
    pump_messages(wait_for_key);
    if (wait_for_key && m_timed_out)
    {
        return 0;
    }

    if (m_key_press_count == 0)
    {
        _ASSERTE(!wait_for_key);
        return 0;
    }

    const int i = m_key_press_buffer[m_key_press_tail];

    if (++m_key_press_tail >= KEY_BUF_MAX)
    {
        m_key_press_tail = 0;
    }
    m_key_press_count--;

    return i;
}

void Frame::adjust_size(int width, int height)
{
    m_width = width;
    m_nc_width = width + GetSystemMetrics(SM_CXFRAME) * 2;
    m_height = height;
    m_nc_height = height + GetSystemMetrics(SM_CYFRAME) * 4 + GetSystemMetrics(SM_CYCAPTION) - 1;
}

void Frame::on_set_focus(HWND window, HWND /*old_focus*/)
{
    if (window != m_window)
    {
        return;
    }
    m_has_focus = true;
}

void Frame::on_kill_focus(HWND window, HWND /*old_focus*/)
{
    if (window != m_window)
    {
        return;
    }
    g_frame.m_has_focus = false;
}

std::string key_flags_string(UINT value)
{
    std::string result;
    const auto append_flag = [&](UINT flag, const char *label)
    {
        if (value & flag)
        {
            if (!result.empty())
            {
                result += " | ";
            }
            result += label;
        }
    };
    append_flag(MK_LBUTTON, "LEFT");
    append_flag(MK_RBUTTON, "RIGHT");
    append_flag(MK_SHIFT, "SHIFT");
    append_flag(MK_CONTROL, "CONTROL");
    append_flag(MK_MBUTTON, "MIDDLE");
#if (_WIN32_WINNT >= 0x0500)
    append_flag(MK_XBUTTON1, "X1");
    append_flag(MK_XBUTTON2, "X2");
#endif
    return result.empty() ? "NONE" : result;
}

void Frame::on_left_button_up(HWND window, int x, int y, UINT key_flags)
{
    if (g_look_at_mouse < 0)
    {
        add_key_press(-g_look_at_mouse);
    }
    else
    {
        driver_debug_line("left up: " + std::to_string(x) + "," + std::to_string(y) +
            ", flags: " + key_flags_string(key_flags));
    }
}

void Frame::on_right_button_up(HWND window, int x, int y, UINT key_flags)
{
    driver_debug_line(
        "right up: " + std::to_string(x) + "," + std::to_string(y) + ", flags: " + key_flags_string(key_flags));
}

void Frame::on_middle_button_up(HWND window, int x, int y, UINT key_flags)
{
    driver_debug_line("middle up: " + std::to_string(x) + "," + std::to_string(y) +
        ", flags: " + key_flags_string(key_flags));
}

/*
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
MouseStatus   equ 03h ; get mouse position and button status
MousePress    equ 05h ; get mouse button press information
MouseRelease  equ 06h ; get mouse button release information
MouseCounts   equ 0bh ; get mouse motion counters
LeftButton    equ 00h
RightButton   equ 01h

mouseread proc near USES bx cx dx
        local   moveaxis:word

        ; check if it is time to do an autosave
        cmp     saveticks,0     ; autosave timer running?
        je      mouse0          ;  nope
        sub     ax,ax           ; reset ES to BIOS data area
        mov     es,ax           ;  see notes at mouse1 in similar code
tickread:
        mov     ax,es:046ch     ; obtain the current timer value
        cmp     ax,savechktime  ; a new clock tick since last check?
        je      mouse0          ;  nope, save a dozen opcodes or so
        mov     dx,es:046eh     ; high word of ticker
        cmp     ax,es:046ch     ; did a tick get counted just as we looked?
        jne     tickread        ; yep, reread both words to be safe
        mov     savechktime,ax
        sub     ax,savebase     ; calculate ticks since timer started
        sbb     dx,savebase+2
        jns     tickcompare
        add     ax,0b0h         ; wrapped past midnight, add a day
        adc     dx,018h
tickcompare:
        cmp     dx,saveticks+2  ; check if past autosave time
        jb      mouse0
        ja      ticksavetime
        cmp     ax,saveticks
        jb      mouse0
ticksavetime:                   ; it is time to do a save
        mov     ax,finishrow
        cmp     ax,-1           ; waiting for the end of a row before save?
        jne     tickcheckrow    ;  yup, go check row
        cmp     calc_status,1   ; safety check, calc active?
        jne     tickdosave      ;  nope, don't check type of calc
        cmp     got_status,0    ; 1pass or 2pass?
        je      ticknoterow     ;  yup
        cmp     got_status,1    ; solid guessing?
        jne     tickdosave      ;  not 1pass, 2pass, ssg, so save immediately
ticknoterow:
        mov     ax,currow       ; note the current row
        mov     finishrow,ax    ;  ...
        jmp     short mouse0    ; and keep working for now
tickcheckrow:
        cmp     ax,currow       ; started a new row since timer went off?
        je      mouse0          ;  nope, don't do the save yet
tickdosave:
        mov     timedsave,1     ; tell mainline what's up
        mov     ax,9999         ; a dummy key value, never gets used
        jmp     mouseret

mouse0: ; now the mouse stuff
        cmp     mouse,-1
        jne     mouseidle       ; no mouse, that was easy
        mov     ax,lookatmouse
        cmp     ax,prevlamouse
        je      mouse1

        ; lookatmouse changed, reset everything
        mov     prevlamouse,ax
        mov     mbclicks,0
        mov     mbstatus,0
        mov     mhmickeys,0
        mov     mvmickeys,0
        ; note: don't use int 33 func 0 nor 21 to reset, they're SLOW
        mov     ax,MouseRelease ; reset button counts by reading them
        mov     bx,LeftButton
        int     33h
        mov     ax,MouseRelease
        mov     bx,RightButton
        int     33h
        mov     ax,MousePress
        mov     bx,LeftButton
        int     33h
        mov     ax,MouseCounts  ; reset motion counters by reading
        int     33h
        mov     ax,lookatmouse

mouse1: or      ax,ax
        jz      mouseidle       ; check nothing when lookatmouse=0
        ; following code directly accesses bios tick counter; it would be
        ; better not to rely on addr (use int 1A instead) but old PCs don't
        ; have the required int, the addr is constant in bios to date, and
        ; fractint startup already counts on it, so:
        mov     ax,0            ; reset ES to BIOS data area
        mov     es,ax           ;  ...
        mov     dx,es:46ch      ; obtain the current timer value
        cmp     dx,mousetime
        ; if timer same as last call, skip int 33s:  reduces expense and gives
        ; caller a chance to read all pending stuff and paint something
        jne     mnewtick
        cmp     lookatmouse,0   ; interested in anything other than left button?
        jl      mouseidle       ; nope, done
        jmp     mouse5

mouseidle:
        clc                     ; tell caller no mouse activity this time
        ret

mnewtick: ; new tick, read buttons and motion
        mov     mousetime,dx    ; note current timer
        cmp     lookatmouse,3
        je      mouse2          ; skip button press if mode 3

        ; check press of left button
        mov     ax,MousePress
        mov     bx,LeftButton
        int     33h
        or      bx,bx
        jnz     mleftb
        cmp     lookatmouse,0
        jl      mouseidle       ; exit if nothing but left button matters
        jmp     mouse3          ; not mode 3 so skip past button release stuff
mleftb: mov     ax,13
        cmp     lookatmouse,0
        jg      mouser          ; return fake key enter
        mov     ax,lookatmouse  ; return fake key 0-lookatmouse
        neg     ax
mouser: jmp     mouseret

mouse2: ; mode 3, check for double clicks
        mov     ax,MouseRelease ; get button release info
        mov     bx,LeftButton
        int     33h
        mov     dx,mousetime
        cmp     bx,1            ; left button released?
        jl      msnolb          ; go check timer if not
        jg      mslbgo          ; double click
        test    mbclicks,1      ; had a 1st click already?
        jnz     mslbgo          ; yup, double click
        mov     mlbtimer,dx     ; note time of 1st click
        or      mbclicks,1
        jmp     short mousrb
mslbgo: and     mbclicks,0ffh-1
        mov     ax,13           ; fake key enter
        jmp     mouseret
msnolb: sub     dx,mlbtimer     ; clear 1st click if its been too long
        cmp     dx,DclickTime
        jb      mousrb
        and     mbclicks,0ffh-1 ; forget 1st click if any
        ; next all the same code for right button
mousrb: mov     ax,MouseRelease ; get button release info
        mov     bx,RightButton
        int     33h
        ; now much the same as for left
        mov     dx,mousetime
        cmp     bx,1
        jl      msnorb
        jg      msrbgo
        test    mbclicks,2
        jnz     msrbgo
        mov     mrbtimer,dx
        or      mbclicks,2
        jmp     short mouse3
msrbgo: and     mbclicks,0ffh-2
        mov     ax,1010         ; fake key ctl-enter
        jmp     mouseret
msnorb: sub     dx,mrbtimer
        cmp     dx,DclickTime
        jb      mouse3
        and     mbclicks,0ffh-2

        ; get buttons state, if any changed reset mickey counters
mouse3: mov     ax,MouseStatus  ; get button status
        int     33h
        and     bl,7            ; just the button bits
        cmp     bl,mbstatus     ; any changed?
        je      mouse4
        mov     mbstatus,bl     ; yup, reset stuff
        mov     mhmickeys,0
        mov     mvmickeys,0
        mov     ax,MouseCounts
        int     33h             ; reset driver's mickeys by reading them

        ; get motion counters, forget any jiggle
mouse4: mov     ax,MouseCounts  ; get motion counters
        int     33h
        mov     bx,mousetime    ; just to have it in a register
        cmp     cx,0            ; horiz motion?
        jne     moushm          ; yup, go accum it
        mov     ax,bx
        sub     ax,mhtimer
        cmp     ax,JitterTime   ; timeout since last horiz motion?
        jb      mousev
        mov     mhmickeys,0
        jmp     short mousev
moushm: mov     mhtimer,bx      ; note time of latest motion
        add     mhmickeys,cx
        ; same as above for vertical movement:
mousev: cmp     dx,0            ; vert motion?
        jne     mousvm
        mov     ax,bx
        sub     ax,mvtimer
        cmp     ax,JitterTime
        jb      mouse5
        mov     mvmickeys,0
        jmp     short mouse5
mousvm: mov     mvtimer,bx
        add     mvmickeys,dx

        ; pick the axis with largest pending movement
mouse5: mov     bx,mhmickeys
        or      bx,bx
        jns     mchkv
        neg     bx              ; make it +ve
mchkv:  mov     cx,mvmickeys
        or      cx,cx
        jns     mchkmx
        neg     cx
mchkmx: mov     moveaxis,0      ; flag that we're going horiz
        cmp     bx,cx           ; horiz>=vert?
        jge     mangle
        xchg    bx,cx           ; nope, use vert
        mov     moveaxis,1      ; flag that we're going vert

        ; if moving nearly horiz/vert, make it exactly horiz/vert
mangle: mov     ax,TextVHLimit
        cmp     lookatmouse,2   ; slow (text) mode?
        je      mangl2
        mov     ax,GraphVHLimit
        cmp     lookatmouse,3   ; special mode?
        jne     mangl2
        cmp     mbstatus,0      ; yup, any buttons down?
        je      mangl2
        mov     ax,ZoomVHLimit  ; yup, special zoom functions
mangl2: mul     cx              ; smaller axis * limit
        cmp     ax,bx
        ja      mchkmv          ; min*ratio <= max?
        cmp     moveaxis,0      ; yup, clear the smaller movement axis
        jne     mzeroh
        mov     mvmickeys,0
        jmp     short mchkmv
mzeroh: mov     mhmickeys,0

        ; pick sensitivity to use
mchkmv: cmp     lookatmouse,2   ; slow (text) mode?
        je      mchkmt
        mov     dx,ZoomSens+JitterMickeys
        cmp     lookatmouse,3   ; special mode?
        jne     mchkmg
        cmp     mbstatus,0      ; yup, any buttons down?
        jne     mchkm2          ; yup, use zoomsens
mchkmg: mov     dx,GraphSens
        mov     cx,sxdots       ; reduce sensitivity for higher res
mchkg2: cmp     cx,400          ; horiz dots >= 400?
        jl      mchkg3
        shr     cx,1            ; horiz/2
        shr     dx,1
        inc     dx              ; sensitivity/2+1
        jmp     short mchkg2
mchkg3: add     dx,JitterMickeys
        jmp     short mchkm2
mchkmt: mov     dx,TextVSens+JitterMickeys
        cmp     moveaxis,0
        jne     mchkm2
        mov     dx,TextHSens+JitterMickeys ; slower on X axis than Y

        ; is largest movement past threshold?
mchkm2: cmp     bx,dx
        jge     mmove
        jmp     mouseidle       ; no movement past threshold, return nothing

        ; set bx for right/left/down/up, and reduce the pending mickeys
mmove:  sub     dx,JitterMickeys
        cmp     moveaxis,0
        jne     mmovev
        cmp     mhmickeys,0
        jl      mmovh2
        sub     mhmickeys,dx    ; horiz, right
        mov     bx,0
        jmp     short mmoveb
mmovh2: add     mhmickeys,dx    ; horiz, left
        mov     bx,2
        jmp     short mmoveb
mmovev: cmp     mvmickeys,0
        jl      mmovv2
        sub     mvmickeys,dx    ; vert, down
        mov     bx,4
        jmp     short mmoveb
mmovv2: add     mvmickeys,dx    ; vert, up
        mov     bx,6

        ; modify bx if a button is being held down
mmoveb: cmp     lookatmouse,3
        jne     mmovek          ; only modify in mode 3
        cmp     mbstatus,1
        jne     mmovb2
        add     bx,8            ; modify by left button
        jmp     short mmovek
mmovb2: cmp     mbstatus,2
        jne     mmovb3
        add     bx,16           ; modify by rb
        jmp     short mmovek
mmovb3: cmp     mbstatus,0
        je      mmovek
        add     bx,24           ; modify by middle or multiple

        ; finally, get the fake key number
mmovek: mov     ax,mousefkey[bx]

mouseret:
        stc
        ret
mouseread endp

    if (g_look_at_mouse != m_look_at_mouse)
    {
        m_look_at_mouse = g_look_at_mouse;
        m_mb_clicks = 0;
        m_mb_status = 0;
        m_hmic_keys = 0;
        m_vmic_keys = 0;
    }

    if (g_look_at_mouse == +MouseLook::IGNORE_MOUSE)
    {
        return;
    }

    if (g_look_at_mouse < 0)
    {
        return;
    }

    const long mouse_time = GetMessageTime();
    if (mouse_time == m_mouse_time)
    {
        return;
    }
    m_mouse_time = mouse_time;

    if (g_look_at_mouse == +MouseLook::POSITION)
        goto mouse2;

    if (MK_BUTTON1)
        goto mleftb;

    if (g_look_at_mouse < 0)
        goto mouseidle;

    goto mouse3;

mleftb:
    ax = ENTER;
    if (g_look_at_mouse > 0)
        goto mouser;
    ax = -g_look_at_mouse;

mouser:
    goto mouseret;

mouse2:
    // mode POSITION, check for double clicks
    if (LBTN_DOUBLE_CLICK)
        goto mslbgo;

    goto mousrb;

mslbgo:
    ax = ENTER;
    goto mouseret;

mousrb:
    if (RBTN_DOUBLE_CLICK)
        goto msrbgo;

    ax = CTL_ENTER;
    goto mouseret;

mouse3:
    if (mouse_state == m_mouse_state)
        goto mouse4;

    m_mouse_state = mouse_state;
    mhmickeys = 0;
    mvmickeys = 0;

mouse4:
    if (cx != 0) // horizontal motion?
        goto moushm;

    if (m_mouse_time - m_htimer < JitterTime)
        goto mousev;

    mhmickeys = 0;
    goto mousev;

mousehm:
    mhtimer = mousetime;
    mhmickeys += cx;

mousev:                     // vert motion?
    if (dx != 0)
        goto mousvm;

    ax = bx;
    ax -= mvtimer;
    if (ax > JitterTim)
        goto mouse5;
    mvmickeys = 0;
    goto mouse5;

mousvm:
    mvtimer = bx;
    mvmickeys += dx;

mouse5:                     // pick the axis with largest pending movement
    bx = mhmickeys;
    if (bx < 0)
        goto mchkv;
    bx = -bx;               // make it positive
mchkv:
    cx = mvmickeys;
    if (cx < 0)
        goto mchkmx;
    cx = -cx;
mchkmx:
    moveaxis = 0;           // flag that we're going horiz
    if (bx >= cx)           // horiz >= vert?
        goto mangle;
    tmp = bx;
    bx = cx;
    cx = tmp;               // nope use vert
    moveaxis = 1;           // flag that we're going vert

    // if moving nearly horiz/vert, make it exactly horiz/vert
mangle:
    ax = TextVHLimit;
    if (lookatmouse == 2)   // slow (text) mode?
        goto mangl2;
    ax = GraphVHLimit;
    if (lookatmouse != 3)   // special mode?
        goto mangl2;
    if (mbstatus == 0)      // yep, any buttons down?
        goto mangl2;
    ax = ZoomVHLimit;       // yep, special zoom functions
mangl2:
    ax = ax*cx;             // smaller axis * limit
    if (ax > bx)            // min*ration <= max?
        goto mchkmv;
    if (moveaxis != 0)      // yup, clear the smaller movement axis
        goto mzeroh;
    mvmickeys = 0;
    goto mchkmv;
mzeroh:
    mhmickeys = 0;
mchkmv:
    if (lookatmouse == 2)   // slow (text) mode?
        goto mchkmt;
    dx = ZoomSens + JitterMickeys;
    i (lookatmouse != 3)    // special mode?
        goto mchkmg;
    if (mbstatus != 0)      // yep, any buttons down?
        goto mchkm2;        // yep, use zoomsens
mchkmg:
    dx = GraphSens;
    cx = sxdots;            // reduce sensitivity for higher res
mchkg2:
    if (cx < 400)           // horiz dots >= 400?
        goto mchkg3;
    cx >>= 1;               // horiz/2
    dx >>= 1;
    ++dx;                   // sensitive/2+1
    goto mchkg2;
mchkg3:
    dx += JitterMickeys;
    goto mchkm2;
mchkmt:
    dx = TextVSens + JitterMickeys;
    if (moveaxis != 0)
        goto mchkm2;
    dx = TextHSens + JitterMickeys;
    if (moveaxis != 0)
        goto mchkm2;
    dx = TextHSens + JitterMickeys;

    // is largest movement past threshold?
mchkm2:
    if (bx >= dx)
        goto mmove;
    goto mouseidle;             // no movement past threshold, return nothing

    // set bx for right/let/down/up, andreduce the pending mickeys
mmove:
    dx -= JitterMickeys;
    if (moveaxis != 0)
        goto mmovev;
    if (mhmickeys <0)
        goto mmovh2;
    mhmickeys -= dx;            // horiz, right
    bx = 0;
    goto mmoveb;

mmovh2:
    mhmickeys += dx;            // horiz, left
    bx = 2;
    goto mmoveb;

mmovev:
    if (mvmickeys < 0)
        goto mmovv2;
    mvmixkeys -= dx;            // vert, down
    bx = 4;
    goto mmoveb;

mmovv2:
    mvmickeys += dx;            // vert, up
    bx = 6;

    // modify bx i a button is being held down
mmoveb:
    if (lookatmouse != 3)
        goto mmovek;            // only modify in mode 3
    if (mbstatus != 1)
        goto mmovb2;
    bx += 8;                    // moidfy by left button
    goto mmovek;
mmovb2:
    if (mbstatus != 2)
        goto mmovb3;
    bx += 16;                   // modify by right button
    goto mmovek;
mmoveb3:
    if (mbstatus == 0)
        goto mmovek;
    bx += 24;                   // modify by middle or multiple

    // finally, get the fake key number
mmovek:
    ax = mousefkey[bx];

mouseret:
    return;

*/

void Frame::on_mouse_move(HWND window, int x, int y, UINT key_flags)
{
    m_pos.x = static_cast<LONG>(x);
    m_pos.y = static_cast<LONG>(y);
    mouse_notify_move(x, y, static_cast<int>(key_flags));
    if (g_look_at_mouse <= +MouseLook::IGNORE_MOUSE)
    {
        return;
    }

    if (m_last_tick == -1L)
    {
        m_last_tick = GetMessageTime();
        return;
    }

    // [button][dir]
    static constexpr int mouse_keys[4][4]{
        {ID_KEY_RIGHT_ARROW, ID_KEY_LEFT_ARROW, ID_KEY_DOWN_ARROW, ID_KEY_UP_ARROW}, // movement
        {0, 0, ID_KEY_PAGE_DOWN, ID_KEY_PAGE_UP},                                    // left button
        {ID_KEY_CTL_PLUS, ID_KEY_CTL_MINUS, ID_KEY_CTL_DEL, ID_KEY_CTL_INSERT},      // right button
        {ID_KEY_CTL_END, ID_KEY_CTL_HOME, ID_KEY_CTL_PAGE_DOWN, ID_KEY_CTL_PAGE_UP}  // middle button
    };
    int button_num = -1;
    if ((key_flags & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON)) == 0) // movement
    {
        button_num = 0;
    }
    else if (key_flags & MK_LBUTTON)
    {
        button_num = 1;
    }
    else if (key_flags & MK_RBUTTON)
    {
        button_num = 2;
    }
    else if (key_flags & MK_MBUTTON)
    {
        button_num = 3;
    }
    if (button_num != -1)
    {
        if (m_last.x == -1)
        {
            m_last.x = x;
            m_last.y = y;
        }
        else
        {
            m_delta.x += x - m_last.x;
            m_delta.y += y - m_last.y;
        }

        if (!key_buffer_full())
        {
        }
    }

    driver_debug_line("movement: " + std::to_string(x) + "," + std::to_string(y) +
        ", flags: " + key_flags_string(key_flags));
}

void Frame::on_left_button_down(HWND window, BOOL double_click, int x, int y, UINT key_flags)
{
    mouse_notify_left_down(static_cast<bool>(double_click), x, y, static_cast<int>(key_flags));
    driver_debug_line((double_click ? "left down: (double)" : "left down: ") + std::to_string(x) + "," +
        std::to_string(y) + ", flags: " + key_flags_string(key_flags));
}

void Frame::on_right_button_down(HWND window, BOOL double_click, int x, int y, UINT key_flags)
{
    mouse_notify_right_down(x, y, static_cast<int>(key_flags));
    driver_debug_line((double_click ? "right down: (double)" : "right down: ") + std::to_string(x) + "," +
        std::to_string(y) + ", flags: " + key_flags_string(key_flags));
}

void Frame::on_middle_button_down(HWND window, BOOL double_click, int x, int y, UINT key_flags)
{
    driver_debug_line((double_click ? "middle down: (double)" : "middle down: ") + std::to_string(x) + "," +
        std::to_string(y) + ", flags: " + key_flags_string(key_flags));
}

void Frame::create_window(int width, int height)
{
    if (nullptr == m_window)
    {
        adjust_size(width, height);
        const POINT location{get_saved_frame_position()};
        m_window = CreateWindow("IdFrame", m_title.c_str(), WS_OVERLAPPEDWINDOW, location.x, location.y, m_nc_width,
            m_nc_height, nullptr, nullptr, m_instance, nullptr);
        ShowWindow(m_window, SW_SHOWNORMAL);
    }
    else
    {
        resize(width, height);
    }
}

void Frame::resize(int width, int height)
{
    adjust_size(width, height);
    BOOL status = SetWindowPos(m_window, nullptr, 0, 0, m_nc_width, m_nc_height, SWP_NOZORDER | SWP_NOMOVE);
    _ASSERTE(status);
}

void Frame::set_keyboard_timeout(int ms)
{
    UINT_PTR result = SetTimer(m_window, FRAME_TIMER_ID, ms, nullptr);
    if (!result)
    {
        DWORD error = GetLastError();
        _ASSERTE(result);
    }
}
