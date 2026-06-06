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
constexpr const char *FONT_NAME{"fixed"};

struct RgbColor
{
    unsigned short red;
    unsigned short green;
    unsigned short blue;
};

constexpr unsigned short INTENSITY_128{0x8000};
constexpr unsigned short INTENSITY_192{0xc000};
constexpr unsigned short INTENSITY_255{0xffff};

constexpr RgbColor TEXT_COLORS[]{
    {0, 0, 0},
    {0, 0, INTENSITY_128},
    {0, INTENSITY_128, 0},
    {0, INTENSITY_128, INTENSITY_128},
    {INTENSITY_128, 0, 0},
    {INTENSITY_128, 0, INTENSITY_128},
    {INTENSITY_128, INTENSITY_128, 0},
    {INTENSITY_192, INTENSITY_192, INTENSITY_192},
    {0, 0, 0},
    {0, 0, INTENSITY_255},
    {0, INTENSITY_255, 0},
    {0, INTENSITY_255, INTENSITY_255},
    {INTENSITY_255, 0, 0},
    {INTENSITY_255, 0, INTENSITY_255},
    {INTENSITY_255, INTENSITY_255, 0},
    {INTENSITY_255, INTENSITY_255, INTENSITY_255},
};

} // namespace

X11Text::X11Text()
{
    clear();
}

X11Text::~X11Text()
{
    destroy();
}

bool X11Text::init(X11Connection &connection)
{
    if (!connection.is_open())
    {
        return false;
    }

    m_connection = &connection;
    Display *display = m_connection->display();
    if (m_font == nullptr)
    {
        m_font = XLoadQueryFont(display, FONT_NAME);
        if (m_font == nullptr)
        {
            return false;
        }

        const int text_width{XTextWidth(m_font, "W", 1)};
        m_char_width = std::max(text_width, m_font->max_bounds.width);
        m_char_width = std::max(m_char_width, 1);
        m_char_ascent = std::max(m_font->ascent, 1);
        m_char_descent = std::max(m_font->descent, 0);
        m_char_height = std::max(m_char_ascent + m_char_descent, 1);
    }
    load_colors();
    return true;
}

bool X11Text::create(const Window parent)
{
    if (m_connection == nullptr || !m_connection->is_open() || parent == None)
    {
        return false;
    }
    if (m_font == nullptr && !init(*m_connection))
    {
        return false;
    }
    if (m_window != None)
    {
        show();
        return true;
    }

    Display *display = m_connection->display();
    XSetWindowAttributes attributes{};
    attributes.background_pixel = color(0);
    attributes.border_pixel = color(0);
    attributes.colormap = m_connection->colormap();
    attributes.event_mask = ExposureMask | StructureNotifyMask | KeyPressMask;

    m_window = XCreateWindow(display, parent, 0, 0, width(), height(), 0, m_connection->depth(), InputOutput,
        m_connection->visual(), CWBackPixel | CWBorderPixel | CWColormap | CWEventMask, &attributes);
    if (m_window == None)
    {
        return false;
    }

    m_gc = XCreateGC(display, m_window, 0, nullptr);
    if (m_gc == nullptr)
    {
        XDestroyWindow(display, m_window);
        m_window = None;
        return false;
    }

    XSetFont(display, m_gc, m_font->fid);
    XMapWindow(display, m_window);
    XFlush(display);
    m_mapped = true;
    invalidate(0, 0, X11_TEXT_MAX_COL - 1, X11_TEXT_MAX_ROW - 1);
    return true;
}

void X11Text::destroy()
{
    if (m_connection == nullptr || !m_connection->is_open())
    {
        m_window = None;
        m_gc = nullptr;
        m_font = nullptr;
        m_mapped = false;
        return;
    }

    Display *display = m_connection->display();
    if (m_window != None)
    {
        XDestroyWindow(display, m_window);
        m_window = None;
    }
    if (m_gc != nullptr)
    {
        XFreeGC(display, m_gc);
        m_gc = nullptr;
    }
    if (m_font != nullptr)
    {
        XFreeFont(display, m_font);
        m_font = nullptr;
    }
    XFlush(display);
    m_mapped = false;
    m_cursor_visible = false;
}

void X11Text::show()
{
    if (m_connection == nullptr || !m_connection->is_open() || m_window == None)
    {
        return;
    }

    Display *display = m_connection->display();
    XMapRaised(display, m_window);
    m_mapped = true;
    invalidate(0, 0, X11_TEXT_MAX_COL - 1, X11_TEXT_MAX_ROW - 1);
}

void X11Text::hide()
{
    if (m_connection == nullptr || !m_connection->is_open() || m_window == None || !m_mapped)
    {
        return;
    }

    XUnmapWindow(m_connection->display(), m_window);
    XFlush(m_connection->display());
    m_mapped = false;
}

void X11Text::put_string(const int x_pos, const int y_pos, const int attr, const char *text, int &end_row, int &end_col)
{
    assert(is_valid_position(y_pos, x_pos));
    const Byte text_attr{static_cast<Byte>(attr & 0xff)};
    int row{y_pos};
    int col{x_pos - 1};
    int max_row{y_pos};
    int max_col{x_pos - 1};
    bool changed{};

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
                max_row = std::max(max_row, row);
                max_col = std::max(max_col, col);
                changed = true;
            }
        }
    }
    if (i > 0)
    {
        end_row = row;
        end_col = col + 1;
        if (changed)
        {
            invalidate(x_pos, y_pos, max_col, max_row);
        }
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
    invalidate(0, top, X11_TEXT_MAX_COL - 1, bot);
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
    if (end > begin)
    {
        const int last{end - 1};
        const int first_row{begin / X11_TEXT_MAX_COL};
        const int last_row{last / X11_TEXT_MAX_COL};
        if (first_row == last_row)
        {
            invalidate(begin % X11_TEXT_MAX_COL, first_row, last % X11_TEXT_MAX_COL, last_row);
        }
        else
        {
            invalidate(0, first_row, X11_TEXT_MAX_COL - 1, last_row);
        }
    }
}

void X11Text::clear()
{
    std::fill(m_screen.m_chars.begin(), m_screen.m_chars.end(), ' ');
    std::fill(m_screen.m_attrs.begin(), m_screen.m_attrs.end(), DEFAULT_ATTR);
    invalidate(0, 0, X11_TEXT_MAX_COL - 1, X11_TEXT_MAX_ROW - 1);
}

X11Screen X11Text::get_screen() const
{
    return m_screen;
}

void X11Text::set_screen(const X11Screen &screen)
{
    m_screen = screen;
    invalidate(0, 0, X11_TEXT_MAX_COL - 1, X11_TEXT_MAX_ROW - 1);
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
    invalidate(col, row, col, row);
}

void X11Text::move_cursor(const int row, const int col)
{
    if (m_cursor_visible)
    {
        invalidate(m_cursor_col, m_cursor_row, m_cursor_col, m_cursor_row);
    }

    m_cursor_row = row;
    m_cursor_col = col;
    m_cursor_visible = is_valid_position(row, col);
    if (m_cursor_visible)
    {
        invalidate(m_cursor_col, m_cursor_row, m_cursor_col, m_cursor_row);
    }
}

void X11Text::hide_cursor()
{
    if (!m_cursor_visible)
    {
        return;
    }

    m_cursor_visible = false;
    invalidate(m_cursor_col, m_cursor_row, m_cursor_col, m_cursor_row);
}

bool X11Text::handle_event(const XEvent &event)
{
    if (m_window == None)
    {
        return false;
    }
    if (event.type == Expose && event.xexpose.window == m_window)
    {
        paint_region(event.xexpose.x, event.xexpose.y, event.xexpose.width, event.xexpose.height);
        return true;
    }
    if (event.type == DestroyNotify && event.xdestroywindow.window == m_window)
    {
        m_window = None;
        m_mapped = false;
        return true;
    }
    if (event.type == MapNotify && event.xmap.window == m_window)
    {
        m_mapped = true;
        return true;
    }
    if (event.type == UnmapNotify && event.xunmap.window == m_window)
    {
        m_mapped = false;
        return true;
    }
    return false;
}

Window X11Text::window() const
{
    return m_window;
}

int X11Text::width() const
{
    return X11_TEXT_MAX_COL * m_char_width;
}

int X11Text::height() const
{
    return X11_TEXT_MAX_ROW * m_char_height;
}

void X11Text::load_colors()
{
    if (m_connection == nullptr || !m_connection->is_open())
    {
        return;
    }

    Display *display = m_connection->display();
    for (size_t i = 0; i < m_colors.size(); ++i)
    {
        XColor color{};
        color.red = TEXT_COLORS[i].red;
        color.green = TEXT_COLORS[i].green;
        color.blue = TEXT_COLORS[i].blue;
        color.flags = DoRed | DoGreen | DoBlue;
        if (XAllocColor(display, m_connection->colormap(), &color))
        {
            m_colors[i] = color.pixel;
        }
        else
        {
            m_colors[i] =
                i == 0 ? BlackPixel(display, m_connection->screen()) : WhitePixel(display, m_connection->screen());
        }
    }
}

unsigned long X11Text::color(const int index) const
{
    return m_colors[index & 0x0f];
}

void X11Text::invalidate(int left, int top, int right, int bottom)
{
    if (m_window == None || m_gc == nullptr || !m_mapped)
    {
        return;
    }

    left = std::max(left, 0);
    top = std::max(top, 0);
    right = std::min(right, X11_TEXT_MAX_COL - 1);
    bottom = std::min(bottom, X11_TEXT_MAX_ROW - 1);
    if (right < left || bottom < top)
    {
        return;
    }

    paint_cells(left, top, right, bottom);
    XFlush(m_connection->display());
}

void X11Text::paint_region(const int pixel_x, const int pixel_y, const int pixel_width, const int pixel_height)
{
    if (pixel_width <= 0 || pixel_height <= 0)
    {
        return;
    }

    const int left{pixel_x / m_char_width};
    const int top{pixel_y / m_char_height};
    const int right{(pixel_x + pixel_width - 1) / m_char_width};
    const int bottom{(pixel_y + pixel_height - 1) / m_char_height};
    paint_cells(left, top, right, bottom);
}

void X11Text::paint_cells(int left, int top, int right, int bottom)
{
    if (m_connection == nullptr || !m_connection->is_open() || m_window == None || m_gc == nullptr)
    {
        return;
    }

    left = std::max(left, 0);
    top = std::max(top, 0);
    right = std::min(right, X11_TEXT_MAX_COL - 1);
    bottom = std::min(bottom, X11_TEXT_MAX_ROW - 1);
    if (right < left || bottom < top)
    {
        return;
    }

    Display *display = m_connection->display();
    for (int row = top; row <= bottom; ++row)
    {
        int run_start{left};
        Byte attr{m_screen.attrs(row, left)};
        for (int col = left; col <= right + 1; ++col)
        {
            const bool end_run{col > right || m_screen.attrs(row, col) != attr};
            if (!end_run)
            {
                continue;
            }

            const int length{col - run_start};
            const int foreground{attr & 0x0f};
            const int background{attr >> 4 & 0x0f};
            XSetForeground(display, m_gc, color(background));
            XFillRectangle(display, m_window, m_gc, run_start * m_char_width, row * m_char_height,
                length * m_char_width, m_char_height);
            XSetForeground(display, m_gc, color(foreground));
            XDrawString(display, m_window, m_gc, run_start * m_char_width, row * m_char_height + m_char_ascent,
                &m_screen.chars(row, run_start), length);
            if (col <= right)
            {
                run_start = col;
                attr = m_screen.attrs(row, col);
            }
        }
    }

    paint_cursor(left, top, right, bottom);
}

void X11Text::paint_cursor(const int left, const int top, const int right, const int bottom)
{
    if (!m_cursor_visible || !is_valid_position(m_cursor_row, m_cursor_col) || m_cursor_col < left ||
        m_cursor_col > right || m_cursor_row < top || m_cursor_row > bottom)
    {
        return;
    }

    const Byte attr{m_screen.attrs(m_cursor_row, m_cursor_col)};
    const int foreground{attr & 0x0f};
    const int cursor_height{std::min(2, m_char_height)};
    XSetForeground(m_connection->display(), m_gc, color(foreground));
    XFillRectangle(m_connection->display(), m_window, m_gc, m_cursor_col * m_char_width,
        (m_cursor_row + 1) * m_char_height - cursor_height, m_char_width, cursor_height);
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
