// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "Point.h"
#include "big.h"
#include "id.h"

#include <complex>
#include <string>

#define MAXPOWER 28
#define MAXFILTER 9

class PertEngine
{
public:
    void initialize_frame(
        bf_t x_center_bf, bf_t y_center_bf, double x_center, double y_center, double zoom_radius);
    int calculate_one_frame(double bailout, int powerin, int InsideFilterIn, int OutsideFilterIn,
        int biomorph, int subtype, void (*plot)(int, int, int), int potential(double, long));

private:
    int calculate_point(int x, int y, double tempRadius, int window_radius, double bailout,
        Point *glitchPoints, void (*plot)(int, int, int), int potential(double, long));
    void reference_zoom_point_bf(const BFComplex &BigCentre, int maxIteration);
    void reference_zoom_point(const std::complex<double> &center, int maxIteration);
    void load_pascal(long PascalArray[], int n);
    double diff_abs(const double c, const double d);
    void pert_functions(
        std::complex<double> *XRef, std::complex<double> *DeltaSubN, std::complex<double> *DeltaSub0);
    void ref_functions_bf(const BFComplex &center, BFComplex *Z, BFComplex *ZTimes2);
    void ref_functions(
        const std::complex<double> &center, std::complex<double> *Z, std::complex<double> *ZTimes2);
    void cleanup();

    std::string m_status;
    std::complex<double> *m_x_sub_n{};
    double *m_perturbation_tolerance_check{};
    double m_calculated_real_delta{};
    double m_calculated_imaginary_delta{};
    double m_z_coordinate_magnitude_squared{};
    long m_pascal_array[MAXPOWER]{};
    Point *m_points_remaining{};
    Point *m_glitch_points{};
    double m_param[MAX_PARAMS]{};
    bool m_is_potential{};
    int m_width{};
    int m_height{};
    int m_max_iteration{};
    int m_power{};
    int m_subtype{};
    int m_biomorph{};
    int m_inside_method{};  // the number of the inside filter
    int m_outside_method{}; // the number of the outside filter
    long m_glitch_point_count{};
    long m_remaining_point_count{};
    bf_t m_x_zoom_pt_bf{};
    bf_t m_y_zoom_pt_bf{};
    double m_x_zoom_pt{};
    double m_y_zoom_pt{};
    double m_zoom_radius{};
    double m_x_centre{};
    double m_y_centre{};
    bool m_calculate_glitches{true};
    unsigned int m_num_coefficients{5};
    double m_percent_glitch_tolerance{0.1}; // What percentage of the image is okay to be glitched.
    int m_reference_points{};
    int m_saved{}; // keep track of bigflt memory
    bf_math_type m_math_type{};
};
