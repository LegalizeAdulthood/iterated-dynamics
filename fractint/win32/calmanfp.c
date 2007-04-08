/* calmanfp.c
 * This file contains routines to replace calmanfp.asm.
 *
 * This file Copyright 1992 Ken Shirriff.  It may be used according to the
 * fractint license conditions, blah blah blah.
 */

#include "port.h"
#include "prototyp.h"
#include "fractype.h"
#include "externs.h"
#include "drivers.h"

static int inside_color, periodicity_color;

void calcmandfpasmstart(void)
{
	inside_color = (g_inside < 0) ? maxit : g_inside;
	periodicity_color = (g_periodicity_check < 0) ? 7 : inside_color;
	g_old_color_iter = 0;
}

#define ABS(x) ((x) < 0?-(x):(x))

/* If USE_NEW is 1, the magnitude is used for periodicity checking instead
	of the x and y values.  This is experimental. */
#define USE_NEW 0

long calcmandfpasm_c(void)
{
	long cx;
	long savedand;
	int savedincr;
	long tmpfsd;
#if USE_NEW
	double x, y, x2, y2, xy, Cx, Cy, savedmag;
#else
	double x, y, x2, y2, xy, Cx, Cy, savedx, savedy;
#endif

	if (g_periodicity_check == 0)
	{
		g_old_color_iter = 0;      /* don't check periodicity */
	}
	else if (g_reset_periodicity != 0)
	{
		g_old_color_iter = maxit - 255;
	}

	tmpfsd = maxit - g_first_saved_and;
	if (g_old_color_iter > tmpfsd) /* this defeats checking periodicity immediately */
	{
		g_old_color_iter = tmpfsd; /* but matches the code in standard_fractal() */
	}

	/* initparms */
#if USE_NEW
	savedmag = 0;
#else
	savedx = 0;
	savedy = 0;
#endif
	g_orbit_index = 0;
	savedand = g_first_saved_and;
	savedincr = 1;             /* start checking the very first time */
	g_input_counter--;                /* Only check the keyboard sometimes */
	if (g_input_counter < 0)
	{
		int key;
		g_input_counter = 1000;
		key = driver_key_pressed();
		if (key)
		{
			if (key == 'o' || key == 'O')
			{
				driver_get_key();
				g_show_orbit = g_show_orbit ? FALSE : TRUE;
			}
			else
			{
				g_color_iter = -1;
				return -1;
			}
		}
	}

	cx = maxit;
	if (fractype != JULIAFP && fractype != JULIA)
	{
		/* Mandelbrot_87 */
		Cx = g_initial_z.x;
		Cy = g_initial_z.y;
		x = g_parameter.x + Cx;
		y = g_parameter.y + Cy;
	}
	else
	{
		/* dojulia_87 */
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

	/* top_of_cs_loop_87 */
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

		/* no_save_new_xy_87 */
		if (cx < g_old_color_iter)  /* check periodicity */
		{
			if (((maxit - cx) & savedand) == 0)
			{
#if USE_NEW
				savedmag = g_magnitude;
#else
				savedx = x;
				savedy = y;
#endif
				savedincr--;
				if (savedincr == 0)
				{
					savedand = (savedand << 1) + 1;
					savedincr = g_next_saved_incr;
				}
			}
			else
			{
#if USE_NEW
				if (ABS(g_magnitude-savedmag) < g_close_enough)
				{
#else
				if (ABS(savedx-x) < g_close_enough && ABS(savedy-y) < g_close_enough)
				{
#endif
/*		    g_old_color_iter = 65535;  */
					g_old_color_iter = maxit;
					g_real_color_iter = maxit;
					g_input_counter = g_input_counter-(maxit-cx);
					g_color_iter = periodicity_color;
					goto pop_stack;
				}
			}
		}
		/* no_periodicity_check_87 */
		if (g_show_orbit)
		{
			plot_orbit(x, y, -1);
		}
		/* no_show_orbit_87 */
	} /* while (--cx > 0) */

	/* reached maxit */
	/* check periodicity immediately next time, remember we count down from maxit */
	g_old_color_iter = maxit;
	g_input_counter -= maxit;
	g_real_color_iter = maxit;
	g_color_iter = inside_color;

pop_stack:
	if (g_orbit_index)
	{
		orbit_scrub();
	}
	return g_color_iter;

over_bailout_87:
	if (g_outside <= -2)
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
	g_color_iter = g_real_color_iter = maxit-cx;
	if (g_color_iter == 0)
	{
		g_color_iter = 1;
	}
	g_input_counter -= g_real_color_iter;
	if (g_outside == -1)
	{
	}
	else if (g_outside > -2)
	{
		g_color_iter = g_outside;
	}
	else
	{
		/* special_outside */
		if (g_outside == REAL)
		{
			g_color_iter += (long) g_new_z.x + 7;
		}
		else if (g_outside == IMAG)
		{
			g_color_iter += (long) g_new_z.y + 7;
		}
		else if (g_outside == MULT && g_new_z.y != 0.0)
		{
		g_color_iter = (long) ((double) g_color_iter*(g_new_z.x/g_new_z.y));
		}
		else if (g_outside == SUM)
		{
			g_color_iter +=  (long) (g_new_z.x + g_new_z.y);
		}
		else if (g_outside == ATAN)
		{
			g_color_iter = (long) fabs(atan2(g_new_z.y, g_new_z.x)*g_atan_colors/PI);
		}
		/* check_color */
		if ((g_color_iter <= 0 || g_color_iter > maxit) && g_outside != FMOD)
		{
			if (g_save_release < 1961)
			{
				g_color_iter = 0;
			}
			else
			{
				g_color_iter = 1;
			}
		}
	}

	goto pop_stack;
}

long cdecl calcmandfpasm_287(void)
{
	return calcmandfpasm_c();
}

long cdecl calcmandfpasm_87(void)
{
	return calcmandfpasm_c();
}
