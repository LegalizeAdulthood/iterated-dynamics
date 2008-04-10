//
//	This file contains the "big number" high precision versions of the
//	fractal routines.
//
#include <climits>
#include <string>

#include "port.h"
#include "prototyp.h"
#include "helpdefs.h"
#include "fractype.h"

#include "biginit.h"
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

	square_bn(bntmpsqrx, g_new_z_bn.real());
	square_bn(bntmpsqry, g_new_z_bn.imag());
	add_bn(bntmp, bn_t(bntmpsqrx, g_shift_factor), bn_t(bntmpsqry, g_shift_factor));

	longmagnitude = bntoint(bntmp);  // works with any fractal type
	if (longmagnitude >= long(g_rq_limit))
	{
		return 1;
	}
	copy_bn(g_old_z_bn.real(), g_new_z_bn.real());
	copy_bn(g_old_z_bn.imag(), g_new_z_bn.imag());
	return 0;
}

int bail_out_real_bn()
{
	long longtempsqrx;

	square_bn(bntmpsqrx, g_new_z_bn.real());
	square_bn(bntmpsqry, g_new_z_bn.imag());
	longtempsqrx = bntoint(bn_t(bntmpsqrx, g_shift_factor));
	if (longtempsqrx >= long(g_rq_limit))
	{
		return 1;
	}
	copy_bn(g_old_z_bn.real(), g_new_z_bn.real());
	copy_bn(g_old_z_bn.imag(), g_new_z_bn.imag());
	return 0;
}


int bail_out_imag_bn()
{
	long longtempsqry;

	square_bn(bntmpsqrx, g_new_z_bn.real());
	square_bn(bntmpsqry, g_new_z_bn.imag());
	longtempsqry = bntoint(bn_t(bntmpsqry, g_shift_factor));
	if (longtempsqry >= long(g_rq_limit))
	{
		return 1;
	}
	copy_bn(g_old_z_bn.real(), g_new_z_bn.real());
	copy_bn(g_old_z_bn.imag(), g_new_z_bn.imag());
	return 0;
}

int bail_out_or_bn()
{
	long longtempsqrx;
	long longtempsqry;

	square_bn(bntmpsqrx, g_new_z_bn.real());
	square_bn(bntmpsqry, g_new_z_bn.imag());
	longtempsqrx = bntoint(bn_t(bntmpsqrx, g_shift_factor));
	longtempsqry = bntoint(bn_t(bntmpsqry, g_shift_factor));
	if (longtempsqrx >= long(g_rq_limit) || longtempsqry >= long(g_rq_limit))
	{
		return 1;
	}
	copy_bn(g_old_z_bn.real(), g_new_z_bn.real());
	copy_bn(g_old_z_bn.imag(), g_new_z_bn.imag());
	return 0;
}

int bail_out_and_bn()
{
	long longtempsqrx;
	long longtempsqry;

	square_bn(bntmpsqrx, g_new_z_bn.real());
	square_bn(bntmpsqry, g_new_z_bn.imag());
	longtempsqrx = bntoint(bn_t(bntmpsqrx, g_shift_factor));
	longtempsqry = bntoint(bn_t(bntmpsqry, g_shift_factor));
	if (longtempsqrx >= long(g_rq_limit) && longtempsqry >= long(g_rq_limit))
	{
		return 1;
	}
	copy_bn(g_old_z_bn.real(), g_new_z_bn.real());
	copy_bn(g_old_z_bn.imag(), g_new_z_bn.imag());
	return 0;
}

int bail_out_manhattan_bn()
{
	long longtempmag;

	square_bn(bntmpsqrx, g_new_z_bn.real());
	square_bn(bntmpsqry, g_new_z_bn.imag());
	// note: in next five lines, g_old_z_bn is just used as a temporary variable
	abs_bn(g_old_z_bn.real(), g_new_z_bn.real());
	abs_bn(g_old_z_bn.imag(), g_new_z_bn.imag());
	add_bn(bntmp, g_old_z_bn.real(), g_old_z_bn.imag());
	square_bn(g_old_z_bn.real(), bntmp);
	longtempmag = bntoint(bn_t(g_old_z_bn.real(), g_shift_factor));
	if (longtempmag >= long(g_rq_limit))
	{
		return 1;
	}
	copy_bn(g_old_z_bn.real(), g_new_z_bn.real());
	copy_bn(g_old_z_bn.imag(), g_new_z_bn.imag());
	return 0;
}

int bail_out_manhattan_r_bn()
{
	long longtempmag;

	square_bn(bntmpsqrx, g_new_z_bn.real());
	square_bn(bntmpsqry, g_new_z_bn.imag());
	add_bn(bntmp, g_new_z_bn.real(), g_new_z_bn.imag()); // don't need abs since we square it next
	// note: in next two lines, g_old_z_bn is just used as a temporary variable
	square_bn(g_old_z_bn.real(), bntmp);
	longtempmag = bntoint(bn_t(g_old_z_bn.real(), g_shift_factor));
	if (longtempmag >= long(g_rq_limit))
	{
		return 1;
	}
	copy_bn(g_old_z_bn.real(), g_new_z_bn.real());
	copy_bn(g_old_z_bn.imag(), g_new_z_bn.imag());
	return 0;
}

int bail_out_mod_bf()
{
	long longmagnitude;

	square_bf(bftmpsqrx, g_new_z_bf.real());
	square_bf(bftmpsqry, g_new_z_bf.imag());
	add_bf(bftmp, bftmpsqrx, bftmpsqry);

	longmagnitude = bftoint(bftmp);
	if (longmagnitude >= long(g_rq_limit))
	{
		return 1;
	}
	copy_bf(g_old_z_bf.real(), g_new_z_bf.real());
	copy_bf(g_old_z_bf.imag(), g_new_z_bf.imag());
	return 0;
}

int bail_out_real_bf()
{
	long longtempsqrx;

	square_bf(bftmpsqrx, g_new_z_bf.real());
	square_bf(bftmpsqry, g_new_z_bf.imag());
	longtempsqrx = bftoint(bftmpsqrx);
	if (longtempsqrx >= long(g_rq_limit))
	{
		return 1;
	}
	copy_bf(g_old_z_bf.real(), g_new_z_bf.real());
	copy_bf(g_old_z_bf.imag(), g_new_z_bf.imag());
	return 0;
}

int bail_out_imag_bf()
{
	long longtempsqry;

	square_bf(bftmpsqrx, g_new_z_bf.real());
	square_bf(bftmpsqry, g_new_z_bf.imag());
	longtempsqry = bftoint(bftmpsqry);
	if (longtempsqry >= long(g_rq_limit))
	{
		return 1;
	}
	copy_bf(g_old_z_bf.real(), g_new_z_bf.real());
	copy_bf(g_old_z_bf.imag(), g_new_z_bf.imag());
	return 0;
}

int bail_out_or_bf()
{
	long longtempsqrx;
	long longtempsqry;

	square_bf(bftmpsqrx, g_new_z_bf.real());
	square_bf(bftmpsqry, g_new_z_bf.imag());
	longtempsqrx = bftoint(bftmpsqrx);
	longtempsqry = bftoint(bftmpsqry);
	if (longtempsqrx >= long(g_rq_limit) || longtempsqry >= long(g_rq_limit))
	{
		return 1;
	}
	copy_bf(g_old_z_bf.real(), g_new_z_bf.real());
	copy_bf(g_old_z_bf.imag(), g_new_z_bf.imag());
	return 0;
}

int bail_out_and_bf()
{
	long longtempsqrx;
	long longtempsqry;

	square_bf(bftmpsqrx, g_new_z_bf.real());
	square_bf(bftmpsqry, g_new_z_bf.imag());
	longtempsqrx = bftoint(bftmpsqrx);
	longtempsqry = bftoint(bftmpsqry);
	if (longtempsqrx >= long(g_rq_limit) && longtempsqry >= long(g_rq_limit))
	{
		return 1;
	}
	copy_bf(g_old_z_bf.real(), g_new_z_bf.real());
	copy_bf(g_old_z_bf.imag(), g_new_z_bf.imag());
	return 0;
}

int bail_out_manhattan_bf()
{
	long longtempmag;

	square_bf(bftmpsqrx, g_new_z_bf.real());
	square_bf(bftmpsqry, g_new_z_bf.imag());
	// note: in next five lines, g_old_z_bf is just used as a temporary variable
	abs_bf(g_old_z_bf.real(), g_new_z_bf.real());
	abs_bf(g_old_z_bf.imag(), g_new_z_bf.imag());
	add_bf(bftmp, g_old_z_bf.real(), g_old_z_bf.imag());
	square_bf(g_old_z_bf.real(), bftmp);
	longtempmag = bftoint(g_old_z_bf.real());
	if (longtempmag >= long(g_rq_limit))
	{
		return 1;
	}
	copy_bf(g_old_z_bf.real(), g_new_z_bf.real());
	copy_bf(g_old_z_bf.imag(), g_new_z_bf.imag());
	return 0;
}

int bail_out_manhattan_r_bf()
{
	long longtempmag;

	square_bf(bftmpsqrx, g_new_z_bf.real());
	square_bf(bftmpsqry, g_new_z_bf.imag());
	add_bf(bftmp, g_new_z_bf.real(), g_new_z_bf.imag()); // don't need abs since we square it next
	// note: in next two lines, g_old_z_bf is just used as a temporary variable
	square_bf(g_old_z_bf.real(), bftmp);
	longtempmag = bftoint(g_old_z_bf.real());
	if (longtempmag >= long(g_rq_limit))
	{
		return 1;
	}
	copy_bf(g_old_z_bf.real(), g_new_z_bf.real());
	copy_bf(g_old_z_bf.imag(), g_new_z_bf.imag());
	return 0;
}

bool mandelbrot_setup_bn()
{
	// this should be set up dynamically based on corners
	BigStackSaver savedStack;
	bn_t bntemp1(g_bn_length);
	bn_t bntemp2(g_bn_length);

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
		int t = std::abs(g_periodicity_check);
		while (t--)
		{
			half_a_bn(bnclosenuff);
		}
	}

	g_c_exp = int(g_parameters[P2_REAL]);
	switch (g_fractal_type)
	{
	case FRACTYPE_JULIA_FP:
		bftobn(bnparm.real(), bfparms[0]);
		bftobn(bnparm.imag(), bfparms[1]);
		break;
	case FRACTYPE_MANDELBROT_Z_POWER_FP:
		init_big_pi();
		if (double(g_c_exp) == g_parameters[P2_REAL] && (g_c_exp & 1)) // odd exponents
		{
			g_symmetry = SYMMETRY_XY_AXIS_NO_PARAMETER;
		}
		if (g_parameters[P2_IMAG] != 0)
		{
			g_symmetry = SYMMETRY_NONE;
		}
		break;
	case FRACTYPE_JULIA_Z_POWER_FP:
		init_big_pi();
		bftobn(bnparm.real(), bfparms[0]);
		bftobn(bnparm.imag(), bfparms[1]);
		if ((g_c_exp & 1) || g_parameters[P2_IMAG] != 0.0 || double(g_c_exp) != g_parameters[P2_REAL])
		{
			g_symmetry = SYMMETRY_NONE;
		}
		break;
	}

	return true;
}

bool mandelbrot_setup_bf()
{
	// this should be set up dynamically based on corners
	BigStackSaver savedStack;
	bf_t bftemp1(g_bf_length);
	bf_t bftemp2(g_bf_length);

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
		int t = std::abs(g_periodicity_check);
		while (t--)
		{
			half_a_bf(bfclosenuff);
		}
	}

	g_c_exp = int(g_parameters[P2_REAL]);
	switch (g_fractal_type)
	{
	case FRACTYPE_JULIA_FP:
		copy_bf(bfparm.real(), bfparms[0]);
		copy_bf(bfparm.imag(), bfparms[1]);
		break;
	case FRACTYPE_MANDELBROT_Z_POWER_FP:
		init_big_pi();
		if (double(g_c_exp) == g_parameters[P2_REAL] && (g_c_exp & 1)) // odd exponents
		{
			g_symmetry = SYMMETRY_XY_AXIS_NO_PARAMETER;
		}
		if (g_parameters[P2_IMAG] != 0)
		{
			g_symmetry = SYMMETRY_NONE;
		}
		break;
	case FRACTYPE_JULIA_Z_POWER_FP:
		init_big_pi();
		copy_bf(bfparm.real(), bfparms[0]);
		copy_bf(bfparm.imag(), bfparms[1]);
		if ((g_c_exp & 1) || g_parameters[P2_IMAG] != 0.0 || double(g_c_exp) != g_parameters[P2_REAL])
		{
			g_symmetry = SYMMETRY_NONE;
		}
		break;
	}

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
	// g_parameter.real() = xmin + col*delx + row*g_delta_x2
	mult_bn_int(bnparm.real(), bnxdel, (U16)g_col);
	mult_bn_int(bntmp, bnxdel2, (U16)g_row);

	add_a_bn(bnparm.real(), bntmp);
	add_a_bn(bnparm.real(), bnxmin);

	// g_parameter.imag() = ymax - row*dely - col*g_delta_y2;
	// note: in next four lines, g_old_z_bn is just used as a temporary variable
	mult_bn_int(g_old_z_bn.real(), bnydel,  (U16)g_row);
	mult_bn_int(g_old_z_bn.imag(), bnydel2, (U16)g_col);
	add_a_bn(g_old_z_bn.real(), g_old_z_bn.imag());
	sub_bn(bnparm.imag(), bnymax, g_old_z_bn.real());

	copy_bn(g_old_z_bn.real(), bnparm.real());
	copy_bn(g_old_z_bn.imag(), bnparm.imag());

	if (inside_coloring_beauty_of_fractals_allowed())
	{
		// kludge to match "Beauty of Fractals" picture since we start
		// Mandelbrot iteration with init rather than 0
		floattobn(g_old_z_bn.real(), g_parameters[P1_REAL]); // initial pertubation of parameters set
		floattobn(g_old_z_bn.imag(), g_parameters[P1_IMAG]);
		g_color_iter = -1;
	}
	else
	{
		floattobn(g_new_z_bn.real(), g_parameters[P1_REAL]);
		floattobn(g_new_z_bn.imag(), g_parameters[P1_IMAG]);
		add_a_bn(g_old_z_bn.real(), g_new_z_bn.real());
		add_a_bn(g_old_z_bn.imag(), g_new_z_bn.imag());
	}

	// square has side effect - must copy first
	copy_bn(g_new_z_bn.real(), g_old_z_bn.real());
	copy_bn(g_new_z_bn.imag(), g_old_z_bn.imag());

	// Square these to g_r_length bytes of precision
	square_bn(bntmpsqrx, g_new_z_bn.real());
	square_bn(bntmpsqry, g_new_z_bn.imag());

	return 1;                  // 1st iteration has been done
}

int mandelbrot_per_pixel_bf()
{
	// g_parameter.real() = xmin + col*delx + row*g_delta_x2
	mult_bf_int(bfparm.real(), bfxdel, (U16)g_col);
	mult_bf_int(bftmp, bfxdel2, (U16)g_row);

	add_a_bf(bfparm.real(), bftmp);
	add_a_bf(bfparm.real(), g_escape_time_state.m_grid_bf.x_min());

	// g_parameter.imag() = ymax - row*dely - col*g_delta_y2;
	// note: in next four lines, g_old_z_bf is just used as a temporary variable
	mult_bf_int(g_old_z_bf.real(), bfydel,  (U16)g_row);
	mult_bf_int(g_old_z_bf.imag(), bfydel2, (U16)g_col);
	add_a_bf(g_old_z_bf.real(), g_old_z_bf.imag());
	sub_bf(bfparm.imag(), g_escape_time_state.m_grid_bf.y_max(), g_old_z_bf.real());

	copy_bf(g_old_z_bf.real(), bfparm.real());
	copy_bf(g_old_z_bf.imag(), bfparm.imag());

	if (inside_coloring_beauty_of_fractals_allowed())
	{
		// kludge to match "Beauty of Fractals" picture since we start
		// Mandelbrot iteration with g_initial_z rather than 0
		floattobf(g_old_z_bf.real(), g_parameters[P1_REAL]); // initial pertubation of parameters set
		floattobf(g_old_z_bf.imag(), g_parameters[P1_IMAG]);
		g_color_iter = -1;
	}
	else
	{
		floattobf(g_new_z_bf.real(), g_parameters[P1_REAL]);
		floattobf(g_new_z_bf.imag(), g_parameters[P1_IMAG]);
		add_a_bf(g_old_z_bf.real(), g_new_z_bf.real());
		add_a_bf(g_old_z_bf.imag(), g_new_z_bf.imag());
	}

	// square has side effect - must copy first
	copy_bf(g_new_z_bf.real(), g_old_z_bf.real());
	copy_bf(g_new_z_bf.imag(), g_old_z_bf.imag());

	// Square these to g_rbf_length bytes of precision
	square_bf(bftmpsqrx, g_new_z_bf.real());
	square_bf(bftmpsqry, g_new_z_bf.imag());

	return 1;                  // 1st iteration has been done
}

int julia_per_pixel_bn()
{
	// old.real() = xmin + col*delx + row*g_delta_x2
	mult_bn_int(g_old_z_bn.real(), bnxdel, (U16)g_col);
	mult_bn_int(bntmp, bnxdel2, (U16)g_row);

	add_a_bn(g_old_z_bn.real(), bntmp);
	add_a_bn(g_old_z_bn.real(), bnxmin);

	// old.imag() = ymax - row*dely - col*g_delta_y2;
	// note: in next four lines, g_new_z_bn is just used as a temporary variable
	mult_bn_int(g_new_z_bn.real(), bnydel,  (U16)g_row);
	mult_bn_int(g_new_z_bn.imag(), bnydel2, (U16)g_col);
	add_a_bn(g_new_z_bn.real(), g_new_z_bn.imag());
	sub_bn(g_old_z_bn.imag(), bnymax, g_new_z_bn.real());

	// square has side effect - must copy first
	copy_bn(g_new_z_bn.real(), g_old_z_bn.real());
	copy_bn(g_new_z_bn.imag(), g_old_z_bn.imag());

	// Square these to g_r_length bytes of precision
	square_bn(bntmpsqrx, g_new_z_bn.real());
	square_bn(bntmpsqry, g_new_z_bn.imag());

	return 1;                  // 1st iteration has been done
}

int julia_per_pixel_bf()
{
	// old.real() = xmin + col*delx + row*g_delta_x2
	mult_bf_int(g_old_z_bf.real(), bfxdel, (U16)g_col);
	mult_bf_int(bftmp, bfxdel2, (U16)g_row);

	add_a_bf(g_old_z_bf.real(), bftmp);
	add_a_bf(g_old_z_bf.real(), g_escape_time_state.m_grid_bf.x_min());

	// old.imag() = ymax - row*dely - col*g_delta_y2;
	// note: in next four lines, g_new_z_bf is just used as a temporary variable
	mult_bf_int(g_new_z_bf.real(), bfydel,  (U16)g_row);
	mult_bf_int(g_new_z_bf.imag(), bfydel2, (U16)g_col);
	add_a_bf(g_new_z_bf.real(), g_new_z_bf.imag());
	sub_bf(g_old_z_bf.imag(), g_escape_time_state.m_grid_bf.y_max(), g_new_z_bf.real());

	// square has side effect - must copy first
	copy_bf(g_new_z_bf.real(), g_old_z_bf.real());
	copy_bf(g_new_z_bf.imag(), g_old_z_bf.imag());

	// Square these to g_rbf_length bytes of precision
	square_bf(bftmpsqrx, g_new_z_bf.real());
	square_bf(bftmpsqry, g_new_z_bf.imag());

	return 1;                  // 1st iteration has been done
}

int julia_orbit_bn()
{
	// Don't forget, with bn_t numbers, after multiplying or squaring
	// you must shift over by g_shift_factor to get the bn number.

	// bntmpsqrx and bntmpsqry were previously squared before getting to
	// this function, so they must be shifted.

	// new.real(tmpsqrx - tmpsqry + g_parameter.real());
	sub_a_bn(bn_t(bntmpsqrx, g_shift_factor), bn_t(bntmpsqry, g_shift_factor));
	add_bn(g_new_z_bn.real(), bn_t(bntmpsqrx, g_shift_factor), bnparm.real());

	// new.imag() = 2*g_old_z_bn.real()*g_old_z_bn.imag() + g_parameter.imag();
	mult_bn(bntmp, g_old_z_bn.real(), g_old_z_bn.imag()); // ok to use unsafe here
	double_a_bn(bn_t(bntmp, g_shift_factor));
	add_bn(g_new_z_bn.imag(), bn_t(bntmp, g_shift_factor), bnparm.imag());

	return g_externs.BailOutBn();
}

int julia_orbit_bf()
{
	// new.real(tmpsqrx - tmpsqry + g_parameter.real());
	sub_a_bf(bftmpsqrx, bftmpsqry);
	add_bf(g_new_z_bf.real(), bftmpsqrx, bfparm.real());

	// new.imag() = 2*g_old_z_bf.real()*g_old_z_bf.imag() + g_parameter.imag();
	mult_bf(bftmp, g_old_z_bf.real(), g_old_z_bf.imag()); // ok to use unsafe here
	double_a_bf(bftmp);
	add_bf(g_new_z_bf.imag(), bftmp, bfparm.imag());
	return g_externs.BailOutBf();
}

int julia_z_power_orbit_bn()
{
	ComplexBigNum parm2;
	BigStackSaver savedStack;

	parm2.real(bn_t(g_bn_length));
	parm2.imag(bn_t(g_bn_length));

	floattobn(parm2.real(), g_parameters[P2_REAL]);
	floattobn(parm2.imag(), g_parameters[P2_IMAG]);
	complex_power_bn(&g_new_z_bn, &g_old_z_bn, &parm2);
	add_bn(g_new_z_bn.real(), bnparm.real(), bn_t(g_new_z_bn.real(), g_shift_factor));
	add_bn(g_new_z_bn.imag(), bnparm.imag(), bn_t(g_new_z_bn.imag(), g_shift_factor));
	return g_externs.BailOutBn();
}

int julia_z_power_orbit_bf()
{
	ComplexBigFloat parm2;
	BigStackSaver savedStack;

	parm2.real(bf_t(g_bf_length));
	parm2.imag(bf_t(g_bf_length));

	floattobf(parm2.real(), g_parameters[P2_REAL]);
	floattobf(parm2.imag(), g_parameters[P2_IMAG]);
	ComplexPower_bf(&g_new_z_bf, &g_old_z_bf, &parm2);
	add_bf(g_new_z_bf.real(), bfparm.real(), g_new_z_bf.real());
	add_bf(g_new_z_bf.imag(), bfparm.imag(), g_new_z_bf.imag());
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

	// g_new_z_bn.real(bntmpsqrx - bntmpsqry + bnparm.real());
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
	add_bn(g_new_z_bn.real(), bntmpsqrx + g_shift_factor, bnparm.real());

	// new.imag() = 2*g_old_z_bn.real()*g_old_z_bn.imag() + old.imag();
	// Multiply g_old_z_bn.real()*g_old_z_bn.imag() to g_r_length precision.
	mult_bn(bntmp, g_old_z_bn.real(), g_old_z_bn.imag());

	//
	// Double g_old_z_bn.real()*g_old_z_bn.imag() by shifting bits, including one of those bits
	// calculated in the previous mult_bn().  Therefore, use g_r_length.
	//
	g_bn_length = g_r_length;
	double_a_bn(bntmp);
	g_bn_length = oldbnlength;

	// Convert back to a single width bignumber and add bnparm.imag()
	add_bn(g_new_z_bn.imag(), bntmp + g_shift_factor, bnparm.imag());

	copy_bn(g_old_z_bn.real(), g_new_z_bn.real());
	copy_bn(g_old_z_bn.imag(), g_new_z_bn.imag());

	// Square these to g_r_length bytes of precision
	square_bn(bntmpsqrx, g_old_z_bn.real());
	square_bn(bntmpsqry, g_old_z_bn.imag());

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

ComplexBigFloat *complex_log_bf(ComplexBigFloat *t, ComplexBigFloat *s)
{
	square_bf(t->real(), s->real());
	square_bf(t->imag(), s->imag());
	add_a_bf(t->real(), t->imag());
	ln_bf(t->real(), t->real());
	half_a_bf(t->real());
	atan2_bf(t->imag(), s->imag(), s->real());
	return t;
}

ComplexBigFloat *cplxmul_bf(ComplexBigFloat *t, ComplexBigFloat *x, ComplexBigFloat *y)
{
	BigStackSaver savedStack;
	bf_t tmp1(g_rbf_length);
	mult_bf(t->real(), x->real(), y->real());
	mult_bf(t->imag(), x->imag(), y->imag());
	sub_bf(t->real(), t->real(), t->imag());

	mult_bf(tmp1, x->real(), y->imag());
	mult_bf(t->imag(), x->imag(), y->real());
	add_bf(t->imag(), tmp1, t->imag());
	return t;
}

ComplexBigFloat *ComplexPower_bf(ComplexBigFloat *t, ComplexBigFloat *xx, ComplexBigFloat *yy)
{
	ComplexBigFloat tmp;
	BigStackSaver savedStack;
	bf_t e2x(g_rbf_length);
	bf_t siny(g_rbf_length);
	bf_t cosy(g_rbf_length);
	tmp.real(bf_t(g_rbf_length));
	tmp.imag(bf_t(g_rbf_length));

	// 0 raised to anything is 0
	if (is_bf_zero(xx->real()) && is_bf_zero(xx->imag()))
	{
		clear_bf(t->real());
		clear_bf(t->imag());
		return t;
	}

	complex_log_bf(t, xx);
	cplxmul_bf(&tmp, t, yy);
	exp_bf(e2x, tmp.real());
	sincos_bf(siny, cosy, tmp.imag());
	mult_bf(t->real(), e2x, cosy);
	mult_bf(t->imag(), e2x, siny);
	return t;
}

ComplexBigNum *complex_log_bn(ComplexBigNum *t, ComplexBigNum *s)
{
	square_bn(t->real(), s->real());
	square_bn(t->imag(), s->imag());
	add_a_bn(bn_t(t->real(), g_shift_factor), bn_t(t->imag(), g_shift_factor));
	ln_bn(t->real(), bn_t(t->real(), g_shift_factor));
	half_a_bn(t->real());
	atan2_bn(t->imag(), s->imag(), s->real());
	return t;
}

ComplexBigNum *complex_multiply_bn(ComplexBigNum *t, ComplexBigNum *x, ComplexBigNum *y)
{
	BigStackSaver savedStack;
	bn_t tmp1(g_r_length);
	mult_bn(t->real(), x->real(), y->real());
	mult_bn(t->imag(), x->imag(), y->imag());
	sub_bn(t->real(), bn_t(t->real(), g_shift_factor), bn_t(t->imag(), g_shift_factor));

	mult_bn(tmp1, x->real(), y->imag());
	mult_bn(t->imag(), x->imag(), y->real());
	add_bn(t->imag(), bn_t(tmp1, g_shift_factor), bn_t(t->imag(), g_shift_factor));
	return t;
}

// note: complex_power_bn() returns need to be +g_shift_factor'ed
ComplexBigNum *complex_power_bn(ComplexBigNum *t, ComplexBigNum *xx, ComplexBigNum *yy)
{
	BigStackSaver savedStack;
	bn_t e2x(g_bn_length);
	bn_t siny(g_bn_length);
	bn_t cosy(g_bn_length);
	ComplexBigNum tmp;
	tmp.real(bn_t(g_r_length));
	tmp.imag(bn_t(g_r_length));

	// 0 raised to anything is 0
	if (is_bn_zero(xx->real()) && is_bn_zero(xx->imag()))
	{
		clear_bn(t->real());
		clear_bn(t->imag());
		return t;
	}

	complex_log_bn(t, xx);
	complex_multiply_bn(&tmp, t, yy);
	exp_bn(e2x, tmp.real());
	sincos_bn(siny, cosy, tmp.imag());
	mult_bn(t->real(), e2x, cosy);
	mult_bn(t->imag(), e2x, siny);
	return t;
}
