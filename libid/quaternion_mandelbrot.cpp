#include "quaternion_mandelbrot.h"

#include "port.h"
#include "prototyp.h"

#include "calcfrac.h"
#include "fractals.h"
#include "id_data.h"
#include "pixel_grid.h"

#define Q0 0
#define Q1 0

int quaternionjulfp_per_pixel()
{
    g_old_z.x = g_dx_pixel();
    g_old_z.y = g_dy_pixel();
    g_float_param->x = g_params[4];
    g_float_param->y = g_params[5];
    g_quaternion_c  = g_params[0];
    g_quaternion_ci = g_params[1];
    g_quaternion_cj = g_params[2];
    g_quaternino_ck = g_params[3];
    return 0;
}

int quaternionfp_per_pixel()
{
    g_old_z.x = 0;
    g_old_z.y = 0;
    g_float_param->x = 0;
    g_float_param->y = 0;
    g_quaternion_c  = g_dx_pixel();
    g_quaternion_ci = g_dy_pixel();
    g_quaternion_cj = g_params[2];
    g_quaternino_ck = g_params[3];
    return 0;
}

int QuaternionFPFractal()
{
    double a0, a1, a2, a3, n0, n1, n2, n3;
    a0 = g_old_z.x;
    a1 = g_old_z.y;
    a2 = g_float_param->x;
    a3 = g_float_param->y;

    n0 = a0*a0-a1*a1-a2*a2-a3*a3 + g_quaternion_c;
    n1 = 2*a0*a1 + g_quaternion_ci;
    n2 = 2*a0*a2 + g_quaternion_cj;
    n3 = 2*a0*a3 + g_quaternino_ck;
    // Check bailout
    g_magnitude = a0*a0+a1*a1+a2*a2+a3*a3;
    if (g_magnitude > g_magnitude_limit)
    {
        return 1;
    }
    g_new_z.x = n0;
    g_old_z.x = g_new_z.x;
    g_new_z.y = n1;
    g_old_z.y = g_new_z.y;
    g_float_param->x = n2;
    g_float_param->y = n3;
    return 0;
}
