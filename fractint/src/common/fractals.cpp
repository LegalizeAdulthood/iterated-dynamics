//
//	FRACTALS.C, FRACTALP.C and CALCFRAC.C actually calculate the fractal
//	images (well, SOMEBODY had to do it!).  The modules are set up so that
//	all logic that is independent of any fractal-specific code is in
//	CALCFRAC.C, the code that IS fractal-specific is in FRACTALS.C, and the
//	structure that ties (we hope!) everything together is in FRACTALP.C.
//	Original author Tim Wegner, but just about ALL the authors have
//	contributed SOME code to this routine at one time or another, or
//	contributed to one of the many massive restructurings.
//
//	The Fractal-specific routines are divided into three categories:
//
//	1. Routines that are called once-per-orbit to calculate the orbit
//		value. These have names like "XxxxFractal", and their function
//		pointers are stored in g_fractal_specific[g_fractal_type].orbitcalc. EVERY
//		new fractal type needs one of these. Return 0 to continue iterations,
//		1 if we're done. Results for integer fractals are left in 'g_new_z_l.real()' and
//		'g_new_z_l.imag()', for floating point fractals in 'new.real()' and 'new.imag()'.
//
//	2. Routines that are called once per pixel to set various variables
//		prior to the orbit calculation. These have names like xxx_per_pixel
//		and are fairly generic - chances are one is right for your new type.
//		They are stored in g_fractal_specific[g_fractal_type].per_pixel.
//
//	3. Routines that are called once per screen to set various variables.
//		These have names like XxxxSetup, and are stored in
//		g_fractal_specific[g_fractal_type].per_image.
//
//	4. The main fractal routine. Usually this will be standard_fractal(),
//		but if you have written a stand-alone fractal routine independent
//		of the standard_fractal mechanisms, your routine name goes here,
//		stored in g_fractal_specific[g_fractal_type].calculate_type.per_image.
//
//	Adding a new fractal type should be simply a matter of adding an item
//	to the 'g_fractal_specific' structure, writing (or re-using one of the existing)
//	an appropriate setup, per_image, per_pixel, and orbit routines.
//
#include <climits>
#include <string>

#include "port.h"
#include "prototyp.h"
#include "helpdefs.h"
#include "fractype.h"
#include "externs.h"

#include "calcfrac.h"
#include "Externals.h"
#include "fpu.h"
#include "fracsubr.h"
#include "fractals.h"
#include "hcmplx.h"
#include "mpmath.h"
#include "EscapeTime.h"
#include "Formula.h"
#include "MathUtil.h"
#include "QuaternionEngine.h"

static double dx_pixel_calc();
static double dy_pixel_calc();
static long lx_pixel_calc();
static long ly_pixel_calc();

ComplexL g_coefficient_l = { 0, 0 };
ComplexL g_initial_z_l = { 0, 0 };
ComplexL g_old_z_l = { 0, 0 };
ComplexL g_new_z_l = { 0, 0 };
ComplexL g_temp_z_l = { 0, 0 };
ComplexL g_temp_sqr_l = { 0, 0 };
ComplexL g_parameter_l = { 0, 0 };
ComplexL g_parameter2_l = { 0, 0 };

StdComplexD g_coefficient(0.0, 0.0);
ComplexD g_temp_sqr = { 0.0, 0.0 };
ComplexD g_parameter = { 0.0, 0.0 };
ComplexD g_parameter2 = { 0, 0 };

int g_degree = 0;
double g_threshold = 0.0;
ComplexD g_power = { 0.0, 0.0};
int g_bit_shift_minus_1 = 0;
double g_two_pi = MathUtil::Pi*2.0;
int g_c_exp = 0;
ComplexD *g_float_parameter = 0;
ComplexL *g_long_parameter = 0; // used here and in jb.c
double g_cos_x = 0.0;
double g_sin_x = 0.0;
long g_one_fudge = 0;
long g_two_fudge = 0;

// pre-calculated values for fractal types Magnet2M & Magnet2J
static ComplexD s_3_c_minus_1 = { 0.0, 0.0 };		// 3*(g_float_parameter - 1)
static ComplexD s_3_c_minus_2 = { 0.0, 0.0 };        // 3*(g_float_parameter - 2)
static ComplexD s_c_minus_1_c_minus_2 = { 0.0, 0.0 }; // (g_float_parameter - 1)*(g_float_parameter - 2)
static ComplexD s_temp2 = { 0.0, 0.0 };
static double s_cos_y = 0.0;
static double s_sin_y = 0.0;
static double s_temp_exp = 0.0;
static double s_old_x_init_x_fp = 0.0;
static double s_old_y_init_y_fp = 0.0;
static double s_old_x_init_y_fp = 0.0;
static double s_old_y_init_x_fp = 0.0;
static long s_old_x_init_x = 0;
static long s_old_y_init_y = 0;
static long s_old_x_init_y = 0;
static long s_old_y_init_x = 0;
// temporary variables for trig use
static long s_cos_x_l = 0;
static long s_sin_x_l = 0;
static long s_cos_y_l = 0;
static long s_sin_y_l = 0;
static double s_xt;
static double s_yt;
static ComplexL s_temp_z2_l = { 0, 0 };

void magnet2_precalculate_fp() // precalculation for Magnet2 (M & J) for speed
{
	ComplexD const c_minus_1 = (*g_float_parameter - 1.0);
	ComplexD const c_minus_2 = (*g_float_parameter - 2.0);
	s_3_c_minus_1 = 3.0*c_minus_1;
	s_3_c_minus_2 = 3.0*c_minus_2;
	s_c_minus_1_c_minus_2 = c_minus_1*c_minus_2;
}

static void square_magnitude()
{
	g_temp_sqr.real(sqr(g_new_z.real()));
	g_temp_sqr.imag(sqr(g_new_z.imag()));
	g_magnitude = g_temp_sqr.real() + g_temp_sqr.imag();
}

// --------------------------------------------------------------------
// Bailout Routines Macros
// --------------------------------------------------------------------
int bail_out_mod_fp()
{
	square_magnitude();
	if (g_magnitude >= g_rq_limit)
	{
		return 1;
	}
	g_old_z = g_new_z;
	return 0;
}

int bail_out_real_fp()
{
	square_magnitude();
	if (g_temp_sqr.real() >= g_rq_limit)
	{
		return 1;
	}
	g_old_z = g_new_z;
	return 0;
}

int bail_out_imag_fp()
{
	square_magnitude();
	if (g_temp_sqr.imag() >= g_rq_limit)
	{
		return 1;
	}
	g_old_z = g_new_z;
	return 0;
}

int bail_out_or_fp()
{
	square_magnitude();
	if (g_temp_sqr.real() >= g_rq_limit || g_temp_sqr.imag() >= g_rq_limit)
	{
		return 1;
	}
	g_old_z = g_new_z;
	return 0;
}

int bail_out_and_fp()
{
	square_magnitude();
	if (g_temp_sqr.real() >= g_rq_limit && g_temp_sqr.imag() >= g_rq_limit)
	{
		return 1;
	}
	g_old_z = g_new_z;
	return 0;
}

int bail_out_manhattan_fp()
{
	square_magnitude();
	double manhmag = std::abs(g_new_z.real()) + std::abs(g_new_z.imag());
	if ((manhmag*manhmag) >= g_rq_limit)
	{
		return 1;
	}
	g_old_z = g_new_z;
	return 0;
}

int bail_out_manhattan_r_fp()
{
	square_magnitude();
	double manrmag = g_new_z.real() + g_new_z.imag(); // don't need std::abs() since we square it next
	if (manrmag*manrmag >= g_rq_limit)
	{
		return 1;
	}
	g_old_z = g_new_z;
	return 0;
}

static long const TRIG_LIMIT_16 = (8L << 16);		// domain limit of fast trig functions

inline long TRIG_ARG_L(long x)
{
	if (std::abs(x) > TRIG_LIMIT_16)
	{
		return DoubleToFudge(fmod(FudgeToDouble(x), g_two_pi));
	}
	return x;
}

// --------------------------------------------------------------------
// Fractal (once per iteration) routines
// --------------------------------------------------------------------

// Raise complex number (base) to the (exp) power, storing the result
// in complex (result).
//
void pow(ComplexD const *base, int exp, ComplexD *result)
{
	if (exp < 0)
	{
		pow(base, -exp, result);
		*result = reciprocal(*result);
		return;
	}

	s_xt = base->real();
	s_yt = base->imag();

	if (exp & 1)
	{
		result->real(s_xt);
		result->imag(s_yt);
	}
	else
	{
		result->real(1.0);
		result->imag(0.0);
	}

	exp >>= 1;
	while (exp)
	{
		double temp = s_xt*s_xt - s_yt*s_yt;
		s_yt = 2*s_xt*s_yt;
		s_xt = temp;

		if (exp & 1)
		{
				temp = s_xt*result->real() - s_yt*result->imag();
				result->imag(result->imag()*s_xt + s_yt*result->real());
				result->real(temp);
		}
		exp >>= 1;
	}
}

#if !defined(NO_FIXED_POINT_MATH)
// long version
static long lxt, lyt, lt2;
int complex_power_l(ComplexL *base, int exp, ComplexL *result, int bit_shift)
{
	long maxarg = 64L << bit_shift;

	if (exp < 0)
	{
		g_overflow = (complex_power_l(base, -exp, result, bit_shift) != 0);
		LCMPLXrecip(*result, *result);
		return g_overflow;
	}

	g_overflow = false;
	lxt = base->real();
	lyt = base->imag();

	if (exp & 1)
	{
		result->real(lxt);
		result->imag(lyt);
	}
	else
	{
		result->real(1L << bit_shift);
		result->imag(0L);
	}

	exp >>= 1;
	while (exp)
	{
		//
		// if (std::abs(lxt) >= maxarg || std::abs(lyt) >= maxarg)
		//	return -1;
		//
		lt2 = multiply(lxt, lxt, bit_shift) - multiply(lyt, lyt, bit_shift);
		lyt = multiply(lxt, lyt, g_bit_shift_minus_1);
		if (g_overflow)
		{
			return g_overflow;
		}
		lxt = lt2;

		if (exp & 1)
		{
				lt2 = multiply(lxt, result->real(), bit_shift) - multiply(lyt, result->imag(), bit_shift);
				result->imag(multiply(result->imag(), lxt, bit_shift) + multiply(lyt, result->real(), bit_shift));
				result->real(lt2);
		}
		exp >>= 1;
	}
	if (result->real() == 0 && result->imag() == 0)
	{
		g_overflow = true;
	}
	return g_overflow;
}
#endif

int barnsley1_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	// Barnsley's Mandelbrot type M1 from "Fractals
	// Everywhere" by Michael Barnsley, p. 322

	// calculate intermediate products
	s_old_x_init_x = multiply(g_old_z_l.real(), g_long_parameter->real(), g_bit_shift);
	s_old_y_init_y = multiply(g_old_z_l.imag(), g_long_parameter->imag(), g_bit_shift);
	s_old_x_init_y = multiply(g_old_z_l.real(), g_long_parameter->imag(), g_bit_shift);
	s_old_y_init_x = multiply(g_old_z_l.imag(), g_long_parameter->real(), g_bit_shift);
	// orbit calculation
	if (g_old_z_l.real() >= 0)
	{
		g_new_z_l.real((s_old_x_init_x - g_long_parameter->real() - s_old_y_init_y));
		g_new_z_l.imag((s_old_y_init_x - g_long_parameter->imag() + s_old_x_init_y));
	}
	else
	{
		g_new_z_l.real((s_old_x_init_x + g_long_parameter->real() - s_old_y_init_y));
		g_new_z_l.imag((s_old_y_init_x + g_long_parameter->imag() + s_old_x_init_y));
	}
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

int barnsley1_orbit_fp()
{
	// Barnsley's Mandelbrot type M1 from "Fractals
	// Everywhere" by Michael Barnsley, p. 322
	// note that fast >= 287 equiv in fracsuba.asm must be kept in step

	// calculate intermediate products
	s_old_x_init_x_fp = g_old_z.real()*g_float_parameter->real();
	s_old_x_init_y_fp = g_old_z.real()*g_float_parameter->imag();
	s_old_y_init_x_fp = g_old_z.imag()*g_float_parameter->real();
	s_old_y_init_y_fp = g_old_z.imag()*g_float_parameter->imag();
	// orbit calculation
	if (g_old_z.real() >= 0)
	{
		g_new_z = MakeComplexT(s_old_x_init_x_fp - s_old_y_init_y_fp, s_old_y_init_x_fp + s_old_x_init_y_fp)
			- *g_float_parameter;
	}
	else
	{
		g_new_z = MakeComplexT(s_old_x_init_x_fp - s_old_y_init_y_fp, s_old_y_init_x_fp + s_old_x_init_y_fp)
			+ *g_float_parameter;
	}
	return g_externs.BailOutFp();
}

int barnsley2_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	// An unnamed Mandelbrot/Julia function from "Fractals
	// Everywhere" by Michael Barnsley, p. 331, example 4.2
	// note that fast >= 287 equiv in fracsuba.asm must be kept in step

	// calculate intermediate products
	s_old_x_init_x = multiply(g_old_z_l.real(), g_long_parameter->real(), g_bit_shift);
	s_old_y_init_y = multiply(g_old_z_l.imag(), g_long_parameter->imag(), g_bit_shift);
	s_old_y_init_x = multiply(g_old_z_l.imag(), g_long_parameter->real(), g_bit_shift);
	s_old_x_init_y = multiply(g_old_z_l.real(), g_long_parameter->imag(), g_bit_shift);

	// orbit calculation
	if (s_old_x_init_y + s_old_y_init_x >= 0)
	{
		g_new_z_l.real(s_old_x_init_x - g_long_parameter->real() - s_old_y_init_y);
		g_new_z_l.imag(s_old_y_init_x - g_long_parameter->imag() + s_old_x_init_y);
	}
	else
	{
		g_new_z_l.real(s_old_x_init_x + g_long_parameter->real() - s_old_y_init_y);
		g_new_z_l.imag(s_old_y_init_x + g_long_parameter->imag() + s_old_x_init_y);
	}
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

int barnsley2_orbit_fp()
{
	// An unnamed Mandelbrot/Julia function from "Fractals
	// Everywhere" by Michael Barnsley, p. 331, example 4.2

	// calculate intermediate products
	s_old_x_init_x_fp = g_old_z.real()*g_float_parameter->real();
	s_old_x_init_y_fp = g_old_z.real()*g_float_parameter->imag();
	s_old_y_init_x_fp = g_old_z.imag()*g_float_parameter->real();
	s_old_y_init_y_fp = g_old_z.imag()*g_float_parameter->imag();

	// orbit calculation
	if (s_old_x_init_y_fp + s_old_y_init_x_fp >= 0)
	{
		g_new_z = MakeComplexT(s_old_x_init_x_fp - s_old_y_init_y_fp, s_old_y_init_x_fp + s_old_x_init_y_fp)
			- *g_float_parameter;
	}
	else
	{
		g_new_z = MakeComplexT(s_old_x_init_x_fp - s_old_y_init_y_fp, s_old_y_init_x_fp + s_old_x_init_y_fp)
			+ *g_float_parameter;
	}
	return g_externs.BailOutFp();
}

int julia_orbit()
{
	// used for C prototype of fast integer math routines for classic
	// Mandelbrot and Julia
	g_new_z_l.real(g_temp_sqr_l.real() - g_temp_sqr_l.imag() + g_long_parameter->real());
	g_new_z_l.imag(multiply(g_old_z_l.real(), g_old_z_l.imag(), g_bit_shift_minus_1) + g_long_parameter->imag());
	return g_externs.BailOutL();
}

int julia_orbit_fp()
{
	// floating point version of classical Mandelbrot/Julia
	// note that fast >= 287 equiv in fracsuba.asm must be kept in step
	g_new_z = MakeComplexT(g_temp_sqr.real() - g_temp_sqr.imag(), 2.0*g_old_z.real()*g_old_z.imag())
		+ *g_float_parameter;
	return g_externs.BailOutFp();
}

int lambda_orbit_fp()
{
	// variation of classical Mandelbrot/Julia
	// note that fast >= 287 equiv in fracsuba.asm must be kept in step

	g_temp_sqr.real(g_old_z.real() - g_temp_sqr.real() + g_temp_sqr.imag());
	g_temp_sqr.imag(-g_old_z.imag()*g_old_z.real());
	g_temp_sqr.imag(g_temp_sqr.imag() + g_temp_sqr.imag() + g_old_z.imag());

	g_new_z = *g_float_parameter*g_temp_sqr;
	return g_externs.BailOutFp();
}

int lambda_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	// variation of classical Mandelbrot/Julia

	// in complex math) temp = Z*(1-Z)
	g_temp_sqr_l.real(g_old_z_l.real() - g_temp_sqr_l.real() + g_temp_sqr_l.imag());
	g_temp_sqr_l.imag(g_old_z_l.imag() - multiply(g_old_z_l.imag(), g_old_z_l.real(), g_bit_shift_minus_1));
	// (in complex math) Z = Lambda*Z
	g_new_z_l.real(multiply(g_long_parameter->real(), g_temp_sqr_l.real(), g_bit_shift)
		- multiply(g_long_parameter->imag(), g_temp_sqr_l.imag(), g_bit_shift));
	g_new_z_l.imag(multiply(g_long_parameter->real(), g_temp_sqr_l.imag(), g_bit_shift)
		+ multiply(g_long_parameter->imag(), g_temp_sqr_l.real(), g_bit_shift));
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

int sierpinski_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	// following code translated from basic - see "Fractals
	// Everywhere" by Michael Barnsley, p. 251, Program 7.1.1
	g_new_z_l.real((g_old_z_l.real() << 1));				// new.real() = 2*old.real()
	g_new_z_l.imag((g_old_z_l.imag() << 1));				// new.imag() = 2*old.imag()
	if (g_old_z_l.imag() > g_temp_z_l.imag())				// if old.imag() > .5
	{
		g_new_z_l.imag(g_new_z_l.imag() - g_temp_z_l.real()); // new.imag() = 2*old.imag() - 1
	}
	else if (g_old_z_l.real() > g_temp_z_l.imag())			// if old.real() > .5
	{
		g_new_z_l.real(g_new_z_l.real() - g_temp_z_l.real()); // new.real() = 2*old.real() - 1
	}
	// end barnsley code
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

int sierpinski_orbit_fp()
{
	// following code translated from basic - see "Fractals
	// Everywhere" by Michael Barnsley, p. 251, Program 7.1.1
	g_new_z = g_old_z + g_old_z;
	if (g_old_z.imag() > .5)
	{
		g_new_z.imag(g_new_z.imag() - 1);
	}
	else if (g_old_z.real() > .5)
	{
		g_new_z.real(g_new_z.real() - 1);
	}

	// end barnsley code
	return g_externs.BailOutFp();
}

int lambda_exponent_orbit_fp()
{
	// found this in  "Science of Fractal Images"
	if ((std::abs(g_old_z.imag()) >= 1.0e3)
		|| (std::abs(g_old_z.real()) >= 8.0))
	{
		return 1;
	}
	s_sin_y = std::sin(g_old_z.imag());
	s_cos_y = std::cos(g_old_z.imag());

	if (g_old_z.real() >= g_rq_limit && s_cos_y >= 0.0)
	{
		return 1;
	}
	s_temp_exp = std::exp(g_old_z.real());
	g_temp_z = s_temp_exp*MakeComplexT(s_cos_y, s_sin_y);

	// multiply by lamda
	g_new_z = *g_float_parameter*g_temp_z;
	g_old_z = g_new_z;
	return 0;
}

int lambda_exponent_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	long tmp;
	// found this in  "Science of Fractal Images"
	assert((1000L << g_bit_shift) == DoubleToFudge(1000.0));
	assert((8L << g_bit_shift) == DoubleToFudge(8.0));
	if ((std::abs(g_old_z_l.imag()) >= (1000L << g_bit_shift))
		|| (std::abs(g_old_z_l.real()) >= (8L << g_bit_shift)))
	{
		return 1;
	}

	SinCos086(g_old_z_l.imag(), &s_sin_y_l,  &s_cos_y_l);

	if (g_old_z_l.real() >= g_rq_limit_l && s_cos_y_l >= 0L)
	{
		return 1;
	}
	tmp = Exp086(g_old_z_l.real());

	g_temp_z_l.real(multiply(tmp,      s_cos_y_l,   g_bit_shift));
	g_temp_z_l.imag(multiply(tmp,      s_sin_y_l,   g_bit_shift));

	g_new_z_l.real(multiply(g_long_parameter->real(), g_temp_z_l.real(), g_bit_shift)
			- multiply(g_long_parameter->imag(), g_temp_z_l.imag(), g_bit_shift));
	g_new_z_l.imag(multiply(g_long_parameter->real(), g_temp_z_l.imag(), g_bit_shift)
			+ multiply(g_long_parameter->imag(), g_temp_z_l.real(), g_bit_shift));
	g_old_z_l = g_new_z_l;
	return 0;
#else
	return 0;
#endif
}

int trig_plus_exponent_orbit_fp()
{
	// another Scientific American biomorph type
	// z(n + 1) = e**z(n) + trig(z(n)) + C

	if (std::abs(g_old_z.real()) >= 6.4e2) // DOMAIN errors
	{
		return 1;
	}
	s_temp_exp = std::exp(g_old_z.real());
	s_sin_y = std::sin(g_old_z.imag());
	s_cos_y = std::cos(g_old_z.imag());
	g_new_z = CMPLXtrig0(g_old_z);

	// new =   trig(old) + e**old + C
	g_new_z = g_new_z + s_temp_exp*MakeComplexT(s_cos_y, s_sin_y) + *g_float_parameter;
	return g_externs.BailOutFp();
}

int trig_plus_exponent_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	// calculate exp(z)
	long tmp;

	// domain check for fast transcendental functions
	if ((std::abs(g_old_z_l.real()) > TRIG_LIMIT_16)
		|| (std::abs(g_old_z_l.imag()) > TRIG_LIMIT_16))
	{
		return 1;
	}

	tmp = Exp086(g_old_z_l.real());
	SinCos086  (g_old_z_l.imag(), &s_sin_y_l,  &s_cos_y_l);
	LCMPLXtrig0(g_old_z_l, g_new_z_l);
	g_new_z_l.real(g_new_z_l.real() + multiply(tmp, s_cos_y_l, g_bit_shift) + g_long_parameter->real());
	g_new_z_l.imag(g_new_z_l.imag() + multiply(tmp, s_sin_y_l, g_bit_shift) + g_long_parameter->imag());
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

int marks_lambda_orbit()
{
	// Mark Peterson's variation of "lambda" function

	// Z1 = (C^(exp-1)*Z**2) + C
#if !defined(NO_FIXED_POINT_MATH)
	g_temp_z_l.real(g_temp_sqr_l.real() - g_temp_sqr_l.imag());
	g_temp_z_l.imag(multiply(g_old_z_l.real() , g_old_z_l.imag() , g_bit_shift_minus_1));

	g_new_z_l.real(multiply(g_coefficient_l.real(), g_temp_z_l.real(), g_bit_shift)
		- multiply(g_coefficient_l.imag(), g_temp_z_l.imag(), g_bit_shift) + g_long_parameter->real());
	g_new_z_l.imag(multiply(g_coefficient_l.real(), g_temp_z_l.imag(), g_bit_shift)
		+ multiply(g_coefficient_l.imag(), g_temp_z_l.real(), g_bit_shift) + g_long_parameter->imag());

	return g_externs.BailOutL();
#else
	return 0;
#endif
}

int marks_lambda_orbit_fp()
{
	// Mark Peterson's variation of "lambda" function

	// Z1 = (C^(exp-1)*Z**2) + C
	g_temp_z = MakeComplexT(g_temp_sqr.real() - g_temp_sqr.imag(), g_old_z.real()*g_old_z.imag()*2);

	g_new_z = MakeComplexT(g_coefficient)*g_temp_z + *g_float_parameter;

	return g_externs.BailOutFp();
}

int unity_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	// brought to you by Mark Peterson - you won't find this in any fractal
	// books unless they saw it here first - Mark invented it!
	long xx_one = multiply(g_old_z_l.real(), g_old_z_l.real(), g_bit_shift) + multiply(g_old_z_l.imag(), g_old_z_l.imag(), g_bit_shift);
	if ((xx_one > g_two_fudge) || (std::abs(xx_one - g_one_fudge) < g_delta_min))
	{
		return 1;
	}
	g_old_z_l.imag(multiply(g_two_fudge - xx_one, g_old_z_l.real(), g_bit_shift));
	g_old_z_l.real(multiply(g_two_fudge - xx_one, g_old_z_l.imag(), g_bit_shift));
	g_new_z_l = g_old_z_l;  // TW added this line
	return 0;
#else
	return 0;
#endif
}

int unity_orbit_fp()
{
	// brought to you by Mark Peterson - you won't find this in any fractal
	// books unless they saw it here first - Mark invented it!
	double xx_one = norm(g_old_z);
	if ((xx_one > 2.0) || (std::abs(xx_one - 1.0) < g_delta_min_fp))
	{
		return 1;
	}
	g_old_z = (2.0 - xx_one)*g_old_z;
	g_new_z = g_old_z;
	return 0;
}

int mandel4_orbit()
{
	// By writing this code, Bert has left behind the excuse "don't
	// know what a fractal is, just know how to make'em go fast".
	// Bert is hereby declared a bonafide fractal expert! Supposedly
	// this routine calculates the Mandelbrot/Julia set based on the
	// polynomial z**4 + lambda, but I wouldn't know -- can't follow
	// all that integer math speedup stuff - Tim

	// first, compute (x + iy)**2
#if !defined(NO_FIXED_POINT_MATH)
	g_new_z_l.real(g_temp_sqr_l.real() - g_temp_sqr_l.imag());
	g_new_z_l.imag(multiply(g_old_z_l.real(), g_old_z_l.imag(), g_bit_shift_minus_1));
	if (g_externs.BailOutL())
	{
		return 1;
	}

	// then, compute ((x + iy)**2)**2 + lambda
	g_new_z_l.real(g_temp_sqr_l.real() - g_temp_sqr_l.imag() + g_long_parameter->real());
	g_new_z_l.imag(multiply(g_old_z_l.real(), g_old_z_l.imag(), g_bit_shift_minus_1) + g_long_parameter->imag());
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

int mandel4_orbit_fp()
{
	// first, compute (x + iy)**2
	g_new_z = sqr(g_old_z);
	if (g_externs.BailOutFp())
	{
		return 1;
	}

	// then, compute ((x + iy)**2)**2 + lambda
	g_new_z = sqr(g_new_z) + *g_float_parameter;
	return g_externs.BailOutFp();
}

int z_to_z_plus_z_orbit_fp()
{
	pow(&g_old_z, int(g_parameters[P2_REAL]), &g_new_z);
	g_old_z = pow(g_old_z, g_old_z);
	g_new_z = g_new_z + g_old_z + *g_float_parameter;
	return g_externs.BailOutFp();
}

int z_power_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	if (complex_power_l(&g_old_z_l, g_c_exp, &g_new_z_l, g_bit_shift))
	{
		g_new_z_l.real(g_new_z_l.imag(8L << g_bit_shift));
	}
	g_new_z_l = g_new_z_l + *g_long_parameter;
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

int complex_z_power_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	ComplexD x = ComplexFudgeToDouble(g_old_z_l);
	ComplexD y = ComplexFudgeToDouble(g_parameter2_l);
	x = pow(x, y);
	if (std::abs(x.real()) < g_fudge_limit && std::abs(x.imag()) < g_fudge_limit)
	{
		g_new_z_l = ComplexDoubleToFudge(x);
	}
	else
	{
		g_overflow = true;
	}
	g_new_z_l = g_new_z_l + *g_long_parameter;
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

int z_power_orbit_fp()
{
	pow(&g_old_z, g_c_exp, &g_new_z);
	g_new_z = g_new_z + *g_float_parameter;
	return g_externs.BailOutFp();
}

int complex_z_power_orbit_fp()
{
	g_new_z = pow(g_old_z, g_parameter2) + *g_float_parameter;
	return g_externs.BailOutFp();
}

int barnsley3_orbit()
{
	// An unnamed Mandelbrot/Julia function from "Fractals
	// Everywhere" by Michael Barnsley, p. 292, example 4.1

	// calculate intermediate products
#if !defined(NO_FIXED_POINT_MATH)
	s_old_x_init_x = multiply(g_old_z_l.real(), g_old_z_l.real(), g_bit_shift);
	s_old_y_init_y = multiply(g_old_z_l.imag(), g_old_z_l.imag(), g_bit_shift);
	s_old_x_init_y = multiply(g_old_z_l.real(), g_old_z_l.imag(), g_bit_shift);

	// orbit calculation
	if (g_old_z_l.real() > 0)
	{
		g_new_z_l.real(s_old_x_init_x   - s_old_y_init_y - g_externs.Fudge());
		g_new_z_l.imag(s_old_x_init_y << 1);
	}
	else
	{
		g_new_z_l.real(s_old_x_init_x - s_old_y_init_y - g_externs.Fudge()
			+ multiply(g_long_parameter->real(), g_old_z_l.real(), g_bit_shift));
		g_new_z_l.imag(s_old_x_init_y <<1);

		// This term added by Tim Wegner to make dependent on the
		// imaginary part of the parameter. (Otherwise Mandelbrot
		// is uninteresting.
		g_new_z_l.imag(g_new_z_l.imag() + multiply(g_long_parameter->imag(), g_old_z_l.real(), g_bit_shift));
	}
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

//
//	An unnamed Mandelbrot/Julia function from "Fractals
//	Everywhere" by Michael Barnsley, p. 292, example 4.1
//
int barnsley3_orbit_fp()
{
	// calculate intermediate products
	s_old_x_init_x_fp = sqr(g_old_z.real());
	s_old_y_init_y_fp = sqr(g_old_z.imag());
	s_old_x_init_y_fp = g_old_z.real()*g_old_z.imag();

	// orbit calculation
	if (g_old_z.real() > 0)
	{
		g_new_z = MakeComplexT(s_old_x_init_x_fp - s_old_y_init_y_fp - 1.0, s_old_x_init_y_fp*2);
	}
	else
	{
		g_new_z = MakeComplexT(
			s_old_x_init_x_fp - s_old_y_init_y_fp - 1.0 + g_float_parameter->real()*g_old_z.real(),
			s_old_x_init_y_fp*2);

		// This term added by Tim Wegner to make dependent on the
		// imaginary part of the parameter. (Otherwise Mandelbrot
		// is uninteresting.
		g_new_z.imag(g_new_z.imag() + g_float_parameter->imag()*g_old_z.real());
	}
	return g_externs.BailOutFp();
}

// From Scientific American, July 1989
// A Biomorph
// z(n + 1) = trig(z(n)) + z(n)**2 + C
int trig_plus_z_squared_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	LCMPLXtrig0(g_old_z_l, g_new_z_l);
	g_new_z_l.real(g_new_z_l.real() + g_temp_sqr_l.real() - g_temp_sqr_l.imag() + g_long_parameter->real());
	g_new_z_l.imag(g_new_z_l.imag() + multiply(g_old_z_l.real(), g_old_z_l.imag(), g_bit_shift_minus_1) + g_long_parameter->imag());
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

// From Scientific American, July 1989
// A Biomorph
// z(n + 1) = trig(z(n)) + z(n)**2 + C
int trig_plus_z_squared_orbit_fp()
{
	g_new_z = CMPLXtrig0(g_old_z) + sqr(g_old_z) + *g_float_parameter;
	return g_externs.BailOutFp();
}

// Richard8 {c = z = pixel: z = sin(z) + sin(pixel), |z| <= 50}
int richard8_orbit_fp()
{
	g_new_z = CMPLXtrig0(g_old_z);
	// g_temp_z = CMPLXtrig1(*g_float_parameter);
	g_new_z = g_new_z + g_temp_z;
	return g_externs.BailOutFp();
}

// Richard8 {c = z = pixel: z = sin(z) + sin(pixel), |z| <= 50}
int richard8_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	LCMPLXtrig0(g_old_z_l, g_new_z_l);
	// LCMPLXtrig1(*g_long_parameter, g_temp_z_l);
	g_new_z_l = g_new_z_l + g_temp_z_l;
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

int popcorn_old_orbit_fp()
{
	g_temp_z = g_old_z*3.0;
	g_sin_x = std::sin(g_temp_z.real());
	g_cos_x = std::cos(g_temp_z.real());
	s_sin_y = std::sin(g_temp_z.imag());
	s_cos_y = std::cos(g_temp_z.imag());
	g_temp_z = MakeComplexT(g_sin_x/g_cos_x, s_sin_y/s_cos_y) + g_old_z;
	g_sin_x = std::sin(g_temp_z.real());
	g_cos_x = std::cos(g_temp_z.real());
	s_sin_y = std::sin(g_temp_z.imag());
	s_cos_y = std::cos(g_temp_z.imag());
	g_new_z = g_old_z - g_parameter.real()*MakeComplexT(s_sin_y, g_sin_x);
	if (g_plot_color == plot_color_none)
	{
		plot_orbit(g_new_z.real(), g_new_z.imag(), 1 + g_row % g_colors);
		g_old_z = g_new_z;
	}
	else
	{
		// FLOATBAILOUT();
		// PB The above line was weird, not what it seems to be!  But, bracketing
		// it or always doing it (either of which seem more likely to be what
		// was intended) changes the image for the worse, so I'm not touching it.
		// Same applies to int form in next routine.
		// PB later: recoded inline, still leaving it weird
		g_temp_sqr.real(sqr(g_new_z.real()));
	}
	g_temp_sqr.imag(sqr(g_new_z.imag()));
	g_magnitude = g_temp_sqr.real() + g_temp_sqr.imag();
	if (g_magnitude >= g_rq_limit)
	{
		return 1;
	}
	g_old_z = g_new_z;
	return 0;
}

int popcorn_orbit_fp()
{
	g_temp_z = g_old_z*3.0;
	g_sin_x = std::sin(g_temp_z.real());
	g_cos_x = std::cos(g_temp_z.real());
	s_sin_y = std::sin(g_temp_z.imag());
	s_cos_y = std::cos(g_temp_z.imag());
	g_temp_z = MakeComplexT(g_sin_x/g_cos_x, s_sin_y/s_cos_y) + g_old_z;
	g_sin_x = std::sin(g_temp_z.real());
	g_cos_x = std::cos(g_temp_z.real());
	s_sin_y = std::sin(g_temp_z.imag());
	s_cos_y = std::cos(g_temp_z.imag());
	g_new_z = g_old_z - g_parameter.real()*MakeComplexT(s_sin_y, g_sin_x);
	//
	// g_new_z.real(g_old_z.real() - g_parameter.real()*sin(g_old_z.imag() + tan(3*g_old_z.imag())));
	// g_new_z.imag(g_old_z.imag() - g_parameter.real()*sin(g_old_z.real() + tan(3*g_old_z.real())));
	//
	if (g_plot_color == plot_color_none)
	{
		plot_orbit(g_new_z.real(), g_new_z.imag(), 1 + g_row % g_colors);
		g_old_z = g_new_z;
	}
	// else
	// FLOATBAILOUT();
	// PB The above line was weird, not what it seems to be!  But, bracketing
	//		it or always doing it (either of which seem more likely to be what
	//		was intended) changes the image for the worse, so I'm not touching it.
	//		Same applies to int form in next routine.
	// PB later: recoded inline, still leaving it weird
	// JCO: sqr's should always be done, else magnitude could be wrong
	g_temp_sqr.real(sqr(g_new_z.real()));
	g_temp_sqr.imag(sqr(g_new_z.imag()));
	g_magnitude = g_temp_sqr.real() + g_temp_sqr.imag();
	if (g_magnitude >= g_rq_limit
		|| std::abs(g_new_z.real()) > g_rq_limit2
		|| std::abs(g_new_z.imag()) > g_rq_limit2)
	{
		return 1;
	}
	g_old_z = g_new_z;
	return 0;
}

int popcorn_old_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	g_temp_z_l = g_old_z_l;
	g_temp_z_l.real(g_temp_z_l.real()*3L);
	g_temp_z_l.imag(g_temp_z_l.imag()*3L);
	g_temp_z_l.real(TRIG_ARG_L(g_temp_z_l.real()));
	g_temp_z_l.imag(TRIG_ARG_L(g_temp_z_l.imag()));
	SinCos086(g_temp_z_l.real(), &s_sin_x_l, &s_cos_x_l);
	SinCos086(g_temp_z_l.imag(), &s_sin_y_l, &s_cos_y_l);
	g_temp_z_l.real(divide(s_sin_x_l, s_cos_x_l, g_bit_shift) + g_old_z_l.real());
	g_temp_z_l.imag(divide(s_sin_y_l, s_cos_y_l, g_bit_shift) + g_old_z_l.imag());
	g_temp_z_l.real(TRIG_ARG_L(g_temp_z_l.real()));
	g_temp_z_l.imag(TRIG_ARG_L(g_temp_z_l.imag()));
	SinCos086(g_temp_z_l.real(), &s_sin_x_l, &s_cos_x_l);
	SinCos086(g_temp_z_l.imag(), &s_sin_y_l, &s_cos_y_l);
	g_new_z_l.real(g_old_z_l.real() - multiply(g_parameter_l.real(), s_sin_y_l, g_bit_shift));
	g_new_z_l.imag(g_old_z_l.imag() - multiply(g_parameter_l.real(), s_sin_x_l, g_bit_shift));
	if (g_plot_color == plot_color_none)
	{
		plot_orbit_i(g_new_z_l.real(), g_new_z_l.imag(), 1 + g_row % g_colors);
		g_old_z_l = g_new_z_l;
	}
	else
	{
		// LONGBAILOUT();
		// PB above still the old way, is weird, see notes in FP popcorn case
		g_temp_sqr_l.real(lsqr(g_new_z_l.real()));
		g_temp_sqr_l.imag(lsqr(g_new_z_l.imag()));
	}
	g_magnitude_l = g_temp_sqr_l.real() + g_temp_sqr_l.imag();
	if (g_magnitude_l >= g_rq_limit_l
		|| g_magnitude_l < 0
		|| std::abs(g_new_z_l.real()) > g_rq_limit2_l
		|| std::abs(g_new_z_l.imag()) > g_rq_limit2_l)
	{
		return 1;
	}
	g_old_z_l = g_new_z_l;
	return 0;
#else
	return 0;
#endif
}

int popcorn_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	g_temp_z_l = g_old_z_l;
	g_temp_z_l.real(g_temp_z_l.real()*3L);
	g_temp_z_l.imag(g_temp_z_l.imag()*3L);
	g_temp_z_l.real(TRIG_ARG_L(g_temp_z_l.real()));
	g_temp_z_l.imag(TRIG_ARG_L(g_temp_z_l.imag()));
	SinCos086(g_temp_z_l.real(), &s_sin_x_l, &s_cos_x_l);
	SinCos086(g_temp_z_l.imag(), &s_sin_y_l, &s_cos_y_l);
	g_temp_z_l.real(divide(s_sin_x_l, s_cos_x_l, g_bit_shift) + g_old_z_l.real());
	g_temp_z_l.imag(divide(s_sin_y_l, s_cos_y_l, g_bit_shift) + g_old_z_l.imag());
	g_temp_z_l.real(TRIG_ARG_L(g_temp_z_l.real()));
	g_temp_z_l.imag(TRIG_ARG_L(g_temp_z_l.imag()));
	SinCos086(g_temp_z_l.real(), &s_sin_x_l, &s_cos_x_l);
	SinCos086(g_temp_z_l.imag(), &s_sin_y_l, &s_cos_y_l);
	g_new_z_l.real(g_old_z_l.real() - multiply(g_parameter_l.real(), s_sin_y_l, g_bit_shift));
	g_new_z_l.imag(g_old_z_l.imag() - multiply(g_parameter_l.real(), s_sin_x_l, g_bit_shift));
	if (g_plot_color == plot_color_none)
	{
		plot_orbit_i(g_new_z_l.real(), g_new_z_l.imag(), 1 + g_row % g_colors);
		g_old_z_l = g_new_z_l;
	}
	// else
	// JCO: sqr's should always be done, else magnitude could be wrong
	g_temp_sqr_l.real(lsqr(g_new_z_l.real()));
	g_temp_sqr_l.imag(lsqr(g_new_z_l.imag()));
	g_magnitude_l = g_temp_sqr_l.real() + g_temp_sqr_l.imag();
	if (g_magnitude_l >= g_rq_limit_l
		|| g_magnitude_l < 0
		|| std::abs(g_new_z_l.real()) > g_rq_limit2_l
		|| std::abs(g_new_z_l.imag()) > g_rq_limit2_l)
	{
		return 1;
	}
	g_old_z_l = g_new_z_l;
	return 0;
#else
	return 0;
#endif
}

// Popcorn generalization proposed by HB

int popcorn_fn_orbit_fp()
{
	ComplexD tmpx;
	ComplexD tmpy;

	// tmpx contains the generalized value of the old real "x" equation
	g_temp_z = g_parameter2*g_old_z.imag();		// tmp = (C*old.imag())
	tmpx = CMPLXtrig1(g_temp_z);				// tmpx = trig1(tmp)
	tmpx.real(tmpx.real() + g_old_z.imag());	// tmpx = old.imag() + trig1(tmp)
	g_temp_z = CMPLXtrig0(tmpx);				// tmp = trig0(tmpx)
	tmpx = g_temp_z*g_parameter;				// tmpx = tmp*h

	// tmpy contains the generalized value of the old real "y" equation
	g_temp_z = g_parameter2*g_old_z.real();		// tmp = (C*old.real())
	tmpy = CMPLXtrig3(g_temp_z);				// tmpy = trig3(tmp)
	tmpy.real(tmpy.real() + g_old_z.real());	// tmpy = old.real() + trig1(tmp)
	g_temp_z = CMPLXtrig2(tmpy);				// tmp = trig2(tmpy)

	tmpy = g_temp_z*g_parameter;				// tmpy = tmp*h

	g_new_z = g_old_z - MakeComplexT(tmpx.real() + tmpy.imag(), tmpy.real() + tmpx.imag());

	if (g_plot_color == plot_color_none)
	{
		plot_orbit(g_new_z.real(), g_new_z.imag(), 1 + g_row % g_colors);
		g_old_z = g_new_z;
	}

	g_temp_sqr.real(sqr(g_new_z.real()));
	g_temp_sqr.imag(sqr(g_new_z.imag()));
	g_magnitude = g_temp_sqr.real() + g_temp_sqr.imag();
	if (g_magnitude >= g_rq_limit
		|| std::abs(g_new_z.real()) > g_rq_limit2
		|| std::abs(g_new_z.imag()) > g_rq_limit2)
	{
		return 1;
	}
	g_old_z = g_new_z;
	return 0;
}

inline void fix_overflow(ComplexL &arg)
{
	if (g_overflow)
	{
		arg.real(g_externs.Fudge());
		arg.imag(0);
		g_overflow = false;
	}
}

int popcorn_fn_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	ComplexL ltmpx, ltmpy;

	g_overflow = false;

	// ltmpx contains the generalized value of the old real "x" equation
	LCMPLXtimesreal(g_parameter2_l, g_old_z_l.imag(), g_temp_z_l); // tmp = (C*old.imag())
	LCMPLXtrig1(g_temp_z_l, ltmpx);             // tmpx = trig1(tmp)
	fix_overflow(ltmpx);
	ltmpx.real(ltmpx.real() + g_old_z_l.imag());                   // tmpx = old.imag() + trig1(tmp)
	LCMPLXtrig0(ltmpx, g_temp_z_l);             // tmp = trig0(tmpx)
	fix_overflow(g_temp_z_l);
	LCMPLXmult(g_temp_z_l, g_parameter_l, ltmpx);        // tmpx = tmp*h

	// ltmpy contains the generalized value of the old real "y" equation
	LCMPLXtimesreal(g_parameter2_l, g_old_z_l.real(), g_temp_z_l); // tmp = (C*old.real())
	LCMPLXtrig3(g_temp_z_l, ltmpy);             // tmpy = trig3(tmp)
	fix_overflow(ltmpy);
	ltmpy.real(ltmpy.real() + g_old_z_l.real());                   // tmpy = old.real() + trig1(tmp)
	LCMPLXtrig2(ltmpy, g_temp_z_l);             // tmp = trig2(tmpy)
	fix_overflow(g_temp_z_l);
	LCMPLXmult(g_temp_z_l, g_parameter_l, ltmpy);        // tmpy = tmp*h

	g_new_z_l.real(g_old_z_l.real() - ltmpx.real() - ltmpy.imag());
	g_new_z_l.imag(g_old_z_l.imag() - ltmpy.real() - ltmpx.imag());

	if (g_plot_color == plot_color_none)
	{
		plot_orbit_i(g_new_z_l.real(), g_new_z_l.imag(), 1 + g_row % g_colors);
		g_old_z_l = g_new_z_l;
	}
	g_temp_sqr_l.real(lsqr(g_new_z_l.real()));
	g_temp_sqr_l.imag(lsqr(g_new_z_l.imag()));
	g_magnitude_l = g_temp_sqr_l.real() + g_temp_sqr_l.imag();
	if (g_magnitude_l >= g_rq_limit_l || g_magnitude_l < 0
		|| std::abs(g_new_z_l.real()) > g_rq_limit2_l
		|| std::abs(g_new_z_l.imag()) > g_rq_limit2_l)
	{
		return 1;
	}
	g_old_z_l = g_new_z_l;
	return 0;
#else
	return 0;
#endif
}

int marks_complex_mandelbrot_orbit()
{
	g_new_z = sqr(g_old_z)*g_coefficient + *g_float_parameter;
	return g_externs.BailOutFp();
}

int spider_orbit_fp()
{
	// Spider(XAXIS) { c = z=pixel: z = z*z + c; c = c/2 + z, |z| <= 4 }
	g_new_z = sqr(g_old_z) + g_temp_z;
	g_temp_z = g_temp_z/2.0 + g_new_z;
	return g_externs.BailOutFp();
}

int spider_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	// Spider(XAXIS) { c = z=pixel: z = z*z + c; c = c/2 + z, |z| <= 4 }
	g_new_z_l.real(g_temp_sqr_l.real() - g_temp_sqr_l.imag() + g_temp_z_l.real());
	g_new_z_l.imag(multiply(g_old_z_l.real(), g_old_z_l.imag(), g_bit_shift_minus_1) + g_temp_z_l.imag());
	g_temp_z_l.real((g_temp_z_l.real() >> 1) + g_new_z_l.real());
	g_temp_z_l.imag((g_temp_z_l.imag() >> 1) + g_new_z_l.imag());
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

int tetrate_orbit_fp()
{
	// Tetrate(XAXIS) { c = z=pixel: z = c^z, |z| <= (P1 + 3) }
	g_new_z = pow(*g_float_parameter, g_old_z);
	return g_externs.BailOutFp();
}

int z_trig_z_plus_z_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	// z = (p1*z*trig(z)) + p2*z
	LCMPLXtrig0(g_old_z_l, g_temp_z_l);					// g_temp_z_l = trig(old)
	LCMPLXmult(g_parameter_l, g_temp_z_l, g_temp_z_l);	// g_temp_z_l  = p1*trig(old)
	ComplexL temp;
	LCMPLXmult(g_old_z_l, g_temp_z_l, temp);			// temp = p1*old*trig(old)
	LCMPLXmult(g_parameter2_l, g_old_z_l, g_temp_z_l);	// g_temp_z_l  = p2*old
	LCMPLXadd(temp, g_temp_z_l, g_new_z_l);				// g_new_z_l  = temp + p2*old
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

int scott_z_trig_z_plus_z_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	// z = (z*trig(z)) + z
	LCMPLXtrig0(g_old_z_l, g_temp_z_l);          // g_temp_z_l  = trig(old)
	LCMPLXmult(g_old_z_l, g_temp_z_l, g_new_z_l);       // g_new_z_l  = old*trig(old)
	LCMPLXadd(g_new_z_l, g_old_z_l, g_new_z_l);        // g_new_z_l  = trig(old) + old
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

int skinner_z_trig_z_minus_z_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	// z = (z*trig(z))-z
	LCMPLXtrig0(g_old_z_l, g_temp_z_l);          // g_temp_z_l  = trig(old)
	LCMPLXmult(g_old_z_l, g_temp_z_l, g_new_z_l);       // g_new_z_l  = old*trig(old)
	LCMPLXsub(g_new_z_l, g_old_z_l, g_new_z_l);        // g_new_z_l  = trig(old) - old
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

int z_trig_z_plus_z_orbit_fp()
{
	// z = (p1*z*trig(z)) + p2*z
	g_temp_z = CMPLXtrig0(g_old_z);				// tmp  = trig(old)
	g_temp_z = g_parameter*g_temp_z;			// tmp  = p1*trig(old)
	s_temp2 = g_old_z*g_temp_z;					// s_temp2 = p1*old*trig(old)
	g_temp_z = g_parameter2*g_old_z;			// tmp  = p2*old
	g_new_z = s_temp2 + g_temp_z;				// new  = p1*trig(old) + p2*old
	return g_externs.BailOutFp();
}

int scott_z_trig_z_plus_z_orbit_fp()
{
	// z = (z*trig(z)) + z
	g_temp_z = CMPLXtrig0(g_old_z);				// tmp  = trig(old)
	g_new_z = g_old_z*g_temp_z;					// new  = old*trig(old)
	g_new_z = g_new_z + g_old_z;				// new  = trig(old) + old
	return g_externs.BailOutFp();
}

int skinner_z_trig_z_minus_z_orbit_fp()
{
	// z = (z*trig(z))-z
	g_temp_z = CMPLXtrig0(g_old_z);         // tmp  = trig(old)
	g_new_z = g_old_z*g_temp_z - g_old_z;
	return g_externs.BailOutFp();
}

int sqr_1_over_trig_z_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	// z = sqr(1/trig(z))
	LCMPLXtrig0(g_old_z_l, g_old_z_l);
	LCMPLXrecip(g_old_z_l, g_old_z_l);
	LCMPLXsqr(g_old_z_l, g_new_z_l);
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

int sqr_1_over_trig_z_orbit_fp()
{
	// z = sqr(1/trig(z))
	g_old_z = reciprocal(CMPLXtrig0(g_old_z));
	g_new_z = sqr(g_old_z);
	return g_externs.BailOutFp();
}

int trig_plus_trig_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	// z = trig(0, z)*p1 + trig1(z)*p2
	LCMPLXtrig0(g_old_z_l, g_temp_z_l);
	LCMPLXmult(g_parameter_l, g_temp_z_l, g_temp_z_l);
	ComplexL temp;
	LCMPLXtrig1(g_old_z_l, temp);
	LCMPLXmult(g_parameter2_l, temp, g_old_z_l);
	LCMPLXadd(g_temp_z_l, g_old_z_l, g_new_z_l);
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

int trig_plus_trig_orbit_fp()
{
	// z = trig0(z)*p1 + trig1(z)*p2
	g_temp_z = CMPLXtrig0(g_old_z);
	g_temp_z = g_parameter*g_temp_z;
	g_old_z = CMPLXtrig1(g_old_z);
	g_old_z = g_parameter2*g_old_z;
	g_new_z = g_temp_z + g_old_z;
	return g_externs.BailOutFp();
}

// The following four fractals are based on the idea of parallel
//	or alternate calculations.  The shift is made when the mod
//	reaches a given value.

int lambda_trig_or_trig_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	// z = trig0(z)*p1 if mod(old) < p2.real() and
	// trig1(z)*p1 if mod(old) >= p2.real()
	if ((norm(g_old_z_l)) < g_parameter2_l.real())
	{
		LCMPLXtrig0(g_old_z_l, g_temp_z_l);
		LCMPLXmult(*g_long_parameter, g_temp_z_l, g_new_z_l);
	}
	else
	{
		LCMPLXtrig1(g_old_z_l, g_temp_z_l);
		LCMPLXmult(*g_long_parameter, g_temp_z_l, g_new_z_l);
	}
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

int lambda_trig_or_trig_orbit_fp()
{
	// z = trig0(z)*p1 if mod(old) < p2.real() and
	//		trig1(z)*p1 if mod(old) >= p2.real()
	if (norm(g_old_z) < g_parameter2.real())
	{
		g_old_z = CMPLXtrig0(g_old_z);
	}
	else
	{
		g_old_z = CMPLXtrig1(g_old_z);
	}
	g_new_z = *g_float_parameter * g_old_z;
	return g_externs.BailOutFp();
}

int julia_trig_or_trig_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	// z = trig0(z) + p1 if mod(old) < p2.real() and
	//		trig1(z) + p1 if mod(old) >= p2.real()
	if (norm(g_old_z_l) < g_parameter2_l.real())
	{
		LCMPLXtrig0(g_old_z_l, g_temp_z_l);
		LCMPLXadd(*g_long_parameter, g_temp_z_l, g_new_z_l);
	}
	else
	{
		LCMPLXtrig1(g_old_z_l, g_temp_z_l);
		LCMPLXadd(*g_long_parameter, g_temp_z_l, g_new_z_l);
	}
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

int julia_trig_or_trig_orbit_fp()
{
	// z = trig0(z) + p1 if mod(old) < p2.real() and
	//     trig1(z) + p1 if mod(old) >= p2.real()
	if (norm(g_old_z) < g_parameter2.real())
	{
		g_old_z = CMPLXtrig0(g_old_z);
	}
	else
	{
		g_old_z = CMPLXtrig1(g_old_z);
	}
	g_new_z = *g_float_parameter + g_old_z;
	return g_externs.BailOutFp();
}

int phoenix_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	// z(n + 1) = z(n)^2 + p + qy(n),  y(n + 1) = z(n)
	g_temp_z_l.real(multiply(g_old_z_l.real(), g_old_z_l.imag(), g_bit_shift));
	g_new_z_l.real(g_temp_sqr_l.real() - g_temp_sqr_l.imag() + g_long_parameter->real()
		+ multiply(g_long_parameter->imag(), s_temp_z2_l.real(), g_bit_shift));
	g_new_z_l.imag(g_temp_z_l.real() + g_temp_z_l.real()
		+ multiply(g_long_parameter->imag(), s_temp_z2_l.imag(), g_bit_shift));
	s_temp_z2_l = g_old_z_l; // set s_temp_z2_l to Y value
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

int phoenix_orbit_fp()
{
	// z(n + 1) = z(n)^2 + p + qy(n),  y(n + 1) = z(n)
	g_temp_z.real(g_old_z.real()*g_old_z.imag());
	g_new_z.real(g_temp_sqr.real() - g_temp_sqr.imag() + g_float_parameter->real() + g_float_parameter->imag()*s_temp2.real());
	g_new_z.imag(g_temp_z.real()   + g_temp_z.real()   + g_float_parameter->imag()*s_temp2.imag());
	s_temp2 = g_old_z; // set s_temp2 to Y value
	return g_externs.BailOutFp();
}

int phoenix_complex_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	// z(n + 1) = z(n)^2 + p + qy(n),  y(n + 1) = z(n)
	g_temp_z_l.real(multiply(g_old_z_l.real(), g_old_z_l.imag(), g_bit_shift));
	g_new_z_l.real(g_temp_sqr_l.real() - g_temp_sqr_l.imag() + g_long_parameter->real()
		+ multiply(g_parameter2_l.real(), s_temp_z2_l.real(), g_bit_shift)
		- multiply(g_parameter2_l.imag(), s_temp_z2_l.imag(), g_bit_shift));
	g_new_z_l.imag(g_temp_z_l.real() + g_temp_z_l.real() + g_long_parameter->imag()
		+ multiply(g_parameter2_l.real(), s_temp_z2_l.imag(), g_bit_shift)
		+ multiply(g_parameter2_l.imag(), s_temp_z2_l.real(), g_bit_shift));
	s_temp_z2_l = g_old_z_l; // set s_temp_z2_l to Y value
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

int phoenix_complex_orbit_fp()
{
	// z(n + 1) = z(n)^2 + p1 + p2*y(n),  y(n + 1) = z(n)
	g_new_z = sqr(g_old_z) + *g_float_parameter + g_parameter2*s_temp2;
	s_temp2 = g_old_z; // set s_temp2 to Y value
	return g_externs.BailOutFp();
}

int phoenix_plus_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	// z(n + 1) = z(n)^(degree-1)*(z(n) + p) + qy(n),  y(n + 1) = z(n)
	int i;
	ComplexL loldplus;
	loldplus = g_old_z_l;
	g_temp_z_l = g_old_z_l;
	for (i = 1; i < g_degree; i++)  // degree >= 2, degree = degree-1 in setup
	{
		LCMPLXmult(g_old_z_l, g_temp_z_l, g_temp_z_l); // = old^(degree-1)
	}
	loldplus.real(loldplus.real() + g_long_parameter->real());
	ComplexL lnewminus;
	LCMPLXmult(g_temp_z_l, loldplus, lnewminus);
	g_new_z_l.real(lnewminus.real() + multiply(g_long_parameter->imag(), s_temp_z2_l.real(), g_bit_shift));
	g_new_z_l.imag(lnewminus.imag() + multiply(g_long_parameter->imag(), s_temp_z2_l.imag(), g_bit_shift));
	s_temp_z2_l = g_old_z_l; // set s_temp_z2_l to Y value
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

int phoenix_plus_orbit_fp()
{
	// z(n + 1) = z(n)^(degree-1)*(z(n) + p) + qy(n),  y(n + 1) = z(n)
	ComplexD oldplus = g_old_z;
	g_temp_z = g_old_z;
	for (int i = 1; i < g_degree; i++)  // degree >= 2, degree = degree-1 in setup
	{
		g_temp_z = g_old_z*g_temp_z; // = old^(degree-1)
	}
	oldplus = oldplus + g_float_parameter->real();
	ComplexD newminus = g_temp_z*oldplus;
	g_new_z = newminus + g_float_parameter->imag()*s_temp2;
	s_temp2 = g_old_z; // set s_temp2 to Y value
	return g_externs.BailOutFp();
}

int phoenix_minus_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	// z(n + 1) = z(n)^(degree-2)*(z(n)^2 + p) + qy(n),  y(n + 1) = z(n)
	int i;
	ComplexL loldsqr;
	LCMPLXmult(g_old_z_l, g_old_z_l, loldsqr);
	g_temp_z_l = g_old_z_l;
	for (i = 1; i < g_degree; i++)  // degree >= 3, degree = degree-2 in setup
	{
		LCMPLXmult(g_old_z_l, g_temp_z_l, g_temp_z_l); // = old^(degree-2)
	}
	loldsqr.real(loldsqr.real() + g_long_parameter->real());
	ComplexL lnewminus;
	LCMPLXmult(g_temp_z_l, loldsqr, lnewminus);
	g_new_z_l.real(lnewminus.real() + multiply(g_long_parameter->imag(), s_temp_z2_l.real(), g_bit_shift));
	g_new_z_l.imag(lnewminus.imag() + multiply(g_long_parameter->imag(), s_temp_z2_l.imag(), g_bit_shift));
	s_temp_z2_l = g_old_z_l; // set s_temp_z2_l to Y value
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

int phoenix_minus_orbit_fp()
{
	// z(n + 1) = z(n)^(degree-2)*(z(n)^2 + p) + qy(n),  y(n + 1) = z(n)
	ComplexD oldsqr = g_old_z*g_old_z;
	g_temp_z = g_old_z;
	for (int i = 1; i < g_degree; i++)  // degree >= 3, degree = degree-2 in setup
	{
		g_temp_z = g_old_z*g_temp_z; // = old^(degree-2)
	}
	oldsqr = oldsqr + g_float_parameter->real();
	ComplexD newminus = g_temp_z*oldsqr;
	g_new_z = newminus + g_float_parameter->imag()*s_temp2;
	s_temp2 = g_old_z; // set s_temp2 to Y value
	return g_externs.BailOutFp();
}

int phoenix_complex_plus_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	// z(n + 1) = z(n)^(degree-1)*(z(n) + p) + qy(n),  y(n + 1) = z(n)
	int i;
	ComplexL loldplus;
	loldplus = g_old_z_l;
	g_temp_z_l = g_old_z_l;
	for (i = 1; i < g_degree; i++)  // degree >= 2, degree = degree-1 in setup
	{
		LCMPLXmult(g_old_z_l, g_temp_z_l, g_temp_z_l); // = old^(degree-1)
	}
	loldplus.real(loldplus.real() + g_long_parameter->real());
	loldplus.imag(loldplus.imag() + g_long_parameter->imag());
	ComplexL lnewminus;
	LCMPLXmult(g_temp_z_l, loldplus, lnewminus);
	LCMPLXmult(g_parameter2_l, s_temp_z2_l, g_temp_z_l);
	g_new_z_l.real(lnewminus.real() + g_temp_z_l.real());
	g_new_z_l.imag(lnewminus.imag() + g_temp_z_l.imag());
	s_temp_z2_l = g_old_z_l; // set s_temp_z2_l to Y value
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

int phoenix_complex_plus_orbit_fp()
{
	// z(n + 1) = z(n)^(degree-1)*(z(n) + p) + qy(n),  y(n + 1) = z(n)
	int i;
	ComplexD oldplus;
	ComplexD newminus;
	oldplus = g_old_z;
	g_temp_z = g_old_z;
	for (i = 1; i < g_degree; i++)  // degree >= 2, degree = degree-1 in setup
	{
		g_temp_z = g_old_z*g_temp_z; // = old^(degree-1)
	}
	oldplus.real(oldplus.real() + g_float_parameter->real());
	oldplus.imag(oldplus.imag() + g_float_parameter->imag());
	newminus = g_temp_z*oldplus;
	g_new_z = newminus + g_parameter2*s_temp2;
	s_temp2 = g_old_z; // set s_temp2 to Y value
	return g_externs.BailOutFp();
}

int phoenix_complex_minus_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	// z(n + 1) = z(n)^(degree-2)*(z(n)^2 + p) + qy(n),  y(n + 1) = z(n)
	int i;
	ComplexL loldsqr;
	LCMPLXmult(g_old_z_l, g_old_z_l, loldsqr);
	g_temp_z_l = g_old_z_l;
	for (i = 1; i < g_degree; i++)  // degree >= 3, degree = degree-2 in setup
	{
		LCMPLXmult(g_old_z_l, g_temp_z_l, g_temp_z_l); // = old^(degree-2)
	}
	loldsqr.real(loldsqr.real() + g_long_parameter->real());
	loldsqr.imag(loldsqr.imag() + g_long_parameter->imag());
	ComplexL lnewminus;
	LCMPLXmult(g_temp_z_l, loldsqr, lnewminus);
	LCMPLXmult(g_parameter2_l, s_temp_z2_l, g_temp_z_l);
	g_new_z_l.real(lnewminus.real() + g_temp_z_l.real());
	g_new_z_l.imag(lnewminus.imag() + g_temp_z_l.imag());
	s_temp_z2_l = g_old_z_l; // set s_temp_z2_l to Y value
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

int phoenix_complex_minus_orbit_fp()
{
	// z(n + 1) = z(n)^(degree-2)*(z(n)^2 + p) + qy(n),  y(n + 1) = z(n)
	int i;
	ComplexD oldsqr;
	ComplexD newminus;
	oldsqr = g_old_z*g_old_z;
	g_temp_z = g_old_z;
	for (i = 1; i < g_degree; i++)  // degree >= 3, degree = degree-2 in setup
	{
		g_temp_z = g_old_z*g_temp_z; // = old^(degree-2)
	}
	oldsqr.real(oldsqr.real() + g_float_parameter->real());
	oldsqr.imag(oldsqr.imag() + g_float_parameter->imag());
	newminus = g_temp_z*oldsqr;
	g_new_z = newminus + g_parameter2*s_temp2;
	s_temp2 = g_old_z; // set s_temp2 to Y value
	return g_externs.BailOutFp();
}

int scott_trig_plus_trig_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	// z = trig0(z) + trig1(z)
	LCMPLXtrig0(g_old_z_l, g_temp_z_l);
	LCMPLXtrig1(g_old_z_l, g_old_z_l);
	LCMPLXadd(g_temp_z_l, g_old_z_l, g_new_z_l);
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

int scott_trig_plus_trig_orbit_fp()
{
	// z = trig0(z) + trig1(z)
	g_temp_z = CMPLXtrig0(g_old_z);
	s_temp2 = CMPLXtrig1(g_old_z);
	g_new_z = g_temp_z + s_temp2;
	return g_externs.BailOutFp();
}

int skinner_trig_sub_trig_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	// z = trig(0, z)-trig1(z)
	LCMPLXtrig0(g_old_z_l, g_temp_z_l);
	LCMPLXtrig1(g_old_z_l, s_temp_z2_l);
	LCMPLXsub(g_temp_z_l, s_temp_z2_l, g_new_z_l);
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

int skinner_trig_sub_trig_orbit_fp()
{
	// z = trig0(z)-trig1(z)
	g_temp_z = CMPLXtrig0(g_old_z);
	s_temp2 = CMPLXtrig1(g_old_z);
	g_new_z = g_temp_z - s_temp2;
	return g_externs.BailOutFp();
}

int trig_trig_orbit_fp()
{
	// z = trig0(z)*trig1(z)
	g_temp_z = CMPLXtrig0(g_old_z);
	g_old_z = CMPLXtrig1(g_old_z);
	g_new_z = g_temp_z*g_old_z;
	return g_externs.BailOutFp();
}

#if !defined(NO_FIXED_POINT_MATH)
// call float version of fractal if integer math overflow
int try_float_fractal(int (*fpFractal)())
{
	g_overflow = false;
	// g_old_z_l had better not be changed!
	g_old_z = ComplexFudgeToDouble(g_old_z_l);
	g_temp_sqr.real(sqr(g_old_z.real()));
	g_temp_sqr.imag(sqr(g_old_z.imag()));
	fpFractal();
	g_new_z_l = ComplexDoubleToFudge(g_new_z);
	return 0;
}
#endif

int trig_trig_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	ComplexL temp;
	// z = trig0(z)*trig1(z)
	LCMPLXtrig0(g_old_z_l, g_temp_z_l);
	LCMPLXtrig1(g_old_z_l, temp);
	LCMPLXmult(g_temp_z_l, temp, g_new_z_l);
	if (g_overflow)
	{
		try_float_fractal(trig_trig_orbit_fp);
	}
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

//
// Next six orbit functions are one type - extra functions are
// special cases written for speed.
//
int trig_plus_sqr_orbit() // generalization of Scott and Skinner types
{
#if !defined(NO_FIXED_POINT_MATH)
	// { z = pixel: z = (p1, p2)*trig(z) + (p3, p4)*sqr(z), |z|<BAILOUT }
	LCMPLXtrig0(g_old_z_l, g_temp_z_l);     // g_temp_z_l = trig(g_old_z_l)
	LCMPLXmult(g_parameter_l, g_temp_z_l, g_new_z_l); // g_new_z_l = g_parameter_l*trig(g_old_z_l)
	LCMPLXsqr_old(g_temp_z_l);         // g_temp_z_l = sqr(g_old_z_l)
	LCMPLXmult(g_parameter2_l, g_temp_z_l, g_temp_z_l); // g_temp_z_l = g_parameter2_l*sqr(g_old_z_l)
	LCMPLXadd(g_new_z_l, g_temp_z_l, g_new_z_l);   // g_new_z_l = g_parameter_l*trig(g_old_z_l) + g_parameter2_l*sqr(g_old_z_l)
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

int trig_plus_sqr_orbit_fp() // generalization of Scott and Skinner types
{
	// { z = pixel: z = (p1, p2)*trig(z) + (p3, p4)*sqr(z), |z|<BAILOUT }
	g_temp_z = CMPLXtrig0(g_old_z);		// tmp = trig(old)
	g_new_z = g_parameter*g_temp_z;		// new = g_parameter*trig(old)

	CMPLXsqr_old(g_temp_z);				// tmp = sqr(old)
	s_temp2 = g_parameter2*g_temp_z;	// tmp = g_parameter2*sqr(old)
	g_new_z = g_new_z + s_temp2;		// new = g_parameter*trig(old) + g_parameter2*sqr(old)
	return g_externs.BailOutFp();
}

int scott_trig_plus_sqr_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	// { z = pixel: z = trig(z) + sqr(z), |z|<BAILOUT }
	LCMPLXtrig0(g_old_z_l, g_new_z_l);    // g_new_z_l = trig(g_old_z_l)
	LCMPLXsqr_old(g_temp_z_l);        // g_old_z_l = sqr(g_old_z_l)
	LCMPLXadd(g_temp_z_l, g_new_z_l, g_new_z_l);  // g_new_z_l = trig(g_old_z_l) + sqr(g_old_z_l)
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

int scott_trig_plus_sqr_orbit_fp() // float version
{
	// { z = pixel: z = sin(z) + sqr(z), |z|<BAILOUT }
	g_new_z = CMPLXtrig0(g_old_z);		// new = trig(old)
	CMPLXsqr_old(g_temp_z);				// tmp = sqr(old)
	g_new_z = g_new_z + g_temp_z;		// new = trig(old) + sqr(old)
	return g_externs.BailOutFp();
}

int skinner_trig_sub_sqr_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	// { z = pixel: z = sin(z)-sqr(z), |z|<BAILOUT }
	LCMPLXtrig0(g_old_z_l, g_new_z_l);    // g_new_z_l = trig(g_old_z_l)
	LCMPLXsqr_old(g_temp_z_l);        // g_old_z_l = sqr(g_old_z_l)
	LCMPLXsub(g_new_z_l, g_temp_z_l, g_new_z_l);  // g_new_z_l = trig(g_old_z_l)-sqr(g_old_z_l)
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

int skinner_trig_sub_sqr_orbit_fp()
{
	// { z = pixel: z = sin(z)-sqr(z), |z|<BAILOUT }
	g_new_z = CMPLXtrig0(g_old_z);       // new = trig(old)
	CMPLXsqr_old(g_temp_z);				// old = sqr(old)
	g_new_z = g_new_z - g_temp_z;		// new = trig(old)-sqr(old)
	return g_externs.BailOutFp();
}

int trig_z_squared_orbit_fp()
{
	// { z = pixel: z = trig(z*z), |z|<TEST }
	CMPLXsqr_old(g_temp_z);
	g_new_z = CMPLXtrig0(g_temp_z);
	return g_externs.BailOutFp();
}

int trig_z_squared_orbit() // this doesn't work very well
{
#if !defined(NO_FIXED_POINT_MATH)
	// { z = pixel: z = trig(z*z), |z|<TEST }
	long l16triglim_2 = 8L << 15;
	LCMPLXsqr_old(g_temp_z_l);
	if (std::abs(g_temp_z_l.real()) > l16triglim_2 || std::abs(g_temp_z_l.imag()) > l16triglim_2)
	{
		g_overflow = true;
	}

	if (g_overflow)
	{
		try_float_fractal(trig_z_squared_orbit_fp);
	}
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

int sqr_trig_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	// { z = pixel: z = sqr(trig(z)), |z|<TEST}
	LCMPLXtrig0(g_old_z_l, g_temp_z_l);
	LCMPLXsqr(g_temp_z_l, g_new_z_l);
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

int sqr_trig_orbit_fp()
{
	// SZSB(XYAXIS) { z = pixel, TEST = (p1 + 3): z = sin(z)*sin(z), |z|<TEST}
	g_new_z = sqr(CMPLXtrig0(g_old_z));
	return g_externs.BailOutFp();
}

int magnet1_orbit_fp()    // Z = ((Z**2 + C - 1)/(2Z + C - 2))**2
{                   // In "Beauty of Fractals", code by Kev Allen.
	ComplexD const top = sqr(g_old_z) + *g_float_parameter - 1.0;			// top = Z**2 + C-1
	ComplexD const bot = g_old_z + g_old_z + *g_float_parameter - 2.0;	// bot = 2*Z + C-2

	double const div = norm(bot);                // tmp = top/bot
	if (div < FLT_MIN)
	{
		return 1;
	}
	g_new_z = sqr(top/bot);						// Z = tmp**2
	return g_externs.BailOutFp();
}

// Z = ((Z**3 + 3(C-1)Z + (C-1)(C-2))/
// (3Z**2 + 3(C-2)Z + (C-1)(C-2) + 1))**2
int magnet2_orbit_fp()
{
	// In "Beauty of Fractals", code by Kev Allen.
	ComplexD top;
	ComplexD bot;
	double div;

	top.real(g_old_z.real()*(g_temp_sqr.real()-g_temp_sqr.imag()-g_temp_sqr.imag()-g_temp_sqr.imag() + s_3_c_minus_1.real())
			- g_old_z.imag()*s_3_c_minus_1.imag() + s_c_minus_1_c_minus_2.real());
	top.imag(g_old_z.imag()*(g_temp_sqr.real() + g_temp_sqr.real() + g_temp_sqr.real()-g_temp_sqr.imag() + s_3_c_minus_1.real())
			+ g_old_z.real()*s_3_c_minus_1.imag() + s_c_minus_1_c_minus_2.imag());

	bot.real(g_temp_sqr.real() - g_temp_sqr.imag());
	bot.real(bot.real() + bot.real() + bot.real()
			+ g_old_z.real()*s_3_c_minus_2.real() - g_old_z.imag()*s_3_c_minus_2.imag()
			+ s_c_minus_1_c_minus_2.real() + 1.0);
	bot.imag(g_old_z.real()*g_old_z.imag());
	bot.imag(bot.imag() + bot.imag());
	bot.imag(bot.imag() + bot.imag() + bot.imag()
			+ g_old_z.real()*s_3_c_minus_2.imag() + g_old_z.imag()*s_3_c_minus_2.real()
			+ s_c_minus_1_c_minus_2.imag());

	div = norm(bot);                // tmp = top/bot
	if (div < FLT_MIN)
	{
		return 1;
	}
	g_new_z = sqr(top/bot);

	return g_externs.BailOutFp();
}

int lambda_trig_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	if (std::abs(g_old_z_l.real()) >= g_rq_limit2_l
		|| std::abs(g_old_z_l.imag()) >= g_rq_limit2_l)
	{
		return 1;
	}
	LCMPLXtrig0(g_old_z_l, g_temp_z_l);           // g_temp_z_l = trig(g_old_z_l)
	LCMPLXmult(*g_long_parameter, g_temp_z_l, g_new_z_l);   // g_new_z_l = g_long_parameter*trig(g_old_z_l)
	g_old_z_l = g_new_z_l;
	return 0;
#else
	return 0;
#endif
}

int lambda_trig_orbit_fp()
{
	if (std::abs(g_old_z.real()) >= g_rq_limit2
		|| std::abs(g_old_z.imag()) >= g_rq_limit2)
	{
		return 1;
	}
	g_temp_z = CMPLXtrig0(g_old_z);              // tmp = trig(old)
	g_new_z = *g_float_parameter*g_temp_z;		// new = g_long_parameter*trig(old)
	g_old_z = g_new_z;
	return 0;
}

// bailouts are different for different trig functions
int lambda_trig1_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	if (std::abs(g_old_z_l.imag()) >= g_rq_limit2_l)
	{
		return 1;
	}
	LCMPLXtrig0(g_old_z_l, g_temp_z_l);           // g_temp_z_l = trig(g_old_z_l)
	LCMPLXmult(*g_long_parameter, g_temp_z_l, g_new_z_l);   // g_new_z_l = g_long_parameter*trig(g_old_z_l)
	g_old_z_l = g_new_z_l;
	return 0;
#else
	return 0;
#endif
}

int lambda_trig1_orbit_fp()
{
	if (std::abs(g_old_z.imag()) >= g_rq_limit2)
	{
		return 1;
	}
	g_temp_z = CMPLXtrig0(g_old_z);              // tmp = trig(old)
	g_new_z = *g_float_parameter*g_temp_z;		// new = g_long_parameter*trig(old)
	g_old_z = g_new_z;
	return 0;
}

int lambda_trig2_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	if (std::abs(g_old_z_l.real()) >= g_rq_limit2_l)
	{
		return 1;
	}
	LCMPLXtrig0(g_old_z_l, g_temp_z_l);           // g_temp_z_l = trig(g_old_z_l)
	LCMPLXmult(*g_long_parameter, g_temp_z_l, g_new_z_l);   // g_new_z_l = g_long_parameter*trig(g_old_z_l)
	g_old_z_l = g_new_z_l;
	return 0;
#else
	return 0;
#endif
}

int lambda_trig2_orbit_fp()
{
#if !defined(NO_FIXED_POINT_MATH)
	if (std::abs(g_old_z.real()) >= g_rq_limit2)
	{
		return 1;
	}
	g_temp_z = CMPLXtrig0(g_old_z);              // tmp = trig(old)
	g_new_z = *g_float_parameter*g_temp_z;		// new = g_long_parameter*trig(old)
	g_old_z = g_new_z;
	return 0;
#else
	return 0;
#endif
}

int man_o_war_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	// From Art Matrix via Lee Skinner
	g_new_z_l.real(g_temp_sqr_l.real() - g_temp_sqr_l.imag() + g_temp_z_l.real() + g_long_parameter->real());
	g_new_z_l.imag(multiply(g_old_z_l.real(), g_old_z_l.imag(), g_bit_shift_minus_1) + g_temp_z_l.imag() + g_long_parameter->imag());
	g_temp_z_l = g_old_z_l;
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

int man_o_war_orbit_fp()
{
	// From Art Matrix via Lee Skinner
	g_new_z = sqr(g_old_z) + g_temp_z + *g_float_parameter;
	g_temp_z = g_old_z;
	return g_externs.BailOutFp();
}

//	MarksMandelPwr (XAXIS)
//	{
//		z = pixel, c = z ^ (z - 1):
//			z = c*sqr(z) + pixel,
//		|z| <= 4
//	}
//
int marks_mandel_power_orbit_fp()
{
	g_new_z = g_temp_z*CMPLXtrig0(g_old_z) + *g_float_parameter;
	return g_externs.BailOutFp();
}

int marks_mandel_power_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	LCMPLXtrig0(g_old_z_l, g_new_z_l);
	LCMPLXmult(g_temp_z_l, g_new_z_l, g_new_z_l);
	g_new_z_l = g_new_z_l + *g_long_parameter;
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

//
//	I was coding Marksmandelpower and failed to use some temporary
//	variables. The result was nice, and since my name is not on any fractal,
//	I thought I would immortalize myself with this error!
//	Tim Wegner
//
int tims_error_orbit_fp()
{
	g_new_z = CMPLXtrig0(g_old_z)*g_temp_z + *g_float_parameter;
	return g_externs.BailOutFp();
}

int tims_error_orbit()
{
#if !defined(NO_FIXED_POINT_MATH)
	LCMPLXtrig0(g_old_z_l, g_new_z_l);
	g_new_z_l.real(multiply(g_new_z_l.real(), g_temp_z_l.real(), g_bit_shift)-multiply(g_new_z_l.imag(), g_temp_z_l.imag(), g_bit_shift));
	g_new_z_l.imag(multiply(g_new_z_l.real(), g_temp_z_l.imag(), g_bit_shift)-multiply(g_new_z_l.imag(), g_temp_z_l.real(), g_bit_shift));
	g_new_z_l.real(g_new_z_l.real() + g_long_parameter->real());
	g_new_z_l.imag(g_new_z_l.imag() + g_long_parameter->imag());
	return g_externs.BailOutL();
#else
	return 0;
#endif
}

int circle_orbit_fp()
{
	long i;
	i = long(g_parameters[P1_REAL]*(g_temp_sqr.real() + g_temp_sqr.imag()));
	g_color_iter = i % g_colors;
	return 1;
}

/*
int circle_orbit()
{
	long i;
	i = multiply(g_parameter_l.real(), (g_temp_sqr_l.real() + g_temp_sqr_l.imag()), g_bit_shift);
	i >>= g_bit_shift;
	g_color_iter = i % g_colors);
	return 1;
}
*/

// --------------------------------------------------------------------
// Initialization (once per pixel) routines
// --------------------------------------------------------------------

// transform points with reciprocal function
void invert_z(ComplexD *z)
{
	// Normalize values to center of circle
	ComplexD const center = MakeComplexT(g_f_x_center, g_f_y_center);
	ComplexD temp = g_externs.DPixel() - center;
	g_temp_sqr.real(sqr(temp.real()) + sqr(temp.imag()));  // Get old radius
	g_temp_sqr.real((std::abs(g_temp_sqr.real()) > FLT_MIN) ? (g_f_radius/g_temp_sqr.real()) : FLT_MAX);
	*z = g_temp_sqr.real()*temp + center;	// Perform inversion, Renormalize
}

int julia_per_pixel_l()
{
#if !defined(NO_FIXED_POINT_MATH)
	// integer julia types
	// lambda
	// barnsleyj1
	// barnsleyj2
	// sierpinski
	if (g_invert)
	{
		// invert
		invert_z(&g_old_z);

		// watch out for overflow
		if (norm(g_old_z) >= 127)
		{
			g_old_z.real(8);  // value to bail out in one iteration
			g_old_z.imag(8);
		}

		// convert to fudged longs
		g_old_z_l = ComplexDoubleToFudge(g_old_z);
	}
	else
	{
		g_old_z_l = g_externs.LPixel();
	}
	return 0;
#else
	return 0;
#endif
}

int richard8_per_pixel()
{
#if !defined(NO_FIXED_POINT_MATH)
	mandelbrot_per_pixel_l();
	LCMPLXtrig1(*g_long_parameter, g_temp_z_l);
	LCMPLXmult(g_temp_z_l, g_parameter2_l, g_temp_z_l);
	return 1;
#else
	return 0;
#endif
}

int mandelbrot_per_pixel_l()
{
#if !defined(NO_FIXED_POINT_MATH)
	// integer mandel types
	// barnsleym1
	// barnsleym2
	g_initial_z_l = g_externs.LPixel();

	if (g_invert)
	{
		// invert
		invert_z(&g_initial_z);

		// watch out for overflow
		if (norm(g_initial_z) >= 127)
		{
			g_initial_z.real(8);  // value to bail out in one iteration
			g_initial_z.imag(8);
		}

		// convert to fudged longs
		g_initial_z_l = ComplexDoubleToFudge(g_initial_z);
	}

	g_old_z_l = (g_externs.UseInitialOrbitZ() == INITIALZ_ORBIT) ? g_initial_orbit_z_l : g_initial_z_l;

	g_old_z_l.real(g_old_z_l.real() + g_parameter_l.real());    // initial pertubation of parameters set
	g_old_z_l.imag(g_old_z_l.imag() + g_parameter_l.imag());
	return 1; // 1st iteration has been done
#else
	return 0;
#endif
}

int julia_per_pixel()
{
	// julia

	if (g_invert)
	{
		// invert
		invert_z(&g_old_z);

		// watch out for overflow
		if (g_bit_shift <= 24)
		{
			if (norm(g_old_z) >= 127)
			{
				g_old_z.real(8);  // value to bail out in one iteration
				g_old_z.imag(8);
			}
		}
		else if (norm(g_old_z) >= 4.0)
		{
			g_old_z.real(2);  // value to bail out in one iteration
			g_old_z.imag(2);
		}

		// convert to fudged longs
		g_old_z_l = ComplexDoubleToFudge(g_old_z);
	}
	else
	{
		g_old_z_l = g_externs.LPixel();
	}

	g_temp_sqr_l.real(multiply(g_old_z_l.real(), g_old_z_l.real(), g_bit_shift));
	g_temp_sqr_l.imag(multiply(g_old_z_l.imag(), g_old_z_l.imag(), g_bit_shift));
	g_temp_z_l = g_old_z_l;
	return 0;
}

int marks_mandelbrot_power_per_pixel()
{
#if !defined(NO_FIXED_POINT_MATH)
	mandelbrot_per_pixel();
	g_temp_z_l = g_old_z_l;
	g_temp_z_l.real(g_temp_z_l.real() - g_externs.Fudge());
	LCMPLXpwr(g_old_z_l, g_temp_z_l, g_temp_z_l);
	return 1;
#else
	return 0;
#endif
}

int mandelbrot_per_pixel()
{
	if (g_invert)
	{
		invert_z(&g_initial_z);

		// watch out for overflow
		if (g_bit_shift <= 24)
		{
			if (norm(g_initial_z) >= 127)
			{
				g_initial_z.real(8);  // value to bail out in one iteration
				g_initial_z.imag(8);
			}
		}
		else if (norm(g_initial_z) >= 4)
		{
			g_initial_z.real(2);  // value to bail out in one iteration
			g_initial_z.imag(2);
		}

		// convert to fudged longs
		g_initial_z_l = ComplexDoubleToFudge(g_initial_z);
	}
	else
	{
		g_initial_z_l = g_externs.LPixel();
	}
	switch (g_fractal_type)
	{
	case FRACTYPE_MANDELBROT_LAMBDA:              // Critical Value 0.5 + 0.0i
		g_old_z_l.real(g_externs.Fudge() >> 1);
		g_old_z_l.imag(0);
		break;
	default:
		g_old_z_l = g_initial_z_l;
		break;
	}

	// alter g_initial_z value
	if (g_externs.UseInitialOrbitZ() == INITIALZ_ORBIT)
	{
		g_old_z_l = g_initial_orbit_z_l;
	}
	else if (g_externs.UseInitialOrbitZ() == INITIALZ_PIXEL)
	{
		g_old_z_l = g_initial_z_l;
	}

	if (inside_coloring_beauty_of_fractals_allowed())
	{
		// kludge to match "Beauty of Fractals" picture since we start
		// Mandelbrot iteration with g_initial_z rather than 0
		g_old_z_l = g_parameter_l;				// initial pertubation of parameters set
		g_color_iter = -1;
	}
	else
	{
		g_old_z_l.real(g_old_z_l.real() + g_parameter_l.real()); // initial pertubation of parameters set
		g_old_z_l.imag(g_old_z_l.imag() + g_parameter_l.imag());
	}
	g_temp_z_l = g_initial_z_l; // for spider
	g_temp_sqr_l.real(multiply(g_old_z_l.real(), g_old_z_l.real(), g_bit_shift));
	g_temp_sqr_l.imag(multiply(g_old_z_l.imag(), g_old_z_l.imag(), g_bit_shift));
	return 1; // 1st iteration has been done
}

int marks_mandelbrot_per_pixel()
{
#if !defined(NO_FIXED_POINT_MATH)
	// marksmandel
	if (g_invert)
	{
		invert_z(&g_initial_z);

		// watch out for overflow
		if (norm(g_initial_z) >= 127)
		{
			g_initial_z.real(8);  // value to bail out in one iteration
			g_initial_z.imag(8);
		}

		// convert to fudged longs
		g_initial_z_l = ComplexDoubleToFudge(g_initial_z);
	}
	else
	{
		g_initial_z_l = g_externs.LPixel();
	}

	g_old_z_l = (g_externs.UseInitialOrbitZ() == INITIALZ_ORBIT) ? g_initial_orbit_z_l : g_initial_z_l;

	g_old_z_l.real(g_old_z_l.real() + g_parameter_l.real());    // initial pertubation of parameters set
	g_old_z_l.imag(g_old_z_l.imag() + g_parameter_l.imag());

	if (g_c_exp > 3)
	{
		complex_power_l(&g_old_z_l, g_c_exp-1, &g_coefficient_l, g_bit_shift);
	}
	else if (g_c_exp == 3)
	{
		g_coefficient_l.real(multiply(g_old_z_l.real(), g_old_z_l.real(), g_bit_shift)
			- multiply(g_old_z_l.imag(), g_old_z_l.imag(), g_bit_shift));
		g_coefficient_l.imag(multiply(g_old_z_l.real(), g_old_z_l.imag(), g_bit_shift_minus_1));
	}
	else if (g_c_exp == 2)
	{
		g_coefficient_l = g_old_z_l;
	}
	else if (g_c_exp < 2)
	{
		g_coefficient_l.real(1L << g_bit_shift);
		g_coefficient_l.imag(0L);
	}

	g_temp_sqr_l.real(multiply(g_old_z_l.real(), g_old_z_l.real(), g_bit_shift));
	g_temp_sqr_l.imag(multiply(g_old_z_l.imag(), g_old_z_l.imag(), g_bit_shift));
#endif
	return 1; // 1st iteration has been done
}

int marks_mandelbrot_per_pixel_fp()
{
	// marksmandel

	if (g_invert)
	{
		invert_z(&g_initial_z);
	}
	else
	{
		g_initial_z = g_externs.DPixel();
	}

	g_old_z = (g_externs.UseInitialOrbitZ() == INITIALZ_ORBIT) ? g_initial_orbit_z : g_initial_z;
	g_old_z = g_old_z + g_parameter;			// initial pertubation of parameters set

	g_temp_sqr.real(sqr(g_old_z.real()));
	g_temp_sqr.imag(sqr(g_old_z.imag()));

	if (g_c_exp > 3)
	{
		pow(g_old_z, g_c_exp-1, g_coefficient);
	}
	else if (g_c_exp == 3)
	{
		g_coefficient = ComplexStdFromT(sqr(g_old_z));
	}
	else if (g_c_exp == 2)
	{
		g_coefficient = ComplexStdFromT(g_old_z);
	}
	else if (g_c_exp < 2)
	{
		g_coefficient = StdComplexD(1.0, 0.0);
	}

	return 1; // 1st iteration has been done
}

int marks_mandelbrot_power_per_pixel_fp()
{
	mandelbrot_per_pixel_fp();
	g_temp_z = g_old_z;
	g_temp_z.real(g_temp_z.real() - 1.0);
	g_temp_z = pow(g_old_z, g_temp_z);
	return 1;
}

int mandelbrot_per_pixel_fp()
{
	// floating point mandelbrot
	// mandelfp

	if (g_invert)
	{
		invert_z(&g_initial_z);
	}
	else
	{
		g_initial_z = g_externs.DPixel();
	}
	switch (g_fractal_type)
	{
	case FRACTYPE_MAGNET_2M:
		magnet2_precalculate_fp();
	case FRACTYPE_MAGNET_1M:           // Crit Val Zero both, but neither
		g_old_z.real(g_old_z.imag(0.0)); // is of the form f(Z, C) = Z*g(Z) + C
		break;
	case FRACTYPE_MANDELBROT_LAMBDA_FP:            // Critical Value 0.5 + 0.0i
		g_old_z.real(0.5);
		g_old_z.imag(0.0);
		break;
	default:
		g_old_z = g_initial_z;
		break;
	}

	// alter g_initial_z value
	if (g_externs.UseInitialOrbitZ() == INITIALZ_ORBIT)
	{
		g_old_z = g_initial_orbit_z;
	}
	else if (g_externs.UseInitialOrbitZ() == INITIALZ_PIXEL)
	{
		g_old_z = g_initial_z;
	}

	if (inside_coloring_beauty_of_fractals_allowed())
	{
		// kludge to match "Beauty of Fractals" picture since we start
		// Mandelbrot iteration with g_initial_z rather than 0
		g_old_z = g_parameter;				// initial pertubation of parameters set
		g_color_iter = -1;
	}
	else
	{
		g_old_z = g_old_z + g_parameter;
	}
	g_temp_z = g_initial_z; // for spider
	g_temp_sqr.real(sqr(g_old_z.real()));  // precalculated value for regular Mandelbrot
	g_temp_sqr.imag(sqr(g_old_z.imag()));
	return 1; // 1st iteration has been done
}

int julia_per_pixel_fp()
{
	// floating point julia
	// juliafp
	if (g_invert)
	{
		invert_z(&g_old_z);
	}
	else
	{
		g_old_z = g_externs.DPixel();
	}
	g_temp_sqr.real(sqr(g_old_z.real()));  // precalculated value for regular Julia
	g_temp_sqr.imag(sqr(g_old_z.imag()));
	g_temp_z = g_old_z;
	return 0;
}

int julia_per_pixel_mpc()
{
	return 0;
}

int other_richard8_per_pixel_fp()
{
	other_mandelbrot_per_pixel_fp();
	g_temp_z = CMPLXtrig1(*g_float_parameter);
	g_temp_z = g_temp_z*g_parameter2;
	return 1;
}

int other_mandelbrot_per_pixel_fp()
{
	if (g_invert)
	{
		invert_z(&g_initial_z);
	}
	else
	{
		g_initial_z = g_externs.DPixel();
	}

	g_old_z = (g_externs.UseInitialOrbitZ() == INITIALZ_ORBIT) ? g_initial_orbit_z : g_initial_z;
	g_old_z = g_old_z + g_parameter;		// initial pertubation of parameters set

	return 1; // 1st iteration has been done
}

int other_julia_per_pixel_fp()
{
	if (g_invert)
	{
		invert_z(&g_old_z);
	}
	else
	{
		g_old_z = g_externs.DPixel();
	}
	return 0;
}

int marks_complex_mandelbrot_per_pixel()
{
	if (g_invert)
	{
		invert_z(&g_initial_z);
	}
	else
	{
		g_initial_z = g_externs.DPixel();
	}
	g_old_z = g_initial_z + g_parameter;	// initial pertubation of parameters set
	g_temp_sqr.real(sqr(g_old_z.real()));  // precalculated value
	g_temp_sqr.imag(sqr(g_old_z.imag()));
	g_coefficient = ComplexStdFromT(pow(g_initial_z, g_power));
	return 1;
}

int phoenix_per_pixel()
{
#if !defined(NO_FIXED_POINT_MATH)
	if (g_invert)
	{
		// invert
		invert_z(&g_old_z);

		// watch out for overflow
		if (norm(g_old_z) >= 127)
		{
			g_old_z.real(8);  // value to bail out in one iteration
			g_old_z.imag(8);
		}

		// convert to fudged longs
		g_old_z_l = ComplexDoubleToFudge(g_old_z);
	}
	else
	{
		g_old_z_l = g_externs.LPixel();
	}
	g_temp_sqr_l.real(multiply(g_old_z_l.real(), g_old_z_l.real(), g_bit_shift));
	g_temp_sqr_l.imag(multiply(g_old_z_l.imag(), g_old_z_l.imag(), g_bit_shift));
	s_temp_z2_l.real(0L); // use s_temp_z2_l as the complex Y value
	s_temp_z2_l.imag(0L);
	return 0;
#else
	return 0;
#endif
}

int phoenix_per_pixel_fp()
{
	if (g_invert)
	{
		invert_z(&g_old_z);
	}
	else
	{
		g_old_z = g_externs.DPixel();
	}
	g_temp_sqr.real(sqr(g_old_z.real()));  // precalculated value
	g_temp_sqr.imag(sqr(g_old_z.imag()));
	s_temp2 = MakeComplexT(0.0);	// use s_temp2 as the complex Y value
	return 0;
}

int mandelbrot_phoenix_per_pixel()
{
#if !defined(NO_FIXED_POINT_MATH)
	g_initial_z_l = g_externs.LPixel();

	if (g_invert)
	{
		// invert
		invert_z(&g_initial_z);

		// watch out for overflow
		if (norm(g_initial_z) >= 127)
		{
			g_initial_z.real(8);  // value to bail out in one iteration
			g_initial_z.imag(8);
		}

		// convert to fudged longs
		g_initial_z_l = ComplexDoubleToFudge(g_initial_z);
	}

	g_old_z_l = (g_externs.UseInitialOrbitZ() == INITIALZ_ORBIT) ? g_initial_orbit_z_l : g_initial_z_l;

	g_old_z_l.real(g_old_z_l.real() + g_parameter_l.real());    // initial pertubation of parameters set
	g_old_z_l.imag(g_old_z_l.imag() + g_parameter_l.imag());
	g_temp_sqr_l.real(multiply(g_old_z_l.real(), g_old_z_l.real(), g_bit_shift));
	g_temp_sqr_l.imag(multiply(g_old_z_l.imag(), g_old_z_l.imag(), g_bit_shift));
	s_temp_z2_l.real(0L);
	s_temp_z2_l.imag(0L);
	return 1; // 1st iteration has been done
#else
	return 0;
#endif
}

int mandelbrot_phoenix_per_pixel_fp()
{
	if (g_invert)
	{
		invert_z(&g_initial_z);
	}
	else
	{
		g_initial_z = g_externs.DPixel();
	}

	g_old_z = (g_externs.UseInitialOrbitZ() == INITIALZ_ORBIT) ? g_initial_orbit_z : g_initial_z;
	g_old_z = g_old_z + g_parameter;		// initial pertubation of parameters set
	g_temp_sqr.real(sqr(g_old_z.real()));	// precalculated value
	g_temp_sqr.imag(sqr(g_old_z.imag()));
	s_temp2 = MakeComplexT(0.0);
	return 1; // 1st iteration has been done
}

int hyper_complex_orbit_fp()
{
	HyperComplexD hold;
	hold.real(g_old_z.real());
	hold.imag(g_old_z.imag());
	hold.z(g_float_parameter->real());
	hold.t(g_float_parameter->imag());

	HyperComplexD hnew;
	HComplexTrig0(&hold, &hnew);

	hnew.real(hnew.real() + g_c_quaternion.real());
	hnew.imag(hnew.imag() + g_c_quaternion.R_component_2());
	hnew.z(hnew.z() + g_c_quaternion.R_component_3());
	hnew.t(hnew.t() + g_c_quaternion.R_component_4());

	g_old_z.real(hnew.real());
	g_old_z.imag(hnew.imag());
	g_new_z = g_old_z;
	g_float_parameter->real(hnew.z());
	g_float_parameter->imag(hnew.t());

	// Check bailout
	g_magnitude = norm(g_old_z) + norm(*g_float_parameter);
	if (g_magnitude > g_rq_limit)
	{
		return 1;
	}
	return 0;
}

// Beauty of Fractals pp. 125 - 127
int volterra_lotka_orbit_fp()
{
	double const half = g_parameters[P1_REAL]/2.0;
	double const xy = g_old_z.real()*g_old_z.imag();
	double const u = g_old_z.real() - xy;
	double const w = -g_old_z.imag() + xy;
	double const a = g_old_z.real() + g_parameters[P1_IMAG]*u;
	double const b = g_old_z.imag() + g_parameters[P1_IMAG]*w;
	double const ab = a*b;
	g_new_z = g_old_z + half*MakeComplexT(u + a - ab, w - b + ab);
	return g_externs.BailOutFp();
}

// Science of Fractal Images pp. 185, 187
int escher_orbit_fp()
{
	g_new_z.real(g_temp_sqr.real() - g_temp_sqr.imag()); // standard Julia with C == (0.0, 0.0i)
	g_new_z.imag(2.0*g_old_z.real()*g_old_z.imag());
	ComplexD oldtest;
	oldtest.real(g_new_z.real()*15.0);    // scale it
	oldtest.imag(g_new_z.imag()*15.0);
	ComplexD testsqr;
	testsqr.real(sqr(oldtest.real()));  // set up to test with user-specified ...
	testsqr.imag(sqr(oldtest.imag()));  // ... Julia as the target set
	double testsize = 0.0;
	long testiter = 0;
	while (testsize <= g_rq_limit && testiter < g_max_iteration) // nested Julia loop
	{
		ComplexD newtest;
		newtest.real(testsqr.real() - testsqr.imag() + g_parameters[P1_REAL]);
		newtest.imag(2.0*oldtest.real()*oldtest.imag() + g_parameters[P1_IMAG]);
		testsqr.real(sqr(newtest.real()));
		testsqr.imag(sqr(newtest.imag()));
		testsize = testsqr.real() + testsqr.imag();
		oldtest = newtest;
		testiter++;
	}
	if (testsize > g_rq_limit) // point not in target set
	{
		return g_externs.BailOutFp();
	}
	else // make distinct level sets if point stayed in target set
	{
		g_color_iter = ((3L*g_color_iter) % 255L) + 1L;
		return 1;
	}
}

// from formula by Jim Muth
class MandelbrotMix4
{
public:
	MandelbrotMix4() : _a(0.0), _b(0.0), _c(MakeComplexT(0.0)), _d(0.0), _f(0.0), _g(0.0), _h(0.0),
		_j(MakeComplexT(0.0)), _k(0.0), _l(MakeComplexT(0.0)), _z(MakeComplexT(0.0))
	{ }

	bool setup();
	int per_pixel();
	int orbit();

private:
	double _a;
	double _b;
	ComplexD _c;
	double _d;
	double _f;
	double _g;
	double _h;
	ComplexD _j;
	double _k;
	ComplexD _l;
	ComplexD _z;
};

static MandelbrotMix4 s_mandelmix4;

bool MandelbrotMix4::setup()
{
	int sign_array = 0;
	_a = g_parameters[P1_REAL];				// a = real(p1),
	_b = g_parameters[P1_IMAG];				// b = imag(p1),
	_d = g_parameters[P2_REAL];				// d = real(p2),
	_f = g_parameters[P2_IMAG];				// f = imag(p2),
	_k = g_parameters[P3_REAL] + 1.0;		// k = real(p3) + 1,
	_l.real(g_parameters[P3_IMAG] + 100.0);
	_l.imag(0.0);							// l = imag(p3) + 100,
	_g = 1.0/_f;							// g = 1/f,
	_h = 1.0/_d;							// h = 1/d,
	g_temp_z = MakeComplexT(_f - _b);		// tmp = f-b
	_j = reciprocal(g_temp_z);				// j = 1/(f-b)
	g_temp_z = MakeComplexT(-_a*_b*_g*_h);	// z = (-a*b*g*h)^j,

	//	This code kludge attempts to duplicate the behavior
	//	of the parser in determining the sign of zero of the
	//	imaginary part of the argument of the power function. The
	//	reason this is important is that the complex arctangent
	//	returns PI in one case and -PI in the other, depending
	//	on the sign bit of zero, and we wish the results to be
	//	compatible with Jim Muth's mix4 formula using the parser.
	//
	//	First create a number encoding the signs of a, b, g , h. Our
	//	kludge assumes that those signs determine the behavior.
	//
	if (_a < 0.0)
	{
		sign_array += 8;
	}
	if (_b < 0.0)
	{
		sign_array += 4;
	}
	if (_g < 0.0)
	{
		sign_array += 2;
	}
	if (_h < 0.0)
	{
		sign_array += 1;
	}
	// TODO: does this really do anything? 0.0 == -0.0
	if (g_temp_z.imag() == 0.0) // we know tmp.imag() IS zero but ...
	{
		switch (sign_array)
		{
		//
		//	Add to this list the magic numbers of any cases
		//	in which the fractal does not match the formula version
		//
		case 15: // 1111
		case 10: // 1010
		case  6: // 0110
		case  5: // 0101
		case  3: // 0011
		case  0: // 0000
			g_temp_z.imag(-g_temp_z.imag()); // swap sign bit
		default: // do nothing - remaining cases already OK
			break;
		}
		// in case our kludge failed, let the user fix it
		if (DEBUGMODE_SWAP_SIGN == g_debug_mode)
		{
			g_temp_z.imag(-g_temp_z.imag());
		}
	}

	g_temp_z = pow(g_temp_z, _j);   // note: z is old
	// in case our kludge failed, let the user fix it
	if (g_parameters[P4_REAL] < 0.0)
	{
		g_temp_z = conj(g_temp_z);
	}

	if (g_externs.BailOut() == 0)
	{
		g_rq_limit = _l.real();
		g_rq_limit2 = g_rq_limit*g_rq_limit;
	}
	return true;
}

int MandelbrotMix4::per_pixel()
{
	if (g_invert)
	{
		invert_z(&g_initial_z);
	}
	else
	{
		g_initial_z = g_externs.DPixel();
	}
	g_old_z = g_temp_z;
	_c = CMPLXtrig0(g_initial_z);        // c = fn1(pixel):
	return 0; // 1st iteration has been NOT been done
}

int MandelbrotMix4::orbit()
{
	// z = k*((a*(z^b)) + (d*(z^f))) + c,
	ComplexD z_b = pow(g_old_z, MakeComplexT(_b));     // (z^b)
	ComplexD z_f = pow(g_old_z, MakeComplexT(_f));     // (z^f)
	g_new_z = _k*(_a*z_b + _d*z_f) + _c;
	return g_externs.BailOutFp();
}

bool mandelbrot_mix4_setup()
{
	return s_mandelmix4.setup();
}

int mandelbrot_mix4_per_pixel_fp()
{
	return s_mandelmix4.per_pixel();
}

int mandelbrot_mix4_orbit_fp()
{
	return s_mandelmix4.orbit();
}

//
// The following functions calculate the real and imaginary complex
// coordinates of the point in the complex plane corresponding to
// the screen coordinates (col, row) at the current zoom corners
// settings. The functions come in two flavors. One looks up the pixel
// values using the precalculated grid arrays g_x0, g_x1, g_y0, and g_y1,
// which has a speed advantage but is limited to MAX_PIXELS image
// dimensions. The other calculates the complex coordinates at a
// cost of two additions and two multiplications for each component,
// but works at any resolution.
//

// Real component, grid lookup version - requires g_x0/g_x1 arrays
static double dx_pixel_grid()
{
	return g_escape_time_state.m_grid_fp.x_pixel_grid(g_col, g_row);
}

// Real component, calculation version - does not require arrays
double dx_pixel_calc()
{
	return double(g_escape_time_state.m_grid_fp.x_min()
		+ g_col*g_escape_time_state.m_grid_fp.delta_x()
		+ g_row*g_escape_time_state.m_grid_fp.delta_x2());
}

// Imaginary component, grid lookup version - requires g_y0/g_y1 arrays
static double dy_pixel_grid()
{
	return g_escape_time_state.m_grid_fp.y_pixel_grid(g_col, g_row);
}

// Imaginary component, calculation version - does not require arrays
static double dy_pixel_calc()
{
	return double(g_escape_time_state.m_grid_fp.y_max() - g_row*g_escape_time_state.m_grid_fp.delta_y() - g_col*g_escape_time_state.m_grid_fp.delta_y2());
}

// Real component, grid lookup version - requires g_x0_l/g_x1_l arrays
static long lx_pixel_grid()
{
	return g_escape_time_state.m_grid_l.x_pixel_grid(g_col, g_row);
}

// Real component, calculation version - does not require arrays
static long lx_pixel_calc()
{
	return g_escape_time_state.m_grid_l.x_min() + g_col*g_escape_time_state.m_grid_l.delta_x() + g_row*g_escape_time_state.m_grid_l.delta_x2();
}

// Imaginary component, grid lookup version - requires g_y0_l/g_y1_l arrays
static long ly_pixel_grid()
{
	return g_escape_time_state.m_grid_l.y_pixel_grid(g_col, g_row);
}

// Imaginary component, calculation version - does not require arrays
static long ly_pixel_calc()
{
	return g_escape_time_state.m_grid_l.y_max() - g_row*g_escape_time_state.m_grid_l.delta_y() - g_col*g_escape_time_state.m_grid_l.delta_y2();
}

void set_pixel_calc_functions()
{
	if (g_escape_time_state.m_use_grid)
	{
		g_externs.SetDxPixel(dx_pixel_grid);
		g_externs.SetDyPixel(dy_pixel_grid);
		g_externs.SetLxPixel(lx_pixel_grid);
		g_externs.SetLyPixel(ly_pixel_grid);
	}
	else
	{
		g_externs.SetDxPixel(dx_pixel_calc);
		g_externs.SetDyPixel(dy_pixel_calc);
		g_externs.SetLxPixel(lx_pixel_calc);
		g_externs.SetLyPixel(ly_pixel_calc);
	}
}

void initialize_pixel_calc_functions(Externals &externs)
{
	externs.SetDxPixel(dx_pixel_grid);
	externs.SetDyPixel(dy_pixel_grid);
	externs.SetLxPixel(lx_pixel_grid);
	externs.SetLyPixel(ly_pixel_grid);
}
