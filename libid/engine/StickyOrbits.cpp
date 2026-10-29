// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "engine/StickyOrbits.h"

#include "engine/calcfrac.h"
#include "engine/convert_center_mag.h"
#include "engine/fractals.h"
#include "engine/LogicalScreen.h"
#include "engine/OrbitPlot.h"
#include "engine/PertEngine.h"
#include "engine/resume.h"
#include "engine/SoundOutput.h"
#include "engine/VideoInfo.h"
#include "engine/work_list.h"
#include "fractals/fractalp.h"
#include "fractals/lorenz.h"
#include "misc/id.h"
#include "misc/run_cleanup.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdlib>
#include <exception>
#include <stdexcept>

using namespace id::fractals;

namespace id::engine
{

namespace
{

constexpr long ORBIT_ITERATION_CHUNK = 2048;

} // namespace

OrbitDrawMode g_draw_mode{OrbitDrawMode::RECTANGLE};

long sticky_orbit_chunk_limit(const long count, const long max_iterations)
{
    return count + std::min(ORBIT_ITERATION_CHUNK, max_iterations - count);
}

void StickyOrbits::initialize_rectangle()
{
    m_row = g_begin_pt.y;
    m_col = g_begin_pt.x;
    m_traversal = Traversal::RECTANGLE;
}

void StickyOrbits::initialize_line()
{
    const int resume_accumulator{m_line.accumulator};
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
    // A zero pass identifies traversal that has not started yet.
    if (m_line.resuming && g_work_pass != 0)
    {
        m_line.accumulator = resume_accumulator;
    }
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

void StickyOrbits::start_orbit(PertEngine *perturbation)
{
    select_orbit_color();
    per_pixel();
    if (perturbation != nullptr)
    {
        const bool burning_ship{g_fractal_type == FractalType::BURNING_SHIP};
        const bool conjugate_value{burning_ship && static_cast<int>(g_params[2]) > 2};
        if (math::g_bf_math == math::BFMathType::BIG_NUM)
        {
            perturbation->begin_orbit_bn(math::g_param_z_bn, math::g_old_z_bn, 0);
        }
        else if (math::g_bf_math == math::BFMathType::BIG_FLT)
        {
            if (burning_ship)
            {
                math::neg_a_bf(math::g_param_z_bf.y);
            }
            if (conjugate_value)
            {
                math::neg_a_bf(math::g_old_z_bf.y);
            }
            std::exception_ptr failure;
            try
            {
                perturbation->begin_orbit_bf(math::g_param_z_bf, math::g_old_z_bf, 0);
            }
            catch (...)
            {
                failure = std::current_exception();
            }
            misc::run_cleanup(
                failure,
                [burning_ship]
                {
                    if (burning_ship)
                    {
                        math::neg_a_bf(math::g_param_z_bf.y);
                    }
                },
                [conjugate_value]
                {
                    if (conjugate_value)
                    {
                        math::neg_a_bf(math::g_old_z_bf.y);
                    }
                });
        }
        else
        {
            if (g_float_param == nullptr)
            {
                throw std::logic_error("Sticky perturbation requires an initialized recurrence parameter");
            }
            const std::complex<double> parameter{g_float_param->x, burning_ship ? -g_float_param->y : g_float_param->y};
            const std::complex<double> value{g_old_z.x, conjugate_value ? -g_old_z.y : g_old_z.y};
            perturbation->begin_orbit(parameter, value, 0);
        }
    }
    m_orbit.count = 0;
    m_orbit.active = true;
    m_orbit.point_pending = false;
    m_orbit.escaped = false;
}

bool StickyOrbits::advance_orbit(PertEngine *perturbation)
{
    if (!m_orbit.active)
    {
        start_orbit(perturbation);
    }

    const long stop_count = sticky_orbit_chunk_limit(m_orbit.count, g_max_iterations);
    for (; m_orbit.count < stop_count; ++m_orbit.count)
    {
        if (!m_orbit.point_pending)
        {
            if (perturbation != nullptr)
            {
                if (!m_orbit.escaped || g_fractal_type == FractalType::MANDEL)
                {
                    const long before{perturbation->orbit_iteration()};
                    perturbation->iterate_orbit(g_fractal_type == FractalType::MANDEL);
                    if (perturbation->orbit_iteration() == before)
                    {
                        return false;
                    }
                    const std::complex<double> value{perturbation->orbit_value()};
                    m_orbit.point = {value.real(), value.imag()};
                    if (g_fractal_type == FractalType::BURNING_SHIP && static_cast<int>(g_params[2]) > 2)
                    {
                        m_orbit.point.y = -m_orbit.point.y;
                    }
                    m_orbit.escaped = perturbation->orbit_escaped();
                }
                if (m_orbit.escaped && g_periodicity_check)
                {
                    continue;
                }
            }
            else if (orbit_calc() == 1 && g_periodicity_check)
            {
                continue;
            }
            else
            {
                m_orbit.point = g_new_z;
            }

            if (m_orbit.count < m_orbit.resume_count || m_orbit.count < g_orbit_skip_points ||
                m_orbit.count % g_orbit_interval)
            {
                continue;
            }

            m_orbit.col = static_cast<int>(m_orbit.screen_convert.a * m_orbit.point.x +
                m_orbit.screen_convert.b * m_orbit.point.y + m_orbit.screen_convert.e);
            m_orbit.row = static_cast<int>(m_orbit.screen_convert.c * m_orbit.point.x +
                m_orbit.screen_convert.d * m_orbit.point.y + m_orbit.screen_convert.f);
            if (m_orbit.col <= 0 || m_orbit.col >= g_logical_screen.x_dots || m_orbit.row <= 0 ||
                m_orbit.row >= g_logical_screen.y_dots)
            {
                if (g_periodicity_check)
                {
                    m_orbit.active = false;
                    m_orbit.resume_count = 0;
                    if (perturbation != nullptr)
                    {
                        perturbation->finish_orbit();
                    }
                    return true;
                }
                continue;
            }
            m_orbit.point_pending = true;
        }
        if (!g_sound->play({m_orbit.point.x, m_orbit.point.y, 0.0}))
        {
            return false;
        }
        g_plot(m_orbit.col, m_orbit.row, m_orbit.color % g_colors);
        m_orbit.point_pending = false;
    }

    if (m_orbit.count < g_max_iterations)
    {
        return false;
    }

    m_orbit.active = false;
    m_orbit.resume_count = 0;
    if (perturbation != nullptr)
    {
        perturbation->finish_orbit();
    }
    return true;
}

bool StickyOrbits::draw_rectangle(PertEngine *perturbation)
{
    if (m_row > g_i_stop_pt.y)
    {
        m_traversal = Traversal::DONE;
        return true;
    }

    g_current_row = m_row;
    g_row = m_row;
    g_col = m_col;
    if (!advance_orbit(perturbation))
    {
        return false;
    }

    advance_after_seed();
    return done();
}

bool StickyOrbits::draw_line(PertEngine *perturbation)
{
    if (line_done())
    {
        m_traversal = Traversal::DONE;
        return true;
    }

    g_row = m_row;
    g_col = m_col;
    if (!advance_orbit(perturbation))
    {
        return false;
    }

    advance_after_seed();
    return done();
}

bool StickyOrbits::draw_function(PertEngine *perturbation)
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
    if (!advance_orbit(perturbation))
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

bool StickyOrbits::iterate(PertEngine *perturbation)
{
    if (done())
    {
        return true;
    }
    if (!m_initialized)
    {
        initialize();
    }

    switch (m_traversal)
    {
    case Traversal::RECTANGLE:
        return draw_rectangle(perturbation);

    case Traversal::LINE:
        return draw_line(perturbation);

    case Traversal::FUNCTION:
        return draw_function(perturbation);

    case Traversal::START:
    case Traversal::DONE:
    default:
        return true;
    }
}

int StickyOrbits::encoded_orbit_color() const
{
    const int color = m_orbit.color + 1;
    return m_orbit.active ? color : -color;
}

void StickyOrbits::save_current_work()
{
    WorkList work{g_start_pt, g_stop_pt, g_begin_pt, g_work_symmetry, g_work_pass};
    if (m_initialized)
    {
        work.begin = {m_col, m_row};
        work.pass = encoded_orbit_color();
    }
    switch (m_traversal)
    {
    case Traversal::RECTANGLE:
    case Traversal::LINE:
        break;

    case Traversal::FUNCTION:
        work.begin = {m_function.angle, 0};
        break;

    case Traversal::START:
        break;

    case Traversal::DONE:
    default:
        return;
    }
    if (g_num_work_list >= MAX_CALC_WORK)
    {
        throw std::logic_error{"No room to suspend sticky orbit work"};
    }
    // The resume count belongs to this seed, before any other queued work.
    std::move_backward(g_work_list, g_work_list + g_num_work_list, g_work_list + g_num_work_list + 1);
    g_work_list[0] = work;
    ++g_num_work_list;
}

void StickyOrbits::suspend()
{
    if (done())
    {
        return;
    }

    save_current_work();
    if (m_initialized)
    {
        m_orbit.resume_count = m_orbit.active ? std::max(m_orbit.resume_count, m_orbit.count) : 0;
    }
    m_traversal = Traversal::DONE;
    m_orbit.active = false;
}

void StickyOrbits::save_resume() const
{
    put_resume(m_orbit.resume_count);
    put_resume(m_line.accumulator);
}

void StickyOrbits::restore_resume(const int version)
{
    get_resume(m_orbit.resume_count);
    if (version >= 5)
    {
        get_resume(m_line.accumulator);
        m_line.resuming = true;
    }
    if (m_orbit.resume_count < 0)
    {
        throw std::logic_error{"Invalid sticky orbit resume count"};
    }
}

} // namespace id::engine
