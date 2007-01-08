#define WIN32_LEAN_AND_MEAN
#define STRICT
#include <windows.h>
#include <windowsx.h>

#include "frame.h"

Frame g_frame = { 0 };

static void frame_OnClose(HWND window)
{
}

static void frame_forward(UINT msg, WPARAM wp, LPARAM lp)
{
	if (g_frame.child)
	{
		OutputDebugString("frame_forward\n");
		PostMessage(g_frame.child, msg, wp, lp);
	}
}

static void frame_OnSetFocus(WPARAM wp, LPARAM lp)
{
	frame_forward(WM_SETFOCUS, wp, lp);
	g_frame.has_focus = TRUE;
	OutputDebugString("frame_OnSetFocus\n");
}

static void frame_OnKillFocus(WPARAM wp, LPARAM lp)
{
	frame_forward(WM_KILLFOCUS, wp, lp);
	g_frame.has_focus = FALSE;
	OutputDebugString("frame_OnKillFocus\n");
}

static void frame_OnPaint(HWND window)
{
    PAINTSTRUCT ps;
    HDC hDC = BeginPaint(window, &ps);
	EndPaint(window, &ps);
	OutputDebugString("frame_OnPaint\n");
}

static void frame_OnKeyDown(WPARAM wp, LPARAM lp)
{
	frame_forward(WM_KEYDOWN, wp, lp);
	OutputDebugString("frame_OnKeyDown\n");
}

static void frame_OnChar(WPARAM wp, LPARAM lp)
{
	frame_forward(WM_CHAR, wp, lp);
	OutputDebugString("frame_OnChar\n");
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
	case WM_SETFOCUS:		frame_OnSetFocus(wp, lp);						break;
	case WM_KILLFOCUS:		frame_OnKillFocus(wp, lp);						break;
	case WM_PAINT:			HANDLE_WM_PAINT(window, wp, lp, frame_OnPaint);	break;
	case WM_KEYDOWN:		frame_OnKeyDown(wp, lp);						break;
	case WM_SYSKEYDOWN:		frame_OnKeyDown(wp, lp);						break;
	case WM_CHAR:			frame_OnChar(wp, lp);							break;
	default:				return DefWindowProc(window, message, wp, lp);	break;
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

void frame_set_child(HWND child)
{
	g_frame.child = child;
	if (g_frame.has_focus)
	{
		SetFocus(g_frame.child);
	}
}
