// SPDX-License-Identifier: GPL-3.0-only
//
// frothy basin routines

#include "fractals/FrothyBasin.h"

#include "engine/calcfrac.h"
#include "engine/color_state.h"
#include "engine/fractals.h"
#include "engine/id_data.h"
#include "engine/orbit.h"
#include "engine/pixel_grid.h"
#include "engine/show_dot.h"
#include "fractals/newton.h"
#include "io/loadmap.h"
#include "ui/spindac.h"

#include <algorithm>
#include <cmath>

using namespace id::engine;
using namespace id::io;
using namespace id::math;
using namespace id::ui;

namespace id::fractals
{

constexpr double FROTH_CLOSE{1e-6};                     // seems like a good value
constexpr double SQRT3{1.732050807568877193};           //
constexpr double FROTH_SLOPE{SQRT3};                    //
constexpr double FROTH_CRITICAL_A{1.028713768218725};   // 1.0287137682187249127

FrothyBasin g_frothy_basin{};

double FrothyBasin::top_x_mapping(const double x)
{
    return x * x - x -3 * m_a * m_a / 4;
}

// color maps which attempt to replicate the images of James Alexander.
void FrothyBasin::set_froth_palette()
{
    if (g_color_state != ColorState::DEFAULT_MAP)   // 0 means g_dac_box matches default
    {
        return;
    }
    if (g_colors >= 16)
    {
        const char *map_name;
        if (g_colors >= 256)
        {
            if (m_attractors == 6)
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
            if (m_attractors == 6)
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
        g_color_state = ColorState::DEFAULT_MAP; // treat map as default
        spin_dac(0, 1);
    }
}

bool FrothyBasin::per_image()
{
    constexpr double sin_theta = SQRT3 / 2; // sin(2*PI/3)
    constexpr double cos_theta = -0.5;      // cos(2*PI/3)

    if (g_params[0] != 2.0)
    {
        g_params[0] = 1.0;
    }
    if (g_params[1] != 0.0)
    {
        g_params[1] = 1.0;
    }
    m_repeat_mapping = g_params[0] == 2.0;
    m_alt_color = static_cast<int>(g_params[1]);
    m_a = g_params[2];
    if (std::abs(m_a) <= FROTH_CRITICAL_A)
    {
        m_attractors = !m_repeat_mapping ? 3 : 6;
    }
    else
    {
        m_attractors = !m_repeat_mapping ? 2 : 3;
    }

    // new improved values
    // 0.5 is the value that causes the mapping to reach a minimum
    constexpr double x0 = 0.5;
    // a/2 is the value that causes the y value to be invariant over the mappings
    m_half_a = m_a/2;
    const double y0 = m_half_a;
    m_top_x1 = top_x_mapping(x0);
    m_top_x2 = top_x_mapping(m_top_x1);
    m_top_x3 = top_x_mapping(m_top_x2);
    m_top_x4 = top_x_mapping(m_top_x3);

    // rotate 120 degrees counter-clock-wise
    m_left_x1 = m_top_x1 * cos_theta - y0 * sin_theta;
    m_left_x2 = m_top_x2 * cos_theta - y0 * sin_theta;
    m_left_x3 = m_top_x3 * cos_theta - y0 * sin_theta;
    m_left_x4 = m_top_x4 * cos_theta - y0 * sin_theta;

    // rotate 120 degrees clock-wise
    m_right_x1 = m_top_x1 * cos_theta + y0 * sin_theta;
    m_right_x2 = m_top_x2 * cos_theta + y0 * sin_theta;
    m_right_x3 = m_top_x3 * cos_theta + y0 * sin_theta;
    m_right_x4 = m_top_x4 * cos_theta + y0 * sin_theta;

    // if 2 attractors, use same shades as 3 attractors
    m_shades = (g_colors-1) / std::max(3, m_attractors);

    // rqlim needs to be at least sq(1+sqrt(1+sq(a))),
    // which is never bigger than 6.93..., so we'll call it 7.0
    g_magnitude_limit = std::max(g_magnitude_limit, 7.0);
    set_froth_palette();
    // make the best of the .map situation
    if (m_attractors != 6 && g_colors >= 16)
    {
        g_orbit_color = (m_shades << 1) + 1;
    }
    else
    {
        g_orbit_color = g_colors - 1;
    }

    return true;
}

int FrothyBasin::orbit()
{
    g_new_z.x = g_temp_sqr_x - g_temp_sqr_y - g_old_z.x - m_a * g_old_z.y;
    g_new_z.y = 2.0 * g_old_z.x * g_old_z.y - m_a * g_old_z.x + g_old_z.y;
    if (m_repeat_mapping)
    {
        g_old_z = g_new_z;
        g_new_z.x = sqr(g_old_z.x) - sqr(g_old_z.y) - g_old_z.x - m_a * g_old_z.y;
        g_new_z.y = 2.0 * g_old_z.x * g_old_z.y - m_a * g_old_z.x + g_old_z.y;
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

bool FrothyBasin::keyboard_check()
{
    g_keyboard_check_interval -= std::abs(g_real_color_iter);
    return g_keyboard_check_interval <= 0;
}

void FrothyBasin::keyboard_reset()
{
    g_keyboard_check_interval = g_max_keyboard_check_interval;
}

int FrothyBasin::calc()
{
    int found_attractor = 0;

    g_orbit_save_index = 0;
    g_color_iter = 0;
    if (g_show_dot > 0)
    {
        g_plot(g_col, g_row, g_show_dot % g_colors);
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
    while (!found_attractor && g_temp_sqr_x + g_temp_sqr_y < g_magnitude_limit &&
        g_color_iter < g_max_iterations)
    {
        // simple formula: z = z^2 + conj(z*(-1+ai))
        // but it's the attractor that makes this so interesting
        g_new_z.x = g_temp_sqr_x - g_temp_sqr_y - g_old_z.x - m_a * g_old_z.y;
        g_old_z.y += (g_old_z.x + g_old_z.x) * g_old_z.y - m_a * g_old_z.x;
        g_old_z.x = g_new_z.x;
        if (m_repeat_mapping)
        {
            g_new_z.x = sqr(g_old_z.x) - sqr(g_old_z.y) - g_old_z.x - m_a * g_old_z.y;
            g_old_z.y += (g_old_z.x + g_old_z.x) * g_old_z.y - m_a * g_old_z.x;
            g_old_z.x = g_new_z.x;
        }

        g_color_iter++;

        if (g_show_orbit)
        {
            plot_orbit(g_old_z.x, g_old_z.y, -1);
        }

        if (std::abs(m_half_a - g_old_z.y) < FROTH_CLOSE && g_old_z.x >= m_top_x1 &&
            g_old_z.x <= m_top_x2)
        {
            if ((!m_repeat_mapping && m_attractors == 2) ||
                (m_repeat_mapping && m_attractors == 3))
            {
                found_attractor = 1;
            }
            else if (g_old_z.x <= m_top_x3)
            {
                found_attractor = 1;
            }
            else if (g_old_z.x >= m_top_x4)
            {
                if (!m_repeat_mapping)
                {
                    found_attractor = 1;
                }
                else
                {
                    found_attractor = 2;
                }
            }
        }
        else if (std::abs(FROTH_SLOPE * g_old_z.x - m_a - g_old_z.y) < FROTH_CLOSE &&
            g_old_z.x <= m_right_x1 && g_old_z.x >= m_right_x2)
        {
            if (!m_repeat_mapping && m_attractors == 2)
            {
                found_attractor = 2;
            }
            else if (m_repeat_mapping && m_attractors == 3)
            {
                found_attractor = 3;
            }
            else if (g_old_z.x >= m_right_x3)
            {
                if (!m_repeat_mapping)
                {
                    found_attractor = 2;
                }
                else
                {
                    found_attractor = 4;
                }
            }
            else if (g_old_z.x <= m_right_x4)
            {
                if (!m_repeat_mapping)
                {
                    found_attractor = 3;
                }
                else
                {
                    found_attractor = 6;
                }
            }
        }
        else if (std::abs(-FROTH_SLOPE * g_old_z.x - m_a - g_old_z.y) < FROTH_CLOSE &&
            g_old_z.x <= m_left_x1 && g_old_z.x >= m_left_x2)
        {
            if (!m_repeat_mapping && m_attractors == 2)
            {
                found_attractor = 2;
            }
            else if (m_repeat_mapping && m_attractors == 3)
            {
                found_attractor = 2;
            }
            else if (g_old_z.x >= m_left_x3)
            {
                if (!m_repeat_mapping)
                {
                    found_attractor = 3;
                }
                else
                {
                    found_attractor = 5;
                }
            }
            else if (g_old_z.x <= m_left_x4)
            {
                if (!m_repeat_mapping)
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

    // inside - Here's where non-palette based images would be nice.  Instead,
    // we'll use blocks of (colors-1)/3 or (colors-1)/6 and use special froth
    // color maps in attempt to replicate the images of James Alexander.
    if (found_attractor)
    {
        if (g_colors >= 256)
        {
            if (!m_alt_color)
            {
                g_color_iter = std::min<long>(g_color_iter, m_shades);
            }
            else
            {
                g_color_iter = m_shades * g_color_iter / g_max_iterations;
            }
            if (g_color_iter == 0)
            {
                g_color_iter = 1;
            }
            g_color_iter += m_shades * (found_attractor-1);
        }
        else if (g_colors >= 16)
        {
            // only alternate coloring scheme available for 16 colors

            // Trying to make a better 16 color distribution.
            // Since there are only a few possibilities, just handle each case.
            // This is a guess work here.
            const long l_shade = (g_color_iter << 16) / g_max_iterations;
            if (m_attractors != 6) // either 2 or 3 attractors
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

    g_color = std::abs(g_color_iter);

    g_plot(g_col, g_row, g_color);

    return g_color;
}

bool froth_per_image()
{
    return g_frothy_basin.per_image();
}

// These last two froth functions are for the orbit-in-window feature.
// Normally, this feature requires standard_fractal, but since it is the
// attractor that makes the frothybasin type so unique, it is worth
// putting in as a stand-alone.

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
    return g_frothy_basin.orbit();
}

} // namespace id::fractals
