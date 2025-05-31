// SPDX-License-Identifier: GPL-3.0-only
//
/*

Miscellaneous fractal-specific code

*/
#include "fractals/bifurcation.h"

#include "engine/calcfrac.h"
#include "engine/engine_timer.h"
#include "engine/fractals.h"
#include "engine/id_data.h"
#include "engine/resume.h"
#include "fractals/fractalp.h"
#include "fractals/population.h"
#include "math/arg.h"
#include "math/fixed_pt.h"
#include "misc/Driver.h"
#include "misc/id.h"
#include "ui/stop_msg.h"

#include <algorithm>
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
static bool s_mono{};
static int s_outside_x{};
static double s_bif_close_enough{}, s_bif_saved_pop{};
static int s_bif_saved_inc{};
static long s_bif_saved_and{};
static long s_beta{};

//*********** standalone engine for "bifurcation" types **************

//*************************************************************
// The following code now forms a generalised Fractal Engine
// for Bifurcation fractal typeS.  By rights, it now belongs in
// calcfract.cpp, but it's easier for me to leave it here !

// Besides generalisation, enhancements include Periodicity
// Checking during the plotting phase (AND halfway through the
// filter cycle, if possible, to halve calc times), quicker
// floating-point calculations for the standard Verhulst type,
// and new bifurcation types (integer bifurcation, f.p & int
// biflambda - the real equivalent of complex Lambda sets -
// and f.p renditions of bifurcations of r*sin(Pi*p), which
// spurred Mitchel Feigenbaum on to discover his Number).

// To add further types, extend the FractalSpecific[] array in
// usual way, with Bifurcation as the engine, and the name of
// the routine that calculates the next bifurcation generation
// as the "orbitcalc" routine in the FractalSpecific[] entry.

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
    try
    {
        s_verhulst_array.resize(g_i_y_stop + 1);
    }
    catch (const std::bad_alloc &)
    {
        stop_msg("Insufficient free memory for calculation.");
        return -1;
    }

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
    if (g_periodicity_check && (unsigned long) g_max_iterations < s_filter_cycles)
    {
        s_filter_cycles = (s_filter_cycles - g_max_iterations + 1) / 2;
        s_half_time_check = true;
    }

    g_init.y = (double) (g_y_max - g_i_y_stop * g_delta_y); // bottom pixels

    while (x <= g_i_x_stop)
    {
        if (driver_key_pressed())
        {
            s_verhulst_array.clear();
            alloc_resume(10, 1);
            put_resume(x);
            return -1;
        }

        g_rate = (double) (g_x_min + x * g_delta_x);
        verhulst();        // calculate array once per column

        for (int y = g_i_y_stop; y >= 0; y--) // should be iystop & >=0
        {
            int color = s_verhulst_array[y];
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

    g_population = (g_param_z1.y == 0) ? SEED : g_param_z1.y;

    g_overflow = false;

    for (unsigned long counter = 0UL; counter < s_filter_cycles ; counter++)
    {
        if (g_cur_fractal_specific->orbit_calc())
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
            if (g_cur_fractal_specific->orbit_calc())
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
                if (g_cur_fractal_specific->orbit_calc())
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
        if (g_cur_fractal_specific->orbit_calc())
        {
            return;
        }

        // assign population value to Y coordinate in pixels
        pixel_row = g_i_y_stop - (int) ((g_population - g_init.y) / g_delta_y);

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
    s_bif_saved_pop = -1.0;
    s_bif_close_enough = (double) g_delta_y / 8.0;
}

// Bifurcation Population Periodicity Check
// Returns : true if periodicity found, else false
static bool bif_periodic(long time)
{
    if ((time & s_bif_saved_and) == 0)      // time to save a new value
    {
        s_bif_saved_pop = g_population;
        if (--s_bif_saved_inc == 0)
        {
            s_bif_saved_and = (s_bif_saved_and << 1) + 1;
            s_bif_saved_inc = 4;
        }
    }
    else                         // check against an old save
    {
        if (std::abs(s_bif_saved_pop - g_population) <= s_bif_close_enough)
        {
            return true;
        }
    }
    return false;
}

//********************************************************************
/*                                                                                                    */
// The following are Bifurcation "orbitcalc" routines...
/*                                                                                                    */
//********************************************************************
int bifurc_verhulst_trig_orbit()
{
    //  Population = Pop + Rate * fn(Pop) * (1 - fn(Pop))
    g_tmp_z.x = g_population;
    g_tmp_z.y = 0;
    cmplx_trig0(g_tmp_z, g_tmp_z);
    g_population += g_rate * g_tmp_z.x * (1 - g_tmp_z.x);
    return population_orbit();
}

int bifurc_stewart_trig_orbit()
{
    //  Population = (Rate * fn(Population) * fn(Population)) - 1.0
    g_tmp_z.x = g_population;
    g_tmp_z.y = 0;
    cmplx_trig0(g_tmp_z, g_tmp_z);
    g_population = (g_rate * g_tmp_z.x * g_tmp_z.x) - 1.0;
    return population_orbit();
}

int bifurc_set_trig_pi_orbit()
{
    g_tmp_z.x = g_population * PI;
    g_tmp_z.y = 0;
    cmplx_trig0(g_tmp_z, g_tmp_z);
    g_population = g_rate * g_tmp_z.x;
    return population_orbit();
}

int bifurc_add_trig_pi_orbit()
{
    g_tmp_z.x = g_population * PI;
    g_tmp_z.y = 0;
    cmplx_trig0(g_tmp_z, g_tmp_z);
    g_population += g_rate * g_tmp_z.x;
    return population_orbit();
}

int bifurc_lambda_trig_orbit()
{
    //  Population = Rate * fn(Population) * (1 - fn(Population))
    g_tmp_z.x = g_population;
    g_tmp_z.y = 0;
    cmplx_trig0(g_tmp_z, g_tmp_z);
    g_population = g_rate * g_tmp_z.x * (1 - g_tmp_z.x);
    return population_orbit();
}

int bifurc_may_orbit()
{
    /* X = (lambda * X) / (1 + X)^beta, from R.May as described in Pickover,
            Computers, Pattern, Chaos, and Beauty, page 153 */
    g_tmp_z.x = 1.0 + g_population;
    g_tmp_z.x = std::pow(g_tmp_z.x, -s_beta); // pow in math.h included with math/mpmath.h
    g_population = (g_rate * g_population) * g_tmp_z.x;
    return population_orbit();
}

bool bifurc_may_per_image()
{
    g_params[2] = std::max(g_params[2], 2.0);
    s_beta = static_cast<long>(g_params[2]);
    engine_timer(g_cur_fractal_specific->calc_type);
    return false;
}
