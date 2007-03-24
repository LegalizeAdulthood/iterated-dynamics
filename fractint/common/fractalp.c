/*
		This module consists only of the fractalspecific structure
		and a *slew* of defines needed to get it to compile
*/
#include <string.h>

/* includes needed for fractalspecific */

  /* see Fractint.c for a description of the "include"  hierarchy */
#include "port.h"
#include "prototyp.h"
#include "helpdefs.h"
#include "fractype.h"

/* functions defined elswhere needed for fractalspecific */
/* moved to prototyp.h */

/* parameter descriptions */
/* Note: parameters preceded by + are integer parameters */
/*       parameters preceded by # are U32 parameters */

/* for Mandelbrots */
static char realz0[] = "Real Perturbation of Z(0)";
static char imagz0[] = "Imaginary Perturbation of Z(0)";

/* for Julias */
static char realparm[] = "Real Part of Parameter";
static char imagparm[] = "Imaginary Part of Parameter";

/* for Newtons */
static char newtdegree[] = "+Polynomial Degree (>= 2)";

/* for MarksMandel/Julia */
static char exponent[]   = "Real part of Exponent";
static char imexponent[] = "Imag part of Exponent";

/* for Lorenz */
static char timestep[]     = "Time Step";

/* for formula */
static char p1real[] = "Real portion of p1";
static char p2real[] = "Real portion of p2";
static char p3real[] = "Real portion of p3";
static char p4real[] = "Real portion of p4";
static char p5real[] = "Real portion of p5";
static char p1imag[] = "Imaginary portion of p1";
static char p2imag[] = "Imaginary portion of p2";
static char p3imag[] = "Imaginary portion of p3";
static char p4imag[] = "Imaginary portion of p4";
static char p5imag[] = "Imaginary portion of p5";

/* trig functions */
static char recoeftrg1[] = "Real Coefficient First Function";
static char imcoeftrg1[] = "Imag Coefficient First Function";
static char recoeftrg2[] = "Real Coefficient Second Function";
static char imcoeftrg2[] = "Imag Coefficient Second Function";

/* KAM Torus */
static char kamangle[] = "Angle (radians)";
static char kamstep[] =  "Step size";
static char kamstop[] =  "Stop value";
static char pointsperorbit[] = "+Points per orbit";

/* popcorn and julia popcorn generalized */
static char step_x[] = "Step size (real)";
static char step_y[] = "Step size (imaginary)";
static char constant_x[] = "Constant C (real)";
static char constant_y[] = "Constant C (imaginary)";

/* bifurcations */
static char filt[] = "+Filter Cycles";
static char seed[] = "Seed Population";

/* frothy basins */
static char frothmapping[] = "+Apply mapping once (1) or twice (2)";
static char frothshade[] =  "+Enter non-zero value for alternate color shading";
static char frothavalue[] =  "A (imaginary part of C)";

/* plasma and ant */

static char s_randomseed[] = "+Random Seed Value (0 = Random, 1 = Reuse Last)";

/* ifs */
static char color_method[] = "+Coloring method (0,1)";

/* phoenix fractals */
static char degreeZ[] = "Degree = 0 | >= 2 | <= -3";

/* julia inverse */
static char s_maxhits[] = "Max Hits per Pixel";

/* halley */
static char order[] = {"+Order (integer > 1)"};
static char real_relax[] = {"Real Relaxation coefficient"};
static char epsilon[] = {"Epsilon"};
static char imag_relax[] = {"Imag Relaxation coefficient"};
/* cellular */
static char cell_init[] = {"#Initial String | 0 = Random | -1 = Reuse Last Random"};
static char cell_rule[] = {"#Rule = # of digits (see below) | 0 = Random"};
static char cell_type[] = {"+Type (see below)"};
static char cell_strt[] = {"#Starting Row Number"};

/* bailout defines */
#define FTRIGBAILOUT 2500
#define LTRIGBAILOUT   64
#define FROTHBAILOUT    7
#define STDBAILOUT      4
#define NOBAILOUT       0

MOREPARAMS moreparams[] =
{
	{ICON             , { "Omega", "+Degree of symmetry",   "", "", "", ""}, {0, 3, 0, 0, 0, 0}},
	{ICON3D           , { "Omega", "+Degree of symmetry",   "", "", "", ""}, {0, 3, 0, 0, 0, 0}},
	{HYPERCMPLXJFP    , { "zj",      "zk",          "", "", "", ""}, {0, 0, 0, 0, 0, 0}},
	{QUATJULFP        , { "zj",      "zk",          "", "", "", ""}, {0, 0, 0, 0, 0, 0}},
	{PHOENIXCPLX      , { degreeZ, "",          "", "", "", ""}, {0, 0, 0, 0, 0, 0}},
	{PHOENIXFPCPLX    , { degreeZ, "",          "", "", "", ""}, {0, 0, 0, 0, 0, 0}},
	{MANDPHOENIXCPLX  , { degreeZ, "",          "", "", "", ""}, {0, 0, 0, 0, 0, 0}},
	{MANDPHOENIXFPCPLX, { degreeZ, "",          "", "", "", ""}, {0, 0, 0, 0, 0, 0}},
	{FORMULA  , { p3real, p3imag, p4real, p4imag, p5real, p5imag}, {0, 0, 0, 0, 0, 0}},
	{FFORMULA , { p3real, p3imag, p4real, p4imag, p5real, p5imag}, {0, 0, 0, 0, 0, 0}},
	{ANT              , { "+Wrap?", s_randomseed, "", "", "", ""}, {1, 0, 0, 0, 0, 0}},
	{MANDELBROTMIX4   , { p3real, p3imag,        "", "", "", ""}, {0, 0, 0, 0, 0, 0}},
	{-1               , { NULL, NULL, NULL, NULL, NULL, NULL    }, {0, 0, 0, 0, 0, 0}}
};

/*
     type math orbitcalc fnct per_pixel fnct per_image fnct
	|-----|----|--------------|--------------|--------------| */
struct alternatemathstuff alternatemath[] =
{
#define USEBN
#ifdef USEBN
	{JULIAFP, 1, JuliabnFractal, juliabn_per_pixel,  MandelbnSetup},
	{MANDELFP, 1, JuliabnFractal, mandelbn_per_pixel, MandelbnSetup},
#else
	{JULIAFP, 2, JuliabfFractal, juliabf_per_pixel,  MandelbfSetup},
	{MANDELFP, 2, JuliabfFractal, mandelbf_per_pixel, MandelbfSetup},
#endif
/*
NOTE: The default precision for bf_math=BIGNUM is not high enough
		for JuliaZpowerbnFractal.  If you want to test BIGNUM (1) instead
		of the usual BIGFLT (2), then set bfdigits on the command to
		increase the precision.
*/
	{FPJULIAZPOWER, 2, JuliaZpowerbfFractal, juliabf_per_pixel, MandelbfSetup  },
	{FPMANDELZPOWER, 2, JuliaZpowerbfFractal, mandelbf_per_pixel, MandelbfSetup},
	{-1,            0, NULL,                NULL,               NULL         }
};

/* These are only needed for types with both integer and float variations */
char t_barnsleyj1[]= "*barnsleyj1";
char t_barnsleyj2[]= "*barnsleyj2";
char t_barnsleyj3[]= "*barnsleyj3";
char t_barnsleym1[]= "*barnsleym1";
char t_barnsleym2[]= "*barnsleym2";
char t_barnsleym3[]= "*barnsleym3";
char t_bifplussinpi[]= "*bif+sinpi";
char t_bifeqsinpi[]= "*bif=sinpi";
char t_biflambda[]= "*biflambda";
char t_bifmay[]= "*bifmay";
char t_bifstewart[]= "*bifstewart";
char t_bifurcation[]= "*bifurcation";
char t_fn_z_plusfn_pix_[]= "*fn(z)+fn(pix)";
char t_fn_zz_[]= "*fn(z*z)";
char t_fnfn[]= "*fn*fn";
char t_fnzplusz[]= "*fn*z+z";
char t_fnplusfn[]= "*fn+fn";
char t_formula[]= "*formula";
char t_henon[]= "*henon";
char t_ifs3d[]= "*ifs3d";
char t_julfnplusexp[]= "*julfn+exp";
char t_julfnpluszsqrd[]= "*julfn+zsqrd";
char t_julia[]= "*julia";
char t_julia_fnorfn_[]= "*julia(fn||fn)";
char t_julia4[]= "*julia4";
char t_julia_inverse[]= "*julia_inverse";
char t_julibrot[]= "*julibrot";
char t_julzpower[]= "*julzpower";
char t_kamtorus[]= "*kamtorus";
char t_kamtorus3d[]= "*kamtorus3d";
char t_lambda[]= "*lambda";
char t_lambda_fnorfn_[]= "*lambda(fn||fn)";
char t_lambdafn[]= "*lambdafn";
char t_lorenz[]= "*lorenz";
char t_lorenz3d[]= "*lorenz3d";
char t_mandel[]= "*mandel";
char t_mandel_fnorfn_[]= "*mandel(fn||fn)";
char t_mandel4[]= "*mandel4";
char t_mandelfn[]= "*mandelfn";
char t_mandellambda[]= "*mandellambda";
char t_mandphoenix[]= "*mandphoenix";
char t_mandphoenixcplx[]= "*mandphoenixclx";
char t_manfnplusexp[]= "*manfn+exp";
char t_manfnpluszsqrd[]= "*manfn+zsqrd";
char t_manlam_fnorfn_[]= "*manlam(fn||fn)";
char t_manowar[]= "*manowar";
char t_manowarj[]= "*manowarj";
char t_manzpower[]= "*manzpower";
char t_marksjulia[]= "*marksjulia";
char t_marksmandel[]= "*marksmandel";
char t_marksmandelpwr[]= "*marksmandelpwr";
char t_newtbasin[]= "*newtbasin";
char t_newton[]= "*newton";
char t_phoenix[]= "*phoenix";
char t_phoenixcplx[]= "*phoenixcplx";
char t_popcorn[]= "*popcorn";
char t_popcornjul[]= "*popcornjul";
char t_rossler3d[]= "*rossler3d";
char t_sierpinski[]= "*sierpinski";
char t_spider[]= "*spider";
char t_sqr_1divfn_[]= "*sqr(1/fn)";
char t_sqr_fn_[]= "*sqr(fn)";
char t_tims_error[]= "*tim's_error";
char t_unity[]= "*unity";
char t_frothybasin[]= "*frothybasin";
char t_halley[]= "*halley";

/* use next to cast orbitcalcs() that have arguments */
#define VF int(*)(void)

struct fractalspecificstuff fractalspecific[]=
{
	/*
	{
	fractal name,
		{parameter text strings},
		{parameter values},
		helptext, helpformula, flags,
		xmin, xmax, ymin, ymax,
		int, tojulia, tomandel, tofloat, symmetry,
		orbit fnct, per_pixel fnct, per_image fnct, calctype fcnt,
		bailout
	}
	*/

	{
	t_mandel + 1,
		{realz0, imagz0, "", ""},
		{0, 0, 0, 0},
		HT_MANDEL, HF_MANDEL, WINFRAC + BAILTEST,
		-2.5f, 1.5f, -1.5f, 1.5f,
		1, JULIA, NOFRACTAL, MANDELFP, XAXIS_NOPARM,
		JuliaFractal, mandel_per_pixel, MandelSetup, StandardFractal,
		STDBAILOUT
	},

	{
	t_julia + 1,
		{realparm, imagparm, "", ""},
		{0.3, 0.6, 0, 0},
		HT_JULIA, HF_JULIA, WINFRAC + OKJB + BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		1, NOFRACTAL, MANDEL, JULIAFP, ORIGIN,
		JuliaFractal, julia_per_pixel, JuliaSetup, StandardFractal,
		STDBAILOUT
	},

	{
	t_newtbasin,
		{newtdegree, "Enter non-zero value for stripes", "", ""},
		{3, 0, 0, 0},
		HT_NEWTBAS, HF_NEWTBAS, WINFRAC,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, NOFRACTAL, MPNEWTBASIN, NOSYM,
		NewtonFractal2, otherjuliafp_per_pixel, NewtonSetup, StandardFractal,
		NOBAILOUT
	},

	{
	t_lambda + 1,
		{realparm, imagparm, "", ""},
		{0.85, 0.6, 0, 0},
		HT_LAMBDA, HF_LAMBDA, WINFRAC + OKJB + BAILTEST,
		-1.5f, 2.5f, -1.5f, 1.5f,
		1, NOFRACTAL, MANDELLAMBDA, LAMBDAFP, NOSYM,
		LambdaFractal, julia_per_pixel, JulialongSetup, StandardFractal,
		STDBAILOUT
	},

	{
	t_mandel,
		{realz0, imagz0, "", ""},
		{0, 0, 0, 0},
		HT_MANDEL, HF_MANDEL, WINFRAC + BAILTEST + BF_MATH,
		-2.5f, 1.5f, -1.5f, 1.5f,
		0, JULIAFP, NOFRACTAL, MANDEL, XAXIS_NOPARM,
		JuliafpFractal, mandelfp_per_pixel, MandelfpSetup, StandardFractal,
		STDBAILOUT
	},

	{
	t_newton,
		{newtdegree, "", "", ""},
		{3, 0, 0, 0},
		HT_NEWT, HF_NEWT, WINFRAC,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, NOFRACTAL, MPNEWTON, XAXIS,
		NewtonFractal2, otherjuliafp_per_pixel, NewtonSetup, StandardFractal,
		NOBAILOUT
	},

	{
	t_julia,
		{realparm, imagparm, "", ""},
		{0.3, 0.6, 0, 0},
		HT_JULIA, HF_JULIA, WINFRAC + OKJB + BAILTEST + BF_MATH,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, MANDELFP, JULIA, ORIGIN,
		JuliafpFractal, juliafp_per_pixel,  JuliafpSetup, StandardFractal,
		STDBAILOUT
	},

	{
	"plasma",
		{"Graininess Factor (0 or 0.125 to 100, default is 2)",
       "+Algorithm (0 = original, 1 = new)",
       "+Random Seed Value (0 = Random, 1 = Reuse Last)",
       "+Save as Pot File? (0 = No,     1 = Yes)"
		},
		{2, 0, 0, 0},
		HT_PLASMA, HF_PLASMA, NOZOOM + NOGUESS + NOTRACE + NORESUME + WINFRAC,
		-2.0f, 2.0f, -1.5f, 1.5f,
		1, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
		NULL, NULL, StandaloneSetup, plasma,
		NOBAILOUT
	},

	{
	t_mandelfn,
		{realz0, imagz0, "", ""},
		{0, 0, 0, 0},
		HT_MANDFN, HF_MANDFN, TRIG1 + WINFRAC,
		-8.0f, 8.0f, -6.0f, 6.0f,
		0, LAMBDATRIGFP, NOFRACTAL, MANDELTRIG, XYAXIS_NOPARM,
		LambdaTrigfpFractal, othermandelfp_per_pixel, MandelTrigSetup,
                                                         StandardFractal,
		LTRIGBAILOUT
	},

	{
	t_manowar,
		{realz0, imagz0, "", ""},
		{0, 0, 0, 0},
		HT_SCOTSKIN, HF_MANOWAR, WINFRAC + BAILTEST,
		-2.5f, 1.5f, -1.5f, 1.5f,
		0, MANOWARJFP, NOFRACTAL, MANOWAR, XAXIS_NOPARM,
		ManOWarfpFractal, mandelfp_per_pixel, MandelfpSetup, StandardFractal,
		STDBAILOUT
	},

	{
	t_manowar + 1,
		{realz0, imagz0, "", ""},
		{0, 0, 0, 0},
		HT_SCOTSKIN, HF_MANOWAR, WINFRAC + BAILTEST,
		-2.5f,  1.5f, -1.5f, 1.5f,
		1, MANOWARJ, NOFRACTAL, MANOWARFP, XAXIS_NOPARM,
		ManOWarFractal, mandel_per_pixel, MandellongSetup, StandardFractal,
		STDBAILOUT
	},

	{
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
		NULL, NULL, StandaloneSetup, test,
		STDBAILOUT
	},

	{
	t_sierpinski + 1,
		{"", "", "", ""},
		{0, 0, 0, 0},
		HT_SIER, HF_SIER, WINFRAC,
		-4.0f/3.0f, 96.0f/45.0f, -0.9f, 1.7f,
		1, NOFRACTAL, NOFRACTAL, SIERPINSKIFP, NOSYM,
		SierpinskiFractal, long_julia_per_pixel, SierpinskiSetup,
                                                        StandardFractal,
		127
	},

	{
	t_barnsleym1 + 1,
		{realz0, imagz0, "", ""},
		{0, 0, 0, 0},
		HT_BARNS, HF_BARNSM1, WINFRAC + BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		1, BARNSLEYJ1, NOFRACTAL, BARNSLEYM1FP, XYAXIS_NOPARM,
		Barnsley1Fractal, long_mandel_per_pixel, MandellongSetup,
                                                        StandardFractal,
		STDBAILOUT
	},

	{
	t_barnsleyj1 + 1,
		{realparm, imagparm, "", ""},
		{0.6, 1.1, 0, 0},
		HT_BARNS, HF_BARNSJ1, WINFRAC + OKJB + BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		1, NOFRACTAL, BARNSLEYM1, BARNSLEYJ1FP, ORIGIN,
		Barnsley1Fractal, long_julia_per_pixel, JulialongSetup, StandardFractal,
		STDBAILOUT
	},

	{
	t_barnsleym2 + 1,
		{realz0, imagz0, "", ""},
		{0, 0, 0, 0},
		HT_BARNS, HF_BARNSM2, WINFRAC + BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		1, BARNSLEYJ2, NOFRACTAL, BARNSLEYM2FP, YAXIS_NOPARM,
		Barnsley2Fractal, long_mandel_per_pixel, MandellongSetup,
                                                        StandardFractal,
		STDBAILOUT
	},

	{
	t_barnsleyj2 + 1,
		{realparm, imagparm, "", ""},
		{0.6, 1.1, 0, 0},
		HT_BARNS, HF_BARNSJ2, WINFRAC + OKJB + BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		1, NOFRACTAL, BARNSLEYM2, BARNSLEYJ2FP, ORIGIN,
		Barnsley2Fractal, long_julia_per_pixel, JulialongSetup, StandardFractal,
		STDBAILOUT
	},

	{
	t_sqr_fn_ + 1,
		{"", "", "", ""},
		{0, 0, 0, 0},
		HT_SCOTSKIN, HF_SQRFN, TRIG1 + WINFRAC + BAILTEST,
		-4.0f, 4.0f, -3.0f, 3.0f,
		16, NOFRACTAL, NOFRACTAL, SQRTRIGFP, XAXIS,
		SqrTrigFractal, long_julia_per_pixel, SqrTrigSetup, StandardFractal,
		LTRIGBAILOUT
	},

	{
	t_sqr_fn_,
		{"", "", "", ""},
		{0, 0, 0, 0},
		HT_SCOTSKIN, HF_SQRFN, TRIG1 + WINFRAC + BAILTEST,
		-4.0f, 4.0f, -3.0f, 3.0f,
		0, NOFRACTAL, NOFRACTAL, SQRTRIG, XAXIS,
		SqrTrigfpFractal, otherjuliafp_per_pixel, SqrTrigSetup, StandardFractal,
		LTRIGBAILOUT
	},

	{
	t_fnplusfn + 1,
		{recoeftrg1, imcoeftrg1, recoeftrg2, imcoeftrg2},
		{1, 0, 1, 0},
		HT_SCOTSKIN, HF_FNPLUSFN, TRIG2 + WINFRAC + BAILTEST,
		-4.0f, 4.0f, -3.0f, 3.0f,
		16, NOFRACTAL, NOFRACTAL, TRIGPLUSTRIGFP, XAXIS,
		TrigPlusTrigFractal, long_julia_per_pixel, TrigPlusTriglongSetup,
                                                            StandardFractal,
		LTRIGBAILOUT
	},

	{
	t_mandellambda + 1,
		{realz0, imagz0, "", ""},
		{0, 0, 0, 0},
		HT_MLAMBDA, HF_MLAMBDA, WINFRAC + BAILTEST,
		-3.0f, 5.0f, -3.0f, 3.0f,
		1, LAMBDA, NOFRACTAL, MANDELLAMBDAFP, XAXIS_NOPARM,
		LambdaFractal, mandel_per_pixel, MandellongSetup, StandardFractal,
		STDBAILOUT
	},

	{
	t_marksmandel + 1,
		{realz0, imagz0, exponent, ""},
		{0, 0, 1, 0},
		HT_MARKS, HF_MARKSMAND, WINFRAC + BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		1, MARKSJULIA, NOFRACTAL, MARKSMANDELFP, NOSYM,
		MarksLambdaFractal, marksmandel_per_pixel, MandellongSetup,
                                                        StandardFractal,
		STDBAILOUT
	},

	{
	t_marksjulia + 1,
		{realparm, imagparm, exponent, ""},
		{0.1, 0.9, 1, 0},
		HT_MARKS, HF_MARKSJULIA, WINFRAC + BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		1, NOFRACTAL, MARKSMANDEL, MARKSJULIAFP, ORIGIN,
		MarksLambdaFractal, julia_per_pixel, MarksJuliaSetup, StandardFractal,
		STDBAILOUT
	},

	{
	t_unity + 1,
		{"", "", "", ""},
		{0, 0, 0, 0},
		HT_UNITY, HF_UNITY, WINFRAC,
		-2.0f, 2.0f, -1.5f, 1.5f,
		1, NOFRACTAL, NOFRACTAL, UNITYFP, XYAXIS,
		UnityFractal, long_julia_per_pixel, UnitySetup, StandardFractal,
		NOBAILOUT
	},

	{
	t_mandel4 + 1,
		{realz0, imagz0, "", ""},
		{0, 0, 0, 0},
		HT_MANDJUL4, HF_MANDEL4, WINFRAC + BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		1, JULIA4, NOFRACTAL, MANDEL4FP, XAXIS_NOPARM,
		Mandel4Fractal, mandel_per_pixel, MandellongSetup, StandardFractal,
		STDBAILOUT
	},

	{
	t_julia4 + 1,
		{realparm, imagparm, "", ""},
		{0.6, 0.55, 0, 0},
		HT_MANDJUL4, HF_JULIA4, WINFRAC + OKJB + BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		1, NOFRACTAL, MANDEL4, JULIA4FP, ORIGIN,
		Mandel4Fractal, julia_per_pixel, JulialongSetup, StandardFractal,
		STDBAILOUT
	},

	{
	"ifs",
		{color_method, "", "", ""},
		{0, 0, 0, 0},
		HT_IFS, -4, NOGUESS + NOTRACE + NORESUME + WINFRAC + INFCALC,
		-8.0f, 8.0f, -1.0f, 11.0f,
		16, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
		NULL, NULL, StandaloneSetup, ifs,
		NOBAILOUT
	},

	{
	t_ifs3d,
		{color_method, "", "", ""},
		{0, 0, 0, 0},
		HT_IFS, -4, NOGUESS + NOTRACE + NORESUME + WINFRAC + PARMS3D + INFCALC,
		-11.0f, 11.0f, -11.0f, 11.0f,
		16, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
		NULL, NULL, StandaloneSetup, ifs,
		NOBAILOUT
	},

	{
	t_barnsleym3 + 1,
		{realz0, imagz0, "", ""},
		{0, 0, 0, 0},
		HT_BARNS, HF_BARNSM3, WINFRAC + BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		1, BARNSLEYJ3, NOFRACTAL, BARNSLEYM3FP, XAXIS_NOPARM,
		Barnsley3Fractal, long_mandel_per_pixel, MandellongSetup,
                                                      StandardFractal,
		STDBAILOUT
	},

	{
	t_barnsleyj3 + 1,
		{realparm, imagparm, "", ""},
		{0.1, 0.36, 0, 0},
		HT_BARNS, HF_BARNSJ3, WINFRAC + OKJB + BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		1, NOFRACTAL, BARNSLEYM3, BARNSLEYJ3FP, NOSYM,
		Barnsley3Fractal, long_julia_per_pixel, JulialongSetup, StandardFractal,
		STDBAILOUT
	},

	{
	t_fn_zz_ + 1,
		{"", "", "", ""},
		{0, 0, 0, 0},
		HT_SCOTSKIN, HF_FNZTIMESZ, TRIG1 + WINFRAC + BAILTEST,
		-4.0f, 4.0f, -3.0f, 3.0f,
		16, NOFRACTAL, NOFRACTAL, TRIGSQRFP, XYAXIS,
		TrigZsqrdFractal, julia_per_pixel, JulialongSetup, StandardFractal,
		STDBAILOUT
	},

	{
	t_fn_zz_,
		{"", "", "", ""},
		{0, 0, 0, 0},
		HT_SCOTSKIN, HF_FNZTIMESZ, TRIG1 + WINFRAC + BAILTEST,
		-4.0f, 4.0f, -3.0f, 3.0f,
		0, NOFRACTAL, NOFRACTAL, TRIGSQR, XYAXIS,
		TrigZsqrdfpFractal, juliafp_per_pixel, JuliafpSetup, StandardFractal,
		STDBAILOUT
	},

	{
	t_bifurcation,
		{filt, seed, "", ""},
		{1000.0, 0.66, 0, 0},
		HT_BIF, HF_BIFURCATION, TRIG1 + NOGUESS + NOTRACE + NOROTATE + WINFRAC,
		1.9f, 3.0f, 0.0f, 1.34f,
		0, NOFRACTAL, NOFRACTAL, LBIFURCATION, NOSYM,
		BifurcVerhulstTrig, NULL, StandaloneSetup, Bifurcation,
		NOBAILOUT
	},

	{
	t_fnplusfn,
		{recoeftrg1, imcoeftrg1, recoeftrg2, imcoeftrg2},
		{1, 0, 1, 0},
		HT_SCOTSKIN, HF_FNPLUSFN, TRIG2 + WINFRAC + BAILTEST,
		-4.0f, 4.0f, -3.0f, 3.0f,
		0, NOFRACTAL, NOFRACTAL, TRIGPLUSTRIG, XAXIS,
		TrigPlusTrigfpFractal, otherjuliafp_per_pixel, TrigPlusTrigfpSetup,
                                                            StandardFractal,
		LTRIGBAILOUT
	},

	{
	t_fnfn + 1,
		{"", "", "", ""},
		{0, 0, 0, 0},
		HT_SCOTSKIN, HF_FNTIMESFN, TRIG2 + WINFRAC + BAILTEST,
		-4.0f, 4.0f, -3.0f, 3.0f,
		16, NOFRACTAL, NOFRACTAL, TRIGXTRIGFP, XAXIS,
		TrigXTrigFractal, long_julia_per_pixel, FnXFnSetup, StandardFractal,
		LTRIGBAILOUT
	},

	{
	t_fnfn,
		{"", "", "", ""},
		{0, 0, 0, 0},
		HT_SCOTSKIN, HF_FNTIMESFN, TRIG2 + WINFRAC + BAILTEST,
		-4.0f, 4.0f, -3.0f, 3.0f,
		0, NOFRACTAL, NOFRACTAL, TRIGXTRIG, XAXIS,
		TrigXTrigfpFractal, otherjuliafp_per_pixel, FnXFnSetup, StandardFractal,
		LTRIGBAILOUT
	},

	{
	t_sqr_1divfn_ + 1,
		{"", "", "", ""},
		{0, 0, 0, 0},
		HT_SCOTSKIN, HF_SQROVFN, TRIG1 + WINFRAC + BAILTEST,
		-4.0f, 4.0f, -3.0f, 3.0f,
		16, NOFRACTAL, NOFRACTAL, SQR1OVERTRIGFP, NOSYM,
		Sqr1overTrigFractal, long_julia_per_pixel, SqrTrigSetup,
                                                         StandardFractal,
		LTRIGBAILOUT
	},

	{
	t_sqr_1divfn_,
		{"", "", "", ""},
		{0, 0, 0, 0},
		HT_SCOTSKIN, HF_SQROVFN, TRIG1 + WINFRAC + BAILTEST,
		-4.0f, 4.0f, -3.0f, 3.0f,
		0, NOFRACTAL, NOFRACTAL, SQR1OVERTRIG, NOSYM,
		Sqr1overTrigfpFractal, otherjuliafp_per_pixel, SqrTrigSetup,
                                                        StandardFractal,
		LTRIGBAILOUT
	},

	{
	t_fnzplusz + 1,
		{recoeftrg1, imcoeftrg1, "Real Coefficient Second Term", "Imag Coefficient Second Term"},
		{1, 0, 1, 0},
		HT_SCOTSKIN, HF_FNXZPLUSZ, TRIG1 + WINFRAC + BAILTEST,
		-4.0f, 4.0f, -3.0f, 3.0f,
		1, NOFRACTAL, NOFRACTAL, ZXTRIGPLUSZFP, XAXIS,
		ZXTrigPlusZFractal, julia_per_pixel, ZXTrigPlusZSetup, StandardFractal,
		LTRIGBAILOUT
	},

	{
	t_fnzplusz,
		{recoeftrg1, imcoeftrg1, "Real Coefficient Second Term", "Imag Coefficient Second Term"},
		{1, 0, 1, 0},
		HT_SCOTSKIN, HF_FNXZPLUSZ, TRIG1 + WINFRAC + BAILTEST,
		-4.0f, 4.0f, -3.0f, 3.0f,
		0, NOFRACTAL, NOFRACTAL, ZXTRIGPLUSZ, XAXIS,
		ZXTrigPlusZfpFractal, juliafp_per_pixel, ZXTrigPlusZSetup,
                                                           StandardFractal,
		LTRIGBAILOUT
	},

	{
	t_kamtorus,
		{kamangle, kamstep, kamstop, pointsperorbit},
		{1.3, .05, 1.5, 150},
		HT_KAM, HF_KAM, NOGUESS + NOTRACE + WINFRAC,
		-1.0f, 1.0f, -.75f, .75f,
		0, NOFRACTAL, NOFRACTAL, KAM, NOSYM,
		(VF)kamtorusfloatorbit, NULL, orbit3dfloatsetup, orbit2dfloat,
		NOBAILOUT
	},

	{
	t_kamtorus + 1,
		{kamangle, kamstep, kamstop, pointsperorbit},
		{1.3, .05, 1.5, 150},
		HT_KAM, HF_KAM, NOGUESS + NOTRACE + WINFRAC,
		-1.0f, 1.0f, -.75f, .75f,
		16, NOFRACTAL, NOFRACTAL, KAMFP, NOSYM,
		(VF)kamtoruslongorbit, NULL, orbit3dlongsetup, orbit2dlong,
		NOBAILOUT
	},

	{
	t_kamtorus3d,
		{kamangle, kamstep, kamstop, pointsperorbit},
		{1.3, .05, 1.5, 150},
		HT_KAM, HF_KAM, NOGUESS + NOTRACE + NORESUME + WINFRAC + PARMS3D,
		-3.0f, 3.0f, -1.0f, 3.5f,
		0, NOFRACTAL, NOFRACTAL, KAM3D, NOSYM,
		(VF)kamtorusfloatorbit, NULL, orbit3dfloatsetup, orbit3dfloat,
		NOBAILOUT
	},

	{
	t_kamtorus3d + 1,
		{kamangle, kamstep, kamstop, pointsperorbit},
		{1.3, .05, 1.5, 150},
		HT_KAM, HF_KAM, NOGUESS + NOTRACE + NORESUME + WINFRAC + PARMS3D,
		-3.0f, 3.0f, -1.0f, 3.5f,
		16, NOFRACTAL, NOFRACTAL, KAM3DFP, NOSYM,
		(VF)kamtoruslongorbit, NULL, orbit3dlongsetup, orbit3dlong,
		NOBAILOUT
	},

	{
	t_lambdafn + 1,
		{realparm, imagparm, "", ""},
		{1.0, 0.4, 0, 0},
		HT_LAMBDAFN, HF_LAMBDAFN, TRIG1 + WINFRAC + OKJB,
		-4.0f, 4.0f, -3.0f, 3.0f,
		16, NOFRACTAL, MANDELTRIG, LAMBDATRIGFP, PI_SYM,
		(VF)LambdaTrigFractal, long_julia_per_pixel, LambdaTrigSetup,
                                                         StandardFractal,
		LTRIGBAILOUT
	},

	{
	t_manfnpluszsqrd + 1,
		{realz0, imagz0, "", ""},
		{0, 0, 0, 0},
		HT_PICKMJ, HF_MANDFNPLUSZSQRD, TRIG1 + WINFRAC + BAILTEST,
		-2.5f, 1.5f, -1.5f, 1.5f,
		16, LJULTRIGPLUSZSQRD, NOFRACTAL, FPMANTRIGPLUSZSQRD, XAXIS_NOPARM,
		TrigPlusZsquaredFractal, mandel_per_pixel, MandellongSetup,
                                                            StandardFractal,
		STDBAILOUT
	},

	{
	t_julfnpluszsqrd + 1,
		{realparm, imagparm, "", ""},
		{-0.5, 0.5, 0, 0},
		HT_PICKMJ, HF_JULFNPLUSZSQRD, TRIG1 + WINFRAC + OKJB + BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		16, NOFRACTAL, LMANTRIGPLUSZSQRD, FPJULTRIGPLUSZSQRD, NOSYM,
		TrigPlusZsquaredFractal, julia_per_pixel, JuliafnPlusZsqrdSetup,
                                                         StandardFractal,
		STDBAILOUT
	},

	{
	t_manfnpluszsqrd,
		{realz0, imagz0, "", ""},
		{0, 0, 0, 0},
		HT_PICKMJ, HF_MANDFNPLUSZSQRD, TRIG1 + WINFRAC + BAILTEST,
		-2.5f, 1.5f, -1.5f, 1.5f,
		0, FPJULTRIGPLUSZSQRD,   NOFRACTAL, LMANTRIGPLUSZSQRD, XAXIS_NOPARM,
		TrigPlusZsquaredfpFractal, mandelfp_per_pixel, MandelfpSetup,
                                                            StandardFractal,
		STDBAILOUT
	},

	{
	t_julfnpluszsqrd,
		{realparm, imagparm, "", ""},
		{-0.5, 0.5, 0, 0},
		HT_PICKMJ, HF_JULFNPLUSZSQRD, TRIG1 + WINFRAC + OKJB + BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, FPMANTRIGPLUSZSQRD, LJULTRIGPLUSZSQRD, NOSYM,
		TrigPlusZsquaredfpFractal, juliafp_per_pixel, JuliafnPlusZsqrdSetup,
                                                            StandardFractal,
		STDBAILOUT
	},

	{
	t_lambdafn,
		{realparm, imagparm, "", ""},
		{1.0, 0.4, 0, 0},
		HT_LAMBDAFN, HF_LAMBDAFN, TRIG1 + WINFRAC + OKJB,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, MANDELTRIGFP, LAMBDATRIG, PI_SYM,
		LambdaTrigfpFractal, otherjuliafp_per_pixel, LambdaTrigSetup,
                                                          StandardFractal,
		LTRIGBAILOUT
	},

	{
	t_mandelfn + 1,
		{realz0, imagz0, "", ""},
		{0, 0, 0, 0},
		HT_MANDFN, HF_MANDFN, TRIG1 + WINFRAC,
		-8.0f, 8.0f, -6.0f, 6.0f,
		16, LAMBDATRIG, NOFRACTAL, MANDELTRIGFP, XYAXIS_NOPARM,
		LambdaTrigFractal, long_mandel_per_pixel, MandelTrigSetup,
                                                       StandardFractal,
		LTRIGBAILOUT
	},

	{
	t_manzpower + 1,
		{realz0, imagz0, exponent, imexponent},
		{0, 0, 2, 0},
		HT_PICKMJ, HF_MANZPOWER, WINFRAC + BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		1, LJULIAZPOWER, NOFRACTAL, FPMANDELZPOWER, XAXIS_NOIMAG,
		longZpowerFractal, long_mandel_per_pixel, MandellongSetup,
                                                       StandardFractal,
		STDBAILOUT
	},

	{
	t_julzpower + 1,
		{realparm, imagparm, exponent, imexponent},
		{0.3, 0.6, 2, 0},
		HT_PICKMJ, HF_JULZPOWER, WINFRAC + OKJB + BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		1, NOFRACTAL, LMANDELZPOWER, FPJULIAZPOWER, ORIGIN,
		longZpowerFractal, long_julia_per_pixel, JulialongSetup,
                                                       StandardFractal,
		STDBAILOUT
	},

	{
	t_manzpower,
		{realz0, imagz0, exponent, imexponent},
		{0, 0, 2, 0},
		HT_PICKMJ, HF_MANZPOWER, WINFRAC + BAILTEST + BF_MATH,
		-2.5f, 1.5f, -1.5f, 1.5f,
		0, FPJULIAZPOWER, NOFRACTAL, LMANDELZPOWER, XAXIS_NOIMAG,
		floatZpowerFractal, othermandelfp_per_pixel, MandelfpSetup,
                                                       StandardFractal,
		STDBAILOUT
	},

	{
	t_julzpower,
		{realparm, imagparm, exponent, imexponent},
		{0.3, 0.6, 2, 0},
		HT_PICKMJ, HF_JULZPOWER, WINFRAC + OKJB + BAILTEST + BF_MATH,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, FPMANDELZPOWER, LJULIAZPOWER, ORIGIN,
		floatZpowerFractal, otherjuliafp_per_pixel, JuliafpSetup,
                                                       StandardFractal,
		STDBAILOUT
	},

	{
	"manzzpwr",
		{realz0, imagz0, exponent, ""},
		{0, 0, 2, 0},
		HT_PICKMJ, HF_MANZZPWR, WINFRAC + BAILTEST,
		-2.5f, 1.5f, -1.5f, 1.5f,
		0, FPJULZTOZPLUSZPWR, NOFRACTAL, NOFRACTAL, XAXIS_NOPARM,
		floatZtozPluszpwrFractal, othermandelfp_per_pixel, MandelfpSetup,
                                                          StandardFractal,
		STDBAILOUT
	},

	{
	"julzzpwr",
		{realparm, imagparm, exponent, ""},
		{-0.3, 0.3, 2, 0},
		HT_PICKMJ, HF_JULZZPWR, WINFRAC + OKJB + BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, FPMANZTOZPLUSZPWR, NOFRACTAL, NOSYM,
		floatZtozPluszpwrFractal, otherjuliafp_per_pixel, JuliafpSetup,
                                                          StandardFractal,
		STDBAILOUT
	},

	{
	t_manfnplusexp + 1,
		{realz0, imagz0, "", ""},
		{0, 0, 0, 0},
		HT_PICKMJ, HF_MANDFNPLUSEXP, TRIG1 + WINFRAC + BAILTEST,
		-8.0f, 8.0f, -6.0f, 6.0f,
		16, LJULTRIGPLUSEXP, NOFRACTAL, FPMANTRIGPLUSEXP, XAXIS_NOPARM,
		LongTrigPlusExponentFractal, long_mandel_per_pixel, MandellongSetup,
                                                            StandardFractal,
		STDBAILOUT
	},

	{
	t_julfnplusexp + 1,
		{realparm, imagparm, "", ""},
		{0, 0, 0, 0},
		HT_PICKMJ, HF_JULFNPLUSEXP, TRIG1 + WINFRAC + OKJB + BAILTEST,
		-4.0f, 4.0f, -3.0f, 3.0f,
		16, NOFRACTAL, LMANTRIGPLUSEXP, FPJULTRIGPLUSEXP, NOSYM,
		LongTrigPlusExponentFractal, long_julia_per_pixel, JulialongSetup,
                                                           StandardFractal,
		STDBAILOUT
	},

	{
	t_manfnplusexp,
		{realz0, imagz0, "", ""},
		{0, 0, 0, 0},
		HT_PICKMJ, HF_MANDFNPLUSEXP, TRIG1 + WINFRAC + BAILTEST,
		-8.0f, 8.0f, -6.0f, 6.0f,
		0, FPJULTRIGPLUSEXP, NOFRACTAL, LMANTRIGPLUSEXP, XAXIS_NOPARM,
		FloatTrigPlusExponentFractal, othermandelfp_per_pixel, MandelfpSetup,
                                                             StandardFractal,
		STDBAILOUT
	},

	{
	t_julfnplusexp,
		{realparm, imagparm, "", ""},
		{0, 0, 0, 0},
		HT_PICKMJ, HF_JULFNPLUSEXP, TRIG1 + WINFRAC + OKJB + BAILTEST,
		-4.0f, 4.0f, -3.0f, 3.0f,
		0, NOFRACTAL, FPMANTRIGPLUSEXP, LJULTRIGPLUSEXP, NOSYM,
		FloatTrigPlusExponentFractal, otherjuliafp_per_pixel, JuliafpSetup,
                                                           StandardFractal,
		STDBAILOUT
	},

	{
	t_popcorn,
		{step_x, step_y, constant_x, constant_y},
		{0.05, 0, 3.00, 0},
		HT_POPCORN, HF_POPCORN, NOGUESS + NOTRACE + WINFRAC + TRIG4,
		-3.0f, 3.0f, -2.25f, 2.25f,
		0, NOFRACTAL, NOFRACTAL, LPOPCORN, NOPLOT,
		PopcornFractalFn, otherjuliafp_per_pixel, JuliafpSetup, popcorn,
		STDBAILOUT
	},

	{
	t_popcorn + 1,
		{step_x, step_y, constant_x, constant_y},
		{0.05, 0, 3.00, 0},
		HT_POPCORN, HF_POPCORN, NOGUESS + NOTRACE + WINFRAC + TRIG4,
		-3.0f, 3.0f, -2.25f, 2.25f,
		16, NOFRACTAL, NOFRACTAL, FPPOPCORN, NOPLOT,
		LPopcornFractalFn, long_julia_per_pixel, JulialongSetup, popcorn,
		STDBAILOUT
	},

	{
	t_lorenz,
		{timestep, "a", "b", "c"},
		{.02, 5, 15, 1},
		HT_LORENZ, HF_LORENZ, NOGUESS + NOTRACE + INFCALC + WINFRAC,
		-15.0f, 15.0f, 0.0f, 30.0f,
		0, NOFRACTAL, NOFRACTAL, LLORENZ, NOSYM,
		(VF)lorenz3dfloatorbit, NULL, orbit3dfloatsetup, orbit2dfloat,
		NOBAILOUT
	},

	{
	t_lorenz + 1,
		{timestep, "a", "b", "c"},
		{.02, 5, 15, 1},
		HT_LORENZ, HF_LORENZ, NOGUESS + NOTRACE + INFCALC + WINFRAC,
		-15.0f, 15.0f, 0.0f, 30.0f,
		16, NOFRACTAL, NOFRACTAL, FPLORENZ, NOSYM,
		(VF)lorenz3dlongorbit, NULL, orbit3dlongsetup, orbit2dlong,
		NOBAILOUT
	},

	{
	t_lorenz3d + 1,
		{timestep, "a", "b", "c"},
		{.02, 5, 15, 1},
		HT_LORENZ, HF_LORENZ, NOGUESS + NOTRACE + NORESUME + WINFRAC + PARMS3D + INFCALC,
		-30.0f, 30.0f, -30.0f, 30.0f,
		16, NOFRACTAL, NOFRACTAL, FPLORENZ3D, NOSYM,
		(VF)lorenz3dlongorbit, NULL, orbit3dlongsetup, orbit3dlong,
		NOBAILOUT
	},

	{
	t_newton + 1,
		{newtdegree, "", "", ""},
		{3, 0, 0, 0},
		HT_NEWT, HF_NEWT, WINFRAC,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, NOFRACTAL, NEWTON, XAXIS,
		MPCNewtonFractal, MPCjulia_per_pixel, NewtonSetup, StandardFractal,
		NOBAILOUT
	},

	{
	t_newtbasin + 1,
		{newtdegree, "Enter non-zero value for stripes", "", ""},
		{3, 0, 0, 0},
		HT_NEWTBAS, HF_NEWTBAS, WINFRAC,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, NOFRACTAL, NEWTBASIN, NOSYM,
		MPCNewtonFractal, MPCjulia_per_pixel, NewtonSetup, StandardFractal,
		NOBAILOUT
	},

	{
	"complexnewton",
		{"Real part of Degree", "Imag part of Degree", "Real part of Root", "Imag part of Root"},
		{3, 0, 1, 0},
		HT_NEWTCMPLX, HF_COMPLEXNEWT, WINFRAC,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
		ComplexNewton, otherjuliafp_per_pixel, ComplexNewtonSetup,
                                                     StandardFractal,
		NOBAILOUT
	},

	{
	"complexbasin",
		{"Real part of Degree", "Imag part of Degree", "Real part of Root", "Imag part of Root"},
		{3, 0, 1, 0},
		HT_NEWTCMPLX, HF_COMPLEXNEWT, WINFRAC,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
		ComplexBasin, otherjuliafp_per_pixel, ComplexNewtonSetup,
                                                     StandardFractal,
		NOBAILOUT
	},

	{
	"cmplxmarksmand",
		{realz0, imagz0, exponent, imexponent},
		{0, 0, 1, 0},
		HT_MARKS, HF_CMPLXMARKSMAND, WINFRAC + BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, COMPLEXMARKSJUL, NOFRACTAL, NOFRACTAL, NOSYM,
		MarksCplxMand, MarksCplxMandperp, MandelfpSetup, StandardFractal,
		STDBAILOUT
	},

	{
	"cmplxmarksjul",
		{realparm, imagparm, exponent, imexponent},
		{0.3, 0.6, 1, 0},
		HT_MARKS, HF_CMPLXMARKSJUL, WINFRAC + BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, COMPLEXMARKSMAND, NOFRACTAL, NOSYM,
		MarksCplxMand, juliafp_per_pixel, JuliafpSetup, StandardFractal,
		STDBAILOUT
	},

	{
	t_formula + 1,
		{p1real, p1imag, p2real, p2imag},
		{0, 0, 0, 0},
		HT_FORMULA, -2, WINFRAC + MORE,
		-2.0f, 2.0f, -1.5f, 1.5f,
		1, NOFRACTAL, NOFRACTAL, FFORMULA, SETUP_SYM,
		Formula, form_per_pixel, intFormulaSetup, StandardFractal,
		0
	},

	{
	t_formula,
		{p1real, p1imag, p2real, p2imag},
		{0, 0, 0, 0},
		HT_FORMULA, -2, WINFRAC + MORE,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, NOFRACTAL, FORMULA, SETUP_SYM,
		Formula, form_per_pixel, fpFormulaSetup, StandardFractal,
		0
	},

	{
	t_sierpinski,
		{"", "", "", ""},
		{0, 0, 0, 0},
		HT_SIER, HF_SIER, WINFRAC,
		-4.0f/3.0f, 96.0f/45.0f, -0.9f, 1.7f,
		0, NOFRACTAL, NOFRACTAL, SIERPINSKI, NOSYM,
		SierpinskiFPFractal, otherjuliafp_per_pixel, SierpinskiFPSetup,
                                                       StandardFractal,
		127
	},

	{
	t_lambda,
		{realparm, imagparm, "", ""},
		{0.85, 0.6, 0, 0},
		HT_LAMBDA, HF_LAMBDA, WINFRAC + OKJB + BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, MANDELLAMBDAFP, LAMBDA, NOSYM,
		LambdaFPFractal, juliafp_per_pixel, JuliafpSetup, StandardFractal,
		STDBAILOUT
	},

	{
	t_barnsleym1,
		{realz0, imagz0, "", ""},
		{0, 0, 0, 0},
		HT_BARNS, HF_BARNSM1, WINFRAC + BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, BARNSLEYJ1FP, NOFRACTAL, BARNSLEYM1, XYAXIS_NOPARM,
		Barnsley1FPFractal, othermandelfp_per_pixel, MandelfpSetup,
                                                       StandardFractal,
		STDBAILOUT
	},

	{
	t_barnsleyj1,
		{realparm, imagparm, "", ""},
		{0.6, 1.1, 0, 0},
		HT_BARNS, HF_BARNSJ1, WINFRAC + OKJB + BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, BARNSLEYM1FP, BARNSLEYJ1, ORIGIN,
		Barnsley1FPFractal, otherjuliafp_per_pixel, JuliafpSetup,
                                                     StandardFractal,
		STDBAILOUT
	},

	{
	t_barnsleym2,
		{realz0, imagz0, "", ""},
		{0, 0, 0, 0},
		HT_BARNS, HF_BARNSM2, WINFRAC + BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, BARNSLEYJ2FP, NOFRACTAL, BARNSLEYM2, YAXIS_NOPARM,
		Barnsley2FPFractal, othermandelfp_per_pixel, MandelfpSetup,
                                                        StandardFractal,
		STDBAILOUT
	},

	{
	t_barnsleyj2,
		{realparm, imagparm, "", ""},
		{0.6, 1.1, 0, 0},
		HT_BARNS, HF_BARNSJ2, WINFRAC + OKJB + BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, BARNSLEYM2FP, BARNSLEYJ2, ORIGIN,
		Barnsley2FPFractal, otherjuliafp_per_pixel, JuliafpSetup,
                                                       StandardFractal,
		STDBAILOUT
	},

	{
	t_barnsleym3,
		{realz0, imagz0, "", ""},
		{0, 0, 0, 0},
		HT_BARNS, HF_BARNSM3, WINFRAC + BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, BARNSLEYJ3FP, NOFRACTAL, BARNSLEYM3, XAXIS_NOPARM,
		Barnsley3FPFractal, othermandelfp_per_pixel, MandelfpSetup,
                                                      StandardFractal,
		STDBAILOUT
	},

	{
	t_barnsleyj3,
		{realparm, imagparm, "", ""},
		{0.6, 1.1, 0, 0},
		HT_BARNS, HF_BARNSJ3, WINFRAC + OKJB + BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, BARNSLEYM3FP, BARNSLEYJ3, NOSYM,
		Barnsley3FPFractal, otherjuliafp_per_pixel, JuliafpSetup,
                                                    StandardFractal,
		STDBAILOUT
	},

	{
	t_mandellambda,
		{realz0, imagz0, "", ""},
		{0, 0, 0, 0},
		HT_MLAMBDA, HF_MLAMBDA, WINFRAC + BAILTEST,
		-3.0f, 5.0f, -3.0f, 3.0f,
		0, LAMBDAFP, NOFRACTAL, MANDELLAMBDA, XAXIS_NOPARM,
		LambdaFPFractal, mandelfp_per_pixel, MandelfpSetup, StandardFractal,
		STDBAILOUT
	},

	{
	t_julibrot + 1,
		{"", "", "", ""},
		{0, 0, 0, 0},
		HT_JULIBROT, -1, NOGUESS + NOTRACE + NOROTATE + NORESUME + WINFRAC,
		-2.0f, 2.0f, -1.5f, 1.5f,
		1, NOFRACTAL, NOFRACTAL, JULIBROTFP, NOSYM,
		JuliaFractal, jb_per_pixel, JulibrotSetup, Std4dFractal,
		STDBAILOUT
	},

	{
	t_lorenz3d,
		{timestep, "a", "b", "c"},
		{.02, 5, 15, 1},
		HT_LORENZ, HF_LORENZ, NOGUESS + NOTRACE + NORESUME + WINFRAC + PARMS3D + INFCALC,
		-30.0f, 30.0f, -30.0f, 30.0f,
		0, NOFRACTAL, NOFRACTAL, LLORENZ3D, NOSYM,
		(VF)lorenz3dfloatorbit, NULL, orbit3dfloatsetup, orbit3dfloat,
		NOBAILOUT
	},

	{
	t_rossler3d + 1,
		{timestep, "a", "b", "c"},
		{.04, .2, .2, 5.7},
		HT_ROSS, HF_ROSS, NOGUESS + NOTRACE + NORESUME + WINFRAC + PARMS3D + INFCALC,
		-30.0f, 30.0f, -20.0f, 40.0f,
		16, NOFRACTAL, NOFRACTAL, FPROSSLER, NOSYM,
		(VF)rosslerlongorbit, NULL, orbit3dlongsetup, orbit3dlong,
		NOBAILOUT
	},

	{
	t_rossler3d,
		{timestep, "a", "b", "c"},
		{.04, .2, .2, 5.7},
		HT_ROSS, HF_ROSS, NOGUESS + NOTRACE + NORESUME + WINFRAC + PARMS3D + INFCALC,
		-30.0f, 30.0f, -20.0f, 40.0f,
		0, NOFRACTAL, NOFRACTAL, LROSSLER, NOSYM,
		(VF)rosslerfloatorbit, NULL, orbit3dfloatsetup, orbit3dfloat,
		NOBAILOUT
	},

	{
	t_henon + 1,
		{"a", "b", "", ""},
		{1.4, .3, 0, 0},
		HT_HENON, HF_HENON, NOGUESS + NOTRACE + INFCALC + WINFRAC,
		-1.4f, 1.4f, -.5f, .5f,
		16, NOFRACTAL, NOFRACTAL, FPHENON, NOSYM,
		(VF)henonlongorbit, NULL, orbit3dlongsetup, orbit2dlong,
		NOBAILOUT
	},

	{
	t_henon,
		{"a", "b", "", ""},
		{1.4, .3, 0, 0},
		HT_HENON, HF_HENON, NOGUESS + NOTRACE + INFCALC + WINFRAC,
		-1.4f, 1.4f, -.5f, .5f,
		0, NOFRACTAL, NOFRACTAL, LHENON, NOSYM,
		(VF)henonfloatorbit, NULL, orbit3dfloatsetup, orbit2dfloat,
		NOBAILOUT
	},

	{
	"pickover",
		{"a", "b", "c", "d"},
		{2.24, .43, -.65, -2.43},
		HT_PICK, HF_PICKOVER, NOGUESS + NOTRACE + NORESUME + WINFRAC + PARMS3D,
		-8.0f/3.0f, 8.0f/3.0f, -2.0f, 2.0f,
		0, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
		(VF)pickoverfloatorbit, NULL, orbit3dfloatsetup, orbit3dfloat,
		NOBAILOUT
	},

	{
	"gingerbreadman",
		{"Initial x", "Initial y", "", ""},
		{-.1, 0, 0, 0},
		HT_GINGER, HF_GINGER, NOGUESS + NOTRACE + INFCALC + WINFRAC,
		-4.5f, 8.5f, -4.5f, 8.5f,
		0, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
		(VF)gingerbreadfloatorbit, NULL, orbit3dfloatsetup, orbit2dfloat,
		NOBAILOUT
	},

	{
	"diffusion",
		{"+Border size",
       "+Type (0=Central, 1=Falling, 2=Square Cavity)",
       "+Color change rate (0=Random)",
		""
		},
		{10, 0, 0, 0},
		HT_DIFFUS, HF_DIFFUS, NOZOOM + NOGUESS + NOTRACE + WINFRAC,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
		NULL, NULL, StandaloneSetup, diffusion,
		NOBAILOUT
	},

	{
	t_unity,
		{"", "", "", ""},
		{0, 0, 0, 0},
		HT_UNITY, HF_UNITY, WINFRAC,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, NOFRACTAL, UNITY, XYAXIS,
		UnityfpFractal, otherjuliafp_per_pixel, StandardSetup, StandardFractal,
		NOBAILOUT
	},

	{
	t_spider,
		{realz0, imagz0, "", ""},
		{0, 0, 0, 0},
		HT_SCOTSKIN, HF_SPIDER, WINFRAC + BAILTEST,
		-2.5f, 1.5f, -1.5f, 1.5f,
		0, NOFRACTAL, NOFRACTAL, SPIDER, XAXIS_NOPARM,
		SpiderfpFractal, mandelfp_per_pixel, MandelfpSetup, StandardFractal,
		STDBAILOUT
	},

	{
	t_spider + 1,
		{realz0, imagz0, "", ""},
		{0, 0, 0, 0},
		HT_SCOTSKIN, HF_SPIDER, WINFRAC + BAILTEST,
		-2.5f, 1.5f, -1.5f, 1.5f,
		1, NOFRACTAL, NOFRACTAL, SPIDERFP, XAXIS_NOPARM,
		SpiderFractal, mandel_per_pixel, MandellongSetup, StandardFractal,
		STDBAILOUT
	},

	{
	"tetrate",
		{realz0, imagz0, "", ""},
		{0, 0, 0, 0},
		HT_SCOTSKIN, HF_TETRATE, WINFRAC + BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, NOFRACTAL, NOFRACTAL, XAXIS_NOIMAG,
		TetratefpFractal, othermandelfp_per_pixel, MandelfpSetup,
                                                     StandardFractal,
		STDBAILOUT
	},

	{
	"magnet1m",
		{realz0, imagz0, "", ""},
		{0, 0, 0, 0},
		HT_MAGNET, HF_MAGM1, WINFRAC,
		-4.0f, 4.0f, -3.0f, 3.0f,
		0, MAGNET1J, NOFRACTAL, NOFRACTAL, XAXIS_NOPARM,
		Magnet1Fractal, mandelfp_per_pixel, MandelfpSetup, StandardFractal,
		100
	},

	{
	"magnet1j",
		{realparm, imagparm, "", ""},
		{0, 0, 0, 0},
		HT_MAGNET, HF_MAGJ1, WINFRAC,
		-8.0f, 8.0f, -6.0f, 6.0f,
		0, NOFRACTAL, MAGNET1M, NOFRACTAL, XAXIS_NOIMAG,
		Magnet1Fractal, juliafp_per_pixel, JuliafpSetup, StandardFractal,
		100
	},

	{
	"magnet2m",
		{realz0, imagz0, "", ""},
		{0, 0, 0, 0},
		HT_MAGNET, HF_MAGM2, WINFRAC,
		-1.5f, 3.7f, -1.95f, 1.95f,
		0, MAGNET2J, NOFRACTAL, NOFRACTAL, XAXIS_NOPARM,
		Magnet2Fractal, mandelfp_per_pixel, MandelfpSetup, StandardFractal,
		100
	},

	{
	"magnet2j",
		{realparm, imagparm, "", ""},
		{0, 0, 0, 0},
		HT_MAGNET, HF_MAGJ2, WINFRAC,
		-8.0f, 8.0f, -6.0f, 6.0f,
		0, NOFRACTAL, MAGNET2M, NOFRACTAL, XAXIS_NOIMAG,
		Magnet2Fractal, juliafp_per_pixel, JuliafpSetup, StandardFractal,
		100
	},

	{
	t_bifurcation + 1,
		{filt, seed, "", ""},
		{1000.0, 0.66, 0, 0},
		HT_BIF, HF_BIFURCATION, TRIG1 + NOGUESS + NOTRACE + NOROTATE + WINFRAC,
		1.9f, 3.0f, 0.0f, 1.34f,
		1, NOFRACTAL, NOFRACTAL, BIFURCATION, NOSYM,
		LongBifurcVerhulstTrig, NULL, StandaloneSetup, Bifurcation,
		NOBAILOUT
	},

	{
	t_biflambda + 1,
		{filt, seed, "", ""},
		{1000.0, 0.66, 0, 0},
		HT_BIF, HF_BIFLAMBDA, TRIG1 + NOGUESS + NOTRACE + NOROTATE + WINFRAC,
		-2.0f, 4.0f, -1.0f, 2.0f,
		1, NOFRACTAL, NOFRACTAL, BIFLAMBDA, NOSYM,
		LongBifurcLambdaTrig, NULL, StandaloneSetup, Bifurcation,
		NOBAILOUT
	},

	{
	t_biflambda,
		{filt, seed, "", ""},
		{1000.0, 0.66, 0, 0},
		HT_BIF, HF_BIFLAMBDA, TRIG1 + NOGUESS + NOTRACE + NOROTATE + WINFRAC,
		-2.0f, 4.0f, -1.0f, 2.0f,
		0, NOFRACTAL, NOFRACTAL, LBIFLAMBDA, NOSYM,
		BifurcLambdaTrig, NULL, StandaloneSetup, Bifurcation,
		NOBAILOUT
	},

	{
	t_bifplussinpi,
		{filt, seed, "", ""},
		{1000.0, 0.66, 0, 0},
		HT_BIF, HF_BIFPLUSSINPI, TRIG1 + NOGUESS + NOTRACE + NOROTATE + WINFRAC,
		0.275f, 1.45f, 0.0f, 2.0f,
		0, NOFRACTAL, NOFRACTAL, LBIFADSINPI, NOSYM,
		BifurcAddTrigPi, NULL, StandaloneSetup, Bifurcation,
		NOBAILOUT
	},

	{
	t_bifeqsinpi,
		{filt, seed, "", ""},
		{1000.0, 0.66, 0, 0},
		HT_BIF, HF_BIFEQSINPI, TRIG1 + NOGUESS + NOTRACE + NOROTATE + WINFRAC,
		-2.5f, 2.5f, -3.5f, 3.5f,
		0, NOFRACTAL, NOFRACTAL, LBIFEQSINPI, NOSYM,
		BifurcSetTrigPi, NULL, StandaloneSetup, Bifurcation,
		NOBAILOUT
	},

	{
	t_popcornjul,
		{step_x, step_y, constant_x, constant_y},
		{0.05, 0, 3.00, 0},
		HT_POPCORN, HF_POPCJUL, WINFRAC + TRIG4,
		-3.0f, 3.0f, -2.25f, 2.25f,
		0, NOFRACTAL, NOFRACTAL, LPOPCORNJUL, NOSYM,
		PopcornFractalFn, otherjuliafp_per_pixel, JuliafpSetup, StandardFractal,
		STDBAILOUT
	},

	{
	t_popcornjul + 1,
		{step_x, step_y, constant_x, constant_y},
		{0.05, 0, 3.0, 0},
		HT_POPCORN, HF_POPCJUL, WINFRAC + TRIG4,
		-3.0f, 3.0f, -2.25f, 2.25f,
		16, NOFRACTAL, NOFRACTAL, FPPOPCORNJUL, NOSYM,
		LPopcornFractalFn, long_julia_per_pixel, JulialongSetup, StandardFractal,
		STDBAILOUT
	},

	{
	"lsystem",
		{" + Order", "", "", ""},
		{2, 0, 0, 0},
		HT_LSYS, -3, NOZOOM + NORESUME + NOGUESS + NOTRACE + WINFRAC,
		-1.0f, 1.0f, -1.0f, 1.0f,
		1, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
		NULL, NULL, StandaloneSetup, Lsystem,
		NOBAILOUT
	},

	{
	t_manowarj,
		{realparm, imagparm, "", ""},
		{0, 0, 0, 0},
		HT_SCOTSKIN, HF_MANOWARJ, WINFRAC + OKJB + BAILTEST,
		-2.5f, 1.5f, -1.5f, 1.5f,
		0, NOFRACTAL, MANOWARFP, MANOWARJ, NOSYM,
		ManOWarfpFractal, juliafp_per_pixel, JuliafpSetup, StandardFractal,
		STDBAILOUT
	},

	{
	t_manowarj + 1,
		{realparm, imagparm, "", ""},
		{0, 0, 0, 0},
		HT_SCOTSKIN, HF_MANOWARJ, WINFRAC + OKJB + BAILTEST,
		-2.5f, 1.5f, -1.5f, 1.5f,
		1, NOFRACTAL, MANOWAR, MANOWARJFP, NOSYM,
		ManOWarFractal, julia_per_pixel, JulialongSetup, StandardFractal,
		STDBAILOUT
	},

	{
	t_fn_z_plusfn_pix_,
		{realz0, imagz0, recoeftrg2, imcoeftrg2},
		{0, 0, 1, 0},
		HT_SCOTSKIN, HF_FNPLUSFNPIX, TRIG2 + WINFRAC + BAILTEST,
		-3.6f, 3.6f, -2.7f, 2.7f,
		0, NOFRACTAL, NOFRACTAL, FNPLUSFNPIXLONG, NOSYM,
		Richard8fpFractal, otherrichard8fp_per_pixel, MandelfpSetup,
                                                       StandardFractal,
		LTRIGBAILOUT
	},

	{
	t_fn_z_plusfn_pix_ + 1,
		{realz0, imagz0, recoeftrg2, imcoeftrg2},
		{0, 0, 1, 0},
		HT_SCOTSKIN, HF_FNPLUSFNPIX, TRIG2 + WINFRAC + BAILTEST,
		-3.6f, 3.6f, -2.7f, 2.7f,
		1, NOFRACTAL, NOFRACTAL, FNPLUSFNPIXFP, NOSYM,
		Richard8Fractal, long_richard8_per_pixel, MandellongSetup,
                                                      StandardFractal,
		LTRIGBAILOUT
	},

	{
	t_marksmandelpwr,
		{realz0, imagz0, "", ""},
		{0, 0, 0, 0},
		HT_MARKS, HF_MARKSMANDPWR, TRIG1 + WINFRAC + BAILTEST,
		-2.5f, 1.5f, -1.5f, 1.5f,
		0, NOFRACTAL, NOFRACTAL, MARKSMANDELPWR, XAXIS_NOPARM,
		MarksMandelPwrfpFractal, marks_mandelpwrfp_per_pixel, MandelfpSetup,
                                                             StandardFractal,
		STDBAILOUT
	},

	{
	t_marksmandelpwr + 1,
		{realz0, imagz0, "", ""},
		{0, 0, 0, 0},
		HT_MARKS, HF_MARKSMANDPWR, TRIG1 + WINFRAC + BAILTEST,
		-2.5f, 1.5f, -1.5f, 1.5f,
		1, NOFRACTAL, NOFRACTAL, MARKSMANDELPWRFP, XAXIS_NOPARM,
		MarksMandelPwrFractal, marks_mandelpwr_per_pixel, MandellongSetup,
                                                          StandardFractal,
		STDBAILOUT
	},

	{
	t_tims_error,
		{realz0, imagz0, "", ""},
		{0, 0, 0, 0},
		HT_MARKS, HF_TIMSERR, WINFRAC + TRIG1 + BAILTEST,
		-2.9f, 4.3f, -2.7f, 2.7f,
		0, NOFRACTAL, NOFRACTAL, TIMSERROR, XAXIS_NOPARM,
		TimsErrorfpFractal, marks_mandelpwrfp_per_pixel, MandelfpSetup,
                                                         StandardFractal,
		STDBAILOUT
	},

	{
	t_tims_error + 1,
		{realz0, imagz0, "", ""},
		{0, 0, 0, 0},
		HT_MARKS, HF_TIMSERR, WINFRAC + TRIG1 + BAILTEST,
		-2.9f, 4.3f, -2.7f, 2.7f,
		1, NOFRACTAL, NOFRACTAL, TIMSERRORFP, XAXIS_NOPARM,
		TimsErrorFractal, marks_mandelpwr_per_pixel, MandellongSetup,
                                                       StandardFractal,
		STDBAILOUT
	},

	{
	t_bifeqsinpi + 1,
		{filt, seed, "", ""},
		{1000.0, 0.66, 0, 0},
		HT_BIF, HF_BIFEQSINPI, TRIG1 + NOGUESS + NOTRACE + NOROTATE + WINFRAC,
		-2.5f, 2.5f, -3.5f, 3.5f,
		1, NOFRACTAL, NOFRACTAL, BIFEQSINPI, NOSYM,
		LongBifurcSetTrigPi, NULL, StandaloneSetup, Bifurcation,
		NOBAILOUT
	},

	{
	t_bifplussinpi + 1,
		{filt, seed, "", ""},
		{1000.0, 0.66, 0, 0},
		HT_BIF, HF_BIFPLUSSINPI, TRIG1 + NOGUESS + NOTRACE + NOROTATE + WINFRAC,
		0.275f, 1.45f, 0.0f, 2.0f,
		1, NOFRACTAL, NOFRACTAL, BIFADSINPI, NOSYM,
		LongBifurcAddTrigPi, NULL, StandaloneSetup, Bifurcation,
		NOBAILOUT
	},

	{
	t_bifstewart,
		{filt, seed, "", ""},
		{1000.0, 0.66, 0, 0},
		HT_BIF, HF_BIFSTEWART, TRIG1 + NOGUESS + NOTRACE + NOROTATE + WINFRAC,
		0.7f, 2.0f, -1.1f, 1.1f,
		0, NOFRACTAL, NOFRACTAL, LBIFSTEWART, NOSYM,
		BifurcStewartTrig, NULL, StandaloneSetup, Bifurcation,
		NOBAILOUT
	},

	{
	t_bifstewart + 1,
		{filt, seed, "", ""},
		{1000.0, 0.66, 0, 0},
		HT_BIF, HF_BIFSTEWART, TRIG1 + NOGUESS + NOTRACE + NOROTATE + WINFRAC,
		0.7f, 2.0f, -1.1f, 1.1f,
		1, NOFRACTAL, NOFRACTAL, BIFSTEWART, NOSYM,
		LongBifurcStewartTrig, NULL, StandaloneSetup, Bifurcation,
		NOBAILOUT
	},

	{
	"hopalong",
		{"a", "b", "c", ""},
		{.4, 1, 0, 0},
		HT_MARTIN, HF_HOPALONG, NOGUESS + NOTRACE + INFCALC + WINFRAC,
		-2.0f, 3.0f, -1.625f, 2.625f,
		0, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
		(VF)hopalong2dfloatorbit, NULL, orbit3dfloatsetup, orbit2dfloat,
		NOBAILOUT
	},

	{
	"circle",
		{"magnification", "", "", ""},
		{200000L, 0, 0, 0},
		HT_CIRCLE, HF_CIRCLE, WINFRAC,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, NOFRACTAL, NOFRACTAL, XYAXIS,
		CirclefpFractal, juliafp_per_pixel, JuliafpSetup, StandardFractal,
		NOBAILOUT
	},

	{
	"martin",
		{"a", "", "", ""},
		{3.14, 0, 0, 0},
		HT_MARTIN, HF_MARTIN, NOGUESS + NOTRACE + INFCALC + WINFRAC,
		-32.0f, 32.0f, -24.0f, 24.0f,
		0, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
		(VF)martin2dfloatorbit, NULL, orbit3dfloatsetup, orbit2dfloat,
		NOBAILOUT
	},

	{
	"lyapunov",
		{"+Order (integer)", "Population Seed", "+Filter Cycles", ""},
		{0, 0.5, 0, 0},
		HT_LYAPUNOV, HT_LYAPUNOV, WINFRAC,
		-8.0f, 8.0f, -6.0f, 6.0f,
		0, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
		BifurcLambda, NULL, lya_setup, lyapunov,
		NOBAILOUT
	},

	{
	"lorenz3d1",
		{timestep, "a", "b", "c"},
		{.02, 5, 15, 1},
		HT_LORENZ, HF_LORENZ3D1,
								NOGUESS + NOTRACE + NORESUME + WINFRAC + PARMS3D + INFCALC,
		-30.0f, 30.0f, -30.0f, 30.0f,
		0, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
		(VF)lorenz3d1floatorbit, NULL, orbit3dfloatsetup, orbit3dfloat,
		NOBAILOUT
	},

	{
	"lorenz3d3",
		{timestep, "a", "b", "c"},
		{.02, 10, 28, 2.66},
		HT_LORENZ, HF_LORENZ3D3,
                      NOGUESS + NOTRACE + NORESUME + WINFRAC + PARMS3D + INFCALC,
		-30.0f, 30.0f, -30.0f, 30.0f,
		0, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
		(VF)lorenz3d3floatorbit, NULL, orbit3dfloatsetup, orbit3dfloat,
		NOBAILOUT
	},

	{
	"lorenz3d4",
		{timestep, "a", "b", "c"},
		{.02, 10, 28, 2.66},
		HT_LORENZ, HF_LORENZ3D4,
                       NOGUESS + NOTRACE + NORESUME + WINFRAC + PARMS3D + INFCALC,
		-30.0f, 30.0f, -30.0f, 30.0f,
		0, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
		(VF)lorenz3d4floatorbit, NULL, orbit3dfloatsetup, orbit3dfloat,
		NOBAILOUT
	},

	{
	t_lambda_fnorfn_ + 1,
		{realparm, imagparm, "Function Shift Value", ""},
		{1, 0.1, 1, 0},
		HT_FNORFN, HF_LAMBDAFNFN, TRIG2 + WINFRAC + BAILTEST,
		-4.0f, 4.0f, -3.0f, 3.0f,
		16, NOFRACTAL, LMANLAMFNFN, FPLAMBDAFNFN, ORIGIN,
		LambdaTrigOrTrigFractal, long_julia_per_pixel, LambdaTrigOrTrigSetup,
                                                             StandardFractal,
		LTRIGBAILOUT
	},

	{
	t_lambda_fnorfn_,
		{realparm, imagparm, "Function Shift Value", ""},
		{1, 0.1, 1, 0},
		HT_FNORFN, HF_LAMBDAFNFN, TRIG2 + WINFRAC + BAILTEST,
		-4.0f, 4.0f, -3.0f, 3.0f,
		0, NOFRACTAL, FPMANLAMFNFN, LLAMBDAFNFN, ORIGIN,
		LambdaTrigOrTrigfpFractal, otherjuliafp_per_pixel,
                                      LambdaTrigOrTrigSetup, StandardFractal,
		LTRIGBAILOUT
	},

	{
	t_julia_fnorfn_ + 1,
		{realparm, imagparm, "Function Shift Value", ""},
		{0, 0, 8, 0},
		HT_FNORFN, HF_JULIAFNFN, TRIG2 + WINFRAC + BAILTEST,
		-4.0f, 4.0f, -3.0f, 3.0f,
		16, NOFRACTAL, LMANFNFN, FPJULFNFN, XAXIS,
		JuliaTrigOrTrigFractal, long_julia_per_pixel, JuliaTrigOrTrigSetup,
                                                           StandardFractal,
		LTRIGBAILOUT
	},

	{
	t_julia_fnorfn_,
		{realparm, imagparm, "Function Shift Value", ""},
		{0, 0, 8, 0},
		HT_FNORFN, HF_JULIAFNFN, TRIG2 + WINFRAC + BAILTEST,
		-4.0f, 4.0f, -3.0f, 3.0f,
		0, NOFRACTAL, FPMANFNFN, LJULFNFN, XAXIS,
		JuliaTrigOrTrigfpFractal, otherjuliafp_per_pixel,
                                    JuliaTrigOrTrigSetup, StandardFractal,
		LTRIGBAILOUT
	},

	{
	t_manlam_fnorfn_ + 1,
		{realz0, imagz0, "Function Shift Value", ""},
		{0, 0, 10, 0},
		HT_FNORFN, HF_MANLAMFNFN, TRIG2 + WINFRAC + BAILTEST,
		-4.0f, 4.0f, -3.0f, 3.0f,
		16, LLAMBDAFNFN, NOFRACTAL, FPMANLAMFNFN, XAXIS_NOPARM,
		LambdaTrigOrTrigFractal, long_mandel_per_pixel,
                                      ManlamTrigOrTrigSetup, StandardFractal,
		LTRIGBAILOUT
	},

	{
	t_manlam_fnorfn_,
		{realz0, imagz0, "Function Shift Value", ""},
		{0, 0, 10, 0},
		HT_FNORFN, HF_MANLAMFNFN, TRIG2 + WINFRAC + BAILTEST,
		-4.0f, 4.0f, -3.0f, 3.0f,
		0, FPLAMBDAFNFN, NOFRACTAL, LMANLAMFNFN, XAXIS_NOPARM,
		LambdaTrigOrTrigfpFractal, othermandelfp_per_pixel,
                                     ManlamTrigOrTrigSetup, StandardFractal,
		LTRIGBAILOUT
	},

	{
	t_mandel_fnorfn_ + 1,
		{realz0, imagz0, "Function Shift Value", ""},
		{0, 0, 0.5, 0},
		HT_FNORFN, HF_MANDELFNFN, TRIG2 + WINFRAC + BAILTEST,
		-4.0f, 4.0f, -3.0f, 3.0f,
		16, LJULFNFN, NOFRACTAL, FPMANFNFN, XAXIS_NOPARM,
		JuliaTrigOrTrigFractal, long_mandel_per_pixel,
                                   MandelTrigOrTrigSetup, StandardFractal,
		LTRIGBAILOUT
	},

	{
	t_mandel_fnorfn_,
		{realz0, imagz0, "Function Shift Value", ""},
		{0, 0, 0.5, 0},
		HT_FNORFN, HF_MANDELFNFN, TRIG2 + WINFRAC + BAILTEST,
		-4.0f, 4.0f, -3.0f, 3.0f,
		0, FPJULFNFN, NOFRACTAL, LMANFNFN, XAXIS_NOPARM,
		JuliaTrigOrTrigfpFractal, othermandelfp_per_pixel,
                                    MandelTrigOrTrigSetup, StandardFractal,
		LTRIGBAILOUT
	},

	{
	t_bifmay + 1,
		{filt, seed, "Beta >= 2", ""},
		{300.0, 0.9, 5, 0},
		HT_BIF, HF_BIFMAY, NOGUESS + NOTRACE + NOROTATE + WINFRAC,
		-3.5f, -0.9f, -0.5f, 3.2f,
		16, NOFRACTAL, NOFRACTAL, BIFMAY, NOSYM,
		LongBifurcMay, NULL, BifurcMaySetup, Bifurcation,
		NOBAILOUT
	},

	{
	t_bifmay,
		{filt, seed, "Beta >= 2", ""},
		{300.0, 0.9, 5, 0},
		HT_BIF, HF_BIFMAY, NOGUESS + NOTRACE + NOROTATE + WINFRAC,
		-3.5f, -0.9f, -0.5f, 3.2f,
		0, NOFRACTAL, NOFRACTAL, LBIFMAY, NOSYM,
		BifurcMay, NULL, BifurcMaySetup, Bifurcation,
		NOBAILOUT
	},

	{
	t_halley + 1,
		{order, real_relax, epsilon, imag_relax},
		{6, 1.0, 0.0001, 0},
		HT_HALLEY, HF_HALLEY, WINFRAC,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, NOFRACTAL, HALLEY, XYAXIS,
		MPCHalleyFractal, MPCHalley_per_pixel, HalleySetup, StandardFractal,
		NOBAILOUT
	},

	{
	t_halley,
		{order, real_relax, epsilon, imag_relax},
		{6, 1.0, 0.0001, 0},
		HT_HALLEY, HF_HALLEY, WINFRAC,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, NOFRACTAL, MPHALLEY, XYAXIS,
		HalleyFractal, Halley_per_pixel, HalleySetup, StandardFractal,
		NOBAILOUT
	},

	{
	"dynamic",
		{"+# of intervals (<0 = connect)", "time step (<0 = Euler)", "a", "b"},
		{50, .1, 1, 3},
		HT_DYNAM, HF_DYNAM, NOGUESS + NOTRACE + WINFRAC + TRIG1,
		-20.0f, 20.0f, -20.0f, 20.0f,
		0, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
		(VF)dynamfloat, NULL, dynam2dfloatsetup, dynam2dfloat,
		NOBAILOUT
	},

	{
	"quat",
		{"notused", "notused", "cj", "ck"},
		{0, 0, 0, 0},
		HT_QUAT, HF_QUAT, WINFRAC + OKJB,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, QUATJULFP, NOFRACTAL, NOFRACTAL, XAXIS,
		QuaternionFPFractal, quaternionfp_per_pixel, MandelfpSetup,
                                                       StandardFractal,
		LTRIGBAILOUT
	},

	{
	"quatjul",
		{"c1", "ci", "cj", "ck"},
		{-.745, 0, .113, .05},
		HT_QUAT, HF_QUATJ, WINFRAC + OKJB + MORE,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, QUATFP, NOFRACTAL, ORIGIN,
		QuaternionFPFractal, quaternionjulfp_per_pixel, JuliafpSetup,
                                                        StandardFractal,
		LTRIGBAILOUT
	},

	{
	"cellular",
		{cell_init, cell_rule, cell_type, cell_strt},
		{11.0, 3311100320.0, 41.0, 0},
		HT_CELLULAR, HF_CELLULAR, NOGUESS + NOTRACE + NOZOOM + WINFRAC,
		-1.0f, 1.0f, -1.0f, 1.0f,
		0, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
		NULL, NULL, CellularSetup, cellular,
		NOBAILOUT
	},

	{
	t_julibrot,
		{"", "", "", ""},
		{0, 0, 0, 0},
		HT_JULIBROT, -1, NOGUESS + NOTRACE + NOROTATE + NORESUME + WINFRAC,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, NOFRACTAL, JULIBROT, NOSYM,
		JuliafpFractal, jbfp_per_pixel, JulibrotSetup, Std4dfpFractal,
		STDBAILOUT
	},

#ifdef RANDOM_RUN
	{
	t_julia_inverse + 1,
		{realparm, imagparm, s_maxhits, "Random Run Interval"},
		{-0.11, 0.6557, 4, 1024},
		HT_INVERSE, HF_INVERSE, NOGUESS + NOTRACE + INFCALC + NORESUME,
		-2.0f, 2.0f, -1.5f, 1.5f,
		24, NOFRACTAL, MANDEL, INVERSEJULIAFP, NOSYM,
		Linverse_julia_orbit, NULL, orbit3dlongsetup, inverse_julia_per_image,
		NOBAILOUT
	},

	{
	t_julia_inverse,
		{realparm, imagparm, s_maxhits, "Random Run Interval"},
		{-0.11, 0.6557, 4, 1024},
		HT_INVERSE, HF_INVERSE, NOGUESS + NOTRACE + INFCALC + NORESUME,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, MANDEL, INVERSEJULIA, NOSYM,
		Minverse_julia_orbit, NULL, orbit3dfloatsetup, inverse_julia_per_image,
		NOBAILOUT
	},
#else
	{
	t_julia_inverse + 1,
		{realparm, imagparm, s_maxhits, ""},
		{-0.11, 0.6557, 4, 1024},
		HT_INVERSE, HF_INVERSE, NOGUESS + NOTRACE + INFCALC + NORESUME,
		-2.0f, 2.0f, -1.5f, 1.5f,
		24, NOFRACTAL, MANDEL, INVERSEJULIAFP, NOSYM,
		Linverse_julia_orbit, NULL, orbit3dlongsetup, inverse_julia_per_image,
		NOBAILOUT
	},

	{
	t_julia_inverse,
		{realparm, imagparm, s_maxhits, ""},
		{-0.11, 0.6557, 4, 1024},
		HT_INVERSE, HF_INVERSE, NOGUESS + NOTRACE + INFCALC + NORESUME,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, MANDEL, INVERSEJULIA, NOSYM,
		Minverse_julia_orbit, NULL, orbit3dfloatsetup, inverse_julia_per_image,
		NOBAILOUT
	},

#endif

	{
	"mandelcloud",
		{"+# of intervals (<0 = connect)", "", "", ""},
		{50, 0, 0, 0},
		HT_MANDELCLOUD, HF_MANDELCLOUD, NOGUESS + NOTRACE + WINFRAC,
		-2.5f, 1.5f, -1.5f, 1.5f,
		0, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
		(VF)mandelcloudfloat, NULL, dynam2dfloatsetup, dynam2dfloat,
		NOBAILOUT
	},

	{
	t_phoenix + 1,
		{p1real, p2real, degreeZ, ""},
		{0.56667, -0.5, 0, 0},
		HT_PHOENIX, HF_PHOENIX, WINFRAC + BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		1, NOFRACTAL, MANDPHOENIX, PHOENIXFP, XAXIS,
		LongPhoenixFractal, long_phoenix_per_pixel, PhoenixSetup,
                                                      StandardFractal,
		STDBAILOUT
	},

	{
	t_phoenix,
		{p1real, p2real, degreeZ, ""},
		{0.56667, -0.5, 0, 0},
		HT_PHOENIX, HF_PHOENIX, WINFRAC + BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, MANDPHOENIXFP, PHOENIX, XAXIS,
		PhoenixFractal, phoenix_per_pixel, PhoenixSetup, StandardFractal,
		STDBAILOUT
	},

	{
	t_mandphoenix + 1,
		{realz0, imagz0, degreeZ, ""},
		{0.0, 0.0, 0, 0},
		HT_PHOENIX, HF_MANDPHOENIX, WINFRAC + BAILTEST,
		-2.5f, 1.5f, -1.5f, 1.5f,
		1, PHOENIX, NOFRACTAL, MANDPHOENIXFP, NOSYM,
		LongPhoenixFractal, long_mandphoenix_per_pixel, MandPhoenixSetup,
                                                          StandardFractal,
		STDBAILOUT
	},

	{
	t_mandphoenix,
		{realz0, imagz0, degreeZ, ""},
		{0.0, 0.0, 0, 0},
		HT_PHOENIX, HF_MANDPHOENIX, WINFRAC + BAILTEST,
		-2.5f, 1.5f, -1.5f, 1.5f,
		0, PHOENIXFP, NOFRACTAL, MANDPHOENIX, NOSYM,
		PhoenixFractal, mandphoenix_per_pixel, MandPhoenixSetup,
                                                     StandardFractal,
		STDBAILOUT
	},

	{
	"hypercomplex",
		{"notused", "notused", "cj", "ck"},
		{0, 0, 0, 0},
		HT_HYPERC, HF_HYPERC, WINFRAC + OKJB + TRIG1,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, HYPERCMPLXJFP, NOFRACTAL, NOFRACTAL, XAXIS,
		HyperComplexFPFractal, quaternionfp_per_pixel, MandelfpSetup,
                                                        StandardFractal,
		LTRIGBAILOUT
	},

	{
	"hypercomplexj",
		{"c1", "ci", "cj", "ck"},
		{-.745, 0, .113, .05},
		HT_HYPERC, HF_HYPERCJ, WINFRAC + OKJB + TRIG1 + MORE,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, HYPERCMPLXFP, NOFRACTAL, ORIGIN,
		HyperComplexFPFractal, quaternionjulfp_per_pixel, JuliafpSetup,
                                                          StandardFractal,
		LTRIGBAILOUT
	},

	{
	t_frothybasin + 1,
		{frothmapping, frothshade, frothavalue, ""},
		{1, 0, 1.028713768218725, 0},
		HT_FROTH, HF_FROTH, NOTRACE + WINFRAC,
		-2.8f, 2.8f, -2.355f, 1.845f,
		28, NOFRACTAL, NOFRACTAL, FROTHFP, NOSYM,
		froth_per_orbit, froth_per_pixel, froth_setup, calcfroth,
		FROTHBAILOUT
	},

	{
	t_frothybasin,
		{frothmapping, frothshade, frothavalue, ""},
		{1, 0, 1.028713768218725, 0},
		HT_FROTH, HF_FROTH, NOTRACE + WINFRAC,
		-2.8f, 2.8f, -2.355f, 1.845f,
		0, NOFRACTAL, NOFRACTAL, FROTH, NOSYM,
		froth_per_orbit, froth_per_pixel, froth_setup, calcfroth,
		FROTHBAILOUT
	},

	{
	t_mandel4,
		{realz0, imagz0, "", ""},
		{0, 0, 0, 0},
		HT_MANDJUL4, HF_MANDEL4, WINFRAC + BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, JULIA4FP, NOFRACTAL, MANDEL4, XAXIS_NOPARM,
		Mandel4fpFractal, mandelfp_per_pixel, MandelfpSetup, StandardFractal,
		STDBAILOUT
	},

	{
	t_julia4,
		{realparm, imagparm, "", ""},
		{0.6, 0.55, 0, 0},
		HT_MANDJUL4, HF_JULIA4, WINFRAC + OKJB + BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, MANDEL4FP, JULIA4, ORIGIN,
		Mandel4fpFractal, juliafp_per_pixel, JuliafpSetup, StandardFractal,
		STDBAILOUT
	},

	{
	t_marksmandel,
		{realz0, imagz0, exponent, ""},
		{0, 0, 1, 0},
		HT_MARKS, HF_MARKSMAND, WINFRAC + BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, MARKSJULIAFP, NOFRACTAL, MARKSMANDEL, NOSYM,
		MarksLambdafpFractal, marksmandelfp_per_pixel, MandelfpSetup,
                                                        StandardFractal,
		STDBAILOUT
	},

	{
	t_marksjulia,
		{realparm, imagparm, exponent, ""},
		{0.1, 0.9, 1, 0},
		HT_MARKS, HF_MARKSJULIA, WINFRAC + BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, MARKSMANDELFP, MARKSJULIA, ORIGIN,
		MarksLambdafpFractal, juliafp_per_pixel, MarksJuliafpSetup,
                                                      StandardFractal,
		STDBAILOUT
	},

		/* dmf */
	{
	"icons",
		{"Lambda", "Alpha", "Beta", "Gamma"},
		{-2.34, 2.0, 0.2, 0.1},
		HT_ICON, HF_ICON, NOGUESS + NOTRACE + WINFRAC + INFCALC + MORE,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
		(VF)iconfloatorbit, NULL, orbit3dfloatsetup, orbit2dfloat,
		NOBAILOUT
	},

		/* dmf */
	{
	"icons3d",
		{"Lambda", "Alpha", "Beta", "Gamma"},
		{-2.34, 2.0, 0.2, 0.1},
		HT_ICON, HF_ICON, NOGUESS + NOTRACE + WINFRAC + INFCALC + PARMS3D + MORE,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
		(VF)iconfloatorbit, NULL, orbit3dfloatsetup, orbit3dfloat,
		NOBAILOUT
	},

	{
	t_phoenixcplx + 1,
		{p1real, p1imag, p2real, p2imag},
		{0.2, 0, 0.3, 0},
		HT_PHOENIX, HF_PHOENIXCPLX, WINFRAC + MORE + BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		1, NOFRACTAL, MANDPHOENIXCPLX, PHOENIXFPCPLX, ORIGIN,
		LongPhoenixFractalcplx, long_phoenix_per_pixel, PhoenixCplxSetup,
                                                           StandardFractal,
		STDBAILOUT
	},

	{
	t_phoenixcplx,
		{p1real, p1imag, p2real, p2imag},
		{0.2, 0, 0.3, 0},
		HT_PHOENIX, HF_PHOENIXCPLX, WINFRAC + MORE + BAILTEST,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, MANDPHOENIXFPCPLX, PHOENIXCPLX, ORIGIN,
		PhoenixFractalcplx, phoenix_per_pixel, PhoenixCplxSetup,
                                                   StandardFractal,
		STDBAILOUT
	},

	{
	t_mandphoenixcplx + 1,
		{realz0, imagz0, p2real, p2imag},
		{0, 0, 0.5, 0},
		HT_PHOENIX, HF_MANDPHOENIXCPLX, WINFRAC + MORE + BAILTEST,
		-2.5f, 1.5f, -1.5f, 1.5f,
		1, PHOENIXCPLX, NOFRACTAL, MANDPHOENIXFPCPLX, XAXIS,
		LongPhoenixFractalcplx, long_mandphoenix_per_pixel,
                                   MandPhoenixCplxSetup, StandardFractal,
		STDBAILOUT
	},

	{
	t_mandphoenixcplx,
		{realz0, imagz0, p2real, p2imag},
		{0, 0, 0.5, 0},
		HT_PHOENIX, HF_MANDPHOENIXCPLX, WINFRAC + MORE + BAILTEST,
		-2.5f, 1.5f, -1.5f, 1.5f,
		0, PHOENIXFPCPLX, NOFRACTAL, MANDPHOENIXCPLX, XAXIS,
		PhoenixFractalcplx, mandphoenix_per_pixel, MandPhoenixCplxSetup,
                                                          StandardFractal,
		STDBAILOUT
	},

	{
	"ant",
		{"#Rule String (1's and non-1's, 0 rand)",
       "#Maxpts",
       "+Numants (max 256)",
       "+Ant type (1 or 2)"
		},
		{1100, 1.0E9, 1, 1},
		HT_ANT, HF_ANT, WINFRAC + NOZOOM + NOGUESS + NOTRACE + NORESUME + MORE,
		-1.0f, 1.0f, -1.0f, 1.0f,
		0, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
		NULL, NULL, StandaloneSetup, ant,
		NOBAILOUT
	},

	{
	"chip",
		{"a", "b", "c", ""},
		{-15, -19, 1, 0},
		HT_MARTIN, HF_CHIP, NOGUESS + NOTRACE + INFCALC + WINFRAC,
		-760.0f, 760.0f, -570.0f, 570.0f,
		0, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
		(VF)chip2dfloatorbit, NULL, orbit3dfloatsetup, orbit2dfloat,
		NOBAILOUT
	},

	{
	"quadruptwo",
		{"a", "b", "c", ""},
		{34, 1, 5, 0},
		HT_MARTIN, HF_QUADRUPTWO, NOGUESS + NOTRACE + INFCALC + WINFRAC,
		-82.93367f, 112.2749f, -55.76383f, 90.64257f,
		0, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
		(VF)quadruptwo2dfloatorbit, NULL, orbit3dfloatsetup, orbit2dfloat,
		NOBAILOUT
	},

	{
	"threeply",
		{"a", "b", "c", ""},
		{-55, -1, -42, 0},
		HT_MARTIN, HF_THREEPLY, NOGUESS + NOTRACE + INFCALC + WINFRAC,
		-8000.0f, 8000.0f, -6000.0f, 6000.0f,
		0, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
		(VF)threeply2dfloatorbit, NULL, orbit3dfloatsetup, orbit2dfloat,
		NOBAILOUT
	},

	{
	"volterra-lotka",
		{"h", "p", "", ""},
		{0.739, 0.739, 0, 0},
		HT_VL, HF_VL, WINFRAC,
		0.0f, 6.0f, 0.0f, 4.5f, 
		0, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
		VLfpFractal, otherjuliafp_per_pixel, VLSetup, StandardFractal, 
		256
	},

	{
	"escher_julia",
		{realparm, imagparm, "", ""},
		{0.32, 0.043, 0, 0},
		HT_ESCHER, HF_ESCHER, WINFRAC,
		-1.6f, 1.6f, -1.2f, 1.2f,
		0, NOFRACTAL, NOFRACTAL, NOFRACTAL, ORIGIN,
		EscherfpFractal, juliafp_per_pixel, StandardSetup, 
          StandardFractal,
		STDBAILOUT
	},                     
  
/* From Pickovers' "Chaos in Wonderland"      */
/* included by Humberto R. Baptista           */
/* code adapted from king.cpp bt James Rankin */

	{
	"latoocarfian",
		{"a", "b", "c", "d"},
		{-0.966918, 2.879879, 0.765145, 0.744728},
		HT_LATOO, HF_LATOO, NOGUESS + NOTRACE + WINFRAC + INFCALC + MORE + TRIG4,
		-2.0f, 2.0f, -1.5f, 1.5f,
		0, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
		(VF)latoofloatorbit, NULL, orbit3dfloatsetup, orbit2dfloat,
		NOBAILOUT
	},
#if 0
	{
	"mandelbrotmix4",
		{p1real, p1imag, p2real, p2imag},
		{0.05, 3, -1.5, -2},
		HT_MANDELBROTMIX4, HF_MANDELBROTMIX4, WINFRAC + BAILTEST + TRIG1 + MORE,
		-2.5f, 1.5f, -1.5f, 1.5f,
		0, NOFRACTAL, NOFRACTAL, NOFRACTAL, NOSYM,
		MandelbrotMix4fpFractal, MandelbrotMix4fp_per_pixel, MandelbrotMix4Setup, StandardFractal,
		STDBAILOUT
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

int num_fractal_types = (sizeof(fractalspecific)/
		sizeof(struct fractalspecificstuff)) -1;

/*
 *  Returns 1 if the formula parameter is not used in the current
 *  formula.  If the parameter is used, or not a formula fractal,
 *  a 0 is returned.  Note: this routine only works for formula types.
 */
int paramnotused(int parm)
{
	int ret = 0;

	/* sanity check */
	if (fractype != FORMULA && fractype != FFORMULA)
		return 0;

	switch (parm/2) {
		case 0:
			if (!uses_p1)
				ret = 1;
			break;
		case 1:
			if (!uses_p2)
				ret = 1;
			break;
		case 2:
			if (!uses_p3)
				ret = 1;
			break;
		case 3:
			if (!uses_p4)
				ret = 1;
			break;
		case 4:
			if (!uses_p5)
				ret = 1;
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
int typehasparm(int type, int parm, char *buf)
{
	int extra;
	char *ret = NULL;
	if (0 <= parm && parm < 4)
		ret=fractalspecific[type].param[parm];
	else if (parm >= 4 && parm < MAXPARAMS)
		if ((extra=find_extra_param(type)) > -1)
			ret=moreparams[extra].param[parm-4];
	if (ret)
		if (*ret == 0)
			ret = NULL;

	if (type == FORMULA || type == FFORMULA)
		if (paramnotused(parm))
			ret = NULL;

	if (ret && buf != NULL)
		strcpy(buf, ret);
	return ret ? 1 : 0;
}
