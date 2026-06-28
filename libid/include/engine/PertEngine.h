// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "engine/Point.h"
#include "math/big.h"

#include <complex>
#include <string>
#include <vector>

namespace id::engine
{

class PertEngine
{
public:
    math::BFComplex initialize_frame_bf(double zoom_radius);
    void initialize_frame(const std::complex<double> &center, double zoom_radius);
    void initialize_pixel_strategy();
    int calculate_pixel(int col, int row);
    int calculate_one_frame();
    bool done() const;
    bool iterate_glitches();
    bool iterate();
    void finish();
    void suspend();

private:
    void cleanup();
    void complete_frame();
    void initialize_frame_state();
    void initialize_point_list();
    void initialize_pixel_state();
    void allocate_working_values();
    bool needs_reference_point() const;
    void reset_for_frame(double zoom_radius);
    bool select_reference_point();
    int calculate_point_chunk();
    int calculate_point(const Point &pt, double magnified_radius, int window_radius);
    void prepare_glitch_retries();
    void reference_zoom_point(const math::BFComplex &center, int max_iteration);
    void reference_zoom_point(const std::complex<double> &center, int max_iteration);

    std::string m_status;
    std::vector<std::complex<double>> m_xn;
    std::vector<double> m_perturbation_tolerance_check;
    math::BFComplex m_c_bf{};
    math::BFComplex m_reference_coordinate_bf{};
    math::BigFloat m_tmp_bf{};
    std::complex<double> m_c{};
    std::complex<double> m_reference_coordinate{};
    double m_delta_real{};
    double m_delta_imag{};
    std::vector<Point> m_points_remaining;
    std::vector<Point> m_glitch_points;
    Point m_reference_point{};
    long m_current_point{};
    long m_glitch_point_count{};
    long m_remaining_point_count{};
    math::BFComplex m_center_bf{};
    std::complex<double> m_center{};
    double m_magnified_radius{};
    double m_zoom_radius{};
    int m_window_radius{};
    int m_last_checked{};
    int m_result{};
    bool m_calculate_glitches{true};
    bool m_center_stack_saved{};
    bool m_done{true};
    bool m_frame_initialized{};
    bool m_reference_selected{};
    double m_percent_glitch_tolerance{0.1}; // What percentage of the image is okay to be glitched.
    int m_reference_points{};
    int m_saved_stack{};
};

} // namespace id::engine
