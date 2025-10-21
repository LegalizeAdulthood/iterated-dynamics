// SPDX-License-Identifier: GPL-3.0-only
//
/*
    This file includes miscellaneous plot functions and logic
    for 3D, used by lorenz and line3d
*/
#include "geometry/plot3d.h"

#include "engine/calcfrac.h"
#include "engine/LogicalScreen.h"
#include "engine/spindac.h"
#include "engine/VideoInfo.h"
#include "fractals/fractype.h"
#include "geometry/line3d.h"
#include "io/loadmap.h"
#include "ui/diskvid.h"
#include "ui/video.h"

#include <config/port.h>

#include <cmath>

using namespace id::engine;
using namespace id::fractals;
using namespace id::io;
using namespace id::ui;

namespace id::geometry
{

// Use these palette indices for red/blue - same on ega/vga
enum
{
    PAL_BLUE = 1,
    PAL_RED = 2,
    PAL_MAGENTA = 3
};

StereoImage g_which_image;
int g_xx_adjust1{};
int g_yy_adjust1{};
int g_eye_separation{};
GlassesType g_glasses_type{};
int g_x_shift1{};
int g_y_shift1{};
math::Point2i g_adjust_3d{};
int g_red_crop_left{4};
int g_red_crop_right{};
int g_blue_crop_left{};
int g_blue_crop_right{4};
int g_red_bright{80};
int g_blue_bright{100};

static int s_red_local_left{};
static int s_red_local_right{};
static int s_blue_local_left{};
static int s_blue_local_right{};
static Byte s_t_red{};

static void plot3d_superimpose16(int x, int y, int color);
static void plot3d_superimpose256(int x, int y, int color);
static void plot_ifs3d_superimpose256(int x, int y, int color);

// Bresenham's algorithm for drawing line
void draw_line(const int x1, const int y1, const int x2, const int y2, const int color)
{
    // uses Bresenham algorithm to draw a line
    // vector components
    int row;
    int col;
    int final; // final row or column number
    int g;     // used to test for new row or column
    int inc1;  // G increment when row or column doesn't change
    int inc2;  // G increment when row or column changes

    const int dx = x2 - x1;                   // find vector components
    const int dy = y2 - y1;
    bool pos_slope = dx > 0;                   // is slope positive?
    if (dy < 0)
    {
        pos_slope = !pos_slope;
    }
    if (std::abs(dx) > std::abs(dy))                  // shallow line case
    {
        if (dx > 0)         // determine start point and last column
        {
            col = x1;
            row = y1;
            final = x2;
        }
        else
        {
            col = x2;
            row = y2;
            final = x1;
        }
        inc1 = 2 * std::abs(dy);             // determine increments and initial G
        g = inc1 - std::abs(dx);
        inc2 = 2 * (std::abs(dy) - std::abs(dx));
        if (pos_slope)
        {
            while (col <= final)    // step through columns checking for new row
            {
                g_plot(col, row, color);
                col++;
                if (g >= 0)             // it's time to change rows
                {
                    row++;      // positive slope so increment through the rows
                    g += inc2;
                }
                else                          // stay at the same row
                {
                    g += inc1;
                }
            }
        }
        else
        {
            while (col <= final)    // step through columns checking for new row
            {
                g_plot(col, row, color);
                col++;
                if (g > 0)              // it's time to change rows
                {
                    row--;      // negative slope so decrement through the rows
                    g += inc2;
                }
                else                          // stay at the same row
                {
                    g += inc1;
                }
            }
        }
    }   // if |dX| > |dY|
    else                            // steep line case
    {
        if (dy > 0)             // determine start point and last row
        {
            col = x1;
            row = y1;
            final = y2;
        }
        else
        {
            col = x2;
            row = y2;
            final = y1;
        }
        inc1 = 2 * std::abs(dx);             // determine increments and initial G
        g = inc1 - std::abs(dy);
        inc2 = 2 * (std::abs(dx) - std::abs(dy));
        if (pos_slope)
        {
            while (row <= final)    // step through rows checking for new column
            {
                g_plot(col, row, color);
                row++;
                if (g >= 0)                 // it's time to change columns
                {
                    col++;  // positive slope so increment through the columns
                    g += inc2;
                }
                else                      // stay at the same column
                {
                    g += inc1;
                }
            }
        }
        else
        {
            while (row <= final)    // step through rows checking for new column
            {
                g_plot(col, row, color);
                row++;
                if (g > 0)                  // it's time to change columns
                {
                    col--;  // negative slope so decrement through the columns
                    g += inc2;
                }
                else                      // stay at the same column
                {
                    g += inc1;
                }
            }
        }
    }
}   // draw_line

static void plot3d_superimpose16(const int x, const int y, const int /*color*/)
{
    const int tmp = get_color(x, y);

    if (g_which_image == StereoImage::RED) // RED
    {
        int color = PAL_RED;
        if (tmp > 0 && tmp != color)
        {
            color = PAL_MAGENTA;
        }
        if (s_red_local_left < x && x < s_red_local_right)
        {
            g_put_color(x, y, color);
            if (g_targa_out)
            {
                targa_color(x, y, color);
            }
        }
    }
    else if (g_which_image == StereoImage::BLUE)   // BLUE
    {
        if (s_blue_local_left < x && x < s_blue_local_right)
        {
            int color = PAL_BLUE;
            if (tmp > 0 && tmp != color)
            {
                color = PAL_MAGENTA;
            }
            g_put_color(x, y, color);
            if (g_targa_out)
            {
                targa_color(x, y, color);
            }
        }
    }
}

static void plot3d_superimpose256(const int x, const int y, int color)
{
    int tmp;

    const Byte t_c = static_cast<Byte>(255 - color);

    if (color != 0)         // Keeps index 0 still 0
    {
        color = g_colors - color; //  Reverses color order
        color = 1 + color / 18; //  Maps colors 1-255 to 15 even ranges
    }

    tmp = get_color(x, y);
    // map to 16 colors
    if (g_which_image == StereoImage::RED) // RED
    {
        if (s_red_local_left < x && x < s_red_local_right)
        {
            // Overwrite prev Red don't mess w/blue
            g_put_color(x, y, color| tmp & 240);
            if (g_targa_out)
            {
                if (!illumine())
                {
                    targa_color(x, y, color| tmp & 240);
                }
                else
                {
                    targa_write_disk(x+g_logical_screen.x_offset, y+g_logical_screen.y_offset, t_c, 0, 0);
                }
            }
        }
    }
    else if (g_which_image == StereoImage::BLUE)   // BLUE
    {
        if (s_blue_local_left < x && x < s_blue_local_right)
        {
            // Overwrite previous blue, don't mess with existing red
            color = color <<4;
            g_put_color(x, y, color| tmp & 15);
            if (g_targa_out)
            {
                if (!illumine())
                {
                    targa_color(x, y, color| tmp & 15);
                }
                else
                {
                    targa_read_disk(x+g_logical_screen.x_offset, y+g_logical_screen.y_offset, &s_t_red, reinterpret_cast<Byte *>(&tmp), reinterpret_cast<Byte *>(&tmp));
                    targa_write_disk(x+g_logical_screen.x_offset, y+g_logical_screen.y_offset, s_t_red, 0, t_c);
                }
            }
        }
    }
}

static void plot_ifs3d_superimpose256(const int x, const int y, int color)
{
    int tmp;

    const Byte t_c = static_cast<Byte>(255 - color);

    if (color != 0)         // Keeps index 0 still 0
    {
        // my mind is fried - lower indices = darker colors is EASIER!
        color = g_colors - color; //  Reverses color order
        color = 1 + color / 18; //  Looks weird but maps colors 1-255 to 15 relatively even ranges
    }

    tmp = get_color(x, y);
    // map to 16 colors
    if (g_which_image == StereoImage::RED) // RED
    {
        if (s_red_local_left < x && x < s_red_local_right)
        {
            g_put_color(x, y, color|tmp);
            if (g_targa_out)
            {
                if (!illumine())
                {
                    targa_color(x, y, color|tmp);
                }
                else
                {
                    targa_write_disk(x+g_logical_screen.x_offset, y+g_logical_screen.y_offset, t_c, 0, 0);
                }
            }
        }
    }
    else if (g_which_image == StereoImage::BLUE)   // BLUE
    {
        if (s_blue_local_left < x && x < s_blue_local_right)
        {
            color = color <<4;
            g_put_color(x, y, color|tmp);
            if (g_targa_out)
            {
                if (!illumine())
                {
                    targa_color(x, y, color|tmp);
                }
                else
                {
                    targa_read_disk(x+g_logical_screen.x_offset, y+g_logical_screen.y_offset, &s_t_red, reinterpret_cast<Byte *>(&tmp), reinterpret_cast<Byte *>(&tmp));
                    targa_write_disk(x+g_logical_screen.x_offset, y+g_logical_screen.y_offset, s_t_red, 0, t_c);
                }
            }
        }
    }
}

static void plot3d_alternate(const int x, const int y, int color)
{
    const Byte t_c = static_cast<Byte>(255 - color);
    // low res high color red/blue 3D plot function
    // if g_which_image = RED, compresses color to lower 128 colors

    // my mind is STILL fried - lower indices = darker colors is EASIER!
    color = g_colors - color;
    if (g_which_image == StereoImage::RED && !(x + y &1)) // - lower half palette
    {
        if (s_red_local_left < x && x < s_red_local_right)
        {
            g_put_color(x, y, color >> 1);
            if (g_targa_out)
            {
                if (!illumine())
                {
                    targa_color(x, y, color >> 1);
                }
                else
                {
                    targa_write_disk(x+g_logical_screen.x_offset, y+g_logical_screen.y_offset, t_c, 0, 0);
                }
            }
        }
    }
    else if (g_which_image == StereoImage::BLUE && x + y & 1)  // - upper half palette
    {
        if (s_blue_local_left < x && x < s_blue_local_right)
        {
            g_put_color(x, y, (color >> 1)+(g_colors >> 1));
            if (g_targa_out)
            {
                if (!illumine())
                {
                    targa_color(x, y, (color >> 1)+(g_colors >> 1));
                }
                else
                {
                    targa_write_disk(x+g_logical_screen.x_offset, y+g_logical_screen.y_offset, s_t_red, 0, t_c);
                }
            }
        }
    }
}

static void plot3d_cross_eyed_a(int x, int y, const int color)
{
    x /= 2;
    y /= 2;
    if (g_which_image == StereoImage::BLUE)
    {
        x += g_logical_screen.x_dots/2;
    }
    if (g_row_count >= g_logical_screen.y_dots/2)
    {
        // hidden surface kludge
        if (get_color(x, y) != 0)
        {
            return;
        }
    }
    g_put_color(x, y, color);
}

static void plot3d_cross_eyed_b(int x, int y, const int color)
{
    x /= 2;
    y /= 2;
    if (g_which_image == StereoImage::BLUE)
    {
        x += g_logical_screen.x_dots/2;
    }
    g_put_color(x, y, color);
}

static void plot3d_cross_eyed_c(const int x, const int y, const int color)
{
    if (g_row_count >= g_logical_screen.y_dots/2)
    {
        // hidden surface kludge
        if (get_color(x, y) != 0)
        {
            return;
        }
    }
    g_put_color(x, y, color);
}

void plot_setup()
{
    double d_red_bright  = 0;
    double d_blue_bright = 0;

    // set funny glasses plot function
    switch (g_glasses_type)
    {
    case GlassesType::ALTERNATING:
        g_standard_plot = plot3d_alternate;
        break;

    case GlassesType::SUPERIMPOSE:
        if (g_colors == 256)
        {
            if (g_fractal_type != FractalType::IFS_3D)
            {
                g_standard_plot = plot3d_superimpose256;
            }
            else
            {
                g_standard_plot = plot_ifs3d_superimpose256;
            }
        }
        else
        {
            g_standard_plot = plot3d_superimpose16;
        }
        break;

    case GlassesType::STEREO_PAIR: // crosseyed mode
        if (g_screen_x_dots < 2*g_logical_screen.x_dots)
        {
            if (g_x_rot == 0 && g_y_rot == 0)
            {
                g_standard_plot = plot3d_cross_eyed_a; // use hidden surface kludge
            }
            else
            {
                g_standard_plot = plot3d_cross_eyed_b;
            }
        }
        else if (g_x_rot == 0 && g_y_rot == 0)
        {
            g_standard_plot = plot3d_cross_eyed_c; // use hidden surface kludge
        }
        else
        {
            g_standard_plot = g_put_color;
        }
        break;

    default:
        g_standard_plot = g_put_color;
        break;
    }

    g_x_shift = static_cast<int>(g_shift_x * static_cast<double>(g_logical_screen.x_dots) / 100);
    g_x_shift1 = g_x_shift;
    g_y_shift = static_cast<int>(g_shift_y * static_cast<double>(g_logical_screen.y_dots) / 100);
    g_y_shift1 = g_y_shift;

    if (g_glasses_type != GlassesType::NONE)
    {
        s_red_local_left  = static_cast<int>(g_red_crop_left * static_cast<double>(g_logical_screen.x_dots) / 100.0);
        s_red_local_right = static_cast<int>((100 - g_red_crop_right) * static_cast<double>(g_logical_screen.x_dots) / 100.0);
        s_blue_local_left = static_cast<int>(g_blue_crop_left * static_cast<double>(g_logical_screen.x_dots) / 100.0);
        s_blue_local_right = static_cast<int>((100 - g_blue_crop_right) * static_cast<double>(g_logical_screen.x_dots) / 100.0);
        d_red_bright    = static_cast<double>(g_red_bright) /100.0;
        d_blue_bright   = static_cast<double>(g_blue_bright) /100.0;

        switch (g_which_image)
        {
        case StereoImage::RED:
            g_x_shift  += static_cast<int>(g_eye_separation * static_cast<double>(g_logical_screen.x_dots) / 200);
            g_xx_adjust = static_cast<int>(
                (g_adjust_3d.x + g_converge_x_adjust) * static_cast<double>(g_logical_screen.x_dots) / 100);
            g_x_shift1 -= static_cast<int>(g_eye_separation * static_cast<double>(g_logical_screen.x_dots) / 200);
            g_xx_adjust1 = static_cast<int>(
                (g_adjust_3d.x - g_converge_x_adjust) * static_cast<double>(g_logical_screen.x_dots) / 100);
            if (g_glasses_type == GlassesType::STEREO_PAIR && g_screen_x_dots >= 2*g_logical_screen.x_dots)
            {
                g_logical_screen.x_offset = g_screen_x_dots / 2 - g_logical_screen.x_dots;
            }
            break;

        case StereoImage::BLUE:
            g_x_shift  -= static_cast<int>(g_eye_separation * static_cast<double>(g_logical_screen.x_dots) / 200);
            g_xx_adjust = static_cast<int>(
                (g_adjust_3d.x - g_converge_x_adjust) * static_cast<double>(g_logical_screen.x_dots) / 100);
            if (g_glasses_type == GlassesType::STEREO_PAIR && g_screen_x_dots >= 2*g_logical_screen.x_dots)
            {
                g_logical_screen.x_offset = g_screen_x_dots / 2;
            }
            break;

        default:
            break;
        }
    }
    else
    {
        g_xx_adjust = static_cast<int>(g_adjust_3d.x * static_cast<double>(g_logical_screen.x_dots) / 100);
    }
    g_yy_adjust = static_cast<int>(-(g_adjust_3d.y * static_cast<double>(g_logical_screen.y_dots)) / 100);

    if (g_map_set)
    {
        validate_luts(g_map_name); // read the palette file
        if (glasses_alternating_or_superimpose())
        {
            if (g_glasses_type == GlassesType::SUPERIMPOSE && g_colors < 256)
            {
                g_dac_box[PAL_RED  ][0] = 255;
                g_dac_box[PAL_RED  ][1] =  0;
                g_dac_box[PAL_RED  ][2] =  0;

                g_dac_box[PAL_BLUE ][0] =  0;
                g_dac_box[PAL_BLUE ][1] =  0;
                g_dac_box[PAL_BLUE ][2] = 255;

                g_dac_box[PAL_MAGENTA][0] = 255;
                g_dac_box[PAL_MAGENTA][1] = 0;
                g_dac_box[PAL_MAGENTA][2] = 255;
            }
            for (auto &elem : g_dac_box)
            {
                elem[0] = static_cast<Byte>(elem[0] * d_red_bright);
                elem[2] = static_cast<Byte>(elem[2] * d_blue_bright);
            }
        }
        refresh_dac();
    }
}

} // namespace id::geometry
