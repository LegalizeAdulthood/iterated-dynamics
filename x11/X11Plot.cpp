// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#include "X11Plot.h"

#include "engine/spindac.h"

#include <X11/Xutil.h>

#include <algorithm>
#include <cassert>
#include <limits>

using namespace id::engine;

namespace id::misc
{

namespace
{

int count_bits(unsigned long mask)
{
    int result{};
    while (mask != 0)
    {
        if ((mask & 1UL) != 0)
        {
            ++result;
        }
        mask >>= 1;
    }
    return result;
}

int first_bit(unsigned long mask)
{
    int result{};
    while (mask != 0 && (mask & 1UL) == 0)
    {
        ++result;
        mask >>= 1;
    }
    return result;
}

X11ColorMask make_color_mask(const unsigned long mask)
{
    return X11ColorMask{mask, first_bit(mask), count_bits(mask)};
}

struct PlotBufferSize
{
    std::size_t row_len{};
    std::size_t pixels_len{};
};

bool get_plot_buffer_size(const int width, const int height, PlotBufferSize &result)
{
    if (width <= 0 || height <= 0)
    {
        return false;
    }

    const std::size_t row_len{static_cast<std::size_t>(width)};
    const std::size_t rows{static_cast<std::size_t>(height)};
    if (row_len > std::numeric_limits<std::size_t>::max() / rows)
    {
        return false;
    }

    result = {row_len, row_len * rows};
    return true;
}

unsigned long scale_component(const Byte value, const X11ColorMask mask)
{
    if (mask.bits == 0)
    {
        return 0;
    }

    const unsigned long max_value{(1UL << mask.bits) - 1};
    const unsigned long scaled{(static_cast<unsigned long>(value) * max_value + 127) / 255};
    return (scaled << mask.shift) & mask.mask;
}

const Byte FONT_8x8[8][1024 / 8]{
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x14, 0x14, 0x08,
        0x00, 0x18, 0x08, 0x04, 0x10, 0x08, 0x08, 0x00, 0x00, 0x00, 0x01, 0x1C, 0x08, 0x1C, 0x1C, 0x0C, 0x3E, 0x1C,
        0x3E, 0x1C, 0x1C, 0x00, 0x00, 0x04, 0x00, 0x20, 0x1C, 0x1C, 0x1C, 0x3C, 0x1C, 0x3C, 0x3E, 0x3E, 0x1C, 0x22,
        0x1C, 0x0E, 0x22, 0x10, 0x41, 0x22, 0x1C, 0x1C, 0x1C, 0x3C, 0x1C, 0x3E, 0x22, 0x22, 0x41, 0x22, 0x22, 0x3E,
        0x1C, 0x40, 0x1C, 0x08, 0x00, 0x10, 0x00, 0x10, 0x00, 0x02, 0x00, 0x0C, 0x00, 0x20, 0x00, 0x00, 0x20, 0x18,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x08, 0x30, 0x00,
        0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x14, 0x14, 0x1E,
        0x32, 0x28, 0x08, 0x08, 0x08, 0x49, 0x08, 0x00, 0x00, 0x00, 0x02, 0x22, 0x18, 0x22, 0x22, 0x14, 0x20, 0x22,
        0x02, 0x22, 0x22, 0x0C, 0x0C, 0x08, 0x00, 0x10, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x20, 0x20, 0x22, 0x22,
        0x08, 0x04, 0x22, 0x10, 0x63, 0x32, 0x22, 0x12, 0x22, 0x22, 0x22, 0x08, 0x22, 0x22, 0x41, 0x22, 0x22, 0x02,
        0x10, 0x20, 0x04, 0x14, 0x00, 0x08, 0x1C, 0x10, 0x00, 0x02, 0x00, 0x12, 0x00, 0x20, 0x08, 0x08, 0x20, 0x08,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x08, 0x08, 0x00,
        0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x7F, 0x28,
        0x34, 0x10, 0x00, 0x10, 0x04, 0x2A, 0x08, 0x00, 0x00, 0x00, 0x04, 0x22, 0x08, 0x02, 0x02, 0x24, 0x20, 0x20,
        0x04, 0x22, 0x22, 0x0C, 0x0C, 0x10, 0x7F, 0x08, 0x02, 0x2E, 0x22, 0x22, 0x20, 0x22, 0x20, 0x20, 0x20, 0x22,
        0x08, 0x04, 0x24, 0x10, 0x55, 0x2A, 0x22, 0x12, 0x22, 0x22, 0x20, 0x08, 0x22, 0x22, 0x41, 0x14, 0x14, 0x04,
        0x10, 0x10, 0x04, 0x22, 0x00, 0x00, 0x02, 0x1C, 0x1C, 0x0E, 0x1C, 0x10, 0x1D, 0x2C, 0x00, 0x00, 0x24, 0x08,
        0xB6, 0x2C, 0x1C, 0x2C, 0x1A, 0x2C, 0x1C, 0x1C, 0x24, 0x22, 0x41, 0x22, 0x12, 0x3C, 0x10, 0x08, 0x08, 0x30,
        0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x14, 0x1C,
        0x08, 0x28, 0x00, 0x10, 0x04, 0x1C, 0x7F, 0x00, 0x7F, 0x00, 0x08, 0x2A, 0x08, 0x04, 0x0C, 0x3E, 0x3C, 0x3C,
        0x08, 0x1C, 0x1E, 0x00, 0x00, 0x20, 0x00, 0x04, 0x04, 0x2A, 0x3E, 0x3C, 0x20, 0x22, 0x3C, 0x3E, 0x2E, 0x3E,
        0x08, 0x04, 0x38, 0x10, 0x49, 0x2A, 0x22, 0x1C, 0x22, 0x3C, 0x1C, 0x08, 0x22, 0x14, 0x2A, 0x08, 0x08, 0x08,
        0x10, 0x08, 0x04, 0x00, 0x00, 0x00, 0x1E, 0x12, 0x20, 0x12, 0x22, 0x38, 0x22, 0x32, 0x08, 0x08, 0x28, 0x08,
        0x49, 0x12, 0x22, 0x12, 0x24, 0x30, 0x20, 0x08, 0x24, 0x22, 0x41, 0x14, 0x12, 0x04, 0x20, 0x08, 0x04, 0x49,
        0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x7F, 0x0A,
        0x16, 0x46, 0x00, 0x10, 0x04, 0x2A, 0x08, 0x0C, 0x00, 0x00, 0x10, 0x22, 0x08, 0x08, 0x02, 0x04, 0x02, 0x22,
        0x10, 0x22, 0x02, 0x0C, 0x0C, 0x10, 0x7F, 0x08, 0x08, 0x2E, 0x22, 0x22, 0x20, 0x22, 0x20, 0x20, 0x22, 0x22,
        0x08, 0x24, 0x24, 0x10, 0x41, 0x26, 0x22, 0x10, 0x22, 0x28, 0x02, 0x08, 0x22, 0x14, 0x2A, 0x14, 0x08, 0x10,
        0x10, 0x04, 0x04, 0x00, 0x00, 0x00, 0x22, 0x12, 0x20, 0x12, 0x3E, 0x10, 0x22, 0x22, 0x08, 0x08, 0x30, 0x08,
        0x49, 0x12, 0x22, 0x12, 0x24, 0x20, 0x18, 0x08, 0x24, 0x22, 0x49, 0x08, 0x12, 0x08, 0x10, 0x08, 0x08, 0x06,
        0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x3C,
        0x26, 0x44, 0x00, 0x08, 0x08, 0x49, 0x08, 0x0C, 0x00, 0x0C, 0x20, 0x22, 0x08, 0x10, 0x22, 0x04, 0x22, 0x22,
        0x10, 0x22, 0x22, 0x0C, 0x0C, 0x08, 0x00, 0x10, 0x00, 0x20, 0x22, 0x22, 0x22, 0x22, 0x20, 0x20, 0x22, 0x22,
        0x08, 0x24, 0x22, 0x10, 0x41, 0x22, 0x22, 0x10, 0x22, 0x24, 0x22, 0x08, 0x22, 0x08, 0x14, 0x22, 0x08, 0x20,
        0x10, 0x02, 0x04, 0x00, 0x00, 0x00, 0x22, 0x12, 0x20, 0x12, 0x20, 0x10, 0x1E, 0x22, 0x08, 0x08, 0x28, 0x08,
        0x41, 0x12, 0x22, 0x1C, 0x1C, 0x20, 0x04, 0x08, 0x24, 0x14, 0x55, 0x14, 0x0E, 0x10, 0x10, 0x08, 0x08, 0x00,
        0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x14, 0x08,
        0x00, 0x3A, 0x00, 0x04, 0x10, 0x08, 0x08, 0x04, 0x00, 0x0C, 0x40, 0x1C, 0x1C, 0x3E, 0x1C, 0x0E, 0x1C, 0x1C,
        0x10, 0x1C, 0x1C, 0x00, 0x04, 0x04, 0x00, 0x20, 0x08, 0x1C, 0x22, 0x3C, 0x1C, 0x3C, 0x3E, 0x20, 0x1C, 0x22,
        0x1C, 0x18, 0x22, 0x1E, 0x41, 0x22, 0x1C, 0x10, 0x1C, 0x22, 0x1C, 0x08, 0x1C, 0x08, 0x14, 0x22, 0x08, 0x3E,
        0x1C, 0x01, 0x1C, 0x00, 0x00, 0x00, 0x1D, 0x2C, 0x1C, 0x0D, 0x1C, 0x10, 0x02, 0x22, 0x08, 0x08, 0x24, 0x08,
        0x41, 0x12, 0x1C, 0x10, 0x04, 0x20, 0x38, 0x08, 0x1A, 0x08, 0x22, 0x22, 0x02, 0x3C, 0x0C, 0x08, 0x30, 0x00,
        0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x7F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1C, 0x00, 0x00, 0x30, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x10, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1C, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00}};

} // namespace

bool X11Plot::init(X11Connection &connection)
{
    if (!connection.is_open())
    {
        return false;
    }

    m_connection = &connection;
    init_color_masks();
    init_palette();
    update_color_lookup();
    return true;
}

bool X11Plot::create(const Window parent, const int width, const int height)
{
    if (m_connection == nullptr || !m_connection->is_open() || parent == None || width <= 0 || height <= 0)
    {
        return false;
    }

    resize(width, height);
    if (m_window != None)
    {
        return true;
    }

    Display *display = m_connection->display();
    XSetWindowAttributes attributes{};
    attributes.background_pixel = m_color_lookup[0];
    attributes.border_pixel = m_color_lookup[0];
    attributes.colormap = m_connection->colormap();
    attributes.event_mask =
        ExposureMask | StructureNotifyMask | KeyPressMask | PointerMotionMask | ButtonPressMask | ButtonReleaseMask;

    m_window = XCreateWindow(display, parent, 0, 0, m_width, m_height, 0, m_connection->depth(), InputOutput,
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

    return true;
}

void X11Plot::destroy()
{
    if (m_connection == nullptr || !m_connection->is_open())
    {
        m_window = None;
        m_gc = nullptr;
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
    XFlush(display);
    m_mapped = false;
}

void X11Plot::show()
{
    if (m_connection == nullptr || !m_connection->is_open() || m_window == None)
    {
        return;
    }

    XMapRaised(m_connection->display(), m_window);
    m_mapped = true;
    paint_region(0, 0, m_width, m_height);
    reset_dirty();
    XFlush(m_connection->display());
}

void X11Plot::hide()
{
    if (m_connection == nullptr || !m_connection->is_open() || m_window == None || !m_mapped)
    {
        return;
    }

    XUnmapWindow(m_connection->display(), m_window);
    XFlush(m_connection->display());
    m_mapped = false;
}

void X11Plot::set_position(const int x, const int y)
{
    if (m_connection == nullptr || !m_connection->is_open() || m_window == None)
    {
        return;
    }

    XMoveWindow(m_connection->display(), m_window, x, y);
    XFlush(m_connection->display());
}

bool X11Plot::resize(const int width, const int height)
{
    PlotBufferSize buffer_size{};
    if (!get_plot_buffer_size(width, height, buffer_size))
    {
        return false;
    }
    if (width == m_width && height == m_height && has_pixels())
    {
        return false;
    }

    m_width = width;
    m_height = height;
    m_row_len = buffer_size.row_len;
    m_pixels_len = buffer_size.pixels_len;
    m_pixels.assign(m_pixels_len, 0);
    m_saved_pixels.clear();
    m_xor_pixels.clear();
    reset_dirty();
    mark_dirty(0, 0, m_width, m_height);

    if (m_connection != nullptr && m_connection->is_open() && m_window != None)
    {
        XResizeWindow(m_connection->display(), m_window, m_width, m_height);
        XFlush(m_connection->display());
    }
    return true;
}

void X11Plot::clear()
{
    if (!has_pixels())
    {
        return;
    }

    std::fill(m_pixels.begin(), m_pixels.end(), 0);
    m_xor_pixels.clear();
    mark_dirty(0, 0, m_width, m_height);
}

void X11Plot::write_pixel(const int x, const int y, const int color)
{
    if (!is_valid_position(x, y))
    {
        return;
    }

    m_pixels[pixel_offset(x, y)] = static_cast<Byte>(color & 0xff);
    mark_dirty(x, y, x + 1, y + 1);
}

int X11Plot::read_pixel(const int x, const int y) const
{
    if (!is_valid_position(x, y))
    {
        return 0;
    }

    return m_pixels[pixel_offset(x, y)];
}

void X11Plot::save_xor_pixel(const int x, const int y)
{
    if (!is_valid_position(x, y))
    {
        return;
    }
    m_xor_pixels.push_back(X11XorPixel{x, y, m_pixels[pixel_offset(x, y)]});
}

void X11Plot::save_xor_line(int x1, int y1, const int x2, const int y2)
{
    const int dx{x2 > x1 ? x2 - x1 : x1 - x2};
    const int sx{x1 < x2 ? 1 : -1};
    const int dy{y2 > y1 ? y1 - y2 : y2 - y1};
    const int sy{y1 < y2 ? 1 : -1};
    int err{dx + dy};

    while (true)
    {
        save_xor_pixel(x1, y1);
        if (x1 == x2 && y1 == y2)
        {
            break;
        }
        const int e2{2 * err};
        if (e2 >= dy)
        {
            err += dy;
            x1 += sx;
        }
        if (e2 <= dx)
        {
            err += dx;
            y1 += sy;
        }
    }
}

void X11Plot::draw_xor_line(const int x1, const int y1, const int x2, const int y2)
{
    if (m_connection == nullptr || !m_connection->is_open() || m_window == None || !m_mapped)
    {
        return;
    }

    flush();
    save_xor_line(x1, y1, x2, y2);
    Display *display{m_connection->display()};
    XGCValues values{};
    values.function = GXxor;
    values.foreground = WhitePixel(display, m_connection->screen());
    values.line_style = LineOnOffDash;
    values.cap_style = CapButt;
    GC xor_gc{XCreateGC(display, m_window, GCFunction | GCForeground | GCLineStyle | GCCapStyle, &values)};
    if (xor_gc == nullptr)
    {
        return;
    }

    const char dashes[]{2, 2};
    XSetDashes(display, xor_gc, 0, dashes, static_cast<int>(sizeof(dashes)));
    XDrawLine(display, m_window, xor_gc, x1, y1, x2, y2);
    XFreeGC(display, xor_gc);
    XFlush(display);
}

void X11Plot::clear_xor_lines()
{
    if (m_xor_pixels.empty())
    {
        return;
    }
    if (m_connection == nullptr || !m_connection->is_open() || m_window == None || m_gc == nullptr || !m_mapped)
    {
        m_xor_pixels.clear();
        return;
    }

    Display *display{m_connection->display()};
    for (const X11XorPixel &pixel : m_xor_pixels)
    {
        XSetForeground(display, m_gc, m_color_lookup[pixel.color]);
        XDrawPoint(display, m_window, m_gc, pixel.x, pixel.y);
    }
    XFlush(display);
    m_xor_pixels.clear();
}

void X11Plot::read_palette()
{
    if (!g_got_real_dac)
    {
        return;
    }

    for (size_t i = 0; i < m_clut.size(); ++i)
    {
        g_dac_box[i][0] = m_clut[i][0];
        g_dac_box[i][1] = m_clut[i][1];
        g_dac_box[i][2] = m_clut[i][2];
    }
}

void X11Plot::write_palette()
{
    for (size_t i = 0; i < m_clut.size(); ++i)
    {
        m_clut[i][0] = g_dac_box[i][0];
        m_clut[i][1] = g_dac_box[i][1];
        m_clut[i][2] = g_dac_box[i][2];
    }
    update_color_lookup();
    mark_dirty(0, 0, m_width, m_height);
    flush();
}

void X11Plot::display_string(int x, const int y, const int fg, const int bg, const char *text)
{
    if (text == nullptr)
    {
        return;
    }

    while (*text != '\0')
    {
        for (int row = 0; row < 8; ++row)
        {
            int x1{x};
            int col{8};
            const Byte pixel{FONT_8x8[row][static_cast<unsigned char>(*text)]};
            while (col-- > 0)
            {
                const int color{(pixel & (1 << col)) != 0 ? fg : bg};
                write_pixel(x1++, y + row, color);
            }
        }
        x += 8;
        ++text;
    }
    flush();
}

void X11Plot::flush()
{
    if (!m_dirty.active || m_window == None || m_gc == nullptr || !m_mapped)
    {
        return;
    }

    paint_region(m_dirty.left, m_dirty.top, m_dirty.right - m_dirty.left, m_dirty.bottom - m_dirty.top);
    reset_dirty();
    XFlush(m_connection->display());
}

void X11Plot::save_graphics()
{
    m_saved_pixels = m_pixels;
}

void X11Plot::restore_graphics()
{
    if (m_saved_pixels.size() != m_pixels.size())
    {
        return;
    }

    m_pixels = m_saved_pixels;
    m_xor_pixels.clear();
    mark_dirty(0, 0, m_width, m_height);
}

bool X11Plot::handle_event(const XEvent &event)
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

Window X11Plot::window() const
{
    return m_window;
}

bool X11Plot::is_mapped() const
{
    return m_mapped;
}

int X11Plot::width() const
{
    return m_width;
}

int X11Plot::height() const
{
    return m_height;
}

void X11Plot::init_palette()
{
    for (std::array<Byte, 3> &color : m_clut)
    {
        color.fill(0);
    }
}

void X11Plot::update_color_lookup()
{
    for (size_t i = 0; i < m_color_lookup.size(); ++i)
    {
        m_color_lookup[i] = make_pixel(m_clut[i][0], m_clut[i][1], m_clut[i][2]);
    }
}

void X11Plot::init_color_masks()
{
    assert(m_connection != nullptr);
    const Visual *visual{m_connection->visual()};
    m_red_mask = make_color_mask(visual->red_mask);
    m_green_mask = make_color_mask(visual->green_mask);
    m_blue_mask = make_color_mask(visual->blue_mask);
}

unsigned long X11Plot::make_pixel(const Byte red, const Byte green, const Byte blue) const
{
    return scale_component(red, m_red_mask) | scale_component(green, m_green_mask) | scale_component(blue, m_blue_mask);
}

void X11Plot::mark_dirty(int left, int top, int right, int bottom)
{
    if (!has_pixels())
    {
        return;
    }

    left = std::clamp(left, 0, m_width);
    top = std::clamp(top, 0, m_height);
    right = std::clamp(right, 0, m_width);
    bottom = std::clamp(bottom, 0, m_height);
    if (right <= left || bottom <= top)
    {
        return;
    }

    if (!m_dirty.active)
    {
        m_dirty = X11DirtyRect{left, top, right, bottom, true};
        return;
    }

    m_dirty.left = std::min(m_dirty.left, left);
    m_dirty.top = std::min(m_dirty.top, top);
    m_dirty.right = std::max(m_dirty.right, right);
    m_dirty.bottom = std::max(m_dirty.bottom, bottom);
}

void X11Plot::reset_dirty()
{
    m_dirty = X11DirtyRect{};
}

void X11Plot::paint_region(int x, int y, int width, int height)
{
    if (!has_pixels() || m_connection == nullptr || !m_connection->is_open() || m_window == None || m_gc == nullptr)
    {
        return;
    }

    const int left{std::clamp(x, 0, m_width)};
    const int top{std::clamp(y, 0, m_height)};
    const int right{std::clamp(x + width, 0, m_width)};
    const int bottom{std::clamp(y + height, 0, m_height)};
    if (right <= left || bottom <= top)
    {
        return;
    }

    Display *display = m_connection->display();
    const int image_width{right - left};
    const int image_height{bottom - top};
    XImage *image = XCreateImage(
        display, m_connection->visual(), m_connection->depth(), ZPixmap, 0, nullptr, image_width, image_height, 32, 0);
    if (image == nullptr)
    {
        return;
    }

    std::vector<char> image_data{
        static_cast<std::size_t>(image->bytes_per_line) * static_cast<std::size_t>(image_height)};
    image->data = image_data.data();
    for (int row = 0; row < image_height; ++row)
    {
        for (int col = 0; col < image_width; ++col)
        {
            const int source_x{left + col};
            const int source_y{top + row};
            const Byte color{m_pixels[pixel_offset(source_x, source_y)]};
            XPutPixel(image, col, row, m_color_lookup[color]);
        }
    }
    XPutImage(display, m_window, m_gc, image, 0, 0, left, top, image_width, image_height);
    image->data = nullptr;
    XDestroyImage(image);
}

std::size_t X11Plot::pixel_offset(const int x, const int y) const
{
    return static_cast<std::size_t>(m_height - y - 1) * m_row_len + static_cast<std::size_t>(x);
}

bool X11Plot::has_pixels() const
{
    return m_width > 0 && m_height > 0 && m_pixels.size() == m_pixels_len;
}

bool X11Plot::is_valid_position(const int x, const int y) const
{
    return has_pixels() && x >= 0 && x < m_width && y >= 0 && y < m_height;
}

} // namespace id::misc
