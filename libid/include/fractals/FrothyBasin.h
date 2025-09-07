// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

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

    int calc();
    bool keyboard_check();
    void keyboard_reset();

    bool per_image();
    int orbit();

private:
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
};

extern FrothyBasin g_frothy_basin;

int froth_orbit();
int froth_per_pixel();
bool froth_per_image();

} // namespace id::fractals
