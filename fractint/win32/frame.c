#define WIN32_LEAN_AND_MEAN
#define STRICT
#include <windows.h>
#include <windowsx.h>

#include "port.h"
#include "prototyp.h"
#include "fractint.h"
#include "wintext.h"
#include "frame.h"

Frame g_frame = { 0 };

static void frame_OnClose(HWND window)
{
}

static void frame_OnSetFocus(HWND window, HWND old_focus)
{
	g_frame.has_focus = TRUE;
}

static void frame_OnKillFocus(HWND window, HWND old_focus)
{
	g_frame.has_focus = FALSE;
}

static void frame_OnPaint(HWND window)
{
    PAINTSTRUCT ps;
    HDC hDC = BeginPaint(window, &ps);
	EndPaint(window, &ps);
	OutputDebugString("frame_OnPaint\n");
}

static void frame_add_key_press(unsigned int key)
{
	if (g_frame.keypress_count >= KEYBUFMAX)
	{
		_ASSERTE(g_frame.keypress_count < KEYBUFMAX);
		/* no room */
		return;
	}

	g_frame.keypress_buffer[g_frame.keypress_head] = key;
	if (++g_frame.keypress_head >= KEYBUFMAX)
	{
		g_frame.keypress_head = 0;
	}
	g_frame.keypress_count++;
}

static int mod_key(int modifier, int code, int fik)
{
	SHORT state = GetKeyState(modifier);
	if ((state & 0x8000) != 0)
	{
		return fik;
	}
	return 1000 + code;
}

#define ALT_KEY(fik_)				mod_key(VK_MENU, i, fik_)
#define CTL_KEY(fik_)				mod_key(VK_CONTROL, i, fik_)
#define SHF_KEY(fik_)				mod_key(VK_SHIFT, i, fik_)

static void frame_OnKeyDown(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
{
	/* KEYUP, KEYDOWN, and CHAR msgs go to the 'keypressed' code */
	/* a key has been pressed - maybe ASCII, maybe not */
	/* if it's an ASCII key, 'WM_CHAR' will handle it  */
	unsigned int i, j, k;
	i = MapVirtualKey(vk, 0);
	j = MapVirtualKey(vk, 2);
	k = (i << 8) + j;

	/* handle modifier keys on the non-WM_CHAR keys */
	switch (vk)
	{
	case VK_F1:			i = ALT_KEY(FIK_ALT_F1);		break;
	case VK_DELETE:		i = CTL_KEY(FIK_CTL_DEL);		break;
	case VK_END:		i = CTL_KEY(FIK_CTL_END);		break;
	case VK_RETURN:		i = CTL_KEY(FIK_CTL_ENTER);		break;
	case VK_HOME:		i = CTL_KEY(FIK_CTL_HOME);		break;
	case VK_INSERT:		i = CTL_KEY(FIK_CTL_INSERT);	break;
	case VK_SUBTRACT:	i = CTL_KEY(FIK_CTL_MINUS);		break;
	case VK_PRIOR:		i = CTL_KEY(FIK_CTL_PAGE_UP);	break;
	case VK_NEXT:		i = CTL_KEY(FIK_CTL_PAGE_DOWN);	break;

	case VK_TAB:
		if (0x8000 & GetKeyState(VK_CONTROL))
		{
			i = FIK_CTL_TAB;
			j = 0;
		}
		break;

	default:
		if (0 == j)
		{
			i += 1000;
		}
		break;
	}

	/* use this call only for non-ASCII keys */
	if (!(vk == VK_SHIFT || vk == VK_CONTROL) && (j == 0))
	{
		frame_add_key_press(i);
	}
}

static void frame_OnChar(HWND hwnd, TCHAR ch, int cRepeat)
{
	/* KEYUP, KEYDOWN, and CHAR msgs go to the SG code */
	/* an ASCII key has been pressed */
	unsigned int i, j, k;
	i = (unsigned int)((cRepeat & 0x00ff0000) >> 16);
	j = ch;
	k = (i << 8) + j;
	frame_add_key_press(k);
}

static void frame_OnGetMinMaxInfo(HWND hwnd, LPMINMAXINFO lpMinMaxInfo)
{
	lpMinMaxInfo->ptMaxSize.x = g_frame.width;
	lpMinMaxInfo->ptMaxSize.y = g_frame.height;
}

static LRESULT CALLBACK frame_proc(HWND window, UINT message, WPARAM wp, LPARAM lp)
{
	switch (message)
	{
	case WM_GETMINMAXINFO:	HANDLE_WM_GETMINMAXINFO(window, wp, lp, frame_OnGetMinMaxInfo); break;
	case WM_SETFOCUS:		HANDLE_WM_SETFOCUS(window, wp, lp, frame_OnSetFocus);			break;
	case WM_KILLFOCUS:		HANDLE_WM_KILLFOCUS(window, wp, lp, frame_OnKillFocus);			break;
	case WM_PAINT:			HANDLE_WM_PAINT(window, wp, lp, frame_OnPaint);					break;
	case WM_KEYDOWN:		HANDLE_WM_KEYDOWN(window, wp, lp, frame_OnKeyDown);				break;
	case WM_SYSKEYDOWN:		HANDLE_WM_SYSKEYDOWN(window, wp, lp, frame_OnKeyDown);			break;
	case WM_CHAR:			HANDLE_WM_CHAR(window, wp, lp, frame_OnChar);					break;
	default:				return DefWindowProc(window, message, wp, lp);					break;
	}
	return 0;
}

void frame_init(HINSTANCE instance, LPCSTR title)
{
    BOOL status;
	LPCSTR windowClass = "FractintFrame";
    WNDCLASS  wc;

	status = GetClassInfo(instance, windowClass, &wc);
	if (!status)
	{
		g_frame.instance = instance;
		strcpy(g_frame.title, title);

		wc.style = 0;
		wc.lpfnWndProc = frame_proc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = g_frame.instance;
		wc.hIcon = NULL;
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = GetStockObject(BLACK_BRUSH);
		wc.lpszMenuName = g_frame.title;
		wc.lpszClassName = windowClass;

		status = RegisterClass(&wc);
	}

	g_frame.keypress_count = 0;
    g_frame.keypress_head  = 0;
    g_frame.keypress_tail  = 0;
}

int frame_pump_messages(int waitflag)
{
	MSG msg;

	for (;;)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE) == 0)
		{
			/* no messages waiting */
			if (waitflag == 0 || g_frame.keypress_count != 0)
			{
				return (g_frame.keypress_count == 0) ? 0 : 1;
			}
		}

		if (GetMessage(&msg, NULL, 0, 0))
		{
			// translate accelerator here?
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return g_frame.keypress_count == 0 ? 0 : 1;
}

int frame_get_key_press(int option)
{
	int i;

	frame_pump_messages(option);

	if (g_frame.keypress_count == 0)
	{
		_ASSERTE(option == 0);
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

void frame_window(int width, int height)
{
	if (NULL == g_frame.window)
	{
		g_frame.width = width;
		g_frame.height = height;
		g_frame.window = CreateWindow("FractintFrame",
			g_frame.title,
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT,               /* default horizontal position */
			CW_USEDEFAULT,               /* default vertical position */
			g_frame.width,
			g_frame.height,
			NULL, NULL, g_frame.instance,
			NULL);
		ShowWindow(g_frame.window, SW_SHOWNORMAL);
	}
}
