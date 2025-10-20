// SPDX-License-Identifier: GPL-3.0-only
//
#include "fractals/Diffusion.h"

#include "engine/calcfrac.h"
#include "engine/LogicalScreen.h"
#include "engine/random_seed.h"
#include "engine/resume.h"
#include "engine/VideoInfo.h"
#include "math/fpu087.h"
#include "misc/id.h"
#include "ui/video.h"

#include <algorithm>
#include <cstdlib>

//**************** standalone engine for "diffusion" *******************

using namespace id::engine;
using namespace id::math;
using namespace id::ui;

namespace id::fractals
{

static int random(const int x)
{
    return std::rand() % x;
}

bool Diffusion::keyboard_check_needed()
{
    return (++m_kbd_check & 0x7f) == 1;
}

Diffusion::Diffusion() :
    m_border(std::max(static_cast<int>(g_params[0]), 10)),
    m_mode(std::clamp(static_cast<DiffusionMode>(g_params[1]), DiffusionMode::CENTRAL, DiffusionMode::SQUARE_CAVITY)),
    m_color_shift(static_cast<int>(g_params[2])),
    m_color_count(static_cast<int>(g_params[2]))
{
    set_random_seed();

    switch (m_mode)
    {
    case DiffusionMode::CENTRAL:
        m_x_max = g_logical_screen.x_dots / 2 + m_border; // Initial box
        m_x_min = g_logical_screen.x_dots / 2 - m_border;
        m_y_max = g_logical_screen.y_dots / 2 + m_border;
        m_y_min = g_logical_screen.y_dots / 2 - m_border;
        break;

    case DiffusionMode::FALLING_PARTICLES:
        m_x_max = g_logical_screen.x_dots / 2 + m_border; // Initial box
        m_x_min = g_logical_screen.x_dots / 2 - m_border;
        m_y_min = g_logical_screen.y_dots - m_border;
        break;

    case DiffusionMode::SQUARE_CAVITY:
        if (g_logical_screen.x_dots > g_logical_screen.y_dots)
        {
            m_radius = static_cast<float>(g_logical_screen.y_dots - m_border);
        }
        else
        {
            m_radius = static_cast<float>(g_logical_screen.x_dots - m_border);
        }
        break;
    }

    if (g_resuming) // restore work list, if we can't the above will stay in place
    {
        start_resume();
        if (m_mode != DiffusionMode::SQUARE_CAVITY)
        {
            get_resume(m_x_max, m_x_min, m_y_max, m_y_min);
        }
        else
        {
            get_resume(m_x_max, m_x_min, m_y_max, m_radius);
        }
        end_resume();
    }

    switch (m_mode)
    {
    case DiffusionMode::CENTRAL:
        g_put_color(g_logical_screen.x_dots / 2, g_logical_screen.y_dots / 2, m_current_color);
        break;

    case DiffusionMode::FALLING_PARTICLES:
        for (int i = 0; i <= g_logical_screen.x_dots; i++)
        {
            g_put_color(i, g_logical_screen.y_dots - 1, m_current_color);
        }
        break;

    case DiffusionMode::SQUARE_CAVITY:
        if (g_logical_screen.x_dots > g_logical_screen.y_dots)
        {
            for (int i = 0; i < g_logical_screen.y_dots; i++)
            {
                g_put_color(g_logical_screen.x_dots / 2 - g_logical_screen.y_dots / 2, i, m_current_color);
                g_put_color(g_logical_screen.x_dots / 2 + g_logical_screen.y_dots / 2, i, m_current_color);
                g_put_color(g_logical_screen.x_dots / 2 - g_logical_screen.y_dots / 2 + i, 0, m_current_color);
                g_put_color(g_logical_screen.x_dots / 2 - g_logical_screen.y_dots / 2 + i,
                    g_logical_screen.y_dots - 1, m_current_color);
            }
        }
        else
        {
            for (int i = 0; i < g_logical_screen.x_dots; i++)
            {
                g_put_color(0, g_logical_screen.y_dots / 2 - g_logical_screen.x_dots / 2 + i, m_current_color);
                g_put_color(g_logical_screen.x_dots - 1,
                    g_logical_screen.y_dots / 2 - g_logical_screen.x_dots / 2 + i, m_current_color);
                g_put_color(i, g_logical_screen.y_dots / 2 - g_logical_screen.x_dots / 2, m_current_color);
                g_put_color(i, g_logical_screen.y_dots / 2 + g_logical_screen.x_dots / 2, m_current_color);
            }
        }
        break;
    }
}

void Diffusion::release_new_particle()
{
    if (!m_particle_needed)
    {
        return;
    }

    switch (m_mode)
    {
    case DiffusionMode::CENTRAL:
    {
        // Release new point on a circle inside the box
        const double angle = 2 * static_cast<double>(std::rand()) / (RAND_MAX / PI);
        double cosine;
        double sine;
        sin_cos(angle, &sine, &cosine);
        m_x = static_cast<int>(cosine * (m_x_max - m_x_min) + g_logical_screen.x_dots);
        m_y = static_cast<int>(sine * (m_y_max - m_y_min) + g_logical_screen.y_dots);
        m_x /= 2;
        m_y /= 2;
        break;
    }

    case DiffusionMode::FALLING_PARTICLES:
        // Release new point on the line ymin somewhere between xmin and xmax
        m_y = m_y_min;
        m_x = random(m_x_max - m_x_min) + (g_logical_screen.x_dots - m_x_max + m_x_min) / 2;
        break;

    case DiffusionMode::SQUARE_CAVITY:
    {
        // Release new point on a circle inside the box with radius given by the radius variable
        const double angle = 2 * static_cast<double>(std::rand()) / (RAND_MAX / PI);
        double cosine;
        double sine;
        sin_cos(angle, &sine, &cosine);
        m_x = static_cast<int>(cosine * m_radius + g_logical_screen.x_dots);
        m_y = static_cast<int>(sine * m_radius + g_logical_screen.y_dots);
        m_x /= 2;
        m_y /= 2;
        break;
    }
    }

    m_particle_needed = false;
}

void Diffusion::suspend()
{
    alloc_resume(20, 1);
    if (m_mode != DiffusionMode::SQUARE_CAVITY)
    {
        put_resume(m_x_max, m_x_min, m_y_max, m_y_min);
    }
    else
    {
        put_resume(m_x_max, m_x_min, m_y_max, m_radius);
    }
}

bool Diffusion::iterate()
{
    release_new_particle();
    if (move_particle())
    {
        return true;
    }
    color_particle();
    return !adjust_limits();
}

bool Diffusion::move_particle()
{
    // Loop as long as the point (x,y) is surrounded by color 0 on all eight sides
    while (get_color(m_x + 1, m_y + 1) == 0 && get_color(m_x + 1, m_y) == 0 && get_color(m_x + 1, m_y - 1) == 0 //
        && get_color(m_x, m_y + 1) == 0 && get_color(m_x, m_y - 1) == 0 && get_color(m_x - 1, m_y + 1) == 0     //
        && get_color(m_x - 1, m_y) == 0 && get_color(m_x - 1, m_y - 1) == 0)
    {
        // Erase moving point
        if (g_show_orbit)
        {
            g_put_color(m_x, m_y, 0);
        }

        if (m_mode == DiffusionMode::CENTRAL)
        {
            // Make sure point is inside the box
            if (m_x == m_x_max)
            {
                m_x--;
            }
            else if (m_x == m_x_min)
            {
                m_x++;
            }
            if (m_y == m_y_max)
            {
                m_y--;
            }
            else if (m_y == m_y_min)
            {
                m_y++;
            }
        }

        // Make sure point is on the screen below ymin, but
        // we need a 1 pixel margin because of the next random step.
        if (m_mode == DiffusionMode::FALLING_PARTICLES)
        {
            if (m_x >= g_logical_screen.x_dots - 1)
            {
                m_x--;
            }
            else if (m_x <= 1)
            {
                m_x++;
            }
            if (m_y < m_y_min)
            {
                m_y++;
            }
        }

        // Take one random step
        m_x += random(3) - 1;
        m_y += random(3) - 1;

        // Show the moving point
        if (g_show_orbit)
        {
            g_put_color(m_x, m_y, random(g_colors - 1) + 1);
        }

        if (keyboard_check_needed())
        {
            return true;
        }
    }

    m_particle_needed = true;

    return false;
}

void Diffusion::color_particle()
{
    // If we're doing color shifting then check to see if we need to shift
    if (m_color_shift)
    {
        // If we're doing color shifting then use current color
        g_put_color(m_x, m_y, m_current_color);
        if (!--m_color_count)
        {
            // If the counter reaches zero then shift
            m_current_color++;           // Increase the current color and wrap
            m_current_color %= g_colors; // around skipping zero
            if (!m_current_color)
            {
                m_current_color++;
            }
            m_color_count = m_color_shift; // and reset the counter
        }
    }
    else
    {
        // pick a color at random
        g_put_color(m_x, m_y, random(g_colors - 1) + 1);
    }
}

// If the new point is close to an edge, we may need to increase
// some limits so that the limits expand to match the growing
// fractal.
bool Diffusion::adjust_limits()
{
    switch (m_mode)
    {
    case DiffusionMode::CENTRAL:
        if (m_x + m_border > m_x_max ||
            m_x - m_border < m_x_min ||
            m_y - m_border < m_y_min ||
            m_y + m_border > m_y_max)
        {
            // Increase box size, but not past the edge of the screen
            m_y_min--;
            m_y_max++;
            m_x_min--;
            m_x_max++;
            if (m_y_min == 0 || m_x_min == 0)
            {
                return true;
            }
        }
        break;

    case DiffusionMode::FALLING_PARTICLES:
        // Decrease ymin, but not past top of screen
        if (m_y - m_border < m_y_min)
        {
            m_y_min--;
        }
        if (m_y_min == 0)
        {
            return true;
        }
        break;

    case DiffusionMode::SQUARE_CAVITY:
    {
        // Decrease the radius where points are released to stay away
        // from the fractal.  It might be decreased by 1 or 2
        const double r = sqr(static_cast<float>(m_x) - g_logical_screen.x_dots / 2) +
            sqr(static_cast<float>(m_y) - g_logical_screen.y_dots / 2);
        if (r <= m_border * m_border)
        {
            return true;
        }
        while ((m_radius - m_border) * (m_radius - m_border) > r)
        {
            m_radius--;
        }
        break;
    }
    }
    return false;
}

} // namespace id::fractals
