// SPDX-License-Identifier: GPL-3.0-only
//
#include "engine/get_julia_attractor.h"

#include "engine/calcfrac.h"
#include "engine/cmdfiles.h"
#include "engine/fractals.h"
#include "engine/id_data.h"
#include "fractals/fractalp.h"
#include "math/cmplx.h"
#include "math/fixed_pt.h"

#include <algorithm>
#include <cmath>

namespace id
{

void get_julia_attractor(double real, double imag)
{
    DComplex result{};

    if (g_attractors == 0 && !g_finite_attractor)   // not magnet & not requested
    {
        return;
    }

    if (g_attractors >= MAX_NUM_ATTRACTORS)       // space for more attractors ?
    {
        return;                  // Bad luck - no room left !
    }

    int save_periodicity_check = g_periodicity_check;
    long save_max_iterations = g_max_iterations;
    g_periodicity_check = 0;
    g_old_z.x = real;                    // prepare for f.p orbit calc
    g_old_z.y = imag;
    g_temp_sqr_x = sqr(g_old_z.x);
    g_temp_sqr_y = sqr(g_old_z.y);

    // we're going to try at least this hard
    g_max_iterations = std::max(g_max_iterations, 500L);
    g_color_iter = 0;
    g_overflow = false;
    while (++g_color_iter < g_max_iterations)
    {
        if (g_cur_fractal_specific->orbit_calc() || g_overflow)
        {
            break;
        }
    }
    if (g_color_iter >= g_max_iterations)      // if orbit stays in the lake
    {
        result = g_new_z;
        for (int i = 0; i < 10; i++)
        {
            g_overflow = false;
            if (!g_cur_fractal_specific->orbit_calc() && !g_overflow) // if it stays in the lake
            {
                // and doesn't move far, probably found a finite attractor
                if (std::abs(result.x - g_new_z.x) < g_close_enough &&
                    std::abs(result.y - g_new_z.y) < g_close_enough)
                {
                    g_attractor[g_attractors] = g_new_z;
                    g_attractor_period[g_attractors] = i + 1;
                    g_attractors++; // another attractor - coloured lakes !
                    break;
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
        g_periodicity_check = save_periodicity_check;
    }
    g_max_iterations = save_max_iterations;
}

} // namespace id
