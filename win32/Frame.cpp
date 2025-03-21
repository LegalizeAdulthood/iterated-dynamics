// SPDX-License-Identifier: GPL-3.0-only
//
#include "Frame.h"

#include "resource.h"
#include "WinText.h"

#include "misc/Driver.h"
#include "ui/goodbye.h"
#include "ui/id_keys.h"

#include "win_defines.h"
#include <atlbase.h>
#include <tchar.h>
#include <Windows.h>
#include <windowsx.h>

#include <cassert>
#include <cctype>
#include <stdexcept>

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
    BeginPaint(window, &ps);
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

static bool has_mod(int modifier)
{
    return (GetKeyState(modifier) & 0x8000) != 0;
}

static unsigned int mod_key(int modifier, int code, int id_key, unsigned int *j = nullptr)
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
static void debug_key_strokes(const std::string &text)
{
    driver_debug_line(text);
}
#else
static void debug_key_strokes(const std::string &text)
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
    unsigned int i = MapVirtualKeyA(vk, MAPVK_VK_TO_VSC);
    unsigned int j = MapVirtualKeyA(vk, MAPVK_VK_TO_CHAR);
    const bool alt = has_mod(VK_MENU);
    const bool ctrl = has_mod(VK_CONTROL);
    debug_key_strokes("OnKeyDown vk: " + std::to_string(vk) + ", vsc: " + std::to_string(i) + ", char: " + std::to_string(j));

    // handle modifier keys on the non-WM_CHAR keys
    if (VK_F1 <= vk && vk <= VK_F10)
    {
        if (has_mod(VK_SHIFT))
        {
            i = mod_key(VK_SHIFT, i, ID_KEY_SHF_F1 + (vk - VK_F1));
        }
        else if (has_mod(VK_CONTROL))
        {
            i = mod_key(VK_CONTROL, i, ID_KEY_CTL_F1 + (vk - VK_F1));
        }
        else if (alt)
        {
            i = mod_key(VK_MENU, i, ID_KEY_ALT_F1 + (vk - VK_F1));
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

    if (j != 0)
    {
        if (alt)
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
        if (ctrl)
        {
            if (j == '-')
            {
                i = ID_KEY_CTL_MINUS;
                add_key_press(i);
            }
            else if (j == '+' || j == '=')
            {
                i = ID_KEY_CTL_PLUS;
                add_key_press(i);
            }
        }
    }

    // use this call only for non-ASCII keys
    if (vk != VK_SHIFT && vk != VK_CONTROL && vk != VK_MENU && j == 0)
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
    unsigned int i = (unsigned int) ((num_repeat & 0x00ff0000) >> 16);
    unsigned int j = ch;
    unsigned int k = (i << 8) + j;
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

static void frame_on_close(HWND window)
{
    g_frame.on_close(window);
}

static void frame_on_set_focus(HWND window, HWND old_focus)
{
    g_frame.on_set_focus(window, old_focus);
}

static void frame_on_kill_focus(HWND window, HWND old_focus)
{
    g_frame.on_kill_focus(window, old_focus);
}

static void frame_on_paint(HWND window)
{
    g_frame.on_paint(window);
}

static void frame_on_key_down(HWND hwnd, UINT vk, BOOL down, int repeat, UINT flags)
{
    g_frame.on_key_down(hwnd, vk, down, repeat, flags);
}

static void frame_on_char(HWND hwnd, TCHAR ch, int repeat)
{
    g_frame.on_char(hwnd, ch, repeat);
}

static void frame_on_get_min_max_info(HWND hwnd, LPMINMAXINFO info)
{
    g_frame.on_get_min_max_info(hwnd, info);
}

static void frame_on_timer(HWND window, UINT id)
{
    g_frame.on_timer(window, id);
}

static void frame_on_primary_button_down(HWND window, BOOL double_click, int x, int y, UINT key_flags)
{
    g_frame.on_primary_button_down(window, double_click, x, y, key_flags);
}

static void frame_on_secondary_button_down(HWND window, BOOL double_click, int x, int y, UINT key_flags)
{
    g_frame.on_secondary_button_down(window, double_click, x, y, key_flags);
}

static void frame_on_middle_button_down(HWND window, BOOL double_click, int x, int y, UINT key_flags)
{
    g_frame.on_middle_button_down(window, double_click, x, y, key_flags);
}

static void frame_on_primary_button_up(HWND window, int x, int y, UINT key_flags)
{
    g_frame.on_primary_button_up(window, x, y, key_flags);
}

static void frame_on_secondary_button_up(HWND window, int x, int y, UINT key_flags)
{
    g_frame.on_secondary_button_up(window, x, y, key_flags);
}

static void frame_on_middle_button_up(HWND window, int x, int y, UINT key_flags)
{
    g_frame.on_middle_button_up(window, x, y, key_flags);
}

static void frame_on_mouse_move(HWND window, int x, int y, UINT key_flags)
{
    g_frame.on_mouse_move(window,x, y, key_flags);
}

static LRESULT CALLBACK frame_window_proc(HWND window, UINT message, WPARAM wp, LPARAM lp)
{
    switch (message)
    {
    case WM_CLOSE:
        HANDLE_WM_CLOSE(window, wp, lp, frame_on_close);
        break;

    case WM_GETMINMAXINFO:
        HANDLE_WM_GETMINMAXINFO(window, wp, lp, frame_on_get_min_max_info);
        break;

    case WM_SETFOCUS:
        HANDLE_WM_SETFOCUS(window, wp, lp, frame_on_set_focus);
        break;

    case WM_KILLFOCUS:
        HANDLE_WM_KILLFOCUS(window, wp, lp, frame_on_kill_focus);
        break;

    case WM_PAINT:
        HANDLE_WM_PAINT(window, wp, lp, frame_on_paint);
        break;

    case WM_KEYDOWN:
        HANDLE_WM_KEYDOWN(window, wp, lp, frame_on_key_down);
        break;

    case WM_SYSKEYDOWN:
        HANDLE_WM_SYSKEYDOWN(window, wp, lp, frame_on_key_down);
        break;

    case WM_CHAR:
        HANDLE_WM_CHAR(window, wp, lp, frame_on_char);
        break;

    case WM_TIMER:
        HANDLE_WM_TIMER(window, wp, lp, frame_on_timer);
        break;

    case WM_LBUTTONUP:
        HANDLE_WM_LBUTTONUP(window, wp, lp, frame_on_primary_button_up);
        break;

    case WM_RBUTTONUP:
        HANDLE_WM_RBUTTONUP(window, wp, lp, frame_on_secondary_button_up);
        break;

    case WM_MBUTTONUP:
        HANDLE_WM_MBUTTONUP(window, wp, lp, frame_on_middle_button_up);
        break;

    case WM_MOUSEMOVE:
        HANDLE_WM_MOUSEMOVE(window, wp, lp, frame_on_mouse_move);
        break;

    case WM_LBUTTONDOWN:
        HANDLE_WM_LBUTTONDOWN(window, wp, lp, frame_on_primary_button_down);
        break;

    case WM_LBUTTONDBLCLK:
        HANDLE_WM_LBUTTONDBLCLK(window, wp, lp, frame_on_primary_button_down);
        break;

    case WM_RBUTTONDOWN:
        HANDLE_WM_RBUTTONDOWN(window, wp, lp, frame_on_secondary_button_down);
        break;

    case WM_RBUTTONDBLCLK:
        HANDLE_WM_RBUTTONDBLCLK(window, wp, lp, frame_on_secondary_button_down);
        break;

    case WM_MBUTTONDOWN:
        HANDLE_WM_MBUTTONDOWN(window, wp, lp, frame_on_middle_button_down);
        break;

    case WM_MBUTTONDBLCLK:
        HANDLE_WM_MBUTTONDBLCLK(window, wp, lp, frame_on_middle_button_down);
        break;

    default:
        return DefWindowProc(window, message, wp, lp);
    }
    return 0;
}

void Frame::init(HINSTANCE instance, LPCSTR title)
{
    LPCTSTR window_class = _T("IdFrame");
    WNDCLASS  wc;

    bool status = GetClassInfo(instance, window_class, &wc) != 0;
    if (!status)
    {
        m_instance = instance;
        m_title = title;

        wc.style = CS_DBLCLKS;
        wc.lpfnWndProc = frame_window_proc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = m_instance;
        wc.hIcon = LoadIcon(instance, MAKEINTRESOURCE(IDI_ITERATED_DYNAMICS));
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_BACKGROUND+1);
        wc.lpszMenuName = m_title.c_str();
        wc.lpszClassName = window_class;

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

static std::string key_flags_string(UINT value)
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

static int get_mouse_look_key()
{
    const int key{+g_look_at_mouse};
    assert(key < 0);
    return -key;
}

void Frame::on_mouse_move(HWND window, int x, int y, UINT key_flags)
{
    m_pos.x = static_cast<LONG>(x);
    m_pos.y = static_cast<LONG>(y);
    mouse_notify_move(x, y, static_cast<int>(key_flags));
    if (g_look_at_mouse <= MouseLook::IGNORE_MOUSE)
    {
        return;
    }

    if (m_last_tick == -1L)
    {
        m_last_tick = GetMessageTime();
        return;
    }

    // [button][dir]
    static constexpr int MOUSE_KEYS[4][4]{
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

void Frame::on_primary_button_down(HWND window, BOOL double_click, int x, int y, UINT key_flags)
{
    SetCapture(m_window);
    mouse_notify_primary_down(static_cast<bool>(double_click), x, y, static_cast<int>(key_flags));
    driver_debug_line((double_click ? "left down: (double)" : "left down: ") + std::to_string(x) + "," +
        std::to_string(y) + ", flags: " + key_flags_string(key_flags));
}

void Frame::on_secondary_button_down(HWND window, BOOL double_click, int x, int y, UINT key_flags)
{
    SetCapture(m_window);
    mouse_notify_secondary_down(static_cast<bool>(double_click), x, y, static_cast<int>(key_flags));
    driver_debug_line((double_click ? "right down: (double)" : "right down: ") + std::to_string(x) + "," +
        std::to_string(y) + ", flags: " + key_flags_string(key_flags));
}

void Frame::on_middle_button_down(HWND window, BOOL double_click, int x, int y, UINT key_flags)
{
    SetCapture(m_window);
    mouse_notify_middle_down(static_cast<bool>(double_click), x, y, static_cast<int>(key_flags));
    driver_debug_line((double_click ? "middle down: (double)" : "middle down: ") + std::to_string(x) + "," +
        std::to_string(y) + ", flags: " + key_flags_string(key_flags));
}

void Frame::on_primary_button_up(HWND window, int x, int y, UINT key_flags)
{
    ReleaseCapture();
    mouse_notify_primary_up(x, y, static_cast<int>(key_flags));
    if (+g_look_at_mouse < 0)
    {
        add_key_press(get_mouse_look_key());
    }
    else
    {
        driver_debug_line("left up: " + std::to_string(x) + "," + std::to_string(y) +
            ", flags: " + key_flags_string(key_flags));
    }
}

void Frame::on_secondary_button_up(HWND window, int x, int y, UINT key_flags)
{
    ReleaseCapture();
    mouse_notify_secondary_up(x, y, key_flags);
    driver_debug_line("right up: " + std::to_string(x) + "," + std::to_string(y) +
        ", flags: " + key_flags_string(key_flags));
}

void Frame::on_middle_button_up(HWND window, int x, int y, UINT key_flags)
{
    ReleaseCapture();
    mouse_notify_middle_up(x, y, static_cast<int>(key_flags));
    driver_debug_line("middle up: " + std::to_string(x) + "," + std::to_string(y) +
        ", flags: " + key_flags_string(key_flags));
}

void Frame::create_window(int width, int height)
{
    if (nullptr == m_window)
    {
        adjust_size(width, height);
        const POINT location{get_saved_frame_position()};
        m_window = CreateWindowA("IdFrame", m_title.c_str(), WS_OVERLAPPEDWINDOW, location.x,
            location.y, m_nc_width, m_nc_height, nullptr, nullptr, m_instance, nullptr);
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
        throw std::runtime_error("SetTimer failed: " + std::to_string(error));
    }
}
