// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "math/cmplx.h"

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
    void resume();
    void suspend();
    bool done() const;
    void iterate();
    bool overlay_orbit_point_pending() const;
    math::DComplex pending_overlay_orbit_point() const;
    void complete_pending_overlay_orbit_point();
    bool overlay_scrub_pending() const;
    void complete_overlay_scrub();

    int orbit();

private:
    void advance_pixel();
    int calc();
    void finish_pixel();
    void start_pixel();
    double top_x_mapping(double x);
    void set_froth_palette();

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
    bool m_overlay_scrub_completed{};
    bool m_overlay_scrub_pending{};
    bool m_pixel_active{};
};

extern FrothyBasin g_frothy_basin;

int froth_orbit();
int froth_per_pixel();

} // namespace id::fractals
