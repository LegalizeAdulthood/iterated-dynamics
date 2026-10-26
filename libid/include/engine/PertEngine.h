// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include "engine/calcfrac.h"
#include "engine/Point.h"
#include "engine/UserData.h"
#include "math/big.h"

#include <array>
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
    // Advance one reference chunk; true when the reference is complete.
    bool initialize_pixel_strategy();

    int calculate_pixel(const Point &pt, bool incremental = false);
    int calculate_pixel(int col, int row, bool incremental = false);
    bool orbit_overlay_displayed() const;
    void set_glitch_tolerance(double tolerance);
    double glitch_tolerance_threshold(const std::complex<double> &value) const;
    const DistanceEstimator &distance_estimator() const;

    // Number of references computed for the current frame.
    int reference_count() const;

    void finish();

private:
    static constexpr long STAR_TRAIL_ITERATIONS{16};

    struct ReferencePoint
    {
        std::complex<double> pixel_position;
        std::complex<double> delta;
        std::complex<double> coordinate;
        std::vector<std::complex<double>> orbit;
        std::vector<double> tolerance;
    };

    struct PixelState
    {
        enum class Phase
        {
            ORBIT,
            BAILOUT,
            OUTPUT,
            STATISTICS,
            PERIODICITY,
            SAVE_PERIODICITY,
            CHECK_PERIODICITY,
            COMPLETE
        };

        Phase phase{Phase::ORBIT};
        std::complex<double> position{};
        std::complex<double> parameter_displacement{};
        std::complex<double> orbit_displacement{};
        std::complex<double> w{};
        std::complex<double> old{};
        std::complex<double> saved_z{};
        std::complex<double> precise_previous{};
        std::array<double, STAR_TRAIL_ITERATIONS> tan_table{};
        math::BFComplex precise_coordinate{};
        math::BFComplex precise_orbit{};
        math::BFComplex precise_saved{};
        math::BigFloat precise_tolerance{};
        std::size_t sample{};
        std::size_t saved_sample{};
        std::size_t precise_sample{};
        long iteration{};
        long saved_and{};
        long saved_iteration{};
        long min_index{};
        double saved_error{};
        double min_orbit{1e5};
        double mem_value{};
        double total_distance{};
        double magnitude{};
        int saved_incr{1};
        int epsilon_axis{};
        int stack{};
        bool active{};
        bool star_trail{};
        bool high_precision{};
        bool check_periodicity{};
        bool have_saved_sample{};
        bool saved_is_precise{};
        bool bof{};
        bool at_reference{};
        bool remember_fmod{};
    };

    void cleanup();
    void initialize_pixel_state();
    void allocate_working_values();
    void set_reference_point(const std::complex<double> &pixel_position);
    bool iterate_reference();
    std::complex<double> pixel_delta(const std::complex<double> &pixel_position) const;
    std::complex<double> parameter_delta(const std::complex<double> &pixel_position) const;
    std::complex<double> inverted_coordinate(const std::complex<double> &pixel_position) const;
    void inverted_coordinate(math::BFComplex coordinate, const std::complex<double> &pixel_position) const;
    void coordinate_bf(math::BFComplex coordinate, const std::complex<double> &pixel_position) const;
    void reset_for_frame(const std::complex<double> &half_horizontal, const std::complex<double> &half_vertical);
    void begin_point(const Point &pt);
    void clear_point();
    void clear_output();
    int calculate_point(const Point &pt, std::size_t &remaining);
    int finish_point(const Point &pt);
    bool advance_precise_orbit(std::size_t target, std::size_t &remaining);
    bool save_precise_value(std::size_t &remaining);
    void advance_reference_bf(std::size_t end);
    void advance_reference(std::size_t end);

    ReferencePoint m_reference{};
    PixelState m_pixel{};
    DistanceEstimator m_distance_estimator{};
    int m_reference_count{};
    long m_cycle_len{-1};
    math::BFComplex m_reference_coordinate_bf{};
    math::BFComplex m_reference_z_bf{};
    math::BigFloat m_tmp_bf{};
    math::BFComplex m_center_bf{};
    std::complex<double> m_center{};
    std::complex<double> m_reference_z{};
    std::size_t m_reference_sample{};
    std::size_t m_reference_samples{};
    // Accepted output survives reference replacement for the same pixel.
    std::size_t m_orbit_output_sample{};
    std::complex<double> m_half_horizontal{};
    std::complex<double> m_half_vertical{};
    bool m_center_stack_saved{};
    bool m_done{true};
    bool m_frame_initialized{};
    bool m_reference_ready{};
    bool m_orbit_output_started{};
    bool m_orbit_output_pending{};
    bool m_orbit_overlay_displayed{};
    double m_glitch_tolerance{DEFAULT_PERTURBATION_TOLERANCE};
    int m_saved_stack{};
};

} // namespace id::engine
