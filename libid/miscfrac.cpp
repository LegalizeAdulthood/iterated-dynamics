// SPDX-License-Identifier: GPL-3.0-only
//
/*

Miscellaneous fractal-specific code

*/
#include "miscfrac.h"

#include "port.h"
#include "prototyp.h"

#include "calcfrac.h"
#include "check_key.h"
#include "cmdfiles.h"
#include "debug_flags.h"
#include "diskvid.h"
#include "drivers.h"
#include "engine_timer.h"
#include "fixed_pt.h"
#include "fpu087.h"
#include "fractalp.h"
#include "fractals.h"
#include "id.h"
#include "id_data.h"
#include "loadmap.h"
#include "mpmath.h"
#include "newton.h"
#include "not_disk_msg.h"
#include "parser.h"
#include "pixel_grid.h"
#include "resume.h"
#include "rotate.h"
#include "spindac.h"
#include "sqr.h"
#include "stop_msg.h"
#include "testpt.h"
#include "trig_fns.h"
#include "video.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

// routines in this module

static void set_plasma_palette();
static U16 adjust(int xa, int ya, int x, int y, int xb, int yb);
static void sub_divide(int x1, int y1, int x2, int y2);
static void verhulst();
static void bif_period_init();
static bool bif_periodic(long time);

enum
{
    DEFAULTFILTER = 1000     /* "Beauty of Fractals" recommends using 5000
                               (p.25), but that seems unnecessary. Can
                               override this value with a nonzero param1 */
};

constexpr double SEED{0.66}; // starting value for population

static constexpr U16 (*s_get_color)(int x, int y){[](int x, int y) { return (U16) get_color(x, y); }};
static U16 (*s_get_pix)(int x, int y){s_get_color};
static int s_i_parm_x{};     // iparmx = parm.x * 8
static int s_shift_value{}; // shift based on #colors
static int s_recur1{1};
static int s_p_colors{};
static int s_recur_level{};
static U16 s_max_plasma{};
static int s_plasma_check{};                        // to limit kbd checking
static std::vector<int> s_verhulst_array;
static unsigned long s_filter_cycles{};
static bool s_half_time_check{};
static long s_population_l{}, s_rate_l{};
static double s_population{}, s_rate{};
static bool s_mono{};
static int s_outside_x{};
static long s_pi_l{};
static long s_bif_close_enough_l{}, s_bif_saved_pop_l{}; // poss future use
static double s_bif_close_enough{}, s_bif_saved_pop{};
static int s_bif_saved_inc{};
static long s_bif_saved_and{};
static long s_beta{};
static int s_lya_length{}, s_lya_seed_ok{};
static int s_lya_rxy[34]{};

using PlotFn = void(*)(int, int, int);

//**************** standalone engine for "test" *******************

int test()
{
    int startpass = 0;
    int startrow = startpass;
    if (g_resuming)
    {
        start_resume();
        get_resume(sizeof(startrow), &startrow, sizeof(startpass), &startpass, 0);
        end_resume();
    }
    if (test_start())   // assume it was stand-alone, doesn't want passes logic
    {
        return 0;
    }
    int numpasses = (g_std_calc_mode == '1') ? 0 : 1;
    for (int passes = startpass; passes <= numpasses ; passes++)
    {
        for (g_row = startrow; g_row <= g_i_y_stop; g_row = g_row+1+numpasses)
        {
            for (g_col = 0; g_col <= g_i_x_stop; g_col++)       // look at each point on screen
            {
                int color;
                g_init.x = g_dx_pixel();
                g_init.y = g_dy_pixel();
                if (driver_key_pressed())
                {
                    test_end();
                    alloc_resume(20, 1);
                    put_resume(sizeof(g_row), &g_row, sizeof(passes), &passes, 0);
                    return -1;
                }
                color = test_pt(g_init.x, g_init.y, g_param_z1.x, g_param_z1.y, g_max_iterations, g_inside_color);
                if (color >= g_colors)
                {
                    // avoid trouble if color is 0
                    if (g_colors < 16)
                    {
                        color &= g_and_color;
                    }
                    else
                    {
                        color = ((color-1) % g_and_color) + 1; // skip color zero
                    }
                }
                (*g_plot)(g_col, g_row, color);
                if (numpasses && (passes == 0))
                {
                    (*g_plot)(g_col, g_row+1, color);
                }
            }
        }
        startrow = passes + 1;
    }
    test_end();
    return 0;
}

//**************** standalone engine for "plasma" *******************

// returns a random 16 bit value that is never 0
U16 rand16()
{
    U16 value;
    value = (U16)rand15();
    value <<= 1;
    value = (U16)(value + (rand15()&1));
    if (value < 1)
    {
        value = 1;
    }
    return value;
}

static void put_pot(int x, int y, U16 color)
{
    if (color < 1)
    {
        color = 1;
    }
    g_put_color(x, y, (color >> 8) ? (color >> 8) : 1);  // don't write 0
    /* we don't write this if driver_diskp() because the above putcolor
          was already a "writedisk" in that case */
    if (!driver_diskp())
    {
        disk_write_pixel(x+g_logical_screen_x_offset, y+g_logical_screen_y_offset, color >> 8);    // upper 8 bits
    }
    disk_write_pixel(x+g_logical_screen_x_offset, y+g_screen_y_dots+g_logical_screen_y_offset, color&255); // lower 8 bits
}

// fixes border
static void put_pot_border(int x, int y, U16 color)
{
    if ((x == 0) || (y == 0) || (x == g_logical_screen_x_dots-1) || (y == g_logical_screen_y_dots-1))
    {
        color = (U16)g_outside_color;
    }
    put_pot(x, y, color);
}

// fixes border
static void put_color_border(int x, int y, int color)
{
    if ((x == 0) || (y == 0) || (x == g_logical_screen_x_dots-1) || (y == g_logical_screen_y_dots-1))
    {
        color = g_outside_color;
    }
    if (color < 1)
    {
        color = 1;
    }
    g_put_color(x, y, color);
}

static U16 get_pot(int x, int y)
{
    U16 color;

    color = (U16)disk_read_pixel(x+g_logical_screen_x_offset, y+g_logical_screen_y_offset);
    color = (U16)((color << 8) + (U16) disk_read_pixel(x+g_logical_screen_x_offset, y+g_screen_y_dots+g_logical_screen_y_offset));
    return color;
}

static U16 adjust(int xa, int ya, int x, int y, int xb, int yb)
{
    S32 pseudorandom;
    pseudorandom = ((S32)s_i_parm_x)*((rand15()-16383));
    pseudorandom = pseudorandom * s_recur1;
    pseudorandom = pseudorandom >> s_shift_value;
    pseudorandom = (((S32)s_get_pix(xa, ya)+(S32)s_get_pix(xb, yb)+1) >> 1)+pseudorandom;
    if (s_max_plasma == 0)
    {
        if (pseudorandom >= s_p_colors)
        {
            pseudorandom = s_p_colors-1;
        }
    }
    else if (pseudorandom >= (S32)s_max_plasma)
    {
        pseudorandom = s_max_plasma;
    }
    if (pseudorandom < 1)
    {
        pseudorandom = 1;
    }
    g_plot(x, y, (U16)pseudorandom);
    return (U16)pseudorandom;
}

static bool new_sub_d(int x1, int y1, int x2, int y2, int recur)
{
    int x;
    int y;
    int nx1;
    int nx;
    int ny1;
    int ny;
    S32 i;
    S32 v;

    struct Sub
    {
        BYTE t; // top of stack
        int v[16]; // subdivided value
        BYTE r[16];  // recursion level
    };

    static Sub subx;
    static Sub suby;

    s_recur1 = (int)(320L >> recur);
    suby.t = 2;
    suby.v[0] = y2;
    ny   = suby.v[0];
    suby.v[2] = y1;
    ny1 = suby.v[2];
    suby.r[2] = 0;
    suby.r[0] = suby.r[2];
    suby.r[1] = 1;
    suby.v[1] = (ny1 + ny) >> 1;
    y = suby.v[1];

    while (suby.t >= 1)
    {
        if ((++s_plasma_check & 0x0f) == 1)
        {
            if (driver_key_pressed())
            {
                s_plasma_check--;
                return true;
            }
        }
        while (suby.r[suby.t-1] < (BYTE)recur)
        {
            //     1.  Create new entry at top of the stack
            //     2.  Copy old top value to new top value.
            //            This is largest y value.
            //     3.  Smallest y is now old mid point
            //     4.  Set new mid point recursion level
            //     5.  New mid point value is average
            //            of largest and smallest

            suby.t++;
            suby.v[suby.t] = suby.v[suby.t-1];
            ny1  = suby.v[suby.t];
            ny   = suby.v[suby.t-2];
            suby.r[suby.t] = suby.r[suby.t-1];
            suby.v[suby.t-1]   = (ny1 + ny) >> 1;
            y    = suby.v[suby.t-1];
            suby.r[suby.t-1]   = (BYTE)(std::max(suby.r[suby.t], suby.r[suby.t-2])+1);
        }
        subx.t = 2;
        subx.v[0] = x2;
        nx  = subx.v[0];
        subx.v[2] = x1;
        nx1 = subx.v[2];
        subx.r[2] = 0;
        subx.r[0] = subx.r[2];
        subx.r[1] = 1;
        subx.v[1] = (nx1 + nx) >> 1;
        x = subx.v[1];

        while (subx.t >= 1)
        {
            while (subx.r[subx.t-1] < (BYTE)recur)
            {
                subx.t++; // move the top ofthe stack up 1
                subx.v[subx.t] = subx.v[subx.t-1];
                nx1  = subx.v[subx.t];
                nx   = subx.v[subx.t-2];
                subx.r[subx.t] = subx.r[subx.t-1];
                subx.v[subx.t-1]   = (nx1 + nx) >> 1;
                x    = subx.v[subx.t-1];
                subx.r[subx.t-1]   = (BYTE)(std::max(subx.r[subx.t], subx.r[subx.t-2])+1);
            }

            i = s_get_pix(nx, y);
            if (i == 0)
            {
                i = adjust(nx, ny1, nx, y , nx, ny);
            }
            // cppcheck-suppress AssignmentIntegerToAddress
            v = i;
            i = s_get_pix(x, ny);
            if (i == 0)
            {
                i = adjust(nx1, ny, x , ny, nx, ny);
            }
            v += i;
            if (s_get_pix(x, y) == 0)
            {
                i = s_get_pix(x, ny1);
                if (i == 0)
                {
                    i = adjust(nx1, ny1, x , ny1, nx, ny1);
                }
                v += i;
                i = s_get_pix(nx1, y);
                if (i == 0)
                {
                    i = adjust(nx1, ny1, nx1, y , nx1, ny);
                }
                v += i;
                g_plot(x, y, (U16)((v + 2) >> 2));
            }

            if (subx.r[subx.t-1] == (BYTE)recur)
            {
                subx.t = (BYTE)(subx.t - 2);
            }
        }

        if (suby.r[suby.t-1] == (BYTE)recur)
        {
            suby.t = (BYTE)(suby.t - 2);
        }
    }
    return false;
}

static void sub_divide(int x1, int y1, int x2, int y2)
{
    int x;
    int y;
    S32 v;
    S32 i;
    if ((++s_plasma_check & 0x7f) == 1)
    {
        if (driver_key_pressed())
        {
            s_plasma_check--;
            return;
        }
    }
    if (x2-x1 < 2 && y2-y1 < 2)
    {
        return;
    }
    s_recur_level++;
    s_recur1 = (int)(320L >> s_recur_level);

    x = (x1+x2) >> 1;
    y = (y1+y2) >> 1;
    v = s_get_pix(x, y1);
    if (v == 0)
    {
        v = adjust(x1, y1, x , y1, x2, y1);
    }
    i = v;
    v = s_get_pix(x2, y);
    if (v == 0)
    {
        v = adjust(x2, y1, x2, y , x2, y2);
    }
    i += v;
    v = s_get_pix(x, y2);
    if (v == 0)
    {
        v = adjust(x1, y2, x , y2, x2, y2);
    }
    i += v;
    v = s_get_pix(x1, y);
    if (v == 0)
    {
        v = adjust(x1, y1, x1, y , x1, y2);
    }
    i += v;

    if (s_get_pix(x, y) == 0)
    {
        g_plot(x, y, (U16)((i+2) >> 2));
    }

    sub_divide(x1, y1, x , y);
    sub_divide(x , y1, x2, y);
    sub_divide(x , y , x2, y2);
    sub_divide(x1, y , x , y2);
    s_recur_level--;
}

int plasma()
{
    U16 rnd[4];
    bool OldPotFlag = false;
    bool OldPot16bit = false;
    s_plasma_check = 0;

    if (g_colors < 4)
    {
        stop_msg("Plasma Clouds can requires 4 or more color video");
        return -1;
    }
    s_i_parm_x = (int)(g_params[0] * 8);
    if (g_param_z1.x <= 0.0)
    {
        s_i_parm_x = 0;
    }
    if (g_param_z1.x >= 100)
    {
        s_i_parm_x = 800;
    }
    g_params[0] = (double)s_i_parm_x / 8.0;  // let user know what was used
    if (g_params[1] < 0)
    {
        g_params[1] = 0;  // limit parameter values
    }
    if (g_params[1] > 1)
    {
        g_params[1] = 1;
    }
    if (g_params[2] < 0)
    {
        g_params[2] = 0;  // limit parameter values
    }
    if (g_params[2] > 1)
    {
        g_params[2] = 1;
    }
    if (g_params[3] < 0)
    {
        g_params[3] = 0;  // limit parameter values
    }
    if (g_params[3] > 1)
    {
        g_params[3] = 1;
    }

    if (!g_random_seed_flag && g_params[2] == 1)
    {
        --g_random_seed;
    }
    if (g_params[2] != 0 && g_params[2] != 1)
    {
        g_random_seed = (int)g_params[2];
    }
    s_max_plasma = (U16)g_params[3];  // max_plasma is used as a flag for potential

    if (s_max_plasma != 0)
    {
        if (pot_start_disk() >= 0)
        {
            s_max_plasma = 0xFFFF;
            if (g_outside_color >= COLOR_BLACK)
            {
                g_plot    = (PlotFn)put_pot_border;
            }
            else
            {
                g_plot    = (PlotFn)put_pot;
            }
            s_get_pix =  get_pot;
            OldPotFlag = g_potential_flag;
            OldPot16bit = g_potential_16bit;
        }
        else
        {
            s_max_plasma = 0;        // can't do potential (startdisk failed)
            g_params[3]   = 0;
            if (g_outside_color >= COLOR_BLACK)
            {
                g_plot    = put_color_border;
            }
            else
            {
                g_plot    = g_put_color;
            }
            s_get_pix = s_get_color;
        }
    }
    else
    {
        if (g_outside_color >= COLOR_BLACK)
        {
            g_plot    = put_color_border;
        }
        else
        {
            g_plot    = g_put_color;
        }
            s_get_pix = s_get_color;
    }
    std::srand(g_random_seed);
    if (!g_random_seed_flag)
    {
        ++g_random_seed;
    }

    if (g_colors == 256)                     // set the (256-color) palette
    {
        set_plasma_palette();             // skip this if < 256 colors
    }

    if (g_colors > 16)
    {
        s_shift_value = 18;
    }
    else
    {
        if (g_colors > 4)
        {
            s_shift_value = 22;
        }
        else
        {
            if (g_colors > 2)
            {
                s_shift_value = 24;
            }
            else
            {
                s_shift_value = 25;
            }
        }
    }
    if (s_max_plasma != 0)
    {
        s_shift_value = 10;
    }

    if (s_max_plasma == 0)
    {
        s_p_colors = std::min(g_colors, 256);
        for (auto &elem : rnd)
        {
            elem = (U16)(1+(((rand15()/s_p_colors)*(s_p_colors-1)) >> (s_shift_value-11)));
        }
    }
    else
    {
        for (auto &elem : rnd)
        {
            elem = rand16();
        }
    }
    if (g_debug_flag == debug_flags::prevent_plasma_random)
    {
        for (auto &elem : rnd)
        {
            elem = 1;
        }
    }

    g_plot(0,      0,  rnd[0]);
    g_plot(g_logical_screen_x_dots-1,      0,  rnd[1]);
    g_plot(g_logical_screen_x_dots-1, g_logical_screen_y_dots-1,  rnd[2]);
    g_plot(0, g_logical_screen_y_dots-1,  rnd[3]);

    int n;
    s_recur_level = 0;
    if (g_params[1] == 0)
    {
        sub_divide(0, 0, g_logical_screen_x_dots-1, g_logical_screen_y_dots-1);
    }
    else
    {
        int i = 1;
        int k = 1;
        s_recur1 = 1;
        while (new_sub_d(0, 0, g_logical_screen_x_dots-1, g_logical_screen_y_dots-1, i) == 0)
        {
            k = k * 2;
            if (k  >(int)std::max(g_logical_screen_x_dots-1, g_logical_screen_y_dots-1))
            {
                break;
            }
            if (driver_key_pressed())
            {
                n = 1;
                goto done;
            }
            i++;
        }
    }
    if (!driver_key_pressed())
    {
        n = 0;
    }
    else
    {
        n = 1;
    }
done:
    if (s_max_plasma != 0)
    {
        g_potential_flag = OldPotFlag;
        g_potential_16bit = OldPot16bit;
    }
    g_plot    = g_put_color;
    s_get_pix = s_get_color;
    return n;
}

static void set_plasma_palette()
{
    static BYTE const Red[3]   = { 63, 0, 0 };
    static BYTE const Green[3] = { 0, 63, 0 };
    static BYTE const Blue[3]  = { 0,  0, 63 };

    if (g_map_specified || g_colors_preloaded)
    {
        return;    // map= specified
    }

    g_dac_box[0][0] = 0;
    g_dac_box[0][1] = 0;
    g_dac_box[0][2] = 0;
    for (int i = 1; i <= 85; i++)
    {
        g_dac_box[i][0] = (BYTE)((i*Green[0] + (86-i)*Blue[0])/85);
        g_dac_box[i][1] = (BYTE)((i*Green[1] + (86-i)*Blue[1])/85);
        g_dac_box[i][2] = (BYTE)((i*Green[2] + (86-i)*Blue[2])/85);

        g_dac_box[i+85][0] = (BYTE)((i*Red[0] + (86-i)*Green[0])/85);
        g_dac_box[i+85][1] = (BYTE)((i*Red[1] + (86-i)*Green[1])/85);
        g_dac_box[i+85][2] = (BYTE)((i*Red[2] + (86-i)*Green[2])/85);
        g_dac_box[i+170][0] = (BYTE)((i*Blue[0] + (86-i)*Red[0])/85);
        g_dac_box[i+170][1] = (BYTE)((i*Blue[1] + (86-i)*Red[1])/85);
        g_dac_box[i+170][2] = (BYTE)((i*Blue[2] + (86-i)*Red[2])/85);
    }
    spin_dac(0, 1);
}

//**************** standalone engine for "diffusion" *******************

inline int random(int x)
{
    return std::rand() % x;
}

int diffusion()
{
    int xmax;
    int ymax;
    int xmin;
    int ymin;   // Current maximum coordinates
    int border; // Distance between release point and fractal
    int mode;   // Determines diffusion type:  0 = central (classic)
    //                             1 = falling particles
    //                             2 = square cavity
    int colorshift; // If zero, select colors at random, otherwise shift the color every colorshift points
    int colorcount;
    int currentcolor;
    double cosine;
    double sine;
    double angle;
    int x;
    int y;
    float r;
    float radius;

    if (driver_diskp())
    {
        not_disk_msg();
    }

    y = -1;
    x = y;
    g_bit_shift = 16;
    g_fudge_factor = 1L << 16;

    border = (int)g_params[0];
    mode = (int)g_params[1];
    colorshift = (int)g_params[2];

    colorcount = colorshift; // Counts down from colorshift
    currentcolor = 1;  // Start at color 1 (color 0 is probably invisible)

    if (mode > 2)
    {
        mode = 0;
    }

    if (border <= 0)
    {
        border = 10;
    }

    std::srand(g_random_seed);
    if (!g_random_seed_flag)
    {
        ++g_random_seed;
    }

    if (mode == 0)
    {
        xmax = g_logical_screen_x_dots / 2 + border;  // Initial box
        xmin = g_logical_screen_x_dots / 2 - border;
        ymax = g_logical_screen_y_dots / 2 + border;
        ymin = g_logical_screen_y_dots / 2 - border;
    }
    if (mode == 1)
    {
        xmax = g_logical_screen_x_dots / 2 + border;  // Initial box
        xmin = g_logical_screen_x_dots / 2 - border;
        ymin = g_logical_screen_y_dots - border;
    }
    if (mode == 2)
    {
        if (g_logical_screen_x_dots > g_logical_screen_y_dots)
        {
            radius = (float)(g_logical_screen_y_dots - border);
        }
        else
        {
            radius = (float)(g_logical_screen_x_dots - border);
        }
    }
    if (g_resuming) // restore worklist, if we can't the above will stay in place
    {
        start_resume();
        if (mode != 2)
        {
            get_resume(sizeof(xmax), &xmax, sizeof(xmin), &xmin, sizeof(ymax), &ymax,
                       sizeof(ymin), &ymin, 0);
        }
        else
        {
            get_resume(sizeof(xmax), &xmax, sizeof(xmin), &xmin, sizeof(ymax), &ymax,
                       sizeof(radius), &radius, 0);
        }
        end_resume();
    }

    switch (mode)
    {
    case 0: // Single seed point in the center
        g_put_color(g_logical_screen_x_dots / 2, g_logical_screen_y_dots / 2, currentcolor);
        break;
    case 1: // Line along the bottom
        for (int i = 0; i <= g_logical_screen_x_dots; i++)
        {
            g_put_color(i, g_logical_screen_y_dots-1, currentcolor);
        }
        break;
    case 2: // Large square that fills the screen
        if (g_logical_screen_x_dots > g_logical_screen_y_dots)
        {
            for (int i = 0; i < g_logical_screen_y_dots; i++)
            {
                g_put_color(g_logical_screen_x_dots/2-g_logical_screen_y_dots/2 , i , currentcolor);
                g_put_color(g_logical_screen_x_dots/2+g_logical_screen_y_dots/2 , i , currentcolor);
                g_put_color(g_logical_screen_x_dots/2-g_logical_screen_y_dots/2+i , 0 , currentcolor);
                g_put_color(g_logical_screen_x_dots/2-g_logical_screen_y_dots/2+i , g_logical_screen_y_dots-1 , currentcolor);
            }
        }
        else
        {
            for (int i = 0; i < g_logical_screen_x_dots; i++)
            {
                g_put_color(0 , g_logical_screen_y_dots/2-g_logical_screen_x_dots/2+i , currentcolor);
                g_put_color(g_logical_screen_x_dots-1 , g_logical_screen_y_dots/2-g_logical_screen_x_dots/2+i , currentcolor);
                g_put_color(i , g_logical_screen_y_dots/2-g_logical_screen_x_dots/2 , currentcolor);
                g_put_color(i , g_logical_screen_y_dots/2+g_logical_screen_x_dots/2 , currentcolor);
            }
        }
        break;
    }

    while (true)
    {
        switch (mode)
        {
        case 0: // Release new point on a circle inside the box
            angle = 2*(double)std::rand()/(RAND_MAX/PI);
            sin_cos(&angle, &sine, &cosine);
            x = (int)(cosine*(xmax-xmin) + g_logical_screen_x_dots);
            y = (int)(sine  *(ymax-ymin) + g_logical_screen_y_dots);
            x = x >> 1; // divide by 2
            y = y >> 1;
            break;
        case 1: /* Release new point on the line ymin somewhere between xmin
                 and xmax */
            y = ymin;
            x = random(xmax-xmin) + (g_logical_screen_x_dots-xmax+xmin)/2;
            break;
        case 2: /* Release new point on a circle inside the box with radius
                 given by the radius variable */
            angle = 2*(double)std::rand()/(RAND_MAX/PI);
            sin_cos(&angle, &sine, &cosine);
            x = (int)(cosine*radius + g_logical_screen_x_dots);
            y = (int)(sine  *radius + g_logical_screen_y_dots);
            x = x >> 1;
            y = y >> 1;
            break;
        }

        // Loop as long as the point (x,y) is surrounded by color 0
        // on all eight sides

        while ((get_color(x+1, y+1) == 0) && (get_color(x+1, y) == 0)
            && (get_color(x+1, y-1) == 0) && (get_color(x  , y+1) == 0)
            && (get_color(x  , y-1) == 0) && (get_color(x-1, y+1) == 0)
            && (get_color(x-1, y) == 0) && (get_color(x-1, y-1) == 0))
        {
            // Erase moving point
            if (g_show_orbit)
            {
                g_put_color(x, y, 0);
            }

            if (mode == 0)
            {
                // Make sure point is inside the box
                if (x == xmax)
                {
                    x--;
                }
                else if (x == xmin)
                {
                    x++;
                }
                if (y == ymax)
                {
                    y--;
                }
                else if (y == ymin)
                {
                    y++;
                }
            }

            if (mode == 1) /* Make sure point is on the screen below ymin, but
                    we need a 1 pixel margin because of the next random step.*/
            {
                if (x >= g_logical_screen_x_dots-1)
                {
                    x--;
                }
                else if (x <= 1)
                {
                    x++;
                }
                if (y < ymin)
                {
                    y++;
                }
            }

            // Take one random step
            x += random(3) - 1;
            y += random(3) - 1;

            // Check keyboard
            if ((++s_plasma_check & 0x7f) == 1)
            {
                if (check_key())
                {
                    alloc_resume(20, 1);
                    if (mode != 2)
                    {
                        put_resume(sizeof(xmax), &xmax, sizeof(xmin), &xmin,
                                   sizeof(ymax), &ymax, sizeof(ymin), &ymin, 0);
                    }
                    else
                    {
                        put_resume(sizeof(xmax), &xmax, sizeof(xmin), &xmin,
                                   sizeof(ymax), &ymax, sizeof(radius), &radius, 0);
                    }

                    s_plasma_check--;
                    return 1;
                }
            }

            // Show the moving point
            if (g_show_orbit)
            {
                g_put_color(x, y, random(g_colors-1)+1);
            }
        } // End of loop, now fix the point

        /* If we're doing colorshifting then use currentcolor, otherwise
           pick one at random */
        g_put_color(x, y, colorshift?currentcolor:random(g_colors-1)+1);

        // If we're doing colorshifting then check to see if we need to shift
        if (colorshift)
        {
            if (!--colorcount)
            {
                // If the counter reaches zero then shift
                currentcolor++;      // Increase the current color and wrap
                currentcolor %= g_colors;  // around skipping zero
                if (!currentcolor)
                {
                    currentcolor++;
                }
                colorcount = colorshift;  // and reset the counter
            }
        }

        /* If the new point is close to an edge, we may need to increase
           some limits so that the limits expand to match the growing
           fractal. */

        switch (mode)
        {
        case 0:
            if (((x+border) > xmax) || ((x-border) < xmin)
                || ((y-border) < ymin) || ((y+border) > ymax))
            {
                // Increase box size, but not past the edge of the screen
                ymin--;
                ymax++;
                xmin--;
                xmax++;
                if ((ymin == 0) || (xmin == 0))
                {
                    return 0;
                }
            }
            break;
        case 1: // Decrease ymin, but not past top of screen
            if (y-border < ymin)
            {
                ymin--;
            }
            if (ymin == 0)
            {
                return 0;
            }
            break;
        case 2: /* Decrease the radius where points are released to stay away
                 from the fractal.  It might be decreased by 1 or 2 */
            r = sqr((float)x-g_logical_screen_x_dots/2) + sqr((float)y-g_logical_screen_y_dots/2);
            if (r <= border*border)
            {
                return 0;
            }
            while ((radius-border)*(radius-border) > r)
            {
                radius--;
            }
            break;
        }
    }
}

//*********** standalone engine for "bifurcation" types **************

//*************************************************************
// The following code now forms a generalised Fractal Engine
// for Bifurcation fractal typeS.  By rights it now belongs in
// CALCFRACT.C, but it's easier for me to leave it here !

// Besides generalisation, enhancements include Periodicity
// Checking during the plotting phase (AND halfway through the
// filter cycle, if possible, to halve calc times), quicker
// floating-point calculations for the standard Verhulst type,
// and new bifurcation types (integer bifurcation, f.p & int
// biflambda - the real equivalent of complex Lambda sets -
// and f.p renditions of bifurcations of r*sin(Pi*p), which
// spurred Mitchel Feigenbaum on to discover his Number).

// To add further types, extend the fractalspecific[] array in
// usual way, with Bifurcation as the engine, and the name of
// the routine that calculates the next bifurcation generation
// as the "orbitcalc" routine in the fractalspecific[] entry.

// Bifurcation "orbitcalc" routines get called once per screen
// pixel column.  They should calculate the next generation
// from the doubles Rate & Population (or the longs lRate &
// lPopulation if they use integer math), placing the result
// back in Population (or lPopulation).  They should return 0
// if all is ok, or any non-zero value if calculation bailout
// is desirable (e.g. in case of errors, or the series tending
// to infinity).                Have fun !
//*************************************************************

inline bool population_exceeded()
{
    constexpr double limit{100000.0};
    return std::fabs(s_population) > limit;
}

inline int population_orbit()
{
    return population_exceeded() ? 1 : 0;
}

int bifurcation()
{
    int x = 0;
    if (g_resuming)
    {
        start_resume();
        get_resume(sizeof(x), &x, 0);
        end_resume();
    }
    bool resized = false;
    try
    {
        s_verhulst_array.resize(g_i_y_stop + 1);
        resized = true;
    }
    catch (std::bad_alloc const&)
    {
    }
    if (!resized)
    {
        stop_msg("Insufficient free memory for calculation.");
        return -1;
    }

    s_pi_l = (long)(PI * g_fudge_factor);

    for (int y = 0; y <= g_i_y_stop; y++)   // should be iystop
    {
        s_verhulst_array[y] = 0;
    }

    s_mono = false;
    if (g_colors == 2)
    {
        s_mono = true;
    }
    if (s_mono)
    {
        if (g_inside_color != COLOR_BLACK)
        {
            s_outside_x = 0;
            g_inside_color = 1;
        }
        else
        {
            s_outside_x = 1;
        }
    }

    s_filter_cycles = (g_param_z1.x <= 0) ? DEFAULTFILTER : (long)g_param_z1.x;
    s_half_time_check = false;
    if (g_periodicity_check && (unsigned long)g_max_iterations < s_filter_cycles)
    {
        s_filter_cycles = (s_filter_cycles - g_max_iterations + 1) / 2;
        s_half_time_check = true;
    }

    if (g_integer_fractal)
    {
        g_l_init.y = g_l_y_max - g_i_y_stop*g_l_delta_y;            // Y-value of
    }
    else
    {
        g_init.y = (double)(g_y_max - g_i_y_stop*g_delta_y); // bottom pixels
    }

    while (x <= g_i_x_stop)
    {
        if (driver_key_pressed())
        {
            s_verhulst_array.clear();
            alloc_resume(10, 1);
            put_resume(sizeof(x), &x, 0);
            return -1;
        }

        if (g_integer_fractal)
        {
            s_rate_l = g_l_x_min + x*g_l_delta_x;
        }
        else
        {
            s_rate = (double)(g_x_min + x*g_delta_x);
        }
        verhulst();        // calculate array once per column

        for (int y = g_i_y_stop; y >= 0; y--) // should be iystop & >=0
        {
            int color;
            color = s_verhulst_array[y];
            if (color && s_mono)
            {
                color = g_inside_color;
            }
            else if ((!color) && s_mono)
            {
                color = s_outside_x;
            }
            else if (color>=g_colors)
            {
                color = g_colors-1;
            }
            s_verhulst_array[y] = 0;
            (*g_plot)(x, y, color); // was row-1, but that's not right?
        }
        x++;
    }
    s_verhulst_array.clear();
    return 0;
}

static void verhulst()          // P. F. Verhulst (1845)
{
    unsigned int pixel_row;

    if (g_integer_fractal)
    {
        s_population_l = (g_param_z1.y == 0) ? (long)(SEED*g_fudge_factor) : (long)(g_param_z1.y*g_fudge_factor);
    }
    else
    {
        s_population = (g_param_z1.y == 0) ? SEED : g_param_z1.y;
    }

    g_overflow = false;

    for (unsigned long counter = 0UL; counter < s_filter_cycles ; counter++)
    {
        if (g_cur_fractal_specific->orbitcalc())
        {
            return;
        }
    }
    if (s_half_time_check) // check for periodicity at half-time
    {
        bif_period_init();
        unsigned long counter;
        for (counter = 0; counter < (unsigned long)g_max_iterations ; counter++)
        {
            if (g_cur_fractal_specific->orbitcalc())
            {
                return;
            }
            if (g_periodicity_check && bif_periodic(counter))
            {
                break;
            }
        }
        if (counter >= (unsigned long)g_max_iterations)   // if not periodic, go the distance
        {
            for (counter = 0; counter < s_filter_cycles ; counter++)
            {
                if (g_cur_fractal_specific->orbitcalc())
                {
                    return;
                }
            }
        }
    }

    if (g_periodicity_check)
    {
        bif_period_init();
    }
    for (unsigned long counter = 0UL; counter < (unsigned long)g_max_iterations ; counter++)
    {
        if (g_cur_fractal_specific->orbitcalc())
        {
            return;
        }

        // assign population value to Y coordinate in pixels
        if (g_integer_fractal)
        {
            pixel_row = g_i_y_stop - (int)((s_population_l - g_l_init.y) / g_l_delta_y); // iystop
        }
        else
        {
            pixel_row = g_i_y_stop - (int)((s_population - g_init.y) / g_delta_y);
        }

        // if it's visible on the screen, save it in the column array
        if (pixel_row <= (unsigned int)g_i_y_stop)
        {
            s_verhulst_array[ pixel_row ] ++;
        }
        if (g_periodicity_check && bif_periodic(counter))
        {
            if (pixel_row <= (unsigned int)g_i_y_stop)
            {
                s_verhulst_array[ pixel_row ] --;
            }
            break;
        }
    }
}

static void bif_period_init()
{
    s_bif_saved_inc = 1;
    s_bif_saved_and = 1;
    if (g_integer_fractal)
    {
        s_bif_saved_pop_l = -1;
        s_bif_close_enough_l = g_l_delta_y / 8;
    }
    else
    {
        s_bif_saved_pop = -1.0;
        s_bif_close_enough = (double)g_delta_y / 8.0;
    }
}

// Bifurcation Population Periodicity Check
// Returns : true if periodicity found, else false
static bool bif_periodic(long time)
{
    if ((time & s_bif_saved_and) == 0)      // time to save a new value
    {
        if (g_integer_fractal)
        {
            s_bif_saved_pop_l = s_population_l;
        }
        else
        {
            s_bif_saved_pop =  s_population;
        }
        if (--s_bif_saved_inc == 0)
        {
            s_bif_saved_and = (s_bif_saved_and << 1) + 1;
            s_bif_saved_inc = 4;
        }
    }
    else                         // check against an old save
    {
        if (g_integer_fractal)
        {
            if (labs(s_bif_saved_pop_l-s_population_l) <= s_bif_close_enough_l)
            {
                return true;
            }
        }
        else
        {
            if (std::fabs(s_bif_saved_pop-s_population) <= s_bif_close_enough)
            {
                return true;
            }
        }
    }
    return false;
}

//********************************************************************
/*                                                                                                    */
// The following are Bifurcation "orbitcalc" routines...
/*                                                                                                    */
//********************************************************************
int bifurc_lambda() // Used by lyanupov
{
    s_population = s_rate * s_population * (1 - s_population);
    return population_orbit();
}

int bifurc_verhulst_trig()
{
    //  Population = Pop + Rate * fn(Pop) * (1 - fn(Pop))
    g_tmp_z.x = s_population;
    g_tmp_z.y = 0;
    cmplx_trig0(g_tmp_z, g_tmp_z);
    s_population += s_rate * g_tmp_z.x * (1 - g_tmp_z.x);
    return population_orbit();
}

int long_bifurc_verhulst_trig()
{
    g_l_temp.x = s_population_l;
    g_l_temp.y = 0;
    trig0(g_l_temp, g_l_temp);
    g_l_temp.y = g_l_temp.x - multiply(g_l_temp.x, g_l_temp.x, g_bit_shift);
    s_population_l += multiply(s_rate_l, g_l_temp.y, g_bit_shift);
    return g_overflow;
}

int bifurc_stewart_trig()
{
    //  Population = (Rate * fn(Population) * fn(Population)) - 1.0
    g_tmp_z.x = s_population;
    g_tmp_z.y = 0;
    cmplx_trig0(g_tmp_z, g_tmp_z);
    s_population = (s_rate * g_tmp_z.x * g_tmp_z.x) - 1.0;
    return population_orbit();
}

int long_bifurc_stewart_trig()
{
    g_l_temp.x = s_population_l;
    g_l_temp.y = 0;
    trig0(g_l_temp, g_l_temp);
    s_population_l = multiply(g_l_temp.x, g_l_temp.x, g_bit_shift);
    s_population_l = multiply(s_population_l, s_rate_l,      g_bit_shift);
    s_population_l -= g_fudge_factor;
    return g_overflow;
}

int bifurc_set_trig_pi()
{
    g_tmp_z.x = s_population * PI;
    g_tmp_z.y = 0;
    cmplx_trig0(g_tmp_z, g_tmp_z);
    s_population = s_rate * g_tmp_z.x;
    return population_orbit();
}

int long_bifurc_set_trig_pi()
{
    g_l_temp.x = multiply(s_population_l, s_pi_l, g_bit_shift);
    g_l_temp.y = 0;
    trig0(g_l_temp, g_l_temp);
    s_population_l = multiply(s_rate_l, g_l_temp.x, g_bit_shift);
    return g_overflow;
}

int bifurc_add_trig_pi()
{
    g_tmp_z.x = s_population * PI;
    g_tmp_z.y = 0;
    cmplx_trig0(g_tmp_z, g_tmp_z);
    s_population += s_rate * g_tmp_z.x;
    return population_orbit();
}

int long_bifurc_add_trig_pi()
{
    g_l_temp.x = multiply(s_population_l, s_pi_l, g_bit_shift);
    g_l_temp.y = 0;
    trig0(g_l_temp, g_l_temp);
    s_population_l += multiply(s_rate_l, g_l_temp.x, g_bit_shift);
    return g_overflow;
}

int bifurc_lambda_trig()
{
    //  Population = Rate * fn(Population) * (1 - fn(Population))
    g_tmp_z.x = s_population;
    g_tmp_z.y = 0;
    cmplx_trig0(g_tmp_z, g_tmp_z);
    s_population = s_rate * g_tmp_z.x * (1 - g_tmp_z.x);
    return population_orbit();
}

int long_bifurc_lambda_trig()
{
    g_l_temp.x = s_population_l;
    g_l_temp.y = 0;
    trig0(g_l_temp, g_l_temp);
    g_l_temp.y = g_l_temp.x - multiply(g_l_temp.x, g_l_temp.x, g_bit_shift);
    s_population_l = multiply(s_rate_l, g_l_temp.y, g_bit_shift);
    return g_overflow;
}

int bifurc_may()
{
    /* X = (lambda * X) / (1 + X)^beta, from R.May as described in Pickover,
            Computers, Pattern, Chaos, and Beauty, page 153 */
    g_tmp_z.x = 1.0 + s_population;
    g_tmp_z.x = std::pow(g_tmp_z.x, -s_beta); // pow in math.h included with mpmath.h
    s_population = (s_rate * s_population) * g_tmp_z.x;
    return population_orbit();
}

int long_bifurc_may()
{
    g_l_temp.x = s_population_l + g_fudge_factor;
    g_l_temp.y = 0;
    g_l_param2.x = s_beta * g_fudge_factor;
    lcmplx_pwr(g_l_temp, g_l_param2, g_l_temp);
    s_population_l = multiply(s_rate_l, s_population_l, g_bit_shift);
    s_population_l = divide(s_population_l, g_l_temp.x, g_bit_shift);
    return g_overflow;
}

bool bifurc_may_setup()
{

    s_beta = (long)g_params[2];
    if (s_beta < 2)
    {
        s_beta = 2;
    }
    g_params[2] = (double)s_beta;

    timer(timer_type::ENGINE, g_cur_fractal_specific->calctype);
    return false;
}

// Here Endeth the Generalised Bifurcation Fractal Engine

// END Phil Wilson's Code (modified slightly by Kev Allen et. al. !)

//****************** standalone engine for "popcorn" *******************

int popcorn()   // subset of std engine
{
    int start_row;
    start_row = 0;
    if (g_resuming)
    {
        start_resume();
        get_resume(sizeof(start_row), &start_row, 0);
        end_resume();
    }
    g_keyboard_check_interval = g_max_keyboard_check_interval;
    g_plot = no_plot;
    g_l_temp_sqr_x = 0;
    g_temp_sqr_x = g_l_temp_sqr_x;
    for (g_row = start_row; g_row <= g_i_y_stop; g_row++)
    {
        g_reset_periodicity = true;
        for (g_col = 0; g_col <= g_i_x_stop; g_col++)
        {
            if (standard_fractal() == -1) // interrupted
            {
                alloc_resume(10, 1);
                put_resume(sizeof(g_row), &g_row, 0);
                return -1;
            }
            g_reset_periodicity = false;
        }
    }
    g_calc_status = calc_status_value::COMPLETED;
    return 0;
}

//****************** standalone engine for "lyapunov" ********************
//** save_release behavior:                                             **
//**    1730 & prior: ignores inside=, calcmode='1', (a,b)->(x,y)       **
//**    1731: other calcmodes and inside=nnn                            **
//**    1732: the infamous axis swap: (b,a)->(x,y),                     **
//**            the order parameter becomes a long int                  **
//************************************************************************

static int lyapunov_cycles(long filter_cycles, double a, double b);

int lyapunov()
{
    double a;
    double b;

    if (driver_key_pressed())
    {
        return -1;
    }
    g_overflow = false;
    if (g_params[1] == 1)
    {
        s_population = (1.0+std::rand())/(2.0+RAND_MAX);
    }
    else if (g_params[1] == 0)
    {
        if (population_exceeded() || s_population == 0 || s_population == 1)
        {
            s_population = (1.0+std::rand())/(2.0+RAND_MAX);
        }
    }
    else
    {
        s_population = g_params[1];
    }
    (*g_plot)(g_col, g_row, 1);
    if (g_invert != 0)
    {
        invertz2(&g_init);
        a = g_init.y;
        b = g_init.x;
    }
    else
    {
        a = g_dy_pixel();
        b = g_dx_pixel();
    }
    g_color = lyapunov_cycles(s_filter_cycles, a, b);
    if (g_inside_color > COLOR_BLACK && g_color == 0)
    {
        g_color = g_inside_color;
    }
    else if (g_color>=g_colors)
    {
        g_color = g_colors-1;
    }
    (*g_plot)(g_col, g_row, g_color);
    return g_color;
}

bool lya_setup()
{
    /* This routine sets up the sequence for forcing the Rate parameter
        to vary between the two values.  It fills the array lyaRxy[] and
        sets lyaLength to the length of the sequence.

        The sequence is coded in the bit pattern in an integer.
        Briefly, the sequence starts with an A the leading zero bits
        are ignored and the remaining bit sequence is decoded.  The
        sequence ends with a B.  Not all possible sequences can be
        represented in this manner, but every possible sequence is
        either represented as itself, as a rotation of one of the
        representable sequences, or as the inverse of a representable
        sequence (swapping 0s and 1s in the array.)  Sequences that
        are the rotation and/or inverses of another sequence will generate
        the same lyapunov exponents.

        A few examples follow:
            number    sequence
                0       ab
                1       aab
                2       aabb
                3       aaab
                4       aabbb
                5       aabab
                6       aaabb (this is a duplicate of 4, a rotated inverse)
                7       aaaab
                8       aabbbb  etc.
         */

    long i;

    s_filter_cycles = (long)g_params[2];
    if (s_filter_cycles == 0)
    {
        s_filter_cycles = g_max_iterations/2;
    }
    s_lya_seed_ok = g_params[1] > 0 && g_params[1] <= 1 && g_debug_flag != debug_flags::force_standard_fractal;
    s_lya_length = 1;

    i = (long)g_params[0];
    s_lya_rxy[0] = 1;
    int t;
    for (t = 31; t >= 0; t--)
    {
        if (i & (1 << t))
        {
            break;
        }
    }
    for (; t >= 0; t--)
    {
        s_lya_rxy[s_lya_length++] = (i & (1<<t)) != 0;
    }
    s_lya_rxy[s_lya_length++] = 0;
    if (g_inside_color < COLOR_BLACK)
    {
        stop_msg("Sorry, inside options other than inside=nnn are not supported by the lyapunov");
        g_inside_color = 1;
    }
    if (g_user_std_calc_mode == 'o')
    {
        // Oops,lyapunov type
        g_user_std_calc_mode = '1';  // doesn't use new & breaks orbits
        g_std_calc_mode = '1';
    }
    return true;
}

static int lyapunov_cycles(long filter_cycles, double a, double b)
{
    int color;
    int lnadjust;
    double total;
    double temp;
    // e10=22026.4657948  e-10=0.0000453999297625

    total = 1.0;
    lnadjust = 0;
    long i;
    for (i = 0; i < filter_cycles; i++)
    {
        for (int count = 0; count < s_lya_length; count++)
        {
            s_rate = s_lya_rxy[count] ? a : b;
            if (g_cur_fractal_specific->orbitcalc())
            {
                g_overflow = true;
                goto jumpout;
            }
        }
    }
    for (i = 0; i < g_max_iterations/2; i++)
    {
        for (int count = 0; count < s_lya_length; count++)
        {
            s_rate = s_lya_rxy[count] ? a : b;
            if (g_cur_fractal_specific->orbitcalc())
            {
                g_overflow = true;
                goto jumpout;
            }
            temp = std::fabs(s_rate-2.0*s_rate*s_population);
            total *= temp;
            if (total == 0)
            {
                g_overflow = true;
                goto jumpout;
            }
        }
        while (total > 22026.4657948)
        {
            total *= 0.0000453999297625;
            lnadjust += 10;
        }
        while (total < 0.0000453999297625)
        {
            total *= 22026.4657948;
            lnadjust -= 10;
        }
    }

jumpout:
    if (g_overflow || total <= 0 || (temp = std::log(total) + lnadjust) > 0)
    {
        color = 0;
    }
    else
    {
        double lyap;
        if (g_log_map_flag)
        {
            lyap = -temp/((double) s_lya_length*i);
        }
        else
        {
            lyap = 1 - std::exp(temp/((double) s_lya_length*i));
        }
        color = 1 + (int)(lyap * (g_colors-1));
    }
    return color;
}
