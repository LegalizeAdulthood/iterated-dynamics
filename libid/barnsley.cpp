#include "barnsley.h"

#include "bailout_formula.h"
#include "calcfrac.h"
#include "fixed_pt.h"
#include "fractals.h"

int Barnsley1Fractal()
{
    // Barnsley's Mandelbrot type M1 from "Fractals Everywhere" by Michael Barnsley, p. 322

    // calculate intermediate products
    const long oldxinitx   = multiply(g_l_old_z.x, g_long_param->x, g_bit_shift);
    const long oldyinity   = multiply(g_l_old_z.y, g_long_param->y, g_bit_shift);
    const long oldxinity   = multiply(g_l_old_z.x, g_long_param->y, g_bit_shift);
    const long oldyinitx   = multiply(g_l_old_z.y, g_long_param->x, g_bit_shift);
    // orbit calculation
    if (g_l_old_z.x >= 0)
    {
        g_l_new_z.x = (oldxinitx - g_long_param->x - oldyinity);
        g_l_new_z.y = (oldyinitx - g_long_param->y + oldxinity);
    }
    else
    {
        g_l_new_z.x = (oldxinitx + g_long_param->x - oldyinity);
        g_l_new_z.y = (oldyinitx + g_long_param->y + oldxinity);
    }
    return g_bailout_long();
}

int Barnsley1FPFractal()
{
    // Barnsley's Mandelbrot type M1 from "Fractals Everywhere" by Michael Barnsley, p. 322

    // calculate intermediate products
    const double foldxinitx = g_old_z.x * g_float_param->x;
    const double foldyinity = g_old_z.y * g_float_param->y;
    const double foldxinity = g_old_z.x * g_float_param->y;
    const double foldyinitx = g_old_z.y * g_float_param->x;
    // orbit calculation
    if (g_old_z.x >= 0)
    {
        g_new_z.x = (foldxinitx - g_float_param->x - foldyinity);
        g_new_z.y = (foldyinitx - g_float_param->y + foldxinity);
    }
    else
    {
        g_new_z.x = (foldxinitx + g_float_param->x - foldyinity);
        g_new_z.y = (foldyinitx + g_float_param->y + foldxinity);
    }
    return g_bailout_float();
}

int Barnsley2Fractal()
{
    // An unnamed Mandelbrot/Julia function from "Fractals Everywhere" by Michael Barnsley, p. 331,
    // example 4.2

    // calculate intermediate products
    const long oldxinitx   = multiply(g_l_old_z.x, g_long_param->x, g_bit_shift);
    const long oldyinity   = multiply(g_l_old_z.y, g_long_param->y, g_bit_shift);
    const long oldxinity   = multiply(g_l_old_z.x, g_long_param->y, g_bit_shift);
    const long oldyinitx   = multiply(g_l_old_z.y, g_long_param->x, g_bit_shift);

    // orbit calculation
    if (oldxinity + oldyinitx >= 0)
    {
        g_l_new_z.x = oldxinitx - g_long_param->x - oldyinity;
        g_l_new_z.y = oldyinitx - g_long_param->y + oldxinity;
    }
    else
    {
        g_l_new_z.x = oldxinitx + g_long_param->x - oldyinity;
        g_l_new_z.y = oldyinitx + g_long_param->y + oldxinity;
    }
    return g_bailout_long();
}

int Barnsley2FPFractal()
{
    // An unnamed Mandelbrot/Julia function from "Fractals Everywhere" by Michael Barnsley, p. 331,
    // example 4.2

    // calculate intermediate products
    const double foldxinitx = g_old_z.x * g_float_param->x;
    const double foldyinity = g_old_z.y * g_float_param->y;
    const double foldxinity = g_old_z.x * g_float_param->y;
    const double foldyinitx = g_old_z.y * g_float_param->x;

    // orbit calculation
    if (foldxinity + foldyinitx >= 0)
    {
        g_new_z.x = foldxinitx - g_float_param->x - foldyinity;
        g_new_z.y = foldyinitx - g_float_param->y + foldxinity;
    }
    else
    {
        g_new_z.x = foldxinitx + g_float_param->x - foldyinity;
        g_new_z.y = foldyinitx + g_float_param->y + foldxinity;
    }
    return g_bailout_float();
}

int Barnsley3Fractal()
{
    // An unnamed Mandelbrot/Julia function from "Fractals Everywhere" by Michael Barnsley, p. 292,
    // example 4.1

    // calculate intermediate products
    const long oldxinitx   = multiply(g_l_old_z.x, g_l_old_z.x, g_bit_shift);
    const long oldyinity   = multiply(g_l_old_z.y, g_l_old_z.y, g_bit_shift);
    const long oldxinity   = multiply(g_l_old_z.x, g_l_old_z.y, g_bit_shift);

    // orbit calculation
    if (g_l_old_z.x > 0)
    {
        g_l_new_z.x = oldxinitx   - oldyinity - g_fudge_factor;
        g_l_new_z.y = oldxinity << 1;
    }
    else
    {
        g_l_new_z.x = oldxinitx - oldyinity - g_fudge_factor
                 + multiply(g_long_param->x, g_l_old_z.x, g_bit_shift);
        g_l_new_z.y = oldxinity <<1;

        /* This term added by Tim Wegner to make dependent on the
           imaginary part of the parameter. (Otherwise Mandelbrot
           is uninteresting. */
        g_l_new_z.y += multiply(g_long_param->y, g_l_old_z.x, g_bit_shift);
    }
    return g_bailout_long();
}

int Barnsley3FPFractal()
{
    // An unnamed Mandelbrot/Julia function from "Fractals Everywhere" by Michael Barnsley, p. 292,
    // example 4.1

    // calculate intermediate products
    const double foldxinitx  = g_old_z.x * g_old_z.x;
    const double foldyinity  = g_old_z.y * g_old_z.y;
    const double foldxinity  = g_old_z.x * g_old_z.y;

    // orbit calculation
    if (g_old_z.x > 0)
    {
        g_new_z.x = foldxinitx - foldyinity - 1.0;
        g_new_z.y = foldxinity * 2;
    }
    else
    {
        g_new_z.x = foldxinitx - foldyinity -1.0 + g_float_param->x * g_old_z.x;
        g_new_z.y = foldxinity * 2;

        /* This term added by Tim Wegner to make dependent on the
           imaginary part of the parameter. (Otherwise Mandelbrot
           is uninteresting. */
        g_new_z.y += g_float_param->y * g_old_z.x;
    }
    return g_bailout_float();
}
