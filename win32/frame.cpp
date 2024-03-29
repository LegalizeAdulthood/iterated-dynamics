#include "port.h"
#include "prototyp.h"

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

#define FRAME_TIMER_ID 2

Frame g_frame = { 0 };

static void frame_OnClose(HWND window)
{
    PostQuitMessage(0);
}

static void frame_OnSetFocus(HWND window, HWND old_focus)
{
    g_frame.has_focus = true;
}

static void frame_OnKillFocus(HWND window, HWND old_focus)
{
    g_frame.has_focus = false;
}

static void frame_OnPaint(HWND window)
{
    PAINTSTRUCT ps;
    HDC hDC = BeginPaint(window, &ps);
    EndPaint(window, &ps);
}

static void frame_add_key_press(unsigned int key)
{
    if (g_frame.keypress_count >= KEYBUFMAX)
    {
        _ASSERTE(g_frame.keypress_count < KEYBUFMAX);
        // no room
        return;
    }

    g_frame.keypress_buffer[g_frame.keypress_head] = key;
    if (++g_frame.keypress_head >= KEYBUFMAX)
    {
        g_frame.keypress_head = 0;
    }
    g_frame.keypress_count++;
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
    info->ptMaxSize.x = g_frame.nc_width;
    info->ptMaxSize.y = g_frame.nc_height;
    info->ptMaxTrackSize = info->ptMaxSize;
    info->ptMinTrackSize = info->ptMaxSize;
}

static void frame_OnTimer(HWND window, UINT id)
{
    _ASSERTE(g_frame.window == window);
    _ASSERTE(FRAME_TIMER_ID == id);
    g_frame.timed_out = true;
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

void frame_init(HINSTANCE instance, LPCSTR title)
{
    LPCTSTR windowClass = _T("FractintFrame");
    WNDCLASS  wc;

    bool status = GetClassInfo(instance, windowClass, &wc) != 0;
    if (!status)
    {
        g_frame.instance = instance;
        std::strcpy(g_frame.title, title);

        wc.style = 0;
        wc.lpfnWndProc = frame_proc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = g_frame.instance;
        wc.hIcon = nullptr;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_BACKGROUND+1);
        wc.lpszMenuName = g_frame.title;
        wc.lpszClassName = windowClass;

        status = RegisterClass(&wc) != 0;
    }
    _ASSERTE(status);

    g_frame.keypress_count = 0;
    g_frame.keypress_head  = 0;
    g_frame.keypress_tail  = 0;
}

int frame_pump_messages(bool waitflag)
{
    MSG msg;
    bool quitting = false;
    g_frame.timed_out = false;

    while (!quitting)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_NOREMOVE) == 0)
        {
            // no messages waiting
            if (!waitflag
                || (g_frame.keypress_count != 0)
                || (waitflag && g_frame.timed_out))
            {
                return (g_frame.keypress_count > 0) ? 1 : 0;
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

    return g_frame.keypress_count == 0 ? 0 : 1;
}

int frame_get_key_press(int wait_for_key)
{
    int i;

    frame_pump_messages(wait_for_key != 0);
    if (wait_for_key && g_frame.timed_out)
    {
        return 0;
    }

    if (g_frame.keypress_count == 0)
    {
        _ASSERTE(wait_for_key == 0);
        return 0;
    }

    i = g_frame.keypress_buffer[g_frame.keypress_tail];

    if (++g_frame.keypress_tail >= KEYBUFMAX)
    {
        g_frame.keypress_tail = 0;
    }
    g_frame.keypress_count--;

    return i;
}

static void frame_adjust_size(int width, int height)
{
    g_frame.width = width;
    g_frame.nc_width = width + GetSystemMetrics(SM_CXFRAME)*2;
    g_frame.height = height;
    g_frame.nc_height = height +
                        GetSystemMetrics(SM_CYFRAME)*4 + GetSystemMetrics(SM_CYCAPTION) - 1;
}

void frame_window(int width, int height)
{
    if (nullptr == g_frame.window)
    {
        frame_adjust_size(width, height);
        g_frame.window = CreateWindow("FractintFrame",
                                      g_frame.title,
                                      WS_OVERLAPPEDWINDOW,
                                      CW_USEDEFAULT,               // default horizontal position
                                      CW_USEDEFAULT,               // default vertical position
                                      g_frame.nc_width,
                                      g_frame.nc_height,
                                      nullptr, nullptr, g_frame.instance,
                                      nullptr);
        ShowWindow(g_frame.window, SW_SHOWNORMAL);
    }
    else
    {
        frame_resize(width, height);
    }
}

void frame_resize(int width, int height)
{
    frame_adjust_size(width, height);
    BOOL status = SetWindowPos(g_frame.window, nullptr,
                          0, 0, g_frame.nc_width, g_frame.nc_height,
                          SWP_NOZORDER | SWP_NOMOVE);
    _ASSERTE(status);
}

void frame_set_keyboard_timeout(int ms)
{
    UINT_PTR result = SetTimer(g_frame.window, FRAME_TIMER_ID, ms, nullptr);
    if (!result)
    {
        DWORD error = GetLastError();
        _ASSERTE(result);
    }
}
