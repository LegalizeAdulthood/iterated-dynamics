// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <config/port.h>

#include "win_defines.h"
#include <Windows.h>

#include <string>
#include <vector>

namespace id::misc
{

struct Plot
{
    int init(HINSTANCE instance, LPCSTR title);
    void terminate();
    void create_window(HWND parent);
    void write_pixel(int x, int y, int color);
    int read_pixel(int x, int y);
    void write_span(int y, int x, int last_x, const Byte *pixels);
    void flush();
    void read_span(int y, int x, int last_x, Byte *pixels);
    void set_line_mode(int mode);
    void draw_line(int x1, int y1, int x2, int y2, int color);
    int resize();
    int read_palette();
    int write_palette();
    void schedule_alarm(int secs);
    void clear();
    void redraw();
    void display_string(int x, int y, int fg, int bg, const char *text);
    void save_graphics();
    void restore_graphics();
    HWND get_window() const
    {
        return m_window;
    }
    int get_width() const
    {
        return m_width;
    }
    int get_height() const
    {
        return m_height;
    }

    // message handlers
    void on_paint(HWND window);

private:
    void set_dirty_region(int x_min, int y_min, int x_max, int y_max);
    void init_pixels();
    void create_backing_store();

    HINSTANCE m_instance{};
    std::string m_title;
    HWND m_parent{};
    HWND m_window{};
    HDC m_memory_dc{};
    HBITMAP m_rendering{};
    HBITMAP m_backup{};
    HFONT m_font{};
    bool m_dirty{};
    RECT m_dirty_region{};
    BITMAPINFO m_bmi{};          // contains first clut entry too
    RGBQUAD m_bmi_colors[255]{}; // color look up table NOLINT(clang-diagnostic-unused-private-field)
    std::vector<Byte> m_pixels;
    std::vector<Byte> m_saved_pixels;
    size_t m_pixels_len{};
    size_t m_row_len{};
    int m_width{};
    int m_height{};
    unsigned char m_clut[256][3]{};
};

} // namespace id::misc
