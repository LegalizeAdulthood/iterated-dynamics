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
    void initialize_frame(
        const math::BFComplex &center_bf, const std::complex<double> &center, double zoom_radius);
    int calculate_one_frame();

private:
    void initialize_frame_state();
    void initialize_point_list();
    void allocate_working_values();
    bool needs_reference_point() const;
    bool select_reference_point();
    int calculate_point_chunk();
    int calculate_point(const Point &pt, double magnified_radius, int window_radius);
    void prepare_glitch_retries();
    void reference_zoom_point(const math::BFComplex &center, int max_iteration);
    void reference_zoom_point(const std::complex<double> &center, int max_iteration);
    void cleanup();

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
    bool m_calculate_glitches{true};
    double m_percent_glitch_tolerance{0.1}; // What percentage of the image is okay to be glitched.
    int m_reference_points{};
    int m_saved_stack{};
};

} // namespace id::engine
