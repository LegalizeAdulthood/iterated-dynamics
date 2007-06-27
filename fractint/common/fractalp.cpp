/*
		This module consists only of the g_fractal_specific structure
		and a *slew* of defines needed to get it to compile
*/
#include <string.h>

/* includes needed for g_fractal_specific */

/* see Fractint.cpp for a description of the include hierarchy */
#include "port.h"
#include "prototyp.h"
#include "helpdefs.h"
#include "fractype.h"

#include "ant.h"
#include "Browse.h"
#include "calcfrac.h"
#include "fractalb.h"
#include "fractalp.h"
#include "fractals.h"
#include "frasetup.h"
#include "jb.h"
#include "lorenz.h"
#include "lsys.h"
#include "miscfrac.h"
#include "prompts1.h"

#include "Halley.h"
#include "Newton.h"
#include "Formula.h"

/* functions defined elswhere needed for g_fractal_specific */
/* moved to prototyp.h */

/* parameter descriptions */
/* Note: parameters preceded by + are integer parameters */
/*       parameters preceded by # are U32 parameters */

/* for Mandelbrots */
static char s_real_z0[] = "Real Perturbation of Z(0)";
static char s_imag_z0[] = "Imaginary Perturbation of Z(0)";

/* for Julias */
static char s_parameter_real[] = "Real Part of Parameter";
static char s_parameter_imag[] = "Imaginary Part of Parameter";

/* for Newtons */
static char s_newton_degree[] = "+Polynomial Degree (>= 2)";

/* for MarksMandel/Julia */
static char s_exponent[]   = "Real part of Exponent";
static char s_exponent_imag[] = "Imag part of Exponent";

/* for Lorenz */
static char s_time_step[]     = "Time Step";

/* for formula */
static char s_p1_real[] = "Real portion of p1";
static char s_p2_real[] = "Real portion of p2";
static char s_p3_real[] = "Real portion of p3";
static char s_p4_real[] = "Real portion of p4";
static char s_p5_real[] = "Real portion of p5";
static char s_p1_imag[] = "Imaginary portion of p1";
static char s_p2_imag[] = "Imaginary portion of p2";
static char s_p3_imag[] = "Imaginary portion of p3";
static char s_p4_imag[] = "Imaginary portion of p4";
static char s_p5_imag[] = "Imaginary portion of p5";

/* trig functions */
static char s_trig1_coefficient_re[] = "Real Coefficient First Function";
static char s_trig1_coefficient_im[] = "Imag Coefficient First Function";
static char s_trig2_coefficient_re[] = "Real Coefficient Second Function";
static char s_trig2_coefficient_im[] = "Imag Coefficient Second Function";

/* KAM Torus */
static char s_kam_angle[] = "Angle (radians)";
static char s_kam_step[] =  "Step size";
static char s_kam_stop[] =  "Stop value";
static char s_points_per_orbit[] = "+Points per orbit";

/* popcorn and julia popcorn generalized */
static char s_step_x[] = "Step size (real)";
static char s_step_y[] = "Step size (imaginary)";
static char s_constant_x[] = "Constant C (real)";
static char s_constant_y[] = "Constant C (imaginary)";

/* bifurcations */
static char s_filter_cycles[] = "+Filter Cycles";
static char s_seed_population[] = "Seed Population";

/* frothy basins */
static char s_frothy_mapping[] = "+Apply mapping once (1) or twice (2)";
static char s_frothy_shade[] =  "+Enter non-zero value for alternate color shading";
static char s_frothy_a_value[] =  "A (imaginary part of C)";

/* plasma and ant */

static char s_random_seed[] = "+Random Seed Value (0 = Random, 1 = Reuse Last)";

/* ifs */
static char s_color_method[] = "+Coloring method (0,1)";

/* phoenix fractals */
static char s_degree_z[] = "Degree = 0 | >= 2 | <= -3";

/* julia inverse */
static char s_max_hits_per_pixel[] = "Max Hits per Pixel";

/* halley */
static char s_order[] = {"+Order (integer > 1)"};
static char s_real_relaxation_coefficient[] = {"Real Relaxation coefficient"};
static char s_epsilon[] = {"Epsilon"};
static char s_imag_relaxation_coefficient[] = {"Imag Relaxation coefficient"};

/* bailout defines */
#define ORBIT_BAILOUT_TRIG_L			64
#define ORBIT_BAILOUT_FROTHY_BASIN	7
#define ORBIT_BAILOUT_STANDARD		4
#define ORBIT_BAILOUT_NONE			0

more_parameters g_more_parameters[] =
{
	{FRACTYPE_ICON             , { "Omega", "+Degree of symmetry",   "", "", "", ""}, {0, 3, 0, 0, 0, 0}},
	{FRACTYPE_ICON_3D           , { "Omega", "+Degree of symmetry",   "", "", "", ""}, {0, 3, 0, 0, 0, 0}},
	{FRACTYPE_HYPERCOMPLEX_JULIA_FP    , { "zj",      "zk",          "", "", "", ""}, {0, 0, 0, 0, 0, 0}},
	{FRACTYPE_QUATERNION_JULIA_FP        , { "zj",      "zk",          "", "", "", ""}, {0, 0, 0, 0, 0, 0}},
	{FRACTYPE_PHOENIX_COMPLEX      , { s_degree_z, "",          "", "", "", ""}, {0, 0, 0, 0, 0, 0}},
	{FRACTYPE_PHOENIX_COMPLEX_FP    , { s_degree_z, "",          "", "", "", ""}, {0, 0, 0, 0, 0, 0}},
	{FRACTYPE_MANDELBROT_PHOENIX_COMPLEX  , { s_degree_z, "",          "", "", "", ""}, {0, 0, 0, 0, 0, 0}},
	{FRACTYPE_MANDELBROT_PHOENIX_COMPLEX_FP, { s_degree_z, "",          "", "", "", ""}, {0, 0, 0, 0, 0, 0}},
	{FRACTYPE_FORMULA  , { s_p3_real, s_p3_imag, s_p4_real, s_p4_imag, s_p5_real, s_p5_imag}, {0, 0, 0, 0, 0, 0}},
	{FRACTYPE_FORMULA_FP , { s_p3_real, s_p3_imag, s_p4_real, s_p4_imag, s_p5_real, s_p5_imag}, {0, 0, 0, 0, 0, 0}},
	{FRACTYPE_ANT              , { "+Wrap?", s_random_seed, "", "", "", ""}, {1, 0, 0, 0, 0, 0}},
	{FRACTYPE_MANDELBROT_MIX4   , { s_p3_real, s_p3_imag,        "", "", "", ""}, {0, 0, 0, 0, 0, 0}},
	{-1               , { NULL, NULL, NULL, NULL, NULL, NULL    }, {0, 0, 0, 0, 0, 0}}
};

/*
	type  math orbitcalc fnct per_pixel fnct per_image fnct
	|-----|----|--------------|--------------|--------------| */
static alternate_math s_alternate_math[] =
{
#define USEBN
#ifdef USEBN
	{
		FRACTYPE_JULIA_FP, BIGNUM,
		julia_orbit_bn, julia_per_pixel_bn, mandelbrot_setup_bn
	},
	{
		FRACTYPE_MANDELBROT_FP, BIGNUM,
		julia_orbit_bn, mandelbrot_per_pixel_bn, mandelbrot_setup_bn
	},
#else
	{
		FRACTYPE_JULIA_FP, BIGFLT,
		julia_orbit_bf, julia_per_pixel_bf,  mandelbrot_setup_bf
	},
	{
		FRACTYPE_MANDELBROT_FP, BIGFLT,
		julia_orbit_bf, mandelbrot_per_pixel_bf, mandelbrot_setup_bf
	},
#endif
	/*
		NOTE: The default precision for g_bf_math=BIGNUM is not high enough
		for julia_z_power_orbit_bn.  If you want to test BIGNUM (1) instead
		of the usual BIGFLT (2), then set bfdigits on the command to
		increase the precision.
	*/
	{
		FRACTYPE_JULIA_Z_POWER_FP, BIGFLT,
		julia_z_power_orbit_bf, julia_per_pixel_bf, mandelbrot_setup_bf
	},
	{
		FRACTYPE_MANDELBROT_Z_POWER_FP, BIGFLT,
		julia_z_power_orbit_bf, mandelbrot_per_pixel_bf, mandelbrot_setup_bf
	}
};

/* These are only needed for types with both integer and float variations */
static char s_barnsleyj1_name[] = "*barnsleyj1";
static char s_barnsleyj2_name[] = "*barnsleyj2";
static char s_barnsleyj3_name[] = "*barnsleyj3";
static char s_barnsleym1_name[] = "*barnsleym1";
static char s_barnsleym2_name[] = "*barnsleym2";
static char s_barnsleym3_name[] = "*barnsleym3";
static char s_bifplussinpi_name[] = "*bif+sinpi";
static char s_bifeqsinpi_name[] = "*bif=sinpi";
static char s_biflambda_name[] = "*biflambda";
static char s_bifmay_name[] = "*bifmay";
static char s_bifstewart_name[] = "*bifstewart";
static char s_bifurcation_name[] = "*bifurcation";
static char s_fn_z_plusfn_pix__name[] = "*fn(z)+fn(pix)";
static char s_fn_zz__name[] = "*fn(z*z)";
static char s_fnfn_name[] = "*fn*fn";
static char s_fnzplusz_name[] = "*fn*z+z";
static char s_fnplusfn_name[] = "*fn+fn";
static char s_formula_name[] = "*formula";
static char s_henon_name[] = "*henon";
static char s_ifs3d_name[] = "*ifs3d";
static char s_julfnplusexp_name[] = "*julfn+exp";
static char s_julfnpluszsqrd_name[] = "*julfn+zsqrd";
static char s_julia_name[] = "*julia";
static char s_julia_fnorfn__name[] = "*julia(fn||fn)";
static char s_julia4_name[] = "*julia4";
static char s_julia_inverse_name[] = "*julia_inverse";
static char s_julibrot_name[] = "*julibrot";
static char s_julzpower_name[] = "*julzpower";
static char s_kamtorus_name[] = "*kamtorus";
static char s_kamtorus3d_name[] = "*kamtorus3d";
static char s_lambda_name[] = "*lambda";
static char s_lambda_fnorfn__name[] = "*lambda(fn||fn)";
static char s_lambdafn_name[] = "*lambdafn";
static char s_lorenz_name[] = "*lorenz";
static char s_lorenz3d_name[] = "*lorenz3d";
static char s_mandel_name[] = "*mandel";
static char s_mandel_fnorfn__name[] = "*mandel(fn||fn)";
static char s_mandel4_name[] = "*mandel4";
static char s_mandelfn_name[] = "*mandelfn";
static char s_mandellambda_name[] = "*mandellambda";
static char s_mandphoenix_name[] = "*mandphoenix";
static char s_mandphoenixcplx_name[] = "*mandphoenixclx";
static char s_manfnplusexp_name[] = "*manfn+exp";
static char s_manfnpluszsqrd_name[] = "*manfn+zsqrd";
static char s_manlam_fnorfn__name[] = "*manlam(fn||fn)";
static char s_manowar_name[] = "*manowar";
static char s_manowarj_name[] = "*manowarj";
static char s_manzpower_name[] = "*manzpower";
static char s_marksjulia_name[] = "*marksjulia";
static char s_marksmandel_name[] = "*marksmandel";
static char s_marksmandelpwr_name[] = "*marksmandelpwr";
static char s_newtbasin_name[] = "*newtbasin";
static char s_newton_name[] = "*newton";
static char s_phoenix_name[] = "*phoenix";
static char s_phoenixcplx_name[] = "*phoenixcplx";
static char s_popcorn_name[] = "*popcorn";
static char s_popcornjul_name[] = "*popcornjul";
static char s_rossler3d_name[] = "*rossler3d";
static char s_sierpinski_name[] = "*sierpinski";
static char s_spider_name[] = "*spider";
static char s_sqr_1divfn__name[] = "*sqr(1/fn)";
static char s_sqr_fn__name[] = "*sqr(fn)";
static char s_tims_error_name[] = "*tim's_error";
static char s_unity_name[] = "*unity";
static char s_frothybasin_name[] = "*frothybasin";
static char s_halley_name[] = "*halley";

/* use next to cast orbitcalcs() that have arguments */
#define VF int(*)()

FractalTypeSpecificData g_fractal_specific[] =
{
	/*
	{ [OLDTYPEINDEX, ]NEWTYPEINDEX
		fractal type,
		fractal name,
		{parameter text strings},
		{parameter values},
		helptext, helpformula, flags,
		x_min, x_max, y_min, y_max,
		integer, tojulia, tomandel,
		tofloat, symmetry,
		orbit fnct, per_pixel fnct,
		per_image fnct, calctype fcnt,
		bailout
	}
	*/

	{
	FRACTYPE_MANDELBROT,
	s_mandel_name + 1,
		{s_real_z0, s_imag_z0, "", ""},
		{0, 0, 0, 0},
		HT_MANDEL, HF_MANDEL, FRACTALFLAG_BAIL_OUT_TESTS,
		-2.5f, 1.5f, -1.5f, 1.5f,
		1, FRACTYPE_JULIA, FRACTYPE_NO_FRACTAL,
		FRACTYPE_MANDELBROT_FP, SYMMETRY_X_AXIS_NO_PARAMETER,
		julia_orbit, mandelbrot_per_pixel,
		mandelbrot_setup, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{
	FRACTYPE_JULIA,
	s_julia_name + 1,
		{s_parameter_real, s_parameter_imag, "", ""},
		{0.3, 0.6, 0, 0},
		HT_JULIA, HF_JULIA, FRACTALFLAG_JULIBROT | FRACTALFLAG_BAIL_OUT_TESTS,
		-2.0f, 2.0f, -1.5f, 1.5f,
		1, FRACTYPE_NO_FRACTAL, FRACTYPE_MANDELBROT,
		FRACTYPE_JULIA_FP, SYMMETRY_ORIGIN,
		julia_orbit, julia_per_pixel,
		julia_setup, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{
	FRACTYPE_NEWTON_BASIN,
	s_newtbasin_name,
		{s_newton_degree, "Enter non-zero value for stripes", "", ""},
		{3, 0, 0, 0},
		HT_NEWTBAS, HF_NEWTBAS, 0,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		// TODO: FRACTYPE_..._MP
		FRACTYPE_NEWTON_BASIN_MP, SYMMETRY_NONE,
		newton2_orbit, other_julia_per_pixel_fp,
		newton_setup, standard_fractal,
		ORBIT_BAILOUT_NONE
	},

	{
	FRACTYPE_LAMBDA,
	s_lambda_name + 1,
		{s_parameter_real, s_parameter_imag, "", ""},
		{0.85, 0.6, 0, 0},
		HT_LAMBDA, HF_LAMBDA, FRACTALFLAG_JULIBROT | FRACTALFLAG_BAIL_OUT_TESTS,
		-1.5f, 2.5f, -1.5f, 1.5f,
		1, FRACTYPE_NO_FRACTAL, FRACTYPE_MANDELBROT_LAMBDA,
		FRACTYPE_LAMBDA_FP, SYMMETRY_NONE,
		lambda_orbit, julia_per_pixel,
		julia_setup_l, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{
	FRACTYPE_MANDELBROT_FP,
	s_mandel_name,
		{s_real_z0, s_imag_z0, "", ""},
		{0, 0, 0, 0},
		HT_MANDEL, HF_MANDEL, FRACTALFLAG_BAIL_OUT_TESTS | FRACTALFLAG_ARBITRARY_PRECISION,
		-2.5f, 1.5f, -1.5f, 1.5f,
		0, FRACTYPE_JULIA_FP, FRACTYPE_NO_FRACTAL,
		FRACTYPE_MANDELBROT, SYMMETRY_X_AXIS_NO_PARAMETER,
		julia_orbit_fp, mandelbrot_per_pixel_fp,
		mandelbrot_setup_fp, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{
	FRACTYPE_NEWTON,
	s_newton_name,
		{s_newton_degree, "", "", ""},
		{3, 0, 0, 0},
		HT_NEWT, HF_NEWT, 0,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		// TODO: FRACTYPE_..._MP
		FRACTYPE_NEWTON_MP, SYMMETRY_X_AXIS,
		newton2_orbit, other_julia_per_pixel_fp,
		newton_setup, standard_fractal,
		ORBIT_BAILOUT_NONE
	},

	{
	FRACTYPE_JULIA_FP,
	s_julia_name,
		{s_parameter_real, s_parameter_imag, "", ""},
		{0.3, 0.6, 0, 0},
		HT_JULIA, HF_JULIA, FRACTALFLAG_JULIBROT | FRACTALFLAG_BAIL_OUT_TESTS | FRACTALFLAG_ARBITRARY_PRECISION,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_MANDELBROT_FP,
		FRACTYPE_JULIA, SYMMETRY_ORIGIN,
		julia_orbit_fp, julia_per_pixel_fp,
		julia_setup_fp, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{
	FRACTYPE_PLASMA,
	"plasma",
		{"Graininess Factor (0 or 0.125 to 100, default is 2)",
		"+Algorithm (0 = original, 1 = new)",
		"+Random Seed Value (0 = Random, 1 = Reuse Last)",
		"+Save as Pot File? (0 = No,     1 = Yes)"
		},
		{2, 0, 0, 0},
		HT_PLASMA, HF_PLASMA, FRACTALFLAG_NO_ZOOM | FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_NOT_RESUMABLE,
		-2.0f, 2.0f, -1.5f, 1.5f,
		1, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_NONE,
		NULL, NULL,
		stand_alone_setup, plasma,
		ORBIT_BAILOUT_NONE
	},

	{ /* FRACTYPE_OBSOLETE_LAMBDA_SINE, FRACTYPE_MANDELBROT_FUNC_FP */
	FRACTYPE_MANDELBROT_FUNC_FP,
	s_mandelfn_name,
		{s_real_z0, s_imag_z0, "", ""},
		{0, 0, 0, 0},
		HT_MANDFN, HF_MANDFN, FRACTALFLAG_1_FUNCTION,
		-8.0f, 8.0f, -6.0f, 6.0f,
		0, FRACTYPE_LAMBDA_FUNC_FP, FRACTYPE_NO_FRACTAL,
		FRACTYPE_MANDELBROT_FUNC, SYMMETRY_XY_AXIS_NO_PARAMETER,
		lambda_trig_orbit_fp, other_mandelbrot_per_pixel_fp,
		mandelbrot_trig_setup, standard_fractal,
		ORBIT_BAILOUT_TRIG_L
	},

	{ /* FRACTYPE_OBSOLETE_LAMBDA_COS, FRACTYPE_MAN_O_WAR_FP */
	FRACTYPE_MAN_O_WAR_FP,
	s_manowar_name,
		{s_real_z0, s_imag_z0, "", ""},
		{0, 0, 0, 0},
		HT_SCOTSKIN, HF_MANOWAR, FRACTALFLAG_BAIL_OUT_TESTS,
		-2.5f, 1.5f, -1.5f, 1.5f,
		0, FRACTYPE_MAN_O_WAR_JULIA_FP, FRACTYPE_NO_FRACTAL,
		FRACTYPE_MAN_O_WAR, SYMMETRY_X_AXIS_NO_PARAMETER,
		man_o_war_orbit_fp, mandelbrot_per_pixel_fp,
		mandelbrot_setup_fp, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{ /* FRACTYPE_OBSOLETE_LAMBDA_EXP, FRACTYPE_MAN_O_WAR */
	FRACTYPE_MAN_O_WAR,
	s_manowar_name + 1,
		{s_real_z0, s_imag_z0, "", ""},
		{0, 0, 0, 0},
		HT_SCOTSKIN, HF_MANOWAR, FRACTALFLAG_BAIL_OUT_TESTS,
		-2.5f,  1.5f, -1.5f, 1.5f,
		1, FRACTYPE_MAN_O_WAR_JULIA, FRACTYPE_NO_FRACTAL,
		FRACTYPE_MAN_O_WAR_FP, SYMMETRY_X_AXIS_NO_PARAMETER,
		man_o_war_orbit, mandelbrot_per_pixel,
		mandelbrot_setup_l, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{
	FRACTYPE_TEST,
	"test",
		{"(testpt Param #1)",
		"(testpt param #2)",
		"(testpt param #3)",
		"(testpt param #4)"
		},
		{0, 0, 0, 0},
		HT_TEST, HF_TEST, 0,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_NONE,
		NULL, NULL,
		stand_alone_setup, test,
		ORBIT_BAILOUT_STANDARD
	},

	{
	FRACTYPE_SIERPINSKI,
	s_sierpinski_name + 1,
		{"", "", "", ""},
		{0, 0, 0, 0},
		HT_SIER, HF_SIER, 0,
		-4.0f/3.0f, 96.0f/45.0f, -0.9f, 1.7f,
		1, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_SIERPINSKI_FP, SYMMETRY_NONE,
		sierpinski_orbit, julia_per_pixel_l,
		sierpinski_setup, standard_fractal,
		127
	},

	{
	FRACTYPE_BARNSLEY_M1,
	s_barnsleym1_name + 1,
		{s_real_z0, s_imag_z0, "", ""},
		{0, 0, 0, 0},
		HT_BARNS, HF_BARNSM1, FRACTALFLAG_BAIL_OUT_TESTS,
		-2.0f, 2.0f, -1.5f, 1.5f,
		1, FRACTYPE_BARNSLEY_J1, FRACTYPE_NO_FRACTAL,
		FRACTYPE_BARNSLEY_M1_FP, SYMMETRY_XY_AXIS_NO_PARAMETER,
		barnsley1_orbit, mandelbrot_per_pixel_l,
		mandelbrot_setup_l, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{
	FRACTYPE_BARNSLEY_J1,
	s_barnsleyj1_name + 1,
		{s_parameter_real, s_parameter_imag, "", ""},
		{0.6, 1.1, 0, 0},
		HT_BARNS, HF_BARNSJ1, FRACTALFLAG_JULIBROT | FRACTALFLAG_BAIL_OUT_TESTS,
		-2.0f, 2.0f, -1.5f, 1.5f,
		1, FRACTYPE_NO_FRACTAL, FRACTYPE_BARNSLEY_M1,
		FRACTYPE_BARNSLEY_J1_FP, SYMMETRY_ORIGIN,
		barnsley1_orbit, julia_per_pixel_l,
		julia_setup_l, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{
	FRACTYPE_BARNSLEY_M2,
	s_barnsleym2_name + 1,
		{s_real_z0, s_imag_z0, "", ""},
		{0, 0, 0, 0},
		HT_BARNS, HF_BARNSM2, FRACTALFLAG_BAIL_OUT_TESTS,
		-2.0f, 2.0f, -1.5f, 1.5f,
		1, FRACTYPE_BARNSLEY_J2, FRACTYPE_NO_FRACTAL,
		FRACTYPE_BARNSLEY_M2_FP, SYMMETRY_Y_AXIS_NO_PARAMETER,
		barnsley2_orbit, mandelbrot_per_pixel_l,
		mandelbrot_setup_l, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{
	FRACTYPE_BARNSLEY_J2,
	s_barnsleyj2_name + 1,
		{s_parameter_real, s_parameter_imag, "", ""},
		{0.6, 1.1, 0, 0},
		HT_BARNS, HF_BARNSJ2, FRACTALFLAG_JULIBROT | FRACTALFLAG_BAIL_OUT_TESTS,
		-2.0f, 2.0f, -1.5f, 1.5f,
		1, FRACTYPE_NO_FRACTAL, FRACTYPE_BARNSLEY_M2,
		FRACTYPE_BARNSLEY_J2_FP, SYMMETRY_ORIGIN,
		barnsley2_orbit, julia_per_pixel_l,
		julia_setup_l, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{ /* FRACTYPE_OBSOLETE_MANDELBROT_SINE, FRACTYPE_SQR_FUNC */
	FRACTYPE_SQR_FUNC,
	s_sqr_fn__name + 1,
		{"", "", "", ""},
		{0, 0, 0, 0},
		HT_SCOTSKIN, HF_SQRFN, FRACTALFLAG_1_FUNCTION | FRACTALFLAG_BAIL_OUT_TESTS,
		-4.0f, 4.0f, -3.0f, 3.0f,
		16, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_SQR_FUNC_FP, SYMMETRY_X_AXIS,
		sqr_trig_orbit, julia_per_pixel_l,
		sqr_trig_setup, standard_fractal,
		ORBIT_BAILOUT_TRIG_L
	},

	{ /* FRACTYPE_OBSOLETE_MANDELBROT_COS, FRACTYPE_SQR_FUNC_FP */
	FRACTYPE_SQR_FUNC_FP,
	s_sqr_fn__name,
		{"", "", "", ""},
		{0, 0, 0, 0},
		HT_SCOTSKIN, HF_SQRFN, FRACTALFLAG_1_FUNCTION | FRACTALFLAG_BAIL_OUT_TESTS,
		-4.0f, 4.0f, -3.0f, 3.0f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_SQR_FUNC, SYMMETRY_X_AXIS,
		sqr_trig_orbit_fp, other_julia_per_pixel_fp,
		sqr_trig_setup, standard_fractal,
		ORBIT_BAILOUT_TRIG_L
	},

	{ /* FRACTYPE_OBSOLETE_MANDELBROT_EXP, FRACTYPE_FUNC_PLUS_FUNC */
	FRACTYPE_FUNC_PLUS_FUNC,
	s_fnplusfn_name + 1,
		{s_trig1_coefficient_re, s_trig1_coefficient_im, s_trig2_coefficient_re, s_trig2_coefficient_im},
		{1, 0, 1, 0},
		HT_SCOTSKIN, HF_FNPLUSFN, FRACTALFLAG_2_FUNCTIONS | FRACTALFLAG_BAIL_OUT_TESTS,
		-4.0f, 4.0f, -3.0f, 3.0f,
		16, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_FUNC_PLUS_FUNC_FP, SYMMETRY_X_AXIS,
		trig_plus_trig_orbit, julia_per_pixel_l,
		trig_plus_trig_setup_l, standard_fractal,
		ORBIT_BAILOUT_TRIG_L
	},

	{
	FRACTYPE_MANDELBROT_LAMBDA,
	s_mandellambda_name + 1,
		{s_real_z0, s_imag_z0, "", ""},
		{0, 0, 0, 0},
		HT_MLAMBDA, HF_MLAMBDA, FRACTALFLAG_BAIL_OUT_TESTS,
		-3.0f, 5.0f, -3.0f, 3.0f,
		1, FRACTYPE_LAMBDA, FRACTYPE_NO_FRACTAL,
		FRACTYPE_MANDELBROT_LAMBDA_FP, SYMMETRY_X_AXIS_NO_PARAMETER,
		lambda_orbit, mandelbrot_per_pixel,
		mandelbrot_setup_l, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{
	FRACTYPE_MARKS_MANDELBROT,
	s_marksmandel_name + 1,
		{s_real_z0, s_imag_z0, s_exponent, ""},
		{0, 0, 1, 0},
		HT_MARKS, HF_MARKSMAND, FRACTALFLAG_BAIL_OUT_TESTS,
		-2.0f, 2.0f, -1.5f, 1.5f,
		1, FRACTYPE_MARKS_JULIA, FRACTYPE_NO_FRACTAL,
		FRACTYPE_MARKS_MANDELBROT_FP, SYMMETRY_NONE,
		marks_lambda_orbit, marks_mandelbrot_per_pixel,
		mandelbrot_setup_l, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{
	FRACTYPE_MARKS_JULIA,
	s_marksjulia_name + 1,
		{s_parameter_real, s_parameter_imag, s_exponent, ""},
		{0.1, 0.9, 1, 0},
		HT_MARKS, HF_MARKSJULIA, FRACTALFLAG_BAIL_OUT_TESTS,
		-2.0f, 2.0f, -1.5f, 1.5f,
		1, FRACTYPE_NO_FRACTAL, FRACTYPE_MARKS_MANDELBROT,
		FRACTYPE_MARKS_JULIA_FP, SYMMETRY_ORIGIN,
		marks_lambda_orbit, julia_per_pixel,
		marks_julia_setup, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{
	FRACTYPE_UNITY,
	s_unity_name + 1,
		{"", "", "", ""},
		{0, 0, 0, 0},
		HT_UNITY, HF_UNITY, 0,
		-2.0f, 2.0f, -1.5f, 1.5f,
		1, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_UNITY_FP, SYMMETRY_XY_AXIS,
		unity_orbit, julia_per_pixel_l,
		unity_setup, standard_fractal,
		ORBIT_BAILOUT_NONE
	},

	{
	FRACTYPE_MANDELBROT_4,
	s_mandel4_name + 1,
		{s_real_z0, s_imag_z0, "", ""},
		{0, 0, 0, 0},
		HT_MANDJUL4, HF_MANDEL4, FRACTALFLAG_BAIL_OUT_TESTS,
		-2.0f, 2.0f, -1.5f, 1.5f,
		1, FRACTYPE_JULIA_4, FRACTYPE_NO_FRACTAL,
		FRACTYPE_MANDELBROT_4_FP, SYMMETRY_X_AXIS_NO_PARAMETER,
		mandel4_orbit, mandelbrot_per_pixel,
		mandelbrot_setup_l, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{
	FRACTYPE_JULIA_4,
	s_julia4_name + 1,
		{s_parameter_real, s_parameter_imag, "", ""},
		{0.6, 0.55, 0, 0},
		HT_MANDJUL4, HF_JULIA4, FRACTALFLAG_JULIBROT | FRACTALFLAG_BAIL_OUT_TESTS,
		-2.0f, 2.0f, -1.5f, 1.5f,
		1, FRACTYPE_NO_FRACTAL, FRACTYPE_MANDELBROT_4,
		FRACTYPE_JULIA_4_FP, SYMMETRY_ORIGIN,
		mandel4_orbit, julia_per_pixel,
		julia_setup_l, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{
	FRACTYPE_IFS,
	"ifs",
		{s_color_method, "", "", ""},
		{0, 0, 0, 0},
		HT_IFS, -4, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_NOT_RESUMABLE | FRACTALFLAG_INFINITE_CALCULATION,
		-8.0f, 8.0f, -1.0f, 11.0f,
		16, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_NONE,
		NULL, NULL,
		stand_alone_setup, ifs,
		ORBIT_BAILOUT_NONE
	},

	{
	FRACTYPE_IFS_3D,
	s_ifs3d_name,
		{s_color_method, "", "", ""},
		{0, 0, 0, 0},
		HT_IFS, -4, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_NOT_RESUMABLE | FRACTALFLAG_3D_PARAMETERS | FRACTALFLAG_INFINITE_CALCULATION,
		-11.0f, 11.0f, -11.0f, 11.0f,
		16, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_NONE,
		NULL, NULL,
		stand_alone_setup, ifs,
		ORBIT_BAILOUT_NONE
	},

	{
	FRACTYPE_BARNSLEY_M3,
	s_barnsleym3_name + 1,
		{s_real_z0, s_imag_z0, "", ""},
		{0, 0, 0, 0},
		HT_BARNS, HF_BARNSM3, FRACTALFLAG_BAIL_OUT_TESTS,
		-2.0f, 2.0f, -1.5f, 1.5f,
		1, FRACTYPE_BARNSLEY_J3, FRACTYPE_NO_FRACTAL,
		FRACTYPE_BARNSLEY_M3_FP, SYMMETRY_X_AXIS_NO_PARAMETER,
		barnsley3_orbit, mandelbrot_per_pixel_l,
		mandelbrot_setup_l, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{
	FRACTYPE_BARNSLEY_J3,
	s_barnsleyj3_name + 1,
		{s_parameter_real, s_parameter_imag, "", ""},
		{0.1, 0.36, 0, 0},
		HT_BARNS, HF_BARNSJ3, FRACTALFLAG_JULIBROT | FRACTALFLAG_BAIL_OUT_TESTS,
		-2.0f, 2.0f, -1.5f, 1.5f,
		1, FRACTYPE_NO_FRACTAL, FRACTYPE_BARNSLEY_M3,
		FRACTYPE_BARNSLEY_J3_FP, SYMMETRY_NONE,
		barnsley3_orbit, julia_per_pixel_l,
		julia_setup_l, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{ /* FRACTYPE_OBSOLETE_DEM_MANDELBROT, FRACTYPE_FUNC_SQR */
	FRACTYPE_FUNC_SQR,
	s_fn_zz__name + 1,
		{"", "", "", ""},
		{0, 0, 0, 0},
		HT_SCOTSKIN, HF_FNZTIMESZ, FRACTALFLAG_1_FUNCTION | FRACTALFLAG_BAIL_OUT_TESTS,
		-4.0f, 4.0f, -3.0f, 3.0f,
		16, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_FUNC_SQR_FP, SYMMETRY_XY_AXIS,
		trig_z_squared_orbit, julia_per_pixel,
		julia_setup_l, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{ /* FRACTYPE_OBSOLETE_DEM_JULIA, FRACTYPE_FUNC_SQR_FP */
	FRACTYPE_FUNC_SQR_FP,
	s_fn_zz__name,
		{"", "", "", ""},
		{0, 0, 0, 0},
		HT_SCOTSKIN, HF_FNZTIMESZ, FRACTALFLAG_1_FUNCTION | FRACTALFLAG_BAIL_OUT_TESTS,
		-4.0f, 4.0f, -3.0f, 3.0f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_FUNC_SQR, SYMMETRY_XY_AXIS,
		trig_z_squared_orbit_fp, julia_per_pixel_fp,
		julia_setup_fp, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{
	FRACTYPE_BIFURCATION,
	s_bifurcation_name,
		{s_filter_cycles, s_seed_population, "", ""},
		{1000.0, 0.66, 0, 0},
		HT_BIF, HF_BIFURCATION, FRACTALFLAG_1_FUNCTION | FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_NO_ZOOM_BOX_ROTATE,
		1.9f, 3.0f, 0.0f, 1.34f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_BIFURCATION_L, SYMMETRY_NONE,
		bifurcation_verhulst_trig_fp, NULL,
		stand_alone_setup, bifurcation,
		ORBIT_BAILOUT_NONE
	},

	{ /* FRACTYPE_OBSOLETE_MANDELBROT_SINH, FRACTYPE_FUNC_PLUS_FUNC_FP */
	FRACTYPE_FUNC_PLUS_FUNC_FP,
	s_fnplusfn_name,
		{s_trig1_coefficient_re, s_trig1_coefficient_im, s_trig2_coefficient_re, s_trig2_coefficient_im},
		{1, 0, 1, 0},
		HT_SCOTSKIN, HF_FNPLUSFN, FRACTALFLAG_2_FUNCTIONS | FRACTALFLAG_BAIL_OUT_TESTS,
		-4.0f, 4.0f, -3.0f, 3.0f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_FUNC_PLUS_FUNC, SYMMETRY_X_AXIS,
		trig_plus_trig_orbit_fp, other_julia_per_pixel_fp,
		trig_plus_trig_setup_fp, standard_fractal,
		ORBIT_BAILOUT_TRIG_L
	},

	{ /* FRACTYPE_OBSOLETE_LAMBDA_SINH, FRACTYPE_FUNC_TIMES_FUNC */
	FRACTYPE_FUNC_TIMES_FUNC,
	s_fnfn_name + 1,
		{"", "", "", ""},
		{0, 0, 0, 0},
		HT_SCOTSKIN, HF_FNTIMESFN, FRACTALFLAG_2_FUNCTIONS | FRACTALFLAG_BAIL_OUT_TESTS,
		-4.0f, 4.0f, -3.0f, 3.0f,
		16, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_FUNC_TIMES_FUNC_FP, SYMMETRY_X_AXIS,
		trig_trig_orbit, julia_per_pixel_l,
		fn_fn_setup, standard_fractal,
		ORBIT_BAILOUT_TRIG_L
	},

	{ /* FRACTYPE_OBSOLETE_MANDELBROT_COSH, FRACTYPE_FUNC_TIMES_FUNC_FP */
	FRACTYPE_FUNC_TIMES_FUNC_FP,
	s_fnfn_name,
		{"", "", "", ""},
		{0, 0, 0, 0},
		HT_SCOTSKIN, HF_FNTIMESFN, FRACTALFLAG_2_FUNCTIONS | FRACTALFLAG_BAIL_OUT_TESTS,
		-4.0f, 4.0f, -3.0f, 3.0f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_FUNC_TIMES_FUNC, SYMMETRY_X_AXIS,
		trig_trig_orbit_fp, other_julia_per_pixel_fp,
		fn_fn_setup, standard_fractal,
		ORBIT_BAILOUT_TRIG_L
	},

	{ /* FRACTYPE_OBSOLETE_LAMBDA_COSH, FRACTYPE_SQR_RECIPROCAL_FUNC */
	FRACTYPE_SQR_RECIPROCAL_FUNC,
	s_sqr_1divfn__name + 1,
		{"", "", "", ""},
		{0, 0, 0, 0},
		HT_SCOTSKIN, HF_SQROVFN, FRACTALFLAG_1_FUNCTION | FRACTALFLAG_BAIL_OUT_TESTS,
		-4.0f, 4.0f, -3.0f, 3.0f,
		16, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_SQR_RECIPROCAL_FUNC_FP, SYMMETRY_NONE,
		sqr_1_over_trig_z_orbit, julia_per_pixel_l,
		sqr_trig_setup, standard_fractal,
		ORBIT_BAILOUT_TRIG_L
	},

	{ /* FRACTYPE_OBSOLETE_MANDELBROT_SINE_L, FRACTYPE_SQR_RECIPROCAL_FUNC_FP */
	FRACTYPE_SQR_RECIPROCAL_FUNC_FP,
	s_sqr_1divfn__name,
		{"", "", "", ""},
		{0, 0, 0, 0},
		HT_SCOTSKIN, HF_SQROVFN, FRACTALFLAG_1_FUNCTION | FRACTALFLAG_BAIL_OUT_TESTS,
		-4.0f, 4.0f, -3.0f, 3.0f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_SQR_RECIPROCAL_FUNC, SYMMETRY_NONE,
		sqr_1_over_trig_z_orbit_fp, other_julia_per_pixel_fp,
		sqr_trig_setup, standard_fractal,
		ORBIT_BAILOUT_TRIG_L
	},

	{ /* FRACTYPE_OBSOLETE_LAMBDA_SINE_L, FRACTYPE_FUNC_TIMES_Z_PLUS_Z */
	FRACTYPE_FUNC_TIMES_Z_PLUS_Z,
	s_fnzplusz_name + 1,
		{s_trig1_coefficient_re, s_trig1_coefficient_im, "Real Coefficient Second Term", "Imag Coefficient Second Term"},
		{1, 0, 1, 0},
		HT_SCOTSKIN, HF_FNXZPLUSZ, FRACTALFLAG_1_FUNCTION | FRACTALFLAG_BAIL_OUT_TESTS,
		-4.0f, 4.0f, -3.0f, 3.0f,
		1, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_FUNC_TIMES_Z_PLUS_Z_FP, SYMMETRY_X_AXIS,
		z_trig_z_plus_z_orbit, julia_per_pixel,
		z_trig_plus_z_setup, standard_fractal,
		ORBIT_BAILOUT_TRIG_L
	},

	{ /* FRACTYPE_OBSOLETE_MANDELBROT_COS_L, FRACTYPE_FUNC_TIMES_Z_PLUS_Z_FP */
	FRACTYPE_FUNC_TIMES_Z_PLUS_Z_FP,
	s_fnzplusz_name,
		{s_trig1_coefficient_re, s_trig1_coefficient_im, "Real Coefficient Second Term", "Imag Coefficient Second Term"},
		{1, 0, 1, 0},
		HT_SCOTSKIN, HF_FNXZPLUSZ, FRACTALFLAG_1_FUNCTION | FRACTALFLAG_BAIL_OUT_TESTS,
		-4.0f, 4.0f, -3.0f, 3.0f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_FUNC_TIMES_Z_PLUS_Z, SYMMETRY_X_AXIS,
		z_trig_z_plus_z_orbit_fp, julia_per_pixel_fp,
		z_trig_plus_z_setup, standard_fractal,
		ORBIT_BAILOUT_TRIG_L
	},

	{ /* FRACTYPE_OBSOLETE_LAMBDA_COS_L, FRACTYPE_KAM_TORUS_FP */
	FRACTYPE_KAM_TORUS_FP,
	s_kamtorus_name,
		{s_kam_angle, s_kam_step, s_kam_stop, s_points_per_orbit},
		{1.3, .05, 1.5, 150},
		HT_KAM, HF_KAM, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING,
		-1.0f, 1.0f, -.75f, .75f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_KAM_TORUS, SYMMETRY_NONE,
		(VF) kam_torus_orbit_fp, NULL,
		orbit_3d_setup_fp, orbit_2d_fp,
		ORBIT_BAILOUT_NONE
	},

	{ /* FRACTYPE_OBSOLETE_MANDELBROT_SINH_L, FRACTYPE_KAM_TORUS */
	FRACTYPE_KAM_TORUS,
	s_kamtorus_name + 1,
		{s_kam_angle, s_kam_step, s_kam_stop, s_points_per_orbit},
		{1.3, .05, 1.5, 150},
		HT_KAM, HF_KAM, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING,
		-1.0f, 1.0f, -.75f, .75f,
		16, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_KAM_TORUS_FP, SYMMETRY_NONE,
		(VF) kam_torus_orbit, NULL,
		orbit_3d_setup, orbit_2d,
		ORBIT_BAILOUT_NONE
	},

	{ /* FRACTYPE_OBSOLETE_LAMBDA_SINH_L, FRACTYPE_KAM_TORUS_3D_FP */
	FRACTYPE_KAM_TORUS_3D_FP,
	s_kamtorus3d_name,
		{s_kam_angle, s_kam_step, s_kam_stop, s_points_per_orbit},
		{1.3, .05, 1.5, 150},
		HT_KAM, HF_KAM, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_NOT_RESUMABLE | FRACTALFLAG_3D_PARAMETERS,
		-3.0f, 3.0f, -1.0f, 3.5f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_KAM_TORUS_3D, SYMMETRY_NONE,
		(VF) kam_torus_orbit_fp, NULL,
		orbit_3d_setup_fp, orbit_3d_fp,
		ORBIT_BAILOUT_NONE
	},

	{ /* FRACTYPE_OBSOLETE_MANDELBROT_COSH_L, FRACTYPE_KAM_TORUS_3D */
	FRACTYPE_KAM_TORUS_3D,
	s_kamtorus3d_name + 1,
		{s_kam_angle, s_kam_step, s_kam_stop, s_points_per_orbit},
		{1.3, .05, 1.5, 150},
		HT_KAM, HF_KAM, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_NOT_RESUMABLE | FRACTALFLAG_3D_PARAMETERS,
		-3.0f, 3.0f, -1.0f, 3.5f,
		16, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_KAM_TORUS_3D_FP, SYMMETRY_NONE,
		(VF) kam_torus_orbit, NULL,
		orbit_3d_setup, orbit_3d,
		ORBIT_BAILOUT_NONE
	},

	{ /* FRACTYPE_OBSOLETE_LAMBDA_COSH_L, FRACTYPE_LAMBDA_FUNC */
	FRACTYPE_LAMBDA_FUNC,
	s_lambdafn_name + 1,
		{s_parameter_real, s_parameter_imag, "", ""},
		{1.0, 0.4, 0, 0},
		HT_LAMBDAFN, HF_LAMBDAFN, FRACTALFLAG_1_FUNCTION | FRACTALFLAG_JULIBROT,
		-4.0f, 4.0f, -3.0f, 3.0f,
		16, FRACTYPE_NO_FRACTAL, FRACTYPE_MANDELBROT_FUNC,
		FRACTYPE_LAMBDA_FUNC_FP, SYMMETRY_PI,
		(VF) lambda_trig_orbit, julia_per_pixel_l,
		lambda_trig_setup, standard_fractal,
		ORBIT_BAILOUT_TRIG_L
	},

	{
	FRACTYPE_MANDELBROT_FUNC_PLUS_Z_SQUARED_L,
	s_manfnpluszsqrd_name + 1,
		{s_real_z0, s_imag_z0, "", ""},
		{0, 0, 0, 0},
		HT_PICKMJ, HF_MANDFNPLUSZSQRD, FRACTALFLAG_1_FUNCTION | FRACTALFLAG_BAIL_OUT_TESTS,
		-2.5f, 1.5f, -1.5f, 1.5f,
		16, FRACTYPE_JULIA_FUNC_PLUS_Z_SQUARED_L, FRACTYPE_NO_FRACTAL,
		FRACTYPE_MANDELBROT_FUNC_PLUS_Z_SQUARED_FP, SYMMETRY_X_AXIS_NO_PARAMETER,
		trig_plus_z_squared_orbit, mandelbrot_per_pixel,
		mandelbrot_setup_l, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{
	FRACTYPE_JULIA_FUNC_PLUS_Z_SQUARED_L,
	s_julfnpluszsqrd_name + 1,
		{s_parameter_real, s_parameter_imag, "", ""},
		{-0.5, 0.5, 0, 0},
		HT_PICKMJ, HF_JULFNPLUSZSQRD, FRACTALFLAG_1_FUNCTION | FRACTALFLAG_JULIBROT | FRACTALFLAG_BAIL_OUT_TESTS,
		-2.0f, 2.0f, -1.5f, 1.5f,
		16, FRACTYPE_NO_FRACTAL, FRACTYPE_MANDELBROT_FUNC_PLUS_Z_SQUARED_L,
		FRACTYPE_JULIA_FUNC_PLUS_Z_SQUARED_FP, SYMMETRY_NONE,
		trig_plus_z_squared_orbit, julia_per_pixel,
		julia_fn_plus_z_squared_setup, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{
	FRACTYPE_MANDELBROT_FUNC_PLUS_Z_SQUARED_FP,
	s_manfnpluszsqrd_name,
		{s_real_z0, s_imag_z0, "", ""},
		{0, 0, 0, 0},
		HT_PICKMJ, HF_MANDFNPLUSZSQRD, FRACTALFLAG_1_FUNCTION | FRACTALFLAG_BAIL_OUT_TESTS,
		-2.5f, 1.5f, -1.5f, 1.5f,
		0, FRACTYPE_JULIA_FUNC_PLUS_Z_SQUARED_FP, FRACTYPE_NO_FRACTAL,
		FRACTYPE_MANDELBROT_FUNC_PLUS_Z_SQUARED_L, SYMMETRY_X_AXIS_NO_PARAMETER,
		trig_plus_z_squared_orbit_fp, mandelbrot_per_pixel_fp,
		mandelbrot_setup_fp, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{
	FRACTYPE_JULIA_FUNC_PLUS_Z_SQUARED_FP,
	s_julfnpluszsqrd_name,
		{s_parameter_real, s_parameter_imag, "", ""},
		{-0.5, 0.5, 0, 0},
		HT_PICKMJ, HF_JULFNPLUSZSQRD, FRACTALFLAG_1_FUNCTION | FRACTALFLAG_JULIBROT | FRACTALFLAG_BAIL_OUT_TESTS,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_MANDELBROT_FUNC_PLUS_Z_SQUARED_FP,
		FRACTYPE_JULIA_FUNC_PLUS_Z_SQUARED_L, SYMMETRY_NONE,
		trig_plus_z_squared_orbit_fp, julia_per_pixel_fp,
		julia_fn_plus_z_squared_setup, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{ /* FRACTYPE_OBSOLETE_MANDELBROT_EXP_L, FRACTYPE_LAMBDA_FUNC_FP */
	FRACTYPE_LAMBDA_FUNC_FP,
	s_lambdafn_name,
		{s_parameter_real, s_parameter_imag, "", ""},
		{1.0, 0.4, 0, 0},
		HT_LAMBDAFN, HF_LAMBDAFN, FRACTALFLAG_1_FUNCTION | FRACTALFLAG_JULIBROT,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_MANDELBROT_FUNC_FP, FRACTYPE_LAMBDA_FUNC, SYMMETRY_PI,
		lambda_trig_orbit_fp, other_julia_per_pixel_fp,
		lambda_trig_setup, standard_fractal,
		ORBIT_BAILOUT_TRIG_L
	},

	{ /* FRACTYPE_OBSOLETE_LAMBDA_EXP_L, FRACTYPE_MANDELBROT_FUNC */
	FRACTYPE_MANDELBROT_FUNC,
	s_mandelfn_name + 1,
		{s_real_z0, s_imag_z0, "", ""},
		{0, 0, 0, 0},
		HT_MANDFN, HF_MANDFN, FRACTALFLAG_1_FUNCTION,
		-8.0f, 8.0f, -6.0f, 6.0f,
		16, FRACTYPE_LAMBDA_FUNC, FRACTYPE_NO_FRACTAL,
		FRACTYPE_MANDELBROT_FUNC_FP, SYMMETRY_XY_AXIS_NO_PARAMETER,
		lambda_trig_orbit, mandelbrot_per_pixel_l,
		mandelbrot_trig_setup, standard_fractal,
		ORBIT_BAILOUT_TRIG_L
	},

	{
	FRACTYPE_MANDELBROT_Z_POWER_L,
	s_manzpower_name + 1,
		{s_real_z0, s_imag_z0, s_exponent, s_exponent_imag},
		{0, 0, 2, 0},
		HT_PICKMJ, HF_MANZPOWER, FRACTALFLAG_BAIL_OUT_TESTS,
		-2.0f, 2.0f, -1.5f, 1.5f,
		1, FRACTYPE_JULIA_Z_POWER_L, FRACTYPE_NO_FRACTAL,
		FRACTYPE_MANDELBROT_Z_POWER_FP, SYMMETRY_X_AXIS_NO_IMAGINARY,
		z_power_orbit, mandelbrot_per_pixel_l,
		mandelbrot_setup_l, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{
	FRACTYPE_JULIA_Z_POWER_L,
	s_julzpower_name + 1,
		{s_parameter_real, s_parameter_imag, s_exponent, s_exponent_imag},
		{0.3, 0.6, 2, 0},
		HT_PICKMJ, HF_JULZPOWER, FRACTALFLAG_JULIBROT | FRACTALFLAG_BAIL_OUT_TESTS,
		-2.0f, 2.0f, -1.5f, 1.5f,
		1, FRACTYPE_NO_FRACTAL, FRACTYPE_MANDELBROT_Z_POWER_L,
		FRACTYPE_JULIA_Z_POWER_FP, SYMMETRY_ORIGIN,
		z_power_orbit, julia_per_pixel_l,
		julia_setup_l, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{
	FRACTYPE_MANDELBROT_Z_POWER_FP,
	s_manzpower_name,
		{s_real_z0, s_imag_z0, s_exponent, s_exponent_imag},
		{0, 0, 2, 0},
		HT_PICKMJ, HF_MANZPOWER, FRACTALFLAG_BAIL_OUT_TESTS | FRACTALFLAG_ARBITRARY_PRECISION,
		-2.5f, 1.5f, -1.5f, 1.5f,
		0, FRACTYPE_JULIA_Z_POWER_FP, FRACTYPE_NO_FRACTAL,
		FRACTYPE_MANDELBROT_Z_POWER_L, SYMMETRY_X_AXIS_NO_IMAGINARY,
		z_power_orbit_fp, other_mandelbrot_per_pixel_fp,
		mandelbrot_setup_fp, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{
	FRACTYPE_JULIA_Z_POWER_FP,
	s_julzpower_name,
		{s_parameter_real, s_parameter_imag, s_exponent, s_exponent_imag},
		{0.3, 0.6, 2, 0},
		HT_PICKMJ, HF_JULZPOWER, FRACTALFLAG_JULIBROT | FRACTALFLAG_BAIL_OUT_TESTS | FRACTALFLAG_ARBITRARY_PRECISION,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_MANDELBROT_Z_POWER_FP,
		FRACTYPE_JULIA_Z_POWER_L, SYMMETRY_ORIGIN,
		z_power_orbit_fp, other_julia_per_pixel_fp,
		julia_setup_fp, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{
	FRACTYPE_MANDELBROT_Z_TO_Z_PLUS_Z_POWER_FP,
	"manzzpwr",
		{s_real_z0, s_imag_z0, s_exponent, ""},
		{0, 0, 2, 0},
		HT_PICKMJ, HF_MANZZPWR, FRACTALFLAG_BAIL_OUT_TESTS,
		-2.5f, 1.5f, -1.5f, 1.5f,
		0, FRACTYPE_JULIA_Z_TO_Z_PLUS_Z_POWER_FP, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_X_AXIS_NO_PARAMETER,
		z_to_z_plus_z_orbit_fp, other_mandelbrot_per_pixel_fp,
		mandelbrot_setup_fp, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{
	FRACTYPE_JULIA_Z_TO_Z_PLUS_Z_POWER_FP,
	"julzzpwr",
		{s_parameter_real, s_parameter_imag, s_exponent, ""},
		{-0.3, 0.3, 2, 0},
		HT_PICKMJ, HF_JULZZPWR, FRACTALFLAG_JULIBROT | FRACTALFLAG_BAIL_OUT_TESTS,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_MANDELBROT_Z_TO_Z_PLUS_Z_POWER_FP,
		FRACTYPE_NO_FRACTAL, SYMMETRY_NONE,
		z_to_z_plus_z_orbit_fp, other_julia_per_pixel_fp,
		julia_setup_fp, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{
	FRACTYPE_MANDELBROT_FUNC_PLUS_EXP_L,
	s_manfnplusexp_name + 1,
		{s_real_z0, s_imag_z0, "", ""},
		{0, 0, 0, 0},
		HT_PICKMJ, HF_MANDFNPLUSEXP, FRACTALFLAG_1_FUNCTION | FRACTALFLAG_BAIL_OUT_TESTS,
		-8.0f, 8.0f, -6.0f, 6.0f,
		16, FRACTYPE_JULIA_FUNC_PLUS_EXP_L, FRACTYPE_NO_FRACTAL,
		FRACTYPE_MANDELBROT_FUNC_PLUS_EXP_FP, SYMMETRY_X_AXIS_NO_PARAMETER,
		trig_plus_exponent_orbit, mandelbrot_per_pixel_l,
		mandelbrot_setup_l, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{
	FRACTYPE_JULIA_FUNC_PLUS_EXP_L,
	s_julfnplusexp_name + 1,
		{s_parameter_real, s_parameter_imag, "", ""},
		{0, 0, 0, 0},
		HT_PICKMJ, HF_JULFNPLUSEXP, FRACTALFLAG_1_FUNCTION | FRACTALFLAG_JULIBROT | FRACTALFLAG_BAIL_OUT_TESTS,
		-4.0f, 4.0f, -3.0f, 3.0f,
		16, FRACTYPE_NO_FRACTAL, FRACTYPE_MANDELBROT_FUNC_PLUS_EXP_L,
		FRACTYPE_JULIA_FUNC_PLUS_EXP_FP, SYMMETRY_NONE,
		trig_plus_exponent_orbit, julia_per_pixel_l,
		julia_setup_l, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{
	FRACTYPE_MANDELBROT_FUNC_PLUS_EXP_FP,
	s_manfnplusexp_name,
		{s_real_z0, s_imag_z0, "", ""},
		{0, 0, 0, 0},
		HT_PICKMJ, HF_MANDFNPLUSEXP, FRACTALFLAG_1_FUNCTION | FRACTALFLAG_BAIL_OUT_TESTS,
		-8.0f, 8.0f, -6.0f, 6.0f,
		0, FRACTYPE_JULIA_FUNC_PLUS_EXP_FP, FRACTYPE_NO_FRACTAL,
		FRACTYPE_MANDELBROT_FUNC_PLUS_EXP_L, SYMMETRY_X_AXIS_NO_PARAMETER,
		trig_plus_exponent_orbit_fp, other_mandelbrot_per_pixel_fp,
		mandelbrot_setup_fp, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{
	FRACTYPE_JULIA_FUNC_PLUS_EXP_FP,
	s_julfnplusexp_name,
		{s_parameter_real, s_parameter_imag, "", ""},
		{0, 0, 0, 0},
		HT_PICKMJ, HF_JULFNPLUSEXP, FRACTALFLAG_1_FUNCTION | FRACTALFLAG_JULIBROT | FRACTALFLAG_BAIL_OUT_TESTS,
		-4.0f, 4.0f, -3.0f, 3.0f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_MANDELBROT_FUNC_PLUS_EXP_FP,
		FRACTYPE_JULIA_FUNC_PLUS_EXP_L, SYMMETRY_NONE,
		trig_plus_exponent_orbit_fp, other_julia_per_pixel_fp,
		julia_setup_fp, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{
	FRACTYPE_POPCORN_FP,
	s_popcorn_name,
		{s_step_x, s_step_y, s_constant_x, s_constant_y},
		{0.05, 0, 3.00, 0},
		HT_POPCORN, HF_POPCORN, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_4_FUNCTIONS,
		-3.0f, 3.0f, -2.25f, 2.25f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_POPCORN_L, SYMMETRY_NO_PLOT,
		popcorn_fn_orbit_fp, other_julia_per_pixel_fp,
		julia_setup_fp, popcorn,
		ORBIT_BAILOUT_STANDARD
	},

	{
	FRACTYPE_POPCORN_L,
	s_popcorn_name + 1,
		{s_step_x, s_step_y, s_constant_x, s_constant_y},
		{0.05, 0, 3.00, 0},
		HT_POPCORN, HF_POPCORN, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_4_FUNCTIONS,
		-3.0f, 3.0f, -2.25f, 2.25f,
		16, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_POPCORN_FP, SYMMETRY_NO_PLOT,
		popcorn_fn_orbit, julia_per_pixel_l,
		julia_setup_l, popcorn,
		ORBIT_BAILOUT_STANDARD
	},

	{
	FRACTYPE_LORENZ_FP,
	s_lorenz_name,
		{s_time_step, "a", "b", "c"},
		{.02, 5, 15, 1},
		HT_LORENZ, HF_LORENZ, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_INFINITE_CALCULATION,
		-15.0f, 15.0f, 0.0f, 30.0f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_LORENZ_L, SYMMETRY_NONE,
		(VF) lorenz_3d_orbit_fp, NULL,
		orbit_3d_setup_fp, orbit_2d_fp,
		ORBIT_BAILOUT_NONE
	},

	{
	FRACTYPE_LORENZ_L,
	s_lorenz_name + 1,
		{s_time_step, "a", "b", "c"},
		{.02, 5, 15, 1},
		HT_LORENZ, HF_LORENZ, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_INFINITE_CALCULATION,
		-15.0f, 15.0f, 0.0f, 30.0f,
		16, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_LORENZ_FP, SYMMETRY_NONE,
		(VF) lorenz_3d_orbit, NULL,
		orbit_3d_setup, orbit_2d,
		ORBIT_BAILOUT_NONE
	},

	{
	FRACTYPE_LORENZ_3D_L,
	s_lorenz3d_name + 1,
		{s_time_step, "a", "b", "c"},
		{.02, 5, 15, 1},
		HT_LORENZ, HF_LORENZ, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_NOT_RESUMABLE | FRACTALFLAG_3D_PARAMETERS | FRACTALFLAG_INFINITE_CALCULATION,
		-30.0f, 30.0f, -30.0f, 30.0f,
		16, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_LORENZ_3D_FP, SYMMETRY_NONE,
		(VF) lorenz_3d_orbit, NULL,
		orbit_3d_setup, orbit_3d,
		ORBIT_BAILOUT_NONE
	},

	{
		// TODO: FRACTYPE_..._MP
	FRACTYPE_NEWTON_MP,
	s_newton_name + 1,
		{s_newton_degree, "", "", ""},
		{3, 0, 0, 0},
		HT_NEWT, HF_NEWT, 0,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NEWTON, SYMMETRY_X_AXIS,
		newton_orbit_mpc, julia_per_pixel_mpc,
		newton_setup, standard_fractal,
		ORBIT_BAILOUT_NONE
	},

	{
		// TODO: FRACTYPE_..._MP
	FRACTYPE_NEWTON_BASIN_MP,
	s_newtbasin_name + 1,
		{s_newton_degree, "Enter non-zero value for stripes", "", ""},
		{3, 0, 0, 0},
		HT_NEWTBAS, HF_NEWTBAS, 0,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NEWTON_BASIN, SYMMETRY_NONE,
		newton_orbit_mpc, julia_per_pixel_mpc,
		newton_setup, standard_fractal,
		ORBIT_BAILOUT_NONE
	},

	{
	FRACTYPE_NEWTON_COMPLEX,
	"complexnewton",
		{"Real part of Degree", "Imag part of Degree", "Real part of Root", "Imag part of Root"},
		{3, 0, 1, 0},
		HT_NEWTCMPLX, HF_COMPLEXNEWT, 0,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_NONE,
		complex_newton, other_julia_per_pixel_fp,
		complex_newton_setup, standard_fractal,
		ORBIT_BAILOUT_NONE
	},

	{
	FRACTYPE_NEWTON_BASIN_COMPLEX,
	"complexbasin",
		{"Real part of Degree", "Imag part of Degree", "Real part of Root", "Imag part of Root"},
		{3, 0, 1, 0},
		HT_NEWTCMPLX, HF_COMPLEXNEWT, 0,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_NONE,
		complex_basin, other_julia_per_pixel_fp,
		complex_newton_setup, standard_fractal,
		ORBIT_BAILOUT_NONE
	},

	{
	FRACTYPE_MARKS_MANDELBROT_COMPLEX,
	"cmplxmarksmand",
		{s_real_z0, s_imag_z0, s_exponent, s_exponent_imag},
		{0, 0, 1, 0},
		HT_MARKS, HF_CMPLXMARKSMAND, FRACTALFLAG_BAIL_OUT_TESTS,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, FRACTYPE_MARKS_JULIA_COMPLEX, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_NONE,
		marks_complex_mandelbrot_orbit, marks_complex_mandelbrot_per_pixel,
		mandelbrot_setup_fp, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{
	FRACTYPE_MARKS_JULIA_COMPLEX,
	"cmplxmarksjul",
		{s_parameter_real, s_parameter_imag, s_exponent, s_exponent_imag},
		{0.3, 0.6, 1, 0},
		HT_MARKS, HF_CMPLXMARKSJUL, FRACTALFLAG_BAIL_OUT_TESTS,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_MARKS_MANDELBROT_COMPLEX,
		FRACTYPE_NO_FRACTAL, SYMMETRY_NONE,
		marks_complex_mandelbrot_orbit, julia_per_pixel_fp,
		julia_setup_fp, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{
	FRACTYPE_FORMULA,
	s_formula_name + 1,
		{s_p1_real, s_p1_imag, s_p2_real, s_p2_imag},
		{0, 0, 0, 0},
		HT_FORMULA, -2, FRACTALFLAG_MORE_PARAMETERS,
		-2.0f, 2.0f, -1.5f, 1.5f,
		1, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_FORMULA_FP, SYMMETRY_SETUP,
		formula_orbit, form_per_pixel,
		formula_setup_int, standard_fractal,
		0
	},

	{
	FRACTYPE_FORMULA_FP,
	s_formula_name,
		{s_p1_real, s_p1_imag, s_p2_real, s_p2_imag},
		{0, 0, 0, 0},
		HT_FORMULA, -2, FRACTALFLAG_MORE_PARAMETERS,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_FORMULA, SYMMETRY_SETUP,
		formula_orbit, form_per_pixel,
		formula_setup_fp, standard_fractal,
		0
	},

	{
	FRACTYPE_SIERPINSKI_FP,
	s_sierpinski_name,
		{"", "", "", ""},
		{0, 0, 0, 0},
		HT_SIER, HF_SIER, 0,
		-4.0f/3.0f, 96.0f/45.0f, -0.9f, 1.7f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_SIERPINSKI, SYMMETRY_NONE,
		sierpinski_orbit_fp, other_julia_per_pixel_fp,
		sierpinski_setup_fp, standard_fractal,
		127
	},

	{
	FRACTYPE_LAMBDA_FP,
	s_lambda_name,
		{s_parameter_real, s_parameter_imag, "", ""},
		{0.85, 0.6, 0, 0},
		HT_LAMBDA, HF_LAMBDA, FRACTALFLAG_JULIBROT | FRACTALFLAG_BAIL_OUT_TESTS,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_MANDELBROT_LAMBDA_FP,
		FRACTYPE_LAMBDA, SYMMETRY_NONE,
		lambda_orbit_fp, julia_per_pixel_fp,
		julia_setup_fp, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{
	FRACTYPE_BARNSLEY_M1_FP,
	s_barnsleym1_name,
		{s_real_z0, s_imag_z0, "", ""},
		{0, 0, 0, 0},
		HT_BARNS, HF_BARNSM1, FRACTALFLAG_BAIL_OUT_TESTS,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, FRACTYPE_BARNSLEY_J1_FP, FRACTYPE_NO_FRACTAL,
		FRACTYPE_BARNSLEY_M1, SYMMETRY_XY_AXIS_NO_PARAMETER,
		barnsley1_orbit_fp, other_mandelbrot_per_pixel_fp,
		mandelbrot_setup_fp, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{
	FRACTYPE_BARNSLEY_J1_FP,
	s_barnsleyj1_name,
		{s_parameter_real, s_parameter_imag, "", ""},
		{0.6, 1.1, 0, 0},
		HT_BARNS, HF_BARNSJ1, FRACTALFLAG_JULIBROT | FRACTALFLAG_BAIL_OUT_TESTS,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_BARNSLEY_M1_FP,
		FRACTYPE_BARNSLEY_J1, SYMMETRY_ORIGIN,
		barnsley1_orbit_fp, other_julia_per_pixel_fp,
		julia_setup_fp, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{
	FRACTYPE_BARNSLEY_M2_FP,
	s_barnsleym2_name,
		{s_real_z0, s_imag_z0, "", ""},
		{0, 0, 0, 0},
		HT_BARNS, HF_BARNSM2, FRACTALFLAG_BAIL_OUT_TESTS,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, FRACTYPE_BARNSLEY_J2_FP, FRACTYPE_NO_FRACTAL,
		FRACTYPE_BARNSLEY_M2, SYMMETRY_Y_AXIS_NO_PARAMETER,
		barnsley2_orbit_fp, other_mandelbrot_per_pixel_fp,
		mandelbrot_setup_fp, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{
	FRACTYPE_BARNSLEY_J2_FP,
	s_barnsleyj2_name,
		{s_parameter_real, s_parameter_imag, "", ""},
		{0.6, 1.1, 0, 0},
		HT_BARNS, HF_BARNSJ2, FRACTALFLAG_JULIBROT | FRACTALFLAG_BAIL_OUT_TESTS,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_BARNSLEY_M2_FP,
		FRACTYPE_BARNSLEY_J2, SYMMETRY_ORIGIN,
		barnsley2_orbit_fp, other_julia_per_pixel_fp,
		julia_setup_fp, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{
	FRACTYPE_BARNSLEY_M3_FP,
	s_barnsleym3_name,
		{s_real_z0, s_imag_z0, "", ""},
		{0, 0, 0, 0},
		HT_BARNS, HF_BARNSM3, FRACTALFLAG_BAIL_OUT_TESTS,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, FRACTYPE_BARNSLEY_J3_FP, FRACTYPE_NO_FRACTAL,
		FRACTYPE_BARNSLEY_M3, SYMMETRY_X_AXIS_NO_PARAMETER,
		barnsley3_orbit_fp, other_mandelbrot_per_pixel_fp,
		mandelbrot_setup_fp, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{
	FRACTYPE_BARNSLEY_J3_FP,
	s_barnsleyj3_name,
		{s_parameter_real, s_parameter_imag, "", ""},
		{0.6, 1.1, 0, 0},
		HT_BARNS, HF_BARNSJ3, FRACTALFLAG_JULIBROT | FRACTALFLAG_BAIL_OUT_TESTS,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_BARNSLEY_M3_FP,
		FRACTYPE_BARNSLEY_J3, SYMMETRY_NONE,
		barnsley3_orbit_fp, other_julia_per_pixel_fp,
		julia_setup_fp, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{
	FRACTYPE_MANDELBROT_LAMBDA_FP,
	s_mandellambda_name,
		{s_real_z0, s_imag_z0, "", ""},
		{0, 0, 0, 0},
		HT_MLAMBDA, HF_MLAMBDA, FRACTALFLAG_BAIL_OUT_TESTS,
		-3.0f, 5.0f, -3.0f, 3.0f,
		0, FRACTYPE_LAMBDA_FP, FRACTYPE_NO_FRACTAL,
		FRACTYPE_MANDELBROT_LAMBDA, SYMMETRY_X_AXIS_NO_PARAMETER,
		lambda_orbit_fp, mandelbrot_per_pixel_fp,
		mandelbrot_setup_fp, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{
	FRACTYPE_JULIBROT,
	s_julibrot_name + 1,
		{"", "", "", ""},
		{0, 0, 0, 0},
		HT_JULIBROT, -1, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_NO_ZOOM_BOX_ROTATE | FRACTALFLAG_NOT_RESUMABLE,
		-2.0f, 2.0f, -1.5f, 1.5f,
		1, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_JULIBROT_FP, SYMMETRY_NONE,
		julia_orbit, julibrot_per_pixel,
		julibrot_setup, standard_4d_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{
	FRACTYPE_LORENZ_3D_FP,
	s_lorenz3d_name,
		{s_time_step, "a", "b", "c"},
		{.02, 5, 15, 1},
		HT_LORENZ, HF_LORENZ, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_NOT_RESUMABLE | FRACTALFLAG_3D_PARAMETERS | FRACTALFLAG_INFINITE_CALCULATION,
		-30.0f, 30.0f, -30.0f, 30.0f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_LORENZ_3D_L, SYMMETRY_NONE,
		(VF) lorenz_3d_orbit_fp, NULL,
		orbit_3d_setup_fp, orbit_3d_fp,
		ORBIT_BAILOUT_NONE
	},

	{
	FRACTYPE_ROSSLER_L,
	s_rossler3d_name + 1,
		{s_time_step, "a", "b", "c"},
		{.04, .2, .2, 5.7},
		HT_ROSS, HF_ROSS, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_NOT_RESUMABLE | FRACTALFLAG_3D_PARAMETERS | FRACTALFLAG_INFINITE_CALCULATION,
		-30.0f, 30.0f, -20.0f, 40.0f,
		16, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_ROSSLER_FP, SYMMETRY_NONE,
		(VF) rossler_orbit, NULL,
		orbit_3d_setup, orbit_3d,
		ORBIT_BAILOUT_NONE
	},

	{
	FRACTYPE_ROSSLER_FP,
	s_rossler3d_name,
		{s_time_step, "a", "b", "c"},
		{.04, .2, .2, 5.7},
		HT_ROSS, HF_ROSS, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_NOT_RESUMABLE | FRACTALFLAG_3D_PARAMETERS | FRACTALFLAG_INFINITE_CALCULATION,
		-30.0f, 30.0f, -20.0f, 40.0f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_ROSSLER_L, SYMMETRY_NONE,
		(VF) rossler_orbit_fp, NULL,
		orbit_3d_setup_fp, orbit_3d_fp,
		ORBIT_BAILOUT_NONE
	},

	{
	FRACTYPE_HENON_L,
	s_henon_name + 1,
		{"a", "b", "", ""},
		{1.4, .3, 0, 0},
		HT_HENON, HF_HENON, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_INFINITE_CALCULATION,
		-1.4f, 1.4f, -.5f, .5f,
		16, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_HENON_FP, SYMMETRY_NONE,
		(VF) henon_orbit, NULL,
		orbit_3d_setup, orbit_2d,
		ORBIT_BAILOUT_NONE
	},

	{
	FRACTYPE_HENON_FP,
	s_henon_name,
		{"a", "b", "", ""},
		{1.4, .3, 0, 0},
		HT_HENON, HF_HENON, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_INFINITE_CALCULATION,
		-1.4f, 1.4f, -.5f, .5f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_HENON_L, SYMMETRY_NONE,
		(VF) henon_orbit_fp, NULL,
		orbit_3d_setup_fp, orbit_2d_fp,
		ORBIT_BAILOUT_NONE
	},

	{
	FRACTYPE_PICKOVER_FP,
	"pickover",
		{"a", "b", "c", "d"},
		{2.24, .43, -.65, -2.43},
		HT_PICK, HF_PICKOVER, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_NOT_RESUMABLE | FRACTALFLAG_3D_PARAMETERS,
		-8.0f/3.0f, 8.0f/3.0f, -2.0f, 2.0f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_NONE,
		(VF) pickover_orbit_fp, NULL,
		orbit_3d_setup_fp, orbit_3d_fp,
		ORBIT_BAILOUT_NONE
	},

	{
	FRACTYPE_GINGERBREAD_FP,
	"gingerbreadman",
		{"Initial x", "Initial y", "", ""},
		{-.1, 0, 0, 0},
		HT_GINGER, HF_GINGER, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_INFINITE_CALCULATION,
		-4.5f, 8.5f, -4.5f, 8.5f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_NONE,
		(VF) gingerbread_orbit_fp, NULL,
		orbit_3d_setup_fp, orbit_2d_fp,
		ORBIT_BAILOUT_NONE
	},

	{
	FRACTYPE_DIFFUSION,
	"diffusion",
		{"+Border size",
		"+Type (0=Central, 1=Falling, 2=Square Cavity)",
		"+Color change rate (0=Random)",
		""
		},
		{10, 0, 0, 0},
		HT_DIFFUS, HF_DIFFUS, FRACTALFLAG_NO_ZOOM | FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_NONE,
		NULL, NULL,
		stand_alone_setup, diffusion,
		ORBIT_BAILOUT_NONE
	},

	{
	FRACTYPE_UNITY_FP,
	s_unity_name,
		{"", "", "", ""},
		{0, 0, 0, 0},
		HT_UNITY, HF_UNITY, 0,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_UNITY, SYMMETRY_XY_AXIS,
		unity_orbit_fp, other_julia_per_pixel_fp,
		standard_setup, standard_fractal,
		ORBIT_BAILOUT_NONE
	},

	{
	FRACTYPE_SPIDER_FP,
	s_spider_name,
		{s_real_z0, s_imag_z0, "", ""},
		{0, 0, 0, 0},
		HT_SCOTSKIN, HF_SPIDER, FRACTALFLAG_BAIL_OUT_TESTS,
		-2.5f, 1.5f, -1.5f, 1.5f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_SPIDER, SYMMETRY_X_AXIS_NO_PARAMETER,
		spider_orbit_fp, mandelbrot_per_pixel_fp,
		mandelbrot_setup_fp, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{
	FRACTYPE_SPIDER,
	s_spider_name + 1,
		{s_real_z0, s_imag_z0, "", ""},
		{0, 0, 0, 0},
		HT_SCOTSKIN, HF_SPIDER, FRACTALFLAG_BAIL_OUT_TESTS,
		-2.5f, 1.5f, -1.5f, 1.5f,
		1, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_SPIDER_FP, SYMMETRY_X_AXIS_NO_PARAMETER,
		spider_orbit, mandelbrot_per_pixel,
		mandelbrot_setup_l, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{
	FRACTYPE_TETRATE_FP,
	"tetrate",
		{s_real_z0, s_imag_z0, "", ""},
		{0, 0, 0, 0},
		HT_SCOTSKIN, HF_TETRATE, FRACTALFLAG_BAIL_OUT_TESTS,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_X_AXIS_NO_IMAGINARY,
		tetrate_orbit_fp, other_mandelbrot_per_pixel_fp,
		mandelbrot_setup_fp, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{
	FRACTYPE_MAGNET_1M,
	"magnet1m",
		{s_real_z0, s_imag_z0, "", ""},
		{0, 0, 0, 0},
		HT_MAGNET, HF_MAGM1, 0,
		-4.0f, 4.0f, -3.0f, 3.0f,
		0, FRACTYPE_MAGNET_1J, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_X_AXIS_NO_PARAMETER,
		magnet1_orbit_fp, mandelbrot_per_pixel_fp,
		mandelbrot_setup_fp, standard_fractal,
		100
	},

	{
	FRACTYPE_MAGNET_1J,
	"magnet1j",
		{s_parameter_real, s_parameter_imag, "", ""},
		{0, 0, 0, 0},
		HT_MAGNET, HF_MAGJ1, 0,
		-8.0f, 8.0f, -6.0f, 6.0f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_MAGNET_1M,
		FRACTYPE_NO_FRACTAL, SYMMETRY_X_AXIS_NO_IMAGINARY,
		magnet1_orbit_fp, julia_per_pixel_fp,
		julia_setup_fp, standard_fractal,
		100
	},

	{
	FRACTYPE_MAGNET_2M,
	"magnet2m",
		{s_real_z0, s_imag_z0, "", ""},
		{0, 0, 0, 0},
		HT_MAGNET, HF_MAGM2, 0,
		-1.5f, 3.7f, -1.95f, 1.95f,
		0, FRACTYPE_MAGNET_2J, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_X_AXIS_NO_PARAMETER,
		magnet2_orbit_fp, mandelbrot_per_pixel_fp,
		mandelbrot_setup_fp, standard_fractal,
		100
	},

	{
	FRACTYPE_MAGNET_2J,
	"magnet2j",
		{s_parameter_real, s_parameter_imag, "", ""},
		{0, 0, 0, 0},
		HT_MAGNET, HF_MAGJ2, 0,
		-8.0f, 8.0f, -6.0f, 6.0f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_MAGNET_2M,
		FRACTYPE_NO_FRACTAL, SYMMETRY_X_AXIS_NO_IMAGINARY,
		magnet2_orbit_fp, julia_per_pixel_fp,
		julia_setup_fp, standard_fractal,
		100
	},

	{
	FRACTYPE_BIFURCATION_L,
	s_bifurcation_name + 1,
		{s_filter_cycles, s_seed_population, "", ""},
		{1000.0, 0.66, 0, 0},
		HT_BIF, HF_BIFURCATION, FRACTALFLAG_1_FUNCTION | FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_NO_ZOOM_BOX_ROTATE,
		1.9f, 3.0f, 0.0f, 1.34f,
		1, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_BIFURCATION, SYMMETRY_NONE,
		bifurcation_verhulst_trig, NULL,
		stand_alone_setup, bifurcation,
		ORBIT_BAILOUT_NONE
	},

	{
	FRACTYPE_BIFURCATION_LAMBDA_L,
	s_biflambda_name + 1,
		{s_filter_cycles, s_seed_population, "", ""},
		{1000.0, 0.66, 0, 0},
		HT_BIF, HF_BIFLAMBDA, FRACTALFLAG_1_FUNCTION | FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_NO_ZOOM_BOX_ROTATE,
		-2.0f, 4.0f, -1.0f, 2.0f,
		1, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_BIFURCATION_LAMBDA, SYMMETRY_NONE,
		bifurcation_lambda_trig, NULL,
		stand_alone_setup, bifurcation,
		ORBIT_BAILOUT_NONE
	},

	{
	FRACTYPE_BIFURCATION_LAMBDA,
	s_biflambda_name,
		{s_filter_cycles, s_seed_population, "", ""},
		{1000.0, 0.66, 0, 0},
		HT_BIF, HF_BIFLAMBDA, FRACTALFLAG_1_FUNCTION | FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_NO_ZOOM_BOX_ROTATE,
		-2.0f, 4.0f, -1.0f, 2.0f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_BIFURCATION_LAMBDA_L, SYMMETRY_NONE,
		bifurcation_lambda_trig_fp, NULL,
		stand_alone_setup, bifurcation,
		ORBIT_BAILOUT_NONE
	},

	{
	FRACTYPE_BIFURCATION_PLUS_FUNC_PI,
	s_bifplussinpi_name,
		{s_filter_cycles, s_seed_population, "", ""},
		{1000.0, 0.66, 0, 0},
		HT_BIF, HF_BIFPLUSSINPI, FRACTALFLAG_1_FUNCTION | FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_NO_ZOOM_BOX_ROTATE,
		0.275f, 1.45f, 0.0f, 2.0f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_BIFURCATION_PLUS_FUNC_PI_L, SYMMETRY_NONE,
		bifurcation_add_trig_pi_fp, NULL,
		stand_alone_setup, bifurcation,
		ORBIT_BAILOUT_NONE
	},

	{
	FRACTYPE_BIFURCATION_EQUAL_FUNC_PI,
	s_bifeqsinpi_name,
		{s_filter_cycles, s_seed_population, "", ""},
		{1000.0, 0.66, 0, 0},
		HT_BIF, HF_BIFEQSINPI, FRACTALFLAG_1_FUNCTION | FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_NO_ZOOM_BOX_ROTATE,
		-2.5f, 2.5f, -3.5f, 3.5f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_BIFURCATION_EQUAL_FUNC_PI_L, SYMMETRY_NONE,
		bifurcation_set_trig_pi_fp, NULL,
		stand_alone_setup, bifurcation,
		ORBIT_BAILOUT_NONE
	},

	{
	FRACTYPE_POPCORN_JULIA_FP,
	s_popcornjul_name,
		{s_step_x, s_step_y, s_constant_x, s_constant_y},
		{0.05, 0, 3.00, 0},
		HT_POPCORN, HF_POPCJUL, FRACTALFLAG_4_FUNCTIONS,
		-3.0f, 3.0f, -2.25f, 2.25f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_POPCORN_JULIA_L, SYMMETRY_NONE,
		popcorn_fn_orbit_fp, other_julia_per_pixel_fp,
		julia_setup_fp, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{
	FRACTYPE_POPCORN_JULIA_L,
	s_popcornjul_name + 1,
		{s_step_x, s_step_y, s_constant_x, s_constant_y},
		{0.05, 0, 3.0, 0},
		HT_POPCORN, HF_POPCJUL, FRACTALFLAG_4_FUNCTIONS,
		-3.0f, 3.0f, -2.25f, 2.25f,
		16, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_POPCORN_JULIA_FP, SYMMETRY_NONE,
		popcorn_fn_orbit, julia_per_pixel_l,
		julia_setup_l, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{
	FRACTYPE_L_SYSTEM,
	"lsystem",
		{" + Order", "", "", ""},
		{2, 0, 0, 0},
		HT_LSYS, -3, FRACTALFLAG_NO_ZOOM | FRACTALFLAG_NOT_RESUMABLE | FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING,
		-1.0f, 1.0f, -1.0f, 1.0f,
		1, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_NONE,
		NULL, NULL,
		stand_alone_setup, l_system,
		ORBIT_BAILOUT_NONE
	},

	{
	FRACTYPE_MAN_O_WAR_JULIA_FP,
	s_manowarj_name,
		{s_parameter_real, s_parameter_imag, "", ""},
		{0, 0, 0, 0},
		HT_SCOTSKIN, HF_MANOWARJ, FRACTALFLAG_JULIBROT | FRACTALFLAG_BAIL_OUT_TESTS,
		-2.5f, 1.5f, -1.5f, 1.5f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_MAN_O_WAR_FP,
		FRACTYPE_MAN_O_WAR_JULIA, SYMMETRY_NONE,
		man_o_war_orbit_fp, julia_per_pixel_fp,
		julia_setup_fp, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{
	FRACTYPE_MAN_O_WAR_JULIA,
	s_manowarj_name + 1,
		{s_parameter_real, s_parameter_imag, "", ""},
		{0, 0, 0, 0},
		HT_SCOTSKIN, HF_MANOWARJ, FRACTALFLAG_JULIBROT | FRACTALFLAG_BAIL_OUT_TESTS,
		-2.5f, 1.5f, -1.5f, 1.5f,
		1, FRACTYPE_NO_FRACTAL, FRACTYPE_MAN_O_WAR,
		FRACTYPE_MAN_O_WAR_JULIA_FP, SYMMETRY_NONE,
		man_o_war_orbit, julia_per_pixel,
		julia_setup_l, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{
	FRACTYPE_FUNC_PLUS_FUNC_PIXEL_FP,
	s_fn_z_plusfn_pix__name,
		{s_real_z0, s_imag_z0, s_trig2_coefficient_re, s_trig2_coefficient_im},
		{0, 0, 1, 0},
		HT_SCOTSKIN, HF_FNPLUSFNPIX, FRACTALFLAG_2_FUNCTIONS | FRACTALFLAG_BAIL_OUT_TESTS,
		-3.6f, 3.6f, -2.7f, 2.7f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_FUNC_PLUS_FUNC_PIXEL_L, SYMMETRY_NONE,
		richard8_orbit_fp, other_richard8_per_pixel_fp,
		mandelbrot_setup_fp, standard_fractal,
		ORBIT_BAILOUT_TRIG_L
	},

	{
	FRACTYPE_FUNC_PLUS_FUNC_PIXEL_L,
	s_fn_z_plusfn_pix__name + 1,
		{s_real_z0, s_imag_z0, s_trig2_coefficient_re, s_trig2_coefficient_im},
		{0, 0, 1, 0},
		HT_SCOTSKIN, HF_FNPLUSFNPIX, FRACTALFLAG_2_FUNCTIONS | FRACTALFLAG_BAIL_OUT_TESTS,
		-3.6f, 3.6f, -2.7f, 2.7f,
		1, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_FUNC_PLUS_FUNC_PIXEL_FP, SYMMETRY_NONE,
		richard8_orbit, richard8_per_pixel,
		mandelbrot_setup_l, standard_fractal,
		ORBIT_BAILOUT_TRIG_L
	},

	{
	FRACTYPE_MARKS_MANDELBROT_POWER_FP,
	s_marksmandelpwr_name,
		{s_real_z0, s_imag_z0, "", ""},
		{0, 0, 0, 0},
		HT_MARKS, HF_MARKSMANDPWR, FRACTALFLAG_1_FUNCTION | FRACTALFLAG_BAIL_OUT_TESTS,
		-2.5f, 1.5f, -1.5f, 1.5f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_MARKS_MANDELBROT_POWER, SYMMETRY_X_AXIS_NO_PARAMETER,
		marks_mandel_power_orbit_fp, marks_mandelbrot_power_per_pixel_fp,
		mandelbrot_setup_fp, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{
	FRACTYPE_MARKS_MANDELBROT_POWER,
	s_marksmandelpwr_name + 1,
		{s_real_z0, s_imag_z0, "", ""},
		{0, 0, 0, 0},
		HT_MARKS, HF_MARKSMANDPWR, FRACTALFLAG_1_FUNCTION | FRACTALFLAG_BAIL_OUT_TESTS,
		-2.5f, 1.5f, -1.5f, 1.5f,
		1, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_MARKS_MANDELBROT_POWER_FP, SYMMETRY_X_AXIS_NO_PARAMETER,
		marks_mandel_power_orbit, marks_mandelbrot_power_per_pixel,
		mandelbrot_setup_l, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{
	FRACTYPE_TIMS_ERROR_FP,
	s_tims_error_name,
		{s_real_z0, s_imag_z0, "", ""},
		{0, 0, 0, 0},
		HT_MARKS, HF_TIMSERR, FRACTALFLAG_1_FUNCTION | FRACTALFLAG_BAIL_OUT_TESTS,
		-2.9f, 4.3f, -2.7f, 2.7f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_TIMS_ERROR, SYMMETRY_X_AXIS_NO_PARAMETER,
		tims_error_orbit_fp, marks_mandelbrot_power_per_pixel_fp,
		mandelbrot_setup_fp, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{
	FRACTYPE_TIMS_ERROR,
	s_tims_error_name + 1,
		{s_real_z0, s_imag_z0, "", ""},
		{0, 0, 0, 0},
		HT_MARKS, HF_TIMSERR, FRACTALFLAG_1_FUNCTION | FRACTALFLAG_BAIL_OUT_TESTS,
		-2.9f, 4.3f, -2.7f, 2.7f,
		1, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_TIMS_ERROR_FP, SYMMETRY_X_AXIS_NO_PARAMETER,
		tims_error_orbit, marks_mandelbrot_power_per_pixel,
		mandelbrot_setup_l, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{
	FRACTYPE_BIFURCATION_EQUAL_FUNC_PI_L,
	s_bifeqsinpi_name + 1,
		{s_filter_cycles, s_seed_population, "", ""},
		{1000.0, 0.66, 0, 0},
		HT_BIF, HF_BIFEQSINPI, FRACTALFLAG_1_FUNCTION | FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_NO_ZOOM_BOX_ROTATE,
		-2.5f, 2.5f, -3.5f, 3.5f,
		1, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_BIFURCATION_EQUAL_FUNC_PI, SYMMETRY_NONE,
		bifurcation_set_trig_pi, NULL,
		stand_alone_setup, bifurcation,
		ORBIT_BAILOUT_NONE
	},

	{
	FRACTYPE_BIFURCATION_PLUS_FUNC_PI_L,
	s_bifplussinpi_name + 1,
		{s_filter_cycles, s_seed_population, "", ""},
		{1000.0, 0.66, 0, 0},
		HT_BIF, HF_BIFPLUSSINPI, FRACTALFLAG_1_FUNCTION | FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_NO_ZOOM_BOX_ROTATE,
		0.275f, 1.45f, 0.0f, 2.0f,
		1, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_BIFURCATION_PLUS_FUNC_PI, SYMMETRY_NONE,
		bifurcation_add_trig_pi, NULL,
		stand_alone_setup, bifurcation,
		ORBIT_BAILOUT_NONE
	},

	{
	FRACTYPE_BIFURCATION_STEWART,
	s_bifstewart_name,
		{s_filter_cycles, s_seed_population, "", ""},
		{1000.0, 0.66, 0, 0},
		HT_BIF, HF_BIFSTEWART, FRACTALFLAG_1_FUNCTION | FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_NO_ZOOM_BOX_ROTATE,
		0.7f, 2.0f, -1.1f, 1.1f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_BIFURCATION_STEWART_L, SYMMETRY_NONE,
		bifurcation_stewart_trig_fp, NULL,
		stand_alone_setup, bifurcation,
		ORBIT_BAILOUT_NONE
	},

	{
	FRACTYPE_BIFURCATION_STEWART_L,
	s_bifstewart_name + 1,
		{s_filter_cycles, s_seed_population, "", ""},
		{1000.0, 0.66, 0, 0},
		HT_BIF, HF_BIFSTEWART, FRACTALFLAG_1_FUNCTION | FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_NO_ZOOM_BOX_ROTATE,
		0.7f, 2.0f, -1.1f, 1.1f,
		1, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_BIFURCATION_STEWART, SYMMETRY_NONE,
		bifurcation_stewart_trig, NULL,
		stand_alone_setup, bifurcation,
		ORBIT_BAILOUT_NONE
	},

	{
	FRACTYPE_HOPALONG_FP,
	"hopalong",
		{"a", "b", "c", ""},
		{.4, 1, 0, 0},
		HT_MARTIN, HF_HOPALONG, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_INFINITE_CALCULATION,
		-2.0f, 3.0f, -1.625f, 2.625f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_NONE,
		(VF) hopalong_2d_orbit_fp, NULL,
		orbit_3d_setup_fp, orbit_2d_fp,
		ORBIT_BAILOUT_NONE
	},

	{
	FRACTYPE_CIRCLE_FP,
	"circle",
		{"magnification", "", "", ""},
		{200000L, 0, 0, 0},
		HT_CIRCLE, HF_CIRCLE, 0,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_XY_AXIS,
		circle_orbit_fp, julia_per_pixel_fp,
		julia_setup_fp, standard_fractal,
		ORBIT_BAILOUT_NONE
	},

	{
	FRACTYPE_MARTIN_FP,
	"martin",
		{"a", "", "", ""},
		{3.14, 0, 0, 0},
		HT_MARTIN, HF_MARTIN, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_INFINITE_CALCULATION,
		-32.0f, 32.0f, -24.0f, 24.0f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_NONE,
		(VF) martin_2d_orbit_fp, NULL,
		orbit_3d_setup_fp, orbit_2d_fp,
		ORBIT_BAILOUT_NONE
	},

	{
	FRACTYPE_LYAPUNOV,
	"lyapunov",
		{"+Order (integer)", "Population Seed", "+Filter Cycles", ""},
		{0, 0.5, 0, 0},
		HT_LYAPUNOV, HT_LYAPUNOV, 0,
		-8.0f, 8.0f, -6.0f, 6.0f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_NONE,
		bifurcation_lambda, NULL,
		lyapunov_setup, lyapunov,
		ORBIT_BAILOUT_NONE
	},

	{
	FRACTYPE_LORENZ_3D_1_FP,
	"lorenz3d1",
		{s_time_step, "a", "b", "c"},
		{.02, 5, 15, 1},
		HT_LORENZ, HF_LORENZ3D1, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_NOT_RESUMABLE | FRACTALFLAG_3D_PARAMETERS | FRACTALFLAG_INFINITE_CALCULATION,
		-30.0f, 30.0f, -30.0f, 30.0f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_NONE,
		(VF) lorenz_3d1_orbit_fp, NULL,
		orbit_3d_setup_fp, orbit_3d_fp,
		ORBIT_BAILOUT_NONE
	},

	{
	FRACTYPE_LORENZ_3D_3_FP,
	"lorenz3d3",
		{s_time_step, "a", "b", "c"},
		{.02, 10, 28, 2.66},
		HT_LORENZ, HF_LORENZ3D3, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_NOT_RESUMABLE | FRACTALFLAG_3D_PARAMETERS | FRACTALFLAG_INFINITE_CALCULATION,
		-30.0f, 30.0f, -30.0f, 30.0f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_NONE,
		(VF) lorenz_3d3_orbit_fp, NULL,
		orbit_3d_setup_fp, orbit_3d_fp,
		ORBIT_BAILOUT_NONE
	},

	{
	FRACTYPE_LORENZ_3D_4_FP,
	"lorenz3d4",
		{s_time_step, "a", "b", "c"},
		{.02, 10, 28, 2.66},
		HT_LORENZ, HF_LORENZ3D4, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_NOT_RESUMABLE | FRACTALFLAG_3D_PARAMETERS | FRACTALFLAG_INFINITE_CALCULATION,
		-30.0f, 30.0f, -30.0f, 30.0f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_NONE,
		(VF) lorenz_3d4_orbit_fp, NULL,
		orbit_3d_setup_fp, orbit_3d_fp,
		ORBIT_BAILOUT_NONE
	},

	{
	FRACTYPE_LAMBDA_FUNC_OR_FUNC_L,
	s_lambda_fnorfn__name + 1,
		{s_parameter_real, s_parameter_imag, "Function Shift Value", ""},
		{1, 0.1, 1, 0},
		HT_FNORFN, HF_LAMBDAFNFN, FRACTALFLAG_2_FUNCTIONS | FRACTALFLAG_BAIL_OUT_TESTS,
		-4.0f, 4.0f, -3.0f, 3.0f,
		16, FRACTYPE_NO_FRACTAL, FRACTYPE_MANDELBROT_LAMBDA_FUNC_OR_FUNC_L,
		FRACTYPE_LAMBDA_FUNC_OR_FUNC_FP, SYMMETRY_ORIGIN,
		lambda_trig_or_trig_orbit, julia_per_pixel_l,
		lambda_trig_or_trig_setup, standard_fractal,
		ORBIT_BAILOUT_TRIG_L
	},

	{
	FRACTYPE_LAMBDA_FUNC_OR_FUNC_FP,
	s_lambda_fnorfn__name,
		{s_parameter_real, s_parameter_imag, "Function Shift Value", ""},
		{1, 0.1, 1, 0},
		HT_FNORFN, HF_LAMBDAFNFN, FRACTALFLAG_2_FUNCTIONS | FRACTALFLAG_BAIL_OUT_TESTS,
		-4.0f, 4.0f, -3.0f, 3.0f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_MANDELBROT_LAMBDA_FUNC_OR_FUNC_FP,
		FRACTYPE_LAMBDA_FUNC_OR_FUNC_L, SYMMETRY_ORIGIN,
		lambda_trig_or_trig_orbit_fp, other_julia_per_pixel_fp,
		lambda_trig_or_trig_setup, standard_fractal,
		ORBIT_BAILOUT_TRIG_L
	},

	{
	FRACTYPE_JULIA_FUNC_OR_FUNC_L,
	s_julia_fnorfn__name + 1,
		{s_parameter_real, s_parameter_imag, "Function Shift Value", ""},
		{0, 0, 8, 0},
		HT_FNORFN, HF_JULIAFNFN, FRACTALFLAG_2_FUNCTIONS | FRACTALFLAG_BAIL_OUT_TESTS,
		-4.0f, 4.0f, -3.0f, 3.0f,
		16, FRACTYPE_NO_FRACTAL, FRACTYPE_MANDELBROT_FUNC_OR_FUNC_L,
		FRACTYPE_JULIA_FUNC_OR_FUNC_FP, SYMMETRY_X_AXIS,
		julia_trig_or_trig_orbit, julia_per_pixel_l,
		julia_trig_or_trig_setup, standard_fractal,
		ORBIT_BAILOUT_TRIG_L
	},

	{
	FRACTYPE_JULIA_FUNC_OR_FUNC_FP,
	s_julia_fnorfn__name,
		{s_parameter_real, s_parameter_imag, "Function Shift Value", ""},
		{0, 0, 8, 0},
		HT_FNORFN, HF_JULIAFNFN, FRACTALFLAG_2_FUNCTIONS | FRACTALFLAG_BAIL_OUT_TESTS,
		-4.0f, 4.0f, -3.0f, 3.0f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_MANDELBROT_FUNC_OR_FUNC_FP,
		FRACTYPE_JULIA_FUNC_OR_FUNC_L, SYMMETRY_X_AXIS,
		julia_trig_or_trig_orbit_fp, other_julia_per_pixel_fp,
		julia_trig_or_trig_setup, standard_fractal,
		ORBIT_BAILOUT_TRIG_L
	},

	{
	FRACTYPE_MANDELBROT_LAMBDA_FUNC_OR_FUNC_L,
	s_manlam_fnorfn__name + 1,
		{s_real_z0, s_imag_z0, "Function Shift Value", ""},
		{0, 0, 10, 0},
		HT_FNORFN, HF_MANLAMFNFN, FRACTALFLAG_2_FUNCTIONS | FRACTALFLAG_BAIL_OUT_TESTS,
		-4.0f, 4.0f, -3.0f, 3.0f,
		16, FRACTYPE_LAMBDA_FUNC_OR_FUNC_L, FRACTYPE_NO_FRACTAL,
		FRACTYPE_MANDELBROT_LAMBDA_FUNC_OR_FUNC_FP, SYMMETRY_X_AXIS_NO_PARAMETER,
		lambda_trig_or_trig_orbit, mandelbrot_per_pixel_l,
		mandelbrot_lambda_trig_or_trig_setup, standard_fractal,
		ORBIT_BAILOUT_TRIG_L
	},

	{
	FRACTYPE_MANDELBROT_LAMBDA_FUNC_OR_FUNC_FP,
	s_manlam_fnorfn__name,
		{s_real_z0, s_imag_z0, "Function Shift Value", ""},
		{0, 0, 10, 0},
		HT_FNORFN, HF_MANLAMFNFN, FRACTALFLAG_2_FUNCTIONS | FRACTALFLAG_BAIL_OUT_TESTS,
		-4.0f, 4.0f, -3.0f, 3.0f,
		0, FRACTYPE_LAMBDA_FUNC_OR_FUNC_FP, FRACTYPE_NO_FRACTAL,
		FRACTYPE_MANDELBROT_LAMBDA_FUNC_OR_FUNC_L, SYMMETRY_X_AXIS_NO_PARAMETER,
		lambda_trig_or_trig_orbit_fp, other_mandelbrot_per_pixel_fp,
		mandelbrot_lambda_trig_or_trig_setup, standard_fractal,
		ORBIT_BAILOUT_TRIG_L
	},

	{
	FRACTYPE_MANDELBROT_FUNC_OR_FUNC_L,
	s_mandel_fnorfn__name + 1,
		{s_real_z0, s_imag_z0, "Function Shift Value", ""},
		{0, 0, 0.5, 0},
		HT_FNORFN, HF_MANDELFNFN, FRACTALFLAG_2_FUNCTIONS | FRACTALFLAG_BAIL_OUT_TESTS,
		-4.0f, 4.0f, -3.0f, 3.0f,
		16, FRACTYPE_JULIA_FUNC_OR_FUNC_L, FRACTYPE_NO_FRACTAL,
		FRACTYPE_MANDELBROT_FUNC_OR_FUNC_FP, SYMMETRY_X_AXIS_NO_PARAMETER,
		julia_trig_or_trig_orbit, mandelbrot_per_pixel_l,
		mandelbrot_trig_or_trig_setup, standard_fractal,
		ORBIT_BAILOUT_TRIG_L
	},

	{
	FRACTYPE_MANDELBROT_FUNC_OR_FUNC_FP,
	s_mandel_fnorfn__name,
		{s_real_z0, s_imag_z0, "Function Shift Value", ""},
		{0, 0, 0.5, 0},
		HT_FNORFN, HF_MANDELFNFN, FRACTALFLAG_2_FUNCTIONS | FRACTALFLAG_BAIL_OUT_TESTS,
		-4.0f, 4.0f, -3.0f, 3.0f,
		0, FRACTYPE_JULIA_FUNC_OR_FUNC_FP, FRACTYPE_NO_FRACTAL,
		FRACTYPE_MANDELBROT_FUNC_OR_FUNC_L, SYMMETRY_X_AXIS_NO_PARAMETER,
		julia_trig_or_trig_orbit_fp, other_mandelbrot_per_pixel_fp,
		mandelbrot_trig_or_trig_setup, standard_fractal,
		ORBIT_BAILOUT_TRIG_L
	},

	{
	FRACTYPE_BIFURCATION_MAY_L,
	s_bifmay_name + 1,
		{s_filter_cycles, s_seed_population, "Beta >= 2", ""},
		{300.0, 0.9, 5, 0},
		HT_BIF, HF_BIFMAY, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_NO_ZOOM_BOX_ROTATE,
		-3.5f, -0.9f, -0.5f, 3.2f,
		16, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_BIFURCATION_MAY, SYMMETRY_NONE,
		bifurcation_may, NULL,
		bifurcation_may_setup, bifurcation,
		ORBIT_BAILOUT_NONE
	},

	{
	FRACTYPE_BIFURCATION_MAY,
	s_bifmay_name,
		{s_filter_cycles, s_seed_population, "Beta >= 2", ""},
		{300.0, 0.9, 5, 0},
		HT_BIF, HF_BIFMAY, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_NO_ZOOM_BOX_ROTATE,
		-3.5f, -0.9f, -0.5f, 3.2f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_BIFURCATION_MAY_L, SYMMETRY_NONE,
		bifurcation_may_fp, NULL,
		bifurcation_may_setup, bifurcation,
		ORBIT_BAILOUT_NONE
	},

	{
		// TODO: FRACTYPE_..._MP
	FRACTYPE_HALLEY_MP,
	s_halley_name + 1,
		{s_order, s_real_relaxation_coefficient, s_epsilon, s_imag_relaxation_coefficient},
		{6, 1.0, 0.0001, 0},
		HT_HALLEY, HF_HALLEY, 0,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_HALLEY, SYMMETRY_XY_AXIS,
		halley_orbit_mpc, halley_per_pixel_mpc,
		halley_setup, standard_fractal,
		ORBIT_BAILOUT_NONE
	},

	{
	FRACTYPE_HALLEY,
	s_halley_name,
		{s_order, s_real_relaxation_coefficient, s_epsilon, s_imag_relaxation_coefficient},
		{6, 1.0, 0.0001, 0},
		HT_HALLEY, HF_HALLEY, 0,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		// TODO: FRACTYPE_..._MP
		FRACTYPE_HALLEY_MP, SYMMETRY_XY_AXIS,
		halley_orbit_fp, halley_per_pixel,
		halley_setup, standard_fractal,
		ORBIT_BAILOUT_NONE
	},

	{
	FRACTYPE_DYNAMIC_FP,
	"dynamic",
		{"+# of intervals (<0 = connect)", "time step (<0 = Euler)", "a", "b"},
		{50, .1, 1, 3},
		HT_DYNAM, HF_DYNAM, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_1_FUNCTION,
		-20.0f, 20.0f, -20.0f, 20.0f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_NONE,
		(VF) dynamic_orbit_fp, NULL,
		dynamic_2d_setup_fp, dynamic_2d_fp,
		ORBIT_BAILOUT_NONE
	},

	{
	FRACTYPE_QUATERNION_FP,
	"quat",
		{"notused", "notused", "cj", "ck"},
		{0, 0, 0, 0},
		HT_QUAT, HF_QUAT, FRACTALFLAG_JULIBROT,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, FRACTYPE_QUATERNION_JULIA_FP, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_X_AXIS,
		quaternion_orbit_fp, quaternion_per_pixel_fp,
		mandelbrot_setup_fp, standard_fractal,
		ORBIT_BAILOUT_TRIG_L
	},

	{
	FRACTYPE_QUATERNION_JULIA_FP,
	"quatjul",
		{"c1", "ci", "cj", "ck"},
		{-.745, 0, .113, .05},
		HT_QUAT, HF_QUATJ, FRACTALFLAG_JULIBROT | FRACTALFLAG_MORE_PARAMETERS,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_QUATERNION_FP,
		FRACTYPE_NO_FRACTAL, SYMMETRY_ORIGIN,
		quaternion_orbit_fp, quaternion_julia_per_pixel_fp,
		julia_setup_fp, standard_fractal,
		ORBIT_BAILOUT_TRIG_L
	},

	{
	FRACTYPE_CELLULAR,
	"cellular",
		{
			"#Initial String | 0 = Random | -1 = Reuse Last Random",
			"#Rule = # of digits (see below) | 0 = Random",
			"+Type (see below)",
			"#Starting Row Number"
		},
		{11.0, 3311100320.0, 41.0, 0},
		HT_CELLULAR, HF_CELLULAR, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_NO_ZOOM,
		-1.0f, 1.0f, -1.0f, 1.0f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_NONE,
		NULL, NULL,
		cellular_setup, cellular,
		ORBIT_BAILOUT_NONE
	},

	{
	FRACTYPE_JULIBROT_FP,
	s_julibrot_name,
		{"", "", "", ""},
		{0, 0, 0, 0},
		HT_JULIBROT, -1, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_NO_ZOOM_BOX_ROTATE | FRACTALFLAG_NOT_RESUMABLE,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_JULIBROT, SYMMETRY_NONE,
		julia_orbit_fp, julibrot_per_pixel_fp,
		julibrot_setup, standard_4d_fractal_fp,
		ORBIT_BAILOUT_STANDARD
	},

#ifdef RANDOM_RUN
	{
	FRACTYPE_INVERSE_JULIA,
	s_julia_inverse_name + 1,
		{s_parameter_real, s_parameter_imag, s_max_hits_per_pixel, "Random Run Interval"},
		{-0.11, 0.6557, 4, 1024},
		HT_INVERSE, HF_INVERSE, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_INFINITE_CALCULATION | FRACTALFLAG_NOT_RESUMABLE,
		-2.0f, 2.0f, -1.5f, 1.5f,
		24, FRACTYPE_NO_FRACTAL, FRACTYPE_MANDELBROT,
		FRACTYPE_INVERSE_JULIA_FP, SYMMETRY_NONE,
		Linverse_julia_orbit, NULL,
		orbit_3d_setup, inverse_julia_per_image,
		ORBIT_BAILOUT_NONE
	},

	{
	FRACTYPE_INVERSE_JULIA_FP,
	s_julia_inverse_name,
		{s_parameter_real, s_parameter_imag, s_max_hits_per_pixel, "Random Run Interval"},
		{-0.11, 0.6557, 4, 1024},
		HT_INVERSE, HF_INVERSE, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_INFINITE_CALCULATION | FRACTALFLAG_NOT_RESUMABLE,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_MANDELBROT,
		FRACTYPE_INVERSE_JULIA, SYMMETRY_NONE,
		Minverse_julia_orbit, NULL,
		orbit_3d_setup_fp, inverse_julia_per_image,
		ORBIT_BAILOUT_NONE
	},
#else
	{
	FRACTYPE_INVERSE_JULIA,
	s_julia_inverse_name + 1,
		{s_parameter_real, s_parameter_imag, s_max_hits_per_pixel, ""},
		{-0.11, 0.6557, 4, 1024},
		HT_INVERSE, HF_INVERSE, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_INFINITE_CALCULATION | FRACTALFLAG_NOT_RESUMABLE,
		-2.0f, 2.0f, -1.5f, 1.5f,
		24, FRACTYPE_NO_FRACTAL, FRACTYPE_MANDELBROT,
		FRACTYPE_INVERSE_JULIA_FP, SYMMETRY_NONE,
		Linverse_julia_orbit, NULL,
		orbit_3d_setup, inverse_julia_per_image,
		ORBIT_BAILOUT_NONE
	},

	{
	FRACTYPE_INVERSE_JULIA_FP,
	s_julia_inverse_name,
		{s_parameter_real, s_parameter_imag, s_max_hits_per_pixel, ""},
		{-0.11, 0.6557, 4, 1024},
		HT_INVERSE, HF_INVERSE, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_INFINITE_CALCULATION | FRACTALFLAG_NOT_RESUMABLE,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_MANDELBROT,
		FRACTYPE_INVERSE_JULIA, SYMMETRY_NONE,
		Minverse_julia_orbit, NULL,
		orbit_3d_setup_fp, inverse_julia_per_image,
		ORBIT_BAILOUT_NONE
	},

#endif

	{
	FRACTYPE_MANDELBROT_CLOUD,
	"mandelcloud",
		{"+# of intervals (<0 = connect)", "", "", ""},
		{50, 0, 0, 0},
		HT_MANDELCLOUD, HF_MANDELCLOUD, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING,
		-2.5f, 1.5f, -1.5f, 1.5f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_NONE,
		(VF) mandel_cloud_orbit_fp, NULL,
		dynamic_2d_setup_fp, dynamic_2d_fp,
		ORBIT_BAILOUT_NONE
	},

	{
	FRACTYPE_PHOENIX,
	s_phoenix_name + 1,
		{s_p1_real, s_p2_real, s_degree_z, ""},
		{0.56667, -0.5, 0, 0},
		HT_PHOENIX, HF_PHOENIX, FRACTALFLAG_BAIL_OUT_TESTS,
		-2.0f, 2.0f, -1.5f, 1.5f,
		1, FRACTYPE_NO_FRACTAL, FRACTYPE_MANDELBROT_PHOENIX,
		FRACTYPE_PHOENIX_FP, SYMMETRY_X_AXIS,
		phoenix_orbit, phoenix_per_pixel,
		phoenix_setup, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{
	FRACTYPE_PHOENIX_FP,
	s_phoenix_name,
		{s_p1_real, s_p2_real, s_degree_z, ""},
		{0.56667, -0.5, 0, 0},
		HT_PHOENIX, HF_PHOENIX, FRACTALFLAG_BAIL_OUT_TESTS,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_MANDELBROT_PHOENIX_FP,
		FRACTYPE_PHOENIX, SYMMETRY_X_AXIS,
		phoenix_orbit_fp, phoenix_per_pixel_fp,
		phoenix_setup, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{
	FRACTYPE_MANDELBROT_PHOENIX,
	s_mandphoenix_name + 1,
		{s_real_z0, s_imag_z0, s_degree_z, ""},
		{0.0, 0.0, 0, 0},
		HT_PHOENIX, HF_MANDPHOENIX, FRACTALFLAG_BAIL_OUT_TESTS,
		-2.5f, 1.5f, -1.5f, 1.5f,
		1, FRACTYPE_PHOENIX, FRACTYPE_NO_FRACTAL,
		FRACTYPE_MANDELBROT_PHOENIX_FP, SYMMETRY_NONE,
		phoenix_orbit, mandelbrot_phoenix_per_pixel,
		mandelbrot_phoenix_setup, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{
	FRACTYPE_MANDELBROT_PHOENIX_FP,
	s_mandphoenix_name,
		{s_real_z0, s_imag_z0, s_degree_z, ""},
		{0.0, 0.0, 0, 0},
		HT_PHOENIX, HF_MANDPHOENIX, FRACTALFLAG_BAIL_OUT_TESTS,
		-2.5f, 1.5f, -1.5f, 1.5f,
		0, FRACTYPE_PHOENIX_FP, FRACTYPE_NO_FRACTAL,
		FRACTYPE_MANDELBROT_PHOENIX, SYMMETRY_NONE,
		phoenix_orbit_fp, mandelbrot_phoenix_per_pixel_fp,
		mandelbrot_phoenix_setup, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{
	FRACTYPE_HYPERCOMPLEX_FP,
	"hypercomplex",
		{"notused", "notused", "cj", "ck"},
		{0, 0, 0, 0},
		HT_HYPERC, HF_HYPERC, FRACTALFLAG_JULIBROT | FRACTALFLAG_1_FUNCTION,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, FRACTYPE_HYPERCOMPLEX_JULIA_FP, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_X_AXIS,
		hyper_complex_orbit_fp, quaternion_per_pixel_fp,
		mandelbrot_setup_fp, standard_fractal,
		ORBIT_BAILOUT_TRIG_L
	},

	{
	FRACTYPE_HYPERCOMPLEX_JULIA_FP,
	"hypercomplexj",
		{"c1", "ci", "cj", "ck"},
		{-.745, 0, .113, .05},
		HT_HYPERC, HF_HYPERCJ, FRACTALFLAG_JULIBROT | FRACTALFLAG_1_FUNCTION | FRACTALFLAG_MORE_PARAMETERS,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_HYPERCOMPLEX_FP,
		FRACTYPE_NO_FRACTAL, SYMMETRY_ORIGIN,
		hyper_complex_orbit_fp, quaternion_julia_per_pixel_fp,
		julia_setup_fp, standard_fractal,
		ORBIT_BAILOUT_TRIG_L
	},

	{
	FRACTYPE_FROTHY_BASIN,
	s_frothybasin_name + 1,
		{s_frothy_mapping, s_frothy_shade, s_frothy_a_value, ""},
		{1, 0, 1.028713768218725, 0},
		HT_FROTH, HF_FROTH, FRACTALFLAG_NO_BOUNDARY_TRACING,
		-2.8f, 2.8f, -2.355f, 1.845f,
		28, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_FROTHY_BASIN_FP, SYMMETRY_NONE,
		froth_per_orbit, froth_per_pixel,
		froth_setup, froth_calc,
		ORBIT_BAILOUT_FROTHY_BASIN
	},

	{
	FRACTYPE_FROTHY_BASIN_FP,
	s_frothybasin_name,
		{s_frothy_mapping, s_frothy_shade, s_frothy_a_value, ""},
		{1, 0, 1.028713768218725, 0},
		HT_FROTH, HF_FROTH, FRACTALFLAG_NO_BOUNDARY_TRACING,
		-2.8f, 2.8f, -2.355f, 1.845f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_FROTHY_BASIN, SYMMETRY_NONE,
		froth_per_orbit, froth_per_pixel,
		froth_setup, froth_calc,
		ORBIT_BAILOUT_FROTHY_BASIN
	},

	{
	FRACTYPE_MANDELBROT_4_FP,
	s_mandel4_name,
		{s_real_z0, s_imag_z0, "", ""},
		{0, 0, 0, 0},
		HT_MANDJUL4, HF_MANDEL4, FRACTALFLAG_BAIL_OUT_TESTS,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, FRACTYPE_JULIA_4_FP, FRACTYPE_NO_FRACTAL,
		FRACTYPE_MANDELBROT_4, SYMMETRY_X_AXIS_NO_PARAMETER,
		mandel4_orbit_fp, mandelbrot_per_pixel_fp,
		mandelbrot_setup_fp, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{
	FRACTYPE_JULIA_4_FP,
	s_julia4_name,
		{s_parameter_real, s_parameter_imag, "", ""},
		{0.6, 0.55, 0, 0},
		HT_MANDJUL4, HF_JULIA4, FRACTALFLAG_JULIBROT | FRACTALFLAG_BAIL_OUT_TESTS,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_MANDELBROT_4_FP,
		FRACTYPE_JULIA_4, SYMMETRY_ORIGIN,
		mandel4_orbit_fp, julia_per_pixel_fp,
		julia_setup_fp, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{
	FRACTYPE_MARKS_MANDELBROT_FP,
	s_marksmandel_name,
		{s_real_z0, s_imag_z0, s_exponent, ""},
		{0, 0, 1, 0},
		HT_MARKS, HF_MARKSMAND, FRACTALFLAG_BAIL_OUT_TESTS,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, FRACTYPE_MARKS_JULIA_FP, FRACTYPE_NO_FRACTAL,
		FRACTYPE_MARKS_MANDELBROT, SYMMETRY_NONE,
		marks_lambda_orbit_fp, marks_mandelbrot_per_pixel_fp,
		mandelbrot_setup_fp, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{
	FRACTYPE_MARKS_JULIA_FP,
	s_marksjulia_name,
		{s_parameter_real, s_parameter_imag, s_exponent, ""},
		{0.1, 0.9, 1, 0},
		HT_MARKS, HF_MARKSJULIA, FRACTALFLAG_BAIL_OUT_TESTS,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_MARKS_MANDELBROT_FP,
		FRACTYPE_MARKS_JULIA, SYMMETRY_ORIGIN,
		marks_lambda_orbit_fp, julia_per_pixel_fp,
		marks_julia_setup_fp, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	/* dmf */
	{
	FRACTYPE_ICON,
	"icons",
		{"Lambda", "Alpha", "Beta", "Gamma"},
		{-2.34, 2.0, 0.2, 0.1},
		HT_ICON, HF_ICON, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_INFINITE_CALCULATION | FRACTALFLAG_MORE_PARAMETERS,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_NONE,
		(VF) icon_orbit_fp, NULL,
		orbit_3d_setup_fp, orbit_2d_fp,
		ORBIT_BAILOUT_NONE
	},

	/* dmf */
	{
	FRACTYPE_ICON_3D,
	"icons3d",
		{"Lambda", "Alpha", "Beta", "Gamma"},
		{-2.34, 2.0, 0.2, 0.1},
		HT_ICON, HF_ICON, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_INFINITE_CALCULATION | FRACTALFLAG_3D_PARAMETERS | FRACTALFLAG_MORE_PARAMETERS,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_NONE,
		(VF) icon_orbit_fp, NULL,
		orbit_3d_setup_fp, orbit_3d_fp,
		ORBIT_BAILOUT_NONE
	},

	{
	FRACTYPE_PHOENIX_COMPLEX,
	s_phoenixcplx_name + 1,
		{s_p1_real, s_p1_imag, s_p2_real, s_p2_imag},
		{0.2, 0, 0.3, 0},
		HT_PHOENIX, HF_PHOENIXCPLX, FRACTALFLAG_MORE_PARAMETERS | FRACTALFLAG_BAIL_OUT_TESTS,
		-2.0f, 2.0f, -1.5f, 1.5f,
		1, FRACTYPE_NO_FRACTAL, FRACTYPE_MANDELBROT_PHOENIX_COMPLEX,
		FRACTYPE_PHOENIX_COMPLEX_FP, SYMMETRY_ORIGIN,
		phoenix_complex_orbit, phoenix_per_pixel,
		phoenix_complex_setup, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{
	FRACTYPE_PHOENIX_COMPLEX_FP,
	s_phoenixcplx_name,
		{s_p1_real, s_p1_imag, s_p2_real, s_p2_imag},
		{0.2, 0, 0.3, 0},
		HT_PHOENIX, HF_PHOENIXCPLX, FRACTALFLAG_MORE_PARAMETERS | FRACTALFLAG_BAIL_OUT_TESTS,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_MANDELBROT_PHOENIX_COMPLEX_FP,
		FRACTYPE_PHOENIX_COMPLEX, SYMMETRY_ORIGIN,
		phoenix_complex_orbit_fp, phoenix_per_pixel_fp,
		phoenix_complex_setup, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{
	FRACTYPE_MANDELBROT_PHOENIX_COMPLEX,
	s_mandphoenixcplx_name + 1,
		{s_real_z0, s_imag_z0, s_p2_real, s_p2_imag},
		{0, 0, 0.5, 0},
		HT_PHOENIX, HF_MANDPHOENIXCPLX, FRACTALFLAG_MORE_PARAMETERS | FRACTALFLAG_BAIL_OUT_TESTS,
		-2.5f, 1.5f, -1.5f, 1.5f,
		1, FRACTYPE_PHOENIX_COMPLEX, FRACTYPE_NO_FRACTAL,
		FRACTYPE_MANDELBROT_PHOENIX_COMPLEX_FP, SYMMETRY_X_AXIS,
		phoenix_complex_orbit, mandelbrot_phoenix_per_pixel,
		mandelbrot_phoenix_complex_setup, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{
	FRACTYPE_MANDELBROT_PHOENIX_COMPLEX_FP,
	s_mandphoenixcplx_name,
		{s_real_z0, s_imag_z0, s_p2_real, s_p2_imag},
		{0, 0, 0.5, 0},
		HT_PHOENIX, HF_MANDPHOENIXCPLX, FRACTALFLAG_MORE_PARAMETERS | FRACTALFLAG_BAIL_OUT_TESTS,
		-2.5f, 1.5f, -1.5f, 1.5f,
		0, FRACTYPE_PHOENIX_COMPLEX_FP, FRACTYPE_NO_FRACTAL,
		FRACTYPE_MANDELBROT_PHOENIX_COMPLEX, SYMMETRY_X_AXIS,
		phoenix_complex_orbit_fp, mandelbrot_phoenix_per_pixel_fp,
		mandelbrot_phoenix_complex_setup, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	{
	FRACTYPE_ANT,
	"ant",
		{"#Rule String (1's and non-1's, 0 rand)",
		"#Maxpts",
		"+Numants (max 256)",
		"+Ant type (1 or 2)"
		},
		{1100, 1.0E9, 1, 1},
		HT_ANT, HF_ANT, FRACTALFLAG_NO_ZOOM | FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_NOT_RESUMABLE | FRACTALFLAG_MORE_PARAMETERS,
		-1.0f, 1.0f, -1.0f, 1.0f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_NONE,
		NULL, NULL,
		stand_alone_setup, ant,
		ORBIT_BAILOUT_NONE
	},

	{
	FRACTYPE_CHIP,
	"chip",
		{"a", "b", "c", ""},
		{-15, -19, 1, 0},
		HT_MARTIN, HF_CHIP, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_INFINITE_CALCULATION,
		-760.0f, 760.0f, -570.0f, 570.0f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_NONE,
		(VF) chip_2d_orbit_fp, NULL,
		orbit_3d_setup_fp, orbit_2d_fp,
		ORBIT_BAILOUT_NONE
	},

	{
	FRACTYPE_QUADRUP_TWO,
	"quadruptwo",
		{"a", "b", "c", ""},
		{34, 1, 5, 0},
		HT_MARTIN, HF_QUADRUPTWO, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_INFINITE_CALCULATION,
		-82.93367f, 112.2749f, -55.76383f, 90.64257f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_NONE,
		(VF) quadrup_two_2d_orbit_fp, NULL,
		orbit_3d_setup_fp, orbit_2d_fp,
		ORBIT_BAILOUT_NONE
	},

	{
	FRACTYPE_THREE_PLY,
	"threeply",
		{"a", "b", "c", ""},
		{-55, -1, -42, 0},
		HT_MARTIN, HF_THREEPLY, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_INFINITE_CALCULATION,
		-8000.0f, 8000.0f, -6000.0f, 6000.0f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_NONE,
		(VF) three_ply_2d_orbit_fp, NULL,
		orbit_3d_setup_fp, orbit_2d_fp,
		ORBIT_BAILOUT_NONE
	},

	{
	FRACTYPE_VOLTERRA_LOTKA,
	"volterra-lotka",
		{"h", "p", "", ""},
		{0.739, 0.739, 0, 0},
		HT_VL, HF_VL, 0,
		0.0f, 6.0f, 0.0f, 4.5f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_NONE,
		volterra_lotka_orbit_fp, other_julia_per_pixel_fp,
		volterra_lotka_setup, standard_fractal,
		256
	},

	{
	FRACTYPE_ESCHER_JULIA,
	"escher_julia",
		{s_parameter_real, s_parameter_imag, "", ""},
		{0.32, 0.043, 0, 0},
		HT_ESCHER, HF_ESCHER, 0,
		-1.6f, 1.6f, -1.2f, 1.2f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_ORIGIN,
		escher_orbit_fp, julia_per_pixel_fp,
		standard_setup, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},

	/* From Pickovers' "Chaos in Wonderland"      */
	/* included by Humberto R. Baptista           */
	/* code adapted from king.cpp bt James Rankin */
	{
	FRACTYPE_LATOOCARFIAN,
	"latoocarfian",
		{"a", "b", "c", "d"},
		{-0.966918, 2.879879, 0.765145, 0.744728},
		HT_LATOO, HF_LATOO, FRACTALFLAG_NO_SOLID_GUESSING | FRACTALFLAG_NO_BOUNDARY_TRACING | FRACTALFLAG_INFINITE_CALCULATION | FRACTALFLAG_MORE_PARAMETERS | FRACTALFLAG_4_FUNCTIONS,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_NONE,
		(VF) latoo_orbit_fp, NULL,
		orbit_3d_setup_fp, orbit_2d_fp,
		ORBIT_BAILOUT_NONE
	},
#if 0
	{
	FRACTYPE_MANDELBROT_MIX4,
	"mandelbrotmix4",
		{s_p1_real, s_p1_imag, s_p2_real, s_p2_imag},
		{0.05, 3, -1.5, -2},
		HT_MANDELBROTMIX4, HF_MANDELBROTMIX4, FRACTALFLAG_BAIL_OUT_TESTS | FRACTALFLAG_1_FUNCTION | FRACTALFLAG_MORE_PARAMETERS,
		-2.5f, 1.5f, -1.5f, 1.5f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_NONE,
		mandelbrot_mix4_orbit_fp, mandelbrot_mix4_per_pixel_fp,
		mandelbrot_mix4_setup, standard_fractal,
		ORBIT_BAILOUT_STANDARD
	},
#endif
	{
		0,
		NULL,            /* marks the END of the list */
		{NULL, NULL, NULL, NULL},
		{0, 0, 0, 0},
		-1, -1, 0,
		0.0f, 0.0f, 0.0f, 0.0f,
		0, FRACTYPE_NO_FRACTAL, FRACTYPE_NO_FRACTAL,
		FRACTYPE_NO_FRACTAL, SYMMETRY_NONE,
		NULL, NULL,
		NULL, NULL,
		0
	}
};

int g_num_fractal_types = NUM_OF(g_fractal_specific)-1;

/*
 *  Returns 1 if the formula parameter is not used in the current
 *  formula.  If the parameter is used, or not a formula fractal,
 *  a 0 is returned.  Note: this routine only works for formula types.
 */
int parameter_not_used(int parm)
{
	int ret = 0;

	/* sanity check */
	if (!fractal_type_formula(g_fractal_type))
	{
		return 0;
	}

	switch (parm/2)
	{
	case 0:
		if (!g_formula_state.uses_p1())
		{
			ret = 1;
		}
		break;
	case 1:
		if (!g_formula_state.uses_p2())
		{
			ret = 1;
		}
		break;
	case 2:
		if (!g_formula_state.uses_p3())
		{
			ret = 1;
		}
		break;
	case 3:
		if (!g_formula_state.uses_p4())
		{
			ret = 1;
		}
		break;
	case 4:
		if (!g_formula_state.uses_p5())
		{
			ret = 1;
		}
		break;
	default:
		ret = 0;
		break;
	}
	return ret;
}

/*
 *  Returns 1 if parameter number parm exists for type. If the
 *  parameter exists, the parameter prompt string is copied to buf.
 *  Pass in NULL for buf if only the existence of the parameter is
 *  needed, and not the prompt string.
 */
int type_has_parameter(int type, int parm, char *buf)
{
	int extra;
	char *ret = NULL;
	if (0 <= parm && parm < 4)
	{
		ret = g_fractal_specific[type].parameters[parm];
	}
	else if (parm >= 4 && parm < MAX_PARAMETERS)
	{
		extra = find_extra_parameter(type);
		if (extra > -1)
		{
			ret = g_more_parameters[extra].parameters[parm-4];
		}
	}
	if (ret)
	{
		if (*ret == 0)
		{
			ret = NULL;
		}
	}

	if (fractal_type_formula(type))
	{
		if (parameter_not_used(parm))
		{
			ret = NULL;
		}
	}

	if (ret && buf != NULL)
	{
		strcpy(buf, ret);
	}
	return ret ? 1 : 0;
}

/* locate alternate math record */
alternate_math *find_alternate_math(int math)
{
	if (math == 0)
	{
		return NULL;
	}
	for (int i = 0; i < NUM_OF(s_alternate_math); i++)
	{
		if ((g_fractal_type == s_alternate_math[i].type) && s_alternate_math[i].math)
		{
			return &s_alternate_math[i];
		}
	}
	return NULL;
}

bool fractal_type_formula(int fractal_type)
{
	return fractal_type == FRACTYPE_FORMULA || fractal_type == FRACTYPE_FORMULA_FP;
}

bool fractal_type_julia(int fractal_type)
{
	return fractal_type == FRACTYPE_JULIA_FP || fractal_type == FRACTYPE_JULIA;
}

bool fractal_type_inverse_julia(int fractal_type)
{
	return fractal_type == FRACTYPE_INVERSE_JULIA || fractal_type == FRACTYPE_INVERSE_JULIA_FP;
}

bool fractal_type_julia_or_inverse(int fractal_type)
{
	return fractal_type_julia(fractal_type) || fractal_type_inverse_julia(fractal_type);
}

bool fractal_type_mandelbrot(int fractal_type)
{
	return fractal_type == FRACTYPE_MANDELBROT || fractal_type == FRACTYPE_MANDELBROT_FP;
}

bool fractal_type_ifs(int fractal_type)
{
	return fractal_type == FRACTYPE_IFS || fractal_type == FRACTYPE_IFS_3D;
}

bool fractal_type_none(int fractal_type)
{
	return fractal_type == FRACTYPE_NO_FRACTAL;
}

bool fractal_type_ant_or_cellular(int fractal_type)
{
	return fractal_type == FRACTYPE_CELLULAR || fractal_type == FRACTYPE_ANT;
}

bool fractal_type_julibrot(int fractal_type)
{
	return fractal_type == FRACTYPE_JULIBROT || fractal_type == FRACTYPE_JULIBROT_FP;
}
