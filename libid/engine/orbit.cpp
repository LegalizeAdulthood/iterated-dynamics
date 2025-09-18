// SPDX-License-Identifier: GPL-3.0-only
//
#include "engine/orbit.h"

#include "engine/calcfrac.h"
#include "engine/cmdfiles.h"
#include "engine/id_data.h"
#include "engine/wait_until.h"
#include "misc/debug_flags.h"
#include "misc/Driver.h"
#include "misc/ValueSaver.h"
#include "ui/sound.h"
#include "ui/video.h"

using namespace id::misc;
using namespace id::ui;

namespace id::engine
{

static void plot_d_orbit(double dx, double dy, int color);

enum
{
    NUM_SAVE_ORBIT = 1500
};

static int s_save_orbit[NUM_SAVE_ORBIT]{}; // array to save orbit values

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

static void plot_d_orbit(double dx, double dy, int color)
{
    if (g_orbit_save_index >= NUM_SAVE_ORBIT-3)
    {
        return;
    }
    int i = static_cast<int>(dy * g_plot_mx1 - dx * g_plot_mx2);
    i += g_logical_screen_x_offset;
    if (i < 0 || i >= g_screen_x_dots)
    {
        return;
    }
    int j = static_cast<int>(dx * g_plot_my1 - dy * g_plot_my2);
    j += g_logical_screen_y_offset;
    if (j < 0 || j >= g_screen_y_dots)
    {
        return;
    }
    int save_screen_x_offset = g_logical_screen_x_offset;
    int save_screen_y_offset = g_logical_screen_y_offset;
    g_logical_screen_y_offset = 0;
    g_logical_screen_x_offset = 0;
    // save orbit value
    if (color == -1)
    {
        s_save_orbit[g_orbit_save_index++] = i;
        s_save_orbit[g_orbit_save_index++] = j;
        const int c = get_color(i, j);
        s_save_orbit[g_orbit_save_index++] = c;
        g_put_color(i, j, c^g_orbit_color);
    }
    else
    {
        g_put_color(i, j, color);
    }
    g_logical_screen_x_offset = save_screen_x_offset;
    g_logical_screen_y_offset = save_screen_y_offset;
    if (g_debug_flag == DebugFlags::FORCE_SCALED_SOUND_FORMULA)
    {
        if ((g_sound_flag & SOUNDFLAG_ORBIT_MASK) == SOUNDFLAG_X)   // sound = x
        {
            write_sound(i * 1000 / g_logical_screen_x_dots + g_base_hertz);
        }
        else if ((g_sound_flag & SOUNDFLAG_ORBIT_MASK) > SOUNDFLAG_X)     // sound = y or z
        {
            write_sound(j * 1000 / g_logical_screen_y_dots + g_base_hertz);
        }
        else if (g_orbit_delay > 0)
        {
            wait_until(g_orbit_delay);
        }
    }
    else
    {
        if ((g_sound_flag & SOUNDFLAG_ORBIT_MASK) == SOUNDFLAG_X)   // sound = x
        {
            write_sound(i + g_base_hertz);
        }
        else if ((g_sound_flag & SOUNDFLAG_ORBIT_MASK) == SOUNDFLAG_Y)     // sound = y
        {
            write_sound(j + g_base_hertz);
        }
        else if ((g_sound_flag & SOUNDFLAG_ORBIT_MASK) == SOUNDFLAG_Z)     // sound = z
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

void plot_orbit(double real, double imag, int color)
{
    plot_d_orbit(real-g_x_min, imag-g_y_max, color);
}

void scrub_orbit()
{
    driver_mute();
    ValueSaver save_screen_x_offset{g_logical_screen_x_offset, 0};
    ValueSaver save_screen_y_offset{g_logical_screen_y_offset, 0};
    while (g_orbit_save_index >= 3)
    {
        int c = s_save_orbit[--g_orbit_save_index];
        int j = s_save_orbit[--g_orbit_save_index];
        int i = s_save_orbit[--g_orbit_save_index];
        g_put_color(i, j, c);
    }
}

} // namespace id::engine
