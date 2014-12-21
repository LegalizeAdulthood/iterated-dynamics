/*
        This module consists only of the fractalspecific structure
        and a *slew* of defines needed to get it to compile
*/
#include <float.h>
#include <string.h>
#include "port.h"
#include "prototyp.h"
#include "helpdefs.h"
#include "fractype.h"

// functions defined elsewhere needed for fractalspecific
// moved to prototyp.h

// parameter descriptions
// Note: parameters preceded by + are integer parameters
//       parameters preceded by # are U32 parameters

// for Mandelbrots
static char realz0[] = "Real Perturbation of Z(0)";
static char imagz0[] = "Imaginary Perturbation of Z(0)";

// for Julias
static char realparm[] = "Real Part of Parameter";
static char imagparm[] = "Imaginary Part of Parameter";

// for Newtons
static char newtdegree[] = "+Polynomial Degree (>= 2)";

// for MarksMandel/Julia
static char exponent[]   = "Real part of Exponent";
static char imexponent[] = "Imag part of Exponent";

// for Lorenz
static char timestep[]     = "Time Step";

// for formula
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

// trig functions
static char recoeftrg1[] = "Real Coefficient First Function";
static char imcoeftrg1[] = "Imag Coefficient First Function";
static char recoeftrg2[] = "Real Coefficient Second Function";
static char imcoeftrg2[] = "Imag Coefficient Second Function";

// KAM Torus
static char kamangle[] = "Angle (radians)";
static char kamstep[] =  "Step size";
static char kamstop[] =  "Stop value";
static char pointsperorbit[] = "+Points per orbit";

// popcorn and julia popcorn generalized
static char step_x[] = "Step size (real)";
static char step_y[] = "Step size (imaginary)";
static char constant_x[] = "Constant C (real)";
static char constant_y[] = "Constant C (imaginary)";

// bifurcations
static char filt[] = "+Filter Cycles";
static char seed[] = "Seed Population";

// frothy basins
static char frothmapping[] = "+Apply mapping once (1) or twice (2)";
static char frothshade[] =  "+Enter non-zero value for alternate color shading";
static char frothavalue[] =  "A (imaginary part of C)";

// plasma and ant

static char s_randomseed[] = "+Random Seed Value (0 = Random, 1 = Reuse Last)";

// ifs
static char color_method[] = "+Coloring method (0,1)";

// phoenix fractals
static char degreeZ[] = "Degree = 0 | >= 2 | <= -3";

// julia inverse
static char s_maxhits[] = "Max Hits per Pixel";

// halley
static char order[] = {"+Order (integer > 1)"};
static char real_relax[] = {"Real Relaxation coefficient"};
static char epsilon[] = {"Epsilon"};
static char imag_relax[] = {"Imag Relaxation coefficient"};
// cellular
static char cell_init[] = {"#Initial String | 0 = Random | -1 = Reuse Last Random"};
static char cell_rule[] = {"#Rule = # of digits (see below) | 0 = Random"};
static char cell_type[] = {"+Type (see below)"};
static char cell_strt[] = {"#Starting Row Number"};

// bailout defines
#define FTRIGBAILOUT 2500
#define LTRIGBAILOUT   64
#define FROTHBAILOUT    7
#define STDBAILOUT      4
#define NOBAILOUT       0

MOREPARAMS moreparams[] =
{
    {fractal_type::ICON             ,{ "Omega", "+Degree of symmetry",   "","","",""},{0,3,0,0,0,0}},
    {fractal_type::ICON3D           ,{ "Omega", "+Degree of symmetry",   "","","",""},{0,3,0,0,0,0}},
    {fractal_type::HYPERCMPLXJFP    ,{ "zj",      "zk",          "","","",""},{0,0,0,0,0,0}},
    {fractal_type::QUATJULFP        ,{ "zj",      "zk",          "","","",""},{0,0,0,0,0,0}},
    {fractal_type::PHOENIXCPLX      ,{ degreeZ, "",          "","","",""},{0,0,0,0,0,0}},
    {fractal_type::PHOENIXFPCPLX    ,{ degreeZ, "",          "","","",""},{0,0,0,0,0,0}},
    {fractal_type::MANDPHOENIXCPLX  ,{ degreeZ, "",          "","","",""},{0,0,0,0,0,0}},
    {fractal_type::MANDPHOENIXFPCPLX,{ degreeZ, "",          "","","",""},{0,0,0,0,0,0}},
    {fractal_type::FORMULA  ,{ p3real,p3imag,p4real,p4imag,p5real,p5imag},{0,0,0,0,0,0}},
    {fractal_type::FFORMULA ,{ p3real,p3imag,p4real,p4imag,p5real,p5imag},{0,0,0,0,0,0}},
    {fractal_type::ANT              ,{ "+Wrap?",s_randomseed,"","","",""},{1,0,0,0,0,0}},
    {fractal_type::MANDELBROTMIX4   ,{ p3real,p3imag,        "","","",""},{0,0,0,0,0,0}},
    {fractal_type::NOFRACTAL        ,{ nullptr,nullptr,nullptr,nullptr,nullptr,nullptr    },{0,0,0,0,0,0}}
};

/*
     type math orbitcalc fnct per_pixel fnct per_image fnct
   |-----|----|--------------|--------------|--------------| */
AlternateMath alternatemath[] =
{
#define USEBN
#ifdef USEBN
    {fractal_type::JULIAFP, bf_math_type::BIGNUM,JuliabnFractal,juliabn_per_pixel,  MandelbnSetup},
    {fractal_type::MANDELFP,bf_math_type::BIGNUM,JuliabnFractal,mandelbn_per_pixel, MandelbnSetup},
#else
    {fractal_type::JULIAFP, bf_math_type::BIGFLT,JuliabfFractal,juliabf_per_pixel,  MandelbfSetup},
    {fractal_type::MANDELFP,bf_math_type::BIGFLT,JuliabfFractal,mandelbf_per_pixel, MandelbfSetup},
#endif
    /*
    NOTE: The default precision for bf_math=BIGNUM is not high enough
          for JuliaZpowerbnFractal.  If you want to test BIGNUM (1) instead
          of the usual BIGFLT (2), then set bfdigits on the command to
          increase the precision.
    */
    {fractal_type::FPJULIAZPOWER, bf_math_type::BIGFLT, JuliaZpowerbfFractal,juliabf_per_pixel, MandelbfSetup  },
    {fractal_type::FPMANDELZPOWER, bf_math_type::BIGFLT ,JuliaZpowerbfFractal,mandelbf_per_pixel, MandelbfSetup},
    {fractal_type::NOFRACTAL, bf_math_type::NONE,nullptr,                nullptr,               nullptr         }
};

// These are only needed for types with both integer and float variations
char t_barnsleyj1[] = "*barnsleyj1";
char t_barnsleyj2[] = "*barnsleyj2";
char t_barnsleyj3[] = "*barnsleyj3";
char t_barnsleym1[] = "*barnsleym1";
char t_barnsleym2[] = "*barnsleym2";
char t_barnsleym3[] = "*barnsleym3";
char t_bifplussinpi[] = "*bif+sinpi";
char t_bifeqsinpi[] = "*bif=sinpi";
char t_biflambda[] = "*biflambda";
char t_bifmay[] = "*bifmay";
char t_bifstewart[] = "*bifstewart";
char t_bifurcation[] = "*bifurcation";
char t_fn_z_plusfn_pix_[] = "*fn(z)+fn(pix)";
char t_fn_zz_[] = "*fn(z*z)";
char t_fnfn[] = "*fn*fn";
char t_fnzplusz[] = "*fn*z+z";
char t_fnplusfn[] = "*fn+fn";
char t_formula[] = "*formula";
char t_henon[] = "*henon";
char t_ifs3d[] = "*ifs3d";
char t_julfnplusexp[] = "*julfn+exp";
char t_julfnpluszsqrd[] = "*julfn+zsqrd";
char t_julia[] = "*julia";
char t_julia_fnorfn_[] = "*julia(fn||fn)";
char t_julia4[] = "*julia4";
char t_julia_inverse[] = "*julia_inverse";
char t_julibrot[] = "*julibrot";
char t_julzpower[] = "*julzpower";
char t_kamtorus[] = "*kamtorus";
char t_kamtorus3d[] = "*kamtorus3d";
char t_lambda[] = "*lambda";
char t_lambda_fnorfn_[] = "*lambda(fn||fn)";
char t_lambdafn[] = "*lambdafn";
char t_lorenz[] = "*lorenz";
char t_lorenz3d[] = "*lorenz3d";
char t_mandel[] = "*mandel";
char t_mandel_fnorfn_[] = "*mandel(fn||fn)";
char t_mandel4[] = "*mandel4";
char t_mandelfn[] = "*mandelfn";
char t_mandellambda[] = "*mandellambda";
char t_mandphoenix[] = "*mandphoenix";
char t_mandphoenixcplx[] = "*mandphoenixclx";
char t_manfnplusexp[] = "*manfn+exp";
char t_manfnpluszsqrd[] = "*manfn+zsqrd";
char t_manlam_fnorfn_[] = "*manlam(fn||fn)";
char t_manowar[] = "*manowar";
char t_manowarj[] = "*manowarj";
char t_manzpower[] = "*manzpower";
char t_marksjulia[] = "*marksjulia";
char t_marksmandel[] = "*marksmandel";
char t_marksmandelpwr[] = "*marksmandelpwr";
char t_newtbasin[] = "*newtbasin";
char t_newton[] = "*newton";
char t_phoenix[] = "*phoenix";
char t_phoenixcplx[] = "*phoenixcplx";
char t_popcorn[] = "*popcorn";
char t_popcornjul[] = "*popcornjul";
char t_rossler3d[] = "*rossler3d";
char t_sierpinski[] = "*sierpinski";
char t_spider[] = "*spider";
char t_sqr_1divfn_[] = "*sqr(1/fn)";
char t_sqr_fn_[] = "*sqr(fn)";
char t_tims_error[] = "*tim's_error";
char t_unity[] = "*unity";
char t_frothybasin[] = "*frothybasin";
char t_halley[] = "*halley";

// use next to cast orbitcalcs() that have arguments
#define VF int(*)()

fractalspecificstuff fractalspecific[] =
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
        t_mandel+1,
        {realz0, imagz0, "", ""},
        {0, 0, 0, 0},
        HT_MANDEL, HF_MANDEL, BAILTEST,
        -2.5F, 1.5F, -1.5F, 1.5F,
        1, fractal_type::JULIA, fractal_type::NOFRACTAL, fractal_type::MANDELFP, symmetry_type::X_AXIS_NO_PARAM,
        JuliaFractal, mandel_per_pixel,MandelSetup, StandardFractal,
        STDBAILOUT
    },

    {
        t_julia+1,
        {realparm, imagparm, "", ""},
        {0.3, 0.6, 0, 0},
        HT_JULIA, HF_JULIA, OKJB+BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        1, fractal_type::NOFRACTAL, fractal_type::MANDEL, fractal_type::JULIAFP, symmetry_type::ORIGIN,
        JuliaFractal, julia_per_pixel, JuliaSetup, StandardFractal,
        STDBAILOUT
    },

    {
        t_newtbasin,
        {newtdegree, "Enter non-zero value for stripes", "", ""},
        {3, 0, 0, 0},
        HT_NEWTBAS, HF_NEWTBAS, 0,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::MPNEWTBASIN, symmetry_type::NONE,
        NewtonFractal2, otherjuliafp_per_pixel, NewtonSetup, StandardFractal,
        NOBAILOUT
    },

    {
        t_lambda+1,
        {realparm, imagparm, "", ""},
        {0.85, 0.6, 0, 0},
        HT_LAMBDA, HF_LAMBDA, OKJB+BAILTEST,
        -1.5F, 2.5F, -1.5F, 1.5F,
        1, fractal_type::NOFRACTAL, fractal_type::MANDELLAMBDA, fractal_type::LAMBDAFP, symmetry_type::NONE,
        LambdaFractal, julia_per_pixel, JulialongSetup, StandardFractal,
        STDBAILOUT
    },

    {
        t_mandel,
        {realz0, imagz0, "", ""},
        {0, 0, 0, 0},
        HT_MANDEL, HF_MANDEL, BAILTEST+BF_MATH,
        -2.5F, 1.5F, -1.5F, 1.5F,
        0, fractal_type::JULIAFP, fractal_type::NOFRACTAL, fractal_type::MANDEL, symmetry_type::X_AXIS_NO_PARAM,
        JuliafpFractal, mandelfp_per_pixel, MandelfpSetup, StandardFractal,
        STDBAILOUT
    },

    {
        t_newton,
        {newtdegree, "", "", ""},
        {3, 0, 0, 0},
        HT_NEWT, HF_NEWT, 0,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::MPNEWTON, symmetry_type::X_AXIS,
        NewtonFractal2, otherjuliafp_per_pixel, NewtonSetup, StandardFractal,
        NOBAILOUT
    },

    {
        t_julia,
        {realparm, imagparm, "", ""},
        {0.3, 0.6, 0, 0},
        HT_JULIA, HF_JULIA, OKJB+BAILTEST+BF_MATH,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, fractal_type::NOFRACTAL, fractal_type::MANDELFP, fractal_type::JULIA, symmetry_type::ORIGIN,
        JuliafpFractal, juliafp_per_pixel,  JuliafpSetup, StandardFractal,
        STDBAILOUT
    },

    {
        "plasma",
        {   "Graininess Factor (0 or 0.125 to 100, default is 2)",
            "+Algorithm (0 = original, 1 = new)",
            "+Random Seed Value (0 = Random, 1 = Reuse Last)",
            "+Save as Pot File? (0 = No,     1 = Yes)"
        },
        {2, 0, 0, 0},
        HT_PLASMA, HF_PLASMA, NOZOOM+NOGUESS+NOTRACE+NORESUME,
        -2.0F, 2.0F, -1.5F, 1.5F,
        1, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, symmetry_type::NONE,
        nullptr, nullptr, StandaloneSetup, plasma,
        NOBAILOUT
    },

    {
        t_mandelfn,
        {realz0, imagz0, "", ""},
        {0, 0, 0, 0},
        HT_MANDFN, HF_MANDFN, TRIG1,
        -8.0F, 8.0F, -6.0F, 6.0F,
        0, fractal_type::LAMBDATRIGFP, fractal_type::NOFRACTAL, fractal_type::MANDELTRIG, symmetry_type::XY_AXIS_NO_PARAM,
        LambdaTrigfpFractal,othermandelfp_per_pixel,MandelTrigSetup,
        StandardFractal,
        LTRIGBAILOUT
    },

    {
        t_manowar,
        {realz0, imagz0, "", ""},
        {0, 0, 0, 0},
        HT_SCOTSKIN, HF_MANOWAR, BAILTEST,
        -2.5F, 1.5F, -1.5F, 1.5F,
        0, fractal_type::MANOWARJFP, fractal_type::NOFRACTAL, fractal_type::MANOWAR, symmetry_type::X_AXIS_NO_PARAM,
        ManOWarfpFractal, mandelfp_per_pixel, MandelfpSetup, StandardFractal,
        STDBAILOUT
    },

    {
        t_manowar+1,
        {realz0, imagz0, "", ""},
        {0, 0, 0, 0},
        HT_SCOTSKIN, HF_MANOWAR, BAILTEST,
        -2.5F, 1.5F, -1.5F, 1.5F,
        1, fractal_type::MANOWARJ, fractal_type::NOFRACTAL, fractal_type::MANOWARFP, symmetry_type::X_AXIS_NO_PARAM,
        ManOWarFractal, mandel_per_pixel, MandellongSetup, StandardFractal,
        STDBAILOUT
    },

    {
        "test",
        {   "(testpt Param #1)",
            "(testpt param #2)",
            "(testpt param #3)",
            "(testpt param #4)"
        },
        {0, 0, 0, 0},
        HT_TEST, HF_TEST, 0,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, symmetry_type::NONE,
        nullptr, nullptr, StandaloneSetup, test,
        STDBAILOUT
    },

    {
        t_sierpinski+1,
        {"", "", "", ""},
        {0, 0, 0, 0},
        HT_SIER, HF_SIER, 0,
        -4.0F/3.0F, 96.0F/45.0F, -0.9F, 1.7F,
        1, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::SIERPINSKIFP, symmetry_type::NONE,
        SierpinskiFractal, long_julia_per_pixel, SierpinskiSetup,
        StandardFractal,
        127
    },

    {
        t_barnsleym1+1,
        {realz0, imagz0, "", ""},
        {0, 0, 0, 0},
        HT_BARNS, HF_BARNSM1, BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        1, fractal_type::BARNSLEYJ1, fractal_type::NOFRACTAL, fractal_type::BARNSLEYM1FP, symmetry_type::XY_AXIS_NO_PARAM,
        Barnsley1Fractal, long_mandel_per_pixel, MandellongSetup,
        StandardFractal,
        STDBAILOUT
    },

    {
        t_barnsleyj1+1,
        {realparm, imagparm, "", ""},
        {0.6, 1.1, 0, 0},
        HT_BARNS, HF_BARNSJ1, OKJB+BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        1, fractal_type::NOFRACTAL, fractal_type::BARNSLEYM1, fractal_type::BARNSLEYJ1FP, symmetry_type::ORIGIN,
        Barnsley1Fractal, long_julia_per_pixel, JulialongSetup, StandardFractal,
        STDBAILOUT
    },

    {
        t_barnsleym2+1,
        {realz0, imagz0, "", ""},
        {0, 0, 0, 0},
        HT_BARNS, HF_BARNSM2, BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        1, fractal_type::BARNSLEYJ2, fractal_type::NOFRACTAL, fractal_type::BARNSLEYM2FP, symmetry_type::Y_AXIS_NO_PARAM,
        Barnsley2Fractal, long_mandel_per_pixel, MandellongSetup,
        StandardFractal,
        STDBAILOUT
    },

    {
        t_barnsleyj2+1,
        {realparm, imagparm, "", ""},
        {0.6, 1.1, 0, 0},
        HT_BARNS, HF_BARNSJ2, OKJB+BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        1, fractal_type::NOFRACTAL, fractal_type::BARNSLEYM2, fractal_type::BARNSLEYJ2FP, symmetry_type::ORIGIN,
        Barnsley2Fractal, long_julia_per_pixel, JulialongSetup, StandardFractal,
        STDBAILOUT
    },

    {
        t_sqr_fn_+1,
        {"", "", "", ""},
        {0, 0, 0, 0},
        HT_SCOTSKIN, HF_SQRFN, TRIG1+BAILTEST,
        -4.0F, 4.0F, -3.0F, 3.0F,
        16, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::SQRTRIGFP, symmetry_type::X_AXIS,
        SqrTrigFractal, long_julia_per_pixel, SqrTrigSetup, StandardFractal,
        LTRIGBAILOUT
    },

    {
        t_sqr_fn_,
        {"", "", "", ""},
        {0, 0, 0, 0},
        HT_SCOTSKIN, HF_SQRFN, TRIG1+BAILTEST,
        -4.0F, 4.0F, -3.0F, 3.0F,
        0, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::SQRTRIG, symmetry_type::X_AXIS,
        SqrTrigfpFractal, otherjuliafp_per_pixel, SqrTrigSetup, StandardFractal,
        LTRIGBAILOUT
    },

    {
        t_fnplusfn+1,
        {recoeftrg1, imcoeftrg1, recoeftrg2, imcoeftrg2},
        {1, 0, 1, 0},
        HT_SCOTSKIN, HF_FNPLUSFN, TRIG2+BAILTEST,
        -4.0F, 4.0F, -3.0F, 3.0F,
        16, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::TRIGPLUSTRIGFP, symmetry_type::X_AXIS,
        TrigPlusTrigFractal, long_julia_per_pixel, TrigPlusTriglongSetup,
        StandardFractal,
        LTRIGBAILOUT
    },

    {
        t_mandellambda+1,
        {realz0, imagz0, "", ""},
        {0, 0, 0, 0},
        HT_MLAMBDA, HF_MLAMBDA, BAILTEST,
        -3.0F, 5.0F, -3.0F, 3.0F,
        1, fractal_type::LAMBDA, fractal_type::NOFRACTAL, fractal_type::MANDELLAMBDAFP, symmetry_type::X_AXIS_NO_PARAM,
        LambdaFractal, mandel_per_pixel,MandellongSetup, StandardFractal,
        STDBAILOUT
    },

    {
        t_marksmandel+1,
        {realz0, imagz0, exponent, ""},
        {0, 0, 1, 0},
        HT_MARKS, HF_MARKSMAND, BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        1, fractal_type::MARKSJULIA, fractal_type::NOFRACTAL, fractal_type::MARKSMANDELFP, symmetry_type::NONE,
        MarksLambdaFractal, marksmandel_per_pixel, MandellongSetup,
        StandardFractal,
        STDBAILOUT
    },

    {
        t_marksjulia+1,
        {realparm, imagparm, exponent, ""},
        {0.1, 0.9, 1, 0},
        HT_MARKS, HF_MARKSJULIA, BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        1, fractal_type::NOFRACTAL, fractal_type::MARKSMANDEL, fractal_type::MARKSJULIAFP, symmetry_type::ORIGIN,
        MarksLambdaFractal, julia_per_pixel, MarksJuliaSetup, StandardFractal,
        STDBAILOUT
    },

    {
        t_unity+1,
        {"", "", "", ""},
        {0, 0, 0, 0},
        HT_UNITY, HF_UNITY, 0,
        -2.0F, 2.0F, -1.5F, 1.5F,
        1, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::UNITYFP, symmetry_type::XY_AXIS,
        UnityFractal, long_julia_per_pixel, UnitySetup, StandardFractal,
        NOBAILOUT
    },

    {
        t_mandel4+1,
        {realz0, imagz0, "", ""},
        {0, 0, 0, 0},
        HT_MANDJUL4, HF_MANDEL4, BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        1, fractal_type::JULIA4, fractal_type::NOFRACTAL, fractal_type::MANDEL4FP, symmetry_type::X_AXIS_NO_PARAM,
        Mandel4Fractal, mandel_per_pixel, MandellongSetup, StandardFractal,
        STDBAILOUT
    },

    {
        t_julia4+1,
        {realparm, imagparm, "", ""},
        {0.6, 0.55, 0, 0},
        HT_MANDJUL4, HF_JULIA4, OKJB+BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        1, fractal_type::NOFRACTAL, fractal_type::MANDEL4, fractal_type::JULIA4FP, symmetry_type::ORIGIN,
        Mandel4Fractal, julia_per_pixel, JulialongSetup, StandardFractal,
        STDBAILOUT
    },

    {
        "ifs",
        {color_method, "", "", ""},
        {0, 0, 0, 0},
        HT_IFS, -4, NOGUESS+NOTRACE+NORESUME+INFCALC,
        -8.0F, 8.0F, -1.0F, 11.0F,
        16, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, symmetry_type::NONE,
        nullptr, nullptr, StandaloneSetup, ifs,
        NOBAILOUT
    },

    {
        t_ifs3d,
        {color_method, "", "", ""},
        {0, 0, 0, 0},
        HT_IFS, -4, NOGUESS+NOTRACE+NORESUME+PARMS3D+INFCALC,
        -11.0F, 11.0F, -11.0F, 11.0F,
        16, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, symmetry_type::NONE,
        nullptr, nullptr, StandaloneSetup, ifs,
        NOBAILOUT
    },

    {
        t_barnsleym3+1,
        {realz0, imagz0, "", ""},
        {0, 0, 0, 0},
        HT_BARNS, HF_BARNSM3, BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        1, fractal_type::BARNSLEYJ3,fractal_type::NOFRACTAL, fractal_type::BARNSLEYM3FP, symmetry_type::X_AXIS_NO_PARAM,
        Barnsley3Fractal, long_mandel_per_pixel, MandellongSetup,
        StandardFractal,
        STDBAILOUT
    },

    {
        t_barnsleyj3+1,
        {realparm, imagparm, "", ""},
        {0.1, 0.36, 0, 0},
        HT_BARNS, HF_BARNSJ3, OKJB+BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        1, fractal_type::NOFRACTAL, fractal_type::BARNSLEYM3, fractal_type::BARNSLEYJ3FP, symmetry_type::NONE,
        Barnsley3Fractal, long_julia_per_pixel, JulialongSetup, StandardFractal,
        STDBAILOUT
    },

    {
        t_fn_zz_+1,
        {"", "", "", ""},
        {0, 0, 0, 0},
        HT_SCOTSKIN, HF_FNZTIMESZ, TRIG1+BAILTEST,
        -4.0F, 4.0F, -3.0F, 3.0F,
        16, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::TRIGSQRFP, symmetry_type::XY_AXIS,
        TrigZsqrdFractal, julia_per_pixel, JulialongSetup, StandardFractal,
        STDBAILOUT
    },

    {
        t_fn_zz_,
        {"", "", "", ""},
        {0, 0, 0, 0},
        HT_SCOTSKIN, HF_FNZTIMESZ, TRIG1+BAILTEST,
        -4.0F, 4.0F, -3.0F, 3.0F,
        0, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::TRIGSQR, symmetry_type::XY_AXIS,
        TrigZsqrdfpFractal, juliafp_per_pixel, JuliafpSetup, StandardFractal,
        STDBAILOUT
    },

    {
        t_bifurcation,
        {filt, seed, "", ""},
        {1000.0, 0.66, 0, 0},
        HT_BIF, HF_BIFURCATION, TRIG1+NOGUESS+NOTRACE+NOROTATE,
        1.9F, 3.0F, 0.0F, 1.34F,
        0, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::LBIFURCATION, symmetry_type::NONE,
        BifurcVerhulstTrig, nullptr, StandaloneSetup, Bifurcation,
        NOBAILOUT
    },

    {
        t_fnplusfn,
        {recoeftrg1, imcoeftrg1, recoeftrg2, imcoeftrg2},
        {1, 0, 1, 0},
        HT_SCOTSKIN, HF_FNPLUSFN, TRIG2+BAILTEST,
        -4.0F, 4.0F, -3.0F, 3.0F,
        0, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::TRIGPLUSTRIG, symmetry_type::X_AXIS,
        TrigPlusTrigfpFractal, otherjuliafp_per_pixel, TrigPlusTrigfpSetup,
        StandardFractal,
        LTRIGBAILOUT
    },

    {
        t_fnfn+1,
        {"", "", "", ""},
        {0, 0, 0, 0},
        HT_SCOTSKIN, HF_FNTIMESFN, TRIG2+BAILTEST,
        -4.0F, 4.0F, -3.0F, 3.0F,
        16, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::TRIGXTRIGFP, symmetry_type::X_AXIS,
        TrigXTrigFractal, long_julia_per_pixel, FnXFnSetup, StandardFractal,
        LTRIGBAILOUT
    },

    {
        t_fnfn,
        {"", "", "", ""},
        {0, 0, 0, 0},
        HT_SCOTSKIN, HF_FNTIMESFN, TRIG2+BAILTEST,
        -4.0F, 4.0F, -3.0F, 3.0F,
        0, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::TRIGXTRIG, symmetry_type::X_AXIS,
        TrigXTrigfpFractal, otherjuliafp_per_pixel, FnXFnSetup, StandardFractal,
        LTRIGBAILOUT
    },

    {
        t_sqr_1divfn_+1,
        {"", "", "", ""},
        {0, 0, 0, 0},
        HT_SCOTSKIN, HF_SQROVFN, TRIG1+BAILTEST,
        -4.0F, 4.0F, -3.0F, 3.0F,
        16, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::SQR1OVERTRIGFP, symmetry_type::NONE,
        Sqr1overTrigFractal, long_julia_per_pixel, SqrTrigSetup,
        StandardFractal,
        LTRIGBAILOUT
    },

    {
        t_sqr_1divfn_,
        {"", "", "", ""},
        {0, 0, 0, 0},
        HT_SCOTSKIN, HF_SQROVFN, TRIG1+BAILTEST,
        -4.0F, 4.0F, -3.0F, 3.0F,
        0, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::SQR1OVERTRIG, symmetry_type::NONE,
        Sqr1overTrigfpFractal, otherjuliafp_per_pixel, SqrTrigSetup,
        StandardFractal,
        LTRIGBAILOUT
    },

    {
        t_fnzplusz+1,
        {recoeftrg1, imcoeftrg1, "Real Coefficient Second Term", "Imag Coefficient Second Term"},
        {1, 0, 1, 0},
        HT_SCOTSKIN, HF_FNXZPLUSZ, TRIG1+BAILTEST,
        -4.0F, 4.0F, -3.0F, 3.0F,
        1, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::ZXTRIGPLUSZFP, symmetry_type::X_AXIS,
        ZXTrigPlusZFractal, julia_per_pixel, ZXTrigPlusZSetup, StandardFractal,
        LTRIGBAILOUT
    },

    {
        t_fnzplusz,
        {recoeftrg1, imcoeftrg1, "Real Coefficient Second Term", "Imag Coefficient Second Term"},
        {1, 0, 1, 0},
        HT_SCOTSKIN, HF_FNXZPLUSZ, TRIG1+BAILTEST,
        -4.0F, 4.0F, -3.0F, 3.0F,
        0, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::ZXTRIGPLUSZ, symmetry_type::X_AXIS,
        ZXTrigPlusZfpFractal, juliafp_per_pixel, ZXTrigPlusZSetup,
        StandardFractal,
        LTRIGBAILOUT
    },

    {
        t_kamtorus,
        {kamangle, kamstep, kamstop, pointsperorbit},
        {1.3, .05, 1.5, 150},
        HT_KAM, HF_KAM, NOGUESS+NOTRACE,
        -1.0F, 1.0F, -.75F, .75F,
        0, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::KAM, symmetry_type::NONE,
        (VF)kamtorusfloatorbit, nullptr, orbit3dfloatsetup, orbit2dfloat,
        NOBAILOUT
    },

    {
        t_kamtorus+1,
        {kamangle, kamstep, kamstop, pointsperorbit},
        {1.3, .05, 1.5, 150},
        HT_KAM, HF_KAM, NOGUESS+NOTRACE,
        -1.0F, 1.0F, -.75F, .75F,
        16, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::KAMFP, symmetry_type::NONE,
        (VF)kamtoruslongorbit, nullptr, orbit3dlongsetup, orbit2dlong,
        NOBAILOUT
    },

    {
        t_kamtorus3d,
        {kamangle, kamstep, kamstop, pointsperorbit},
        {1.3, .05, 1.5, 150},
        HT_KAM, HF_KAM, NOGUESS+NOTRACE+NORESUME+PARMS3D,
        -3.0F, 3.0F, -1.0F, 3.5F,
        0, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::KAM3D, symmetry_type::NONE,
        (VF)kamtorusfloatorbit, nullptr, orbit3dfloatsetup, orbit3dfloat,
        NOBAILOUT
    },

    {
        t_kamtorus3d+1,
        {kamangle, kamstep, kamstop, pointsperorbit},
        {1.3, .05, 1.5, 150},
        HT_KAM, HF_KAM, NOGUESS+NOTRACE+NORESUME+PARMS3D,
        -3.0F, 3.0F, -1.0F, 3.5F,
        16, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::KAM3DFP, symmetry_type::NONE,
        (VF)kamtoruslongorbit, nullptr, orbit3dlongsetup, orbit3dlong,
        NOBAILOUT
    },

    {
        t_lambdafn+1,
        {realparm, imagparm, "", ""},
        {1.0, 0.4, 0, 0},
        HT_LAMBDAFN, HF_LAMBDAFN, TRIG1+OKJB,
        -4.0F, 4.0F, -3.0F, 3.0F,
        16, fractal_type::NOFRACTAL, fractal_type::MANDELTRIG, fractal_type::LAMBDATRIGFP, symmetry_type::PI_SYM,
        (VF)LambdaTrigFractal, long_julia_per_pixel, LambdaTrigSetup,
        StandardFractal,
        LTRIGBAILOUT
    },

    {
        t_manfnpluszsqrd+1,
        {realz0, imagz0, "", ""},
        {0, 0, 0, 0},
        HT_PICKMJ, HF_MANDFNPLUSZSQRD, TRIG1+BAILTEST,
        -2.5F, 1.5F, -1.5F, 1.5F,
        16, fractal_type::LJULTRIGPLUSZSQRD, fractal_type::NOFRACTAL, fractal_type::FPMANTRIGPLUSZSQRD, symmetry_type::X_AXIS_NO_PARAM,
        TrigPlusZsquaredFractal, mandel_per_pixel, MandellongSetup,
        StandardFractal,
        STDBAILOUT
    },

    {
        t_julfnpluszsqrd+1,
        {realparm, imagparm, "", ""},
        {-0.5, 0.5, 0, 0},
        HT_PICKMJ, HF_JULFNPLUSZSQRD, TRIG1+OKJB+BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        16, fractal_type::NOFRACTAL, fractal_type::LMANTRIGPLUSZSQRD, fractal_type::FPJULTRIGPLUSZSQRD, symmetry_type::NONE,
        TrigPlusZsquaredFractal, julia_per_pixel, JuliafnPlusZsqrdSetup,
        StandardFractal,
        STDBAILOUT
    },

    {
        t_manfnpluszsqrd,
        {realz0, imagz0, "", ""},
        {0, 0, 0, 0},
        HT_PICKMJ, HF_MANDFNPLUSZSQRD, TRIG1+BAILTEST,
        -2.5F, 1.5F, -1.5F, 1.5F,
        0, fractal_type::FPJULTRIGPLUSZSQRD, fractal_type::NOFRACTAL, fractal_type::LMANTRIGPLUSZSQRD, symmetry_type::X_AXIS_NO_PARAM,
        TrigPlusZsquaredfpFractal, mandelfp_per_pixel, MandelfpSetup,
        StandardFractal,
        STDBAILOUT
    },

    {
        t_julfnpluszsqrd,
        {realparm, imagparm, "", ""},
        {-0.5, 0.5, 0, 0},
        HT_PICKMJ, HF_JULFNPLUSZSQRD, TRIG1+OKJB+BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, fractal_type::NOFRACTAL, fractal_type::FPMANTRIGPLUSZSQRD, fractal_type::LJULTRIGPLUSZSQRD, symmetry_type::NONE,
        TrigPlusZsquaredfpFractal, juliafp_per_pixel, JuliafnPlusZsqrdSetup,
        StandardFractal,
        STDBAILOUT
    },

    {
        t_lambdafn,
        {realparm, imagparm, "", ""},
        {1.0, 0.4, 0, 0},
        HT_LAMBDAFN, HF_LAMBDAFN, TRIG1+OKJB,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, fractal_type::NOFRACTAL, fractal_type::MANDELTRIGFP, fractal_type::LAMBDATRIG, symmetry_type::PI_SYM,
        LambdaTrigfpFractal, otherjuliafp_per_pixel, LambdaTrigSetup,
        StandardFractal,
        LTRIGBAILOUT
    },

    {
        t_mandelfn+1,
        {realz0, imagz0, "", ""},
        {0, 0, 0, 0},
        HT_MANDFN, HF_MANDFN, TRIG1,
        -8.0F, 8.0F, -6.0F, 6.0F,
        16, fractal_type::LAMBDATRIG, fractal_type::NOFRACTAL, fractal_type::MANDELTRIGFP, symmetry_type::XY_AXIS_NO_PARAM,
        LambdaTrigFractal, long_mandel_per_pixel, MandelTrigSetup,
        StandardFractal,
        LTRIGBAILOUT
    },

    {
        t_manzpower+1,
        {realz0, imagz0, exponent, imexponent},
        {0, 0, 2, 0},
        HT_PICKMJ, HF_MANZPOWER, BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        1, fractal_type::LJULIAZPOWER, fractal_type::NOFRACTAL, fractal_type::FPMANDELZPOWER, symmetry_type::X_AXIS_NO_IMAG,
        longZpowerFractal, long_mandel_per_pixel, MandellongSetup,
        StandardFractal,
        STDBAILOUT
    },

    {
        t_julzpower+1,
        {realparm, imagparm, exponent, imexponent},
        {0.3, 0.6, 2, 0},
        HT_PICKMJ, HF_JULZPOWER, OKJB+BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        1, fractal_type::NOFRACTAL, fractal_type::LMANDELZPOWER, fractal_type::FPJULIAZPOWER, symmetry_type::ORIGIN,
        longZpowerFractal, long_julia_per_pixel, JulialongSetup,
        StandardFractal,
        STDBAILOUT
    },

    {
        t_manzpower,
        {realz0, imagz0, exponent, imexponent},
        {0, 0, 2, 0},
        HT_PICKMJ, HF_MANZPOWER, BAILTEST+BF_MATH,
        -2.5F, 1.5F, -1.5F, 1.5F,
        0, fractal_type::FPJULIAZPOWER, fractal_type::NOFRACTAL, fractal_type::LMANDELZPOWER, symmetry_type::X_AXIS_NO_IMAG,
        floatZpowerFractal, othermandelfp_per_pixel, MandelfpSetup,
        StandardFractal,
        STDBAILOUT
    },

    {
        t_julzpower,
        {realparm, imagparm, exponent, imexponent},
        {0.3, 0.6, 2, 0},
        HT_PICKMJ, HF_JULZPOWER, OKJB+BAILTEST+BF_MATH,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, fractal_type::NOFRACTAL, fractal_type::FPMANDELZPOWER, fractal_type::LJULIAZPOWER, symmetry_type::ORIGIN,
        floatZpowerFractal, otherjuliafp_per_pixel, JuliafpSetup,
        StandardFractal,
        STDBAILOUT
    },

    {
        "manzzpwr",
        {realz0, imagz0, exponent, ""},
        {0, 0, 2, 0},
        HT_PICKMJ, HF_MANZZPWR, BAILTEST,
        -2.5F, 1.5F, -1.5F, 1.5F,
        0, fractal_type::FPJULZTOZPLUSZPWR, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, symmetry_type::X_AXIS_NO_PARAM,
        floatZtozPluszpwrFractal, othermandelfp_per_pixel, MandelfpSetup,
        StandardFractal,
        STDBAILOUT
    },

    {
        "julzzpwr",
        {realparm, imagparm, exponent, ""},
        {-0.3, 0.3, 2, 0},
        HT_PICKMJ, HF_JULZZPWR, OKJB+BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, fractal_type::NOFRACTAL, fractal_type::FPMANZTOZPLUSZPWR, fractal_type::NOFRACTAL, symmetry_type::NONE,
        floatZtozPluszpwrFractal, otherjuliafp_per_pixel, JuliafpSetup,
        StandardFractal,
        STDBAILOUT
    },

    {
        t_manfnplusexp+1,
        {realz0, imagz0, "", ""},
        {0, 0, 0, 0},
        HT_PICKMJ, HF_MANDFNPLUSEXP, TRIG1+BAILTEST,
        -8.0F, 8.0F, -6.0F, 6.0F,
        16, fractal_type::LJULTRIGPLUSEXP, fractal_type::NOFRACTAL, fractal_type::FPMANTRIGPLUSEXP, symmetry_type::X_AXIS_NO_PARAM,
        LongTrigPlusExponentFractal, long_mandel_per_pixel, MandellongSetup,
        StandardFractal,
        STDBAILOUT
    },

    {
        t_julfnplusexp+1,
        {realparm, imagparm, "", ""},
        {0, 0, 0, 0},
        HT_PICKMJ, HF_JULFNPLUSEXP, TRIG1+OKJB+BAILTEST,
        -4.0F, 4.0F, -3.0F, 3.0F,
        16, fractal_type::NOFRACTAL, fractal_type::LMANTRIGPLUSEXP, fractal_type::FPJULTRIGPLUSEXP, symmetry_type::NONE,
        LongTrigPlusExponentFractal, long_julia_per_pixel, JulialongSetup,
        StandardFractal,
        STDBAILOUT
    },

    {
        t_manfnplusexp,
        {realz0, imagz0, "", ""},
        {0, 0, 0, 0},
        HT_PICKMJ, HF_MANDFNPLUSEXP, TRIG1+BAILTEST,
        -8.0F, 8.0F, -6.0F, 6.0F,
        0, fractal_type::FPJULTRIGPLUSEXP, fractal_type::NOFRACTAL, fractal_type::LMANTRIGPLUSEXP, symmetry_type::X_AXIS_NO_PARAM,
        FloatTrigPlusExponentFractal, othermandelfp_per_pixel, MandelfpSetup,
        StandardFractal,
        STDBAILOUT
    },

    {
        t_julfnplusexp,
        {realparm, imagparm, "", ""},
        {0, 0, 0, 0},
        HT_PICKMJ, HF_JULFNPLUSEXP, TRIG1+OKJB+BAILTEST,
        -4.0F, 4.0F, -3.0F, 3.0F,
        0, fractal_type::NOFRACTAL, fractal_type::FPMANTRIGPLUSEXP, fractal_type::LJULTRIGPLUSEXP, symmetry_type::NONE,
        FloatTrigPlusExponentFractal, otherjuliafp_per_pixel, JuliafpSetup,
        StandardFractal,
        STDBAILOUT
    },

    {
        t_popcorn,
        {step_x, step_y, constant_x, constant_y},
        {0.05, 0, 3.00, 0},
        HT_POPCORN, HF_POPCORN, NOGUESS+NOTRACE+TRIG4,
        -3.0F, 3.0F, -2.25F, 2.25F,
        0, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::LPOPCORN, symmetry_type::NO_PLOT,
        PopcornFractalFn, otherjuliafp_per_pixel, JuliafpSetup, popcorn,
        STDBAILOUT
    },

    {
        t_popcorn+1,
        {step_x, step_y, constant_x, constant_y},
        {0.05, 0, 3.00, 0},
        HT_POPCORN, HF_POPCORN, NOGUESS+NOTRACE+TRIG4,
        -3.0F, 3.0F, -2.25F, 2.25F,
        16, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::FPPOPCORN, symmetry_type::NO_PLOT,
        LPopcornFractalFn, long_julia_per_pixel, JulialongSetup, popcorn,
        STDBAILOUT
    },

    {
        t_lorenz,
        {timestep, "a", "b", "c"},
        {.02, 5, 15, 1},
        HT_LORENZ, HF_LORENZ, NOGUESS+NOTRACE+INFCALC,
        -15.0F, 15.0F, 0.0F, 30.0F,
        0, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::LLORENZ, symmetry_type::NONE,
        (VF)lorenz3dfloatorbit, nullptr, orbit3dfloatsetup, orbit2dfloat,
        NOBAILOUT
    },

    {
        t_lorenz+1,
        {timestep, "a", "b", "c"},
        {.02, 5, 15, 1},
        HT_LORENZ, HF_LORENZ, NOGUESS+NOTRACE+INFCALC,
        -15.0F, 15.0F, 0.0F, 30.0F,
        16, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::FPLORENZ, symmetry_type::NONE,
        (VF)lorenz3dlongorbit, nullptr, orbit3dlongsetup, orbit2dlong,
        NOBAILOUT
    },

    {
        t_lorenz3d+1,
        {timestep, "a", "b", "c"},
        {.02, 5, 15, 1},
        HT_LORENZ, HF_LORENZ, NOGUESS+NOTRACE+NORESUME+PARMS3D+INFCALC,
        -30.0F, 30.0F, -30.0F, 30.0F,
        16, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::FPLORENZ3D, symmetry_type::NONE,
        (VF)lorenz3dlongorbit, nullptr, orbit3dlongsetup, orbit3dlong,
        NOBAILOUT
    },

    {
        t_newton+1,
        {newtdegree, "", "", ""},
        {3, 0, 0, 0},
        HT_NEWT, HF_NEWT, 0,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::NEWTON, symmetry_type::X_AXIS,
        MPCNewtonFractal, MPCjulia_per_pixel, NewtonSetup, StandardFractal,
        NOBAILOUT
    },

    {
        t_newtbasin+1,
        {newtdegree, "Enter non-zero value for stripes", "", ""},
        {3, 0, 0, 0},
        HT_NEWTBAS, HF_NEWTBAS, 0,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::NEWTBASIN, symmetry_type::NONE,
        MPCNewtonFractal, MPCjulia_per_pixel, NewtonSetup, StandardFractal,
        NOBAILOUT
    },

    {
        "complexnewton",
        {"Real part of Degree", "Imag part of Degree", "Real part of Root", "Imag part of Root"},
        {3, 0, 1, 0},
        HT_NEWTCMPLX, HF_COMPLEXNEWT, 0,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, symmetry_type::NONE,
        ComplexNewton, otherjuliafp_per_pixel, ComplexNewtonSetup,
        StandardFractal,
        NOBAILOUT
    },

    {
        "complexbasin",
        {"Real part of Degree", "Imag part of Degree", "Real part of Root", "Imag part of Root"},
        {3, 0, 1, 0},
        HT_NEWTCMPLX, HF_COMPLEXNEWT, 0,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, symmetry_type::NONE,
        ComplexBasin, otherjuliafp_per_pixel, ComplexNewtonSetup,
        StandardFractal,
        NOBAILOUT
    },

    {
        "cmplxmarksmand",
        {realz0, imagz0, exponent, imexponent},
        {0, 0, 1, 0},
        HT_MARKS, HF_CMPLXMARKSMAND, BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, fractal_type::COMPLEXMARKSJUL, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, symmetry_type::NONE,
        MarksCplxMand, MarksCplxMandperp, MandelfpSetup, StandardFractal,
        STDBAILOUT
    },

    {
        "cmplxmarksjul",
        {realparm, imagparm, exponent, imexponent},
        {0.3, 0.6, 1, 0},
        HT_MARKS, HF_CMPLXMARKSJUL, BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, fractal_type::NOFRACTAL, fractal_type::COMPLEXMARKSMAND, fractal_type::NOFRACTAL, symmetry_type::NONE,
        MarksCplxMand, juliafp_per_pixel, JuliafpSetup, StandardFractal,
        STDBAILOUT
    },

    {
        t_formula+1,
        {p1real, p1imag, p2real, p2imag},
        {0, 0, 0, 0},
        HT_FORMULA, -2, MORE,
        -2.0F, 2.0F, -1.5F, 1.5F,
        1, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::FFORMULA, symmetry_type::SETUP,
        Formula, form_per_pixel, intFormulaSetup, StandardFractal,
        0
    },

    {
        t_formula,
        {p1real, p1imag, p2real, p2imag},
        {0, 0, 0, 0},
        HT_FORMULA, -2, MORE,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::FORMULA, symmetry_type::SETUP,
        Formula, form_per_pixel, fpFormulaSetup, StandardFractal,
        0
    },

    {
        t_sierpinski,
        {"", "", "", ""},
        {0, 0, 0, 0},
        HT_SIER, HF_SIER, 0,
        -4.0F/3.0F, 96.0F/45.0F, -0.9F, 1.7F,
        0, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::SIERPINSKI, symmetry_type::NONE,
        SierpinskiFPFractal, otherjuliafp_per_pixel, SierpinskiFPSetup,
        StandardFractal,
        127
    },

    {
        t_lambda,
        {realparm, imagparm, "", ""},
        {0.85, 0.6, 0, 0},
        HT_LAMBDA, HF_LAMBDA, OKJB+BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, fractal_type::NOFRACTAL, fractal_type::MANDELLAMBDAFP, fractal_type::LAMBDA, symmetry_type::NONE,
        LambdaFPFractal, juliafp_per_pixel, JuliafpSetup, StandardFractal,
        STDBAILOUT
    },

    {
        t_barnsleym1,
        {realz0, imagz0, "", ""},
        {0, 0, 0, 0},
        HT_BARNS, HF_BARNSM1, BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, fractal_type::BARNSLEYJ1FP, fractal_type::NOFRACTAL, fractal_type::BARNSLEYM1, symmetry_type::XY_AXIS_NO_PARAM,
        Barnsley1FPFractal, othermandelfp_per_pixel, MandelfpSetup,
        StandardFractal,
        STDBAILOUT
    },

    {
        t_barnsleyj1,
        {realparm, imagparm, "", ""},
        {0.6, 1.1, 0, 0},
        HT_BARNS, HF_BARNSJ1, OKJB+BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, fractal_type::NOFRACTAL, fractal_type::BARNSLEYM1FP, fractal_type::BARNSLEYJ1, symmetry_type::ORIGIN,
        Barnsley1FPFractal, otherjuliafp_per_pixel, JuliafpSetup,
        StandardFractal,
        STDBAILOUT
    },

    {
        t_barnsleym2,
        {realz0, imagz0, "", ""},
        {0, 0, 0, 0},
        HT_BARNS, HF_BARNSM2, BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, fractal_type::BARNSLEYJ2FP, fractal_type::NOFRACTAL, fractal_type::BARNSLEYM2, symmetry_type::Y_AXIS_NO_PARAM,
        Barnsley2FPFractal, othermandelfp_per_pixel, MandelfpSetup,
        StandardFractal,
        STDBAILOUT
    },

    {
        t_barnsleyj2,
        {realparm, imagparm, "", ""},
        {0.6, 1.1, 0, 0},
        HT_BARNS, HF_BARNSJ2, OKJB+BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, fractal_type::NOFRACTAL, fractal_type::BARNSLEYM2FP, fractal_type::BARNSLEYJ2, symmetry_type::ORIGIN,
        Barnsley2FPFractal, otherjuliafp_per_pixel, JuliafpSetup,
        StandardFractal,
        STDBAILOUT
    },

    {
        t_barnsleym3,
        {realz0, imagz0, "", ""},
        {0, 0, 0, 0},
        HT_BARNS, HF_BARNSM3, BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, fractal_type::BARNSLEYJ3FP, fractal_type::NOFRACTAL, fractal_type::BARNSLEYM3, symmetry_type::X_AXIS_NO_PARAM,
        Barnsley3FPFractal, othermandelfp_per_pixel, MandelfpSetup,
        StandardFractal,
        STDBAILOUT
    },

    {
        t_barnsleyj3,
        {realparm, imagparm, "", ""},
        {0.6, 1.1, 0, 0},
        HT_BARNS, HF_BARNSJ3, OKJB+BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, fractal_type::NOFRACTAL, fractal_type::BARNSLEYM3FP, fractal_type::BARNSLEYJ3, symmetry_type::NONE,
        Barnsley3FPFractal, otherjuliafp_per_pixel, JuliafpSetup,
        StandardFractal,
        STDBAILOUT
    },

    {
        t_mandellambda,
        {realz0, imagz0, "", ""},
        {0, 0, 0, 0},
        HT_MLAMBDA, HF_MLAMBDA, BAILTEST,
        -3.0F, 5.0F, -3.0F, 3.0F,
        0, fractal_type::LAMBDAFP, fractal_type::NOFRACTAL, fractal_type::MANDELLAMBDA, symmetry_type::X_AXIS_NO_PARAM,
        LambdaFPFractal, mandelfp_per_pixel, MandelfpSetup, StandardFractal,
        STDBAILOUT
    },

    {
        t_julibrot+1,
        {"", "", "", ""},
        {0, 0, 0, 0},
        HT_JULIBROT, -1, NOGUESS+NOTRACE+NOROTATE+NORESUME,
        -2.0F, 2.0F, -1.5F, 1.5F,
        1, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::JULIBROTFP, symmetry_type::NONE,
        JuliaFractal, jb_per_pixel, JulibrotSetup, Std4dFractal,
        STDBAILOUT
    },

    {
        t_lorenz3d,
        {timestep, "a", "b", "c"},
        {.02, 5, 15, 1},
        HT_LORENZ, HF_LORENZ, NOGUESS+NOTRACE+NORESUME+PARMS3D+INFCALC,
        -30.0F, 30.0F, -30.0F, 30.0F,
        0, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::LLORENZ3D, symmetry_type::NONE,
        (VF)lorenz3dfloatorbit, nullptr, orbit3dfloatsetup, orbit3dfloat,
        NOBAILOUT
    },

    {
        t_rossler3d+1,
        {timestep, "a", "b", "c"},
        {.04, .2, .2, 5.7},
        HT_ROSS, HF_ROSS, NOGUESS+NOTRACE+NORESUME+PARMS3D+INFCALC,
        -30.0F, 30.0F, -20.0F, 40.0F,
        16, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::FPROSSLER, symmetry_type::NONE,
        (VF)rosslerlongorbit, nullptr, orbit3dlongsetup, orbit3dlong,
        NOBAILOUT
    },

    {
        t_rossler3d,
        {timestep, "a", "b", "c"},
        {.04, .2, .2, 5.7},
        HT_ROSS, HF_ROSS, NOGUESS+NOTRACE+NORESUME+PARMS3D+INFCALC,
        -30.0F, 30.0F, -20.0F, 40.0F,
        0, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::LROSSLER, symmetry_type::NONE,
        (VF)rosslerfloatorbit, nullptr, orbit3dfloatsetup, orbit3dfloat,
        NOBAILOUT
    },

    {
        t_henon+1,
        {"a", "b", "", ""},
        {1.4, .3, 0, 0},
        HT_HENON, HF_HENON, NOGUESS+NOTRACE+INFCALC,
        -1.4F, 1.4F, -.5F, .5F,
        16, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::FPHENON, symmetry_type::NONE,
        (VF)henonlongorbit, nullptr, orbit3dlongsetup, orbit2dlong,
        NOBAILOUT
    },

    {
        t_henon,
        {"a", "b", "", ""},
        {1.4, .3, 0, 0},
        HT_HENON, HF_HENON, NOGUESS+NOTRACE+INFCALC,
        -1.4F, 1.4F, -.5F, .5F,
        0, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::LHENON, symmetry_type::NONE,
        (VF)henonfloatorbit, nullptr, orbit3dfloatsetup, orbit2dfloat,
        NOBAILOUT
    },

    {
        "pickover",
        {"a", "b", "c", "d"},
        {2.24, .43, -.65, -2.43},
        HT_PICK, HF_PICKOVER, NOGUESS+NOTRACE+NORESUME+PARMS3D,
        -8.0F/3.0F, 8.0F/3.0F, -2.0F, 2.0F,
        0, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, symmetry_type::NONE,
        (VF)pickoverfloatorbit, nullptr, orbit3dfloatsetup, orbit3dfloat,
        NOBAILOUT
    },

    {
        "gingerbreadman",
        {"Initial x", "Initial y", "", ""},
        {-.1, 0, 0, 0},
        HT_GINGER, HF_GINGER, NOGUESS+NOTRACE+INFCALC,
        -4.5F, 8.5F, -4.5F, 8.5F,
        0, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, symmetry_type::NONE,
        (VF)gingerbreadfloatorbit, nullptr, orbit3dfloatsetup, orbit2dfloat,
        NOBAILOUT
    },

    {
        "diffusion",
        {   "+Border size",
            "+Type (0=Central,1=Falling,2=Square Cavity)",
            "+Color change rate (0=Random)",
            ""
        },
        {10, 0, 0, 0},
        HT_DIFFUS, HF_DIFFUS, NOZOOM+NOGUESS+NOTRACE,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, symmetry_type::NONE,
        nullptr, nullptr, StandaloneSetup, diffusion,
        NOBAILOUT
    },

    {
        t_unity,
        {"", "", "", ""},
        {0, 0, 0, 0},
        HT_UNITY, HF_UNITY, 0,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::UNITY, symmetry_type::XY_AXIS,
        UnityfpFractal, otherjuliafp_per_pixel, StandardSetup, StandardFractal,
        NOBAILOUT
    },

    {
        t_spider,
        {realz0, imagz0, "", ""},
        {0, 0, 0, 0},
        HT_SCOTSKIN, HF_SPIDER, BAILTEST,
        -2.5F, 1.5F, -1.5F, 1.5F,
        0, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::SPIDER, symmetry_type::X_AXIS_NO_PARAM,
        SpiderfpFractal, mandelfp_per_pixel, MandelfpSetup, StandardFractal,
        STDBAILOUT
    },

    {
        t_spider+1,
        {realz0, imagz0, "", ""},
        {0, 0, 0, 0},
        HT_SCOTSKIN, HF_SPIDER, BAILTEST,
        -2.5F, 1.5F, -1.5F, 1.5F,
        1, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::SPIDERFP, symmetry_type::X_AXIS_NO_PARAM,
        SpiderFractal, mandel_per_pixel, MandellongSetup,StandardFractal,
        STDBAILOUT
    },

    {
        "tetrate",
        {realz0, imagz0, "", ""},
        {0, 0, 0, 0},
        HT_SCOTSKIN, HF_TETRATE, BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, symmetry_type::X_AXIS_NO_IMAG,
        TetratefpFractal, othermandelfp_per_pixel, MandelfpSetup,
        StandardFractal,
        STDBAILOUT
    },

    {
        "magnet1m",
        {realz0, imagz0, "", ""},
        {0, 0, 0, 0},
        HT_MAGNET, HF_MAGM1, 0,
        -4.0F, 4.0F, -3.0F, 3.0F,
        0, fractal_type::MAGNET1J, fractal_type::NOFRACTAL,fractal_type::NOFRACTAL, symmetry_type::X_AXIS_NO_PARAM,
        Magnet1Fractal, mandelfp_per_pixel, MandelfpSetup, StandardFractal,
        100
    },

    {
        "magnet1j",
        {realparm, imagparm, "", ""},
        {0, 0, 0, 0},
        HT_MAGNET, HF_MAGJ1, 0,
        -8.0F, 8.0F, -6.0F, 6.0F,
        0, fractal_type::NOFRACTAL, fractal_type::MAGNET1M, fractal_type::NOFRACTAL, symmetry_type::X_AXIS_NO_IMAG,
        Magnet1Fractal, juliafp_per_pixel, JuliafpSetup, StandardFractal,
        100
    },

    {
        "magnet2m",
        {realz0, imagz0, "", ""},
        {0, 0, 0, 0},
        HT_MAGNET, HF_MAGM2, 0,
        -1.5F, 3.7F, -1.95F, 1.95F,
        0, fractal_type::MAGNET2J, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, symmetry_type::X_AXIS_NO_PARAM,
        Magnet2Fractal, mandelfp_per_pixel, MandelfpSetup, StandardFractal,
        100
    },

    {
        "magnet2j",
        {realparm, imagparm, "", ""},
        {0, 0, 0, 0},
        HT_MAGNET, HF_MAGJ2, 0,
        -8.0F, 8.0F, -6.0F, 6.0F,
        0, fractal_type::NOFRACTAL, fractal_type::MAGNET2M, fractal_type::NOFRACTAL, symmetry_type::X_AXIS_NO_IMAG,
        Magnet2Fractal, juliafp_per_pixel, JuliafpSetup, StandardFractal,
        100
    },

    {
        t_bifurcation+1,
        {filt, seed, "", ""},
        {1000.0, 0.66, 0, 0},
        HT_BIF, HF_BIFURCATION, TRIG1+NOGUESS+NOTRACE+NOROTATE,
        1.9F, 3.0F, 0.0F, 1.34F,
        1, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::BIFURCATION, symmetry_type::NONE,
        LongBifurcVerhulstTrig, nullptr, StandaloneSetup, Bifurcation,
        NOBAILOUT
    },

    {
        t_biflambda+1,
        {filt, seed, "", ""},
        {1000.0, 0.66, 0, 0},
        HT_BIF, HF_BIFLAMBDA, TRIG1+NOGUESS+NOTRACE+NOROTATE,
        -2.0F, 4.0F, -1.0F, 2.0F,
        1, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::BIFLAMBDA, symmetry_type::NONE,
        LongBifurcLambdaTrig, nullptr, StandaloneSetup, Bifurcation,
        NOBAILOUT
    },

    {
        t_biflambda,
        {filt, seed, "", ""},
        {1000.0, 0.66, 0, 0},
        HT_BIF, HF_BIFLAMBDA, TRIG1+NOGUESS+NOTRACE+NOROTATE,
        -2.0F, 4.0F, -1.0F, 2.0F,
        0, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::LBIFLAMBDA, symmetry_type::NONE,
        BifurcLambdaTrig, nullptr, StandaloneSetup, Bifurcation,
        NOBAILOUT
    },

    {
        t_bifplussinpi,
        {filt, seed, "", ""},
        {1000.0, 0.66, 0, 0},
        HT_BIF, HF_BIFPLUSSINPI, TRIG1+NOGUESS+NOTRACE+NOROTATE,
        0.275F, 1.45F, 0.0F, 2.0F,
        0, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::LBIFADSINPI, symmetry_type::NONE,
        BifurcAddTrigPi, nullptr, StandaloneSetup, Bifurcation,
        NOBAILOUT
    },

    {
        t_bifeqsinpi,
        {filt, seed, "", ""},
        {1000.0, 0.66, 0, 0},
        HT_BIF, HF_BIFEQSINPI, TRIG1+NOGUESS+NOTRACE+NOROTATE,
        -2.5F, 2.5F, -3.5F, 3.5F,
        0, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::LBIFEQSINPI, symmetry_type::NONE,
        BifurcSetTrigPi, nullptr, StandaloneSetup, Bifurcation,
        NOBAILOUT
    },

    {
        t_popcornjul,
        {step_x, step_y, constant_x, constant_y},
        {0.05, 0, 3.00, 0},
        HT_POPCORN, HF_POPCJUL, TRIG4,
        -3.0F, 3.0F, -2.25F, 2.25F,
        0, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::LPOPCORNJUL, symmetry_type::NONE,
        PopcornFractalFn, otherjuliafp_per_pixel, JuliafpSetup,StandardFractal,
        STDBAILOUT
    },

    {
        t_popcornjul+1,
        {step_x, step_y, constant_x, constant_y},
        {0.05, 0, 3.0, 0},
        HT_POPCORN, HF_POPCJUL, TRIG4,
        -3.0F, 3.0F, -2.25F, 2.25F,
        16, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::FPPOPCORNJUL, symmetry_type::NONE,
        LPopcornFractalFn, long_julia_per_pixel, JulialongSetup, StandardFractal,
        STDBAILOUT
    },

    {
        "lsystem",
        {"+Order", "", "", ""},
        {2, 0, 0, 0},
        HT_LSYS, -3, NOZOOM+NORESUME+NOGUESS+NOTRACE,
        -1.0F, 1.0F, -1.0F, 1.0F,
        1, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, symmetry_type::NONE,
        nullptr, nullptr, StandaloneSetup, Lsystem,
        NOBAILOUT
    },

    {
        t_manowarj,
        {realparm, imagparm, "", ""},
        {0, 0, 0, 0},
        HT_SCOTSKIN, HF_MANOWARJ, OKJB+BAILTEST,
        -2.5F, 1.5F, -1.5F, 1.5F,
        0, fractal_type::NOFRACTAL, fractal_type::MANOWARFP, fractal_type::MANOWARJ, symmetry_type::NONE,
        ManOWarfpFractal, juliafp_per_pixel, JuliafpSetup, StandardFractal,
        STDBAILOUT
    },

    {
        t_manowarj+1,
        {realparm, imagparm, "", ""},
        {0, 0, 0, 0},
        HT_SCOTSKIN, HF_MANOWARJ, OKJB+BAILTEST,
        -2.5F, 1.5F, -1.5F, 1.5F,
        1, fractal_type::NOFRACTAL, fractal_type::MANOWAR, fractal_type::MANOWARJFP, symmetry_type::NONE,
        ManOWarFractal, julia_per_pixel, JulialongSetup, StandardFractal,
        STDBAILOUT
    },

    {
        t_fn_z_plusfn_pix_,
        {realz0, imagz0, recoeftrg2, imcoeftrg2},
        {0, 0, 1, 0},
        HT_SCOTSKIN, HF_FNPLUSFNPIX, TRIG2+BAILTEST,
        -3.6F, 3.6F, -2.7F, 2.7F,
        0, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::FNPLUSFNPIXLONG, symmetry_type::NONE,
        Richard8fpFractal, otherrichard8fp_per_pixel, MandelfpSetup,
        StandardFractal,
        LTRIGBAILOUT
    },

    {
        t_fn_z_plusfn_pix_+1,
        {realz0, imagz0, recoeftrg2, imcoeftrg2},
        {0, 0, 1, 0},
        HT_SCOTSKIN, HF_FNPLUSFNPIX, TRIG2+BAILTEST,
        -3.6F, 3.6F, -2.7F, 2.7F,
        1, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::FNPLUSFNPIXFP, symmetry_type::NONE,
        Richard8Fractal, long_richard8_per_pixel, MandellongSetup,
        StandardFractal,
        LTRIGBAILOUT
    },

    {
        t_marksmandelpwr,
        {realz0, imagz0, "", ""},
        {0, 0, 0, 0},
        HT_MARKS, HF_MARKSMANDPWR, TRIG1+BAILTEST,
        -2.5F, 1.5F, -1.5F, 1.5F,
        0, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::MARKSMANDELPWR, symmetry_type::X_AXIS_NO_PARAM,
        MarksMandelPwrfpFractal, marks_mandelpwrfp_per_pixel, MandelfpSetup,
        StandardFractal,
        STDBAILOUT
    },

    {
        t_marksmandelpwr+1,
        {realz0, imagz0, "", ""},
        {0, 0, 0, 0},
        HT_MARKS, HF_MARKSMANDPWR, TRIG1+BAILTEST,
        -2.5F, 1.5F, -1.5F, 1.5F,
        1, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::MARKSMANDELPWRFP, symmetry_type::X_AXIS_NO_PARAM,
        MarksMandelPwrFractal, marks_mandelpwr_per_pixel, MandellongSetup,
        StandardFractal,
        STDBAILOUT
    },

    {
        t_tims_error,
        {realz0, imagz0, "", ""},
        {0, 0, 0, 0},
        HT_MARKS, HF_TIMSERR, TRIG1+BAILTEST,
        -2.9F, 4.3F, -2.7F, 2.7F,
        0, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::TIMSERROR, symmetry_type::X_AXIS_NO_PARAM,
        TimsErrorfpFractal, marks_mandelpwrfp_per_pixel, MandelfpSetup,
        StandardFractal,
        STDBAILOUT
    },

    {
        t_tims_error+1,
        {realz0, imagz0, "", ""},
        {0, 0, 0, 0},
        HT_MARKS, HF_TIMSERR, TRIG1+BAILTEST,
        -2.9F, 4.3F, -2.7F, 2.7F,
        1, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::TIMSERRORFP, symmetry_type::X_AXIS_NO_PARAM,
        TimsErrorFractal, marks_mandelpwr_per_pixel, MandellongSetup,
        StandardFractal,
        STDBAILOUT
    },

    {
        t_bifeqsinpi+1,
        {filt, seed, "", ""},
        {1000.0, 0.66, 0, 0},
        HT_BIF, HF_BIFEQSINPI, TRIG1+NOGUESS+NOTRACE+NOROTATE,
        -2.5F, 2.5F, -3.5F, 3.5F,
        1, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::BIFEQSINPI, symmetry_type::NONE,
        LongBifurcSetTrigPi, nullptr, StandaloneSetup, Bifurcation,
        NOBAILOUT
    },

    {
        t_bifplussinpi+1,
        {filt, seed, "", ""},
        {1000.0, 0.66, 0, 0},
        HT_BIF, HF_BIFPLUSSINPI, TRIG1+NOGUESS+NOTRACE+NOROTATE,
        0.275F, 1.45F, 0.0F, 2.0F,
        1, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::BIFADSINPI, symmetry_type::NONE,
        LongBifurcAddTrigPi, nullptr, StandaloneSetup, Bifurcation,
        NOBAILOUT
    },

    {
        t_bifstewart,
        {filt, seed, "", ""},
        {1000.0, 0.66, 0, 0},
        HT_BIF, HF_BIFSTEWART, TRIG1+NOGUESS+NOTRACE+NOROTATE,
        0.7F, 2.0F, -1.1F, 1.1F,
        0, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::LBIFSTEWART, symmetry_type::NONE,
        BifurcStewartTrig, nullptr, StandaloneSetup, Bifurcation,
        NOBAILOUT
    },

    {
        t_bifstewart+1,
        {filt, seed, "", ""},
        {1000.0, 0.66, 0, 0},
        HT_BIF, HF_BIFSTEWART, TRIG1+NOGUESS+NOTRACE+NOROTATE,
        0.7F, 2.0F, -1.1F, 1.1F,
        1, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::BIFSTEWART, symmetry_type::NONE,
        LongBifurcStewartTrig, nullptr, StandaloneSetup, Bifurcation,
        NOBAILOUT
    },

    {
        "hopalong",
        {"a", "b", "c", ""},
        {.4, 1, 0, 0},
        HT_MARTIN, HF_HOPALONG, NOGUESS+NOTRACE+INFCALC,
        -2.0F, 3.0F, -1.625F, 2.625F,
        0, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, symmetry_type::NONE,
        (VF)hopalong2dfloatorbit, nullptr, orbit3dfloatsetup, orbit2dfloat,
        NOBAILOUT
    },

    {
        "circle",
        {"magnification", "", "", ""},
        {200000L, 0, 0, 0},
        HT_CIRCLE, HF_CIRCLE, 0,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, symmetry_type::XY_AXIS,
        CirclefpFractal, juliafp_per_pixel, JuliafpSetup, StandardFractal,
        NOBAILOUT
    },

    {
        "martin",
        {"a", "", "", ""},
        {3.14, 0, 0, 0},
        HT_MARTIN, HF_MARTIN, NOGUESS+NOTRACE+INFCALC,
        -32.0F, 32.0F, -24.0F, 24.0F,
        0, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, symmetry_type::NONE,
        (VF)martin2dfloatorbit, nullptr, orbit3dfloatsetup, orbit2dfloat,
        NOBAILOUT
    },

    {
        "lyapunov",
        {"+Order (integer)", "Population Seed", "+Filter Cycles", ""},
        {0, 0.5, 0, 0},
        HT_LYAPUNOV, HT_LYAPUNOV, 0,
        -8.0F, 8.0F, -6.0F, 6.0F,
        0, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, symmetry_type::NONE,
        BifurcLambda, nullptr, lya_setup, lyapunov,
        NOBAILOUT
    },

    {
        "lorenz3d1",
        {timestep, "a", "b", "c"},
        {.02, 5, 15, 1},
        HT_LORENZ, HF_LORENZ3D1,
        NOGUESS+NOTRACE+NORESUME+PARMS3D+INFCALC,
        -30.0F, 30.0F, -30.0F, 30.0F,
        0, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, symmetry_type::NONE,
        (VF)lorenz3d1floatorbit, nullptr, orbit3dfloatsetup, orbit3dfloat,
        NOBAILOUT
    },

    {
        "lorenz3d3",
        {timestep, "a", "b", "c"},
        {.02, 10, 28, 2.66},
        HT_LORENZ, HF_LORENZ3D3,
        NOGUESS+NOTRACE+NORESUME+PARMS3D+INFCALC,
        -30.0F, 30.0F, -30.0F, 30.0F,
        0, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, symmetry_type::NONE,
        (VF)lorenz3d3floatorbit, nullptr, orbit3dfloatsetup, orbit3dfloat,
        NOBAILOUT
    },

    {
        "lorenz3d4",
        {timestep, "a", "b", "c"},
        {.02, 10, 28, 2.66},
        HT_LORENZ, HF_LORENZ3D4,
        NOGUESS+NOTRACE+NORESUME+PARMS3D+INFCALC,
        -30.0F, 30.0F, -30.0F, 30.0F,
        0, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, symmetry_type::NONE,
        (VF)lorenz3d4floatorbit, nullptr, orbit3dfloatsetup, orbit3dfloat,
        NOBAILOUT
    },

    {
        t_lambda_fnorfn_+1,
        {realparm, imagparm, "Function Shift Value", ""},
        {1, 0.1, 1, 0},
        HT_FNORFN, HF_LAMBDAFNFN, TRIG2+BAILTEST,
        -4.0F, 4.0F, -3.0F, 3.0F,
        16, fractal_type::NOFRACTAL, fractal_type::LMANLAMFNFN, fractal_type::FPLAMBDAFNFN, symmetry_type::ORIGIN,
        LambdaTrigOrTrigFractal, long_julia_per_pixel, LambdaTrigOrTrigSetup,
        StandardFractal,
        LTRIGBAILOUT
    },

    {
        t_lambda_fnorfn_,
        {realparm, imagparm, "Function Shift Value", ""},
        {1, 0.1, 1, 0},
        HT_FNORFN, HF_LAMBDAFNFN, TRIG2+BAILTEST,
        -4.0F, 4.0F, -3.0F, 3.0F,
        0, fractal_type::NOFRACTAL, fractal_type::FPMANLAMFNFN, fractal_type::LLAMBDAFNFN, symmetry_type::ORIGIN,
        LambdaTrigOrTrigfpFractal, otherjuliafp_per_pixel,
        LambdaTrigOrTrigSetup, StandardFractal,
        LTRIGBAILOUT
    },

    {
        t_julia_fnorfn_+1,
        {realparm, imagparm, "Function Shift Value", ""},
        {0, 0, 8, 0},
        HT_FNORFN, HF_JULIAFNFN, TRIG2+BAILTEST,
        -4.0F, 4.0F, -3.0F, 3.0F,
        16, fractal_type::NOFRACTAL, fractal_type::LMANFNFN, fractal_type::FPJULFNFN, symmetry_type::X_AXIS,
        JuliaTrigOrTrigFractal, long_julia_per_pixel, JuliaTrigOrTrigSetup,
        StandardFractal,
        LTRIGBAILOUT
    },

    {
        t_julia_fnorfn_,
        {realparm, imagparm, "Function Shift Value", ""},
        {0, 0, 8, 0},
        HT_FNORFN, HF_JULIAFNFN, TRIG2+BAILTEST,
        -4.0F, 4.0F, -3.0F, 3.0F,
        0, fractal_type::NOFRACTAL, fractal_type::FPMANFNFN, fractal_type::LJULFNFN, symmetry_type::X_AXIS,
        JuliaTrigOrTrigfpFractal, otherjuliafp_per_pixel,
        JuliaTrigOrTrigSetup, StandardFractal,
        LTRIGBAILOUT
    },

    {
        t_manlam_fnorfn_+1,
        {realz0, imagz0, "Function Shift Value", ""},
        {0, 0, 10, 0},
        HT_FNORFN, HF_MANLAMFNFN, TRIG2+BAILTEST,
        -4.0F, 4.0F, -3.0F, 3.0F,
        16, fractal_type::LLAMBDAFNFN, fractal_type::NOFRACTAL, fractal_type::FPMANLAMFNFN, symmetry_type::X_AXIS_NO_PARAM,
        LambdaTrigOrTrigFractal, long_mandel_per_pixel,
        ManlamTrigOrTrigSetup, StandardFractal,
        LTRIGBAILOUT
    },

    {
        t_manlam_fnorfn_,
        {realz0, imagz0, "Function Shift Value", ""},
        {0, 0, 10, 0},
        HT_FNORFN, HF_MANLAMFNFN, TRIG2+BAILTEST,
        -4.0F, 4.0F, -3.0F, 3.0F,
        0, fractal_type::FPLAMBDAFNFN, fractal_type::NOFRACTAL, fractal_type::LMANLAMFNFN, symmetry_type::X_AXIS_NO_PARAM,
        LambdaTrigOrTrigfpFractal, othermandelfp_per_pixel,
        ManlamTrigOrTrigSetup, StandardFractal,
        LTRIGBAILOUT
    },

    {
        t_mandel_fnorfn_+1,
        {realz0, imagz0, "Function Shift Value", ""},
        {0, 0, 0.5, 0},
        HT_FNORFN, HF_MANDELFNFN, TRIG2+BAILTEST,
        -4.0F, 4.0F, -3.0F, 3.0F,
        16, fractal_type::LJULFNFN, fractal_type::NOFRACTAL, fractal_type::FPMANFNFN, symmetry_type::X_AXIS_NO_PARAM,
        JuliaTrigOrTrigFractal, long_mandel_per_pixel,
        MandelTrigOrTrigSetup, StandardFractal,
        LTRIGBAILOUT
    },

    {
        t_mandel_fnorfn_,
        {realz0, imagz0, "Function Shift Value", ""},
        {0, 0, 0.5, 0},
        HT_FNORFN, HF_MANDELFNFN, TRIG2+BAILTEST,
        -4.0F, 4.0F, -3.0F, 3.0F,
        0, fractal_type::FPJULFNFN, fractal_type::NOFRACTAL, fractal_type::LMANFNFN, symmetry_type::X_AXIS_NO_PARAM,
        JuliaTrigOrTrigfpFractal, othermandelfp_per_pixel,
        MandelTrigOrTrigSetup, StandardFractal,
        LTRIGBAILOUT
    },

    {
        t_bifmay+1,
        {filt, seed, "Beta >= 2", ""},
        {300.0, 0.9, 5, 0},
        HT_BIF, HF_BIFMAY, NOGUESS+NOTRACE+NOROTATE,
        -3.5F, -0.9F, -0.5F, 3.2F,
        16, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::BIFMAY, symmetry_type::NONE,
        LongBifurcMay, nullptr, BifurcMaySetup, Bifurcation,
        NOBAILOUT
    },

    {
        t_bifmay,
        {filt, seed, "Beta >= 2", ""},
        {300.0, 0.9, 5, 0},
        HT_BIF, HF_BIFMAY, NOGUESS+NOTRACE+NOROTATE,
        -3.5F, -0.9F, -0.5F, 3.2F,
        0, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::LBIFMAY, symmetry_type::NONE,
        BifurcMay, nullptr, BifurcMaySetup, Bifurcation,
        NOBAILOUT
    },

    {
        t_halley+1,
        {order, real_relax, epsilon, imag_relax},
        {6, 1.0, 0.0001, 0},
        HT_HALLEY, HF_HALLEY, 0,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::HALLEY, symmetry_type::XY_AXIS,
        MPCHalleyFractal, MPCHalley_per_pixel, HalleySetup, StandardFractal,
        NOBAILOUT
    },

    {
        t_halley,
        {order, real_relax, epsilon, imag_relax},
        {6, 1.0, 0.0001, 0},
        HT_HALLEY, HF_HALLEY, 0,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::MPHALLEY, symmetry_type::XY_AXIS,
        HalleyFractal, Halley_per_pixel, HalleySetup, StandardFractal,
        NOBAILOUT
    },

    {
        "dynamic",
        {"+# of intervals (<0 = connect)", "time step (<0 = Euler)", "a", "b"},
        {50, .1, 1, 3},
        HT_DYNAM, HF_DYNAM, NOGUESS+NOTRACE+TRIG1,
        -20.0F, 20.0F, -20.0F, 20.0F,
        0, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, symmetry_type::NONE,
        (VF)dynamfloat, nullptr, dynam2dfloatsetup, dynam2dfloat,
        NOBAILOUT
    },

    {
        "quat",
        {"notused", "notused", "cj", "ck"},
        {0, 0, 0, 0},
        HT_QUAT, HF_QUAT, OKJB,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, fractal_type::QUATJULFP, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, symmetry_type::X_AXIS,
        QuaternionFPFractal, quaternionfp_per_pixel, MandelfpSetup,
        StandardFractal,
        LTRIGBAILOUT
    },

    {
        "quatjul",
        {"c1", "ci", "cj", "ck"},
        {-.745, 0, .113, .05},
        HT_QUAT, HF_QUATJ, OKJB+MORE,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, fractal_type::NOFRACTAL, fractal_type::QUATFP, fractal_type::NOFRACTAL, symmetry_type::ORIGIN,
        QuaternionFPFractal, quaternionjulfp_per_pixel, JuliafpSetup,
        StandardFractal,
        LTRIGBAILOUT
    },

    {
        "cellular",
        {cell_init, cell_rule, cell_type, cell_strt},
        {11.0, 3311100320.0, 41.0, 0},
        HT_CELLULAR, HF_CELLULAR, NOGUESS+NOTRACE+NOZOOM,
        -1.0F, 1.0F, -1.0F, 1.0F,
        0, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, symmetry_type::NONE,
        nullptr, nullptr, CellularSetup, cellular,
        NOBAILOUT
    },

    {
        t_julibrot,
        {"", "", "", ""},
        {0, 0, 0, 0},
        HT_JULIBROT, -1, NOGUESS+NOTRACE+NOROTATE+NORESUME,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::JULIBROT, symmetry_type::NONE,
        JuliafpFractal, jbfp_per_pixel, JulibrotSetup, Std4dfpFractal,
        STDBAILOUT
    },

#ifdef RANDOM_RUN
    {
        t_julia_inverse+1,
        {realparm, imagparm, s_maxhits, "Random Run Interval"},
        {-0.11, 0.6557, 4, 1024},
        HT_INVERSE, HF_INVERSE, NOGUESS+NOTRACE+INFCALC+NORESUME,
        -2.0F, 2.0F, -1.5F, 1.5F,
        24, fractal_type::NOFRACTAL, fractal_type::MANDEL, fractal_type::INVERSEJULIAFP, symmetry_type::NONE,
        Linverse_julia_orbit, nullptr, orbit3dlongsetup, inverse_julia_per_image,
        NOBAILOUT
    },

    {
        t_julia_inverse,
        {realparm, imagparm, s_maxhits, "Random Run Interval"},
        {-0.11, 0.6557, 4, 1024},
        HT_INVERSE, HF_INVERSE, NOGUESS+NOTRACE+INFCALC+NORESUME,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, fractal_type::NOFRACTAL, fractal_type::MANDEL, fractal_type::INVERSEJULIA, symmetry_type::NONE,
        Minverse_julia_orbit, nullptr, orbit3dfloatsetup, inverse_julia_per_image,
        NOBAILOUT
    },
#else
    {
        t_julia_inverse+1,
        {realparm, imagparm, s_maxhits, ""},
        {-0.11, 0.6557, 4, 1024},
        HT_INVERSE, HF_INVERSE, NOGUESS+NOTRACE+INFCALC+NORESUME,
        -2.0F, 2.0F, -1.5F, 1.5F,
        24, fractal_type::NOFRACTAL, fractal_type::MANDEL, fractal_type::INVERSEJULIAFP, symmetry_type::NONE,
        Linverse_julia_orbit, nullptr, orbit3dlongsetup, inverse_julia_per_image,
        NOBAILOUT
    },

    {
        t_julia_inverse,
        {realparm, imagparm, s_maxhits, ""},
        {-0.11, 0.6557, 4, 1024},
        HT_INVERSE, HF_INVERSE, NOGUESS+NOTRACE+INFCALC+NORESUME,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, fractal_type::NOFRACTAL, fractal_type::MANDEL, fractal_type::INVERSEJULIA, symmetry_type::NONE,
        Minverse_julia_orbit, nullptr, orbit3dfloatsetup, inverse_julia_per_image,
        NOBAILOUT
    },

#endif

    {
        "mandelcloud",
        {"+# of intervals (<0 = connect)", "", "", ""},
        {50, 0, 0, 0},
        HT_MANDELCLOUD, HF_MANDELCLOUD, NOGUESS+NOTRACE,
        -2.5F, 1.5F, -1.5F, 1.5F,
        0, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, symmetry_type::NONE,
        (VF)mandelcloudfloat, nullptr, dynam2dfloatsetup, dynam2dfloat,
        NOBAILOUT
    },

    {
        t_phoenix+1,
        {p1real, p2real, degreeZ, ""},
        {0.56667, -0.5, 0, 0},
        HT_PHOENIX, HF_PHOENIX, BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        1, fractal_type::NOFRACTAL, fractal_type::MANDPHOENIX, fractal_type::PHOENIXFP, symmetry_type::X_AXIS,
        LongPhoenixFractal, long_phoenix_per_pixel, PhoenixSetup,
        StandardFractal,
        STDBAILOUT
    },

    {
        t_phoenix,
        {p1real, p2real, degreeZ, ""},
        {0.56667, -0.5, 0, 0},
        HT_PHOENIX, HF_PHOENIX, BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, fractal_type::NOFRACTAL, fractal_type::MANDPHOENIXFP, fractal_type::PHOENIX, symmetry_type::X_AXIS,
        PhoenixFractal, phoenix_per_pixel, PhoenixSetup, StandardFractal,
        STDBAILOUT
    },

    {
        t_mandphoenix+1,
        {realz0, imagz0, degreeZ, ""},
        {0.0, 0.0, 0, 0},
        HT_PHOENIX, HF_MANDPHOENIX, BAILTEST,
        -2.5F, 1.5F, -1.5F, 1.5F,
        1, fractal_type::PHOENIX, fractal_type::NOFRACTAL, fractal_type::MANDPHOENIXFP, symmetry_type::NONE,
        LongPhoenixFractal, long_mandphoenix_per_pixel, MandPhoenixSetup,
        StandardFractal,
        STDBAILOUT
    },

    {
        t_mandphoenix,
        {realz0, imagz0, degreeZ, ""},
        {0.0, 0.0, 0, 0},
        HT_PHOENIX, HF_MANDPHOENIX, BAILTEST,
        -2.5F, 1.5F, -1.5F, 1.5F,
        0, fractal_type::PHOENIXFP, fractal_type::NOFRACTAL, fractal_type::MANDPHOENIX, symmetry_type::NONE,
        PhoenixFractal, mandphoenix_per_pixel, MandPhoenixSetup,
        StandardFractal,
        STDBAILOUT
    },

    {
        "hypercomplex",
        {"notused", "notused", "cj", "ck"},
        {0, 0, 0, 0},
        HT_HYPERC, HF_HYPERC, OKJB+TRIG1,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, fractal_type::HYPERCMPLXJFP, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, symmetry_type::X_AXIS,
        HyperComplexFPFractal, quaternionfp_per_pixel, MandelfpSetup,
        StandardFractal,
        LTRIGBAILOUT
    },

    {
        "hypercomplexj",
        {"c1", "ci", "cj", "ck"},
        {-.745, 0, .113, .05},
        HT_HYPERC, HF_HYPERCJ, OKJB+TRIG1+MORE,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, fractal_type::NOFRACTAL, fractal_type::HYPERCMPLXFP, fractal_type::NOFRACTAL, symmetry_type::ORIGIN,
        HyperComplexFPFractal, quaternionjulfp_per_pixel, JuliafpSetup,
        StandardFractal,
        LTRIGBAILOUT
    },

    {
        t_frothybasin+1,
        {frothmapping, frothshade, frothavalue, ""},
        {1, 0, 1.028713768218725, 0},
        HT_FROTH, HF_FROTH, NOTRACE,
        -2.8F, 2.8F, -2.355F, 1.845F,
        28, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::FROTHFP, symmetry_type::NONE,
        froth_per_orbit, froth_per_pixel, froth_setup, calcfroth,
        FROTHBAILOUT
    },

    {
        t_frothybasin,
        {frothmapping, frothshade, frothavalue, ""},
        {1, 0, 1.028713768218725, 0},
        HT_FROTH, HF_FROTH, NOTRACE,
        -2.8F, 2.8F, -2.355F, 1.845F,
        0, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::FROTH, symmetry_type::NONE,
        froth_per_orbit, froth_per_pixel, froth_setup, calcfroth,
        FROTHBAILOUT
    },

    {
        t_mandel4,
        {realz0, imagz0, "", ""},
        {0, 0, 0, 0},
        HT_MANDJUL4, HF_MANDEL4, BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, fractal_type::JULIA4FP, fractal_type::NOFRACTAL, fractal_type::MANDEL4, symmetry_type::X_AXIS_NO_PARAM,
        Mandel4fpFractal, mandelfp_per_pixel, MandelfpSetup, StandardFractal,
        STDBAILOUT
    },

    {
        t_julia4,
        {realparm, imagparm, "", ""},
        {0.6, 0.55, 0, 0},
        HT_MANDJUL4, HF_JULIA4, OKJB+BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, fractal_type::NOFRACTAL, fractal_type::MANDEL4FP, fractal_type::JULIA4, symmetry_type::ORIGIN,
        Mandel4fpFractal, juliafp_per_pixel, JuliafpSetup,StandardFractal,
        STDBAILOUT
    },

    {
        t_marksmandel,
        {realz0, imagz0, exponent, ""},
        {0, 0, 1, 0},
        HT_MARKS, HF_MARKSMAND, BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, fractal_type::MARKSJULIAFP, fractal_type::NOFRACTAL, fractal_type::MARKSMANDEL, symmetry_type::NONE,
        MarksLambdafpFractal, marksmandelfp_per_pixel, MandelfpSetup,
        StandardFractal,
        STDBAILOUT
    },

    {
        t_marksjulia,
        {realparm, imagparm, exponent, ""},
        {0.1, 0.9, 1, 0},
        HT_MARKS, HF_MARKSJULIA, BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, fractal_type::NOFRACTAL, fractal_type::MARKSMANDELFP, fractal_type::MARKSJULIA, symmetry_type::ORIGIN,
        MarksLambdafpFractal, juliafp_per_pixel, MarksJuliafpSetup,
        StandardFractal,
        STDBAILOUT
    },

    {
        "icons",
        {"Lambda", "Alpha", "Beta", "Gamma"},
        {-2.34, 2.0, 0.2, 0.1},
        HT_ICON, HF_ICON, NOGUESS+NOTRACE+INFCALC+MORE,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, symmetry_type::NONE,
        (VF)iconfloatorbit, nullptr, orbit3dfloatsetup, orbit2dfloat,
        NOBAILOUT
    },

    {
        "icons3d",
        {"Lambda", "Alpha", "Beta", "Gamma"},
        {-2.34, 2.0, 0.2, 0.1},
        HT_ICON, HF_ICON, NOGUESS+NOTRACE+INFCALC+PARMS3D+MORE,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, symmetry_type::NONE,
        (VF)iconfloatorbit, nullptr, orbit3dfloatsetup, orbit3dfloat,
        NOBAILOUT
    },

    {
        t_phoenixcplx+1,
        {p1real, p1imag, p2real, p2imag},
        {0.2, 0, 0.3, 0},
        HT_PHOENIX, HF_PHOENIXCPLX, MORE+BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        1, fractal_type::NOFRACTAL, fractal_type::MANDPHOENIXCPLX, fractal_type::PHOENIXFPCPLX, symmetry_type::ORIGIN,
        LongPhoenixFractalcplx, long_phoenix_per_pixel, PhoenixCplxSetup,
        StandardFractal,
        STDBAILOUT
    },

    {
        t_phoenixcplx,
        {p1real, p1imag, p2real, p2imag},
        {0.2, 0, 0.3, 0},
        HT_PHOENIX, HF_PHOENIXCPLX, MORE+BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, fractal_type::NOFRACTAL, fractal_type::MANDPHOENIXFPCPLX, fractal_type::PHOENIXCPLX, symmetry_type::ORIGIN,
        PhoenixFractalcplx, phoenix_per_pixel, PhoenixCplxSetup,
        StandardFractal,
        STDBAILOUT
    },

    {
        t_mandphoenixcplx+1,
        {realz0, imagz0, p2real, p2imag},
        {0, 0, 0.5, 0},
        HT_PHOENIX, HF_MANDPHOENIXCPLX, MORE+BAILTEST,
        -2.5F, 1.5F, -1.5F, 1.5F,
        1, fractal_type::PHOENIXCPLX, fractal_type::NOFRACTAL, fractal_type::MANDPHOENIXFPCPLX, symmetry_type::X_AXIS,
        LongPhoenixFractalcplx, long_mandphoenix_per_pixel,
        MandPhoenixCplxSetup, StandardFractal,
        STDBAILOUT
    },

    {
        t_mandphoenixcplx,
        {realz0, imagz0, p2real, p2imag},
        {0, 0, 0.5, 0},
        HT_PHOENIX, HF_MANDPHOENIXCPLX, MORE+BAILTEST,
        -2.5F, 1.5F, -1.5F, 1.5F,
        0, fractal_type::PHOENIXFPCPLX, fractal_type::NOFRACTAL, fractal_type::MANDPHOENIXCPLX, symmetry_type::X_AXIS,
        PhoenixFractalcplx, mandphoenix_per_pixel, MandPhoenixCplxSetup,
        StandardFractal,
        STDBAILOUT
    },

    {
        "ant",
        {   "#Rule String (1's and non-1's, 0 rand)",
            "#Maxpts",
            "+Numants (max 256)",
            "+Ant type (1 or 2)"
        },
        {1100, 1.0E9, 1, 1},
        HT_ANT, HF_ANT, NOZOOM+NOGUESS+NOTRACE+NORESUME+MORE,
        -1.0F, 1.0F, -1.0F, 1.0F,
        0, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, symmetry_type::NONE,
        nullptr, nullptr, StandaloneSetup, ant,
        NOBAILOUT
    },

    {
        "chip",
        {"a", "b", "c", ""},
        {-15,-19,1,0},
        HT_MARTIN, HF_CHIP, NOGUESS+NOTRACE+INFCALC,
        -760.0F, 760.0F, -570.0F, 570.0F,
        0, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, symmetry_type::NONE,
        (VF)chip2dfloatorbit, nullptr, orbit3dfloatsetup, orbit2dfloat,
        NOBAILOUT
    },

    {
        "quadruptwo",
        {"a", "b", "c", ""},
        {34, 1, 5, 0},
        HT_MARTIN, HF_QUADRUPTWO, NOGUESS+NOTRACE+INFCALC,
        -82.93367F, 112.2749F, -55.76383F, 90.64257F,
        0, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, symmetry_type::NONE,
        (VF)quadruptwo2dfloatorbit, nullptr, orbit3dfloatsetup, orbit2dfloat,
        NOBAILOUT
    },

    {
        "threeply",
        {"a", "b", "c", ""},
        {-55, -1, -42, 0},
        HT_MARTIN, HF_THREEPLY, NOGUESS+NOTRACE+INFCALC,
        -8000.0F, 8000.0F, -6000.0F, 6000.0F,
        0, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, symmetry_type::NONE,
        (VF)threeply2dfloatorbit, nullptr, orbit3dfloatsetup, orbit2dfloat,
        NOBAILOUT
    },

    {
        "volterra-lotka",
        {"h", "p", "", ""},
        {0.739, 0.739, 0, 0},
        HT_VL, HF_VL, 0,
        0.0F, 6.0F, 0.0F, 4.5F,
        0, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, symmetry_type::NONE,
        VLfpFractal, otherjuliafp_per_pixel, VLSetup, StandardFractal,
        256
    },

    {
        "escher_julia",
        {realparm, imagparm, "", ""},
        {0.32, 0.043, 0, 0},
        HT_ESCHER, HF_ESCHER, 0,
        -1.6F, 1.6F, -1.2F, 1.2F,
        0, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, symmetry_type::ORIGIN,
        EscherfpFractal, juliafp_per_pixel, StandardSetup,
        StandardFractal,
        STDBAILOUT
    },

    // From Pickovers' "Chaos in Wonderland"
    // included by Humberto R. Baptista
    // code adapted from king.cpp bt James Rankin

    {
        "latoocarfian",
        {"a", "b", "c", "d"},
        {-0.966918, 2.879879, 0.765145, 0.744728},
        HT_LATOO, HF_LATOO, NOGUESS+NOTRACE+INFCALC+MORE+TRIG4,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, symmetry_type::NONE,
        (VF)latoofloatorbit, nullptr, orbit3dfloatsetup, orbit2dfloat,
        NOBAILOUT
    },
    {
        "mandelbrotmix4",
        {p1real, p1imag, p2real, p2imag},
        {0.05, 3, -1.5, -2},
        HT_MANDELBROTMIX4, HF_MANDELBROTMIX4, BAILTEST+TRIG1+MORE,
        -2.5F, 1.5F, -1.5F, 1.5F,
        0, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, symmetry_type::NONE,
        MandelbrotMix4fpFractal, MandelbrotMix4fp_per_pixel, MandelbrotMix4Setup, StandardFractal,
        STDBAILOUT
    },

    {
        nullptr,            // marks the END of the list
        {nullptr, nullptr, nullptr, nullptr},
        {0, 0, 0, 0},
        -1, -1, 0,
        0.0F, 0.0F, 0.0F, 0.0F,
        0, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, fractal_type::NOFRACTAL, symmetry_type::NONE,
        nullptr, nullptr, nullptr, nullptr,
        0
    }
};

int num_fractal_types = (sizeof(fractalspecific)/
                         sizeof(fractalspecificstuff)) -1;

/*
 *  Returns 1 if the formula parameter is not used in the current
 *  formula.  If the parameter is used, or not a formula fractal,
 *  a 0 is returned.  Note: this routine only works for formula types.
 */
bool paramnotused(int parm)
{
    bool ret = false;

    // sanity check
    if (fractype != fractal_type::FORMULA && fractype != fractal_type::FFORMULA)
        return false;

    switch (parm/2)
    {
    case 0:
        if (!uses_p1)
            ret = true;
        break;
    case 1:
        if (!uses_p2)
            ret = true;
        break;
    case 2:
        if (!uses_p3)
            ret = true;
        break;
    case 3:
        if (!uses_p4)
            ret = true;
        break;
    case 4:
        if (!uses_p5)
            ret = true;
        break;
    default:
        ret = false;
        break;
    }
    return (ret);
}

/*
 *  Returns 1 if parameter number parm exists for type. If the
 *  parameter exists, the parameter prompt string is copied to buf.
 *  Pass in nullptr for buf if only the existence of the parameter is
 *  needed, and not the prompt string.
 */
bool typehasparm(fractal_type type, int parm, char *buf)
{
    const char *ret = nullptr;
    if (0 <= parm && parm < 4)
        ret = fractalspecific[static_cast<int>(type)].param[parm];
    else if (parm >= 4 && parm < MAXPARAMS)
    {
        int const extra = find_extra_param(type);
        if (extra > -1)
        {
            ret = moreparams[extra].param[parm-4];
        }
    }
    if (ret)
        if (*ret == 0)
            ret = nullptr;

    if (type == fractal_type::FORMULA || type == fractal_type::FFORMULA)
        if (paramnotused(parm))
            ret = nullptr;

    if (ret && buf != nullptr)
        strcpy(buf,ret);
    return ret != nullptr;
}
