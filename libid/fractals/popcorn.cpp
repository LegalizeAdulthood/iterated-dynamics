// SPDX-License-Identifier: GPL-3.0-only
//
#include "fractals/popcorn.h"

#include "engine/calcfrac.h"
#include "engine/fractals.h"
#include "engine/log_map.h"
#include "engine/orbit.h"
#include "engine/resume.h"
#include "engine/trig_fns.h"
#include "engine/VideoInfo.h"
#include "fractals/fractype.h"
#include "math/arg.h"
#include "math/fixed_pt.h"
#include "math/fpu087.h"
#include "misc/debug_flags.h"
#include "misc/version.h"

#include <algorithm>
#include <cassert>
#include <cmath>

using namespace id::engine;
using namespace id::math;
using namespace id::misc;

namespace id::fractals
{

bool popcorn_uses_default_functions()
{
    return g_trig_index[0] == TrigFn::SIN       //
        && g_trig_index[1] == TrigFn::TAN       //
        && g_trig_index[2] == TrigFn::SIN       //
        && g_trig_index[3] == TrigFn::TAN       //
        && std::abs(g_param_z2.x - 3.0) < .0001 //
        && g_param_z2.y == 0                    //
        && g_param_z1.y == 0;
}

Popcorn::Popcorn() = default;

Popcorn::~Popcorn() = default;

void Popcorn::resume()
{
    int start_row = 0;
    if (g_resuming)
    {
        start_resume();
        get_resume(start_row);
        end_resume();
    }
    g_keyboard_check_interval = g_max_keyboard_check_interval;
    g_temp_sqr_x = 0;
    m_map_mode = select_map_mode();
    m_render_mode = select_render_mode();
    if (m_render_mode == RenderMode::IMAGE_ORBITS)
    {
        g_plot = no_plot;
    }
    m_color_iter = 0;
    m_orbit_step_result = 0;
    m_image_orbit_plot_pending = false;
    m_orbit_step_completed = false;
    m_row = start_row;
    m_col = 0;
    m_seed_active = false;
    m_done = false;
    if (m_row > g_i_stop_pt.y)
    {
        complete();
    }
}

void Popcorn::suspend()
{
    alloc_resume(10, 1);
    put_resume(m_row);
    m_done = true;
}

bool Popcorn::done() const
{
    return m_done;
}

void Popcorn::iterate()
{
    if (m_done || m_image_orbit_plot_pending)
    {
        return;
    }

    if (!m_seed_active)
    {
        start_seed();
    }
    if (m_done)
    {
        return;
    }

    if (m_orbit_step_completed)
    {
        if (m_orbit_step_result != 0)
        {
            advance_seed();
            return;
        }
        m_orbit_step_result = 0;
        m_orbit_step_completed = false;
    }

    ++m_color_iter;
    g_color_iter = m_color_iter;
    if (g_color_iter >= g_max_iterations)
    {
        advance_seed();
        return;
    }

    m_orbit_step_result = orbit_step();
    m_orbit_step_completed = true;
    if (m_image_orbit_plot_pending)
    {
        return;
    }
    if (m_orbit_step_result != 0)
    {
        advance_seed();
        return;
    }
    m_orbit_step_result = 0;
    m_orbit_step_completed = false;
}

void Popcorn::advance_seed()
{
    g_real_color_iter = m_color_iter;
    if (m_render_mode == RenderMode::ESCAPE_TIME)
    {
        plot_escape_time_seed();
    }
    g_reset_periodicity = false;
    m_seed_active = false;
    m_orbit_step_completed = false;
    m_orbit_step_result = 0;
    ++m_col;
    g_col = m_col;
    if (m_col > g_i_stop_pt.x)
    {
        ++m_row;
        g_row = m_row;
        if (m_row > g_i_stop_pt.y)
        {
            complete();
        }
        else
        {
            m_col = 0;
        }
    }
}

void Popcorn::start_seed()
{
    g_row = m_row;
    g_col = m_col;
    g_reset_periodicity = m_col == 0;
    g_color_iter = 0;
    m_color_iter = 0;
    m_orbit_step_result = 0;
    m_orbit_step_completed = false;
    other_julia_per_pixel();
    m_seed_active = true;
}

bool Popcorn::image_orbit_plot_pending() const
{
    return m_image_orbit_plot_pending;
}

OrbitPlot &Popcorn::pending_image_orbit_plot()
{
    assert(m_image_orbit_plot_pending);
    return m_image_orbit_plot;
}

void Popcorn::complete_pending_image_orbit_plot()
{
    assert(m_image_orbit_plot_pending);
    assert(m_image_orbit_plot.done());
    m_image_orbit_plot_pending = false;
}

void Popcorn::queue_image_orbit_plot(const double real, const double imag, const int color)
{
    if (!m_image_orbit_plot_pending)
    {
        m_image_orbit_plot.reset_image(real, imag, color);
        m_image_orbit_plot_pending = true;
    }
}

void Popcorn::setup_julia_attractor(const double real, const double imag)
{
    if (g_attractor.count == 0 && !g_attractor.enabled)
    {
        return;
    }
    if (g_attractor.count >= MAX_NUM_ATTRACTORS)
    {
        return;
    }

    Popcorn popcorn;
    popcorn.m_map_mode = select_map_mode();
    popcorn.m_render_mode = RenderMode::ESCAPE_TIME;

    const int save_periodicity_check{g_periodicity_check};
    const long save_max_iterations{g_max_iterations};
    g_periodicity_check = 0;
    g_old_z.x = real;
    g_old_z.y = imag;
    g_temp_sqr_x = sqr(g_old_z.x);
    g_temp_sqr_y = sqr(g_old_z.y);

    g_max_iterations = std::max(g_max_iterations, 500L);
    g_color_iter = 0;
    g_overflow = false;
    while (++g_color_iter < g_max_iterations)
    {
        if (popcorn.orbit_step() || g_overflow)
        {
            break;
        }
    }
    if (g_color_iter >= g_max_iterations)
    {
        const DComplex prev_z{g_new_z};
        for (int i = 0; i < 10; i++)
        {
            g_overflow = false;
            if (!popcorn.orbit_step() && !g_overflow)
            {
                if (std::abs(prev_z.x - g_new_z.x) < g_close_enough && std::abs(prev_z.y - g_new_z.y) < g_close_enough)
                {
                    g_attractor.z[g_attractor.count] = g_new_z;
                    g_attractor.period[g_attractor.count] = i + 1;
                    g_attractor.count++;
                    break;
                }
            }
            else
            {
                break;
            }
        }
    }
    if (g_attractor.count == 0)
    {
        g_periodicity_check = save_periodicity_check;
    }
    g_max_iterations = save_max_iterations;
}

void Popcorn::complete()
{
    g_calc_status = CalcStatus::COMPLETED;
    m_done = true;
}

int Popcorn::legacy_real_orbit_step()
{
    g_tmp_z = g_old_z;
    g_tmp_z.x *= 3.0;
    g_tmp_z.y *= 3.0;
    sin_cos(g_tmp_z.x, g_sin_x, g_cos_x);
    double sin_y;
    double cos_y;
    sin_cos(g_tmp_z.y, sin_y, cos_y);
    g_tmp_z.x = g_sin_x / g_cos_x + g_old_z.x;
    g_tmp_z.y = sin_y / cos_y + g_old_z.y;
    sin_cos(g_tmp_z.x, g_sin_x, g_cos_x);
    sin_cos(g_tmp_z.y, sin_y, cos_y);
    g_new_z.x = g_old_z.x - g_param_z1.x * sin_y;
    g_new_z.y = g_old_z.y - g_param_z1.x * g_sin_x;
    if (m_render_mode == RenderMode::IMAGE_ORBITS)
    {
        queue_image_orbit_plot(g_new_z.x, g_new_z.y, 1 + g_row % g_colors);
        g_old_z = g_new_z;
    }
    else
    {
        g_temp_sqr_x = sqr(g_new_z.x);
    }
    g_temp_sqr_y = sqr(g_new_z.y);
    g_magnitude = g_temp_sqr_x + g_temp_sqr_y;
    if (g_magnitude >= g_magnitude_limit)
    {
        return 1;
    }
    g_old_z = g_new_z;
    return 0;
}

int Popcorn::current_real_orbit_step()
{
    g_tmp_z = g_old_z;
    g_tmp_z.x *= 3.0;
    g_tmp_z.y *= 3.0;
    sin_cos(g_tmp_z.x, g_sin_x, g_cos_x);
    double sin_y;
    double cos_y;
    sin_cos(g_tmp_z.y, sin_y, cos_y);
    g_tmp_z.x = g_sin_x / g_cos_x + g_old_z.x;
    g_tmp_z.y = sin_y / cos_y + g_old_z.y;
    sin_cos(g_tmp_z.x, g_sin_x, g_cos_x);
    sin_cos(g_tmp_z.y, sin_y, cos_y);
    g_new_z.x = g_old_z.x - g_param_z1.x * sin_y;
    g_new_z.y = g_old_z.y - g_param_z1.x * g_sin_x;
    if (m_render_mode == RenderMode::IMAGE_ORBITS)
    {
        queue_image_orbit_plot(g_new_z.x, g_new_z.y, 1 + g_row % g_colors);
        g_old_z = g_new_z;
    }
    g_temp_sqr_x = sqr(g_new_z.x);
    g_temp_sqr_y = sqr(g_new_z.y);
    g_magnitude = g_temp_sqr_x + g_temp_sqr_y;
    if (g_magnitude >= g_magnitude_limit || std::abs(g_new_z.x) > g_magnitude_limit2 ||
        std::abs(g_new_z.y) > g_magnitude_limit2)
    {
        return 1;
    }
    g_old_z = g_new_z;
    return 0;
}

// Popcorn generalization

int Popcorn::generalized_orbit_step()
{
    DComplex tmp_x;
    DComplex tmp_y;

    // tmpx contains the generalized value of the old real "x" equation
    g_tmp_z = g_param_z2 * g_old_z.y;       // tmp = (C * old.y)
    cmplx_trig1(g_tmp_z, tmp_x);            // tmpx = trig1(tmp)
    tmp_x.x += g_old_z.y;                   // tmpx = old.y + trig1(tmp)
    cmplx_trig0(tmp_x, g_tmp_z);            // tmp = trig0(tmpx)
    cmplx_mult(g_tmp_z, g_param_z1, tmp_x); // tmpx = tmp * h

    // tmpy contains the generalized value of the old real "y" equation
    g_tmp_z = g_param_z2 * g_old_z.x;       // tmp = (C * old.x)
    cmplx_trig3(g_tmp_z, tmp_y);            // tmpy = trig3(tmp)
    tmp_y.x += g_old_z.x;                   // tmpy = old.x + trig1(tmp)
    cmplx_trig2(tmp_y, g_tmp_z);            // tmp = trig2(tmpy)
    cmplx_mult(g_tmp_z, g_param_z1, tmp_y); // tmpy = tmp * h

    g_new_z.x = g_old_z.x - tmp_x.x - tmp_y.y;
    g_new_z.y = g_old_z.y - tmp_y.x - tmp_x.y;

    if (m_render_mode == RenderMode::IMAGE_ORBITS)
    {
        queue_image_orbit_plot(g_new_z.x, g_new_z.y, 1 + g_row % g_colors);
        g_old_z = g_new_z;
    }

    g_temp_sqr_x = sqr(g_new_z.x);
    g_temp_sqr_y = sqr(g_new_z.y);
    g_magnitude = g_temp_sqr_x + g_temp_sqr_y;
    if (g_magnitude >= g_magnitude_limit || std::abs(g_new_z.x) > g_magnitude_limit2 ||
        std::abs(g_new_z.y) > g_magnitude_limit2)
    {
        return 1;
    }
    g_old_z = g_new_z;
    return 0;
}

Popcorn::MapMode Popcorn::select_map_mode()
{
    if (g_version <= parse_legacy_version(1960))
    {
        return MapMode::LEGACY_REAL;
    }
    if (popcorn_uses_default_functions() && g_debug_flag == DebugFlags::FORCE_REAL_POPCORN)
    {
        return MapMode::CURRENT_REAL;
    }
    return MapMode::GENERALIZED;
}

Popcorn::RenderMode Popcorn::select_render_mode()
{
    return g_fractal_type == FractalType::POPCORN_JUL ? RenderMode::ESCAPE_TIME : RenderMode::IMAGE_ORBITS;
}

void Popcorn::plot_escape_time_seed()
{
    g_color_iter = m_color_iter;
    if (g_color_iter >= g_max_iterations)
    {
        g_color_iter = g_inside_method >= ColorMethod::COLOR ? g_inside_color : g_max_iterations;
    }
    else if (g_outside_method >= ColorMethod::COLOR)
    {
        g_color_iter = g_outside_color;
    }
    else if (!g_log_map_table.empty() || g_log_map_calculate)
    {
        g_color_iter = log_table_calc(g_color_iter);
    }

    g_color = std::abs(g_color_iter);
    if (g_color_iter >= g_colors)
    {
        if (g_colors < 16)
        {
            g_color = static_cast<int>(g_color_iter & g_and_color);
        }
        else
        {
            g_color = static_cast<int>((g_color_iter - 1) % g_and_color + 1);
        }
    }
    g_plot(g_col, g_row, g_color);
}

int Popcorn::orbit_step()
{
    switch (m_map_mode)
    {
    case MapMode::LEGACY_REAL:
        return legacy_real_orbit_step();

    case MapMode::CURRENT_REAL:
        return current_real_orbit_step();

    case MapMode::GENERALIZED:
        return generalized_orbit_step();
    }
    return generalized_orbit_step();
}

} // namespace id::fractals
