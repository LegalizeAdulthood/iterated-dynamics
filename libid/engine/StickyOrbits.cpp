// SPDX-License-Identifier: GPL-3.0-only
//
#include "engine/StickyOrbits.h"

#include "engine/calcfrac.h"
#include "engine/convert_center_mag.h"
#include "engine/LogicalScreen.h"
#include "engine/orbit.h"
#include "engine/sound.h"
#include "engine/VideoInfo.h"
#include "engine/work_list.h"
#include "fractals/fractalp.h"
#include "fractals/lorenz.h"
#include "misc/id.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

using namespace id::fractals;

namespace id::engine
{

namespace
{

constexpr long ORBIT_ITERATION_CHUNK = 2048;

} // namespace

OrbitDrawMode g_draw_mode{OrbitDrawMode::RECTANGLE};

void StickyOrbits::initialize_rectangle()
{
    m_row = g_begin_pt.y;
    m_col = g_begin_pt.x;
    m_traversal = Traversal::RECTANGLE;
}

void StickyOrbits::initialize_line()
{
    const int dx = g_i_stop_pt.x - g_i_start_pt.x;
    const int dy = g_i_stop_pt.y - g_i_start_pt.y;
    bool pos_slope = dx > 0;
    if (dy < 0)
    {
        pos_slope = !pos_slope;
    }

    if (std::abs(dx) > std::abs(dy))
    {
        if (dx > 0)
        {
            m_col = g_begin_pt.x;
            m_row = g_begin_pt.y;
            m_line.final = g_i_stop_pt.x;
        }
        else
        {
            m_col = g_i_stop_pt.x;
            m_row = g_i_stop_pt.y;
            m_line.final = g_begin_pt.x;
        }
        m_line.inc1 = 2 * std::abs(dy);
        m_line.accumulator = m_line.inc1 - std::abs(dx);
        m_line.inc2 = 2 * (std::abs(dy) - std::abs(dx));
        m_line.shallow = true;
    }
    else
    {
        if (dy > 0)
        {
            m_col = g_begin_pt.x;
            m_row = g_begin_pt.y;
            m_line.final = g_i_stop_pt.y;
        }
        else
        {
            m_col = g_i_stop_pt.x;
            m_row = g_i_stop_pt.y;
            m_line.final = g_begin_pt.y;
        }
        m_line.inc1 = 2 * std::abs(dx);
        m_line.accumulator = m_line.inc1 - std::abs(dy);
        m_line.inc2 = 2 * (std::abs(dx) - std::abs(dy));
        m_line.shallow = false;
    }

    m_line.positive_slope = pos_slope;
    m_traversal = Traversal::LINE;
}

// this code does not yet work???
void StickyOrbits::initialize_function()
{
    double x_ctr;
    double y_ctr;
    LDouble magnification;
    double x_mag_factor;
    double rotation;
    double skew;

    cvt_center_mag(x_ctr, y_ctr, magnification, x_mag_factor, rotation, skew);
    if (rotation <= 0)
    {
        rotation += 360;
    }

    m_function.angle = g_begin_pt.x;
    m_function.rotation = rotation;
    m_function.x_center = x_ctr;
    m_function.y_center = y_ctr;
    m_function.x_mag_factor = x_mag_factor;
    m_function.x_factor = g_logical_screen.x_dots / 2.0;
    m_function.y_factor = g_logical_screen.y_dots / 2.0;
    m_traversal = Traversal::FUNCTION;
}

void StickyOrbits::initialize()
{
    m_initialized = true;
    g_passes = Passes::ORBITS;
    g_total_passes = 1;

    if (!plot_orbits2d_setup(&m_orbit.screen_convert))
    {
        g_std_calc_mode = CalcMode::SOLID_GUESS;
        m_interrupted = true;
        m_traversal = Traversal::DONE;
        return;
    }

    if (g_work_pass != 0)
    {
        m_orbit.color = std::abs(g_work_pass) - 1;
        m_orbit.seed_color_selected = g_work_pass > 0;
    }

    switch (g_draw_mode)
    {
    case OrbitDrawMode::RECTANGLE:
    default:
        initialize_rectangle();
        break;

    case OrbitDrawMode::LINE:
        initialize_line();
        break;

    case OrbitDrawMode::FUNCTION:
        initialize_function();
        break;
    }
}

bool StickyOrbits::line_done() const
{
    return m_line.shallow ? m_col > m_line.final : m_row > m_line.final;
}

void StickyOrbits::advance_line()
{
    if (m_line.shallow)
    {
        ++m_col;
        if ((m_line.positive_slope && m_line.accumulator >= 0) || (!m_line.positive_slope && m_line.accumulator > 0))
        {
            m_row += m_line.positive_slope ? 1 : -1;
            m_line.accumulator += m_line.inc2;
        }
        else
        {
            m_line.accumulator += m_line.inc1;
        }
    }
    else
    {
        ++m_row;
        if ((m_line.positive_slope && m_line.accumulator >= 0) || (!m_line.positive_slope && m_line.accumulator > 0))
        {
            m_col += m_line.positive_slope ? 1 : -1;
            m_line.accumulator += m_line.inc2;
        }
        else
        {
            m_line.accumulator += m_line.inc1;
        }
    }
}

void StickyOrbits::advance_after_seed()
{
    switch (m_traversal)
    {
    case Traversal::RECTANGLE:
        ++m_col;
        if (m_col > g_i_stop_pt.x)
        {
            m_col = g_i_start_pt.x;
            ++m_row;
        }
        if (m_row > g_i_stop_pt.y)
        {
            m_traversal = Traversal::DONE;
        }
        break;

    case Traversal::LINE:
        advance_line();
        if (line_done())
        {
            m_traversal = Traversal::DONE;
        }
        break;

    case Traversal::FUNCTION:
        ++m_function.angle;
        if (m_function.angle >= m_function.rotation)
        {
            m_traversal = Traversal::DONE;
        }
        break;

    case Traversal::START:
    case Traversal::DONE:
    default:
        break;
    }
}

void StickyOrbits::select_orbit_color()
{
    if (m_orbit.seed_color_selected)
    {
        m_orbit.seed_color_selected = false;
        return;
    }

    if (g_inside_method > ColorMethod::COLOR)
    {
        m_orbit.color = g_inside_color;
    }
    else
    {
        ++m_orbit.color;
        if (m_orbit.color >= g_colors)
        {
            m_orbit.color = 1;
        }
    }
}

void StickyOrbits::start_orbit()
{
    select_orbit_color();
    per_pixel();
    m_orbit.count = 0;
    m_orbit.active = true;
}

bool StickyOrbits::advance_orbit()
{
    if (!m_orbit.active)
    {
        start_orbit();
    }

    const long stop_count = std::min(g_max_iterations, m_orbit.count + ORBIT_ITERATION_CHUNK);
    for (; m_orbit.count < stop_count; ++m_orbit.count)
    {
        if (orbit_calc() == 1 && g_periodicity_check)
        {
            continue;
        }

        if (m_orbit.count < g_orbit_skip_points || m_orbit.count % g_orbit_interval)
        {
            continue;
        }

        const int col = static_cast<int>(
            m_orbit.screen_convert.a * g_new_z.x + m_orbit.screen_convert.b * g_new_z.y + m_orbit.screen_convert.e);
        const int row = static_cast<int>(
            m_orbit.screen_convert.c * g_new_z.x + m_orbit.screen_convert.d * g_new_z.y + m_orbit.screen_convert.f);
        if (col > 0 && col < g_logical_screen.x_dots && row > 0 && row < g_logical_screen.y_dots)
        {
            if ((g_sound_flag & SOUNDFLAG_ORBIT_MASK) > SOUNDFLAG_BEEP)
            {
                write_sound(g_base_hertz);
            }

            g_plot(col, row, m_orbit.color % g_colors);
        }
        else if (g_periodicity_check)
        {
            m_orbit.active = false;
            return true;
        }
    }

    if (m_orbit.count < g_max_iterations)
    {
        return false;
    }

    m_orbit.active = false;
    return true;
}

bool StickyOrbits::draw_rectangle()
{
    if (m_row > g_i_stop_pt.y)
    {
        m_traversal = Traversal::DONE;
        return true;
    }

    g_current_row = m_row;
    g_row = m_row;
    g_col = m_col;
    if (!advance_orbit())
    {
        return false;
    }

    advance_after_seed();
    return done();
}

bool StickyOrbits::draw_line()
{
    if (line_done())
    {
        m_traversal = Traversal::DONE;
        return true;
    }

    g_row = m_row;
    g_col = m_col;
    if (!advance_orbit())
    {
        return false;
    }

    advance_after_seed();
    return done();
}

bool StickyOrbits::draw_function()
{
    constexpr double FACTOR = PI / 180.0;

    if (m_function.angle >= m_function.rotation)
    {
        m_traversal = Traversal::DONE;
        return true;
    }

    const double theta = static_cast<double>(m_function.angle) * FACTOR;
    g_col = static_cast<int>(m_function.x_factor + (m_function.x_center + m_function.x_mag_factor * std::cos(theta)));
    g_row = static_cast<int>(m_function.y_factor + (m_function.y_center + m_function.x_mag_factor * std::sin(theta)));
    if (!advance_orbit())
    {
        return false;
    }

    advance_after_seed();
    return done();
}

bool StickyOrbits::done() const
{
    return m_traversal == Traversal::DONE;
}

bool StickyOrbits::iterate()
{
    if (!m_initialized)
    {
        initialize();
    }
    if (done())
    {
        return true;
    }

    switch (m_traversal)
    {
    case Traversal::RECTANGLE:
        return draw_rectangle();

    case Traversal::LINE:
        return draw_line();

    case Traversal::FUNCTION:
        return draw_function();

    case Traversal::START:
    case Traversal::DONE:
    default:
        return true;
    }
}

int StickyOrbits::scan()
{
    while (!iterate())
    {
    }

    return m_interrupted ? -1 : 0;
}

int StickyOrbits::encoded_orbit_color() const
{
    const int color = m_orbit.color + 1;
    return m_orbit.active ? color : -color;
}

void StickyOrbits::save_current_work()
{
    const int pass = encoded_orbit_color();
    switch (m_traversal)
    {
    case Traversal::RECTANGLE:
    case Traversal::LINE:
        add_work_list(g_start_pt.x, g_start_pt.y, g_stop_pt.x, g_stop_pt.y, m_col, m_row, pass, g_work_symmetry);
        break;

    case Traversal::FUNCTION:
        add_work_list(m_function.angle, 0, 0, 0, 0, 0, pass, g_work_symmetry);
        break;

    case Traversal::START:
    case Traversal::DONE:
    default:
        break;
    }
}

void StickyOrbits::suspend()
{
    if (!m_initialized || done())
    {
        return;
    }

    save_current_work();
    m_traversal = Traversal::DONE;
    m_orbit.active = false;
}

} // namespace id::engine
