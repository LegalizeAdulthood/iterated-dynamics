/* calmanfp.c
 * This file contains routines to replace calmanfp.asm.
 *
 * This file Copyright 1992 Ken Shirriff.  It may be used according to the
 * fractint license conditions, blah blah blah.
 */
#include <string>

#include "port.h"
#include "id.h"
#include "prototyp.h"
#include "fractype.h"
#include "externs.h"
#include "drivers.h"

#include "Externals.h"
#include "fracsubr.h"
#include "MathUtil.h"

static int inside_color, periodicity_color;

void calculate_mandelbrot_start_fp_asm()
{
	inside_color = (g_inside < 0) ? g_max_iteration : g_inside;
	periodicity_color = (g_periodicity_check < 0) ? 7 : inside_color;
	g_old_color_iter = 0;
}

#define ABS(x) ((x) < 0?-(x):(x))


static long OutsideColorModeInverseTangent(ComplexD const &new_z)
{
	return long(fabs(atan2(new_z.y, new_z.x)*g_externs.AtanColors()/MathUtil::Pi));
}

static long OutsideColorModeSum(long color_iter, ComplexD const &new_z)
{
	return color_iter + long(new_z.x + new_z.y);
}

static long OutsideColorModeMultiply(long color_iter, ComplexD const &new_z)
{
	return (new_z.y != 0.0) ?
		long(double(color_iter)*(new_z.x/new_z.y)) : color_iter;
}

static long OutsideColorModeImaginary(long color_iter, ComplexD const &new_z)
{
	return color_iter + long(new_z.y + 7);
}

static long OutsideColorModeReal(long color_iter, ComplexD const &new_z)
{
	return color_iter + long(new_z.x + 7);
}

long calculate_mandelbrot_fp_asm()
{
	double savedx;
	double savedy;

	if (g_periodicity_check == 0)
	{
		g_old_color_iter = 0;      // don't check periodicity 
	}
	else if (g_reset_periodicity)
	{
		g_old_color_iter = g_max_iteration - 255;
	}

	{
		long tmpfsd = g_max_iteration - g_first_saved_and;
		if (g_old_color_iter > tmpfsd) // this defeats checking periodicity immediately 
		{
			g_old_color_iter = tmpfsd; // but matches the code in standard_fractal() 
		}
	}

	// initparms 
	savedx = 0;
	savedy = 0;
	g_orbit_index = 0;
	long savedand = g_first_saved_and;
	int savedincr = 1;             // start checking the very first time 
	g_input_counter--;                // Only check the keyboard sometimes 
	if (g_input_counter < 0)
	{
		g_input_counter = 1000;
		int key = driver_key_pressed();
		if (key)
		{
			if (key == 'o' || key == 'O')
			{
				driver_get_key();
				g_show_orbit = !g_show_orbit;
			}
			else
			{
				g_color_iter = -1;
				return -1;
			}
		}
	}

	long cx = g_max_iteration;
	double x;
	double y;
	double x2;
	double y2;
	double xy;
	double Cx;
	double Cy;
	if (!fractal_type_julia(g_fractal_type))
	{
		// Mandelbrot_87 
		Cx = g_initial_z.x;
		Cy = g_initial_z.y;
		x = g_parameter.x + Cx;
		y = g_parameter.y + Cy;
	}
	else
	{
		// dojulia_87 
		Cx = g_parameter.x;
		Cy = g_parameter.y;
		x = g_initial_z.x;
		y = g_initial_z.y;
		x2 = x*x;
		y2 = y*y;
		xy = x*y;
		x = x2-y2 + Cx;
		y = 2*xy + Cy;
	}
	x2 = x*x;
	y2 = y*y;
	xy = x*y;

	// top_of_cs_loop_87 
	while (--cx > 0)
	{
		x = x2-y2 + Cx;
		y = 2*xy + Cy;
		x2 = x*x;
		y2 = y*y;
		xy = x*y;
		g_magnitude = x2 + y2;

		if (g_magnitude >= g_rq_limit)
		{
			goto over_bailout_87;
		}

		// no_save_new_xy_87 
		if (cx < g_old_color_iter)  // check periodicity 
		{
			if (((g_max_iteration - cx) & savedand) == 0)
			{
				savedx = x;
				savedy = y;
				savedincr--;
				if (savedincr == 0)
				{
					savedand = (savedand << 1) + 1;
					savedincr = g_next_saved_incr;
				}
			}
			else
			{
				if (ABS(savedx-x) < g_close_enough && ABS(savedy-y) < g_close_enough)
				{
					g_old_color_iter = g_max_iteration;
					g_real_color_iter = g_max_iteration;
					g_input_counter = g_input_counter-(g_max_iteration-cx);
					g_color_iter = periodicity_color;
					goto pop_stack;
				}
			}
		}
		// no_periodicity_check_87 
		if (g_show_orbit)
		{
			plot_orbit(x, y, -1);
		}
		// no_show_orbit_87 
	} // while (--cx > 0) 

	// reached maxit 
	// check periodicity immediately next time, remember we count down from maxit 
	g_old_color_iter = g_max_iteration;
	g_input_counter -= g_max_iteration;
	g_real_color_iter = g_max_iteration;
	g_color_iter = inside_color;

pop_stack:
	if (g_orbit_index)
	{
		orbit_scrub();
	}
	return g_color_iter;

over_bailout_87:
	if (g_outside <= COLORMODE_REAL)
	{
		g_new_z.x = x;
		g_new_z.y = y;
	}
	if (cx-10 > 0)
	{
		g_old_color_iter = cx-10;
	}
	else
	{
		g_old_color_iter = 0;
	}
	g_color_iter = g_max_iteration-cx;
	g_real_color_iter = g_max_iteration-cx;
	if (g_color_iter == 0)
	{
		g_color_iter = 1;
	}
	g_input_counter -= g_real_color_iter;
	if (g_outside == COLORMODE_ITERATION)
	{
	}
	else if (g_outside > COLORMODE_REAL)
	{
		g_color_iter = g_outside;
	}
	else
	{
		// special_outside 
		if (g_outside == COLORMODE_REAL)
		{
			g_color_iter = OutsideColorModeReal(g_color_iter, g_new_z);
		}
		else if (g_outside == COLORMODE_IMAGINARY)
		{
			g_color_iter = OutsideColorModeImaginary(g_color_iter, g_new_z);
		}
		else if (g_outside == COLORMODE_MULTIPLY)
		{
			g_color_iter = OutsideColorModeMultiply(g_color_iter, g_new_z);
		}
		else if (g_outside == COLORMODE_SUM)
		{
			g_color_iter = OutsideColorModeSum(g_color_iter, g_new_z);
		}
		else if (g_outside == COLORMODE_INVERSE_TANGENT)
		{
			g_color_iter = OutsideColorModeInverseTangent(g_new_z);
		}
		// check_color 
		if ((g_color_iter <= 0 || g_color_iter > g_max_iteration) && g_outside != COLORMODE_FLOAT_MODULUS)
		{
			g_color_iter = 1;
		}
	}

	goto pop_stack;
}
