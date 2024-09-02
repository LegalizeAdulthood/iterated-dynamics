#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#define KEYBUFMAX 80
#define WINTEXT_MAX_COL 80
#define WINTEXT_MAX_ROW 25

#include <vector>

struct Screen
{
    std::vector<BYTE> chars;
    std::vector<BYTE> attrs;
};

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
    Screen screen_get();
    void screen_set(const Screen &screen);
    void hide_cursor();
    void schedule_alarm(int secs);
    int get_char_attr(int row, int col);
    void put_char_attr(int row, int col, int char_attr);
    void resume();
    void set_parent(HWND parent)
    {
        m_parent = parent;
    }

    int m_text_mode{};
    bool m_alt_f4_hit{};
    int m_showing_cursor{};
    char m_chars[WINTEXT_MAX_ROW][WINTEXT_MAX_COL]{}; // Local copy of the screen characters and attributes
    unsigned char m_attrs[WINTEXT_MAX_ROW][WINTEXT_MAX_COL]{};
    bool m_buffer_init{}; // false if 'screen' is uninitialized
    HFONT m_font{};
    int m_char_font{};
    int m_char_width{};
    int m_char_height{};
    int m_char_xchars{};
    int m_char_ychars{};
    int m_max_width{};
    int m_max_height{};
    int m_cursor_x{};
    int m_cursor_y{};
    int m_cursor_type{};
    bool m_cursor_owned{};
    HBITMAP m_bitmap[3]{};
    short m_cursor_pattern[3][40]{};
    char m_title[128]{};
    HWND m_window{};

private:
    void invalidate(int left, int bot, int right, int top);

    HWND m_parent{};
    HINSTANCE m_instance{};
};
