// SPDX-License-Identifier: GPL-3.0-only
//
#include "fractals/diffusion.h"

#include "engine/calcfrac.h"
#include "engine/check_key.h"
#include "engine/id_data.h"
#include "engine/resume.h"
#include "math/fixed_pt.h"
#include "math/fpu087.h"
#include "misc/drivers.h"
#include "misc/id.h"
#include "ui/cmdfiles.h"
#include "ui/not_disk_msg.h"
#include "ui/video.h"

#include <cstdlib>

static int s_kbd_check{}; // to limit kbd checking

//**************** standalone engine for "diffusion" *******************

static int random(int x)
{
    return std::rand() % x;
}

static bool keyboard_check_needed()
{
    return (++s_kbd_check & 0x7f) == 1;
}

int diffusion()
{
    int x_max;
    int y_max;
    int x_min;
    int y_min;   // Current maximum coordinates
    double cosine;
    double sine;
    double angle;
    float r;
    float radius;

    if (driver_is_disk())
    {
        not_disk_msg();
    }

    int y = -1;
    int x = y;
    g_bit_shift = 16;
    g_fudge_factor = 1L << 16;

    int border = (int) g_params[0]; // Distance between release point and fractal
    int mode = (int) g_params[1];   // Determines diffusion type:  0 = central (classic)
    //                                 1 = falling particles
    //                                 2 = square cavity
    // If zero, select colors at random, otherwise shift the color every colorshift points
    int color_shift = (int) g_params[2];

    int color_count = color_shift; // Counts down from colorshift
    int current_color = 1;  // Start at color 1 (color 0 is probably invisible)

    if (mode > 2)
    {
        mode = 0;
    }

    if (border <= 0)
    {
        border = 10;
    }

    std::srand(g_random_seed);
    if (!g_random_seed_flag)
    {
        ++g_random_seed;
    }

    if (mode == 0)
    {
        x_max = g_logical_screen_x_dots / 2 + border;  // Initial box
        x_min = g_logical_screen_x_dots / 2 - border;
        y_max = g_logical_screen_y_dots / 2 + border;
        y_min = g_logical_screen_y_dots / 2 - border;
    }
    if (mode == 1)
    {
        x_max = g_logical_screen_x_dots / 2 + border;  // Initial box
        x_min = g_logical_screen_x_dots / 2 - border;
        y_min = g_logical_screen_y_dots - border;
    }
    if (mode == 2)
    {
        if (g_logical_screen_x_dots > g_logical_screen_y_dots)
        {
            radius = (float)(g_logical_screen_y_dots - border);
        }
        else
        {
            radius = (float)(g_logical_screen_x_dots - border);
        }
    }
    if (g_resuming) // restore worklist, if we can't the above will stay in place
    {
        start_resume();
        if (mode != 2)
        {
            get_resume(x_max, x_min, y_max, y_min);
        }
        else
        {
            get_resume(x_max, x_min, y_max, radius);
        }
        end_resume();
    }

    switch (mode)
    {
    case 0: // Single seed point in the center
        g_put_color(g_logical_screen_x_dots / 2, g_logical_screen_y_dots / 2, current_color);
        break;
    case 1: // Line along the bottom
        for (int i = 0; i <= g_logical_screen_x_dots; i++)
        {
            g_put_color(i, g_logical_screen_y_dots-1, current_color);
        }
        break;
    case 2: // Large square that fills the screen
        if (g_logical_screen_x_dots > g_logical_screen_y_dots)
        {
            for (int i = 0; i < g_logical_screen_y_dots; i++)
            {
                g_put_color(g_logical_screen_x_dots/2-g_logical_screen_y_dots/2 , i , current_color);
                g_put_color(g_logical_screen_x_dots/2+g_logical_screen_y_dots/2 , i , current_color);
                g_put_color(g_logical_screen_x_dots/2-g_logical_screen_y_dots/2+i , 0 , current_color);
                g_put_color(g_logical_screen_x_dots/2-g_logical_screen_y_dots/2+i , g_logical_screen_y_dots-1 , current_color);
            }
        }
        else
        {
            for (int i = 0; i < g_logical_screen_x_dots; i++)
            {
                g_put_color(0 , g_logical_screen_y_dots/2-g_logical_screen_x_dots/2+i , current_color);
                g_put_color(g_logical_screen_x_dots-1 , g_logical_screen_y_dots/2-g_logical_screen_x_dots/2+i , current_color);
                g_put_color(i , g_logical_screen_y_dots/2-g_logical_screen_x_dots/2 , current_color);
                g_put_color(i , g_logical_screen_y_dots/2+g_logical_screen_x_dots/2 , current_color);
            }
        }
        break;
    }

    while (true)
    {
        switch (mode)
        {
        case 0: // Release new point on a circle inside the box
            angle = 2*(double)std::rand()/(RAND_MAX/PI);
            sin_cos(&angle, &sine, &cosine);
            x = (int)(cosine*(x_max-x_min) + g_logical_screen_x_dots);
            y = (int)(sine  *(y_max-y_min) + g_logical_screen_y_dots);
            x = x >> 1; // divide by 2
            y = y >> 1;
            break;
        case 1: /* Release new point on the line ymin somewhere between xmin
                 and xmax */
            y = y_min;
            x = random(x_max-x_min) + (g_logical_screen_x_dots-x_max+x_min)/2;
            break;
        case 2: /* Release new point on a circle inside the box with radius
                 given by the radius variable */
            angle = 2*(double)std::rand()/(RAND_MAX/PI);
            sin_cos(&angle, &sine, &cosine);
            x = (int)(cosine*radius + g_logical_screen_x_dots);
            y = (int)(sine  *radius + g_logical_screen_y_dots);
            x = x >> 1;
            y = y >> 1;
            break;
        }

        // Loop as long as the point (x,y) is surrounded by color 0
        // on all eight sides

        while ((get_color(x+1, y+1) == 0) && (get_color(x+1, y) == 0)
            && (get_color(x+1, y-1) == 0) && (get_color(x  , y+1) == 0)
            && (get_color(x  , y-1) == 0) && (get_color(x-1, y+1) == 0)
            && (get_color(x-1, y) == 0) && (get_color(x-1, y-1) == 0))
        {
            // Erase moving point
            if (g_show_orbit)
            {
                g_put_color(x, y, 0);
            }

            if (mode == 0)
            {
                // Make sure point is inside the box
                if (x == x_max)
                {
                    x--;
                }
                else if (x == x_min)
                {
                    x++;
                }
                if (y == y_max)
                {
                    y--;
                }
                else if (y == y_min)
                {
                    y++;
                }
            }

            if (mode == 1) /* Make sure point is on the screen below ymin, but
                    we need a 1 pixel margin because of the next random step.*/
            {
                if (x >= g_logical_screen_x_dots-1)
                {
                    x--;
                }
                else if (x <= 1)
                {
                    x++;
                }
                if (y < y_min)
                {
                    y++;
                }
            }

            // Take one random step
            x += random(3) - 1;
            y += random(3) - 1;

            // Check keyboard
            if (keyboard_check_needed() && check_key())
            {
                alloc_resume(20, 1);
                if (mode != 2)
                {
                    put_resume(x_max, x_min, y_max, y_min);
                }
                else
                {
                    put_resume(x_max, x_min, y_max, radius);
                }

                s_kbd_check--;
                return 1;
            }

            // Show the moving point
            if (g_show_orbit)
            {
                g_put_color(x, y, random(g_colors-1)+1);
            }
        } // End of loop, now fix the point

        /* If we're doing colorshifting then use currentcolor, otherwise
           pick one at random */
        g_put_color(x, y, color_shift?current_color:random(g_colors-1)+1);

        // If we're doing colorshifting then check to see if we need to shift
        if (color_shift)
        {
            if (!--color_count)
            {
                // If the counter reaches zero then shift
                current_color++;      // Increase the current color and wrap
                current_color %= g_colors;  // around skipping zero
                if (!current_color)
                {
                    current_color++;
                }
                color_count = color_shift;  // and reset the counter
            }
        }

        /* If the new point is close to an edge, we may need to increase
           some limits so that the limits expand to match the growing
           fractal. */

        switch (mode)
        {
        case 0:
            if (((x+border) > x_max) || ((x-border) < x_min)
                || ((y-border) < y_min) || ((y+border) > y_max))
            {
                // Increase box size, but not past the edge of the screen
                y_min--;
                y_max++;
                x_min--;
                x_max++;
                if ((y_min == 0) || (x_min == 0))
                {
                    return 0;
                }
            }
            break;
        case 1: // Decrease ymin, but not past top of screen
            if (y-border < y_min)
            {
                y_min--;
            }
            if (y_min == 0)
            {
                return 0;
            }
            break;
        case 2: /* Decrease the radius where points are released to stay away
                 from the fractal.  It might be decreased by 1 or 2 */
            r = sqr((float)x-g_logical_screen_x_dots/2) + sqr((float)y-g_logical_screen_y_dots/2);
            if (r <= border*border)
            {
                return 0;
            }
            while ((radius-border)*(radius-border) > r)
            {
                radius--;
            }
            break;
        }
    }
}
