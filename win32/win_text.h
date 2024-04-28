#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#define KEYBUFMAX 80
#define WINTEXT_MAX_COL 80
#define WINTEXT_MAX_ROW 25

struct WinText
{
    bool initialize(HINSTANCE instance, HWND parent, LPCSTR title);
    void destroy();
    int text_on();
    int text_off();
    void put_string(int xpos, int ypos, int attrib, char const *string, int *end_row, int *end_col);
    void scroll_up(int top, int bot);
    void paint_screen(int xmin, int xmax, int ymin, int ymax);
    void cursor(int xpos, int ypos, int cursor_type);
    void set_attr(int row, int col, int attr, int count);
    void clear();
    BYTE *screen_get();
    void screen_set(const BYTE *copy);
    void hide_cursor();
    void schedule_alarm(int secs);
    int get_char_attr(int row, int col);
    void put_char_attr(int row, int col, int char_attr);
    void resume();

    void invalidate(int left, int bot, int right, int top);

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

inline bool wintext_initialize(WinText *win_text, HINSTANCE instance, HWND parent , LPCSTR title)
{
    return win_text->initialize(instance, parent, title);
}

inline void wintext_destroy(WinText *win_text)
{
    win_text->destroy();
}

inline int wintext_texton(WinText *win_text)
{
    return win_text->text_on();
}

inline int wintext_textoff(WinText *win_text)
{
    return win_text->text_off();
}

inline void wintext_putstring(
    WinText *win_text, int xpos, int ypos, int attrib, char const *string, int *end_row, int *end_col)
{
    win_text->put_string(xpos, ypos, attrib, string, end_row, end_col);
}

inline void wintext_scroll_up(WinText *win_text, int top, int bot)
{
    win_text->scroll_up(top, bot);
}

inline void wintext_paintscreen(WinText *win_text, int xmin, int xmax, int ymin, int ymax)
{
    win_text->paint_screen(xmin, xmax, ymin, ymax);
}

inline void wintext_cursor(WinText *win_text, int xpos, int ypos, int cursor_type)
{
    win_text->cursor(xpos, ypos, cursor_type);
}

inline void wintext_set_attr(WinText *win_text, int row, int col, int attr, int count)
{
    win_text->set_attr(row, col, attr, count);
}

inline void wintext_clear(WinText *win_text)
{
    win_text->clear();
}

inline BYTE *wintext_screen_get(WinText *win_text)
{
    return win_text->screen_get();
}

inline void wintext_screen_set(WinText *win_text, const BYTE *copy)
{
    win_text->screen_set(copy);
}

inline void wintext_hide_cursor(WinText *win_text)
{
    win_text->hide_cursor();
}

inline void wintext_schedule_alarm(WinText *win_text, int secs)
{
    win_text->schedule_alarm(secs);
}

inline int wintext_get_char_attr(WinText *win_text, int row, int col)
{
    return win_text->get_char_attr(row, col);
}

inline void wintext_put_char_attr(WinText *win_text, int row, int col, int char_attr)
{
    win_text->put_char_attr(row, col, char_attr);
}

inline void wintext_resume(WinText *win_text)
{
    win_text->resume();
}
