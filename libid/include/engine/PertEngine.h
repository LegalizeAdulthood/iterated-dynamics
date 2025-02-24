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
    int calculate_reference();
    long get_glitch_point_count();
    void push_glitch(int x, int y, int value);
    bool is_glitched();
    bool is_pixel_complete(int x, int y);
    void set_glitched(bool status);
    void set_points_count(long count);
    void set_glitch_points_count(long count);
    void cleanup();


private:
//    int calculate_point(const Point &pt, double magnified_radius, int window_radius);
    void reference_zoom_point(const BFComplex &center, int max_iteration);
    void reference_zoom_point(const std::complex<double> &center, int max_iteration);

    std::string m_status;
    std::vector<std::complex<double>> m_xn;
    std::vector<double> m_perturbation_tolerance_check;
    double m_delta_real{};
    double m_delta_imag{};
    std::vector<Point> m_points_remaining;
    std::vector<Point> m_glitch_points;
    long m_glitch_point_count{};
    long m_remaining_point_count{};
    long m_points_count{};
    std::complex<double> m_center{};
    BFComplex m_center_bf{};
    std::complex<double> m_c{};
    BFComplex m_c_bf{};
    double m_zoom_radius{};
    bool m_calculate_glitches{true};
    double m_percent_glitch_tolerance{0.1}; // What percentage of the image is okay to be glitched.
    int m_reference_points{};
    int m_saved_stack{};
    std::vector<int> m_glitches{};
    bool m_glitched{};

    std::complex<double> m_old_reference_coordinate{};

    std::complex<double> m_delta_sub_0{};
    std::complex<double> m_delta_sub_n{};
};
