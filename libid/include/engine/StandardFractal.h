// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
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
    int calculate_mandelbrot_pixel();
    int calculate_standard_pixel(bool yield_to_ui);
    bool consume_traversal_yield();
    bool consume_orbit_overlay_displayed();
    void queue_overlay_orbit_plot(double real, double imag, bool mark_as_plotted = true);
    void scrub_overlay();

    void resume();

    void suspend();
    void abort();

    bool done() const;

    void begin_chunk();
    // False ends this work chunk; done() distinguishes image completion.
    bool select_pixel();
    math::Point2i selected_pixel() const;
    // False retains the selected pixel for continuation.
    bool calculate_pixel();
    void iterate();

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
    void complete_pending_orbit_plot();
    void finish_work_list();
    OrbitPlot &pending_orbit_plot();
    void pop_work_list_front();
    bool prepare_work_item();
    void finish_work_item();
    void restore_dispatch();
    void run_pending_orbit_plot();
    bool scrub_overlay_orbit(bool yield_to_ui);
    void start_next_pass();
    void start_perturbation_frame();
    void start_timer();
    void start_work_list();
    bool standard_presentation_yield_enabled() const;
    bool standard_pixel_yield_enabled() const;
    void update_timer();

    fractals::FractalDispatch m_saved_dispatch{};
    PertEngine m_pert_engine{};
    StandardPass m_standard_pass{};
    std::array<double, 16> m_tan_table{};
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
    int m_standard_pixel_col{};
    int m_standard_pixel_row{};
    int m_saved_incr{1};
    int m_standard_pixel_orbit_calc_result{};
    bool m_attracted{};
    bool m_caught_a_cycle{};
    bool m_dispatch_saved{};
    bool m_perturbation_active{};
    bool m_perturbation_requested{};
    bool m_standard_pixel_active{};
    bool m_standard_pixel_completed_yield{};
    bool m_incomplete_standard_pixel_yield{};
    bool m_standard_pixel_iteration_yielded{};
    bool m_standard_pixel_orbit_complete{};
    bool m_standard_pixel_orbit_calc_completed{};
    bool m_standard_pixel_iteration_started{};
    bool m_standard_pixel_orbit_plotted{};
    bool m_standard_pixel_overlay_scrubbed{};
    bool m_timer_started{};
    bool m_orbit_plot_pending{};
    bool m_orbit_plot_marks_standard_orbit{};
    bool m_orbit_overlay_displayed{};
    bool m_overlay_scrub_pending{};
    bool m_work_item_active{};
    bool m_work_list_started{};
};

StandardFractal *active_standard_fractal();
StandardPassStatus current_standard_pass_status();
StandardFractal *set_active_standard_fractal(StandardFractal *standard_fractal);

} // namespace id::engine
