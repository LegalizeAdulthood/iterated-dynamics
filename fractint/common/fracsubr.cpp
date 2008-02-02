/*
	fracsubr.cpp contains subroutines which belong primarily to calcfrac.cpp and
	fractals.cpp, i.e. which are non-fractal-specific fractal engine subroutines.
*/
#include <algorithm>
#include <string>

#include <assert.h>
#include <memory.h>
#include <stdarg.h>
#ifndef XFRACT
#include <sys/timeb.h>
#endif
#include <sys/types.h>
#include <time.h>

#include "port.h"
#include "prototyp.h"
#include "fractype.h"

#include "calcfrac.h"
#include "drivers.h"
#include "fihelp.h"
#include "fracsubr.h"
#include "fractalb.h"
#include "fractalp.h"
#include "miscovl.h"
#include "miscres.h"
#include "realdos.h"

#include "EscapeTime.h"
#include "FiniteAttractor.h"
#include "SoundState.h"
#include "WorkList.h"

#if defined(_WIN32)
#define ftimex ftime
#define timebx timeb
#endif

/* g_fudge all values up by 2 << FUDGE_FACTOR{,2} */
enum
{
	FUDGE_FACTOR     = 29,
	FUDGE_FACTOR2    = 24,
};

static int s_save_orbit[1500] = { 0 };	/* array to save orbit values */

/* routines in this module      */
static long   fudge_to_long(double d);
static double fudge_to_double(long value);
static void   adjust_to_limits(double expand);
static void   smallest_add(double *);
static int    ratio_bad(double, double);
static void   plot_orbit_d(double, double, int);
static void   adjust_to_limits_bf(double expand);
static void   smallest_add_bf(bf_t);

void fractal_float_to_bf()
{
	int i;
	init_bf_dec(get_precision_dbl(CURRENTREZ));
	floattobf(g_escape_time_state.m_grid_bf.x_min(), g_escape_time_state.m_grid_fp.x_min());
	floattobf(g_escape_time_state.m_grid_bf.x_max(), g_escape_time_state.m_grid_fp.x_max());
	floattobf(g_escape_time_state.m_grid_bf.y_min(), g_escape_time_state.m_grid_fp.y_min());
	floattobf(g_escape_time_state.m_grid_bf.y_max(), g_escape_time_state.m_grid_fp.y_max());
	floattobf(g_escape_time_state.m_grid_bf.x_3rd(), g_escape_time_state.m_grid_fp.x_3rd());
	floattobf(g_escape_time_state.m_grid_bf.y_3rd(), g_escape_time_state.m_grid_fp.y_3rd());

	for (i = 0; i < MAX_PARAMETERS; i++)
	{
		if (type_has_parameter(g_fractal_type, i))
		{
			floattobf(bfparms[i], g_parameters[i]);
		}
	}
	g_calculation_status = CALCSTAT_PARAMS_CHANGED;
}


void calculate_fractal_initialize_bail_out_limit()
{
	if (g_potential_flag && g_potential_parameter[2] != 0.0)
	{
		g_rq_limit = g_potential_parameter[2];
	}
	else if (g_bail_out) /* user input bailout */
	{
		g_rq_limit = g_bail_out;
	}
	else if (g_biomorph != -1) /* biomorph benefits from larger bailout */
	{
		g_rq_limit = 100;
	}
	else
	{
		g_rq_limit = g_current_fractal_specific->orbit_bailout;
	}
	if (g_integer_fractal) /* the bailout limit mustn't be too high here */
	{
		if (g_rq_limit > 127.0)
		{
			g_rq_limit = 127.0;
		}
	}
}

/* initialize a *pile* of stuff for fractal calculation */
void calculate_fractal_initialize()
{
	int tries = 0;
	g_color_iter = 0;
	g_old_color_iter = 0;

	/* set up grid array compactly leaving space at end */
	/* space req for grid is 2(g_x_dots + g_y_dots)*sizeof(long or double) */
	/* space available in extraseg is 65536 Bytes */
	{
		long xytemp = g_x_dots + g_y_dots;
		if ((!g_user_float_flag && (xytemp*sizeof(long) > 32768))
			|| (g_user_float_flag && (xytemp*sizeof(double) > 32768))
			|| DEBUGMODE_NO_PIXEL_GRID == g_debug_mode)
		{
			g_escape_time_state.m_use_grid = false;
			g_float_flag = true;
			g_user_float_flag = true;
		}
		else
		{
			g_escape_time_state.m_use_grid = true;
		}
	}

	g_escape_time_state.set_grids();

	if (!g_current_fractal_specific->arbitrary_precision())
	{
		int fractal_type_float = g_current_fractal_specific->tofloat;
		if (fractal_type_none(fractal_type_float))
		{
			g_bf_math = 0;
		}
		else if (!g_fractal_specific[fractal_type_float].arbitrary_precision())
		{
			g_bf_math = 0;
		}
		else if (g_bf_math)
		{
			g_fractal_type = fractal_type_float;
			g_current_fractal_specific = &g_fractal_specific[g_fractal_type];
		}
	}

	/* switch back to double when zooming out if using arbitrary precision */
	if (g_bf_math)
	{
		int gotprec = get_precision_bf(CURRENTREZ);
		if ((gotprec <= DBL_DIG + 1 && g_debug_mode != DEBUGMODE_NO_BIG_TO_FLOAT) || g_math_tolerance[1] >= 1.0)
		{
			corners_bf_to_float();
			g_bf_math = 0;
		}
		else
		{
			init_bf_dec(gotprec);
		}
	}
	else if (DEBUGMODE_NO_BIG_TO_FLOAT == g_debug_mode)
	{
		if (fractal_type_mandelbrot(g_fractal_type))
		{
			g_fractal_type = FRACTYPE_MANDELBROT_FP;
			g_current_fractal_specific = &g_fractal_specific[g_fractal_type];
			fractal_float_to_bf();
			g_user_float_flag = true;
		}
		else if (fractal_type_julia(g_fractal_type))
		{
			g_fractal_type = FRACTYPE_JULIA_FP;
			g_current_fractal_specific = &g_fractal_specific[g_fractal_type];
			fractal_float_to_bf();
			g_user_float_flag = true;
		}
		else if (g_fractal_type == FRACTYPE_MANDELBROT_Z_POWER_L
			|| g_fractal_type == FRACTYPE_MANDELBROT_Z_POWER_FP)
		{
			g_fractal_type = FRACTYPE_MANDELBROT_Z_POWER_FP;
			g_current_fractal_specific = &g_fractal_specific[g_fractal_type];
			fractal_float_to_bf();
			g_user_float_flag = true;
		}
		else if (g_fractal_type == FRACTYPE_JULIA_Z_POWER_L
			|| g_fractal_type == FRACTYPE_JULIA_Z_POWER_FP)
		{
			g_fractal_type = FRACTYPE_JULIA_Z_POWER_FP;
			g_current_fractal_specific = &g_fractal_specific[g_fractal_type];
			fractal_float_to_bf();
			g_user_float_flag = true;
		}
	}
	else
	{
		free_bf_vars();
	}
	g_float_flag = (g_bf_math || g_user_float_flag);
	if (g_calculation_status == CALCSTAT_RESUMABLE)  /* on resume, ensure g_float_flag correct */
	{
		g_float_flag = (g_current_fractal_specific->isinteger == 0);
	}
	/* if floating pt only, set g_float_flag for TAB screen */
	if (!g_current_fractal_specific->isinteger
		&& fractal_type_none(g_current_fractal_specific->tofloat))
	{
		g_float_flag = true;
	}
	if (g_user_standard_calculation_mode == CALCMODE_SYNCHRONOUS_ORBITS)
	{
		if (fractal_type_mandelbrot(g_fractal_type))
		{
			g_float_flag = true;
		}
		else
		{
			g_user_standard_calculation_mode = CALCMODE_SINGLE_PASS;
		}
	}

init_restart:
#if defined(_WIN32)
	_ASSERTE(_CrtCheckMemory());
#endif

	/* the following variables may be forced to a different setting due to
		calc routine constraints;  usr_xxx is what the user last said is wanted,
		xxx is what we actually do in the current situation */
	g_standard_calculation_mode = g_user_standard_calculation_mode;
	g_periodicity_check = g_user_periodicity_check;
	g_distance_test = g_user_distance_test;
	g_biomorph = g_user_biomorph;

	if (g_inside == COLORMODE_INVERSE_TANGENT_INTEGER)
	{
		g_periodicity_check = 0;
	}

	g_potential_flag = false;
	if (g_potential_parameter[0] != 0.0
		&& (g_current_fractal_specific->calculate_type == standard_fractal
			|| g_current_fractal_specific->calculate_type == calculate_mandelbrot_l
			|| g_current_fractal_specific->calculate_type == calculate_mandelbrot_fp))
	{
		g_potential_flag = true;
		g_distance_test = 0;
		g_user_distance_test = 0;    /* can't do distest too */
	}

	if (g_distance_test)
	{
		g_float_flag = true;  /* force floating point for dist est */
	}

	if (g_float_flag)  /* ensure type matches g_float_flag */
	{
		if (g_current_fractal_specific->isinteger != 0
			&& !fractal_type_none(g_current_fractal_specific->tofloat))
		{
			g_fractal_type = g_current_fractal_specific->tofloat;
		}
	}
	else
	{
		if (g_current_fractal_specific->isinteger == 0
			&& !fractal_type_none(g_current_fractal_specific->tofloat))
		{
			g_fractal_type = g_current_fractal_specific->tofloat;
		}
	}
	/* match Julibrot with integer mode of orbit */
	if (g_fractal_type == FRACTYPE_JULIBROT_FP)
	{
		if (g_fractal_specific[g_new_orbit_type].isinteger)
		{
			int i = g_fractal_specific[g_new_orbit_type].tofloat;
			if (!fractal_type_none(i))
			{
				g_new_orbit_type = i;
			}
			else
			{
				g_fractal_type = FRACTYPE_JULIBROT;
			}
		}
		else
		{
			int i = g_fractal_specific[g_new_orbit_type].tofloat;
			if (!fractal_type_none(i))
			{
				g_new_orbit_type = i;
			}
			else
			{
				g_fractal_type = FRACTYPE_JULIBROT_FP;
			}
		}
	}

	g_current_fractal_specific = &g_fractal_specific[g_fractal_type];
	g_integer_fractal = g_current_fractal_specific->isinteger;

	calculate_fractal_initialize_bail_out_limit();

	double ftemp;
	if (g_current_fractal_specific->no_zoom_box_rotate())
	{
		/* ensure min < max and unrotated rectangle */
		if (g_escape_time_state.m_grid_fp.x_min() > g_escape_time_state.m_grid_fp.x_max())
		{
			ftemp = g_escape_time_state.m_grid_fp.x_max();
			g_escape_time_state.m_grid_fp.x_max() = g_escape_time_state.m_grid_fp.x_min();
			g_escape_time_state.m_grid_fp.x_min() = ftemp;
		}
		if (g_escape_time_state.m_grid_fp.y_min() > g_escape_time_state.m_grid_fp.y_max())
		{
			ftemp = g_escape_time_state.m_grid_fp.y_max();
			g_escape_time_state.m_grid_fp.y_max() = g_escape_time_state.m_grid_fp.y_min();
			g_escape_time_state.m_grid_fp.y_min() = ftemp;
		}
		g_escape_time_state.m_grid_fp.x_3rd() = g_escape_time_state.m_grid_fp.x_min();
		g_escape_time_state.m_grid_fp.y_3rd() = g_escape_time_state.m_grid_fp.y_min();
	}

	/* set up g_bit_shift for integer math */
	g_bit_shift = FUDGE_FACTOR2; /* by default, the smaller shift */
	if (g_integer_fractal > 1)  /* use specific override from table */
	{
		g_bit_shift = g_integer_fractal;
	}
	if (g_integer_fractal == 0)  /* float? */
	{
		int i = g_current_fractal_specific->tofloat;
		if (!fractal_type_none(i)) /* -> int? */
		{
			if (g_fractal_specific[i].isinteger > 1) /* specific shift? */
			{
				g_bit_shift = g_fractal_specific[i].isinteger;
			}
		}
		else
		{
			g_bit_shift = 16;  /* to allow larger corners */
		}
	}

	/* We want this code if we're using the assembler calculate_mandelbrot_l */
	if (g_fractal_type == FRACTYPE_MANDELBROT || g_fractal_type == FRACTYPE_JULIA)  /* adust shift bits if.. */
	{
		if (!g_potential_flag                            /* not using potential */
			&& (g_parameters[0] > -2.0 && g_parameters[0] < 2.0)  /* parameters not too large */
			&& (g_parameters[1] > -2.0 && g_parameters[1] < 2.0)
			&& !g_invert                                /* and not inverting */
			&& g_biomorph == -1                         /* and not biomorphing */
			&& g_rq_limit <= 4.0                           /* and bailout not too high */
			&& (g_outside > COLORMODE_REAL || g_outside < COLORMODE_INVERSE_TANGENT)         /* and no funny outside stuff */
			&& g_debug_mode != DEBUGMODE_FORCE_BITSHIFT	/* and not debugging */
			&& g_proximity <= 2.0                       /* and g_proximity not too large */
			&& g_bail_out_test == BAILOUT_MODULUS)                     /* and bailout test = mod */
		{
			g_bit_shift = FUDGE_FACTOR;                  /* use the larger g_bit_shift */
		}
	}

	g_fudge = 1L << g_bit_shift;

	g_attractor_radius_l = g_fudge/32768L;
	g_attractor_radius_fp = 1.0/32768L;

	/* now setup arrays of real coordinates corresponding to each pixel */
	if (g_bf_math)
	{
		adjust_to_limits_bf(1.0); /* make sure all corners in valid range */
	}
	else
	{
		adjust_to_limits(1.0); /* make sure all corners in valid range */
		g_escape_time_state.m_grid_fp.delta_x()  = (LDBL)(g_escape_time_state.m_grid_fp.x_max() - g_escape_time_state.m_grid_fp.x_3rd())/(LDBL)g_dx_size; /* calculate stepsizes */
		g_escape_time_state.m_grid_fp.delta_y()  = (LDBL)(g_escape_time_state.m_grid_fp.y_max() - g_escape_time_state.m_grid_fp.y_3rd())/(LDBL)g_dy_size;
		g_escape_time_state.m_grid_fp.delta_x2() = (LDBL)(g_escape_time_state.m_grid_fp.x_3rd() - g_escape_time_state.m_grid_fp.x_min())/(LDBL)g_dy_size;
		g_escape_time_state.m_grid_fp.delta_y2() = (LDBL)(g_escape_time_state.m_grid_fp.y_3rd() - g_escape_time_state.m_grid_fp.y_min())/(LDBL)g_dx_size;
		g_escape_time_state.fill_grid_fp();
	}

	if (!fractal_type_ant_or_cellular(g_fractal_type))  /* fudge_to_long fails w >10 digits in double */
	{
		g_c_real = fudge_to_long(g_parameters[0]); /* integer equivs for it all */
		g_c_imag = fudge_to_long(g_parameters[1]);
		g_escape_time_state.m_grid_l.x_min() = fudge_to_long(g_escape_time_state.m_grid_fp.x_min());
		g_escape_time_state.m_grid_l.x_max() = fudge_to_long(g_escape_time_state.m_grid_fp.x_max());
		g_escape_time_state.m_grid_l.x_3rd() = fudge_to_long(g_escape_time_state.m_grid_fp.x_3rd());
		g_escape_time_state.m_grid_l.y_min() = fudge_to_long(g_escape_time_state.m_grid_fp.y_min());
		g_escape_time_state.m_grid_l.y_max() = fudge_to_long(g_escape_time_state.m_grid_fp.y_max());
		g_escape_time_state.m_grid_l.y_3rd() = fudge_to_long(g_escape_time_state.m_grid_fp.y_3rd());
		g_escape_time_state.m_grid_l.delta_x() = fudge_to_long(g_escape_time_state.m_grid_fp.delta_x());
		g_escape_time_state.m_grid_l.delta_y() = fudge_to_long(g_escape_time_state.m_grid_fp.delta_y());
		g_escape_time_state.m_grid_l.delta_x2() = fudge_to_long(g_escape_time_state.m_grid_fp.delta_x2());
		g_escape_time_state.m_grid_l.delta_y2() = fudge_to_long(g_escape_time_state.m_grid_fp.delta_y2());
	}

	/* skip this if plasma to avoid 3d problems */
	/* skip if g_bf_math to avoid extraseg conflict with g_x0 arrays */
	/* skip if ifs, ifs3d, or lsystem to avoid crash when mathtolerance */
	/* is set.  These types don't auto switch between float and integer math */
	if (g_fractal_type != FRACTYPE_PLASMA
		&& g_bf_math == 0
		&& !fractal_type_ifs(g_fractal_type)
		&& g_fractal_type != FRACTYPE_L_SYSTEM)
	{
		if (g_integer_fractal && !g_invert && g_escape_time_state.m_use_grid)
		{
			if ((g_escape_time_state.m_grid_l.delta_x()  == 0 && g_escape_time_state.m_grid_fp.delta_x()  != 0.0)
				|| (g_escape_time_state.m_grid_l.delta_x2() == 0 && g_escape_time_state.m_grid_fp.delta_x2() != 0.0)
				|| (g_escape_time_state.m_grid_l.delta_y()  == 0 && g_escape_time_state.m_grid_fp.delta_y()  != 0.0)
				|| (g_escape_time_state.m_grid_l.delta_y2() == 0 && g_escape_time_state.m_grid_fp.delta_y2() != 0.0))
			{
				goto expand_retry;
			}

			g_escape_time_state.fill_grid_l();   /* fill up the x, y grids */
			/* past max res?  check corners within 10% of expected */
			if (   ratio_bad(double(g_escape_time_state.m_grid_l.x0(g_x_dots - 1)) - g_escape_time_state.m_grid_l.x_min(), double(g_escape_time_state.m_grid_l.x_max()) - g_escape_time_state.m_grid_l.x_3rd())
				|| ratio_bad(double(g_escape_time_state.m_grid_l.y0(g_y_dots - 1)) - g_escape_time_state.m_grid_l.y_max(), double(g_escape_time_state.m_grid_l.y_3rd()) - g_escape_time_state.m_grid_l.y_max())
				|| ratio_bad(double(g_escape_time_state.m_grid_l.x1((g_y_dots/2) - 1)), (double(g_escape_time_state.m_grid_l.x_3rd()) - g_escape_time_state.m_grid_l.x_min())/2)
				|| ratio_bad(double(g_escape_time_state.m_grid_l.y1((g_x_dots/2) - 1)), (double(g_escape_time_state.m_grid_l.y_min()) - g_escape_time_state.m_grid_l.y_3rd())/2))
			{
expand_retry:
				if (g_integer_fractal          /* integer fractal type? */
					&& !fractal_type_none(g_current_fractal_specific->tofloat))
				{
					g_float_flag = true;           /* switch to floating pt */
				}
				else
				{
					adjust_to_limits(2.0);   /* double the size */
				}
				if (g_calculation_status == CALCSTAT_RESUMABLE)       /* due to restore of an old file? */
				{
					g_calculation_status = CALCSTAT_PARAMS_CHANGED;         /*   whatever, it isn't resumable */
				}
				goto init_restart;
			} /* end if ratio bad */

			/* re-set corners to match reality */
			g_escape_time_state.m_grid_l.x_max() = g_escape_time_state.m_grid_l.x0(g_x_dots-1) + g_escape_time_state.m_grid_l.x1(g_y_dots-1);
			g_escape_time_state.m_grid_l.y_min() = g_escape_time_state.m_grid_l.y0(g_y_dots-1) + g_escape_time_state.m_grid_l.y1(g_x_dots-1);
			g_escape_time_state.m_grid_l.x_3rd() = g_escape_time_state.m_grid_l.x_min() + g_escape_time_state.m_grid_l.x1(g_y_dots-1);
			g_escape_time_state.m_grid_l.y_3rd() = g_escape_time_state.m_grid_l.y0(g_y_dots-1);
			g_escape_time_state.m_grid_fp.x_min() = fudge_to_double(g_escape_time_state.m_grid_l.x_min());
			g_escape_time_state.m_grid_fp.x_max() = fudge_to_double(g_escape_time_state.m_grid_l.x_max());
			g_escape_time_state.m_grid_fp.x_3rd() = fudge_to_double(g_escape_time_state.m_grid_l.x_3rd());
			g_escape_time_state.m_grid_fp.y_min() = fudge_to_double(g_escape_time_state.m_grid_l.y_min());
			g_escape_time_state.m_grid_fp.y_max() = fudge_to_double(g_escape_time_state.m_grid_l.y_max());
			g_escape_time_state.m_grid_fp.y_3rd() = fudge_to_double(g_escape_time_state.m_grid_l.y_3rd());
		} /* end if (g_integer_fractal && !g_invert && g_escape_time_state.m_use_grid) */
		else
		{
			/* set up dx0 and dy0 analogs of g_x0_l and g_y0_l */
			/* put fractal parameters in doubles */
			double dx0 = g_escape_time_state.m_grid_fp.x_min();                /* fill up the x, y grids */
			double dy0 = g_escape_time_state.m_grid_fp.y_max();
			double dx1 = 0;
			double dy1 = 0;
			/* this way of defining the dx and dy arrays is not the most
				accurate, but it is kept because it is used to determine
				the limit of resolution */
			for (int i = 1; i < g_x_dots; i++)
			{
				dx0 = double(dx0 + double(g_escape_time_state.m_grid_fp.delta_x()));
				dy1 = double(dy1 - double(g_escape_time_state.m_grid_fp.delta_y2()));
			}
			for (int i = 1; i < g_y_dots; i++)
			{
				dy0 = double(dy0 - double(g_escape_time_state.m_grid_fp.delta_y()));
				dx1 = double(dx1 + double(g_escape_time_state.m_grid_fp.delta_x2()));
			}
			if (g_bf_math == 0) /* redundant test, leave for now */
			{
				/* Following is the old logic for detecting failure of double
					precision. It has two advantages: it is independent of the
					representation of numbers, and it is sensitive to resolution
					(allows depper zooms at lower resolution. However it fails
					for rotations of exactly 90 degrees, so we added a safety net
					by using the magnification.  */
				if (++tries < 2) /* for safety */
				{
					if (tries > 1)
					{
						stop_message(STOPMSG_NORMAL, "precision-detection error");
					}
					/* Previously there were four tests of distortions in the
					zoom box used to detect precision limitations. In some
					cases of rotated/skewed zoom boxs, this causes the algorithm
					to bail out to arbitrary precision too soon. The logic
					now only tests the larger of the two deltas in an attempt
					to repair this bug. This should never make the transition
					to arbitrary precision sooner, but always later.*/
					double testx_try;
					double testx_exact;
					if (fabs(g_escape_time_state.m_grid_fp.x_max()-g_escape_time_state.m_grid_fp.x_3rd()) > fabs(g_escape_time_state.m_grid_fp.x_3rd()-g_escape_time_state.m_grid_fp.x_min()))
					{
						testx_exact  = g_escape_time_state.m_grid_fp.x_max()-g_escape_time_state.m_grid_fp.x_3rd();
						testx_try    = dx0-g_escape_time_state.m_grid_fp.x_min();
					}
					else
					{
						testx_exact  = g_escape_time_state.m_grid_fp.x_3rd()-g_escape_time_state.m_grid_fp.x_min();
						testx_try    = dx1;
					}
					double testy_exact;
					double testy_try;
					if (fabs(g_escape_time_state.m_grid_fp.y_3rd()-g_escape_time_state.m_grid_fp.y_max()) > fabs(g_escape_time_state.m_grid_fp.y_min()-g_escape_time_state.m_grid_fp.y_3rd()))
					{
						testy_exact = g_escape_time_state.m_grid_fp.y_3rd()-g_escape_time_state.m_grid_fp.y_max();
						testy_try   = dy0-g_escape_time_state.m_grid_fp.y_max();
					}
					else
					{
						testy_exact = g_escape_time_state.m_grid_fp.y_min()-g_escape_time_state.m_grid_fp.y_3rd();
						testy_try   = dy1;
					}
					if (ratio_bad(testx_try, testx_exact) ||
						ratio_bad(testy_try, testy_exact))
					{
						if (g_current_fractal_specific->arbitrary_precision())
						{
							fractal_float_to_bf();
							goto init_restart;
						}
						goto expand_retry;
					} /* end if ratio_bad etc. */
				} /* end if tries < 2 */
			} /* end if g_bf_math == 0 */

			/* if long double available, this is more accurate */
			g_escape_time_state.fill_grid_fp();       /* fill up the x, y grids */

			/* re-set corners to match reality */
			g_escape_time_state.m_grid_fp.x_max() = double(g_escape_time_state.m_grid_fp.x_min() + (g_x_dots-1)*g_escape_time_state.m_grid_fp.delta_x() + (g_y_dots-1)*g_escape_time_state.m_grid_fp.delta_x2());
			g_escape_time_state.m_grid_fp.y_min() = double(g_escape_time_state.m_grid_fp.y_max() - (g_y_dots-1)*g_escape_time_state.m_grid_fp.delta_y() - (g_x_dots-1)*g_escape_time_state.m_grid_fp.delta_y2());
			g_escape_time_state.m_grid_fp.x_3rd() = double(g_escape_time_state.m_grid_fp.x_min() + (g_y_dots-1)*g_escape_time_state.m_grid_fp.delta_x2());
			g_escape_time_state.m_grid_fp.y_3rd() = double(g_escape_time_state.m_grid_fp.y_max() - (g_y_dots-1)*g_escape_time_state.m_grid_fp.delta_y());
		} /* end else */
	} /* end if not plasma */

	/* for periodicity close-enough, and for unity: */
	/*     std::min(std::max(g_delta_x, g_delta_x2), std::max(g_delta_y, g_delta_y2)      */
	g_delta_min_fp = fabs(double(g_escape_time_state.m_grid_fp.delta_x()));
	if (fabs(double(g_escape_time_state.m_grid_fp.delta_x2())) > g_delta_min_fp)
	{
		g_delta_min_fp = fabs(double(g_escape_time_state.m_grid_fp.delta_x2()));
	}
	if (fabs(double(g_escape_time_state.m_grid_fp.delta_y())) > fabs(double(g_escape_time_state.m_grid_fp.delta_y2())))
	{
		if (fabs(double(g_escape_time_state.m_grid_fp.delta_y())) < g_delta_min_fp)
		{
			g_delta_min_fp = fabs(double(g_escape_time_state.m_grid_fp.delta_y()));
		}
	}
	else if (fabs(double(g_escape_time_state.m_grid_fp.delta_y2())) < g_delta_min_fp)
	{
		g_delta_min_fp = fabs(double(g_escape_time_state.m_grid_fp.delta_y2()));
	}
	g_delta_min = fudge_to_long(g_delta_min_fp);

	/* calculate factors which plot real values to screen co-ords */
	/* calcfrac.c plot_orbit routines have comments about this    */
	ftemp = double(-g_escape_time_state.m_grid_fp.delta_y2()*g_escape_time_state.m_grid_fp.delta_x2()*g_dx_size*g_dy_size - (g_escape_time_state.m_grid_fp.x_max() - g_escape_time_state.m_grid_fp.x_3rd())*(g_escape_time_state.m_grid_fp.y_3rd() - g_escape_time_state.m_grid_fp.y_max()));
	if (ftemp != 0.0)
	{
		g_plot_mx1 = double(g_escape_time_state.m_grid_fp.delta_x2()*g_dx_size*g_dy_size/ftemp);
		g_plot_mx2 = (g_escape_time_state.m_grid_fp.y_3rd()-g_escape_time_state.m_grid_fp.y_max())*g_dx_size/ftemp;
		g_plot_my1 = double(-g_escape_time_state.m_grid_fp.delta_y2()*g_dx_size*g_dy_size/ftemp);
		g_plot_my2 = (g_escape_time_state.m_grid_fp.x_max()-g_escape_time_state.m_grid_fp.x_3rd())*g_dy_size/ftemp;
	}
	if (g_bf_math == 0)
	{
		free_bf_vars();
	}
}

static long fudge_to_long(double d)
{
	d *= g_fudge;
	if (d > 0)
	{
		d += 0.5;
	}
	else
	{
		d -= 0.5;
	}
	return long(d);
}

static double fudge_to_double(long value)
{
	return double(value)/double(g_fudge);
}

void adjust_corner_bf(float aspect_drift)
{
	/* make edges very near vert/horiz exact, to ditch rounding errs and */
	/* to avoid problems when delta per axis makes too large a ratio     */
	double ftemp;
	double Xmagfactor;
	double Rotation;
	double Skew;
	LDBL Magnification;

	bf_t bftemp, bftemp2;
	bf_t btmp1;
	int saved = save_stack();
	bftemp  = alloc_stack(g_rbf_length + 2);
	bftemp2 = alloc_stack(g_rbf_length + 2);
	btmp1  =  alloc_stack(g_rbf_length + 2);

	/* While we're at it, let's adjust the Xmagfactor as well */
	/* use bftemp, bftemp2 as bfXctr, bfYctr */
	convert_center_mag_bf(bftemp, bftemp2, &Magnification, &Xmagfactor, &Rotation, &Skew);
	ftemp = fabs(Xmagfactor);
	if (ftemp != 1 && ftemp >= (1-aspect_drift) && ftemp <= (1 + aspect_drift))
	{
		Xmagfactor = sign(Xmagfactor);
		convert_corners_bf(bftemp, bftemp2, Magnification, Xmagfactor, Rotation, Skew);
	}

	/* ftemp = fabs(x3rd-xmin); */
	abs_a_bf(sub_bf(bftemp, g_escape_time_state.m_grid_bf.x_3rd(), g_escape_time_state.m_grid_bf.x_min()));

	/* ftemp2 = fabs(xmax-x3rd); */
	abs_a_bf(sub_bf(bftemp2, g_escape_time_state.m_grid_bf.x_max(), g_escape_time_state.m_grid_bf.x_3rd()));

	/* if ((ftemp = fabs(x3rd-xmin)) < (ftemp2 = fabs(xmax-x3rd))) */
	if (cmp_bf(bftemp, bftemp2) < 0)
	{
		/* if (ftemp*10000 < ftemp2 && y3rd != ymax) */
		if (cmp_bf(mult_bf_int(btmp1, bftemp, 10000), bftemp2) < 0
			&& cmp_bf(g_escape_time_state.m_grid_bf.y_3rd(), g_escape_time_state.m_grid_bf.y_max()) != 0)
		{
			/* x3rd = xmin; */
			copy_bf(g_escape_time_state.m_grid_bf.x_3rd(), g_escape_time_state.m_grid_bf.x_min());
		}
	}

	/* else if (ftemp2*10000 < ftemp && y3rd != ymin) */
	if (cmp_bf(mult_bf_int(btmp1, bftemp2, 10000), bftemp) < 0
		&& cmp_bf(g_escape_time_state.m_grid_bf.y_3rd(), g_escape_time_state.m_grid_bf.y_min()) != 0)
	{
		/* x3rd = xmax; */
		copy_bf(g_escape_time_state.m_grid_bf.x_3rd(), g_escape_time_state.m_grid_bf.x_max());
	}

	/* ftemp = fabs(y3rd-ymin); */
	abs_a_bf(sub_bf(bftemp, g_escape_time_state.m_grid_bf.y_3rd(), g_escape_time_state.m_grid_bf.y_min()));

	/* ftemp2 = fabs(ymax-y3rd); */
	abs_a_bf(sub_bf(bftemp2, g_escape_time_state.m_grid_bf.y_max(), g_escape_time_state.m_grid_bf.y_3rd()));

	/* if ((ftemp = fabs(y3rd-ymin)) < (ftemp2 = fabs(ymax-y3rd))) */
	if (cmp_bf(bftemp, bftemp2) < 0)
	{
		/* if (ftemp*10000 < ftemp2 && x3rd != xmax) */
		if (cmp_bf(mult_bf_int(btmp1, bftemp, 10000), bftemp2) < 0
			&& cmp_bf(g_escape_time_state.m_grid_bf.x_3rd(), g_escape_time_state.m_grid_bf.x_max()) != 0)
		{
			/* y3rd = ymin; */
			copy_bf(g_escape_time_state.m_grid_bf.y_3rd(), g_escape_time_state.m_grid_bf.y_min());
		}
	}

	/* else if (ftemp2*10000 < ftemp && x3rd != xmin) */
	if (cmp_bf(mult_bf_int(btmp1, bftemp2, 10000), bftemp) < 0
		&& cmp_bf(g_escape_time_state.m_grid_bf.x_3rd(), g_escape_time_state.m_grid_bf.x_min()) != 0)
	{
		/* y3rd = ymax; */
		copy_bf(g_escape_time_state.m_grid_bf.y_3rd(), g_escape_time_state.m_grid_bf.y_max());
	}


	restore_stack(saved);
}

void adjust_corner_bf()
{
	adjust_corner_bf(g_aspect_drift);
}

void adjust_corner(float aspect_drift)
{
	/* make edges very near vert/horiz exact, to ditch rounding errs and */
	/* to avoid problems when delta per axis makes too large a ratio     */
	double Xctr;
	double Yctr;
	double Xmagfactor;
	double Rotation;
	double Skew;
	LDBL Magnification;

	double ftemp;
	if (!g_integer_fractal)
	{
		/* While we're at it, let's adjust the Xmagfactor as well */
		convert_center_mag(&Xctr, &Yctr, &Magnification, &Xmagfactor, &Rotation, &Skew);
		ftemp = fabs(Xmagfactor);
		if (ftemp != 1 && ftemp >= (1-aspect_drift) && ftemp <= (1 + aspect_drift))
		{
			Xmagfactor = sign(Xmagfactor);
			convert_corners(Xctr, Yctr, Magnification, Xmagfactor, Rotation, Skew);
		}
	}

	ftemp = fabs(g_escape_time_state.m_grid_fp.x_3rd()-g_escape_time_state.m_grid_fp.x_min());
	double ftemp2;
	ftemp2 = fabs(g_escape_time_state.m_grid_fp.x_max()-g_escape_time_state.m_grid_fp.x_3rd());
	if (ftemp < ftemp2)
	{
		if (ftemp*10000 < ftemp2 && g_escape_time_state.m_grid_fp.y_3rd() != g_escape_time_state.m_grid_fp.y_max())
		{
			g_escape_time_state.m_grid_fp.x_3rd() = g_escape_time_state.m_grid_fp.x_min();
		}
	}

	if (ftemp2*10000 < ftemp && g_escape_time_state.m_grid_fp.y_3rd() != g_escape_time_state.m_grid_fp.y_min())
	{
		g_escape_time_state.m_grid_fp.x_3rd() = g_escape_time_state.m_grid_fp.x_max();
	}

	ftemp = fabs(g_escape_time_state.m_grid_fp.y_3rd()-g_escape_time_state.m_grid_fp.y_min());
	ftemp2 = fabs(g_escape_time_state.m_grid_fp.y_max()-g_escape_time_state.m_grid_fp.y_3rd());
	if (ftemp < ftemp2)
	{
		if (ftemp*10000 < ftemp2 && g_escape_time_state.m_grid_fp.x_3rd() != g_escape_time_state.m_grid_fp.x_max())
		{
			g_escape_time_state.m_grid_fp.y_3rd() = g_escape_time_state.m_grid_fp.y_min();
		}
	}

	if (ftemp2*10000 < ftemp && g_escape_time_state.m_grid_fp.x_3rd() != g_escape_time_state.m_grid_fp.x_min())
	{
		g_escape_time_state.m_grid_fp.y_3rd() = g_escape_time_state.m_grid_fp.y_max();
	}

}

void adjust_corner()
{
	adjust_corner(g_aspect_drift);
}

static void adjust_to_limits_bf(double expand)
{
	LDBL limit;
	bf_t bcornerx[4], bcornery[4];
	bf_t blowx, bhighx, blowy, bhighy, blimit, bftemp;
	bf_t bcenterx, bcentery, badjx, badjy, btmp1, btmp2;
	bf_t bexpand;
	int i;
	int saved = save_stack();
	bcornerx[0] = alloc_stack(g_rbf_length + 2);
	bcornerx[1] = alloc_stack(g_rbf_length + 2);
	bcornerx[2] = alloc_stack(g_rbf_length + 2);
	bcornerx[3] = alloc_stack(g_rbf_length + 2);
	bcornery[0] = alloc_stack(g_rbf_length + 2);
	bcornery[1] = alloc_stack(g_rbf_length + 2);
	bcornery[2] = alloc_stack(g_rbf_length + 2);
	bcornery[3] = alloc_stack(g_rbf_length + 2);
	blowx       = alloc_stack(g_rbf_length + 2);
	bhighx      = alloc_stack(g_rbf_length + 2);
	blowy       = alloc_stack(g_rbf_length + 2);
	bhighy      = alloc_stack(g_rbf_length + 2);
	blimit      = alloc_stack(g_rbf_length + 2);
	bftemp      = alloc_stack(g_rbf_length + 2);
	bcenterx    = alloc_stack(g_rbf_length + 2);
	bcentery    = alloc_stack(g_rbf_length + 2);
	badjx       = alloc_stack(g_rbf_length + 2);
	badjy       = alloc_stack(g_rbf_length + 2);
	btmp1       = alloc_stack(g_rbf_length + 2);
	btmp2       = alloc_stack(g_rbf_length + 2);
	bexpand     = alloc_stack(g_rbf_length + 2);

	limit = 32767.99;

	floattobf(blimit, limit);
	floattobf(bexpand, expand);

	add_bf(bcenterx, g_escape_time_state.m_grid_bf.x_min(), g_escape_time_state.m_grid_bf.x_max());
	half_a_bf(bcenterx);

	/* centery = (ymin + ymax)/2; */
	add_bf(bcentery, g_escape_time_state.m_grid_bf.y_min(), g_escape_time_state.m_grid_bf.y_max());
	half_a_bf(bcentery);

	/* if (xmin == centerx) { */
	if (cmp_bf(g_escape_time_state.m_grid_bf.x_min(), bcenterx) == 0)  /* ohoh, infinitely thin, fix it */
	{
		smallest_add_bf(g_escape_time_state.m_grid_bf.x_max());
		/* bfxmin -= bfxmax-centerx; */
		sub_a_bf(g_escape_time_state.m_grid_bf.x_min(), sub_bf(btmp1, g_escape_time_state.m_grid_bf.x_max(), bcenterx));
	}

	/* if (bfymin == centery) */
	if (cmp_bf(g_escape_time_state.m_grid_bf.y_min(), bcentery) == 0)
	{
		smallest_add_bf(g_escape_time_state.m_grid_bf.y_max());
		/* bfymin -= bfymax-centery; */
		sub_a_bf(g_escape_time_state.m_grid_bf.y_min(), sub_bf(btmp1, g_escape_time_state.m_grid_bf.y_max(), bcentery));
	}

	/* if (bfx3rd == centerx) */
	if (cmp_bf(g_escape_time_state.m_grid_bf.x_3rd(), bcenterx) == 0)
	{
		smallest_add_bf(g_escape_time_state.m_grid_bf.x_3rd());
	}

	/* if (bfy3rd == centery) */
	if (cmp_bf(g_escape_time_state.m_grid_bf.y_3rd(), bcentery) == 0)
	{
		smallest_add_bf(g_escape_time_state.m_grid_bf.y_3rd());
	}

	/* setup array for easier manipulation */
	/* cornerx[0] = xmin; */
	copy_bf(bcornerx[0], g_escape_time_state.m_grid_bf.x_min());

	/* cornerx[1] = xmax; */
	copy_bf(bcornerx[1], g_escape_time_state.m_grid_bf.x_max());

	/* cornerx[2] = x3rd; */
	copy_bf(bcornerx[2], g_escape_time_state.m_grid_bf.x_3rd());

	/* cornerx[3] = xmin + (xmax-x3rd); */
	sub_bf(bcornerx[3], g_escape_time_state.m_grid_bf.x_max(), g_escape_time_state.m_grid_bf.x_3rd());
	add_a_bf(bcornerx[3], g_escape_time_state.m_grid_bf.x_min());

	/* cornery[0] = ymax; */
	copy_bf(bcornery[0], g_escape_time_state.m_grid_bf.y_max());

	/* cornery[1] = ymin; */
	copy_bf(bcornery[1], g_escape_time_state.m_grid_bf.y_min());

	/* cornery[2] = y3rd; */
	copy_bf(bcornery[2], g_escape_time_state.m_grid_bf.y_3rd());

	/* cornery[3] = ymin + (ymax-y3rd); */
	sub_bf(bcornery[3], g_escape_time_state.m_grid_bf.y_max(), g_escape_time_state.m_grid_bf.y_3rd());
	add_a_bf(bcornery[3], g_escape_time_state.m_grid_bf.y_min());

	/* if caller wants image size adjusted, do that first */
	if (expand != 1.0)
	{
		for (i = 0; i < 4; ++i)
		{
			/* cornerx[i] = centerx + (cornerx[i]-centerx)*expand; */
			sub_bf(btmp1, bcornerx[i], bcenterx);
			mult_bf(bcornerx[i], btmp1, bexpand);
			add_a_bf(bcornerx[i], bcenterx);

			/* cornery[i] = centery + (cornery[i]-centery)*expand; */
			sub_bf(btmp1, bcornery[i], bcentery);
			mult_bf(bcornery[i], btmp1, bexpand);
			add_a_bf(bcornery[i], bcentery);
		}
	}

	/* get min/max x/y values */
	/* lowx = highx = cornerx[0]; */
	copy_bf(blowx, bcornerx[0]);
	copy_bf(bhighx, bcornerx[0]);

	/* lowy = highy = cornery[0]; */
	copy_bf(blowy, bcornery[0]);
	copy_bf(bhighy, bcornery[0]);

	for (i = 1; i < 4; ++i)
	{
		/* if (cornerx[i] < lowx)               lowx  = cornerx[i]; */
		if (cmp_bf(bcornerx[i], blowx) < 0)
		{
			copy_bf(blowx, bcornerx[i]);
		}

		/* if (cornerx[i] > highx)              highx = cornerx[i]; */
		if (cmp_bf(bcornerx[i], bhighx) > 0)
		{
			copy_bf(bhighx, bcornerx[i]);
		}

		/* if (cornery[i] < lowy)               lowy  = cornery[i]; */
		if (cmp_bf(bcornery[i], blowy) < 0)
		{
			copy_bf(blowy, bcornery[i]);
		}

		/* if (cornery[i] > highy)              highy = cornery[i]; */
		if (cmp_bf(bcornery[i], bhighy) > 0)
		{
			copy_bf(bhighy, bcornery[i]);
		}
	}

	/* if image is too large, downsize it maintaining center */
	/* ftemp = highx-lowx; */
	sub_bf(bftemp, bhighx, blowx);

	/* if (highy-lowy > ftemp) ftemp = highy-lowy; */
	if (cmp_bf(sub_bf(btmp1, bhighy, blowy), bftemp) > 0)
	{
		copy_bf(bftemp, btmp1);
	}

	/* if image is too large, downsize it maintaining center */

	floattobf(btmp1, limit*2.0);
	copy_bf(btmp2, bftemp);
	div_bf(bftemp, btmp1, btmp2);
	floattobf(btmp1, 1.0);
	if (cmp_bf(bftemp, btmp1) < 0)
	{
		for (i = 0; i < 4; ++i)
		{
			/* cornerx[i] = centerx + (cornerx[i]-centerx)*ftemp; */
			sub_bf(btmp1, bcornerx[i], bcenterx);
			mult_bf(bcornerx[i], btmp1, bftemp);
			add_a_bf(bcornerx[i], bcenterx);

			/* cornery[i] = centery + (cornery[i]-centery)*ftemp; */
			sub_bf(btmp1, bcornery[i], bcentery);
			mult_bf(bcornery[i], btmp1, bftemp);
			add_a_bf(bcornery[i], bcentery);
		}
	}

	/* if any corner has x or y past limit, move the image */
	/* adjx = adjy = 0; */
	clear_bf(badjx);
	clear_bf(badjy);

	for (i = 0; i < 4; ++i)
	{
		/* if (cornerx[i] > limit && (ftemp = cornerx[i] - limit) > adjx)
			adjx = ftemp; */
		if (cmp_bf(bcornerx[i], blimit) > 0 &&
			cmp_bf(sub_bf(bftemp, bcornerx[i], blimit), badjx) > 0)
		{
			copy_bf(badjx, bftemp);
		}

		/* if (cornerx[i] < -limit && (ftemp = cornerx[i] + limit) < adjx)
			adjx = ftemp; */
		if (cmp_bf(bcornerx[i], neg_bf(btmp1, blimit)) < 0 &&
			cmp_bf(add_bf(bftemp, bcornerx[i], blimit), badjx) < 0)
		{
			copy_bf(badjx, bftemp);
		}

		/* if (cornery[i] > limit  && (ftemp = cornery[i] - limit) > adjy)
			adjy = ftemp; */
		if (cmp_bf(bcornery[i], blimit) > 0 &&
			cmp_bf(sub_bf(bftemp, bcornery[i], blimit), badjy) > 0)
		{
			copy_bf(badjy, bftemp);
		}

		/* if (cornery[i] < -limit && (ftemp = cornery[i] + limit) < adjy)
			adjy = ftemp; */
		if (cmp_bf(bcornery[i], neg_bf(btmp1, blimit)) < 0 &&
			cmp_bf(add_bf(bftemp, bcornery[i], blimit), badjy) < 0)
		{
			copy_bf(badjy, bftemp);
		}
	}

	/* if (g_calculation_status == CALCSTAT_RESUMABLE && (adjx != 0 || adjy != 0) && (g_z_width == 1.0))
		g_calculation_status = CALCSTAT_PARAMS_CHANGED; */
	if (g_calculation_status == CALCSTAT_RESUMABLE
		&& (is_bf_not_zero(badjx)|| is_bf_not_zero(badjy)) && (g_z_width == 1.0))
	{
		g_calculation_status = CALCSTAT_PARAMS_CHANGED;
	}

	/* xmin = cornerx[0] - adjx; */
	sub_bf(g_escape_time_state.m_grid_bf.x_min(), bcornerx[0], badjx);
	/* xmax = cornerx[1] - adjx; */
	sub_bf(g_escape_time_state.m_grid_bf.x_max(), bcornerx[1], badjx);
	/* x3rd = cornerx[2] - adjx; */
	sub_bf(g_escape_time_state.m_grid_bf.x_3rd(), bcornerx[2], badjx);
	/* ymax = cornery[0] - adjy; */
	sub_bf(g_escape_time_state.m_grid_bf.y_max(), bcornery[0], badjy);
	/* ymin = cornery[1] - adjy; */
	sub_bf(g_escape_time_state.m_grid_bf.y_min(), bcornery[1], badjy);
	/* y3rd = cornery[2] - adjy; */
	sub_bf(g_escape_time_state.m_grid_bf.y_3rd(), bcornery[2], badjy);

	adjust_corner_bf(); /* make 3rd corner exact if very near other co-ords */
	restore_stack(saved);
}

static void adjust_to_limits(double expand)
{
	double cornerx[4];
	double cornery[4];
	double lowx;
	double highx;
	double lowy;
	double highy;
	double limit;
	double ftemp;
	double centerx;
	double centery;
	double adjx;
	double adjy;
	int i;

	limit = 32767.99;

	if (g_integer_fractal)
	{
		limit = 1023.99;
		if (g_bit_shift >= 24)
		{
			limit = 31.99;
		}
		if (g_bit_shift >= 29)
		{
			limit = 3.99;
		}
	}

	centerx = g_escape_time_state.m_grid_fp.x_center();
	centery = g_escape_time_state.m_grid_fp.y_center();

	if (g_escape_time_state.m_grid_fp.x_min() == centerx)  /* ohoh, infinitely thin, fix it */
	{
		smallest_add(&g_escape_time_state.m_grid_fp.x_max());
		g_escape_time_state.m_grid_fp.x_min() -= g_escape_time_state.m_grid_fp.x_max()-centerx;
	}

	if (g_escape_time_state.m_grid_fp.y_min() == centery)
	{
		smallest_add(&g_escape_time_state.m_grid_fp.y_max());
		g_escape_time_state.m_grid_fp.y_min() -= g_escape_time_state.m_grid_fp.y_max()-centery;
	}

	if (g_escape_time_state.m_grid_fp.x_3rd() == centerx)
	{
		smallest_add(&g_escape_time_state.m_grid_fp.x_3rd());
	}

	if (g_escape_time_state.m_grid_fp.y_3rd() == centery)
	{
		smallest_add(&g_escape_time_state.m_grid_fp.y_3rd());
	}

	/* setup array for easier manipulation */
	cornerx[0] = g_escape_time_state.m_grid_fp.x_min();
	cornerx[1] = g_escape_time_state.m_grid_fp.x_max();
	cornerx[2] = g_escape_time_state.m_grid_fp.x_3rd();
	cornerx[3] = g_escape_time_state.m_grid_fp.x_min() + (g_escape_time_state.m_grid_fp.x_max()-g_escape_time_state.m_grid_fp.x_3rd());

	cornery[0] = g_escape_time_state.m_grid_fp.y_max();
	cornery[1] = g_escape_time_state.m_grid_fp.y_min();
	cornery[2] = g_escape_time_state.m_grid_fp.y_3rd();
	cornery[3] = g_escape_time_state.m_grid_fp.y_min() + (g_escape_time_state.m_grid_fp.y_max()-g_escape_time_state.m_grid_fp.y_3rd());

	/* if caller wants image size adjusted, do that first */
	if (expand != 1.0)
	{
		for (i = 0; i < 4; ++i)
		{
			cornerx[i] = centerx + (cornerx[i]-centerx)*expand;
			cornery[i] = centery + (cornery[i]-centery)*expand;
		}
	}
	/* get min/max x/y values */
	lowx = cornerx[0];
	highx = cornerx[0];
	lowy = cornery[0];
	highy = cornery[0];

	for (i = 1; i < 4; ++i)
	{
		if (cornerx[i] < lowx)
		{
			lowx  = cornerx[i];
		}
		if (cornerx[i] > highx)
		{
			highx = cornerx[i];
		}
		if (cornery[i] < lowy)
		{
			lowy  = cornery[i];
		}
		if (cornery[i] > highy)
		{
			highy = cornery[i];
		}
	}

	/* if image is too large, downsize it maintaining center */
	ftemp = highx-lowx;

	if (highy-lowy > ftemp)
	{
		ftemp = highy-lowy;
	}

	/* if image is too large, downsize it maintaining center */
	ftemp = limit*2/ftemp;
	if (ftemp < 1.0)
	{
		for (i = 0; i < 4; ++i)
		{
			cornerx[i] = centerx + (cornerx[i]-centerx)*ftemp;
			cornery[i] = centery + (cornery[i]-centery)*ftemp;
		}
	}

	/* if any corner has x or y past limit, move the image */
	adjx = 0;
	adjy = 0;

	for (i = 0; i < 4; ++i)
	{
		if (cornerx[i] > limit)
		{
			ftemp = cornerx[i] - limit;
			if (ftemp > adjx)
			{
				adjx = ftemp;
			}
		}
		else if (cornerx[i] < -limit)
		{
			ftemp = cornerx[i] + limit;
			if (ftemp < adjx)
			{
				adjx = ftemp;
			}
		}
		if (cornery[i] > limit)
		{
			ftemp = cornery[i] - limit;
			if (ftemp > adjy)
			{
				adjy = ftemp;
			}
		}
		else if (cornery[i] < -limit)
		{
			ftemp = cornery[i] + limit;
			if (ftemp < adjy)
			{
				adjy = ftemp;
			}
		}
	}
	if (g_calculation_status == CALCSTAT_RESUMABLE && (adjx != 0 || adjy != 0) && (g_z_width == 1.0))
	{
		g_calculation_status = CALCSTAT_PARAMS_CHANGED;
	}
	g_escape_time_state.m_grid_fp.x_min() = cornerx[0] - adjx;
	g_escape_time_state.m_grid_fp.x_max() = cornerx[1] - adjx;
	g_escape_time_state.m_grid_fp.x_3rd() = cornerx[2] - adjx;
	g_escape_time_state.m_grid_fp.y_max() = cornery[0] - adjy;
	g_escape_time_state.m_grid_fp.y_min() = cornery[1] - adjy;
	g_escape_time_state.m_grid_fp.y_3rd() = cornery[2] - adjy;

	adjust_corner(); /* make 3rd corner exact if very near other co-ords */
}

static void smallest_add(double *num)
{
	*num += *num*5.0e-16;
}

static void smallest_add_bf(bf_t num)
{
	bf_t btmp1;
	int saved = save_stack();
	btmp1 = alloc_stack(g_bf_length + 2);
	mult_bf(btmp1, floattobf(btmp1, 5.0e-16), num);
	add_a_bf(num, btmp1);
	restore_stack(saved);
}

static int ratio_bad(double actual, double desired)
{
	double ftemp;
	double tol;
	tol = g_math_tolerance[g_integer_fractal ? 0 : 1];
	if (tol <= 0.0)
	{
		return 1;
	}
	else if (tol >= 1.0)
	{
		return 0;
	}
	ftemp = 0;
	if (desired != 0 && g_debug_mode != DEBUGMODE_NO_INT_TO_FLOAT)
	{
		ftemp = actual/desired;
		if (ftemp < (1.0-tol) || ftemp > (1.0 + tol))
		{
			return 1;
		}
	}
	return 0;
}


/* Showing orbit requires converting real co-ords to screen co-ords.
	Define:
		Xs == xmax-x3rd               Ys == y3rd-ymax
		W  == g_x_dots-1                   D  == g_y_dots-1
	We know that:
		realx == lx0(col) + lx1(row)
		realy == ly0(row) + ly1(col)
		lx0(col) == (col/width)*Xs + xmin
		lx1(row) == row*g_delta_x_fp
		ly0(row) == (row/D)*Ys + ymax
		ly1(col) == col*(-g_delta_y_fp)
	so:
		realx == (col/W)*Xs + xmin + row*g_delta_x_fp
		realy == (row/D)*Ys + ymax + col*(-g_delta_y_fp)
	and therefore:
		row == (realx-xmin - (col/W)*Xs)/Xv    (1)
		col == (realy-ymax - (row/D)*Ys)/Yv    (2)
	substitute (2) into (1) and solve for row:
		row == ((realx-xmin)*(-g_delta_y2_fp)*W*D - (realy-ymax)*Xs*D)
						/((-g_delta_y2_fp)*W*g_delta_x2_fp*D-Ys*Xs)
*/

/* sleep N*a tenth of a millisecond */

static void sleep_ms_old(long ms)
{
	static long scalems = 0L;
	struct timebx t1, t2;
	int const SLEEPINIT = 250; /* milliseconds for calibration */
	bool save_tab_display_enabled = g_tab_display_enabled;
	HelpModeSaver saved_help(-1);
	g_tab_display_enabled  = false;
	if (scalems == 0L) /* calibrate */
	{
		/* selects a value of scalems that makes the units
			10000 per sec independent of CPU speed */
		int i;
		int elapsed;
		scalems = 1L;
		if (driver_key_pressed()) /* check at start, hope to get start of timeslice */
		{
			goto sleepexit;
		}
		/* calibrate, assume slow computer first */
		show_temp_message("Calibrating timer");
		do
		{
			scalems *= 2;
			ftimex(&t2);
			do  /* wait for the start of a new tick */
			{
				ftimex(&t1);
			}
			while (t2.time == t1.time && t2.millitm == t1.millitm);
			sleep_ms_old(10L*SLEEPINIT); /* about 1/4 sec */
			ftimex(&t2);
			if (driver_key_pressed())
			{
				scalems = 0L;
				clear_temp_message();
				goto sleepexit;
			}
			elapsed = int(t2.time-t1.time)*1000 + t2.millitm-t1.millitm;
		}
		while (elapsed < SLEEPINIT);
		/* once more to see if faster (eg multi-tasking) */
		do  /* wait for the start of a new tick */
		{
			ftimex(&t1);
		}
		while (t2.time == t1.time && t2.millitm == t1.millitm);
		sleep_ms_old(10L*SLEEPINIT);
		ftimex(&t2);
		i = int(t2.time-t1.time)*1000 + t2.millitm-t1.millitm;
		if (i < elapsed)
		{
			elapsed = (i == 0) ? 1 : i;
		}
		scalems = long(float(SLEEPINIT)/float(elapsed)*scalems);
		clear_temp_message();
	}
	if (ms > 10L*SLEEPINIT)  /* using ftime is probably more accurate */
	{
		ms /= 10;
		ftimex(&t1);
		while (true)
		{
			if (driver_key_pressed())
			{
				break;
			}
			ftimex(&t2);
			if (long((t2.time-t1.time)*1000 + t2.millitm-t1.millitm) >= ms)
			{
				break;
			}
		}
	}
	else if (!driver_key_pressed())
	{
		ms *= scalems;
		while (ms-- >= 0)
		{
		}
	}
sleepexit:
	g_tab_display_enabled  = save_tab_display_enabled;
}

static void sleep_ms_new(long ms)
{
	uclock_t next_time;
	uclock_t now = usec_clock();
	next_time = now + ms*100;
	while ((now = usec_clock()) < next_time)
	{
		if (driver_key_pressed())
		{
			break;
		}
	}
}

void sleep_ms(long ms)
{
	if (DEBUGMODE_OLD_TIMER == g_debug_mode)
	{
		sleep_ms_old(ms);
	}
	else
	{
		sleep_ms_new(ms);
	}
}

/*
* wait until wait_time microseconds from the
* last call has elapsed.
*/
enum
{
	MAX_INDEX = 2
};
static uclock_t next_time[MAX_INDEX];
void wait_until(int index, uclock_t wait_time)
{
	if (DEBUGMODE_OLD_TIMER == g_debug_mode)
	{
		sleep_ms_old(wait_time);
	}
	else
	{
		uclock_t now;
		while ((now = usec_clock()) < next_time[index])
		{
			if (driver_key_pressed())
			{
				break;
			}
		}
		next_time[index] = now + wait_time*100; /* wait until this time next call */
	}
}

void reset_clock()
{
	int i;
	restart_uclock();
	for (i = 0; i < MAX_INDEX; i++)
	{
		next_time[i] = 0;
	}
}

static float const LOG2 = 0.693147180f;
static float const LOG32 = 3.465735902f;

static void plot_orbit_d(double dx, double dy, int color)
{
	if (g_orbit_index >= 1500-3)
	{
		return;
	}
	int i = int(dy*g_plot_mx1 - dx*g_plot_mx2) + g_screen_x_offset;
	if (i < 0 || i >= g_screen_width)
	{
		return;
	}
	int j = int(dx*g_plot_my1 - dy*g_plot_my2) + g_screen_y_offset;
	if (j < 0 || j >= g_screen_height)
	{
		return;
	}

	{
		ValueSaver<int> save_sx_offset(g_screen_x_offset, 0);
		ValueSaver<int> save_sy_offset(g_screen_y_offset, 0);
		/* save orbit value */
		if (color == -1)
		{
			*(s_save_orbit + g_orbit_index++) = i;
			*(s_save_orbit + g_orbit_index++) = j;
			int c = get_color(i, j);
			*(s_save_orbit + g_orbit_index++) = c;
			g_plot_color_put_color(i, j, c ^ g_orbit_color);
		}
		else
		{
			g_plot_color_put_color(i, j, color);
		}
	}
	g_sound_state.orbit(i, j);
	/* placing sleep_ms here delays each dot */
}

void plot_orbit_i(long ix, long iy, int color)
{
	plot_orbit_d(double(ix)/g_fudge - g_escape_time_state.m_grid_fp.x_min(),
		double(iy)/g_fudge - g_escape_time_state.m_grid_fp.y_max(),
		color);
}

void plot_orbit(double real, double imag, int color)
{
	plot_orbit_d(real - g_escape_time_state.m_grid_fp.x_min(),
		imag - g_escape_time_state.m_grid_fp.y_max(),
		color);
}

void orbit_scrub()
{
	driver_mute();
	ValueSaver<int> save_sx_offset(g_screen_x_offset, 0);
	ValueSaver<int> save_sy_offset(g_screen_y_offset, 0);
	while (g_orbit_index >= 3)
	{
		int c = *(s_save_orbit + --g_orbit_index);
		int j = *(s_save_orbit + --g_orbit_index);
		int i = *(s_save_orbit + --g_orbit_index);
		g_plot_color_put_color(i, j, c);
	}
}

void get_julia_attractor(double real, double imag)
{
	if (g_num_attractors == 0 && g_finite_attractor == FINITE_ATTRACTOR_NO) /* not magnet & not requested */
	{
		return;
	}

	if (g_num_attractors >= MAX_NUM_ATTRACTORS)     /* space for more attractors ?  */
	{
		return;                  /* Bad luck - no room left !    */
	}

	int save_periodicity_check = g_periodicity_check;
	ValueSaver<long> save_max_iteration(g_max_iteration, g_max_iteration);
	g_periodicity_check = 0;
	g_old_z.x = real;                    /* prepare for f.p orbit calc */
	g_old_z.y = imag;
	g_temp_sqr_x = sqr(g_old_z.x);
	g_temp_sqr_y = sqr(g_old_z.y);

	g_old_z_l.x = (long(real)) << g_bit_shift;		/* prepare for int orbit calc */
	g_old_z_l.y = (long(imag)) << g_bit_shift;
	g_temp_sqr_x_l = (long(g_temp_sqr_x)) << g_bit_shift;
	g_temp_sqr_y_l = (long(g_temp_sqr_y)) << g_bit_shift;

	if (g_max_iteration < 500)         /* we're going to try at least this hard */
	{
		g_max_iteration = 500;
	}
	g_color_iter = 0;
	g_overflow = false;
	while (++g_color_iter < g_max_iteration)
	{
		if (g_current_fractal_specific->orbitcalc() || g_overflow)
		{
			break;
		}
	}
	if (g_color_iter >= g_max_iteration)      /* if orbit stays in the lake */
	{
		ComplexL result_l;
		ComplexD result;
		if (g_integer_fractal)   /* remember where it went to */
		{
			result_l = g_new_z_l;
		}
		else
		{
			result =  g_new_z;
		}
		for (int i = 0; i < 10; i++)
		{
			g_overflow = false;
			if (!g_current_fractal_specific->orbitcalc() && !g_overflow)
			/* if it stays in the lake and doesn't move far, probably found a finite attractor */
			{
				if (g_integer_fractal)
				{
					if (labs(result_l.x - g_new_z_l.x) < g_close_enough_l
						&& labs(result_l.y - g_new_z_l.y) < g_close_enough_l)
					{
						g_attractors_l[g_num_attractors] = g_new_z_l;
						g_attractor_period[g_num_attractors] = i + 1;
						g_num_attractors++;   /* another attractor - coloured lakes ! */
						break;
					}
				}
				else
				{
					if (fabs(result.x - g_new_z.x) < g_close_enough
						&& fabs(result.y - g_new_z.y) < g_close_enough)
					{
						g_attractors[g_num_attractors] = g_new_z;
						g_attractor_period[g_num_attractors] = i + 1;
						g_num_attractors++;   /* another attractor - coloured lakes ! */
						break;
					}
				}
			}
			else
			{
				break;
			}
		}
	}
	if (g_num_attractors == 0)
	{
		g_periodicity_check = save_periodicity_check;
	}
}

int solid_guess_block_size(int width, int height) /* used by solidguessing and by zoom panning */
{
	/* blocksize 4 if <300 rows, 8 if 300-599, 16 if 600-1199, 32 if >= 1200 */
	int blocksize = 4;
	int i = 300;
	while (i <= height)
	{
		blocksize *= 2;
		i *= 2;
	}
	/* increase blocksize if prefix array not big enough */
	while (blocksize*(MAX_X_BLOCK - 2) < width || blocksize*(MAX_Y_BLOCK - 2)*16 < height)
	{
		blocksize *= 2;
	}
	return blocksize;
}

int solid_guess_block_size()
{
	return solid_guess_block_size(g_x_dots, g_y_dots);
}
