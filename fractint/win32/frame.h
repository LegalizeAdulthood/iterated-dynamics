#if !defined(FRAME_H)
#define FRAME_H

#define KEYBUFMAX 80

#define BUTTON_LEFT 0
#define BUTTON_RIGHT 1
#define BUTTON_MIDDLE 2

class Frame
{
public:
	void init(HINSTANCE instance, LPCSTR title);
	void create(int width, int height);
	int key_pressed(void);
	int get_key_press(int option);
	int pump_messages(int waitflag);
	void schedule_alarm(int soon);
	void resize(int width, int height);
	void set_keyboard_timeout(int ms);

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

	static Frame *s_frame;
};

#endif
