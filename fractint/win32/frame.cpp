#define WIN32_LEAN_AND_MEAN
#define STRICT
#include <windows.h>
#include <windowsx.h>

#include "port.h"
#include "prototyp.h"
#include "fractint.h"
#include "drivers.h"

#include "wintext.h"
#include "frame.h"

#define FRAME_TIMER_ID 2

#define TextHSens 22
#define TextVSens 44
#define GraphSens 5
#define ZoomSens 20
#define TextVHLimit 6
#define GraphVHLimit 14
#define ZoomVHLimit 1
#define JitterMickeys 3

Frame *Frame::s_frame = NULL;

/*
; translate table for mouse movement -> fake keys
mousefkey dw   1077, 1075, 1080, 1072  ; right, left, down, up     just movement
		dw        0,   0, 1081, 1073  ;            , pgdn, pgup  + left button
		dw    1144, 1142, 1147, 1146  ; kpad+, kpad-, cdel, cins  + rt   button
		dw    1117, 1119, 1118, 1132  ; ctl-end, home, pgdn, pgup + mid/multi
*/
static int s_mouse_keys[16] =
{
	/* right			left			down				up */
	FIK_RIGHT_ARROW,	FIK_LEFT_ARROW,	FIK_DOWN_ARROW,		FIK_UP_ARROW,	/* no buttons */
	0,					0,				FIK_PAGE_DOWN,		FIK_PAGE_UP,	/* left button */
	FIK_CTL_PLUS,		FIK_CTL_MINUS,	FIK_CTL_DEL,		FIK_CTL_INSERT,	/* right button */
	FIK_CTL_END,		FIK_CTL_HOME,	FIK_CTL_PAGE_DOWN,	FIK_CTL_PAGE_UP	/* middle button */
};

void Frame::OnClose(HWND window)
{
	PostQuitMessage(0);
}

void Frame::OnSetFocus(HWND window, HWND old_focus)
{
	s_frame->m_has_focus = TRUE;
}

void Frame::OnKillFocus(HWND window, HWND old_focus)
{
	s_frame->m_has_focus = FALSE;
}

void Frame::OnPaint(HWND window)
{
	PAINTSTRUCT ps;
	HDC hDC = BeginPaint(window, &ps);
	EndPaint(window, &ps);
}

int Frame::add_key_press(unsigned int key)
{
	if (m_keypress_count >= KEYBUFMAX)
	{
		_ASSERTE(m_keypress_count < KEYBUFMAX);
		/* no room */
		return 1;
	}

	m_keypress_buffer[m_keypress_head] = key;
	if (++m_keypress_head >= KEYBUFMAX)
	{
		m_keypress_head = 0;
	}
	m_keypress_count++;
	return m_keypress_count == KEYBUFMAX;
}

int Frame::mod_key(int modifier, int code, int fik, unsigned int *j)
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

#define ALT_KEY(fik_)		mod_key(VK_MENU, i, fik_, NULL)
#define CTL_KEY(fik_)		mod_key(VK_CONTROL, i, fik_, NULL)
#define CTL_KEY2(fik_, j_)	mod_key(VK_CONTROL, i, fik_, j_)
#define SHF_KEY(fik_)		mod_key(VK_SHIFT, i, fik_, NULL)

void Frame::OnKeyDown(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
{
	/* KEYUP, KEYDOWN, and CHAR msgs go to the 'keypressed' code */
	/* a key has been pressed - maybe ASCII, maybe not */
	/* if it's an ASCII key, 'WM_CHAR' will handle it  */
	unsigned int i, j, k;
	i = MapVirtualKey(vk, 0);
	j = MapVirtualKey(vk, 2);
	k = (i << 8) + j;

	/* handle modifier keys on the non-WM_CHAR keys */
	if (VK_F1 <= vk && vk <= VK_F10)
	{
		BOOL ctl = GetKeyState(VK_CONTROL) & 0x8000;
		BOOL alt = GetKeyState(VK_MENU) & 0x8000;
		BOOL  shift = GetKeyState(VK_SHIFT) & 0x8000;

		if (shift)
		{
			i = SHF_KEY(FIK_SF1 + (vk - VK_F1));
		}
		else if (ctl)
		{
			i = CTL_KEY(FIK_CTL_F1 + (vk - VK_F1));
		}
		else if (alt)
		{
			i = ALT_KEY(FIK_ALT_F1 + (vk - VK_F1));
		}
		else
		{
			i = FIK_F1 + vk - VK_F1;
		}
	}
	else
	{
		switch (vk)
		{
		/* sorted in FIK_xxx order */
		case VK_DELETE:		i = CTL_KEY(FIK_CTL_DEL);			break;
		case VK_DOWN:		i = CTL_KEY(FIK_CTL_DOWN_ARROW);	break;
		case VK_END:		i = CTL_KEY(FIK_CTL_END);			break;
		case VK_RETURN:		i = CTL_KEY(FIK_CTL_ENTER);			break;
		case VK_HOME:		i = CTL_KEY(FIK_CTL_HOME);			break;
		case VK_INSERT:		i = CTL_KEY(FIK_CTL_INSERT);		break;
		case VK_LEFT:		i = CTL_KEY(FIK_CTL_LEFT_ARROW);	break;
		case VK_PRIOR:		i = CTL_KEY(FIK_CTL_PAGE_UP);		break;
		case VK_NEXT:		i = CTL_KEY(FIK_CTL_PAGE_DOWN);		break;
		case VK_RIGHT:		i = CTL_KEY(FIK_CTL_RIGHT_ARROW);	break;
		case VK_UP:			i = CTL_KEY(FIK_CTL_UP_ARROW);		break;

		case VK_TAB:		i = CTL_KEY2(FIK_CTL_TAB, &j);		break;
		case VK_ADD:		i = CTL_KEY2(FIK_CTL_PLUS, &j);		break;
		case VK_SUBTRACT:	i = CTL_KEY2(FIK_CTL_MINUS, &j);	break;

		default:
			if (0 == j)
			{
				i += 1000;
			}
			break;
		}
	}

	/* use this call only for non-ASCII keys */
	if (!(vk == VK_SHIFT || vk == VK_CONTROL || vk == VK_MENU) && (j == 0))
	{
		s_frame->add_key_press(i);
	}
}

void Frame::OnChar(HWND hwnd, TCHAR ch, int cRepeat)
{
	/* KEYUP, KEYDOWN, and CHAR msgs go to the SG code */
	/* an ASCII key has been pressed */
	unsigned int i, j, k;
	i = (unsigned int)((cRepeat & 0x00ff0000) >> 16);
	j = ch;
	k = (i << 8) + j;
	s_frame->add_key_press(k);
}

void Frame::OnGetMinMaxInfo(HWND hwnd, LPMINMAXINFO info)
{
	info->ptMaxSize.x = s_frame->m_nc_width;
	info->ptMaxSize.y = s_frame->m_nc_height;
	info->ptMaxTrackSize = info->ptMaxSize;
	info->ptMinTrackSize = info->ptMaxSize;
}

void Frame::OnTimer(HWND window, UINT id)
{
	_ASSERTE(s_frame->m_window == window);
	_ASSERTE(FRAME_TIMER_ID == id);
	s_frame->m_timed_out = TRUE;
	KillTimer(window, FRAME_TIMER_ID);
}

void Frame::OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags)
{
	int key_index;

	/* if we're mouse snooping and there's a button down, then record delta movement */
	if (LOOK_MOUSE_NONE == g_look_at_mouse)
	{
		return;
	}

	s_frame->m_delta_x = x - s_frame->m_start_x;
	s_frame->m_delta_y = y - s_frame->m_start_y;

	/* ignore small movements */
	if ((abs(s_frame->m_delta_x) > (GraphSens + JitterMickeys))
			|| (abs(s_frame->m_delta_y) > (GraphSens + JitterMickeys)))
	{
		s_frame->m_start_x = x;
		s_frame->m_start_y = y;
		if (abs(s_frame->m_delta_x) > abs(s_frame->m_delta_y))
		{
			/* x-axis changes more */
			key_index = (s_frame->m_delta_x > 0) ? 0 : 1;
		}
		else
		{
			/* y-axis changes more */
			key_index = (s_frame->m_delta_y > 0) ? 2 : 3;
		}

		/* synthesize keystroke */
		if (s_frame->m_button_down[BUTTON_LEFT])
		{
			key_index += 4;
		}
		else if (s_frame->m_button_down[BUTTON_RIGHT])
		{
			key_index += 8;
		}
		else if (s_frame->m_button_down[BUTTON_MIDDLE])
		{
			key_index += 12;
		}
		else
		{
			/* no buttons down */

		}
		s_frame->add_key_press(s_mouse_keys[key_index]);
	}
}

void Frame::OnLeftButtonDown(HWND hwnd, BOOL doubleClick, int x, int y, UINT keyFlags)
{
	s_frame->m_button_down[BUTTON_LEFT] = TRUE;
	if (doubleClick && (LOOK_MOUSE_NONE != g_look_at_mouse))
	{
		s_frame->add_key_press(FIK_ENTER);
	}
}

void Frame::OnLeftButtonUp(HWND hwnd, int x, int y, UINT keyFlags)
{
	s_frame->m_button_down[BUTTON_LEFT] = FALSE;
}

void Frame::OnRightButtonDown(HWND hwnd, BOOL doubleClick, int x, int y, UINT keyFlags)
{
	s_frame->m_button_down[BUTTON_RIGHT] = TRUE;
	if (doubleClick && (LOOK_MOUSE_NONE != g_look_at_mouse))
	{
		s_frame->add_key_press(FIK_CTL_ENTER);
	}
}

void Frame::OnRightButtonUp(HWND hwnd, int x, int y, UINT keyFlags)
{
	s_frame->m_button_down[BUTTON_RIGHT] = FALSE;
}

void Frame::OnMiddleButtonDown(HWND hwnd, BOOL doubleClick, int x, int y, UINT keyFlags)
{
	s_frame->m_button_down[BUTTON_MIDDLE] = TRUE;
}

void Frame::OnMiddleButtonUp(HWND hwnd, int x, int y, UINT keyFlags)
{
	s_frame->m_button_down[BUTTON_MIDDLE] = FALSE;
}

LRESULT CALLBACK Frame::proc(HWND window, UINT message, WPARAM wp, LPARAM lp)
{
	switch (message)
	{
	case WM_CLOSE:			HANDLE_WM_CLOSE(window, wp, lp, OnClose);					break;
	case WM_GETMINMAXINFO:	HANDLE_WM_GETMINMAXINFO(window, wp, lp, OnGetMinMaxInfo); 	break;
	case WM_SETFOCUS:		HANDLE_WM_SETFOCUS(window, wp, lp, OnSetFocus);				break;
	case WM_KILLFOCUS:		HANDLE_WM_KILLFOCUS(window, wp, lp, OnKillFocus);			break;
	case WM_PAINT:			HANDLE_WM_PAINT(window, wp, lp, OnPaint);					break;
	case WM_KEYDOWN:		HANDLE_WM_KEYDOWN(window, wp, lp, OnKeyDown);				break;
	case WM_SYSKEYDOWN:		HANDLE_WM_SYSKEYDOWN(window, wp, lp, OnKeyDown);			break;
	case WM_CHAR:			HANDLE_WM_CHAR(window, wp, lp, OnChar);						break;
	case WM_TIMER:			HANDLE_WM_TIMER(window, wp, lp, OnTimer);					break;
	case WM_MOUSEMOVE:		HANDLE_WM_MOUSEMOVE(window, wp, lp, OnMouseMove);			break;
	case WM_LBUTTONDOWN:	HANDLE_WM_LBUTTONDOWN(window, wp, lp, OnLeftButtonDown);	break;
	case WM_LBUTTONUP:		HANDLE_WM_LBUTTONUP(window, wp, lp, OnLeftButtonUp);		break;
	case WM_LBUTTONDBLCLK:	HANDLE_WM_LBUTTONDBLCLK(window, wp, lp, OnLeftButtonDown);	break;
	case WM_MBUTTONDOWN:	HANDLE_WM_MBUTTONDOWN(window, wp, lp, OnMiddleButtonDown); 	break;
	case WM_MBUTTONUP:		HANDLE_WM_MBUTTONUP(window, wp, lp, OnMiddleButtonUp);		break;
	case WM_RBUTTONDOWN:	HANDLE_WM_RBUTTONDOWN(window, wp, lp, OnRightButtonDown);	break;
	case WM_RBUTTONDBLCLK:	HANDLE_WM_RBUTTONDBLCLK(window, wp, lp, OnRightButtonDown);	break;
	case WM_RBUTTONUP:		HANDLE_WM_RBUTTONUP(window, wp, lp, OnRightButtonUp);		break;
	default:				return DefWindowProc(window, message, wp, lp);				break;
	}
	return 0;
}

void Frame::init(HINSTANCE instance, LPCSTR title)
{
	BOOL status;
	LPCSTR windowClass = "FractintFrame";
	WNDCLASS  wc;

	status = GetClassInfo(instance, windowClass, &wc);
	if (!status)
	{
		m_instance = instance;
		strcpy(m_title, title);

		wc.style = CS_DBLCLKS;
		wc.lpfnWndProc = proc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = m_instance;
		wc.hIcon = NULL;
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH) (COLOR_BACKGROUND + 1);
		wc.lpszMenuName = m_title;
		wc.lpszClassName = windowClass;

		status = RegisterClass(&wc);
	}

	m_keypress_count = 0;
	m_keypress_head  = 0;
	m_keypress_tail  = 0;
}

int Frame::pump_messages(int waitflag)
{
	MSG msg;
	BOOL quitting = FALSE;
	m_timed_out = FALSE;

	while (!quitting)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE) == 0)
		{
			/* no messages waiting */
			if (!waitflag
				|| (m_keypress_count != 0)
				|| (waitflag && m_timed_out))
			{
				return (m_keypress_count > 0) ? 1 : 0;
			}
		}

		{
			int result = GetMessage(&msg, NULL, 0, 0);
			if (result > 0)
			{
				// translate accelerator here?
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else if (0 == result)
			{
				quitting = TRUE;
			}
		}
	}

	if (quitting)
	{
		goodbye();
	}

	return m_keypress_count == 0 ? 0 : 1;
}

int Frame::get_key_press(int wait_for_key)
{
	int i;

	if (g_look_at_mouse != m_look_mouse)
	{
		m_look_mouse = g_look_at_mouse;
		m_delta_x = 0;
		m_delta_y = 0;
		m_start_x = -1;
		m_start_y = -1;
		m_button_down[BUTTON_LEFT] = FALSE;
		m_button_down[BUTTON_MIDDLE] = FALSE;
		m_button_down[BUTTON_RIGHT] = FALSE;
	}

	pump_messages(wait_for_key);
	if (wait_for_key && m_timed_out)
	{
		return 0;
	}

	if (m_keypress_count == 0)
	{
		_ASSERTE(wait_for_key == 0);
		return 0;
	}

	i = m_keypress_buffer[m_keypress_tail];

	if (++m_keypress_tail >= KEYBUFMAX)
	{
		m_keypress_tail = 0;
	}
	m_keypress_count--;
	return i;
}

void Frame::adjust_size(int width, int height)
{
	m_width = width;
	m_nc_width = width + GetSystemMetrics(SM_CXFRAME)*2;
	m_height = height;
	m_nc_height = height +
		GetSystemMetrics(SM_CYFRAME)*2 + GetSystemMetrics(SM_CYCAPTION) - 1;
}

void Frame::create(int width, int height)
{
	if (NULL == m_window)
	{
		adjust_size(width, height);
		m_window = CreateWindow("FractintFrame",
			m_title,
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT,               /* default horizontal position */
			CW_USEDEFAULT,               /* default vertical position */
			m_nc_width,
			m_nc_height,
			NULL, NULL, m_instance,
			NULL);
		ShowWindow(m_window, SW_SHOWNORMAL);
	}
	else
	{
		resize(width, height);
	}
}

void Frame::resize(int width, int height)
{
	BOOL status;

	adjust_size(width, height);
	status = SetWindowPos(m_window, NULL,
		0, 0, m_nc_width, m_nc_height,
		SWP_NOZORDER | SWP_NOMOVE);
	_ASSERTE(status);

}

void Frame::set_keyboard_timeout(int ms)
{
	UINT_PTR result = SetTimer(m_window, FRAME_TIMER_ID, ms, NULL);
	if (!result)
	{
		DWORD error = GetLastError();
		_ASSERTE(result == FRAME_TIMER_ID);
	}
}
