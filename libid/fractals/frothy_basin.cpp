// SPDX-License-Identifier: GPL-3.0-only
//
// frothy basin routines

#include "fractals/frothy_basin.h"

#include "engine/calcfrac.h"
#include "engine/check_key.h"
#include "engine/fractals.h"
#include "engine/id_data.h"
#include "engine/orbit.h"
#include "engine/pixel_grid.h"
#include "fractals/newton.h"
#include "io/loadmap.h"
#include "misc/Driver.h"
#include "ui/cmdfiles.h"
#include "ui/spindac.h"

#include <algorithm>
#include <cmath>

constexpr double FROTH_CLOSE{1e-6};                     // seems like a good value
constexpr double SQRT3{1.732050807568877193};           //
constexpr double FROTH_SLOPE{SQRT3};                    //
constexpr double FROTH_CRITICAL_A{1.028713768218725};   // 1.0287137682187249127

struct Froth
{
    int repeat_mapping;
    int alt_color;
    int attractors;
    int shades;
    double a;
    double half_a;
    double top_x1;
    double top_x2;
    double top_x3;
    double top_x4;
    double left_x1;
    double left_x2;
    double left_x3;
    double left_x4;
    double right_x1;
    double right_x2;
    double right_x3;
    double right_x4;
};

static Froth s_fsp{};

static double froth_top_x_mapping(double x)
{
    return x * x - x -3 * s_fsp.a * s_fsp.a / 4;
}

// color maps which attempt to replicate the images of James Alexander.
static void set_froth_palette()
{
    if (g_color_state != ColorState::DEFAULT)   // 0 means g_dac_box matches default
    {
        return;
    }
    if (g_colors >= 16)
    {
        const char *map_name;
        if (g_colors >= 256)
        {
            if (s_fsp.attractors == 6)
            {
                map_name = "froth6.map";
            }
            else
            {
                map_name = "froth3.map";
            }
        }
        else // colors >= 16
        {
            if (s_fsp.attractors == 6)
            {
                map_name = "froth616.map";
            }
            else
            {
                map_name = "froth316.map";
            }
        }
        if (validate_luts(map_name))
        {
            return;
        }
        g_color_state = ColorState::DEFAULT; // treat map as default
        spin_dac(0, 1);
    }
}

bool froth_per_image()
{
    constexpr double sin_theta = SQRT3 / 2; // sin(2*PI/3)
    constexpr double cos_theta = -0.5;      // cos(2*PI/3)

    // for the all important backwards compatibility
    if (g_params[0] != 2)
    {
        g_params[0] = 1;
    }
    if (g_params[1] != 0)
    {
        g_params[1] = 1;
    }
    s_fsp.repeat_mapping = (int)g_params[0] == 2;
    s_fsp.alt_color = (int)g_params[1];
    s_fsp.a = g_params[2];
    if (std::abs(s_fsp.a) <= FROTH_CRITICAL_A)
    {
        s_fsp.attractors = !s_fsp.repeat_mapping ? 3 : 6;
    }
    else
    {
        s_fsp.attractors = !s_fsp.repeat_mapping ? 2 : 3;
    }

    // new improved values
    // 0.5 is the value that causes the mapping to reach a minimum
    constexpr double x0 = 0.5;
    // a/2 is the value that causes the y value to be invariant over the mappings
    s_fsp.half_a = s_fsp.a/2;
    const double y0 = s_fsp.half_a;
    s_fsp.top_x1 = froth_top_x_mapping(x0);
    s_fsp.top_x2 = froth_top_x_mapping(s_fsp.top_x1);
    s_fsp.top_x3 = froth_top_x_mapping(s_fsp.top_x2);
    s_fsp.top_x4 = froth_top_x_mapping(s_fsp.top_x3);

    // rotate 120 degrees counter-clock-wise
    s_fsp.left_x1 = s_fsp.top_x1 * cos_theta - y0 * sin_theta;
    s_fsp.left_x2 = s_fsp.top_x2 * cos_theta - y0 * sin_theta;
    s_fsp.left_x3 = s_fsp.top_x3 * cos_theta - y0 * sin_theta;
    s_fsp.left_x4 = s_fsp.top_x4 * cos_theta - y0 * sin_theta;

    // rotate 120 degrees clock-wise
    s_fsp.right_x1 = s_fsp.top_x1 * cos_theta + y0 * sin_theta;
    s_fsp.right_x2 = s_fsp.top_x2 * cos_theta + y0 * sin_theta;
    s_fsp.right_x3 = s_fsp.top_x3 * cos_theta + y0 * sin_theta;
    s_fsp.right_x4 = s_fsp.top_x4 * cos_theta + y0 * sin_theta;

    // if 2 attractors, use same shades as 3 attractors
    s_fsp.shades = (g_colors-1) / std::max(3, s_fsp.attractors);

    // rqlim needs to be at least sq(1+sqrt(1+sq(a))),
    // which is never bigger than 6.93..., so we'll call it 7.0
    g_magnitude_limit = std::max(g_magnitude_limit, 7.0);
    set_froth_palette();
    // make the best of the .map situation
    if (s_fsp.attractors != 6 && g_colors >= 16)
    {
        g_orbit_color = (s_fsp.shades << 1) + 1;
    }
    else
    {
        g_orbit_color = g_colors - 1;
    }

    return true;
}

void froth_cleanup()
{
}

// Froth Fractal type
int froth_type()   // per pixel 1/2/g, called with row & col set
{
    int found_attractor = 0;

    if (check_key())
    {
        return -1;
    }

    g_orbit_save_index = 0;
    g_color_iter = 0;
    if (g_show_dot > 0)
    {
        (*g_plot)(g_col, g_row, g_show_dot % g_colors);
    }
    if (g_invert != 0)
    {
        invertz2(&g_tmp_z);
        g_old_z = g_tmp_z;
    }
    else
    {
        g_old_z.x = g_dx_pixel();
        g_old_z.y = g_dy_pixel();
    }

    g_temp_sqr_x = sqr(g_old_z.x);
    g_temp_sqr_y = sqr(g_old_z.y);
    while (!found_attractor && (g_temp_sqr_x + g_temp_sqr_y < g_magnitude_limit) &&
        (g_color_iter < g_max_iterations))
    {
        // simple formula: z = z^2 + conj(z*(-1+ai))
        // but it's the attractor that makes this so interesting
        g_new_z.x = g_temp_sqr_x - g_temp_sqr_y - g_old_z.x - s_fsp.a * g_old_z.y;
        g_old_z.y += (g_old_z.x + g_old_z.x) * g_old_z.y - s_fsp.a * g_old_z.x;
        g_old_z.x = g_new_z.x;
        if (s_fsp.repeat_mapping)
        {
            g_new_z.x = sqr(g_old_z.x) - sqr(g_old_z.y) - g_old_z.x - s_fsp.a * g_old_z.y;
            g_old_z.y += (g_old_z.x + g_old_z.x) * g_old_z.y - s_fsp.a * g_old_z.x;
            g_old_z.x = g_new_z.x;
        }

        g_color_iter++;

        if (g_show_orbit)
        {
            if (driver_key_pressed())
            {
                break;
            }
            plot_orbit(g_old_z.x, g_old_z.y, -1);
        }

        if (std::abs(s_fsp.half_a - g_old_z.y) < FROTH_CLOSE && g_old_z.x >= s_fsp.top_x1 &&
            g_old_z.x <= s_fsp.top_x2)
        {
            if ((!s_fsp.repeat_mapping && s_fsp.attractors == 2) ||
                (s_fsp.repeat_mapping && s_fsp.attractors == 3))
            {
                found_attractor = 1;
            }
            else if (g_old_z.x <= s_fsp.top_x3)
            {
                found_attractor = 1;
            }
            else if (g_old_z.x >= s_fsp.top_x4)
            {
                if (!s_fsp.repeat_mapping)
                {
                    found_attractor = 1;
                }
                else
                {
                    found_attractor = 2;
                }
            }
        }
        else if (std::abs(FROTH_SLOPE * g_old_z.x - s_fsp.a - g_old_z.y) < FROTH_CLOSE &&
            g_old_z.x <= s_fsp.right_x1 && g_old_z.x >= s_fsp.right_x2)
        {
            if (!s_fsp.repeat_mapping && s_fsp.attractors == 2)
            {
                found_attractor = 2;
            }
            else if (s_fsp.repeat_mapping && s_fsp.attractors == 3)
            {
                found_attractor = 3;
            }
            else if (g_old_z.x >= s_fsp.right_x3)
            {
                if (!s_fsp.repeat_mapping)
                {
                    found_attractor = 2;
                }
                else
                {
                    found_attractor = 4;
                }
            }
            else if (g_old_z.x <= s_fsp.right_x4)
            {
                if (!s_fsp.repeat_mapping)
                {
                    found_attractor = 3;
                }
                else
                {
                    found_attractor = 6;
                }
            }
        }
        else if (std::abs(-FROTH_SLOPE * g_old_z.x - s_fsp.a - g_old_z.y) < FROTH_CLOSE &&
            g_old_z.x <= s_fsp.left_x1 && g_old_z.x >= s_fsp.left_x2)
        {
            if (!s_fsp.repeat_mapping && s_fsp.attractors == 2)
            {
                found_attractor = 2;
            }
            else if (s_fsp.repeat_mapping && s_fsp.attractors == 3)
            {
                found_attractor = 2;
            }
            else if (g_old_z.x >= s_fsp.left_x3)
            {
                if (!s_fsp.repeat_mapping)
                {
                    found_attractor = 3;
                }
                else
                {
                    found_attractor = 5;
                }
            }
            else if (g_old_z.x <= s_fsp.left_x4)
            {
                if (!s_fsp.repeat_mapping)
                {
                    found_attractor = 2;
                }
                else
                {
                    found_attractor = 3;
                }
            }
        }
        g_temp_sqr_x = sqr(g_old_z.x);
        g_temp_sqr_y = sqr(g_old_z.y);
    }
    if (g_show_orbit)
    {
        scrub_orbit();
    }

    g_real_color_iter = g_color_iter;
    if ((g_keyboard_check_interval -= std::abs((int)g_real_color_iter)) <= 0)
    {
        if (check_key())
        {
            return -1;
        }
        g_keyboard_check_interval = g_max_keyboard_check_interval;
    }

    // inside - Here's where non-palette based images would be nice.  Instead,
    // we'll use blocks of (colors-1)/3 or (colors-1)/6 and use special froth
    // color maps in attempt to replicate the images of James Alexander.
    if (found_attractor)
    {
        if (g_colors >= 256)
        {
            if (!s_fsp.alt_color)
            {
                g_color_iter = std::min<long>(g_color_iter, s_fsp.shades);
            }
            else
            {
                g_color_iter = s_fsp.shades * g_color_iter / g_max_iterations;
            }
            if (g_color_iter == 0)
            {
                g_color_iter = 1;
            }
            g_color_iter += s_fsp.shades * (found_attractor-1);
        }
        else if (g_colors >= 16)
        {
            // only alternate coloring scheme available for 16 colors

            // Trying to make a better 16 color distribution.
            // Since there are only a few possibilities, just handle each case.
            // This is a guess work here.
            long l_shade = (g_color_iter << 16) / g_max_iterations;
            if (s_fsp.attractors != 6) // either 2 or 3 attractors
            {
                if (l_shade < 2622)         // 0.04
                {
                    g_color_iter = 1;
                }
                else if (l_shade < 10486)     // 0.16
                {
                    g_color_iter = 2;
                }
                else if (l_shade < 23593)     // 0.36
                {
                    g_color_iter = 3;
                }
                else if (l_shade < 41943L)     // 0.64
                {
                    g_color_iter = 4;
                }
                else
                {
                    g_color_iter = 5;
                }
                g_color_iter += 5 * (found_attractor-1);
            }
            else // 6 attractors
            {
                if (l_shade < 10486)        // 0.16
                {
                    g_color_iter = 1;
                }
                else
                {
                    g_color_iter = 2;
                }
                g_color_iter += 2 * (found_attractor-1);
            }
        }
        else   // use a color corresponding to the attractor
        {
            g_color_iter = found_attractor;
        }
        g_old_color_iter = g_color_iter;
    }
    else   // outside, or inside but didn't get sucked in by attractor.
    {
        g_color_iter = 0;
    }

    g_color = std::abs((int)(g_color_iter));

    (*g_plot)(g_col, g_row, g_color);

    return g_color;
}

/*
These last two froth functions are for the orbit-in-window feature.
Normally, this feature requires standard_fractal, but since it is the
attractor that makes the frothybasin type so unique, it is worth
putting in as a stand-alone.
*/

int froth_per_pixel()
{
    g_old_z.x = g_dx_pixel();
    g_old_z.y = g_dy_pixel();
    g_temp_sqr_x = sqr(g_old_z.x);
    g_temp_sqr_y = sqr(g_old_z.y);
    return 0;
}

int froth_orbit()
{
    g_new_z.x = g_temp_sqr_x - g_temp_sqr_y - g_old_z.x - s_fsp.a * g_old_z.y;
    g_new_z.y = 2.0 * g_old_z.x * g_old_z.y - s_fsp.a * g_old_z.x + g_old_z.y;
    if (s_fsp.repeat_mapping)
    {
        g_old_z = g_new_z;
        g_new_z.x = sqr(g_old_z.x) - sqr(g_old_z.y) - g_old_z.x - s_fsp.a * g_old_z.y;
        g_new_z.y = 2.0 * g_old_z.x * g_old_z.y - s_fsp.a * g_old_z.x + g_old_z.y;
    }

    g_temp_sqr_x = sqr(g_new_z.x);
    g_temp_sqr_y = sqr(g_new_z.y);
    if (g_temp_sqr_x + g_temp_sqr_y >= g_magnitude_limit)
    {
        return 1;
    }
    g_old_z = g_new_z;
    return 0;
}
