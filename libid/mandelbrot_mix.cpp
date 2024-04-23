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

static MandelbrotMix s_mix{};

#define A s_mix.a
#define B s_mix.b
#define C s_mix.c
#define D s_mix.d
#define F s_mix.f
#define G s_mix.g
#define H s_mix.h
#define J s_mix.j
#define K s_mix.k
#define L s_mix.l
#define Z s_mix.z

bool MandelbrotMix4Setup()
{
    int sign_array = 0;
    A.x = g_params[0];
    A.y = 0.0;    // a=real(p1),
    B.x = g_params[1];
    B.y = 0.0;    // b=imag(p1),
    D.x = g_params[2];
    D.y = 0.0;    // d=real(p2),
    F.x = g_params[3];
    F.y = 0.0;    // f=imag(p2),
    K.x = g_params[4]+1.0;
    K.y = 0.0;    // k=real(p3)+1,
    L.x = g_params[5]+100.0;
    L.y = 0.0;    // l=imag(p3)+100,
    CMPLXrecip(F, G);                // g=1/f,
    CMPLXrecip(D, H);                // h=1/d,
    g_tmp_z = F - B;              // tmp = f-b
    CMPLXrecip(g_tmp_z, J);              // j = 1/(f-b)
    g_tmp_z = -A;
    CMPLXmult(g_tmp_z, B, g_tmp_z);           // z=(-a*b*g*h)^j,
    CMPLXmult(g_tmp_z, G, g_tmp_z);
    CMPLXmult(g_tmp_z, H, g_tmp_z);

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
    if (A.x < 0.0)
    {
        sign_array += 8;
    }
    if (B.x < 0.0)
    {
        sign_array += 4;
    }
    if (G.x < 0.0)
    {
        sign_array += 2;
    }
    if (H.x < 0.0)
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
        case 15: // 1111
        case 10: // 1010
        case  6: // 0110
        case  5: // 0101
        case  3: // 0011
        case  0: // 0000
            g_tmp_z.y = -g_tmp_z.y; // swap sign bit
        default: // do nothing - remaining cases already OK
            ;
        }
        // in case our kludge failed, let the user fix it
        if (g_debug_flag == debug_flags::mandelbrot_mix4_flip_sign)
        {
            g_tmp_z.y = -g_tmp_z.y;
        }
    }

    CMPLXpwr(g_tmp_z, J, g_tmp_z);   // note: z is old
    // in case our kludge failed, let the user fix it
    if (g_params[6] < 0.0)
    {
        g_tmp_z.y = -g_tmp_z.y;
    }

    if (g_bail_out == 0)
    {
        g_magnitude_limit = L.x;
        g_magnitude_limit2 = g_magnitude_limit*g_magnitude_limit;
    }
    return true;
}

int MandelbrotMix4fp_per_pixel()
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
    CMPLXtrig0(g_init, C);        // c=fn1(pixel):
    return 0; // 1st iteration has been NOT been done
}

int MandelbrotMix4fpFractal() // from formula by Jim Muth
{
    // z=k*((a*(z^b))+(d*(z^f)))+c,
    DComplex z_b;
    DComplex z_f;
    CMPLXpwr(g_old_z, B, z_b);     // (z^b)
    CMPLXpwr(g_old_z, F, z_f);     // (z^f)
    g_new_z.x = K.x*A.x*z_b.x + K.x*D.x*z_f.x + C.x;
    g_new_z.y = K.x*A.x*z_b.y + K.x*D.x*z_f.y + C.y;
    return g_bailout_float();
}
#undef A
#undef B
#undef C
#undef D
#undef F
#undef G
#undef H
#undef J
#undef K
#undef L
