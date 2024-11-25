// SPDX-License-Identifier: GPL-3.0-only
//
#include "halley.h"

#include "port.h"
#include "prototyp.h"

#include "calcfrac.h"
#include "cmdfiles.h"
#include "fractalp.h"
#include "fractals.h"
#include "fractype.h"
#include "mpmath.h"
#include "mpmath_c.h"
#include "newton.h"
#include "pixel_grid.h"

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
        g_fractal_type = fractal_type::HALLEY; // float on
    }
    else
    {
        g_fractal_type = fractal_type::MPHALLEY;
    }

    g_cur_fractal_specific = &g_fractal_specific[+g_fractal_type];

    g_degree = (int)g_param_z1.x;
    if (g_degree < 2)
    {
        g_degree = 2;
    }
    g_params[0] = (double)g_degree;

    //  precalculated values
    s_halley_a_plus_one = g_degree + 1; // a+1
    s_halley_a_plus_one_times_degree = s_halley_a_plus_one * g_degree;

    if (g_fractal_type == fractal_type::MPHALLEY)
    {
        s_halley_mp_a_plus_one = *d2MP((double)s_halley_a_plus_one);
        s_halley_mp_a_plus_one_times_degree = *d2MP((double)s_halley_a_plus_one_times_degree);
        s_mpc_temp_param.x = *d2MP(g_param_z1.y);
        s_mpc_temp_param.y = *d2MP(g_param_z2.y);
        s_mp_temp_param2_x = *d2MP(g_param_z2.x);
        g_mp_one        = *d2MP(1.0);
    }

    if (g_degree % 2)
    {
        g_symmetry = symmetry_type::X_AXIS;   // odd
    }
    else
    {
        g_symmetry = symmetry_type::XY_AXIS; // even
    }
    return true;
}

static int  Halleybailout()
{
    if (std::fabs(modulus(g_new_z)-modulus(g_old_z)) < g_param_z2.x)
    {
        return 1;
    }
    g_old_z = g_new_z;
    return 0;
}

static bool MPCHalleybailout()
{
    static MP mptmpbailout;
    mptmpbailout = *MPabs(*MPsub(MPCmod(s_mpc_new), MPCmod(s_mpc_old)));
    if (MPcmp(mptmpbailout, s_mp_temp_param2_x) < 0)
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

    MPC mpcXtoAlessOne, mpcXtoA;
    MPC mpcXtoAplusOne; // a-1, a, a+1
    MPC mpcFX, mpcF1prime, mpcF2prime, mpcHalnumer1;
    MPC mpcHalnumer2, mpcHaldenom, mpctmp2;

    g_mp_overflow = false;
    mpcXtoAlessOne.x = s_mpc_old.x;
    mpcXtoAlessOne.y = s_mpc_old.y;
    for (int ihal = 2; ihal < g_degree; ihal++)
    {
        mpctmp2.x = *MPsub(*MPmul(mpcXtoAlessOne.x, s_mpc_old.x), *MPmul(mpcXtoAlessOne.y, s_mpc_old.y));
        mpctmp2.y = *MPadd(*MPmul(mpcXtoAlessOne.x, s_mpc_old.y), *MPmul(mpcXtoAlessOne.y, s_mpc_old.x));
        mpcXtoAlessOne.x = mpctmp2.x;
        mpcXtoAlessOne.y = mpctmp2.y;
    }
    mpcXtoA.x = *MPsub(*MPmul(mpcXtoAlessOne.x, s_mpc_old.x), *MPmul(mpcXtoAlessOne.y, s_mpc_old.y));
    mpcXtoA.y = *MPadd(*MPmul(mpcXtoAlessOne.x, s_mpc_old.y), *MPmul(mpcXtoAlessOne.y, s_mpc_old.x));
    mpcXtoAplusOne.x = *MPsub(*MPmul(mpcXtoA.x, s_mpc_old.x), *MPmul(mpcXtoA.y, s_mpc_old.y));
    mpcXtoAplusOne.y = *MPadd(*MPmul(mpcXtoA.x, s_mpc_old.y), *MPmul(mpcXtoA.y, s_mpc_old.x));

    mpcFX.x = *MPsub(mpcXtoAplusOne.x, s_mpc_old.x);
    mpcFX.y = *MPsub(mpcXtoAplusOne.y, s_mpc_old.y); // FX = X^(a+1) - X  = F

    mpcF2prime.x = *MPmul(s_halley_mp_a_plus_one_times_degree, mpcXtoAlessOne.x); // mpAp1deg in setup
    mpcF2prime.y = *MPmul(s_halley_mp_a_plus_one_times_degree, mpcXtoAlessOne.y);        // F"

    mpcF1prime.x = *MPsub(*MPmul(s_halley_mp_a_plus_one, mpcXtoA.x), g_mp_one);
    mpcF1prime.y = *MPmul(s_halley_mp_a_plus_one, mpcXtoA.y);                   //  F'

    mpctmp2.x = *MPsub(*MPmul(mpcF2prime.x, mpcFX.x), *MPmul(mpcF2prime.y, mpcFX.y));
    mpctmp2.y = *MPadd(*MPmul(mpcF2prime.x, mpcFX.y), *MPmul(mpcF2prime.y, mpcFX.x));
    //  F * F"

    mpcHaldenom.x = *MPadd(mpcF1prime.x, mpcF1prime.x);
    mpcHaldenom.y = *MPadd(mpcF1prime.y, mpcF1prime.y);      //  2 * F'

    mpcHalnumer1 = MPCdiv(mpctmp2, mpcHaldenom);        //  F"F/2F'
    mpctmp2.x = *MPsub(mpcF1prime.x, mpcHalnumer1.x);
    mpctmp2.y = *MPsub(mpcF1prime.y, mpcHalnumer1.y); //  F' - F"F/2F'
    mpcHalnumer2 = MPCdiv(mpcFX, mpctmp2);

    mpctmp2   =  MPCmul(s_mpc_temp_param, mpcHalnumer2);  // mpctmpparm is
    // relaxation coef.
    s_mpc_new = MPCsub(s_mpc_old, mpctmp2);
    g_new_z    = MPC2cmplx(s_mpc_new);
    return MPCHalleybailout() || g_mp_overflow ? 1 : 0;
}

int halley_fractal()
{
    //  X(X^a - 1) = 0, Halley Map
    //  a = parm.x = degree, relaxation coeff. = parm.y, epsilon = parm2.x
    DComplex XtoAlessOne, XtoA, XtoAplusOne; // a-1, a, a+1
    DComplex FX, F1prime, F2prime, Halnumer1, Halnumer2, Haldenom;
    DComplex relax;

    XtoAlessOne = g_old_z;
    for (int ihal = 2; ihal < g_degree; ihal++)
    {
        FPUcplxmul(&g_old_z, &XtoAlessOne, &XtoAlessOne);
    }
    FPUcplxmul(&g_old_z, &XtoAlessOne, &XtoA);
    FPUcplxmul(&g_old_z, &XtoA, &XtoAplusOne);

    FX = XtoAplusOne - g_old_z;        // FX = X^(a+1) - X  = F
    F2prime.x = s_halley_a_plus_one_times_degree * XtoAlessOne.x; // g_halley_a_plus_one_times_degree in setup
    F2prime.y = s_halley_a_plus_one_times_degree * XtoAlessOne.y;        // F"

    F1prime.x = s_halley_a_plus_one * XtoA.x - 1.0;
    F1prime.y = s_halley_a_plus_one * XtoA.y;                             //  F'

    FPUcplxmul(&F2prime, &FX, &Halnumer1);                  //  F * F"
    Haldenom.x = F1prime.x + F1prime.x;
    Haldenom.y = F1prime.y + F1prime.y;                     //  2 * F'

    FPUcplxdiv(&Halnumer1, &Haldenom, &Halnumer1);         //  F"F/2F'
    Halnumer2 = F1prime - Halnumer1;          //  F' - F"F/2F'
    FPUcplxdiv(&FX, &Halnumer2, &Halnumer2);
    // parm.y is relaxation coef.
    relax.x = g_param_z1.y;
    relax.y = g_params[3];
    FPUcplxmul(&relax, &Halnumer2, &Halnumer2);
    g_new_z.x = g_old_z.x - Halnumer2.x;
    g_new_z.y = g_old_z.y - Halnumer2.y;
    return Halleybailout();
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

    s_mpc_old.x = *d2MP(g_init.x);
    s_mpc_old.y = *d2MP(g_init.y);

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
