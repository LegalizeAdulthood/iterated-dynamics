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
    bool image_orbit_plot_pending() const;
    engine::OrbitPlot &pending_image_orbit_plot();
    void complete_pending_image_orbit_plot();
    void queue_image_orbit_plot(double real, double imag, int color);

private:
    struct ImageOrbitPoint
    {
        double real{};
        double imag{};
        int color{};
    };

    void complete();

    std::unique_ptr<engine::StandardFractal> m_standard_fractal;
    ImageOrbitPoint m_pending_image_orbit_plot{};
    int m_row{};
    int m_col{};
    bool m_image_orbit_plot_pending{};
    bool m_done{};
};

int popcorn_fractal_old();
int popcorn_fractal();
int popcorn_orbit();

} // namespace id::fractals
