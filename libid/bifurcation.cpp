// SPDX-License-Identifier: GPL-3.0-only
//
/*

Miscellaneous fractal-specific code

*/
#include "bifurcation.h"

#include "calcfrac.h"
#include "cmdfiles.h"
#include "drivers.h"
#include "engine_timer.h"
#include "fixed_pt.h"
#include "fractalp.h"
#include "fractals.h"
#include "id.h"
#include "id_data.h"
#include "mpmath.h"
#include "population.h"
#include "resume.h"
#include "stop_msg.h"

#include <cmath>
#include <new>
#include <vector>

static void verhulst();
static void bif_period_init();
static bool bif_periodic(long time);

enum
{
    DEFAULT_FILTER = 1000     /* "Beauty of Fractals" recommends using 5000
                               (p.25), but that seems unnecessary. Can
                               override this value with a nonzero param1 */
};

constexpr double SEED{0.66}; // starting value for population

static std::vector<int> s_verhulst_array;
static unsigned long s_filter_cycles{};
static bool s_half_time_check{};
static long s_population_l{}, s_rate_l{};
static bool s_mono{};
static int s_outside_x{};
static long s_pi_l{};
static long s_bif_close_enough_l{}, s_bif_saved_pop_l{}; // poss future use
static double s_bif_close_enough{}, s_bif_saved_pop{};
static int s_bif_saved_inc{};
static long s_bif_saved_and{};
static long s_beta{};

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

    s_filter_cycles = (g_param_z1.x <= 0) ? DEFAULT_FILTER : (long)g_param_z1.x;
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
            g_rate = (double)(g_x_min + x*g_delta_x);
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
        g_population = (g_param_z1.y == 0) ? SEED : g_param_z1.y;
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
            pixel_row = g_i_y_stop - (int)((g_population - g_init.y) / g_delta_y);
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
            s_bif_saved_pop =  g_population;
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
            if (std::fabs(s_bif_saved_pop-g_population) <= s_bif_close_enough)
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
int bifurc_verhulst_trig()
{
    //  Population = Pop + Rate * fn(Pop) * (1 - fn(Pop))
    g_tmp_z.x = g_population;
    g_tmp_z.y = 0;
    cmplx_trig0(g_tmp_z, g_tmp_z);
    g_population += g_rate * g_tmp_z.x * (1 - g_tmp_z.x);
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
    g_tmp_z.x = g_population;
    g_tmp_z.y = 0;
    cmplx_trig0(g_tmp_z, g_tmp_z);
    g_population = (g_rate * g_tmp_z.x * g_tmp_z.x) - 1.0;
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
    g_tmp_z.x = g_population * PI;
    g_tmp_z.y = 0;
    cmplx_trig0(g_tmp_z, g_tmp_z);
    g_population = g_rate * g_tmp_z.x;
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
    g_tmp_z.x = g_population * PI;
    g_tmp_z.y = 0;
    cmplx_trig0(g_tmp_z, g_tmp_z);
    g_population += g_rate * g_tmp_z.x;
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
    g_tmp_z.x = g_population;
    g_tmp_z.y = 0;
    cmplx_trig0(g_tmp_z, g_tmp_z);
    g_population = g_rate * g_tmp_z.x * (1 - g_tmp_z.x);
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
    g_tmp_z.x = 1.0 + g_population;
    g_tmp_z.x = std::pow(g_tmp_z.x, -s_beta); // pow in math.h included with mpmath.h
    g_population = (g_rate * g_population) * g_tmp_z.x;
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

    engine_timer(g_cur_fractal_specific->calctype);
    return false;
}
