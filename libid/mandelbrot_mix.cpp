// SPDX-License-Identifier: GPL-3.0-only
//
#include "mandelbrot_mix.h"

#include "port.h"
#include "prototyp.h"

#include "bailout_formula.h"
#include "cmdfiles.h"
#include "debug_flags.h"
#include "id_data.h"
#include "mpmath.h"
#include "newton.h"
#include "pixel_grid.h"

/* re-use static roots variable
   memory for mandelmix4 */

struct MandelbrotMix
{
    bool setup();
    int per_pixel();
    int iterate();

    DComplex a;
    DComplex b;
    DComplex c;
    DComplex d;
    DComplex f;
    DComplex g;
    DComplex h;
    DComplex j;
    DComplex k;
    DComplex l;
    DComplex z;
};

bool MandelbrotMix::setup()
{
    int sign_array = 0;
    a.x = g_params[0];
    a.y = 0.0; // a=real(p1),
    b.x = g_params[1];
    b.y = 0.0; // b=imag(p1),
    d.x = g_params[2];
    d.y = 0.0; // d=real(p2),
    f.x = g_params[3];
    f.y = 0.0; // f=imag(p2),
    k.x = g_params[4] + 1.0;
    k.y = 0.0; // k=real(p3)+1,
    l.x = g_params[5] + 100.0;
    l.y = 0.0;              // l=imag(p3)+100,
    CMPLXrecip(f, g);       // g=1/f,
    CMPLXrecip(d, h);       // h=1/d,
    g_tmp_z = f - b;        // tmp = f-b
    CMPLXrecip(g_tmp_z, j); // j = 1/(f-b)
    g_tmp_z = -a;
    CMPLXmult(g_tmp_z, b, g_tmp_z); // z=(-a*b*g*h)^j,
    CMPLXmult(g_tmp_z, g, g_tmp_z);
    CMPLXmult(g_tmp_z, h, g_tmp_z);

    /*
       This code kludge attempts to duplicate the behavior
       of the parser in determining the sign of zero of the
       imaginary part of the argument of the power function. The
       reason this is important is that the complex arctangent
       returns PI in one case and -PI in the other, depending
       on the sign bit of zero, and we wish the results to be
       compatible with Jim Muth's mix4 formula using the parser.

       First create a number encoding the signs of a, b, g , h. Our
       kludge assumes that those signs determine the behavior.
     */
    if (a.x < 0.0)
    {
        sign_array += 8;
    }
    if (b.x < 0.0)
    {
        sign_array += 4;
    }
    if (g.x < 0.0)
    {
        sign_array += 2;
    }
    if (h.x < 0.0)
    {
        sign_array += 1;
    }
    if (g_tmp_z.y == 0.0) // we know tmp.y IS zero but ...
    {
        switch (sign_array)
        {
        /*
           Add to this list the magic numbers of any cases
           in which the fractal does not match the formula version
         */
        case 15:                    // 1111
        case 10:                    // 1010
        case 6:                     // 0110
        case 5:                     // 0101
        case 3:                     // 0011
        case 0:                     // 0000
            g_tmp_z.y = -g_tmp_z.y; // swap sign bit
        default:                    // do nothing - remaining cases already OK
            ;
        }
        // in case our kludge failed, let the user fix it
        if (g_debug_flag == debug_flags::mandelbrot_mix4_flip_sign)
        {
            g_tmp_z.y = -g_tmp_z.y;
        }
    }

    CMPLXpwr(g_tmp_z, j, g_tmp_z); // note: z is old
    // in case our kludge failed, let the user fix it
    if (g_params[6] < 0.0)
    {
        g_tmp_z.y = -g_tmp_z.y;
    }

    if (g_bail_out == 0)
    {
        g_magnitude_limit = l.x;
        g_magnitude_limit2 = g_magnitude_limit * g_magnitude_limit;
    }
    return true;
}

int MandelbrotMix::per_pixel()
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
    g_old_z = g_tmp_z;
    CMPLXtrig0(g_init, c); // c=fn1(pixel):
    return 0;              // 1st iteration has been NOT been done
}

int MandelbrotMix::iterate()
{
    // z=k*((a*(z^b))+(d*(z^f)))+c,
    DComplex z_b;
    DComplex z_f;
    CMPLXpwr(g_old_z, b, z_b); // (z^b)
    CMPLXpwr(g_old_z, f, z_f); // (z^f)
    g_new_z.x = k.x * a.x * z_b.x + k.x * d.x * z_f.x + c.x;
    g_new_z.y = k.x * a.x * z_b.y + k.x * d.x * z_f.y + c.y;
    return g_bailout_float();
}

static MandelbrotMix s_mix{};

bool mandelbrot_mix4_setup()
{
    return s_mix.setup();
}

int mandelbrot_mix4_fp_per_pixel()
{
    return s_mix.per_pixel();
}

int mandelbrot_mix4_fp_fractal() // from formula by Jim Muth
{
    return s_mix.iterate();
}
