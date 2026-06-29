// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <memory>

namespace id::engine
{

class OrbitPlot;
class StandardFractal;

} // namespace id::engine

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
    int orbit_step();
    bool image_orbit_plot_pending() const;
    engine::OrbitPlot &pending_image_orbit_plot();
    void complete_pending_image_orbit_plot();
    void queue_image_orbit_plot(double real, double imag, int color);

private:
    enum class MapMode
    {
        LEGACY_REAL,
        CURRENT_REAL,
        GENERALIZED
    };

    struct ImageOrbitPoint
    {
        double real{};
        double imag{};
        int color{};
    };

    static MapMode select_map_mode();

    int legacy_real_orbit_step();
    int current_real_orbit_step();
    int generalized_orbit_step();
    void complete();

    std::unique_ptr<engine::StandardFractal> m_standard_fractal;
    MapMode m_map_mode{MapMode::GENERALIZED};
    ImageOrbitPoint m_pending_image_orbit_plot{};
    int m_row{};
    int m_col{};
    bool m_image_orbit_plot_pending{};
    bool m_done{};
};

bool popcorn_uses_default_functions();
int popcorn_fractal_old();
int popcorn_fractal();
int popcorn_orbit();

} // namespace id::fractals
