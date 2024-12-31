// SPDX-License-Identifier: GPL-3.0-only
//
#include "plasma.h"

#include "calcfrac.h"
#include "cmdfiles.h"
#include "debug_flags.h"
#include "diskvid.h"
#include "drivers.h"
#include "fractals.h"
#include "id_data.h"
#include "rand15.h"
#include "rotate.h"
#include "sized_types.h"
#include "spindac.h"
#include "stop_msg.h"
#include "video.h"

#include <algorithm>
#include <cstdlib>

// routines in this module

static void set_plasma_palette();
static U16 adjust(int xa, int ya, int x, int y, int xb, int yb);
static void sub_divide(int x1, int y1, int x2, int y2);

enum
{
    DEFAULT_FILTER = 1000     /* "Beauty of Fractals" recommends using 5000
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
static int s_kbd_check{};                        // to limit kbd checking

using PlotFn = void(*)(int, int, int);

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
        Byte t; // top of stack
        int v[16]; // subdivided value
        Byte r[16];  // recursion level
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
        if ((++s_kbd_check & 0x0f) == 1)
        {
            if (driver_key_pressed())
            {
                s_kbd_check--;
                return true;
            }
        }
        while (suby.r[suby.t-1] < (Byte)recur)
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
            suby.r[suby.t-1]   = (Byte)(std::max(suby.r[suby.t], suby.r[suby.t-2])+1);
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
            while (subx.r[subx.t-1] < (Byte)recur)
            {
                subx.t++; // move the top ofthe stack up 1
                subx.v[subx.t] = subx.v[subx.t-1];
                nx1  = subx.v[subx.t];
                nx   = subx.v[subx.t-2];
                subx.r[subx.t] = subx.r[subx.t-1];
                subx.v[subx.t-1]   = (nx1 + nx) >> 1;
                x    = subx.v[subx.t-1];
                subx.r[subx.t-1]   = (Byte)(std::max(subx.r[subx.t], subx.r[subx.t-2])+1);
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

            if (subx.r[subx.t-1] == (Byte)recur)
            {
                subx.t = (Byte)(subx.t - 2);
            }
        }

        if (suby.r[suby.t-1] == (Byte)recur)
        {
            suby.t = (Byte)(suby.t - 2);
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
    if ((++s_kbd_check & 0x7f) == 1)
    {
        if (driver_key_pressed())
        {
            s_kbd_check--;
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
    s_kbd_check = 0;

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
    if (g_debug_flag == DebugFlags::PREVENT_PLASMA_RANDOM)
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
    static Byte const Red[3]   = { 63, 0, 0 };
    static Byte const Green[3] = { 0, 63, 0 };
    static Byte const Blue[3]  = { 0,  0, 63 };

    if (g_map_specified || g_colors_preloaded)
    {
        return;    // map= specified
    }

    g_dac_box[0][0] = 0;
    g_dac_box[0][1] = 0;
    g_dac_box[0][2] = 0;
    for (int i = 1; i <= 85; i++)
    {
        g_dac_box[i][0] = (Byte)((i*Green[0] + (86-i)*Blue[0])/85);
        g_dac_box[i][1] = (Byte)((i*Green[1] + (86-i)*Blue[1])/85);
        g_dac_box[i][2] = (Byte)((i*Green[2] + (86-i)*Blue[2])/85);

        g_dac_box[i+85][0] = (Byte)((i*Red[0] + (86-i)*Green[0])/85);
        g_dac_box[i+85][1] = (Byte)((i*Red[1] + (86-i)*Green[1])/85);
        g_dac_box[i+85][2] = (Byte)((i*Red[2] + (86-i)*Green[2])/85);
        g_dac_box[i+170][0] = (Byte)((i*Blue[0] + (86-i)*Red[0])/85);
        g_dac_box[i+170][1] = (Byte)((i*Blue[1] + (86-i)*Red[1])/85);
        g_dac_box[i+170][2] = (Byte)((i*Blue[2] + (86-i)*Red[2])/85);
    }
    spin_dac(0, 1);
}
