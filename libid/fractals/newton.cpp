// SPDX-License-Identifier: GPL-3.0-only
//
#include "fractals/newton.h"

#include "calcfrac.h"
#include "cmdfiles.h"
#include "debug_flags.h"
#include "fpu087.h"
#include "fractals.h"
#include "fractals/fractalp.h"
#include "fractype.h"
#include "id_data.h"
#include "mpmath.h"
#include "mpmath_c.h"
#include "pixel_grid.h"

#include <cfloat>
#include <cmath>
#include <vector>

static double                s_degree_minus_1_over_degree{};
static std::vector<MPC>      s_mpc_roots;
static double                s_newton_r_over_d{};
static std::vector<DComplex> s_roots;
static double                s_threshold{};

inline double distance(const DComplex &z1, const DComplex &z2)
{
    return sqr(z1.x - z2.x) + sqr(z1.y - z2.y);
}

inline MP mp_sqr(MP z)
{
    return *mp_mul(z, z);
}

inline MP mp_distance(const MPC &z1, const MPC &z2)
{
    return *mp_add(mp_sqr(*mp_sub(z1.x, z2.x)), mp_sqr(*mp_sub(z1.y, z2.y)));
}

static MPC s_mpc_old{};
static MPC s_mpc_temp1{};
static double s_two_pi{};
static DComplex s_temp{};
static DComplex s_base_log{};
static DComplex s_c_degree{3.0, 0.0};
static DComplex s_c_root{1.0, 0.0};
static MP s_newton_mp_r_over_d{};
static MP s_mp_degree_minus_1_over_degree{};
static MP s_mp_threshold{};

// transform points with reciprocal function
void invertz2(DComplex *z)
{
    z->x = g_dx_pixel();
    z->y = g_dy_pixel();
    z->x -= g_f_x_center;
    z->y -= g_f_y_center;  // Normalize values to center of circle

    g_temp_sqr_x = sqr(z->x) + sqr(z->y);  // Get old radius
    if (std::fabs(g_temp_sqr_x) > FLT_MIN)
    {
        g_temp_sqr_x = g_f_radius / g_temp_sqr_x;
    }
    else
    {
        g_temp_sqr_x = FLT_MAX;   // a big number, but not TOO big
    }
    z->x *= g_temp_sqr_x;
    z->y *= g_temp_sqr_x;      // Perform inversion
    z->x += g_f_x_center;
    z->y += g_f_y_center; // Renormalize
}

// Distance of complex z from unit circle
inline double distance1(const DComplex &z)
{
    return sqr(z.x - 1.0) + sqr(z.y);
}

int newton_fractal2()
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
        if (g_fractal_type == FractalType::NEWT_BASIN || g_fractal_type == FractalType::NEWT_BASIN_MP)
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
    else
    {
        t2 = 1.0 / t2;
        g_old_z.x = t2 * (g_new_z.x * g_tmp_z.x + g_new_z.y * g_tmp_z.y);
        g_old_z.y = t2 * (g_new_z.y * g_tmp_z.x - g_new_z.x * g_tmp_z.y);
    }
    return 0;
}

bool complex_newton_setup()
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
        fpu_cmplx_log(&s_c_root, &s_base_log);
        s_two_pi = std::asin(1.0) * 4;
    }
    return true;
}

int complex_newton()
{
    DComplex cd1;

    /* new = ((cdegree-1) * old**cdegree) + croot
             ----------------------------------
                  cdegree * old**(cdegree-1)         */

    cd1.x = s_c_degree.x - 1.0;
    cd1.y = s_c_degree.y;

    s_temp = complex_power(g_old_z, cd1);
    fpu_cmplx_mul(&s_temp, &g_old_z, &g_new_z);

    g_tmp_z.x = g_new_z.x - s_c_root.x;
    g_tmp_z.y = g_new_z.y - s_c_root.y;
    if ((sqr(g_tmp_z.x) + sqr(g_tmp_z.y)) < s_threshold)
    {
        return 1;
    }

    fpu_cmplx_mul(&g_new_z, &cd1, &g_tmp_z);
    g_tmp_z.x += s_c_root.x;
    g_tmp_z.y += s_c_root.y;

    fpu_cmplx_mul(&s_temp, &s_c_degree, &cd1);
    fpu_cmplx_div(&g_tmp_z, &cd1, &g_old_z);
    if (g_overflow)
    {
        return 1;
    }
    g_new_z = g_old_z;
    return 0;
}

int complex_basin()
{
    DComplex cd1;

    /* new = ((cdegree-1) * old**cdegree) + croot
             ----------------------------------
                  cdegree * old**(cdegree-1)         */

    cd1.x = s_c_degree.x - 1.0;
    cd1.y = s_c_degree.y;

    s_temp = complex_power(g_old_z, cd1);
    fpu_cmplx_mul(&s_temp, &g_old_z, &g_new_z);

    g_tmp_z.x = g_new_z.x - s_c_root.x;
    g_tmp_z.y = g_new_z.y - s_c_root.y;
    if ((sqr(g_tmp_z.x) + sqr(g_tmp_z.y)) < s_threshold)
    {
        if (std::fabs(g_old_z.y) < .01)
        {
            g_old_z.y = 0.0;
        }
        fpu_cmplx_log(&g_old_z, &s_temp);
        fpu_cmplx_mul(&s_temp, &s_c_degree, &g_tmp_z);
        double mod = g_tmp_z.y/s_two_pi;
        g_color_iter = (long)mod;
        if (std::fabs(mod - g_color_iter) > 0.5)
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

    fpu_cmplx_mul(&g_new_z, &cd1, &g_tmp_z);
    g_tmp_z.x += s_c_root.x;
    g_tmp_z.y += s_c_root.y;

    fpu_cmplx_mul(&s_temp, &s_c_degree, &cd1);
    fpu_cmplx_div(&g_tmp_z, &cd1, &g_old_z);
    if (g_overflow)
    {
        return 1;
    }
    g_new_z = g_old_z;
    return 0;
}

// Newton/NewtBasin Routines
bool newton_setup()
{
    if (g_debug_flag != DebugFlags::ALLOW_NEWTON_MP_TYPE)
    {
        if (g_fractal_type == FractalType::NEWTON_MP)
        {
            g_fractal_type = FractalType::NEWTON;
        }
        else if (g_fractal_type == FractalType::NEWT_BASIN_MP)
        {
            g_fractal_type = FractalType::NEWT_BASIN;
        }
        g_cur_fractal_specific = &g_fractal_specific[+g_fractal_type];
    }
    // set up table of roots of 1 along unit circle
    g_degree = (int)g_param_z1.x;
    if (g_degree < 2)
    {
        g_degree = 3;   // defaults to 3, but 2 is possible
    }

    // precalculated values
    s_newton_r_over_d       = 1.0 / (double)g_degree;
    s_degree_minus_1_over_degree      = (double)(g_degree - 1) / (double)g_degree;
    g_max_color     = 0;
    s_threshold    = .3*PI/g_degree; // less than half distance between roots
    if (g_fractal_type == FractalType::NEWTON_MP || g_fractal_type == FractalType::NEWT_BASIN_MP)
    {
        s_newton_mp_r_over_d = *d_to_mp(s_newton_r_over_d);
        s_mp_degree_minus_1_over_degree = *d_to_mp(s_degree_minus_1_over_degree);
        s_mp_threshold = *d_to_mp(s_threshold);
        g_mp_one = *d_to_mp(1.0);
    }

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
            s_roots[i].x = std::cos(i*PI*2.0/(double)g_degree);
            s_roots[i].y = std::sin(i*PI*2.0/(double)g_degree);
        }
    }
    else if (g_fractal_type == FractalType::NEWT_BASIN_MP)
    {
        if (g_param_z1.y)
        {
            g_basin = 2;    //stripes
        }
        else
        {
            g_basin = 1;
        }

        s_mpc_roots.resize(g_degree);

        // list of roots to discover where we converged for newtbasin
        for (int i = 0; i < g_degree; i++)
        {
            s_mpc_roots[i].x = *d_to_mp(std::cos(i*PI*2.0/(double)g_degree));
            s_mpc_roots[i].y = *d_to_mp(std::sin(i*PI*2.0/(double)g_degree));
        }
    }

    g_params[0] = (double)g_degree;
    if (g_degree%4 == 0)
    {
        g_symmetry = SymmetryType::XY_AXIS;
    }
    else
    {
        g_symmetry = SymmetryType::X_AXIS;
    }

    g_calc_type = standard_fractal;
    return true;
}

int mpc_newton_fractal()
{
    g_mp_overflow = false;
    MPC mpc_tmp = mpc_pow(s_mpc_old, g_degree - 1);

    MPC mpc_new;
    mpc_new.x = *mp_sub(*mp_mul(mpc_tmp.x, s_mpc_old.x), *mp_mul(mpc_tmp.y, s_mpc_old.y));
    mpc_new.y = *mp_add(*mp_mul(mpc_tmp.x, s_mpc_old.y), *mp_mul(mpc_tmp.y, s_mpc_old.x));
    s_mpc_temp1.x = *mp_sub(mpc_new.x, g_mpc_one.x);
    s_mpc_temp1.y = *mp_sub(mpc_new.y, g_mpc_one.y);
    if (mp_cmp(mpc_mod(s_mpc_temp1), s_mp_threshold) < 0)
    {
        if (g_fractal_type == FractalType::NEWT_BASIN_MP)
        {
            long tmp_color = -1;
            for (int i = 0; i < g_degree; i++)
                if (mp_cmp(mp_distance(s_mpc_roots[i], s_mpc_old), s_mp_threshold) < 0)
                {
                    if (g_basin == 2)
                    {
                        tmp_color = 1+(i&7) + ((g_color_iter&1) << 3);
                    }
                    else
                    {
                        tmp_color = 1+i;
                    }
                    break;
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

    mpc_new.x = *mp_add(*mp_mul(s_mp_degree_minus_1_over_degree, mpc_new.x), s_newton_mp_r_over_d);
    mpc_new.y = *mp_mul(mpc_new.y, s_mp_degree_minus_1_over_degree);
    MP temp2 = mpc_mod(mpc_tmp);
    temp2 = *mp_div(g_mp_one, temp2);
    s_mpc_old.x = *mp_mul(temp2, (*mp_add(*mp_mul(mpc_new.x, mpc_tmp.x), *mp_mul(mpc_new.y, mpc_tmp.y))));
    s_mpc_old.y = *mp_mul(temp2, (*mp_sub(*mp_mul(mpc_new.y, mpc_tmp.x), *mp_mul(mpc_new.x, mpc_tmp.y))));
    g_new_z.x = *mp_to_d(s_mpc_old.x);
    g_new_z.y = *mp_to_d(s_mpc_old.y);
    return g_mp_overflow ? 1 : 0;
}

int mpc_julia_per_pixel()
{
    // floating point julia
    // juliafp
    if (g_invert != 0)
    {
        invertz2(&g_old_z);
    }
    else
    {
        g_old_z.x = g_dx_pixel();
        g_old_z.y = g_dy_pixel();
    }
    s_mpc_old.x = *d_to_mp(g_old_z.x);
    s_mpc_old.y = *d_to_mp(g_old_z.y);
    return 0;
}
