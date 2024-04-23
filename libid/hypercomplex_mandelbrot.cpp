#include "hypercomplex_mandelbrot.h"

#include "port.h"
#include "prototyp.h"

#include "calcfrac.h"
#include "cmplx.h"
#include "fractals.h"
#include "hcmplx.h"
#include "sqr.h"

int HyperComplexFPFractal()
{
    DHyperComplex hold;
    DHyperComplex hnew;
    hold.x = g_old_z.x;
    hold.y = g_old_z.y;
    hold.z = g_float_param->x;
    hold.t = g_float_param->y;

    HComplexTrig0(&hold, &hnew);

    hnew.x += g_quaternion_c;
    hnew.y += g_quaternion_ci;
    hnew.z += g_quaternion_cj;
    hnew.t += g_quaternino_ck;

    g_new_z.x = hnew.x;
    g_old_z.x = g_new_z.x;
    g_new_z.y = hnew.y;
    g_old_z.y = g_new_z.y;
    g_float_param->x = hnew.z;
    g_float_param->y = hnew.t;

    // Check bailout
    g_magnitude = sqr(g_old_z.x)+sqr(g_old_z.y)+sqr(g_float_param->x)+sqr(g_float_param->y);
    if (g_magnitude > g_magnitude_limit)
    {
        return 1;
    }
    return 0;
}
