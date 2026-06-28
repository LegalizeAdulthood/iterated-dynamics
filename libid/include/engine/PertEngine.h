// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "engine/Point.h"
#include "engine/UserData.h"
#include "math/big.h"

#include <complex>
#include <vector>

namespace id::engine
{

class PertEngine
{
public:
    math::BFComplex initialize_frame_bf(double zoom_radius);
    void initialize_frame(const std::complex<double> &center, double zoom_radius);
    void initialize_pixel_strategy();
    int calculate_pixel(const Point &pt);
    int calculate_pixel(int col, int row);
    bool retry_needed(const std::vector<Point> &points) const;
    bool select_retry_reference(const std::vector<Point> &points);
    void set_glitch_tolerance(double tolerance);
    double glitch_tolerance_threshold(const std::complex<double> &value) const;
    std::vector<Point> take_glitch_points();
    void finish();

private:
    void cleanup();
    void complete_frame();
    void initialize_pixel_state();
    void allocate_working_values();
    bool select_center_reference();
    void select_reference_point(const Point &pt);
    void reset_for_frame(double zoom_radius);
    int calculate_point(const Point &pt, double magnified_radius, int window_radius);
    void reference_zoom_point(const math::BFComplex &center, int max_iteration);
    void reference_zoom_point(const std::complex<double> &center, int max_iteration);

    std::vector<std::complex<double>> m_xn;
    std::vector<double> m_perturbation_tolerance_check;
    math::BFComplex m_c_bf{};
    math::BFComplex m_reference_coordinate_bf{};
    math::BigFloat m_tmp_bf{};
    std::complex<double> m_c{};
    std::complex<double> m_reference_coordinate{};
    double m_delta_real{};
    double m_delta_imag{};
    std::vector<Point> m_glitch_points;
    long m_glitch_point_count{};
    math::BFComplex m_center_bf{};
    std::complex<double> m_center{};
    double m_magnified_radius{};
    double m_zoom_radius{};
    int m_window_radius{};
    bool m_calculate_glitches{true};
    bool m_center_stack_saved{};
    bool m_done{true};
    bool m_frame_initialized{};
    double m_glitch_tolerance{DEFAULT_PERTURBATION_TOLERANCE};
    double m_percent_glitch_tolerance{0.1}; // What percentage of the image is okay to be glitched.
    int m_reference_points{};
    int m_saved_stack{};
};

} // namespace id::engine
