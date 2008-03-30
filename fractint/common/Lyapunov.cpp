//
//	Miscellaneous fractal-specific code
//
#include <string>

#include "port.h"
#include "prototyp.h"
#include "fractype.h"

#include "calcfrac.h"
#include "drivers.h"
#include "Externals.h"
#include "fractals.h"
#include "realdos.h"

// data local to this module
static int s_lyapunov_length;
static int s_lyapunov_r_xy[34];
static double	s_population;
static long		s_filter_cycles;
static double	s_rate;

// routines local to this module
static int lyapunov_cycles(long, double, double);

// standalone engine for "lyapunov"
// Roy Murphy [76376, 721]
// revision history:
// initial version: Winter '91
//    Fall '92 integration of Nicholas Wilt's ASM speedups
//    Jan 93' integration with calcfrac() yielding boundary tracing,
//    tesseral, and solid guessing, and inversion, inside=nnn
//
int lyapunov()
{
	double a;
	double b;

	if (driver_key_pressed())
	{
		return -1;
	}
	g_overflow = false;
	if (g_parameters[1] == 1)
	{
		s_population = (1.0 + rand())/(2.0 + RAND_MAX);
	}
	else if (g_parameters[1] == 0)
	{
		if (fabs(s_population) > BIG || s_population == 0 || s_population == 1)
		{
			s_population = (1.0 + rand())/(2.0 + RAND_MAX);
		}
	}
	else
	{
		s_population = g_parameters[1];
	}
	g_plot_color(g_col, g_row, 1);
	if (g_invert)
	{
		invert_z(&g_initial_z);
		a = g_initial_z.imag();
		b = g_initial_z.real();
	}
	else
	{
		a = g_externs.DyPixel();
		b = g_externs.DxPixel();
	}
	g_color = lyapunov_cycles(s_filter_cycles, a, b);
	if (g_externs.Inside() > 0 && g_color == 0)
	{
		g_color = g_externs.Inside();
	}
	else if (g_color >= g_colors)
	{
		g_color = g_colors-1;
	}
	g_plot_color(g_col, g_row, g_color);
	return g_color;
}

bool lyapunov_setup()
{
	//
	//	This routine sets up the sequence for forcing the s_rate parameter
	//	to vary between the two values.  It fills the array s_lyapunov_r_xy[] and
	//	sets s_lyapunov_length to the length of the sequence.
	//
	//	The sequence is coded in the bit pattern in an integer.
	//	Briefly, the sequence starts with an A the leading zero bits
	//	are ignored and the remaining bit sequence is decoded.  The
	//	sequence ends with a B.  Not all possible sequences can be
	//	represented in this manner, but every possible sequence is
	//	either represented as itself, as a rotation of one of the
	//	representable sequences, or as the inverse of a representable
	//	sequence (swapping 0s and 1s in the array.)  Sequences that
	//	are the rotation and/or inverses of another sequence will generate
	//	the same lyapunov exponents.
	//
	//	A few examples follow:
	//			number    sequence
	//				0       ab
	//				1       aab
	//				2       aabb
	//				3       aaab
	//				4       aabbb
	//				5       aabab
	//				6       aaabb (this is a duplicate of 4, a rotated inverse)
	//				7       aaaab
	//				8       aabbbb  etc.
	//
	long i;
	int t;

	s_filter_cycles = long(g_parameters[2]);
	if (s_filter_cycles == 0)
	{
		s_filter_cycles = g_max_iteration/2;
	}
	s_lyapunov_length = 1;

	i = long(g_parameters[0]);
	s_lyapunov_r_xy[0] = 1;
	for (t = 31; t >= 0; t--)
	{
		if (i & (1 << t))
		{
			break;
		}
	}
	for (; t >= 0; t--)
	{
		s_lyapunov_r_xy[s_lyapunov_length++] = (i & (1 << t)) != 0;
	}
	s_lyapunov_r_xy[s_lyapunov_length++] = 0;
	if (g_externs.Inside() < 0)
	{
		stop_message(STOPMSG_NORMAL, "Sorry, inside options other than inside=nnn are not supported by the lyapunov");
		g_externs.SetInside(1);
	}
	if (g_externs.UserStandardCalculationMode() == CALCMODE_ORBITS)  // Oops, lyapunov type
	{
		ForceOnePass();
	}
	return true;
}

static int lyapunov_cycles(long filter_cycles, double a, double b)
{
	int color;
	int count;
	int lnadjust;
	long i;
	double lyap;
	double total;
	double temp;
	// e10 = 22026.4657948  e-10 = 0.0000453999297625

	total = 1.0;
	lnadjust = 0;
	for (i = 0; i < filter_cycles; i++)
	{
		for (count = 0; count < s_lyapunov_length; count++)
		{
			s_rate = s_lyapunov_r_xy[count] ? a : b;
			if (g_current_fractal_specific->orbitcalc())
			{
				g_overflow = true;
				goto jumpout;
			}
		}
	}
	for (i = 0; i < g_max_iteration/2; i++)
	{
		for (count = 0; count < s_lyapunov_length; count++)
		{
			s_rate = s_lyapunov_r_xy[count] ? a : b;
			if (g_current_fractal_specific->orbitcalc())
			{
				g_overflow = true;
				goto jumpout;
			}
			temp = fabs(s_rate-2.0*s_rate*s_population);
			total *= temp;
			if (total == 0)
			{
				g_overflow = true;
				goto jumpout;
			}
		}
		while (total > 22026.4657948)
		{
			total *= 0.0000453999297625;
			lnadjust += 10;
		}
		while (total < 0.0000453999297625)
		{
			total *= 22026.4657948;
			lnadjust -= 10;
		}
	}

jumpout:
	temp = log(total) + lnadjust;
	if (g_overflow || total <= 0 || temp > 0)
	{
		color = 0;
	}
	else
	{
		lyap = g_log_palette_mode
			? -temp/(double(s_lyapunov_length)*i)
			: 1 - exp(temp/(double(s_lyapunov_length)*i));
		color = 1 + int(lyap*(g_colors-1));
	}
	return color;
}
