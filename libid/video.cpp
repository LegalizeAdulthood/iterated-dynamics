// SPDX-License-Identifier: GPL-3.0-only
//
#include "video.h"

#include "calcfrac.h"
#include "cmdfiles.h"
#include "cmplx.h"
#include "diskvid.h"
#include "drivers.h"
#include "find_file.h"
#include "id.h"
#include "helpdefs.h"
#include "id_data.h"
#include "id_main.h"
#include "init_failure.h"
#include "make_path.h"
#include "mpmath.h"
#include "read_ticker.h"
#include "rotate.h"
#include "special_dirs.h"
#include "stack_avail.h"
#include "zoom.h"

#ifdef WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef STRICT
#define STRICT
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <crtdbg.h>
#endif

#include <cassert>
#include <cstdarg>

static void (*s_write_pixel)(int x, int y, int color){};
static int (*s_read_pixel)(int x, int y){};
static void (*s_write_span)(int y, int x, int lastx, BYTE const *pixels){};
static void (*s_read_span)(int y, int x, int lastx, BYTE *pixels){};

// Global variables (yuck!)
int g_row_count{};
int g_vesa_x_res{};
int g_vesa_y_res{};
int g_video_start_x{};
int g_video_start_y{};

// read_span(int row, int startcol, int stopcol, BYTE *pixels)
//
// This routine is a 'span' analog of 'getcolor()', and gets a horizontal
// span of pixels from the screen and stores it in pixels[] at one byte per
// pixel
//
void read_span(int row, int startcol, int stopcol, BYTE *pixels)
{
    if (startcol + g_logical_screen_x_offset >= g_screen_x_dots || row + g_logical_screen_y_offset >= g_screen_y_dots)
    {
        return;
    }
    assert(s_read_span);
    (*s_read_span)(row + g_logical_screen_y_offset, startcol + g_logical_screen_x_offset, stopcol + g_logical_screen_x_offset, pixels);
}

// write_span(int row, int startcol, int stopcol, BYTE *pixels)
//
// This routine is a 'span' analog of 'putcolor()', and puts a horizontal
// span of pixels to the screen from pixels[] at one byte per pixel
// Called by the GIF decoder
//
void write_span(int row, int startcol, int stopcol, BYTE const *pixels)
{
    if (startcol + g_logical_screen_x_offset >= g_screen_x_dots || row + g_logical_screen_y_offset > g_screen_y_dots)
    {
        return;
    }
    assert(s_write_span);
    (*s_write_span)(row + g_logical_screen_y_offset, startcol + g_logical_screen_x_offset, stopcol + g_logical_screen_x_offset, pixels);
}

static void normal_write_span(int y, int x, int lastx, BYTE const *pixels)
{
    int width = lastx - x + 1;
    assert(s_write_pixel);
    for (int i = 0; i < width; i++)
    {
        (*s_write_pixel)(x + i, y, pixels[i]);
    }
}

static void normal_read_span(int y, int x, int lastx, BYTE *pixels)
{
    int width = lastx - x + 1;
    assert(s_read_pixel);
    for (int i = 0; i < width; i++)
    {
        pixels[i] = (*s_read_pixel)(x + i, y);
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
    assert(FALSE);
}

static int null_read_pixel(int a, int b)
{
    assert(FALSE);
    return 0;
}

void set_null_video()
{
    s_write_pixel = null_write_pixel;
    s_read_pixel = null_read_pixel;
}

// Return the color on the screen at the (xdot, ydot) point
//
int get_color(int xdot, int ydot)
{
    const int x1 = xdot + g_logical_screen_x_offset;
    const int y1 = ydot + g_logical_screen_y_offset;
    if (x1 < 0 || y1 < 0 || x1 >= g_screen_x_dots || y1 >= g_screen_y_dots)
    {
        // this can happen in boundary trace
        return 0;
    }
    assert(s_read_pixel);
    return (*s_read_pixel)(x1, y1);
}

// write the color on the screen at the (xdot, ydot) point
//
void put_color_a(int xdot, int ydot, int color)
{
    int x1 = xdot + g_logical_screen_x_offset;
    int y1 = ydot + g_logical_screen_y_offset;
    assert(x1 >= 0 && x1 <= g_screen_x_dots);
    assert(y1 >= 0 && y1 <= g_screen_y_dots);
    assert(s_write_pixel);
    (*s_write_pixel)(x1, y1, color & g_and_color);
}

// This routine is a 'line' analog of 'putcolor()', and sends an
// entire line of pixels to the screen (0 <= xdot < xdots) at a clip
// Called by the GIF decoder
//
int out_line(BYTE *pixels, int linelen)
{
#ifdef WIN32
    assert(_CrtCheckMemory());
#endif
    if (g_row_count + g_logical_screen_y_offset >= g_screen_y_dots)
    {
        return 0;
    }
    assert(s_write_span);
    (*s_write_span)(g_row_count + g_logical_screen_y_offset, g_logical_screen_x_offset, linelen + g_logical_screen_x_offset - 1, pixels);
    g_row_count++;
    return 0;
}
