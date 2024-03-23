#include "quartic_mandelbrot.h"

#include "port.h"
#include "prototyp.h"

#include "bailout_formula.h"
#include "calcfrac.h"
#include "fractals.h"

int Mandel4Fractal()
{
    /*
       this routine calculates the Mandelbrot/Julia set based on the
       polynomial z**4 + lambda
     */

    // first, compute (x + iy)**2
    g_l_new_z.x  = g_l_temp_sqr_x - g_l_temp_sqr_y;
    g_l_new_z.y = multiply(g_l_old_z.x, g_l_old_z.y, g_bit_shift_less_1);
    if (g_bailout_long())
    {
        return 1;
    }

    // then, compute ((x + iy)**2)**2 + lambda
    g_l_new_z.x  = g_l_temp_sqr_x - g_l_temp_sqr_y + g_long_param->x;
    g_l_new_z.y = multiply(g_l_old_z.x, g_l_old_z.y, g_bit_shift_less_1) + g_long_param->y;
    return g_bailout_long();
}

int Mandel4fpFractal()
{
    // first, compute (x + iy)**2
    g_new_z.x  = g_temp_sqr_x - g_temp_sqr_y;
    g_new_z.y = g_old_z.x*g_old_z.y*2;
    if (g_bailout_float())
    {
        return 1;
    }

    // then, compute ((x + iy)**2)**2 + lambda
    g_new_z.x  = g_temp_sqr_x - g_temp_sqr_y + g_float_param->x;
    g_new_z.y =  g_old_z.x*g_old_z.y*2 + g_float_param->y;
    return g_bailout_float();
}
