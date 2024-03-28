#include "get_julia_attractor.h"

#include "port.h"
#include "prototyp.h"

#include "calcfrac.h"
#include "cmdfiles.h"
#include "fixed_pt.h"
#include "fractalp.h"
#include "fractals.h"
#include "id_data.h"
#include "sqr.h"

#include <cmath>

void get_julia_attractor(double real, double imag)
{
    LComplex lresult = { 0 };
    DComplex result = { 0.0 };
    int savper;
    long savmaxit;

    if (g_attractors == 0 && !g_finite_attractor)   // not magnet & not requested
    {
        return;
    }

    if (g_attractors >= MAX_NUM_ATTRACTORS)       // space for more attractors ?
    {
        return;                  // Bad luck - no room left !
    }

    savper = g_periodicity_check;
    savmaxit = g_max_iterations;
    g_periodicity_check = 0;
    g_old_z.x = real;                    // prepare for f.p orbit calc
    g_old_z.y = imag;
    g_temp_sqr_x = sqr(g_old_z.x);
    g_temp_sqr_y = sqr(g_old_z.y);

    g_l_old_z.x = (long)real;     // prepare for int orbit calc
    g_l_old_z.y = (long)imag;
    g_l_temp_sqr_x = (long)g_temp_sqr_x;
    g_l_temp_sqr_y = (long)g_temp_sqr_y;

    g_l_old_z.x = g_l_old_z.x << g_bit_shift;
    g_l_old_z.y = g_l_old_z.y << g_bit_shift;
    g_l_temp_sqr_x = g_l_temp_sqr_x << g_bit_shift;
    g_l_temp_sqr_y = g_l_temp_sqr_y << g_bit_shift;

    if (g_max_iterations < 500)           // we're going to try at least this hard
    {
        g_max_iterations = 500;
    }
    g_color_iter = 0;
    g_overflow = false;
    while (++g_color_iter < g_max_iterations)
    {
        if (g_cur_fractal_specific->orbitcalc() || g_overflow)
        {
            break;
        }
    }
    if (g_color_iter >= g_max_iterations)      // if orbit stays in the lake
    {
        if (g_integer_fractal)     // remember where it went to
        {
            lresult = g_l_new_z;
        }
        else
        {
            result =  g_new_z;
        }
        for (int i = 0; i < 10; i++)
        {
            g_overflow = false;
            if (!g_cur_fractal_specific->orbitcalc() && !g_overflow) // if it stays in the lake
            {
                // and doesn't move far, probably
                if (g_integer_fractal)   //   found a finite attractor
                {
                    if (labs(lresult.x-g_l_new_z.x) < g_l_close_enough
                        && labs(lresult.y-g_l_new_z.y) < g_l_close_enough)
                    {
                        g_l_attractor[g_attractors] = g_l_new_z;
                        g_attractor_period[g_attractors] = i+1;
                        g_attractors++;   // another attractor - coloured lakes !
                        break;
                    }
                }
                else
                {
                    if (std::fabs(result.x-g_new_z.x) < g_close_enough
                        && std::fabs(result.y-g_new_z.y) < g_close_enough)
                    {
                        g_attractor[g_attractors] = g_new_z;
                        g_attractor_period[g_attractors] = i+1;
                        g_attractors++;   // another attractor - coloured lakes !
                        break;
                    }
                }
            }
            else
            {
                break;
            }
        }
    }
    if (g_attractors == 0)
    {
        g_periodicity_check = savper;
    }
    g_max_iterations = savmaxit;
}
