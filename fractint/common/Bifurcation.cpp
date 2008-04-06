#include <algorithm>
#include <climits>
#include <sstream>
#include <string>

#include <boost/format.hpp>

#include "port.h"
#include "prototyp.h"
#include "fractype.h"

#include "drivers.h"
#include "EscapeTime.h"
#include "Externals.h"
#include "Formula.h"
#include "MathUtil.h"
#include "mpmath.h"
#include "realdos.h"
#include "resume.h"

// for bifurcation type:
static long const DEFAULT_FILTER = 1000;
// "Beauty of Fractals" recommends using 5000 (p.25), but that seems unnecessary.
// Can override this value with a nonzero param1

static double const SEED = 0.66;               // starting value for population

static void verhulst();
static void bifurcation_period_init();
static int bifurcation_periodic(long time);

static int *	s_verhulst_array = 0;
static double	s_population;
static long		s_population_l;
static long		s_filter_cycles;
static bool		s_half_time_check;
static int		s_bifurcation_saved_increment;
static long		s_bifurcation_saved_mask;
static double	s_bifurcation_saved_population;
static long		s_bifurcation_saved_population_l;
static double	s_bifurcation_close_enough;
static long		s_bifurcation_close_enough_l;
static double	s_rate;
static long		s_rate_l;
static long		s_pi_l;
static bool		s_mono = false;
static int		s_outside_x;
static long		s_beta;

static void verhulst()          // P. F. Verhulst (1845)
{
	{
		double const population = (g_parameter.imag() == 0) ? SEED : g_parameter.imag();
		if (g_integer_fractal)
		{
			s_population_l = DoubleToFudge(population);
		}
		else
		{
			s_population = population;
		}
	}

	bool errors = false;
	g_overflow = false;

	for (int counter = 0; counter < s_filter_cycles; counter++)
	{
		errors = (g_current_fractal_specific->orbitcalc() != 0);
		if (errors)
		{
			return;
		}
	}
	if (s_half_time_check) // check for periodicity at half-time
	{
		bifurcation_period_init();
		int counter;
		for (counter = 0; counter < g_max_iteration; counter++)
		{
			errors = (g_current_fractal_specific->orbitcalc() != 0);
			if (errors)
			{
				return;
			}
			if (g_periodicity_check && bifurcation_periodic(counter))
			{
				break;
			}
		}
		if (counter >= g_max_iteration)   // if not periodic, go the distance
		{
			for (counter = 0; counter < s_filter_cycles; counter++)
			{
				errors = (g_current_fractal_specific->orbitcalc() != 0);
				if (errors)
				{
					return;
				}
			}
		}
	}

	if (g_periodicity_check)
	{
		bifurcation_period_init();
	}
	for (int counter = 0; counter < g_max_iteration; counter++)
	{
		errors = (g_current_fractal_specific->orbitcalc() != 0);
		if (errors)
		{
			return;
		}

		// assign population value to Y coordinate in pixels
		int pixel_row = g_integer_fractal
			? (g_y_stop - int((s_population_l - g_initial_z_l.imag())/g_escape_time_state.m_grid_l.delta_y()))
			: (g_y_stop - int((s_population - g_initial_z.imag())/g_escape_time_state.m_grid_fp.delta_y()));

		// if it's visible on the screen, save it in the column array
		if (pixel_row <= g_y_stop)
		{
			s_verhulst_array[pixel_row] ++;
		}
		if (g_periodicity_check && bifurcation_periodic(counter))
		{
			if (pixel_row <= g_y_stop)
			{
				s_verhulst_array[pixel_row] --;
			}
			break;
		}
	}
}

static void bifurcation_period_init()
{
	s_bifurcation_saved_increment = 1;
	s_bifurcation_saved_mask = 1;
	if (g_integer_fractal)
	{
		s_bifurcation_saved_population_l = -1;
		s_bifurcation_close_enough_l = g_escape_time_state.m_grid_l.delta_y()/8;
	}
	else
	{
		s_bifurcation_saved_population = -1.0;
		s_bifurcation_close_enough = double(g_escape_time_state.m_grid_fp.delta_y())/8.0;
	}
}

// Bifurcation s_population Periodicity Check
// Returns : 1 if periodicity found, else 0
static int bifurcation_periodic(long time)
{
	if ((time & s_bifurcation_saved_mask) == 0)      // time to save a new value
	{
		if (g_integer_fractal)
		{
			s_bifurcation_saved_population_l = s_population_l;
		}
		else
		{
			s_bifurcation_saved_population =  s_population;
		}
		if (--s_bifurcation_saved_increment == 0)
		{
			s_bifurcation_saved_mask = (s_bifurcation_saved_mask << 1) + 1;
			s_bifurcation_saved_increment = 4;
		}
	}
	else                         // check against an old save
	{
		if (g_integer_fractal)
		{
			if (labs(s_bifurcation_saved_population_l-s_population_l) <= s_bifurcation_close_enough_l)
			{
				return 1;
			}
		}
		else
		{
			if (fabs(s_bifurcation_saved_population-s_population) <= s_bifurcation_close_enough)
			{
				return 1;
			}
		}
	}
	return 0;
}

//
// The following are Bifurcation "orbitcalc" routines...
//
int bifurcation_lambda() // Used by lyanupov
{
	s_population = s_rate*s_population*(1 - s_population);
	return fabs(s_population) > BIG;
}

int bifurcation_verhulst_trig_fp()
{
	g_temp_z.real(s_population);
	g_temp_z.imag(0);
	CMPLXtrig0(g_temp_z, g_temp_z);
	s_population += s_rate*g_temp_z.real()*(1 - g_temp_z.real());
	return fabs(s_population) > BIG;
}

int bifurcation_verhulst_trig()
{
#if !defined(NO_FIXED_POINT_MATH)
	g_temp_z_l.real(s_population_l);
	g_temp_z_l.imag(0);
	LCMPLXtrig0(g_temp_z_l, g_temp_z_l);
	g_temp_z_l.imag(g_temp_z_l.real() - multiply(g_temp_z_l.real(), g_temp_z_l.real(), g_bit_shift));
	s_population_l += multiply(s_rate_l, g_temp_z_l.imag(), g_bit_shift);
#endif
	return g_overflow;
}

int bifurcation_stewart_trig_fp()
{
	g_temp_z.real(s_population);
	g_temp_z.imag(0);
	CMPLXtrig0(g_temp_z, g_temp_z);
	s_population = (s_rate*g_temp_z.real()*g_temp_z.real()) - 1.0;
	return fabs(s_population) > BIG;
}

int bifurcation_stewart_trig()
{
#if !defined(NO_FIXED_POINT_MATH)
	g_temp_z_l.real(s_population_l);
	g_temp_z_l.imag(0);
	LCMPLXtrig0(g_temp_z_l, g_temp_z_l);
	s_population_l = multiply(g_temp_z_l.real(), g_temp_z_l.real(), g_bit_shift);
	s_population_l = multiply(s_population_l, s_rate_l,      g_bit_shift);
	s_population_l -= g_externs.Fudge();
#endif
	return g_overflow;
}

int bifurcation_set_trig_pi_fp()
{
	g_temp_z.real(s_population*MathUtil::Pi);
	g_temp_z.imag(0);
	CMPLXtrig0(g_temp_z, g_temp_z);
	s_population = s_rate*g_temp_z.real();
	return fabs(s_population) > BIG;
}

int bifurcation_set_trig_pi()
{
#if !defined(NO_FIXED_POINT_MATH)
	g_temp_z_l.real(multiply(s_population_l, s_pi_l, g_bit_shift));
	g_temp_z_l.imag(0);
	LCMPLXtrig0(g_temp_z_l, g_temp_z_l);
	s_population_l = multiply(s_rate_l, g_temp_z_l.real(), g_bit_shift);
#endif
	return g_overflow;
}

int bifurcation_add_trig_pi_fp()
{
	g_temp_z.real(s_population*MathUtil::Pi);
	g_temp_z.imag(0);
	CMPLXtrig0(g_temp_z, g_temp_z);
	s_population += s_rate*g_temp_z.real();
	return fabs(s_population) > BIG;
}

int bifurcation_add_trig_pi()
{
#if !defined(NO_FIXED_POINT_MATH)
	g_temp_z_l.real(multiply(s_population_l, s_pi_l, g_bit_shift));
	g_temp_z_l.imag(0);
	LCMPLXtrig0(g_temp_z_l, g_temp_z_l);
	s_population_l += multiply(s_rate_l, g_temp_z_l.real(), g_bit_shift);
#endif
	return g_overflow;
}

int bifurcation_lambda_trig_fp()
{
	// s_population = s_rate*fn(s_population)*(1 - fn(s_population))
	g_temp_z.real(s_population);
	g_temp_z.imag(0);
	CMPLXtrig0(g_temp_z, g_temp_z);
	s_population = s_rate*g_temp_z.real()*(1 - g_temp_z.real());
	return fabs(s_population) > BIG;
}

int bifurcation_lambda_trig()
{
#if !defined(NO_FIXED_POINT_MATH)
	g_temp_z_l.real(s_population_l);
	g_temp_z_l.imag(0);
	LCMPLXtrig0(g_temp_z_l, g_temp_z_l);
	g_temp_z_l.imag(g_temp_z_l.real() - multiply(g_temp_z_l.real(), g_temp_z_l.real(), g_bit_shift));
	s_population_l = multiply(s_rate_l, g_temp_z_l.imag(), g_bit_shift);
#endif
	return g_overflow;
}

int bifurcation_may_fp()
{
	// X = (lambda * X)/(1 + X)^s_beta, from R.May as described in Pickover,
	//			Computers, Pattern, Chaos, and Beauty, page 153
	g_temp_z.real(1.0 + s_population);
	g_temp_z.real(pow(g_temp_z.real(), -s_beta)); // pow in math.h included with mpmath.h
	s_population = (s_rate*s_population)*g_temp_z.real();
	return fabs(s_population) > BIG;
}

int bifurcation_may()
{
#if !defined(NO_FIXED_POINT_MATH)
	g_temp_z_l.real(s_population_l + g_externs.Fudge());
	g_temp_z_l.imag(0);
	g_parameter2_l.real(DoubleToFudge(s_beta));
	LCMPLXpwr(g_temp_z_l, g_parameter2_l, g_temp_z_l);
	s_population_l = multiply(s_rate_l, s_population_l, g_bit_shift);
	s_population_l = divide(s_population_l, g_temp_z_l.real(), g_bit_shift);
#endif
	return g_overflow;
}

bool bifurcation_may_setup()
{
	s_beta = long(g_parameters[P2_REAL]);
	if (s_beta < 2)
	{
		s_beta = 2;
	}
	g_parameters[P2_REAL] = double(s_beta);

	timer_engine(g_current_fractal_specific->calculate_type);
	return false;
}

// standalone engine for "bifurcation" types

// The following code now forms a generalised Fractal Engine
// for Bifurcation fractal typeS.  By rights it now belongs in
// CALCFRACT.C, but it's easier for me to leave it here !
//
// Original code by Phil Wilson, hacked around by Kev Allen.
//
// Besides generalisation, enhancements include Periodicity
// Checking during the plotting phase (AND halfway through the
// filter cycle, if possible, to halve calc times), quicker
// floating-point calculations for the standard Verhulst type,
// and new bifurcation types (integer bifurcation, f.p & int
// biflambda - the real equivalent of complex Lambda sets -
// and f.p renditions of bifurcations of r*sin(Pi*p), which
// spurred Mitchel Feigenbaum on to discover his Number).
//
// To add further types, extend the g_fractal_specific[] array in
// usual way, with Bifurcation as the engine, and the name of
// the routine that calculates the next bifurcation generation
// as the "orbitcalc" routine in the g_fractal_specific[] entry.
//
// Bifurcation "orbitcalc" routines get called once per screen
// pixel column.  They should calculate the next generation
// from the doubles s_rate & s_population (or the longs s_rate_l &
// s_population_l if they use integer math), placing the result
// back in s_population (or s_population_l).  They should return 0
// if all is ok, or any non-zero value if calculation bailout
// is desirable (eg in case of errors, or the series tending
// to infinity).                Have fun !
//

int bifurcation()
{
	unsigned long array_size;
	int row;
	int column;
	column = 0;
	if (g_resuming)
	{
		start_resume();
		get_resume(sizeof(column), &column);
		end_resume();
	}
	array_size =  (g_y_stop + 1)*sizeof(int); // should be g_y_stop + 1
	s_verhulst_array = new int[array_size];
	if (s_verhulst_array == 0)
	{
		stop_message(STOPMSG_NORMAL, "Insufficient free memory for calculation.");
		return -1;
	}

	s_pi_l = DoubleToFudge(MathUtil::Pi);

	for (row = 0; row <= g_y_stop; row++) // should be g_y_stop
	{
		s_verhulst_array[row] = 0;
	}

	s_mono = false;
	if (s_mono)
	{
		if (g_externs.Inside())
		{
			s_outside_x = 0;
			g_externs.SetInside(1);
		}
		else
		{
			s_outside_x = 1;
		}
	}

	s_filter_cycles = (g_parameter.real() <= 0) ? DEFAULT_FILTER : long(g_parameter.real());
	s_half_time_check = false;
	if (g_periodicity_check && g_max_iteration < s_filter_cycles)
	{
		s_filter_cycles = (s_filter_cycles - g_max_iteration + 1)/2;
		s_half_time_check = true;
	}

	// Y-value of bottom pixels
	if (g_integer_fractal)
	{
		g_initial_z_l.imag(g_escape_time_state.m_grid_l.y_max() - g_y_stop*g_escape_time_state.m_grid_l.delta_y());
	}
	else
	{
		g_initial_z.imag(g_escape_time_state.m_grid_fp.y_max() - g_y_stop*g_escape_time_state.m_grid_fp.delta_y());
	}

	while (column <= g_x_stop)
	{
		if (driver_key_pressed())
		{
			delete[] s_verhulst_array;
			alloc_resume(10, 1);
			put_resume(sizeof(column), &column);
			return -1;
		}

		if (g_integer_fractal)
		{
			s_rate_l = g_escape_time_state.m_grid_l.x_min() + column*g_escape_time_state.m_grid_l.delta_x();
		}
		else
		{
			s_rate = double(g_escape_time_state.m_grid_fp.x_min() + column*g_escape_time_state.m_grid_fp.delta_x());
		}
		verhulst();        // calculate array once per column

		for (row = g_y_stop; row >= 0; row--) // should be g_y_stop & >= 0
		{
			int color = s_verhulst_array[row];
			if (s_mono)
			{
				color = color ? g_externs.Inside() : s_outside_x;
			}
			else if (color >= g_colors)
			{
				color = g_colors-1;
			}
			s_verhulst_array[row] = 0;
			g_plot_color(column, row, color); // was row-1, but that's not right?
		}
		column++;
	}
	delete[] s_verhulst_array;
	return 0;
}

