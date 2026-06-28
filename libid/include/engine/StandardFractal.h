// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "engine/calcfrac.h"
#include "engine/PertEngine.h"
#include "engine/StandardPass.h"
#include "fractals/fractalp.h"

#include <array>
#include <cstddef>
#include <vector>

namespace id::engine
{

class OrbitPlot;

class StandardFractal
{
public:
    int calculate_standard_pixel(bool yield_to_ui);
    bool consume_standard_pixel_yield();
    void complete_pending_orbit_plot();
    void queue_image_orbit_plot(double real, double imag, int color, bool mark_as_plotted = false);
    void queue_overlay_orbit_plot(double real, double imag, bool mark_as_plotted = true);

    void resume();

    void suspend();

    bool done() const;

    void iterate();

    bool orbit_plot_pending() const;
    OrbitPlot &pending_orbit_plot();
    StandardPassStatus standard_pass_status() const;

private:
    enum class Phase
    {
        START,
        WORK_LIST,
        COMPLETE
    };

    enum class AfterWorkList
    {
        COMPLETE,
        START_THREE_PASS_FOLLOWUP
    };

    void clear_standard_pixel();
    void complete();
    void finish_work_list();
    void pop_work_list_front();
    void restore_dispatch();
    void run_current_work_item();
    void run_current_work_item_mode();
    void run_perturbation_glitch_retries();
    void start_next_pass();
    void start_perturbation_frame();
    void start_timer();
    void start_work_list();
    bool standard_pixel_yield_enabled() const;
    void update_timer();

    fractals::FractalDispatch m_saved_dispatch{};
    PertEngine m_pert_engine{};
    StandardPass m_standard_pass{};
    std::array<double, 16> m_tan_table{};
    std::vector<Point> m_perturbation_retry_points{};
    math::DComplex m_deriv{};
    math::DComplex m_dem_new{};
    math::DComplex m_last_z{};
    CalcMode m_requested_calc_mode{};
    AfterWorkList m_after_work_list{AfterWorkList::COMPLETE};
    Phase m_phase{Phase::START};
    double m_mem_value{};
    double m_min_orbit{100000.0};
    double m_total_dist{};
    long m_cycle_len{-1};
    long m_dem_color{-1};
    long m_min_index{};
    long m_save_max_it{};
    long m_saved_and{};
    long m_saved_color_iter{};
    int m_check_freq{};
    int m_hooper{};
    std::size_t m_perturbation_retry_index{};
    int m_standard_pixel_col{};
    int m_standard_pixel_row{};
    int m_saved_incr{1};
    int m_standard_pixel_orbit_calc_result{};
    bool m_attracted{};
    bool m_caught_a_cycle{};
    bool m_dispatch_saved{};
    bool m_perturbation_active{};
    bool m_perturbation_requested{};
    bool m_perturbation_retry_active{};
    bool m_standard_pixel_active{};
    bool m_standard_pixel_completed_yield{};
    bool m_standard_pixel_input_checked{};
    bool m_standard_pixel_orbit_calc_completed{};
    bool m_standard_pixel_iteration_started{};
    bool m_standard_pixel_orbit_plotted{};
    bool m_timer_started{};
    bool m_orbit_plot_pending{};
    bool m_orbit_plot_marks_standard_orbit{};
    bool m_work_item_active{};
    bool m_work_item_yielded{};
    bool m_work_list_started{};
};

StandardFractal *active_standard_fractal();
StandardPassStatus current_standard_pass_status();
StandardFractal *set_active_standard_fractal(StandardFractal *standard_fractal);
void submit_image_orbit_plot(double real, double imag, int color);

} // namespace id::engine
