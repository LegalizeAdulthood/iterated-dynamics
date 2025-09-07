// SPDX-License-Identifier: GPL-3.0-only
//
/*
    This file includes miscellaneous plot functions and logic
    for 3D, used by lorenz and line3d
*/
#include "3d/plot3d.h"

#include "3d/line3d.h"
#include "engine/calcfrac.h"
#include "engine/cmdfiles.h"
#include "engine/id_data.h"
#include "fractals/fractype.h"
#include "io/loadmap.h"
#include "ui/diskvid.h"
#include "ui/rotate.h"
#include "ui/spindac.h"
#include "ui/video.h"

#include <config/port.h>

#include <cmath>

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
int g_adjust_3d_x{};
int g_adjust_3d_y{};
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
void draw_line(int x1, int y1, int x2, int y2, int color)

{
    // uses Bresenham algorithm to draw a line
    // vector components
    int row;
    int col;
    int final; // final row or column number
    int g;     // used to test for new row or column
    int inc1;  // G increment when row or column doesn't change
    int inc2;  // G increment when row or column changes

    int dx = x2 - x1;                   // find vector components
    int dy = y2 - y1;
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

static void plot3d_superimpose16(int x, int y, int /*color*/)
{
    int tmp = get_color(x, y);

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
                id::targa_color(x, y, color);
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
                id::targa_color(x, y, color);
            }
        }
    }
}

static void plot3d_superimpose256(int x, int y, int color)
{
    int tmp;

    Byte t_c = (Byte) (255 - color);

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
            g_put_color(x, y, color|(tmp&240));
            if (g_targa_out)
            {
                if (!id::illumine())
                {
                    id::targa_color(x, y, color|(tmp&240));
                }
                else
                {
                    targa_write_disk(x+g_logical_screen_x_offset, y+g_logical_screen_y_offset, t_c, 0, 0);
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
            g_put_color(x, y, color|(tmp&15));
            if (g_targa_out)
            {
                if (!id::illumine())
                {
                    id::targa_color(x, y, color|(tmp&15));
                }
                else
                {
                    targa_read_disk(x+g_logical_screen_x_offset, y+g_logical_screen_y_offset, &s_t_red, (Byte *)&tmp, (Byte *)&tmp);
                    targa_write_disk(x+g_logical_screen_x_offset, y+g_logical_screen_y_offset, s_t_red, 0, t_c);
                }
            }
        }
    }
}

static void plot_ifs3d_superimpose256(int x, int y, int color)
{
    int tmp;

    Byte t_c = (Byte) (255 - color);

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
                if (!id::illumine())
                {
                    id::targa_color(x, y, color|tmp);
                }
                else
                {
                    targa_write_disk(x+g_logical_screen_x_offset, y+g_logical_screen_y_offset, t_c, 0, 0);
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
                if (!id::illumine())
                {
                    id::targa_color(x, y, color|tmp);
                }
                else
                {
                    targa_read_disk(x+g_logical_screen_x_offset, y+g_logical_screen_y_offset, &s_t_red, (Byte *)&tmp, (Byte *)&tmp);
                    targa_write_disk(x+g_logical_screen_x_offset, y+g_logical_screen_y_offset, s_t_red, 0, t_c);
                }
            }
        }
    }
}

static void plot3d_alternate(int x, int y, int color)
{
    Byte t_c = (Byte) (255 - color);
    // low res high color red/blue 3D plot function
    // if g_which_image = RED, compresses color to lower 128 colors

    // my mind is STILL fried - lower indices = darker colors is EASIER!
    color = g_colors - color;
    if ((g_which_image == StereoImage::RED) && !((x+y)&1)) // - lower half palette
    {
        if (s_red_local_left < x && x < s_red_local_right)
        {
            g_put_color(x, y, color >> 1);
            if (g_targa_out)
            {
                if (!id::illumine())
                {
                    id::targa_color(x, y, color >> 1);
                }
                else
                {
                    targa_write_disk(x+g_logical_screen_x_offset, y+g_logical_screen_y_offset, t_c, 0, 0);
                }
            }
        }
    }
    else if ((g_which_image == StereoImage::BLUE) && ((x+y)&1))  // - upper half palette
    {
        if (s_blue_local_left < x && x < s_blue_local_right)
        {
            g_put_color(x, y, (color >> 1)+(g_colors >> 1));
            if (g_targa_out)
            {
                if (!id::illumine())
                {
                    id::targa_color(x, y, (color >> 1)+(g_colors >> 1));
                }
                else
                {
                    targa_write_disk(x+g_logical_screen_x_offset, y+g_logical_screen_y_offset, s_t_red, 0, t_c);
                }
            }
        }
    }
}

static void plot3d_cross_eyed_a(int x, int y, int color)
{
    x /= 2;
    y /= 2;
    if (g_which_image == StereoImage::BLUE)
    {
        x += g_logical_screen_x_dots/2;
    }
    if (g_row_count >= g_logical_screen_y_dots/2)
    {
        // hidden surface kludge
        if (get_color(x, y) != 0)
        {
            return;
        }
    }
    g_put_color(x, y, color);
}

static void plot3d_cross_eyed_b(int x, int y, int color)
{
    x /= 2;
    y /= 2;
    if (g_which_image == StereoImage::BLUE)
    {
        x += g_logical_screen_x_dots/2;
    }
    g_put_color(x, y, color);
}

static void plot3d_cross_eyed_c(int x, int y, int color)
{
    if (g_row_count >= g_logical_screen_y_dots/2)
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
        id::g_standard_plot = plot3d_alternate;
        break;

    case GlassesType::SUPERIMPOSE:
        if (g_colors == 256)
        {
            if (g_fractal_type != FractalType::IFS_3D)
            {
                id::g_standard_plot = plot3d_superimpose256;
            }
            else
            {
                id::g_standard_plot = plot_ifs3d_superimpose256;
            }
        }
        else
        {
            id::g_standard_plot = plot3d_superimpose16;
        }
        break;

    case GlassesType::STEREO_PAIR: // crosseyed mode
        if (g_screen_x_dots < 2*g_logical_screen_x_dots)
        {
            if (id::g_x_rot == 0 && id::g_y_rot == 0)
            {
                id::g_standard_plot = plot3d_cross_eyed_a; // use hidden surface kludge
            }
            else
            {
                id::g_standard_plot = plot3d_cross_eyed_b;
            }
        }
        else if (id::g_x_rot == 0 && id::g_y_rot == 0)
        {
            id::g_standard_plot = plot3d_cross_eyed_c; // use hidden surface kludge
        }
        else
        {
            id::g_standard_plot = g_put_color;
        }
        break;

    default:
        id::g_standard_plot = g_put_color;
        break;
    }

    id::g_x_shift = (int)((id::g_shift_x * (double)g_logical_screen_x_dots)/100);
    g_x_shift1 = id::g_x_shift;
    id::g_y_shift = (int)((id::g_shift_y * (double)g_logical_screen_y_dots)/100);
    g_y_shift1 = id::g_y_shift;

    if (g_glasses_type != GlassesType::NONE)
    {
        s_red_local_left  = (int)((g_red_crop_left      * (double)g_logical_screen_x_dots)/100.0);
        s_red_local_right = (int)(((100 - g_red_crop_right) * (double)g_logical_screen_x_dots)/100.0);
        s_blue_local_left = (int)((g_blue_crop_left     * (double)g_logical_screen_x_dots)/100.0);
        s_blue_local_right = (int)(((100 - g_blue_crop_right) * (double)g_logical_screen_x_dots)/100.0);
        d_red_bright    = (double)g_red_bright/100.0;
        d_blue_bright   = (double)g_blue_bright/100.0;

        switch (g_which_image)
        {
        case StereoImage::RED:
            id::g_x_shift  += (int)((g_eye_separation* (double)g_logical_screen_x_dots)/200);
            id::g_xx_adjust = (int)(((g_adjust_3d_x+id::g_converge_x_adjust)* (double)g_logical_screen_x_dots)/100);
            g_x_shift1 -= (int)((g_eye_separation* (double)g_logical_screen_x_dots)/200);
            g_xx_adjust1 = (int)(((g_adjust_3d_x-id::g_converge_x_adjust)* (double)g_logical_screen_x_dots)/100);
            if (g_glasses_type == GlassesType::STEREO_PAIR && g_screen_x_dots >= 2*g_logical_screen_x_dots)
            {
                g_logical_screen_x_offset = g_screen_x_dots / 2 - g_logical_screen_x_dots;
            }
            break;

        case StereoImage::BLUE:
            id::g_x_shift  -= (int)((g_eye_separation* (double)g_logical_screen_x_dots)/200);
            id::g_xx_adjust = (int)(((g_adjust_3d_x-id::g_converge_x_adjust)* (double)g_logical_screen_x_dots)/100);
            if (g_glasses_type == GlassesType::STEREO_PAIR && g_screen_x_dots >= 2*g_logical_screen_x_dots)
            {
                g_logical_screen_x_offset = g_screen_x_dots / 2;
            }
            break;

        default:
            break;
        }
    }
    else
    {
        id::g_xx_adjust = (int)((g_adjust_3d_x* (double)g_logical_screen_x_dots)/100);
    }
    id::g_yy_adjust = (int)(-(g_adjust_3d_y* (double)g_logical_screen_y_dots)/100);

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
                elem[0] = (Byte)(elem[0] * d_red_bright);
                elem[2] = (Byte)(elem[2] * d_blue_bright);
            }
        }
        spin_dac(0, 1); // load it, but don't spin
    }
}
