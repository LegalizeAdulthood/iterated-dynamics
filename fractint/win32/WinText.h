#if !defined(WINTEXT_H)
#define WINTEXT_H

#define KEYBUFMAX 80
#define WINTEXT_MAX_COL 80
#define WINTEXT_MAX_ROW 25

class WinText
{
public:
	WinText();

	void clear();
	void cursor(int, int, int);
	void destroy();
	BOOL initialize(HINSTANCE, HWND, LPCSTR);
	void paintscreen(int, int, int, int);
	void putstring(int, int, int, const char *, int *, int *);
	void scroll_up(int top, int bot);
	void set_attr(int row, int col, int attr, int count);
	void create(HWND parent);
	BYTE *screen_get();
	void screen_set(const BYTE *copy);
	void hide_cursor();
	void schedule_alarm(int delay);
	int get_char_attr(int row, int col);
	void put_char_attr(int row, int col, int char_attr);
	void set_focus(void);
	void kill_focus(void);
	void resume();

	int max_width() const	{ return m_max_width; }
	int max_height() const	{ return m_max_height; }
	HWND window() const		{ return m_window; }

private:
	int textoff();
	int texton();
	void invalidate(int left, int bot, int right, int top);

	static LRESULT CALLBACK proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	static void OnClose(HWND window);
	static void OnSetFocus(HWND window, HWND old_focus);
	static void OnKillFocus(HWND window, HWND old_focus);
	static void OnPaint(HWND window);
	static void OnSize(HWND window, UINT state, int cx, int cy);
	static void OnGetMinMaxInfo(HWND hwnd, LPMINMAXINFO lpMinMaxInfo);
	static VOID CALLBACK timer_redraw(HWND window, UINT msg, UINT_PTR idEvent, DWORD dwTime);

	static LPCTSTR s_window_class;
	static bool s_showing_cursor;
	static WinText *s_me;

	int m_text_mode;
	int m_alt_f4_hit;
	int m_showing_cursor;

	/* Local copy of the "screen" characters and attributes */
	char m_chars[WINTEXT_MAX_ROW][WINTEXT_MAX_COL];
	char m_attrs[WINTEXT_MAX_ROW][WINTEXT_MAX_COL];
	int m_buffer_init;     /* zero if 'screen' is uninitialized */

	/* font information */

	HFONT m_font;
	int m_char_font;
	int m_char_width;
	int m_char_height;
	int m_char_xchars;
	int m_char_ychars;
	int m_max_width;
	int m_max_height;

	/* "cursor" variables (AKA the "caret" in Window-Speak) */
	int m_cursor_x;
	int m_cursor_y;
	int m_cursor_type;
	int m_cursor_owned;
	HBITMAP m_bitmap[3];
	short m_cursor_pattern[3][40];

	char m_title_text[128];			/* title-bar text */

	/* a few Windows variables we need to remember globally */

	HWND m_window;					/* a Global copy of hWnd */
	HWND m_parent_window;				/* a Global copy of hWnd's Parent */
	HINSTANCE m_instance;			/* a global copy of hInstance */
};

#endif
