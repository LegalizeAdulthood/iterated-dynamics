#include <limits.h>
#include <string.h>
#include <string>

#include "port.h"
#include "prototyp.h"
#include "helpdefs.h"
#include "fractype.h"

#include "calcfrac.h"
#include "Externals.h"
#include "FiniteAttractor.h"
#include "fracsubr.h"
#include "fractals.h"
#include "frasetup.h"
#include "jb.h"
#include "mpmath.h"
#include "SoundState.h"

long calculate_mandelbrot_fp_asm();

// --------------------------------------------------------------------
// Setup (once per fractal image) routines
// --------------------------------------------------------------------

bool mandelbrot_setup()           // Mandelbrot Routine
{
	if (g_debug_mode != DEBUGMODE_NO_ASM_MANDEL
		&& !g_invert
		&& g_decomposition[0] == 0
		&& g_rq_limit == 4.0
		&& g_bit_shift == 29
		&& !g_potential_flag
		&& g_externs.Biomorph() == BIOMORPH_NONE
		&& g_externs.Inside() > COLORMODE_Z_MAGNITUDE
		&& g_externs.Outside() >= COLORMODE_ITERATION
		&& g_externs.UseInitialOrbitZ() != INITIALZ_ORBIT
		&& !g_using_jiim
		&& g_externs.BailOutTest() == BAILOUT_MODULUS
		&& (g_orbit_save & ORBITSAVE_SOUND) == 0)
	{
		g_calculate_type = calculate_mandelbrot_l; // the normal case - use CALCMAND
	}
	else
	{
		// special case: use the main processing loop
		g_calculate_type = standard_fractal;
		g_long_parameter = &g_initial_z_l;
	}
	return true;
}

bool julia_setup()            // Julia Routine
{
	if (g_debug_mode != DEBUGMODE_NO_ASM_MANDEL
		&& !g_invert
		&& g_decomposition[0] == 0
		&& g_rq_limit == 4.0
		&& g_bit_shift == 29
		&& !g_potential_flag
		&& g_externs.Biomorph() == BIOMORPH_NONE
		&& g_externs.Inside() > COLORMODE_Z_MAGNITUDE
		&& g_externs.Outside() >= COLORMODE_ITERATION
		&& (g_finite_attractor == FINITE_ATTRACTOR_NO)
		&& !g_using_jiim
		&& g_externs.BailOutTest() == BAILOUT_MODULUS
		&& (g_orbit_save & ORBITSAVE_SOUND) == 0)
	{
		g_calculate_type = calculate_mandelbrot_l; // the normal case - use CALCMAND
	}
	else
	{
		// special case: use the main processing loop
		g_calculate_type = standard_fractal;
		g_long_parameter = &g_parameter_l;
		get_julia_attractor(0.0, 0.0);   // another attractor?
	}
	return true;
}

bool stand_alone_setup()
{
	timer_engine(g_current_fractal_specific->calculate_type);
	return false;           // effectively disable solid-guessing
}

bool unity_setup()
{
	g_periodicity_check = 0;
	g_one_fudge = (1L << g_bit_shift);
	g_two_fudge = g_one_fudge + g_one_fudge;
	return true;
}

bool mandelbrot_setup_fp()
{
	g_bf_math = 0;
	g_c_exp = int(g_parameters[P2_REAL]);
	g_power.x = g_parameters[P2_REAL] - 1.0;
	g_power.y = g_parameters[P2_IMAG];
	g_float_parameter = &g_initial_z;
	switch (g_fractal_type)
	{
	case FRACTYPE_MARKS_MANDELBROT_FP:
		if (g_c_exp < 1)
		{
			g_c_exp = 1;
			g_parameters[P2_REAL] = 1;
		}
		if (!(g_c_exp & 1))
		{
			g_symmetry = SYMMETRY_XY_AXIS_NO_PARAMETER;    // odd exponents
		}
		if (g_c_exp & 1)
		{
			g_symmetry = SYMMETRY_X_AXIS_NO_PARAMETER;
		}
		break;
	case FRACTYPE_MANDELBROT_FP:
		//
		// floating point code could probably be altered to handle many of
		// the situations that otherwise are using standard_fractal().
		// calculate_mandelbrot_fp() can currently handle invert, any g_rq_limit, g_potential_flag
		// zmag, epsilon cross, and all the current outside options
		//											Wes Loewer 11/03/91
		// Took out support for inside= options, for speed. 7/13/97
		//
		if (g_debug_mode != DEBUGMODE_NO_ASM_MANDEL
			&& !g_distance_test
			&& g_decomposition[0] == 0
			&& g_externs.Biomorph() == BIOMORPH_NONE
			&& (g_externs.Inside() >= COLORMODE_ITERATION)
			// uncomment this next line if more outside options are added
			&& g_externs.Outside() >= COLORMODE_INVERSE_TANGENT
			&& g_externs.UseInitialOrbitZ() != INITIALZ_ORBIT
			&& (g_sound_state.flags() & SOUNDFLAG_ORBITMASK) < SOUNDFLAG_X
			&& !g_using_jiim
			&& g_externs.BailOutTest() == BAILOUT_MODULUS
			&& (g_orbit_save & ORBITSAVE_SOUND) == 0)
		{
			g_calculate_type = calculate_mandelbrot_fp; // the normal case - use calculate_mandelbrot_fp
			g_calculate_mandelbrot_asm_fp = calculate_mandelbrot_fp_asm;
		}
		else
		{
			// special case: use the main processing loop
			g_calculate_type = standard_fractal;
		}
		break;
	case FRACTYPE_MANDELBROT_Z_POWER_FP:
		if (double(g_c_exp) == g_parameters[P2_REAL] && (g_c_exp & 1)) // odd exponents
		{
			g_symmetry = SYMMETRY_XY_AXIS_NO_PARAMETER;
		}
		if (g_parameters[P2_IMAG] != 0)
		{
			g_symmetry = SYMMETRY_NONE;
		}
		standard_4d_fractal_set_orbit_calc(z_power_orbit_fp, complex_z_power_orbit_fp);
		break;
	case FRACTYPE_MAGNET_1M:
	case FRACTYPE_MAGNET_2M:
		g_attractors[0].x = 1.0;      // 1.0 + 0.0i always attracts
		g_attractors[0].y = 0.0;      // - both MAGNET1 and MAGNET2
		g_attractor_period[0] = 1;
		g_num_attractors = 1;
		break;
	case FRACTYPE_SPIDER_FP:
		if (g_periodicity_check == 1) // if not user set
		{
			g_periodicity_check = 4;
		}
		break;
	case FRACTYPE_OBSOLETE_MANDELBROT_EXP:
		g_symmetry = SYMMETRY_X_AXIS_NO_PARAMETER;
		break;
// Added to account for symmetry in manfn + exp and manfn + zsqrd

	case FRACTYPE_MANDELBROT_FUNC_PLUS_EXP_FP:
	case FRACTYPE_MANDELBROT_FUNC_PLUS_Z_SQUARED_FP:
		g_symmetry = (g_parameter.imag() == 0.0) ? SYMMETRY_X_AXIS : SYMMETRY_NONE;
		if ((g_function_index[0] == FUNCTION_LOG) || (g_function_index[0] == FUNCTION_FLIP))
		{
			g_symmetry = SYMMETRY_NONE;
		}
		break;
	case FRACTYPE_QUATERNION_FP:
		g_float_parameter = &g_temp_z;
		g_num_attractors = 0;
		g_periodicity_check = 0;
		break;
	case FRACTYPE_HYPERCOMPLEX_FP:
		g_float_parameter = &g_temp_z;
		g_num_attractors = 0;
		g_periodicity_check = 0;
		if (g_parameters[P2_REAL] != 0)
		{
			g_symmetry = SYMMETRY_NONE;
		}
		if (g_function_index[0] == FUNCTION_FLIP)
		{
			g_symmetry = SYMMETRY_NONE;
		}
		break;
	case FRACTYPE_TIMS_ERROR_FP:
		if (g_function_index[0] == FUNCTION_FLIP)
		{
			g_symmetry = SYMMETRY_NONE;
		}
		break;
	case FRACTYPE_MARKS_MANDELBROT_POWER_FP:
		if (g_function_index[0] == FUNCTION_FLIP)
		{
			g_symmetry = SYMMETRY_NONE;
		}
		break;
	default:
		break;
	}
	return true;
}

void current_fractal_specific_set_orbit_calc(int (*orbit_calc)(void))
{
	if (orbit_calc != 0)
	{
		g_current_fractal_specific->orbitcalc = orbit_calc;
	}
}

void current_fractal_specific_set_per_pixel(int (*per_pixel)(void))
{
	if (per_pixel != 0)
	{
		g_current_fractal_specific->per_pixel = per_pixel;
	}
}

bool julia_setup_fp()
{
	g_c_exp = int(g_parameters[P2_REAL]);
	g_float_parameter = &g_parameter;
	if (g_fractal_type == FRACTYPE_MARKS_JULIA_COMPLEX)
	{
		g_power.x = g_parameters[P2_REAL] - 1.0;
		g_power.y = g_parameters[P2_IMAG];
		g_coefficient = ComplexPower(*g_float_parameter, g_power);
	}
	switch (g_fractal_type)
	{
	case FRACTYPE_JULIA_FP:
		//
		// floating point code could probably be altered to handle many of
		// the situations that otherwise are using standard_fractal().
		// calculate_mandelbrot_fp() can currently handle invert, any g_rq_limit, g_potential_flag
		// zmag, epsilon cross, and all the current outside options
		//											Wes Loewer 11/03/91
		//
		if (g_debug_mode != DEBUGMODE_NO_ASM_MANDEL
			&& !g_distance_test
			&& g_decomposition[0] == 0
			&& g_externs.Biomorph() == BIOMORPH_NONE
			&& (g_externs.Inside() >= COLORMODE_ITERATION)
			// uncomment this next line if more outside options are added
			&& g_externs.Outside() >= COLORMODE_INVERSE_TANGENT
			&& g_externs.UseInitialOrbitZ() != INITIALZ_ORBIT
			&& (g_sound_state.flags() & SOUNDFLAG_ORBITMASK) < SOUNDFLAG_X
			&& (g_finite_attractor == FINITE_ATTRACTOR_NO)
			&& !g_using_jiim
			&& g_externs.BailOutTest() == BAILOUT_MODULUS
			&& (g_orbit_save & ORBITSAVE_SOUND) == 0)
		{
			g_calculate_type = calculate_mandelbrot_fp; // the normal case - use calculate_mandelbrot_fp
			g_calculate_mandelbrot_asm_fp = calculate_mandelbrot_fp_asm;
		}
		else
		{
			// special case: use the main processing loop
			g_calculate_type = standard_fractal;
			get_julia_attractor(0.0, 0.0);   // another attractor?
		}
		break;
	case FRACTYPE_JULIA_Z_POWER_FP:
		if ((g_c_exp & 1) || g_parameters[P2_IMAG] != 0.0 || double(g_c_exp) != g_parameters[P2_REAL])
		{
			g_symmetry = SYMMETRY_NONE;
		}
		standard_4d_fractal_set_orbit_calc(z_power_orbit_fp, complex_z_power_orbit_fp);
		get_julia_attractor(g_parameters[P1_REAL], g_parameters[P1_IMAG]); // another attractor?
		break;
	case FRACTYPE_MAGNET_2J:
		magnet2_precalculate_fp();
	case FRACTYPE_MAGNET_1J:
		g_attractors[0].x = 1.0;      // 1.0 + 0.0i always attracts
		g_attractors[0].y = 0.0;      // - both MAGNET1 and MAGNET2
		g_attractor_period[0] = 1;
		g_num_attractors = 1;
		get_julia_attractor(0.0, 0.0);   // another attractor?
		break;
	case FRACTYPE_LAMBDA_FP:
		get_julia_attractor(0.0, 0.0);   // another attractor?
		get_julia_attractor(0.5, 0.0);   // another attractor?
		break;
	// TODO: should this really be here?
	case FRACTYPE_OBSOLETE_LAMBDA_EXP:
		if (g_parameter.imag() == 0.0)
		{
			g_symmetry = SYMMETRY_X_AXIS;
		}
		get_julia_attractor(0.0, 0.0);   // another attractor?
		break;
	// Added to account for symmetry in julfn + exp and julfn + zsqrd

	case FRACTYPE_JULIA_FUNC_PLUS_EXP_FP:
	case FRACTYPE_JULIA_FUNC_PLUS_Z_SQUARED_FP:
		g_symmetry = (g_parameter.imag() == 0.0) ? SYMMETRY_X_AXIS : SYMMETRY_NONE;
		if ((g_function_index[0] == FUNCTION_LOG) || (g_function_index[0] == FUNCTION_FLIP))
		{
			g_symmetry = SYMMETRY_NONE;
		}
		get_julia_attractor(0.0, 0.0);   // another attractor?
		break;
	case FRACTYPE_HYPERCOMPLEX_JULIA_FP:
		if (g_parameters[P2_REAL] != 0)
		{
			g_symmetry = SYMMETRY_NONE;
		}
		if (g_function_index[0] != FUNCTION_SQR)
		{
			g_symmetry = SYMMETRY_NONE;
		}
	case FRACTYPE_QUATERNION_JULIA_FP:
		g_num_attractors = 0;   // attractors broken since code checks r, i not j, k
		g_periodicity_check = 0;
		if (g_parameters[P3_REAL] != 0.0 || g_parameters[P3_IMAG] != 0)
		{
			g_symmetry = SYMMETRY_NONE;
		}
		break;
	case FRACTYPE_POPCORN_FP:
	case FRACTYPE_POPCORN_JULIA_FP:
		{
			bool default_functions = false;
			if (g_function_index[0] == FUNCTION_SIN &&
				g_function_index[1] == FUNCTION_TAN &&
				g_function_index[2] == FUNCTION_SIN &&
				g_function_index[3] == FUNCTION_TAN &&
				fabs(g_parameter2.real() - 3.0) < .0001 &&
				g_parameter2.imag() == 0 &&
				g_parameter.imag() == 0)
			{
				default_functions = true;
				if (g_fractal_type == FRACTYPE_POPCORN_JULIA_FP)
				{
					g_symmetry = SYMMETRY_ORIGIN;
				}
			}
			int (*orbit_calc)(void) = popcorn_fn_orbit_fp;
			if (default_functions && DEBUGMODE_REAL_POPCORN == g_debug_mode)
			{
				orbit_calc = popcorn_orbit_fp;
			}
			// TODO: don't write to g_current_fractal_specific
			current_fractal_specific_set_orbit_calc(orbit_calc);
			get_julia_attractor(0.0, 0.0);   // another attractor?
		}
		break;
	case FRACTYPE_CIRCLE_FP:
		if (g_externs.Inside() == COLORMODE_STAR_TRAIL) // FRACTYPE_CIRCLE_FP locks up when used with STARTRAIL
		{
			g_externs.SetInside(0); // arbitrarily set inside = NUMB
		}
		get_julia_attractor(0.0, 0.0);   // another attractor?
		break;
	default:
		get_julia_attractor(0.0, 0.0);   // another attractor?
		break;
	}
	return true;
}

bool mandelbrot_setup_l()
{
	g_c_exp = int(g_parameters[P2_REAL]);
	if (g_fractal_type == FRACTYPE_MARKS_MANDELBROT && g_c_exp < 1)
	{
		g_c_exp = 1;
		g_parameters[P2_REAL] = 1;
	}
	if ((g_fractal_type == FRACTYPE_MARKS_MANDELBROT   && !(g_c_exp & 1)) ||
		(g_fractal_type == FRACTYPE_MANDELBROT_Z_POWER_L && (g_c_exp & 1)))
	{
		g_symmetry = SYMMETRY_XY_AXIS_NO_PARAMETER;    // odd exponents
	}
	if ((g_fractal_type == FRACTYPE_MARKS_MANDELBROT && (g_c_exp & 1)) || g_fractal_type == FRACTYPE_OBSOLETE_MANDELBROT_EXP_L)
	{
		g_symmetry = SYMMETRY_X_AXIS_NO_PARAMETER;
	}
	g_long_parameter = &g_initial_z_l;
	if (g_fractal_type == FRACTYPE_MANDELBROT_Z_POWER_L)
	{
		standard_4d_fractal_set_orbit_calc(z_power_orbit, complex_z_power_orbit);
		if (g_parameters[P2_IMAG] != 0 || double(g_c_exp) != g_parameters[P2_REAL])
		{
			g_symmetry = SYMMETRY_NONE;
		}
	}
	// Added to account for symmetry in manfn + exp and manfn + zsqrd

	if ((g_fractal_type == FRACTYPE_MANDELBROT_FUNC_PLUS_EXP_L) || (g_fractal_type == FRACTYPE_MANDELBROT_FUNC_PLUS_Z_SQUARED_L))
	{
		g_symmetry = (g_parameter.imag() == 0.0) ? SYMMETRY_X_AXIS : SYMMETRY_NONE;
		if ((g_function_index[0] == FUNCTION_LOG) || (g_function_index[0] == FUNCTION_FLIP))
		{
			g_symmetry = SYMMETRY_NONE;
		}
	}
	if (g_fractal_type == FRACTYPE_TIMS_ERROR)
	{
		if (g_function_index[0] == FUNCTION_FLIP)
		{
			g_symmetry = SYMMETRY_NONE;
		}
	}
	if (g_fractal_type == FRACTYPE_MARKS_MANDELBROT_POWER)
	{
		if (g_function_index[0] == FUNCTION_FLIP)
		{
			g_symmetry = SYMMETRY_NONE;
		}
	}
	return true;
}

bool spider_setup_l()
{
	if (!mandelbrot_setup_l())
	{
		return false;
	}

	if (g_periodicity_check == 1)
	{
		g_periodicity_check = 4;
	}
	return true;
}

bool julia_setup_l()
{
	g_c_exp = int(g_parameters[P2_REAL]);
	g_long_parameter = &g_parameter_l;
	switch (g_fractal_type)
	{
	case FRACTYPE_JULIA_Z_POWER_L:
		if ((g_c_exp & 1) || g_parameters[P2_IMAG] != 0.0 || double(g_c_exp) != g_parameters[P2_REAL])
		{
			g_symmetry = SYMMETRY_NONE;
		}
		standard_4d_fractal_set_orbit_calc(z_power_orbit, complex_z_power_orbit);
		break;
	case FRACTYPE_LAMBDA:
		get_julia_attractor(0.0, 0.0);   // another attractor?
		get_julia_attractor(0.5, 0.0);   // another attractor?
		break;
	case FRACTYPE_OBSOLETE_LAMBDA_EXP_L:
		if (g_parameter_l.imag() == 0)
		{
			g_symmetry = SYMMETRY_X_AXIS;
		}
		break;
	// Added to account for symmetry in julfn + exp and julfn + zsqrd

	case FRACTYPE_JULIA_FUNC_PLUS_EXP_L:
	case FRACTYPE_JULIA_FUNC_PLUS_Z_SQUARED_L:
		g_symmetry = (g_parameter.imag() == 0.0) ? SYMMETRY_X_AXIS : SYMMETRY_NONE;
		if ((g_function_index[0] == FUNCTION_LOG) || (g_function_index[0] == FUNCTION_FLIP))
		{
			g_symmetry = SYMMETRY_NONE;
		}
		get_julia_attractor(0.0, 0.0);   // another attractor?
		break;
	case FRACTYPE_POPCORN_L:
	case FRACTYPE_POPCORN_JULIA_L:
		{
			bool default_functions = false;
			if (g_function_index[0] == FUNCTION_SIN &&
				g_function_index[1] == FUNCTION_TAN &&
				g_function_index[2] == FUNCTION_SIN &&
				g_function_index[3] == FUNCTION_TAN &&
				fabs(g_parameter2.real() - 3.0) < .0001 &&
				g_parameter2.imag() == 0 &&
				g_parameter.imag() == 0)
			{
				default_functions = true;
				if (g_fractal_type == FRACTYPE_POPCORN_JULIA_L)
				{
					g_symmetry = SYMMETRY_ORIGIN;
				}
			}
			int (*orbit_calc)(void) = popcorn_fn_orbit;
			if (default_functions && DEBUGMODE_REAL_POPCORN == g_debug_mode)
			{
				orbit_calc = popcorn_orbit;
			}
			// TODO: don't write to g_current_fractal_specific
			current_fractal_specific_set_orbit_calc(orbit_calc);
			get_julia_attractor(0.0, 0.0);   // another attractor?
		}
		break;
	default:
		get_julia_attractor(0.0, 0.0);   // another attractor?
		break;
	}
	return true;
}

bool trig_plus_sqr_setup_l()
{
	int (*per_pixel)(void) = julia_per_pixel;
	int (*orbit_calc)(void) = trig_plus_sqr_orbit;
	if (g_parameter_l.real() == g_externs.Fudge() && g_parameter_l.imag() == 0L && g_parameter2_l.imag() == 0L
		&& g_debug_mode != DEBUGMODE_NO_ASM_MANDEL)
	{
		if (g_parameter2_l.real() == g_externs.Fudge())        // Scott variant
		{
			orbit_calc =  scott_trig_plus_sqr_orbit;
		}
		else if (g_parameter2_l.real() == -g_externs.Fudge())  // Skinner variant
		{
			orbit_calc =  skinner_trig_sub_sqr_orbit;
		}
	}
	// TODO: don't write to g_current_fractal_specific
	current_fractal_specific_set_per_pixel(per_pixel);
	current_fractal_specific_set_orbit_calc(orbit_calc);
	return julia_setup_l();
}

bool trig_plus_sqr_setup_fp()
{
	int (*per_pixel)(void) = julia_per_pixel_fp;
	int (*orbit_calc)(void) = trig_plus_sqr_orbit_fp;
	if (g_parameter.real() == 1.0 && g_parameter.imag() == 0.0 && g_parameter2.imag() == 0.0
		&& g_debug_mode != DEBUGMODE_NO_ASM_MANDEL)
	{
		if (g_parameter2.real() == 1.0)        // Scott variant
		{
			orbit_calc =  scott_trig_plus_sqr_orbit_fp;
		}
		else if (g_parameter2.real() == -1.0)  // Skinner variant
		{
			orbit_calc =  skinner_trig_sub_sqr_orbit_fp;
		}
	}
	// TODO: don't write to g_current_fractal_specific
	current_fractal_specific_set_per_pixel(per_pixel);
	current_fractal_specific_set_orbit_calc(orbit_calc);
	return julia_setup_fp();
}

static int fn_plus_fn_symmetry() // set symmetry matrix for fn + fn type
{
	static SymmetryType fnplusfn[7][7] =
	{	// fn2 -> sin				cos					sinh				cosh				exp					log					sqr
	// fn1
	// sin 	{ SYMMETRY_PI,		SYMMETRY_X_AXIS,	SYMMETRY_XY_AXIS,	SYMMETRY_X_AXIS,	SYMMETRY_X_AXIS,	SYMMETRY_X_AXIS,	SYMMETRY_X_AXIS },
	// cos 	{ SYMMETRY_X_AXIS,	SYMMETRY_PI,		SYMMETRY_X_AXIS,	SYMMETRY_XY_AXIS,	SYMMETRY_X_AXIS,	SYMMETRY_X_AXIS,	SYMMETRY_X_AXIS },
	// sinh	{ SYMMETRY_XY_AXIS, SYMMETRY_X_AXIS,	SYMMETRY_XY_AXIS,	SYMMETRY_X_AXIS,	SYMMETRY_X_AXIS,	SYMMETRY_X_AXIS,	SYMMETRY_X_AXIS },
	// cosh	{ SYMMETRY_X_AXIS,	SYMMETRY_XY_AXIS,	SYMMETRY_X_AXIS,	SYMMETRY_XY_AXIS,	SYMMETRY_X_AXIS,	SYMMETRY_X_AXIS,	SYMMETRY_X_AXIS },
	// exp 	{ SYMMETRY_X_AXIS,	SYMMETRY_XY_AXIS,	SYMMETRY_X_AXIS,	SYMMETRY_X_AXIS,	SYMMETRY_XY_AXIS,	SYMMETRY_X_AXIS,	SYMMETRY_X_AXIS },
	// log 	{ SYMMETRY_X_AXIS,	SYMMETRY_X_AXIS,	SYMMETRY_X_AXIS,	SYMMETRY_X_AXIS,	SYMMETRY_X_AXIS,	SYMMETRY_X_AXIS,	SYMMETRY_X_AXIS },
	// sqr 	{ SYMMETRY_X_AXIS,	SYMMETRY_X_AXIS,	SYMMETRY_X_AXIS,	SYMMETRY_X_AXIS,	SYMMETRY_X_AXIS,	SYMMETRY_X_AXIS,	SYMMETRY_XY_AXIS }
	};
	if (g_parameter.imag() == 0.0 && g_parameter2.imag() == 0.0)
	{
		if (g_function_index[0] <= NUM_OF(fnplusfn) && g_function_index[1] <= NUM_OF(fnplusfn))
		{
			g_symmetry = fnplusfn[g_function_index[0]][g_function_index[1]];
		}
		else if (g_function_index[0] == FUNCTION_FLIP || g_function_index[1] == FUNCTION_FLIP)
		{
			g_symmetry = SYMMETRY_NONE;
		}
	}                 // defaults to SYMMETRY_X_AXIS symmetry
	else
	{
		g_symmetry = SYMMETRY_NONE;
	}
	return 0;
}

bool trig_plus_trig_setup_l()
{
	fn_plus_fn_symmetry();
	if (g_function_index[1] == FUNCTION_SQR)
	{
		return trig_plus_sqr_setup_l();
	}
	int (*per_pixel)(void) = julia_per_pixel_l;
	int (*orbit_calc)(void) =  trig_plus_trig_orbit;
	if (g_parameter_l.real() == g_externs.Fudge()
		&& g_parameter_l.imag() == 0L
		&& g_parameter2_l.imag() == 0L
		&& g_debug_mode != DEBUGMODE_NO_ASM_MANDEL)
	{
		if (g_parameter2_l.real() == g_externs.Fudge())        // Scott variant
		{
			orbit_calc =  scott_trig_plus_trig_orbit;
		}
		else if (g_parameter2_l.real() == -g_externs.Fudge())  // Skinner variant
		{
			orbit_calc =  skinner_trig_sub_trig_orbit;
		}
	}
	// TODO: don't write to g_current_fractal_specific
	current_fractal_specific_set_per_pixel(per_pixel);
	current_fractal_specific_set_orbit_calc(orbit_calc);

	return julia_setup_l();
}

bool trig_plus_trig_setup_fp()
{
	fn_plus_fn_symmetry();
	if (g_function_index[1] == FUNCTION_SQR)
	{
		return trig_plus_sqr_setup_fp();
	}
	int (*per_pixel)(void) =  other_julia_per_pixel_fp;
	int (*orbit_calc)(void) =  trig_plus_trig_orbit_fp;
	if (g_parameter.real() == 1.0 && g_parameter.imag() == 0.0 && g_parameter2.imag() == 0.0
		&& g_debug_mode != DEBUGMODE_NO_ASM_MANDEL)
	{
		if (g_parameter2.real() == 1.0)        // Scott variant
		{
			orbit_calc =  scott_trig_plus_trig_orbit_fp;
		}
		else if (g_parameter2.real() == -1.0)  // Skinner variant
		{
			orbit_calc =  skinner_trig_sub_trig_orbit_fp;
		}
	}
	// TODO: don't write to g_current_fractal_specific
	current_fractal_specific_set_per_pixel(per_pixel);
	current_fractal_specific_set_orbit_calc(orbit_calc);
	return julia_setup_fp();
}

bool lambda_trig_or_trig_setup()
{
	g_long_parameter = &g_parameter_l;
	g_float_parameter = &g_parameter;
	if ((g_function_index[0] == FUNCTION_EXP) || (g_function_index[1] == FUNCTION_EXP))
	{
		g_symmetry = SYMMETRY_NONE;
	}
	if ((g_function_index[0] == FUNCTION_LOG) || (g_function_index[1] == FUNCTION_LOG))
	{
		g_symmetry = SYMMETRY_X_AXIS;
	}
	get_julia_attractor(0.0, 0.0);      // an attractor?
	return true;
}

bool julia_trig_or_trig_setup()
{
	// default symmetry is SYMMETRY_X_AXIS
	g_long_parameter = &g_parameter_l; // added to consolidate code 10/1/92 JCO
	g_float_parameter = &g_parameter;
	if (g_parameter.imag() != 0.0)
	{
		g_symmetry = SYMMETRY_NONE;
	}
	if (g_function_index[0] == FUNCTION_FLIP || g_function_index[1] == FUNCTION_FLIP) // FLIP
	{
		g_symmetry = SYMMETRY_NONE;
	}
	get_julia_attractor(0.0, 0.0);      // an attractor?
	return true;
}

bool mandelbrot_lambda_trig_or_trig_setup()
{ // psuedo
	// default symmetry is SYMMETRY_X_AXIS
	g_long_parameter = &g_initial_z_l; // added to consolidate code 10/1/92 JCO
	g_float_parameter = &g_initial_z;
	if (g_function_index[0] == FUNCTION_SQR)
	{
		g_symmetry = SYMMETRY_NONE;
	}
	if ((g_function_index[0] == FUNCTION_LOG) || (g_function_index[1] == FUNCTION_LOG))
	{
		g_symmetry = SYMMETRY_NONE;
	}
	return true;
}

bool mandelbrot_trig_or_trig_setup()
{
	// default symmetry is SYMMETRY_XY_AXIS_NO_PARAMETER
	g_long_parameter = &g_initial_z_l; // added to consolidate code 10/1/92 JCO
	g_float_parameter = &g_initial_z;
	if ((g_function_index[0] == FUNCTION_FLIP) || (g_function_index[1] == FUNCTION_FLIP)) // FLIP  JCO 5/28/92
	{
		g_symmetry = SYMMETRY_NONE;
	}
	return true;
}

bool z_trig_plus_z_setup()
{
	// static char ZXTrigPlusZSym1[] =
	// fn1 ->  sin   cos    sinh  cosh exp   log   sqr
	// {SYMMETRY_X_AXIS, SYMMETRY_XY_AXIS, SYMMETRY_X_AXIS, SYMMETRY_XY_AXIS, SYMMETRY_X_AXIS, SYMMETRY_NONE, SYMMETRY_XY_AXIS};
	// static char ZXTrigPlusZSym2[] =
	// fn1 ->  sin   cos    sinh  cosh exp   log   sqr
	// {SYMMETRY_NONE, SYMMETRY_ORIGIN, SYMMETRY_NONE, SYMMETRY_ORIGIN, SYMMETRY_NONE, SYMMETRY_NONE, SYMMETRY_ORIGIN};

	if (g_parameters[P1_IMAG] == 0.0 && g_parameters[P2_IMAG] == 0.0)
	{
		// symmetry = ZXTrigPlusZSym1[g_function_index[0]];
		switch (g_function_index[0])
		{
		case FUNCTION_COSXX:
		case FUNCTION_SINH:
		case FUNCTION_SQR:
		case FUNCTION_COS:
			g_symmetry = SYMMETRY_XY_AXIS;
			break;
		case FUNCTION_FLIP:
			g_symmetry = SYMMETRY_Y_AXIS;
			break;
		case FUNCTION_LOG:
			g_symmetry = SYMMETRY_NONE;
			break;
		default:
			g_symmetry = SYMMETRY_X_AXIS;
			break;
		}
	}
	else
	{
		switch (g_function_index[0])
		{
		case FUNCTION_COSXX:
		case FUNCTION_SINH:
		case FUNCTION_SQR:
		case FUNCTION_COS:
			g_symmetry = SYMMETRY_ORIGIN;
			break;
		case FUNCTION_FLIP:
			g_symmetry = SYMMETRY_NONE;
			break;
		default:
			g_symmetry = SYMMETRY_NONE;
			break;
		}
	}
	if (g_current_fractal_specific->isinteger)
	{
		int (*orbit_calc)(void) =  z_trig_z_plus_z_orbit;
		if (g_parameter_l.real() == g_externs.Fudge() && g_parameter_l.imag() == 0L && g_parameter2_l.imag() == 0L
			&& g_debug_mode != DEBUGMODE_NO_ASM_MANDEL)
		{
			if (g_parameter2_l.real() == g_externs.Fudge())     // Scott variant
			{
				orbit_calc =  scott_z_trig_z_plus_z_orbit;
			}
			else if (g_parameter2_l.real() == -g_externs.Fudge())  // Skinner variant
			{
				orbit_calc =  skinner_z_trig_z_minus_z_orbit;
			}
		}
		// TODO: don't write to g_current_fractal_specific
		current_fractal_specific_set_orbit_calc(orbit_calc);
		return julia_setup_l();
	}

	int (*orbit_calc)(void) = z_trig_z_plus_z_orbit_fp;
	if (g_parameter.real() == 1.0 && g_parameter.imag() == 0.0 && g_parameter2.imag() == 0.0
		&& g_debug_mode != DEBUGMODE_NO_ASM_MANDEL)
	{
		if (g_parameter2.real() == 1.0)     // Scott variant
		{
			orbit_calc =  scott_z_trig_z_plus_z_orbit_fp;
		}
		else if (g_parameter2.real() == -1.0)       // Skinner variant
		{
			orbit_calc =  skinner_z_trig_z_minus_z_orbit_fp;
		}
	}
	// TODO: don't write to g_current_fractal_specific
	current_fractal_specific_set_orbit_calc(orbit_calc);
	return julia_setup_fp();
}

bool lambda_trig_setup()
{
	bool is_integer = (g_current_fractal_specific->isinteger != 0);
	int (*orbit_calc)(void) = is_integer ? lambda_trig_orbit : lambda_trig_orbit_fp;
	switch (g_function_index[0])
	{
	case FUNCTION_SIN:
	case FUNCTION_COSXX:
	case FUNCTION_COS:
		g_symmetry = SYMMETRY_PI;
		orbit_calc = is_integer ? lambda_trig1_orbit : lambda_trig1_orbit_fp;
		break;
	case FUNCTION_SINH:
		g_symmetry = SYMMETRY_ORIGIN;
		orbit_calc = is_integer ? lambda_trig2_orbit : lambda_trig2_orbit_fp;
		break;
	case FUNCTION_SQR:
		g_symmetry = SYMMETRY_ORIGIN;
		break;
	case FUNCTION_EXP:
		orbit_calc = is_integer ? lambda_exponent_orbit : lambda_exponent_orbit_fp;
		g_symmetry = SYMMETRY_NONE;
		break;
	case FUNCTION_LOG:
		g_symmetry = SYMMETRY_NONE;
		break;
	default:
		g_symmetry = SYMMETRY_ORIGIN;
		break;
	}
	// TODO: don't write to g_current_fractal_specific
	current_fractal_specific_set_orbit_calc(orbit_calc);
	get_julia_attractor(0.0, 0.0);      // an attractor?
	return is_integer ? julia_setup_l() : julia_setup_fp();
}

bool julia_fn_plus_z_squared_setup()
{
	// static char fnpluszsqrd[] =
	// fn1 ->  sin   cos    sinh  cosh   sqr    exp   log
	// sin    {SYMMETRY_NONE, SYMMETRY_ORIGIN, SYMMETRY_NONE, SYMMETRY_ORIGIN, SYMMETRY_ORIGIN, SYMMETRY_NONE, SYMMETRY_NONE};

	switch (g_function_index[0])
	{
	case FUNCTION_COSXX:
	case FUNCTION_SINH:
	case FUNCTION_SQR:
	case FUNCTION_COS:
	case FUNCTION_TAN:
	case FUNCTION_TANH:
		g_symmetry = SYMMETRY_ORIGIN;
	}
	return g_current_fractal_specific->isinteger ? julia_setup_l() : julia_setup_fp();
}

bool sqr_trig_setup()
{
	// static char SqrTrigSym[] =
	// fn1 ->  sin    cos    sinh   cosh   sqr    exp   log
	// {SYMMETRY_PI, SYMMETRY_PI, SYMMETRY_XY_AXIS, SYMMETRY_XY_AXIS, SYMMETRY_XY_AXIS, SYMMETRY_X_AXIS, SYMMETRY_X_AXIS};
	// symmetry = SqrTrigSym[g_function_index[0]];      JCO  5/9/92
	switch (g_function_index[0])
	{
	case FUNCTION_SIN:
	case FUNCTION_COSXX:
	case FUNCTION_COS:
		g_symmetry = SYMMETRY_PI;
	}
	return g_current_fractal_specific->isinteger ? julia_setup_l() : julia_setup_fp();
}

bool fn_fn_setup()
{
	static SymmetryType fnxfn[7][7] =
	{// fn2 ->sin     cos    sinh    cosh  exp    log    sqr
	// fn1
	// sin  {SYMMETRY_PI, SYMMETRY_Y_AXIS, SYMMETRY_XY_AXIS, SYMMETRY_XY_AXIS, SYMMETRY_X_AXIS, SYMMETRY_NONE, SYMMETRY_XY_AXIS},
	// cos  {SYMMETRY_Y_AXIS, SYMMETRY_PI, SYMMETRY_XY_AXIS, SYMMETRY_XY_AXIS, SYMMETRY_X_AXIS, SYMMETRY_NONE, SYMMETRY_XY_AXIS},
	// sinh {SYMMETRY_XY_AXIS, SYMMETRY_XY_AXIS, SYMMETRY_XY_AXIS, SYMMETRY_XY_AXIS, SYMMETRY_X_AXIS, SYMMETRY_NONE, SYMMETRY_XY_AXIS},
	// cosh {SYMMETRY_XY_AXIS, SYMMETRY_XY_AXIS, SYMMETRY_XY_AXIS, SYMMETRY_XY_AXIS, SYMMETRY_X_AXIS, SYMMETRY_NONE, SYMMETRY_XY_AXIS},
	// exp  {SYMMETRY_X_AXIS, SYMMETRY_X_AXIS, SYMMETRY_X_AXIS, SYMMETRY_X_AXIS, SYMMETRY_X_AXIS, SYMMETRY_NONE, SYMMETRY_XY_AXIS},
	// log  {SYMMETRY_NONE, SYMMETRY_NONE, SYMMETRY_NONE, SYMMETRY_NONE, SYMMETRY_NONE, SYMMETRY_X_AXIS, SYMMETRY_NONE},
	// sqr  {SYMMETRY_XY_AXIS, SYMMETRY_XY_AXIS, SYMMETRY_XY_AXIS, SYMMETRY_XY_AXIS, SYMMETRY_XY_AXIS, SYMMETRY_NONE, SYMMETRY_XY_AXIS},
	};
	if (g_function_index[0] < NUM_OF(fnxfn) && g_function_index[1] < NUM_OF(fnxfn))
	{
		g_symmetry = fnxfn[g_function_index[0]][g_function_index[1]];
	}
	else
	{
		if (g_function_index[0] == FUNCTION_LOG || g_function_index[1] == FUNCTION_LOG)
		{
			g_symmetry = SYMMETRY_NONE;
		}
		// TODO: this whole if block looks bogus?
		if (g_function_index[0] == FUNCTION_COS || g_function_index[1] == FUNCTION_COS)
		{
			if (g_function_index[0] == FUNCTION_SIN || g_function_index[1] == FUNCTION_SIN)
			{
				g_symmetry = SYMMETRY_PI;
			}
			if (g_function_index[0] == FUNCTION_COSXX || g_function_index[1] == FUNCTION_COSXX)
			{
				g_symmetry = SYMMETRY_PI;
			}
		}
		if (g_function_index[0] == FUNCTION_COS && g_function_index[1] == FUNCTION_COS)
		{
			g_symmetry = SYMMETRY_PI;
		}
	}
	return g_current_fractal_specific->isinteger ? julia_setup_l() : julia_setup_fp();
}

bool mandelbrot_trig_setup()
{
	bool is_integer = (g_current_fractal_specific->isinteger != 0);
	int (*orbit_calc)(void) = is_integer ? lambda_trig_orbit : lambda_trig_orbit_fp;
	g_symmetry = SYMMETRY_XY_AXIS_NO_PARAMETER;
	switch (g_function_index[0])
	{
	case FUNCTION_SIN:
	case FUNCTION_COSXX:
		orbit_calc = is_integer ? lambda_trig1_orbit : lambda_trig1_orbit_fp;
		break;
	case FUNCTION_SINH:
		orbit_calc = is_integer ? lambda_trig2_orbit : lambda_trig2_orbit_fp;
		break;
	case FUNCTION_EXP:
		g_symmetry = SYMMETRY_X_AXIS_NO_PARAMETER;
		orbit_calc = is_integer ? lambda_exponent_orbit : lambda_exponent_orbit_fp;
		break;
	case FUNCTION_LOG:
		g_symmetry = SYMMETRY_X_AXIS_NO_PARAMETER;
		break;
	default:   // added for additional functions, JCO 5/25/92
		g_symmetry = SYMMETRY_XY_AXIS_NO_PARAMETER;
		break;
	}
	// TODO: don't write to g_current_fractal_specific
	current_fractal_specific_set_orbit_calc(orbit_calc);
	return is_integer ? mandelbrot_setup_l() : mandelbrot_setup_fp();
}

bool marks_julia_setup()
{
#if !defined(NO_FIXED_POINT_MATH)
	if (g_parameters[P2_REAL] < 1)
	{
		g_parameters[P2_REAL] = 1;
	}
	g_c_exp = int(g_parameters[P2_REAL]);
	g_long_parameter = &g_parameter_l;
	g_old_z_l = *g_long_parameter;
	if (g_c_exp > 3)
	{
		complex_power_l(&g_old_z_l, g_c_exp-1, &g_coefficient_l, g_bit_shift);
	}
	else if (g_c_exp == 3)
	{
		g_coefficient_l.real(multiply(g_old_z_l.real(), g_old_z_l.real(), g_bit_shift) - multiply(g_old_z_l.imag(), g_old_z_l.imag(), g_bit_shift));
		g_coefficient_l.imag(multiply(g_old_z_l.real(), g_old_z_l.imag(), g_bit_shift_minus_1));
	}
	else if (g_c_exp == 2)
	{
		g_coefficient_l = g_old_z_l;
	}
	else if (g_c_exp < 2)
	{
		assert((1L << g_bit_shift) == DoubleToFudge(1.0));
		g_coefficient_l.real(1L << g_bit_shift);
		g_coefficient_l.imag(0L);
	}
	get_julia_attractor(0.0, 0.0);      // an attractor?
#endif
	return true;
}

bool marks_julia_setup_fp()
{
	if (g_parameters[P2_REAL] < 1)
	{
		g_parameters[P2_REAL] = 1;
	}
	g_c_exp = int(g_parameters[P2_REAL]);
	g_float_parameter = &g_parameter;
	g_old_z = *g_float_parameter;
	if (g_c_exp > 3)
	{
		complex_power(&g_old_z, g_c_exp-1, &g_coefficient);
	}
	else if (g_c_exp == 3)
	{
		g_coefficient.x = sqr(g_old_z.real()) - sqr(g_old_z.imag());
		g_coefficient.y = g_old_z.real()*g_old_z.imag()*2;
	}
	else if (g_c_exp == 2)
	{
		g_coefficient = g_old_z;
	}
	else if (g_c_exp < 2)
	{
		g_coefficient.x = 1.0;
		g_coefficient.y = 0.0;
	}
	get_julia_attractor(0.0, 0.0);      // an attractor?
	return true;
}

bool sierpinski_setup()
{
	// sierpinski
	g_periodicity_check = 0;					// disable periodicity checks
	g_temp_z_l.real(DoubleToFudge(1.0));
	g_temp_z_l.imag(DoubleToFudge(0.5));
	return true;
}

bool sierpinski_setup_fp()
{
	// sierpinski
	g_periodicity_check = 0;                // disable periodicity checks
	g_temp_z.x = 1;
	g_temp_z.y = 0.5;
	return true;
}

bool phoenix_setup()
{
	g_long_parameter = &g_parameter_l; // added to consolidate code 10/1/92 JCO
	g_float_parameter = &g_parameter;
	g_degree = int(g_parameter2.real());
	if (g_degree < 2 && g_degree > -3)
	{
		g_degree = 0;
	}
	g_parameters[P2_REAL] = double(g_degree);
	int (*orbit_calc)(void) = 0;
	if (g_degree == 0)
	{
		orbit_calc = g_user_float_flag ? phoenix_orbit_fp : phoenix_orbit;
	}
	if (g_degree >= 2)
	{
		g_degree--;
		orbit_calc = g_user_float_flag ? phoenix_plus_orbit_fp : phoenix_plus_orbit;
	}
	if (g_degree <= -3)
	{
		g_degree = abs(g_degree) - 2;
		orbit_calc = g_user_float_flag ? phoenix_minus_orbit_fp : phoenix_minus_orbit;
	}
	// TODO: don't write to g_current_fractal_specific
	current_fractal_specific_set_orbit_calc(orbit_calc);

	return true;
}

bool phoenix_complex_setup()
{
	g_long_parameter = &g_parameter_l;
	g_float_parameter = &g_parameter;
	g_degree = int(g_parameters[P3_REAL]);
	if (g_degree < 2 && g_degree > -3)
	{
		g_degree = 0;
	}
	g_parameters[P3_REAL] = double(g_degree);
	int (*orbit_calc)(void) = 0;
	if (g_degree == 0)
	{
		g_symmetry = (g_parameter2.real() != 0 || g_parameter2.imag() != 0) ? SYMMETRY_NONE : SYMMETRY_ORIGIN;
		if (g_parameter.imag() == 0 && g_parameter2.imag() == 0)
		{
			g_symmetry = SYMMETRY_X_AXIS;
		}
		orbit_calc = g_user_float_flag ? phoenix_complex_orbit_fp : phoenix_complex_orbit;
	}
	if (g_degree >= 2)
	{
		g_degree--;
		g_symmetry = (g_parameter.imag() == 0 && g_parameter2.imag() == 0) ? SYMMETRY_X_AXIS : SYMMETRY_NONE;
		orbit_calc = g_user_float_flag ? phoenix_complex_plus_orbit_fp : phoenix_complex_plus_orbit;
	}
	if (g_degree <= -3)
	{
		g_degree = abs(g_degree) - 2;
		g_symmetry = (g_parameter.imag() == 0 && g_parameter2.imag() == 0) ? SYMMETRY_X_AXIS : SYMMETRY_NONE;
		orbit_calc = g_user_float_flag ? phoenix_complex_minus_orbit_fp : phoenix_complex_minus_orbit;
	}
	// TODO: don't write to g_current_fractal_specific
	current_fractal_specific_set_orbit_calc(orbit_calc);

	return true;
}

bool mandelbrot_phoenix_setup()
{
	g_long_parameter = &g_initial_z_l; // added to consolidate code 10/1/92 JCO
	g_float_parameter = &g_initial_z;
	g_degree = int(g_parameter2.real());
	if (g_degree < 2 && g_degree > -3)
	{
		g_degree = 0;
	}
	g_parameters[P2_REAL] = double(g_degree);
	int (*orbit_calc)(void) = 0;
	if (g_degree == 0)
	{
		orbit_calc = g_user_float_flag ? phoenix_orbit_fp : phoenix_orbit;
	}
	if (g_degree >= 2)
	{
		g_degree--;
		orbit_calc = g_user_float_flag ? phoenix_plus_orbit_fp : phoenix_plus_orbit;
	}
	if (g_degree <= -3)
	{
		g_degree = abs(g_degree) - 2;
		orbit_calc = g_user_float_flag ? phoenix_minus_orbit_fp : phoenix_minus_orbit;
	}
	// TODO: don't write to g_current_fractal_specific
	current_fractal_specific_set_orbit_calc(orbit_calc);

	return true;
}

bool mandelbrot_phoenix_complex_setup()
{
	g_long_parameter = &g_initial_z_l; // added to consolidate code 10/1/92 JCO
	g_float_parameter = &g_initial_z;
	g_degree = int(g_parameters[P3_REAL]);
	if (g_degree < 2 && g_degree > -3)
	{
		g_degree = 0;
	}
	g_parameters[P3_REAL] = double(g_degree);
	if (g_parameter.imag() != 0 || g_parameter2.imag() != 0)
	{
		g_symmetry = SYMMETRY_NONE;
	}
	int (*orbit_calc)(void) = 0;
	if (g_degree == 0)
	{
		orbit_calc = g_user_float_flag ? phoenix_complex_orbit_fp : phoenix_complex_orbit;
	}
	if (g_degree >= 2)
	{
		g_degree--;
		orbit_calc = g_user_float_flag ? phoenix_complex_plus_orbit_fp : phoenix_complex_plus_orbit;
	}
	if (g_degree <= -3)
	{
		g_degree = abs(g_degree) - 2;
		orbit_calc = g_user_float_flag ? phoenix_complex_minus_orbit_fp : phoenix_complex_minus_orbit;
	}
	// TODO: don't write to g_current_fractal_specific
	current_fractal_specific_set_orbit_calc(orbit_calc);

	return true;
}

bool standard_setup()
{
	if (g_fractal_type == FRACTYPE_UNITY_FP)
	{
		g_periodicity_check = 0;
	}
	return true;
}

bool volterra_lotka_setup()
{
	if (g_parameters[P1_REAL] < 0.0)
	{
		g_parameters[P1_REAL] = 0.0;
	}
	if (g_parameters[P1_IMAG] < 0.0)
	{
		g_parameters[P1_IMAG] = 0.0;
	}
	if (g_parameters[P1_REAL] > 1.0)
	{
		g_parameters[P1_REAL] = 1.0;
	}
	if (g_parameters[P1_IMAG] > 1.0)
	{
		g_parameters[P1_IMAG] = 1.0;
	}
	g_float_parameter = &g_parameter;
	return true;
}
