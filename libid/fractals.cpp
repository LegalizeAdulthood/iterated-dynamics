// SPDX-License-Identifier: GPL-3.0-only
//
/*
Actually calculate the fractal images (well, SOMEBODY had to do it!).

The modules are set up so that all logic that is independent of any
fractal-specific code is in calcfrac, the code that IS fractal-specific
is in fractals, and the structure that ties (we hope!) everything together
is in fractalp.

Original author is Tim Wegner, but just about ALL the authors have
contributed SOME code to this routine at one time or another, or
contributed to one of the many massive restructurings.

The Fractal-specific routines are divided into three categories:

1. Routines that are called once-per-orbit to calculate the orbit
   value. These have names like "XxxxFractal", and their function
   pointers are stored in fractalspecific[fractype].orbitcalc. EVERY
   new fractal type needs one of these. Return 0 to continue iterations,
   1 if we're done. Results for integer fractals are left in 'lnew.x' and
   'lnew.y', for floating point fractals in 'new.x' and 'new.y'.

2. Routines that are called once per pixel to set various variables
   prior to the orbit calculation. These have names like xxx_per_pixel
   and are fairly generic - chances are one is right for your new type.
   They are stored in fractalspecific[fractype].per_pixel.

3. Routines that are called once per screen to set various variables.
   These have names like XxxxSetup, and are stored in
   fractalspecific[fractype].per_image.

4. The main fractal routine. Usually this will be standard_fractal(),
   but if you have written a stand-alone fractal routine independent
   of the standard_fractal mechanisms, your routine name goes here,
   stored in fractalspecific[fractype].calctype.per_image.

Adding a new fractal type should be simply a matter of adding an item
to the 'fractalspecific' structure, writing (or re-using one of the existing)
an appropriate setup, per_image, per_pixel, and orbit routines.

--------------------------------------------------------------------   */
#include "fractals.h"

#include "bailout_formula.h"
#include "biginit.h"
#include "calcfrac.h"
#include "cmdfiles.h"
#include "fractype.h"
#include "magnet.h"
#include "mpmath.h"
#include "mpmath_c.h"
#include "newton.h"
#include "pixel_grid.h"

#include <cfloat>
#include <cmath>

inline void conjugate(DComplex &pz)
{
    pz.y = - pz.y;
}

LComplex g_l_old_z{};
LComplex g_l_new_z{};
LComplex g_l_param{};
LComplex g_l_init{};
LComplex g_l_temp{};
LComplex g_l_param2{};
long g_l_temp_sqr_x{};
long g_l_temp_sqr_y{};
int g_max_color{};
int g_degree{};
int g_basin{};
long g_fudge_half{};
DComplex g_power_z{};
int g_bit_shift_less_1{}; // bit shift less 1
int g_c_exponent{};
DComplex g_param_z1{};
DComplex g_param_z2{};
DComplex *g_float_param{};
LComplex *g_long_param{};
double g_sin_x{};
double g_cos_x{};
double g_temp_sqr_x{};
double g_temp_sqr_y{};
double g_quaternion_c{};
double g_quaternion_ci{};
double g_quaternion_cj{};
double g_quaternion_ck{};

// --------------------------------------------------------------------
//              Bailout Routines
// --------------------------------------------------------------------

int  fp_mod_bailout()
{
    g_temp_sqr_x = sqr(g_new_z.x);
    g_temp_sqr_y = sqr(g_new_z.y);
    g_magnitude = g_temp_sqr_x + g_temp_sqr_y;
    if (g_magnitude >= g_magnitude_limit)
    {
        return 1;
    }
    g_old_z = g_new_z;
    return 0;
}

int  fp_real_bailout()
{
    g_temp_sqr_x = sqr(g_new_z.x);
    g_temp_sqr_y = sqr(g_new_z.y);
    g_magnitude = g_temp_sqr_x + g_temp_sqr_y;
    if (g_temp_sqr_x >= g_magnitude_limit)
    {
        return 1;
    }
    g_old_z = g_new_z;
    return 0;
}

int  fp_imag_bailout()
{
    g_temp_sqr_x = sqr(g_new_z.x);
    g_temp_sqr_y = sqr(g_new_z.y);
    g_magnitude = g_temp_sqr_x + g_temp_sqr_y;
    if (g_temp_sqr_y >= g_magnitude_limit)
    {
        return 1;
    }
    g_old_z = g_new_z;
    return 0;
}

int  fp_or_bailout()
{
    g_temp_sqr_x = sqr(g_new_z.x);
    g_temp_sqr_y = sqr(g_new_z.y);
    g_magnitude = g_temp_sqr_x + g_temp_sqr_y;
    if (g_temp_sqr_x >= g_magnitude_limit || g_temp_sqr_y >= g_magnitude_limit)
    {
        return 1;
    }
    g_old_z = g_new_z;
    return 0;
}

int  fp_and_bailout()
{
    g_temp_sqr_x = sqr(g_new_z.x);
    g_temp_sqr_y = sqr(g_new_z.y);
    g_magnitude = g_temp_sqr_x + g_temp_sqr_y;
    if (g_temp_sqr_x >= g_magnitude_limit && g_temp_sqr_y >= g_magnitude_limit)
    {
        return 1;
    }
    g_old_z = g_new_z;
    return 0;
}

int  fp_manh_bailout()
{
    double manhmag;
    g_temp_sqr_x = sqr(g_new_z.x);
    g_temp_sqr_y = sqr(g_new_z.y);
    g_magnitude = g_temp_sqr_x + g_temp_sqr_y;
    manhmag = std::fabs(g_new_z.x) + std::fabs(g_new_z.y);
    if ((manhmag * manhmag) >= g_magnitude_limit)
    {
        return 1;
    }
    g_old_z = g_new_z;
    return 0;
}

int  fp_manr_bailout()
{
    double manrmag;
    g_temp_sqr_x = sqr(g_new_z.x);
    g_temp_sqr_y = sqr(g_new_z.y);
    g_magnitude = g_temp_sqr_x + g_temp_sqr_y;
    manrmag = g_new_z.x + g_new_z.y; // don't need abs() since we square it next
    if ((manrmag * manrmag) >= g_magnitude_limit)
    {
        return 1;
    }
    g_old_z = g_new_z;
    return 0;
}

// --------------------------------------------------------------------
//              Fractal (once per iteration) routines
// --------------------------------------------------------------------
static double xt;
static double yt;
static double t2;

/* Raise complex number (base) to the (exp) power, storing the result
** in complex (result).
*/
void cpower(DComplex *base, int exp, DComplex *result)
{
    if (exp < 0)
    {
        cpower(base, -exp, result);
        cmplx_recip(*result, *result);
        return;
    }

    xt = base->x;
    yt = base->y;

    if (exp & 1)
    {
        result->x = xt;
        result->y = yt;
    }
    else
    {
        result->x = 1.0;
        result->y = 0.0;
    }

    exp >>= 1;
    while (exp)
    {
        t2 = xt * xt - yt * yt;
        yt = 2 * xt * yt;
        xt = t2;

        if (exp & 1)
        {
            t2 = xt * result->x - yt * result->y;
            result->y = result->y * xt + yt * result->x;
            result->x = t2;
        }
        exp >>= 1;
    }
}

// long version
static long lxt;
static long lyt;
static long lt2;

int lcpower(LComplex *base, int exp, LComplex *result, int bitshift)
{
    if (exp < 0)
    {
        g_overflow = lcpower(base, -exp, result, bitshift) != 0;
        lcmplx_recip(*result, *result);
        return g_overflow ? 1 : 0;
    }

    g_overflow = false;
    lxt = base->x;
    lyt = base->y;

    if (exp & 1)
    {
        result->x = lxt;
        result->y = lyt;
    }
    else
    {
        result->x = 1L << bitshift;
        result->y = 0L;
    }

    exp >>= 1;
    while (exp)
    {
        lt2 = multiply(lxt, lxt, bitshift) - multiply(lyt, lyt, bitshift);
        lyt = multiply(lxt, lyt, g_bit_shift_less_1);
        if (g_overflow)
        {
            return g_overflow;
        }
        lxt = lt2;

        if (exp & 1)
        {
            lt2 = multiply(lxt, result->x, bitshift) - multiply(lyt, result->y, bitshift);
            result->y = multiply(result->y, lxt, bitshift) + multiply(lyt, result->x, bitshift);
            result->x = lt2;
        }
        exp >>= 1;
    }
    if (result->x == 0 && result->y == 0)
    {
        g_overflow = true;
    }
    return g_overflow;
}

int complex_mult(DComplex arg1, DComplex arg2, DComplex *pz)
{
    pz->x = arg1.x * arg2.x - arg1.y * arg2.y;
    pz->y = arg1.x * arg2.y + arg1.y * arg2.x;
    return 0;
}

int complex_div(DComplex numerator, DComplex denominator, DComplex *pout)
{
    double mod = modulus(denominator);
    if (mod < FLT_MIN)
    {
        return 1;
    }
    conjugate(denominator);
    complex_mult(numerator, denominator, pout);
    pout->x = pout->x/mod;
    pout->y = pout->y/mod;
    return 0;
}

int julia_fractal()
{
    g_l_new_z.x  = g_l_temp_sqr_x - g_l_temp_sqr_y + g_long_param->x;
    g_l_new_z.y = multiply(g_l_old_z.x, g_l_old_z.y, g_bit_shift_less_1) + g_long_param->y;
    return g_bailout_long();
}

int julia_fp_fractal()
{
    g_new_z.x = g_temp_sqr_x - g_temp_sqr_y + g_float_param->x;
    g_new_z.y = 2.0 * g_old_z.x * g_old_z.y + g_float_param->y;
    return g_bailout_float();
}

long g_fudge_one;
long g_fudge_two;

int long_cmplx_z_power_fractal()
{
    DComplex x;
    DComplex y;

    x.x = (double)g_l_old_z.x / g_fudge_factor;
    x.y = (double)g_l_old_z.y / g_fudge_factor;
    y.x = (double)g_l_param2.x / g_fudge_factor;
    y.y = (double)g_l_param2.y / g_fudge_factor;
    x = complex_power(x, y);
    if (std::fabs(x.x) < g_fudge_limit && std::fabs(x.y) < g_fudge_limit)
    {
        g_l_new_z.x = (long)(x.x * g_fudge_factor);
        g_l_new_z.y = (long)(x.y * g_fudge_factor);
    }
    else
    {
        g_overflow = true;
    }
    g_l_new_z.x += g_long_param->x;
    g_l_new_z.y += g_long_param->y;
    return g_bailout_long();
}

int float_cmplx_z_power_fractal()
{
    g_new_z = complex_power(g_old_z, g_param_z2);
    g_new_z.x += g_float_param->x;
    g_new_z.y += g_float_param->y;
    return g_bailout_float();
}

// --------------------------------------------------------------------
//              Initialization (once per pixel) routines
// --------------------------------------------------------------------

int long_julia_per_pixel()
{
    // integer julia types
    // lambda
    // barnsleyj1
    // barnsleyj2
    // sierpinski
    if (g_invert != 0)
    {
        // invert
        invertz2(&g_old_z);

        // watch out for overflow
        if (sqr(g_old_z.x)+sqr(g_old_z.y) >= 127)
        {
            g_old_z.x = 8;  // value to bail out in one iteration
            g_old_z.y = 8;
        }

        // convert to fudged longs
        g_l_old_z.x = (long)(g_old_z.x*g_fudge_factor);
        g_l_old_z.y = (long)(g_old_z.y*g_fudge_factor);
    }
    else
    {
        g_l_old_z.x = g_l_x_pixel();
        g_l_old_z.y = g_l_y_pixel();
    }
    return 0;
}

int long_mandel_per_pixel()
{
    // integer mandel types
    // barnsleym1
    // barnsleym2
    g_l_init.x = g_l_x_pixel();
    g_l_init.y = g_l_y_pixel();

    if (g_invert != 0)
    {
        // invert
        invertz2(&g_init);

        // watch out for overflow
        if (sqr(g_init.x)+sqr(g_init.y) >= 127)
        {
            g_init.x = 8;  // value to bail out in one iteration
            g_init.y = 8;
        }

        // convert to fudged longs
        g_l_init.x = (long)(g_init.x*g_fudge_factor);
        g_l_init.y = (long)(g_init.y*g_fudge_factor);
    }

    if (g_use_init_orbit == InitOrbitMode::VALUE)
    {
        g_l_old_z = g_l_init_orbit;
    }
    else
    {
        g_l_old_z = g_l_init;
    }

    g_l_old_z.x += g_l_param.x;    // initial perturbation of parameters set
    g_l_old_z.y += g_l_param.y;
    return 1; // 1st iteration has been done
}

int julia_per_pixel()
{
    // julia

    if (g_invert != 0)
    {
        // invert
        invertz2(&g_old_z);

        // watch out for overflow
        if (g_bit_shift <= 24)
        {
            if (sqr(g_old_z.x)+sqr(g_old_z.y) >= 127)
            {
                g_old_z.x = 8;  // value to bail out in one iteration
                g_old_z.y = 8;
            }
        }
        if (g_bit_shift >  24)
        {
            if (sqr(g_old_z.x)+sqr(g_old_z.y) >= 4.0)
            {
                g_old_z.x = 2;  // value to bail out in one iteration
                g_old_z.y = 2;
            }
        }

        // convert to fudged longs
        g_l_old_z.x = (long)(g_old_z.x*g_fudge_factor);
        g_l_old_z.y = (long)(g_old_z.y*g_fudge_factor);
    }
    else
    {
        g_l_old_z.x = g_l_x_pixel();
        g_l_old_z.y = g_l_y_pixel();
    }

    g_l_temp_sqr_x = multiply(g_l_old_z.x, g_l_old_z.x, g_bit_shift);
    g_l_temp_sqr_y = multiply(g_l_old_z.y, g_l_old_z.y, g_bit_shift);
    g_l_temp = g_l_old_z;
    return 0;
}

int mandel_per_pixel()
{
    // mandel

    if (g_invert != 0)
    {
        invertz2(&g_init);

        // watch out for overflow
        if (g_bit_shift <= 24)
        {
            if (sqr(g_init.x)+sqr(g_init.y) >= 127)
            {
                g_init.x = 8;  // value to bail out in one iteration
                g_init.y = 8;
            }
        }
        if (g_bit_shift >  24)
        {
            if (sqr(g_init.x)+sqr(g_init.y) >= 4)
            {
                g_init.x = 2;  // value to bail out in one iteration
                g_init.y = 2;
            }
        }

        // convert to fudged longs
        g_l_init.x = (long)(g_init.x*g_fudge_factor);
        g_l_init.y = (long)(g_init.y*g_fudge_factor);
    }
    else
    {
        g_l_init.x = g_l_x_pixel();
        g_l_init.y = g_l_y_pixel();
    }
    switch (g_fractal_type)
    {
    case FractalType::MANDEL_LAMBDA:              // Critical Value 0.5 + 0.0i
        g_l_old_z.x = g_fudge_half;
        g_l_old_z.y = 0;
        break;
    default:
        g_l_old_z = g_l_init;
        break;
    }

    // alter init value
    if (g_use_init_orbit == InitOrbitMode::VALUE)
    {
        g_l_old_z = g_l_init_orbit;
    }
    else if (g_use_init_orbit == InitOrbitMode::PIXEL)
    {
        g_l_old_z = g_l_init;
    }

    if ((g_inside_color == BOF60 || g_inside_color == BOF61) && g_bof_match_book_images)
    {
        /* kludge to match "Beauty of Fractals" picture since we start
           Mandelbrot iteration with init rather than 0 */
        g_l_old_z.x = g_l_param.x; // initial pertubation of parameters set
        g_l_old_z.y = g_l_param.y;
        g_color_iter = -1;
    }
    else
    {
        g_l_old_z.x += g_l_param.x; // initial pertubation of parameters set
        g_l_old_z.y += g_l_param.y;
    }
    g_l_temp = g_l_init; // for spider
    g_l_temp_sqr_x = multiply(g_l_old_z.x, g_l_old_z.x, g_bit_shift);
    g_l_temp_sqr_y = multiply(g_l_old_z.y, g_l_old_z.y, g_bit_shift);
    return 1; // 1st iteration has been done
}

int mandel_fp_per_pixel()
{
    // floating point mandelbrot
    // mandelfp
    // burning ship

    if (g_invert != 0)
    {
        invertz2(&g_init);
    }
    else
    {
        g_init.x = g_dx_pixel();
        g_init.y = g_dy_pixel();
    }
    switch (g_fractal_type)
    {
    case FractalType::BURNING_SHIP:
        g_old_z.x = 0.0;
        g_old_z.y = 0.0;
        break;
    case FractalType::MAGNET_2M:
        float_pre_calc_magnet2();
    case FractalType::MAGNET_1M:  // NOLINT(clang-diagnostic-implicit-fallthrough)
        g_old_z.y = 0.0;       // Critical Val Zero both, but neither
        g_old_z.x = g_old_z.y; // is of the form f(Z,C) = Z*g(Z)+C
        break;
    case FractalType::MANDEL_LAMBDA_FP:            // Critical Value 0.5 + 0.0i
        g_old_z.x = 0.5;
        g_old_z.y = 0.0;
        break;
    default:
        g_old_z = g_init;
        break;
    }

    // alter init value
    if (g_use_init_orbit == InitOrbitMode::VALUE)
    {
        g_old_z = g_init_orbit;
    }
    else if (g_use_init_orbit == InitOrbitMode::PIXEL)
    {
        g_old_z = g_init;
    }

    if ((g_inside_color == BOF60 || g_inside_color == BOF61) && g_bof_match_book_images)
    {
        /* kludge to match "Beauty of Fractals" picture since we start
           Mandelbrot iteration with init rather than 0 */
        g_old_z.x = g_param_z1.x; // initial pertubation of parameters set
        g_old_z.y = g_param_z1.y;
        g_color_iter = -1;
    }
    else
    {
        g_old_z.x += g_param_z1.x;
        g_old_z.y += g_param_z1.y;
    }
    g_tmp_z = g_init; // for spider
    g_temp_sqr_x = sqr(g_old_z.x);  // precalculated value for regular Mandelbrot
    g_temp_sqr_y = sqr(g_old_z.y);
    return 1; // 1st iteration has been done
}

void mandel_ref_pt(const std::complex<double> &center, std::complex<double> &z)
{
    const double real_sqr = sqr(z.real());
    const double imag_sqr = sqr(z.imag());
    const double real = real_sqr - imag_sqr + center.real();
    const double imag = 2.0 * z.real() * z.imag() + center.imag();
    z.real(real);
    z.imag(imag);
}

void mandel_ref_pt_bf(const BFComplex &center, BFComplex &z)
{
    BigStackSaver saved;
    bf_t temp_real = alloc_stack(g_r_bf_length + 2);
    bf_t real_sqr = alloc_stack(g_r_bf_length + 2);
    bf_t imag_sqr = alloc_stack(g_r_bf_length + 2);
    bf_t real_imag = alloc_stack(g_r_bf_length + 2);
    BFComplex z2;
    z2.x = alloc_stack(g_r_bf_length + 2);
    z2.y = alloc_stack(g_r_bf_length + 2);
    double_bf(z2.x, z.x);
    double_bf(z2.y, z.y);

    square_bf(real_sqr, z.x);
    square_bf(imag_sqr, z.y);
    sub_bf(temp_real, real_sqr, imag_sqr);
    add_bf(z.x, temp_real, center.x);
    mult_bf(real_imag, z2.x, z.y);
    add_bf(z.y, real_imag, center.y);
}

void mandel_perturb(
    const std::complex<double> &ref, std::complex<double> &delta_n, const std::complex<double> &delta0)
{
    const double r{ref.real()};
    const double i{ref.imag()};
    const double a{delta_n.real()};
    const double b{delta_n.imag()};
    const double a0{delta0.real()};
    const double b0{delta0.imag()};

    const double dnr{(2 * r + a) * a - (2 * i + b) * b + a0};
    const double dni{2 * ((r + a) * b + i * a) + b0};
    delta_n.imag(dni);
    delta_n.real(dnr);
}

int julia_fp_per_pixel()
{
    // floating point julia
    // juliafp
    if (g_invert != 0)
    {
        invertz2(&g_old_z);
    }
    else
    {
        g_old_z.x = g_dx_pixel();
        g_old_z.y = g_dy_pixel();
    }
    g_temp_sqr_x = sqr(g_old_z.x);  // precalculated value for regular Julia
    g_temp_sqr_y = sqr(g_old_z.y);
    g_tmp_z = g_old_z;
    return 0;
}

int other_mandel_fp_per_pixel()
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

    if (g_use_init_orbit == InitOrbitMode::VALUE)
    {
        g_old_z = g_init_orbit;
    }
    else
    {
        g_old_z = g_init;
    }

    g_old_z.x += g_param_z1.x;      // initial pertubation of parameters set
    g_old_z.y += g_param_z1.y;

    return 1; // 1st iteration has been done
}

int other_julia_fp_per_pixel()
{
    if (g_invert != 0)
    {
        invertz2(&g_old_z);
    }
    else
    {
        g_old_z.x = g_dx_pixel();
        g_old_z.y = g_dy_pixel();
    }
    return 0;
}

