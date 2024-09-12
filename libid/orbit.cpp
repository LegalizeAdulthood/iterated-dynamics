#include "orbit.h"

#include "port.h"
#include "prototyp.h"

#include "calcfrac.h"
#include "cmdfiles.h"
#include "debug_flags.h"
#include "drivers.h"
#include "fixed_pt.h"
#include "id_data.h"
#include "sound.h"
#include "video.h"
#include "wait_until.h"

static void   plotdorbit(double, double, int);

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

static void plotdorbit(double dx, double dy, int color)
{
    int i;
    int j;
    int save_sxoffs;
    int save_syoffs;
    if (g_orbit_save_index >= NUM_SAVE_ORBIT-3)
    {
        return;
    }
    i = (int)(dy * g_plot_mx1 - dx * g_plot_mx2);
    i += g_logical_screen_x_offset;
    if (i < 0 || i >= g_screen_x_dots)
    {
        return;
    }
    j = (int)(dx * g_plot_my1 - dy * g_plot_my2);
    j += g_logical_screen_y_offset;
    if (j < 0 || j >= g_screen_y_dots)
    {
        return;
    }
    save_sxoffs = g_logical_screen_x_offset;
    save_syoffs = g_logical_screen_y_offset;
    g_logical_screen_y_offset = 0;
    g_logical_screen_x_offset = g_logical_screen_y_offset;
    // save orbit value
    if (color == -1)
    {
        s_save_orbit[g_orbit_save_index++] = i;
        s_save_orbit[g_orbit_save_index++] = j;
        int const c = getcolor(i, j);
        s_save_orbit[g_orbit_save_index++] = c;
        g_put_color(i, j, c^g_orbit_color);
    }
    else
    {
        g_put_color(i, j, color);
    }
    g_logical_screen_x_offset = save_sxoffs;
    g_logical_screen_y_offset = save_syoffs;
    if (g_debug_flag == debug_flags::force_scaled_sound_formula)
    {
        if ((g_sound_flag & SOUNDFLAG_ORBITMASK) == SOUNDFLAG_X)   // sound = x
        {
            w_snd((int)(i*1000/g_logical_screen_x_dots+g_base_hertz));
        }
        else if ((g_sound_flag & SOUNDFLAG_ORBITMASK) > SOUNDFLAG_X)     // sound = y or z
        {
            w_snd((int)(j*1000/g_logical_screen_y_dots+g_base_hertz));
        }
        else if (g_orbit_delay > 0)
        {
            wait_until(0, g_orbit_delay);
        }
    }
    else
    {
        if ((g_sound_flag & SOUNDFLAG_ORBITMASK) == SOUNDFLAG_X)   // sound = x
        {
            w_snd((int)(i+g_base_hertz));
        }
        else if ((g_sound_flag & SOUNDFLAG_ORBITMASK) == SOUNDFLAG_Y)     // sound = y
        {
            w_snd((int)(j+g_base_hertz));
        }
        else if ((g_sound_flag & SOUNDFLAG_ORBITMASK) == SOUNDFLAG_Z)     // sound = z
        {
            w_snd((int)(i+j+g_base_hertz));
        }
        else if (g_orbit_delay > 0)
        {
            wait_until(0, g_orbit_delay);
        }
    }

    // placing sleepms here delays each dot
}

void iplot_orbit(long ix, long iy, int color)
{
    plotdorbit((double)ix/g_fudge_factor-g_x_min, (double)iy/g_fudge_factor-g_y_max, color);
}

void plot_orbit(double real, double imag, int color)
{
    plotdorbit(real-g_x_min, imag-g_y_max, color);
}

void scrub_orbit()
{
    int i;
    int j;
    int c;
    int save_sxoffs;
    int save_syoffs;
    driver_mute();
    save_sxoffs = g_logical_screen_x_offset;
    save_syoffs = g_logical_screen_y_offset;
    g_logical_screen_y_offset = 0;
    g_logical_screen_x_offset = g_logical_screen_y_offset;
    while (g_orbit_save_index >= 3)
    {
        c = s_save_orbit[--g_orbit_save_index];
        j = s_save_orbit[--g_orbit_save_index];
        i = s_save_orbit[--g_orbit_save_index];
        g_put_color(i, j, c);
    }
    g_logical_screen_x_offset = save_sxoffs;
    g_logical_screen_y_offset = save_syoffs;
}
