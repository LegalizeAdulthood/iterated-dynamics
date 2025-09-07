// SPDX-License-Identifier: GPL-3.0-only
//
#include "fractals/Bifurcation.h"

#include "engine/calcfrac.h"
#include "engine/engine_timer.h"
#include "engine/fractals.h"
#include "engine/id_data.h"
#include "engine/resume.h"
#include "fractals/fractalp.h"
#include "fractals/population.h"
#include "math/arg.h"
#include "math/fixed_pt.h"
#include "misc/id.h"

#include <cmath>
#include <vector>

using namespace id::math;

namespace id::fractals
{

enum
{
    DEFAULT_FILTER = 1000     /* "Beauty of Fractals" recommends using 5000
                               (p.25), but that seems unnecessary. Can
                               override this value with a nonzero param1 */
};

constexpr double SEED{0.66}; // starting value for population

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
void Bifurcation::resume()
{
    start_resume();
    get_resume(m_x);
    end_resume();
}

Bifurcation::Bifurcation()
{
    m_verhulst.resize(g_i_stop_pt.y + 1);

    for (int y = 0; y <= g_i_stop_pt.y; y++) // should be iystop
    {
        m_verhulst[y] = 0;
    }

    m_mono = false;
    if (g_colors == 2)
    {
        m_mono = true;
    }
    if (m_mono)
    {
        if (g_inside_color != COLOR_BLACK)
        {
            m_outside_x = 0;
            g_inside_color = 1;
        }
        else
        {
            m_outside_x = 1;
        }
    }

    m_filter_cycles = (g_param_z1.x <= 0) ? DEFAULT_FILTER : (long) g_param_z1.x;
    m_half_time_check = false;
    if (g_periodicity_check && (unsigned long) g_max_iterations < m_filter_cycles)
    {
        m_filter_cycles = (m_filter_cycles - g_max_iterations + 1) / 2;
        m_half_time_check = true;
    }

    g_init.y = (double) (g_y_max - g_i_stop_pt.y * g_delta_y); // bottom pixels
}

void Bifurcation::suspend()
{
    m_verhulst.clear();
    alloc_resume(10, 1);
    put_resume(m_x);
}

bool Bifurcation::iterate()
{
    if (m_x <= g_i_stop_pt.x)
    {
        g_rate = (double) (g_x_min + m_x * g_delta_x);
        verhulst();        // calculate array once per column

        for (int y = g_i_stop_pt.y; y >= 0; y--) // should be iystop & >=0
        {
            int color = m_verhulst[y];
            if (color && m_mono)
            {
                color = g_inside_color;
            }
            else if ((!color) && m_mono)
            {
                color = m_outside_x;
            }
            else if (color>=g_colors)
            {
                color = g_colors-1;
            }
            m_verhulst[y] = 0;
            g_plot(m_x, y, color); // was row-1, but that's not right?
        }
        ++m_x;
        return true;
    }
    return false;
}

void Bifurcation::verhulst()          // P. F. Verhulst (1845)
{
    unsigned int pixel_row;

    g_population = (g_param_z1.y == 0) ? SEED : g_param_z1.y;

    g_overflow = false;

    for (unsigned long counter = 0UL; counter < m_filter_cycles; counter++)
    {
        if (g_cur_fractal_specific->orbit_calc())
        {
            return;
        }
    }
    if (m_half_time_check) // check for periodicity at half-time
    {
        s_period.init();
        unsigned long counter;
        for (counter = 0; counter < (unsigned long)g_max_iterations ; counter++)
        {
            if (g_cur_fractal_specific->orbit_calc())
            {
                return;
            }
            if (g_periodicity_check && s_period.periodic(counter))
            {
                break;
            }
        }
        if (counter >= (unsigned long)g_max_iterations)   // if not periodic, go the distance
        {
            for (counter = 0; counter < m_filter_cycles ; counter++)
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
        s_period.init();
    }
    for (unsigned long counter = 0UL; counter < (unsigned long)g_max_iterations ; counter++)
    {
        if (g_cur_fractal_specific->orbit_calc())
        {
            return;
        }

        // assign population value to Y coordinate in pixels
        pixel_row = g_i_stop_pt.y - (int) ((g_population - g_init.y) / g_delta_y);

        // if it's visible on the screen, save it in the column array
        if (pixel_row <= (unsigned int)g_i_stop_pt.y)
        {
            m_verhulst[ pixel_row ] ++;
        }
        if (g_periodicity_check && s_period.periodic(counter))
        {
            if (pixel_row <= (unsigned int)g_i_stop_pt.y)
            {
                m_verhulst[ pixel_row ] --;
            }
            break;
        }
    }
}

void BifurcationPeriod::init()
{
    saved_inc = 1;
    saved_and = 1;
    saved_pop = -1.0;
    close_enough = (double) g_delta_y / 8.0;
}

// Bifurcation Population Periodicity Check
// Returns : true if periodicity found, else false
bool BifurcationPeriod::periodic(long time)
{
    if ((time & saved_and) == 0)      // time to save a new value
    {
        saved_pop = g_population;
        if (--saved_inc == 0)
        {
            saved_and = (saved_and << 1) + 1;
            saved_inc = 4;
        }
    }
    else                         // check against an old save
    {
        if (std::abs(saved_pop - g_population) <= close_enough)
        {
            return true;
        }
    }
    return false;
}

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

} // namespace id::fractals
