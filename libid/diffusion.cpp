// SPDX-License-Identifier: GPL-3.0-only
//
#include "diffusion.h"

#include "calcfrac.h"
#include "check_key.h"
#include "cmdfiles.h"
#include "drivers.h"
#include "fixed_pt.h"
#include "fpu087.h"
#include "id.h"
#include "id_data.h"
#include "not_disk_msg.h"
#include "resume.h"
#include "video.h"

#include <cstdlib>

static int s_kbd_check{}; // to limit kbd checking

//**************** standalone engine for "diffusion" *******************

inline int random(int x)
{
    return std::rand() % x;
}

inline bool keyboard_check_needed()
{
    return (++s_kbd_check & 0x7f) == 1;
}

int diffusion()
{
    int xmax;
    int ymax;
    int xmin;
    int ymin;   // Current maximum coordinates
    double cosine;
    double sine;
    double angle;
    float r;
    float radius;

    if (driver_diskp())
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
    int colorshift = (int) g_params[2];

    int colorcount = colorshift; // Counts down from colorshift
    int currentcolor = 1;  // Start at color 1 (color 0 is probably invisible)

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
        xmax = g_logical_screen_x_dots / 2 + border;  // Initial box
        xmin = g_logical_screen_x_dots / 2 - border;
        ymax = g_logical_screen_y_dots / 2 + border;
        ymin = g_logical_screen_y_dots / 2 - border;
    }
    if (mode == 1)
    {
        xmax = g_logical_screen_x_dots / 2 + border;  // Initial box
        xmin = g_logical_screen_x_dots / 2 - border;
        ymin = g_logical_screen_y_dots - border;
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
            get_resume(xmax, xmin, ymax, ymin);
        }
        else
        {
            get_resume(xmax, xmin, ymax, radius);
        }
        end_resume();
    }

    switch (mode)
    {
    case 0: // Single seed point in the center
        g_put_color(g_logical_screen_x_dots / 2, g_logical_screen_y_dots / 2, currentcolor);
        break;
    case 1: // Line along the bottom
        for (int i = 0; i <= g_logical_screen_x_dots; i++)
        {
            g_put_color(i, g_logical_screen_y_dots-1, currentcolor);
        }
        break;
    case 2: // Large square that fills the screen
        if (g_logical_screen_x_dots > g_logical_screen_y_dots)
        {
            for (int i = 0; i < g_logical_screen_y_dots; i++)
            {
                g_put_color(g_logical_screen_x_dots/2-g_logical_screen_y_dots/2 , i , currentcolor);
                g_put_color(g_logical_screen_x_dots/2+g_logical_screen_y_dots/2 , i , currentcolor);
                g_put_color(g_logical_screen_x_dots/2-g_logical_screen_y_dots/2+i , 0 , currentcolor);
                g_put_color(g_logical_screen_x_dots/2-g_logical_screen_y_dots/2+i , g_logical_screen_y_dots-1 , currentcolor);
            }
        }
        else
        {
            for (int i = 0; i < g_logical_screen_x_dots; i++)
            {
                g_put_color(0 , g_logical_screen_y_dots/2-g_logical_screen_x_dots/2+i , currentcolor);
                g_put_color(g_logical_screen_x_dots-1 , g_logical_screen_y_dots/2-g_logical_screen_x_dots/2+i , currentcolor);
                g_put_color(i , g_logical_screen_y_dots/2-g_logical_screen_x_dots/2 , currentcolor);
                g_put_color(i , g_logical_screen_y_dots/2+g_logical_screen_x_dots/2 , currentcolor);
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
            x = (int)(cosine*(xmax-xmin) + g_logical_screen_x_dots);
            y = (int)(sine  *(ymax-ymin) + g_logical_screen_y_dots);
            x = x >> 1; // divide by 2
            y = y >> 1;
            break;
        case 1: /* Release new point on the line ymin somewhere between xmin
                 and xmax */
            y = ymin;
            x = random(xmax-xmin) + (g_logical_screen_x_dots-xmax+xmin)/2;
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
                if (x == xmax)
                {
                    x--;
                }
                else if (x == xmin)
                {
                    x++;
                }
                if (y == ymax)
                {
                    y--;
                }
                else if (y == ymin)
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
                if (y < ymin)
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
                    put_resume(xmax, xmin, ymax, ymin);
                }
                else
                {
                    put_resume(xmax, xmin, ymax, radius);
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
        g_put_color(x, y, colorshift?currentcolor:random(g_colors-1)+1);

        // If we're doing colorshifting then check to see if we need to shift
        if (colorshift)
        {
            if (!--colorcount)
            {
                // If the counter reaches zero then shift
                currentcolor++;      // Increase the current color and wrap
                currentcolor %= g_colors;  // around skipping zero
                if (!currentcolor)
                {
                    currentcolor++;
                }
                colorcount = colorshift;  // and reset the counter
            }
        }

        /* If the new point is close to an edge, we may need to increase
           some limits so that the limits expand to match the growing
           fractal. */

        switch (mode)
        {
        case 0:
            if (((x+border) > xmax) || ((x-border) < xmin)
                || ((y-border) < ymin) || ((y+border) > ymax))
            {
                // Increase box size, but not past the edge of the screen
                ymin--;
                ymax++;
                xmin--;
                xmax++;
                if ((ymin == 0) || (xmin == 0))
                {
                    return 0;
                }
            }
            break;
        case 1: // Decrease ymin, but not past top of screen
            if (y-border < ymin)
            {
                ymin--;
            }
            if (ymin == 0)
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
