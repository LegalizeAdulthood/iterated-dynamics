// SPDX-License-Identifier: GPL-3.0-only
//
#include "engine/OrbitPlot.h"

#include "engine/calc_frac_init.h"
#include "engine/calcfrac.h"
#include "engine/ImageRegion.h"
#include "engine/LogicalScreen.h"
#include "engine/SoundOutput.h"
#include "engine/VideoInfo.h"
#include "misc/ValueSaver.h"
#include "ui/video.h"

using namespace id::misc;
using namespace id::ui;

namespace id::engine
{

int g_orbit_delay{};
int g_orbit_skip_points{};

static OrbitPlot s_orbit_plot;

/* Showing orbit requires converting real co-ords to screen co-ords.
   Define:
       Xs == xxmax-xx3rd               Ys == yy3rd-yymax
       W  == xdots-1                   D  == ydots-1
   We know that:
       realx == lx0[col] + lx1[row]
       realy == ly0[row] + ly1[col]
       lx0[col] == (col/width) * Xs + xxmin
       lx1[row] == row * delxx
       ly0[row] == (row/D) * Ys + yymax
       ly1[col] == col * (0-delyy)
  so:
       realx == (col/W) * Xs + xxmin + row * delxx
       realy == (row/D) * Ys + yymax + col * (0-delyy)
  and therefore:
       row == (realx-xxmin - (col/W)*Xs) / Xv    (1)
       col == (realy-yymax - (row/D)*Ys) / Yv    (2)
  substitute (2) into (1) and solve for row:
       row == ((realx-xxmin)*(0-delyy2)*W*D - (realy-yymax)*Xs*D)
                      / ((0-delyy2)*W*delxx2*D-Ys*Xs)
  */

OrbitPlot::OrbitPlot()
{
    static constexpr int INITIAL_SAVED_OVERLAY_POINTS{500};
    m_saved_overlay.reserve(INITIAL_SAVED_OVERLAY_POINTS);
}

void OrbitPlot::reset(const double real, const double imag)
{
    if (!g_orbit_save_flag && !m_saved_overlay.empty())
    {
        m_saved_overlay.clear();
    }
    m_real = real;
    m_imag = imag;
    m_done = false;
}

bool OrbitPlot::iterate()
{
    if (m_done)
    {
        return false;
    }
    return plot(m_real - g_image_region.m_min.x, m_imag - g_image_region.m_max.y);
}

bool OrbitPlot::done() const
{
    return m_done;
}

bool OrbitPlot::plot(const double dx, const double dy)
{
    int i = static_cast<int>(dy * g_plot_mx1 - dx * g_plot_mx2);
    i += g_logical_screen.x_offset;
    if (i < 0 || i >= g_screen_x_dots)
    {
        m_done = true;
        return false;
    }
    int j = static_cast<int>(dx * g_plot_my1 - dy * g_plot_my2);
    j += g_logical_screen.y_offset;
    if (j < 0 || j >= g_screen_y_dots)
    {
        m_done = true;
        return false;
    }
    if (!g_sound->play({static_cast<double>(i), static_cast<double>(j), 0.0}))
    {
        return false;
    }
    {
        ValueSaver save_screen_x_offset{g_logical_screen.x_offset, 0};
        ValueSaver save_screen_y_offset{g_logical_screen.y_offset, 0};
        plot(i, j);
    }
    m_done = true;
    return true;
}

void OrbitPlot::plot(const int x, const int y)
{
    const int color = get_color(x, y);
    save_overlay_point(x, y, color);
    g_put_color(x, y, color ^ g_orbit_color);
}

void OrbitPlot::save_overlay_point(const int x, const int y, const int color)
{
    m_saved_overlay.push_back(SavedOverlayPoint{x, y, color});
    update_orbit_save_flag();
}

void OrbitPlot::update_orbit_save_flag()
{
    g_orbit_save_flag = !m_saved_overlay.empty();
}

OrbitPlot &orbit_plot()
{
    return s_orbit_plot;
}

void OrbitPlot::scrub()
{
    if (!g_orbit_save_flag || m_saved_overlay.empty())
    {
        m_saved_overlay.clear();
        g_orbit_save_flag = false;
        return;
    }
    ValueSaver save_screen_x_offset{g_logical_screen.x_offset, 0};
    ValueSaver save_screen_y_offset{g_logical_screen.y_offset, 0};
    while (!m_saved_overlay.empty())
    {
        const SavedOverlayPoint point{m_saved_overlay.back()};
        m_saved_overlay.pop_back();
        g_put_color(point.x, point.y, point.color);
    }
    update_orbit_save_flag();
}

} // namespace id::engine
