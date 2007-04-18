/*
		This module consists only of the g_fractal_specific structure
		and a *slew* of defines needed to get it to compile
*/
#include <string.h>

/* includes needed for g_fractal_specific */

/* see Fractint.c for a description of the "include"  hierarchy */
extern "C"
{
#include "port.h"
#include "prototyp.h"
#include "helpdefs.h"
#include "fractype.h"
}

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
#define BAILOUT_TRIG_L			64
#define BAILOUT_FROTHY_BASIN	7
#define BAILOUT_STANDARD		4
#define BAILOUT_NONE			0

extern "C" more_parameters g_more_parameters[] =
{
	{ICON             , { "Omega", "+Degree of symmetry",   "", "", "", ""}, {0, 3, 0, 0, 0, 0}},
	{ICON3D           , { "Omega", "+Degree of symmetry",   "", "", "", ""}, {0, 3, 0, 0, 0, 0}},
	{HYPERCMPLXJFP    , { "zj",      "zk",          "", "", "", ""}, {0, 0, 0, 0, 0, 0}},
	{QUATJULFP        , { "zj",      "zk",          "", "", "", ""}, {0, 0, 0, 0, 0, 0}},
	{PHOENIXCPLX      , { s_degree_z, "",          "", "", "", ""}, {0, 0, 0, 0, 0, 0}},
	{PHOENIXFPCPLX    , { s_degree_z, "",          "", "", "", ""}, {0, 0, 0, 0, 0, 0}},
	{MANDPHOENIXCPLX  , { s_degree_z, "",          "", "", "", ""}, {0, 0, 0, 0, 0, 0}},
	{MANDPHOENIXFPCPLX, { s_degree_z, "",          "", "", "", ""}, {0, 0, 0, 0, 0, 0}},
	{FORMULA  , { s_p3_real, s_p3_imag, s_p4_real, s_p4_imag, s_p5_real, s_p5_imag}, {0, 0, 0, 0, 0, 0}},
	{FFORMULA , { s_p3_real, s_p3_imag, s_p4_real, s_p4_imag, s_p5_real, s_p5_imag}, {0, 0, 0, 0, 0, 0}},
	{ANT              , { "+Wrap?", s_random_seed, "", "", "", ""}, {1, 0, 0, 0, 0, 0}},
	{MANDELBROTMIX4   , { s_p3_real, s_p3_imag,        "", "", "", ""}, {0, 0, 0, 0, 0, 0}},
	{-1               , { NULL, NULL, NULL, NULL, NULL, NULL    }, {0, 0, 0, 0, 0, 0}}
};

/*
	type  math orbitcalc fnct per_pixel fnct per_image fnct
	|-----|----|--------------|--------------|--------------| */
extern "C" alternate_math g_alternate_math[] =
{
#define USEBN
#ifdef USEBN
	{ JULIAFP,	BIGNUM, julia_orbit_bn, julia_per_pixel_bn,  mandelbrot_setup_bn },
	{ MANDELFP,	BIGNUM, julia_orbit_bn, mandelbrot_per_pixel_bn, mandelbrot_setup_bn },
#else
	{ JULIAFP,	BIGFLT, julia_orbit_bf, julia_per_pixel_bf,  mandelbrot_setup_bf },
	{ MANDELFP,	BIGFLT, julia_orbit_bf, mandelbrot_per_pixel_bf, mandelbrot_setup_bf },
#endif
	/*
	NOTE: The default precision for g_bf_math=BIGNUM is not high enough
	for julia_z_power_orbit_bn.  If you want to test BIGNUM (1) instead
	of the usual BIGFLT (2), then set bfdigits on the command to
	increase the precision.
	*/
	{ FPJULIAZPOWER,  BIGFLT, julia_z_power_orbit_bf, julia_per_pixel_bf, mandelbrot_setup_bf },
	{ FPMANDELZPOWER, BIGFLT, julia_z_power_orbit_bf, mandelbrot_per_pixel_bf, mandelbrot_setup_bf }
};
extern "C" int g_alternate_math_len = NUM_OF(g_alternate_math);

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
#define VF int(*)(void)

extern "C" struct fractal_specific_stuff g_fractal_specific[] =
{
	/*
	{ [OLDTYPEINDEX, ]NEWTYPEINDEX
		fractal name,
		{parameter text strings},
		{parameter values},
		helptext, helpformula, flags,
		g_x_min, g_x_max, g_y_min, g_y_max,
		int, tojulia, tomandel, tofloat, symmetry,
		orbit fnct, per_pixel fnct, per_image fnct, calctype fcnt,
		bailout
	}
	*/

	{ /* MANDEL */
	s_mandel_name + 1,
		{s_real_z0, s_imag_z0, "", ""},
		{0, 0, 0, 0},
		HT_MANDEL, HF_MANDEL, BAILTEST,
		-2.5f, 1.5f, -1.5f, 1.5f,
		1, JULIA, NOFRACTAL, MANDELFP, XAXIS_NOPARM,
		julia_orbit, mandelbrot_per_pixel, mandelbrot_setup, standard_fractal,
		BAILOUT_STANDARD
	},

	{ /* JULIA */
	s_julia_name + 1,
		{s_parameter_real, s_parameter_imag, "", ""},
		{0.3, 0.6, 0, 0},
		HT_JULIA, HF_JULIA, OKJB | BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		1, NOFRACTAL, MANDEL, JULIAFP, ORIGIN,
		julia_orbit, julia_per_pixel, julia_setup, standard_fractal,
		BAILOUT_STANDARD
	},

	{ /* NEWTBASIN */
	s_newtbasin_name,
		{s_newton_degree, "Enter non-zero value for stripes", "", ""},
		{3, 0, 0, 0},
		HT_NEWTBAS, HF_NEWTBAS, 0,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, NOFRACTAL, MPNEWTBASIN, NOSYM,
		newton2_orbit, other_julia_per_pixel_fp, newton_setup, standard_fractal,
		BAILOUT_NONE
	},

	{ /* LAMBDA */
	s_lambda_name + 1,
		{s_parameter_real, s_parameter_imag, "", ""},
		{0.85, 0.6, 0, 0},
		HT_LAMBDA, HF_LAMBDA, OKJB | BAILTEST,
		-1.5f, 2.5f, -1.5f, 1.5f,
		1, NOFRACTAL, MANDELLAMBDA, LAMBDAFP, NOSYM,
		lambda_orbit, julia_per_pixel, julia_setup_l, standard_fractal,
		BAILOUT_STANDARD
	},

	{ /* MANDELFP */
	s_mandel_name,
		{s_real_z0, s_imag_z0, "", ""},
		{0, 0, 0, 0},
		HT_MANDEL, HF_MANDEL, BAILTEST | BF_MATH,
		-2.5f, 1.5f, -1.5f, 1.5f,
		0, JULIAFP, NOFRACTAL, MANDEL, XAXIS_NOPARM,
		julia_orbit_fp, mandelbrot_per_pixel_fp, mandelbrot_setup_fp, standard_fractal,
		BAILOUT_STANDARD
	},

	{ /* NEWTON */
	s_newton_name,
		{s_newton_degree, "", "", ""},
		{3, 0, 0, 0},
		HT_NEWT, HF_NEWT, 0,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, NOFRACTAL, MPNEWTON, XAXIS,
		newton2_orbit, other_julia_per_pixel_fp, newton_setup, standard_fractal,
		BAILOUT_NONE
	},

	{ /* JULIAFP */
	s_julia_name,
		{s_parameter_real, s_parameter_imag, "", ""},
		{0.3, 0.6, 0, 0},
		HT_JULIA, HF_JULIA, OKJB | BAILTEST | BF_MATH,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, MANDELFP, JULIA, ORIGIN,
		julia_orbit_fp, julia_per_pixel_fp,  julia_setup_fp, standard_fractal,
		BAILOUT_STANDARD
	},

	{ /* PLASMA */
	"plasma",
		{"Graininess Factor (0 or 0.125 to 100, default is 2)",
		"+Algorithm (0 = original, 1 = new)",
		"+Random Seed Value (0 = Random, 1 = Reuse Last)",
		"+Save as Pot File? (0 = No,     1 = Yes)"
		},
		{2, 0, 0, 0},
		HT_PLASMA, HF_PLASMA, NOZOOM | NOGUESS | NOTRACE | NORESUME,
		-2.0f, 2.0f, -1.5f, 1.5f,
		1, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
		NULL, NULL, stand_alone_setup, plasma,
		BAILOUT_NONE
	},

	{ /* LAMBDASINE, MANDELTRIGFP */
	s_mandelfn_name,
		{s_real_z0, s_imag_z0, "", ""},
		{0, 0, 0, 0},
		HT_MANDFN, HF_MANDFN, TRIG1,
		-8.0f, 8.0f, -6.0f, 6.0f,
		0, LAMBDATRIGFP, NOFRACTAL, MANDELTRIG, XYAXIS_NOPARM,
		lambda_trig_orbit_fp, other_mandelbrot_per_pixel_fp, mandelbrot_trig_setup,
			standard_fractal,
		BAILOUT_TRIG_L
	},

	{ /* LAMBDACOS, MANOWARFP */
	s_manowar_name,
		{s_real_z0, s_imag_z0, "", ""},
		{0, 0, 0, 0},
		HT_SCOTSKIN, HF_MANOWAR, BAILTEST,
		-2.5f, 1.5f, -1.5f, 1.5f,
		0, MANOWARJFP, NOFRACTAL, MANOWAR, XAXIS_NOPARM,
		man_o_war_orbit_fp, mandelbrot_per_pixel_fp, mandelbrot_setup_fp, standard_fractal,
		BAILOUT_STANDARD
	},

	{ /* LAMBDAEXP, MANOWAR */
	s_manowar_name + 1,
		{s_real_z0, s_imag_z0, "", ""},
		{0, 0, 0, 0},
		HT_SCOTSKIN, HF_MANOWAR, BAILTEST,
		-2.5f,  1.5f, -1.5f, 1.5f,
		1, MANOWARJ, NOFRACTAL, MANOWARFP, XAXIS_NOPARM,
		man_o_war_orbit, mandelbrot_per_pixel, mandelbrot_setup_l, standard_fractal,
		BAILOUT_STANDARD
	},

	{ /* TEST */
	"test",
		{"(testpt Param #1)",
		"(testpt param #2)",
		"(testpt param #3)",
		"(testpt param #4)"
		},
		{0, 0, 0, 0},
		HT_TEST, HF_TEST, 0,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
		NULL, NULL, stand_alone_setup, test,
		BAILOUT_STANDARD
	},

	{ /* SIERPINSKI */
	s_sierpinski_name + 1,
		{"", "", "", ""},
		{0, 0, 0, 0},
		HT_SIER, HF_SIER, 0,
		-4.0f/3.0f, 96.0f/45.0f, -0.9f, 1.7f,
		1, NOFRACTAL, NOFRACTAL, SIERPINSKIFP, NOSYM,
		sierpinski_orbit, julia_per_pixel_l, sierpinski_setup,
				standard_fractal,
		127
	},

	{ /* BARNSLEYM1 */
	s_barnsleym1_name + 1,
		{s_real_z0, s_imag_z0, "", ""},
		{0, 0, 0, 0},
		HT_BARNS, HF_BARNSM1, BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		1, BARNSLEYJ1, NOFRACTAL, BARNSLEYM1FP, XYAXIS_NOPARM,
		barnsley1_orbit, mandelbrot_per_pixel_l, mandelbrot_setup_l,
				standard_fractal,
		BAILOUT_STANDARD
	},

	{ /* BARNSLEYJ1 */
	s_barnsleyj1_name + 1,
		{s_parameter_real, s_parameter_imag, "", ""},
		{0.6, 1.1, 0, 0},
		HT_BARNS, HF_BARNSJ1, OKJB | BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		1, NOFRACTAL, BARNSLEYM1, BARNSLEYJ1FP, ORIGIN,
		barnsley1_orbit, julia_per_pixel_l, julia_setup_l, standard_fractal,
		BAILOUT_STANDARD
	},

	{ /* BARNSLEYM2 */
	s_barnsleym2_name + 1,
		{s_real_z0, s_imag_z0, "", ""},
		{0, 0, 0, 0},
		HT_BARNS, HF_BARNSM2, BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		1, BARNSLEYJ2, NOFRACTAL, BARNSLEYM2FP, YAXIS_NOPARM,
		barnsley2_orbit, mandelbrot_per_pixel_l, mandelbrot_setup_l,
				standard_fractal,
		BAILOUT_STANDARD
	},

	{ /* BARNSLEYJ2 */
	s_barnsleyj2_name + 1,
		{s_parameter_real, s_parameter_imag, "", ""},
		{0.6, 1.1, 0, 0},
		HT_BARNS, HF_BARNSJ2, OKJB | BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		1, NOFRACTAL, BARNSLEYM2, BARNSLEYJ2FP, ORIGIN,
		barnsley2_orbit, julia_per_pixel_l, julia_setup_l, standard_fractal,
		BAILOUT_STANDARD
	},

	{ /* MANDELSINE, SQRTRIG */
	s_sqr_fn__name + 1,
		{"", "", "", ""},
		{0, 0, 0, 0},
		HT_SCOTSKIN, HF_SQRFN, TRIG1 | BAILTEST,
		-4.0f, 4.0f, -3.0f, 3.0f,
		16, NOFRACTAL, NOFRACTAL, SQRTRIGFP, XAXIS,
		sqr_trig_orbit, julia_per_pixel_l, sqr_trig_setup, standard_fractal,
		BAILOUT_TRIG_L
	},

	{ /* MANDELCOS, SQRTRIGFP */
	s_sqr_fn__name,
		{"", "", "", ""},
		{0, 0, 0, 0},
		HT_SCOTSKIN, HF_SQRFN, TRIG1 | BAILTEST,
		-4.0f, 4.0f, -3.0f, 3.0f,
		0, NOFRACTAL, NOFRACTAL, SQRTRIG, XAXIS,
		sqr_trig_orbit_fp, other_julia_per_pixel_fp, sqr_trig_setup, standard_fractal,
		BAILOUT_TRIG_L
	},

	{ /* MANDELEXP, TRIGPLUSTRIG */
	s_fnplusfn_name + 1,
		{s_trig1_coefficient_re, s_trig1_coefficient_im, s_trig2_coefficient_re, s_trig2_coefficient_im},
		{1, 0, 1, 0},
		HT_SCOTSKIN, HF_FNPLUSFN, TRIG2 | BAILTEST,
		-4.0f, 4.0f, -3.0f, 3.0f,
		16, NOFRACTAL, NOFRACTAL, TRIGPLUSTRIGFP, XAXIS,
		trig_plus_trig_orbit, julia_per_pixel_l, trig_plus_trig_setup_l,
				standard_fractal,
		BAILOUT_TRIG_L
	},

	{ /* MANDELLAMBDA */
	s_mandellambda_name + 1,
		{s_real_z0, s_imag_z0, "", ""},
		{0, 0, 0, 0},
		HT_MLAMBDA, HF_MLAMBDA, BAILTEST,
		-3.0f, 5.0f, -3.0f, 3.0f,
		1, LAMBDA, NOFRACTAL, MANDELLAMBDAFP, XAXIS_NOPARM,
		lambda_orbit, mandelbrot_per_pixel, mandelbrot_setup_l, standard_fractal,
		BAILOUT_STANDARD
	},

	{ /* MARKSMANDEL */
	s_marksmandel_name + 1,
		{s_real_z0, s_imag_z0, s_exponent, ""},
		{0, 0, 1, 0},
		HT_MARKS, HF_MARKSMAND, BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		1, MARKSJULIA, NOFRACTAL, MARKSMANDELFP, NOSYM,
		marks_lambda_orbit, marks_mandelbrot_per_pixel, mandelbrot_setup_l,
				standard_fractal,
		BAILOUT_STANDARD
	},

	{ /* MARKSJULIA */
	s_marksjulia_name + 1,
		{s_parameter_real, s_parameter_imag, s_exponent, ""},
		{0.1, 0.9, 1, 0},
		HT_MARKS, HF_MARKSJULIA, BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		1, NOFRACTAL, MARKSMANDEL, MARKSJULIAFP, ORIGIN,
		marks_lambda_orbit, julia_per_pixel, marks_julia_setup, standard_fractal,
		BAILOUT_STANDARD
	},

	{ /* UNITY */
	s_unity_name + 1,
		{"", "", "", ""},
		{0, 0, 0, 0},
		HT_UNITY, HF_UNITY, 0,
		-2.0f, 2.0f, -1.5f, 1.5f,
		1, NOFRACTAL, NOFRACTAL, UNITYFP, XYAXIS,
		unity_orbit, julia_per_pixel_l, unity_setup, standard_fractal,
		BAILOUT_NONE
	},

	{ /* MANDEL4 */
	s_mandel4_name + 1,
		{s_real_z0, s_imag_z0, "", ""},
		{0, 0, 0, 0},
		HT_MANDJUL4, HF_MANDEL4, BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		1, JULIA4, NOFRACTAL, MANDEL4FP, XAXIS_NOPARM,
		mandel4_orbit, mandelbrot_per_pixel, mandelbrot_setup_l, standard_fractal,
		BAILOUT_STANDARD
	},

	{ /* JULIA4 */
	s_julia4_name + 1,
		{s_parameter_real, s_parameter_imag, "", ""},
		{0.6, 0.55, 0, 0},
		HT_MANDJUL4, HF_JULIA4, OKJB | BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		1, NOFRACTAL, MANDEL4, JULIA4FP, ORIGIN,
		mandel4_orbit, julia_per_pixel, julia_setup_l, standard_fractal,
		BAILOUT_STANDARD
	},

	{ /* IFS */
	"ifs",
		{s_color_method, "", "", ""},
		{0, 0, 0, 0},
		HT_IFS, -4, NOGUESS | NOTRACE | NORESUME | INFCALC,
		-8.0f, 8.0f, -1.0f, 11.0f,
		16, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
		NULL, NULL, stand_alone_setup, ifs,
		BAILOUT_NONE
	},

	{ /* IFS3D */
	s_ifs3d_name,
		{s_color_method, "", "", ""},
		{0, 0, 0, 0},
		HT_IFS, -4, NOGUESS | NOTRACE | NORESUME | PARMS3D | INFCALC,
		-11.0f, 11.0f, -11.0f, 11.0f,
		16, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
		NULL, NULL, stand_alone_setup, ifs,
		BAILOUT_NONE
	},

	{ /* BARNSLEYM3 */
	s_barnsleym3_name + 1,
		{s_real_z0, s_imag_z0, "", ""},
		{0, 0, 0, 0},
		HT_BARNS, HF_BARNSM3, BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		1, BARNSLEYJ3, NOFRACTAL, BARNSLEYM3FP, XAXIS_NOPARM,
		barnsley3_orbit, mandelbrot_per_pixel_l, mandelbrot_setup_l,
			standard_fractal,
		BAILOUT_STANDARD
	},

	{ /* BARNSLEYJ3 */
	s_barnsleyj3_name + 1,
		{s_parameter_real, s_parameter_imag, "", ""},
		{0.1, 0.36, 0, 0},
		HT_BARNS, HF_BARNSJ3, OKJB | BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		1, NOFRACTAL, BARNSLEYM3, BARNSLEYJ3FP, NOSYM,
		barnsley3_orbit, julia_per_pixel_l, julia_setup_l, standard_fractal,
		BAILOUT_STANDARD
	},

	{ /* DEMM, TRIGSQR */
	s_fn_zz__name + 1,
		{"", "", "", ""},
		{0, 0, 0, 0},
		HT_SCOTSKIN, HF_FNZTIMESZ, TRIG1 | BAILTEST,
		-4.0f, 4.0f, -3.0f, 3.0f,
		16, NOFRACTAL, NOFRACTAL, TRIGSQRFP, XYAXIS,
		trig_z_squared_orbit, julia_per_pixel, julia_setup_l, standard_fractal,
		BAILOUT_STANDARD
	},

	{ /* DEMJ, TRIGSQRFP */
	s_fn_zz__name,
		{"", "", "", ""},
		{0, 0, 0, 0},
		HT_SCOTSKIN, HF_FNZTIMESZ, TRIG1 | BAILTEST,
		-4.0f, 4.0f, -3.0f, 3.0f,
		0, NOFRACTAL, NOFRACTAL, TRIGSQR, XYAXIS,
		trig_z_squared_orbit_fp, julia_per_pixel_fp, julia_setup_fp, standard_fractal,
		BAILOUT_STANDARD
	},

	{ /* BIFURCATION */
	s_bifurcation_name,
		{s_filter_cycles, s_seed_population, "", ""},
		{1000.0, 0.66, 0, 0},
		HT_BIF, HF_BIFURCATION, TRIG1 | NOGUESS | NOTRACE | NOROTATE,
		1.9f, 3.0f, 0.0f, 1.34f,
		0, NOFRACTAL, NOFRACTAL, LBIFURCATION, NOSYM,
		bifurcation_verhulst_trig_fp, NULL, stand_alone_setup, bifurcation,
		BAILOUT_NONE
	},

	{ /* MANDELSINH, TRIGPLUSTRIGFP */
	s_fnplusfn_name,
		{s_trig1_coefficient_re, s_trig1_coefficient_im, s_trig2_coefficient_re, s_trig2_coefficient_im},
		{1, 0, 1, 0},
		HT_SCOTSKIN, HF_FNPLUSFN, TRIG2 | BAILTEST,
		-4.0f, 4.0f, -3.0f, 3.0f,
		0, NOFRACTAL, NOFRACTAL, TRIGPLUSTRIG, XAXIS,
		trig_plus_trig_orbit_fp, other_julia_per_pixel_fp, trig_plus_trig_setup_fp,
			standard_fractal,
		BAILOUT_TRIG_L
	},

	{ /* LAMBDASINH, TRIGXTRIG */
	s_fnfn_name + 1,
		{"", "", "", ""},
		{0, 0, 0, 0},
		HT_SCOTSKIN, HF_FNTIMESFN, TRIG2 | BAILTEST,
		-4.0f, 4.0f, -3.0f, 3.0f,
		16, NOFRACTAL, NOFRACTAL, TRIGXTRIGFP, XAXIS,
		trig_trig_orbit, julia_per_pixel_l, fn_fn_setup, standard_fractal,
		BAILOUT_TRIG_L
	},

	{ /* MANDELCOSH, TRIGXTRIGFP */
	s_fnfn_name,
		{"", "", "", ""},
		{0, 0, 0, 0},
		HT_SCOTSKIN, HF_FNTIMESFN, TRIG2 | BAILTEST,
		-4.0f, 4.0f, -3.0f, 3.0f,
		0, NOFRACTAL, NOFRACTAL, TRIGXTRIG, XAXIS,
		trig_trig_orbit_fp, other_julia_per_pixel_fp, fn_fn_setup, standard_fractal,
		BAILOUT_TRIG_L
	},

	{ /* LAMBDACOSH, SQR1OVERTRIG */
	s_sqr_1divfn__name + 1,
		{"", "", "", ""},
		{0, 0, 0, 0},
		HT_SCOTSKIN, HF_SQROVFN, TRIG1 | BAILTEST,
		-4.0f, 4.0f, -3.0f, 3.0f,
		16, NOFRACTAL, NOFRACTAL, SQR1OVERTRIGFP, NOSYM,
		sqr_1_over_trig_z_orbit, julia_per_pixel_l, sqr_trig_setup,
			standard_fractal,
		BAILOUT_TRIG_L
	},

	{ /* LMANDELSINE, SQR1OVERTRIGFP */
	s_sqr_1divfn__name,
		{"", "", "", ""},
		{0, 0, 0, 0},
		HT_SCOTSKIN, HF_SQROVFN, TRIG1 | BAILTEST,
		-4.0f, 4.0f, -3.0f, 3.0f,
		0, NOFRACTAL, NOFRACTAL, SQR1OVERTRIG, NOSYM,
		sqr_1_over_trig_z_orbit_fp, other_julia_per_pixel_fp, sqr_trig_setup,
			standard_fractal,
		BAILOUT_TRIG_L
	},

	{ /* LLAMBDASINE, ZXTRIGPLUSZ */
	s_fnzplusz_name + 1,
		{s_trig1_coefficient_re, s_trig1_coefficient_im, "Real Coefficient Second Term", "Imag Coefficient Second Term"},
		{1, 0, 1, 0},
		HT_SCOTSKIN, HF_FNXZPLUSZ, TRIG1 | BAILTEST,
		-4.0f, 4.0f, -3.0f, 3.0f,
		1, NOFRACTAL, NOFRACTAL, ZXTRIGPLUSZFP, XAXIS,
		z_trig_z_plus_z_orbit, julia_per_pixel, z_trig_plus_z_setup, standard_fractal,
		BAILOUT_TRIG_L
	},

	{ /* LMANDELCOS, ZXTRIGPLUSZFP */
	s_fnzplusz_name,
		{s_trig1_coefficient_re, s_trig1_coefficient_im, "Real Coefficient Second Term", "Imag Coefficient Second Term"},
		{1, 0, 1, 0},
		HT_SCOTSKIN, HF_FNXZPLUSZ, TRIG1 | BAILTEST,
		-4.0f, 4.0f, -3.0f, 3.0f,
		0, NOFRACTAL, NOFRACTAL, ZXTRIGPLUSZ, XAXIS,
		z_trig_z_plus_z_orbit_fp, julia_per_pixel_fp, z_trig_plus_z_setup,
			standard_fractal,
		BAILOUT_TRIG_L
	},

	{ /* LLAMBDACOS, KAMFP */
	s_kamtorus_name,
		{s_kam_angle, s_kam_step, s_kam_stop, s_points_per_orbit},
		{1.3, .05, 1.5, 150},
		HT_KAM, HF_KAM, NOGUESS | NOTRACE,
		-1.0f, 1.0f, -.75f, .75f,
		0, NOFRACTAL, NOFRACTAL, KAM, NOSYM,
		(VF) kam_torus_orbit_fp, NULL, orbit_3d_setup_fp, orbit_2d_fp,
		BAILOUT_NONE
	},

	{ /* LMANDELSINH, KAM */
	s_kamtorus_name + 1,
		{s_kam_angle, s_kam_step, s_kam_stop, s_points_per_orbit},
		{1.3, .05, 1.5, 150},
		HT_KAM, HF_KAM, NOGUESS | NOTRACE,
		-1.0f, 1.0f, -.75f, .75f,
		16, NOFRACTAL, NOFRACTAL, KAMFP, NOSYM,
		(VF) kam_torus_orbit, NULL, orbit_3d_setup, orbit_2d,
		BAILOUT_NONE
	},

	{ /* LLAMBDASINH, KAM3DFP */
	s_kamtorus3d_name,
		{s_kam_angle, s_kam_step, s_kam_stop, s_points_per_orbit},
		{1.3, .05, 1.5, 150},
		HT_KAM, HF_KAM, NOGUESS | NOTRACE | NORESUME | PARMS3D,
		-3.0f, 3.0f, -1.0f, 3.5f,
		0, NOFRACTAL, NOFRACTAL, KAM3D, NOSYM,
		(VF) kam_torus_orbit_fp, NULL, orbit_3d_setup_fp, orbit_3d_fp,
		BAILOUT_NONE
	},

	{ /* LMANDELCOSH, KAM3D */
	s_kamtorus3d_name + 1,
		{s_kam_angle, s_kam_step, s_kam_stop, s_points_per_orbit},
		{1.3, .05, 1.5, 150},
		HT_KAM, HF_KAM, NOGUESS | NOTRACE | NORESUME | PARMS3D,
		-3.0f, 3.0f, -1.0f, 3.5f,
		16, NOFRACTAL, NOFRACTAL, KAM3DFP, NOSYM,
		(VF) kam_torus_orbit, NULL, orbit_3d_setup, orbit_3d,
		BAILOUT_NONE
	},

	{ /* LLAMBDACOSH, LAMBDATRIG */
	s_lambdafn_name + 1,
		{s_parameter_real, s_parameter_imag, "", ""},
		{1.0, 0.4, 0, 0},
		HT_LAMBDAFN, HF_LAMBDAFN, TRIG1 | OKJB,
		-4.0f, 4.0f, -3.0f, 3.0f,
		16, NOFRACTAL, MANDELTRIG, LAMBDATRIGFP, PI_SYM,
		(VF) lambda_trig_orbit, julia_per_pixel_l, lambda_trig_setup,
			standard_fractal,
		BAILOUT_TRIG_L
	},

	{ /* LMANTRIGPLUSZSQRD */
	s_manfnpluszsqrd_name + 1,
		{s_real_z0, s_imag_z0, "", ""},
		{0, 0, 0, 0},
		HT_PICKMJ, HF_MANDFNPLUSZSQRD, TRIG1 | BAILTEST,
		-2.5f, 1.5f, -1.5f, 1.5f,
		16, LJULTRIGPLUSZSQRD, NOFRACTAL, FPMANTRIGPLUSZSQRD, XAXIS_NOPARM,
		trig_plus_z_squared_orbit, mandelbrot_per_pixel, mandelbrot_setup_l,
			standard_fractal,
		BAILOUT_STANDARD
	},

	{ /* LJULTRIGPLUSZSQRD */
	s_julfnpluszsqrd_name + 1,
		{s_parameter_real, s_parameter_imag, "", ""},
		{-0.5, 0.5, 0, 0},
		HT_PICKMJ, HF_JULFNPLUSZSQRD, TRIG1 | OKJB | BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		16, NOFRACTAL, LMANTRIGPLUSZSQRD, FPJULTRIGPLUSZSQRD, NOSYM,
		trig_plus_z_squared_orbit, julia_per_pixel, julia_fn_plus_z_squared_setup,
			standard_fractal,
		BAILOUT_STANDARD
	},

	{ /* FPMANTRIGPLUSZSQRD */
	s_manfnpluszsqrd_name,
		{s_real_z0, s_imag_z0, "", ""},
		{0, 0, 0, 0},
		HT_PICKMJ, HF_MANDFNPLUSZSQRD, TRIG1 | BAILTEST,
		-2.5f, 1.5f, -1.5f, 1.5f,
		0, FPJULTRIGPLUSZSQRD,   NOFRACTAL, LMANTRIGPLUSZSQRD, XAXIS_NOPARM,
		trig_plus_z_squared_orbit_fp, mandelbrot_per_pixel_fp, mandelbrot_setup_fp,
			standard_fractal,
		BAILOUT_STANDARD
	},

	{ /* FPJULTRIGPLUSZSQRD */
	s_julfnpluszsqrd_name,
		{s_parameter_real, s_parameter_imag, "", ""},
		{-0.5, 0.5, 0, 0},
		HT_PICKMJ, HF_JULFNPLUSZSQRD, TRIG1 | OKJB | BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, FPMANTRIGPLUSZSQRD, LJULTRIGPLUSZSQRD, NOSYM,
		trig_plus_z_squared_orbit_fp, julia_per_pixel_fp, julia_fn_plus_z_squared_setup,
			standard_fractal,
		BAILOUT_STANDARD
	},

	{ /* LMANDELEXP, LAMBDATRIGFP */
	s_lambdafn_name,
		{s_parameter_real, s_parameter_imag, "", ""},
		{1.0, 0.4, 0, 0},
		HT_LAMBDAFN, HF_LAMBDAFN, TRIG1 | OKJB,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, MANDELTRIGFP, LAMBDATRIG, PI_SYM,
		lambda_trig_orbit_fp, other_julia_per_pixel_fp, lambda_trig_setup,
			standard_fractal,
		BAILOUT_TRIG_L
	},

	{ /* LLAMBDAEXP, MANDELTRIG */
	s_mandelfn_name + 1,
		{s_real_z0, s_imag_z0, "", ""},
		{0, 0, 0, 0},
		HT_MANDFN, HF_MANDFN, TRIG1,
		-8.0f, 8.0f, -6.0f, 6.0f,
		16, LAMBDATRIG, NOFRACTAL, MANDELTRIGFP, XYAXIS_NOPARM,
		lambda_trig_orbit, mandelbrot_per_pixel_l, mandelbrot_trig_setup,
			standard_fractal,
		BAILOUT_TRIG_L
	},

	{ /* LMANDELZPOWER */
	s_manzpower_name + 1,
		{s_real_z0, s_imag_z0, s_exponent, s_exponent_imag},
		{0, 0, 2, 0},
		HT_PICKMJ, HF_MANZPOWER, BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		1, LJULIAZPOWER, NOFRACTAL, FPMANDELZPOWER, XAXIS_NOIMAG,
		z_power_orbit, mandelbrot_per_pixel_l, mandelbrot_setup_l,
			standard_fractal,
		BAILOUT_STANDARD
	},

	{ /* LJULIAZPOWER */
	s_julzpower_name + 1,
		{s_parameter_real, s_parameter_imag, s_exponent, s_exponent_imag},
		{0.3, 0.6, 2, 0},
		HT_PICKMJ, HF_JULZPOWER, OKJB | BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		1, NOFRACTAL, LMANDELZPOWER, FPJULIAZPOWER, ORIGIN,
		z_power_orbit, julia_per_pixel_l, julia_setup_l,
			standard_fractal,
		BAILOUT_STANDARD
	},

	{ /* FPMANDELZPOWER */
	s_manzpower_name,
		{s_real_z0, s_imag_z0, s_exponent, s_exponent_imag},
		{0, 0, 2, 0},
		HT_PICKMJ, HF_MANZPOWER, BAILTEST | BF_MATH,
		-2.5f, 1.5f, -1.5f, 1.5f,
		0, FPJULIAZPOWER, NOFRACTAL, LMANDELZPOWER, XAXIS_NOIMAG,
		z_power_orbit_fp, other_mandelbrot_per_pixel_fp, mandelbrot_setup_fp,
			standard_fractal,
		BAILOUT_STANDARD
	},

	{ /* FPJULIAZPOWER */
	s_julzpower_name,
		{s_parameter_real, s_parameter_imag, s_exponent, s_exponent_imag},
		{0.3, 0.6, 2, 0},
		HT_PICKMJ, HF_JULZPOWER, OKJB | BAILTEST | BF_MATH,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, FPMANDELZPOWER, LJULIAZPOWER, ORIGIN,
		z_power_orbit_fp, other_julia_per_pixel_fp, julia_setup_fp,
			standard_fractal,
		BAILOUT_STANDARD
	},

	{ /* FPMANZTOZPLUSZPWR */
	"manzzpwr",
		{s_real_z0, s_imag_z0, s_exponent, ""},
		{0, 0, 2, 0},
		HT_PICKMJ, HF_MANZZPWR, BAILTEST,
		-2.5f, 1.5f, -1.5f, 1.5f,
		0, FPJULZTOZPLUSZPWR, NOFRACTAL, NOFRACTAL, XAXIS_NOPARM,
		z_to_z_plus_z_orbit_fp, other_mandelbrot_per_pixel_fp, mandelbrot_setup_fp,
			standard_fractal,
		BAILOUT_STANDARD
	},

	{ /* FPJULZTOZPLUSZPWR */
	"julzzpwr",
		{s_parameter_real, s_parameter_imag, s_exponent, ""},
		{-0.3, 0.3, 2, 0},
		HT_PICKMJ, HF_JULZZPWR, OKJB | BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, FPMANZTOZPLUSZPWR, NOFRACTAL, NOSYM,
		z_to_z_plus_z_orbit_fp, other_julia_per_pixel_fp, julia_setup_fp,
			standard_fractal,
		BAILOUT_STANDARD
	},

	{ /* LMANTRIGPLUSEXP */
	s_manfnplusexp_name + 1,
		{s_real_z0, s_imag_z0, "", ""},
		{0, 0, 0, 0},
		HT_PICKMJ, HF_MANDFNPLUSEXP, TRIG1 | BAILTEST,
		-8.0f, 8.0f, -6.0f, 6.0f,
		16, LJULTRIGPLUSEXP, NOFRACTAL, FPMANTRIGPLUSEXP, XAXIS_NOPARM,
		trig_plus_exponent_orbit, mandelbrot_per_pixel_l, mandelbrot_setup_l,
			standard_fractal,
		BAILOUT_STANDARD
	},

	{ /* LJULTRIGPLUSEXP */
	s_julfnplusexp_name + 1,
		{s_parameter_real, s_parameter_imag, "", ""},
		{0, 0, 0, 0},
		HT_PICKMJ, HF_JULFNPLUSEXP, TRIG1 | OKJB | BAILTEST,
		-4.0f, 4.0f, -3.0f, 3.0f,
		16, NOFRACTAL, LMANTRIGPLUSEXP, FPJULTRIGPLUSEXP, NOSYM,
		trig_plus_exponent_orbit, julia_per_pixel_l, julia_setup_l,
			standard_fractal,
		BAILOUT_STANDARD
	},

	{ /* FPMANTRIGPLUSEXP */
	s_manfnplusexp_name,
		{s_real_z0, s_imag_z0, "", ""},
		{0, 0, 0, 0},
		HT_PICKMJ, HF_MANDFNPLUSEXP, TRIG1 | BAILTEST,
		-8.0f, 8.0f, -6.0f, 6.0f,
		0, FPJULTRIGPLUSEXP, NOFRACTAL, LMANTRIGPLUSEXP, XAXIS_NOPARM,
		trig_plus_exponent_orbit_fp, other_mandelbrot_per_pixel_fp, mandelbrot_setup_fp,
 			standard_fractal,
		BAILOUT_STANDARD
	},

	{ /* FPJULTRIGPLUSEXP */
	s_julfnplusexp_name,
		{s_parameter_real, s_parameter_imag, "", ""},
		{0, 0, 0, 0},
		HT_PICKMJ, HF_JULFNPLUSEXP, TRIG1 | OKJB | BAILTEST,
		-4.0f, 4.0f, -3.0f, 3.0f,
		0, NOFRACTAL, FPMANTRIGPLUSEXP, LJULTRIGPLUSEXP, NOSYM,
		trig_plus_exponent_orbit_fp, other_julia_per_pixel_fp, julia_setup_fp,
			standard_fractal,
		BAILOUT_STANDARD
	},

	{ /* FPPOPCORN */
	s_popcorn_name,
		{s_step_x, s_step_y, s_constant_x, s_constant_y},
		{0.05, 0, 3.00, 0},
		HT_POPCORN, HF_POPCORN, NOGUESS | NOTRACE | TRIG4,
		-3.0f, 3.0f, -2.25f, 2.25f,
		0, NOFRACTAL, NOFRACTAL, LPOPCORN, NOPLOT,
		popcorn_fn_orbit_fp, other_julia_per_pixel_fp, julia_setup_fp, popcorn,
		BAILOUT_STANDARD
	},

	{ /* LPOPCORN */
	s_popcorn_name + 1,
		{s_step_x, s_step_y, s_constant_x, s_constant_y},
		{0.05, 0, 3.00, 0},
		HT_POPCORN, HF_POPCORN, NOGUESS | NOTRACE | TRIG4,
		-3.0f, 3.0f, -2.25f, 2.25f,
		16, NOFRACTAL, NOFRACTAL, FPPOPCORN, NOPLOT,
		popcorn_fn_orbit, julia_per_pixel_l, julia_setup_l, popcorn,
		BAILOUT_STANDARD
	},

	{ /* FPLORENZ */
	s_lorenz_name,
		{s_time_step, "a", "b", "c"},
		{.02, 5, 15, 1},
		HT_LORENZ, HF_LORENZ, NOGUESS | NOTRACE | INFCALC,
		-15.0f, 15.0f, 0.0f, 30.0f,
		0, NOFRACTAL, NOFRACTAL, LLORENZ, NOSYM,
		(VF) lorenz_3d_orbit_fp, NULL, orbit_3d_setup_fp, orbit_2d_fp,
		BAILOUT_NONE
	},

	{ /* LLORENZ */
	s_lorenz_name + 1,
		{s_time_step, "a", "b", "c"},
		{.02, 5, 15, 1},
		HT_LORENZ, HF_LORENZ, NOGUESS | NOTRACE | INFCALC,
		-15.0f, 15.0f, 0.0f, 30.0f,
		16, NOFRACTAL, NOFRACTAL, FPLORENZ, NOSYM,
		(VF) lorenz_3d_orbit, NULL, orbit_3d_setup, orbit_2d,
		BAILOUT_NONE
	},

	{ /* LLORENZ3D */
	s_lorenz3d_name + 1,
		{s_time_step, "a", "b", "c"},
		{.02, 5, 15, 1},
		HT_LORENZ, HF_LORENZ, NOGUESS | NOTRACE | NORESUME | PARMS3D | INFCALC,
		-30.0f, 30.0f, -30.0f, 30.0f,
		16, NOFRACTAL, NOFRACTAL, FPLORENZ3D, NOSYM,
		(VF) lorenz_3d_orbit, NULL, orbit_3d_setup, orbit_3d,
		BAILOUT_NONE
	},

	{ /* MPNEWTON */
	s_newton_name + 1,
		{s_newton_degree, "", "", ""},
		{3, 0, 0, 0},
		HT_NEWT, HF_NEWT, 0,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, NOFRACTAL, NEWTON, XAXIS,
		newton_orbit_mpc, julia_per_pixel_mpc, newton_setup, standard_fractal,
		BAILOUT_NONE
	},

	{ /* MPNEWTBASIN */
	s_newtbasin_name + 1,
		{s_newton_degree, "Enter non-zero value for stripes", "", ""},
		{3, 0, 0, 0},
		HT_NEWTBAS, HF_NEWTBAS, 0,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, NOFRACTAL, NEWTBASIN, NOSYM,
		newton_orbit_mpc, julia_per_pixel_mpc, newton_setup, standard_fractal,
		BAILOUT_NONE
	},

	{ /* COMPLEXNEWTON */
	"complexnewton",
		{"Real part of Degree", "Imag part of Degree", "Real part of Root", "Imag part of Root"},
		{3, 0, 1, 0},
		HT_NEWTCMPLX, HF_COMPLEXNEWT, 0,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
		complex_newton, other_julia_per_pixel_fp, complex_newton_setup,
			standard_fractal,
		BAILOUT_NONE
	},

	{ /* COMPLEXBASIN */
	"complexbasin",
		{"Real part of Degree", "Imag part of Degree", "Real part of Root", "Imag part of Root"},
		{3, 0, 1, 0},
		HT_NEWTCMPLX, HF_COMPLEXNEWT, 0,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
		complex_basin, other_julia_per_pixel_fp, complex_newton_setup,
			standard_fractal,
		BAILOUT_NONE
	},

	{ /* COMPLEXMARKSMAND */
	"cmplxmarksmand",
		{s_real_z0, s_imag_z0, s_exponent, s_exponent_imag},
		{0, 0, 1, 0},
		HT_MARKS, HF_CMPLXMARKSMAND, BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, COMPLEXMARKSJUL, NOFRACTAL, NOFRACTAL, NOSYM,
		marks_complex_mandelbrot_orbit, marks_complex_mandelbrot_per_pixel, mandelbrot_setup_fp, standard_fractal,
		BAILOUT_STANDARD
	},

	{ /* COMPLEXMARKSJUL */
	"cmplxmarksjul",
		{s_parameter_real, s_parameter_imag, s_exponent, s_exponent_imag},
		{0.3, 0.6, 1, 0},
		HT_MARKS, HF_CMPLXMARKSJUL, BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, COMPLEXMARKSMAND, NOFRACTAL, NOSYM,
		marks_complex_mandelbrot_orbit, julia_per_pixel_fp, julia_setup_fp, standard_fractal,
		BAILOUT_STANDARD
	},

	{ /* FORMULA */
	s_formula_name + 1,
		{s_p1_real, s_p1_imag, s_p2_real, s_p2_imag},
		{0, 0, 0, 0},
		HT_FORMULA, -2, MORE,
		-2.0f, 2.0f, -1.5f, 1.5f,
		1, NOFRACTAL, NOFRACTAL, FFORMULA, SETUP_SYM,
		Formula, form_per_pixel, formula_setup_int, standard_fractal,
		0
	},

	{ /* FFORMULA */
	s_formula_name,
		{s_p1_real, s_p1_imag, s_p2_real, s_p2_imag},
		{0, 0, 0, 0},
		HT_FORMULA, -2, MORE,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, NOFRACTAL, FORMULA, SETUP_SYM,
		Formula, form_per_pixel, formula_setup_fp, standard_fractal,
		0
	},

	{ /* SIERPINSKIFP */
	s_sierpinski_name,
		{"", "", "", ""},
		{0, 0, 0, 0},
		HT_SIER, HF_SIER, 0,
		-4.0f/3.0f, 96.0f/45.0f, -0.9f, 1.7f,
		0, NOFRACTAL, NOFRACTAL, SIERPINSKI, NOSYM,
		sierpinski_orbit_fp, other_julia_per_pixel_fp, sierpinski_setup_fp,
			standard_fractal,
		127
	},

	{ /* LAMBDAFP */
	s_lambda_name,
		{s_parameter_real, s_parameter_imag, "", ""},
		{0.85, 0.6, 0, 0},
		HT_LAMBDA, HF_LAMBDA, OKJB | BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, MANDELLAMBDAFP, LAMBDA, NOSYM,
		lambda_orbit_fp, julia_per_pixel_fp, julia_setup_fp, standard_fractal,
		BAILOUT_STANDARD
	},

	{ /* BARNSLEYM1FP */
	s_barnsleym1_name,
		{s_real_z0, s_imag_z0, "", ""},
		{0, 0, 0, 0},
		HT_BARNS, HF_BARNSM1, BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, BARNSLEYJ1FP, NOFRACTAL, BARNSLEYM1, XYAXIS_NOPARM,
		barnsley1_orbit_fp, other_mandelbrot_per_pixel_fp, mandelbrot_setup_fp,
			standard_fractal,
		BAILOUT_STANDARD
	},

	{ /* BARNSLEYJ1FP */
	s_barnsleyj1_name,
		{s_parameter_real, s_parameter_imag, "", ""},
		{0.6, 1.1, 0, 0},
		HT_BARNS, HF_BARNSJ1, OKJB | BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, BARNSLEYM1FP, BARNSLEYJ1, ORIGIN,
		barnsley1_orbit_fp, other_julia_per_pixel_fp, julia_setup_fp,
			standard_fractal,
		BAILOUT_STANDARD
	},

	{ /* BARNSLEYM2FP */
	s_barnsleym2_name,
		{s_real_z0, s_imag_z0, "", ""},
		{0, 0, 0, 0},
		HT_BARNS, HF_BARNSM2, BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, BARNSLEYJ2FP, NOFRACTAL, BARNSLEYM2, YAXIS_NOPARM,
		barnsley2_orbit_fp, other_mandelbrot_per_pixel_fp, mandelbrot_setup_fp,
			standard_fractal,
		BAILOUT_STANDARD
	},

	{ /* BARNSLEYJ2FP */
	s_barnsleyj2_name,
		{s_parameter_real, s_parameter_imag, "", ""},
		{0.6, 1.1, 0, 0},
		HT_BARNS, HF_BARNSJ2, OKJB | BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, BARNSLEYM2FP, BARNSLEYJ2, ORIGIN,
		barnsley2_orbit_fp, other_julia_per_pixel_fp, julia_setup_fp,
			standard_fractal,
		BAILOUT_STANDARD
	},

	{ /* BARNSLEYM3FP */
	s_barnsleym3_name,
		{s_real_z0, s_imag_z0, "", ""},
		{0, 0, 0, 0},
		HT_BARNS, HF_BARNSM3, BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, BARNSLEYJ3FP, NOFRACTAL, BARNSLEYM3, XAXIS_NOPARM,
		barnsley3_orbit_fp, other_mandelbrot_per_pixel_fp, mandelbrot_setup_fp,
			standard_fractal,
		BAILOUT_STANDARD
	},

	{ /* BARNSLEYJ3FP */
	s_barnsleyj3_name,
		{s_parameter_real, s_parameter_imag, "", ""},
		{0.6, 1.1, 0, 0},
		HT_BARNS, HF_BARNSJ3, OKJB | BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, BARNSLEYM3FP, BARNSLEYJ3, NOSYM,
		barnsley3_orbit_fp, other_julia_per_pixel_fp, julia_setup_fp,
			standard_fractal,
		BAILOUT_STANDARD
	},

	{ /* MANDELLAMBDAFP */
	s_mandellambda_name,
		{s_real_z0, s_imag_z0, "", ""},
		{0, 0, 0, 0},
		HT_MLAMBDA, HF_MLAMBDA, BAILTEST,
		-3.0f, 5.0f, -3.0f, 3.0f,
		0, LAMBDAFP, NOFRACTAL, MANDELLAMBDA, XAXIS_NOPARM,
		lambda_orbit_fp, mandelbrot_per_pixel_fp, mandelbrot_setup_fp, standard_fractal,
		BAILOUT_STANDARD
	},

	{ /* JULIBROT */
	s_julibrot_name + 1,
		{"", "", "", ""},
		{0, 0, 0, 0},
		HT_JULIBROT, -1, NOGUESS | NOTRACE | NOROTATE | NORESUME,
		-2.0f, 2.0f, -1.5f, 1.5f,
		1, NOFRACTAL, NOFRACTAL, JULIBROTFP, NOSYM,
		julia_orbit, julibrot_per_pixel, julibrot_setup, standard_4d_fractal,
		BAILOUT_STANDARD
	},

	{ /* FPLORENZ3D */
	s_lorenz3d_name,
		{s_time_step, "a", "b", "c"},
		{.02, 5, 15, 1},
		HT_LORENZ, HF_LORENZ, NOGUESS | NOTRACE | NORESUME | PARMS3D | INFCALC,
		-30.0f, 30.0f, -30.0f, 30.0f,
		0, NOFRACTAL, NOFRACTAL, LLORENZ3D, NOSYM,
		(VF) lorenz_3d_orbit_fp, NULL, orbit_3d_setup_fp, orbit_3d_fp,
		BAILOUT_NONE
	},

	{ /* LROSSLER */
	s_rossler3d_name + 1,
		{s_time_step, "a", "b", "c"},
		{.04, .2, .2, 5.7},
		HT_ROSS, HF_ROSS, NOGUESS | NOTRACE | NORESUME | PARMS3D | INFCALC,
		-30.0f, 30.0f, -20.0f, 40.0f,
		16, NOFRACTAL, NOFRACTAL, FPROSSLER, NOSYM,
		(VF) rossler_orbit, NULL, orbit_3d_setup, orbit_3d,
		BAILOUT_NONE
	},

	{ /* FPROSSLER */
	s_rossler3d_name,
		{s_time_step, "a", "b", "c"},
		{.04, .2, .2, 5.7},
		HT_ROSS, HF_ROSS, NOGUESS | NOTRACE | NORESUME | PARMS3D | INFCALC,
		-30.0f, 30.0f, -20.0f, 40.0f,
		0, NOFRACTAL, NOFRACTAL, LROSSLER, NOSYM,
		(VF) rossler_orbit_fp, NULL, orbit_3d_setup_fp, orbit_3d_fp,
		BAILOUT_NONE
	},

	{ /* LHENON */
	s_henon_name + 1,
		{"a", "b", "", ""},
		{1.4, .3, 0, 0},
		HT_HENON, HF_HENON, NOGUESS | NOTRACE | INFCALC,
		-1.4f, 1.4f, -.5f, .5f,
		16, NOFRACTAL, NOFRACTAL, FPHENON, NOSYM,
		(VF) henon_orbit, NULL, orbit_3d_setup, orbit_2d,
		BAILOUT_NONE
	},

	{ /* FPHENON */
	s_henon_name,
		{"a", "b", "", ""},
		{1.4, .3, 0, 0},
		HT_HENON, HF_HENON, NOGUESS | NOTRACE | INFCALC,
		-1.4f, 1.4f, -.5f, .5f,
		0, NOFRACTAL, NOFRACTAL, LHENON, NOSYM,
		(VF) henon_orbit_fp, NULL, orbit_3d_setup_fp, orbit_2d_fp,
		BAILOUT_NONE
	},

	{ /* FPPICKOVER */
	"pickover",
		{"a", "b", "c", "d"},
		{2.24, .43, -.65, -2.43},
		HT_PICK, HF_PICKOVER, NOGUESS | NOTRACE | NORESUME | PARMS3D,
		-8.0f/3.0f, 8.0f/3.0f, -2.0f, 2.0f,
		0, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
		(VF) pickover_orbit_fp, NULL, orbit_3d_setup_fp, orbit_3d_fp,
		BAILOUT_NONE
	},

	{ /* FPGINGERBREAD */
	"gingerbreadman",
		{"Initial x", "Initial y", "", ""},
		{-.1, 0, 0, 0},
		HT_GINGER, HF_GINGER, NOGUESS | NOTRACE | INFCALC,
		-4.5f, 8.5f, -4.5f, 8.5f,
		0, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
		(VF) gingerbread_orbit_fp, NULL, orbit_3d_setup_fp, orbit_2d_fp,
		BAILOUT_NONE
	},

	{ /* DIFFUSION */
	"diffusion",
		{"+Border size",
		"+Type (0=Central, 1=Falling, 2=Square Cavity)",
		"+Color change rate (0=Random)",
		""
		},
		{10, 0, 0, 0},
		HT_DIFFUS, HF_DIFFUS, NOZOOM | NOGUESS | NOTRACE,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
		NULL, NULL, stand_alone_setup, diffusion,
		BAILOUT_NONE
	},

	{ /* UNITYFP */
	s_unity_name,
		{"", "", "", ""},
		{0, 0, 0, 0},
		HT_UNITY, HF_UNITY, 0,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, NOFRACTAL, UNITY, XYAXIS,
		unity_orbit_fp, other_julia_per_pixel_fp, standard_setup, standard_fractal,
		BAILOUT_NONE
	},

	{ /* SPIDERFP */
	s_spider_name,
		{s_real_z0, s_imag_z0, "", ""},
		{0, 0, 0, 0},
		HT_SCOTSKIN, HF_SPIDER, BAILTEST,
		-2.5f, 1.5f, -1.5f, 1.5f,
		0, NOFRACTAL, NOFRACTAL, SPIDER, XAXIS_NOPARM,
		spider_orbit_fp, mandelbrot_per_pixel_fp, mandelbrot_setup_fp, standard_fractal,
		BAILOUT_STANDARD
	},

	{ /* SPIDER */
	s_spider_name + 1,
		{s_real_z0, s_imag_z0, "", ""},
		{0, 0, 0, 0},
		HT_SCOTSKIN, HF_SPIDER, BAILTEST,
		-2.5f, 1.5f, -1.5f, 1.5f,
		1, NOFRACTAL, NOFRACTAL, SPIDERFP, XAXIS_NOPARM,
		spider_orbit, mandelbrot_per_pixel, mandelbrot_setup_l, standard_fractal,
		BAILOUT_STANDARD
	},

	{ /* TETRATEFP */
	"tetrate",
		{s_real_z0, s_imag_z0, "", ""},
		{0, 0, 0, 0},
		HT_SCOTSKIN, HF_TETRATE, BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, NOFRACTAL, NOFRACTAL, XAXIS_NOIMAG,
		tetrate_orbit_fp, other_mandelbrot_per_pixel_fp, mandelbrot_setup_fp,
			standard_fractal,
		BAILOUT_STANDARD
	},

	{ /* MAGNET1M */
	"magnet1m",
		{s_real_z0, s_imag_z0, "", ""},
		{0, 0, 0, 0},
		HT_MAGNET, HF_MAGM1, 0,
		-4.0f, 4.0f, -3.0f, 3.0f,
		0, MAGNET1J, NOFRACTAL, NOFRACTAL, XAXIS_NOPARM,
		magnet1_orbit_fp, mandelbrot_per_pixel_fp, mandelbrot_setup_fp, standard_fractal,
		100
	},

	{ /* MAGNET1J */
	"magnet1j",
		{s_parameter_real, s_parameter_imag, "", ""},
		{0, 0, 0, 0},
		HT_MAGNET, HF_MAGJ1, 0,
		-8.0f, 8.0f, -6.0f, 6.0f,
		0, NOFRACTAL, MAGNET1M, NOFRACTAL, XAXIS_NOIMAG,
		magnet1_orbit_fp, julia_per_pixel_fp, julia_setup_fp, standard_fractal,
		100
	},

	{ /* MAGNET2M */
	"magnet2m",
		{s_real_z0, s_imag_z0, "", ""},
		{0, 0, 0, 0},
		HT_MAGNET, HF_MAGM2, 0,
		-1.5f, 3.7f, -1.95f, 1.95f,
		0, MAGNET2J, NOFRACTAL, NOFRACTAL, XAXIS_NOPARM,
		magnet2_orbit_fp, mandelbrot_per_pixel_fp, mandelbrot_setup_fp, standard_fractal,
		100
	},

	{ /* MAGNET2J */
	"magnet2j",
		{s_parameter_real, s_parameter_imag, "", ""},
		{0, 0, 0, 0},
		HT_MAGNET, HF_MAGJ2, 0,
		-8.0f, 8.0f, -6.0f, 6.0f,
		0, NOFRACTAL, MAGNET2M, NOFRACTAL, XAXIS_NOIMAG,
		magnet2_orbit_fp, julia_per_pixel_fp, julia_setup_fp, standard_fractal,
		100
	},

	{ /* LBIFURCATION */
	s_bifurcation_name + 1,
		{s_filter_cycles, s_seed_population, "", ""},
		{1000.0, 0.66, 0, 0},
		HT_BIF, HF_BIFURCATION, TRIG1 | NOGUESS | NOTRACE | NOROTATE,
		1.9f, 3.0f, 0.0f, 1.34f,
		1, NOFRACTAL, NOFRACTAL, BIFURCATION, NOSYM,
		bifurcation_verhulst_trig, NULL, stand_alone_setup, bifurcation,
		BAILOUT_NONE
	},

	{ /* LBIFLAMBDA */
	s_biflambda_name + 1,
		{s_filter_cycles, s_seed_population, "", ""},
		{1000.0, 0.66, 0, 0},
		HT_BIF, HF_BIFLAMBDA, TRIG1 | NOGUESS | NOTRACE | NOROTATE,
		-2.0f, 4.0f, -1.0f, 2.0f,
		1, NOFRACTAL, NOFRACTAL, BIFLAMBDA, NOSYM,
		bifurcation_lambda_trig, NULL, stand_alone_setup, bifurcation,
		BAILOUT_NONE
	},

	{ /* BIFLAMBDA */
	s_biflambda_name,
		{s_filter_cycles, s_seed_population, "", ""},
		{1000.0, 0.66, 0, 0},
		HT_BIF, HF_BIFLAMBDA, TRIG1 | NOGUESS | NOTRACE | NOROTATE,
		-2.0f, 4.0f, -1.0f, 2.0f,
		0, NOFRACTAL, NOFRACTAL, LBIFLAMBDA, NOSYM,
		bifurcation_lambda_trig_fp, NULL, stand_alone_setup, bifurcation,
		BAILOUT_NONE
	},

	{ /* BIFADSINPI */
	s_bifplussinpi_name,
		{s_filter_cycles, s_seed_population, "", ""},
		{1000.0, 0.66, 0, 0},
		HT_BIF, HF_BIFPLUSSINPI, TRIG1 | NOGUESS | NOTRACE | NOROTATE,
		0.275f, 1.45f, 0.0f, 2.0f,
		0, NOFRACTAL, NOFRACTAL, LBIFADSINPI, NOSYM,
		bifurcation_add_trig_pi_fp, NULL, stand_alone_setup, bifurcation,
		BAILOUT_NONE
	},

	{ /* BIFEQSINPI */
	s_bifeqsinpi_name,
		{s_filter_cycles, s_seed_population, "", ""},
		{1000.0, 0.66, 0, 0},
		HT_BIF, HF_BIFEQSINPI, TRIG1 | NOGUESS | NOTRACE | NOROTATE,
		-2.5f, 2.5f, -3.5f, 3.5f,
		0, NOFRACTAL, NOFRACTAL, LBIFEQSINPI, NOSYM,
		bifurcation_set_trig_pi_fp, NULL, stand_alone_setup, bifurcation,
		BAILOUT_NONE
	},

	{ /* FPPOPCORNJUL */
	s_popcornjul_name,
		{s_step_x, s_step_y, s_constant_x, s_constant_y},
		{0.05, 0, 3.00, 0},
		HT_POPCORN, HF_POPCJUL, TRIG4,
		-3.0f, 3.0f, -2.25f, 2.25f,
		0, NOFRACTAL, NOFRACTAL, LPOPCORNJUL, NOSYM,
		popcorn_fn_orbit_fp, other_julia_per_pixel_fp, julia_setup_fp, standard_fractal,
		BAILOUT_STANDARD
	},

	{ /* LPOPCORNJUL */
	s_popcornjul_name + 1,
		{s_step_x, s_step_y, s_constant_x, s_constant_y},
		{0.05, 0, 3.0, 0},
		HT_POPCORN, HF_POPCJUL, TRIG4,
		-3.0f, 3.0f, -2.25f, 2.25f,
		16, NOFRACTAL, NOFRACTAL, FPPOPCORNJUL, NOSYM,
		popcorn_fn_orbit, julia_per_pixel_l, julia_setup_l, standard_fractal,
		BAILOUT_STANDARD
	},

	{ /* LSYSTEM */
	"lsystem",
		{" + Order", "", "", ""},
		{2, 0, 0, 0},
		HT_LSYS, -3, NOZOOM | NORESUME | NOGUESS | NOTRACE,
		-1.0f, 1.0f, -1.0f, 1.0f,
		1, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
		NULL, NULL, stand_alone_setup, l_system,
		BAILOUT_NONE
	},

	{ /* MANOWARJFP */
	s_manowarj_name,
		{s_parameter_real, s_parameter_imag, "", ""},
		{0, 0, 0, 0},
		HT_SCOTSKIN, HF_MANOWARJ, OKJB | BAILTEST,
		-2.5f, 1.5f, -1.5f, 1.5f,
		0, NOFRACTAL, MANOWARFP, MANOWARJ, NOSYM,
		man_o_war_orbit_fp, julia_per_pixel_fp, julia_setup_fp, standard_fractal,
		BAILOUT_STANDARD
	},

	{ /* MANOWARJ */
	s_manowarj_name + 1,
		{s_parameter_real, s_parameter_imag, "", ""},
		{0, 0, 0, 0},
		HT_SCOTSKIN, HF_MANOWARJ, OKJB | BAILTEST,
		-2.5f, 1.5f, -1.5f, 1.5f,
		1, NOFRACTAL, MANOWAR, MANOWARJFP, NOSYM,
		man_o_war_orbit, julia_per_pixel, julia_setup_l, standard_fractal,
		BAILOUT_STANDARD
	},

	{ /* FNPLUSFNPIXFP */
	s_fn_z_plusfn_pix__name,
		{s_real_z0, s_imag_z0, s_trig2_coefficient_re, s_trig2_coefficient_im},
		{0, 0, 1, 0},
		HT_SCOTSKIN, HF_FNPLUSFNPIX, TRIG2 | BAILTEST,
		-3.6f, 3.6f, -2.7f, 2.7f,
		0, NOFRACTAL, NOFRACTAL, FNPLUSFNPIXLONG, NOSYM,
		richard8_orbit_fp, other_richard8_per_pixel_fp, mandelbrot_setup_fp,
			standard_fractal,
		BAILOUT_TRIG_L
	},

	{ /* FNPLUSFNPIXLONG */
	s_fn_z_plusfn_pix__name + 1,
		{s_real_z0, s_imag_z0, s_trig2_coefficient_re, s_trig2_coefficient_im},
		{0, 0, 1, 0},
		HT_SCOTSKIN, HF_FNPLUSFNPIX, TRIG2 | BAILTEST,
		-3.6f, 3.6f, -2.7f, 2.7f,
		1, NOFRACTAL, NOFRACTAL, FNPLUSFNPIXFP, NOSYM,
		richard8_orbit, richard8_per_pixel, mandelbrot_setup_l,
			standard_fractal,
		BAILOUT_TRIG_L
	},

	{ /* MARKSMANDELPWRFP */
	s_marksmandelpwr_name,
		{s_real_z0, s_imag_z0, "", ""},
		{0, 0, 0, 0},
		HT_MARKS, HF_MARKSMANDPWR, TRIG1 | BAILTEST,
		-2.5f, 1.5f, -1.5f, 1.5f,
		0, NOFRACTAL, NOFRACTAL, MARKSMANDELPWR, XAXIS_NOPARM,
		marks_mandel_power_orbit_fp, marks_mandelbrot_power_per_pixel_fp, mandelbrot_setup_fp,
 			standard_fractal,
		BAILOUT_STANDARD
	},

	{ /* MARKSMANDELPWR */
	s_marksmandelpwr_name + 1,
		{s_real_z0, s_imag_z0, "", ""},
		{0, 0, 0, 0},
		HT_MARKS, HF_MARKSMANDPWR, TRIG1 | BAILTEST,
		-2.5f, 1.5f, -1.5f, 1.5f,
		1, NOFRACTAL, NOFRACTAL, MARKSMANDELPWRFP, XAXIS_NOPARM,
		marks_mandel_power_orbit, marks_mandelbrot_power_per_pixel, mandelbrot_setup_l,
			standard_fractal,
		BAILOUT_STANDARD
	},

	{ /* TIMSERRORFP */
	s_tims_error_name,
		{s_real_z0, s_imag_z0, "", ""},
		{0, 0, 0, 0},
		HT_MARKS, HF_TIMSERR, TRIG1 | BAILTEST,
		-2.9f, 4.3f, -2.7f, 2.7f,
		0, NOFRACTAL, NOFRACTAL, TIMSERROR, XAXIS_NOPARM,
		tims_error_orbit_fp, marks_mandelbrot_power_per_pixel_fp, mandelbrot_setup_fp,
			standard_fractal,
		BAILOUT_STANDARD
	},

	{ /* TIMSERROR */
	s_tims_error_name + 1,
		{s_real_z0, s_imag_z0, "", ""},
		{0, 0, 0, 0},
		HT_MARKS, HF_TIMSERR, TRIG1 | BAILTEST,
		-2.9f, 4.3f, -2.7f, 2.7f,
		1, NOFRACTAL, NOFRACTAL, TIMSERRORFP, XAXIS_NOPARM,
		tims_error_orbit, marks_mandelbrot_power_per_pixel, mandelbrot_setup_l,
			standard_fractal,
		BAILOUT_STANDARD
	},

	{ /* LBIFEQSINPI */
	s_bifeqsinpi_name + 1,
		{s_filter_cycles, s_seed_population, "", ""},
		{1000.0, 0.66, 0, 0},
		HT_BIF, HF_BIFEQSINPI, TRIG1 | NOGUESS | NOTRACE | NOROTATE,
		-2.5f, 2.5f, -3.5f, 3.5f,
		1, NOFRACTAL, NOFRACTAL, BIFEQSINPI, NOSYM,
		bifurcation_set_trig_pi, NULL, stand_alone_setup, bifurcation,
		BAILOUT_NONE
	},

	{ /* LBIFADSINPI */
	s_bifplussinpi_name + 1,
		{s_filter_cycles, s_seed_population, "", ""},
		{1000.0, 0.66, 0, 0},
		HT_BIF, HF_BIFPLUSSINPI, TRIG1 | NOGUESS | NOTRACE | NOROTATE,
		0.275f, 1.45f, 0.0f, 2.0f,
		1, NOFRACTAL, NOFRACTAL, BIFADSINPI, NOSYM,
		bifurcation_add_trig_pi, NULL, stand_alone_setup, bifurcation,
		BAILOUT_NONE
	},

	{ /* BIFSTEWART */
	s_bifstewart_name,
		{s_filter_cycles, s_seed_population, "", ""},
		{1000.0, 0.66, 0, 0},
		HT_BIF, HF_BIFSTEWART, TRIG1 | NOGUESS | NOTRACE | NOROTATE,
		0.7f, 2.0f, -1.1f, 1.1f,
		0, NOFRACTAL, NOFRACTAL, LBIFSTEWART, NOSYM,
		bifurcation_stewart_trig_fp, NULL, stand_alone_setup, bifurcation,
		BAILOUT_NONE
	},

	{ /* LBIFSTEWART */
	s_bifstewart_name + 1,
		{s_filter_cycles, s_seed_population, "", ""},
		{1000.0, 0.66, 0, 0},
		HT_BIF, HF_BIFSTEWART, TRIG1 | NOGUESS | NOTRACE | NOROTATE,
		0.7f, 2.0f, -1.1f, 1.1f,
		1, NOFRACTAL, NOFRACTAL, BIFSTEWART, NOSYM,
		bifurcation_stewart_trig, NULL, stand_alone_setup, bifurcation,
		BAILOUT_NONE
	},

	{ /* FPHOPALONG */
	"hopalong",
		{"a", "b", "c", ""},
		{.4, 1, 0, 0},
		HT_MARTIN, HF_HOPALONG, NOGUESS | NOTRACE | INFCALC,
		-2.0f, 3.0f, -1.625f, 2.625f,
		0, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
		(VF) hopalong_2d_orbit_fp, NULL, orbit_3d_setup_fp, orbit_2d_fp,
		BAILOUT_NONE
	},

	{ /* FPCIRCLE */
	"circle",
		{"magnification", "", "", ""},
		{200000L, 0, 0, 0},
		HT_CIRCLE, HF_CIRCLE, 0,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, NOFRACTAL, NOFRACTAL, XYAXIS,
		circle_orbit_fp, julia_per_pixel_fp, julia_setup_fp, standard_fractal,
		BAILOUT_NONE
	},

	{ /* FPMARTIN */
	"martin",
		{"a", "", "", ""},
		{3.14, 0, 0, 0},
		HT_MARTIN, HF_MARTIN, NOGUESS | NOTRACE | INFCALC,
		-32.0f, 32.0f, -24.0f, 24.0f,
		0, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
		(VF) martin_2d_orbit_fp, NULL, orbit_3d_setup_fp, orbit_2d_fp,
		BAILOUT_NONE
	},

	{ /* LYAPUNOV */
	"lyapunov",
		{"+Order (integer)", "Population Seed", "+Filter Cycles", ""},
		{0, 0.5, 0, 0},
		HT_LYAPUNOV, HT_LYAPUNOV, 0,
		-8.0f, 8.0f, -6.0f, 6.0f,
		0, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
		bifurcation_lambda, NULL, lyapunov_setup, lyapunov,
		BAILOUT_NONE
	},

	{ /* FPLORENZ3D1 */
	"lorenz3d1",
		{s_time_step, "a", "b", "c"},
		{.02, 5, 15, 1},
		HT_LORENZ, HF_LORENZ3D1,
								NOGUESS | NOTRACE | NORESUME | PARMS3D | INFCALC,
		-30.0f, 30.0f, -30.0f, 30.0f,
		0, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
		(VF) lorenz_3d1_orbit_fp, NULL, orbit_3d_setup_fp, orbit_3d_fp,
		BAILOUT_NONE
	},

	{ /* FPLORENZ3D3 */
	"lorenz3d3",
		{s_time_step, "a", "b", "c"},
		{.02, 10, 28, 2.66},
		HT_LORENZ, HF_LORENZ3D3,
			NOGUESS | NOTRACE | NORESUME | PARMS3D | INFCALC,
		-30.0f, 30.0f, -30.0f, 30.0f,
		0, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
		(VF) lorenz_3d3_orbit_fp, NULL, orbit_3d_setup_fp, orbit_3d_fp,
		BAILOUT_NONE
	},

	{ /* FPLORENZ3D4 */
	"lorenz3d4",
		{s_time_step, "a", "b", "c"},
		{.02, 10, 28, 2.66},
		HT_LORENZ, HF_LORENZ3D4,
			NOGUESS | NOTRACE | NORESUME | PARMS3D | INFCALC,
		-30.0f, 30.0f, -30.0f, 30.0f,
		0, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
		(VF) lorenz_3d4_orbit_fp, NULL, orbit_3d_setup_fp, orbit_3d_fp,
		BAILOUT_NONE
	},

	{ /* LLAMBDAFNFN */
	s_lambda_fnorfn__name + 1,
		{s_parameter_real, s_parameter_imag, "Function Shift Value", ""},
		{1, 0.1, 1, 0},
		HT_FNORFN, HF_LAMBDAFNFN, TRIG2 | BAILTEST,
		-4.0f, 4.0f, -3.0f, 3.0f,
		16, NOFRACTAL, LMANLAMFNFN, FPLAMBDAFNFN, ORIGIN,
		lambda_trig_or_trig_orbit, julia_per_pixel_l, lambda_trig_or_trig_setup,
 			standard_fractal,
		BAILOUT_TRIG_L
	},

	{ /* FPLAMBDAFNFN */
	s_lambda_fnorfn__name,
		{s_parameter_real, s_parameter_imag, "Function Shift Value", ""},
		{1, 0.1, 1, 0},
		HT_FNORFN, HF_LAMBDAFNFN, TRIG2 | BAILTEST,
		-4.0f, 4.0f, -3.0f, 3.0f,
		0, NOFRACTAL, FPMANLAMFNFN, LLAMBDAFNFN, ORIGIN,
		lambda_trig_or_trig_orbit_fp, other_julia_per_pixel_fp,
			lambda_trig_or_trig_setup, standard_fractal,
		BAILOUT_TRIG_L
	},

	{ /* LJULFNFN */
	s_julia_fnorfn__name + 1,
		{s_parameter_real, s_parameter_imag, "Function Shift Value", ""},
		{0, 0, 8, 0},
		HT_FNORFN, HF_JULIAFNFN, TRIG2 | BAILTEST,
		-4.0f, 4.0f, -3.0f, 3.0f,
		16, NOFRACTAL, LMANFNFN, FPJULFNFN, XAXIS,
		julia_trig_or_trig_orbit, julia_per_pixel_l, julia_trig_or_trig_setup,
			standard_fractal,
		BAILOUT_TRIG_L
	},

	{ /* FPJULFNFN */
	s_julia_fnorfn__name,
		{s_parameter_real, s_parameter_imag, "Function Shift Value", ""},
		{0, 0, 8, 0},
		HT_FNORFN, HF_JULIAFNFN, TRIG2 | BAILTEST,
		-4.0f, 4.0f, -3.0f, 3.0f,
		0, NOFRACTAL, FPMANFNFN, LJULFNFN, XAXIS,
		julia_trig_or_trig_orbit_fp, other_julia_per_pixel_fp,
			julia_trig_or_trig_setup, standard_fractal,
		BAILOUT_TRIG_L
	},

	{ /* LMANLAMFNFN */
	s_manlam_fnorfn__name + 1,
		{s_real_z0, s_imag_z0, "Function Shift Value", ""},
		{0, 0, 10, 0},
		HT_FNORFN, HF_MANLAMFNFN, TRIG2 | BAILTEST,
		-4.0f, 4.0f, -3.0f, 3.0f,
		16, LLAMBDAFNFN, NOFRACTAL, FPMANLAMFNFN, XAXIS_NOPARM,
		lambda_trig_or_trig_orbit, mandelbrot_per_pixel_l,
			mandelbrot_lambda_trig_or_trig_setup, standard_fractal,
		BAILOUT_TRIG_L
	},

	{ /* FPMANLAMFNFN */
	s_manlam_fnorfn__name,
		{s_real_z0, s_imag_z0, "Function Shift Value", ""},
		{0, 0, 10, 0},
		HT_FNORFN, HF_MANLAMFNFN, TRIG2 | BAILTEST,
		-4.0f, 4.0f, -3.0f, 3.0f,
		0, FPLAMBDAFNFN, NOFRACTAL, LMANLAMFNFN, XAXIS_NOPARM,
		lambda_trig_or_trig_orbit_fp, other_mandelbrot_per_pixel_fp,
			mandelbrot_lambda_trig_or_trig_setup, standard_fractal,
		BAILOUT_TRIG_L
	},

	{ /* LMANFNFN */
	s_mandel_fnorfn__name + 1,
		{s_real_z0, s_imag_z0, "Function Shift Value", ""},
		{0, 0, 0.5, 0},
		HT_FNORFN, HF_MANDELFNFN, TRIG2 | BAILTEST,
		-4.0f, 4.0f, -3.0f, 3.0f,
		16, LJULFNFN, NOFRACTAL, FPMANFNFN, XAXIS_NOPARM,
		julia_trig_or_trig_orbit, mandelbrot_per_pixel_l,
			mandelbrot_trig_or_trig_setup, standard_fractal,
		BAILOUT_TRIG_L
	},

	{ /* FPMANFNFN */
	s_mandel_fnorfn__name,
		{s_real_z0, s_imag_z0, "Function Shift Value", ""},
		{0, 0, 0.5, 0},
		HT_FNORFN, HF_MANDELFNFN, TRIG2 | BAILTEST,
		-4.0f, 4.0f, -3.0f, 3.0f,
		0, FPJULFNFN, NOFRACTAL, LMANFNFN, XAXIS_NOPARM,
		julia_trig_or_trig_orbit_fp, other_mandelbrot_per_pixel_fp,
			mandelbrot_trig_or_trig_setup, standard_fractal,
		BAILOUT_TRIG_L
	},

	{ /* LBIFMAY */
	s_bifmay_name + 1,
		{s_filter_cycles, s_seed_population, "Beta >= 2", ""},
		{300.0, 0.9, 5, 0},
		HT_BIF, HF_BIFMAY, NOGUESS | NOTRACE | NOROTATE,
		-3.5f, -0.9f, -0.5f, 3.2f,
		16, NOFRACTAL, NOFRACTAL, BIFMAY, NOSYM,
		bifurcation_may, NULL, bifurcation_may_setup, bifurcation,
		BAILOUT_NONE
	},

	{ /* BIFMAY */
	s_bifmay_name,
		{s_filter_cycles, s_seed_population, "Beta >= 2", ""},
		{300.0, 0.9, 5, 0},
		HT_BIF, HF_BIFMAY, NOGUESS | NOTRACE | NOROTATE,
		-3.5f, -0.9f, -0.5f, 3.2f,
		0, NOFRACTAL, NOFRACTAL, LBIFMAY, NOSYM,
		bifurcation_may_fp, NULL, bifurcation_may_setup, bifurcation,
		BAILOUT_NONE
	},

	{ /* MPHALLEY */
	s_halley_name + 1,
		{s_order, s_real_relaxation_coefficient, s_epsilon, s_imag_relaxation_coefficient},
		{6, 1.0, 0.0001, 0},
		HT_HALLEY, HF_HALLEY, 0,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, NOFRACTAL, HALLEY, XYAXIS,
		halley_orbit_mpc, halley_per_pixel_mpc, halley_setup, standard_fractal,
		BAILOUT_NONE
	},

	{ /* HALLEY */
	s_halley_name,
		{s_order, s_real_relaxation_coefficient, s_epsilon, s_imag_relaxation_coefficient},
		{6, 1.0, 0.0001, 0},
		HT_HALLEY, HF_HALLEY, 0,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, NOFRACTAL, MPHALLEY, XYAXIS,
		halley_orbit_fp, halley_per_pixel, halley_setup, standard_fractal,
		BAILOUT_NONE
	},

	{ /* DYNAMICFP */
	"dynamic",
		{"+# of intervals (<0 = connect)", "time step (<0 = Euler)", "a", "b"},
		{50, .1, 1, 3},
		HT_DYNAM, HF_DYNAM, NOGUESS | NOTRACE | TRIG1,
		-20.0f, 20.0f, -20.0f, 20.0f,
		0, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
		(VF) dynamic_orbit_fp, NULL, dynamic_2d_setup_fp, dynamic_2d_fp,
		BAILOUT_NONE
	},

	{ /* QUATFP */
	"quat",
		{"notused", "notused", "cj", "ck"},
		{0, 0, 0, 0},
		HT_QUAT, HF_QUAT, OKJB,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, QUATJULFP, NOFRACTAL, NOFRACTAL, XAXIS,
		quaternion_orbit_fp, quaternion_per_pixel_fp, mandelbrot_setup_fp,
			standard_fractal,
		BAILOUT_TRIG_L
	},

	{ /* QUATJULFP */
	"quatjul",
		{"c1", "ci", "cj", "ck"},
		{-.745, 0, .113, .05},
		HT_QUAT, HF_QUATJ, OKJB | MORE,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, QUATFP, NOFRACTAL, ORIGIN,
		quaternion_orbit_fp, quaternion_julia_per_pixel_fp, julia_setup_fp,
			standard_fractal,
		BAILOUT_TRIG_L
	},

	{ /* CELLULAR */
	"cellular",
		{
			"#Initial String | 0 = Random | -1 = Reuse Last Random",
			"#Rule = # of digits (see below) | 0 = Random",
			"+Type (see below)",
			"#Starting Row Number"
		},
		{11.0, 3311100320.0, 41.0, 0},
		HT_CELLULAR, HF_CELLULAR, NOGUESS | NOTRACE | NOZOOM,
		-1.0f, 1.0f, -1.0f, 1.0f,
		0, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
		NULL, NULL, cellular_setup, cellular,
		BAILOUT_NONE
	},

	{ /* JULIBROTFP */
	s_julibrot_name,
		{"", "", "", ""},
		{0, 0, 0, 0},
		HT_JULIBROT, -1, NOGUESS | NOTRACE | NOROTATE | NORESUME,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, NOFRACTAL, JULIBROT, NOSYM,
		julia_orbit_fp, julibrot_per_pixel_fp, julibrot_setup, standard_4d_fractal_fp,
		BAILOUT_STANDARD
	},

#ifdef RANDOM_RUN
	{ /* INVERSEJULIA */
	s_julia_inverse_name + 1,
		{s_parameter_real, s_parameter_imag, s_max_hits_per_pixel, "Random Run Interval"},
		{-0.11, 0.6557, 4, 1024},
		HT_INVERSE, HF_INVERSE, NOGUESS | NOTRACE | INFCALC | NORESUME,
		-2.0f, 2.0f, -1.5f, 1.5f,
		24, NOFRACTAL, MANDEL, INVERSEJULIAFP, NOSYM,
		Linverse_julia_orbit, NULL, orbit_3d_setup, inverse_julia_per_image,
		BAILOUT_NONE
	},

	{ /* INVERSEJULIAFP */
	s_julia_inverse_name,
		{s_parameter_real, s_parameter_imag, s_max_hits_per_pixel, "Random Run Interval"},
		{-0.11, 0.6557, 4, 1024},
		HT_INVERSE, HF_INVERSE, NOGUESS | NOTRACE | INFCALC | NORESUME,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, MANDEL, INVERSEJULIA, NOSYM,
		Minverse_julia_orbit, NULL, orbit_3d_setup_fp, inverse_julia_per_image,
		BAILOUT_NONE
	},
#else
	{ /* INVERSEJULIA */
	s_julia_inverse_name + 1,
		{s_parameter_real, s_parameter_imag, s_max_hits_per_pixel, ""},
		{-0.11, 0.6557, 4, 1024},
		HT_INVERSE, HF_INVERSE, NOGUESS | NOTRACE | INFCALC | NORESUME,
		-2.0f, 2.0f, -1.5f, 1.5f,
		24, NOFRACTAL, MANDEL, INVERSEJULIAFP, NOSYM,
		Linverse_julia_orbit, NULL, orbit_3d_setup, inverse_julia_per_image,
		BAILOUT_NONE
	},

	{ /* INVERSEJULIAFP */
	s_julia_inverse_name,
		{s_parameter_real, s_parameter_imag, s_max_hits_per_pixel, ""},
		{-0.11, 0.6557, 4, 1024},
		HT_INVERSE, HF_INVERSE, NOGUESS | NOTRACE | INFCALC | NORESUME,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, MANDEL, INVERSEJULIA, NOSYM,
		Minverse_julia_orbit, NULL, orbit_3d_setup_fp, inverse_julia_per_image,
		BAILOUT_NONE
	},

#endif

	{ /* MANDELCLOUD */
	"mandelcloud",
		{"+# of intervals (<0 = connect)", "", "", ""},
		{50, 0, 0, 0},
		HT_MANDELCLOUD, HF_MANDELCLOUD, NOGUESS | NOTRACE,
		-2.5f, 1.5f, -1.5f, 1.5f,
		0, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
		(VF) mandel_cloud_orbit_fp, NULL, dynamic_2d_setup_fp, dynamic_2d_fp,
		BAILOUT_NONE
	},

	{ /* PHOENIX */
	s_phoenix_name + 1,
		{s_p1_real, s_p2_real, s_degree_z, ""},
		{0.56667, -0.5, 0, 0},
		HT_PHOENIX, HF_PHOENIX, BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		1, NOFRACTAL, MANDPHOENIX, PHOENIXFP, XAXIS,
		phoenix_orbit, phoenix_per_pixel, phoenix_setup,
			standard_fractal,
		BAILOUT_STANDARD
	},

	{ /* PHOENIXFP */
	s_phoenix_name,
		{s_p1_real, s_p2_real, s_degree_z, ""},
		{0.56667, -0.5, 0, 0},
		HT_PHOENIX, HF_PHOENIX, BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, MANDPHOENIXFP, PHOENIX, XAXIS,
		phoenix_orbit_fp, phoenix_per_pixel_fp, phoenix_setup, standard_fractal,
		BAILOUT_STANDARD
	},

	{ /* MANDPHOENIX */
	s_mandphoenix_name + 1,
		{s_real_z0, s_imag_z0, s_degree_z, ""},
		{0.0, 0.0, 0, 0},
		HT_PHOENIX, HF_MANDPHOENIX, BAILTEST,
		-2.5f, 1.5f, -1.5f, 1.5f,
		1, PHOENIX, NOFRACTAL, MANDPHOENIXFP, NOSYM,
		phoenix_orbit, mandelbrot_phoenix_per_pixel, mandelbrot_phoenix_setup,
			standard_fractal,
		BAILOUT_STANDARD
	},

	{ /* MANDPHOENIXFP */
	s_mandphoenix_name,
		{s_real_z0, s_imag_z0, s_degree_z, ""},
		{0.0, 0.0, 0, 0},
		HT_PHOENIX, HF_MANDPHOENIX, BAILTEST,
		-2.5f, 1.5f, -1.5f, 1.5f,
		0, PHOENIXFP, NOFRACTAL, MANDPHOENIX, NOSYM,
		phoenix_orbit_fp, mandelbrot_phoenix_per_pixel_fp, mandelbrot_phoenix_setup,
			standard_fractal,
		BAILOUT_STANDARD
	},

	{ /* HYPERCMPLXFP */
	"hypercomplex",
		{"notused", "notused", "cj", "ck"},
		{0, 0, 0, 0},
		HT_HYPERC, HF_HYPERC, OKJB | TRIG1,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, HYPERCMPLXJFP, NOFRACTAL, NOFRACTAL, XAXIS,
		hyper_complex_orbit_fp, quaternion_per_pixel_fp, mandelbrot_setup_fp,
			standard_fractal,
		BAILOUT_TRIG_L
	},

	{ /* HYPERCMPLXJFP */
	"hypercomplexj",
		{"c1", "ci", "cj", "ck"},
		{-.745, 0, .113, .05},
		HT_HYPERC, HF_HYPERCJ, OKJB | TRIG1 | MORE,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, HYPERCMPLXFP, NOFRACTAL, ORIGIN,
		hyper_complex_orbit_fp, quaternion_julia_per_pixel_fp, julia_setup_fp,
			standard_fractal,
		BAILOUT_TRIG_L
	},

	{ /* FROTH */
	s_frothybasin_name + 1,
		{s_frothy_mapping, s_frothy_shade, s_frothy_a_value, ""},
		{1, 0, 1.028713768218725, 0},
		HT_FROTH, HF_FROTH, NOTRACE,
		-2.8f, 2.8f, -2.355f, 1.845f,
		28, NOFRACTAL, NOFRACTAL, FROTHFP, NOSYM,
		froth_per_orbit, froth_per_pixel, froth_setup, froth_calc,
		BAILOUT_FROTHY_BASIN
	},

	{ /* FROTHFP */
	s_frothybasin_name,
		{s_frothy_mapping, s_frothy_shade, s_frothy_a_value, ""},
		{1, 0, 1.028713768218725, 0},
		HT_FROTH, HF_FROTH, NOTRACE,
		-2.8f, 2.8f, -2.355f, 1.845f,
		0, NOFRACTAL, NOFRACTAL, FROTH, NOSYM,
		froth_per_orbit, froth_per_pixel, froth_setup, froth_calc,
		BAILOUT_FROTHY_BASIN
	},

	{ /* MANDEL4FP */
	s_mandel4_name,
		{s_real_z0, s_imag_z0, "", ""},
		{0, 0, 0, 0},
		HT_MANDJUL4, HF_MANDEL4, BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, JULIA4FP, NOFRACTAL, MANDEL4, XAXIS_NOPARM,
		mandel4_orbit_fp, mandelbrot_per_pixel_fp, mandelbrot_setup_fp, standard_fractal,
		BAILOUT_STANDARD
	},

	{ /* JULIA4FP */
	s_julia4_name,
		{s_parameter_real, s_parameter_imag, "", ""},
		{0.6, 0.55, 0, 0},
		HT_MANDJUL4, HF_JULIA4, OKJB | BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, MANDEL4FP, JULIA4, ORIGIN,
		mandel4_orbit_fp, julia_per_pixel_fp, julia_setup_fp, standard_fractal,
		BAILOUT_STANDARD
	},

	{ /* MARKSMANDELFP */
	s_marksmandel_name,
		{s_real_z0, s_imag_z0, s_exponent, ""},
		{0, 0, 1, 0},
		HT_MARKS, HF_MARKSMAND, BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, MARKSJULIAFP, NOFRACTAL, MARKSMANDEL, NOSYM,
		marks_lambda_orbit_fp, marks_mandelbrot_per_pixel_fp, mandelbrot_setup_fp,
			standard_fractal,
		BAILOUT_STANDARD
	},

	{ /* MARKSJULIAFP */
	s_marksjulia_name,
		{s_parameter_real, s_parameter_imag, s_exponent, ""},
		{0.1, 0.9, 1, 0},
		HT_MARKS, HF_MARKSJULIA, BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, MARKSMANDELFP, MARKSJULIA, ORIGIN,
		marks_lambda_orbit_fp, julia_per_pixel_fp, marks_julia_setup_fp,
			standard_fractal,
		BAILOUT_STANDARD
	},

	/* dmf */
	{ /* ICON */
	"icons",
		{"Lambda", "Alpha", "Beta", "Gamma"},
		{-2.34, 2.0, 0.2, 0.1},
		HT_ICON, HF_ICON, NOGUESS | NOTRACE | INFCALC | MORE,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
		(VF) icon_orbit_fp, NULL, orbit_3d_setup_fp, orbit_2d_fp,
		BAILOUT_NONE
	},

	/* dmf */
	{ /* ICON3D */
	"icons3d",
		{"Lambda", "Alpha", "Beta", "Gamma"},
		{-2.34, 2.0, 0.2, 0.1},
		HT_ICON, HF_ICON, NOGUESS | NOTRACE | INFCALC | PARMS3D | MORE,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
		(VF) icon_orbit_fp, NULL, orbit_3d_setup_fp, orbit_3d_fp,
		BAILOUT_NONE
	},

	{ /* PHOENIXCPLX */
	s_phoenixcplx_name + 1,
		{s_p1_real, s_p1_imag, s_p2_real, s_p2_imag},
		{0.2, 0, 0.3, 0},
		HT_PHOENIX, HF_PHOENIXCPLX, MORE | BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		1, NOFRACTAL, MANDPHOENIXCPLX, PHOENIXFPCPLX, ORIGIN,
		phoenix_complex_orbit, phoenix_per_pixel, phoenix_complex_setup,
			standard_fractal,
		BAILOUT_STANDARD
	},

	{ /* PHOENIXFPCPLX */
	s_phoenixcplx_name,
		{s_p1_real, s_p1_imag, s_p2_real, s_p2_imag},
		{0.2, 0, 0.3, 0},
		HT_PHOENIX, HF_PHOENIXCPLX, MORE | BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, MANDPHOENIXFPCPLX, PHOENIXCPLX, ORIGIN,
		phoenix_complex_orbit_fp, phoenix_per_pixel_fp, phoenix_complex_setup,
			standard_fractal,
		BAILOUT_STANDARD
	},

	{ /* MANDPHOENIXCPLX */
	s_mandphoenixcplx_name + 1,
		{s_real_z0, s_imag_z0, s_p2_real, s_p2_imag},
		{0, 0, 0.5, 0},
		HT_PHOENIX, HF_MANDPHOENIXCPLX, MORE | BAILTEST,
		-2.5f, 1.5f, -1.5f, 1.5f,
		1, PHOENIXCPLX, NOFRACTAL, MANDPHOENIXFPCPLX, XAXIS,
		phoenix_complex_orbit, mandelbrot_phoenix_per_pixel,
			mandelbrot_phoenix_complex_setup, standard_fractal,
		BAILOUT_STANDARD
	},

	{ /* MANDPHOENIXFPCPLX */
	s_mandphoenixcplx_name,
		{s_real_z0, s_imag_z0, s_p2_real, s_p2_imag},
		{0, 0, 0.5, 0},
		HT_PHOENIX, HF_MANDPHOENIXCPLX, MORE | BAILTEST,
		-2.5f, 1.5f, -1.5f, 1.5f,
		0, PHOENIXFPCPLX, NOFRACTAL, MANDPHOENIXCPLX, XAXIS,
		phoenix_complex_orbit_fp, mandelbrot_phoenix_per_pixel_fp, mandelbrot_phoenix_complex_setup,
			standard_fractal,
		BAILOUT_STANDARD
	},

	{ /* ANT */
	"ant",
		{"#Rule String (1's and non-1's, 0 rand)",
		"#Maxpts",
		"+Numants (max 256)",
		"+Ant type (1 or 2)"
		},
		{1100, 1.0E9, 1, 1},
		HT_ANT, HF_ANT, NOZOOM | NOGUESS | NOTRACE | NORESUME | MORE,
		-1.0f, 1.0f, -1.0f, 1.0f,
		0, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
		NULL, NULL, stand_alone_setup, ant,
		BAILOUT_NONE
	},

	{ /* CHIP */
	"chip",
		{"a", "b", "c", ""},
		{-15, -19, 1, 0},
		HT_MARTIN, HF_CHIP, NOGUESS | NOTRACE | INFCALC,
		-760.0f, 760.0f, -570.0f, 570.0f,
		0, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
		(VF) chip_2d_orbit_fp, NULL, orbit_3d_setup_fp, orbit_2d_fp,
		BAILOUT_NONE
	},

	{ /* QUADRUPTWO */
	"quadruptwo",
		{"a", "b", "c", ""},
		{34, 1, 5, 0},
		HT_MARTIN, HF_QUADRUPTWO, NOGUESS | NOTRACE | INFCALC,
		-82.93367f, 112.2749f, -55.76383f, 90.64257f,
		0, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
		(VF) quadrup_two_2d_orbit_fp, NULL, orbit_3d_setup_fp, orbit_2d_fp,
		BAILOUT_NONE
	},

	{ /* THREEPLY */
	"threeply",
		{"a", "b", "c", ""},
		{-55, -1, -42, 0},
		HT_MARTIN, HF_THREEPLY, NOGUESS | NOTRACE | INFCALC,
		-8000.0f, 8000.0f, -6000.0f, 6000.0f,
		0, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
		(VF) three_ply_2d_orbit_fp, NULL, orbit_3d_setup_fp, orbit_2d_fp,
		BAILOUT_NONE
	},

	{ /* VL */
	"volterra-lotka",
		{"h", "p", "", ""},
		{0.739, 0.739, 0, 0},
		HT_VL, HF_VL, 0,
		0.0f, 6.0f, 0.0f, 4.5f,
		0, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
		volterra_lotka_orbit_fp, other_julia_per_pixel_fp, volterra_lotka_setup, standard_fractal,
		256
	},

	{ /* ESCHER */
	"escher_julia",
		{s_parameter_real, s_parameter_imag, "", ""},
		{0.32, 0.043, 0, 0},
		HT_ESCHER, HF_ESCHER, 0,
		-1.6f, 1.6f, -1.2f, 1.2f,
		0, NOFRACTAL, NOFRACTAL, NOFRACTAL, ORIGIN,
		escher_orbit_fp, julia_per_pixel_fp, standard_setup,
			standard_fractal,
		BAILOUT_STANDARD
	},

	/* From Pickovers' "Chaos in Wonderland"      */
	/* included by Humberto R. Baptista           */
	/* code adapted from king.cpp bt James Rankin */
	{ /* LATOO */
	"latoocarfian",
		{"a", "b", "c", "d"},
		{-0.966918, 2.879879, 0.765145, 0.744728},
		HT_LATOO, HF_LATOO, NOGUESS | NOTRACE | INFCALC | MORE | TRIG4,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
		(VF) latoo_orbit_fp, NULL, orbit_3d_setup_fp, orbit_2d_fp,
		BAILOUT_NONE
	},
#if 0
	{ /* MANDELBROTMIX4 */
	"mandelbrotmix4",
		{s_p1_real, s_p1_imag, s_p2_real, s_p2_imag},
		{0.05, 3, -1.5, -2},
		HT_MANDELBROTMIX4, HF_MANDELBROTMIX4, BAILTEST | TRIG1 | MORE,
		-2.5f, 1.5f, -1.5f, 1.5f,
		0, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
		mandelbrot_mix4_orbit_fp, mandelbrot_mix4_per_pixel_fp, mandelbrot_mix4_setup, standard_fractal,
		BAILOUT_STANDARD
	},
#endif
	{
		NULL,            /* marks the END of the list */
		{NULL, NULL, NULL, NULL},
		{0, 0, 0, 0},
		-1, -1, 0,
		0.0f, 0.0f, 0.0f, 0.0f,
		0, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
		NULL, NULL, NULL, NULL,
		0
	}
};

extern "C" int g_num_fractal_types = NUM_OF(g_fractal_specific)-1;

/*
 *  Returns 1 if the formula parameter is not used in the current
 *  formula.  If the parameter is used, or not a formula fractal,
 *  a 0 is returned.  Note: this routine only works for formula types.
 */
extern "C" int parameter_not_used(int parm)
{
	int ret = 0;

	/* sanity check */
	if (g_fractal_type != FORMULA && g_fractal_type != FFORMULA)
	{
		return 0;
	}

	switch (parm/2)
	{
	case 0:
		if (!g_uses_p1)
		{
			ret = 1;
		}
		break;
	case 1:
		if (!g_uses_p2)
		{
			ret = 1;
		}
		break;
	case 2:
		if (!g_uses_p3)
		{
			ret = 1;
		}
		break;
	case 3:
		if (!g_uses_p4)
		{
			ret = 1;
		}
		break;
	case 4:
		if (!g_uses_p5)
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
extern "C" int type_has_parameter(int type, int parm, char *buf)
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

	if (type == FORMULA || type == FFORMULA)
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
