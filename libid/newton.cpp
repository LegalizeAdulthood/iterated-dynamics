// SPDX-License-Identifier: GPL-3.0-only
//
#include "newton.h"

#include "port.h"
#include "prototyp.h"

#include "calcfrac.h"
#include "cmdfiles.h"
#include "cmplx.h"
#include "debug_flags.h"
#include "fpu087.h"
#include "fractalp.h"
#include "fractals.h"
#include "fractype.h"
#include "id.h"
#include "id_data.h"
#include "mpmath.h"
#include "mpmath_c.h"
#include "pixel_grid.h"
#include "sqr.h"

#include <cmath>
#include <cfloat>

static double                s_degree_minus_1_over_degree{};
static std::vector<MPC>      s_mpc_roots;
static double                s_newton_r_over_d{};
static std::vector<DComplex> s_roots;

inline double distance(const DComplex &z1, const DComplex &z2)
{
    return sqr(z1.x - z2.x) + sqr(z1.y - z2.y);
}

inline MP pMPsqr(MP z)
{
    return *MPmul(z, z);
}

inline MP MPdistance(const MPC &z1, const MPC &z2)
{
    return *MPadd(pMPsqr(*MPsub(z1.x, z2.x)), pMPsqr(*MPsub(z1.y, z2.y)));
}

static MPC s_mpc_old{};
static MPC s_mpc_temp1{};
static double s_two_pi{};
static DComplex s_temp{};
static DComplex s_base_log{};
static DComplex s_cdegree{3.0, 0.0};
static DComplex s_croot{1.0, 0.0};
static MP s_newton_mp_r_over_d{};
static MP s_mp_degree_minus_1_over_degree{};
static MP s_mp_threshold{};

// this code translated to asm - lives in newton.asm
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

int NewtonFractal2()
{
    static char start = 1;
    if (start)
    {
        start = 0;
    }
    cpower(&g_old_z, g_degree-1, &g_tmp_z);
    complex_mult(g_tmp_z, g_old_z, &g_new_z);

    if (distance1(g_new_z) < g_threshold)
    {
        if (g_fractal_type == fractal_type::NEWTBASIN || g_fractal_type == fractal_type::MPNEWTBASIN)
        {
            long tmpcolor;
            tmpcolor = -1;
            /* this code determines which degree-th root of root the
               Newton formula converges to. The roots of a 1 are
               distributed on a circle of radius 1 about the origin. */
            for (int i = 0; i < g_degree; i++)
            {
                /* color in alternating shades with iteration according to
                   which root of 1 it converged to */
                if (distance(s_roots[i], g_old_z) < g_threshold)
                {
                    if (g_basin == 2)
                    {
                        tmpcolor = 1+(i&7)+((g_color_iter&1) << 3);
                    }
                    else
                    {
                        tmpcolor = 1+i;
                    }
                    break;
                }
            }
            if (tmpcolor == -1)
            {
                g_color_iter = g_max_color;
            }
            else
            {
                g_color_iter = tmpcolor;
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

bool ComplexNewtonSetup()
{
    g_threshold = .001;
    g_periodicity_check = 0;
    if (g_params[0] != 0.0
        || g_params[1] != 0.0
        || g_params[2] != 0.0
        || g_params[3] != 0.0)
    {
        s_croot.x = g_params[2];
        s_croot.y = g_params[3];
        s_cdegree.x = g_params[0];
        s_cdegree.y = g_params[1];
        FPUcplxlog(&s_croot, &s_base_log);
        s_two_pi = std::asin(1.0) * 4;
    }
    return true;
}

int ComplexNewton()
{
    DComplex cd1;

    /* new = ((cdegree-1) * old**cdegree) + croot
             ----------------------------------
                  cdegree * old**(cdegree-1)         */

    cd1.x = s_cdegree.x - 1.0;
    cd1.y = s_cdegree.y;

    s_temp = ComplexPower(g_old_z, cd1);
    FPUcplxmul(&s_temp, &g_old_z, &g_new_z);

    g_tmp_z.x = g_new_z.x - s_croot.x;
    g_tmp_z.y = g_new_z.y - s_croot.y;
    if ((sqr(g_tmp_z.x) + sqr(g_tmp_z.y)) < g_threshold)
    {
        return 1;
    }

    FPUcplxmul(&g_new_z, &cd1, &g_tmp_z);
    g_tmp_z.x += s_croot.x;
    g_tmp_z.y += s_croot.y;

    FPUcplxmul(&s_temp, &s_cdegree, &cd1);
    FPUcplxdiv(&g_tmp_z, &cd1, &g_old_z);
    if (g_overflow)
    {
        return 1;
    }
    g_new_z = g_old_z;
    return 0;
}

int ComplexBasin()
{
    DComplex cd1;

    /* new = ((cdegree-1) * old**cdegree) + croot
             ----------------------------------
                  cdegree * old**(cdegree-1)         */

    cd1.x = s_cdegree.x - 1.0;
    cd1.y = s_cdegree.y;

    s_temp = ComplexPower(g_old_z, cd1);
    FPUcplxmul(&s_temp, &g_old_z, &g_new_z);

    g_tmp_z.x = g_new_z.x - s_croot.x;
    g_tmp_z.y = g_new_z.y - s_croot.y;
    if ((sqr(g_tmp_z.x) + sqr(g_tmp_z.y)) < g_threshold)
    {
        if (std::fabs(g_old_z.y) < .01)
        {
            g_old_z.y = 0.0;
        }
        FPUcplxlog(&g_old_z, &s_temp);
        FPUcplxmul(&s_temp, &s_cdegree, &g_tmp_z);
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

    FPUcplxmul(&g_new_z, &cd1, &g_tmp_z);
    g_tmp_z.x += s_croot.x;
    g_tmp_z.y += s_croot.y;

    FPUcplxmul(&s_temp, &s_cdegree, &cd1);
    FPUcplxdiv(&g_tmp_z, &cd1, &g_old_z);
    if (g_overflow)
    {
        return 1;
    }
    g_new_z = g_old_z;
    return 0;
}

// Newton/NewtBasin Routines
bool NewtonSetup()
{
    if (g_debug_flag != debug_flags::allow_mp_newton_type)
    {
        if (g_fractal_type == fractal_type::MPNEWTON)
        {
            g_fractal_type = fractal_type::NEWTON;
        }
        else if (g_fractal_type == fractal_type::MPNEWTBASIN)
        {
            g_fractal_type = fractal_type::NEWTBASIN;
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
    g_threshold    = .3*PI/g_degree; // less than half distance between roots
    if (g_fractal_type == fractal_type::MPNEWTON || g_fractal_type == fractal_type::MPNEWTBASIN)
    {
        s_newton_mp_r_over_d = *d2MP(s_newton_r_over_d);
        s_mp_degree_minus_1_over_degree = *d2MP(s_degree_minus_1_over_degree);
        s_mp_threshold = *d2MP(g_threshold);
        g_mp_one = *d2MP(1.0);
    }

    g_basin = 0;
    s_roots.resize(16);
    if (g_fractal_type == fractal_type::NEWTBASIN)
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
    else if (g_fractal_type == fractal_type::MPNEWTBASIN)
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
            s_mpc_roots[i].x = *d2MP(std::cos(i*PI*2.0/(double)g_degree));
            s_mpc_roots[i].y = *d2MP(std::sin(i*PI*2.0/(double)g_degree));
        }
    }

    g_params[0] = (double)g_degree;
    if (g_degree%4 == 0)
    {
        g_symmetry = symmetry_type::XY_AXIS;
    }
    else
    {
        g_symmetry = symmetry_type::X_AXIS;
    }

    g_calc_type = standard_fractal;
    return true;
}

int MPCNewtonFractal()
{
    g_mp_overflow = false;
    MPC mpctmp = MPCpow(s_mpc_old, g_degree - 1);

    MPC mpcnew;
    mpcnew.x = *MPsub(*MPmul(mpctmp.x, s_mpc_old.x), *MPmul(mpctmp.y, s_mpc_old.y));
    mpcnew.y = *MPadd(*MPmul(mpctmp.x, s_mpc_old.y), *MPmul(mpctmp.y, s_mpc_old.x));
    s_mpc_temp1.x = *MPsub(mpcnew.x, g_mpc_one.x);
    s_mpc_temp1.y = *MPsub(mpcnew.y, g_mpc_one.y);
    if (MPcmp(MPCmod(s_mpc_temp1), s_mp_threshold) < 0)
    {
        if (g_fractal_type == fractal_type::MPNEWTBASIN)
        {
            long tmpcolor;
            tmpcolor = -1;
            for (int i = 0; i < g_degree; i++)
                if (MPcmp(MPdistance(s_mpc_roots[i], s_mpc_old), s_mp_threshold) < 0)
                {
                    if (g_basin == 2)
                    {
                        tmpcolor = 1+(i&7) + ((g_color_iter&1) << 3);
                    }
                    else
                    {
                        tmpcolor = 1+i;
                    }
                    break;
                }
            if (tmpcolor == -1)
            {
                g_color_iter = g_max_color;
            }
            else
            {
                g_color_iter = tmpcolor;
            }
        }
        return 1;
    }

    mpcnew.x = *MPadd(*MPmul(s_mp_degree_minus_1_over_degree, mpcnew.x), s_newton_mp_r_over_d);
    mpcnew.y = *MPmul(mpcnew.y, s_mp_degree_minus_1_over_degree);
    MP temp2 = MPCmod(mpctmp);
    temp2 = *MPdiv(g_mp_one, temp2);
    s_mpc_old.x = *MPmul(temp2, (*MPadd(*MPmul(mpcnew.x, mpctmp.x), *MPmul(mpcnew.y, mpctmp.y))));
    s_mpc_old.y = *MPmul(temp2, (*MPsub(*MPmul(mpcnew.y, mpctmp.x), *MPmul(mpcnew.x, mpctmp.y))));
    g_new_z.x = *MP2d(s_mpc_old.x);
    g_new_z.y = *MP2d(s_mpc_old.y);
    return g_mp_overflow ? 1 : 0;
}

int MPCjulia_per_pixel()
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
    s_mpc_old.x = *d2MP(g_old_z.x);
    s_mpc_old.y = *d2MP(g_old_z.y);
    return 0;
}
