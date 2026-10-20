// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

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
    math::BFComplex initialize_frame_bf(double zoom_radius);
    void initialize_frame(const std::complex<double> &center, double zoom_radius);
    void initialize_pixel_strategy();
    int calculate_pixel(const Point &pt);
    int calculate_pixel(int col, int row);
    void set_glitch_tolerance(double tolerance);
    double glitch_tolerance_threshold(const std::complex<double> &value) const;
    int reference_count() const;
    void finish();

private:
    struct ReferencePoint
    {
        std::complex<double> pixel_position;
        std::complex<double> delta;
        std::vector<std::complex<double>> orbit;
        std::vector<double> tolerance;
    };

    void cleanup();
    void initialize_pixel_state();
    void allocate_working_values();
    void add_reference_point(const std::complex<double> &pixel_position);
    std::size_t closest_reference(const Point &pt) const;
    std::complex<double> pixel_delta(const std::complex<double> &pixel_position) const;
    void reset_for_frame(double zoom_radius);
    int calculate_point(const Point &pt);
    void reference_zoom_point(ReferencePoint &reference, const math::BFComplex &center);
    void reference_zoom_point(ReferencePoint &reference, const std::complex<double> &center);

    std::vector<ReferencePoint> m_references;
    std::size_t m_active_reference{};
    math::BFComplex m_reference_coordinate_bf{};
    math::BigFloat m_tmp_bf{};
    math::BFComplex m_center_bf{};
    std::complex<double> m_center{};
    double m_zoom_radius{};
    int m_window_radius{};
    bool m_center_stack_saved{};
    bool m_done{true};
    bool m_frame_initialized{};
    double m_glitch_tolerance{DEFAULT_PERTURBATION_TOLERANCE};
    int m_saved_stack{};
};

} // namespace id::engine
