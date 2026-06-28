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

    void reset_image(double real, double imag, int color);
    void reset_overlay(double real, double imag);
    void iterate();
    void iterate_without_delay();
    bool done() const;
    bool consume_delay_pending();
    void scrub();

private:
    enum class PlotMode
    {
        IMAGE,
        OVERLAY
    };

    struct SavedOverlayPoint
    {
        int x{};
        int y{};
        int color{};
    };

    void clear_saved_points_if_reset();
    void plot_d_orbit(double dx, double dy);
    void plot_image_point(int x, int y);
    void plot_overlay_point(int x, int y);
    void reset_plot(double real, double imag, int color, PlotMode mode);
    void save_overlay_point(int x, int y, int color);
    void update_orbit_sound(int x, int y);
    void update_saved_overlay_count();

    std::vector<SavedOverlayPoint> m_saved_overlay;
    double m_real{};
    double m_imag{};
    int m_color{};
    PlotMode m_mode{PlotMode::IMAGE};
    bool m_delay_pending{};
    bool m_done{true};
};

OrbitPlot &orbit_plot();
void plot_image_orbit(double real, double imag, int color);
void plot_overlay_orbit(double real, double imag);
void plot_orbit(double real, double imag, int color);
void scrub_orbit();

} // namespace id::engine
