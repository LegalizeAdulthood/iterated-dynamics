/*
    This file includes miscellaneous plot functions and logic
    for 3D, used by lorenz and line3d
*/
#include "port.h"
#include "prototyp.h"

#include "plot3d.h"

#include "calcfrac.h"
#include "cmdfiles.h"
#include "diskvid.h"
#include "fractype.h"
#include "get_color.h"
#include "id_data.h"
#include "line3d.h"
#include "loadmap.h"
#include "os.h"
#include "rotate.h"
#include "spindac.h"

// Use these palette indices for red/blue - same on ega/vga
#define PAL_BLUE    1
#define PAL_RED 2
#define PAL_MAGENTA 3

stereo_images g_which_image;
int g_xx_adjust1;
int g_yy_adjust1;
int g_eye_separation = 0;
int g_glasses_type = 0;
int g_x_shift1;
int g_y_shift1;
int g_adjust_3d_x = 0;
int g_adjust_3d_y = 0;
static int red_local_left;
static int red_local_right;
static int blue_local_left;
static int blue_local_right;
int g_red_crop_left   = 4;
int g_red_crop_right  = 0;
int g_blue_crop_left  = 0;
int g_blue_crop_right = 4;
int g_red_bright      = 80;
int g_blue_bright     = 100;

static BYTE T_RED;

// Bresenham's algorithm for drawing line
void draw_line(int X1, int Y1, int X2, int Y2, int color)

{
    // uses Bresenham algorithm to draw a line
    int dX, dY;                     // vector components
    int row, col,
        final,                      // final row or column number
        G,                  // used to test for new row or column
        inc1,           // G increment when row or column doesn't change
        inc2;               // G increment when row or column changes
    char pos_slope;

    dX = X2 - X1;                   // find vector components
    dY = Y2 - Y1;
    pos_slope = (char)(dX > 0);                   // is slope positive?
    if (dY < 0)
    {
        pos_slope = (char)!pos_slope;
    }
    if (std::abs(dX) > std::abs(dY))                  // shallow line case
    {
        if (dX > 0)         // determine start point and last column
        {
            col = X1;
            row = Y1;
            final = X2;
        }
        else
        {
            col = X2;
            row = Y2;
            final = X1;
        }
        inc1 = 2 * std::abs(dY);             // determine increments and initial G
        G = inc1 - std::abs(dX);
        inc2 = 2 * (std::abs(dY) - std::abs(dX));
        if (pos_slope)
        {
            while (col <= final)    // step through columns checking for new row
            {
                (*g_plot)(col, row, color);
                col++;
                if (G >= 0)             // it's time to change rows
                {
                    row++;      // positive slope so increment through the rows
                    G += inc2;
                }
                else                          // stay at the same row
                {
                    G += inc1;
                }
            }
        }
        else
        {
            while (col <= final)    // step through columns checking for new row
            {
                (*g_plot)(col, row, color);
                col++;
                if (G > 0)              // it's time to change rows
                {
                    row--;      // negative slope so decrement through the rows
                    G += inc2;
                }
                else                          // stay at the same row
                {
                    G += inc1;
                }
            }
        }
    }   // if |dX| > |dY|
    else                            // steep line case
    {
        if (dY > 0)             // determine start point and last row
        {
            col = X1;
            row = Y1;
            final = Y2;
        }
        else
        {
            col = X2;
            row = Y2;
            final = Y1;
        }
        inc1 = 2 * std::abs(dX);             // determine increments and initial G
        G = inc1 - std::abs(dY);
        inc2 = 2 * (std::abs(dX) - std::abs(dY));
        if (pos_slope)
        {
            while (row <= final)    // step through rows checking for new column
            {
                (*g_plot)(col, row, color);
                row++;
                if (G >= 0)                 // it's time to change columns
                {
                    col++;  // positive slope so increment through the columns
                    G += inc2;
                }
                else                      // stay at the same column
                {
                    G += inc1;
                }
            }
        }
        else
        {
            while (row <= final)    // step through rows checking for new column
            {
                (*g_plot)(col, row, color);
                row++;
                if (G > 0)                  // it's time to change columns
                {
                    col--;  // negative slope so decrement through the columns
                    G += inc2;
                }
                else                      // stay at the same column
                {
                    G += inc1;
                }
            }
        }
    }
}   // draw_line

void plot3dsuperimpose16(int x, int y, int color)
{
    int tmp;

    tmp = getcolor(x, y);

    if (g_which_image == stereo_images::RED) // RED
    {
        color = PAL_RED;
        if (tmp > 0 && tmp != color)
        {
            color = PAL_MAGENTA;
        }
        if (red_local_left < x && x < red_local_right)
        {
            g_put_color(x, y, color);
            if (g_targa_out)
            {
                targa_color(x, y, color);
            }
        }
    }
    else if (g_which_image == stereo_images::BLUE)   // BLUE
    {
        if (blue_local_left < x && x < blue_local_right)
        {
            color = PAL_BLUE;
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


void plot3dsuperimpose256(int x, int y, int color)
{
    int tmp;
    BYTE t_c;

    t_c = (BYTE)(255-color);

    if (color != 0)         // Keeps index 0 still 0
    {
        color = g_colors - color; //  Reverses color order
        color = 1 + color / 18; //  Maps colors 1-255 to 15 even ranges
    }

    tmp = getcolor(x, y);
    // map to 16 colors
    if (g_which_image == stereo_images::RED) // RED
    {
        if (red_local_left < x && x < red_local_right)
        {
            // Overwrite prev Red don't mess w/blue
            g_put_color(x, y, color|(tmp&240));
            if (g_targa_out)
            {
                if (!ILLUMINE)
                {
                    targa_color(x, y, color|(tmp&240));
                }
                else
                {
                    targa_writedisk(x+g_logical_screen_x_offset, y+g_logical_screen_y_offset, t_c, 0, 0);
                }
            }
        }
    }
    else if (g_which_image == stereo_images::BLUE)   // BLUE
    {
        if (blue_local_left < x && x < blue_local_right)
        {
            // Overwrite previous blue, don't mess with existing red
            color = color <<4;
            g_put_color(x, y, color|(tmp&15));
            if (g_targa_out)
            {
                if (!ILLUMINE)
                {
                    targa_color(x, y, color|(tmp&15));
                }
                else
                {
                    targa_readdisk(x+g_logical_screen_x_offset, y+g_logical_screen_y_offset, &T_RED, (BYTE *)&tmp, (BYTE *)&tmp);
                    targa_writedisk(x+g_logical_screen_x_offset, y+g_logical_screen_y_offset, T_RED, 0, t_c);
                }
            }
        }
    }
}

void plotIFS3dsuperimpose256(int x, int y, int color)
{
    int tmp;
    BYTE t_c;

    t_c = (BYTE)(255-color);

    if (color != 0)         // Keeps index 0 still 0
    {
        // my mind is fried - lower indices = darker colors is EASIER!
        color = g_colors - color; //  Reverses color order
        color = 1 + color / 18; //  Looks weird but maps colors 1-255 to 15 relatively even ranges
    }

    tmp = getcolor(x, y);
    // map to 16 colors
    if (g_which_image == stereo_images::RED) // RED
    {
        if (red_local_left < x && x < red_local_right)
        {
            g_put_color(x, y, color|tmp);
            if (g_targa_out)
            {
                if (!ILLUMINE)
                {
                    targa_color(x, y, color|tmp);
                }
                else
                {
                    targa_writedisk(x+g_logical_screen_x_offset, y+g_logical_screen_y_offset, t_c, 0, 0);
                }
            }
        }
    }
    else if (g_which_image == stereo_images::BLUE)   // BLUE
    {
        if (blue_local_left < x && x < blue_local_right)
        {
            color = color <<4;
            g_put_color(x, y, color|tmp);
            if (g_targa_out)
            {
                if (!ILLUMINE)
                {
                    targa_color(x, y, color|tmp);
                }
                else
                {
                    targa_readdisk(x+g_logical_screen_x_offset, y+g_logical_screen_y_offset, &T_RED, (BYTE *)&tmp, (BYTE *)&tmp);
                    targa_writedisk(x+g_logical_screen_x_offset, y+g_logical_screen_y_offset, T_RED, 0, t_c);
                }
            }
        }
    }
}

void plot3dalternate(int x, int y, int color)
{
    BYTE t_c;

    t_c = (BYTE)(255-color);
    // lorez high color red/blue 3D plot function
    // if which image = 1, compresses color to lower 128 colors

    // my mind is STILL fried - lower indices = darker colors is EASIER!
    color = g_colors - color;
    if ((g_which_image == stereo_images::RED) && !((x+y)&1)) // - lower half palette
    {
        if (red_local_left < x && x < red_local_right)
        {
            g_put_color(x, y, color >> 1);
            if (g_targa_out)
            {
                if (!ILLUMINE)
                {
                    targa_color(x, y, color >> 1);
                }
                else
                {
                    targa_writedisk(x+g_logical_screen_x_offset, y+g_logical_screen_y_offset, t_c, 0, 0);
                }
            }
        }
    }
    else if ((g_which_image == stereo_images::BLUE) && ((x+y)&1))  // - upper half palette
    {
        if (blue_local_left < x && x < blue_local_right)
        {
            g_put_color(x, y, (color >> 1)+(g_colors >> 1));
            if (g_targa_out)
            {
                if (!ILLUMINE)
                {
                    targa_color(x, y, (color >> 1)+(g_colors >> 1));
                }
                else
                {
                    targa_writedisk(x+g_logical_screen_x_offset, y+g_logical_screen_y_offset, T_RED, 0, t_c);
                }
            }
        }
    }
}

void plot3dcrosseyedA(int x, int y, int color)
{
    x /= 2;
    y /= 2;
    if (g_which_image == stereo_images::BLUE)
    {
        x += g_logical_screen_x_dots/2;
    }
    if (g_row_count >= g_logical_screen_y_dots/2)
    {
        // hidden surface kludge
        if (getcolor(x, y) != 0)
        {
            return;
        }
    }
    g_put_color(x, y, color);
}

void plot3dcrosseyedB(int x, int y, int color)
{
    x /= 2;
    y /= 2;
    if (g_which_image == stereo_images::BLUE)
    {
        x += g_logical_screen_x_dots/2;
    }
    g_put_color(x, y, color);
}

void plot3dcrosseyedC(int x, int y, int color)
{
    if (g_row_count >= g_logical_screen_y_dots/2)
    {
        // hidden surface kludge
        if (getcolor(x, y) != 0)
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
    case 1:
        g_standard_plot = plot3dalternate;
        break;

    case 2:
        if (g_colors == 256)
        {
            if (g_fractal_type != fractal_type::IFS3D)
            {
                g_standard_plot = plot3dsuperimpose256;
            }
            else
            {
                g_standard_plot = plotIFS3dsuperimpose256;
            }
        }
        else
        {
            g_standard_plot = plot3dsuperimpose16;
        }
        break;

    case 4: // crosseyed mode
        if (g_screen_x_dots < 2*g_logical_screen_x_dots)
        {
            if (XROT == 0 && YROT == 0)
            {
                g_standard_plot = plot3dcrosseyedA; // use hidden surface kludge
            }
            else
            {
                g_standard_plot = plot3dcrosseyedB;
            }
        }
        else if (XROT == 0 && YROT == 0)
        {
            g_standard_plot = plot3dcrosseyedC; // use hidden surface kludge
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

    g_x_shift = (int)((XSHIFT * (double)g_logical_screen_x_dots)/100);
    g_x_shift1 = g_x_shift;
    g_y_shift = (int)((YSHIFT * (double)g_logical_screen_y_dots)/100);
    g_y_shift1 = g_y_shift;

    if (g_glasses_type)
    {
        red_local_left  = (int)((g_red_crop_left      * (double)g_logical_screen_x_dots)/100.0);
        red_local_right = (int)(((100 - g_red_crop_right) * (double)g_logical_screen_x_dots)/100.0);
        blue_local_left = (int)((g_blue_crop_left     * (double)g_logical_screen_x_dots)/100.0);
        blue_local_right = (int)(((100 - g_blue_crop_right) * (double)g_logical_screen_x_dots)/100.0);
        d_red_bright    = (double)g_red_bright/100.0;
        d_blue_bright   = (double)g_blue_bright/100.0;

        switch (g_which_image)
        {
        case stereo_images::RED:
            g_x_shift  += (int)((g_eye_separation* (double)g_logical_screen_x_dots)/200);
            g_xx_adjust = (int)(((g_adjust_3d_x+g_converge_x_adjust)* (double)g_logical_screen_x_dots)/100);
            g_x_shift1 -= (int)((g_eye_separation* (double)g_logical_screen_x_dots)/200);
            g_xx_adjust1 = (int)(((g_adjust_3d_x-g_converge_x_adjust)* (double)g_logical_screen_x_dots)/100);
            if (g_glasses_type == 4 && g_screen_x_dots >= 2*g_logical_screen_x_dots)
            {
                g_logical_screen_x_offset = g_screen_x_dots / 2 - g_logical_screen_x_dots;
            }
            break;

        case stereo_images::BLUE:
            g_x_shift  -= (int)((g_eye_separation* (double)g_logical_screen_x_dots)/200);
            g_xx_adjust = (int)(((g_adjust_3d_x-g_converge_x_adjust)* (double)g_logical_screen_x_dots)/100);
            if (g_glasses_type == 4 && g_screen_x_dots >= 2*g_logical_screen_x_dots)
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
        g_xx_adjust = (int)((g_adjust_3d_x* (double)g_logical_screen_x_dots)/100);
    }
    g_yy_adjust = (int)(-(g_adjust_3d_y* (double)g_logical_screen_y_dots)/100);

    if (g_map_set)
    {
        ValidateLuts(g_map_name.c_str()); // read the palette file
        if (g_glasses_type == 1 || g_glasses_type == 2)
        {
            if (g_glasses_type == 2 && g_colors < 256)
            {
                g_dac_box[PAL_RED  ][0] = 63;
                g_dac_box[PAL_RED  ][1] =  0;
                g_dac_box[PAL_RED  ][2] =  0;

                g_dac_box[PAL_BLUE ][0] =  0;
                g_dac_box[PAL_BLUE ][1] =  0;
                g_dac_box[PAL_BLUE ][2] = 63;

                g_dac_box[PAL_MAGENTA][0] = 63;
                g_dac_box[PAL_MAGENTA][1] =    0;
                g_dac_box[PAL_MAGENTA][2] = 63;
            }
            for (auto &elem : g_dac_box)
            {
                elem[0] = (BYTE)(elem[0] * d_red_bright);
                elem[2] = (BYTE)(elem[2] * d_blue_bright);
            }
        }
        spindac(0, 1); // load it, but don't spin
    }
}
