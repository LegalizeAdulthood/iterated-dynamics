// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#pragma once

#include <config/port.h>

#include <array>
#include <cstddef>

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

    void put_string(int x_pos, int y_pos, int attr, const char *text, int &end_row, int &end_col);
    void scroll_up(int top, int bot);
    void set_attr(int row, int col, int attr, int count);
    void clear();
    X11Screen get_screen() const;
    void set_screen(const X11Screen &screen);
    int get_char_attr(int row, int col) const;
    void put_char_attr(int row, int col, int char_attr);

private:
    int offset(int row, int col) const;
    bool is_valid_position(int row, int col) const;

    X11Screen m_screen;
};

} // namespace id::misc
