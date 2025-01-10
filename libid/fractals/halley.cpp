// SPDX-License-Identifier: GPL-3.0-only
//
#include "fractals/halley.h"

#include "cmdfiles.h"
#include "fractals/fractalp.h"
#include "fractals/newton.h"
#include "fractype.h"
#include "engine/id_data.h"
#include "math/mpmath.h"
#include "math/mpmath_c.h"
#include "engine/pixel_grid.h"

#include <algorithm>
#include <cmath>

static MPC s_mpc_old{};
static MPC s_mpc_new{};
static int s_halley_a_plus_one{};
static int s_halley_a_plus_one_times_degree{};
static MP s_halley_mp_a_plus_one{};
static MP s_halley_mp_a_plus_one_times_degree{};
static MPC s_mpc_temp_param{};
static MP s_mp_temp_param2_x{};

bool halley_setup()
{
    // Halley
    g_periodicity_check = 0;

    if (g_user_float_flag)
    {
        g_fractal_type = FractalType::HALLEY; // float on
    }
    else
    {
        g_fractal_type = FractalType::HALLEY_MP;
    }

    g_cur_fractal_specific = &g_fractal_specific[+g_fractal_type];

    g_degree = (int)g_param_z1.x;
    g_degree = std::max(g_degree, 2);
    g_params[0] = (double)g_degree;

    //  precalculated values
    s_halley_a_plus_one = g_degree + 1; // a+1
    s_halley_a_plus_one_times_degree = s_halley_a_plus_one * g_degree;

    if (g_fractal_type == FractalType::HALLEY_MP)
    {
        s_halley_mp_a_plus_one = *d_to_mp((double)s_halley_a_plus_one);
        s_halley_mp_a_plus_one_times_degree = *d_to_mp((double)s_halley_a_plus_one_times_degree);
        s_mpc_temp_param.x = *d_to_mp(g_param_z1.y);
        s_mpc_temp_param.y = *d_to_mp(g_param_z2.y);
        s_mp_temp_param2_x = *d_to_mp(g_param_z2.x);
        g_mp_one        = *d_to_mp(1.0);
    }

    if (g_degree % 2)
    {
        g_symmetry = SymmetryType::X_AXIS;   // odd
    }
    else
    {
        g_symmetry = SymmetryType::XY_AXIS; // even
    }
    return true;
}

static int halley_bailout()
{
    if (std::fabs(modulus(g_new_z)-modulus(g_old_z)) < g_param_z2.x)
    {
        return 1;
    }
    g_old_z = g_new_z;
    return 0;
}

static bool mpc_halley_bailout()
{
    static MP mp_tmp_bail_out;
    mp_tmp_bail_out = *mp_abs(*mp_sub(mpc_mod(s_mpc_new), mpc_mod(s_mpc_old)));
    if (mp_cmp(mp_tmp_bail_out, s_mp_temp_param2_x) < 0)
    {
        return true;
    }
    s_mpc_old = s_mpc_new;
    return false;
}

int mpc_halley_fractal()
{
    //  X(X^a - 1) = 0, Halley Map
    //  a = parm.x,  relaxation coeff. = parm.y,  epsilon = parm2.x

    MPC mpc_x_to_a_less_one, mpc_x_to_a;
    MPC mpc_x_to_a_plus_one; // a-1, a, a+1
    MPC mpc_fx, mpc_f1_prime, mpc_f2_prime, mpc_numer1;
    MPC mpc_numer2, mpc_denom, mpc_tmp2;

    g_mp_overflow = false;
    mpc_x_to_a_less_one.x = s_mpc_old.x;
    mpc_x_to_a_less_one.y = s_mpc_old.y;
    for (int deg = 2; deg < g_degree; deg++)
    {
        mpc_tmp2.x = *mp_sub(*mp_mul(mpc_x_to_a_less_one.x, s_mpc_old.x), *mp_mul(mpc_x_to_a_less_one.y, s_mpc_old.y));
        mpc_tmp2.y = *mp_add(*mp_mul(mpc_x_to_a_less_one.x, s_mpc_old.y), *mp_mul(mpc_x_to_a_less_one.y, s_mpc_old.x));
        mpc_x_to_a_less_one.x = mpc_tmp2.x;
        mpc_x_to_a_less_one.y = mpc_tmp2.y;
    }
    mpc_x_to_a.x = *mp_sub(*mp_mul(mpc_x_to_a_less_one.x, s_mpc_old.x), *mp_mul(mpc_x_to_a_less_one.y, s_mpc_old.y));
    mpc_x_to_a.y = *mp_add(*mp_mul(mpc_x_to_a_less_one.x, s_mpc_old.y), *mp_mul(mpc_x_to_a_less_one.y, s_mpc_old.x));
    mpc_x_to_a_plus_one.x = *mp_sub(*mp_mul(mpc_x_to_a.x, s_mpc_old.x), *mp_mul(mpc_x_to_a.y, s_mpc_old.y));
    mpc_x_to_a_plus_one.y = *mp_add(*mp_mul(mpc_x_to_a.x, s_mpc_old.y), *mp_mul(mpc_x_to_a.y, s_mpc_old.x));

    mpc_fx.x = *mp_sub(mpc_x_to_a_plus_one.x, s_mpc_old.x);
    mpc_fx.y = *mp_sub(mpc_x_to_a_plus_one.y, s_mpc_old.y); // FX = X^(a+1) - X  = F

    mpc_f2_prime.x = *mp_mul(s_halley_mp_a_plus_one_times_degree, mpc_x_to_a_less_one.x); // mpAp1deg in setup
    mpc_f2_prime.y = *mp_mul(s_halley_mp_a_plus_one_times_degree, mpc_x_to_a_less_one.y);        // F"

    mpc_f1_prime.x = *mp_sub(*mp_mul(s_halley_mp_a_plus_one, mpc_x_to_a.x), g_mp_one);
    mpc_f1_prime.y = *mp_mul(s_halley_mp_a_plus_one, mpc_x_to_a.y);                   //  F'

    mpc_tmp2.x = *mp_sub(*mp_mul(mpc_f2_prime.x, mpc_fx.x), *mp_mul(mpc_f2_prime.y, mpc_fx.y));
    mpc_tmp2.y = *mp_add(*mp_mul(mpc_f2_prime.x, mpc_fx.y), *mp_mul(mpc_f2_prime.y, mpc_fx.x));
    //  F * F"

    mpc_denom.x = *mp_add(mpc_f1_prime.x, mpc_f1_prime.x);
    mpc_denom.y = *mp_add(mpc_f1_prime.y, mpc_f1_prime.y);      //  2 * F'

    mpc_numer1 = mpc_div(mpc_tmp2, mpc_denom);        //  F"F/2F'
    mpc_tmp2.x = *mp_sub(mpc_f1_prime.x, mpc_numer1.x);
    mpc_tmp2.y = *mp_sub(mpc_f1_prime.y, mpc_numer1.y); //  F' - F"F/2F'
    mpc_numer2 = mpc_div(mpc_fx, mpc_tmp2);

    mpc_tmp2   =  mpc_mul(s_mpc_temp_param, mpc_numer2);  // mpctmpparm is
    // relaxation coef.
    s_mpc_new = mpc_sub(s_mpc_old, mpc_tmp2);
    g_new_z    = mpc_to_cmplx(s_mpc_new);
    return mpc_halley_bailout() || g_mp_overflow ? 1 : 0;
}

int halley_fractal()
{
    //  X(X^a - 1) = 0, Halley Map
    //  a = parm.x = degree, relaxation coeff. = parm.y, epsilon = parm2.x
    DComplex x_to_a_less_one, x_to_a, x_to_a_plus_one; // a-1, a, a+1
    DComplex fx, f1_prime, f2_prime, numer1, numer2, denom;
    DComplex relax;

    x_to_a_less_one = g_old_z;
    for (int deg = 2; deg < g_degree; deg++)
    {
        fpu_cmplx_mul(&g_old_z, &x_to_a_less_one, &x_to_a_less_one);
    }
    fpu_cmplx_mul(&g_old_z, &x_to_a_less_one, &x_to_a);
    fpu_cmplx_mul(&g_old_z, &x_to_a, &x_to_a_plus_one);

    fx = x_to_a_plus_one - g_old_z;        // FX = X^(a+1) - X  = F
    f2_prime.x = s_halley_a_plus_one_times_degree * x_to_a_less_one.x; // g_halley_a_plus_one_times_degree in setup
    f2_prime.y = s_halley_a_plus_one_times_degree * x_to_a_less_one.y;        // F"

    f1_prime.x = s_halley_a_plus_one * x_to_a.x - 1.0;
    f1_prime.y = s_halley_a_plus_one * x_to_a.y;                             //  F'

    fpu_cmplx_mul(&f2_prime, &fx, &numer1);                  //  F * F"
    denom.x = f1_prime.x + f1_prime.x;
    denom.y = f1_prime.y + f1_prime.y;                     //  2 * F'

    fpu_cmplx_div(&numer1, &denom, &numer1);         //  F"F/2F'
    numer2 = f1_prime - numer1;          //  F' - F"F/2F'
    fpu_cmplx_div(&fx, &numer2, &numer2);
    // parm.y is relaxation coef.
    relax.x = g_param_z1.y;
    relax.y = g_params[3];
    fpu_cmplx_mul(&relax, &numer2, &numer2);
    g_new_z.x = g_old_z.x - numer2.x;
    g_new_z.y = g_old_z.y - numer2.y;
    return halley_bailout();
}

int mpc_halley_per_pixel()
{
    // MPC halley
    if (g_invert != 0)
    {
        invertz2(&g_init);
    }
    else
    {
        g_init.x = g_dx_pixel();
        g_init.y = g_dy_pixel();
    }

    s_mpc_old.x = *d_to_mp(g_init.x);
    s_mpc_old.y = *d_to_mp(g_init.y);

    return 0;
}

int halley_per_pixel()
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

    g_old_z = g_init;

    return 0; // 1st iteration is not done
}
