//
//	This file contains the "big number" high precision versions of the
//	fractal routines.
//
#include <string>

#include <limits.h>
#include <string.h>
#if !defined(__386BSD__)
#if !defined(_WIN32)
#include <malloc.h>
#endif
#endif

#include "port.h"
#include "prototyp.h"
#include "helpdefs.h"
#include "fractype.h"

#include "cmdfiles.h"
#include "Externals.h"
#include "fractalb.h"
#include "fractalp.h"

#include "EscapeTime.h"

int g_bf_math = 0;

void corners_bf_to_float()
{
	int i;
	if (g_bf_math)
	{
		g_escape_time_state.m_grid_fp.x_min() = double(bftofloat(g_escape_time_state.m_grid_bf.x_min()));
		g_escape_time_state.m_grid_fp.y_min() = double(bftofloat(g_escape_time_state.m_grid_bf.y_min()));
		g_escape_time_state.m_grid_fp.x_max() = double(bftofloat(g_escape_time_state.m_grid_bf.x_max()));
		g_escape_time_state.m_grid_fp.y_max() = double(bftofloat(g_escape_time_state.m_grid_bf.y_max()));
		g_escape_time_state.m_grid_fp.x_3rd() = double(bftofloat(g_escape_time_state.m_grid_bf.x_3rd()));
		g_escape_time_state.m_grid_fp.y_3rd() = double(bftofloat(g_escape_time_state.m_grid_bf.y_3rd()));
	}
	for (i = 0; i < MAX_PARAMETERS; i++)
	{
		if (type_has_parameter(g_fractal_type, i))
		{
			g_parameters[i] = double(bftofloat(bfparms[i]));
		}
	}
}

// --------------------------------------------------------------------
// Bignumber Bailout Routines
// --------------------------------------------------------------------

// Note:
// No need to set magnitude
// as color schemes that need it calculate it later.
int bail_out_mod_bn()
{
	long longmagnitude;

	square_bn(bntmpsqrx, g_new_z_bn.x);
	square_bn(bntmpsqry, g_new_z_bn.y);
	add_bn(bntmp, bntmpsqrx + g_shift_factor, bntmpsqry + g_shift_factor);

	longmagnitude = bntoint(bntmp);  // works with any fractal type
	if (longmagnitude >= long(g_rq_limit))
	{
		return 1;
	}
	copy_bn(g_old_z_bn.x, g_new_z_bn.x);
	copy_bn(g_old_z_bn.y, g_new_z_bn.y);
	return 0;
}

int bail_out_real_bn()
{
	long longtempsqrx;

	square_bn(bntmpsqrx, g_new_z_bn.x);
	square_bn(bntmpsqry, g_new_z_bn.y);
	longtempsqrx = bntoint(bntmpsqrx + g_shift_factor);
	if (longtempsqrx >= long(g_rq_limit))
	{
		return 1;
	}
	copy_bn(g_old_z_bn.x, g_new_z_bn.x);
	copy_bn(g_old_z_bn.y, g_new_z_bn.y);
	return 0;
}


int bail_out_imag_bn()
{
	long longtempsqry;

	square_bn(bntmpsqrx, g_new_z_bn.x);
	square_bn(bntmpsqry, g_new_z_bn.y);
	longtempsqry = bntoint(bntmpsqry + g_shift_factor);
	if (longtempsqry >= long(g_rq_limit))
	{
		return 1;
	}
	copy_bn(g_old_z_bn.x, g_new_z_bn.x);
	copy_bn(g_old_z_bn.y, g_new_z_bn.y);
	return 0;
}

int bail_out_or_bn()
{
	long longtempsqrx;
	long longtempsqry;

	square_bn(bntmpsqrx, g_new_z_bn.x);
	square_bn(bntmpsqry, g_new_z_bn.y);
	longtempsqrx = bntoint(bntmpsqrx + g_shift_factor);
	longtempsqry = bntoint(bntmpsqry + g_shift_factor);
	if (longtempsqrx >= long(g_rq_limit) || longtempsqry >= long(g_rq_limit))
	{
		return 1;
	}
	copy_bn(g_old_z_bn.x, g_new_z_bn.x);
	copy_bn(g_old_z_bn.y, g_new_z_bn.y);
	return 0;
}

int bail_out_and_bn()
{
	long longtempsqrx;
	long longtempsqry;

	square_bn(bntmpsqrx, g_new_z_bn.x);
	square_bn(bntmpsqry, g_new_z_bn.y);
	longtempsqrx = bntoint(bntmpsqrx + g_shift_factor);
	longtempsqry = bntoint(bntmpsqry + g_shift_factor);
	if (longtempsqrx >= long(g_rq_limit) && longtempsqry >= long(g_rq_limit))
	{
		return 1;
	}
	copy_bn(g_old_z_bn.x, g_new_z_bn.x);
	copy_bn(g_old_z_bn.y, g_new_z_bn.y);
	return 0;
}

int bail_out_manhattan_bn()
{
	long longtempmag;

	square_bn(bntmpsqrx, g_new_z_bn.x);
	square_bn(bntmpsqry, g_new_z_bn.y);
	// note: in next five lines, g_old_z_bn is just used as a temporary variable
	abs_bn(g_old_z_bn.x, g_new_z_bn.x);
	abs_bn(g_old_z_bn.y, g_new_z_bn.y);
	add_bn(bntmp, g_old_z_bn.x, g_old_z_bn.y);
	square_bn(g_old_z_bn.x, bntmp);
	longtempmag = bntoint(g_old_z_bn.x + g_shift_factor);
	if (longtempmag >= long(g_rq_limit))
	{
		return 1;
	}
	copy_bn(g_old_z_bn.x, g_new_z_bn.x);
	copy_bn(g_old_z_bn.y, g_new_z_bn.y);
	return 0;
}

int bail_out_manhattan_r_bn()
{
	long longtempmag;

	square_bn(bntmpsqrx, g_new_z_bn.x);
	square_bn(bntmpsqry, g_new_z_bn.y);
	add_bn(bntmp, g_new_z_bn.x, g_new_z_bn.y); // don't need abs since we square it next
	// note: in next two lines, g_old_z_bn is just used as a temporary variable
	square_bn(g_old_z_bn.x, bntmp);
	longtempmag = bntoint(g_old_z_bn.x + g_shift_factor);
	if (longtempmag >= long(g_rq_limit))
	{
		return 1;
	}
	copy_bn(g_old_z_bn.x, g_new_z_bn.x);
	copy_bn(g_old_z_bn.y, g_new_z_bn.y);
	return 0;
}

int bail_out_mod_bf()
{
	long longmagnitude;

	square_bf(bftmpsqrx, g_new_z_bf.x);
	square_bf(bftmpsqry, g_new_z_bf.y);
	add_bf(bftmp, bftmpsqrx, bftmpsqry);

	longmagnitude = bftoint(bftmp);
	if (longmagnitude >= long(g_rq_limit))
	{
		return 1;
	}
	copy_bf(g_old_z_bf.x, g_new_z_bf.x);
	copy_bf(g_old_z_bf.y, g_new_z_bf.y);
	return 0;
}

int bail_out_real_bf()
{
	long longtempsqrx;

	square_bf(bftmpsqrx, g_new_z_bf.x);
	square_bf(bftmpsqry, g_new_z_bf.y);
	longtempsqrx = bftoint(bftmpsqrx);
	if (longtempsqrx >= long(g_rq_limit))
	{
		return 1;
	}
	copy_bf(g_old_z_bf.x, g_new_z_bf.x);
	copy_bf(g_old_z_bf.y, g_new_z_bf.y);
	return 0;
}

int bail_out_imag_bf()
{
	long longtempsqry;

	square_bf(bftmpsqrx, g_new_z_bf.x);
	square_bf(bftmpsqry, g_new_z_bf.y);
	longtempsqry = bftoint(bftmpsqry);
	if (longtempsqry >= long(g_rq_limit))
	{
		return 1;
	}
	copy_bf(g_old_z_bf.x, g_new_z_bf.x);
	copy_bf(g_old_z_bf.y, g_new_z_bf.y);
	return 0;
}

int bail_out_or_bf()
{
	long longtempsqrx;
	long longtempsqry;

	square_bf(bftmpsqrx, g_new_z_bf.x);
	square_bf(bftmpsqry, g_new_z_bf.y);
	longtempsqrx = bftoint(bftmpsqrx);
	longtempsqry = bftoint(bftmpsqry);
	if (longtempsqrx >= long(g_rq_limit) || longtempsqry >= long(g_rq_limit))
	{
		return 1;
	}
	copy_bf(g_old_z_bf.x, g_new_z_bf.x);
	copy_bf(g_old_z_bf.y, g_new_z_bf.y);
	return 0;
}

int bail_out_and_bf()
{
	long longtempsqrx;
	long longtempsqry;

	square_bf(bftmpsqrx, g_new_z_bf.x);
	square_bf(bftmpsqry, g_new_z_bf.y);
	longtempsqrx = bftoint(bftmpsqrx);
	longtempsqry = bftoint(bftmpsqry);
	if (longtempsqrx >= long(g_rq_limit) && longtempsqry >= long(g_rq_limit))
	{
		return 1;
	}
	copy_bf(g_old_z_bf.x, g_new_z_bf.x);
	copy_bf(g_old_z_bf.y, g_new_z_bf.y);
	return 0;
}

int bail_out_manhattan_bf()
{
	long longtempmag;

	square_bf(bftmpsqrx, g_new_z_bf.x);
	square_bf(bftmpsqry, g_new_z_bf.y);
	// note: in next five lines, g_old_z_bf is just used as a temporary variable
	abs_bf(g_old_z_bf.x, g_new_z_bf.x);
	abs_bf(g_old_z_bf.y, g_new_z_bf.y);
	add_bf(bftmp, g_old_z_bf.x, g_old_z_bf.y);
	square_bf(g_old_z_bf.x, bftmp);
	longtempmag = bftoint(g_old_z_bf.x);
	if (longtempmag >= long(g_rq_limit))
	{
		return 1;
	}
	copy_bf(g_old_z_bf.x, g_new_z_bf.x);
	copy_bf(g_old_z_bf.y, g_new_z_bf.y);
	return 0;
}

int bail_out_manhattan_r_bf()
{
	long longtempmag;

	square_bf(bftmpsqrx, g_new_z_bf.x);
	square_bf(bftmpsqry, g_new_z_bf.y);
	add_bf(bftmp, g_new_z_bf.x, g_new_z_bf.y); // don't need abs since we square it next
	// note: in next two lines, g_old_z_bf is just used as a temporary variable
	square_bf(g_old_z_bf.x, bftmp);
	longtempmag = bftoint(g_old_z_bf.x);
	if (longtempmag >= long(g_rq_limit))
	{
		return 1;
	}
	copy_bf(g_old_z_bf.x, g_new_z_bf.x);
	copy_bf(g_old_z_bf.y, g_new_z_bf.y);
	return 0;
}

bool mandelbrot_setup_bn()
{
	// this should be set up dynamically based on corners
	bn_t bntemp1, bntemp2;
	int saved = save_stack();
	bntemp1 = alloc_stack(g_bn_length);
	bntemp2 = alloc_stack(g_bn_length);

	bftobn(bnxmin, g_escape_time_state.m_grid_bf.x_min());
	bftobn(bnxmax, g_escape_time_state.m_grid_bf.x_max());
	bftobn(bnymin, g_escape_time_state.m_grid_bf.y_min());
	bftobn(bnymax, g_escape_time_state.m_grid_bf.y_max());
	bftobn(bnx3rd, g_escape_time_state.m_grid_bf.x_3rd());
	bftobn(bny3rd, g_escape_time_state.m_grid_bf.y_3rd());

	g_bf_math = BIGNUM;

	// bnxdel = (bnxmax - bnx3rd)/(g_x_dots-1)
	sub_bn(bnxdel, bnxmax, bnx3rd);
	div_a_bn_int(bnxdel, (U16)(g_x_dots - 1));

	// bnydel = (bnymax - bny3rd)/(g_y_dots-1)
	sub_bn(bnydel, bnymax, bny3rd);
	div_a_bn_int(bnydel, (U16)(g_y_dots - 1));

	// bnxdel2 = (bnx3rd - bnxmin)/(g_y_dots-1)
	sub_bn(bnxdel2, bnx3rd, bnxmin);
	div_a_bn_int(bnxdel2, (U16)(g_y_dots - 1));

	// bnydel2 = (bny3rd - bnymin)/(g_x_dots-1)
	sub_bn(bnydel2, bny3rd, bnymin);
	div_a_bn_int(bnydel2, (U16)(g_x_dots - 1));

	abs_bn(bnclosenuff, bnxdel);
	if (cmp_bn(abs_bn(bntemp1, bnxdel2), bnclosenuff) > 0)
	{
		copy_bn(bnclosenuff, bntemp1);
	}
	if (cmp_bn(abs_bn(bntemp1, bnydel), abs_bn(bntemp2, bnydel2)) > 0)
	{
		if (cmp_bn(bntemp1, bnclosenuff) > 0)
		{
			copy_bn(bnclosenuff, bntemp1);
		}
	}
	else if (cmp_bn(bntemp2, bnclosenuff) > 0)
	{
		copy_bn(bnclosenuff, bntemp2);
	}
	{
		int t = abs(g_periodicity_check);
		while (t--)
		{
			half_a_bn(bnclosenuff);
		}
	}

	g_c_exp = int(g_parameters[2]);
	switch (g_fractal_type)
	{
	case FRACTYPE_JULIA_FP:
		bftobn(bnparm.x, bfparms[0]);
		bftobn(bnparm.y, bfparms[1]);
		break;
	case FRACTYPE_MANDELBROT_Z_POWER_FP:
		init_big_pi();
		if (double(g_c_exp) == g_parameters[2] && (g_c_exp & 1)) // odd exponents
		{
			g_symmetry = SYMMETRY_XY_AXIS_NO_PARAMETER;
		}
		if (g_parameters[3] != 0)
		{
			g_symmetry = SYMMETRY_NONE;
		}
		break;
	case FRACTYPE_JULIA_Z_POWER_FP:
		init_big_pi();
		bftobn(bnparm.x, bfparms[0]);
		bftobn(bnparm.y, bfparms[1]);
		if ((g_c_exp & 1) || g_parameters[3] != 0.0 || double(g_c_exp) != g_parameters[2])
		{
			g_symmetry = SYMMETRY_NONE;
		}
		break;
	}

	restore_stack(saved);
	return true;
}

bool mandelbrot_setup_bf()
{
	// this should be set up dynamically based on corners
	bf_t bftemp1, bftemp2;
	int saved = save_stack();
	bftemp1 = alloc_stack(g_bf_length + 2);
	bftemp2 = alloc_stack(g_bf_length + 2);

	g_bf_math = BIGFLT;

	// bfxdel = (bfxmax - bfx3rd)/(g_x_dots-1)
	sub_bf(bfxdel, g_escape_time_state.m_grid_bf.x_max(), g_escape_time_state.m_grid_bf.x_3rd());
	div_a_bf_int(bfxdel, (U16)(g_x_dots - 1));

	// bfydel = (bfymax - bfy3rd)/(g_y_dots-1)
	sub_bf(bfydel, g_escape_time_state.m_grid_bf.y_max(), g_escape_time_state.m_grid_bf.y_3rd());
	div_a_bf_int(bfydel, (U16)(g_y_dots - 1));

	// bfxdel2 = (bfx3rd - bfxmin)/(g_y_dots-1)
	sub_bf(bfxdel2, g_escape_time_state.m_grid_bf.x_3rd(), g_escape_time_state.m_grid_bf.x_min());
	div_a_bf_int(bfxdel2, (U16)(g_y_dots - 1));

	// bfydel2 = (bfy3rd - bfymin)/(g_x_dots-1)
	sub_bf(bfydel2, g_escape_time_state.m_grid_bf.y_3rd(), g_escape_time_state.m_grid_bf.y_min());
	div_a_bf_int(bfydel2, (U16)(g_x_dots - 1));

	abs_bf(bfclosenuff, bfxdel);
	if (cmp_bf(abs_bf(bftemp1, bfxdel2), bfclosenuff) > 0)
	{
		copy_bf(bfclosenuff, bftemp1);
	}
	if (cmp_bf(abs_bf(bftemp1, bfydel), abs_bf(bftemp2, bfydel2)) > 0)
	{
		if (cmp_bf(bftemp1, bfclosenuff) > 0)
		{
			copy_bf(bfclosenuff, bftemp1);
		}
	}
	else if (cmp_bf(bftemp2, bfclosenuff) > 0)
	{
		copy_bf(bfclosenuff, bftemp2);
	}
	{
		int t = abs(g_periodicity_check);
		while (t--)
		{
			half_a_bf(bfclosenuff);
		}
	}

	g_c_exp = int(g_parameters[2]);
	switch (g_fractal_type)
	{
	case FRACTYPE_JULIA_FP:
		copy_bf(bfparm.x, bfparms[0]);
		copy_bf(bfparm.y, bfparms[1]);
		break;
	case FRACTYPE_MANDELBROT_Z_POWER_FP:
		init_big_pi();
		if (double(g_c_exp) == g_parameters[2] && (g_c_exp & 1)) // odd exponents
		{
			g_symmetry = SYMMETRY_XY_AXIS_NO_PARAMETER;
		}
		if (g_parameters[3] != 0)
		{
			g_symmetry = SYMMETRY_NONE;
		}
		break;
	case FRACTYPE_JULIA_Z_POWER_FP:
		init_big_pi();
		copy_bf(bfparm.x, bfparms[0]);
		copy_bf(bfparm.y, bfparms[1]);
		if ((g_c_exp & 1) || g_parameters[3] != 0.0 || double(g_c_exp) != g_parameters[2])
		{
			g_symmetry = SYMMETRY_NONE;
		}
		break;
	}

	restore_stack(saved);
	return true;
}

bool inside_coloring_beauty_of_fractals()
{
	return (g_externs.Inside() == COLORMODE_BEAUTY_OF_FRACTALS_60 || g_externs.Inside() == COLORMODE_BEAUTY_OF_FRACTALS_61);
}

bool inside_coloring_beauty_of_fractals_allowed()
{
	return inside_coloring_beauty_of_fractals() && g_beauty_of_fractals;
}

int mandelbrot_per_pixel_bn()
{
	// g_parameter.x = xmin + col*delx + row*g_delta_x2
	mult_bn_int(bnparm.x, bnxdel, (U16)g_col);
	mult_bn_int(bntmp, bnxdel2, (U16)g_row);

	add_a_bn(bnparm.x, bntmp);
	add_a_bn(bnparm.x, bnxmin);

	// g_parameter.y = ymax - row*dely - col*g_delta_y2;
	// note: in next four lines, g_old_z_bn is just used as a temporary variable
	mult_bn_int(g_old_z_bn.x, bnydel,  (U16)g_row);
	mult_bn_int(g_old_z_bn.y, bnydel2, (U16)g_col);
	add_a_bn(g_old_z_bn.x, g_old_z_bn.y);
	sub_bn(bnparm.y, bnymax, g_old_z_bn.x);

	copy_bn(g_old_z_bn.x, bnparm.x);
	copy_bn(g_old_z_bn.y, bnparm.y);

	if (inside_coloring_beauty_of_fractals_allowed())
	{
		// kludge to match "Beauty of Fractals" picture since we start
		// Mandelbrot iteration with init rather than 0
		floattobn(g_old_z_bn.x, g_parameters[0]); // initial pertubation of parameters set
		floattobn(g_old_z_bn.y, g_parameters[1]);
		g_color_iter = -1;
	}
	else
	{
		floattobn(g_new_z_bn.x, g_parameters[0]);
		floattobn(g_new_z_bn.y, g_parameters[1]);
		add_a_bn(g_old_z_bn.x, g_new_z_bn.x);
		add_a_bn(g_old_z_bn.y, g_new_z_bn.y);
	}

	// square has side effect - must copy first
	copy_bn(g_new_z_bn.x, g_old_z_bn.x);
	copy_bn(g_new_z_bn.y, g_old_z_bn.y);

	// Square these to g_r_length bytes of precision
	square_bn(bntmpsqrx, g_new_z_bn.x);
	square_bn(bntmpsqry, g_new_z_bn.y);

	return 1;                  // 1st iteration has been done
}

int mandelbrot_per_pixel_bf()
{
	// g_parameter.x = xmin + col*delx + row*g_delta_x2
	mult_bf_int(bfparm.x, bfxdel, (U16)g_col);
	mult_bf_int(bftmp, bfxdel2, (U16)g_row);

	add_a_bf(bfparm.x, bftmp);
	add_a_bf(bfparm.x, g_escape_time_state.m_grid_bf.x_min());

	// g_parameter.y = ymax - row*dely - col*g_delta_y2;
	// note: in next four lines, g_old_z_bf is just used as a temporary variable
	mult_bf_int(g_old_z_bf.x, bfydel,  (U16)g_row);
	mult_bf_int(g_old_z_bf.y, bfydel2, (U16)g_col);
	add_a_bf(g_old_z_bf.x, g_old_z_bf.y);
	sub_bf(bfparm.y, g_escape_time_state.m_grid_bf.y_max(), g_old_z_bf.x);

	copy_bf(g_old_z_bf.x, bfparm.x);
	copy_bf(g_old_z_bf.y, bfparm.y);

	if (inside_coloring_beauty_of_fractals_allowed())
	{
		// kludge to match "Beauty of Fractals" picture since we start
		// Mandelbrot iteration with g_initial_z rather than 0
		floattobf(g_old_z_bf.x, g_parameters[0]); // initial pertubation of parameters set
		floattobf(g_old_z_bf.y, g_parameters[1]);
		g_color_iter = -1;
	}
	else
	{
		floattobf(g_new_z_bf.x, g_parameters[0]);
		floattobf(g_new_z_bf.y, g_parameters[1]);
		add_a_bf(g_old_z_bf.x, g_new_z_bf.x);
		add_a_bf(g_old_z_bf.y, g_new_z_bf.y);
	}

	// square has side effect - must copy first
	copy_bf(g_new_z_bf.x, g_old_z_bf.x);
	copy_bf(g_new_z_bf.y, g_old_z_bf.y);

	// Square these to g_rbf_length bytes of precision
	square_bf(bftmpsqrx, g_new_z_bf.x);
	square_bf(bftmpsqry, g_new_z_bf.y);

	return 1;                  // 1st iteration has been done
}

int julia_per_pixel_bn()
{
	// old.x = xmin + col*delx + row*g_delta_x2
	mult_bn_int(g_old_z_bn.x, bnxdel, (U16)g_col);
	mult_bn_int(bntmp, bnxdel2, (U16)g_row);

	add_a_bn(g_old_z_bn.x, bntmp);
	add_a_bn(g_old_z_bn.x, bnxmin);

	// old.y = ymax - row*dely - col*g_delta_y2;
	// note: in next four lines, g_new_z_bn is just used as a temporary variable
	mult_bn_int(g_new_z_bn.x, bnydel,  (U16)g_row);
	mult_bn_int(g_new_z_bn.y, bnydel2, (U16)g_col);
	add_a_bn(g_new_z_bn.x, g_new_z_bn.y);
	sub_bn(g_old_z_bn.y, bnymax, g_new_z_bn.x);

	// square has side effect - must copy first
	copy_bn(g_new_z_bn.x, g_old_z_bn.x);
	copy_bn(g_new_z_bn.y, g_old_z_bn.y);

	// Square these to g_r_length bytes of precision
	square_bn(bntmpsqrx, g_new_z_bn.x);
	square_bn(bntmpsqry, g_new_z_bn.y);

	return 1;                  // 1st iteration has been done
}

int julia_per_pixel_bf()
{
	// old.x = xmin + col*delx + row*g_delta_x2
	mult_bf_int(g_old_z_bf.x, bfxdel, (U16)g_col);
	mult_bf_int(bftmp, bfxdel2, (U16)g_row);

	add_a_bf(g_old_z_bf.x, bftmp);
	add_a_bf(g_old_z_bf.x, g_escape_time_state.m_grid_bf.x_min());

	// old.y = ymax - row*dely - col*g_delta_y2;
	// note: in next four lines, g_new_z_bf is just used as a temporary variable
	mult_bf_int(g_new_z_bf.x, bfydel,  (U16)g_row);
	mult_bf_int(g_new_z_bf.y, bfydel2, (U16)g_col);
	add_a_bf(g_new_z_bf.x, g_new_z_bf.y);
	sub_bf(g_old_z_bf.y, g_escape_time_state.m_grid_bf.y_max(), g_new_z_bf.x);

	// square has side effect - must copy first
	copy_bf(g_new_z_bf.x, g_old_z_bf.x);
	copy_bf(g_new_z_bf.y, g_old_z_bf.y);

	// Square these to g_rbf_length bytes of precision
	square_bf(bftmpsqrx, g_new_z_bf.x);
	square_bf(bftmpsqry, g_new_z_bf.y);

	return 1;                  // 1st iteration has been done
}

int julia_orbit_bn()
{
	// Don't forget, with bn_t numbers, after multiplying or squaring
	// you must shift over by g_shift_factor to get the bn number.

	// bntmpsqrx and bntmpsqry were previously squared before getting to
	// this function, so they must be shifted.

	// new.x = tmpsqrx - tmpsqry + g_parameter.x;
	sub_a_bn(bntmpsqrx + g_shift_factor, bntmpsqry + g_shift_factor);
	add_bn(g_new_z_bn.x, bntmpsqrx + g_shift_factor, bnparm.x);

	// new.y = 2*g_old_z_bn.x*g_old_z_bn.y + g_parameter.y;
	mult_bn(bntmp, g_old_z_bn.x, g_old_z_bn.y); // ok to use unsafe here
	double_a_bn(bntmp + g_shift_factor);
	add_bn(g_new_z_bn.y, bntmp + g_shift_factor, bnparm.y);

	return g_externs.BailOutBn();
}

int julia_orbit_bf()
{
	// new.x = tmpsqrx - tmpsqry + g_parameter.x;
	sub_a_bf(bftmpsqrx, bftmpsqry);
	add_bf(g_new_z_bf.x, bftmpsqrx, bfparm.x);

	// new.y = 2*g_old_z_bf.x*g_old_z_bf.y + g_parameter.y;
	mult_bf(bftmp, g_old_z_bf.x, g_old_z_bf.y); // ok to use unsafe here
	double_a_bf(bftmp);
	add_bf(g_new_z_bf.y, bftmp, bfparm.y);
	return g_externs.BailOutBf();
}

int julia_z_power_orbit_bn()
{
	ComplexBigNum parm2;
	int saved = save_stack();

	parm2.x = alloc_stack(g_bn_length);
	parm2.y = alloc_stack(g_bn_length);

	floattobn(parm2.x, g_parameters[2]);
	floattobn(parm2.y, g_parameters[3]);
	complex_power_bn(&g_new_z_bn, &g_old_z_bn, &parm2);
	add_bn(g_new_z_bn.x, bnparm.x, g_new_z_bn.x + g_shift_factor);
	add_bn(g_new_z_bn.y, bnparm.y, g_new_z_bn.y + g_shift_factor);
	restore_stack(saved);
	return g_externs.BailOutBn();
}

int julia_z_power_orbit_bf()
{
	ComplexBigFloat parm2;
	int saved = save_stack();

	parm2.x = alloc_stack(g_bf_length + 2);
	parm2.y = alloc_stack(g_bf_length + 2);

	floattobf(parm2.x, g_parameters[2]);
	floattobf(parm2.y, g_parameters[3]);
	ComplexPower_bf(&g_new_z_bf, &g_old_z_bf, &parm2);
	add_bf(g_new_z_bf.x, bfparm.x, g_new_z_bf.x);
	add_bf(g_new_z_bf.y, bfparm.y, g_new_z_bf.y);
	restore_stack(saved);
	return g_externs.BailOutBf();
}


#if 0
//
// the following is an example of how you can take advantage of the bn_t
// format to squeeze a little more precision out of the calculations.
//
int
julia_orbit_bn()
{
	int oldbnlength;
	bn_t mod;
	// using partial precision multiplications

	// g_new_z_bn.x = bntmpsqrx - bntmpsqry + bnparm.x;
	//
	// Since tmpsqrx and tmpsqry where just calculated to g_r_length bytes of
	// precision, we might as well keep that extra precision in this next
	// subtraction.  Therefore, use g_r_length as the length.
	//

	oldbnlength = g_bn_length;
	g_bn_length = g_r_length;
	sub_a_bn(bntmpsqrx, bntmpsqry);
	g_bn_length = oldbnlength;

	//
	// Now that bntmpsqry has been sutracted from bntmpsqrx, we need to treat
	// tmpsqrx as a single width bignumber, so shift to bntmpsqrx + g_shift_factor.
	//
	add_bn(g_new_z_bn.x, bntmpsqrx + g_shift_factor, bnparm.x);

	// new.y = 2*g_old_z_bn.x*g_old_z_bn.y + old.y;
	// Multiply g_old_z_bn.x*g_old_z_bn.y to g_r_length precision.
	mult_bn(bntmp, g_old_z_bn.x, g_old_z_bn.y);

	//
	// Double g_old_z_bn.x*g_old_z_bn.y by shifting bits, including one of those bits
	// calculated in the previous mult_bn().  Therefore, use g_r_length.
	//
	g_bn_length = g_r_length;
	double_a_bn(bntmp);
	g_bn_length = oldbnlength;

	// Convert back to a single width bignumber and add bnparm.y
	add_bn(g_new_z_bn.y, bntmp + g_shift_factor, bnparm.y);

	copy_bn(g_old_z_bn.x, g_new_z_bn.x);
	copy_bn(g_old_z_bn.y, g_new_z_bn.y);

	// Square these to g_r_length bytes of precision
	square_bn(bntmpsqrx, g_old_z_bn.x);
	square_bn(bntmpsqry, g_old_z_bn.y);

	// And add the full g_r_length precision to get those extra bytes
	g_bn_length = g_r_length;
	add_bn(bntmp, bntmpsqrx, bntmpsqry);
	g_bn_length = oldbnlength;

	mod = bntmp + (g_r_length) - (g_int_length << 1);  // where int part starts after mult
	//
	// equivalent to, but faster than, mod = bn_int(tmp + g_shift_factor);
	//

	magnitude = *mod;
	if (magnitude >= g_rq_limit)
	{
		return 1;
	}
	return 0;
}
#endif

ComplexD complex_bn_to_float(ComplexBigNum *s)
{
	ComplexD t;
	t.x = double(bntofloat(s->x));
	t.y = double(bntofloat(s->y));
	return t;
}

ComplexD complex_bf_to_float(ComplexBigFloat *s)
{
	ComplexD t;
	t.x = double(bftofloat(s->x));
	t.y = double(bftofloat(s->y));
	return t;
}

ComplexBigFloat *complex_log_bf(ComplexBigFloat *t, ComplexBigFloat *s)
{
	square_bf(t->x, s->x);
	square_bf(t->y, s->y);
	add_a_bf(t->x, t->y);
	ln_bf(t->x, t->x);
	half_a_bf(t->x);
	atan2_bf(t->y, s->y, s->x);
	return t;
}

ComplexBigFloat *cplxmul_bf(ComplexBigFloat *t, ComplexBigFloat *x, ComplexBigFloat *y)
{
	bf_t tmp1;
	int saved = save_stack();
	tmp1 = alloc_stack(g_rbf_length + 2);
	mult_bf(t->x, x->x, y->x);
	mult_bf(t->y, x->y, y->y);
	sub_bf(t->x, t->x, t->y);

	mult_bf(tmp1, x->x, y->y);
	mult_bf(t->y, x->y, y->x);
	add_bf(t->y, tmp1, t->y);
	restore_stack(saved);
	return t;
}

ComplexBigFloat *ComplexPower_bf(ComplexBigFloat *t, ComplexBigFloat *xx, ComplexBigFloat *yy)
{
	ComplexBigFloat tmp;
	bf_t e2x, siny, cosy;
	int saved = save_stack();
	e2x = alloc_stack(g_rbf_length + 2);
	siny = alloc_stack(g_rbf_length + 2);
	cosy = alloc_stack(g_rbf_length + 2);
	tmp.x = alloc_stack(g_rbf_length + 2);
	tmp.y = alloc_stack(g_rbf_length + 2);

	// 0 raised to anything is 0
	if (is_bf_zero(xx->x) && is_bf_zero(xx->y))
	{
		clear_bf(t->x);
		clear_bf(t->y);
		return t;
	}

	complex_log_bf(t, xx);
	cplxmul_bf(&tmp, t, yy);
	exp_bf(e2x, tmp.x);
	sincos_bf(siny, cosy, tmp.y);
	mult_bf(t->x, e2x, cosy);
	mult_bf(t->y, e2x, siny);
	restore_stack(saved);
	return t;
}

ComplexBigNum *complex_log_bn(ComplexBigNum *t, ComplexBigNum *s)
{
	square_bn(t->x, s->x);
	square_bn(t->y, s->y);
	add_a_bn(t->x + g_shift_factor, t->y + g_shift_factor);
	ln_bn(t->x, t->x + g_shift_factor);
	half_a_bn(t->x);
	atan2_bn(t->y, s->y, s->x);
	return t;
}

ComplexBigNum *complex_multiply_bn(ComplexBigNum *t, ComplexBigNum *x, ComplexBigNum *y)
{
	bn_t tmp1;
	int saved = save_stack();
	tmp1 = alloc_stack(g_r_length);
	mult_bn(t->x, x->x, y->x);
	mult_bn(t->y, x->y, y->y);
	sub_bn(t->x, t->x + g_shift_factor, t->y + g_shift_factor);

	mult_bn(tmp1, x->x, y->y);
	mult_bn(t->y, x->y, y->x);
	add_bn(t->y, tmp1 + g_shift_factor, t->y + g_shift_factor);
	restore_stack(saved);
	return t;
}

// note: complex_power_bn() returns need to be +g_shift_factor'ed
ComplexBigNum *complex_power_bn(ComplexBigNum *t, ComplexBigNum *xx, ComplexBigNum *yy)
{
	ComplexBigNum tmp;
	bn_t e2x, siny, cosy;
	int saved = save_stack();
	e2x = alloc_stack(g_bn_length);
	siny = alloc_stack(g_bn_length);
	cosy = alloc_stack(g_bn_length);
	tmp.x = alloc_stack(g_r_length);
	tmp.y = alloc_stack(g_r_length);

	// 0 raised to anything is 0
	if (is_bn_zero(xx->x) && is_bn_zero(xx->y))
	{
		clear_bn(t->x);
		clear_bn(t->y);
		return t;
	}

	complex_log_bn(t, xx);
	complex_multiply_bn(&tmp, t, yy);
	exp_bn(e2x, tmp.x);
	sincos_bn(siny, cosy, tmp.y);
	mult_bn(t->x, e2x, cosy);
	mult_bn(t->y, e2x, siny);
	restore_stack(saved);
	return t;
}
