// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "engine/calcfrac.h"
#include "engine/StandardPass.h"
#include "fractals/fractalp.h"

#include <array>

namespace id::engine
{

class StandardFractal
{
public:
    int calculate_standard_pixel(bool yield_to_ui);
    bool consume_standard_pixel_yield();

    void resume();

    void suspend();

    bool done() const;

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
    void finish_work_list();
    void pop_work_list_front();
    void restore_dispatch();
    void run_current_work_item();
    void run_current_work_item_mode();
    void start_next_pass();
    void start_timer();
    void start_work_list();
    bool standard_pixel_yield_enabled() const;
    void update_timer();

    fractals::FractalDispatch m_saved_dispatch{};
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
    bool m_attracted{};
    bool m_caught_a_cycle{};
    bool m_dispatch_saved{};
    bool m_standard_pixel_active{};
    bool m_standard_pixel_completed_yield{};
    bool m_standard_pixel_input_checked{};
    bool m_standard_pixel_iteration_started{};
    bool m_timer_started{};
    bool m_work_item_active{};
    bool m_work_item_yielded{};
    bool m_work_list_started{};
};

StandardFractal *active_standard_fractal();
StandardPassStatus current_standard_pass_status();
StandardFractal *set_active_standard_fractal(StandardFractal *standard_fractal);

} // namespace id::engine
