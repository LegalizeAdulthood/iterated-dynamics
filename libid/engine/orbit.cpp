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
    static constexpr int INITIAL_SAVED_ORBIT_POINTS{500};
    m_saved_orbit.reserve(INITIAL_SAVED_ORBIT_POINTS);
}

void OrbitPlot::reset(const double real, const double imag, const int color)
{
    clear_saved_points_if_reset();
    m_real = real;
    m_imag = imag;
    m_color = color;
    m_done = false;
}

void OrbitPlot::iterate()
{
    if (m_done)
    {
        return;
    }
    plot_d_orbit(m_real - g_image_region.m_min.x, m_imag - g_image_region.m_max.y, m_color);
    m_done = true;
}

bool OrbitPlot::done() const
{
    return m_done;
}

void OrbitPlot::clear_saved_points_if_reset()
{
    if (!g_orbit_save_flag && !m_saved_orbit.empty())
    {
        m_saved_orbit.clear();
    }
}

void OrbitPlot::plot_d_orbit(const double dx, const double dy, const int color)
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
    int save_screen_x_offset = g_logical_screen.x_offset;
    int save_screen_y_offset = g_logical_screen.y_offset;
    g_logical_screen.y_offset = 0;
    g_logical_screen.x_offset = 0;
    // save orbit value
    if (color == -1)
    {
        const int c = get_color(i, j);
        save_orbit_point(i, j, c);
        g_put_color(i, j, c ^ g_orbit_color);
    }
    else
    {
        g_put_color(i, j, color);
    }
    g_logical_screen.x_offset = save_screen_x_offset;
    g_logical_screen.y_offset = save_screen_y_offset;
    if (g_debug_flag == DebugFlags::FORCE_SCALED_SOUND_FORMULA)
    {
        if ((g_sound_flag & SOUNDFLAG_ORBIT_MASK) == SOUNDFLAG_X) // sound = x
        {
            write_sound(i * 1000 / g_logical_screen.x_dots + g_base_hertz);
        }
        else if ((g_sound_flag & SOUNDFLAG_ORBIT_MASK) > SOUNDFLAG_X) // sound = y or z
        {
            write_sound(j * 1000 / g_logical_screen.y_dots + g_base_hertz);
        }
        else if (g_orbit_delay > 0)
        {
            wait_until(g_orbit_delay);
        }
    }
    else
    {
        if ((g_sound_flag & SOUNDFLAG_ORBIT_MASK) == SOUNDFLAG_X) // sound = x
        {
            write_sound(i + g_base_hertz);
        }
        else if ((g_sound_flag & SOUNDFLAG_ORBIT_MASK) == SOUNDFLAG_Y) // sound = y
        {
            write_sound(j + g_base_hertz);
        }
        else if ((g_sound_flag & SOUNDFLAG_ORBIT_MASK) == SOUNDFLAG_Z) // sound = z
        {
            write_sound(i + j + g_base_hertz);
        }
        else if (g_orbit_delay > 0)
        {
            wait_until(g_orbit_delay);
        }
    }

    // placing sleepms here delays each dot
}

void OrbitPlot::save_orbit_point(const int x, const int y, const int color)
{
    m_saved_orbit.push_back(SavedOrbitPoint{x, y, color});
    update_saved_point_count();
}

void OrbitPlot::update_saved_point_count()
{
    g_orbit_save_flag = !m_saved_orbit.empty();
}

void plot_orbit(const double real, const double imag, const int color)
{
    s_orbit_plot.reset(real, imag, color);
    while (!s_orbit_plot.done())
    {
        s_orbit_plot.iterate();
    }
}

void OrbitPlot::scrub()
{
    driver_mute();
    if (!g_orbit_save_flag || m_saved_orbit.empty())
    {
        m_saved_orbit.clear();
        g_orbit_save_flag = false;
        return;
    }
    ValueSaver save_screen_x_offset{g_logical_screen.x_offset, 0};
    ValueSaver save_screen_y_offset{g_logical_screen.y_offset, 0};
    while (!m_saved_orbit.empty())
    {
        const SavedOrbitPoint point{m_saved_orbit.back()};
        m_saved_orbit.pop_back();
        g_put_color(point.x, point.y, point.color);
        update_saved_point_count();
    }
}

void scrub_orbit()
{
    s_orbit_plot.scrub();
}

} // namespace id::engine
