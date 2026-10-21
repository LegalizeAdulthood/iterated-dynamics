// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "engine/pixel.h"

#include "engine/calcfrac.h"
#include "engine/LogicalScreen.h"
#include "engine/VideoInfo.h"

#include <stdexcept>

namespace id::engine
{

namespace
{

WritePixel s_write_pixel{};
ReadPixel s_read_pixel{};

} // namespace

void set_pixel_functions(const WritePixel write_pixel, const ReadPixel read_pixel)
{
    if (write_pixel == nullptr || read_pixel == nullptr)
    {
        throw std::logic_error("Pixel functions must be present");
    }
    s_write_pixel = write_pixel;
    s_read_pixel = read_pixel;
}

int get_color(const int x, const int y)
{
    const int x1 = x + g_logical_screen.x_offset;
    const int y1 = y + g_logical_screen.y_offset;
    if (x1 < 0 || y1 < 0 || x1 >= g_screen_x_dots || y1 >= g_screen_y_dots)
    {
        // Boundary tracing can read beyond the image edge.
        return 0;
    }
    if (s_read_pixel == nullptr)
    {
        throw std::logic_error("Pixel reader is not initialized");
    }
    return s_read_pixel(x1, y1);
}

void put_color_a(const int x, const int y, const int color)
{
    const int x1 = x + g_logical_screen.x_offset;
    const int y1 = y + g_logical_screen.y_offset;
    if (x1 < 0 || y1 < 0 || x1 > g_screen_x_dots || y1 > g_screen_y_dots)
    {
        throw std::logic_error("Pixel write is outside the image");
    }
    if (s_write_pixel == nullptr)
    {
        throw std::logic_error("Pixel writer is not initialized");
    }
    s_write_pixel(x1, y1, color & g_and_color);
}

void read_span(const int row, const int start_col, const int stop_col, Byte *pixels)
{
    const int x{start_col + g_logical_screen.x_offset};
    const int y{row + g_logical_screen.y_offset};
    if (x >= g_screen_x_dots || y >= g_screen_y_dots)
    {
        return;
    }
    if (s_read_pixel == nullptr)
    {
        throw std::logic_error("Pixel reader is not initialized");
    }
    const int width{stop_col - start_col + 1};
    for (int i = 0; i < width; ++i)
    {
        pixels[i] = static_cast<Byte>(s_read_pixel(x + i, y));
    }
}

void write_span(const int row, const int start_col, const int stop_col, const Byte *pixels)
{
    const int x{start_col + g_logical_screen.x_offset};
    const int y{row + g_logical_screen.y_offset};
    if (x >= g_screen_x_dots || y > g_screen_y_dots)
    {
        return;
    }
    if (s_write_pixel == nullptr)
    {
        throw std::logic_error("Pixel writer is not initialized");
    }
    const int width{stop_col - start_col + 1};
    for (int i = 0; i < width; ++i)
    {
        s_write_pixel(x + i, y, pixels[i]);
    }
}

} // namespace id::engine
