// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#include "X11Text.h"

#include <algorithm>
#include <cassert>

namespace id::misc
{

namespace
{

constexpr Byte DEFAULT_ATTR{0xf0};
constexpr Byte SCROLL_ATTR{0x00};

} // namespace

X11Text::X11Text()
{
    clear();
}

void X11Text::put_string(const int x_pos, const int y_pos, const int attr, const char *text, int &end_row, int &end_col)
{
    assert(is_valid_position(y_pos, x_pos));
    const Byte text_attr{static_cast<Byte>(attr & 0xff)};
    int row{y_pos};
    int col{x_pos - 1};

    int i{};
    char ch{};
    for (i = 0; text != nullptr && (ch = text[i]) != 0; ++i)
    {
        if (ch == '\r' || ch == '\n')
        {
            if (row < X11_TEXT_MAX_ROW - 1)
            {
                ++row;
            }
            col = x_pos - 1;
        }
        else
        {
            if (++col >= X11_TEXT_MAX_COL)
            {
                if (row < X11_TEXT_MAX_ROW - 1)
                {
                    ++row;
                }
                col = x_pos;
            }
            if (is_valid_position(row, col))
            {
                m_screen.chars(row, col) = ch;
                m_screen.attrs(row, col) = text_attr;
            }
        }
    }
    if (i > 0)
    {
        end_row = row;
        end_col = col + 1;
    }
}

void X11Text::scroll_up(const int top, const int bot)
{
    assert(top >= 0);
    assert(top <= bot);
    assert(bot < X11_TEXT_MAX_ROW);

    for (int row = top; row < bot; ++row)
    {
        for (int col = 0; col < X11_TEXT_MAX_COL; ++col)
        {
            m_screen.chars(row, col) = m_screen.chars(row + 1, col);
            m_screen.attrs(row, col) = m_screen.attrs(row + 1, col);
        }
    }
    for (int col = 0; col < X11_TEXT_MAX_COL; ++col)
    {
        m_screen.chars(bot, col) = ' ';
        m_screen.attrs(bot, col) = SCROLL_ATTR;
    }
}

void X11Text::set_attr(const int row, const int col, const int attr, const int count)
{
    assert(is_valid_position(row, col));
    if (count <= 0)
    {
        return;
    }

    const int begin{offset(row, col)};
    const int end{std::min(begin + count, static_cast<int>(X11Screen::SIZE))};
    std::fill(m_screen.m_attrs.begin() + begin, m_screen.m_attrs.begin() + end, static_cast<Byte>(attr & 0xff));
}

void X11Text::clear()
{
    std::fill(m_screen.m_chars.begin(), m_screen.m_chars.end(), ' ');
    std::fill(m_screen.m_attrs.begin(), m_screen.m_attrs.end(), DEFAULT_ATTR);
}

X11Screen X11Text::get_screen() const
{
    return m_screen;
}

void X11Text::set_screen(const X11Screen &screen)
{
    m_screen = screen;
}

int X11Text::get_char_attr(const int row, const int col) const
{
    assert(is_valid_position(row, col));
    return (static_cast<unsigned char>(m_screen.chars(row, col)) << 8) | (m_screen.attrs(row, col) & 0xff);
}

void X11Text::put_char_attr(const int row, const int col, const int char_attr)
{
    assert(is_valid_position(row, col));
    m_screen.chars(row, col) = static_cast<char>(char_attr >> 8 & 0xff);
    m_screen.attrs(row, col) = static_cast<Byte>(char_attr & 0xff);
}

int X11Text::offset(const int row, const int col) const
{
    return row * X11_TEXT_MAX_COL + col;
}

bool X11Text::is_valid_position(const int row, const int col) const
{
    return row >= 0 && row < X11_TEXT_MAX_ROW && col >= 0 && col < X11_TEXT_MAX_COL;
}

} // namespace id::misc
