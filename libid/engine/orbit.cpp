// SPDX-License-Identifier: GPL-3.0-only
//
#include "engine/orbit.h"

#include "engine/calc_frac_init.h"
#include "engine/calcfrac.h"
#include "engine/ImageRegion.h"
#include "engine/LogicalScreen.h"
#include "engine/sound.h"
#include "engine/VideoInfo.h"
#include "engine/wait_until.h"
#include "misc/debug_flags.h"
#include "misc/Driver.h"
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

void OrbitPlot::reset_image(const double real, const double imag, const int color)
{
    reset_plot(real, imag, color, PlotMode::IMAGE);
}

void OrbitPlot::reset_overlay(const double real, const double imag)
{
    clear_saved_points_if_reset();
    reset_plot(real, imag, -1, PlotMode::OVERLAY);
}

void OrbitPlot::reset_plot(const double real, const double imag, const int color, const PlotMode mode)
{
    m_real = real;
    m_imag = imag;
    m_color = color;
    m_mode = mode;
    m_done = false;
}

void OrbitPlot::iterate()
{
    iterate_without_delay();
    if (consume_delay_pending())
    {
        wait_until(g_orbit_delay);
    }
}

void OrbitPlot::iterate_without_delay()
{
    if (m_done)
    {
        return;
    }
    m_delay_pending = false;
    plot_d_orbit(m_real - g_image_region.m_min.x, m_imag - g_image_region.m_max.y);
    m_done = true;
}

bool OrbitPlot::done() const
{
    return m_done;
}

bool OrbitPlot::consume_delay_pending()
{
    const bool delay_pending{m_delay_pending};
    m_delay_pending = false;
    return delay_pending;
}

void OrbitPlot::clear_saved_points_if_reset()
{
    if (!g_orbit_save_flag && !m_saved_overlay.empty())
    {
        m_saved_overlay.clear();
    }
}

void OrbitPlot::plot_d_orbit(const double dx, const double dy)
{
    int i = static_cast<int>(dy * g_plot_mx1 - dx * g_plot_mx2);
    i += g_logical_screen.x_offset;
    if (i < 0 || i >= g_screen_x_dots)
    {
        return;
    }
    int j = static_cast<int>(dx * g_plot_my1 - dy * g_plot_my2);
    j += g_logical_screen.y_offset;
    if (j < 0 || j >= g_screen_y_dots)
    {
        return;
    }
    {
        ValueSaver save_screen_x_offset{g_logical_screen.x_offset, 0};
        ValueSaver save_screen_y_offset{g_logical_screen.y_offset, 0};
        switch (m_mode)
        {
        case PlotMode::IMAGE:
            plot_image_point(i, j);
            break;

        case PlotMode::OVERLAY:
            plot_overlay_point(i, j);
            break;
        }
    }
    update_orbit_sound(i, j);
}

void OrbitPlot::plot_image_point(const int x, const int y)
{
    g_put_color(x, y, m_color);
}

void OrbitPlot::plot_overlay_point(const int x, const int y)
{
    const int color = get_color(x, y);
    save_overlay_point(x, y, color);
    g_put_color(x, y, color ^ g_orbit_color);
}

void OrbitPlot::update_orbit_sound(const int x, const int y)
{
    if (g_debug_flag == DebugFlags::FORCE_SCALED_SOUND_FORMULA)
    {
        if ((g_sound_flag & SOUNDFLAG_ORBIT_MASK) == SOUNDFLAG_X) // sound = x
        {
            write_sound(x * 1000 / g_logical_screen.x_dots + g_base_hertz);
        }
        else if ((g_sound_flag & SOUNDFLAG_ORBIT_MASK) > SOUNDFLAG_X) // sound = y or z
        {
            write_sound(y * 1000 / g_logical_screen.y_dots + g_base_hertz);
        }
        else if (g_orbit_delay > 0)
        {
            m_delay_pending = true;
        }
    }
    else
    {
        if ((g_sound_flag & SOUNDFLAG_ORBIT_MASK) == SOUNDFLAG_X) // sound = x
        {
            write_sound(x + g_base_hertz);
        }
        else if ((g_sound_flag & SOUNDFLAG_ORBIT_MASK) == SOUNDFLAG_Y) // sound = y
        {
            write_sound(y + g_base_hertz);
        }
        else if ((g_sound_flag & SOUNDFLAG_ORBIT_MASK) == SOUNDFLAG_Z) // sound = z
        {
            write_sound(x + y + g_base_hertz);
        }
        else if (g_orbit_delay > 0)
        {
            m_delay_pending = true;
        }
    }

    // placing sleepms here delays each dot
}

void OrbitPlot::save_overlay_point(const int x, const int y, const int color)
{
    m_saved_overlay.push_back(SavedOverlayPoint{x, y, color});
    update_saved_overlay_count();
}

void OrbitPlot::update_saved_overlay_count()
{
    g_orbit_save_flag = !m_saved_overlay.empty();
}

OrbitPlot &orbit_plot()
{
    return s_orbit_plot;
}

void plot_image_orbit(const double real, const double imag, const int color)
{
    orbit_plot().reset_image(real, imag, color);
    while (!orbit_plot().done())
    {
        orbit_plot().iterate();
    }
}

void plot_overlay_orbit(const double real, const double imag)
{
    orbit_plot().reset_overlay(real, imag);
    while (!orbit_plot().done())
    {
        orbit_plot().iterate();
    }
}

void plot_orbit(const double real, const double imag, const int color)
{
    if (color == -1)
    {
        plot_overlay_orbit(real, imag);
    }
    else
    {
        plot_image_orbit(real, imag, color);
    }
}

void OrbitPlot::scrub()
{
    driver_mute();
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
        update_saved_overlay_count();
    }
}

void scrub_orbit()
{
    orbit_plot().scrub();
}

} // namespace id::engine
