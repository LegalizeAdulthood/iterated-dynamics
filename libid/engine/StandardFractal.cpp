// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "engine/StandardFractal.h"

#include "engine/calc_frac_init.h"
#include "engine/calcfrac.h"
#include "engine/log_map.h"
#include "engine/LogicalScreen.h"
#include "engine/OrbitPlot.h"
#include "engine/Potential.h"
#include "engine/resume.h"
#include "engine/SoundOutput.h"
#include "engine/UserData.h"
#include "engine/work_list.h"
#include "fractals/fractalp.h"
#include "fractals/lyapunov.h"
#include "math/big.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace id::engine
{

long auto_log_map();
void set_symmetry(SymmetryType sym, bool use_list);
void setup_standard_fractal_distance_estimator();

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

bool perturbation_supported()
{
    return bit_set(fractals::g_cur_fractal_specific->flags, fractals::FractalFlags::PERTURB);
}

bool perturbation_auto_requested()
{
    return perturbation_supported() && math::g_bf_math != math::BFMathType::NONE;
}

bool perturbation_requested(const CalcMode calc_mode)
{
    if (!perturbation_supported())
    {
        return false;
    }
    if (g_user.perturbation == PerturbationMode::NO)
    {
        return false;
    }
    if (g_user.perturbation == PerturbationMode::YES || calc_mode == CalcMode::PERTURBATION)
    {
        return true;
    }
    return perturbation_auto_requested();
}

CalcMode traversal_calc_mode(const CalcMode calc_mode)
{
    return calc_mode == CalcMode::PERTURBATION ? CalcMode::SOLID_GUESS : calc_mode;
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

StandardFractal::~StandardFractal()
{
    if (started())
    {
        finish();
    }
}

std::vector<std::string> StandardFractal::resume()
{
    std::vector<std::string> warnings{ImageCalculation::start()};
    m_requested_calc_mode = g_std_calc_mode;
    m_after_work_list = AfterWorkList::COMPLETE;
    m_phase = Phase::START;
    m_dispatch_saved = false;
    m_perturbation_active = false;
    m_perturbation_requested = false;
    m_standard_pass.reset();
    m_work_item_active = false;
    m_standard_pixel_completed_yield = false;
    m_incomplete_standard_pixel_yield = false;
    s_standard_pass_status = {};
    clear_standard_pixel();
    return warnings;
}

bool StandardFractal::potential_storage_required() const
{
    return m_phase == Phase::POTENTIAL_STORAGE;
}

void StandardFractal::complete_potential_storage(const bool available)
{
    if (!potential_storage_required())
    {
        throw std::logic_error{"Potential storage was not requested"};
    }
    g_potential.store_16bit = available;
    initialize_work_list();
}

void StandardFractal::suspend()
{
    m_standard_pass.suspend();
    s_standard_pass_status = standard_pass_status();
    if (m_perturbation_active)
    {
        m_pert_engine.finish();
        m_perturbation_active = false;
    }
    if (g_num_work_list > 0)
    {
        const bool save_pass_resume{m_standard_pass.calc_mode() == CalcMode::ORBIT};
        alloc_resume(sizeof(g_work_list) + sizeof(long) + 20, save_pass_resume ? 3 : 2);
        put_resume_len(sizeof(g_num_work_list), &g_num_work_list, sizeof(g_work_list), g_work_list, 0);
        m_standard_pass.save_resume();
    }
    else
    {
        g_calc_status = CalcStatus::COMPLETED;
    }
    m_work_item_active = false;
    m_standard_pass.reset();
    clear_standard_pixel();
    m_perturbation_active = false;
    complete();
    finish();
}

void StandardFractal::finish()
{
    if (!done())
    {
        abort();
        return;
    }
    ImageCalculation::finish();
}

void StandardFractal::abort()
{
    complete();
    g_calc_status = CalcStatus::NON_RESUMABLE;
    ImageCalculation::finish();
}

bool StandardFractal::done() const
{
    return m_phase == Phase::COMPLETE;
}

bool StandardFractal::consume_traversal_yield()
{
    const bool yielded{m_standard_pixel_completed_yield || m_incomplete_standard_pixel_yield};
    m_standard_pixel_completed_yield = false;
    m_incomplete_standard_pixel_yield = false;
    return yielded;
}

bool StandardFractal::consume_orbit_overlay_displayed()
{
    const bool displayed{m_orbit_overlay_displayed};
    m_orbit_overlay_displayed = false;
    return displayed;
}

void StandardFractal::complete_pending_orbit_plot()
{
    if (!m_orbit_plot_pending)
    {
        throw std::logic_error{"No pending orbit plot"};
    }
    if (!orbit_plot().done())
    {
        throw std::logic_error{"Pending orbit plot is not complete"};
    }
    m_orbit_plot_pending = false;
    if (m_orbit_plot_marks_standard_orbit)
    {
        m_standard_pixel_orbit_plotted = true;
    }
    m_orbit_plot_marks_standard_orbit = false;
}

OrbitPlot &StandardFractal::pending_orbit_plot()
{
    if (!m_orbit_plot_pending)
    {
        throw std::logic_error{"No pending orbit plot"};
    }
    return orbit_plot();
}

StandardPassStatus StandardFractal::standard_pass_status() const
{
    StandardPassStatus status{m_standard_pass.status()};
    status.perturbation_active = m_perturbation_active;
    status.perturbation_reference_count = m_pert_engine.reference_count();
    return status;
}

bool StandardFractal::select_pixel()
{
    if (done() || m_orbit_overlay_displayed)
    {
        return false;
    }
    if (m_orbit_plot_pending)
    {
        run_pending_orbit_plot();
        return false;
    }
    if (m_overlay_scrub_pending)
    {
        scrub_overlay();
        return false;
    }

    start_timer();
    if (!prepare_work_item())
    {
        return false;
    }
    const CalcMode mode{m_standard_pass.calc_mode()};
    if (mode == CalcMode::SYNCHRONOUS_ORBIT || mode == CalcMode::ORBIT)
    {
        ActiveStandardFractalScope active_standard_fractal{*this};
        const bool completed{m_standard_pass.iterate()};
        s_standard_pass_status = standard_pass_status();
        if (completed)
        {
            finish_work_item();
        }
        update_timer();
        return false;
    }
    if (m_standard_pass.select_pixel())
    {
        return true;
    }
    s_standard_pass_status = standard_pass_status();
    if (m_standard_pass.done())
    {
        finish_work_item();
    }
    update_timer();
    return false;
}

math::Point2i StandardFractal::selected_pixel() const
{
    return m_standard_pass.selected_pixel();
}

bool StandardFractal::calculate_pixel()
{
    const math::Point2i point{selected_pixel()};
    g_col = point.x;
    g_row = point.y;
    ActiveStandardFractalScope active_standard_fractal{*this};
    const int color{fractals::calc_type()};
    if (color == -1)
    {
        return false;
    }
    m_standard_pass.complete_pixel(color);
    const CalcMode mode{m_standard_pass.calc_mode()};
    if ((mode == CalcMode::ONE_PASS || mode == CalcMode::TWO_PASS) &&
        fractals::g_dispatch.calc_type() == fractals::lyapunov_type)
    {
        m_standard_pixel_completed_yield = true;
    }
    return true;
}

void StandardFractal::begin_chunk()
{
    if (done() || m_orbit_overlay_displayed || m_orbit_plot_pending || m_overlay_scrub_pending)
    {
        return;
    }
    start_timer();
    if (prepare_work_item())
    {
        const CalcMode mode{m_standard_pass.calc_mode()};
        if (mode != CalcMode::SYNCHRONOUS_ORBIT && mode != CalcMode::ORBIT)
        {
            m_standard_pass.begin_chunk();
        }
    }
}

void StandardFractal::iterate()
{
    if (done() || m_orbit_overlay_displayed)
    {
        return;
    }
    if (m_orbit_plot_pending)
    {
        run_pending_orbit_plot();
        return;
    }
    if (m_overlay_scrub_pending)
    {
        scrub_overlay();
        return;
    }

    begin_chunk();
    while (select_pixel())
    {
        if (!calculate_pixel())
        {
            static_cast<void>(consume_traversal_yield());
            break;
        }
        if (consume_traversal_yield())
        {
            break;
        }
    }
    if (m_work_item_active)
    {
        s_standard_pass_status = standard_pass_status();
    }
    update_timer();
}

void StandardFractal::clear_standard_pixel()
{
    if (m_standard_pixel_active)
    {
        g_max_iterations = m_save_max_it;
    }
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
    m_standard_pixel_orbit_calc_result = 0;
    m_saved_incr = 1;
    m_attracted = false;
    m_caught_a_cycle = false;
    m_standard_pixel_active = false;
    m_standard_pixel_iteration_yielded = false;
    m_standard_pixel_orbit_complete = false;
    m_incomplete_standard_pixel_yield = false;
    m_standard_pixel_orbit_calc_completed = false;
    m_standard_pixel_iteration_started = false;
    m_standard_pixel_orbit_plotted = false;
    m_standard_pixel_overlay_scrubbed = false;
    m_orbit_plot_marks_standard_orbit = false;
    m_orbit_overlay_displayed = false;
    m_overlay_scrub_pending = false;
}

void StandardFractal::complete()
{
    m_work_item_active = false;
    m_standard_pass.reset();
    clear_standard_pixel();
    if (m_perturbation_active)
    {
        m_pert_engine.finish();
        m_perturbation_active = false;
    }
    if (m_requested_calc_mode == CalcMode::THREE_PASS || m_requested_calc_mode == CalcMode::PERTURBATION ||
        m_perturbation_requested)
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
        m_phase = Phase::START_WORK_LIST;
        return;
    }
    complete();
}

void StandardFractal::pop_work_list_front()
{
    if (g_num_work_list <= 0)
    {
        throw std::logic_error{"Cannot pop an empty work list"};
    }
    --g_num_work_list;
    for (int i = 0; i < g_num_work_list; ++i)
    {
        g_work_list[i] = g_work_list[i + 1];
    }
}

void StandardFractal::queue_overlay_orbit_plot(const double real, const double imag, const bool mark_as_plotted)
{
    if (!m_orbit_plot_pending)
    {
        orbit_plot().reset(real, imag);
        m_orbit_plot_pending = true;
        m_orbit_plot_marks_standard_orbit = mark_as_plotted;
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

void StandardFractal::run_pending_orbit_plot()
{
    if (pending_orbit_plot().iterate())
    {
        m_orbit_overlay_displayed = true;
    }
    if (pending_orbit_plot().done())
    {
        complete_pending_orbit_plot();
    }
}

void StandardFractal::scrub_overlay()
{
    scrub_orbit();
    if (m_orbit_plot_pending)
    {
        m_standard_pixel_orbit_plotted = m_orbit_plot_marks_standard_orbit;
        m_orbit_plot_pending = false;
        m_orbit_plot_marks_standard_orbit = false;
    }
    m_overlay_scrub_pending = false;
    m_standard_pixel_overlay_scrubbed = true;
}

bool StandardFractal::scrub_overlay_orbit(const bool yield_to_ui)
{
    if (!g_show_orbit || m_standard_pixel_overlay_scrubbed)
    {
        return false;
    }

    if (!g_sound->ready() || (yield_to_ui && standard_presentation_yield_enabled()))
    {
        m_overlay_scrub_pending = true;
        m_incomplete_standard_pixel_yield = true;
        return true;
    }

    // Compatibility path for pass modes that cannot yet yield here.
    scrub_overlay();
    return false;
}

bool StandardFractal::prepare_work_item()
{
    if (m_phase == Phase::START)
    {
        start_next_pass();
    }
    if (m_phase == Phase::START_WORK_LIST)
    {
        start_work_list();
    }
    if (potential_storage_required())
    {
        return false;
    }
    if (!m_work_item_active && g_num_work_list <= 0)
    {
        finish_work_list();
        return false;
    }

    if (!m_work_item_active)
    {
        ActiveStandardFractalScope active_standard_fractal{*this};
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
        m_perturbation_requested = perturbation_requested(m_requested_calc_mode);

        g_close_enough = g_delta_min * std::pow(2.0, -static_cast<double>(std::abs(g_periodicity_check)));
        g_keyboard_check_interval = g_max_keyboard_check_interval;

        set_symmetry(g_symmetry, true);

        if (!g_resuming && (std::abs(g_log_map_flag) == 2 || (g_log_map_flag && g_log_map_auto_calculate)))
        {
            g_log_map_flag = auto_log_map() * (g_log_map_flag / std::abs(g_log_map_flag));
            setup_log_table();
        }
        m_work_item_active = true;
        if (m_perturbation_requested && perturbation_supported())
        {
            if (!m_perturbation_active)
            {
                start_perturbation_frame();
            }
            fractals::g_dispatch.set_calc_type(standard_fractal_type);
        }
        m_standard_pass.select(g_std_calc_mode);
    }
    return true;
}

void StandardFractal::finish_work_item()
{
    m_standard_pass.reset();
    m_work_item_active = false;
    if (g_num_work_list == 0)
    {
        finish_work_list();
    }
}

void StandardFractal::start_next_pass()
{
    m_requested_calc_mode = g_std_calc_mode;
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
        g_std_calc_mode = traversal_calc_mode(m_requested_calc_mode);
        m_after_work_list = AfterWorkList::COMPLETE;
    }

    m_phase = Phase::START_WORK_LIST;
}

void StandardFractal::start_work_list()
{
    if (!m_dispatch_saved)
    {
        m_saved_dispatch = fractals::g_dispatch;
        m_dispatch_saved = true;
    }
    select_alternate_math_dispatch();

    if (g_potential.flag && g_potential.store_16bit && !g_resuming)
    {
        m_phase = Phase::POTENTIAL_STORAGE;
        return;
    }
    initialize_work_list();
}

void StandardFractal::initialize_work_list()
{
    if (g_potential.flag && g_potential.store_16bit)
    {
        g_std_calc_mode = CalcMode::ONE_PASS;
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
        if (version >= 3)
        {
            m_standard_pass.restore_resume(g_std_calc_mode);
        }
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
    m_phase = Phase::WORK_LIST;
}

bool StandardFractal::standard_pixel_yield_enabled() const
{
    const CalcMode calc_mode{m_standard_pass.calc_mode()};
    return calc_mode == CalcMode::ONE_PASS || calc_mode == CalcMode::TWO_PASS || calc_mode == CalcMode::BOUNDARY_TRACE;
}

bool StandardFractal::standard_presentation_yield_enabled() const
{
    switch (m_standard_pass.calc_mode())
    {
    case CalcMode::ONE_PASS:
    case CalcMode::TWO_PASS:
    case CalcMode::SYNCHRONOUS_ORBIT:
    case CalcMode::BOUNDARY_TRACE:
    case CalcMode::SOLID_GUESS:
    case CalcMode::DIFFUSION:
    case CalcMode::TESSERAL:
    case CalcMode::ORBIT:
        return true;

    default:
        return false;
    }
}

} // namespace id::engine
