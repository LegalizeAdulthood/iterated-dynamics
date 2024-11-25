// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "Point.h"
#include "big.h"

#include <complex>
#include <string>
#include <vector>

class PertEngine
{
public:
    void initialize_frame(const BFComplex &center_bf, const std::complex<double> &center, double zoom_radius);
    int calculate_one_frame();

private:
    int calculate_point(const Point &pt, double magnified_radius, int window_radius);
    void reference_zoom_point(const BFComplex &center, int max_iteration);
    void reference_zoom_point(const std::complex<double> &center, int max_iteration);
    void cleanup();

    std::string m_status;
    std::vector<std::complex<double>> m_xn;
    std::vector<double> m_perturbation_tolerance_check;
    double m_delta_real{};
    double m_delta_imag{};
    std::vector<Point> m_points_remaining;
    std::vector<Point> m_glitch_points;
    long m_glitch_point_count{};
    long m_remaining_point_count{};
    BFComplex m_center_bf{};
    std::complex<double> m_center{};
    double m_zoom_radius{};
    bool m_calculate_glitches{true};
    double m_percent_glitch_tolerance{0.1}; // What percentage of the image is okay to be glitched.
    int m_reference_points{};
    int m_saved_stack{};
};
