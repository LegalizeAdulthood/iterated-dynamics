#include "phoenix.h"

#include "port.h"
#include "prototyp.h"

#include "bailout_formula.h"
#include "calcfrac.h"
#include "cmdfiles.h"
#include "cmplx.h"
#include "fractalp.h"
#include "fractals.h"
#include "id_data.h"
#include "mpmath.h"
#include "newton.h"
#include "pixel_grid.h"

static LComplex s_ltmp2{};
static DComplex s_tmp2{};

int LongPhoenixFractal()
{
    // z(n+1) = z(n)^2 + p + qy(n),  y(n+1) = z(n)
    g_l_temp.x = multiply(g_l_old_z.x, g_l_old_z.y, g_bit_shift);
    g_l_new_z.x = g_l_temp_sqr_x-g_l_temp_sqr_y+g_long_param->x+multiply(g_long_param->y, s_ltmp2.x, g_bit_shift);
    g_l_new_z.y = (g_l_temp.x + g_l_temp.x) + multiply(g_long_param->y, s_ltmp2.y, g_bit_shift);
    s_ltmp2 = g_l_old_z; // set ltmp2 to Y value
    return g_bailout_long();
}

int PhoenixFractal()
{
    // z(n+1) = z(n)^2 + p + qy(n),  y(n+1) = z(n)
    g_tmp_z.x = g_old_z.x * g_old_z.y;
    g_new_z.x = g_temp_sqr_x - g_temp_sqr_y + g_float_param->x + (g_float_param->y * s_tmp2.x);
    g_new_z.y = (g_tmp_z.x + g_tmp_z.x) + (g_float_param->y * s_tmp2.y);
    s_tmp2 = g_old_z; // set tmp2 to Y value
    return g_bailout_float();
}

int LongPhoenixFractalcplx()
{
    // z(n+1) = z(n)^2 + p + qy(n),  y(n+1) = z(n)
    g_l_temp.x = multiply(g_l_old_z.x, g_l_old_z.y, g_bit_shift);
    g_l_new_z.x = g_l_temp_sqr_x-g_l_temp_sqr_y+g_long_param->x+multiply(g_l_param2.x, s_ltmp2.x, g_bit_shift)-multiply(g_l_param2.y, s_ltmp2.y, g_bit_shift);
    g_l_new_z.y = (g_l_temp.x + g_l_temp.x)+g_long_param->y+multiply(g_l_param2.x, s_ltmp2.y, g_bit_shift)+multiply(g_l_param2.y, s_ltmp2.x, g_bit_shift);
    s_ltmp2 = g_l_old_z; // set ltmp2 to Y value
    return g_bailout_long();
}

int PhoenixFractalcplx()
{
    // z(n+1) = z(n)^2 + p1 + p2*y(n),  y(n+1) = z(n)
    g_tmp_z.x = g_old_z.x * g_old_z.y;
    g_new_z.x = g_temp_sqr_x - g_temp_sqr_y + g_float_param->x + (g_param_z2.x * s_tmp2.x) - (g_param_z2.y * s_tmp2.y);
    g_new_z.y = (g_tmp_z.x + g_tmp_z.x) + g_float_param->y + (g_param_z2.x * s_tmp2.y) + (g_param_z2.y * s_tmp2.x);
    s_tmp2 = g_old_z; // set tmp2 to Y value
    return g_bailout_float();
}

int LongPhoenixPlusFractal()
{
    // z(n+1) = z(n)^(degree-1) * (z(n) + p) + qy(n),  y(n+1) = z(n)
    LComplex loldplus;
    LComplex lnewminus;
    loldplus = g_l_old_z;
    g_l_temp = g_l_old_z;
    for (int i = 1; i < g_degree; i++)
    {
        // degree >= 2, degree=degree-1 in setup
        LCMPLXmult(g_l_old_z, g_l_temp, g_l_temp); // = old^(degree-1)
    }
    loldplus.x += g_long_param->x;
    LCMPLXmult(g_l_temp, loldplus, lnewminus);
    g_l_new_z.x = lnewminus.x + multiply(g_long_param->y, s_ltmp2.x, g_bit_shift);
    g_l_new_z.y = lnewminus.y + multiply(g_long_param->y, s_ltmp2.y, g_bit_shift);
    s_ltmp2 = g_l_old_z; // set ltmp2 to Y value
    return g_bailout_long();
}

int PhoenixPlusFractal()
{
    // z(n+1) = z(n)^(degree-1) * (z(n) + p) + qy(n),  y(n+1) = z(n)
    DComplex oldplus;
    DComplex newminus;
    oldplus = g_old_z;
    g_tmp_z = g_old_z;
    for (int i = 1; i < g_degree; i++)
    {
        // degree >= 2, degree=degree-1 in setup
        FPUcplxmul(&g_old_z, &g_tmp_z, &g_tmp_z); // = old^(degree-1)
    }
    oldplus.x += g_float_param->x;
    FPUcplxmul(&g_tmp_z, &oldplus, &newminus);
    g_new_z.x = newminus.x + (g_float_param->y * s_tmp2.x);
    g_new_z.y = newminus.y + (g_float_param->y * s_tmp2.y);
    s_tmp2 = g_old_z; // set tmp2 to Y value
    return g_bailout_float();
}

int LongPhoenixMinusFractal()
{
    // z(n+1) = z(n)^(degree-2) * (z(n)^2 + p) + qy(n),  y(n+1) = z(n)
    LComplex loldsqr;
    LComplex lnewminus;
    LCMPLXmult(g_l_old_z, g_l_old_z, loldsqr);
    g_l_temp = g_l_old_z;
    for (int i = 1; i < g_degree; i++)
    {
        // degree >= 3, degree=degree-2 in setup
        LCMPLXmult(g_l_old_z, g_l_temp, g_l_temp); // = old^(degree-2)
    }
    loldsqr.x += g_long_param->x;
    LCMPLXmult(g_l_temp, loldsqr, lnewminus);
    g_l_new_z.x = lnewminus.x + multiply(g_long_param->y, s_ltmp2.x, g_bit_shift);
    g_l_new_z.y = lnewminus.y + multiply(g_long_param->y, s_ltmp2.y, g_bit_shift);
    s_ltmp2 = g_l_old_z; // set ltmp2 to Y value
    return g_bailout_long();
}

int PhoenixMinusFractal()
{
    // z(n+1) = z(n)^(degree-2) * (z(n)^2 + p) + qy(n),  y(n+1) = z(n)
    DComplex oldsqr;
    DComplex newminus;
    FPUcplxmul(&g_old_z, &g_old_z, &oldsqr);
    g_tmp_z = g_old_z;
    for (int i = 1; i < g_degree; i++)
    {
        // degree >= 3, degree=degree-2 in setup
        FPUcplxmul(&g_old_z, &g_tmp_z, &g_tmp_z); // = old^(degree-2)
    }
    oldsqr.x += g_float_param->x;
    FPUcplxmul(&g_tmp_z, &oldsqr, &newminus);
    g_new_z.x = newminus.x + (g_float_param->y * s_tmp2.x);
    g_new_z.y = newminus.y + (g_float_param->y * s_tmp2.y);
    s_tmp2 = g_old_z; // set tmp2 to Y value
    return g_bailout_float();
}

int LongPhoenixCplxPlusFractal()
{
    // z(n+1) = z(n)^(degree-1) * (z(n) + p) + qy(n),  y(n+1) = z(n)
    LComplex loldplus;
    LComplex lnewminus;
    loldplus = g_l_old_z;
    g_l_temp = g_l_old_z;
    for (int i = 1; i < g_degree; i++)
    {
        // degree >= 2, degree=degree-1 in setup
        LCMPLXmult(g_l_old_z, g_l_temp, g_l_temp); // = old^(degree-1)
    }
    loldplus.x += g_long_param->x;
    loldplus.y += g_long_param->y;
    LCMPLXmult(g_l_temp, loldplus, lnewminus);
    LCMPLXmult(g_l_param2, s_ltmp2, g_l_temp);
    g_l_new_z.x = lnewminus.x + g_l_temp.x;
    g_l_new_z.y = lnewminus.y + g_l_temp.y;
    s_ltmp2 = g_l_old_z; // set ltmp2 to Y value
    return g_bailout_long();
}

int PhoenixCplxPlusFractal()
{
    // z(n+1) = z(n)^(degree-1) * (z(n) + p) + qy(n),  y(n+1) = z(n)
    DComplex oldplus;
    DComplex newminus;
    oldplus = g_old_z;
    g_tmp_z = g_old_z;
    for (int i = 1; i < g_degree; i++)
    {
        // degree >= 2, degree=degree-1 in setup
        FPUcplxmul(&g_old_z, &g_tmp_z, &g_tmp_z); // = old^(degree-1)
    }
    oldplus.x += g_float_param->x;
    oldplus.y += g_float_param->y;
    FPUcplxmul(&g_tmp_z, &oldplus, &newminus);
    FPUcplxmul(&g_param_z2, &s_tmp2, &g_tmp_z);
    g_new_z.x = newminus.x + g_tmp_z.x;
    g_new_z.y = newminus.y + g_tmp_z.y;
    s_tmp2 = g_old_z; // set tmp2 to Y value
    return g_bailout_float();
}

int LongPhoenixCplxMinusFractal()
{
    // z(n+1) = z(n)^(degree-2) * (z(n)^2 + p) + qy(n),  y(n+1) = z(n)
    LComplex loldsqr;
    LComplex lnewminus;
    LCMPLXmult(g_l_old_z, g_l_old_z, loldsqr);
    g_l_temp = g_l_old_z;
    for (int i = 1; i < g_degree; i++)
    {
        // degree >= 3, degree=degree-2 in setup
        LCMPLXmult(g_l_old_z, g_l_temp, g_l_temp); // = old^(degree-2)
    }
    loldsqr.x += g_long_param->x;
    loldsqr.y += g_long_param->y;
    LCMPLXmult(g_l_temp, loldsqr, lnewminus);
    LCMPLXmult(g_l_param2, s_ltmp2, g_l_temp);
    g_l_new_z.x = lnewminus.x + g_l_temp.x;
    g_l_new_z.y = lnewminus.y + g_l_temp.y;
    s_ltmp2 = g_l_old_z; // set ltmp2 to Y value
    return g_bailout_long();
}

int PhoenixCplxMinusFractal()
{
    // z(n+1) = z(n)^(degree-2) * (z(n)^2 + p) + qy(n),  y(n+1) = z(n)
    DComplex oldsqr;
    DComplex newminus;
    FPUcplxmul(&g_old_z, &g_old_z, &oldsqr);
    g_tmp_z = g_old_z;
    for (int i = 1; i < g_degree; i++)
    {
        // degree >= 3, degree=degree-2 in setup
        FPUcplxmul(&g_old_z, &g_tmp_z, &g_tmp_z); // = old^(degree-2)
    }
    oldsqr.x += g_float_param->x;
    oldsqr.y += g_float_param->y;
    FPUcplxmul(&g_tmp_z, &oldsqr, &newminus);
    FPUcplxmul(&g_param_z2, &s_tmp2, &g_tmp_z);
    g_new_z.x = newminus.x + g_tmp_z.x;
    g_new_z.y = newminus.y + g_tmp_z.y;
    s_tmp2 = g_old_z; // set tmp2 to Y value
    return g_bailout_float();
}

int long_phoenix_per_pixel()
{
    if (g_invert != 0)
    {
        // invert
        invertz2(&g_old_z);

        // watch out for overflow
        if (sqr(g_old_z.x)+sqr(g_old_z.y) >= 127)
        {
            g_old_z.x = 8;  // value to bail out in one iteration
            g_old_z.y = 8;
        }

        // convert to fudged longs
        g_l_old_z.x = (long)(g_old_z.x*g_fudge_factor);
        g_l_old_z.y = (long)(g_old_z.y*g_fudge_factor);
    }
    else
    {
        g_l_old_z.x = g_l_x_pixel();
        g_l_old_z.y = g_l_y_pixel();
    }
    g_l_temp_sqr_x = multiply(g_l_old_z.x, g_l_old_z.x, g_bit_shift);
    g_l_temp_sqr_y = multiply(g_l_old_z.y, g_l_old_z.y, g_bit_shift);
    s_ltmp2.x = 0; // use ltmp2 as the complex Y value
    s_ltmp2.y = 0;
    return 0;
}

int phoenix_per_pixel()
{
    if (g_invert != 0)
    {
        invertz2(&g_old_z);
    }
    else
    {
        g_old_z.x = g_dx_pixel();
        g_old_z.y = g_dy_pixel();
    }
    g_temp_sqr_x = sqr(g_old_z.x);  // precalculated value
    g_temp_sqr_y = sqr(g_old_z.y);
    s_tmp2.x = 0; // use tmp2 as the complex Y value
    s_tmp2.y = 0;
    return 0;
}

int long_mandphoenix_per_pixel()
{
    g_l_init.x = g_l_x_pixel();
    g_l_init.y = g_l_y_pixel();

    if (g_invert != 0)
    {
        // invert
        invertz2(&g_init);

        // watch out for overflow
        if (sqr(g_init.x)+sqr(g_init.y) >= 127)
        {
            g_init.x = 8;  // value to bail out in one iteration
            g_init.y = 8;
        }

        // convert to fudged longs
        g_l_init.x = (long)(g_init.x*g_fudge_factor);
        g_l_init.y = (long)(g_init.y*g_fudge_factor);
    }

    if (g_use_init_orbit == init_orbit_mode::value)
    {
        g_l_old_z = g_l_init_orbit;
    }
    else
    {
        g_l_old_z = g_l_init;
    }

    g_l_old_z.x += g_l_param.x;    // initial pertubation of parameters set
    g_l_old_z.y += g_l_param.y;
    g_l_temp_sqr_x = multiply(g_l_old_z.x, g_l_old_z.x, g_bit_shift);
    g_l_temp_sqr_y = multiply(g_l_old_z.y, g_l_old_z.y, g_bit_shift);
    s_ltmp2.x = 0;
    s_ltmp2.y = 0;
    return 1; // 1st iteration has been done
}

int mandphoenix_per_pixel()
{
    if (g_invert != 0)
    {
        invertz2(&g_init);
    }
    else
    {
        g_init.x = g_dx_pixel();
        g_init.y = g_dy_pixel();
    }

    if (g_use_init_orbit == init_orbit_mode::value)
    {
        g_old_z = g_init_orbit;
    }
    else
    {
        g_old_z = g_init;
    }

    g_old_z.x += g_param_z1.x;      // initial pertubation of parameters set
    g_old_z.y += g_param_z1.y;
    g_temp_sqr_x = sqr(g_old_z.x);  // precalculated value
    g_temp_sqr_y = sqr(g_old_z.y);
    s_tmp2.x = 0;
    s_tmp2.y = 0;
    return 1; // 1st iteration has been done
}

bool PhoenixSetup()
{
    g_long_param = &g_l_param;
    g_float_param = &g_param_z1;
    g_degree = (int)g_param_z2.x;
    if (g_degree < 2 && g_degree > -3)
    {
        g_degree = 0;
    }
    g_params[2] = (double)g_degree;
    if (g_degree == 0)
    {
        if (g_user_float_flag)
        {
            g_cur_fractal_specific->orbitcalc =  PhoenixFractal;
        }
        else
        {
            g_cur_fractal_specific->orbitcalc =  LongPhoenixFractal;
        }
    }
    if (g_degree >= 2)
    {
        g_degree = g_degree - 1;
        if (g_user_float_flag)
        {
            g_cur_fractal_specific->orbitcalc =  PhoenixPlusFractal;
        }
        else
        {
            g_cur_fractal_specific->orbitcalc =  LongPhoenixPlusFractal;
        }
    }
    if (g_degree <= -3)
    {
        g_degree = std::abs(g_degree) - 2;
        if (g_user_float_flag)
        {
            g_cur_fractal_specific->orbitcalc =  PhoenixMinusFractal;
        }
        else
        {
            g_cur_fractal_specific->orbitcalc =  LongPhoenixMinusFractal;
        }
    }

    return true;
}

bool PhoenixCplxSetup()
{
    g_long_param = &g_l_param;
    g_float_param = &g_param_z1;
    g_degree = (int)g_params[4];
    if (g_degree < 2 && g_degree > -3)
    {
        g_degree = 0;
    }
    g_params[4] = (double)g_degree;
    if (g_degree == 0)
    {
        if (g_param_z2.x != 0 || g_param_z2.y != 0)
        {
            g_symmetry = symmetry_type::NONE;
        }
        else
        {
            g_symmetry = symmetry_type::ORIGIN;
        }
        if (g_param_z1.y == 0 && g_param_z2.y == 0)
        {
            g_symmetry = symmetry_type::X_AXIS;
        }
        if (g_user_float_flag)
        {
            g_cur_fractal_specific->orbitcalc =  PhoenixFractalcplx;
        }
        else
        {
            g_cur_fractal_specific->orbitcalc =  LongPhoenixFractalcplx;
        }
    }
    if (g_degree >= 2)
    {
        g_degree = g_degree - 1;
        if (g_param_z1.y == 0 && g_param_z2.y == 0)
        {
            g_symmetry = symmetry_type::X_AXIS;
        }
        else
        {
            g_symmetry = symmetry_type::NONE;
        }
        if (g_user_float_flag)
        {
            g_cur_fractal_specific->orbitcalc =  PhoenixCplxPlusFractal;
        }
        else
        {
            g_cur_fractal_specific->orbitcalc =  LongPhoenixCplxPlusFractal;
        }
    }
    if (g_degree <= -3)
    {
        g_degree = std::abs(g_degree) - 2;
        if (g_param_z1.y == 0 && g_param_z2.y == 0)
        {
            g_symmetry = symmetry_type::X_AXIS;
        }
        else
        {
            g_symmetry = symmetry_type::NONE;
        }
        if (g_user_float_flag)
        {
            g_cur_fractal_specific->orbitcalc =  PhoenixCplxMinusFractal;
        }
        else
        {
            g_cur_fractal_specific->orbitcalc =  LongPhoenixCplxMinusFractal;
        }
    }

    return true;
}

bool MandPhoenixSetup()
{
    g_long_param = &g_l_init;
    g_float_param = &g_init;
    g_degree = (int)g_param_z2.x;
    if (g_degree < 2 && g_degree > -3)
    {
        g_degree = 0;
    }
    g_params[2] = (double)g_degree;
    if (g_degree == 0)
    {
        if (g_user_float_flag)
        {
            g_cur_fractal_specific->orbitcalc =  PhoenixFractal;
        }
        else
        {
            g_cur_fractal_specific->orbitcalc =  LongPhoenixFractal;
        }
    }
    if (g_degree >= 2)
    {
        g_degree = g_degree - 1;
        if (g_user_float_flag)
        {
            g_cur_fractal_specific->orbitcalc =  PhoenixPlusFractal;
        }
        else
        {
            g_cur_fractal_specific->orbitcalc =  LongPhoenixPlusFractal;
        }
    }
    if (g_degree <= -3)
    {
        g_degree = std::abs(g_degree) - 2;
        if (g_user_float_flag)
        {
            g_cur_fractal_specific->orbitcalc =  PhoenixMinusFractal;
        }
        else
        {
            g_cur_fractal_specific->orbitcalc =  LongPhoenixMinusFractal;
        }
    }

    return true;
}

bool MandPhoenixCplxSetup()
{
    g_long_param = &g_l_init;
    g_float_param = &g_init;
    g_degree = (int)g_params[4];
    if (g_degree < 2 && g_degree > -3)
    {
        g_degree = 0;
    }
    g_params[4] = (double)g_degree;
    if (g_param_z1.y != 0 || g_param_z2.y != 0)
    {
        g_symmetry = symmetry_type::NONE;
    }
    if (g_degree == 0)
    {
        if (g_user_float_flag)
        {
            g_cur_fractal_specific->orbitcalc =  PhoenixFractalcplx;
        }
        else
        {
            g_cur_fractal_specific->orbitcalc =  LongPhoenixFractalcplx;
        }
    }
    if (g_degree >= 2)
    {
        g_degree = g_degree - 1;
        if (g_user_float_flag)
        {
            g_cur_fractal_specific->orbitcalc =  PhoenixCplxPlusFractal;
        }
        else
        {
            g_cur_fractal_specific->orbitcalc =  LongPhoenixCplxPlusFractal;
        }
    }
    if (g_degree <= -3)
    {
        g_degree = std::abs(g_degree) - 2;
        if (g_user_float_flag)
        {
            g_cur_fractal_specific->orbitcalc =  PhoenixCplxMinusFractal;
        }
        else
        {
            g_cur_fractal_specific->orbitcalc =  LongPhoenixCplxMinusFractal;
        }
    }

    return true;
}
