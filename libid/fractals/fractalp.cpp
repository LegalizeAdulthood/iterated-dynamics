// SPDX-License-Identifier: GPL-3.0-only
//
// This module consists only of the fractalspecific structure
//
#include "fractals/fractalp.h"

#include "engine/fractalb.h"
#include "engine/fractals.h"
#include "fractals/barnsley.h"
#include "fractals/bif_may.h"
#include "fractals/Bifurcation.h"
#include "fractals/burning_ship.h"
#include "fractals/Cellular.h"
#include "fractals/circle_pattern.h"
#include "fractals/divide_brot.h"
#include "fractals/escher.h"
#include "fractals/fn_or_fn.h"
#include "fractals/frasetup.h"
#include "fractals/FrothyBasin.h"
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
#include "fractals/popcorn.h"
#include "fractals/quartic_mandelbrot.h"
#include "fractals/quaternion_mandelbrot.h"
#include "fractals/sierpinski_gasket.h"
#include "fractals/taylor_skinner_variations.h"
#include "fractals/unity.h"
#include "fractals/volterra_lotka.h"
#include "ui/ant.h"
#include "ui/bifurcation.h"
#include "ui/cellular.h"
#include "ui/diffusion.h"
#include "ui/dynamic2d.h"
#include "ui/frothy_basin.h"
#include "ui/inverse_julia.h"
#include "ui/orbit2d.h"
#include "ui/plasma.h"
#include "ui/standard_4d.h"
#include "ui/testpt.h"

#include <algorithm>
#include <array>
#include <iterator>
#include <stdexcept>
#include <string>

using namespace id;
using namespace id::fractals;

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
    {FractalType::HYPER_CMPLX_J,        {"zj", "zk", "", "", "", ""}, {0, 0, 0, 0, 0, 0}},
    {FractalType::QUAT_JUL,             {"zj", "zk", "", "", "", ""}, {0, 0, 0, 0, 0, 0}},
    {FractalType::PHOENIX_CPLX,         {DEGREE_Z, "", "", "", "", ""}, {0, 0, 0, 0, 0, 0}},
    {FractalType::MAND_PHOENIX_CPLX,    {DEGREE_Z, "", "", "", "", ""}, {0, 0, 0, 0, 0, 0}},
    {FractalType::FORMULA,              {P3_REAL, P3_IMAG, P4_REAL, P4_IMAG, P5_REAL, P5_IMAG}, {0, 0, 0, 0, 0, 0}},
    {FractalType::ANT,                  {"+Wrap?", RANDOM_SEED, "", "", "", ""}, {1, 0, 0, 0, 0, 0}},
    {FractalType::MANDELBROT_MIX4,      {P3_REAL, P3_IMAG, "", "", "", ""}, {0, 0, 0, 0, 0, 0}},
    {FractalType::NO_FRACTAL,           {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}, {0, 0, 0, 0, 0, 0}}
};
// clang-format on

//  type math orbitcalc fnct per_pixel fnct per_image fnct
// clang-format off
AlternateMath g_alternate_math[] =
{
#define USE_BN
#ifdef USE_BN
    {FractalType::JULIA, BFMathType::BIG_NUM, julia_orbit_bn, julia_per_pixel_bn,  mandel_per_image_bn},
    {FractalType::MANDEL, BFMathType::BIG_NUM, julia_orbit_bn, mandel_per_pixel_bn, mandel_per_image_bn},
#else
    {FractalType::JULIA, BFMathType::BIG_FLT, julia_orbit_bf, julia_per_pixel_bf,  mandel_per_image_bf},
    {FractalType::MANDEL, BFMathType::BIG_FLT, julia_orbit_bf, mandel_bf_per_pixel, mandel_per_image_bf},
#endif
    /*
    NOTE: The default precision for g_bf_math=BIGNUM is not high enough
          for JuliaZpowerbnFractal.  If you want to test BIGNUM (1) instead
          of the usual BIGFLT (2), then set bfdigits on the command to
          increase the precision.
    */
    {FractalType::JULIA_Z_POWER, BFMathType::BIG_FLT, julia_z_power_orbit_bf, julia_per_pixel_bf, mandel_per_image_bf},
    {FractalType::MANDEL_Z_POWER, BFMathType::BIG_FLT, julia_z_power_orbit_bf, mandel_per_pixel_bf, mandel_per_image_bf},
    {FractalType::DIVIDE_BROT5, BFMathType::BIG_FLT, divide_brot5_orbit_bf, divide_brot5_bf_per_pixel, mandel_per_image_bf},
    {FractalType::BURNING_SHIP, BFMathType::BIG_FLT, burning_ship_bf_fractal, mandel_per_pixel_bf, mandel_per_image_bf},
    {FractalType::NO_FRACTAL, BFMathType::NONE, nullptr, nullptr, nullptr}
};
// clang-format on

// use next to cast orbitcalcs() that have arguments
using VF = int (*)();

// This array is indexed by the enum fractal_type.
FractalSpecific g_fractal_specific[] = {
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
        orbit fn, per_pixel fn, per_image fn,
        calc type fn,
        bailout,
        perturbation ref pt, perturbation bf ref pt, perturbation point
    }
    */

    {
        FractalType::NEWT_BASIN,                                                       //
        "newtbasin",                                                                   //
        {NEWT_DEGREE, "Enter non-zero value for stripes", "", ""},                     //
        {3, 0, 0, 0},                                                                  //
        id::help::HelpLabels::HT_NEWTON_BASINS, id::help::HelpLabels::HF_NEWTON_BASIN, //
        FractalFlags::NONE,                                                            //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                      //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,                              //
        SymmetryType::NONE,                                                            //
        newton_orbit, other_julia_per_pixel, newton_per_image,                         //
        standard_fractal_type,                                                         //
        NO_BAILOUT                                                                     //
    },

    {
        FractalType::MANDEL,                                                     //
        "mandel",                                                                //
        {REAL_Z0, IMAG_Z0, "", ""},                                              //
        {0, 0, 0, 0},                                                            //
        id::help::HelpLabels::HT_MANDEL, id::help::HelpLabels::HF_MANDEL,        //
        FractalFlags::BAIL_TEST | FractalFlags::BF_MATH | FractalFlags::PERTURB, //
        -2.5F, 1.5F, -1.5F, 1.5F,                                                //
        FractalType::JULIA, FractalType::NO_FRACTAL,                             //
        SymmetryType::X_AXIS_NO_PARAM,                                           //
        julia_orbit, mandel_per_pixel, mandel_per_image,                         //
        standard_fractal_type,                                                   //
        STD_BAILOUT,                                                             //
        mandel_ref_pt, mandel_ref_pt_bf, mandel_perturb                          //
    },

    {
        FractalType::NEWTON,                                              //
        "newton",                                                         //
        {NEWT_DEGREE, "", "", ""},                                        //
        {3, 0, 0, 0},                                                     //
        id::help::HelpLabels::HT_NEWTON, id::help::HelpLabels::HF_NEWTON, //
        FractalFlags::NONE,                                               //
        -2.0F, 2.0F, -1.5F, 1.5F,                                         //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,                 //
        SymmetryType::X_AXIS,                                             //
        newton_orbit, other_julia_per_pixel, newton_per_image,            //
        standard_fractal_type,                                            //
        NO_BAILOUT                                                        //
    },

    {
        FractalType::JULIA,                                                    //
        "julia",                                                               //
        {REAL_PARAM, IMAG_PARAM, "", ""},                                      //
        {0.3, 0.6, 0, 0},                                                      //
        id::help::HelpLabels::HT_JULIA, id::help::HelpLabels::HF_JULIA,        //
        FractalFlags::OK_JB | FractalFlags::BAIL_TEST | FractalFlags::BF_MATH, //
        -2.0F, 2.0F, -1.5F, 1.5F,                                              //
        FractalType::NO_FRACTAL, FractalType::MANDEL,                          //
        SymmetryType::ORIGIN,                                                  //
        julia_orbit, julia_per_pixel, julia_per_image,                         //
        standard_fractal_type,                                                 //
        STD_BAILOUT                                                            //
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
        id::help::HelpLabels::HT_PLASMA, id::help::HelpLabels::HF_PLASMA,                                  //
        FractalFlags::NO_ZOOM | FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::NO_RESUME, //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                                          //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,                                                  //
        SymmetryType::NONE,                                                                                //
        nullptr, nullptr, standalone_per_image,                                                            //
        plasma_type,                                                                                       //
        NO_BAILOUT                                                                                         //
    },

    {
        FractalType::MANDEL_FN,                                                 //
        "mandelfn",                                                             //
        {REAL_Z0, IMAG_Z0, "", ""},                                             //
        {0, 0, 0, 0},                                                           //
        id::help::HelpLabels::HT_MANDEL_FN, id::help::HelpLabels::HF_MANDEL_FN, //
        FractalFlags::TRIG1,                                                    //
        -8.0F, 8.0F, -6.0F, 6.0F,                                               //
        FractalType::LAMBDA_FN, FractalType::NO_FRACTAL,                        //
        SymmetryType::XY_AXIS_NO_PARAM,                                         //
        lambda_trig_orbit, other_mandel_per_pixel, mandel_trig_per_image,       //
        standard_fractal_type,                                                  //
        TRIG_BAILOUT_L                                                          //
    },

    {
        FractalType::MAN_O_WAR,                                                                 //
        "manowar",                                                                              //
        {REAL_Z0, IMAG_Z0, "", ""},                                                             //
        {0, 0, 0, 0},                                                                           //
        id::help::HelpLabels::HT_TAYLOR_SKINNER_VARIATIONS, id::help::HelpLabels::HF_MAN_O_WAR, //
        FractalFlags::BAIL_TEST,                                                                //
        -2.5F, 1.5F, -1.5F, 1.5F,                                                               //
        FractalType::MAN_O_WAR_J, FractalType::NO_FRACTAL,                                      //
        SymmetryType::X_AXIS_NO_PARAM,                                                          //
        man_o_war_orbit, mandel_per_pixel, mandel_per_image,                                    //
        standard_fractal_type,                                                                  //
        STD_BAILOUT                                                                             //
    },

    {
        FractalType::TEST,                                            //
        "test",                                                       //
        {
            "(testpt Param #1)",                                      //
            "(testpt param #2)",                                      //
            "(testpt param #3)",                                      //
            "(testpt param #4)"                                       //
        },
        {0, 0, 0, 0},                                                 //
        id::help::HelpLabels::HT_TEST, id::help::HelpLabels::HF_TEST, //
        FractalFlags::NONE,                                           //
        -2.0F, 2.0F, -1.5F, 1.5F,                                     //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,             //
        SymmetryType::NONE,                                           //
        nullptr, nullptr, standalone_per_image,                       //
        test_type,                                                    //
        STD_BAILOUT                                                   //
    },

    {
        FractalType::SQR_FN,                                                                 //
        "sqr(fn)",                                                                           //
        {"", "", "", ""},                                                                    //
        {0, 0, 0, 0},                                                                        //
        id::help::HelpLabels::HT_TAYLOR_SKINNER_VARIATIONS, id::help::HelpLabels::HF_SQR_FN, //
        FractalFlags::TRIG1 | FractalFlags::BAIL_TEST,                                       //
        -4.0F, 4.0F, -3.0F, 3.0F,                                                            //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,                                    //
        SymmetryType::X_AXIS,                                                                //
        sqr_trig_orbit, other_julia_per_pixel, sqr_trig_per_image,                           //
        standard_fractal_type,                                                               //
        TRIG_BAILOUT_L                                                                       //
    },

    {
        FractalType::IFS,                                                                                   //
        "ifs",                                                                                              //
        {COLOR_METHOD, "", "", ""},                                                                         //
        {0, 0, 0, 0},                                                                                       //
        id::help::HelpLabels::HT_IFS, id::help::HelpLabels::SPECIAL_IFS,                                    //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::NO_RESUME | FractalFlags::INF_CALC, //
        -8.0F, 8.0F, -1.0F, 11.0F,                                                                          //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,                                                   //
        SymmetryType::NONE,                                                                                 //
        nullptr, nullptr, standalone_per_image,                                                             //
        ifs_type,                                                                                           //
        NO_BAILOUT                                                                                          //
    },

    {
        FractalType::IFS_3D,                                             //
        "ifs3d",                                                         //
        {COLOR_METHOD, "", "", ""},                                      //
        {0, 0, 0, 0},                                                    //
        id::help::HelpLabels::HT_IFS, id::help::HelpLabels::SPECIAL_IFS, //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::NO_RESUME | FractalFlags::PARAMS_3D |
            FractalFlags::INF_CALC,                                      //
        -11.0F, 11.0F, -11.0F, 11.0F,                                    //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,                //
        SymmetryType::NONE,                                              //
        nullptr, nullptr, standalone_per_image,                          //
        ifs_type,                                                        //
        NO_BAILOUT                                                       //
    },

    {
        FractalType::FN_Z_SQR,                                                                 //
        "fn(z*z)",                                                                             //
        {"", "", "", ""},                                                                      //
        {0, 0, 0, 0},                                                                          //
        id::help::HelpLabels::HT_TAYLOR_SKINNER_VARIATIONS, id::help::HelpLabels::HF_FN_Z_SQR, //
        FractalFlags::TRIG1 | FractalFlags::BAIL_TEST,                                         //
        -4.0F, 4.0F, -3.0F, 3.0F,                                                              //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,                                      //
        SymmetryType::XY_AXIS,                                                                 //
        trig_z_sqrd_orbit, julia_per_pixel, julia_per_image,                                   //
        standard_fractal_type,                                                                 //
        STD_BAILOUT                                                                            //
    },

    {
        FractalType::BIFURCATION,                                                                        //
        "bifurcation",                                                                                   //
        {FILT, SEED_POP, "", ""},                                                                        //
        {1000.0, 0.66, 0, 0},                                                                            //
        id::help::HelpLabels::HT_BIFURCATION, id::help::HelpLabels::HF_BIFURCATION,                      //
        FractalFlags::TRIG1 | FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::NO_ROTATE, //
        1.9F, 3.0F, 0.0F, 1.34F,                                                                         //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,                                                //
        SymmetryType::NONE,                                                                              //
        bifurc_verhulst_trig_orbit, nullptr, standalone_per_image,                                       //
        bifurcation_type,                                                                                //
        NO_BAILOUT                                                                                       //
    },

    {
        FractalType::FN_PLUS_FN,                                                                 //
        "fn+fn",                                                                                 //
        {RE_COEF_TRIG1, IM_COEF_TRIG1, RE_COEF_TRIG2, IM_COEF_TRIG2},                            //
        {1, 0, 1, 0},                                                                            //
        id::help::HelpLabels::HT_TAYLOR_SKINNER_VARIATIONS, id::help::HelpLabels::HF_FN_PLUS_FN, //
        FractalFlags::TRIG2 | FractalFlags::BAIL_TEST,                                           //
        -4.0F, 4.0F, -3.0F, 3.0F,                                                                //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,                                        //
        SymmetryType::X_AXIS,                                                                    //
        trig_plus_trig_orbit, other_julia_per_pixel, trig_plus_trig_per_image,                   //
        standard_fractal_type,                                                                   //
        TRIG_BAILOUT_L                                                                           //
    },

    {
        FractalType::FN_TIMES_FN,                                                                 //
        "fn*fn",                                                                                  //
        {"", "", "", ""},                                                                         //
        {0, 0, 0, 0},                                                                             //
        id::help::HelpLabels::HT_TAYLOR_SKINNER_VARIATIONS, id::help::HelpLabels::HF_FN_TIMES_FN, //
        FractalFlags::TRIG2 | FractalFlags::BAIL_TEST,                                            //
        -4.0F, 4.0F, -3.0F, 3.0F,                                                                 //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,                                         //
        SymmetryType::X_AXIS,                                                                     //
        trig_x_trig_orbit, other_julia_per_pixel, fn_x_fn_per_image,                              //
        standard_fractal_type,                                                                    //
        TRIG_BAILOUT_L                                                                            //
    },

    {
        FractalType::SQR_1_OVER_FN,                                                                 //
        "sqr(1/fn)",                                                                                //
        {"", "", "", ""},                                                                           //
        {0, 0, 0, 0},                                                                               //
        id::help::HelpLabels::HT_TAYLOR_SKINNER_VARIATIONS, id::help::HelpLabels::HF_SQR_1_OVER_FN, //
        FractalFlags::TRIG1 | FractalFlags::BAIL_TEST,                                              //
        -4.0F, 4.0F, -3.0F, 3.0F,                                                                   //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,                                           //
        SymmetryType::NONE,                                                                         //
        sqr_1_over_trig_orbit, other_julia_per_pixel, sqr_trig_per_image,                           //
        standard_fractal_type,                                                                      //
        TRIG_BAILOUT_L                                                                              //
    },

    {
        FractalType::FN_MUL_Z_PLUS_Z,                                                                   //
        "fn*z+z",                                                                                       //
        {RE_COEF_TRIG1, IM_COEF_TRIG1, "Real Coefficient Second Term", "Imag Coefficient Second Term"}, //
        {1, 0, 1, 0},                                                                                   //
        id::help::HelpLabels::HT_TAYLOR_SKINNER_VARIATIONS, id::help::HelpLabels::HF_FN_MUL_Z_PLUS_Z,   //
        FractalFlags::TRIG1 | FractalFlags::BAIL_TEST,                                                  //
        -4.0F, 4.0F, -3.0F, 3.0F,                                                                       //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,                                               //
        SymmetryType::X_AXIS,                                                                           //
        z_x_trig_plus_z_orbit, julia_per_pixel, z_x_trig_plus_z_per_image,                              //
        standard_fractal_type,                                                                          //
        TRIG_BAILOUT_L                                                                                  //
    },

    {
        FractalType::KAM,                                           //
        "kamtorus",                                                 //
        {KAM_ANGLE, KAM_STEP, KAM_STOP, POINTS_PER_ORBIT},          //
        {1.3, .05, 1.5, 150},                                       //
        id::help::HelpLabels::HT_KAM, id::help::HelpLabels::HF_KAM, //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE,            //
        -1.0F, 1.0F, -.75F, .75F,                                   //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,           //
        SymmetryType::NONE,                                         //
        (VF) kam_torus_orbit, nullptr, orbit3d_per_image,           //
        orbit2d_type,                                               //
        NO_BAILOUT                                                  //
    },

    {
        FractalType::KAM_3D,                                        //
        "kamtorus3d",                                               //
        {KAM_ANGLE, KAM_STEP, KAM_STOP, POINTS_PER_ORBIT},          //
        {1.3, .05, 1.5, 150},                                       //
        id::help::HelpLabels::HT_KAM, id::help::HelpLabels::HF_KAM, //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::NO_RESUME |
            FractalFlags::PARAMS_3D,                                //
        -3.0F, 3.0F, -1.0F, 3.5F,                                   //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,           //
        SymmetryType::NONE,                                         //
        (VF) kam_torus_orbit, nullptr, orbit3d_per_image,           //
        orbit3d_type,                                               //
        NO_BAILOUT                                                  //
    },

    {
        FractalType::MANDEL_FN_PLUS_Z_SQRD,                                                             //
        "manfn+zsqrd",                                                                                  //
        {REAL_Z0, IMAG_Z0, "", ""},                                                                     //
        {0, 0, 0, 0},                                                                                   //
        id::help::HelpLabels::HT_PICKOVER_MANDEL_JULIA, id::help::HelpLabels::HF_MANDEL_FN_PLUS_Z_SQRD, //
        FractalFlags::TRIG1 | FractalFlags::BAIL_TEST,                                                  //
        -2.5F, 1.5F, -1.5F, 1.5F,                                                                       //
        FractalType::JULIA_FN_PLUS_Z_SQRD, FractalType::NO_FRACTAL,                                     //
        SymmetryType::X_AXIS_NO_PARAM,                                                                  //
        trig_plus_z_squared_orbit, mandel_per_pixel, mandel_per_image,                                  //
        standard_fractal_type,                                                                          //
        STD_BAILOUT                                                                                     //
    },

    {
        FractalType::JULIA_FN_PLUS_Z_SQRD,                                                             //
        "julfn+zsqrd",                                                                                 //
        {REAL_PARAM, IMAG_PARAM, "", ""},                                                              //
        {-0.5, 0.5, 0, 0},                                                                             //
        id::help::HelpLabels::HT_PICKOVER_MANDEL_JULIA, id::help::HelpLabels::HF_JULIA_FN_PLUS_Z_SQRD, //
        FractalFlags::TRIG1 | FractalFlags::OK_JB | FractalFlags::BAIL_TEST,                           //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                                      //
        FractalType::NO_FRACTAL, FractalType::MANDEL_FN_PLUS_Z_SQRD,                                   //
        SymmetryType::NONE,                                                                            //
        trig_plus_z_squared_orbit, julia_per_pixel, julia_fn_plus_z_sqrd_per_image,                    //
        standard_fractal_type,                                                                         //
        STD_BAILOUT                                                                                    //
    },

    {
        FractalType::LAMBDA_FN,                                                 //
        "lambdafn",                                                             //
        {REAL_PARAM, IMAG_PARAM, "", ""},                                       //
        {1.0, 0.4, 0, 0},                                                       //
        id::help::HelpLabels::HT_LAMBDA_FN, id::help::HelpLabels::HF_LAMBDA_FN, //
        FractalFlags::TRIG1 | FractalFlags::OK_JB,                              //
        -4.0F, 4.0F, -3.0F, 3.0F,                                               //
        FractalType::NO_FRACTAL, FractalType::MANDEL_FN,                        //
        SymmetryType::PI_SYM,                                                   //
        lambda_trig_orbit, other_julia_per_pixel, lambda_trig_per_image,        //
        standard_fractal_type,                                                  //
        TRIG_BAILOUT_L                                                          //
    },

    {
        FractalType::MANDEL_Z_POWER,                                                             //
        "manzpower",                                                                             //
        {REAL_Z0, IMAG_Z0, RE_EXPONENT, IM_EXPONENT},                                            //
        {0, 0, 2, 0},                                                                            //
        id::help::HelpLabels::HT_PICKOVER_MANDEL_JULIA, id::help::HelpLabels::HF_MANDEL_Z_POWER, //
        FractalFlags::BAIL_TEST | FractalFlags::BF_MATH | FractalFlags::PERTURB,                 //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                                //
        FractalType::JULIA_Z_POWER, FractalType::NO_FRACTAL,                                     //
        SymmetryType::X_AXIS_NO_IMAG,                                                            //
        mandel_z_power_orbit, other_mandel_per_pixel, mandel_per_image,                          //
        standard_fractal_type,                                                                   //
        STD_BAILOUT,                                                                             //
        mandel_z_power_ref_pt, mandel_z_power_ref_pt_bf, mandel_z_power_perturb                  //
    },

    {
        FractalType::JULIA_Z_POWER,                                                             //
        "julzpower",                                                                            //
        {REAL_PARAM, IMAG_PARAM, RE_EXPONENT, IM_EXPONENT},                                     //
        {0.3, 0.6, 2, 0},                                                                       //
        id::help::HelpLabels::HT_PICKOVER_MANDEL_JULIA, id::help::HelpLabels::HF_JULIA_Z_POWER, //
        FractalFlags::OK_JB | FractalFlags::BAIL_TEST | FractalFlags::BF_MATH,                  //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                               //
        FractalType::NO_FRACTAL, FractalType::MANDEL_Z_POWER,                                   //
        SymmetryType::ORIGIN,                                                                   //
        mandel_z_power_orbit, other_julia_per_pixel, julia_per_image,                           //
        standard_fractal_type,                                                                  //
        STD_BAILOUT                                                                             //
    },

    {
        FractalType::MAN_Z_TO_Z_PLUS_Z_PWR,                                                        //
        "manzzpwr",                                                                                //
        {REAL_Z0, IMAG_Z0, RE_EXPONENT, ""},                                                       //
        {0, 0, 2, 0},                                                                              //
        id::help::HelpLabels::HT_PICKOVER_MANDEL_JULIA, id::help::HelpLabels::HF_MANDEL_Z_Z_POWER, //
        FractalFlags::BAIL_TEST | FractalFlags::BF_MATH,                                           //
        -2.5F, 1.5F, -1.5F, 1.5F,                                                                  //
        FractalType::JUL_Z_TO_Z_PLUS_Z_PWR, FractalType::NO_FRACTAL,                               //
        SymmetryType::X_AXIS_NO_PARAM,                                                             //
        mandel_z_to_z_plus_z_pwr_orbit, other_mandel_per_pixel, mandel_per_image,                  //
        standard_fractal_type,                                                                     //
        STD_BAILOUT                                                                                //
    },

    {
        FractalType::JUL_Z_TO_Z_PLUS_Z_PWR,                                                     //
        "julzzpwr",                                                                             //
        {REAL_PARAM, IMAG_PARAM, RE_EXPONENT, ""},                                              //
        {-0.3, 0.3, 2, 0},                                                                      //
        id::help::HelpLabels::HT_PICKOVER_MANDEL_JULIA, id::help::HelpLabels::HF_JULIA_Z_Z_PWR, //
        FractalFlags::OK_JB | FractalFlags::BAIL_TEST,                                          //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                               //
        FractalType::NO_FRACTAL, FractalType::MAN_Z_TO_Z_PLUS_Z_PWR,                            //
        SymmetryType::NONE,                                                                     //
        mandel_z_to_z_plus_z_pwr_orbit, other_julia_per_pixel, julia_per_image,                 //
        standard_fractal_type,                                                                  //
        STD_BAILOUT                                                                             //
    },

    {
        FractalType::MANDEL_FN_PLUS_EXP,                                                             //
        "manfn+exp",                                                                                 //
        {REAL_Z0, IMAG_Z0, "", ""},                                                                  //
        {0, 0, 0, 0},                                                                                //
        id::help::HelpLabels::HT_PICKOVER_MANDEL_JULIA, id::help::HelpLabels::HF_MANDEL_FN_PLUS_EXP, //
        FractalFlags::TRIG1 | FractalFlags::BAIL_TEST,                                               //
        -8.0F, 8.0F, -6.0F, 6.0F,                                                                    //
        FractalType::JULIA_FN_PLUS_EXP, FractalType::NO_FRACTAL,                                     //
        SymmetryType::X_AXIS_NO_PARAM,                                                               //
        mandel_trig_plus_exponent_orbit, other_mandel_per_pixel, mandel_per_image,                   //
        standard_fractal_type,                                                                       //
        STD_BAILOUT                                                                                  //
    },

    {
        FractalType::JULIA_FN_PLUS_EXP,                                                             //
        "julfn+exp",                                                                                //
        {REAL_PARAM, IMAG_PARAM, "", ""},                                                           //
        {0, 0, 0, 0},                                                                               //
        id::help::HelpLabels::HT_PICKOVER_MANDEL_JULIA, id::help::HelpLabels::HF_JULIA_FN_PLUS_EXP, //
        FractalFlags::TRIG1 | FractalFlags::OK_JB | FractalFlags::BAIL_TEST,                        //
        -4.0F, 4.0F, -3.0F, 3.0F,                                                                   //
        FractalType::NO_FRACTAL, FractalType::MANDEL_FN_PLUS_EXP,                                   //
        SymmetryType::NONE,                                                                         //
        mandel_trig_plus_exponent_orbit, other_julia_per_pixel, julia_per_image,                    //
        standard_fractal_type,                                                                      //
        STD_BAILOUT                                                                                 //
    },

    {
        FractalType::POPCORN,                                                  //
        "popcorn",                                                             //
        {STEP_X, STEP_Y, CONSTANT_X, CONSTANT_Y},                              //
        {0.05, 0, 3.00, 0},                                                    //
        id::help::HelpLabels::HT_POPCORN, id::help::HelpLabels::HF_POPCORN,    //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::TRIG4, //
        -3.0F, 3.0F, -2.25F, 2.25F,                                            //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,                      //
        SymmetryType::NO_PLOT,                                                 //
        popcorn_orbit, other_julia_per_pixel, julia_per_image,                 //
        popcorn_type,                                                          //
        STD_BAILOUT                                                            //
    },

    {
        FractalType::LORENZ,                                                      //
        "lorenz",                                                                 //
        {TIME_STEP, "a", "b", "c"},                                               //
        {.02, 5, 15, 1},                                                          //
        id::help::HelpLabels::HT_LORENZ, id::help::HelpLabels::HF_LORENZ,         //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::INF_CALC, //
        -15.0F, 15.0F, 0.0F, 30.0F,                                               //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,                         //
        SymmetryType::NONE,                                                       //
        (VF) lorenz3d_orbit, nullptr, orbit3d_per_image,                          //
        orbit2d_type,                                                             //
        NO_BAILOUT                                                                //
    },

    {
        FractalType::COMPLEX_NEWTON,                                                              //
        "complexnewton",                                                                          //
        {"Real part of Degree", "Imag part of Degree", "Real part of Root", "Imag part of Root"}, //
        {3, 0, 1, 0},                                                                             //
        id::help::HelpLabels::HT_NEWTON_COMPLEX, id::help::HelpLabels::HF_COMPLEX_NEWTON,         //
        FractalFlags::NONE,                                                                       //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                                 //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,                                         //
        SymmetryType::NONE,                                                                       //
        complex_newton_orbit, other_julia_per_pixel, complex_newton_per_image,                    //
        standard_fractal_type,                                                                    //
        NO_BAILOUT                                                                                //
    },

    {
        FractalType::COMPLEX_BASIN,                                                               //
        "complexbasin",                                                                           //
        {"Real part of Degree", "Imag part of Degree", "Real part of Root", "Imag part of Root"}, //
        {3, 0, 1, 0},                                                                             //
        id::help::HelpLabels::HT_NEWTON_COMPLEX, id::help::HelpLabels::HF_COMPLEX_NEWTON,         //
        FractalFlags::NONE,                                                                       //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                                 //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,                                         //
        SymmetryType::NONE,                                                                       //
        complex_basin_orbit, other_julia_per_pixel, complex_newton_per_image,                     //
        standard_fractal_type,                                                                    //
        NO_BAILOUT                                                                                //
    },

    {
        FractalType::COMPLEX_MARKS_MAND,                                                           //
        "cmplxmarksmand",                                                                          //
        {REAL_Z0, IMAG_Z0, RE_EXPONENT, IM_EXPONENT},                                              //
        {0, 0, 1, 0},                                                                              //
        id::help::HelpLabels::HT_PETERSON_VARIATIONS, id::help::HelpLabels::HF_COMPLEX_MARKS_MAND, //
        FractalFlags::BAIL_TEST,                                                                   //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                                  //
        FractalType::COMPLEX_MARKS_JUL, FractalType::NO_FRACTAL,                                   //
        SymmetryType::NONE,                                                                        //
        marks_cplx_mand_orbit, marks_cplx_mand_per_pixel, mandel_per_image,                        //
        standard_fractal_type,                                                                     //
        STD_BAILOUT                                                                                //
    },

    {
        FractalType::COMPLEX_MARKS_JUL,                                                           //
        "cmplxmarksjul",                                                                          //
        {REAL_PARAM, IMAG_PARAM, RE_EXPONENT, IM_EXPONENT},                                       //
        {0.3, 0.6, 1, 0},                                                                         //
        id::help::HelpLabels::HT_PETERSON_VARIATIONS, id::help::HelpLabels::HF_COMPLEX_MARKS_JUL, //
        FractalFlags::BAIL_TEST,                                                                  //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                                 //
        FractalType::NO_FRACTAL, FractalType::COMPLEX_MARKS_MAND,                                 //
        SymmetryType::NONE,                                                                       //
        marks_cplx_mand_orbit, julia_per_pixel, julia_per_image,                                  //
        standard_fractal_type,                                                                    //
        STD_BAILOUT                                                                               //
    },

    {
        FractalType::FORMULA,                                                    //
        "formula",                                                               //
        {P1_REAL, P1_IMAG, P2_REAL, P2_IMAG},                                    //
        {0, 0, 0, 0},                                                            //
        id::help::HelpLabels::HT_FORMULA, id::help::HelpLabels::SPECIAL_FORMULA, //
        FractalFlags::MORE,                                                      //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,                        //
        SymmetryType::SETUP,                                                     //
        formula_orbit, formula_per_pixel, formula_per_image,                     //
        standard_fractal_type,                                                   //
        0                                                                        //
    },

    {
        FractalType::SIERPINSKI,                                                  //
        "sierpinski",                                                             //
        {"", "", "", ""},                                                         //
        {0, 0, 0, 0},                                                             //
        id::help::HelpLabels::HT_SIERPINSKI, id::help::HelpLabels::HF_SIERPINSKI, //
        FractalFlags::NONE,                                                       //
        -4.0F / 3.0F, 96.0F / 45.0F, -0.9F, 1.7F,                                 //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,                         //
        SymmetryType::NONE,                                                       //
        sierpinski_orbit, other_julia_per_pixel, sierpinski_per_image,            //
        standard_fractal_type,                                                    //
        127                                                                       //
    },

    {
        FractalType::LAMBDA,                                              //
        "lambda",                                                         //
        {REAL_PARAM, IMAG_PARAM, "", ""},                                 //
        {0.85, 0.6, 0, 0},                                                //
        id::help::HelpLabels::HT_LAMBDA, id::help::HelpLabels::HF_LAMBDA, //
        FractalFlags::OK_JB | FractalFlags::BAIL_TEST,                    //
        -1.5F, 2.5F, -1.5F, 1.5F,                                         //
        FractalType::NO_FRACTAL, FractalType::MANDEL_LAMBDA,              //
        SymmetryType::NONE,                                               //
        lambda_orbit, julia_per_pixel, julia_per_image,                   //
        standard_fractal_type,                                            //
        STD_BAILOUT                                                       //
    },

    {
        FractalType::BARNSLEY_M1,                                                //
        "barnsleym1",                                                            //
        {REAL_Z0, IMAG_Z0, "", ""},                                              //
        {0, 0, 0, 0},                                                            //
        id::help::HelpLabels::HT_BARNSLEY, id::help::HelpLabels::HF_BARNSLEY_M1, //
        FractalFlags::BAIL_TEST,                                                 //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                //
        FractalType::BARNSLEY_J1, FractalType::NO_FRACTAL,                       //
        SymmetryType::XY_AXIS_NO_PARAM,                                          //
        barnsley1_orbit, other_mandel_per_pixel, mandel_per_image,               //
        standard_fractal_type,                                                   //
        STD_BAILOUT                                                              //
    },

    {
        FractalType::BARNSLEY_J1,                                                //
        "barnsleyj1",                                                            //
        {REAL_PARAM, IMAG_PARAM, "", ""},                                        //
        {0.6, 1.1, 0, 0},                                                        //
        id::help::HelpLabels::HT_BARNSLEY, id::help::HelpLabels::HF_BARNSLEY_J1, //
        FractalFlags::OK_JB | FractalFlags::BAIL_TEST,                           //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                //
        FractalType::NO_FRACTAL, FractalType::BARNSLEY_M1,                       //
        SymmetryType::ORIGIN,                                                    //
        barnsley1_orbit, other_julia_per_pixel, julia_per_image,                 //
        standard_fractal_type,                                                   //
        STD_BAILOUT                                                              //
    },

    {
        FractalType::BARNSLEY_M2,                                                //
        "barnsleym2",                                                            //
        {REAL_Z0, IMAG_Z0, "", ""},                                              //
        {0, 0, 0, 0},                                                            //
        id::help::HelpLabels::HT_BARNSLEY, id::help::HelpLabels::HF_BARNSLEY_M2, //
        FractalFlags::BAIL_TEST,                                                 //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                //
        FractalType::BARNSLEY_J2, FractalType::NO_FRACTAL,                       //
        SymmetryType::Y_AXIS_NO_PARAM,                                           //
        barnsley2_orbit, other_mandel_per_pixel, mandel_per_image,               //
        standard_fractal_type,                                                   //
        STD_BAILOUT                                                              //
    },

    {
        FractalType::BARNSLEY_J2,                                                //
        "barnsleyj2",                                                            //
        {REAL_PARAM, IMAG_PARAM, "", ""},                                        //
        {0.6, 1.1, 0, 0},                                                        //
        id::help::HelpLabels::HT_BARNSLEY, id::help::HelpLabels::HF_BARNSLEY_J2, //
        FractalFlags::OK_JB | FractalFlags::BAIL_TEST,                           //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                //
        FractalType::NO_FRACTAL, FractalType::BARNSLEY_M2,                       //
        SymmetryType::ORIGIN,                                                    //
        barnsley2_orbit, other_julia_per_pixel, julia_per_image,                 //
        standard_fractal_type,                                                   //
        STD_BAILOUT                                                              //
    },

    {
        FractalType::BARNSLEY_M3,                                                //
        "barnsleym3",                                                            //
        {REAL_Z0, IMAG_Z0, "", ""},                                              //
        {0, 0, 0, 0},                                                            //
        id::help::HelpLabels::HT_BARNSLEY, id::help::HelpLabels::HF_BARNSLEY_M3, //
        FractalFlags::BAIL_TEST,                                                 //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                //
        FractalType::BARNSLEY_J3, FractalType::NO_FRACTAL,                       //
        SymmetryType::X_AXIS_NO_PARAM,                                           //
        barnsley3_orbit, other_mandel_per_pixel, mandel_per_image,               //
        standard_fractal_type,                                                   //
        STD_BAILOUT                                                              //
    },

    {
        FractalType::BARNSLEY_J3,                                                //
        "barnsleyj3",                                                            //
        {REAL_PARAM, IMAG_PARAM, "", ""},                                        //
        {0.1, 0.36, 0, 0},                                                       //
        id::help::HelpLabels::HT_BARNSLEY, id::help::HelpLabels::HF_BARNSLEY_J3, //
        FractalFlags::OK_JB | FractalFlags::BAIL_TEST,                           //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                //
        FractalType::NO_FRACTAL, FractalType::BARNSLEY_M3,                       //
        SymmetryType::NONE,                                                      //
        barnsley3_orbit, other_julia_per_pixel, julia_per_image,                 //
        standard_fractal_type,                                                   //
        STD_BAILOUT                                                              //
    },

    {
        FractalType::MANDEL_LAMBDA,                                                     //
        "mandellambda",                                                                 //
        {REAL_Z0, IMAG_Z0, "", ""},                                                     //
        {0, 0, 0, 0},                                                                   //
        id::help::HelpLabels::HT_MANDEL_LAMBDA, id::help::HelpLabels::HF_MANDEL_LAMBDA, //
        FractalFlags::BAIL_TEST,                                                        //
        -3.0F, 5.0F, -3.0F, 3.0F,                                                       //
        FractalType::LAMBDA, FractalType::NO_FRACTAL,                                   //
        SymmetryType::X_AXIS_NO_PARAM,                                                  //
        lambda_orbit, mandel_per_pixel, mandel_per_image,                               //
        standard_fractal_type,                                                          //
        STD_BAILOUT                                                                     //
    },

    {
        FractalType::LORENZ_3D,                                           //
        "lorenz3d",                                                       //
        {TIME_STEP, "a", "b", "c"},                                       //
        {.02, 5, 15, 1},                                                  //
        id::help::HelpLabels::HT_LORENZ, id::help::HelpLabels::HF_LORENZ, //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::NO_RESUME | FractalFlags::PARAMS_3D |
            FractalFlags::INF_CALC,                                       //
        -30.0F, 30.0F, -30.0F, 30.0F,                                     //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,                 //
        SymmetryType::NONE,                                               //
        (VF) lorenz3d_orbit, nullptr, orbit3d_per_image,                  //
        orbit3d_type,                                                     //
        NO_BAILOUT                                                        //
    },

    {
        FractalType::ROSSLER,                                               //
        "rossler3d",                                                        //
        {TIME_STEP, "a", "b", "c"},                                         //
        {.04, .2, .2, 5.7},                                                 //
        id::help::HelpLabels::HT_ROSSLER, id::help::HelpLabels::HF_ROSSLER, //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::NO_RESUME | FractalFlags::PARAMS_3D |
            FractalFlags::INF_CALC,                                         //
        -30.0F, 30.0F, -20.0F, 40.0F,                                       //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,                   //
        SymmetryType::NONE,                                                 //
        (VF) rossler_orbit, nullptr, orbit3d_per_image,                     //
        orbit3d_type,                                                       //
        NO_BAILOUT                                                          //
    },

    {
        FractalType::HENON,                                                       //
        "henon",                                                                  //
        {"a", "b", "", ""},                                                       //
        {1.4, .3, 0, 0},                                                          //
        id::help::HelpLabels::HT_HENON, id::help::HelpLabels::HF_HENON,           //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::INF_CALC, //
        -1.4F, 1.4F, -.5F, .5F,                                                   //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,                         //
        SymmetryType::NONE,                                                       //
        (VF) henon_orbit, nullptr, orbit3d_per_image,                             //
        orbit2d_type,                                                             //
        NO_BAILOUT                                                                //
    },

    {
        FractalType::PICKOVER,                                                //
        "pickover",                                                           //
        {"a", "b", "c", "d"},                                                 //
        {2.24, .43, -.65, -2.43},                                             //
        id::help::HelpLabels::HT_PICKOVER, id::help::HelpLabels::HF_PICKOVER, //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::NO_RESUME |
            FractalFlags::PARAMS_3D,                                          //
        -8.0F / 3.0F, 8.0F / 3.0F, -2.0F, 2.0F,                               //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,                     //
        SymmetryType::NONE,                                                   //
        (VF) pickover_orbit, nullptr, orbit3d_per_image,                      //
        orbit3d_type,                                                         //
        NO_BAILOUT                                                            //
    },

    {
        FractalType::GINGERBREAD,                                                 //
        "gingerbreadman",                                                         //
        {"Initial x", "Initial y", "", ""},                                       //
        {-.1, 0, 0, 0},                                                           //
        id::help::HelpLabels::HT_GINGER, id::help::HelpLabels::HF_GINGERBREAD,    //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::INF_CALC, //
        -4.5F, 8.5F, -4.5F, 8.5F,                                                 //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,                         //
        SymmetryType::NONE,                                                       //
        (VF) ginger_bread_orbit, nullptr, orbit3d_per_image,                      //
        orbit2d_type,                                                             //
        NO_BAILOUT                                                                //
    },

    {
        FractalType::DIFFUSION,                                                  //
        "diffusion",                                                             //
        {
            "+Border size",                                                      //
            "+Type (0=Central,1=Falling,2=Square Cavity)",                       //
            "+Color change rate (0=Random)",                                     //
            ""                                                                   //
        },
        {10, 0, 0, 0},                                                           //
        id::help::HelpLabels::HT_DIFFUSION, id::help::HelpLabels::HF_DIFFUSION,  //
        FractalFlags::NO_ZOOM | FractalFlags::NO_GUESS | FractalFlags::NO_TRACE, //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,                        //
        SymmetryType::NONE,                                                      //
        nullptr, nullptr, standalone_per_image,                                  //
        diffusion_type,                                                          //
        NO_BAILOUT                                                               //
    },

    {
        FractalType::UNITY,                                             //
        "unity",                                                        //
        {"", "", "", ""},                                               //
        {0, 0, 0, 0},                                                   //
        id::help::HelpLabels::HT_UNITY, id::help::HelpLabels::HF_UNITY, //
        FractalFlags::NONE,                                             //
        -2.0F, 2.0F, -1.5F, 1.5F,                                       //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,               //
        SymmetryType::XY_AXIS,                                          //
        unity_orbit, other_julia_per_pixel, standard_per_image,         //
        standard_fractal_type,                                          //
        NO_BAILOUT                                                      //
    },

    {
        FractalType::SPIDER,                                                                 //
        "spider",                                                                            //
        {REAL_Z0, IMAG_Z0, "", ""},                                                          //
        {0, 0, 0, 0},                                                                        //
        id::help::HelpLabels::HT_TAYLOR_SKINNER_VARIATIONS, id::help::HelpLabels::HF_SPIDER, //
        FractalFlags::BAIL_TEST,                                                             //
        -2.5F, 1.5F, -1.5F, 1.5F,                                                            //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,                                    //
        SymmetryType::X_AXIS_NO_PARAM,                                                       //
        spider_orbit, mandel_per_pixel, mandel_per_image,                                    //
        standard_fractal_type,                                                               //
        STD_BAILOUT                                                                          //
    },

    {
        FractalType::TETRATE,                                                                 //
        "tetrate",                                                                            //
        {REAL_Z0, IMAG_Z0, "", ""},                                                           //
        {0, 0, 0, 0},                                                                         //
        id::help::HelpLabels::HT_TAYLOR_SKINNER_VARIATIONS, id::help::HelpLabels::HF_TETRATE, //
        FractalFlags::BAIL_TEST,                                                              //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                             //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,                                     //
        SymmetryType::X_AXIS_NO_IMAG,                                                         //
        tetrate_orbit, other_mandel_per_pixel, mandel_per_image,                              //
        standard_fractal_type,                                                                //
        STD_BAILOUT                                                                           //
    },

    {
        FractalType::MAGNET_1M,                                              //
        "magnet1m",                                                          //
        {REAL_Z0, IMAG_Z0, "", ""},                                          //
        {0, 0, 0, 0},                                                        //
        id::help::HelpLabels::HT_MAGNET, id::help::HelpLabels::HF_MAGNET_M1, //
        FractalFlags::NONE,                                                  //
        -4.0F, 4.0F, -3.0F, 3.0F,                                            //
        FractalType::MAGNET_1J, FractalType::NO_FRACTAL,                     //
        SymmetryType::X_AXIS_NO_PARAM,                                       //
        magnet1_orbit, mandel_per_pixel, mandel_per_image,                   //
        standard_fractal_type,                                               //
        100                                                                  //
    },

    {
        FractalType::MAGNET_1J,                                              //
        "magnet1j",                                                          //
        {REAL_PARAM, IMAG_PARAM, "", ""},                                    //
        {0, 0, 0, 0},                                                        //
        id::help::HelpLabels::HT_MAGNET, id::help::HelpLabels::HF_MAGNET_J1, //
        FractalFlags::NONE,                                                  //
        -8.0F, 8.0F, -6.0F, 6.0F,                                            //
        FractalType::NO_FRACTAL, FractalType::MAGNET_1M,                     //
        SymmetryType::X_AXIS_NO_IMAG,                                        //
        magnet1_orbit, julia_per_pixel, julia_per_image,                     //
        standard_fractal_type,                                               //
        100                                                                  //
    },

    {
        FractalType::MAGNET_2M,                                              //
        "magnet2m",                                                          //
        {REAL_Z0, IMAG_Z0, "", ""},                                          //
        {0, 0, 0, 0},                                                        //
        id::help::HelpLabels::HT_MAGNET, id::help::HelpLabels::HF_MAGNET_M2, //
        FractalFlags::NONE,                                                  //
        -1.5F, 3.7F, -1.95F, 1.95F,                                          //
        FractalType::MAGNET_2J, FractalType::NO_FRACTAL,                     //
        SymmetryType::X_AXIS_NO_PARAM,                                       //
        magnet2_orbit, mandel_per_pixel, mandel_per_image,                   //
        standard_fractal_type,                                               //
        100                                                                  //
    },

    {
        FractalType::MAGNET_2J,                                              //
        "magnet2j",                                                          //
        {REAL_PARAM, IMAG_PARAM, "", ""},                                    //
        {0, 0, 0, 0},                                                        //
        id::help::HelpLabels::HT_MAGNET, id::help::HelpLabels::HF_MAGNET_J2, //
        FractalFlags::NONE,                                                  //
        -8.0F, 8.0F, -6.0F, 6.0F,                                            //
        FractalType::NO_FRACTAL, FractalType::MAGNET_2M,                     //
        SymmetryType::X_AXIS_NO_IMAG,                                        //
        magnet2_orbit, julia_per_pixel, julia_per_image,                     //
        standard_fractal_type,                                               //
        100                                                                  //
    },

    {
        FractalType::BIF_LAMBDA,                                                                         //
        "biflambda",                                                                                     //
        {FILT, SEED_POP, "", ""},                                                                        //
        {1000.0, 0.66, 0, 0},                                                                            //
        id::help::HelpLabels::HT_BIFURCATION, id::help::HelpLabels::HF_BIF_LAMBDA,                       //
        FractalFlags::TRIG1 | FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::NO_ROTATE, //
        -2.0F, 4.0F, -1.0F, 2.0F,                                                                        //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,                                                //
        SymmetryType::NONE,                                                                              //
        bifurc_lambda_trig_orbit, nullptr, standalone_per_image,                                         //
        bifurcation_type,                                                                                //
        NO_BAILOUT                                                                                       //
    },

    {
        FractalType::BIF_PLUS_SIN_PI,                                                                    //
        "bif+sinpi",                                                                                     //
        {FILT, SEED_POP, "", ""},                                                                        //
        {1000.0, 0.66, 0, 0},                                                                            //
        id::help::HelpLabels::HT_BIFURCATION, id::help::HelpLabels::HF_BIF_PLUS_SIN_PI,                  //
        FractalFlags::TRIG1 | FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::NO_ROTATE, //
        0.275F, 1.45F, 0.0F, 2.0F,                                                                       //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,                                                //
        SymmetryType::NONE,                                                                              //
        bifurc_add_trig_pi_orbit, nullptr, standalone_per_image,                                         //
        bifurcation_type,                                                                                //
        NO_BAILOUT                                                                                       //
    },

    {
        FractalType::BIF_EQ_SIN_PI,                                                                      //
        "bif=sinpi",                                                                                     //
        {FILT, SEED_POP, "", ""},                                                                        //
        {1000.0, 0.66, 0, 0},                                                                            //
        id::help::HelpLabels::HT_BIFURCATION, id::help::HelpLabels::HF_BIF_EQ_SIN_PI,                    //
        FractalFlags::TRIG1 | FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::NO_ROTATE, //
        -2.5F, 2.5F, -3.5F, 3.5F,                                                                        //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,                                                //
        SymmetryType::NONE,                                                                              //
        bifurc_set_trig_pi_orbit, nullptr, standalone_per_image,                                         //
        bifurcation_type,                                                                                //
        NO_BAILOUT                                                                                       //
    },

    {
        FractalType::POPCORN_JUL,                                                 //
        "popcornjul",                                                             //
        {STEP_X, STEP_Y, CONSTANT_X, CONSTANT_Y},                                 //
        {0.05, 0, 3.00, 0},                                                       //
        id::help::HelpLabels::HT_POPCORN, id::help::HelpLabels::HF_POPCORN_JULIA, //
        FractalFlags::TRIG4,                                                      //
        -3.0F, 3.0F, -2.25F, 2.25F,                                               //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,                         //
        SymmetryType::NONE,                                                       //
        popcorn_orbit, other_julia_per_pixel, julia_per_image,                    //
        standard_fractal_type,                                                    //
        STD_BAILOUT                                                               //
    },

    {
        FractalType::L_SYSTEM,                                                                             //
        "lsystem",                                                                                         //
        {"+Order", "", "", ""},                                                                            //
        {2, 0, 0, 0},                                                                                      //
        id::help::HelpLabels::HT_L_SYSTEM, id::help::HelpLabels::SPECIAL_L_SYSTEM,                         //
        FractalFlags::NO_ZOOM | FractalFlags::NO_RESUME | FractalFlags::NO_GUESS | FractalFlags::NO_TRACE, //
        -1.0F, 1.0F, -1.0F, 1.0F,                                                                          //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,                                                  //
        SymmetryType::NONE,                                                                                //
        nullptr, nullptr, standalone_per_image,                                                            //
        lsystem_type,                                                                                      //
        NO_BAILOUT                                                                                         //
    },

    {
        FractalType::MAN_O_WAR_J,                                                                     //
        "manowarj",                                                                                   //
        {REAL_PARAM, IMAG_PARAM, "", ""},                                                             //
        {0, 0, 0, 0},                                                                                 //
        id::help::HelpLabels::HT_TAYLOR_SKINNER_VARIATIONS, id::help::HelpLabels::HF_MAN_O_WAR_JULIA, //
        FractalFlags::OK_JB | FractalFlags::BAIL_TEST,                                                //
        -2.5F, 1.5F, -1.5F, 1.5F,                                                                     //
        FractalType::NO_FRACTAL, FractalType::MAN_O_WAR,                                              //
        SymmetryType::NONE,                                                                           //
        man_o_war_orbit, julia_per_pixel, julia_per_image,                                            //
        standard_fractal_type,                                                                        //
        STD_BAILOUT                                                                                   //
    },

    {
        FractalType::FN_PLUS_FN_PIX,                                                                 //
        "fn(z)+fn(pix)",                                                                             //
        {REAL_Z0, IMAG_Z0, RE_COEF_TRIG2, IM_COEF_TRIG2},                                            //
        {0, 0, 1, 0},                                                                                //
        id::help::HelpLabels::HT_TAYLOR_SKINNER_VARIATIONS, id::help::HelpLabels::HF_FN_PLUS_FN_PIX, //
        FractalFlags::TRIG2 | FractalFlags::BAIL_TEST,                                               //
        -3.6F, 3.6F, -2.7F, 2.7F,                                                                    //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,                                            //
        SymmetryType::NONE,                                                                          //
        richard_8_orbit, other_richard_8_per_pixel, mandel_per_image,                                //
        standard_fractal_type,                                                                       //
        TRIG_BAILOUT_L                                                                               //
    },

    {
        FractalType::MARKS_MANDEL_PWR,                                                             //
        "marksmandelpwr",                                                                          //
        {REAL_Z0, IMAG_Z0, "", ""},                                                                //
        {0, 0, 0, 0},                                                                              //
        id::help::HelpLabels::HT_PETERSON_VARIATIONS, id::help::HelpLabels::HF_MARKS_MANDEL_POWER, //
        FractalFlags::TRIG1 | FractalFlags::BAIL_TEST,                                             //
        -2.5F, 1.5F, -1.5F, 1.5F,                                                                  //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,                                          //
        SymmetryType::X_AXIS_NO_PARAM,                                                             //
        marks_mandel_pwr_orbit, marks_mandel_pwr_per_pixel, mandel_per_image,                      //
        standard_fractal_type,                                                                     //
        STD_BAILOUT                                                                                //
    },

    {
        FractalType::TIMS_ERROR,                                                           //
        "tim's_error",                                                                     //
        {REAL_Z0, IMAG_Z0, "", ""},                                                        //
        {0, 0, 0, 0},                                                                      //
        id::help::HelpLabels::HT_PETERSON_VARIATIONS, id::help::HelpLabels::HF_TIMS_ERROR, //
        FractalFlags::TRIG1 | FractalFlags::BAIL_TEST,                                     //
        -2.9F, 4.3F, -2.7F, 2.7F,                                                          //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,                                  //
        SymmetryType::X_AXIS_NO_PARAM,                                                     //
        tims_error_orbit, marks_mandel_pwr_per_pixel, mandel_per_image,                    //
        standard_fractal_type,                                                             //
        STD_BAILOUT                                                                        //
    },

    {
        FractalType::BIF_STEWART,                                                                        //
        "bifstewart",                                                                                    //
        {FILT, SEED_POP, "", ""},                                                                        //
        {1000.0, 0.66, 0, 0},                                                                            //
        id::help::HelpLabels::HT_BIFURCATION, id::help::HelpLabels::HF_BIF_STEWART,                      //
        FractalFlags::TRIG1 | FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::NO_ROTATE, //
        0.7F, 2.0F, -1.1F, 1.1F,                                                                         //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,                                                //
        SymmetryType::NONE,                                                                              //
        bifurc_stewart_trig_orbit, nullptr, standalone_per_image,                                        //
        bifurcation_type,                                                                                //
        NO_BAILOUT                                                                                       //
    },

    {
        FractalType::HOPALONG,                                                    //
        "hopalong",                                                               //
        {"a", "b", "c", ""},                                                      //
        {.4, 1, 0, 0},                                                            //
        id::help::HelpLabels::HT_MARTIN, id::help::HelpLabels::HF_HOPALONG,       //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::INF_CALC, //
        -2.0F, 3.0F, -1.625F, 2.625F,                                             //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,                         //
        SymmetryType::NONE,                                                       //
        (VF) hopalong2d_orbit, nullptr, orbit3d_per_image,                        //
        orbit2d_type,                                                             //
        NO_BAILOUT                                                                //
    },

    {
        FractalType::CIRCLE,                                              //
        "circle",                                                         //
        {"magnification", "", "", ""},                                    //
        {200000L, 0, 0, 0},                                               //
        id::help::HelpLabels::HT_CIRCLE, id::help::HelpLabels::HF_CIRCLE, //
        FractalFlags::NONE,                                               //
        -2.0F, 2.0F, -1.5F, 1.5F,                                         //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,                 //
        SymmetryType::XY_AXIS,                                            //
        circle_orbit, julia_per_pixel, julia_per_image,                   //
        standard_fractal_type,                                            //
        NO_BAILOUT                                                        //
    },

    {
        FractalType::MARTIN,                                                      //
        "martin",                                                                 //
        {"a", "", "", ""},                                                        //
        {3.14, 0, 0, 0},                                                          //
        id::help::HelpLabels::HT_MARTIN, id::help::HelpLabels::HF_MARTIN,         //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::INF_CALC, //
        -32.0F, 32.0F, -24.0F, 24.0F,                                             //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,                         //
        SymmetryType::NONE,                                                       //
        (VF) martin2d_orbit, nullptr, orbit3d_per_image,                          //
        orbit2d_type,                                                             //
        NO_BAILOUT                                                                //
    },

    {
        FractalType::LYAPUNOV,                                                //
        "lyapunov",                                                           //
        {"+Order (integer)", "Population Seed", "+Filter Cycles", ""},        //
        {0, 0.5, 0, 0},                                                       //
        id::help::HelpLabels::HT_LYAPUNOV, id::help::HelpLabels::HT_LYAPUNOV, //
        FractalFlags::NONE,                                                   //
        -8.0F, 8.0F, -6.0F, 6.0F,                                             //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,                     //
        SymmetryType::NONE,                                                   //
        lyapunov_orbit, nullptr, lyapunov_per_image,                          //
        lyapunov_type,                                                        //
        NO_BAILOUT                                                            //
    },

    {
        FractalType::LORENZ_3D1,                                              //
        "lorenz3d1",                                                          //
        {TIME_STEP, "a", "b", "c"},                                           //
        {.02, 5, 15, 1},                                                      //
        id::help::HelpLabels::HT_LORENZ, id::help::HelpLabels::HF_LORENZ_3D1, //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::NO_RESUME | FractalFlags::PARAMS_3D |
            FractalFlags::INF_CALC,                                           //
        -30.0F, 30.0F, -30.0F, 30.0F,                                         //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,                     //
        SymmetryType::NONE,                                                   //
        (VF) lorenz3d1_orbit, nullptr, orbit3d_per_image,                     //
        orbit3d_type,                                                         //
        NO_BAILOUT                                                            //
    },

    {
        FractalType::LORENZ_3D3,                                              //
        "lorenz3d3",                                                          //
        {TIME_STEP, "a", "b", "c"},                                           //
        {.02, 10, 28, 2.66},                                                  //
        id::help::HelpLabels::HT_LORENZ, id::help::HelpLabels::HF_LORENZ_3D3, //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::NO_RESUME | FractalFlags::PARAMS_3D |
            FractalFlags::INF_CALC,                                           //
        -30.0F, 30.0F, -30.0F, 30.0F,                                         //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,                     //
        SymmetryType::NONE,                                                   //
        (VF) lorenz3d3_orbit, nullptr, orbit3d_per_image,                     //
        orbit3d_type,                                                         //
        NO_BAILOUT                                                            //
    },

    {
        FractalType::LORENZ_3D4,                                              //
        "lorenz3d4",                                                          //
        {TIME_STEP, "a", "b", "c"},                                           //
        {.02, 10, 28, 2.66},                                                  //
        id::help::HelpLabels::HT_LORENZ, id::help::HelpLabels::HF_LORENZ_3D4, //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::NO_RESUME | FractalFlags::PARAMS_3D |
            FractalFlags::INF_CALC,                                           //
        -30.0F, 30.0F, -30.0F, 30.0F,                                         //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,                     //
        SymmetryType::NONE,                                                   //
        (VF) lorenz3d4_orbit, nullptr, orbit3d_per_image,                     //
        orbit3d_type,                                                         //
        NO_BAILOUT                                                            //
    },

    {
        FractalType::LAMBDA_FN_FN,                                                       //
        "lambda(fn||fn)",                                                                //
        {REAL_PARAM, IMAG_PARAM, "Function Shift Value", ""},                            //
        {1, 0.1, 1, 0},                                                                  //
        id::help::HelpLabels::HT_FN_OR_FN, id::help::HelpLabels::HF_LAMBDA_FN_FN,        //
        FractalFlags::TRIG2 | FractalFlags::BAIL_TEST,                                   //
        -4.0F, 4.0F, -3.0F, 3.0F,                                                        //
        FractalType::NO_FRACTAL, FractalType::MAN_LAM_FN_FN,                             //
        SymmetryType::ORIGIN,                                                            //
        lambda_trig_or_trig_orbit, other_julia_per_pixel, lambda_trig_or_trig_per_image, //
        standard_fractal_type,                                                           //
        TRIG_BAILOUT_L                                                                   //
    },

    {
        FractalType::JUL_FN_FN,                                                        //
        "julia(fn||fn)",                                                               //
        {REAL_PARAM, IMAG_PARAM, "Function Shift Value", ""},                          //
        {0, 0, 8, 0},                                                                  //
        id::help::HelpLabels::HT_FN_OR_FN, id::help::HelpLabels::HF_JULIA_FN_FN,       //
        FractalFlags::TRIG2 | FractalFlags::BAIL_TEST,                                 //
        -4.0F, 4.0F, -3.0F, 3.0F,                                                      //
        FractalType::NO_FRACTAL, FractalType::MAN_FN_FN,                               //
        SymmetryType::X_AXIS,                                                          //
        julia_trig_or_trig_orbit, other_julia_per_pixel, julia_trig_or_trig_per_image, //
        standard_fractal_type,                                                         //
        TRIG_BAILOUT_L                                                                 //
    },

    {
        FractalType::MAN_LAM_FN_FN,                                                        //
        "manlam(fn||fn)",                                                                  //
        {REAL_Z0, IMAG_Z0, "Function Shift Value", ""},                                    //
        {0, 0, 10, 0},                                                                     //
        id::help::HelpLabels::HT_FN_OR_FN, id::help::HelpLabels::HF_MANDEL_LAMBDA_FN_FN,   //
        FractalFlags::TRIG2 | FractalFlags::BAIL_TEST,                                     //
        -4.0F, 4.0F, -3.0F, 3.0F,                                                          //
        FractalType::LAMBDA_FN_FN, FractalType::NO_FRACTAL,                                //
        SymmetryType::X_AXIS_NO_PARAM,                                                     //
        lambda_trig_or_trig_orbit, other_mandel_per_pixel, man_lam_trig_or_trig_per_image, //
        standard_fractal_type,                                                             //
        TRIG_BAILOUT_L                                                                     //
    },

    {
        FractalType::MAN_FN_FN,                                                          //
        "mandel(fn||fn)",                                                                //
        {REAL_Z0, IMAG_Z0, "Function Shift Value", ""},                                  //
        {0, 0, 0.5, 0},                                                                  //
        id::help::HelpLabels::HT_FN_OR_FN, id::help::HelpLabels::HF_MANDEL_FN_FN,        //
        FractalFlags::TRIG2 | FractalFlags::BAIL_TEST,                                   //
        -4.0F, 4.0F, -3.0F, 3.0F,                                                        //
        FractalType::JUL_FN_FN, FractalType::NO_FRACTAL,                                 //
        SymmetryType::X_AXIS_NO_PARAM,                                                   //
        julia_trig_or_trig_orbit, other_mandel_per_pixel, mandel_trig_or_trig_per_image, //
        standard_fractal_type,                                                           //
        TRIG_BAILOUT_L                                                                   //
    },

    {
        FractalType::BIF_MAY,                                                      //
        "bifmay",                                                                  //
        {FILT, SEED_POP, "Beta >= 2", ""},                                         //
        {300.0, 0.9, 5, 0},                                                        //
        id::help::HelpLabels::HT_BIFURCATION, id::help::HelpLabels::HF_BIF_MAY,    //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::NO_ROTATE, //
        -3.5F, -0.9F, -0.5F, 3.2F,                                                 //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,                          //
        SymmetryType::NONE,                                                        //
        bifurc_may_orbit, nullptr, bifurc_may_per_image,                           //
        bifurcation_type,                                                          //
        NO_BAILOUT                                                                 //
    },

    {
        FractalType::HALLEY,                                              //
        "halley",                                                         //
        {ORDER, REAL_RELAX, EPSILON, IMAG_RELAX},                         //
        {6, 1.0, 0.0001, 0},                                              //
        id::help::HelpLabels::HT_HALLEY, id::help::HelpLabels::HF_HALLEY, //
        FractalFlags::NONE,                                               //
        -2.0F, 2.0F, -1.5F, 1.5F,                                         //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,                 //
        SymmetryType::XY_AXIS,                                            //
        halley_orbit, halley_per_pixel, halley_per_image,                 //
        standard_fractal_type,                                            //
        NO_BAILOUT                                                        //
    },

    {
        FractalType::DYNAMIC,                                                      //
        "dynamic",                                                                 //
        {"+# of intervals (<0 = connect)", "time step (<0 = Euler)", "a", "b"},    //
        {50, .1, 1, 3},                                                            //
        id::help::HelpLabels::HT_DYNAMIC_SYSTEM, id::help::HelpLabels::HF_DYNAMIC, //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::TRIG1,     //
        -20.0F, 20.0F, -20.0F, 20.0F,                                              //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,                          //
        SymmetryType::NONE,                                                        //
        (VF) dynamic_orbit, nullptr, dynamic2d_per_image,                          //
        dynamic2d_type,                                                            //
        NO_BAILOUT                                                                 //
    },

    {
        FractalType::QUAT,                                                        //
        "quat",                                                                   //
        {"notused", "notused", "cj", "ck"},                                       //
        {0, 0, 0, 0},                                                             //
        id::help::HelpLabels::HT_QUATERNION, id::help::HelpLabels::HF_QUATERNION, //
        FractalFlags::OK_JB,                                                      //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                 //
        FractalType::QUAT_JUL, FractalType::NO_FRACTAL,                           //
        SymmetryType::X_AXIS,                                                     //
        quaternion_orbit, quaternion_per_pixel, mandel_per_image,                 //
        standard_fractal_type,                                                    //
        TRIG_BAILOUT_L                                                            //
    },

    {
        FractalType::QUAT_JUL,                                                          //
        "quatjul",                                                                      //
        {"c1", "ci", "cj", "ck"},                                                       //
        {-.745, 0, .113, .05},                                                          //
        id::help::HelpLabels::HT_QUATERNION, id::help::HelpLabels::HF_QUATERNION_JULIA, //
        FractalFlags::OK_JB | FractalFlags::MORE,                                       //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                       //
        FractalType::NO_FRACTAL, FractalType::QUAT,                                     //
        SymmetryType::ORIGIN,                                                           //
        quaternion_orbit, quaternion_jul_per_pixel, julia_per_image,                    //
        standard_fractal_type,                                                          //
        TRIG_BAILOUT_L                                                                  //
    },

    {
        FractalType::CELLULAR,                                                   //
        "cellular",                                                              //
        {CELL_INIT, CELL_RULE, CELL_TYPE, CELL_START},                           //
        {11.0, 3311100320.0, 41.0, 0},                                           //
        id::help::HelpLabels::HT_CELLULAR, id::help::HelpLabels::HF_CELLULAR,    //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::NO_ZOOM, //
        -1.0F, 1.0F, -1.0F, 1.0F,                                                //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,                        //
        SymmetryType::NONE,                                                      //
        nullptr, nullptr, cellular_per_image,                                    //
        cellular_type,                                                           //
        NO_BAILOUT                                                               //
    },

    {
        FractalType::JULIBROT,                                         //
        "julibrot",                                                    //
        {"", "", "", ""},                                              //
        {0, 0, 0, 0},                                                  //
        id::help::HelpLabels::HT_JULIBROT, id::help::HelpLabels::NONE, //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::NO_ROTATE |
            FractalFlags::NO_RESUME,                                   //
        -2.0F, 2.0F, -1.5F, 1.5F,                                      //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,              //
        SymmetryType::NONE,                                            //
        julia_orbit, julibrot_per_pixel, julibrot_per_image,           //
        standard_4d_type,                                              //
        STD_BAILOUT                                                    //
    },

#ifdef RANDOM_RUN
    {
        FractalType::INVERSE_JULIA,                                                                         //
        "julia_inverse",                                                                                    //
        {REAL_PARAM, IMAG_PARAM, MAX_HITS, "Random Run Interval"},                                          //
        {-0.11, 0.6557, 4, 1024},                                                                           //
        id::help::HelpLabels::HT_INVERSE_JULIA, id::help::HelpLabels::HF_INVERSE_JULIA,                     //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::INF_CALC | FractalFlags::NO_RESUME, //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                                           //
        FractalType::NO_FRACTAL, FractalType::MANDEL,                                                       //
        SymmetryType::NONE,                                                                                 //
        inverse_julia_orbit, nullptr, orbit3d_per_image,                                                    //
        inverse_julia_fractal_type,                                                                         //
        NO_BAILOUT                                                                                          //
    },

#else
    {
        FractalType::INVERSE_JULIA,                                                                         //
        "julia_inverse",                                                                                    //
        {REAL_PARAM, IMAG_PARAM, MAX_HITS, ""},                                                             //
        {-0.11, 0.6557, 4, 1024},                                                                           //
        id::help::HelpLabels::HT_INVERSE_JULIA, id::help::HelpLabels::HF_INVERSE_JULIA,                     //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::INF_CALC | FractalFlags::NO_RESUME, //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                                           //
        FractalType::NO_FRACTAL, FractalType::MANDEL,                                                       //
        SymmetryType::NONE,                                                                                 //
        inverse_julia_orbit, nullptr, orbit3d_per_image,                                                    //
        inverse_julia_fractal_type,                                                                         //
        NO_BAILOUT                                                                                          //
    },

#endif

    {
        FractalType::MANDEL_CLOUD,                                                    //
        "mandelcloud",                                                                //
        {"+# of intervals (<0 = connect)", "", "", ""},                               //
        {50, 0, 0, 0},                                                                //
        id::help::HelpLabels::HT_MANDEL_CLOUD, id::help::HelpLabels::HF_MANDEL_CLOUD, //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE,                              //
        -2.5F, 1.5F, -1.5F, 1.5F,                                                     //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,                             //
        SymmetryType::NONE,                                                           //
        (VF) mandel_cloud_orbit, nullptr, dynamic2d_per_image,                        //
        dynamic2d_type,                                                               //
        NO_BAILOUT                                                                    //
    },

    {
        FractalType::PHOENIX,                                               //
        "phoenix",                                                          //
        {P1_REAL, P2_REAL, DEGREE_Z, ""},                                   //
        {0.56667, -0.5, 0, 0},                                              //
        id::help::HelpLabels::HT_PHOENIX, id::help::HelpLabels::HF_PHOENIX, //
        FractalFlags::BAIL_TEST,                                            //
        -2.0F, 2.0F, -1.5F, 1.5F,                                           //
        FractalType::NO_FRACTAL, FractalType::MAND_PHOENIX,                 //
        SymmetryType::X_AXIS,                                               //
        phoenix_orbit, phoenix_per_pixel, phoenix_per_image,                //
        standard_fractal_type,                                              //
        STD_BAILOUT                                                         //
    },

    {
        FractalType::MAND_PHOENIX,                                                 //
        "mandphoenix",                                                             //
        {REAL_Z0, IMAG_Z0, DEGREE_Z, ""},                                          //
        {0.0, 0.0, 0, 0},                                                          //
        id::help::HelpLabels::HT_PHOENIX, id::help::HelpLabels::HF_MANDEL_PHOENIX, //
        FractalFlags::BAIL_TEST,                                                   //
        -2.5F, 1.5F, -1.5F, 1.5F,                                                  //
        FractalType::PHOENIX, FractalType::NO_FRACTAL,                             //
        SymmetryType::NONE,                                                        //
        phoenix_orbit, mand_phoenix_per_pixel, mand_phoenix_per_image,             //
        standard_fractal_type,                                                     //
        STD_BAILOUT                                                                //
    },

    {
        FractalType::HYPER_CMPLX,                                                       //
        "hypercomplex",                                                                 //
        {"notused", "notused", "cj", "ck"},                                             //
        {0, 0, 0, 0},                                                                   //
        id::help::HelpLabels::HT_HYPER_COMPLEX, id::help::HelpLabels::HF_HYPER_COMPLEX, //
        FractalFlags::OK_JB | FractalFlags::TRIG1,                                      //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                       //
        FractalType::HYPER_CMPLX_J, FractalType::NO_FRACTAL,                            //
        SymmetryType::X_AXIS,                                                           //
        hyper_complex_orbit, quaternion_per_pixel, mandel_per_image,                    //
        standard_fractal_type,                                                          //
        TRIG_BAILOUT_L                                                                  //
    },

    {
        FractalType::HYPER_CMPLX_J,                                                           //
        "hypercomplexj",                                                                      //
        {"c1", "ci", "cj", "ck"},                                                             //
        {-.745, 0, .113, .05},                                                                //
        id::help::HelpLabels::HT_HYPER_COMPLEX, id::help::HelpLabels::HF_HYPER_COMPLEX_JULIA, //
        FractalFlags::OK_JB | FractalFlags::TRIG1 | FractalFlags::MORE,                       //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                             //
        FractalType::NO_FRACTAL, FractalType::HYPER_CMPLX,                                    //
        SymmetryType::ORIGIN,                                                                 //
        hyper_complex_orbit, quaternion_jul_per_pixel, julia_per_image,                       //
        standard_fractal_type,                                                                //
        TRIG_BAILOUT_L                                                                        //
    },

    {
        FractalType::FROTHY_BASIN,                                      //
        "frothybasin",                                                  //
        {FROTH_MAPPING, FROTH_SHADE, FROTH_A_VALUE, ""},                //
        {1, 0, 1.028713768218725, 0},                                   //
        id::help::HelpLabels::HT_FROTH, id::help::HelpLabels::HF_FROTH, //
        FractalFlags::NO_TRACE,                                         //
        -2.8F, 2.8F, -2.355F, 1.845F,                                   //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,               //
        SymmetryType::NONE,                                             //
        froth_orbit, froth_per_pixel, froth_per_image,                  //
        froth_type,                                                     //
        FROTH_BAILOUT                                                   //
    },

    {
        FractalType::MANDEL4,                                                     //
        "mandel4",                                                                //
        {REAL_Z0, IMAG_Z0, "", ""},                                               //
        {0, 0, 0, 0},                                                             //
        id::help::HelpLabels::HT_MANDEL_JULIA4, id::help::HelpLabels::HF_MANDEL4, //
        FractalFlags::BAIL_TEST,                                                  //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                 //
        FractalType::JULIA4, FractalType::NO_FRACTAL,                             //
        SymmetryType::X_AXIS_NO_PARAM,                                            //
        mandel4_orbit, mandel_per_pixel, mandel_per_image,                        //
        standard_fractal_type,                                                    //
        STD_BAILOUT                                                               //
    },

    {
        FractalType::JULIA4,                                                     //
        "julia4",                                                                //
        {REAL_PARAM, IMAG_PARAM, "", ""},                                        //
        {0.6, 0.55, 0, 0},                                                       //
        id::help::HelpLabels::HT_MANDEL_JULIA4, id::help::HelpLabels::HF_JULIA4, //
        FractalFlags::OK_JB | FractalFlags::BAIL_TEST,                           //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                //
        FractalType::NO_FRACTAL, FractalType::MANDEL4,                           //
        SymmetryType::ORIGIN,                                                    //
        mandel4_orbit, julia_per_pixel, julia_per_image,                         //
        standard_fractal_type,                                                   //
        STD_BAILOUT                                                              //
    },

    {
        FractalType::MARKS_MANDEL,                                                           //
        "marksmandel",                                                                       //
        {REAL_Z0, IMAG_Z0, RE_EXPONENT, ""},                                                 //
        {0, 0, 1, 0},                                                                        //
        id::help::HelpLabels::HT_PETERSON_VARIATIONS, id::help::HelpLabels::HF_MARKS_MANDEL, //
        FractalFlags::BAIL_TEST,                                                             //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                            //
        FractalType::MARKS_JULIA, FractalType::NO_FRACTAL,                                   //
        SymmetryType::NONE,                                                                  //
        marks_lambda_orbit, marks_mandel_per_pixel, mandel_per_image,                        //
        standard_fractal_type,                                                               //
        STD_BAILOUT                                                                          //
    },

    {
        FractalType::MARKS_JULIA,                                                           //
        "marksjulia",                                                                       //
        {REAL_PARAM, IMAG_PARAM, RE_EXPONENT, ""},                                          //
        {0.1, 0.9, 1, 0},                                                                   //
        id::help::HelpLabels::HT_PETERSON_VARIATIONS, id::help::HelpLabels::HF_MARKS_JULIA, //
        FractalFlags::BAIL_TEST,                                                            //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                           //
        FractalType::NO_FRACTAL, FractalType::MARKS_MANDEL,                                 //
        SymmetryType::ORIGIN,                                                               //
        marks_lambda_orbit, julia_per_pixel, marks_julia_per_image,                         //
        standard_fractal_type,                                                              //
        STD_BAILOUT                                                                         //
    },

    {
        FractalType::ICON,                                                                             //
        "icons",                                                                                       //
        {"Lambda", "Alpha", "Beta", "Gamma"},                                                          //
        {-2.34, 2.0, 0.2, 0.1},                                                                        //
        id::help::HelpLabels::HT_ICON, id::help::HelpLabels::HF_ICON,                                  //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::INF_CALC | FractalFlags::MORE, //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                                      //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,                                              //
        SymmetryType::NONE,                                                                            //
        (VF) icon_orbit, nullptr, orbit3d_per_image,                                                   //
        orbit2d_type,                                                                                  //
        NO_BAILOUT                                                                                     //
    },

    {
        FractalType::ICON_3D,                                         //
        "icons3d",                                                    //
        {"Lambda", "Alpha", "Beta", "Gamma"},                         //
        {-2.34, 2.0, 0.2, 0.1},                                       //
        id::help::HelpLabels::HT_ICON, id::help::HelpLabels::HF_ICON, //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::INF_CALC | FractalFlags::PARAMS_3D |
            FractalFlags::MORE,                                       //
        -2.0F, 2.0F, -1.5F, 1.5F,                                     //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,             //
        SymmetryType::NONE,                                           //
        (VF) icon_orbit, nullptr, orbit3d_per_image,                  //
        orbit3d_type,                                                 //
        NO_BAILOUT                                                    //
    },

    {
        FractalType::PHOENIX_CPLX,                                               //
        "phoenixcplx",                                                           //
        {P1_REAL, P1_IMAG, P2_REAL, P2_IMAG},                                    //
        {0.2, 0, 0.3, 0},                                                        //
        id::help::HelpLabels::HT_PHOENIX, id::help::HelpLabels::HF_PHOENIX_CPLX, //
        FractalFlags::MORE | FractalFlags::BAIL_TEST,                            //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                //
        FractalType::NO_FRACTAL, FractalType::MAND_PHOENIX_CPLX,                 //
        SymmetryType::ORIGIN,                                                    //
        phoenix_fractal_cplx_orbit, phoenix_per_pixel, phoenix_cplx_per_image,   //
        standard_fractal_type,                                                   //
        STD_BAILOUT                                                              //
    },

    {
        FractalType::MAND_PHOENIX_CPLX,                                                  //
        "mandphoenixclx",                                                                //
        {REAL_Z0, IMAG_Z0, P2_REAL, P2_IMAG},                                            //
        {0, 0, 0.5, 0},                                                                  //
        id::help::HelpLabels::HT_PHOENIX, id::help::HelpLabels::HF_MANDEL_PHOENIX_CPLX,  //
        FractalFlags::MORE | FractalFlags::BAIL_TEST,                                    //
        -2.5F, 1.5F, -1.5F, 1.5F,                                                        //
        FractalType::PHOENIX_CPLX, FractalType::NO_FRACTAL,                              //
        SymmetryType::X_AXIS,                                                            //
        phoenix_fractal_cplx_orbit, mand_phoenix_per_pixel, mand_phoenix_cplx_per_image, //
        standard_fractal_type,                                                           //
        STD_BAILOUT                                                                      //
    },

    {
        FractalType::ANT,                                           //
        "ant",                                                      //
        {
            "#Rule String (1's and non-1's, 0 rand)",               //
            "#Maxpts",                                              //
            "+Numants (max 256)",                                   //
            "+Ant type (1 or 2)"                                    //
        },
        {1100, 1.0E9, 1, 1},                                        //
        id::help::HelpLabels::HT_ANT, id::help::HelpLabels::HF_ANT, //
        FractalFlags::NO_ZOOM | FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::NO_RESUME |
            FractalFlags::MORE,                                     //
        -1.0F, 1.0F, -1.0F, 1.0F,                                   //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,           //
        SymmetryType::NONE,                                         //
        nullptr, nullptr, standalone_per_image,                     //
        ant_type,                                                   //
        NO_BAILOUT                                                  //
    },

    {
        FractalType::CHIP,                                                        //
        "chip",                                                                   //
        {"a", "b", "c", ""},                                                      //
        {-15, -19, 1, 0},                                                         //
        id::help::HelpLabels::HT_MARTIN, id::help::HelpLabels::HF_CHIP,           //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::INF_CALC, //
        -760.0F, 760.0F, -570.0F, 570.0F,                                         //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,                         //
        SymmetryType::NONE,                                                       //
        (VF) chip2d_orbit, nullptr, orbit3d_per_image,                            //
        orbit2d_type,                                                             //
        NO_BAILOUT                                                                //
    },

    {
        FractalType::QUADRUP_TWO,                                                 //
        "quadruptwo",                                                             //
        {"a", "b", "c", ""},                                                      //
        {34, 1, 5, 0},                                                            //
        id::help::HelpLabels::HT_MARTIN, id::help::HelpLabels::HF_QUADRUP_TWO,    //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::INF_CALC, //
        -82.93367F, 112.2749F, -55.76383F, 90.64257F,                             //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,                         //
        SymmetryType::NONE,                                                       //
        (VF) quadrup_two2d_orbit, nullptr, orbit3d_per_image,                     //
        orbit2d_type,                                                             //
        NO_BAILOUT                                                                //
    },

    {
        FractalType::THREEPLY,                                                    //
        "threeply",                                                               //
        {"a", "b", "c", ""},                                                      //
        {-55, -1, -42, 0},                                                        //
        id::help::HelpLabels::HT_MARTIN, id::help::HelpLabels::HF_THREEPLY,       //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::INF_CALC, //
        -8000.0F, 8000.0F, -6000.0F, 6000.0F,                                     //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,                         //
        SymmetryType::NONE,                                                       //
        (VF) three_ply2d_orbit, nullptr, orbit3d_per_image,                       //
        orbit2d_type,                                                             //
        NO_BAILOUT                                                                //
    },

    {
        FractalType::VL,                                                                  //
        "volterra-lotka",                                                                 //
        {"h", "p", "", ""},                                                               //
        {0.739, 0.739, 0, 0},                                                             //
        id::help::HelpLabels::HT_VOLTERRA_LOTKA, id::help::HelpLabels::HF_VOLTERRA_LOTKA, //
        FractalFlags::NONE,                                                               //
        0.0F, 6.0F, 0.0F, 4.5F,                                                           //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,                                 //
        SymmetryType::NONE,                                                               //
        vl_orbit, other_julia_per_pixel, vl_per_image,                                    //
        standard_fractal_type,                                                            //
        256                                                                               //
    },

    {
        FractalType::ESCHER,                                              //
        "escher_julia",                                                   //
        {REAL_PARAM, IMAG_PARAM, "", ""},                                 //
        {0.32, 0.043, 0, 0},                                              //
        id::help::HelpLabels::HT_ESCHER, id::help::HelpLabels::HF_ESCHER, //
        FractalFlags::NONE,                                               //
        -1.6F, 1.6F, -1.2F, 1.2F,                                         //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,                 //
        SymmetryType::ORIGIN,                                             //
        escher_orbit, julia_per_pixel, standard_per_image,                //
        standard_fractal_type,                                            //
        STD_BAILOUT                                                       //
    },

    // From Pickovers' "Chaos in Wonderland"
    // included by Humberto R. Baptista
    // code adapted from king.cpp bt James Rankin
    {
        FractalType::LATOO,                                                                             //
        "latoocarfian",                                                                                 //
        {"a", "b", "c", "d"},                                                                           //
        {-0.966918, 2.879879, 0.765145, 0.744728},                                                      //
        id::help::HelpLabels::HT_LATOO, id::help::HelpLabels::HF_LATOO,                                 //
        FractalFlags::NO_GUESS | FractalFlags::NO_TRACE | FractalFlags::INF_CALC | FractalFlags::TRIG4, //
        -2.0F, 2.0F, -1.5F, 1.5F,                                                                       //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,                                               //
        SymmetryType::NONE,                                                                             //
        (VF) latoo_orbit, nullptr, orbit3d_per_image,                                                   //
        orbit2d_type,                                                                                   //
        NO_BAILOUT                                                                                      //
    },

    // Jim Muth formula
    {
        FractalType::DIVIDE_BROT5,                                                    //
        "dividebrot5",                                                                //
        {"a", "b", "", ""},                                                           //
        {2.0, 0.0, 0.0, 0.0},                                                         //
        id::help::HelpLabels::HT_DIVIDE_BROT5, id::help::HelpLabels::HF_DIVIDE_BROT5, //
        FractalFlags::BAIL_TEST | FractalFlags::BF_MATH,                              //
        -2.5f, 1.5f, -1.5f, 1.5f,                                                     //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,                             //
        SymmetryType::NONE,                                                           //
        divide_brot5_orbit, divide_brot5_per_pixel, divide_brot5_per_imge,            //
        standard_fractal_type,                                                        //
        16                                                                            //
    },

    // Jim Muth formula
    {
        FractalType::MANDELBROT_MIX4,                                                       //
        "mandelbrotmix4",                                                                   //
        {P1_REAL, P1_IMAG, P2_REAL, P2_IMAG},                                               //
        {0.05, 3, -1.5, -2},                                                                //
        id::help::HelpLabels::HT_MANDELBROT_MIX4, id::help::HelpLabels::HF_MANDELBROT_MIX4, //
        FractalFlags::BAIL_TEST | FractalFlags::TRIG1 | FractalFlags::MORE,                 //
        -2.5F, 1.5F, -1.5F, 1.5F,                                                           //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,                                   //
        SymmetryType::NONE,                                                                 //
        mandelbrot_mix4_orbit, mandelbrot_mix4_per_pixel, mandelbrot_mix4_per_image,        //
        standard_fractal_type,                                                              //
        STD_BAILOUT                                                                         //
    },

    {
        FractalType::BURNING_SHIP,                                                    //
        "burning-ship",                                                               //
        {P1_REAL, P1_IMAG, "degree (2-5)", ""},                                       //
        {0, 0, 2, 0},                                                                 //
        id::help::HelpLabels::HT_BURNING_SHIP, id::help::HelpLabels::HF_BURNING_SHIP, //
        FractalFlags::BAIL_TEST | FractalFlags::PERTURB | FractalFlags::BF_MATH,      //
        -2.5F, 1.5F, -1.2F, 1.8F,                                                     //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,                             //
        SymmetryType::NONE,                                                           //
        burning_ship_orbit, other_mandel_per_pixel, burning_ship_per_image,           //
        standard_fractal_type,                                                        //
        STD_BAILOUT,                                                                  //
        burning_ship_ref_pt, burning_ship_ref_pt_bf, burning_ship_perturb             //
    },

    // marks the END of the list
    {
        FractalType::NO_FRACTAL,                                //
        nullptr,                                                //
        {nullptr, nullptr, nullptr, nullptr},                   //
        {0, 0, 0, 0},                                           //
        id::help::HelpLabels::NONE, id::help::HelpLabels::NONE, //
        FractalFlags::NONE,                                     //
        0.0F, 0.0F, 0.0F, 0.0F,                                 //
        FractalType::NO_FRACTAL, FractalType::NO_FRACTAL,       //
        SymmetryType::NONE,                                     //
        nullptr, nullptr, nullptr, nullptr,                     //
        0,                                                      //
        nullptr, nullptr, nullptr                               //
    } //
};

int g_num_fractal_types = static_cast<int>(std::size(g_fractal_specific)) - 1;

FractalSpecific *g_cur_fractal_specific{};

FractalSpecific *get_fractal_specific(FractalType type)
{
    // g_fractal_specific is sorted by the type member, so we can use binary search.
    if (auto it = std::lower_bound(&g_fractal_specific[0], &g_fractal_specific[g_num_fractal_types], type,
            [](const FractalSpecific &specific, FractalType value) { return specific.type < value; });
        it != std::end(g_fractal_specific) && it->type == type)
    {
        return it;
    }
    throw std::runtime_error("Unknown fractal type " + std::to_string(+type));
}
