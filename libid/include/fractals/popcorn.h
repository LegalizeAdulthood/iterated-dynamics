// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "engine/orbit.h"

namespace id::fractals
{

class Popcorn
{
public:
    Popcorn();
    ~Popcorn();

    Popcorn(const Popcorn &) = delete;
    Popcorn(Popcorn &&) = delete;
    Popcorn &operator=(const Popcorn &) = delete;
    Popcorn &operator=(Popcorn &&) = delete;

    void resume();
    void suspend();
    bool done() const;
    void iterate();
    bool image_orbit_plot_pending() const;
    engine::OrbitPlot &pending_image_orbit_plot();
    void complete_pending_image_orbit_plot();
    void queue_image_orbit_plot(double real, double imag, int color);
    static void setup_julia_attractor(double real, double imag);

private:
    enum class MapMode
    {
        LEGACY_REAL,
        CURRENT_REAL,
        GENERALIZED
    };

    enum class RenderMode
    {
        IMAGE_ORBITS,
        ESCAPE_TIME
    };

    static MapMode select_map_mode();
    static RenderMode select_render_mode();

    void advance_seed();
    int legacy_real_orbit_step();
    int current_real_orbit_step();
    int generalized_orbit_step();
    int orbit_step();
    void plot_escape_time_seed();
    void start_seed();
    void complete();

    engine::OrbitPlot m_image_orbit_plot{};
    MapMode m_map_mode{MapMode::GENERALIZED};
    RenderMode m_render_mode{RenderMode::IMAGE_ORBITS};
    long m_color_iter{};
    int m_orbit_step_result{};
    int m_row{};
    int m_col{};
    bool m_image_orbit_plot_pending{};
    bool m_orbit_step_completed{};
    bool m_seed_active{};
    bool m_done{};
};

bool popcorn_uses_default_functions();

} // namespace id::fractals
