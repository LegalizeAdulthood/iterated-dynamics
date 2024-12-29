// SPDX-License-Identifier: GPL-3.0-only
//
/*

Miscellaneous fractal-specific code

*/
#include "miscfrac.h"

#include "calcfrac.h"
#include "cmdfiles.h"
#include "debug_flags.h"
#include "drivers.h"
#include "engine_timer.h"
#include "fixed_pt.h"
#include "fractalp.h"
#include "fractals.h"
#include "id.h"
#include "id_data.h"
#include "mpmath.h"
#include "newton.h"
#include "pixel_grid.h"
#include "resume.h"
#include "stop_msg.h"

#include <cmath>
#include <cstdlib>
#include <new>
#include <vector>

// routines in this module

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
        get_resume(x);
        end_resume();
    }
    bool resized = false;
    try
    {
        s_verhulst_array.resize(g_i_y_stop + 1);
        resized = true;
    }
    catch (const std::bad_alloc &)
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
            put_resume(x);
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

    timer(TimerType::ENGINE, g_cur_fractal_specific->calctype);
    return false;
}

// Here Endeth the Generalised Bifurcation Fractal Engine

// END Phil Wilson's Code (modified slightly by Kev Allen et. al. !)

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
    s_lya_seed_ok = g_params[1] > 0 && g_params[1] <= 1 && g_debug_flag != DebugFlags::force_standard_fractal;
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
