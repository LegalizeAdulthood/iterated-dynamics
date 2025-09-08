// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/video.h"

#include "engine/calcfrac.h"
#include "engine/cmdfiles.h"
#include "engine/id_data.h"
#include "helpdefs.h"
#include "io/find_file.h"
#include "io/make_path.h"
#include "io/special_dirs.h"
#include "math/cmplx.h"
#include "misc/Driver.h"
#include "misc/id.h"
#include "misc/stack_avail.h"
#include "ui/diskvid.h"
#include "ui/id_main.h"
#include "ui/init_failure.h"
#include "ui/read_ticker.h"
#include "ui/rotate.h"
#include "ui/zoom.h"

#include <cassert>
#include <cstdarg>

using namespace id::engine;
using namespace id::misc;

namespace id::ui
{

static void (*s_write_pixel)(int x, int y, int color){};
static int (*s_read_pixel)(int x, int y){};
static void (*s_write_span)(int y, int x, int last_x, const Byte *pixels){};
static void (*s_read_span)(int y, int x, int last_x, Byte *pixels){};

// Global variables (yuck!)
int g_row_count{};
int g_vesa_x_res{};
int g_vesa_y_res{};
int g_video_start_x{};
int g_video_start_y{};

// read_span(int row, int startcol, int stopcol, Byte *pixels)
//
// This routine is a 'span' analog of 'getcolor()', and gets a horizontal
// span of pixels from the screen and stores it in pixels[] at one byte per
// pixel
//
void read_span(int row, int start_col, int stop_col, Byte *pixels)
{
    if (start_col + g_logical_screen_x_offset >= g_screen_x_dots || row + g_logical_screen_y_offset >= g_screen_y_dots)
    {
        return;
    }
    assert(s_read_span);
    s_read_span(row + g_logical_screen_y_offset, start_col + g_logical_screen_x_offset, stop_col + g_logical_screen_x_offset, pixels);
}

// write_span(int row, int startcol, int stopcol, Byte *pixels)
//
// This routine is a 'span' analog of 'putcolor()', and puts a horizontal
// span of pixels to the screen from pixels[] at one byte per pixel
// Called by the GIF decoder
//
void write_span(int row, int start_col, int stop_col, const Byte *pixels)
{
    if (start_col + g_logical_screen_x_offset >= g_screen_x_dots || row + g_logical_screen_y_offset > g_screen_y_dots)
    {
        return;
    }
    assert(s_write_span);
    s_write_span(row + g_logical_screen_y_offset, start_col + g_logical_screen_x_offset, stop_col + g_logical_screen_x_offset, pixels);
}

static void normal_write_span(int y, int x, int last_x, const Byte *pixels)
{
    int width = last_x - x + 1;
    assert(s_write_pixel);
    for (int i = 0; i < width; i++)
    {
        s_write_pixel(x + i, y, pixels[i]);
    }
}

static void normal_read_span(int y, int x, int last_x, Byte *pixels)
{
    int width = last_x - x + 1;
    assert(s_read_pixel);
    for (int i = 0; i < width; i++)
    {
        pixels[i] = static_cast<Byte>(s_read_pixel(x + i, y));
    }
}

void set_normal_dot()
{
    s_write_pixel = driver_write_pixel;
    s_read_pixel = driver_read_pixel;
}

void set_disk_dot()
{
    s_write_pixel = disk_write_pixel;
    s_read_pixel = disk_read_pixel;
}

void set_normal_span()
{
    s_read_span = normal_read_span;
    s_write_span = normal_write_span;
}

static void null_write_pixel(int a, int b, int c)
{
}

static int null_read_pixel(int a, int b)
{
    return 0;
}

void set_null_video()
{
    s_write_pixel = null_write_pixel;
    s_read_pixel = null_read_pixel;
}

// Return the color on the screen at the (xdot, ydot) point
//
int get_color(int x, int y)
{
    const int x1 = x + g_logical_screen_x_offset;
    const int y1 = y + g_logical_screen_y_offset;
    if (x1 < 0 || y1 < 0 || x1 >= g_screen_x_dots || y1 >= g_screen_y_dots)
    {
        // this can happen in boundary trace
        return 0;
    }
    assert(s_read_pixel);
    return s_read_pixel(x1, y1);
}

// write the color on the screen at the (xdot, ydot) point
//
void put_color_a(int x, int y, int color)
{
    int x1 = x + g_logical_screen_x_offset;
    int y1 = y + g_logical_screen_y_offset;
    assert(x1 >= 0 && x1 <= g_screen_x_dots);
    assert(y1 >= 0 && y1 <= g_screen_y_dots);
    assert(s_write_pixel);
    s_write_pixel(x1, y1, color & g_and_color);
}

// This routine is a 'line' analog of 'putcolor()', and sends an
// entire line of pixels to the screen (0 <= xdot < xdots) at a clip
// Called by the GIF decoder
//
int out_line(Byte *pixels, int line_len)
{
    driver_check_memory();
    if (g_row_count + g_logical_screen_y_offset >= g_screen_y_dots)
    {
        return 0;
    }
    assert(s_write_span);
    s_write_span(g_row_count + g_logical_screen_y_offset, g_logical_screen_x_offset, line_len + g_logical_screen_x_offset - 1, pixels);
    g_row_count++;
    return 0;
}

} // namespace id::ui
