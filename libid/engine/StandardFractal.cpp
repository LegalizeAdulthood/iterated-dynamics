// SPDX-License-Identifier: GPL-3.0-only
//
#include "engine/StandardFractal.h"

#include "engine/calc_frac_init.h"
#include "engine/calcfrac.h"
#include "engine/engine_timer.h"
#include "engine/log_map.h"
#include "engine/LogicalScreen.h"
#include "engine/orbit.h"
#include "engine/Potential.h"
#include "engine/resume.h"
#include "engine/work_list.h"
#include "fractals/fractalp.h"
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

StandardFractal *s_active_standard_fractal{};
StandardPassStatus s_standard_pass_status{};

class ActiveStandardFractalScope
{
public:
    explicit ActiveStandardFractalScope(StandardFractal &standard_fractal) :
        m_previous{set_active_standard_fractal(&standard_fractal)}
    {
    }

    ~ActiveStandardFractalScope()
    {
        set_active_standard_fractal(m_previous);
    }

    ActiveStandardFractalScope(const ActiveStandardFractalScope &) = delete;
    ActiveStandardFractalScope(ActiveStandardFractalScope &&) = delete;
    ActiveStandardFractalScope &operator=(const ActiveStandardFractalScope &) = delete;
    ActiveStandardFractalScope &operator=(ActiveStandardFractalScope &&) = delete;

private:
    StandardFractal *m_previous{};
};

bool calc_type_supports_orbit_mode()
{
    const fractals::CalcType table_calc_type{fractals::g_cur_fractal_specific->calc_type};
    return table_calc_type == standard_fractal_type;
}

CalcMode followup_calc_mode()
{
    return g_logical_screen.x_dots >= 640 ? CalcMode::TWO_PASS : CalcMode::ONE_PASS;
}

bool is_standard_pass_display()
{
    switch (g_passes)
    {
    case Passes::SEQUENTIAL_SCAN:
    case Passes::SOLID_GUESS:
    case Passes::BOUNDARY_TRACE:
    case Passes::TESSERAL:
    case Passes::DIFFUSION:
    case Passes::ORBITS:
        return true;

    default:
        return false;
    }
}

} // namespace

StandardFractal *active_standard_fractal()
{
    return s_active_standard_fractal;
}

StandardPassStatus current_standard_pass_status()
{
    if (!is_standard_pass_display())
    {
        return {};
    }
    if (s_active_standard_fractal != nullptr)
    {
        return s_active_standard_fractal->standard_pass_status();
    }
    return s_standard_pass_status;
}

StandardFractal *set_active_standard_fractal(StandardFractal *standard_fractal)
{
    StandardFractal *previous{s_active_standard_fractal};
    s_active_standard_fractal = standard_fractal;
    return previous;
}

void StandardFractal::resume()
{
    m_requested_calc_mode = g_std_calc_mode;
    m_after_work_list = AfterWorkList::COMPLETE;
    m_phase = Phase::START;
    m_dispatch_saved = false;
    m_perturbation_active = false;
    m_standard_pass.reset();
    m_timer_started = false;
    m_work_item_active = false;
    m_work_item_yielded = false;
    m_work_list_started = false;
    m_standard_pixel_completed_yield = false;
    s_standard_pass_status = {};
    clear_standard_pixel();
}

void StandardFractal::suspend()
{
    m_standard_pass.suspend();
    s_standard_pass_status = m_standard_pass.status();
    if (m_perturbation_active && !m_pert_engine.done())
    {
        m_pert_engine.suspend();
        add_work_list(g_start_pt, g_stop_pt, g_begin_pt, g_work_pass, g_work_symmetry);
        m_perturbation_active = false;
    }
    if (g_num_work_list > 0)
    {
        alloc_resume(sizeof(g_work_list) + 20, 2);
        put_resume_len(sizeof(g_num_work_list), &g_num_work_list, sizeof(g_work_list), g_work_list, 0);
    }
    else
    {
        g_calc_status = CalcStatus::COMPLETED;
    }
    if (m_work_item_active)
    {
        cleanup_standard_fractal_show_dot();
        m_work_item_active = false;
    }
    m_standard_pass.reset();
    clear_standard_pixel();
    m_perturbation_active = false;
    complete();
}

bool StandardFractal::done() const
{
    return m_phase == Phase::COMPLETE;
}

bool StandardFractal::consume_standard_pixel_yield()
{
    const bool yielded{m_standard_pixel_completed_yield};
    m_standard_pixel_completed_yield = false;
    return yielded;
}

StandardPassStatus StandardFractal::standard_pass_status() const
{
    return m_standard_pass.status();
}

void StandardFractal::iterate()
{
    if (done())
    {
        return;
    }

    ActiveStandardFractalScope active_standard_fractal{*this};
    m_work_item_yielded = false;
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

void StandardFractal::clear_standard_pixel()
{
    m_tan_table.fill(0.0);
    m_deriv = {};
    m_dem_new = {};
    m_last_z = {};
    m_mem_value = 0.0;
    m_min_orbit = 100000.0;
    m_total_dist = 0.0;
    m_cycle_len = -1;
    m_dem_color = -1;
    m_min_index = 0;
    m_save_max_it = 0;
    m_saved_and = 0;
    m_saved_color_iter = 0;
    m_check_freq = 0;
    m_hooper = 0;
    m_standard_pixel_col = 0;
    m_standard_pixel_row = 0;
    m_saved_incr = 1;
    m_attracted = false;
    m_caught_a_cycle = false;
    m_standard_pixel_active = false;
    m_standard_pixel_input_checked = false;
    m_standard_pixel_iteration_started = false;
}

void StandardFractal::complete()
{
    if (m_work_item_active)
    {
        cleanup_standard_fractal_show_dot();
        m_work_item_active = false;
    }
    m_standard_pass.reset();
    clear_standard_pixel();
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
    if (!m_work_item_active && g_num_work_list <= 0)
    {
        finish_work_list();
        return;
    }

    if (!m_work_item_active)
    {
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
        m_work_item_active = true;
    }

    run_current_work_item_mode();

    if (done())
    {
        return;
    }
    if (m_work_item_yielded)
    {
        return;
    }
    cleanup_standard_fractal_show_dot();
    m_standard_pass.reset();
    m_work_item_active = false;
    if (g_num_work_list == 0)
    {
        finish_work_list();
    }
}

void StandardFractal::run_current_work_item_mode()
{
    if (g_std_calc_mode == CalcMode::PERTURBATION)
    {
        if (bit_set(fractals::g_cur_fractal_specific->flags, fractals::FractalFlags::PERTURB))
        {
            if (!m_perturbation_active)
            {
                start_perturbation_frame();
            }
            m_pert_engine.iterate();
            if (m_pert_engine.done())
            {
                m_perturbation_active = false;
                g_calc_status = CalcStatus::COMPLETED;
                complete();
            }
            else
            {
                m_work_item_yielded = true;
            }
        }
        return;
    }

    m_standard_pass.select(g_std_calc_mode);
    const bool pass_completed{m_standard_pass.iterate()};
    s_standard_pass_status = m_standard_pass.status();
    if (!pass_completed)
    {
        m_work_item_yielded = true;
    }
    else
    {
        m_standard_pass.reset();
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

bool StandardFractal::standard_pixel_yield_enabled() const
{
    const CalcMode calc_mode{m_standard_pass.calc_mode()};
    return calc_mode == CalcMode::ONE_PASS || calc_mode == CalcMode::TWO_PASS;
}

void StandardFractal::update_timer()
{
    if (m_timer_started)
    {
        g_timer_interval = (std::clock() - g_engine_timer_start) / (CLOCKS_PER_SEC / 100);
    }
}

} // namespace id::engine
