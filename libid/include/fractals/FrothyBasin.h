// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include "math/cmplx.h"

#include <string>

namespace id::fractals
{

class FrothyBasin
{
public:
    FrothyBasin() = default;
    FrothyBasin(const FrothyBasin &) = delete;
    FrothyBasin(FrothyBasin &&) = delete;
    ~FrothyBasin() = default;
    FrothyBasin &operator=(const FrothyBasin &) = delete;
    FrothyBasin &operator=(FrothyBasin &&) = delete;

    bool per_image();
    std::string set_froth_palette();
    void resume();
    void suspend();
    bool done() const;
    void iterate();
    bool consume_orbit_overlay_displayed();
    // clang-format off
    bool overlay_scrub_pending() const { return m_overlay_scrub_pending; }
    // clang-format on
    void scrub_overlay();

    int orbit();

private:
    // clang-format off
    void complete_pending_overlay_orbit_point() { m_overlay_orbit_point_pending = false; }
    // clang-format on
    void advance_pixel();
    int calc();
    void finish_pixel();
    void flush_overlay_orbit_point();
    void start_pixel();
    double top_x_mapping(double x);

    bool m_repeat_mapping{};
    int m_alt_color{};
    int m_attractors{};
    int m_shades{};
    double m_a{};
    double m_half_a{};
    double m_top_x1{};
    double m_top_x2{};
    double m_top_x3{};
    double m_top_x4{};
    double m_left_x1{};
    double m_left_x2{};
    double m_left_x3{};
    double m_left_x4{};
    double m_right_x1{};
    double m_right_x2{};
    double m_right_x3{};
    double m_right_x4{};
    int m_col{};
    int m_row{};
    int m_found_attractor{};
    math::DComplex m_overlay_orbit_point{};
    bool m_overlay_orbit_point_pending{};
    bool m_orbit_overlay_displayed{};
    bool m_overlay_scrub_completed{};
    bool m_overlay_scrub_pending{};
    bool m_pixel_active{};
};

extern FrothyBasin g_frothy_basin;

bool froth_per_image();
int froth_orbit();
int froth_per_pixel();

} // namespace id::fractals
