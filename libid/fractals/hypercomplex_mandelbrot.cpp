// SPDX-License-Identifier: GPL-3.0-only
//
#include "fractals/hypercomplex_mandelbrot.h"

#include "engine/calcfrac.h"
#include "engine/fractals.h"
#include "math/cmplx.h"
#include "math/hcmplx.h"

using namespace id::math;

namespace id::fractals
{

int hyper_complex_orbit()
{
    DHyperComplex h_old;
    DHyperComplex h_new;
    h_old.x = g_old_z.x;
    h_old.y = g_old_z.y;
    h_old.z = g_float_param->x;
    h_old.t = g_float_param->y;

    hcmplx_trig0(&h_old, &h_new);

    h_new.x += g_quaternion_c;
    h_new.y += g_quaternion_ci;
    h_new.z += g_quaternion_cj;
    h_new.t += g_quaternion_ck;

    g_new_z.x = h_new.x;
    g_old_z.x = g_new_z.x;
    g_new_z.y = h_new.y;
    g_old_z.y = g_new_z.y;
    g_float_param->x = h_new.z;
    g_float_param->y = h_new.t;

    // Check bailout
    g_magnitude = sqr(g_old_z.x)+sqr(g_old_z.y)+sqr(g_float_param->x)+sqr(g_float_param->y);
    if (g_magnitude > g_magnitude_limit)
    {
        return 1;
    }
    return 0;
}

} // namespace id::fractals
