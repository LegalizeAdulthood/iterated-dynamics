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

class FrameImpl
{
public:
	FrameImpl();
	void init(HINSTANCE instance, LPCSTR title);
	void create(int width, int height);
	int get_key_press(int option);
	int pump_messages(int waitflag);
	void resize(int width, int height);
	void set_keyboard_timeout(int ms);
	void set_mouse_mode(int new_mode);
	int get_mouse_mode() const { return m_look_mouse; }

	HWND window() const	{ return m_window; }
	int width() const	{ return m_width; }
	int height() const	{ return m_height; }

	static LRESULT CALLBACK proc(HWND window, UINT message, WPARAM wp, LPARAM lp);

private:
	static void OnClose(HWND window);
	static void OnSetFocus(HWND window, HWND old_focus);
	static void OnKillFocus(HWND window, HWND old_focus);
	static void OnPaint(HWND window);
	int add_key_press(unsigned int key);
	static int mod_key(int modifier, int code, int fik, unsigned int *j);
	static void OnKeyDown(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags);
	static void OnChar(HWND hwnd, TCHAR ch, int cRepeat);
	static void OnGetMinMaxInfo(HWND hwnd, LPMINMAXINFO info);
	static void OnTimer(HWND window, UINT id);
	static void OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags);
	static void OnLeftButtonDown(HWND hwnd, BOOL doubleClick, int x, int y, UINT keyFlags);
	static void OnLeftButtonUp(HWND hwnd, int x, int y, UINT keyFlags);
	static void OnRightButtonDown(HWND hwnd, BOOL doubleClick, int x, int y, UINT keyFlags);
	static void OnRightButtonUp(HWND hwnd, int x, int y, UINT keyFlags);
	static void OnMiddleButtonDown(HWND hwnd, BOOL doubleClick, int x, int y, UINT keyFlags);
	static void OnMiddleButtonUp(HWND hwnd, int x, int y, UINT keyFlags);
	void adjust_size(int width, int height);

	HINSTANCE m_instance;
	HWND m_window;
	char m_title[80];
	int m_width;
	int m_height;
	int m_nc_width;
	int m_nc_height;
	HWND m_child;
	BOOL m_has_focus;
	BOOL m_timed_out;

	/* the keypress buffer */
	unsigned int m_keypress_count;
	unsigned int m_keypress_head;
	unsigned int m_keypress_tail;
	unsigned int m_keypress_buffer[KEYBUFMAX];

	/* mouse data */
	BOOL m_button_down[3];
	int m_start_x, m_start_y;
	int m_delta_x, m_delta_y;
	int m_look_mouse;

	static FrameImpl *s_frame;
};

FrameImpl *FrameImpl::s_frame = NULL;

FrameImpl::FrameImpl() :
	m_instance(0),
	m_window(0),
	m_width(0),
	m_height(0),
	m_nc_width(0),
	m_nc_height(0),
	m_child(0),
	m_has_focus(0),
	m_timed_out(0),
	m_keypress_count(0),
	m_keypress_head(0),
	m_keypress_tail(0),
	m_start_x(0),
	m_start_y(0),
	m_delta_x(0),
	m_delta_y(0),
	m_look_mouse(0)
{
	for (int i = 0; i < NUM_OF(m_title); i++)
	{
		m_title[i] = 0;
	}
	for (int i = 0; i < NUM_OF(m_button_down); i++)
	{
		m_button_down[i] = 0;
	}
	for (int i = 0; i < NUM_OF(m_keypress_buffer); i++)
	{
		m_keypress_buffer[i] = 0;
	}
}

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

void FrameImpl::OnClose(HWND window)
{
	PostQuitMessage(0);
}

void FrameImpl::OnSetFocus(HWND window, HWND old_focus)
{
	s_frame->m_has_focus = TRUE;
}

void FrameImpl::OnKillFocus(HWND window, HWND old_focus)
{
	s_frame->m_has_focus = FALSE;
}

void FrameImpl::OnPaint(HWND window)
{
	PAINTSTRUCT ps;
	HDC hDC = BeginPaint(window, &ps);
	EndPaint(window, &ps);
}

int FrameImpl::add_key_press(unsigned int key)
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

int FrameImpl::mod_key(int modifier, int code, int fik, unsigned int *j)
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

void FrameImpl::OnKeyDown(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
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

void FrameImpl::OnChar(HWND hwnd, TCHAR ch, int cRepeat)
{
	/* KEYUP, KEYDOWN, and CHAR msgs go to the SG code */
	/* an ASCII key has been pressed */
	unsigned int i, j, k;
	i = (unsigned int)((cRepeat & 0x00ff0000) >> 16);
	j = ch;
	k = (i << 8) + j;
	s_frame->add_key_press(k);
}

void FrameImpl::OnGetMinMaxInfo(HWND hwnd, LPMINMAXINFO info)
{
	info->ptMaxSize.x = s_frame->m_nc_width;
	info->ptMaxSize.y = s_frame->m_nc_height;
	info->ptMaxTrackSize = info->ptMaxSize;
	info->ptMinTrackSize = info->ptMaxSize;
}

void FrameImpl::OnTimer(HWND window, UINT id)
{
	_ASSERTE(s_frame->m_window == window);
	_ASSERTE(FRAME_TIMER_ID == id);
	s_frame->m_timed_out = TRUE;
	KillTimer(window, FRAME_TIMER_ID);
}

void FrameImpl::OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags)
{
	int key_index;

	return;

	/* if we're mouse snooping and there's a button down, then record delta movement */
	if (LOOK_MOUSE_NONE == s_frame->m_look_mouse)
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

void FrameImpl::OnLeftButtonDown(HWND hwnd, BOOL doubleClick, int x, int y, UINT keyFlags)
{
	s_frame->m_button_down[BUTTON_LEFT] = TRUE;
	if (doubleClick && (LOOK_MOUSE_NONE != s_frame->m_look_mouse))
	{
		s_frame->add_key_press(FIK_ENTER);
	}
}

void FrameImpl::OnLeftButtonUp(HWND hwnd, int x, int y, UINT keyFlags)
{
	s_frame->m_button_down[BUTTON_LEFT] = FALSE;
}

void FrameImpl::OnRightButtonDown(HWND hwnd, BOOL doubleClick, int x, int y, UINT keyFlags)
{
	s_frame->m_button_down[BUTTON_RIGHT] = TRUE;
	if (doubleClick && (LOOK_MOUSE_NONE != s_frame->m_look_mouse))
	{
		s_frame->add_key_press(FIK_CTL_ENTER);
	}
}

void FrameImpl::OnRightButtonUp(HWND hwnd, int x, int y, UINT keyFlags)
{
	s_frame->m_button_down[BUTTON_RIGHT] = FALSE;
}

void FrameImpl::OnMiddleButtonDown(HWND hwnd, BOOL doubleClick, int x, int y, UINT keyFlags)
{
	s_frame->m_button_down[BUTTON_MIDDLE] = TRUE;
}

void FrameImpl::OnMiddleButtonUp(HWND hwnd, int x, int y, UINT keyFlags)
{
	s_frame->m_button_down[BUTTON_MIDDLE] = FALSE;
}

LRESULT CALLBACK FrameImpl::proc(HWND window, UINT message, WPARAM wp, LPARAM lp)
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

void FrameImpl::init(HINSTANCE instance, LPCSTR title)
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

int FrameImpl::pump_messages(int waitflag)
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

void FrameImpl::set_mouse_mode(int new_mode)
{
	if (new_mode != m_look_mouse)
	{
		m_look_mouse = new_mode;
		m_delta_x = 0;
		m_delta_y = 0;
		m_start_x = -1;
		m_start_y = -1;
		m_button_down[BUTTON_LEFT] = FALSE;
		m_button_down[BUTTON_MIDDLE] = FALSE;
		m_button_down[BUTTON_RIGHT] = FALSE;
	}
}

int FrameImpl::get_key_press(int wait_for_key)
{
	int i;

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

void FrameImpl::adjust_size(int width, int height)
{
	m_width = width;
	m_nc_width = width + GetSystemMetrics(SM_CXFRAME)*2;
	m_height = height;
	m_nc_height = height +
		GetSystemMetrics(SM_CYFRAME)*2 + GetSystemMetrics(SM_CYCAPTION) - 1;
}

void FrameImpl::create(int width, int height)
{
	if (NULL == m_window)
	{
		s_frame = this;
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

void FrameImpl::resize(int width, int height)
{
	BOOL status;

	adjust_size(width, height);
	status = SetWindowPos(m_window, NULL,
		0, 0, m_nc_width, m_nc_height,
		SWP_NOZORDER | SWP_NOMOVE);
	_ASSERTE(status);

}

void FrameImpl::set_keyboard_timeout(int ms)
{
	UINT_PTR result = SetTimer(m_window, FRAME_TIMER_ID, ms, NULL);
	if (!result)
	{
		DWORD error = GetLastError();
		_ASSERTE(result == FRAME_TIMER_ID);
	}
}

FrameImpl Frame::s_impl;

void Frame::init(HINSTANCE instance, LPCTSTR title)
{
	s_impl.init(instance, title);
}

void Frame::create(int width, int height)
{
	s_impl.create(width, height);
}

int Frame::get_key_press(int option)
{
	return s_impl.get_key_press(option);
}

int Frame::pump_messages(int waitflag)
{
	return s_impl.pump_messages(waitflag);
}

void Frame::resize(int width, int height)
{
	s_impl.resize(width, height);
}

void Frame::set_keyboard_timeout(int ms)
{
	s_impl.set_keyboard_timeout(ms);
}

HWND Frame::window() const
{
	return s_impl.window();
}

int Frame::width() const
{
	return s_impl.width();
}

int Frame::height() const
{
	return s_impl.height();
}

LRESULT CALLBACK Frame::proc(HWND window, UINT message, WPARAM wp, LPARAM lp)
{
	return FrameImpl::proc(window, message, wp, lp);
}

void Frame::set_mouse_mode(int new_mode)
{
	s_impl.set_mouse_mode(new_mode);
}

int Frame::get_mouse_mode() const
{
	return s_impl.get_mouse_mode();
}
