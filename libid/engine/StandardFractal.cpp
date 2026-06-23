// SPDX-License-Identifier: GPL-3.0-only
//
#include "engine/StandardFractal.h"

#include "engine/boundary_trace.h"
#include "engine/calc_frac_init.h"
#include "engine/calcfrac.h"
#include "engine/diffusion_scan.h"
#include "engine/engine_timer.h"
#include "engine/log_map.h"
#include "engine/LogicalScreen.h"
#include "engine/one_or_two_pass.h"
#include "engine/orbit.h"
#include "engine/Potential.h"
#include "engine/resume.h"
#include "engine/soi.h"
#include "engine/solid_guess.h"
#include "engine/sticky_orbits.h"
#include "engine/tesseral.h"
#include "engine/work_list.h"
#include "fractals/fractalp.h"
#include "ui/check_key.h"
#include "ui/diskvid.h"

#include <cassert>
#include <cmath>
#include <ctime>

namespace id::engine
{

long auto_log_map();
void cleanup_standard_fractal_show_dot();
void set_symmetry(SymmetryType sym, bool use_list);
void setup_standard_fractal_distance_estimator();
void setup_standard_fractal_show_dot();

namespace
{

bool calc_type_supports_orbit_mode()
{
    const fractals::CalcType table_calc_type{fractals::g_cur_fractal_specific->calc_type};
    return table_calc_type == standard_fractal_type;
}

CalcMode followup_calc_mode()
{
    return g_logical_screen.x_dots >= 640 ? CalcMode::TWO_PASS : CalcMode::ONE_PASS;
}

} // namespace

void StandardFractal::resume()
{
    m_requested_calc_mode = g_std_calc_mode;
    m_after_work_list = AfterWorkList::COMPLETE;
    m_phase = Phase::START;
    m_dispatch_saved = false;
    m_timer_started = false;
    m_work_list_started = false;
}

void StandardFractal::suspend()
{
    if (g_num_work_list > 0)
    {
        alloc_resume(sizeof(g_work_list) + 20, 2);
        put_resume_len(sizeof(g_num_work_list), &g_num_work_list, sizeof(g_work_list), g_work_list, 0);
    }
    else
    {
        g_calc_status = CalcStatus::COMPLETED;
    }
    complete();
}

bool StandardFractal::done() const
{
    return m_phase == Phase::COMPLETE;
}

void StandardFractal::iterate()
{
    if (done())
    {
        return;
    }

    start_timer();
    if (m_phase == Phase::START)
    {
        start_next_pass();
    }
    if (m_phase == Phase::WORK_LIST)
    {
        run_current_work_item();
    }
    update_timer();
}

void StandardFractal::complete()
{
    if (m_requested_calc_mode == CalcMode::THREE_PASS)
    {
        g_std_calc_mode = m_requested_calc_mode;
    }
    restore_dispatch();
    update_timer();
    m_phase = Phase::COMPLETE;
}

void StandardFractal::finish_work_list()
{
    g_calc_status = CalcStatus::COMPLETED;
    if (m_after_work_list == AfterWorkList::START_THREE_PASS_FOLLOWUP)
    {
        g_std_calc_mode = followup_calc_mode();
        g_three_pass = false;
        m_after_work_list = AfterWorkList::COMPLETE;
        m_work_list_started = false;
        m_phase = Phase::WORK_LIST;
        return;
    }
    complete();
}

void StandardFractal::pop_work_list_front()
{
    assert(g_num_work_list > 0);
    --g_num_work_list;
    for (int i = 0; i < g_num_work_list; ++i)
    {
        g_work_list[i] = g_work_list[i + 1];
    }
}

void StandardFractal::restore_dispatch()
{
    if (m_dispatch_saved)
    {
        fractals::g_dispatch = m_saved_dispatch;
        m_dispatch_saved = false;
    }
}

void StandardFractal::run_current_work_item()
{
    if (!m_work_list_started)
    {
        start_work_list();
    }
    if (g_num_work_list <= 0)
    {
        finish_work_list();
        return;
    }

    fractals::g_dispatch.init_calc_type(*fractals::g_cur_fractal_specific); // per_image can override
    g_symmetry = fractals::g_cur_fractal_specific->symmetry;                // table symmetry
    g_plot = g_put_color; // defaults when set symmetry not called or does nothing

    g_start_pt = g_work_list[0].start;
    g_i_start_pt = g_work_list[0].start;
    g_stop_pt = g_work_list[0].stop;
    g_i_stop_pt = g_work_list[0].stop;
    g_begin_pt = g_work_list[0].begin;
    g_work_pass = g_work_list[0].pass;
    g_work_symmetry = g_work_list[0].symmetry;
    pop_work_list_front();

    g_calc_status = CalcStatus::IN_PROGRESS;

    fractals::per_image();
    setup_standard_fractal_show_dot();

    g_close_enough = g_delta_min * std::pow(2.0, -static_cast<double>(std::abs(g_periodicity_check)));
    g_keyboard_check_interval = g_max_keyboard_check_interval;

    set_symmetry(g_symmetry, true);

    if (!g_resuming && (std::abs(g_log_map_flag) == 2 || (g_log_map_flag && g_log_map_auto_calculate)))
    {
        g_log_map_flag = auto_log_map() * (g_log_map_flag / std::abs(g_log_map_flag));
        setup_log_table();
    }

    run_current_work_item_mode();
    cleanup_standard_fractal_show_dot();

    if (done())
    {
        return;
    }
    if (ui::check_key())
    {
        suspend();
        return;
    }
    if (g_num_work_list == 0)
    {
        finish_work_list();
    }
}

void StandardFractal::run_current_work_item_mode()
{
    switch (g_std_calc_mode)
    {
    case CalcMode::SYNCHRONOUS_ORBIT:
        soi();
        break;

    case CalcMode::TESSERAL:
        tesseral();
        break;

    case CalcMode::BOUNDARY_TRACE:
        boundary_trace();
        break;

    case CalcMode::SOLID_GUESS:
        if (g_calc_status != CalcStatus::COMPLETED)
        {
            solid_guess();
        }
        break;

    case CalcMode::DIFFUSION:
        diffusion_scan();
        break;

    case CalcMode::ORBIT:
        sticky_orbits();
        break;

    case CalcMode::PERTURBATION:
        if (bit_set(fractals::g_cur_fractal_specific->flags, fractals::FractalFlags::PERTURB))
        {
            complete();
        }
        break;

    default:
        one_or_two_pass();
        break;
    }
}

void StandardFractal::start_next_pass()
{
    if (m_requested_calc_mode == CalcMode::THREE_PASS)
    {
        if (!g_resuming || g_three_pass)
        {
            g_std_calc_mode = CalcMode::SOLID_GUESS;
            g_three_pass = true;
            m_after_work_list = AfterWorkList::START_THREE_PASS_FOLLOWUP;
        }
        else
        {
            g_std_calc_mode = followup_calc_mode();
            m_after_work_list = AfterWorkList::COMPLETE;
        }
    }
    else
    {
        g_three_pass = false;
        g_std_calc_mode = m_requested_calc_mode;
        m_after_work_list = AfterWorkList::COMPLETE;
    }

    m_work_list_started = false;
    m_phase = Phase::WORK_LIST;
}

void StandardFractal::start_timer()
{
    if (!m_timer_started)
    {
        g_engine_timer_start = std::clock();
        m_timer_started = true;
    }
}

void StandardFractal::start_work_list()
{
    if (!m_dispatch_saved)
    {
        m_saved_dispatch = fractals::g_dispatch;
        m_dispatch_saved = true;
    }
    select_alternate_math_dispatch();

    if (g_potential.flag && g_potential.store_16bit)
    {
        const CalcMode tmp_calc_mode = g_std_calc_mode;

        g_std_calc_mode = CalcMode::ONE_PASS;
        if (!g_resuming)
        {
            if (ui::pot_start_disk() < 0)
            {
                g_potential.store_16bit = false;
                g_std_calc_mode = tmp_calc_mode;
            }
        }
    }
    if (g_std_calc_mode == CalcMode::BOUNDARY_TRACE &&
        bit_set(fractals::g_cur_fractal_specific->flags, fractals::FractalFlags::NO_TRACE))
    {
        g_std_calc_mode = CalcMode::ONE_PASS;
    }
    if (g_std_calc_mode == CalcMode::SOLID_GUESS &&
        bit_set(fractals::g_cur_fractal_specific->flags, fractals::FractalFlags::NO_GUESS))
    {
        g_std_calc_mode = CalcMode::ONE_PASS;
    }
    if (g_std_calc_mode == CalcMode::ORBIT && !calc_type_supports_orbit_mode())
    {
        g_std_calc_mode = CalcMode::ONE_PASS;
    }

    g_num_work_list = 0;
    add_work_list({0, 0}, {g_logical_screen.x_dots - 1, g_logical_screen.y_dots - 1}, {0, 0}, 0, 0);
    if (g_resuming)
    {
        const int version = start_resume();
        get_resume_len(sizeof(g_num_work_list), &g_num_work_list, sizeof(g_work_list), g_work_list, 0);
        end_resume();
        if (version < 2)
        {
            g_begin_pt.x = 0;
        }
    }

    if (g_distance_estimator)
    {
        setup_standard_fractal_distance_estimator();
    }
    m_work_list_started = true;
}

void StandardFractal::update_timer()
{
    if (m_timer_started)
    {
        g_timer_interval = (std::clock() - g_engine_timer_start) / (CLOCKS_PER_SEC / 100);
    }
}

} // namespace id::engine
