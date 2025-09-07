// SPDX-License-Identifier: GPL-3.0-only
//
#include "fractals/halley.h"

#include "engine/cmdfiles.h"
#include "engine/fractals.h"
#include "engine/id_data.h"
#include "engine/pixel_grid.h"
#include "fractals/fractalp.h"
#include "fractals/fractype.h"
#include "fractals/newton.h"
#include "math/fpu087.h"

#include <algorithm>
#include <cmath>

using namespace id;

static int s_halley_a_plus_one{};
static int s_halley_a_plus_one_times_degree{};

bool halley_per_image()
{
    // Halley
    g_periodicity_check = 0;

    set_fractal_type(FractalType::HALLEY);

    g_degree = (int)g_param_z1.x;
    g_degree = std::max(g_degree, 2);
    g_params[0] = (double)g_degree;

    //  precalculated values
    s_halley_a_plus_one = g_degree + 1; // a+1
    s_halley_a_plus_one_times_degree = s_halley_a_plus_one * g_degree;

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
    if (std::abs(modulus(g_new_z)-modulus(g_old_z)) < g_param_z2.x)
    {
        return 1;
    }
    g_old_z = g_new_z;
    return 0;
}

int halley_orbit()
{
    //  X(X^a - 1) = 0, Halley Map
    //  a = parm.x = degree, relaxation coeff. = parm.y, epsilon = parm2.x
    DComplex x_to_a_less_one;
    DComplex x_to_a;
    DComplex x_to_a_plus_one; // a-1, a, a+1
    DComplex fx;
    DComplex f1_prime;
    DComplex f2_prime;
    DComplex numer1;
    DComplex numer2;
    DComplex denom;
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
