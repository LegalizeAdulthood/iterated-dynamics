// SPDX-License-Identifier: GPL-3.0-only
//
#include "fractals/newton.h"

#include "engine/calcfrac.h"
#include "engine/fractals.h"
#include "engine/Inversion.h"
#include "engine/pixel_grid.h"
#include "fractals/fractalp.h"
#include "fractals/fractype.h"
#include "math/fixed_pt.h"
#include "math/fpu087.h"
#include "misc/id.h"

#include <cassert>
#include <cfloat>
#include <cmath>
#include <vector>

using namespace id::engine;
using namespace id::math;

namespace id::fractals
{

static double                s_degree_minus_1_over_degree{};
static double                s_newton_r_over_d{};
static std::vector<DComplex> s_roots;
static double                s_threshold{};

static double distance(const DComplex &z1, const DComplex &z2)
{
    return sqr(z1.x - z2.x) + sqr(z1.y - z2.y);
}

static double s_two_pi{};
static DComplex s_temp{};
static DComplex s_base_log{};
static DComplex s_c_degree{3.0, 0.0};
static DComplex s_c_root{1.0, 0.0};

// transform points with reciprocal function
void invertz2(DComplex *z)
{
    z->x = dx_pixel();
    z->y = dy_pixel();
    *z -= g_inversion.center;  // Normalize values to center of circle

    g_temp_sqr_x = sqr(z->x) + sqr(z->y);  // Get old radius
    if (std::abs(g_temp_sqr_x) > FLT_MIN)
    {
        g_temp_sqr_x = g_inversion.radius / g_temp_sqr_x;
    }
    else
    {
        g_temp_sqr_x = FLT_MAX;   // a big number, but not TOO big
    }
    z->x *= g_temp_sqr_x;
    z->y *= g_temp_sqr_x;         // Perform inversion
    *z += g_inversion.center;             // Renormalize
}

// Distance of complex z from unit circle
static double distance1(const DComplex &z)
{
    return sqr(z.x - 1.0) + sqr(z.y);
}

static int complex_mult(const DComplex arg1, const DComplex arg2, DComplex *pz)
{
    pz->x = arg1.x * arg2.x - arg1.y * arg2.y;
    pz->y = arg1.x * arg2.y + arg1.y * arg2.x;
    return 0;
}

int newton_orbit()
{
    static char start = 1;
    if (start)
    {
        start = 0;
    }
    pow(&g_old_z, g_degree-1, &g_tmp_z);
    complex_mult(g_tmp_z, g_old_z, &g_new_z);

    if (distance1(g_new_z) < s_threshold)
    {
        if (g_fractal_type == FractalType::NEWT_BASIN)
        {
            long tmp_color = -1;
            /* this code determines which degree-th root of root the
               Newton formula converges to. The roots of a 1 are
               distributed on a circle of radius 1 about the origin. */
            for (int i = 0; i < g_degree; i++)
            {
                /* color in alternating shades with iteration according to
                   which root of 1 it converged to */
                if (distance(s_roots[i], g_old_z) < s_threshold)
                {
                    if (g_basin == 2)
                    {
                        tmp_color = 1+(i&7)+((g_color_iter&1) << 3);
                    }
                    else
                    {
                        tmp_color = 1+i;
                    }
                    break;
                }
            }
            if (tmp_color == -1)
            {
                g_color_iter = g_max_color;
            }
            else
            {
                g_color_iter = tmp_color;
            }
        }
        return 1;
    }
    g_new_z.x = s_degree_minus_1_over_degree * g_new_z.x + s_newton_r_over_d;
    g_new_z.y *= s_degree_minus_1_over_degree;

    // Watch for divide underflow
    double t2 = g_tmp_z.x * g_tmp_z.x + g_tmp_z.y * g_tmp_z.y;
    if (t2 < FLT_MIN)
    {
        return 1;
    }
    t2 = 1.0 / t2;
    g_old_z.x = t2 * (g_new_z.x * g_tmp_z.x + g_new_z.y * g_tmp_z.y);
    g_old_z.y = t2 * (g_new_z.y * g_tmp_z.x - g_new_z.x * g_tmp_z.y);
    return 0;
}

bool complex_newton_per_image()
{
    s_threshold = .001;
    g_periodicity_check = 0;
    if (g_params[0] != 0.0
        || g_params[1] != 0.0
        || g_params[2] != 0.0
        || g_params[3] != 0.0)
    {
        s_c_root.x = g_params[2];
        s_c_root.y = g_params[3];
        s_c_degree.x = g_params[0];
        s_c_degree.y = g_params[1];
        cmplx_log(s_c_root, s_base_log);
        s_two_pi = std::asin(1.0) * 4;
    }
    return true;
}

int complex_newton_orbit()
{
    DComplex cd1;

    /* new = ((cdegree-1) * old**cdegree) + croot
             ----------------------------------
                  cdegree * old**(cdegree-1)         */

    cd1.x = s_c_degree.x - 1.0;
    cd1.y = s_c_degree.y;

    s_temp = complex_power(g_old_z, cd1);
    fpu_cmplx_mul(s_temp, g_old_z, g_new_z);

    g_tmp_z.x = g_new_z.x - s_c_root.x;
    g_tmp_z.y = g_new_z.y - s_c_root.y;
    if (sqr(g_tmp_z.x) + sqr(g_tmp_z.y) < s_threshold)
    {
        return 1;
    }

    fpu_cmplx_mul(g_new_z, cd1, g_tmp_z);
    g_tmp_z.x += s_c_root.x;
    g_tmp_z.y += s_c_root.y;

    fpu_cmplx_mul(s_temp, s_c_degree, cd1);
    fpu_cmplx_div(g_tmp_z, cd1, g_old_z);
    if (g_overflow)
    {
        return 1;
    }
    g_new_z = g_old_z;
    return 0;
}

int complex_basin_orbit()
{
    DComplex cd1;

    /* new = ((cdegree-1) * old**cdegree) + croot
             ----------------------------------
                  cdegree * old**(cdegree-1)         */

    cd1.x = s_c_degree.x - 1.0;
    cd1.y = s_c_degree.y;

    s_temp = complex_power(g_old_z, cd1);
    fpu_cmplx_mul(s_temp, g_old_z, g_new_z);

    g_tmp_z.x = g_new_z.x - s_c_root.x;
    g_tmp_z.y = g_new_z.y - s_c_root.y;
    if (sqr(g_tmp_z.x) + sqr(g_tmp_z.y) < s_threshold)
    {
        if (std::abs(g_old_z.y) < .01)
        {
            g_old_z.y = 0.0;
        }
        cmplx_log(g_old_z, s_temp);
        fpu_cmplx_mul(s_temp, s_c_degree, g_tmp_z);
        const double mod = g_tmp_z.y/s_two_pi;
        g_color_iter = static_cast<long>(mod);
        if (std::abs(mod - g_color_iter) > 0.5)
        {
            if (mod < 0.0)
            {
                g_color_iter--;
            }
            else
            {
                g_color_iter++;
            }
        }
        g_color_iter += 2;
        if (g_color_iter < 0)
        {
            g_color_iter += 128;
        }
        return 1;
    }

    fpu_cmplx_mul(g_new_z, cd1, g_tmp_z);
    g_tmp_z.x += s_c_root.x;
    g_tmp_z.y += s_c_root.y;

    fpu_cmplx_mul(s_temp, s_c_degree, cd1);
    fpu_cmplx_div(g_tmp_z, cd1, g_old_z);
    if (g_overflow)
    {
        return 1;
    }
    g_new_z = g_old_z;
    return 0;
}

// Newton/NewtBasin Routines
bool newton_per_image()
{
    // TODO: is it necessary to update g_cur_fractal_specific?
    assert(g_cur_fractal_specific == get_fractal_specific(g_fractal_type));
    g_cur_fractal_specific = get_fractal_specific(g_fractal_type);
    // set up table of roots of 1 along unit circle
    g_degree = static_cast<int>(g_param_z1.x);
    if (g_degree < 2)
    {
        g_degree = 3;   // defaults to 3, but 2 is possible
    }

    // precalculated values
    s_newton_r_over_d       = 1.0 / static_cast<double>(g_degree);
    s_degree_minus_1_over_degree      = static_cast<double>(g_degree - 1) / static_cast<double>(g_degree);
    g_max_color     = 0;
    s_threshold    = .3*PI/g_degree; // less than half distance between roots

    g_basin = 0;
    s_roots.resize(16);
    if (g_fractal_type == FractalType::NEWT_BASIN)
    {
        if (g_param_z1.y)
        {
            g_basin = 2; //stripes
        }
        else
        {
            g_basin = 1;
        }
        s_roots.resize(g_degree);

        // list of roots to discover where we converged for newtbasin
        for (int i = 0; i < g_degree; i++)
        {
            s_roots[i].x = std::cos(i*PI*2.0/ static_cast<double>(g_degree));
            s_roots[i].y = std::sin(i*PI*2.0/ static_cast<double>(g_degree));
        }
    }

    g_params[0] = static_cast<double>(g_degree);
    if (g_degree%4 == 0)
    {
        g_symmetry = SymmetryType::XY_AXIS;
    }
    else
    {
        g_symmetry = SymmetryType::X_AXIS;
    }

    g_calc_type = standard_fractal_type;
    return true;
}

} // namespace id::fractals
