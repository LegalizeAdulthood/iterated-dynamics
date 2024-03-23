#include "newton.h"

#include "port.h"
#include "prototyp.h"

#include "calcfrac.h"
#include "cmdfiles.h"
#include "cmplx.h"
#include "fpu087.h"
#include "fractalp.h"
#include "fractals.h"
#include "fractype.h"
#include "id_data.h"
#include "mpmath.h"
#include "mpmath_c.h"
#include "pixel_grid.h"

#include <cmath>

#define distance(z1, z2)  (sqr((z1).x-(z2).x)+sqr((z1).y-(z2).y))

static double t2{};
static double TwoPi;
static DComplex s_temp;
static DComplex BaseLog;
static DComplex cdegree = { 3.0, 0.0 };
static DComplex croot   = { 1.0, 0.0 };

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
#define DIST1(z) (((z).x-1.0)*((z).x-1.0)+((z).y)*((z).y))
#define LDIST1(z) (lsqr((((z).x)-g_fudge_factor)) + lsqr(((z).y)))

int NewtonFractal2()
{
    static char start = 1;
    if (start)
    {
        start = 0;
    }
    cpower(&g_old_z, g_degree-1, &g_tmp_z);
    complex_mult(g_tmp_z, g_old_z, &g_new_z);

    if (DIST1(g_new_z) < g_threshold)
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
                if (distance(g_roots[i], g_old_z) < g_threshold)
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
    g_new_z.x = g_degree_minus_1_over_degree * g_new_z.x + g_newton_r_over_d;
    g_new_z.y *= g_degree_minus_1_over_degree;

    // Watch for divide underflow
    t2 = g_tmp_z.x*g_tmp_z.x + g_tmp_z.y*g_tmp_z.y;
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
        croot.x = g_params[2];
        croot.y = g_params[3];
        cdegree.x = g_params[0];
        cdegree.y = g_params[1];
        FPUcplxlog(&croot, &BaseLog);
        TwoPi = std::asin(1.0) * 4;
    }
    return true;
}

int ComplexNewton()
{
    DComplex cd1;

    /* new = ((cdegree-1) * old**cdegree) + croot
             ----------------------------------
                  cdegree * old**(cdegree-1)         */

    cd1.x = cdegree.x - 1.0;
    cd1.y = cdegree.y;

    s_temp = ComplexPower(g_old_z, cd1);
    FPUcplxmul(&s_temp, &g_old_z, &g_new_z);

    g_tmp_z.x = g_new_z.x - croot.x;
    g_tmp_z.y = g_new_z.y - croot.y;
    if ((sqr(g_tmp_z.x) + sqr(g_tmp_z.y)) < g_threshold)
    {
        return 1;
    }

    FPUcplxmul(&g_new_z, &cd1, &g_tmp_z);
    g_tmp_z.x += croot.x;
    g_tmp_z.y += croot.y;

    FPUcplxmul(&s_temp, &cdegree, &cd1);
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

    cd1.x = cdegree.x - 1.0;
    cd1.y = cdegree.y;

    s_temp = ComplexPower(g_old_z, cd1);
    FPUcplxmul(&s_temp, &g_old_z, &g_new_z);

    g_tmp_z.x = g_new_z.x - croot.x;
    g_tmp_z.y = g_new_z.y - croot.y;
    if ((sqr(g_tmp_z.x) + sqr(g_tmp_z.y)) < g_threshold)
    {
        if (std::fabs(g_old_z.y) < .01)
        {
            g_old_z.y = 0.0;
        }
        FPUcplxlog(&g_old_z, &s_temp);
        FPUcplxmul(&s_temp, &cdegree, &g_tmp_z);
        double mod = g_tmp_z.y/TwoPi;
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
    g_tmp_z.x += croot.x;
    g_tmp_z.y += croot.y;

    FPUcplxmul(&s_temp, &cdegree, &cd1);
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
        g_cur_fractal_specific = &g_fractal_specific[static_cast<int>(g_fractal_type)];
    }
    // set up table of roots of 1 along unit circle
    g_degree = (int)g_param_z1.x;
    if (g_degree < 2)
    {
        g_degree = 3;   // defaults to 3, but 2 is possible
    }

    // precalculated values
    g_newton_r_over_d       = 1.0 / (double)g_degree;
    g_degree_minus_1_over_degree      = (double)(g_degree - 1) / (double)g_degree;
    g_max_color     = 0;
    g_threshold    = .3*PI/g_degree; // less than half distance between roots
    if (g_fractal_type == fractal_type::MPNEWTON || g_fractal_type == fractal_type::MPNEWTBASIN)
    {
        g_newton_mp_r_over_d     = *pd2MP(g_newton_r_over_d);
        g_mp_degree_minus_1_over_degree    = *pd2MP(g_degree_minus_1_over_degree);
        g_mp_threshold  = *pd2MP(g_threshold);
        g_mp_one        = *pd2MP(1.0);
    }

    g_basin = 0;
    g_roots.resize(16);
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
        g_roots.resize(g_degree);

        // list of roots to discover where we converged for newtbasin
        for (int i = 0; i < g_degree; i++)
        {
            g_roots[i].x = std::cos(i*PI*2.0/(double)g_degree);
            g_roots[i].y = std::sin(i*PI*2.0/(double)g_degree);
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

        g_mpc_roots.resize(g_degree);

        // list of roots to discover where we converged for newtbasin
        for (int i = 0; i < g_degree; i++)
        {
            g_mpc_roots[i].x = *pd2MP(std::cos(i*PI*2.0/(double)g_degree));
            g_mpc_roots[i].y = *pd2MP(std::sin(i*PI*2.0/(double)g_degree));
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
    if (g_fractal_type == fractal_type::MPNEWTON || g_fractal_type == fractal_type::MPNEWTBASIN)
    {
        setMPfunctions();
    }
    return true;
}
