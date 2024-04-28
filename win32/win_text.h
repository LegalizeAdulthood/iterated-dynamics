#pragma once

#include <string>

#define KEYBUFMAX 80
#define WINTEXT_MAX_COL 80
#define WINTEXT_MAX_ROW 25

struct WinText
{
    int m_text_mode;
    bool m_alt_f4_hit;
    int m_showing_cursor;
    char m_chars[WINTEXT_MAX_ROW][WINTEXT_MAX_COL]; // Local copy of the screen characters and attributes
    unsigned char m_attrs[WINTEXT_MAX_ROW][WINTEXT_MAX_COL];
    bool m_buffer_init; // false if 'screen' is uninitialized
    HFONT m_font;
    int m_char_font;
    int m_char_width;
    int m_char_height;
    int m_char_xchars;
    int m_char_ychars;
    int m_max_width;
    int m_max_height;
    int m_cursor_x;
    int m_cursor_y;
    int m_cursor_type;
    bool m_cursor_owned;
    HBITMAP m_bitmap[3];
    short m_cursor_pattern[3][40];
    char m_title[128];
    HWND m_window;
    HWND m_parent;
    HINSTANCE m_instance;
};

void         wintext_clear(WinText *);
void         wintext_cursor(WinText *, int, int, int);
void         wintext_destroy(WinText *);
bool         wintext_initialize(WinText *, HINSTANCE, HWND, LPCSTR);
void         wintext_paintscreen(WinText *, int, int, int, int);
void         wintext_putstring(WinText *, int, int, int, char const *, int *, int *);
void         wintext_scroll_up(WinText *, int top, int bot);
void         wintext_set_attr(WinText *, int row, int col, int attr, int count);
int          wintext_textoff(WinText *);
int          wintext_texton(WinText *);
BYTE *       wintext_screen_get(WinText *);
void         wintext_screen_set(WinText *, const BYTE *copy);
void         wintext_hide_cursor(WinText *);
void         wintext_schedule_alarm(WinText *, int secs);
int          wintext_get_char_attr(WinText *, int row, int col);
void         wintext_put_char_attr(WinText *, int row, int col, int char_attr);
void         wintext_set_focus();
void         wintext_kill_focus();
void         wintext_resume(WinText *me);
