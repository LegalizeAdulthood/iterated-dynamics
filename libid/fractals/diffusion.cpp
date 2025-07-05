// SPDX-License-Identifier: GPL-3.0-only
//
#include "fractals/diffusion.h"

#include "engine/calcfrac.h"
#include "engine/check_key.h"
#include "engine/id_data.h"
#include "engine/random_seed.h"
#include "engine/resume.h"
#include "math/fpu087.h"
#include "misc/Driver.h"
#include "misc/id.h"
#include "ui/cmdfiles.h"
#include "ui/not_disk_msg.h"
#include "ui/video.h"

#include <cstdlib>

enum class DiffusionMode
{
    CENTRAL = 0,           // central (classic), single seed point in the center
    FALLING_PARTICLES = 1, // line along the bottom
    SQUARE_CAVITY = 2      // large square that fills the screen
};

class Diffusion
{
public:
    void init();
    void release_new_particle();
    bool keyboard_check_needed();

    int m_kbd_check{};    // to limit kbd checking
    int x_max{};          //
    int y_max{};          //
    int x_min{};          //
    int y_min{};          // Current maximum coordinates
    int y{-1};            //
    int x{-1};            //
    int border{};         // Distance between release point and fractal
    DiffusionMode mode{}; // Determines diffusion type
    int color_shift{};    // 0: select colors at random, otherwise shift the color every color_shift points
    int color_count{};    // Counts down from color_shift
    int current_color{1}; // Start at color 1 (color 0 is probably invisible)
    float radius{};       //
};

static Diffusion s_diffusion;

//**************** standalone engine for "diffusion" *******************

static int random(int x)
{
    return std::rand() % x;
}

bool Diffusion::keyboard_check_needed()
{
    return (++m_kbd_check & 0x7f) == 1;
}

void Diffusion::init()
{
    y = -1;
    x = -1;

    border = (int) g_params[0];
    if (border <= 0)
    {
        border = 10;
    }

    mode = static_cast<DiffusionMode>(g_params[1]);
    if (mode < DiffusionMode::CENTRAL || mode > DiffusionMode::SQUARE_CAVITY)
    {
        mode = DiffusionMode::CENTRAL;
    }

    color_shift = (int) g_params[2];
    color_count = color_shift; // Counts down from colorshift
    current_color = 1;         // Start at color 1 (color 0 is probably invisible)

    set_random_seed();

    radius = 0.0F;
    switch (mode)
    {
    case DiffusionMode::CENTRAL:
        x_max = g_logical_screen_x_dots / 2 + border; // Initial box
        x_min = g_logical_screen_x_dots / 2 - border;
        y_max = g_logical_screen_y_dots / 2 + border;
        y_min = g_logical_screen_y_dots / 2 - border;
        break;

    case DiffusionMode::FALLING_PARTICLES:
        x_max = g_logical_screen_x_dots / 2 + border; // Initial box
        x_min = g_logical_screen_x_dots / 2 - border;
        y_min = g_logical_screen_y_dots - border;
        break;

    case DiffusionMode::SQUARE_CAVITY:
        if (g_logical_screen_x_dots > g_logical_screen_y_dots)
        {
            radius = (float) (g_logical_screen_y_dots - border);
        }
        else
        {
            radius = (float) (g_logical_screen_x_dots - border);
        }
        break;
    }

    if (g_resuming) // restore work list, if we can't the above will stay in place
    {
        start_resume();
        if (mode != DiffusionMode::SQUARE_CAVITY)
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
    case DiffusionMode::CENTRAL:
        g_put_color(g_logical_screen_x_dots / 2, g_logical_screen_y_dots / 2, current_color);
        break;

    case DiffusionMode::FALLING_PARTICLES:
        for (int i = 0; i <= g_logical_screen_x_dots; i++)
        {
            g_put_color(i, g_logical_screen_y_dots - 1, current_color);
        }
        break;

    case DiffusionMode::SQUARE_CAVITY:
        if (g_logical_screen_x_dots > g_logical_screen_y_dots)
        {
            for (int i = 0; i < g_logical_screen_y_dots; i++)
            {
                g_put_color(g_logical_screen_x_dots / 2 - g_logical_screen_y_dots / 2, i, current_color);
                g_put_color(g_logical_screen_x_dots / 2 + g_logical_screen_y_dots / 2, i, current_color);
                g_put_color(g_logical_screen_x_dots / 2 - g_logical_screen_y_dots / 2 + i, 0, current_color);
                g_put_color(g_logical_screen_x_dots / 2 - g_logical_screen_y_dots / 2 + i,
                    g_logical_screen_y_dots - 1, current_color);
            }
        }
        else
        {
            for (int i = 0; i < g_logical_screen_x_dots; i++)
            {
                g_put_color(0, g_logical_screen_y_dots / 2 - g_logical_screen_x_dots / 2 + i, current_color);
                g_put_color(g_logical_screen_x_dots - 1,
                    g_logical_screen_y_dots / 2 - g_logical_screen_x_dots / 2 + i, current_color);
                g_put_color(i, g_logical_screen_y_dots / 2 - g_logical_screen_x_dots / 2, current_color);
                g_put_color(i, g_logical_screen_y_dots / 2 + g_logical_screen_x_dots / 2, current_color);
            }
        }
        break;
    }
}

void Diffusion::release_new_particle()
{
    switch (mode)
    {
    case DiffusionMode::CENTRAL:
    {
        // Release new point on a circle inside the box
        const double angle = 2 * (double) std::rand() / (RAND_MAX / PI);
        double cosine;
        double sine;
        sin_cos(angle, &sine, &cosine);
        x = (int) (cosine * (x_max - x_min) + g_logical_screen_x_dots);
        y = (int) (sine * (y_max - y_min) + g_logical_screen_y_dots);
        x /= 2;
        y /= 2;
        break;
    }

    case DiffusionMode::FALLING_PARTICLES:
        // Release new point on the line ymin somewhere between xmin and xmax
        y = y_min;
        x = random(x_max - x_min) +
            (g_logical_screen_x_dots - x_max + x_min) / 2;
        break;

    case DiffusionMode::SQUARE_CAVITY:
    {
        // Release new point on a circle inside the box with radius given by the radius variable
        const double angle = 2 * (double) std::rand() / (RAND_MAX / PI);
        double cosine;
        double sine;
        sin_cos(angle, &sine, &cosine);
        x = (int) (cosine * radius + g_logical_screen_x_dots);
        y = (int) (sine * radius + g_logical_screen_y_dots);
        x /= 2;
        y /= 2;
        break;
    }
    }
}

int diffusion_type()
{
    s_diffusion.init();

    while (true)
    {
        s_diffusion.release_new_particle();

        // Loop as long as the point (x,y) is surrounded by color 0
        // on all eight sides

        while ((get_color(s_diffusion.x+1, s_diffusion.y+1) == 0) && (get_color(s_diffusion.x+1, s_diffusion.y) == 0)
            && (get_color(s_diffusion.x+1, s_diffusion.y-1) == 0) && (get_color(s_diffusion.x  , s_diffusion.y+1) == 0)
            && (get_color(s_diffusion.x  , s_diffusion.y-1) == 0) && (get_color(s_diffusion.x-1, s_diffusion.y+1) == 0)
            && (get_color(s_diffusion.x-1, s_diffusion.y) == 0) && (get_color(s_diffusion.x-1, s_diffusion.y-1) == 0))
        {
            // Erase moving point
            if (g_show_orbit)
            {
                g_put_color(s_diffusion.x, s_diffusion.y, 0);
            }

            if (s_diffusion.mode == DiffusionMode::CENTRAL)
            {
                // Make sure point is inside the box
                if (s_diffusion.x == s_diffusion.x_max)
                {
                    s_diffusion.x--;
                }
                else if (s_diffusion.x == s_diffusion.x_min)
                {
                    s_diffusion.x++;
                }
                if (s_diffusion.y == s_diffusion.y_max)
                {
                    s_diffusion.y--;
                }
                else if (s_diffusion.y == s_diffusion.y_min)
                {
                    s_diffusion.y++;
                }
            }

            // Make sure point is on the screen below ymin, but
            // we need a 1 pixel margin because of the next random step.
            if (s_diffusion.mode == DiffusionMode::FALLING_PARTICLES)
            {
                if (s_diffusion.x >= g_logical_screen_x_dots-1)
                {
                    s_diffusion.x--;
                }
                else if (s_diffusion.x <= 1)
                {
                    s_diffusion.x++;
                }
                if (s_diffusion.y < s_diffusion.y_min)
                {
                    s_diffusion.y++;
                }
            }

            // Take one random step
            s_diffusion.x += random(3) - 1;
            s_diffusion.y += random(3) - 1;

            // Check keyboard
            if (s_diffusion.keyboard_check_needed() && check_key())
            {
                alloc_resume(20, 1);
                if (s_diffusion.mode != DiffusionMode::SQUARE_CAVITY)
                {
                    put_resume(s_diffusion.x_max, s_diffusion.x_min, s_diffusion.y_max, s_diffusion.y_min);
                }
                else
                {
                    put_resume(s_diffusion.x_max, s_diffusion.x_min, s_diffusion.y_max, s_diffusion.radius);
                }

                s_diffusion.m_kbd_check--;
                return 1;
            }

            // Show the moving point
            if (g_show_orbit)
            {
                g_put_color(s_diffusion.x, s_diffusion.y, random(g_colors-1)+1);
            }
        } // End of loop, now fix the point

        /* If we're doing colorshifting then use currentcolor, otherwise
           pick one at random */
        g_put_color(s_diffusion.x, s_diffusion.y, s_diffusion.color_shift?s_diffusion.current_color:random(g_colors-1)+1);

        // If we're doing colors hifting then check to see if we need to shift
        if (s_diffusion.color_shift)
        {
            if (!--s_diffusion.color_count)
            {
                // If the counter reaches zero then shift
                s_diffusion.current_color++;      // Increase the current color and wrap
                s_diffusion.current_color %= g_colors;  // around skipping zero
                if (!s_diffusion.current_color)
                {
                    s_diffusion.current_color++;
                }
                s_diffusion.color_count = s_diffusion.color_shift;  // and reset the counter
            }
        }

        /* If the new point is close to an edge, we may need to increase
           some limits so that the limits expand to match the growing
           fractal. */

        switch (s_diffusion.mode)
        {
        case DiffusionMode::CENTRAL:
            if (((s_diffusion.x+s_diffusion.border) > s_diffusion.x_max) || ((s_diffusion.x-s_diffusion.border) < s_diffusion.x_min)
                || ((s_diffusion.y-s_diffusion.border) < s_diffusion.y_min) || ((s_diffusion.y+s_diffusion.border) > s_diffusion.y_max))
            {
                // Increase box size, but not past the edge of the screen
                s_diffusion.y_min--;
                s_diffusion.y_max++;
                s_diffusion.x_min--;
                s_diffusion.x_max++;
                if ((s_diffusion.y_min == 0) || (s_diffusion.x_min == 0))
                {
                    return 0;
                }
            }
            break;

        case DiffusionMode::FALLING_PARTICLES:
            // Decrease ymin, but not past top of screen
            if (s_diffusion.y-s_diffusion.border < s_diffusion.y_min)
            {
                s_diffusion.y_min--;
            }
            if (s_diffusion.y_min == 0)
            {
                return 0;
            }
            break;

        case DiffusionMode::SQUARE_CAVITY:
        {
            // Decrease the radius where points are released to stay away
            // from the fractal.  It might be decreased by 1 or 2
            const double r = sqr((float)s_diffusion.x-g_logical_screen_x_dots/2) + sqr((float)s_diffusion.y-g_logical_screen_y_dots/2);
            if (r <= s_diffusion.border*s_diffusion.border)
            {
                return 0;
            }
            while ((s_diffusion.radius-s_diffusion.border)*(s_diffusion.radius-s_diffusion.border) > r)
            {
                s_diffusion.radius--;
            }
            break;
        }
        }
    }
}
