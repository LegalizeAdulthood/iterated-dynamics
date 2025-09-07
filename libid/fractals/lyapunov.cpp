// SPDX-License-Identifier: GPL-3.0-only
//
#include "fractals/lyapunov.h"

#include "engine/calcfrac.h"
#include "engine/fractals.h"
#include "engine/id_data.h"
#include "engine/log_map.h"
#include "engine/pixel_grid.h"
#include "fractals/fractalp.h"
#include "fractals/newton.h"
#include "fractals/population.h"
#include "math/fixed_pt.h"
#include "misc/debug_flags.h"
#include "misc/Driver.h"
#include "ui/stop_msg.h"

#include <cmath>
#include <cstdlib>

using namespace id;

static unsigned long s_filter_cycles{};
static int s_lya_length{};
static int s_lya_seed_ok{};
static int s_lya_rxy[34]{};

static int lyapunov_cycles(long filter_cycles, double a, double b);

// standalone engine for "lyapunov"
int lyapunov_type()
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
        g_population = (1.0+std::rand())/(2.0+RAND_MAX);
    }
    else if (g_params[1] == 0)
    {
        if (population_exceeded() || g_population == 0 || g_population == 1)
        {
            g_population = (1.0+std::rand())/(2.0+RAND_MAX);
        }
    }
    else
    {
        g_population = g_params[1];
    }
    g_plot(g_col, g_row, 1);
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
    g_plot(g_col, g_row, g_color);
    return g_color;
}

// This routine sets up the sequence for forcing the Rate parameter
// to vary between the two values.  It fills the array lyaRxy[] and
// sets lyaLength to the length of the sequence.
//
// The sequence is coded in the bit pattern in an integer.
// Briefly, the sequence starts with an A the leading zero bits
// are ignored and the remaining bit sequence is decoded.  The
// sequence ends with a B.  Not all possible sequences can be
// represented in this manner, but every possible sequence is
// either represented as itself, as a rotation of one of the
// representable sequences, or as the inverse of a representable
// sequence (swapping 0s and 1s in the array.)  Sequences that
// are the rotation and/or inverses of another sequence will generate
// the same lyapunov exponents.
//
// A few examples follow:
//     number    sequence
//         0       ab
//         1       aab
//         2       aabb
//         3       aaab
//         4       aabbb
//         5       aabab
//         6       aaabb (this is a duplicate of 4, a rotated inverse)
//         7       aaaab
//         8       aabbbb  etc.
//
bool lyapunov_per_image()
{
    s_filter_cycles = (long)g_params[2];
    if (s_filter_cycles == 0)
    {
        s_filter_cycles = g_max_iterations/2;
    }
    s_lya_seed_ok = g_params[1] > 0 && g_params[1] <= 1 && g_debug_flag != DebugFlags::FORCE_STANDARD_FRACTAL;
    s_lya_length = 1;

    long i = (long) g_params[0];
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
    if (g_user_std_calc_mode == CalcMode::ORBIT)
    {
        // Oops,lyapunov type
        g_user_std_calc_mode = CalcMode::ONE_PASS;  // doesn't use new & breaks orbits
        g_std_calc_mode = CalcMode::ONE_PASS;
    }
    return true;
}

static int lyapunov_cycles(long filter_cycles, double a, double b)
{
    int color;
    double temp;
    // e10=22026.4657948  e-10=0.0000453999297625

    double total = 1.0;
    int ln_adjust = 0;
    long i;
    for (i = 0; i < filter_cycles; i++)
    {
        for (int count = 0; count < s_lya_length; count++)
        {
            g_rate = s_lya_rxy[count] ? a : b;
            if (g_cur_fractal_specific->orbit_calc())
            {
                g_overflow = true;
                goto jump_out;
            }
        }
    }
    for (i = 0; i < g_max_iterations/2; i++)
    {
        for (int count = 0; count < s_lya_length; count++)
        {
            g_rate = s_lya_rxy[count] ? a : b;
            if (g_cur_fractal_specific->orbit_calc())
            {
                g_overflow = true;
                goto jump_out;
            }
            temp = std::abs(g_rate-2.0*g_rate*g_population);
            total *= temp;
            if (total == 0)
            {
                g_overflow = true;
                goto jump_out;
            }
        }
        while (total > 22026.4657948)
        {
            total *= 0.0000453999297625;
            ln_adjust += 10;
        }
        while (total < 0.0000453999297625)
        {
            total *= 22026.4657948;
            ln_adjust -= 10;
        }
    }

jump_out:
    if (g_overflow || total <= 0 || (temp = std::log(total) + ln_adjust) > 0)
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

int lyapunov_orbit()
{
    g_population = g_rate * g_population * (1 - g_population);
    return population_orbit();
}
