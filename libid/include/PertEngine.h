// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "Point.h"
#include "big.h"
#include "id.h"

#include <complex>
#include <string>
#include <vector>

class PertEngine
{
public:
    void initialize_frame(const BFComplex &center_bf, const std::complex<double> &center, double zoom_radius);
    int calculate_one_frame(int power, int subtype);

private:
    int calculate_point(const Point &pt, double tempRadius, int window_radius);
    void reference_zoom_point_bf(const BFComplex &BigCentre, int maxIteration);
    void reference_zoom_point(const std::complex<double> &center, int maxIteration);
    void pert_functions(
        const std::complex<double> &x_ref, std::complex<double> &delta_n, std::complex<double> &delta0);
    void ref_functions_bf(const BFComplex &center, BFComplex *Z, BFComplex *ZTimes2);
    void ref_functions(
        const std::complex<double> &center, std::complex<double> *Z, std::complex<double> *ZTimes2);
    void cleanup();

    std::string m_status;
    std::vector<std::complex<double>> m_xn;
    std::vector<double> m_perturbation_tolerance_check;
    double m_delta_real{};
    double m_delta_imag{};
    double m_z_magnitude_squared{};
    std::vector<Point> m_points_remaining;
    std::vector<Point> m_glitch_points;
    int m_power{};
    int m_subtype{};
    long m_glitch_point_count{};
    long m_remaining_point_count{};
    BFComplex m_center_bf{};
    std::complex<double> m_center{};
    double m_zoom_radius{};
    bool m_calculate_glitches{true};
    double m_percent_glitch_tolerance{0.1}; // What percentage of the image is okay to be glitched.
    int m_reference_points{};
    int m_saved{}; // keep track of bigflt memory
};
