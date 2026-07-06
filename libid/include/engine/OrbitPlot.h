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

    void reset(double real, double imag);
    bool iterate();
    bool done() const;
    void scrub();

private:
    struct SavedOverlayPoint
    {
        int x{};
        int y{};
        int color{};
    };

    bool plot(double dx, double dy);
    void plot(int x, int y);
    void save_overlay_point(int x, int y, int color);
    void update_orbit_sound(int x, int y);
    void update_orbit_save_flag();

    std::vector<SavedOverlayPoint> m_saved_overlay;
    double m_real{};
    double m_imag{};
    bool m_done{true};
};

OrbitPlot &orbit_plot();

inline void scrub_orbit()
{
    orbit_plot().scrub();
}

} // namespace id::engine
