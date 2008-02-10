//
//	Miscellaneous fractal-specific code
//
#include <algorithm>
#include <sstream>
#include <string>

#include <string.h>
#include <limits.h>

#include <boost/format.hpp>

#include "port.h"
#include "prototyp.h"
#include "fractype.h"

#include "calcfrac.h"
#include "diskvid.h"
#include "drivers.h"
#include "EscapeTime.h"
#include "Formula.h"
#include "fpu.h"
#include "fracsubr.h"
#include "fractals.h"
#include "framain2.h"
#include "loadmap.h"
#include "MathUtil.h"
#include "miscfrac.h"
#include "miscres.h"
#include "realdos.h"
#include "resume.h"
#include "testpt.h"

// for bifurcation type: 
static long const DEFAULT_FILTER = 1000;
// "Beauty of Fractals" recommends using 5000 (p.25), but that seems unnecessary.
// Can override this value with a nonzero param1

static double const SEED = 0.66;               // starting value for population 

// global data 

// data local to this module 
static int *s_verhulst_array = 0;
static long s_filter_cycles;
static bool s_half_time_check;
static long s_population_l;
static long s_rate_l;
static double s_population;
static double s_rate;
static bool s_mono = false;
static int s_outside_x;
static long s_pi_l;
static long s_bifurcation_close_enough_l;
static long s_bifurcation_saved_population_l; // poss future use 
static double s_bifurcation_close_enough;
static double s_bifurcation_saved_population;
static int s_bifurcation_saved_increment;
static long s_bifurcation_saved_mask;
static long s_beta;
static int s_lyapunov_length;
static int s_lyapunov_r_xy[34];

// routines local to this module 
static void verhulst();
static void bifurcation_period_init();
static int bifurcation_periodic(long time);
static int lyapunov_cycles(long, double, double);


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

	s_pi_l = long(MathUtil::Pi*g_fudge);

	for (row = 0; row <= g_y_stop; row++) // should be g_y_stop 
	{
		s_verhulst_array[row] = 0;
	}

	s_mono = false;
	if (s_mono)
	{
		if (g_inside)
		{
			s_outside_x = 0;
			g_inside = 1;
		}
		else
		{
			s_outside_x = 1;
		}
	}

	s_filter_cycles = (g_parameter.x <= 0) ? DEFAULT_FILTER : long(g_parameter.x);
	s_half_time_check = false;
	if (g_periodicity_check && g_max_iteration < s_filter_cycles)
	{
		s_filter_cycles = (s_filter_cycles - g_max_iteration + 1)/2;
		s_half_time_check = true;
	}

	if (g_integer_fractal)
	{
		g_initial_z_l.y = g_escape_time_state.m_grid_l.y_max() - g_y_stop*g_escape_time_state.m_grid_l.delta_y();            // Y-value of    
	}
	else
	{
		g_initial_z.y = double(g_escape_time_state.m_grid_fp.y_max() - g_y_stop*g_escape_time_state.m_grid_fp.delta_y()); // bottom pixels 
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
			int color;
			color = s_verhulst_array[row];
			if (color && s_mono)
			{
				color = g_inside;
			}
			else if ((!color) && s_mono)
			{
				color = s_outside_x;
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

static void verhulst()          // P. F. Verhulst (1845) 
{
	if (g_integer_fractal)
	{
		s_population_l = (g_parameter.y == 0) ? long(SEED*g_fudge) : long(g_parameter.y*g_fudge);
	}
	else
	{
		s_population = (g_parameter.y == 0) ? SEED : g_parameter.y;
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
			? (g_y_stop - int((s_population_l - g_initial_z_l.y)/g_escape_time_state.m_grid_l.delta_y()))
			: (g_y_stop - int((s_population - g_initial_z.y)/g_escape_time_state.m_grid_fp.delta_y()));

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
	g_temp_z.x = s_population;
	g_temp_z.y = 0;
	CMPLXtrig0(g_temp_z, g_temp_z);
	s_population += s_rate*g_temp_z.x*(1 - g_temp_z.x);
	return fabs(s_population) > BIG;
}

int bifurcation_verhulst_trig()
{
#if !defined(XFRACT)
	g_tmp_z_l.x = s_population_l;
	g_tmp_z_l.y = 0;
	LCMPLXtrig0(g_tmp_z_l, g_tmp_z_l);
	g_tmp_z_l.y = g_tmp_z_l.x - multiply(g_tmp_z_l.x, g_tmp_z_l.x, g_bit_shift);
	s_population_l += multiply(s_rate_l, g_tmp_z_l.y, g_bit_shift);
#endif
	return g_overflow;
}

int bifurcation_stewart_trig_fp()
{
	g_temp_z.x = s_population;
	g_temp_z.y = 0;
	CMPLXtrig0(g_temp_z, g_temp_z);
	s_population = (s_rate*g_temp_z.x*g_temp_z.x) - 1.0;
	return fabs(s_population) > BIG;
}

int bifurcation_stewart_trig()
{
#if !defined(XFRACT)
	g_tmp_z_l.x = s_population_l;
	g_tmp_z_l.y = 0;
	LCMPLXtrig0(g_tmp_z_l, g_tmp_z_l);
	s_population_l = multiply(g_tmp_z_l.x, g_tmp_z_l.x, g_bit_shift);
	s_population_l = multiply(s_population_l, s_rate_l,      g_bit_shift);
	s_population_l -= g_fudge;
#endif
	return g_overflow;
}

int bifurcation_set_trig_pi_fp()
{
	g_temp_z.x = s_population*MathUtil::Pi;
	g_temp_z.y = 0;
	CMPLXtrig0(g_temp_z, g_temp_z);
	s_population = s_rate*g_temp_z.x;
	return fabs(s_population) > BIG;
}

int bifurcation_set_trig_pi()
{
#if !defined(XFRACT)
	g_tmp_z_l.x = multiply(s_population_l, s_pi_l, g_bit_shift);
	g_tmp_z_l.y = 0;
	LCMPLXtrig0(g_tmp_z_l, g_tmp_z_l);
	s_population_l = multiply(s_rate_l, g_tmp_z_l.x, g_bit_shift);
#endif
	return g_overflow;
}

int bifurcation_add_trig_pi_fp()
{
	g_temp_z.x = s_population*MathUtil::Pi;
	g_temp_z.y = 0;
	CMPLXtrig0(g_temp_z, g_temp_z);
	s_population += s_rate*g_temp_z.x;
	return fabs(s_population) > BIG;
}

int bifurcation_add_trig_pi()
{
#if !defined(XFRACT)
	g_tmp_z_l.x = multiply(s_population_l, s_pi_l, g_bit_shift);
	g_tmp_z_l.y = 0;
	LCMPLXtrig0(g_tmp_z_l, g_tmp_z_l);
	s_population_l += multiply(s_rate_l, g_tmp_z_l.x, g_bit_shift);
#endif
	return g_overflow;
}

int bifurcation_lambda_trig_fp()
{
	// s_population = s_rate*fn(s_population)*(1 - fn(s_population)) 
	g_temp_z.x = s_population;
	g_temp_z.y = 0;
	CMPLXtrig0(g_temp_z, g_temp_z);
	s_population = s_rate*g_temp_z.x*(1 - g_temp_z.x);
	return fabs(s_population) > BIG;
}

int bifurcation_lambda_trig()
{
#if !defined(XFRACT)
	g_tmp_z_l.x = s_population_l;
	g_tmp_z_l.y = 0;
	LCMPLXtrig0(g_tmp_z_l, g_tmp_z_l);
	g_tmp_z_l.y = g_tmp_z_l.x - multiply(g_tmp_z_l.x, g_tmp_z_l.x, g_bit_shift);
	s_population_l = multiply(s_rate_l, g_tmp_z_l.y, g_bit_shift);
#endif
	return g_overflow;
}

int bifurcation_may_fp()
{
	// X = (lambda * X)/(1 + X)^s_beta, from R.May as described in Pickover,
	//			Computers, Pattern, Chaos, and Beauty, page 153
	g_temp_z.x = 1.0 + s_population;
	g_temp_z.x = pow(g_temp_z.x, -s_beta); // pow in math.h included with mpmath.h 
	s_population = (s_rate*s_population)*g_temp_z.x;
	return fabs(s_population) > BIG;
}

int bifurcation_may()
{
#if !defined(XFRACT)
	g_tmp_z_l.x = s_population_l + g_fudge;
	g_tmp_z_l.y = 0;
	g_parameter2_l.x = s_beta*g_fudge;
	LCMPLXpwr(g_tmp_z_l, g_parameter2_l, g_tmp_z_l);
	s_population_l = multiply(s_rate_l, s_population_l, g_bit_shift);
	s_population_l = divide(s_population_l, g_tmp_z_l.x, g_bit_shift);
#endif
	return g_overflow;
}

bool bifurcation_may_setup()
{
	s_beta = long(g_parameters[2]);
	if (s_beta < 2)
	{
		s_beta = 2;
	}
	g_parameters[2] = double(s_beta);

	timer_engine(g_current_fractal_specific->calculate_type);
	return false;
}

// standalone engine for "popcorn"
int popcorn()   // subset of std engine 
{
	int start_row;
	start_row = 0;
	if (g_resuming)
	{
		start_resume();
		get_resume(sizeof(start_row), &start_row);
		end_resume();
	}
	g_input_counter = g_max_input_counter;
	g_plot_color = plot_color_none;
	g_temp_sqr_x = g_temp_sqr_x_l = 0; // PB added this to cover weird BAILOUTs 
	for (g_row = start_row; g_row <= g_y_stop; g_row++)
	{
		g_reset_periodicity = true;
		for (g_col = 0; g_col <= g_x_stop; g_col++)
		{
			if (standard_fractal() == -1) // interrupted 
			{
				alloc_resume(10, 1);
				put_resume(sizeof(g_row), &g_row);
				return -1;
			}
			g_reset_periodicity = false;
		}
	}
	g_calculation_status = CALCSTAT_COMPLETED;
	return 0;
}

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
		a = g_initial_z.y;
		b = g_initial_z.x;
	}
	else
	{
		a = g_dy_pixel();
		b = g_dx_pixel();
	}
	g_color = lyapunov_cycles(s_filter_cycles, a, b);
	if (g_inside > 0 && g_color == 0)
	{
		g_color = g_inside;
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
	if (g_inside < 0)
	{
		stop_message(STOPMSG_NORMAL, "Sorry, inside options other than inside=nnn are not supported by the lyapunov");
		g_inside = 1;
	}
	if (g_user_standard_calculation_mode == CALCMODE_ORBITS)  // Oops, lyapunov type 
	{
		g_user_standard_calculation_mode = CALCMODE_SINGLE_PASS;  // doesn't use new & breaks orbits 
		g_standard_calculation_mode = CALCMODE_SINGLE_PASS;
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

std::string precision_format(const char *specifier, int precision)
{
	return str(boost::format("%%.%d%s") % precision % specifier);
}

