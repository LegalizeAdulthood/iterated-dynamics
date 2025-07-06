// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

namespace id::fractals
{

enum class DiffusionMode
{
    CENTRAL = 0,           // central (classic), single seed point in the center
    FALLING_PARTICLES = 1, // line along the bottom
    SQUARE_CAVITY = 2      // large square that fills the screen
};

class Diffusion
{
public:
    Diffusion();
    Diffusion(const Diffusion &) = delete;
    Diffusion(Diffusion &&) = delete;
    ~Diffusion() = default;
    Diffusion &operator=(const Diffusion &) = delete;
    Diffusion &operator=(Diffusion &&) = delete;

    void release_new_particle();
    bool move_particle();
    void color_particle();
    bool adjust_limits();
    bool keyboard_check_needed();

private:
    int m_kbd_check{};      // to limit kbd checking
    int m_x_max{};          //
    int m_y_max{};          //
    int m_x_min{};          //
    int m_y_min{};          // Current maximum coordinates
    int m_y{-1};            //
    int m_x{-1};            //
    int m_border{};         // Distance between release point and fractal
    DiffusionMode m_mode{}; // Determines diffusion type
    int m_color_shift{};    // 0: select colors at random, otherwise shift the color every color_shift points
    int m_color_count{};    // Counts down from color_shift
    int m_current_color{1}; // Start at color 1 (color 0 is probably invisible)
    float m_radius{};       //
};

} // namespace id::fractals

int diffusion_type();
