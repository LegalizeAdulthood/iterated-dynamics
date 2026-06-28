// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <vector>

namespace id::engine
{

extern int g_orbit_delay;       // 100-usec orbit playback delay
extern int g_orbit_skip_points; // initial orbit points skipped by passes=o

class OrbitPlot
{
public:
    OrbitPlot();

    void reset(double real, double imag, int color);
    void iterate();
    bool done() const;
    void scrub();

private:
    struct SavedOrbitPoint
    {
        int x{};
        int y{};
        int color{};
    };

    void clear_saved_points_if_reset();
    void plot_d_orbit(double dx, double dy, int color);
    void save_orbit_point(int x, int y, int color);
    void update_saved_point_count();

    std::vector<SavedOrbitPoint> m_saved_orbit;
    double m_real{};
    double m_imag{};
    int m_color{};
    bool m_done{true};
};

OrbitPlot &orbit_plot();
void plot_orbit(double real, double imag, int color);
void scrub_orbit();

} // namespace id::engine
