// SPDX-License-Identifier: GPL-3.0-only
//
#include "port.h"

#include "drivers.h"
#include "goodbye.h"
#include "id.h"
#include "id_keys.h"

#include "win_defines.h"
#include <tchar.h>
#include <Windows.h>
#include <windowsx.h>

#include "win_text.h"
#include "frame.h"
#include "resource.h"

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
    key.Create(HKEY_CURRENT_USER, WINDOW_POS_KEY);
    key.SetDWORDValue(LEFT_POS, rect.left);
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
        pos.y = get_value(TOP_POS);
    }
    else
    {
        pos.x = CW_USEDEFAULT;
        pos.y = CW_USEDEFAULT;
    }
    HMONITOR monitor{MonitorFromPoint(pos, MONITOR_DEFAULTTONULL)};
    if (monitor == nullptr)
    {
        forget_frame_position(key);
        return {};
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
    if (m_key_press_count >= KEYBUFMAX)
    {
        _ASSERTE(m_key_press_count < KEYBUFMAX);
        // no room
        return;
    }

    m_key_press_buffer[g_frame.m_key_press_head] = key;
    if (++m_key_press_head >= KEYBUFMAX)
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

    if (++m_key_press_tail >= KEYBUFMAX)
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
