// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include "engine/calcfrac.h"
#include "engine/Point.h"
#include "engine/UserData.h"
#include "math/big.h"

#include <complex>
#include <cstddef>
#include <vector>

namespace id::engine
{

class PertEngine
{
public:
    // Validate the sample count for both reference buffers without allocating.
    static std::size_t reference_sample_count(long max_iterations);

    math::BFComplex initialize_frame_bf(
        const std::complex<double> &half_horizontal, const std::complex<double> &half_vertical);
    void initialize_frame(const std::complex<double> &center, const std::complex<double> &half_horizontal,
        const std::complex<double> &half_vertical);
    void initialize_pixel_strategy();
    int calculate_pixel(const Point &pt);
    int calculate_pixel(int col, int row);
    void set_glitch_tolerance(double tolerance);
    double glitch_tolerance_threshold(const std::complex<double> &value) const;
    const DistanceEstimator &distance_estimator() const;

    // Number of references computed for the current frame.
    int reference_count() const;

    void finish();

private:
    struct ReferencePoint
    {
        std::complex<double> pixel_position;
        std::complex<double> delta;
        std::complex<double> coordinate;
        std::vector<std::complex<double>> orbit;
        std::vector<double> tolerance;
    };

    void cleanup();
    void initialize_pixel_state();
    void allocate_working_values();
    void set_reference_point(const std::complex<double> &pixel_position);
    std::complex<double> pixel_delta(const std::complex<double> &pixel_position) const;
    std::complex<double> parameter_delta(const std::complex<double> &pixel_position) const;
    std::complex<double> inverted_coordinate(const std::complex<double> &pixel_position) const;
    void inverted_coordinate(math::BFComplex coordinate, const std::complex<double> &pixel_position) const;
    void coordinate_bf(math::BFComplex coordinate, const std::complex<double> &pixel_position) const;
    void reset_for_frame(const std::complex<double> &half_horizontal, const std::complex<double> &half_vertical);
    int calculate_point(const Point &pt);
    void reference_zoom_point(ReferencePoint &reference, const math::BFComplex &center);
    void reference_zoom_point(ReferencePoint &reference, const std::complex<double> &center);

    ReferencePoint m_reference{};
    DistanceEstimator m_distance_estimator{};
    int m_reference_count{};
    long m_cycle_len{-1};
    math::BFComplex m_reference_coordinate_bf{};
    math::BigFloat m_tmp_bf{};
    math::BFComplex m_center_bf{};
    std::complex<double> m_center{};
    std::complex<double> m_half_horizontal{};
    std::complex<double> m_half_vertical{};
    bool m_center_stack_saved{};
    bool m_done{true};
    bool m_frame_initialized{};
    double m_glitch_tolerance{DEFAULT_PERTURBATION_TOLERANCE};
    int m_saved_stack{};
};

} // namespace id::engine
