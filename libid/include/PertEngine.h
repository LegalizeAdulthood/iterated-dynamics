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
    void initialize_frame(
        bf_t x_center_bf, bf_t y_center_bf, double x_center, double y_center, double zoom_radius);
    int calculate_one_frame(int power, int subtype);

private:
    enum
    {
        MAX_POWER = 28,
        MAX_FILTER = 9
    };

    int calculate_point(const Point &pt, double tempRadius, int window_radius, double bailout,
        void (*plot)(int, int, int), int potential(double, long));
    void reference_zoom_point_bf(const BFComplex &BigCentre, int maxIteration);
    void reference_zoom_point(const std::complex<double> &center, int maxIteration);
    void pert_functions(
        std::complex<double> *XRef, std::complex<double> *DeltaSubN, std::complex<double> *DeltaSub0);
    void ref_functions_bf(const BFComplex &center, BFComplex *Z, BFComplex *ZTimes2);
    void ref_functions(
        const std::complex<double> &center, std::complex<double> *Z, std::complex<double> *ZTimes2);
    void cleanup();

    std::string m_status;
    std::complex<double> *m_xn{};
    std::vector<double> m_perturbation_tolerance_check;
    double m_delta_real{};
    double m_delta_imag{};
    double m_z_magnitude_squared{};
    long m_pascal_triangle[MAX_POWER]{};
    std::vector<Point> m_points_remaining;
    std::vector<Point> m_glitch_points;
    int m_power{};
    int m_subtype{};
    long m_glitch_point_count{};
    long m_remaining_point_count{};
    bf_t m_zoom_pt_real_bf{};
    bf_t m_zoom_pt_imag_bf{};
    double m_zoom_pt_real{};
    double m_zoom_pt_imag{};
    double m_zoom_radius{};
    bool m_calculate_glitches{true};
    double m_percent_glitch_tolerance{0.1}; // What percentage of the image is okay to be glitched.
    int m_reference_points{};
    int m_saved{}; // keep track of bigflt memory
};
