// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#pragma once

#include <X11Driver/X11Connection.h>

#include <config/port.h>

#include <array>
#include <cstddef>
#include <string>

namespace id::misc
{

enum
{
    X11_TEXT_MAX_COL = 80,
    X11_TEXT_MAX_ROW = 25,
};

struct X11Screen
{
    char &chars(int row, int col)
    {
        return m_chars[row * X11_TEXT_MAX_COL + col];
    }

    const char &chars(int row, int col) const
    {
        return m_chars[row * X11_TEXT_MAX_COL + col];
    }

    Byte &attrs(int row, int col)
    {
        return m_attrs[row * X11_TEXT_MAX_COL + col];
    }

    const Byte &attrs(int row, int col) const
    {
        return m_attrs[row * X11_TEXT_MAX_COL + col];
    }

    static constexpr size_t SIZE{X11_TEXT_MAX_ROW * X11_TEXT_MAX_COL};
    std::array<char, SIZE> m_chars{};
    std::array<Byte, SIZE> m_attrs{};
};

class X11Text
{
public:
    X11Text();
    ~X11Text();

    void set_font_name(std::string font_name);
    bool init(X11Connection &connection);
    bool create(Window parent, int x, int y);
    void destroy();
    void show();
    void hide();
    void set_position(int x, int y);
    void put_string(int x_pos, int y_pos, int attr, const char *text, int &end_row, int &end_col);
    void scroll_up(int top, int bot);
    void set_attr(int row, int col, int attr, int count);
    void clear();
    X11Screen get_screen() const;
    void set_screen(const X11Screen &screen);
    int get_char_attr(int row, int col) const;
    void put_char_attr(int row, int col, int char_attr);
    void move_cursor(int row, int col);
    void hide_cursor();
    bool handle_event(const XEvent &event);
    Window window() const;
    bool is_mapped() const;
    int width() const;
    int height() const;

private:
    void load_colors();
    unsigned long color(int index) const;
    void invalidate(int left, int top, int right, int bottom);
    void paint_region(int pixel_x, int pixel_y, int pixel_width, int pixel_height);
    void paint_cells(int left, int top, int right, int bottom);
    void paint_cursor(int left, int top, int right, int bottom);
    int offset(int row, int col) const;
    bool is_valid_position(int row, int col) const;

    X11Connection *m_connection{};
    Window m_window{};
    GC m_gc{};
    XFontStruct *m_font{};
    std::string m_font_name;
    std::array<unsigned long, 16> m_colors{};
    int m_char_width{8};
    int m_char_ascent{10};
    int m_char_descent{3};
    int m_char_height{13};
    int m_cursor_row{};
    int m_cursor_col{};
    bool m_cursor_visible{};
    bool m_mapped{};
    X11Screen m_screen;
};

} // namespace id::misc
