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
    bool move_particle();
    void color_particle();
    bool adjust_limits();
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
        x = random(x_max - x_min) + (g_logical_screen_x_dots - x_max + x_min) / 2;
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

bool Diffusion::move_particle()
{
    // Loop as long as the point (x,y) is surrounded by color 0 on all eight sides
    while (get_color(x + 1, y + 1) == 0 && get_color(x + 1, y) == 0 && get_color(x + 1, y - 1) == 0 //
        && get_color(x, y + 1) == 0 && get_color(x, y - 1) == 0 && get_color(x - 1, y + 1) == 0     //
        && get_color(x - 1, y) == 0 && get_color(x - 1, y - 1) == 0)
    {
        // Erase moving point
        if (g_show_orbit)
        {
            g_put_color(x, y, 0);
        }

        if (mode == DiffusionMode::CENTRAL)
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

        // Make sure point is on the screen below ymin, but
        // we need a 1 pixel margin because of the next random step.
        if (mode == DiffusionMode::FALLING_PARTICLES)
        {
            if (x >= g_logical_screen_x_dots - 1)
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
            if (mode != DiffusionMode::SQUARE_CAVITY)
            {
                put_resume(x_max, x_min, y_max, y_min);
            }
            else
            {
                put_resume(x_max, x_min, y_max, radius);
            }

            m_kbd_check--;
            return true;
        }

        // Show the moving point
        if (g_show_orbit)
        {
            g_put_color(x, y, random(g_colors - 1) + 1);
        }
    }
    return false;
}

void Diffusion::color_particle()
{
    // If we're doing color shifting then check to see if we need to shift
    if (color_shift)
    {
        // If we're doing color shifting then use current color
        g_put_color(x, y, current_color);
        if (!--color_count)
        {
            // If the counter reaches zero then shift
            current_color++;           // Increase the current color and wrap
            current_color %= g_colors; // around skipping zero
            if (!current_color)
            {
                current_color++;
            }
            color_count = color_shift; // and reset the counter
        }
    }
    else
    {
        // pick a color at random
        g_put_color(x, y, random(g_colors - 1) + 1);
    }
}

bool Diffusion::adjust_limits()
{
    /* If the new point is close to an edge, we may need to increase
               some limits so that the limits expand to match the growing
               fractal. */

    switch (mode)
    {
    case DiffusionMode::CENTRAL:
        if (((x + border) > x_max) ||
            ((x - border) < x_min) ||
            ((y - border) < y_min) ||
            ((y + border) > y_max))
        {
            // Increase box size, but not past the edge of the screen
            y_min--;
            y_max++;
            x_min--;
            x_max++;
            if ((y_min == 0) || (x_min == 0))
            {
                return true;
            }
        }
        break;

    case DiffusionMode::FALLING_PARTICLES:
        // Decrease ymin, but not past top of screen
        if (y - border < y_min)
        {
            y_min--;
        }
        if (y_min == 0)
        {
            return true;
        }
        break;

    case DiffusionMode::SQUARE_CAVITY:
    {
        // Decrease the radius where points are released to stay away
        // from the fractal.  It might be decreased by 1 or 2
        const double r = sqr((float) x - g_logical_screen_x_dots / 2) +
            sqr((float) y - g_logical_screen_y_dots / 2);
        if (r <= border * border)
        {
            return true;
        }
        while ((radius - border) * (radius - border) > r)
        {
            radius--;
        }
        break;
    }
    }
    return false;
}

int diffusion_type()
{
    Diffusion d;
    d.init();

    while (true)
    {
        d.release_new_particle();

        if (d.move_particle())
        {
            return 1;
        }

        d.color_particle();

        if (d.adjust_limits())
        {
            return 0;
        }
    }
}
