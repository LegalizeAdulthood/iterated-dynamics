// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "win_defines.h"
#include <Windows.h>

#include <array>
#include <vector>

enum : size_t
{
    WINTEXT_MAX_COL = 80,
    WINTEXT_MAX_ROW = 25
};

struct Screen
{
    std::array<BYTE, WINTEXT_MAX_ROW * WINTEXT_MAX_COL> chars;
    std::array<BYTE, WINTEXT_MAX_ROW * WINTEXT_MAX_COL> attrs;
};

class WinText
{
public:
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
    Screen get_screen() const;
    void set_screen(const Screen &screen);
    void hide_cursor();
    void schedule_alarm(int secs);
    int get_char_attr(int row, int col);
    void put_char_attr(int row, int col, int char_attr);
    void resume();
    void set_parent(HWND parent)
    {
        m_parent = parent;
    }
    int get_max_width() const
    {
        return m_max_width;
    }
    int get_max_height() const
    {
        return m_max_height;
    }
    HWND get_window() const
    {
        return m_window;
    }

    // message handlers
    void on_close(HWND window);
    void on_set_focus(HWND window, HWND old_focus);
    void on_kill_focus(HWND window, HWND old_focus);
    void on_paint(HWND window);
    void on_size(HWND window, UINT state, int cx, int cy);
    void on_get_min_max_info(HWND hwnd, LPMINMAXINFO lpMinMaxInfo);

private:
    void invalidate(int left, int bot, int right, int top);

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
    HWND m_parent{};
    HINSTANCE m_instance{};
};
