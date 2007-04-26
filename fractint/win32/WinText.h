#if !defined(WINTEXT_H)
#define WINTEXT_H

#define KEYBUFMAX 80
#define WINTEXT_MAX_COL 80
#define WINTEXT_MAX_ROW 25

class WinText
{
public:
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

extern void			wintext_clear(WinText *);
extern void			wintext_cursor(WinText *, int, int, int);
extern void			wintext_destroy(WinText *);
extern BOOL			wintext_initialize(WinText *, HINSTANCE, HWND, LPCSTR);
extern void			wintext_paintscreen(WinText *, int, int, int, int);
extern void			wintext_putstring(WinText *, int, int, int, const char *, int *, int *);
extern void			wintext_scroll_up(WinText *, int top, int bot);
extern void			wintext_set_attr(WinText *, int row, int col, int attr, int count);
extern int			wintext_textoff(WinText *);
extern int			wintext_texton(WinText *);
extern BYTE *		wintext_screen_get(WinText *);
extern void			wintext_screen_set(WinText *, const BYTE *copy);
extern void			wintext_hide_cursor(WinText *);
extern void			wintext_schedule_alarm(WinText *, int delay);
extern int			wintext_get_char_attr(WinText *, int row, int col);
extern void			wintext_put_char_attr(WinText *, int row, int col, int char_attr);
extern void			wintext_set_focus(void);
extern void			wintext_kill_focus(void);
extern void			wintext_resume(WinText *me);

#endif
