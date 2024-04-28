#include "port.h"

#include "drivers.h"
#include "goodbye.h"
#include "id.h"
#include "id_keys.h"

#define WIN32_LEAN_AND_MEAN
#define STRICT
#include <tchar.h>
#include <Windows.h>
#include <windowsx.h>

#include "WinText.h"
#include "frame.h"
#include "resource.h"

#include <atlbase.h>
#include <tchar.h>

#include <stdexcept>
#include <string>

#define FRAME_TIMER_ID 2

Frame g_frame = { 0 };

constexpr const TCHAR *const LEFT_POS{_T("Left")};
constexpr const TCHAR *const TOP_POS{_T("Top")};
constexpr const TCHAR *const WINDOW_POS_KEY{_T("SOFTWARE\\" ID_VENDOR_NAME "\\" ID_PROGRAM_NAME "\\Settings\\Window")};

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
    return pos;
}

static void frame_OnClose(HWND window)
{
    save_frame_position(window);
    PostQuitMessage(0);
}

static void frame_OnSetFocus(HWND window, HWND old_focus)
{
    g_frame.m_has_focus = true;
}

static void frame_OnKillFocus(HWND window, HWND old_focus)
{
    g_frame.m_has_focus = false;
}

static void frame_OnPaint(HWND window)
{
    PAINTSTRUCT ps;
    HDC hDC = BeginPaint(window, &ps);
    EndPaint(window, &ps);
}

static void frame_add_key_press(unsigned int key)
{
    if (g_frame.m_key_press_count >= KEYBUFMAX)
    {
        _ASSERTE(g_frame.m_key_press_count < KEYBUFMAX);
        // no room
        return;
    }

    g_frame.m_key_press_buffer[g_frame.m_key_press_head] = key;
    if (++g_frame.m_key_press_head >= KEYBUFMAX)
    {
        g_frame.m_key_press_head = 0;
    }
    g_frame.m_key_press_count++;
}

static int mod_key(int modifier, int code, int fik, int *j)
{
    SHORT state = GetKeyState(modifier);
    if ((state & 0x8000) != 0)
    {
        if (j)
        {
            *j = 0;
        }
        return fik;
    }
    return 1000 + code;
}

#define ALT_KEY(fik_)       mod_key(VK_MENU, i, fik_, nullptr)
#define CTL_KEY(fik_)       mod_key(VK_CONTROL, i, fik_, nullptr)
#define CTL_KEY2(fik_, j_)  mod_key(VK_CONTROL, i, fik_, j_)
#define SHF_KEY(fik_)       mod_key(VK_SHIFT, i, fik_, nullptr)

static void frame_OnKeyDown(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
{
    // KEYUP, KEYDOWN, and CHAR msgs go to the 'key pressed' code
    // a key has been pressed - maybe ASCII, maybe not
    // if it's an ASCII key, 'WM_CHAR' will handle it
    unsigned int i;
    int j;
    i = MapVirtualKey(vk, 0);
    j = MapVirtualKey(vk, 2);

    // handle modifier keys on the non-WM_CHAR keys
    if (VK_F1 <= vk && vk <= VK_F10)
    {
        bool ctl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        bool alt = (GetKeyState(VK_MENU) & 0x8000) != 0;
        bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;

        if (shift)
        {
            i = SHF_KEY(ID_KEY_SF1 + (vk - VK_F1));
        }
        else if (ctl)
        {
            i = CTL_KEY(ID_KEY_CTL_F1 + (vk - VK_F1));
        }
        else if (alt)
        {
            i = ALT_KEY(ID_KEY_ALT_F1 + (vk - VK_F1));
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
            i = CTL_KEY(ID_KEY_CTL_DEL);
            break;
        case VK_DOWN:
            i = CTL_KEY(ID_KEY_CTL_DOWN_ARROW);
            break;
        case VK_END:
            i = CTL_KEY(ID_KEY_CTL_END);
            break;
        case VK_RETURN:
            i = CTL_KEY(ID_KEY_CTL_ENTER);
            break;
        case VK_HOME:
            i = CTL_KEY(ID_KEY_CTL_HOME);
            break;
        case VK_INSERT:
            i = CTL_KEY(ID_KEY_CTL_INSERT);
            break;
        case VK_LEFT:
            i = CTL_KEY(ID_KEY_CTL_LEFT_ARROW);
            break;
        case VK_PRIOR:
            i = CTL_KEY(ID_KEY_CTL_PAGE_UP);
            break;
        case VK_NEXT:
            i = CTL_KEY(ID_KEY_CTL_PAGE_DOWN);
            break;
        case VK_RIGHT:
            i = CTL_KEY(ID_KEY_CTL_RIGHT_ARROW);
            break;
        case VK_UP:
            i = CTL_KEY(ID_KEY_CTL_UP_ARROW);
            break;

        case VK_TAB:
            i = CTL_KEY2(ID_KEY_CTL_TAB, &j);
            break;
        case VK_ADD:
            i = CTL_KEY2(ID_KEY_CTL_PLUS, &j);
            break;
        case VK_SUBTRACT:
            i = CTL_KEY2(ID_KEY_CTL_MINUS, &j);
            break;

        default:
            if (0 == j)
            {
                i += 1000;
            }
            break;
        }
    }

    // use this call only for non-ASCII keys
    if (!(vk == VK_SHIFT || vk == VK_CONTROL || vk == VK_MENU) && (j == 0))
    {
        frame_add_key_press(i);
    }
}

static void frame_OnChar(HWND hwnd, TCHAR ch, int cRepeat)
{
    // KEYUP, KEYDOWN, and CHAR msgs go to the SG code
    // an ASCII key has been pressed
    unsigned int i, j, k;
    i = (unsigned int)((cRepeat & 0x00ff0000) >> 16);
    j = ch;
    k = (i << 8) + j;
    frame_add_key_press(k);
}

static void frame_OnGetMinMaxInfo(HWND hwnd, LPMINMAXINFO info)
{
    info->ptMaxSize.x = g_frame.m_nc_width;
    info->ptMaxSize.y = g_frame.m_nc_height;
    info->ptMaxTrackSize = info->ptMaxSize;
    info->ptMinTrackSize = info->ptMaxSize;
}

static void frame_OnTimer(HWND window, UINT id)
{
    _ASSERTE(g_frame.m_window == window);
    _ASSERTE(FRAME_TIMER_ID == id);
    g_frame.m_timed_out = true;
}

static LRESULT CALLBACK frame_proc(HWND window, UINT message, WPARAM wp, LPARAM lp)
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
        break;
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
        wc.lpfnWndProc = frame_proc;
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

int Frame::pump_messages(bool waitflag)
{
    MSG msg;
    bool quitting = false;
    m_timed_out = false;

    while (!quitting)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_NOREMOVE) == 0)
        {
            // no messages waiting
            if (!waitflag
                || m_key_press_count != 0
                || (waitflag && m_timed_out))
            {
                return m_key_press_count > 0 ? 1 : 0;
            }
        }

        {
            int result = GetMessage(&msg, nullptr, 0, 0);
            if (result > 0)
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
    }

    if (quitting)
    {
        goodbye();
    }

    return m_key_press_count == 0 ? 0 : 1;
}

int Frame::get_key_press(bool wait_for_key)
{
    pump_messages(wait_for_key != 0);
    if (wait_for_key && m_timed_out)
    {
        return 0;
    }

    if (m_key_press_count == 0)
    {
        _ASSERTE(wait_for_key == 0);
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
    g_frame.m_width = width;
    g_frame.m_nc_width = width + GetSystemMetrics(SM_CXFRAME)*2;
    g_frame.m_height = height;
    g_frame.m_nc_height = height +
                        GetSystemMetrics(SM_CYFRAME)*4 + GetSystemMetrics(SM_CYCAPTION) - 1;
}

void Frame::window(int width, int height)
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
    BOOL status = SetWindowPos(m_window, nullptr, 0, 0, m_nc_width, g_frame.m_nc_height, SWP_NOZORDER | SWP_NOMOVE);
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
