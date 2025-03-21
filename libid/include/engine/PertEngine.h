// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "engine/Point.h"
#include "math/big.h"

#include <complex>
#include <string>
#include <vector>

class PertEngine
{
public:
    void initialize_frame(const BFComplex &center_bf, const std::complex<double> &center, double zoom_radius);
    int calculate_one_frame();
    int perturbation_per_pixel(int x, int y, double bailout);
    int calculate_orbit(int x, int y, long iteration);
    int get_number_references();

private:
    int calculate_reference(int x, int y);
    void reference_zoom_point(const BFComplex &center, int max_iteration);
    void reference_zoom_point(const std::complex<double> &center, int max_iteration);
    void cleanup();

    std::string m_status;
    std::vector<std::complex<double>> m_xn;
    std::vector<double> m_perturbation_tolerance_check;
    double m_delta_real{};
    double m_delta_imag{};
    BFComplex m_center_bf{};
    std::complex<double> m_center{};
    double m_zoom_radius{};
    double m_percent_glitch_tolerance{0.1}; // What percentage of the image is okay to be glitched.
    int m_reference_points{};
    int m_saved_stack{};
    std::complex<double> m_c{};
    BFComplex m_c_bf{};

    std::complex<double> m_delta_sub_0{};
    std::complex<double> m_delta_sub_n{};
};
