// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#pragma once

#include <X11Driver/X11Connection.h>

#include <config/port.h>

#include <array>
#include <cstddef>
#include <vector>

namespace id::misc
{

struct X11ColorMask
{
    unsigned long mask{};
    int shift{};
    int bits{};
};

struct X11DirtyRect
{
    int left{};
    int top{};
    int right{};
    int bottom{};
    bool active{};
};

struct X11XorPixel
{
    int x{};
    int y{};
    Byte color{};
};

class X11Plot
{
public:
    bool init(X11Connection &connection);
    bool create(Window parent, int width, int height);
    void destroy();
    void show();
    void hide();
    void set_position(int x, int y);
    bool resize(int width, int height);
    void read_palette();
    void write_palette();
    void clear();
    void write_pixel(int x, int y, int color);
    int read_pixel(int x, int y) const;
    void draw_xor_line(int x1, int y1, int x2, int y2);
    void clear_xor_lines();
    void display_string(int x, int y, int fg, int bg, const char *text);
    void flush();
    void save_graphics();
    void restore_graphics();
    bool handle_event(const XEvent &event);
    Window window() const;
    bool is_mapped() const;
    int width() const;
    int height() const;

private:
    void init_palette();
    void update_color_lookup();
    void init_color_masks();
    unsigned long make_pixel(Byte red, Byte green, Byte blue) const;
    void mark_dirty(int left, int top, int right, int bottom);
    void reset_dirty();
    void paint_region(int x, int y, int width, int height);
    std::size_t pixel_offset(int x, int y) const;
    bool has_pixels() const;
    bool is_valid_position(int x, int y) const;
    void save_xor_line(int x1, int y1, int x2, int y2);
    void save_xor_pixel(int x, int y);

    X11Connection *m_connection{};
    Window m_window{};
    GC m_gc{};
    X11ColorMask m_red_mask;
    X11ColorMask m_green_mask;
    X11ColorMask m_blue_mask;
    std::array<std::array<Byte, 3>, 256> m_clut{};
    std::array<unsigned long, 256> m_color_lookup{};
    std::vector<Byte> m_pixels;
    std::vector<Byte> m_saved_pixels;
    std::vector<X11XorPixel> m_xor_pixels;
    int m_width{};
    int m_height{};
    std::size_t m_row_len{};
    std::size_t m_pixels_len{};
    X11DirtyRect m_dirty;
    bool m_mapped{};
};

} // namespace id::misc
