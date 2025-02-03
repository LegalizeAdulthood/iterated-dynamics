// SPDX-License-Identifier: GPL-3.0-only
//
// This module consists only of the fractalspecific structure
//
#include "fractals/fractalp.h"

#include "engine/fractalb.h"
#include "engine/fractals.h"
#include "fractals/ant.h"
#include "fractals/barnsley.h"
#include "fractals/bifurcation.h"
#include "fractals/burning_ship.h"
#include "fractals/cellular.h"
#include "fractals/circle_pattern.h"
#include "fractals/diffusion.h"
#include "fractals/divide_brot.h"
#include "fractals/escher.h"
#include "fractals/fn_or_fn.h"
#include "fractals/frasetup.h"
#include "fractals/frothy_basin.h"
#include "fractals/halley.h"
#include "fractals/hypercomplex_mandelbrot.h"
#include "fractals/jb.h"
#include "fractals/lambda_fn.h"
#include "fractals/lorenz.h"
#include "fractals/lsys_fns.h"
#include "fractals/lyapunov.h"
#include "fractals/magnet.h"
#include "fractals/mandelbrot_mix.h"
#include "fractals/newton.h"
#include "fractals/parser.h"
#include "fractals/peterson_variations.h"
#include "fractals/phoenix.h"
#include "fractals/pickover_mandelbrot.h"
#include "fractals/plasma.h"
#include "fractals/popcorn.h"
#include "fractals/quartic_mandelbrot.h"
#include "fractals/quaternion_mandelbrot.h"
#include "fractals/sierpinski_gasket.h"
#include "fractals/taylor_skinner_variations.h"
#include "fractals/testpt.h"
#include "fractals/unity.h"
#include "fractals/volterra_lotka.h"

// parameter descriptions
// Note: + prefix denotes integer parameters
//       # prefix denotes U32 parameters

// for Mandelbrots
static constexpr const char *REAL_Z0{"Real Perturbation of Z(0)"};
static constexpr const char *IMAG_Z0{"Imaginary Perturbation of Z(0)"};

// for Julias
static constexpr const char *REAL_PARAM{"Real Part of Parameter"};
static constexpr const char *IMAG_PARAM{"Imaginary Part of Parameter"};

// for Newtons
static constexpr const char *NEWT_DEGREE{"+Polynomial Degree (>= 2)"};

// for MarksMandel/Julia
static constexpr const char *RE_EXPONENT{"Real part of Exponent"};
static constexpr const char *IM_EXPONENT{"Imag part of Exponent"};

// for Lorenz
static constexpr const char *TIME_STEP{"Time Step"};

// for formula
static constexpr const char *P1_REAL{"Real portion of p1"};
static constexpr const char *P2_REAL{"Real portion of p2"};
static constexpr const char *P3_REAL{"Real portion of p3"};
static constexpr const char *P4_REAL{"Real portion of p4"};
static constexpr const char *P5_REAL{"Real portion of p5"};
static constexpr const char *P1_IMAG{"Imaginary portion of p1"};
static constexpr const char *P2_IMAG{"Imaginary portion of p2"};
static constexpr const char *P3_IMAG{"Imaginary portion of p3"};
static constexpr const char *P4_IMAG{"Imaginary portion of p4"};
static constexpr const char *P5_IMAG{"Imaginary portion of p5"};

// trig functions
static constexpr const char *RE_COEF_TRIG1{"Real Coefficient First Function"};
static constexpr const char *IM_COEF_TRIG1{"Imag Coefficient First Function"};
static constexpr const char *RE_COEF_TRIG2{"Real Coefficient Second Function"};
static constexpr const char *IM_COEF_TRIG2{"Imag Coefficient Second Function"};

// KAM Torus
static constexpr const char *KAM_ANGLE{"Angle (radians)"};
static constexpr const char *KAM_STEP{"Step size"};
static constexpr const char *KAM_STOP{"Stop value"};
static constexpr const char *POINTS_PER_ORBIT{"+Points per orbit"};

// popcorn and julia popcorn generalized
static constexpr const char *STEP_X{"Step size (real)"};
static constexpr const char *STEP_Y{"Step size (imaginary)"};
static constexpr const char *CONSTANT_X{"Constant C (real)"};
static constexpr const char *CONSTANT_Y{"Constant C (imaginary)"};

// bifurcations
static constexpr const char *FILT{"+Filter Cycles"};
static constexpr const char *SEED_POP{"Seed Population"};

// frothy basins
static constexpr const char *FROTH_MAPPING{"+Apply mapping once (1) or twice (2)"};
static constexpr const char *FROTH_SHADE{"+Enter non-zero value for alternate color shading"};
static constexpr const char *FROTH_A_VALUE{"A (imaginary part of C)"};

// plasma and ant
static constexpr const char *RANDOM_SEED{"+Random Seed Value (0 = Random, 1 = Reuse Last)"};

// ifs
static constexpr const char *COLOR_METHOD{"+Coloring method (0,1)"};

// phoenix fractals
static constexpr const char *DEGREE_Z{"Degree = 0 | >= 2 | <= -3"};

// julia inverse
static constexpr const char *MAX_HITS{"Max Hits per Pixel"};

// halley
static constexpr const char *ORDER{"+Order (integer > 1)"};
static constexpr const char *REAL_RELAX{"Real Relaxation coefficient"};
static constexpr const char *EPSILON{"Epsilon"};
static constexpr const char *IMAG_RELAX{"Imag Relaxation coefficient"};

// cellular
static constexpr const char *CELL_INIT{"#Initial String | 0 = Random | -1 = Reuse Last Random"};
static constexpr const char *CELL_RULE{"#Rule = # of digits (see below) | 0 = Random"};
static constexpr const char *CELL_TYPE{"+Type (see below)"};
static constexpr const char *CELL_START{"#Starting Row Number"};

// bailout values
enum
{
    TRIG_BAILOUT_L = 64,
    FROTH_BAILOUT = 7,
    STD_BAILOUT = 4,
    NO_BAILOUT = 0,
};

// clang-format off
MoreParams g_more_fractal_params[] =
{
    {FractalType::ICON,                 {"Omega", "+Degree of symmetry", "", "", "", ""}, {0, 3, 0, 0, 0, 0}},
    {FractalType::ICON_3D,              {"Omega", "+Degree of symmetry", "", "", "", ""}, {0, 3, 0, 0, 0, 0}},
    {FractalType::HYPER_CMPLX_J_FP,     {"zj", "zk", "", "", "", ""}, {0, 0, 0, 0, 0, 0}},
    {FractalType::QUAT_JUL_FP,          {"zj", "zk", "", "", "", ""}, {0, 0, 0, 0, 0, 0}},
    {FractalType::PHOENIX_CPLX,         {DEGREE_Z, "", "", "", "", ""}, {0, 0, 0, 0, 0, 0}},
    {FractalType::PHOENIX_FP_CPLX,      {DEGREE_Z, "", "", "", "", ""}, {0, 0, 0, 0, 0, 0}},
    {FractalType::MAND_PHOENIX_CPLX,    {DEGREE_Z, "", "", "", "", ""}, {0, 0, 0, 0, 0, 0}},
    {FractalType::MAND_PHOENIX_FP_CPLX, {DEGREE_Z, "", "", "", "", ""}, {0, 0, 0, 0, 0, 0}},
    {FractalType::FORMULA,              {P3_REAL, P3_IMAG, P4_REAL, P4_IMAG, P5_REAL, P5_IMAG}, {0, 0, 0, 0, 0, 0}},
    {FractalType::FORMULA_FP,           {P3_REAL, P3_IMAG, P4_REAL, P4_IMAG, P5_REAL, P5_IMAG}, {0, 0, 0, 0, 0, 0}},
    {FractalType::ANT,                  {"+Wrap?", RANDOM_SEED, "", "", "", ""}, {1, 0, 0, 0, 0, 0}},
    {FractalType::MANDELBROT_MIX4,      {P3_REAL, P3_IMAG, "", "", "", ""}, {0, 0, 0, 0, 0, 0}},
    {FractalType::NO_FRACTAL,           {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}, {0, 0, 0, 0, 0, 0}}
};
// clang-format on

//   type math orbitcalc fnct per_pixel fnct per_image fnct
// |-----|----|--------------|--------------|--------------|
AlternateMath g_alternate_math[] =
{
#define USE_BN
#ifdef USE_BN
    {FractalType::JULIA_FP, BFMathType::BIG_NUM, julia_bn_fractal, julia_bn_per_pixel,  mandel_bn_setup},
    {FractalType::MANDEL_FP, BFMathType::BIG_NUM, julia_bn_fractal, mandel_bn_per_pixel, mandel_bn_setup},
#else
    {FractalType::JULIA_FP, BFMathType::BIG_FLT, julia_bf_fractal, julia_bf_per_pixel,  mandel_bf_setup},
    {FractalType::MANDEL_FP, BFMathType::BIG_FLT, julia_bf_fractal, mandel_bf_per_pixel, mandel_bf_setup},
#endif
    /*
    NOTE: The default precision for g_bf_math=BIGNUM is not high enough
          for JuliaZpowerbnFractal.  If you want to test BIGNUM (1) instead
          of the usual BIGFLT (2), then set bfdigits on the command to
          increase the precision.
    */
    {FractalType::JULIA_Z_POWER_FP, BFMathType::BIG_FLT, julia_z_power_bf_fractal, julia_bf_per_pixel, mandel_bf_setup},
    {FractalType::MANDEL_Z_POWER_FP, BFMathType::BIG_FLT, julia_z_power_bf_fractal, mandel_bf_per_pixel, mandel_bf_setup},
    {FractalType::DIVIDE_BROT5, BFMathType::BIG_FLT, divide_brot5_bf_fractal, divide_brot5_bf_per_pixel, mandel_bf_setup},
    {FractalType::BURNING_SHIP, BFMathType::BIG_FLT, burning_ship_bf_fractal, mandel_bf_per_pixel, mandel_bf_setup},
    {FractalType::NO_FRACTAL, BFMathType::NONE, nullptr, nullptr, nullptr}
};

// These are only needed for types with both integer and float variations
static constexpr const char *T_BARNSLEY_J1{"*barnsleyj1"};
static constexpr const char *T_BARNSLEY_J2{"*barnsleyj2"};
static constexpr const char *T_BARNSLEY_J3{"*barnsleyj3"};
static constexpr const char *T_BARNSLEY_M1{"*barnsleym1"};
static constexpr const char *T_BARNSLEY_M2{"*barnsleym2"};
static constexpr const char *T_BARNSLEY_M3{"*barnsleym3"};
static constexpr const char *T_BIF_PLUS_SIN_PI{"*bif+sinpi"};
static constexpr const char *T_BIF_EQ_SIN_PI{"*bif=sinpi"};
static constexpr const char *T_BIF_LAMBDA{"*biflambda"};
static constexpr const char *T_BIF_MAY{"*bifmay"};
static constexpr const char *T_BIF_STEWART{"*bifstewart"};
static constexpr const char *T_BIFURCATION{"*bifurcation"};
static constexpr const char *T_FN_Z_PLUS_FN_PIX{"*fn(z)+fn(pix)"};
static constexpr const char *T_FN_ZZ{"*fn(z*z)"};
static constexpr const char *T_FN_FN{"*fn*fn"};
static constexpr const char *T_FN_Z_PLUS_Z{"*fn*z+z"};
static constexpr const char *T_FN_PLUS_FN{"*fn+fn"};
static constexpr const char *T_FORMULA{"*formula"};
static constexpr const char *T_HENON{"*henon"};
static constexpr const char *T_IFS3D{"*ifs3d"};
static constexpr const char *T_JUL_FN_PLUS_EXP{"*julfn+exp"};
static constexpr const char *T_JUL_FN_PLUS_Z_SQRD{"*julfn+zsqrd"};
static constexpr const char *T_JULIA{"*julia"};
static constexpr const char *T_JULIA_FN_OR_FN{"*julia(fn||fn)"};
static constexpr const char *T_JULIA4{"*julia4"};
static constexpr const char *T_JULIA_INVERSE{"*julia_inverse"};
static constexpr const char *T_JULIBROT{"*julibrot"};
static constexpr const char *T_JUL_Z_POWER{"*julzpower"};
static constexpr const char *T_KAM_TORUS{"*kamtorus"};
static constexpr const char *T_KAM_TORUS3D{"*kamtorus3d"};
static constexpr const char *T_LAMBDA{"*lambda"};
static constexpr const char *T_LAMBDA_FN_OR_FN{"*lambda(fn||fn)"};
static constexpr const char *T_LAMBDA_FN{"*lambdafn"};
static constexpr const char *T_LORENZ{"*lorenz"};
static constexpr const char *T_LORENZ3D{"*lorenz3d"};
static constexpr const char *T_MANDEL{"*mandel"};
static constexpr const char *T_MANDEL_FN_OR_FN{"*mandel(fn||fn)"};
static constexpr const char *T_MANDEL4{"*mandel4"};
static constexpr const char *T_MANDEL_FN{"*mandelfn"};
static constexpr const char *T_MANDEL_LAMBDA{"*mandellambda"};
static constexpr const char *T_MAND_PHOENIX{"*mandphoenix"};
static constexpr const char *T_MAND_PHOENIX_CPLX{"*mandphoenixclx"};
static constexpr const char *T_MAN_FN_PLUS_EXP{"*manfn+exp"};
static constexpr const char *T_MAN_FN_PLUS_Z_SQRD{"*manfn+zsqrd"};
static constexpr const char *T_MAN_LAM_FN_OR_FN{"*manlam(fn||fn)"};
static constexpr const char *T_MAN_O_WAR{"*manowar"};
static constexpr const char *T_MAN_O_WAR_J{"*manowarj"};
static constexpr const char *T_MAN_Z_POWER{"*manzpower"};
static constexpr const char *T_MARKS_JULIA{"*marksjulia"};
static constexpr const char *T_MARKS_MANDEL{"*marksmandel"};
static constexpr const char *T_MARKS_MANDEL_PWR{"*marksmandelpwr"};
static constexpr const char *T_NEWT_BASIN{"*newtbasin"};
static constexpr const char *T_NEWTON{"*newton"};
static constexpr const char *T_PHOENIX{"*phoenix"};
static constexpr const char *T_PHOENIX_CPLX{"*phoenixcplx"};
static constexpr const char *T_POPCORN{"*popcorn"};
static constexpr const char *T_POPCORN_JUL{"*popcornjul"};
static constexpr const char *T_ROSSLER3D{"*rossler3d"};
static constexpr const char *T_SIERPINSKI{"*sierpinski"};
static constexpr const char *T_SPIDER{"*spider"};
static constexpr const char *T_SQR_1_DIV_FN{"*sqr(1/fn)"};
static constexpr const char *T_SQR_FN{"*sqr(fn)"};
static constexpr const char *T_TIMS_ERROR{"*tim's_error"};
static constexpr const char *T_UNITY{"*unity"};
static constexpr const char *T_FROTHY_BASIN{"*frothybasin"};
static constexpr const char *T_HALLEY{"*halley"};

// use next to cast orbitcalcs() that have arguments
using VF = int(*)();

// This array is indexed by the enum fractal_type.
FractalSpecific g_fractal_specific[] =
{
    /*
    {
        fractal name,
        {parameter text strings},
        {parameter values},
        help text, help formula,
        flags,
        x min, x max, y min, y max,
        int,
        to julia, to mandel, to float,
        symmetry,
        orbit fn, per_pixel fn, per_image fn, calc type fn,
        bailout,
        perturbation ref pt, perturbation bf ref pt, perturbation point
    }
    */

    {
        FractalType::MANDEL,                                                 //
        T_MANDEL + 1,                                                        //
        {REAL_Z0, IMAG_Z0, "", ""},                                          //
        {0, 0, 0, 0},                                                        //
        HelpLabels::HT_MANDEL, HelpLabels::HF_MANDEL,                        //
        FractalFlags::BAIL_TEST | FractalFlags::PERTURB,                     //
        -2.5F, 1.5F, -1.5F, 1.5F,                                            //
        1,                                                                   //
        FractalType::JULIA, FractalType::NO_FRACTAL, FractalType::MANDEL_FP, //
        SymmetryType::X_AXIS_NO_PARAM,                                       //
        julia_fractal, mandel_per_pixel, mandel_setup, standard_fractal,     //
        STD_BAILOUT,                                                         //
        mandel_ref_pt, mandel_ref_pt_bf, mandel_perturb                      //
    },

    {
        FractalType::JULIA,                                                  //
        T_JULIA + 1,                                                         //
        {REAL_PARAM, IMAG_PARAM, "", ""},                                    //
        {0.3, 0.6, 0, 0},                                                    //
        HelpLabels::HT_JULIA, HelpLabels::HF_JULIA,                          //
        FractalFlags::OK_JB | FractalFlags::BAIL_TEST,                       //
        -2.0F, 2.0F, -1.5F, 1.5F,                                            //
        1,                                                                   //
        FractalType::NO_FRACTAL, FractalType::MANDEL, FractalType::JULIA_FP, //
        SymmetryType::ORIGIN,                                                //
        julia_fractal, julia_per_pixel, julia_setup, standard_fractal,       //
        STD_BAILOUT                                                          //
    },

    {
        FractalType::NEWT_BASIN,                                                      //
        T_NEWT_BASIN,                                                                 //
        {NEWT_DEGREE, "Enter non-zero value for stripes", "", ""},                    //
        {3, 0, 0, 0},                                                                 //
        HelpLabels::HT_NEWTON_BASINS, HelpLabels::HF_NEWTON_BASIN,                    //
        FractalFlags::NONE,                                                           //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                     //
        0,                                                                            //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::NEWT_BASIN_MP, //
        SymmetryType::NONE,                                                           //
        newton_fractal2, other_julia_fp_per_pixel, newton_setup, standard_fractal,    //
        NO_BAILOUT                                                                    //
    },

    {
        FractalType::LAMBDA,                                                         //
        T_LAMBDA + 1,                                                                //
        {REAL_PARAM, IMAG_PARAM, "", ""},                                            //
        {0.85, 0.6, 0, 0},                                                           //
        HelpLabels::HT_LAMBDA, HelpLabels::HF_LAMBDA,                                //
        FractalFlags::OK_JB | FractalFlags::BAIL_TEST,                               //
        -1.5F, 2.5F, -1.5F, 1.5F,                                                    //
        1,                                                                           //
        FractalType::NO_FRACTAL, FractalType::MANDEL_LAMBDA, FractalType::LAMBDA_FP, //
        SymmetryType::NONE,                                                          //
        lambda_fractal, julia_per_pixel, julia_long_setup, standard_fractal,         //
        STD_BAILOUT                                                                  //
    },

    {
        FractalType::MANDEL_FP,                                                   //
        T_MANDEL,                                                                 //
        {REAL_Z0, IMAG_Z0, "", ""},                                               //
        {0, 0, 0, 0},                                                             //
        HelpLabels::HT_MANDEL, HelpLabels::HF_MANDEL,                             //
        FractalFlags::BAIL_TEST | FractalFlags::BF_MATH | FractalFlags::PERTURB,  //
        -2.5F, 1.5F, -1.5F, 1.5F,                                                 //
        0,                                                                        //
        FractalType::JULIA_FP, FractalType::NO_FRACTAL, FractalType::MANDEL,      //
        SymmetryType::X_AXIS_NO_PARAM,                                            //
        julia_fp_fractal, mandel_fp_per_pixel, mandel_fp_setup, standard_fractal, //
        STD_BAILOUT,                                                              //
        mandel_ref_pt, mandel_ref_pt_bf, mandel_perturb                           //
    },

    {
        FractalType::NEWTON,                                                       //
        T_NEWTON,                                                                  //
        {NEWT_DEGREE, "", "", ""},                                                 //
        {3, 0, 0, 0},                                                              //
        HelpLabels::HT_NEWTON, HelpLabels::HF_NEWTON,                              //
        FractalFlags::NONE,                                                        //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                  //
        0,                                                                         //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::NEWTON_MP,  //
        SymmetryType::X_AXIS,                                                      //
        newton_fractal2, other_julia_fp_per_pixel, newton_setup, standard_fractal, //
        NO_BAILOUT                                                                 //
    },

    {
        FractalType::JULIA_FP,                                                  //
        T_JULIA,                                                                //
        {REAL_PARAM, IMAG_PARAM, "", ""},                                       //
        {0.3, 0.6, 0, 0},                                                       //
        HelpLabels::HT_JULIA, HelpLabels::HF_JULIA,                             //
        FractalFlags::OK_JB | FractalFlags::BAIL_TEST | FractalFlags::BF_MATH,  //
        -2.0F, 2.0F, -1.5F, 1.5F,                                               //
        0,                                                                      //
        FractalType::NO_FRACTAL, FractalType::MANDEL_FP, FractalType::JULIA,    //
        SymmetryType::ORIGIN,                                                   //
        julia_fp_fractal, julia_fp_per_pixel, julia_fp_setup, standard_fractal, //
        STD_BAILOUT                                                             //
    },

    {
        FractalType::PLASMA,                                                                               //
        "plasma",                                                                                          //
        {
            "Graininess Factor (0 or 0.125 to 100, default is 2)",                                         //
            "+Algorithm (0 = original, 1 = new)",                                                          //
            "+Random Seed Value (0 = Random, 1 = Reuse Last)",                                             //
            "+Save as Pot File? (0 = No,     1 = Yes)"                                                     //
        },                                                                                                 //
        {2, 0, 0, 0},                                                                                      //
        HelpLabels::HT_PLASMA, HelpLabels::HF_PLASMA,                                                      //
        FractalFlags::NO_ZOOM | FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::NO_RESUME, //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                                          //
        1,                                                                                                 //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,                         //
        SymmetryType::NONE,                                                                                //
        nullptr, nullptr, standalone_setup, plasma,                                                        //
        NO_BAILOUT                                                                                         //
    },

    {
        FractalType::MANDEL_TRIG_FP,                                                    //
        T_MANDEL_FN,                                                                    //
        {REAL_Z0, IMAG_Z0, "", ""},                                                     //
        {0, 0, 0, 0},                                                                   //
        HelpLabels::HT_MANDEL_FN, HelpLabels::HF_MANDEL_FN,                             //
        FractalFlags::TRIG1,                                                            //
        -8.0F, 8.0F, -6.0F, 6.0F,                                                       //
        0,                                                                              //
        FractalType::LAMBDA_TRIG_FP, FractalType::NO_FRACTAL, FractalType::MANDEL_TRIG, //
        SymmetryType::XY_AXIS_NO_PARAM,                                                 //
        lambda_trig_fp_fractal, other_mandel_fp_per_pixel, mandel_trig_setup,           //
        standard_fractal,                                                               //
        TRIG_BAILOUT_L                                                                  //
    },

    {
        FractalType::MAN_O_WAR_FP,                                                    //
        T_MAN_O_WAR,                                                                  //
        {REAL_Z0, IMAG_Z0, "", ""},                                                   //
        {0, 0, 0, 0},                                                                 //
        HelpLabels::HT_TAYLOR_SKINNER_VARIATIONS, HelpLabels::HF_MAN_O_WAR,           //
        FractalFlags::BAIL_TEST,                                                      //
        -2.5F, 1.5F, -1.5F, 1.5F,                                                     //
        0,                                                                            //
        FractalType::MAN_O_WAR_J_FP, FractalType::NO_FRACTAL, FractalType::MAN_O_WAR, //
        SymmetryType::X_AXIS_NO_PARAM,                                                //
        man_o_war_fp_fractal, mandel_fp_per_pixel, mandel_fp_setup, standard_fractal, //
        STD_BAILOUT                                                                   //
    },

    {
        FractalType::MAN_O_WAR,                                                       //
        T_MAN_O_WAR + 1,                                                              //
        {REAL_Z0, IMAG_Z0, "", ""},                                                   //
        {0, 0, 0, 0},                                                                 //
        HelpLabels::HT_TAYLOR_SKINNER_VARIATIONS, HelpLabels::HF_MAN_O_WAR,           //
        FractalFlags::BAIL_TEST,                                                      //
        -2.5F, 1.5F, -1.5F, 1.5F,                                                     //
        1,                                                                            //
        FractalType::MAN_O_WAR_J, FractalType::NO_FRACTAL, FractalType::MAN_O_WAR_FP, //
        SymmetryType::X_AXIS_NO_PARAM,                                                //
        man_o_war_fractal, mandel_per_pixel, mandel_long_setup, standard_fractal,     //
        STD_BAILOUT                                                                   //
    },

    {
        FractalType::TEST,                                                         //
        "test",                                                                    //
        {
            "(testpt Param #1)",                                                   //
            "(testpt param #2)",                                                   //
            "(testpt param #3)",                                                   //
            "(testpt param #4)"                                                    //
        },
        {0, 0, 0, 0},                                                              //
        HelpLabels::HT_TEST, HelpLabels::HF_TEST,                                  //
        FractalFlags::NONE,                                                        //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                  //
        0,                                                                         //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, //
        SymmetryType::NONE,                                                        //
        nullptr, nullptr, standalone_setup, test,                                  //
        STD_BAILOUT                                                                //
    },

    {
        FractalType::SIERPINSKI,                                                      //
        T_SIERPINSKI + 1,                                                             //
        {"", "", "", ""},                                                             //
        {0, 0, 0, 0},                                                                 //
        HelpLabels::HT_SIERPINSKI, HelpLabels::HF_SIERPINSKI,                         //
        FractalFlags::NONE,                                                           //
        -4.0F / 3.0F, 96.0F / 45.0F, -0.9F, 1.7F,                                     //
        1,                                                                            //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::SIERPINSKI_FP, //
        SymmetryType::NONE,                                                           //
        sierpinski_fractal, long_julia_per_pixel, sierpinski_setup,                   //
        standard_fractal,                                                             //
        127                                                                           //
    },

    {
        FractalType::BARNSLEY_M1,                                                       //
        T_BARNSLEY_M1 + 1,                                                              //
        {REAL_Z0, IMAG_Z0, "", ""},                                                     //
        {0, 0, 0, 0},                                                                   //
        HelpLabels::HT_BARNSLEY, HelpLabels::HF_BARNSLEY_M1,                            //
        FractalFlags::BAIL_TEST,                                                        //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                       //
        1,                                                                              //
        FractalType::BARNSLEY_J1, FractalType::NO_FRACTAL, FractalType::BARNSLEY_M1_FP, //
        SymmetryType::XY_AXIS_NO_PARAM,                                                 //
        barnsley1_fractal, long_mandel_per_pixel, mandel_long_setup,                    //
        standard_fractal,                                                               //
        STD_BAILOUT                                                                     //
    },

    {
        FractalType::BARNSLEY_J1,                                                       //
        T_BARNSLEY_J1 + 1,                                                              //
        {REAL_PARAM, IMAG_PARAM, "", ""},                                               //
        {0.6, 1.1, 0, 0},                                                               //
        HelpLabels::HT_BARNSLEY, HelpLabels::HF_BARNSLEY_J1,                            //
        FractalFlags::OK_JB | FractalFlags::BAIL_TEST,                                  //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                       //
        1,                                                                              //
        FractalType::NO_FRACTAL, FractalType::BARNSLEY_M1, FractalType::BARNSLEY_J1_FP, //
        SymmetryType::ORIGIN,                                                           //
        barnsley1_fractal, long_julia_per_pixel, julia_long_setup, standard_fractal,    //
        STD_BAILOUT                                                                     //
    },

    {
        FractalType::BARNSLEY_M2,                                                       //
        T_BARNSLEY_M2 + 1,                                                              //
        {REAL_Z0, IMAG_Z0, "", ""},                                                     //
        {0, 0, 0, 0},                                                                   //
        HelpLabels::HT_BARNSLEY, HelpLabels::HF_BARNSLEY_M2,                            //
        FractalFlags::BAIL_TEST,                                                        //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                       //
        1,                                                                              //
        FractalType::BARNSLEY_J2, FractalType::NO_FRACTAL, FractalType::BARNSLEY_M2_FP, //
        SymmetryType::Y_AXIS_NO_PARAM,                                                  //
        barnsley2_fractal, long_mandel_per_pixel, mandel_long_setup,                    //
        standard_fractal,                                                               //
        STD_BAILOUT                                                                     //
    },

    {
        FractalType::BARNSLEY_J2,                                                       //
        T_BARNSLEY_J2 + 1,                                                              //
        {REAL_PARAM, IMAG_PARAM, "", ""},                                               //
        {0.6, 1.1, 0, 0},                                                               //
        HelpLabels::HT_BARNSLEY, HelpLabels::HF_BARNSLEY_J2,                            //
        FractalFlags::OK_JB | FractalFlags::BAIL_TEST,                                  //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                       //
        1,                                                                              //
        FractalType::NO_FRACTAL, FractalType::BARNSLEY_M2, FractalType::BARNSLEY_J2_FP, //
        SymmetryType::ORIGIN,                                                           //
        barnsley2_fractal, long_julia_per_pixel, julia_long_setup, standard_fractal,    //
        STD_BAILOUT                                                                     //
    },

    {
        FractalType::SQR_TRIG,                                                      //
        T_SQR_FN + 1,                                                               //
        {"", "", "", ""},                                                           //
        {0, 0, 0, 0},                                                               //
        HelpLabels::HT_TAYLOR_SKINNER_VARIATIONS, HelpLabels::HF_SQR_FN,            //
        FractalFlags::TRIG1 | FractalFlags::BAIL_TEST,                              //
        -4.0F, 4.0F, -3.0F, 3.0F,                                                   //
        16,                                                                         //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::SQR_TRIG_FP, //
        SymmetryType::X_AXIS,                                                       //
        sqr_trig_fractal, long_julia_per_pixel, sqr_trig_setup, standard_fractal,   //
        TRIG_BAILOUT_L                                                              //
    },

    {
        FractalType::SQR_TRIG_FP,                                                        //
        T_SQR_FN,                                                                        //
        {"", "", "", ""},                                                                //
        {0, 0, 0, 0},                                                                    //
        HelpLabels::HT_TAYLOR_SKINNER_VARIATIONS, HelpLabels::HF_SQR_FN,                 //
        FractalFlags::TRIG1 | FractalFlags::BAIL_TEST,                                   //
        -4.0F, 4.0F, -3.0F, 3.0F,                                                        //
        0,                                                                               //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::SQR_TRIG,         //
        SymmetryType::X_AXIS,                                                            //
        sqr_trig_fp_fractal, other_julia_fp_per_pixel, sqr_trig_setup, standard_fractal, //
        TRIG_BAILOUT_L                                                                   //
    },

    {
        FractalType::TRIG_PLUS_TRIG,                                                      //
        T_FN_PLUS_FN + 1,                                                                 //
        {RE_COEF_TRIG1, IM_COEF_TRIG1, RE_COEF_TRIG2, IM_COEF_TRIG2},                     //
        {1, 0, 1, 0},                                                                     //
        HelpLabels::HT_TAYLOR_SKINNER_VARIATIONS, HelpLabels::HF_FN_PLUS_FN,              //
        FractalFlags::TRIG2 | FractalFlags::BAIL_TEST,                                    //
        -4.0F, 4.0F, -3.0F, 3.0F,                                                         //
        16,                                                                               //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::TRIG_PLUS_TRIG_FP, //
        SymmetryType::X_AXIS,                                                             //
        trig_plus_trig_fractal, long_julia_per_pixel, trig_plus_trig_long_setup,          //
        standard_fractal,                                                                 //
        TRIG_BAILOUT_L                                                                    //
    },

    {
        FractalType::MANDEL_LAMBDA,                                                  //
        T_MANDEL_LAMBDA + 1,                                                         //
        {REAL_Z0, IMAG_Z0, "", ""},                                                  //
        {0, 0, 0, 0},                                                                //
        HelpLabels::HT_MANDEL_LAMBDA, HelpLabels::HF_MANDEL_LAMBDA,                  //
        FractalFlags::BAIL_TEST,                                                     //
        -3.0F, 5.0F, -3.0F, 3.0F,                                                    //
        1,                                                                           //
        FractalType::LAMBDA, FractalType::NO_FRACTAL, FractalType::MANDEL_LAMBDA_FP, //
        SymmetryType::X_AXIS_NO_PARAM,                                               //
        lambda_fractal, mandel_per_pixel, mandel_long_setup, standard_fractal,       //
        STD_BAILOUT                                                                  //
    },

    {
        FractalType::MARKS_MANDEL,                                                       //
        T_MARKS_MANDEL + 1,                                                              //
        {REAL_Z0, IMAG_Z0, RE_EXPONENT, ""},                                             //
        {0, 0, 1, 0},                                                                    //
        HelpLabels::HT_PETERSON_VARIATIONS, HelpLabels::HF_MARKS_MANDEL,                 //
        FractalFlags::BAIL_TEST,                                                         //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                        //
        1,                                                                               //
        FractalType::MARKS_JULIA, FractalType::NO_FRACTAL, FractalType::MARKS_MANDEL_FP, //
        SymmetryType::NONE,                                                              //
        marks_lambda_fractal, marks_mandel_per_pixel, mandel_long_setup,                 //
        standard_fractal,                                                                //
        STD_BAILOUT                                                                      //
    },

    {
        FractalType::MARKS_JULIA,                                                        //
        T_MARKS_JULIA + 1,                                                               //
        {REAL_PARAM, IMAG_PARAM, RE_EXPONENT, ""},                                       //
        {0.1, 0.9, 1, 0},                                                                //
        HelpLabels::HT_PETERSON_VARIATIONS, HelpLabels::HF_MARKS_JULIA,                  //
        FractalFlags::BAIL_TEST,                                                         //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                        //
        1,                                                                               //
        FractalType::NO_FRACTAL, FractalType::MARKS_MANDEL, FractalType::MARKS_JULIA_FP, //
        SymmetryType::ORIGIN,                                                            //
        marks_lambda_fractal, julia_per_pixel, marks_julia_setup, standard_fractal,      //
        STD_BAILOUT                                                                      //
    },

    {
        FractalType::UNITY,                                                      //
        T_UNITY + 1,                                                             //
        {"", "", "", ""},                                                        //
        {0, 0, 0, 0},                                                            //
        HelpLabels::HT_UNITY, HelpLabels::HF_UNITY,                              //
        FractalFlags::NONE,                                                      //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                //
        1,                                                                       //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::UNITY_FP, //
        SymmetryType::XY_AXIS,                                                   //
        unity_fractal, long_julia_per_pixel, unity_setup, standard_fractal,      //
        NO_BAILOUT                                                               //
    },

    {
        FractalType::MANDEL4,                                                   //
        T_MANDEL4 + 1,                                                          //
        {REAL_Z0, IMAG_Z0, "", ""},                                             //
        {0, 0, 0, 0},                                                           //
        HelpLabels::HT_MANDEL_JULIA4, HelpLabels::HF_MANDEL4,                   //
        FractalFlags::BAIL_TEST,                                                //
        -2.0F, 2.0F, -1.5F, 1.5F,                                               //
        1,                                                                      //
        FractalType::JULIA4, FractalType::NO_FRACTAL, FractalType::MANDEL4_FP,  //
        SymmetryType::X_AXIS_NO_PARAM,                                          //
        mandel4_fractal, mandel_per_pixel, mandel_long_setup, standard_fractal, //
        STD_BAILOUT                                                             //
    },

    {
        FractalType::JULIA4,                                                   //
        T_JULIA4 + 1,                                                          //
        {REAL_PARAM, IMAG_PARAM, "", ""},                                      //
        {0.6, 0.55, 0, 0},                                                     //
        HelpLabels::HT_MANDEL_JULIA4, HelpLabels::HF_JULIA4,                   //
        FractalFlags::OK_JB | FractalFlags::BAIL_TEST,                         //
        -2.0F, 2.0F, -1.5F, 1.5F,                                              //
        1,                                                                     //
        FractalType::NO_FRACTAL, FractalType::MANDEL4, FractalType::JULIA4_FP, //
        SymmetryType::ORIGIN,                                                  //
        mandel4_fractal, julia_per_pixel, julia_long_setup, standard_fractal,  //
        STD_BAILOUT                                                            //
    },

    {
        FractalType::IFS,                                                                                   //
        "ifs",                                                                                              //
        {COLOR_METHOD, "", "", ""},                                                                         //
        {0, 0, 0, 0},                                                                                       //
        HelpLabels::HT_IFS, HelpLabels::SPECIAL_IFS,                                                        //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::NO_RESUME | FractalFlags::INF_CALC, //
        -8.0F, 8.0F, -1.0F, 11.0F,                                                                          //
        16,                                                                                                 //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,                          //
        SymmetryType::NONE,                                                                                 //
        nullptr, nullptr, standalone_setup, ifs,                                                            //
        NO_BAILOUT                                                                                          //
    },

    {
        FractalType::IFS_3D,                                                       //
        T_IFS3D,                                                                   //
        {COLOR_METHOD, "", "", ""},                                                //
        {0, 0, 0, 0},                                                              //
        HelpLabels::HT_IFS, HelpLabels::SPECIAL_IFS,                               //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::NO_RESUME | FractalFlags::PARAMS_3D |
            FractalFlags::INF_CALC,                                                //
        -11.0F, 11.0F, -11.0F, 11.0F,                                              //
        16,                                                                        //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, //
        SymmetryType::NONE,                                                        //
        nullptr, nullptr, standalone_setup, ifs,                                   //
        NO_BAILOUT                                                                 //
    },

    {
        FractalType::BARNSLEY_M3,                                                       //
        T_BARNSLEY_M3 + 1,                                                              //
        {REAL_Z0, IMAG_Z0, "", ""},                                                     //
        {0, 0, 0, 0},                                                                   //
        HelpLabels::HT_BARNSLEY, HelpLabels::HF_BARNSLEY_M3,                            //
        FractalFlags::BAIL_TEST,                                                        //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                       //
        1,                                                                              //
        FractalType::BARNSLEY_J3, FractalType::NO_FRACTAL, FractalType::BARNSLEY_M3_FP, //
        SymmetryType::X_AXIS_NO_PARAM,                                                  //
        barnsley3_fractal, long_mandel_per_pixel, mandel_long_setup,                    //
        standard_fractal,                                                               //
        STD_BAILOUT                                                                     //
    },

    {
        FractalType::BARNSLEY_J3,                                                       //
        T_BARNSLEY_J3 + 1,                                                              //
        {REAL_PARAM, IMAG_PARAM, "", ""},                                               //
        {0.1, 0.36, 0, 0},                                                              //
        HelpLabels::HT_BARNSLEY, HelpLabels::HF_BARNSLEY_J3,                            //
        FractalFlags::OK_JB | FractalFlags::BAIL_TEST,                                  //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                       //
        1,                                                                              //
        FractalType::NO_FRACTAL, FractalType::BARNSLEY_M3, FractalType::BARNSLEY_J3_FP, //
        SymmetryType::NONE,                                                             //
        barnsley3_fractal, long_julia_per_pixel, julia_long_setup, standard_fractal,    //
        STD_BAILOUT                                                                     //
    },

    {
        FractalType::TRIG_SQR,                                                      //
        T_FN_ZZ + 1,                                                                //
        {"", "", "", ""},                                                           //
        {0, 0, 0, 0},                                                               //
        HelpLabels::HT_TAYLOR_SKINNER_VARIATIONS, HelpLabels::HF_FN_Z_TIMES_Z,      //
        FractalFlags::TRIG1 | FractalFlags::BAIL_TEST,                              //
        -4.0F, 4.0F, -3.0F, 3.0F,                                                   //
        16,                                                                         //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::TRIG_SQR_FP, //
        SymmetryType::XY_AXIS,                                                      //
        trig_z_sqrd_fractal, julia_per_pixel, julia_long_setup, standard_fractal,   //
        STD_BAILOUT                                                                 //
    },

    {
        FractalType::TRIG_SQR_FP,                                                     //
        T_FN_ZZ,                                                                      //
        {"", "", "", ""},                                                             //
        {0, 0, 0, 0},                                                                 //
        HelpLabels::HT_TAYLOR_SKINNER_VARIATIONS, HelpLabels::HF_FN_Z_TIMES_Z,        //
        FractalFlags::TRIG1 | FractalFlags::BAIL_TEST,                                //
        -4.0F, 4.0F, -3.0F, 3.0F,                                                     //
        0,                                                                            //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::TRIG_SQR,      //
        SymmetryType::XY_AXIS,                                                        //
        trig_z_sqrd_fp_fractal, julia_fp_per_pixel, julia_fp_setup, standard_fractal, //
        STD_BAILOUT                                                                   //
    },

    {
        FractalType::BIFURCATION,                                                                        //
        T_BIFURCATION,                                                                                   //
        {FILT, SEED_POP, "", ""},                                                                        //
        {1000.0, 0.66, 0, 0},                                                                            //
        HelpLabels::HT_BIFURCATION, HelpLabels::HF_BIFURCATION,                                          //
        FractalFlags::TRIG1 | FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::NO_ROTATE, //
        1.9F, 3.0F, 0.0F, 1.34F,                                                                         //
        0,                                                                                               //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::BIFURCATION_L,                    //
        SymmetryType::NONE,                                                                              //
        bifurc_verhulst_trig, nullptr, standalone_setup, bifurcation,                                    //
        NO_BAILOUT                                                                                       //
    },

    {
        FractalType::TRIG_PLUS_TRIG_FP,                                                //
        T_FN_PLUS_FN,                                                                  //
        {RE_COEF_TRIG1, IM_COEF_TRIG1, RE_COEF_TRIG2, IM_COEF_TRIG2},                  //
        {1, 0, 1, 0},                                                                  //
        HelpLabels::HT_TAYLOR_SKINNER_VARIATIONS, HelpLabels::HF_FN_PLUS_FN,           //
        FractalFlags::TRIG2 | FractalFlags::BAIL_TEST,                                 //
        -4.0F, 4.0F, -3.0F, 3.0F,                                                      //
        0,                                                                             //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::TRIG_PLUS_TRIG, //
        SymmetryType::X_AXIS,                                                          //
        trig_plus_trig_fp_fractal, other_julia_fp_per_pixel, trig_plus_trig_fp_setup,  //
        standard_fractal,                                                              //
        TRIG_BAILOUT_L                                                                 //
    },

    {
        FractalType::TRIG_X_TRIG,                                                      //
        T_FN_FN + 1,                                                                   //
        {"", "", "", ""},                                                              //
        {0, 0, 0, 0},                                                                  //
        HelpLabels::HT_TAYLOR_SKINNER_VARIATIONS, HelpLabels::HF_FN_TIMES_FN,          //
        FractalFlags::TRIG2 | FractalFlags::BAIL_TEST,                                 //
        -4.0F, 4.0F, -3.0F, 3.0F,                                                      //
        16,                                                                            //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::TRIG_X_TRIG_FP, //
        SymmetryType::X_AXIS,                                                          //
        trig_x_trig_fractal, long_julia_per_pixel, fn_x_fn_setup, standard_fractal,    //
        TRIG_BAILOUT_L                                                                 //
    },

    {
        FractalType::TRIG_X_TRIG_FP,                                                       //
        T_FN_FN,                                                                           //
        {"", "", "", ""},                                                                  //
        {0, 0, 0, 0},                                                                      //
        HelpLabels::HT_TAYLOR_SKINNER_VARIATIONS, HelpLabels::HF_FN_TIMES_FN,              //
        FractalFlags::TRIG2 | FractalFlags::BAIL_TEST,                                     //
        -4.0F, 4.0F, -3.0F, 3.0F,                                                          //
        0,                                                                                 //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::TRIG_X_TRIG,        //
        SymmetryType::X_AXIS,                                                              //
        trig_x_trig_fp_fractal, other_julia_fp_per_pixel, fn_x_fn_setup, standard_fractal, //
        TRIG_BAILOUT_L                                                                     //
    },

    {
        FractalType::SQR_1_OVER_TRIG,                                                      //
        T_SQR_1_DIV_FN + 1,                                                                //
        {"", "", "", ""},                                                                  //
        {0, 0, 0, 0},                                                                      //
        HelpLabels::HT_TAYLOR_SKINNER_VARIATIONS, HelpLabels::HF_SQR_1_OVER_FN,            //
        FractalFlags::TRIG1 | FractalFlags::BAIL_TEST,                                     //
        -4.0F, 4.0F, -3.0F, 3.0F,                                                          //
        16,                                                                                //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::SQR_1_OVER_TRIG_FP, //
        SymmetryType::NONE,                                                                //
        sqr_1_over_trig_fractal, long_julia_per_pixel, sqr_trig_setup,                     //
        standard_fractal,                                                                  //
        TRIG_BAILOUT_L                                                                     //
    },

    {
        FractalType::SQR_1_OVER_TRIG_FP,                                                //
        T_SQR_1_DIV_FN,                                                                 //
        {"", "", "", ""},                                                               //
        {0, 0, 0, 0},                                                                   //
        HelpLabels::HT_TAYLOR_SKINNER_VARIATIONS, HelpLabels::HF_SQR_1_OVER_FN,         //
        FractalFlags::TRIG1 | FractalFlags::BAIL_TEST,                                  //
        -4.0F, 4.0F, -3.0F, 3.0F,                                                       //
        0,                                                                              //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::SQR_1_OVER_TRIG, //
        SymmetryType::NONE,                                                             //
        sqr_1_over_trig_fp_fractal, other_julia_fp_per_pixel, sqr_trig_setup,           //
        standard_fractal,                                                               //
        TRIG_BAILOUT_L                                                                  //
    },

    {
        FractalType::Z_X_TRIG_PLUS_Z,                                                                   //
        T_FN_Z_PLUS_Z + 1,                                                                              //
        {RE_COEF_TRIG1, IM_COEF_TRIG1, "Real Coefficient Second Term", "Imag Coefficient Second Term"}, //
        {1, 0, 1, 0},                                                                                   //
        HelpLabels::HT_TAYLOR_SKINNER_VARIATIONS, HelpLabels::HF_FN_X_Z_PLUS_Z,                         //
        FractalFlags::TRIG1 | FractalFlags::BAIL_TEST,                                                  //
        -4.0F, 4.0F, -3.0F, 3.0F,                                                                       //
        1,                                                                                              //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::Z_X_TRIG_PLUS_Z_FP,              //
        SymmetryType::X_AXIS,                                                                           //
        z_x_trig_plus_z_fractal, julia_per_pixel, z_x_trig_plus_z_setup, standard_fractal,              //
        TRIG_BAILOUT_L                                                                                  //
    },

    {
        FractalType::Z_X_TRIG_PLUS_Z_FP,                                                                //
        T_FN_Z_PLUS_Z,                                                                                  //
        {RE_COEF_TRIG1, IM_COEF_TRIG1, "Real Coefficient Second Term", "Imag Coefficient Second Term"}, //
        {1, 0, 1, 0},                                                                                   //
        HelpLabels::HT_TAYLOR_SKINNER_VARIATIONS, HelpLabels::HF_FN_X_Z_PLUS_Z,                         //
        FractalFlags::TRIG1 | FractalFlags::BAIL_TEST,                                                  //
        -4.0F, 4.0F, -3.0F, 3.0F,                                                                       //
        0,                                                                                              //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::Z_X_TRIG_PLUS_Z,                 //
        SymmetryType::X_AXIS,                                                                           //
        z_x_trig_plus_z_fp_fractal, julia_fp_per_pixel, z_x_trig_plus_z_setup,                          //
        standard_fractal,                                                                               //
        TRIG_BAILOUT_L                                                                                  //
    },

    {
        FractalType::KAM_FP,                                                     //
        T_KAM_TORUS,                                                             //
        {KAM_ANGLE, KAM_STEP, KAM_STOP, POINTS_PER_ORBIT},                       //
        {1.3, .05, 1.5, 150},                                                    //
        HelpLabels::HT_KAM, HelpLabels::HF_KAM,                                  //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE,                         //
        -1.0F, 1.0F, -.75F, .75F,                                                //
        0,                                                                       //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::KAM,      //
        SymmetryType::NONE,                                                      //
        (VF) kam_torus_float_orbit, nullptr, orbit3d_float_setup, orbit2d_float, //
        NO_BAILOUT                                                               //
    },

    {
        FractalType::KAM,                                                      //
        T_KAM_TORUS + 1,                                                       //
        {KAM_ANGLE, KAM_STEP, KAM_STOP, POINTS_PER_ORBIT},                     //
        {1.3, .05, 1.5, 150},                                                  //
        HelpLabels::HT_KAM, HelpLabels::HF_KAM,                                //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE,                       //
        -1.0F, 1.0F, -.75F, .75F,                                              //
        16,                                                                    //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::KAM_FP, //
        SymmetryType::NONE,                                                    //
        (VF) kam_torus_long_orbit, nullptr, orbit3d_long_setup, orbit2d_long,  //
        NO_BAILOUT                                                             //
    },

    {
        FractalType::KAM_3D_FP,                                                  //
        T_KAM_TORUS3D,                                                           //
        {KAM_ANGLE, KAM_STEP, KAM_STOP, POINTS_PER_ORBIT},                       //
        {1.3, .05, 1.5, 150},                                                    //
        HelpLabels::HT_KAM, HelpLabels::HF_KAM,                                  //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::NO_RESUME |
            FractalFlags::PARAMS_3D,                                             //
        -3.0F, 3.0F, -1.0F, 3.5F,                                                //
        0,                                                                       //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::KAM_3D,   //
        SymmetryType::NONE,                                                      //
        (VF) kam_torus_float_orbit, nullptr, orbit3d_float_setup, orbit3d_float, //
        NO_BAILOUT                                                               //
    },

    {
        FractalType::KAM_3D,                                                      //
        T_KAM_TORUS3D + 1,                                                        //
        {KAM_ANGLE, KAM_STEP, KAM_STOP, POINTS_PER_ORBIT},                        //
        {1.3, .05, 1.5, 150},                                                     //
        HelpLabels::HT_KAM, HelpLabels::HF_KAM,                                   //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::NO_RESUME |
            FractalFlags::PARAMS_3D,                                              //
        -3.0F, 3.0F, -1.0F, 3.5F,                                                 //
        16,                                                                       //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::KAM_3D_FP, //
        SymmetryType::NONE,                                                       //
        (VF) kam_torus_long_orbit, nullptr, orbit3d_long_setup, orbit3d_long,     //
        NO_BAILOUT                                                                //
    },

    {
        FractalType::LAMBDA_TRIG,                                                       //
        T_LAMBDA_FN + 1,                                                                //
        {REAL_PARAM, IMAG_PARAM, "", ""},                                               //
        {1.0, 0.4, 0, 0},                                                               //
        HelpLabels::HT_LAMBDA_FN, HelpLabels::HF_LAMBDA_FN,                             //
        FractalFlags::TRIG1 | FractalFlags::OK_JB,                                      //
        -4.0F, 4.0F, -3.0F, 3.0F,                                                       //
        16,                                                                             //
        FractalType::NO_FRACTAL, FractalType::MANDEL_TRIG, FractalType::LAMBDA_TRIG_FP, //
        SymmetryType::PI_SYM,                                                           //
        lambda_trig_fractal, long_julia_per_pixel, lambda_trig_setup,                   //
        standard_fractal,                                                               //
        TRIG_BAILOUT_L                                                                  //
    },

    {
        FractalType::MAN_TRIG_PLUS_Z_SQRD_L,                                                                //
        T_MAN_FN_PLUS_Z_SQRD + 1,                                                                           //
        {REAL_Z0, IMAG_Z0, "", ""},                                                                         //
        {0, 0, 0, 0},                                                                                       //
        HelpLabels::HT_PICKOVER_MANDEL_JULIA, HelpLabels::HF_MANDEL_FN_PLUS_Z_SQRD,                         //
        FractalFlags::TRIG1 | FractalFlags::BAIL_TEST,                                                      //
        -2.5F, 1.5F, -1.5F, 1.5F,                                                                           //
        16,                                                                                                 //
        FractalType::JUL_TRIG_PLUS_Z_SQRD_L, FractalType::NO_FRACTAL, FractalType::MAN_TRIG_PLUS_Z_SQRD_FP, //
        SymmetryType::X_AXIS_NO_PARAM,                                                                      //
        trig_plus_z_squared_fractal, mandel_per_pixel, mandel_long_setup,                                   //
        standard_fractal,                                                                                   //
        STD_BAILOUT                                                                                         //
    },

    {
        FractalType::JUL_TRIG_PLUS_Z_SQRD_L,                                                                //
        T_JUL_FN_PLUS_Z_SQRD + 1,                                                                           //
        {REAL_PARAM, IMAG_PARAM, "", ""},                                                                   //
        {-0.5, 0.5, 0, 0},                                                                                  //
        HelpLabels::HT_PICKOVER_MANDEL_JULIA, HelpLabels::HF_JULIA_FN_PLUS_Z_SQRD,                          //
        FractalFlags::TRIG1 | FractalFlags::OK_JB | FractalFlags::BAIL_TEST,                                //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                                           //
        16,                                                                                                 //
        FractalType::NO_FRACTAL, FractalType::MAN_TRIG_PLUS_Z_SQRD_L, FractalType::JUL_TRIG_PLUS_Z_SQRD_FP, //
        SymmetryType::NONE,                                                                                 //
        trig_plus_z_squared_fractal, julia_per_pixel, julia_fn_plus_z_sqrd_setup,                           //
        standard_fractal,                                                                                   //
        STD_BAILOUT                                                                                         //
    },

    {
        FractalType::MAN_TRIG_PLUS_Z_SQRD_FP,                                                               //
        T_MAN_FN_PLUS_Z_SQRD,                                                                               //
        {REAL_Z0, IMAG_Z0, "", ""},                                                                         //
        {0, 0, 0, 0},                                                                                       //
        HelpLabels::HT_PICKOVER_MANDEL_JULIA, HelpLabels::HF_MANDEL_FN_PLUS_Z_SQRD,                         //
        FractalFlags::TRIG1 | FractalFlags::BAIL_TEST,                                                      //
        -2.5F, 1.5F, -1.5F, 1.5F,                                                                           //
        0,                                                                                                  //
        FractalType::JUL_TRIG_PLUS_Z_SQRD_FP, FractalType::NO_FRACTAL, FractalType::MAN_TRIG_PLUS_Z_SQRD_L, //
        SymmetryType::X_AXIS_NO_PARAM,                                                                      //
        trig_plus_z_squared_fp_fractal, mandel_fp_per_pixel, mandel_fp_setup,                               //
        standard_fractal,                                                                                   //
        STD_BAILOUT                                                                                         //
    },

    {
        FractalType::JUL_TRIG_PLUS_Z_SQRD_FP,                                                               //
        T_JUL_FN_PLUS_Z_SQRD,                                                                               //
        {REAL_PARAM, IMAG_PARAM, "", ""},                                                                   //
        {-0.5, 0.5, 0, 0},                                                                                  //
        HelpLabels::HT_PICKOVER_MANDEL_JULIA, HelpLabels::HF_JULIA_FN_PLUS_Z_SQRD,                          //
        FractalFlags::TRIG1 | FractalFlags::OK_JB | FractalFlags::BAIL_TEST,                                //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                                           //
        0,                                                                                                  //
        FractalType::NO_FRACTAL, FractalType::MAN_TRIG_PLUS_Z_SQRD_FP, FractalType::JUL_TRIG_PLUS_Z_SQRD_L, //
        SymmetryType::NONE,                                                                                 //
        trig_plus_z_squared_fp_fractal, julia_fp_per_pixel, julia_fn_plus_z_sqrd_setup,                     //
        standard_fractal,                                                                                   //
        STD_BAILOUT                                                                                         //
    },

    {
        FractalType::LAMBDA_TRIG_FP,                                                    //
        T_LAMBDA_FN,                                                                    //
        {REAL_PARAM, IMAG_PARAM, "", ""},                                               //
        {1.0, 0.4, 0, 0},                                                               //
        HelpLabels::HT_LAMBDA_FN, HelpLabels::HF_LAMBDA_FN,                             //
        FractalFlags::TRIG1 | FractalFlags::OK_JB,                                      //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                       //
        0,                                                                              //
        FractalType::NO_FRACTAL, FractalType::MANDEL_TRIG_FP, FractalType::LAMBDA_TRIG, //
        SymmetryType::PI_SYM,                                                           //
        lambda_trig_fp_fractal, other_julia_fp_per_pixel, lambda_trig_setup,            //
        standard_fractal,                                                               //
        TRIG_BAILOUT_L                                                                  //
    },

    {
        FractalType::MANDEL_TRIG,                                                       //
        T_MANDEL_FN + 1,                                                                //
        {REAL_Z0, IMAG_Z0, "", ""},                                                     //
        {0, 0, 0, 0},                                                                   //
        HelpLabels::HT_MANDEL_FN, HelpLabels::HF_MANDEL_FN,                             //
        FractalFlags::TRIG1,                                                            //
        -8.0F, 8.0F, -6.0F, 6.0F,                                                       //
        16,                                                                             //
        FractalType::LAMBDA_TRIG, FractalType::NO_FRACTAL, FractalType::MANDEL_TRIG_FP, //
        SymmetryType::XY_AXIS_NO_PARAM,                                                 //
        lambda_trig_fractal, long_mandel_per_pixel, mandel_trig_setup,                  //
        standard_fractal,                                                               //
        TRIG_BAILOUT_L                                                                  //
    },

    {
        FractalType::MANDEL_Z_POWER_L,                                                         //
        T_MAN_Z_POWER + 1,                                                                     //
        {REAL_Z0, IMAG_Z0, RE_EXPONENT, IM_EXPONENT},                                          //
        {0, 0, 2, 0},                                                                          //
        HelpLabels::HT_PICKOVER_MANDEL_JULIA, HelpLabels::HF_MANDEL_Z_POWER,                   //
        FractalFlags::BAIL_TEST,                                                               //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                              //
        1,                                                                                     //
        FractalType::JULIA_Z_POWER_L, FractalType::NO_FRACTAL, FractalType::MANDEL_Z_POWER_FP, //
        SymmetryType::X_AXIS_NO_IMAG,                                                          //
        long_z_power_fractal, long_mandel_per_pixel, mandel_long_setup,                        //
        standard_fractal,                                                                      //
        STD_BAILOUT                                                                            //
    },

    {
        FractalType::JULIA_Z_POWER_L,                                                          //
        T_JUL_Z_POWER + 1,                                                                     //
        {REAL_PARAM, IMAG_PARAM, RE_EXPONENT, IM_EXPONENT},                                    //
        {0.3, 0.6, 2, 0},                                                                      //
        HelpLabels::HT_PICKOVER_MANDEL_JULIA, HelpLabels::HF_JULIA_Z_POWER,                    //
        FractalFlags::OK_JB | FractalFlags::BAIL_TEST,                                         //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                              //
        1,                                                                                     //
        FractalType::NO_FRACTAL, FractalType::MANDEL_Z_POWER_L, FractalType::JULIA_Z_POWER_FP, //
        SymmetryType::ORIGIN,                                                                  //
        long_z_power_fractal, long_julia_per_pixel, julia_long_setup,                          //
        standard_fractal,                                                                      //
        STD_BAILOUT                                                                            //
    },

    {
        FractalType::MANDEL_Z_POWER_FP,                                                        //
        T_MAN_Z_POWER,                                                                         //
        {REAL_Z0, IMAG_Z0, RE_EXPONENT, IM_EXPONENT},                                          //
        {0, 0, 2, 0},                                                                          //
        HelpLabels::HT_PICKOVER_MANDEL_JULIA, HelpLabels::HF_MANDEL_Z_POWER,                   //
        FractalFlags::BAIL_TEST | FractalFlags::BF_MATH | FractalFlags::PERTURB,               //
        -2.5F, 1.5F, -1.5F, 1.5F,                                                              //
        0,                                                                                     //
        FractalType::JULIA_Z_POWER_FP, FractalType::NO_FRACTAL, FractalType::MANDEL_Z_POWER_L, //
        SymmetryType::X_AXIS_NO_IMAG,                                                          //
        float_z_power_fractal, other_mandel_fp_per_pixel, mandel_fp_setup,                     //
        standard_fractal,                                                                      //
        STD_BAILOUT,                                                                           //
        mandel_z_power_ref_pt, mandel_z_power_ref_pt_bf, mandel_z_power_perturb                //
    },

    {
        FractalType::JULIA_Z_POWER_FP,                                                         //
        T_JUL_Z_POWER,                                                                         //
        {REAL_PARAM, IMAG_PARAM, RE_EXPONENT, IM_EXPONENT},                                    //
        {0.3, 0.6, 2, 0},                                                                      //
        HelpLabels::HT_PICKOVER_MANDEL_JULIA, HelpLabels::HF_JULIA_Z_POWER,                    //
        FractalFlags::OK_JB | FractalFlags::BAIL_TEST | FractalFlags::BF_MATH,                 //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                              //
        0,                                                                                     //
        FractalType::NO_FRACTAL, FractalType::MANDEL_Z_POWER_FP, FractalType::JULIA_Z_POWER_L, //
        SymmetryType::ORIGIN,                                                                  //
        float_z_power_fractal, other_julia_fp_per_pixel, julia_fp_setup,                       //
        standard_fractal,                                                                      //
        STD_BAILOUT                                                                            //
    },

    {
        FractalType::MAN_Z_TO_Z_PLUS_Z_PWR_FP,                                                   //
        "manzzpwr",                                                                              //
        {REAL_Z0, IMAG_Z0, RE_EXPONENT, ""},                                                     //
        {0, 0, 2, 0},                                                                            //
        HelpLabels::HT_PICKOVER_MANDEL_JULIA, HelpLabels::HF_MANDEL_Z_Z_POWER,                   //
        FractalFlags::BAIL_TEST | FractalFlags::BF_MATH,                                         //
        -2.5F, 1.5F, -1.5F, 1.5F,                                                                //
        0,                                                                                       //
        FractalType::JUL_Z_TO_Z_PLUS_Z_PWR_FP, FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, //
        SymmetryType::X_AXIS_NO_PARAM,                                                           //
        float_z_to_z_plus_z_pwr_fractal, other_mandel_fp_per_pixel, mandel_fp_setup,             //
        standard_fractal,                                                                        //
        STD_BAILOUT                                                                              //
    },

    {
        FractalType::JUL_Z_TO_Z_PLUS_Z_PWR_FP,                                                   //
        "julzzpwr",                                                                              //
        {REAL_PARAM, IMAG_PARAM, RE_EXPONENT, ""},                                               //
        {-0.3, 0.3, 2, 0},                                                                       //
        HelpLabels::HT_PICKOVER_MANDEL_JULIA, HelpLabels::HF_JULIA_Z_Z_PWR,                      //
        FractalFlags::OK_JB | FractalFlags::BAIL_TEST,                                           //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                                //
        0,                                                                                       //
        FractalType::NO_FRACTAL, FractalType::MAN_Z_TO_Z_PLUS_Z_PWR_FP, FractalType::NO_FRACTAL, //
        SymmetryType::NONE,                                                                      //
        float_z_to_z_plus_z_pwr_fractal, other_julia_fp_per_pixel, julia_fp_setup,               //
        standard_fractal,                                                                        //
        STD_BAILOUT                                                                              //
    },

    {
        FractalType::MAN_TRIG_PLUS_EXP_L,                                                             //
        T_MAN_FN_PLUS_EXP + 1,                                                                        //
        {REAL_Z0, IMAG_Z0, "", ""},                                                                   //
        {0, 0, 0, 0},                                                                                 //
        HelpLabels::HT_PICKOVER_MANDEL_JULIA, HelpLabels::HF_MANDEL_FN_PLUS_EXP,                      //
        FractalFlags::TRIG1 | FractalFlags::BAIL_TEST,                                                //
        -8.0F, 8.0F, -6.0F, 6.0F,                                                                     //
        16,                                                                                           //
        FractalType::JUL_TRIG_PLUS_EXP_L, FractalType::NO_FRACTAL, FractalType::MAN_TRIG_PLUS_EXP_FP, //
        SymmetryType::X_AXIS_NO_PARAM,                                                                //
        long_trig_plus_exponent_fractal, long_mandel_per_pixel, mandel_long_setup,                    //
        standard_fractal,                                                                             //
        STD_BAILOUT                                                                                   //
    },

    {
        FractalType::JUL_TRIG_PLUS_EXP_L,                                                             //
        T_JUL_FN_PLUS_EXP + 1,                                                                        //
        {REAL_PARAM, IMAG_PARAM, "", ""},                                                             //
        {0, 0, 0, 0},                                                                                 //
        HelpLabels::HT_PICKOVER_MANDEL_JULIA, HelpLabels::HF_JULIA_FN_PLUS_EXP,                       //
        FractalFlags::TRIG1 | FractalFlags::OK_JB | FractalFlags::BAIL_TEST,                          //
        -4.0F, 4.0F, -3.0F, 3.0F,                                                                     //
        16,                                                                                           //
        FractalType::NO_FRACTAL, FractalType::MAN_TRIG_PLUS_EXP_L, FractalType::JUL_TRIG_PLUS_EXP_FP, //
        SymmetryType::NONE,                                                                           //
        long_trig_plus_exponent_fractal, long_julia_per_pixel, julia_long_setup,                      //
        standard_fractal,                                                                             //
        STD_BAILOUT                                                                                   //
    },

    {
        FractalType::MAN_TRIG_PLUS_EXP_FP,                                                            //
        T_MAN_FN_PLUS_EXP,                                                                            //
        {REAL_Z0, IMAG_Z0, "", ""},                                                                   //
        {0, 0, 0, 0},                                                                                 //
        HelpLabels::HT_PICKOVER_MANDEL_JULIA, HelpLabels::HF_MANDEL_FN_PLUS_EXP,                      //
        FractalFlags::TRIG1 | FractalFlags::BAIL_TEST,                                                //
        -8.0F, 8.0F, -6.0F, 6.0F,                                                                     //
        0,                                                                                            //
        FractalType::JUL_TRIG_PLUS_EXP_FP, FractalType::NO_FRACTAL, FractalType::MAN_TRIG_PLUS_EXP_L, //
        SymmetryType::X_AXIS_NO_PARAM,                                                                //
        float_trig_plus_exponent_fractal, other_mandel_fp_per_pixel, mandel_fp_setup,                 //
        standard_fractal,                                                                             //
        STD_BAILOUT                                                                                   //
    },

    {
        FractalType::JUL_TRIG_PLUS_EXP_FP,                                                            //
        T_JUL_FN_PLUS_EXP,                                                                            //
        {REAL_PARAM, IMAG_PARAM, "", ""},                                                             //
        {0, 0, 0, 0},                                                                                 //
        HelpLabels::HT_PICKOVER_MANDEL_JULIA, HelpLabels::HF_JULIA_FN_PLUS_EXP,                       //
        FractalFlags::TRIG1 | FractalFlags::OK_JB | FractalFlags::BAIL_TEST,                          //
        -4.0F, 4.0F, -3.0F, 3.0F,                                                                     //
        0,                                                                                            //
        FractalType::NO_FRACTAL, FractalType::MAN_TRIG_PLUS_EXP_FP, FractalType::JUL_TRIG_PLUS_EXP_L, //
        SymmetryType::NONE,                                                                           //
        float_trig_plus_exponent_fractal, other_julia_fp_per_pixel, julia_fp_setup,                   //
        standard_fractal,                                                                             //
        STD_BAILOUT                                                                                   //
    },

    {
        FractalType::POPCORN_FP,                                                  //
        T_POPCORN,                                                                //
        {STEP_X, STEP_Y, CONSTANT_X, CONSTANT_Y},                                 //
        {0.05, 0, 3.00, 0},                                                       //
        HelpLabels::HT_POPCORN, HelpLabels::HF_POPCORN,                           //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::TRIG4,    //
        -3.0F, 3.0F, -2.25F, 2.25F,                                               //
        0,                                                                        //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::POPCORN_L, //
        SymmetryType::NO_PLOT,                                                    //
        popcorn_fractal_fn, other_julia_fp_per_pixel, julia_fp_setup, popcorn,    //
        STD_BAILOUT                                                               //
    },

    {
        FractalType::POPCORN_L,                                                    //
        T_POPCORN + 1,                                                             //
        {STEP_X, STEP_Y, CONSTANT_X, CONSTANT_Y},                                  //
        {0.05, 0, 3.00, 0},                                                        //
        HelpLabels::HT_POPCORN, HelpLabels::HF_POPCORN,                            //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::TRIG4,     //
        -3.0F, 3.0F, -2.25F, 2.25F,                                                //
        16,                                                                        //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::POPCORN_FP, //
        SymmetryType::NO_PLOT,                                                     //
        long_popcorn_fractal_fn, long_julia_per_pixel, julia_long_setup, popcorn,  //
        STD_BAILOUT                                                                //
    },

    {
        FractalType::LORENZ_FP,                                                   //
        T_LORENZ,                                                                 //
        {TIME_STEP, "a", "b", "c"},                                               //
        {.02, 5, 15, 1},                                                          //
        HelpLabels::HT_LORENZ, HelpLabels::HF_LORENZ,                             //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::INF_CALC, //
        -15.0F, 15.0F, 0.0F, 30.0F,                                               //
        0,                                                                        //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::LORENZ_L,  //
        SymmetryType::NONE,                                                       //
        (VF) lorenz3d_float_orbit, nullptr, orbit3d_float_setup, orbit2d_float,   //
        NO_BAILOUT                                                                //
    },

    {
        FractalType::LORENZ_L,                                                    //
        T_LORENZ + 1,                                                             //
        {TIME_STEP, "a", "b", "c"},                                               //
        {.02, 5, 15, 1},                                                          //
        HelpLabels::HT_LORENZ, HelpLabels::HF_LORENZ,                             //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::INF_CALC, //
        -15.0F, 15.0F, 0.0F, 30.0F,                                               //
        16,                                                                       //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::LORENZ_FP, //
        SymmetryType::NONE,                                                       //
        (VF) lorenz3d_long_orbit, nullptr, orbit3d_long_setup, orbit2d_long,      //
        NO_BAILOUT                                                                //
    },

    {
        FractalType::LORENZ_3D_L,                                                    //
        T_LORENZ3D + 1,                                                              //
        {TIME_STEP, "a", "b", "c"},                                                  //
        {.02, 5, 15, 1},                                                             //
        HelpLabels::HT_LORENZ, HelpLabels::HF_LORENZ,                                //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::NO_RESUME | FractalFlags::PARAMS_3D |
            FractalFlags::INF_CALC,                                                  //
        -30.0F, 30.0F, -30.0F, 30.0F,                                                //
        16,                                                                          //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::LORENZ_3D_FP, //
        SymmetryType::NONE,                                                          //
        (VF) lorenz3d_long_orbit, nullptr, orbit3d_long_setup, orbit3d_long,         //
        NO_BAILOUT                                                                   //
    },

    {
        FractalType::NEWTON_MP,                                                  //
        T_NEWTON + 1,                                                            //
        {NEWT_DEGREE, "", "", ""},                                               //
        {3, 0, 0, 0},                                                            //
        HelpLabels::HT_NEWTON, HelpLabels::HF_NEWTON,                            //
        FractalFlags::NONE,                                                      //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                //
        0,                                                                       //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::NEWTON,   //
        SymmetryType::X_AXIS,                                                    //
        mpc_newton_fractal, mpc_julia_per_pixel, newton_setup, standard_fractal, //
        NO_BAILOUT                                                               //
    },

    {
        FractalType::NEWT_BASIN_MP,                                                //
        T_NEWT_BASIN + 1,                                                          //
        {NEWT_DEGREE, "Enter non-zero value for stripes", "", ""},                 //
        {3, 0, 0, 0},                                                              //
        HelpLabels::HT_NEWTON_BASINS, HelpLabels::HF_NEWTON_BASIN,                 //
        FractalFlags::NONE,                                                        //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                  //
        0,                                                                         //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::NEWT_BASIN, //
        SymmetryType::NONE,                                                        //
        mpc_newton_fractal, mpc_julia_per_pixel, newton_setup, standard_fractal,   //
        NO_BAILOUT                                                                 //
    },

    {
        FractalType::COMPLEX_NEWTON,                                                              //
        "complexnewton",                                                                          //
        {"Real part of Degree", "Imag part of Degree", "Real part of Root", "Imag part of Root"}, //
        {3, 0, 1, 0},                                                                             //
        HelpLabels::HT_NEWTON_COMPLEX, HelpLabels::HF_COMPLEX_NEWTON,                             //
        FractalFlags::NONE,                                                                       //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                                 //
        0,                                                                                        //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,                //
        SymmetryType::NONE,                                                                       //
        complex_newton, other_julia_fp_per_pixel, complex_newton_setup,                           //
        standard_fractal,                                                                         //
        NO_BAILOUT                                                                                //
    },

    {
        FractalType::COMPLEX_BASIN,                                                               //
        "complexbasin",                                                                           //
        {"Real part of Degree", "Imag part of Degree", "Real part of Root", "Imag part of Root"}, //
        {3, 0, 1, 0},                                                                             //
        HelpLabels::HT_NEWTON_COMPLEX, HelpLabels::HF_COMPLEX_NEWTON,                             //
        FractalFlags::NONE,                                                                       //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                                 //
        0,                                                                                        //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,                //
        SymmetryType::NONE,                                                                       //
        complex_basin, other_julia_fp_per_pixel, complex_newton_setup,                            //
        standard_fractal,                                                                         //
        NO_BAILOUT                                                                                //
    },

    {
        FractalType::COMPLEX_MARKS_MAND,                                                  //
        "cmplxmarksmand",                                                                 //
        {REAL_Z0, IMAG_Z0, RE_EXPONENT, IM_EXPONENT},                                     //
        {0, 0, 1, 0},                                                                     //
        HelpLabels::HT_PETERSON_VARIATIONS, HelpLabels::HF_COMPLEX_MARKS_MAND,            //
        FractalFlags::BAIL_TEST,                                                          //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                         //
        0,                                                                                //
        FractalType::COMPLEX_MARKS_JUL, FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, //
        SymmetryType::NONE,                                                               //
        marks_cplx_mand, marks_cplx_mand_per_pixel, mandel_fp_setup, standard_fractal,    //
        STD_BAILOUT                                                                       //
    },

    {
        FractalType::COMPLEX_MARKS_JUL,                                                    //
        "cmplxmarksjul",                                                                   //
        {REAL_PARAM, IMAG_PARAM, RE_EXPONENT, IM_EXPONENT},                                //
        {0.3, 0.6, 1, 0},                                                                  //
        HelpLabels::HT_PETERSON_VARIATIONS, HelpLabels::HF_COMPLEX_MARKS_JUL,              //
        FractalFlags::BAIL_TEST,                                                           //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                          //
        0,                                                                                 //
        FractalType::NO_FRACTAL, FractalType::COMPLEX_MARKS_MAND, FractalType::NO_FRACTAL, //
        SymmetryType::NONE,                                                                //
        marks_cplx_mand, julia_fp_per_pixel, julia_fp_setup, standard_fractal,             //
        STD_BAILOUT                                                                        //
    },

    {
        FractalType::FORMULA,                                                      //
        T_FORMULA + 1,                                                             //
        {P1_REAL, P1_IMAG, P2_REAL, P2_IMAG},                                      //
        {0, 0, 0, 0},                                                              //
        HelpLabels::HT_FORMULA, HelpLabels::SPECIAL_FORMULA,                       //
        FractalFlags::MORE,                                                        //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                  //
        1,                                                                         //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::FORMULA_FP, //
        SymmetryType::SETUP,                                                       //
        formula, form_per_pixel, formula_setup_l, standard_fractal,                //
        0                                                                          //
    },

    {
        FractalType::FORMULA_FP,                                                //
        T_FORMULA,                                                              //
        {P1_REAL, P1_IMAG, P2_REAL, P2_IMAG},                                   //
        {0, 0, 0, 0},                                                           //
        HelpLabels::HT_FORMULA, HelpLabels::SPECIAL_FORMULA,                    //
        FractalFlags::MORE,                                                     //
        -2.0F, 2.0F, -1.5F, 1.5F,                                               //
        0,                                                                      //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::FORMULA, //
        SymmetryType::SETUP,                                                    //
        formula, form_per_pixel, formula_setup_fp, standard_fractal,            //
        0                                                                       //
    },

    {
        FractalType::SIERPINSKI_FP,                                                //
        T_SIERPINSKI,                                                              //
        {"", "", "", ""},                                                          //
        {0, 0, 0, 0},                                                              //
        HelpLabels::HT_SIERPINSKI, HelpLabels::HF_SIERPINSKI,                      //
        FractalFlags::NONE,                                                        //
        -4.0F / 3.0F, 96.0F / 45.0F, -0.9F, 1.7F,                                  //
        0,                                                                         //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::SIERPINSKI, //
        SymmetryType::NONE,                                                        //
        sierpinski_fp_fractal, other_julia_fp_per_pixel, sierpinski_fp_setup,      //
        standard_fractal,                                                          //
        127                                                                        //
    },

    {
        FractalType::LAMBDA_FP,                                                      //
        T_LAMBDA,                                                                    //
        {REAL_PARAM, IMAG_PARAM, "", ""},                                            //
        {0.85, 0.6, 0, 0},                                                           //
        HelpLabels::HT_LAMBDA, HelpLabels::HF_LAMBDA,                                //
        FractalFlags::OK_JB | FractalFlags::BAIL_TEST,                               //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                    //
        0,                                                                           //
        FractalType::NO_FRACTAL, FractalType::MANDEL_LAMBDA_FP, FractalType::LAMBDA, //
        SymmetryType::NONE,                                                          //
        lambda_fp_fractal, julia_fp_per_pixel, julia_fp_setup, standard_fractal,     //
        STD_BAILOUT                                                                  //
    },

    {
        FractalType::BARNSLEY_M1_FP,                                                    //
        T_BARNSLEY_M1,                                                                  //
        {REAL_Z0, IMAG_Z0, "", ""},                                                     //
        {0, 0, 0, 0},                                                                   //
        HelpLabels::HT_BARNSLEY, HelpLabels::HF_BARNSLEY_M1,                            //
        FractalFlags::BAIL_TEST,                                                        //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                       //
        0,                                                                              //
        FractalType::BARNSLEY_J1_FP, FractalType::NO_FRACTAL, FractalType::BARNSLEY_M1, //
        SymmetryType::XY_AXIS_NO_PARAM,                                                 //
        barnsley1_fp_fractal, other_mandel_fp_per_pixel, mandel_fp_setup,               //
        standard_fractal,                                                               //
        STD_BAILOUT                                                                     //
    },

    {
        FractalType::BARNSLEY_J1_FP,                                                    //
        T_BARNSLEY_J1,                                                                  //
        {REAL_PARAM, IMAG_PARAM, "", ""},                                               //
        {0.6, 1.1, 0, 0},                                                               //
        HelpLabels::HT_BARNSLEY, HelpLabels::HF_BARNSLEY_J1,                            //
        FractalFlags::OK_JB | FractalFlags::BAIL_TEST,                                  //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                       //
        0,                                                                              //
        FractalType::NO_FRACTAL, FractalType::BARNSLEY_M1_FP, FractalType::BARNSLEY_J1, //
        SymmetryType::ORIGIN,                                                           //
        barnsley1_fp_fractal, other_julia_fp_per_pixel, julia_fp_setup,                 //
        standard_fractal,                                                               //
        STD_BAILOUT                                                                     //
    },

    {
        FractalType::BARNSLEY_M2_FP,                                                    //
        T_BARNSLEY_M2,                                                                  //
        {REAL_Z0, IMAG_Z0, "", ""},                                                     //
        {0, 0, 0, 0},                                                                   //
        HelpLabels::HT_BARNSLEY, HelpLabels::HF_BARNSLEY_M2,                            //
        FractalFlags::BAIL_TEST,                                                        //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                       //
        0,                                                                              //
        FractalType::BARNSLEY_J2_FP, FractalType::NO_FRACTAL, FractalType::BARNSLEY_M2, //
        SymmetryType::Y_AXIS_NO_PARAM,                                                  //
        barnsley2_fp_fractal, other_mandel_fp_per_pixel, mandel_fp_setup,               //
        standard_fractal,                                                               //
        STD_BAILOUT                                                                     //
    },

    {
        FractalType::BARNSLEY_J2_FP,                                                    //
        T_BARNSLEY_J2,                                                                  //
        {REAL_PARAM, IMAG_PARAM, "", ""},                                               //
        {0.6, 1.1, 0, 0},                                                               //
        HelpLabels::HT_BARNSLEY, HelpLabels::HF_BARNSLEY_J2,                            //
        FractalFlags::OK_JB | FractalFlags::BAIL_TEST,                                  //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                       //
        0,                                                                              //
        FractalType::NO_FRACTAL, FractalType::BARNSLEY_M2_FP, FractalType::BARNSLEY_J2, //
        SymmetryType::ORIGIN,                                                           //
        barnsley2_fp_fractal, other_julia_fp_per_pixel, julia_fp_setup,                 //
        standard_fractal,                                                               //
        STD_BAILOUT                                                                     //
    },

    {
        FractalType::BARNSLEY_M3_FP,                                                    //
        T_BARNSLEY_M3,                                                                  //
        {REAL_Z0, IMAG_Z0, "", ""},                                                     //
        {0, 0, 0, 0},                                                                   //
        HelpLabels::HT_BARNSLEY, HelpLabels::HF_BARNSLEY_M3,                            //
        FractalFlags::BAIL_TEST,                                                        //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                       //
        0,                                                                              //
        FractalType::BARNSLEY_J3_FP, FractalType::NO_FRACTAL, FractalType::BARNSLEY_M3, //
        SymmetryType::X_AXIS_NO_PARAM,                                                  //
        barnsley3_fp_fractal, other_mandel_fp_per_pixel, mandel_fp_setup,               //
        standard_fractal,                                                               //
        STD_BAILOUT                                                                     //
    },

    {
        FractalType::BARNSLEY_J3_FP,                                                    //
        T_BARNSLEY_J3,                                                                  //
        {REAL_PARAM, IMAG_PARAM, "", ""},                                               //
        {0.6, 1.1, 0, 0},                                                               //
        HelpLabels::HT_BARNSLEY, HelpLabels::HF_BARNSLEY_J3,                            //
        FractalFlags::OK_JB | FractalFlags::BAIL_TEST,                                  //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                       //
        0,                                                                              //
        FractalType::NO_FRACTAL, FractalType::BARNSLEY_M3_FP, FractalType::BARNSLEY_J3, //
        SymmetryType::NONE,                                                             //
        barnsley3_fp_fractal, other_julia_fp_per_pixel, julia_fp_setup,                 //
        standard_fractal,                                                               //
        STD_BAILOUT                                                                     //
    },

    {
        FractalType::MANDEL_LAMBDA_FP,                                               //
        T_MANDEL_LAMBDA,                                                             //
        {REAL_Z0, IMAG_Z0, "", ""},                                                  //
        {0, 0, 0, 0},                                                                //
        HelpLabels::HT_MANDEL_LAMBDA, HelpLabels::HF_MANDEL_LAMBDA,                  //
        FractalFlags::BAIL_TEST,                                                     //
        -3.0F, 5.0F, -3.0F, 3.0F,                                                    //
        0,                                                                           //
        FractalType::LAMBDA_FP, FractalType::NO_FRACTAL, FractalType::MANDEL_LAMBDA, //
        SymmetryType::X_AXIS_NO_PARAM,                                               //
        lambda_fp_fractal, mandel_fp_per_pixel, mandel_fp_setup, standard_fractal,   //
        STD_BAILOUT                                                                  //
    },

    {
        FractalType::JULIBROT,                                                      //
        T_JULIBROT + 1,                                                             //
        {"", "", "", ""},                                                           //
        {0, 0, 0, 0},                                                               //
        HelpLabels::HT_JULIBROT, HelpLabels::NONE,                                  //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::NO_ROTATE |
            FractalFlags::NO_RESUME,                                                //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                   //
        1,                                                                          //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::JULIBROT_FP, //
        SymmetryType::NONE,                                                         //
        julia_fractal, jb_per_pixel, julibrot_setup, std_4d_fractal,                //
        STD_BAILOUT                                                                 //
    },

    {
        FractalType::LORENZ_3D_FP,                                                  //
        T_LORENZ3D,                                                                 //
        {TIME_STEP, "a", "b", "c"},                                                 //
        {.02, 5, 15, 1},                                                            //
        HelpLabels::HT_LORENZ, HelpLabels::HF_LORENZ,                               //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::NO_RESUME | FractalFlags::PARAMS_3D |
            FractalFlags::INF_CALC,                                                 //
        -30.0F, 30.0F, -30.0F, 30.0F,                                               //
        0,                                                                          //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::LORENZ_3D_L, //
        SymmetryType::NONE,                                                         //
        (VF) lorenz3d_float_orbit, nullptr, orbit3d_float_setup, orbit3d_float,     //
        NO_BAILOUT                                                                  //
    },

    {
        FractalType::ROSSLER_L,                                                    //
        T_ROSSLER3D + 1,                                                           //
        {TIME_STEP, "a", "b", "c"},                                                //
        {.04, .2, .2, 5.7},                                                        //
        HelpLabels::HT_ROSSLER, HelpLabels::HF_ROSSLER,                            //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::NO_RESUME | FractalFlags::PARAMS_3D |
            FractalFlags::INF_CALC,                                                //
        -30.0F, 30.0F, -20.0F, 40.0F,                                              //
        16,                                                                        //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::ROSSLER_FP, //
        SymmetryType::NONE,                                                        //
        (VF) rossler_long_orbit, nullptr, orbit3d_long_setup, orbit3d_long,        //
        NO_BAILOUT                                                                 //
    },

    {
        FractalType::ROSSLER_FP,                                                  //
        T_ROSSLER3D,                                                              //
        {TIME_STEP, "a", "b", "c"},                                               //
        {.04, .2, .2, 5.7},                                                       //
        HelpLabels::HT_ROSSLER, HelpLabels::HF_ROSSLER,                           //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::NO_RESUME | FractalFlags::PARAMS_3D |
            FractalFlags::INF_CALC,                                               //
        -30.0F, 30.0F, -20.0F, 40.0F,                                             //
        0,                                                                        //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::ROSSLER_L, //
        SymmetryType::NONE,                                                       //
        (VF) rossler_float_orbit, nullptr, orbit3d_float_setup, orbit3d_float,    //
        NO_BAILOUT                                                                //
    },

    {
        FractalType::HENON_L,                                                     //
        T_HENON + 1,                                                              //
        {"a", "b", "", ""},                                                       //
        {1.4, .3, 0, 0},                                                          //
        HelpLabels::HT_HENON, HelpLabels::HF_HENON,                               //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::INF_CALC, //
        -1.4F, 1.4F, -.5F, .5F,                                                   //
        16,                                                                       //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::HENON_FP,  //
        SymmetryType::NONE,                                                       //
        (VF) henon_long_orbit, nullptr, orbit3d_long_setup, orbit2d_long,         //
        NO_BAILOUT                                                                //
    },

    {
        FractalType::HENON_FP,                                                    //
        T_HENON,                                                                  //
        {"a", "b", "", ""},                                                       //
        {1.4, .3, 0, 0},                                                          //
        HelpLabels::HT_HENON, HelpLabels::HF_HENON,                               //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::INF_CALC, //
        -1.4F, 1.4F, -.5F, .5F,                                                   //
        0,                                                                        //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::HENON_L,   //
        SymmetryType::NONE,                                                       //
        (VF) henon_float_orbit, nullptr, orbit3d_float_setup, orbit2d_float,      //
        NO_BAILOUT                                                                //
    },

    {
        FractalType::PICKOVER_FP,                                                  //
        "pickover",                                                                //
        {"a", "b", "c", "d"},                                                      //
        {2.24, .43, -.65, -2.43},                                                  //
        HelpLabels::HT_PICKOVER, HelpLabels::HF_PICKOVER,                          //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::NO_RESUME |
            FractalFlags::PARAMS_3D,                                               //
        -8.0F / 3.0F, 8.0F / 3.0F, -2.0F, 2.0F,                                    //
        0,                                                                         //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, //
        SymmetryType::NONE,                                                        //
        (VF) pickover_float_orbit, nullptr, orbit3d_float_setup, orbit3d_float,    //
        NO_BAILOUT                                                                 //
    },

    {
        FractalType::GINGERBREAD_FP,                                                //
        "gingerbreadman",                                                           //
        {"Initial x", "Initial y", "", ""},                                         //
        {-.1, 0, 0, 0},                                                             //
        HelpLabels::HT_GINGER, HelpLabels::HF_GINGERBREAD,                          //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::INF_CALC,   //
        -4.5F, 8.5F, -4.5F, 8.5F,                                                   //
        0,                                                                          //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,  //
        SymmetryType::NONE,                                                         //
        (VF) ginger_bread_float_orbit, nullptr, orbit3d_float_setup, orbit2d_float, //
        NO_BAILOUT                                                                  //
    },

    {
        FractalType::DIFFUSION,                                                    //
        "diffusion",                                                               //
        {
            "+Border size",                                                        //
            "+Type (0=Central,1=Falling,2=Square Cavity)",                         //
            "+Color change rate (0=Random)",                                       //
            ""                                                                     //
        },
        {10, 0, 0, 0},                                                             //
        HelpLabels::HT_DIFFUSION, HelpLabels::HF_DIFFUSION,                        //
        FractalFlags::NO_ZOOM | FractalFlags::NO_GUESS | FractalFlags::NO_TRACE,   //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                  //
        0,                                                                         //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, //
        SymmetryType::NONE,                                                        //
        nullptr, nullptr, standalone_setup, diffusion,                             //
        NO_BAILOUT                                                                 //
    },

    {
        FractalType::UNITY_FP,                                                        //
        T_UNITY,                                                                      //
        {"", "", "", ""},                                                             //
        {0, 0, 0, 0},                                                                 //
        HelpLabels::HT_UNITY, HelpLabels::HF_UNITY,                                   //
        FractalFlags::NONE,                                                           //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                     //
        0,                                                                            //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::UNITY,         //
        SymmetryType::XY_AXIS,                                                        //
        unity_fp_fractal, other_julia_fp_per_pixel, standard_setup, standard_fractal, //
        NO_BAILOUT                                                                    //
    },

    {
        FractalType::SPIDER_FP,                                                    //
        T_SPIDER,                                                                  //
        {REAL_Z0, IMAG_Z0, "", ""},                                                //
        {0, 0, 0, 0},                                                              //
        HelpLabels::HT_TAYLOR_SKINNER_VARIATIONS, HelpLabels::HF_SPIDER,           //
        FractalFlags::BAIL_TEST,                                                   //
        -2.5F, 1.5F, -1.5F, 1.5F,                                                  //
        0,                                                                         //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::SPIDER,     //
        SymmetryType::X_AXIS_NO_PARAM,                                             //
        spider_fp_fractal, mandel_fp_per_pixel, mandel_fp_setup, standard_fractal, //
        STD_BAILOUT                                                                //
    },

    {
        FractalType::SPIDER,                                                      //
        T_SPIDER + 1,                                                             //
        {REAL_Z0, IMAG_Z0, "", ""},                                               //
        {0, 0, 0, 0},                                                             //
        HelpLabels::HT_TAYLOR_SKINNER_VARIATIONS, HelpLabels::HF_SPIDER,          //
        FractalFlags::BAIL_TEST,                                                  //
        -2.5F, 1.5F, -1.5F, 1.5F,                                                 //
        1,                                                                        //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::SPIDER_FP, //
        SymmetryType::X_AXIS_NO_PARAM,                                            //
        spider_fractal, mandel_per_pixel, mandel_long_setup, standard_fractal,    //
        STD_BAILOUT                                                               //
    },

    {
        FractalType::TETRATE_FP,                                                   //
        "tetrate",                                                                 //
        {REAL_Z0, IMAG_Z0, "", ""},                                                //
        {0, 0, 0, 0},                                                              //
        HelpLabels::HT_TAYLOR_SKINNER_VARIATIONS, HelpLabels::HF_TETRATE,          //
        FractalFlags::BAIL_TEST,                                                   //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                  //
        0,                                                                         //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, //
        SymmetryType::X_AXIS_NO_IMAG,                                              //
        tetrate_fp_fractal, other_mandel_fp_per_pixel, mandel_fp_setup,            //
        standard_fractal,                                                          //
        STD_BAILOUT                                                                //
    },

    {
        FractalType::MAGNET_1M,                                                   //
        "magnet1m",                                                               //
        {REAL_Z0, IMAG_Z0, "", ""},                                               //
        {0, 0, 0, 0},                                                             //
        HelpLabels::HT_MAGNET, HelpLabels::HF_MAGNET_M1,                          //
        FractalFlags::NONE,                                                       //
        -4.0F, 4.0F, -3.0F, 3.0F,                                                 //
        0,                                                                        //
        FractalType::MAGNET_1J, FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, //
        SymmetryType::X_AXIS_NO_PARAM,                                            //
        magnet1_fractal, mandel_fp_per_pixel, mandel_fp_setup, standard_fractal,  //
        100                                                                       //
    },

    {
        FractalType::MAGNET_1J,                                                   //
        "magnet1j",                                                               //
        {REAL_PARAM, IMAG_PARAM, "", ""},                                         //
        {0, 0, 0, 0},                                                             //
        HelpLabels::HT_MAGNET, HelpLabels::HF_MAGNET_J1,                          //
        FractalFlags::NONE,                                                       //
        -8.0F, 8.0F, -6.0F, 6.0F,                                                 //
        0,                                                                        //
        FractalType::NO_FRACTAL, FractalType::MAGNET_1M, FractalType::NO_FRACTAL, //
        SymmetryType::X_AXIS_NO_IMAG,                                             //
        magnet1_fractal, julia_fp_per_pixel, julia_fp_setup, standard_fractal,    //
        100                                                                       //
    },

    {
        FractalType::MAGNET_2M,                                                   //
        "magnet2m",                                                               //
        {REAL_Z0, IMAG_Z0, "", ""},                                               //
        {0, 0, 0, 0},                                                             //
        HelpLabels::HT_MAGNET, HelpLabels::HF_MAGNET_M2,                          //
        FractalFlags::NONE,                                                       //
        -1.5F, 3.7F, -1.95F, 1.95F,                                               //
        0,                                                                        //
        FractalType::MAGNET_2J, FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, //
        SymmetryType::X_AXIS_NO_PARAM,                                            //
        magnet2_fractal, mandel_fp_per_pixel, mandel_fp_setup, standard_fractal,  //
        100                                                                       //
    },

    {
        FractalType::MAGNET_2J,                                                   //
        "magnet2j",                                                               //
        {REAL_PARAM, IMAG_PARAM, "", ""},                                         //
        {0, 0, 0, 0},                                                             //
        HelpLabels::HT_MAGNET, HelpLabels::HF_MAGNET_J2,                          //
        FractalFlags::NONE,                                                       //
        -8.0F, 8.0F, -6.0F, 6.0F,                                                 //
        0,                                                                        //
        FractalType::NO_FRACTAL, FractalType::MAGNET_2M, FractalType::NO_FRACTAL, //
        SymmetryType::X_AXIS_NO_IMAG,                                             //
        magnet2_fractal, julia_fp_per_pixel, julia_fp_setup, standard_fractal,    //
        100                                                                       //
    },

    {
        FractalType::BIFURCATION_L,                                                                      //
        T_BIFURCATION + 1,                                                                               //
        {FILT, SEED_POP, "", ""},                                                                        //
        {1000.0, 0.66, 0, 0},                                                                            //
        HelpLabels::HT_BIFURCATION, HelpLabels::HF_BIFURCATION,                                          //
        FractalFlags::TRIG1 | FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::NO_ROTATE, //
        1.9F, 3.0F, 0.0F, 1.34F,                                                                         //
        1,                                                                                               //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::BIFURCATION,                      //
        SymmetryType::NONE,                                                                              //
        long_bifurc_verhulst_trig, nullptr, standalone_setup, bifurcation,                               //
        NO_BAILOUT                                                                                       //
    },

    {
        FractalType::BIF_LAMBDA_L,                                                                       //
        T_BIF_LAMBDA + 1,                                                                                //
        {FILT, SEED_POP, "", ""},                                                                        //
        {1000.0, 0.66, 0, 0},                                                                            //
        HelpLabels::HT_BIFURCATION, HelpLabels::HF_BIF_LAMBDA,                                           //
        FractalFlags::TRIG1 | FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::NO_ROTATE, //
        -2.0F, 4.0F, -1.0F, 2.0F,                                                                        //
        1,                                                                                               //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::BIF_LAMBDA,                       //
        SymmetryType::NONE,                                                                              //
        long_bifurc_lambda_trig, nullptr, standalone_setup, bifurcation,                                 //
        NO_BAILOUT                                                                                       //
    },

    {
        FractalType::BIF_LAMBDA,                                                                         //
        T_BIF_LAMBDA,                                                                                    //
        {FILT, SEED_POP, "", ""},                                                                        //
        {1000.0, 0.66, 0, 0},                                                                            //
        HelpLabels::HT_BIFURCATION, HelpLabels::HF_BIF_LAMBDA,                                           //
        FractalFlags::TRIG1 | FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::NO_ROTATE, //
        -2.0F, 4.0F, -1.0F, 2.0F,                                                                        //
        0,                                                                                               //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::BIF_LAMBDA_L,                     //
        SymmetryType::NONE,                                                                              //
        bifurc_lambda_trig, nullptr, standalone_setup, bifurcation,                                      //
        NO_BAILOUT                                                                                       //
    },

    {
        FractalType::BIF_PLUS_SIN_PI,                                                                    //
        T_BIF_PLUS_SIN_PI,                                                                               //
        {FILT, SEED_POP, "", ""},                                                                        //
        {1000.0, 0.66, 0, 0},                                                                            //
        HelpLabels::HT_BIFURCATION, HelpLabels::HF_BIF_PLUS_SIN_PI,                                      //
        FractalFlags::TRIG1 | FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::NO_ROTATE, //
        0.275F, 1.45F, 0.0F, 2.0F,                                                                       //
        0,                                                                                               //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::BIF_PLUS_SIN_PI_L,                //
        SymmetryType::NONE,                                                                              //
        bifurc_add_trig_pi, nullptr, standalone_setup, bifurcation,                                      //
        NO_BAILOUT                                                                                       //
    },

    {
        FractalType::BIF_EQ_SIN_PI,                                                                      //
        T_BIF_EQ_SIN_PI,                                                                                 //
        {FILT, SEED_POP, "", ""},                                                                        //
        {1000.0, 0.66, 0, 0},                                                                            //
        HelpLabels::HT_BIFURCATION, HelpLabels::HF_BIF_EQ_SIN_PI,                                        //
        FractalFlags::TRIG1 | FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::NO_ROTATE, //
        -2.5F, 2.5F, -3.5F, 3.5F,                                                                        //
        0,                                                                                               //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::BIF_EQ_SIN_PI_L,                  //
        SymmetryType::NONE,                                                                              //
        bifurc_set_trig_pi, nullptr, standalone_setup, bifurcation,                                      //
        NO_BAILOUT                                                                                       //
    },

    {
        FractalType::POPCORN_JUL_FP,                                                    //
        T_POPCORN_JUL,                                                                  //
        {STEP_X, STEP_Y, CONSTANT_X, CONSTANT_Y},                                       //
        {0.05, 0, 3.00, 0},                                                             //
        HelpLabels::HT_POPCORN, HelpLabels::HF_POPCORN_JULIA,                           //
        FractalFlags::TRIG4,                                                            //
        -3.0F, 3.0F, -2.25F, 2.25F,                                                     //
        0,                                                                              //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::POPCORN_JUL_L,   //
        SymmetryType::NONE,                                                             //
        popcorn_fractal_fn, other_julia_fp_per_pixel, julia_fp_setup, standard_fractal, //
        STD_BAILOUT                                                                     //
    },

    {
        FractalType::POPCORN_JUL_L,                                                        //
        T_POPCORN_JUL + 1,                                                                 //
        {STEP_X, STEP_Y, CONSTANT_X, CONSTANT_Y},                                          //
        {0.05, 0, 3.0, 0},                                                                 //
        HelpLabels::HT_POPCORN, HelpLabels::HF_POPCORN_JULIA,                              //
        FractalFlags::TRIG4,                                                               //
        -3.0F, 3.0F, -2.25F, 2.25F,                                                        //
        16,                                                                                //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::POPCORN_JUL_FP,     //
        SymmetryType::NONE,                                                                //
        long_popcorn_fractal_fn, long_julia_per_pixel, julia_long_setup, standard_fractal, //
        STD_BAILOUT                                                                        //
    },

    {
        FractalType::L_SYSTEM,                                                                             //
        "lsystem",                                                                                         //
        {"+Order", "", "", ""},                                                                            //
        {2, 0, 0, 0},                                                                                      //
        HelpLabels::HT_L_SYSTEM, HelpLabels::SPECIAL_L_SYSTEM,                                             //
        FractalFlags::NO_ZOOM | FractalFlags::NO_RESUME | FractalFlags::NO_GUESS | FractalFlags::NO_TRACE, //
        -1.0F, 1.0F, -1.0F, 1.0F,                                                                          //
        1,                                                                                                 //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,                         //
        SymmetryType::NONE,                                                                                //
        nullptr, nullptr, standalone_setup, lsystem,                                                       //
        NO_BAILOUT                                                                                         //
    },

    {
        FractalType::MAN_O_WAR_J_FP,                                                  //
        T_MAN_O_WAR_J,                                                                //
        {REAL_PARAM, IMAG_PARAM, "", ""},                                             //
        {0, 0, 0, 0},                                                                 //
        HelpLabels::HT_TAYLOR_SKINNER_VARIATIONS, HelpLabels::HF_MAN_O_WAR_JULIA,     //
        FractalFlags::OK_JB | FractalFlags::BAIL_TEST,                                //
        -2.5F, 1.5F, -1.5F, 1.5F,                                                     //
        0,                                                                            //
        FractalType::NO_FRACTAL, FractalType::MAN_O_WAR_FP, FractalType::MAN_O_WAR_J, //
        SymmetryType::NONE,                                                           //
        man_o_war_fp_fractal, julia_fp_per_pixel, julia_fp_setup, standard_fractal,   //
        STD_BAILOUT                                                                   //
    },

    {
        FractalType::MAN_O_WAR_J,                                                     //
        T_MAN_O_WAR_J + 1,                                                            //
        {REAL_PARAM, IMAG_PARAM, "", ""},                                             //
        {0, 0, 0, 0},                                                                 //
        HelpLabels::HT_TAYLOR_SKINNER_VARIATIONS, HelpLabels::HF_MAN_O_WAR_JULIA,     //
        FractalFlags::OK_JB | FractalFlags::BAIL_TEST,                                //
        -2.5F, 1.5F, -1.5F, 1.5F,                                                     //
        1,                                                                            //
        FractalType::NO_FRACTAL, FractalType::MAN_O_WAR, FractalType::MAN_O_WAR_J_FP, //
        SymmetryType::NONE,                                                           //
        man_o_war_fractal, julia_per_pixel, julia_long_setup, standard_fractal,       //
        STD_BAILOUT                                                                   //
    },

    {
        FractalType::FN_PLUS_FN_PIX_FP,                                                     //
        T_FN_Z_PLUS_FN_PIX,                                                                 //
        {REAL_Z0, IMAG_Z0, RE_COEF_TRIG2, IM_COEF_TRIG2},                                   //
        {0, 0, 1, 0},                                                                       //
        HelpLabels::HT_TAYLOR_SKINNER_VARIATIONS, HelpLabels::HF_FN_PLUS_FN_PIX,            //
        FractalFlags::TRIG2 | FractalFlags::BAIL_TEST,                                      //
        -3.6F, 3.6F, -2.7F, 2.7F,                                                           //
        0,                                                                                  //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::FN_PLUS_FN_PIX_LONG, //
        SymmetryType::NONE,                                                                 //
        richard_8_fp_fractal, other_richard_8_fp_per_pixel, mandel_fp_setup,                //
        standard_fractal,                                                                   //
        TRIG_BAILOUT_L                                                                      //
    },

    {
        FractalType::FN_PLUS_FN_PIX_LONG,                                                 //
        T_FN_Z_PLUS_FN_PIX + 1,                                                           //
        {REAL_Z0, IMAG_Z0, RE_COEF_TRIG2, IM_COEF_TRIG2},                                 //
        {0, 0, 1, 0},                                                                     //
        HelpLabels::HT_TAYLOR_SKINNER_VARIATIONS, HelpLabels::HF_FN_PLUS_FN_PIX,          //
        FractalFlags::TRIG2 | FractalFlags::BAIL_TEST,                                    //
        -3.6F, 3.6F, -2.7F, 2.7F,                                                         //
        1,                                                                                //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::FN_PLUS_FN_PIX_FP, //
        SymmetryType::NONE,                                                               //
        richard_8_fractal, long_richard_8_per_pixel, mandel_long_setup,                   //
        standard_fractal,                                                                 //
        TRIG_BAILOUT_L                                                                    //
    },

    {
        FractalType::MARKS_MANDEL_PWR_FP,                                                //
        T_MARKS_MANDEL_PWR,                                                              //
        {REAL_Z0, IMAG_Z0, "", ""},                                                      //
        {0, 0, 0, 0},                                                                    //
        HelpLabels::HT_PETERSON_VARIATIONS, HelpLabels::HF_MARKS_MANDEL_POWER,           //
        FractalFlags::TRIG1 | FractalFlags::BAIL_TEST,                                   //
        -2.5F, 1.5F, -1.5F, 1.5F,                                                        //
        0,                                                                               //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::MARKS_MANDEL_PWR, //
        SymmetryType::X_AXIS_NO_PARAM,                                                   //
        marks_mandel_pwr_fp_fractal, marks_mandel_pwr_fp_per_pixel, mandel_fp_setup,     //
        standard_fractal,                                                                //
        STD_BAILOUT                                                                      //
    },

    {
        FractalType::MARKS_MANDEL_PWR,                                                      //
        T_MARKS_MANDEL_PWR + 1,                                                             //
        {REAL_Z0, IMAG_Z0, "", ""},                                                         //
        {0, 0, 0, 0},                                                                       //
        HelpLabels::HT_PETERSON_VARIATIONS, HelpLabels::HF_MARKS_MANDEL_POWER,              //
        FractalFlags::TRIG1 | FractalFlags::BAIL_TEST,                                      //
        -2.5F, 1.5F, -1.5F, 1.5F,                                                           //
        1,                                                                                  //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::MARKS_MANDEL_PWR_FP, //
        SymmetryType::X_AXIS_NO_PARAM,                                                      //
        marks_mandel_pwr_fractal, marks_mandel_pwr_per_pixel, mandel_long_setup,            //
        standard_fractal,                                                                   //
        STD_BAILOUT                                                                         //
    },

    {
        FractalType::TIMS_ERROR_FP,                                                //
        T_TIMS_ERROR,                                                              //
        {REAL_Z0, IMAG_Z0, "", ""},                                                //
        {0, 0, 0, 0},                                                              //
        HelpLabels::HT_PETERSON_VARIATIONS, HelpLabels::HF_TIMS_ERROR,             //
        FractalFlags::TRIG1 | FractalFlags::BAIL_TEST,                             //
        -2.9F, 4.3F, -2.7F, 2.7F,                                                  //
        0,                                                                         //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::TIMS_ERROR, //
        SymmetryType::X_AXIS_NO_PARAM,                                             //
        tims_error_fp_fractal, marks_mandel_pwr_fp_per_pixel, mandel_fp_setup,     //
        standard_fractal,                                                          //
        STD_BAILOUT                                                                //
    },

    {
        FractalType::TIMS_ERROR,                                                      //
        T_TIMS_ERROR + 1,                                                             //
        {REAL_Z0, IMAG_Z0, "", ""},                                                   //
        {0, 0, 0, 0},                                                                 //
        HelpLabels::HT_PETERSON_VARIATIONS, HelpLabels::HF_TIMS_ERROR,                //
        FractalFlags::TRIG1 | FractalFlags::BAIL_TEST,                                //
        -2.9F, 4.3F, -2.7F, 2.7F,                                                     //
        1,                                                                            //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::TIMS_ERROR_FP, //
        SymmetryType::X_AXIS_NO_PARAM,                                                //
        tims_error_fractal, marks_mandel_pwr_per_pixel, mandel_long_setup,            //
        standard_fractal,                                                             //
        STD_BAILOUT                                                                   //
    },

    {
        FractalType::BIF_EQ_SIN_PI_L,                                                                    //
        T_BIF_EQ_SIN_PI + 1,                                                                             //
        {FILT, SEED_POP, "", ""},                                                                        //
        {1000.0, 0.66, 0, 0},                                                                            //
        HelpLabels::HT_BIFURCATION, HelpLabels::HF_BIF_EQ_SIN_PI,                                        //
        FractalFlags::TRIG1 | FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::NO_ROTATE, //
        -2.5F, 2.5F, -3.5F, 3.5F,                                                                        //
        1,                                                                                               //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::BIF_EQ_SIN_PI,                    //
        SymmetryType::NONE,                                                                              //
        long_bifurc_set_trig_pi, nullptr, standalone_setup, bifurcation,                                 //
        NO_BAILOUT                                                                                       //
    },

    {
        FractalType::BIF_PLUS_SIN_PI_L,                                                                  //
        T_BIF_PLUS_SIN_PI + 1,                                                                           //
        {FILT, SEED_POP, "", ""},                                                                        //
        {1000.0, 0.66, 0, 0},                                                                            //
        HelpLabels::HT_BIFURCATION, HelpLabels::HF_BIF_PLUS_SIN_PI,                                      //
        FractalFlags::TRIG1 | FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::NO_ROTATE, //
        0.275F, 1.45F, 0.0F, 2.0F,                                                                       //
        1,                                                                                               //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::BIF_PLUS_SIN_PI,                  //
        SymmetryType::NONE,                                                                              //
        long_bifurc_add_trig_pi, nullptr, standalone_setup, bifurcation,                                 //
        NO_BAILOUT                                                                                       //
    },

    {
        FractalType::BIF_STEWART,                                                                        //
        T_BIF_STEWART,                                                                                   //
        {FILT, SEED_POP, "", ""},                                                                        //
        {1000.0, 0.66, 0, 0},                                                                            //
        HelpLabels::HT_BIFURCATION, HelpLabels::HF_BIF_STEWART,                                          //
        FractalFlags::TRIG1 | FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::NO_ROTATE, //
        0.7F, 2.0F, -1.1F, 1.1F,                                                                         //
        0,                                                                                               //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::BIF_STEWART_L,                    //
        SymmetryType::NONE,                                                                              //
        bifurc_stewart_trig, nullptr, standalone_setup, bifurcation,                                     //
        NO_BAILOUT                                                                                       //
    },

    {
        FractalType::BIF_STEWART_L,                                                                      //
        T_BIF_STEWART + 1,                                                                               //
        {FILT, SEED_POP, "", ""},                                                                        //
        {1000.0, 0.66, 0, 0},                                                                            //
        HelpLabels::HT_BIFURCATION, HelpLabels::HF_BIF_STEWART,                                          //
        FractalFlags::TRIG1 | FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::NO_ROTATE, //
        0.7F, 2.0F, -1.1F, 1.1F,                                                                         //
        1,                                                                                               //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::BIF_STEWART,                      //
        SymmetryType::NONE,                                                                              //
        long_bifurc_stewart_trig, nullptr, standalone_setup, bifurcation,                                //
        NO_BAILOUT                                                                                       //
    },

    {
        FractalType::HOPALONG_FP,                                                  //
        "hopalong",                                                                //
        {"a", "b", "c", ""},                                                       //
        {.4, 1, 0, 0},                                                             //
        HelpLabels::HT_MARTIN, HelpLabels::HF_HOPALONG,                            //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::INF_CALC,  //
        -2.0F, 3.0F, -1.625F, 2.625F,                                              //
        0,                                                                         //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, //
        SymmetryType::NONE,                                                        //
        (VF) hopalong2d_float_orbit, nullptr, orbit3d_float_setup, orbit2d_float,  //
        NO_BAILOUT                                                                 //
    },

    {
        FractalType::CIRCLE_FP,                                                    //
        "circle",                                                                  //
        {"magnification", "", "", ""},                                             //
        {200000L, 0, 0, 0},                                                        //
        HelpLabels::HT_CIRCLE, HelpLabels::HF_CIRCLE,                              //
        FractalFlags::NONE,                                                        //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                  //
        0,                                                                         //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, //
        SymmetryType::XY_AXIS,                                                     //
        circle_fp_fractal, julia_fp_per_pixel, julia_fp_setup, standard_fractal,   //
        NO_BAILOUT                                                                 //
    },

    {
        FractalType::MARTIN_FP,                                                    //
        "martin",                                                                  //
        {"a", "", "", ""},                                                         //
        {3.14, 0, 0, 0},                                                           //
        HelpLabels::HT_MARTIN, HelpLabels::HF_MARTIN,                              //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::INF_CALC,  //
        -32.0F, 32.0F, -24.0F, 24.0F,                                              //
        0,                                                                         //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, //
        SymmetryType::NONE,                                                        //
        (VF) martin2d_float_orbit, nullptr, orbit3d_float_setup, orbit2d_float,    //
        NO_BAILOUT                                                                 //
    },

    {
        FractalType::LYAPUNOV,                                                     //
        "lyapunov",                                                                //
        {"+Order (integer)", "Population Seed", "+Filter Cycles", ""},             //
        {0, 0.5, 0, 0},                                                            //
        HelpLabels::HT_LYAPUNOV, HelpLabels::HT_LYAPUNOV,                          //
        FractalFlags::NONE,                                                        //
        -8.0F, 8.0F, -6.0F, 6.0F,                                                  //
        0,                                                                         //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, //
        SymmetryType::NONE,                                                        //
        lyapunov_orbit, nullptr, lya_setup, lyapunov,                              //
        NO_BAILOUT                                                                 //
    },

    {
        FractalType::LORENZ_3D1_FP,                                                //
        "lorenz3d1",                                                               //
        {TIME_STEP, "a", "b", "c"},                                                //
        {.02, 5, 15, 1},                                                           //
        HelpLabels::HT_LORENZ, HelpLabels::HF_LORENZ_3D1,                          //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::NO_RESUME | FractalFlags::PARAMS_3D |
            FractalFlags::INF_CALC,                                                //
        -30.0F, 30.0F, -30.0F, 30.0F,                                              //
        0,                                                                         //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, //
        SymmetryType::NONE,                                                        //
        (VF) lorenz3d1_float_orbit, nullptr, orbit3d_float_setup, orbit3d_float,   //
        NO_BAILOUT                                                                 //
    },

    {
        FractalType::LORENZ_3D3_FP,                                                //
        "lorenz3d3",                                                               //
        {TIME_STEP, "a", "b", "c"},                                                //
        {.02, 10, 28, 2.66},                                                       //
        HelpLabels::HT_LORENZ, HelpLabels::HF_LORENZ_3D3,                          //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::NO_RESUME | FractalFlags::PARAMS_3D |
            FractalFlags::INF_CALC,                                                //
        -30.0F, 30.0F, -30.0F, 30.0F,                                              //
        0,                                                                         //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, //
        SymmetryType::NONE,                                                        //
        (VF) lorenz3d3_float_orbit, nullptr, orbit3d_float_setup, orbit3d_float,   //
        NO_BAILOUT                                                                 //
    },

    {
        FractalType::LORENZ_3D4_FP,                                                //
        "lorenz3d4",                                                               //
        {TIME_STEP, "a", "b", "c"},                                                //
        {.02, 10, 28, 2.66},                                                       //
        HelpLabels::HT_LORENZ, HelpLabels::HF_LORENZ_3D4,                          //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::NO_RESUME | FractalFlags::PARAMS_3D |
            FractalFlags::INF_CALC,                                                //
        -30.0F, 30.0F, -30.0F, 30.0F,                                              //
        0,                                                                         //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, //
        SymmetryType::NONE,                                                        //
        (VF) lorenz3d4_float_orbit, nullptr, orbit3d_float_setup, orbit3d_float,   //
        NO_BAILOUT                                                                 //
    },

    {
        FractalType::LAMBDA_FN_FN_L,                                                         //
        T_LAMBDA_FN_OR_FN + 1,                                                               //
        {REAL_PARAM, IMAG_PARAM, "Function Shift Value", ""},                                //
        {1, 0.1, 1, 0},                                                                      //
        HelpLabels::HT_FN_OR_FN, HelpLabels::HF_LAMBDA_FN_FN,                                //
        FractalFlags::TRIG2 | FractalFlags::BAIL_TEST,                                       //
        -4.0F, 4.0F, -3.0F, 3.0F,                                                            //
        16,                                                                                  //
        FractalType::NO_FRACTAL, FractalType::MAN_LAM_FN_FN_L, FractalType::LAMBDA_FN_FN_FP, //
        SymmetryType::ORIGIN,                                                                //
        lambda_trig_or_trig_fractal, long_julia_per_pixel, lambda_trig_or_trig_setup,        //
        standard_fractal,                                                                    //
        TRIG_BAILOUT_L                                                                       //
    },

    {
        FractalType::LAMBDA_FN_FN_FP,                                                        //
        T_LAMBDA_FN_OR_FN,                                                                   //
        {REAL_PARAM, IMAG_PARAM, "Function Shift Value", ""},                                //
        {1, 0.1, 1, 0},                                                                      //
        HelpLabels::HT_FN_OR_FN, HelpLabels::HF_LAMBDA_FN_FN,                                //
        FractalFlags::TRIG2 | FractalFlags::BAIL_TEST,                                       //
        -4.0F, 4.0F, -3.0F, 3.0F,                                                            //
        0,                                                                                   //
        FractalType::NO_FRACTAL, FractalType::MAN_LAM_FN_FN_FP, FractalType::LAMBDA_FN_FN_L, //
        SymmetryType::ORIGIN,                                                                //
        lambda_trig_or_trig_fp_fractal, other_julia_fp_per_pixel,                            //
        lambda_trig_or_trig_setup, standard_fractal,                                         //
        TRIG_BAILOUT_L                                                                       //
    },

    {
        FractalType::JUL_FN_FN_L,                                                     //
        T_JULIA_FN_OR_FN + 1,                                                         //
        {REAL_PARAM, IMAG_PARAM, "Function Shift Value", ""},                         //
        {0, 0, 8, 0},                                                                 //
        HelpLabels::HT_FN_OR_FN, HelpLabels::HF_JULIA_FN_FN,                          //
        FractalFlags::TRIG2 | FractalFlags::BAIL_TEST,                                //
        -4.0F, 4.0F, -3.0F, 3.0F,                                                     //
        16,                                                                           //
        FractalType::NO_FRACTAL, FractalType::MAN_FN_FN_L, FractalType::JUL_FN_FN_FP, //
        SymmetryType::X_AXIS,                                                         //
        julia_trig_or_trig_fractal, long_julia_per_pixel, julia_trig_or_trig_setup,   //
        standard_fractal,                                                             //
        TRIG_BAILOUT_L                                                                //
    },

    {
        FractalType::JUL_FN_FN_FP,                                                    //
        T_JULIA_FN_OR_FN,                                                             //
        {REAL_PARAM, IMAG_PARAM, "Function Shift Value", ""},                         //
        {0, 0, 8, 0},                                                                 //
        HelpLabels::HT_FN_OR_FN, HelpLabels::HF_JULIA_FN_FN,                          //
        FractalFlags::TRIG2 | FractalFlags::BAIL_TEST,                                //
        -4.0F, 4.0F, -3.0F, 3.0F,                                                     //
        0,                                                                            //
        FractalType::NO_FRACTAL, FractalType::MAN_FN_FN_FP, FractalType::JUL_FN_FN_L, //
        SymmetryType::X_AXIS,                                                         //
        julia_trig_or_trig_fp_fractal, other_julia_fp_per_pixel,                      //
        julia_trig_or_trig_setup, standard_fractal,                                   //
        TRIG_BAILOUT_L                                                                //
    },

    {
        FractalType::MAN_LAM_FN_FN_L,                                                        //
        T_MAN_LAM_FN_OR_FN + 1,                                                              //
        {REAL_Z0, IMAG_Z0, "Function Shift Value", ""},                                      //
        {0, 0, 10, 0},                                                                       //
        HelpLabels::HT_FN_OR_FN, HelpLabels::HF_MANDEL_LAMBDA_FN_FN,                         //
        FractalFlags::TRIG2 | FractalFlags::BAIL_TEST,                                       //
        -4.0F, 4.0F, -3.0F, 3.0F,                                                            //
        16,                                                                                  //
        FractalType::LAMBDA_FN_FN_L, FractalType::NO_FRACTAL, FractalType::MAN_LAM_FN_FN_FP, //
        SymmetryType::X_AXIS_NO_PARAM,                                                       //
        lambda_trig_or_trig_fractal, long_mandel_per_pixel,                                  //
        man_lam_trig_or_trig_setup, standard_fractal,                                        //
        TRIG_BAILOUT_L                                                                       //
    },

    {
        FractalType::MAN_LAM_FN_FN_FP,                                                       //
        T_MAN_LAM_FN_OR_FN,                                                                  //
        {REAL_Z0, IMAG_Z0, "Function Shift Value", ""},                                      //
        {0, 0, 10, 0},                                                                       //
        HelpLabels::HT_FN_OR_FN, HelpLabels::HF_MANDEL_LAMBDA_FN_FN,                         //
        FractalFlags::TRIG2 | FractalFlags::BAIL_TEST,                                       //
        -4.0F, 4.0F, -3.0F, 3.0F,                                                            //
        0,                                                                                   //
        FractalType::LAMBDA_FN_FN_FP, FractalType::NO_FRACTAL, FractalType::MAN_LAM_FN_FN_L, //
        SymmetryType::X_AXIS_NO_PARAM,                                                       //
        lambda_trig_or_trig_fp_fractal, other_mandel_fp_per_pixel,                           //
        man_lam_trig_or_trig_setup, standard_fractal,                                        //
        TRIG_BAILOUT_L                                                                       //
    },

    {
        FractalType::MAN_FN_FN_L,                                                     //
        T_MANDEL_FN_OR_FN + 1,                                                        //
        {REAL_Z0, IMAG_Z0, "Function Shift Value", ""},                               //
        {0, 0, 0.5, 0},                                                               //
        HelpLabels::HT_FN_OR_FN, HelpLabels::HF_MANDEL_FN_FN,                         //
        FractalFlags::TRIG2 | FractalFlags::BAIL_TEST,                                //
        -4.0F, 4.0F, -3.0F, 3.0F,                                                     //
        16,                                                                           //
        FractalType::JUL_FN_FN_L, FractalType::NO_FRACTAL, FractalType::MAN_FN_FN_FP, //
        SymmetryType::X_AXIS_NO_PARAM,                                                //
        julia_trig_or_trig_fractal, long_mandel_per_pixel,                            //
        mandel_trig_or_trig_setup, standard_fractal,                                  //
        TRIG_BAILOUT_L                                                                //
    },

    {
        FractalType::MAN_FN_FN_FP,                                                    //
        T_MANDEL_FN_OR_FN,                                                            //
        {REAL_Z0, IMAG_Z0, "Function Shift Value", ""},                               //
        {0, 0, 0.5, 0},                                                               //
        HelpLabels::HT_FN_OR_FN, HelpLabels::HF_MANDEL_FN_FN,                         //
        FractalFlags::TRIG2 | FractalFlags::BAIL_TEST,                                //
        -4.0F, 4.0F, -3.0F, 3.0F,                                                     //
        0,                                                                            //
        FractalType::JUL_FN_FN_FP, FractalType::NO_FRACTAL, FractalType::MAN_FN_FN_L, //
        SymmetryType::X_AXIS_NO_PARAM,                                                //
        julia_trig_or_trig_fp_fractal, other_mandel_fp_per_pixel,                     //
        mandel_trig_or_trig_setup, standard_fractal,                                  //
        TRIG_BAILOUT_L                                                                //
    },

    {
        FractalType::BIF_MAY_L,                                                    //
        T_BIF_MAY + 1,                                                             //
        {FILT, SEED_POP, "Beta >= 2", ""},                                         //
        {300.0, 0.9, 5, 0},                                                        //
        HelpLabels::HT_BIFURCATION, HelpLabels::HF_BIF_MAY,                        //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::NO_ROTATE, //
        -3.5F, -0.9F, -0.5F, 3.2F,                                                 //
        16,                                                                        //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::BIF_MAY,    //
        SymmetryType::NONE,                                                        //
        long_bifurc_may, nullptr, bifurc_may_setup, bifurcation,                   //
        NO_BAILOUT                                                                 //
    },

    {
        FractalType::BIF_MAY,                                                      //
        T_BIF_MAY,                                                                 //
        {FILT, SEED_POP, "Beta >= 2", ""},                                         //
        {300.0, 0.9, 5, 0},                                                        //
        HelpLabels::HT_BIFURCATION, HelpLabels::HF_BIF_MAY,                        //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::NO_ROTATE, //
        -3.5F, -0.9F, -0.5F, 3.2F,                                                 //
        0,                                                                         //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::BIF_MAY_L,  //
        SymmetryType::NONE,                                                        //
        bifurc_may, nullptr, bifurc_may_setup, bifurcation,                        //
        NO_BAILOUT                                                                 //
    },

    {
        FractalType::HALLEY_MP,                                                   //
        T_HALLEY + 1,                                                             //
        {ORDER, REAL_RELAX, EPSILON, IMAG_RELAX},                                 //
        {6, 1.0, 0.0001, 0},                                                      //
        HelpLabels::HT_HALLEY, HelpLabels::HF_HALLEY,                             //
        FractalFlags::NONE,                                                       //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                 //
        0,                                                                        //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::HALLEY,    //
        SymmetryType::XY_AXIS,                                                    //
        mpc_halley_fractal, mpc_halley_per_pixel, halley_setup, standard_fractal, //
        NO_BAILOUT                                                                //
    },

    {
        FractalType::HALLEY,                                                      //
        T_HALLEY,                                                                 //
        {ORDER, REAL_RELAX, EPSILON, IMAG_RELAX},                                 //
        {6, 1.0, 0.0001, 0},                                                      //
        HelpLabels::HT_HALLEY, HelpLabels::HF_HALLEY,                             //
        FractalFlags::NONE,                                                       //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                 //
        0,                                                                        //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::HALLEY_MP, //
        SymmetryType::XY_AXIS,                                                    //
        halley_fractal, halley_per_pixel, halley_setup, standard_fractal,         //
        NO_BAILOUT                                                                //
    },

    {
        FractalType::DYNAMIC_FP,                                                   //
        "dynamic",                                                                 //
        {"+# of intervals (<0 = connect)", "time step (<0 = Euler)", "a", "b"},    //
        {50, .1, 1, 3},                                                            //
        HelpLabels::HT_DYNAMIC_SYSTEM, HelpLabels::HF_DYNAMIC,                     //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::TRIG1,     //
        -20.0F, 20.0F, -20.0F, 20.0F,                                              //
        0,                                                                         //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, //
        SymmetryType::NONE,                                                        //
        (VF) dynam_float, nullptr, dynam2d_float_setup, dynam2d_float,             //
        NO_BAILOUT                                                                 //
    },

    {
        FractalType::QUAT_FP,                                                       //
        "quat",                                                                     //
        {"notused", "notused", "cj", "ck"},                                         //
        {0, 0, 0, 0},                                                               //
        HelpLabels::HT_QUATERNION, HelpLabels::HF_QUATERNION,                       //
        FractalFlags::OK_JB,                                                        //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                   //
        0,                                                                          //
        FractalType::QUAT_JUL_FP, FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, //
        SymmetryType::X_AXIS,                                                       //
        quaternion_fp_fractal, quaternion_fp_per_pixel, mandel_fp_setup,            //
        standard_fractal,                                                           //
        TRIG_BAILOUT_L                                                              //
    },

    {
        FractalType::QUAT_JUL_FP,                                               //
        "quatjul",                                                              //
        {"c1", "ci", "cj", "ck"},                                               //
        {-.745, 0, .113, .05},                                                  //
        HelpLabels::HT_QUATERNION, HelpLabels::HF_QUATERNION_JULIA,             //
        FractalFlags::OK_JB | FractalFlags::MORE,                               //
        -2.0F, 2.0F, -1.5F, 1.5F,                                               //
        0,                                                                      //
        FractalType::NO_FRACTAL, FractalType::QUAT_FP, FractalType::NO_FRACTAL, //
        SymmetryType::ORIGIN,                                                   //
        quaternion_fp_fractal, quaternion_jul_fp_per_pixel, julia_fp_setup,     //
        standard_fractal,                                                       //
        TRIG_BAILOUT_L                                                          //
    },

    {
        FractalType::CELLULAR,                                                     //
        "cellular",                                                                //
        {CELL_INIT, CELL_RULE, CELL_TYPE, CELL_START},                             //
        {11.0, 3311100320.0, 41.0, 0},                                             //
        HelpLabels::HT_CELLULAR, HelpLabels::HF_CELLULAR,                          //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::NO_ZOOM,   //
        -1.0F, 1.0F, -1.0F, 1.0F,                                                  //
        0,                                                                         //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, //
        SymmetryType::NONE,                                                        //
        nullptr, nullptr, cellular_setup, cellular,                                //
        NO_BAILOUT                                                                 //
    },

    {
        FractalType::JULIBROT_FP,                                                //
        T_JULIBROT,                                                              //
        {"", "", "", ""},                                                        //
        {0, 0, 0, 0},                                                            //
        HelpLabels::HT_JULIBROT, HelpLabels::NONE,                               //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::NO_ROTATE |
            FractalFlags::NO_RESUME,                                             //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                //
        0,                                                                       //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::JULIBROT, //
        SymmetryType::NONE,                                                      //
        julia_fp_fractal, jb_fp_per_pixel, julibrot_setup, std_4d_fp_fractal,    //
        STD_BAILOUT                                                              //
    },

#ifdef RANDOM_RUN
    {
        FractalType::INVERSE_JULIA,                                                                         //
        T_JULIA_INVERSE + 1,                                                                                //
        {REAL_PARAM, IMAG_PARAM, MAX_HITS, "Random Run Interval"},                                          //
        {-0.11, 0.6557, 4, 1024},                                                                           //
        HelpLabels::HT_INVERSE_JULIA, HelpLabels::HF_INVERSE_JULIA,                                         //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::INF_CALC | FractalFlags::NO_RESUME, //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                                           //
        24,                                                                                                 //
        FractalType::NO_FRACTAL, FractalType::MANDEL, FractalType::INVERSE_JULIA_FP,                        //
        SymmetryType::NONE,                                                                                 //
        l_inverse_julia_orbit, nullptr, orbit3d_long_setup, inverse_julia_per_image,                        //
        NO_BAILOUT                                                                                          //
    },

    {
        FractalType::INVERSE_JULIA_FP,                                                                      //
        T_JULIA_INVERSE,                                                                                    //
        {REAL_PARAM, IMAG_PARAM, MAX_HITS, "Random Run Interval"},                                          //
        {-0.11, 0.6557, 4, 1024},                                                                           //
        HelpLabels::HT_INVERSE_JULIA, HelpLabels::HF_INVERSE_JULIA,                                         //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::INF_CALC | FractalFlags::NO_RESUME, //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                                           //
        0,                                                                                                  //
        FractalType::NO_FRACTAL, FractalType::MANDEL, FractalType::INVERSE_JULIA,                           //
        SymmetryType::NONE,                                                                                 //
        m_inverse_julia_orbit, nullptr, orbit3d_float_setup, inverse_julia_per_image,                       //
        NO_BAILOUT                                                                                          //
    },

#else
    {
        FractalType::INVERSE_JULIA,                                                                         //
        T_JULIA_INVERSE + 1,                                                                                //
        {REAL_PARAM, IMAG_PARAM, MAX_HITS, ""},                                                             //
        {-0.11, 0.6557, 4, 1024},                                                                           //
        HelpLabels::HT_INVERSE_JULIA, HelpLabels::HF_INVERSE_JULIA,                                         //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::INF_CALC | FractalFlags::NO_RESUME, //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                                           //
        24,                                                                                                 //
        FractalType::NO_FRACTAL, FractalType::MANDEL, FractalType::INVERSE_JULIA_FP,                        //
        SymmetryType::NONE,                                                                                 //
        l_inverse_julia_orbit, nullptr, orbit3d_long_setup, inverse_julia_per_image,                        //
        NO_BAILOUT                                                                                          //
    },

    {
        FractalType::INVERSE_JULIA_FP,                                                                      //
        T_JULIA_INVERSE,                                                                                    //
        {REAL_PARAM, IMAG_PARAM, MAX_HITS, ""},                                                             //
        {-0.11, 0.6557, 4, 1024},                                                                           //
        HelpLabels::HT_INVERSE_JULIA, HelpLabels::HF_INVERSE_JULIA,                                         //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::INF_CALC | FractalFlags::NO_RESUME, //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                                           //
        0,                                                                                                  //
        FractalType::NO_FRACTAL, FractalType::MANDEL, FractalType::INVERSE_JULIA,                           //
        SymmetryType::NONE,                                                                                 //
        m_inverse_julia_orbit, nullptr, orbit3d_float_setup, inverse_julia_per_image,                       //
        NO_BAILOUT                                                                                          //
    },

#endif

    {
        FractalType::MANDEL_CLOUD,                                                 //
        "mandelcloud",                                                             //
        {"+# of intervals (<0 = connect)", "", "", ""},                            //
        {50, 0, 0, 0},                                                             //
        HelpLabels::HT_MANDEL_CLOUD, HelpLabels::HF_MANDEL_CLOUD,                  //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE,                           //
        -2.5F, 1.5F, -1.5F, 1.5F,                                                  //
        0,                                                                         //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, //
        SymmetryType::NONE,                                                        //
        (VF) mandel_cloud_float, nullptr, dynam2d_float_setup, dynam2d_float,      //
        NO_BAILOUT                                                                 //
    },

    {
        FractalType::PHOENIX,                                                        //
        T_PHOENIX + 1,                                                               //
        {P1_REAL, P2_REAL, DEGREE_Z, ""},                                            //
        {0.56667, -0.5, 0, 0},                                                       //
        HelpLabels::HT_PHOENIX, HelpLabels::HF_PHOENIX,                              //
        FractalFlags::BAIL_TEST,                                                     //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                    //
        1,                                                                           //
        FractalType::NO_FRACTAL, FractalType::MAND_PHOENIX, FractalType::PHOENIX_FP, //
        SymmetryType::X_AXIS,                                                        //
        long_phoenix_fractal, long_phoenix_per_pixel, phoenix_setup,                 //
        standard_fractal,                                                            //
        STD_BAILOUT                                                                  //
    },

    {
        FractalType::PHOENIX_FP,                                                     //
        T_PHOENIX,                                                                   //
        {P1_REAL, P2_REAL, DEGREE_Z, ""},                                            //
        {0.56667, -0.5, 0, 0},                                                       //
        HelpLabels::HT_PHOENIX, HelpLabels::HF_PHOENIX,                              //
        FractalFlags::BAIL_TEST,                                                     //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                    //
        0,                                                                           //
        FractalType::NO_FRACTAL, FractalType::MAND_PHOENIX_FP, FractalType::PHOENIX, //
        SymmetryType::X_AXIS,                                                        //
        phoenix_fractal, phoenix_per_pixel, phoenix_setup, standard_fractal,         //
        STD_BAILOUT                                                                  //
    },

    {
        FractalType::MAND_PHOENIX,                                                   //
        T_MAND_PHOENIX + 1,                                                          //
        {REAL_Z0, IMAG_Z0, DEGREE_Z, ""},                                            //
        {0.0, 0.0, 0, 0},                                                            //
        HelpLabels::HT_PHOENIX, HelpLabels::HF_MANDEL_PHOENIX,                       //
        FractalFlags::BAIL_TEST,                                                     //
        -2.5F, 1.5F, -1.5F, 1.5F,                                                    //
        1,                                                                           //
        FractalType::PHOENIX, FractalType::NO_FRACTAL, FractalType::MAND_PHOENIX_FP, //
        SymmetryType::NONE,                                                          //
        long_phoenix_fractal, long_mand_phoenix_per_pixel, mand_phoenix_setup,       //
        standard_fractal,                                                            //
        STD_BAILOUT                                                                  //
    },

    {
        FractalType::MAND_PHOENIX_FP,                                                //
        T_MAND_PHOENIX,                                                              //
        {REAL_Z0, IMAG_Z0, DEGREE_Z, ""},                                            //
        {0.0, 0.0, 0, 0},                                                            //
        HelpLabels::HT_PHOENIX, HelpLabels::HF_MANDEL_PHOENIX,                       //
        FractalFlags::BAIL_TEST,                                                     //
        -2.5F, 1.5F, -1.5F, 1.5F,                                                    //
        0,                                                                           //
        FractalType::PHOENIX_FP, FractalType::NO_FRACTAL, FractalType::MAND_PHOENIX, //
        SymmetryType::NONE,                                                          //
        phoenix_fractal, mand_phoenix_per_pixel, mand_phoenix_setup,                 //
        standard_fractal,                                                            //
        STD_BAILOUT                                                                  //
    },

    {
        FractalType::HYPER_CMPLX_FP,                                                     //
        "hypercomplex",                                                                  //
        {"notused", "notused", "cj", "ck"},                                              //
        {0, 0, 0, 0},                                                                    //
        HelpLabels::HT_HYPER_COMPLEX, HelpLabels::HF_HYPER_COMPLEX,                      //
        FractalFlags::OK_JB | FractalFlags::TRIG1,                                       //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                        //
        0,                                                                               //
        FractalType::HYPER_CMPLX_J_FP, FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, //
        SymmetryType::X_AXIS,                                                            //
        hyper_complex_fp_fractal, quaternion_fp_per_pixel, mandel_fp_setup,              //
        standard_fractal,                                                                //
        TRIG_BAILOUT_L                                                                   //
    },

    {
        FractalType::HYPER_CMPLX_J_FP,                                                 //
        "hypercomplexj",                                                               //
        {"c1", "ci", "cj", "ck"},                                                      //
        {-.745, 0, .113, .05},                                                         //
        HelpLabels::HT_HYPER_COMPLEX, HelpLabels::HF_HYPER_COMPLEX_JULIA,              //
        FractalFlags::OK_JB | FractalFlags::TRIG1 | FractalFlags::MORE,                //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                      //
        0,                                                                             //
        FractalType::NO_FRACTAL, FractalType::HYPER_CMPLX_FP, FractalType::NO_FRACTAL, //
        SymmetryType::ORIGIN,                                                          //
        hyper_complex_fp_fractal, quaternion_jul_fp_per_pixel, julia_fp_setup,         //
        standard_fractal,                                                              //
        TRIG_BAILOUT_L                                                                 //
    },

    {
        FractalType::FROTH,                                                      //
        T_FROTHY_BASIN + 1,                                                      //
        {FROTH_MAPPING, FROTH_SHADE, FROTH_A_VALUE, ""},                         //
        {1, 0, 1.028713768218725, 0},                                            //
        HelpLabels::HT_FROTH, HelpLabels::HF_FROTH,                              //
        FractalFlags::NO_TRACE,                                                  //
        -2.8F, 2.8F, -2.355F, 1.845F,                                            //
        28,                                                                      //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::FROTH_FP, //
        SymmetryType::NONE,                                                      //
        froth_per_orbit, froth_per_pixel, froth_setup, calc_froth,               //
        FROTH_BAILOUT                                                            //
    },

    {
        FractalType::FROTH_FP,                                                //
        T_FROTHY_BASIN,                                                       //
        {FROTH_MAPPING, FROTH_SHADE, FROTH_A_VALUE, ""},                      //
        {1, 0, 1.028713768218725, 0},                                         //
        HelpLabels::HT_FROTH, HelpLabels::HF_FROTH,                           //
        FractalFlags::NO_TRACE,                                               //
        -2.8F, 2.8F, -2.355F, 1.845F,                                         //
        0,                                                                    //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::FROTH, //
        SymmetryType::NONE,                                                   //
        froth_per_orbit, froth_per_pixel, froth_setup, calc_froth,            //
        FROTH_BAILOUT                                                         //
    },

    {
        FractalType::MANDEL4_FP,                                                    //
        T_MANDEL4,                                                                  //
        {REAL_Z0, IMAG_Z0, "", ""},                                                 //
        {0, 0, 0, 0},                                                               //
        HelpLabels::HT_MANDEL_JULIA4, HelpLabels::HF_MANDEL4,                       //
        FractalFlags::BAIL_TEST,                                                    //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                   //
        0,                                                                          //
        FractalType::JULIA4_FP, FractalType::NO_FRACTAL, FractalType::MANDEL4,      //
        SymmetryType::X_AXIS_NO_PARAM,                                              //
        mandel4_fp_fractal, mandel_fp_per_pixel, mandel_fp_setup, standard_fractal, //
        STD_BAILOUT                                                                 //
    },

    {
        FractalType::JULIA4_FP,                                                   //
        T_JULIA4,                                                                 //
        {REAL_PARAM, IMAG_PARAM, "", ""},                                         //
        {0.6, 0.55, 0, 0},                                                        //
        HelpLabels::HT_MANDEL_JULIA4, HelpLabels::HF_JULIA4,                      //
        FractalFlags::OK_JB | FractalFlags::BAIL_TEST,                            //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                 //
        0,                                                                        //
        FractalType::NO_FRACTAL, FractalType::MANDEL4_FP, FractalType::JULIA4,    //
        SymmetryType::ORIGIN,                                                     //
        mandel4_fp_fractal, julia_fp_per_pixel, julia_fp_setup, standard_fractal, //
        STD_BAILOUT                                                               //
    },

    {
        FractalType::MARKS_MANDEL_FP,                                                    //
        T_MARKS_MANDEL,                                                                  //
        {REAL_Z0, IMAG_Z0, RE_EXPONENT, ""},                                             //
        {0, 0, 1, 0},                                                                    //
        HelpLabels::HT_PETERSON_VARIATIONS, HelpLabels::HF_MARKS_MANDEL,                 //
        FractalFlags::BAIL_TEST,                                                         //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                        //
        0,                                                                               //
        FractalType::MARKS_JULIA_FP, FractalType::NO_FRACTAL, FractalType::MARKS_MANDEL, //
        SymmetryType::NONE,                                                              //
        marks_lambda_fp_fractal, marks_mandel_fp_per_pixel, mandel_fp_setup,             //
        standard_fractal,                                                                //
        STD_BAILOUT                                                                      //
    },

    {
        FractalType::MARKS_JULIA_FP,                                                     //
        T_MARKS_JULIA,                                                                   //
        {REAL_PARAM, IMAG_PARAM, RE_EXPONENT, ""},                                       //
        {0.1, 0.9, 1, 0},                                                                //
        HelpLabels::HT_PETERSON_VARIATIONS, HelpLabels::HF_MARKS_JULIA,                  //
        FractalFlags::BAIL_TEST,                                                         //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                        //
        0,                                                                               //
        FractalType::NO_FRACTAL, FractalType::MARKS_MANDEL_FP, FractalType::MARKS_JULIA, //
        SymmetryType::ORIGIN,                                                            //
        marks_lambda_fp_fractal, julia_fp_per_pixel, marks_julia_fp_setup,               //
        standard_fractal,                                                                //
        STD_BAILOUT                                                                      //
    },

    {
        FractalType::ICON,                                                                             //
        "icons",                                                                                       //
        {"Lambda", "Alpha", "Beta", "Gamma"},                                                          //
        {-2.34, 2.0, 0.2, 0.1},                                                                        //
        HelpLabels::HT_ICON, HelpLabels::HF_ICON,                                                      //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::INF_CALC | FractalFlags::MORE, //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                                      //
        0,                                                                                             //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,                     //
        SymmetryType::NONE,                                                                            //
        (VF) icon_float_orbit, nullptr, orbit3d_float_setup, orbit2d_float,                            //
        NO_BAILOUT                                                                                     //
    },

    {
        FractalType::ICON_3D,                                                      //
        "icons3d",                                                                 //
        {"Lambda", "Alpha", "Beta", "Gamma"},                                      //
        {-2.34, 2.0, 0.2, 0.1},                                                    //
        HelpLabels::HT_ICON, HelpLabels::HF_ICON,                                  //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::INF_CALC | FractalFlags::PARAMS_3D |
            FractalFlags::MORE,                                                    //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                  //
        0,                                                                         //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, //
        SymmetryType::NONE,                                                        //
        (VF) icon_float_orbit, nullptr, orbit3d_float_setup, orbit3d_float,        //
        NO_BAILOUT                                                                 //
    },

    {
        FractalType::PHOENIX_CPLX,                                                             //
        T_PHOENIX_CPLX + 1,                                                                    //
        {P1_REAL, P1_IMAG, P2_REAL, P2_IMAG},                                                  //
        {0.2, 0, 0.3, 0},                                                                      //
        HelpLabels::HT_PHOENIX, HelpLabels::HF_PHOENIX_CPLX,                                   //
        FractalFlags::MORE | FractalFlags::BAIL_TEST,                                          //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                              //
        1,                                                                                     //
        FractalType::NO_FRACTAL, FractalType::MAND_PHOENIX_CPLX, FractalType::PHOENIX_FP_CPLX, //
        SymmetryType::ORIGIN,                                                                  //
        long_phoenix_fractal_cplx, long_phoenix_per_pixel, phoenix_cplx_setup,                 //
        standard_fractal,                                                                      //
        STD_BAILOUT                                                                            //
    },

    {
        FractalType::PHOENIX_FP_CPLX,                                                          //
        T_PHOENIX_CPLX,                                                                        //
        {P1_REAL, P1_IMAG, P2_REAL, P2_IMAG},                                                  //
        {0.2, 0, 0.3, 0},                                                                      //
        HelpLabels::HT_PHOENIX, HelpLabels::HF_PHOENIX_CPLX,                                   //
        FractalFlags::MORE | FractalFlags::BAIL_TEST,                                          //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                              //
        0,                                                                                     //
        FractalType::NO_FRACTAL, FractalType::MAND_PHOENIX_FP_CPLX, FractalType::PHOENIX_CPLX, //
        SymmetryType::ORIGIN,                                                                  //
        phoenix_fractal_cplx, phoenix_per_pixel, phoenix_cplx_setup,                           //
        standard_fractal,                                                                      //
        STD_BAILOUT                                                                            //
    },

    {
        FractalType::MAND_PHOENIX_CPLX,                                                        //
        T_MAND_PHOENIX_CPLX + 1,                                                               //
        {REAL_Z0, IMAG_Z0, P2_REAL, P2_IMAG},                                                  //
        {0, 0, 0.5, 0},                                                                        //
        HelpLabels::HT_PHOENIX, HelpLabels::HF_MANDEL_PHOENIX_CPLX,                            //
        FractalFlags::MORE | FractalFlags::BAIL_TEST,                                          //
        -2.5F, 1.5F, -1.5F, 1.5F,                                                              //
        1,                                                                                     //
        FractalType::PHOENIX_CPLX, FractalType::NO_FRACTAL, FractalType::MAND_PHOENIX_FP_CPLX, //
        SymmetryType::X_AXIS,                                                                  //
        long_phoenix_fractal_cplx, long_mand_phoenix_per_pixel,                                //
        mand_phoenix_cplx_setup, standard_fractal,                                             //
        STD_BAILOUT                                                                            //
    },

    {
        FractalType::MAND_PHOENIX_FP_CPLX,                                                     //
        T_MAND_PHOENIX_CPLX,                                                                   //
        {REAL_Z0, IMAG_Z0, P2_REAL, P2_IMAG},                                                  //
        {0, 0, 0.5, 0},                                                                        //
        HelpLabels::HT_PHOENIX, HelpLabels::HF_MANDEL_PHOENIX_CPLX,                            //
        FractalFlags::MORE | FractalFlags::BAIL_TEST,                                          //
        -2.5F, 1.5F, -1.5F, 1.5F,                                                              //
        0,                                                                                     //
        FractalType::PHOENIX_FP_CPLX, FractalType::NO_FRACTAL, FractalType::MAND_PHOENIX_CPLX, //
        SymmetryType::X_AXIS,                                                                  //
        phoenix_fractal_cplx, mand_phoenix_per_pixel, mand_phoenix_cplx_setup,                 //
        standard_fractal,                                                                      //
        STD_BAILOUT                                                                            //
    },

    {
        FractalType::ANT,                                                          //
        "ant",                                                                     //
        {
            "#Rule String (1's and non-1's, 0 rand)",                              //
            "#Maxpts",                                                             //
            "+Numants (max 256)",                                                  //
            "+Ant type (1 or 2)"                                                   //
        },
        {1100, 1.0E9, 1, 1},                                                       //
        HelpLabels::HT_ANT, HelpLabels::HF_ANT,                                    //
        FractalFlags::NO_ZOOM | FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::NO_RESUME |
            FractalFlags::MORE,                                                    //
        -1.0F, 1.0F, -1.0F, 1.0F,                                                  //
        0,                                                                         //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, //
        SymmetryType::NONE,                                                        //
        nullptr, nullptr, standalone_setup, ant,                                   //
        NO_BAILOUT                                                                 //
    },

    {
        FractalType::CHIP,                                                         //
        "chip",                                                                    //
        {"a", "b", "c", ""},                                                       //
        {-15, -19, 1, 0},                                                          //
        HelpLabels::HT_MARTIN, HelpLabels::HF_CHIP,                                //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::INF_CALC,  //
        -760.0F, 760.0F, -570.0F, 570.0F,                                          //
        0,                                                                         //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, //
        SymmetryType::NONE,                                                        //
        (VF) chip2d_float_orbit, nullptr, orbit3d_float_setup, orbit2d_float,      //
        NO_BAILOUT                                                                 //
    },

    {
        FractalType::QUADRUP_TWO,                                                    //
        "quadruptwo",                                                                //
        {"a", "b", "c", ""},                                                         //
        {34, 1, 5, 0},                                                               //
        HelpLabels::HT_MARTIN, HelpLabels::HF_QUADRUP_TWO,                           //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::INF_CALC,    //
        -82.93367F, 112.2749F, -55.76383F, 90.64257F,                                //
        0,                                                                           //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,   //
        SymmetryType::NONE,                                                          //
        (VF) quadrup_two2d_float_orbit, nullptr, orbit3d_float_setup, orbit2d_float, //
        NO_BAILOUT                                                                   //
    },

    {
        FractalType::THREEPLY,                                                     //
        "threeply",                                                                //
        {"a", "b", "c", ""},                                                       //
        {-55, -1, -42, 0},                                                         //
        HelpLabels::HT_MARTIN, HelpLabels::HF_THREEPLY,                            //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::INF_CALC,  //
        -8000.0F, 8000.0F, -6000.0F, 6000.0F,                                      //
        0,                                                                         //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, //
        SymmetryType::NONE,                                                        //
        (VF) three_ply2d_float_orbit, nullptr, orbit3d_float_setup, orbit2d_float, //
        NO_BAILOUT                                                                 //
    },

    {
        FractalType::VL,                                                           //
        "volterra-lotka",                                                          //
        {"h", "p", "", ""},                                                        //
        {0.739, 0.739, 0, 0},                                                      //
        HelpLabels::HT_VOLTERRA_LOTKA, HelpLabels::HF_VOLTERRA_LOTKA,              //
        FractalFlags::NONE,                                                        //
        0.0F, 6.0F, 0.0F, 4.5F,                                                    //
        0,                                                                         //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, //
        SymmetryType::NONE,                                                        //
        vl_fp_fractal, other_julia_fp_per_pixel, vl_setup, standard_fractal,       //
        256                                                                        //
    },

    {
        FractalType::ESCHER,                                                       //
        "escher_julia",                                                            //
        {REAL_PARAM, IMAG_PARAM, "", ""},                                          //
        {0.32, 0.043, 0, 0},                                                       //
        HelpLabels::HT_ESCHER, HelpLabels::HF_ESCHER,                              //
        FractalFlags::NONE,                                                        //
        -1.6F, 1.6F, -1.2F, 1.2F,                                                  //
        0,                                                                         //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, //
        SymmetryType::ORIGIN,                                                      //
        escher_fp_fractal, julia_fp_per_pixel, standard_setup,                     //
        standard_fractal,                                                          //
        STD_BAILOUT                                                                //
    },

    // From Pickovers' "Chaos in Wonderland"
    // included by Humberto R. Baptista
    // code adapted from king.cpp bt James Rankin
    {
        FractalType::LATOO,                                                                             //
        "latoocarfian",                                                                                 //
        {"a", "b", "c", "d"},                                                                           //
        {-0.966918, 2.879879, 0.765145, 0.744728},                                                      //
        HelpLabels::HT_LATOO, HelpLabels::HF_LATOO,                                                     //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::INF_CALC | FractalFlags::TRIG4, //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                                       //
        0,                                                                                              //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,                      //
        SymmetryType::NONE,                                                                             //
        (VF) latoo_float_orbit, nullptr, orbit3d_float_setup, orbit2d_float,                            //
        NO_BAILOUT                                                                                      //
    },

    // Jim Muth formula
    {
        FractalType::DIVIDE_BROT5,                                                                //
        "dividebrot5",                                                                            //
        {"a", "b", "", ""},                                                                       //
        {2.0, 0.0, 0.0, 0.0},                                                                     //
        HelpLabels::HT_DIVIDE_BROT5, HelpLabels::HF_DIVIDE_BROT5,                                 //
        FractalFlags::BAIL_TEST | FractalFlags::BF_MATH,                                          //
        -2.5f, 1.5f, -1.5f, 1.5f,                                                                 //
        0,                                                                                        //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,                //
        SymmetryType::NONE,                                                                       //
        divide_brot5_fp_fractal, divide_brot5_fp_per_pixel, divide_brot5_setup, standard_fractal, //
        16                                                                                        //
    },

    // Jim Muth formula
    {
        FractalType::MANDELBROT_MIX4,                                                                      //
        "mandelbrotmix4",                                                                                  //
        {P1_REAL, P1_IMAG, P2_REAL, P2_IMAG},                                                              //
        {0.05, 3, -1.5, -2},                                                                               //
        HelpLabels::HT_MANDELBROT_MIX4, HelpLabels::HF_MANDELBROT_MIX4,                                    //
        FractalFlags::BAIL_TEST | FractalFlags::TRIG1 | FractalFlags::MORE,                                //
        -2.5F, 1.5F, -1.5F, 1.5F,                                                                          //
        0,                                                                                                 //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,                         //
        SymmetryType::NONE,                                                                                //
        mandelbrot_mix4_fp_fractal, mandelbrot_mix4_fp_per_pixel, mandelbrot_mix4_setup, standard_fractal, //
        STD_BAILOUT                                                                                        //
    },

    {
        FractalType::BURNING_SHIP,                                                          //
        "burning-ship",                                                                     //
        {P1_REAL, P1_IMAG, "degree (2-5)", ""},                                             //
        {0, 0, 2, 0},                                                                       //
        HelpLabels::HT_BURNING_SHIP, HelpLabels::HF_BURNING_SHIP,                           //
        FractalFlags::BAIL_TEST | FractalFlags::PERTURB | FractalFlags::BF_MATH,            //
        -2.5f, 1.5f, -1.2f, 1.8f,                                                           //
        0,                                                                                  //
        FractalType::BURNING_SHIP, FractalType::NO_FRACTAL, FractalType::BURNING_SHIP,      //
        SymmetryType::NONE,                                                                 //
        burning_ship_fp_fractal, other_mandel_fp_per_pixel, mandel_setup, standard_fractal, //
        STD_BAILOUT,                                                                        //
        burning_ship_ref_pt, burning_ship_ref_pt_bf, burning_ship_perturb                   //
    },

    // marks the END of the list
    {
        FractalType::NO_FRACTAL,                                                   //
        nullptr,                                                                   //
        {nullptr, nullptr, nullptr, nullptr},                                      //
        {0, 0, 0, 0},                                                              //
        HelpLabels::NONE, HelpLabels::NONE,                                        //
        FractalFlags::NONE,                                                        //
        0.0F, 0.0F, 0.0F, 0.0F,                                                    //
        0,                                                                         //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, FractalType::NO_FRACTAL, //
        SymmetryType::NONE,                                                        //
        nullptr, nullptr, nullptr, nullptr,                                        //
        0,                                                                         //
    } //
};

int g_num_fractal_types = sizeof(g_fractal_specific) / sizeof(FractalSpecific) - 1;
