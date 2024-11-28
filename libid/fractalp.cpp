// SPDX-License-Identifier: GPL-3.0-only
//
/*
        This module consists only of the fractalspecific structure
*/
#include "port.h"
#include "prototyp.h"

#include "fractalp.h"

#include "ant.h"
#include "barnsley.h"
#include "bifurcation.h"
#include "calcfrac.h"
#include "cellular.h"
#include "circle_pattern.h"
#include "diffusion.h"
#include "divide_brot.h"
#include "escher.h"
#include "fn_or_fn.h"
#include "fractalb.h"
#include "fractals.h"
#include "fractype.h"
#include "frasetup.h"
#include "frothy_basin.h"
#include "halley.h"
#include "helpdefs.h"
#include "hypercomplex_mandelbrot.h"
#include "id.h"
#include "jb.h"
#include "lambda_fn.h"
#include "lorenz.h"
#include "lsys_fns.h"
#include "lyapunov.h"
#include "magnet.h"
#include "mandelbrot_mix.h"
#include "mpmath_c.h"
#include "newton.h"
#include "parser.h"
#include "peterson_variations.h"
#include "phoenix.h"
#include "pickover_mandelbrot.h"
#include "plasma.h"
#include "popcorn.h"
#include "quartic_mandelbrot.h"
#include "quaternion_mandelbrot.h"
#include "sierpinski_gasket.h"
#include "taylor_skinner_variations.h"
#include "testpt.h"
#include "unity.h"
#include "volterra_lotka.h"

// parameter descriptions
// Note: + prefix denotes integer parameters
//       # prefix denotes U32 parameters

// for Mandelbrots
static const char *const s_realz0{"Real Perturbation of Z(0)"};
static const char *const s_imagz0{"Imaginary Perturbation of Z(0)"};

// for Julias
static const char *const s_real_param{"Real Part of Parameter"};
static const char *const s_imag_param{"Imaginary Part of Parameter"};

// for Newtons
static const char *const s_newt_degree{"+Polynomial Degree (>= 2)"};

// for MarksMandel/Julia
static const char *const s_exponent{"Real part of Exponent"};
static const char *const s_im_exponent{"Imag part of Exponent"};

// for Lorenz
static const char *const s_time_step{"Time Step"};

// for formula
static const char *const s_p1_real{"Real portion of p1"};
static const char *const s_p2_real{"Real portion of p2"};
static const char *const s_p3_real{"Real portion of p3"};
static const char *const s_p4_real{"Real portion of p4"};
static const char *const s_p5_real{"Real portion of p5"};
static const char *const s_p1_imag{"Imaginary portion of p1"};
static const char *const s_p2_imag{"Imaginary portion of p2"};
static const char *const s_p3_imag{"Imaginary portion of p3"};
static const char *const s_p4_imag{"Imaginary portion of p4"};
static const char *const s_p5_imag{"Imaginary portion of p5"};

// trig functions
static const char *const s_re_coef_trg1{"Real Coefficient First Function"};
static const char *const s_im_coef_trg1{"Imag Coefficient First Function"};
static const char *const s_re_coef_trg2{"Real Coefficient Second Function"};
static const char *const s_im_coef_trg2{"Imag Coefficient Second Function"};

// KAM Torus
static const char *const s_kam_angle{"Angle (radians)"};
static const char *const s_kam_step{"Step size"};
static const char *const s_kam_stop{"Stop value"};
static const char *const s_points_per_orbit{"+Points per orbit"};

// popcorn and julia popcorn generalized
static const char *const s_step_x{"Step size (real)"};
static const char *const s_step_y{"Step size (imaginary)"};
static const char *const s_constant_x{"Constant C (real)"};
static const char *const s_constant_y{"Constant C (imaginary)"};

// bifurcations
static const char *const s_filt{"+Filter Cycles"};
static const char *const s_seed{"Seed Population"};

// frothy basins
static const char *const s_froth_mapping{"+Apply mapping once (1) or twice (2)"};
static const char *const s_froth_shade{"+Enter non-zero value for alternate color shading"};
static const char *const s_froth_a_value{"A (imaginary part of C)"};

// plasma and ant
static const char *const s_random_seed{"+Random Seed Value (0 = Random, 1 = Reuse Last)"};

// ifs
static const char *const s_color_method{"+Coloring method (0,1)"};

// phoenix fractals
static const char *const s_degree_z{"Degree = 0 | >= 2 | <= -3"};

// julia inverse
static const char *const s_max_hits{"Max Hits per Pixel"};

// halley
static const char *const s_order{"+Order (integer > 1)"};
static const char *const s_real_relax{"Real Relaxation coefficient"};
static const char *const s_epsilon{"Epsilon"};
static const char *const s_imag_relax{"Imag Relaxation coefficient"};

// cellular
static const char *const s_cell_init{"#Initial String | 0 = Random | -1 = Reuse Last Random"};
static const char *const s_cell_rule{"#Rule = # of digits (see below) | 0 = Random"};
static const char *const s_cell_type{"+Type (see below)"};
static const char *const s_cell_strt{"#Starting Row Number"};

// bailout values
enum
{
    LTRIG_BAIL_OUT = 64,
    FROTH_BAIL_OUT = 7,
    STD_BAIL_OUT = 4,
    NO_BAIL_OUT = 0,
};

// clang-format off
MoreParams g_more_fractal_params[] =
{
    {FractalType::ICON,                {"Omega", "+Degree of symmetry",   "", "", "", ""}, {0, 3, 0, 0, 0, 0}},
    {FractalType::ICON3D,              {"Omega", "+Degree of symmetry",   "", "", "", ""}, {0, 3, 0, 0, 0, 0}},
    {FractalType::HYPERCMPLXJFP,       {"zj", "zk", "", "", "", ""}, {0, 0, 0, 0, 0, 0}},
    {FractalType::QUATJULFP,           {"zj", "zk", "", "", "", ""}, {0, 0, 0, 0, 0, 0}},
    {FractalType::PHOENIXCPLX,         {s_degree_z, "", "", "", "", ""}, {0, 0, 0, 0, 0, 0}},
    {FractalType::PHOENIXFPCPLX,       {s_degree_z, "", "", "", "", ""}, {0, 0, 0, 0, 0, 0}},
    {FractalType::MANDPHOENIXCPLX,     {s_degree_z, "", "", "", "", ""}, {0, 0, 0, 0, 0, 0}},
    {FractalType::MANDPHOENIXFPCPLX,   {s_degree_z, "", "", "", "", ""}, {0, 0, 0, 0, 0, 0}},
    {FractalType::FORMULA,             {s_p3_real, s_p3_imag, s_p4_real, s_p4_imag, s_p5_real, s_p5_imag}, {0, 0, 0, 0, 0, 0}},
    {FractalType::FFORMULA,            {s_p3_real, s_p3_imag, s_p4_real, s_p4_imag, s_p5_real, s_p5_imag}, {0, 0, 0, 0, 0, 0}},
    {FractalType::ANT,                 {"+Wrap?", s_random_seed, "", "", "", ""}, {1, 0, 0, 0, 0, 0}},
    {FractalType::MANDELBROTMIX4,      {s_p3_real, s_p3_imag, "", "", "", ""}, {0, 0, 0, 0, 0, 0}},
    {FractalType::NOFRACTAL,           {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr    }, {0, 0, 0, 0, 0, 0}}
};
// clang-format on

//   type math orbitcalc fnct per_pixel fnct per_image fnct
// |-----|----|--------------|--------------|--------------|
AlternateMath g_alternate_math[] =
{
#define USEBN
#ifdef USEBN
    {FractalType::JULIAFP, BFMathType::BIGNUM, julia_bn_fractal, julia_bn_per_pixel,  mandel_bn_setup},
    {FractalType::MANDELFP, BFMathType::BIGNUM, julia_bn_fractal, mandel_bn_per_pixel, mandel_bn_setup},
#else
    {fractal_type::JULIAFP, bf_math_type::BIGFLT, julia_bf_fractal, julia_bf_per_pixel,  mandel_bf_setup},
    {fractal_type::MANDELFP, bf_math_type::BIGFLT, julia_bf_fractal, mandel_bf_per_pixel, mandel_bf_setup},
#endif
    /*
    NOTE: The default precision for g_bf_math=BIGNUM is not high enough
          for JuliaZpowerbnFractal.  If you want to test BIGNUM (1) instead
          of the usual BIGFLT (2), then set bfdigits on the command to
          increase the precision.
    */
    {FractalType::FPJULIAZPOWER, BFMathType::BIGFLT, julia_z_power_bf_fractal, julia_bf_per_pixel, mandel_bf_setup},
    {FractalType::FPMANDELZPOWER, BFMathType::BIGFLT, julia_z_power_bf_fractal, mandel_bf_per_pixel, mandel_bf_setup},
    {FractalType::DIVIDE_BROT5, BFMathType::BIGFLT, divide_brot5_bf_fractal, divide_brot5_bf_per_pixel, mandel_bf_setup},
    {FractalType::NOFRACTAL, BFMathType::NONE, nullptr, nullptr, nullptr}
};

// These are only needed for types with both integer and float variations
const char *const s_t_barnsleyj1{"*barnsleyj1"};
const char *const s_t_barnsleyj2{"*barnsleyj2"};
const char *const s_t_barnsleyj3{"*barnsleyj3"};
const char *const s_t_barnsleym1{"*barnsleym1"};
const char *const s_t_barnsleym2{"*barnsleym2"};
const char *const s_t_barnsleym3{"*barnsleym3"};
const char *const s_t_bifplussinpi{"*bif+sinpi"};
const char *const s_t_bifeqsinpi{"*bif=sinpi"};
const char *const s_t_biflambda{"*biflambda"};
const char *const s_t_bifmay{"*bifmay"};
const char *const s_t_bifstewart{"*bifstewart"};
const char *const s_t_bifurcation{"*bifurcation"};
const char *const s_t_fn_z_plusfn_pix_{"*fn(z)+fn(pix)"};
const char *const s_t_fn_zz_{"*fn(z*z)"};
const char *const s_t_fnfn{"*fn*fn"};
const char *const s_t_fnzplusz{"*fn*z+z"};
const char *const s_t_fnplusfn{"*fn+fn"};
const char *const s_t_formula{"*formula"};
const char *const s_t_henon{"*henon"};
const char *const s_t_ifs3d{"*ifs3d"};
const char *const s_t_julfnplusexp{"*julfn+exp"};
const char *const s_t_julfnpluszsqrd{"*julfn+zsqrd"};
const char *const s_t_julia{"*julia"};
const char *const s_t_julia_fn_or_fn{"*julia(fn||fn)"};
const char *const s_t_julia4{"*julia4"};
const char *const s_t_julia_inverse{"*julia_inverse"};
const char *const s_t_julibrot{"*julibrot"};
const char *const s_t_julzpower{"*julzpower"};
const char *const s_t_kamtorus{"*kamtorus"};
const char *const s_t_kamtorus3d{"*kamtorus3d"};
const char *const s_t_lambda{"*lambda"};
const char *const s_t_lambda_fn_or_fn{"*lambda(fn||fn)"};
const char *const s_t_lambdafn{"*lambdafn"};
const char *const s_t_lorenz{"*lorenz"};
const char *const s_t_lorenz3d{"*lorenz3d"};
const char *const s_t_mandel{"*mandel"};
const char *const s_t_mandel_fn_or_fn{"*mandel(fn||fn)"};
const char *const s_t_mandel4{"*mandel4"};
const char *const s_t_mandelfn{"*mandelfn"};
const char *const s_t_mandellambda{"*mandellambda"};
const char *const s_t_mandphoenix{"*mandphoenix"};
const char *const s_t_mandphoenixcplx{"*mandphoenixclx"};
const char *const s_t_manfnplusexp{"*manfn+exp"};
const char *const s_t_manfnpluszsqrd{"*manfn+zsqrd"};
const char *const s_t_manlam_fnorfn{"*manlam(fn||fn)"};
const char *const s_t_manowar{"*manowar"};
const char *const s_t_manowarj{"*manowarj"};
const char *const s_t_manzpower{"*manzpower"};
const char *const s_t_marksjulia{"*marksjulia"};
const char *const s_t_marksmandel{"*marksmandel"};
const char *const s_t_marksmandelpwr{"*marksmandelpwr"};
const char *const s_t_newtbasin{"*newtbasin"};
const char *const s_t_newton{"*newton"};
const char *const s_t_phoenix{"*phoenix"};
const char *const s_t_phoenixcplx{"*phoenixcplx"};
const char *const s_t_popcorn{"*popcorn"};
const char *const s_t_popcornjul{"*popcornjul"};
const char *const s_t_rossler3d{"*rossler3d"};
const char *const s_t_sierpinski{"*sierpinski"};
const char *const s_t_spider{"*spider"};
const char *const s_t_sqr_1divfn{"*sqr(1/fn)"};
const char *const s_t_sqr_fn{"*sqr(fn)"};
const char *const s_t_tims_error{"*tim's_error"};
const char *const s_t_unity{"*unity"};
const char *const s_t_frothybasin{"*frothybasin"};
const char *const s_t_halley{"*halley"};

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
       helptext, helpformula, flags,
       xmin, xmax, ymin, ymax,
       int, tojulia, tomandel, tofloat, symmetry,
       orbit fnct, per_pixel fnct, per_image fnct, calctype fcnt,
       bailout,
       perturbation ref pt, perturbation bf ref pt, perturbation point
    }
    */

    {
        s_t_mandel+1,
        {s_realz0, s_imagz0, "", ""},
        {0, 0, 0, 0},
        HelpLabels::HT_MANDEL, HelpLabels::HF_MANDEL, FractalFlags::BAILTEST | FractalFlags::PERTURB,
        -2.5F, 1.5F, -1.5F, 1.5F,
        1, FractalType::JULIA, FractalType::NOFRACTAL, FractalType::MANDELFP, SymmetryType::X_AXIS_NO_PARAM,
        julia_fractal, mandel_per_pixel, mandel_setup, standard_fractal,
        STD_BAIL_OUT,
        mandel_ref_pt, mandel_ref_pt_bf, mandel_perturb
    },

    {
        s_t_julia+1,
        {s_real_param, s_imag_param, "", ""},
        {0.3, 0.6, 0, 0},
        HelpLabels::HT_JULIA, HelpLabels::HF_JULIA, FractalFlags::OKJB|FractalFlags::BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        1, FractalType::NOFRACTAL, FractalType::MANDEL, FractalType::JULIAFP, SymmetryType::ORIGIN,
        julia_fractal, julia_per_pixel, julia_setup, standard_fractal,
        STD_BAIL_OUT
    },

    {
        s_t_newtbasin,
        {s_newt_degree, "Enter non-zero value for stripes", "", ""},
        {3, 0, 0, 0},
        HelpLabels::HT_NEWTBAS, HelpLabels::HF_NEWTBAS, FractalFlags::NONE,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::MPNEWTBASIN, SymmetryType::NONE,
        newton_fractal2, other_julia_fp_per_pixel, newton_setup, standard_fractal,
        NO_BAIL_OUT
    },

    {
        s_t_lambda+1,
        {s_real_param, s_imag_param, "", ""},
        {0.85, 0.6, 0, 0},
        HelpLabels::HT_LAMBDA, HelpLabels::HF_LAMBDA, FractalFlags::OKJB|FractalFlags::BAILTEST,
        -1.5F, 2.5F, -1.5F, 1.5F,
        1, FractalType::NOFRACTAL, FractalType::MANDELLAMBDA, FractalType::LAMBDAFP, SymmetryType::NONE,
        lambda_fractal, julia_per_pixel, julia_long_setup, standard_fractal,
        STD_BAIL_OUT
    },

    {
        s_t_mandel,
        {s_realz0, s_imagz0, "", ""},
        {0, 0, 0, 0},
        HelpLabels::HT_MANDEL, HelpLabels::HF_MANDEL, FractalFlags::BAILTEST|FractalFlags::BF_MATH|FractalFlags::PERTURB,
        -2.5F, 1.5F, -1.5F, 1.5F,
        0, FractalType::JULIAFP, FractalType::NOFRACTAL, FractalType::MANDEL, SymmetryType::X_AXIS_NO_PARAM,
        julia_fp_fractal, mandel_fp_per_pixel, mandel_fp_setup, standard_fractal,
        STD_BAIL_OUT,
        mandel_ref_pt, mandel_ref_pt_bf, mandel_perturb
    },

    {
        s_t_newton,
        {s_newt_degree, "", "", ""},
        {3, 0, 0, 0},
        HelpLabels::HT_NEWT, HelpLabels::HF_NEWT, FractalFlags::NONE,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::MPNEWTON, SymmetryType::X_AXIS,
        newton_fractal2, other_julia_fp_per_pixel, newton_setup, standard_fractal,
        NO_BAIL_OUT
    },

    {
        s_t_julia,
        {s_real_param, s_imag_param, "", ""},
        {0.3, 0.6, 0, 0},
        HelpLabels::HT_JULIA, HelpLabels::HF_JULIA, FractalFlags::OKJB|FractalFlags::BAILTEST|FractalFlags::BF_MATH,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, FractalType::NOFRACTAL, FractalType::MANDELFP, FractalType::JULIA, SymmetryType::ORIGIN,
        julia_fp_fractal, julia_fp_per_pixel,  julia_fp_setup, standard_fractal,
        STD_BAIL_OUT
    },

    {
        "plasma",
        {
            "Graininess Factor (0 or 0.125 to 100, default is 2)",
            "+Algorithm (0 = original, 1 = new)",
            "+Random Seed Value (0 = Random, 1 = Reuse Last)",
            "+Save as Pot File? (0 = No,     1 = Yes)"
        },
        {2, 0, 0, 0},
        HelpLabels::HT_PLASMA, HelpLabels::HF_PLASMA, FractalFlags::NOZOOM|FractalFlags::NOGUESS|FractalFlags::NOTRACE|FractalFlags::NORESUME,
        -2.0F, 2.0F, -1.5F, 1.5F,
        1, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::NOFRACTAL, SymmetryType::NONE,
        nullptr, nullptr, standalone_setup, plasma,
        NO_BAIL_OUT
    },

    {
        s_t_mandelfn,
        {s_realz0, s_imagz0, "", ""},
        {0, 0, 0, 0},
        HelpLabels::HT_MANDFN, HelpLabels::HF_MANDFN, FractalFlags::TRIG1,
        -8.0F, 8.0F, -6.0F, 6.0F,
        0, FractalType::LAMBDATRIGFP, FractalType::NOFRACTAL, FractalType::MANDELTRIG, SymmetryType::XY_AXIS_NO_PARAM,
        lambda_trig_fp_fractal, other_mandel_fp_per_pixel, mandel_trig_setup,
        standard_fractal,
        LTRIG_BAIL_OUT
    },

    {
        s_t_manowar,
        {s_realz0, s_imagz0, "", ""},
        {0, 0, 0, 0},
        HelpLabels::HT_SCOTSKIN, HelpLabels::HF_MANOWAR, FractalFlags::BAILTEST,
        -2.5F, 1.5F, -1.5F, 1.5F,
        0, FractalType::MANOWARJFP, FractalType::NOFRACTAL, FractalType::MANOWAR, SymmetryType::X_AXIS_NO_PARAM,
        man_o_war_fp_fractal, mandel_fp_per_pixel, mandel_fp_setup, standard_fractal,
        STD_BAIL_OUT
    },

    {
        s_t_manowar+1,
        {s_realz0, s_imagz0, "", ""},
        {0, 0, 0, 0},
        HelpLabels::HT_SCOTSKIN, HelpLabels::HF_MANOWAR, FractalFlags::BAILTEST,
        -2.5F, 1.5F, -1.5F, 1.5F,
        1, FractalType::MANOWARJ, FractalType::NOFRACTAL, FractalType::MANOWARFP, SymmetryType::X_AXIS_NO_PARAM,
        man_o_war_fractal, mandel_per_pixel, mandel_long_setup, standard_fractal,
        STD_BAIL_OUT
    },

    {
        "test",
        {
            "(testpt Param #1)",
            "(testpt param #2)",
            "(testpt param #3)",
            "(testpt param #4)"
        },
        {0, 0, 0, 0},
        HelpLabels::HT_TEST, HelpLabels::HF_TEST, FractalFlags::NONE,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::NOFRACTAL, SymmetryType::NONE,
        nullptr, nullptr, standalone_setup, test,
        STD_BAIL_OUT
    },

    {
        s_t_sierpinski+1,
        {"", "", "", ""},
        {0, 0, 0, 0},
        HelpLabels::HT_SIER, HelpLabels::HF_SIER, FractalFlags::NONE,
        -4.0F/3.0F, 96.0F/45.0F, -0.9F, 1.7F,
        1, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::SIERPINSKIFP, SymmetryType::NONE,
        sierpinski_fractal, long_julia_per_pixel, sierpinski_setup,
        standard_fractal,
        127
    },

    {
        s_t_barnsleym1+1,
        {s_realz0, s_imagz0, "", ""},
        {0, 0, 0, 0},
        HelpLabels::HT_BARNS, HelpLabels::HF_BARNSM1, FractalFlags::BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        1, FractalType::BARNSLEYJ1, FractalType::NOFRACTAL, FractalType::BARNSLEYM1FP, SymmetryType::XY_AXIS_NO_PARAM,
        barnsley1_fractal, long_mandel_per_pixel, mandel_long_setup,
        standard_fractal,
        STD_BAIL_OUT
    },

    {
        s_t_barnsleyj1+1,
        {s_real_param, s_imag_param, "", ""},
        {0.6, 1.1, 0, 0},
        HelpLabels::HT_BARNS, HelpLabels::HF_BARNSJ1, FractalFlags::OKJB|FractalFlags::BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        1, FractalType::NOFRACTAL, FractalType::BARNSLEYM1, FractalType::BARNSLEYJ1FP, SymmetryType::ORIGIN,
        barnsley1_fractal, long_julia_per_pixel, julia_long_setup, standard_fractal,
        STD_BAIL_OUT
    },

    {
        s_t_barnsleym2+1,
        {s_realz0, s_imagz0, "", ""},
        {0, 0, 0, 0},
        HelpLabels::HT_BARNS, HelpLabels::HF_BARNSM2, FractalFlags::BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        1, FractalType::BARNSLEYJ2, FractalType::NOFRACTAL, FractalType::BARNSLEYM2FP, SymmetryType::Y_AXIS_NO_PARAM,
        barnsley2_fractal, long_mandel_per_pixel, mandel_long_setup,
        standard_fractal,
        STD_BAIL_OUT
    },

    {
        s_t_barnsleyj2+1,
        {s_real_param, s_imag_param, "", ""},
        {0.6, 1.1, 0, 0},
        HelpLabels::HT_BARNS, HelpLabels::HF_BARNSJ2, FractalFlags::OKJB|FractalFlags::BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        1, FractalType::NOFRACTAL, FractalType::BARNSLEYM2, FractalType::BARNSLEYJ2FP, SymmetryType::ORIGIN,
        barnsley2_fractal, long_julia_per_pixel, julia_long_setup, standard_fractal,
        STD_BAIL_OUT
    },

    {
        s_t_sqr_fn+1,
        {"", "", "", ""},
        {0, 0, 0, 0},
        HelpLabels::HT_SCOTSKIN, HelpLabels::HF_SQRFN, FractalFlags::TRIG1|FractalFlags::BAILTEST,
        -4.0F, 4.0F, -3.0F, 3.0F,
        16, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::SQRTRIGFP, SymmetryType::X_AXIS,
        sqr_trig_fractal, long_julia_per_pixel, sqr_trig_setup, standard_fractal,
        LTRIG_BAIL_OUT
    },

    {
        s_t_sqr_fn,
        {"", "", "", ""},
        {0, 0, 0, 0},
        HelpLabels::HT_SCOTSKIN, HelpLabels::HF_SQRFN, FractalFlags::TRIG1|FractalFlags::BAILTEST,
        -4.0F, 4.0F, -3.0F, 3.0F,
        0, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::SQRTRIG, SymmetryType::X_AXIS,
        sqr_trig_fp_fractal, other_julia_fp_per_pixel, sqr_trig_setup, standard_fractal,
        LTRIG_BAIL_OUT
    },

    {
        s_t_fnplusfn+1,
        {s_re_coef_trg1, s_im_coef_trg1, s_re_coef_trg2, s_im_coef_trg2},
        {1, 0, 1, 0},
        HelpLabels::HT_SCOTSKIN, HelpLabels::HF_FNPLUSFN, FractalFlags::TRIG2|FractalFlags::BAILTEST,
        -4.0F, 4.0F, -3.0F, 3.0F,
        16, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::TRIGPLUSTRIGFP, SymmetryType::X_AXIS,
        trig_plus_trig_fractal, long_julia_per_pixel, trig_plus_trig_long_setup,
        standard_fractal,
        LTRIG_BAIL_OUT
    },

    {
        s_t_mandellambda+1,
        {s_realz0, s_imagz0, "", ""},
        {0, 0, 0, 0},
        HelpLabels::HT_MLAMBDA, HelpLabels::HF_MLAMBDA, FractalFlags::BAILTEST,
        -3.0F, 5.0F, -3.0F, 3.0F,
        1, FractalType::LAMBDA, FractalType::NOFRACTAL, FractalType::MANDELLAMBDAFP, SymmetryType::X_AXIS_NO_PARAM,
        lambda_fractal, mandel_per_pixel, mandel_long_setup, standard_fractal,
        STD_BAIL_OUT
    },

    {
        s_t_marksmandel+1,
        {s_realz0, s_imagz0, s_exponent, ""},
        {0, 0, 1, 0},
        HelpLabels::HT_MARKS, HelpLabels::HF_MARKSMAND, FractalFlags::BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        1, FractalType::MARKSJULIA, FractalType::NOFRACTAL, FractalType::MARKSMANDELFP, SymmetryType::NONE,
        marks_lambda_fractal, marks_mandel_per_pixel, mandel_long_setup,
        standard_fractal,
        STD_BAIL_OUT
    },

    {
        s_t_marksjulia+1,
        {s_real_param, s_imag_param, s_exponent, ""},
        {0.1, 0.9, 1, 0},
        HelpLabels::HT_MARKS, HelpLabels::HF_MARKSJULIA, FractalFlags::BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        1, FractalType::NOFRACTAL, FractalType::MARKSMANDEL, FractalType::MARKSJULIAFP, SymmetryType::ORIGIN,
        marks_lambda_fractal, julia_per_pixel, marks_julia_setup, standard_fractal,
        STD_BAIL_OUT
    },

    {
        s_t_unity+1,
        {"", "", "", ""},
        {0, 0, 0, 0},
        HelpLabels::HT_UNITY, HelpLabels::HF_UNITY, FractalFlags::NONE,
        -2.0F, 2.0F, -1.5F, 1.5F,
        1, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::UNITYFP, SymmetryType::XY_AXIS,
        unity_fractal, long_julia_per_pixel, unity_setup, standard_fractal,
        NO_BAIL_OUT
    },

    {
        s_t_mandel4+1,
        {s_realz0, s_imagz0, "", ""},
        {0, 0, 0, 0},
        HelpLabels::HT_MANDJUL4, HelpLabels::HF_MANDEL4, FractalFlags::BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        1, FractalType::JULIA4, FractalType::NOFRACTAL, FractalType::MANDEL4FP, SymmetryType::X_AXIS_NO_PARAM,
        mandel4_fractal, mandel_per_pixel, mandel_long_setup, standard_fractal,
        STD_BAIL_OUT
    },

    {
        s_t_julia4+1,
        {s_real_param, s_imag_param, "", ""},
        {0.6, 0.55, 0, 0},
        HelpLabels::HT_MANDJUL4, HelpLabels::HF_JULIA4, FractalFlags::OKJB|FractalFlags::BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        1, FractalType::NOFRACTAL, FractalType::MANDEL4, FractalType::JULIA4FP, SymmetryType::ORIGIN,
        mandel4_fractal, julia_per_pixel, julia_long_setup, standard_fractal,
        STD_BAIL_OUT
    },

    {
        "ifs",
        {s_color_method, "", "", ""},
        {0, 0, 0, 0},
        HelpLabels::HT_IFS, HelpLabels::SPECIAL_IFS, FractalFlags::NOGUESS|FractalFlags::NOTRACE|FractalFlags::NORESUME|FractalFlags::INFCALC,
        -8.0F, 8.0F, -1.0F, 11.0F,
        16, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::NOFRACTAL, SymmetryType::NONE,
        nullptr, nullptr, standalone_setup, ifs,
        NO_BAIL_OUT
    },

    {
        s_t_ifs3d,
        {s_color_method, "", "", ""},
        {0, 0, 0, 0},
        HelpLabels::HT_IFS, HelpLabels::SPECIAL_IFS, FractalFlags::NOGUESS|FractalFlags::NOTRACE|FractalFlags::NORESUME|FractalFlags::PARMS3D|FractalFlags::INFCALC,
        -11.0F, 11.0F, -11.0F, 11.0F,
        16, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::NOFRACTAL, SymmetryType::NONE,
        nullptr, nullptr, standalone_setup, ifs,
        NO_BAIL_OUT
    },

    {
        s_t_barnsleym3+1,
        {s_realz0, s_imagz0, "", ""},
        {0, 0, 0, 0},
        HelpLabels::HT_BARNS, HelpLabels::HF_BARNSM3, FractalFlags::BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        1, FractalType::BARNSLEYJ3, FractalType::NOFRACTAL, FractalType::BARNSLEYM3FP, SymmetryType::X_AXIS_NO_PARAM,
        barnsley3_fractal, long_mandel_per_pixel, mandel_long_setup,
        standard_fractal,
        STD_BAIL_OUT
    },

    {
        s_t_barnsleyj3+1,
        {s_real_param, s_imag_param, "", ""},
        {0.1, 0.36, 0, 0},
        HelpLabels::HT_BARNS, HelpLabels::HF_BARNSJ3, FractalFlags::OKJB|FractalFlags::BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        1, FractalType::NOFRACTAL, FractalType::BARNSLEYM3, FractalType::BARNSLEYJ3FP, SymmetryType::NONE,
        barnsley3_fractal, long_julia_per_pixel, julia_long_setup, standard_fractal,
        STD_BAIL_OUT
    },

    {
        s_t_fn_zz_+1,
        {"", "", "", ""},
        {0, 0, 0, 0},
        HelpLabels::HT_SCOTSKIN, HelpLabels::HF_FNZTIMESZ, FractalFlags::TRIG1|FractalFlags::BAILTEST,
        -4.0F, 4.0F, -3.0F, 3.0F,
        16, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::TRIGSQRFP, SymmetryType::XY_AXIS,
        trig_z_sqrd_fractal, julia_per_pixel, julia_long_setup, standard_fractal,
        STD_BAIL_OUT
    },

    {
        s_t_fn_zz_,
        {"", "", "", ""},
        {0, 0, 0, 0},
        HelpLabels::HT_SCOTSKIN, HelpLabels::HF_FNZTIMESZ, FractalFlags::TRIG1|FractalFlags::BAILTEST,
        -4.0F, 4.0F, -3.0F, 3.0F,
        0, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::TRIGSQR, SymmetryType::XY_AXIS,
        trig_z_sqrd_fp_fractal, julia_fp_per_pixel, julia_fp_setup, standard_fractal,
        STD_BAIL_OUT
    },

    {
        s_t_bifurcation,
        {s_filt, s_seed, "", ""},
        {1000.0, 0.66, 0, 0},
        HelpLabels::HT_BIF, HelpLabels::HF_BIFURCATION, FractalFlags::TRIG1|FractalFlags::NOGUESS|FractalFlags::NOTRACE|FractalFlags::NOROTATE,
        1.9F, 3.0F, 0.0F, 1.34F,
        0, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::LBIFURCATION, SymmetryType::NONE,
        bifurc_verhulst_trig, nullptr, standalone_setup, bifurcation,
        NO_BAIL_OUT
    },

    {
        s_t_fnplusfn,
        {s_re_coef_trg1, s_im_coef_trg1, s_re_coef_trg2, s_im_coef_trg2},
        {1, 0, 1, 0},
        HelpLabels::HT_SCOTSKIN, HelpLabels::HF_FNPLUSFN, FractalFlags::TRIG2|FractalFlags::BAILTEST,
        -4.0F, 4.0F, -3.0F, 3.0F,
        0, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::TRIGPLUSTRIG, SymmetryType::X_AXIS,
        trig_plus_trig_fp_fractal, other_julia_fp_per_pixel, trig_plus_trig_fp_setup,
        standard_fractal,
        LTRIG_BAIL_OUT
    },

    {
        s_t_fnfn+1,
        {"", "", "", ""},
        {0, 0, 0, 0},
        HelpLabels::HT_SCOTSKIN, HelpLabels::HF_FNTIMESFN, FractalFlags::TRIG2|FractalFlags::BAILTEST,
        -4.0F, 4.0F, -3.0F, 3.0F,
        16, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::TRIGXTRIGFP, SymmetryType::X_AXIS,
        trig_x_trig_fractal, long_julia_per_pixel, fn_x_fn_setup, standard_fractal,
        LTRIG_BAIL_OUT
    },

    {
        s_t_fnfn,
        {"", "", "", ""},
        {0, 0, 0, 0},
        HelpLabels::HT_SCOTSKIN, HelpLabels::HF_FNTIMESFN, FractalFlags::TRIG2|FractalFlags::BAILTEST,
        -4.0F, 4.0F, -3.0F, 3.0F,
        0, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::TRIGXTRIG, SymmetryType::X_AXIS,
        trig_x_trig_fp_fractal, other_julia_fp_per_pixel, fn_x_fn_setup, standard_fractal,
        LTRIG_BAIL_OUT
    },

    {
        s_t_sqr_1divfn+1,
        {"", "", "", ""},
        {0, 0, 0, 0},
        HelpLabels::HT_SCOTSKIN, HelpLabels::HF_SQROVFN, FractalFlags::TRIG1|FractalFlags::BAILTEST,
        -4.0F, 4.0F, -3.0F, 3.0F,
        16, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::SQR1OVERTRIGFP, SymmetryType::NONE,
        sqr_1_over_trig_fractal, long_julia_per_pixel, sqr_trig_setup,
        standard_fractal,
        LTRIG_BAIL_OUT
    },

    {
        s_t_sqr_1divfn,
        {"", "", "", ""},
        {0, 0, 0, 0},
        HelpLabels::HT_SCOTSKIN, HelpLabels::HF_SQROVFN, FractalFlags::TRIG1|FractalFlags::BAILTEST,
        -4.0F, 4.0F, -3.0F, 3.0F,
        0, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::SQR1OVERTRIG, SymmetryType::NONE,
        sqr_1_over_trig_fp_fractal, other_julia_fp_per_pixel, sqr_trig_setup,
        standard_fractal,
        LTRIG_BAIL_OUT
    },

    {
        s_t_fnzplusz+1,
        {s_re_coef_trg1, s_im_coef_trg1, "Real Coefficient Second Term", "Imag Coefficient Second Term"},
        {1, 0, 1, 0},
        HelpLabels::HT_SCOTSKIN, HelpLabels::HF_FNXZPLUSZ, FractalFlags::TRIG1|FractalFlags::BAILTEST,
        -4.0F, 4.0F, -3.0F, 3.0F,
        1, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::ZXTRIGPLUSZFP, SymmetryType::X_AXIS,
        z_x_trig_plus_z_fractal, julia_per_pixel, z_x_trig_plus_z_setup, standard_fractal,
        LTRIG_BAIL_OUT
    },

    {
        s_t_fnzplusz,
        {s_re_coef_trg1, s_im_coef_trg1, "Real Coefficient Second Term", "Imag Coefficient Second Term"},
        {1, 0, 1, 0},
        HelpLabels::HT_SCOTSKIN, HelpLabels::HF_FNXZPLUSZ, FractalFlags::TRIG1|FractalFlags::BAILTEST,
        -4.0F, 4.0F, -3.0F, 3.0F,
        0, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::ZXTRIGPLUSZ, SymmetryType::X_AXIS,
        z_x_trig_plus_z_fp_fractal, julia_fp_per_pixel, z_x_trig_plus_z_setup,
        standard_fractal,
        LTRIG_BAIL_OUT
    },

    {
        s_t_kamtorus,
        {s_kam_angle, s_kam_step, s_kam_stop, s_points_per_orbit},
        {1.3, .05, 1.5, 150},
        HelpLabels::HT_KAM, HelpLabels::HF_KAM, FractalFlags::NOGUESS|FractalFlags::NOTRACE,
        -1.0F, 1.0F, -.75F, .75F,
        0, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::KAM, SymmetryType::NONE,
        (VF)kam_torus_float_orbit, nullptr, orbit3d_float_setup, orbit2d_float,
        NO_BAIL_OUT
    },

    {
        s_t_kamtorus+1,
        {s_kam_angle, s_kam_step, s_kam_stop, s_points_per_orbit},
        {1.3, .05, 1.5, 150},
        HelpLabels::HT_KAM, HelpLabels::HF_KAM, FractalFlags::NOGUESS|FractalFlags::NOTRACE,
        -1.0F, 1.0F, -.75F, .75F,
        16, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::KAMFP, SymmetryType::NONE,
        (VF)kam_torus_long_orbit, nullptr, orbit3d_long_setup, orbit2d_long,
        NO_BAIL_OUT
    },

    {
        s_t_kamtorus3d,
        {s_kam_angle, s_kam_step, s_kam_stop, s_points_per_orbit},
        {1.3, .05, 1.5, 150},
        HelpLabels::HT_KAM, HelpLabels::HF_KAM, FractalFlags::NOGUESS|FractalFlags::NOTRACE|FractalFlags::NORESUME|FractalFlags::PARMS3D,
        -3.0F, 3.0F, -1.0F, 3.5F,
        0, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::KAM3D, SymmetryType::NONE,
        (VF)kam_torus_float_orbit, nullptr, orbit3d_float_setup, orbit3d_float,
        NO_BAIL_OUT
    },

    {
        s_t_kamtorus3d+1,
        {s_kam_angle, s_kam_step, s_kam_stop, s_points_per_orbit},
        {1.3, .05, 1.5, 150},
        HelpLabels::HT_KAM, HelpLabels::HF_KAM, FractalFlags::NOGUESS|FractalFlags::NOTRACE|FractalFlags::NORESUME|FractalFlags::PARMS3D,
        -3.0F, 3.0F, -1.0F, 3.5F,
        16, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::KAM3DFP, SymmetryType::NONE,
        (VF)kam_torus_long_orbit, nullptr, orbit3d_long_setup, orbit3d_long,
        NO_BAIL_OUT
    },

    {
        s_t_lambdafn+1,
        {s_real_param, s_imag_param, "", ""},
        {1.0, 0.4, 0, 0},
        HelpLabels::HT_LAMBDAFN, HelpLabels::HF_LAMBDAFN, FractalFlags::TRIG1|FractalFlags::OKJB,
        -4.0F, 4.0F, -3.0F, 3.0F,
        16, FractalType::NOFRACTAL, FractalType::MANDELTRIG, FractalType::LAMBDATRIGFP, SymmetryType::PI_SYM,
        lambda_trig_fractal, long_julia_per_pixel, lambda_trig_setup,
        standard_fractal,
        LTRIG_BAIL_OUT
    },

    {
        s_t_manfnpluszsqrd+1,
        {s_realz0, s_imagz0, "", ""},
        {0, 0, 0, 0},
        HelpLabels::HT_PICKMJ, HelpLabels::HF_MANDFNPLUSZSQRD, FractalFlags::TRIG1|FractalFlags::BAILTEST,
        -2.5F, 1.5F, -1.5F, 1.5F,
        16, FractalType::LJULTRIGPLUSZSQRD, FractalType::NOFRACTAL, FractalType::FPMANTRIGPLUSZSQRD, SymmetryType::X_AXIS_NO_PARAM,
        trig_plus_z_squared_fractal, mandel_per_pixel, mandel_long_setup,
        standard_fractal,
        STD_BAIL_OUT
    },

    {
        s_t_julfnpluszsqrd+1,
        {s_real_param, s_imag_param, "", ""},
        {-0.5, 0.5, 0, 0},
        HelpLabels::HT_PICKMJ, HelpLabels::HF_JULFNPLUSZSQRD, FractalFlags::TRIG1|FractalFlags::OKJB|FractalFlags::BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        16, FractalType::NOFRACTAL, FractalType::LMANTRIGPLUSZSQRD, FractalType::FPJULTRIGPLUSZSQRD, SymmetryType::NONE,
        trig_plus_z_squared_fractal, julia_per_pixel, julia_fn_plus_z_sqrd_setup,
        standard_fractal,
        STD_BAIL_OUT
    },

    {
        s_t_manfnpluszsqrd,
        {s_realz0, s_imagz0, "", ""},
        {0, 0, 0, 0},
        HelpLabels::HT_PICKMJ, HelpLabels::HF_MANDFNPLUSZSQRD, FractalFlags::TRIG1|FractalFlags::BAILTEST,
        -2.5F, 1.5F, -1.5F, 1.5F,
        0, FractalType::FPJULTRIGPLUSZSQRD, FractalType::NOFRACTAL, FractalType::LMANTRIGPLUSZSQRD, SymmetryType::X_AXIS_NO_PARAM,
        trig_plus_z_squared_fp_fractal, mandel_fp_per_pixel, mandel_fp_setup,
        standard_fractal,
        STD_BAIL_OUT
    },

    {
        s_t_julfnpluszsqrd,
        {s_real_param, s_imag_param, "", ""},
        {-0.5, 0.5, 0, 0},
        HelpLabels::HT_PICKMJ, HelpLabels::HF_JULFNPLUSZSQRD, FractalFlags::TRIG1|FractalFlags::OKJB|FractalFlags::BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, FractalType::NOFRACTAL, FractalType::FPMANTRIGPLUSZSQRD, FractalType::LJULTRIGPLUSZSQRD, SymmetryType::NONE,
        trig_plus_z_squared_fp_fractal, julia_fp_per_pixel, julia_fn_plus_z_sqrd_setup,
        standard_fractal,
        STD_BAIL_OUT
    },

    {
        s_t_lambdafn,
        {s_real_param, s_imag_param, "", ""},
        {1.0, 0.4, 0, 0},
        HelpLabels::HT_LAMBDAFN, HelpLabels::HF_LAMBDAFN, FractalFlags::TRIG1|FractalFlags::OKJB,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, FractalType::NOFRACTAL, FractalType::MANDELTRIGFP, FractalType::LAMBDATRIG, SymmetryType::PI_SYM,
        lambda_trig_fp_fractal, other_julia_fp_per_pixel, lambda_trig_setup,
        standard_fractal,
        LTRIG_BAIL_OUT
    },

    {
        s_t_mandelfn+1,
        {s_realz0, s_imagz0, "", ""},
        {0, 0, 0, 0},
        HelpLabels::HT_MANDFN, HelpLabels::HF_MANDFN, FractalFlags::TRIG1,
        -8.0F, 8.0F, -6.0F, 6.0F,
        16, FractalType::LAMBDATRIG, FractalType::NOFRACTAL, FractalType::MANDELTRIGFP, SymmetryType::XY_AXIS_NO_PARAM,
        lambda_trig_fractal, long_mandel_per_pixel, mandel_trig_setup,
        standard_fractal,
        LTRIG_BAIL_OUT
    },

    {
        s_t_manzpower+1,
        {s_realz0, s_imagz0, s_exponent, s_im_exponent},
        {0, 0, 2, 0},
        HelpLabels::HT_PICKMJ, HelpLabels::HF_MANZPOWER, FractalFlags::BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        1, FractalType::LJULIAZPOWER, FractalType::NOFRACTAL, FractalType::FPMANDELZPOWER, SymmetryType::X_AXIS_NO_IMAG,
        long_z_power_fractal, long_mandel_per_pixel, mandel_long_setup,
        standard_fractal,
        STD_BAIL_OUT
    },

    {
        s_t_julzpower+1,
        {s_real_param, s_imag_param, s_exponent, s_im_exponent},
        {0.3, 0.6, 2, 0},
        HelpLabels::HT_PICKMJ, HelpLabels::HF_JULZPOWER, FractalFlags::OKJB|FractalFlags::BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        1, FractalType::NOFRACTAL, FractalType::LMANDELZPOWER, FractalType::FPJULIAZPOWER, SymmetryType::ORIGIN,
        long_z_power_fractal, long_julia_per_pixel, julia_long_setup,
        standard_fractal,
        STD_BAIL_OUT
    },

    {
        s_t_manzpower,
        {s_realz0, s_imagz0, s_exponent, s_im_exponent},
        {0, 0, 2, 0},
        HelpLabels::HT_PICKMJ, HelpLabels::HF_MANZPOWER, FractalFlags::BAILTEST|FractalFlags::BF_MATH|FractalFlags::PERTURB,
        -2.5F, 1.5F, -1.5F, 1.5F,
        0, FractalType::FPJULIAZPOWER, FractalType::NOFRACTAL, FractalType::LMANDELZPOWER, SymmetryType::X_AXIS_NO_IMAG,
        float_z_power_fractal, other_mandel_fp_per_pixel, mandel_fp_setup,
        standard_fractal,
        STD_BAIL_OUT,
        mandel_z_power_ref_pt, mandel_z_power_ref_pt_bf, mandel_z_power_perturb
    },

    {
        s_t_julzpower,
        {s_real_param, s_imag_param, s_exponent, s_im_exponent},
        {0.3, 0.6, 2, 0},
        HelpLabels::HT_PICKMJ, HelpLabels::HF_JULZPOWER, FractalFlags::OKJB|FractalFlags::BAILTEST|FractalFlags::BF_MATH,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, FractalType::NOFRACTAL, FractalType::FPMANDELZPOWER, FractalType::LJULIAZPOWER, SymmetryType::ORIGIN,
        float_z_power_fractal, other_julia_fp_per_pixel, julia_fp_setup,
        standard_fractal,
        STD_BAIL_OUT
    },

    {
        "manzzpwr",
        {s_realz0, s_imagz0, s_exponent, ""},
        {0, 0, 2, 0},
        HelpLabels::HT_PICKMJ, HelpLabels::HF_MANZZPWR, FractalFlags::BAILTEST|FractalFlags::BF_MATH,
        -2.5F, 1.5F, -1.5F, 1.5F,
        0, FractalType::FPJULZTOZPLUSZPWR, FractalType::NOFRACTAL, FractalType::NOFRACTAL, SymmetryType::X_AXIS_NO_PARAM,
        float_z_to_z_plus_z_pwr_fractal, other_mandel_fp_per_pixel, mandel_fp_setup,
        standard_fractal,
        STD_BAIL_OUT
    },

    {
        "julzzpwr",
        {s_real_param, s_imag_param, s_exponent, ""},
        {-0.3, 0.3, 2, 0},
        HelpLabels::HT_PICKMJ, HelpLabels::HF_JULZZPWR, FractalFlags::OKJB|FractalFlags::BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, FractalType::NOFRACTAL, FractalType::FPMANZTOZPLUSZPWR, FractalType::NOFRACTAL, SymmetryType::NONE,
        float_z_to_z_plus_z_pwr_fractal, other_julia_fp_per_pixel, julia_fp_setup,
        standard_fractal,
        STD_BAIL_OUT
    },

    {
        s_t_manfnplusexp+1,
        {s_realz0, s_imagz0, "", ""},
        {0, 0, 0, 0},
        HelpLabels::HT_PICKMJ, HelpLabels::HF_MANDFNPLUSEXP, FractalFlags::TRIG1|FractalFlags::BAILTEST,
        -8.0F, 8.0F, -6.0F, 6.0F,
        16, FractalType::LJULTRIGPLUSEXP, FractalType::NOFRACTAL, FractalType::FPMANTRIGPLUSEXP, SymmetryType::X_AXIS_NO_PARAM,
        long_trig_plus_exponent_fractal, long_mandel_per_pixel, mandel_long_setup,
        standard_fractal,
        STD_BAIL_OUT
    },

    {
        s_t_julfnplusexp+1,
        {s_real_param, s_imag_param, "", ""},
        {0, 0, 0, 0},
        HelpLabels::HT_PICKMJ, HelpLabels::HF_JULFNPLUSEXP, FractalFlags::TRIG1|FractalFlags::OKJB|FractalFlags::BAILTEST,
        -4.0F, 4.0F, -3.0F, 3.0F,
        16, FractalType::NOFRACTAL, FractalType::LMANTRIGPLUSEXP, FractalType::FPJULTRIGPLUSEXP, SymmetryType::NONE,
        long_trig_plus_exponent_fractal, long_julia_per_pixel, julia_long_setup,
        standard_fractal,
        STD_BAIL_OUT
    },

    {
        s_t_manfnplusexp,
        {s_realz0, s_imagz0, "", ""},
        {0, 0, 0, 0},
        HelpLabels::HT_PICKMJ, HelpLabels::HF_MANDFNPLUSEXP, FractalFlags::TRIG1|FractalFlags::BAILTEST,
        -8.0F, 8.0F, -6.0F, 6.0F,
        0, FractalType::FPJULTRIGPLUSEXP, FractalType::NOFRACTAL, FractalType::LMANTRIGPLUSEXP, SymmetryType::X_AXIS_NO_PARAM,
        float_trig_plus_exponent_fractal, other_mandel_fp_per_pixel, mandel_fp_setup,
        standard_fractal,
        STD_BAIL_OUT
    },

    {
        s_t_julfnplusexp,
        {s_real_param, s_imag_param, "", ""},
        {0, 0, 0, 0},
        HelpLabels::HT_PICKMJ, HelpLabels::HF_JULFNPLUSEXP, FractalFlags::TRIG1|FractalFlags::OKJB|FractalFlags::BAILTEST,
        -4.0F, 4.0F, -3.0F, 3.0F,
        0, FractalType::NOFRACTAL, FractalType::FPMANTRIGPLUSEXP, FractalType::LJULTRIGPLUSEXP, SymmetryType::NONE,
        float_trig_plus_exponent_fractal, other_julia_fp_per_pixel, julia_fp_setup,
        standard_fractal,
        STD_BAIL_OUT
    },

    {
        s_t_popcorn,
        {s_step_x, s_step_y, s_constant_x, s_constant_y},
        {0.05, 0, 3.00, 0},
        HelpLabels::HT_POPCORN, HelpLabels::HF_POPCORN, FractalFlags::NOGUESS|FractalFlags::NOTRACE|FractalFlags::TRIG4,
        -3.0F, 3.0F, -2.25F, 2.25F,
        0, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::LPOPCORN, SymmetryType::NO_PLOT,
        popcorn_fractal_fn, other_julia_fp_per_pixel, julia_fp_setup, popcorn,
        STD_BAIL_OUT
    },

    {
        s_t_popcorn+1,
        {s_step_x, s_step_y, s_constant_x, s_constant_y},
        {0.05, 0, 3.00, 0},
        HelpLabels::HT_POPCORN, HelpLabels::HF_POPCORN, FractalFlags::NOGUESS|FractalFlags::NOTRACE|FractalFlags::TRIG4,
        -3.0F, 3.0F, -2.25F, 2.25F,
        16, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::FPPOPCORN, SymmetryType::NO_PLOT,
        long_popcorn_fractal_fn, long_julia_per_pixel, julia_long_setup, popcorn,
        STD_BAIL_OUT
    },

    {
        s_t_lorenz,
        {s_time_step, "a", "b", "c"},
        {.02, 5, 15, 1},
        HelpLabels::HT_LORENZ, HelpLabels::HF_LORENZ, FractalFlags::NOGUESS|FractalFlags::NOTRACE|FractalFlags::INFCALC,
        -15.0F, 15.0F, 0.0F, 30.0F,
        0, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::LLORENZ, SymmetryType::NONE,
        (VF)lorenz3d_float_orbit, nullptr, orbit3d_float_setup, orbit2d_float,
        NO_BAIL_OUT
    },

    {
        s_t_lorenz+1,
        {s_time_step, "a", "b", "c"},
        {.02, 5, 15, 1},
        HelpLabels::HT_LORENZ, HelpLabels::HF_LORENZ, FractalFlags::NOGUESS|FractalFlags::NOTRACE|FractalFlags::INFCALC,
        -15.0F, 15.0F, 0.0F, 30.0F,
        16, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::FPLORENZ, SymmetryType::NONE,
        (VF)lorenz3d_long_orbit, nullptr, orbit3d_long_setup, orbit2d_long,
        NO_BAIL_OUT
    },

    {
        s_t_lorenz3d+1,
        {s_time_step, "a", "b", "c"},
        {.02, 5, 15, 1},
        HelpLabels::HT_LORENZ, HelpLabels::HF_LORENZ, FractalFlags::NOGUESS|FractalFlags::NOTRACE|FractalFlags::NORESUME|FractalFlags::PARMS3D|FractalFlags::INFCALC,
        -30.0F, 30.0F, -30.0F, 30.0F,
        16, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::FPLORENZ3D, SymmetryType::NONE,
        (VF)lorenz3d_long_orbit, nullptr, orbit3d_long_setup, orbit3d_long,
        NO_BAIL_OUT
    },

    {
        s_t_newton+1,
        {s_newt_degree, "", "", ""},
        {3, 0, 0, 0},
        HelpLabels::HT_NEWT, HelpLabels::HF_NEWT, FractalFlags::NONE,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::NEWTON, SymmetryType::X_AXIS,
        mpc_newton_fractal, mpc_julia_per_pixel, newton_setup, standard_fractal,
        NO_BAIL_OUT
    },

    {
        s_t_newtbasin+1,
        {s_newt_degree, "Enter non-zero value for stripes", "", ""},
        {3, 0, 0, 0},
        HelpLabels::HT_NEWTBAS, HelpLabels::HF_NEWTBAS, FractalFlags::NONE,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::NEWTBASIN, SymmetryType::NONE,
        mpc_newton_fractal, mpc_julia_per_pixel, newton_setup, standard_fractal,
        NO_BAIL_OUT
    },

    {
        "complexnewton",
        {"Real part of Degree", "Imag part of Degree", "Real part of Root", "Imag part of Root"},
        {3, 0, 1, 0},
        HelpLabels::HT_NEWTCMPLX, HelpLabels::HF_COMPLEXNEWT, FractalFlags::NONE,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::NOFRACTAL, SymmetryType::NONE,
        complex_newton, other_julia_fp_per_pixel, complex_newton_setup,
        standard_fractal,
        NO_BAIL_OUT
    },

    {
        "complexbasin",
        {"Real part of Degree", "Imag part of Degree", "Real part of Root", "Imag part of Root"},
        {3, 0, 1, 0},
        HelpLabels::HT_NEWTCMPLX, HelpLabels::HF_COMPLEXNEWT, FractalFlags::NONE,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::NOFRACTAL, SymmetryType::NONE,
        complex_basin, other_julia_fp_per_pixel, complex_newton_setup,
        standard_fractal,
        NO_BAIL_OUT
    },

    {
        "cmplxmarksmand",
        {s_realz0, s_imagz0, s_exponent, s_im_exponent},
        {0, 0, 1, 0},
        HelpLabels::HT_MARKS, HelpLabels::HF_CMPLXMARKSMAND, FractalFlags::BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, FractalType::COMPLEXMARKSJUL, FractalType::NOFRACTAL, FractalType::NOFRACTAL, SymmetryType::NONE,
        marks_cplx_mand, marks_cplx_mand_perp, mandel_fp_setup, standard_fractal,
        STD_BAIL_OUT
    },

    {
        "cmplxmarksjul",
        {s_real_param, s_imag_param, s_exponent, s_im_exponent},
        {0.3, 0.6, 1, 0},
        HelpLabels::HT_MARKS, HelpLabels::HF_CMPLXMARKSJUL, FractalFlags::BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, FractalType::NOFRACTAL, FractalType::COMPLEXMARKSMAND, FractalType::NOFRACTAL, SymmetryType::NONE,
        marks_cplx_mand, julia_fp_per_pixel, julia_fp_setup, standard_fractal,
        STD_BAIL_OUT
    },

    {
        s_t_formula+1,
        {s_p1_real, s_p1_imag, s_p2_real, s_p2_imag},
        {0, 0, 0, 0},
        HelpLabels::HT_FORMULA, HelpLabels::SPECIAL_FORMULA, FractalFlags::MORE,
        -2.0F, 2.0F, -1.5F, 1.5F,
        1, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::FFORMULA, SymmetryType::SETUP,
        formula, form_per_pixel, formula_setup_l, standard_fractal,
        0
    },

    {
        s_t_formula,
        {s_p1_real, s_p1_imag, s_p2_real, s_p2_imag},
        {0, 0, 0, 0},
        HelpLabels::HT_FORMULA, HelpLabels::SPECIAL_FORMULA, FractalFlags::MORE,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::FORMULA, SymmetryType::SETUP,
        formula, form_per_pixel, formula_setup_fp, standard_fractal,
        0
    },

    {
        s_t_sierpinski,
        {"", "", "", ""},
        {0, 0, 0, 0},
        HelpLabels::HT_SIER, HelpLabels::HF_SIER, FractalFlags::NONE,
        -4.0F/3.0F, 96.0F/45.0F, -0.9F, 1.7F,
        0, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::SIERPINSKI, SymmetryType::NONE,
        sierpinski_fp_fractal, other_julia_fp_per_pixel, sierpinski_fp_setup,
        standard_fractal,
        127
    },

    {
        s_t_lambda,
        {s_real_param, s_imag_param, "", ""},
        {0.85, 0.6, 0, 0},
        HelpLabels::HT_LAMBDA, HelpLabels::HF_LAMBDA, FractalFlags::OKJB|FractalFlags::BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, FractalType::NOFRACTAL, FractalType::MANDELLAMBDAFP, FractalType::LAMBDA, SymmetryType::NONE,
        lambda_fp_fractal, julia_fp_per_pixel, julia_fp_setup, standard_fractal,
        STD_BAIL_OUT
    },

    {
        s_t_barnsleym1,
        {s_realz0, s_imagz0, "", ""},
        {0, 0, 0, 0},
        HelpLabels::HT_BARNS, HelpLabels::HF_BARNSM1, FractalFlags::BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, FractalType::BARNSLEYJ1FP, FractalType::NOFRACTAL, FractalType::BARNSLEYM1, SymmetryType::XY_AXIS_NO_PARAM,
        barnsley1_fp_fractal, other_mandel_fp_per_pixel, mandel_fp_setup,
        standard_fractal,
        STD_BAIL_OUT
    },

    {
        s_t_barnsleyj1,
        {s_real_param, s_imag_param, "", ""},
        {0.6, 1.1, 0, 0},
        HelpLabels::HT_BARNS, HelpLabels::HF_BARNSJ1, FractalFlags::OKJB|FractalFlags::BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, FractalType::NOFRACTAL, FractalType::BARNSLEYM1FP, FractalType::BARNSLEYJ1, SymmetryType::ORIGIN,
        barnsley1_fp_fractal, other_julia_fp_per_pixel, julia_fp_setup,
        standard_fractal,
        STD_BAIL_OUT
    },

    {
        s_t_barnsleym2,
        {s_realz0, s_imagz0, "", ""},
        {0, 0, 0, 0},
        HelpLabels::HT_BARNS, HelpLabels::HF_BARNSM2, FractalFlags::BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, FractalType::BARNSLEYJ2FP, FractalType::NOFRACTAL, FractalType::BARNSLEYM2, SymmetryType::Y_AXIS_NO_PARAM,
        barnsley2_fp_fractal, other_mandel_fp_per_pixel, mandel_fp_setup,
        standard_fractal,
        STD_BAIL_OUT
    },

    {
        s_t_barnsleyj2,
        {s_real_param, s_imag_param, "", ""},
        {0.6, 1.1, 0, 0},
        HelpLabels::HT_BARNS, HelpLabels::HF_BARNSJ2, FractalFlags::OKJB|FractalFlags::BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, FractalType::NOFRACTAL, FractalType::BARNSLEYM2FP, FractalType::BARNSLEYJ2, SymmetryType::ORIGIN,
        barnsley2_fp_fractal, other_julia_fp_per_pixel, julia_fp_setup,
        standard_fractal,
        STD_BAIL_OUT
    },

    {
        s_t_barnsleym3,
        {s_realz0, s_imagz0, "", ""},
        {0, 0, 0, 0},
        HelpLabels::HT_BARNS, HelpLabels::HF_BARNSM3, FractalFlags::BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, FractalType::BARNSLEYJ3FP, FractalType::NOFRACTAL, FractalType::BARNSLEYM3, SymmetryType::X_AXIS_NO_PARAM,
        barnsley3_fp_fractal, other_mandel_fp_per_pixel, mandel_fp_setup,
        standard_fractal,
        STD_BAIL_OUT
    },

    {
        s_t_barnsleyj3,
        {s_real_param, s_imag_param, "", ""},
        {0.6, 1.1, 0, 0},
        HelpLabels::HT_BARNS, HelpLabels::HF_BARNSJ3, FractalFlags::OKJB|FractalFlags::BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, FractalType::NOFRACTAL, FractalType::BARNSLEYM3FP, FractalType::BARNSLEYJ3, SymmetryType::NONE,
        barnsley3_fp_fractal, other_julia_fp_per_pixel, julia_fp_setup,
        standard_fractal,
        STD_BAIL_OUT
    },

    {
        s_t_mandellambda,
        {s_realz0, s_imagz0, "", ""},
        {0, 0, 0, 0},
        HelpLabels::HT_MLAMBDA, HelpLabels::HF_MLAMBDA, FractalFlags::BAILTEST,
        -3.0F, 5.0F, -3.0F, 3.0F,
        0, FractalType::LAMBDAFP, FractalType::NOFRACTAL, FractalType::MANDELLAMBDA, SymmetryType::X_AXIS_NO_PARAM,
        lambda_fp_fractal, mandel_fp_per_pixel, mandel_fp_setup, standard_fractal,
        STD_BAIL_OUT
    },

    {
        s_t_julibrot+1,
        {"", "", "", ""},
        {0, 0, 0, 0},
        HelpLabels::HT_JULIBROT, HelpLabels::NONE, FractalFlags::NOGUESS|FractalFlags::NOTRACE|FractalFlags::NOROTATE|FractalFlags::NORESUME,
        -2.0F, 2.0F, -1.5F, 1.5F,
        1, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::JULIBROTFP, SymmetryType::NONE,
        julia_fractal, jb_per_pixel, julibrot_setup, std_4d_fractal,
        STD_BAIL_OUT
    },

    {
        s_t_lorenz3d,
        {s_time_step, "a", "b", "c"},
        {.02, 5, 15, 1},
        HelpLabels::HT_LORENZ, HelpLabels::HF_LORENZ, FractalFlags::NOGUESS|FractalFlags::NOTRACE|FractalFlags::NORESUME|FractalFlags::PARMS3D|FractalFlags::INFCALC,
        -30.0F, 30.0F, -30.0F, 30.0F,
        0, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::LLORENZ3D, SymmetryType::NONE,
        (VF)lorenz3d_float_orbit, nullptr, orbit3d_float_setup, orbit3d_float,
        NO_BAIL_OUT
    },

    {
        s_t_rossler3d+1,
        {s_time_step, "a", "b", "c"},
        {.04, .2, .2, 5.7},
        HelpLabels::HT_ROSS, HelpLabels::HF_ROSS, FractalFlags::NOGUESS|FractalFlags::NOTRACE|FractalFlags::NORESUME|FractalFlags::PARMS3D|FractalFlags::INFCALC,
        -30.0F, 30.0F, -20.0F, 40.0F,
        16, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::FPROSSLER, SymmetryType::NONE,
        (VF)rossler_long_orbit, nullptr, orbit3d_long_setup, orbit3d_long,
        NO_BAIL_OUT
    },

    {
        s_t_rossler3d,
        {s_time_step, "a", "b", "c"},
        {.04, .2, .2, 5.7},
        HelpLabels::HT_ROSS, HelpLabels::HF_ROSS, FractalFlags::NOGUESS|FractalFlags::NOTRACE|FractalFlags::NORESUME|FractalFlags::PARMS3D|FractalFlags::INFCALC,
        -30.0F, 30.0F, -20.0F, 40.0F,
        0, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::LROSSLER, SymmetryType::NONE,
        (VF)rossler_float_orbit, nullptr, orbit3d_float_setup, orbit3d_float,
        NO_BAIL_OUT
    },

    {
        s_t_henon+1,
        {"a", "b", "", ""},
        {1.4, .3, 0, 0},
        HelpLabels::HT_HENON, HelpLabels::HF_HENON, FractalFlags::NOGUESS|FractalFlags::NOTRACE|FractalFlags::INFCALC,
        -1.4F, 1.4F, -.5F, .5F,
        16, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::FPHENON, SymmetryType::NONE,
        (VF)henon_long_orbit, nullptr, orbit3d_long_setup, orbit2d_long,
        NO_BAIL_OUT
    },

    {
        s_t_henon,
        {"a", "b", "", ""},
        {1.4, .3, 0, 0},
        HelpLabels::HT_HENON, HelpLabels::HF_HENON, FractalFlags::NOGUESS|FractalFlags::NOTRACE|FractalFlags::INFCALC,
        -1.4F, 1.4F, -.5F, .5F,
        0, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::LHENON, SymmetryType::NONE,
        (VF)henon_float_orbit, nullptr, orbit3d_float_setup, orbit2d_float,
        NO_BAIL_OUT
    },

    {
        "pickover",
        {"a", "b", "c", "d"},
        {2.24, .43, -.65, -2.43},
        HelpLabels::HT_PICK, HelpLabels::HF_PICKOVER, FractalFlags::NOGUESS|FractalFlags::NOTRACE|FractalFlags::NORESUME|FractalFlags::PARMS3D,
        -8.0F/3.0F, 8.0F/3.0F, -2.0F, 2.0F,
        0, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::NOFRACTAL, SymmetryType::NONE,
        (VF)pickover_float_orbit, nullptr, orbit3d_float_setup, orbit3d_float,
        NO_BAIL_OUT
    },

    {
        "gingerbreadman",
        {"Initial x", "Initial y", "", ""},
        {-.1, 0, 0, 0},
        HelpLabels::HT_GINGER, HelpLabels::HF_GINGER, FractalFlags::NOGUESS|FractalFlags::NOTRACE|FractalFlags::INFCALC,
        -4.5F, 8.5F, -4.5F, 8.5F,
        0, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::NOFRACTAL, SymmetryType::NONE,
        (VF)ginger_bread_float_orbit, nullptr, orbit3d_float_setup, orbit2d_float,
        NO_BAIL_OUT
    },

    {
        "diffusion",
        {
            "+Border size",
            "+Type (0=Central,1=Falling,2=Square Cavity)",
            "+Color change rate (0=Random)",
            ""
        },
        {10, 0, 0, 0},
        HelpLabels::HT_DIFFUS, HelpLabels::HF_DIFFUS, FractalFlags::NOZOOM|FractalFlags::NOGUESS|FractalFlags::NOTRACE,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::NOFRACTAL, SymmetryType::NONE,
        nullptr, nullptr, standalone_setup, diffusion,
        NO_BAIL_OUT
    },

    {
        s_t_unity,
        {"", "", "", ""},
        {0, 0, 0, 0},
        HelpLabels::HT_UNITY, HelpLabels::HF_UNITY, FractalFlags::NONE,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::UNITY, SymmetryType::XY_AXIS,
        unity_fp_fractal, other_julia_fp_per_pixel, standard_setup, standard_fractal,
        NO_BAIL_OUT
    },

    {
        s_t_spider,
        {s_realz0, s_imagz0, "", ""},
        {0, 0, 0, 0},
        HelpLabels::HT_SCOTSKIN, HelpLabels::HF_SPIDER, FractalFlags::BAILTEST,
        -2.5F, 1.5F, -1.5F, 1.5F,
        0, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::SPIDER, SymmetryType::X_AXIS_NO_PARAM,
        spider_fp_fractal, mandel_fp_per_pixel, mandel_fp_setup, standard_fractal,
        STD_BAIL_OUT
    },

    {
        s_t_spider+1,
        {s_realz0, s_imagz0, "", ""},
        {0, 0, 0, 0},
        HelpLabels::HT_SCOTSKIN, HelpLabels::HF_SPIDER, FractalFlags::BAILTEST,
        -2.5F, 1.5F, -1.5F, 1.5F,
        1, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::SPIDERFP, SymmetryType::X_AXIS_NO_PARAM,
        spider_fractal, mandel_per_pixel, mandel_long_setup, standard_fractal,
        STD_BAIL_OUT
    },

    {
        "tetrate",
        {s_realz0, s_imagz0, "", ""},
        {0, 0, 0, 0},
        HelpLabels::HT_SCOTSKIN, HelpLabels::HF_TETRATE, FractalFlags::BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::NOFRACTAL, SymmetryType::X_AXIS_NO_IMAG,
        tetrate_fp_fractal, other_mandel_fp_per_pixel, mandel_fp_setup,
        standard_fractal,
        STD_BAIL_OUT
    },

    {
        "magnet1m",
        {s_realz0, s_imagz0, "", ""},
        {0, 0, 0, 0},
        HelpLabels::HT_MAGNET, HelpLabels::HF_MAGM1, FractalFlags::NONE,
        -4.0F, 4.0F, -3.0F, 3.0F,
        0, FractalType::MAGNET1J, FractalType::NOFRACTAL, FractalType::NOFRACTAL, SymmetryType::X_AXIS_NO_PARAM,
        magnet1_fractal, mandel_fp_per_pixel, mandel_fp_setup, standard_fractal,
        100
    },

    {
        "magnet1j",
        {s_real_param, s_imag_param, "", ""},
        {0, 0, 0, 0},
        HelpLabels::HT_MAGNET, HelpLabels::HF_MAGJ1, FractalFlags::NONE,
        -8.0F, 8.0F, -6.0F, 6.0F,
        0, FractalType::NOFRACTAL, FractalType::MAGNET1M, FractalType::NOFRACTAL, SymmetryType::X_AXIS_NO_IMAG,
        magnet1_fractal, julia_fp_per_pixel, julia_fp_setup, standard_fractal,
        100
    },

    {
        "magnet2m",
        {s_realz0, s_imagz0, "", ""},
        {0, 0, 0, 0},
        HelpLabels::HT_MAGNET, HelpLabels::HF_MAGM2, FractalFlags::NONE,
        -1.5F, 3.7F, -1.95F, 1.95F,
        0, FractalType::MAGNET2J, FractalType::NOFRACTAL, FractalType::NOFRACTAL, SymmetryType::X_AXIS_NO_PARAM,
        magnet2_fractal, mandel_fp_per_pixel, mandel_fp_setup, standard_fractal,
        100
    },

    {
        "magnet2j",
        {s_real_param, s_imag_param, "", ""},
        {0, 0, 0, 0},
        HelpLabels::HT_MAGNET, HelpLabels::HF_MAGJ2, FractalFlags::NONE,
        -8.0F, 8.0F, -6.0F, 6.0F,
        0, FractalType::NOFRACTAL, FractalType::MAGNET2M, FractalType::NOFRACTAL, SymmetryType::X_AXIS_NO_IMAG,
        magnet2_fractal, julia_fp_per_pixel, julia_fp_setup, standard_fractal,
        100
    },

    {
        s_t_bifurcation+1,
        {s_filt, s_seed, "", ""},
        {1000.0, 0.66, 0, 0},
        HelpLabels::HT_BIF, HelpLabels::HF_BIFURCATION, FractalFlags::TRIG1|FractalFlags::NOGUESS|FractalFlags::NOTRACE|FractalFlags::NOROTATE,
        1.9F, 3.0F, 0.0F, 1.34F,
        1, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::BIFURCATION, SymmetryType::NONE,
        long_bifurc_verhulst_trig, nullptr, standalone_setup, bifurcation,
        NO_BAIL_OUT
    },

    {
        s_t_biflambda+1,
        {s_filt, s_seed, "", ""},
        {1000.0, 0.66, 0, 0},
        HelpLabels::HT_BIF, HelpLabels::HF_BIFLAMBDA, FractalFlags::TRIG1|FractalFlags::NOGUESS|FractalFlags::NOTRACE|FractalFlags::NOROTATE,
        -2.0F, 4.0F, -1.0F, 2.0F,
        1, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::BIFLAMBDA, SymmetryType::NONE,
        long_bifurc_lambda_trig, nullptr, standalone_setup, bifurcation,
        NO_BAIL_OUT
    },

    {
        s_t_biflambda,
        {s_filt, s_seed, "", ""},
        {1000.0, 0.66, 0, 0},
        HelpLabels::HT_BIF, HelpLabels::HF_BIFLAMBDA, FractalFlags::TRIG1|FractalFlags::NOGUESS|FractalFlags::NOTRACE|FractalFlags::NOROTATE,
        -2.0F, 4.0F, -1.0F, 2.0F,
        0, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::LBIFLAMBDA, SymmetryType::NONE,
        bifurc_lambda_trig, nullptr, standalone_setup, bifurcation,
        NO_BAIL_OUT
    },

    {
        s_t_bifplussinpi,
        {s_filt, s_seed, "", ""},
        {1000.0, 0.66, 0, 0},
        HelpLabels::HT_BIF, HelpLabels::HF_BIFPLUSSINPI, FractalFlags::TRIG1|FractalFlags::NOGUESS|FractalFlags::NOTRACE|FractalFlags::NOROTATE,
        0.275F, 1.45F, 0.0F, 2.0F,
        0, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::LBIFADSINPI, SymmetryType::NONE,
        bifurc_add_trig_pi, nullptr, standalone_setup, bifurcation,
        NO_BAIL_OUT
    },

    {
        s_t_bifeqsinpi,
        {s_filt, s_seed, "", ""},
        {1000.0, 0.66, 0, 0},
        HelpLabels::HT_BIF, HelpLabels::HF_BIFEQSINPI, FractalFlags::TRIG1|FractalFlags::NOGUESS|FractalFlags::NOTRACE|FractalFlags::NOROTATE,
        -2.5F, 2.5F, -3.5F, 3.5F,
        0, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::LBIFEQSINPI, SymmetryType::NONE,
        bifurc_set_trig_pi, nullptr, standalone_setup, bifurcation,
        NO_BAIL_OUT
    },

    {
        s_t_popcornjul,
        {s_step_x, s_step_y, s_constant_x, s_constant_y},
        {0.05, 0, 3.00, 0},
        HelpLabels::HT_POPCORN, HelpLabels::HF_POPCJUL, FractalFlags::TRIG4,
        -3.0F, 3.0F, -2.25F, 2.25F,
        0, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::LPOPCORNJUL, SymmetryType::NONE,
        popcorn_fractal_fn, other_julia_fp_per_pixel, julia_fp_setup, standard_fractal,
        STD_BAIL_OUT
    },

    {
        s_t_popcornjul+1,
        {s_step_x, s_step_y, s_constant_x, s_constant_y},
        {0.05, 0, 3.0, 0},
        HelpLabels::HT_POPCORN, HelpLabels::HF_POPCJUL, FractalFlags::TRIG4,
        -3.0F, 3.0F, -2.25F, 2.25F,
        16, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::FPPOPCORNJUL, SymmetryType::NONE,
        long_popcorn_fractal_fn, long_julia_per_pixel, julia_long_setup, standard_fractal,
        STD_BAIL_OUT
    },

    {
        "lsystem",
        {"+Order", "", "", ""},
        {2, 0, 0, 0},
        HelpLabels::HT_LSYS, HelpLabels::SPECIAL_L_SYSTEM, FractalFlags::NOZOOM|FractalFlags::NORESUME|FractalFlags::NOGUESS|FractalFlags::NOTRACE,
        -1.0F, 1.0F, -1.0F, 1.0F,
        1, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::NOFRACTAL, SymmetryType::NONE,
        nullptr, nullptr, standalone_setup, lsystem,
        NO_BAIL_OUT
    },

    {
        s_t_manowarj,
        {s_real_param, s_imag_param, "", ""},
        {0, 0, 0, 0},
        HelpLabels::HT_SCOTSKIN, HelpLabels::HF_MANOWARJ, FractalFlags::OKJB|FractalFlags::BAILTEST,
        -2.5F, 1.5F, -1.5F, 1.5F,
        0, FractalType::NOFRACTAL, FractalType::MANOWARFP, FractalType::MANOWARJ, SymmetryType::NONE,
        man_o_war_fp_fractal, julia_fp_per_pixel, julia_fp_setup, standard_fractal,
        STD_BAIL_OUT
    },

    {
        s_t_manowarj+1,
        {s_real_param, s_imag_param, "", ""},
        {0, 0, 0, 0},
        HelpLabels::HT_SCOTSKIN, HelpLabels::HF_MANOWARJ, FractalFlags::OKJB|FractalFlags::BAILTEST,
        -2.5F, 1.5F, -1.5F, 1.5F,
        1, FractalType::NOFRACTAL, FractalType::MANOWAR, FractalType::MANOWARJFP, SymmetryType::NONE,
        man_o_war_fractal, julia_per_pixel, julia_long_setup, standard_fractal,
        STD_BAIL_OUT
    },

    {
        s_t_fn_z_plusfn_pix_,
        {s_realz0, s_imagz0, s_re_coef_trg2, s_im_coef_trg2},
        {0, 0, 1, 0},
        HelpLabels::HT_SCOTSKIN, HelpLabels::HF_FNPLUSFNPIX, FractalFlags::TRIG2|FractalFlags::BAILTEST,
        -3.6F, 3.6F, -2.7F, 2.7F,
        0, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::FNPLUSFNPIXLONG, SymmetryType::NONE,
        richard_8_fp_fractal, other_richard_8_fp_per_pixel, mandel_fp_setup,
        standard_fractal,
        LTRIG_BAIL_OUT
    },

    {
        s_t_fn_z_plusfn_pix_+1,
        {s_realz0, s_imagz0, s_re_coef_trg2, s_im_coef_trg2},
        {0, 0, 1, 0},
        HelpLabels::HT_SCOTSKIN, HelpLabels::HF_FNPLUSFNPIX, FractalFlags::TRIG2|FractalFlags::BAILTEST,
        -3.6F, 3.6F, -2.7F, 2.7F,
        1, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::FNPLUSFNPIXFP, SymmetryType::NONE,
        richard_8_fractal, long_richard_8_per_pixel, mandel_long_setup,
        standard_fractal,
        LTRIG_BAIL_OUT
    },

    {
        s_t_marksmandelpwr,
        {s_realz0, s_imagz0, "", ""},
        {0, 0, 0, 0},
        HelpLabels::HT_MARKS, HelpLabels::HF_MARKSMANDPWR, FractalFlags::TRIG1|FractalFlags::BAILTEST,
        -2.5F, 1.5F, -1.5F, 1.5F,
        0, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::MARKSMANDELPWR, SymmetryType::X_AXIS_NO_PARAM,
        marks_mandel_pwr_fp_fractal, marks_mandel_pwr_fp_per_pixel, mandel_fp_setup,
        standard_fractal,
        STD_BAIL_OUT
    },

    {
        s_t_marksmandelpwr+1,
        {s_realz0, s_imagz0, "", ""},
        {0, 0, 0, 0},
        HelpLabels::HT_MARKS, HelpLabels::HF_MARKSMANDPWR, FractalFlags::TRIG1|FractalFlags::BAILTEST,
        -2.5F, 1.5F, -1.5F, 1.5F,
        1, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::MARKSMANDELPWRFP, SymmetryType::X_AXIS_NO_PARAM,
        marks_mandel_pwr_fractal, marks_mandel_pwr_per_pixel, mandel_long_setup,
        standard_fractal,
        STD_BAIL_OUT
    },

    {
        s_t_tims_error,
        {s_realz0, s_imagz0, "", ""},
        {0, 0, 0, 0},
        HelpLabels::HT_MARKS, HelpLabels::HF_TIMSERR, FractalFlags::TRIG1|FractalFlags::BAILTEST,
        -2.9F, 4.3F, -2.7F, 2.7F,
        0, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::TIMSERROR, SymmetryType::X_AXIS_NO_PARAM,
        tims_error_fp_fractal, marks_mandel_pwr_fp_per_pixel, mandel_fp_setup,
        standard_fractal,
        STD_BAIL_OUT
    },

    {
        s_t_tims_error+1,
        {s_realz0, s_imagz0, "", ""},
        {0, 0, 0, 0},
        HelpLabels::HT_MARKS, HelpLabels::HF_TIMSERR, FractalFlags::TRIG1|FractalFlags::BAILTEST,
        -2.9F, 4.3F, -2.7F, 2.7F,
        1, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::TIMSERRORFP, SymmetryType::X_AXIS_NO_PARAM,
        tims_error_fractal, marks_mandel_pwr_per_pixel, mandel_long_setup,
        standard_fractal,
        STD_BAIL_OUT
    },

    {
        s_t_bifeqsinpi+1,
        {s_filt, s_seed, "", ""},
        {1000.0, 0.66, 0, 0},
        HelpLabels::HT_BIF, HelpLabels::HF_BIFEQSINPI, FractalFlags::TRIG1|FractalFlags::NOGUESS|FractalFlags::NOTRACE|FractalFlags::NOROTATE,
        -2.5F, 2.5F, -3.5F, 3.5F,
        1, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::BIFEQSINPI, SymmetryType::NONE,
        long_bifurc_set_trig_pi, nullptr, standalone_setup, bifurcation,
        NO_BAIL_OUT
    },

    {
        s_t_bifplussinpi+1,
        {s_filt, s_seed, "", ""},
        {1000.0, 0.66, 0, 0},
        HelpLabels::HT_BIF, HelpLabels::HF_BIFPLUSSINPI, FractalFlags::TRIG1|FractalFlags::NOGUESS|FractalFlags::NOTRACE|FractalFlags::NOROTATE,
        0.275F, 1.45F, 0.0F, 2.0F,
        1, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::BIFADSINPI, SymmetryType::NONE,
        long_bifurc_add_trig_pi, nullptr, standalone_setup, bifurcation,
        NO_BAIL_OUT
    },

    {
        s_t_bifstewart,
        {s_filt, s_seed, "", ""},
        {1000.0, 0.66, 0, 0},
        HelpLabels::HT_BIF, HelpLabels::HF_BIFSTEWART, FractalFlags::TRIG1|FractalFlags::NOGUESS|FractalFlags::NOTRACE|FractalFlags::NOROTATE,
        0.7F, 2.0F, -1.1F, 1.1F,
        0, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::LBIFSTEWART, SymmetryType::NONE,
        bifurc_stewart_trig, nullptr, standalone_setup, bifurcation,
        NO_BAIL_OUT
    },

    {
        s_t_bifstewart+1,
        {s_filt, s_seed, "", ""},
        {1000.0, 0.66, 0, 0},
        HelpLabels::HT_BIF, HelpLabels::HF_BIFSTEWART, FractalFlags::TRIG1|FractalFlags::NOGUESS|FractalFlags::NOTRACE|FractalFlags::NOROTATE,
        0.7F, 2.0F, -1.1F, 1.1F,
        1, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::BIFSTEWART, SymmetryType::NONE,
        long_bifurc_stewart_trig, nullptr, standalone_setup, bifurcation,
        NO_BAIL_OUT
    },

    {
        "hopalong",
        {"a", "b", "c", ""},
        {.4, 1, 0, 0},
        HelpLabels::HT_MARTIN, HelpLabels::HF_HOPALONG, FractalFlags::NOGUESS|FractalFlags::NOTRACE|FractalFlags::INFCALC,
        -2.0F, 3.0F, -1.625F, 2.625F,
        0, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::NOFRACTAL, SymmetryType::NONE,
        (VF)hopalong2d_float_orbit, nullptr, orbit3d_float_setup, orbit2d_float,
        NO_BAIL_OUT
    },

    {
        "circle",
        {"magnification", "", "", ""},
        {200000L, 0, 0, 0},
        HelpLabels::HT_CIRCLE, HelpLabels::HF_CIRCLE, FractalFlags::NONE,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::NOFRACTAL, SymmetryType::XY_AXIS,
        circle_fp_fractal, julia_fp_per_pixel, julia_fp_setup, standard_fractal,
        NO_BAIL_OUT
    },

    {
        "martin",
        {"a", "", "", ""},
        {3.14, 0, 0, 0},
        HelpLabels::HT_MARTIN, HelpLabels::HF_MARTIN, FractalFlags::NOGUESS|FractalFlags::NOTRACE|FractalFlags::INFCALC,
        -32.0F, 32.0F, -24.0F, 24.0F,
        0, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::NOFRACTAL, SymmetryType::NONE,
        (VF)martin2d_float_orbit, nullptr, orbit3d_float_setup, orbit2d_float,
        NO_BAIL_OUT
    },

    {
        "lyapunov",
        {"+Order (integer)", "Population Seed", "+Filter Cycles", ""},
        {0, 0.5, 0, 0},
        HelpLabels::HT_LYAPUNOV, HelpLabels::HT_LYAPUNOV, FractalFlags::NONE,
        -8.0F, 8.0F, -6.0F, 6.0F,
        0, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::NOFRACTAL, SymmetryType::NONE,
        lyapunov_orbit, nullptr, lya_setup, lyapunov,
        NO_BAIL_OUT
    },

    {
        "lorenz3d1",
        {s_time_step, "a", "b", "c"},
        {.02, 5, 15, 1},
        HelpLabels::HT_LORENZ, HelpLabels::HF_LORENZ3D1,
        FractalFlags::NOGUESS|FractalFlags::NOTRACE|FractalFlags::NORESUME|FractalFlags::PARMS3D|FractalFlags::INFCALC,
        -30.0F, 30.0F, -30.0F, 30.0F,
        0, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::NOFRACTAL, SymmetryType::NONE,
        (VF)lorenz3d1_float_orbit, nullptr, orbit3d_float_setup, orbit3d_float,
        NO_BAIL_OUT
    },

    {
        "lorenz3d3",
        {s_time_step, "a", "b", "c"},
        {.02, 10, 28, 2.66},
        HelpLabels::HT_LORENZ, HelpLabels::HF_LORENZ3D3,
        FractalFlags::NOGUESS|FractalFlags::NOTRACE|FractalFlags::NORESUME|FractalFlags::PARMS3D|FractalFlags::INFCALC,
        -30.0F, 30.0F, -30.0F, 30.0F,
        0, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::NOFRACTAL, SymmetryType::NONE,
        (VF)lorenz3d3_float_orbit, nullptr, orbit3d_float_setup, orbit3d_float,
        NO_BAIL_OUT
    },

    {
        "lorenz3d4",
        {s_time_step, "a", "b", "c"},
        {.02, 10, 28, 2.66},
        HelpLabels::HT_LORENZ, HelpLabels::HF_LORENZ3D4,
        FractalFlags::NOGUESS|FractalFlags::NOTRACE|FractalFlags::NORESUME|FractalFlags::PARMS3D|FractalFlags::INFCALC,
        -30.0F, 30.0F, -30.0F, 30.0F,
        0, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::NOFRACTAL, SymmetryType::NONE,
        (VF)lorenz3d4_float_orbit, nullptr, orbit3d_float_setup, orbit3d_float,
        NO_BAIL_OUT
    },

    {
        s_t_lambda_fn_or_fn+1,
        {s_real_param, s_imag_param, "Function Shift Value", ""},
        {1, 0.1, 1, 0},
        HelpLabels::HT_FNORFN, HelpLabels::HF_LAMBDAFNFN, FractalFlags::TRIG2|FractalFlags::BAILTEST,
        -4.0F, 4.0F, -3.0F, 3.0F,
        16, FractalType::NOFRACTAL, FractalType::LMANLAMFNFN, FractalType::FPLAMBDAFNFN, SymmetryType::ORIGIN,
        lambda_trig_or_trig_fractal, long_julia_per_pixel, lambda_trig_or_trig_setup,
        standard_fractal,
        LTRIG_BAIL_OUT
    },

    {
        s_t_lambda_fn_or_fn,
        {s_real_param, s_imag_param, "Function Shift Value", ""},
        {1, 0.1, 1, 0},
        HelpLabels::HT_FNORFN, HelpLabels::HF_LAMBDAFNFN, FractalFlags::TRIG2|FractalFlags::BAILTEST,
        -4.0F, 4.0F, -3.0F, 3.0F,
        0, FractalType::NOFRACTAL, FractalType::FPMANLAMFNFN, FractalType::LLAMBDAFNFN, SymmetryType::ORIGIN,
        lambda_trig_or_trig_fp_fractal, other_julia_fp_per_pixel,
        lambda_trig_or_trig_setup, standard_fractal,
        LTRIG_BAIL_OUT
    },

    {
        s_t_julia_fn_or_fn+1,
        {s_real_param, s_imag_param, "Function Shift Value", ""},
        {0, 0, 8, 0},
        HelpLabels::HT_FNORFN, HelpLabels::HF_JULIAFNFN, FractalFlags::TRIG2|FractalFlags::BAILTEST,
        -4.0F, 4.0F, -3.0F, 3.0F,
        16, FractalType::NOFRACTAL, FractalType::LMANFNFN, FractalType::FPJULFNFN, SymmetryType::X_AXIS,
        julia_trig_or_trig_fractal, long_julia_per_pixel, julia_trig_or_trig_setup,
        standard_fractal,
        LTRIG_BAIL_OUT
    },

    {
        s_t_julia_fn_or_fn,
        {s_real_param, s_imag_param, "Function Shift Value", ""},
        {0, 0, 8, 0},
        HelpLabels::HT_FNORFN, HelpLabels::HF_JULIAFNFN, FractalFlags::TRIG2|FractalFlags::BAILTEST,
        -4.0F, 4.0F, -3.0F, 3.0F,
        0, FractalType::NOFRACTAL, FractalType::FPMANFNFN, FractalType::LJULFNFN, SymmetryType::X_AXIS,
        julia_trig_or_trig_fp_fractal, other_julia_fp_per_pixel,
        julia_trig_or_trig_setup, standard_fractal,
        LTRIG_BAIL_OUT
    },

    {
        s_t_manlam_fnorfn+1,
        {s_realz0, s_imagz0, "Function Shift Value", ""},
        {0, 0, 10, 0},
        HelpLabels::HT_FNORFN, HelpLabels::HF_MANLAMFNFN, FractalFlags::TRIG2|FractalFlags::BAILTEST,
        -4.0F, 4.0F, -3.0F, 3.0F,
        16, FractalType::LLAMBDAFNFN, FractalType::NOFRACTAL, FractalType::FPMANLAMFNFN, SymmetryType::X_AXIS_NO_PARAM,
        lambda_trig_or_trig_fractal, long_mandel_per_pixel,
        man_lam_trig_or_trig_setup, standard_fractal,
        LTRIG_BAIL_OUT
    },

    {
        s_t_manlam_fnorfn,
        {s_realz0, s_imagz0, "Function Shift Value", ""},
        {0, 0, 10, 0},
        HelpLabels::HT_FNORFN, HelpLabels::HF_MANLAMFNFN, FractalFlags::TRIG2|FractalFlags::BAILTEST,
        -4.0F, 4.0F, -3.0F, 3.0F,
        0, FractalType::FPLAMBDAFNFN, FractalType::NOFRACTAL, FractalType::LMANLAMFNFN, SymmetryType::X_AXIS_NO_PARAM,
        lambda_trig_or_trig_fp_fractal, other_mandel_fp_per_pixel,
        man_lam_trig_or_trig_setup, standard_fractal,
        LTRIG_BAIL_OUT
    },

    {
        s_t_mandel_fn_or_fn+1,
        {s_realz0, s_imagz0, "Function Shift Value", ""},
        {0, 0, 0.5, 0},
        HelpLabels::HT_FNORFN, HelpLabels::HF_MANDELFNFN, FractalFlags::TRIG2|FractalFlags::BAILTEST,
        -4.0F, 4.0F, -3.0F, 3.0F,
        16, FractalType::LJULFNFN, FractalType::NOFRACTAL, FractalType::FPMANFNFN, SymmetryType::X_AXIS_NO_PARAM,
        julia_trig_or_trig_fractal, long_mandel_per_pixel,
        mandel_trig_or_trig_setup, standard_fractal,
        LTRIG_BAIL_OUT
    },

    {
        s_t_mandel_fn_or_fn,
        {s_realz0, s_imagz0, "Function Shift Value", ""},
        {0, 0, 0.5, 0},
        HelpLabels::HT_FNORFN, HelpLabels::HF_MANDELFNFN, FractalFlags::TRIG2|FractalFlags::BAILTEST,
        -4.0F, 4.0F, -3.0F, 3.0F,
        0, FractalType::FPJULFNFN, FractalType::NOFRACTAL, FractalType::LMANFNFN, SymmetryType::X_AXIS_NO_PARAM,
        julia_trig_or_trig_fp_fractal, other_mandel_fp_per_pixel,
        mandel_trig_or_trig_setup, standard_fractal,
        LTRIG_BAIL_OUT
    },

    {
        s_t_bifmay+1,
        {s_filt, s_seed, "Beta >= 2", ""},
        {300.0, 0.9, 5, 0},
        HelpLabels::HT_BIF, HelpLabels::HF_BIFMAY, FractalFlags::NOGUESS|FractalFlags::NOTRACE|FractalFlags::NOROTATE,
        -3.5F, -0.9F, -0.5F, 3.2F,
        16, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::BIFMAY, SymmetryType::NONE,
        long_bifurc_may, nullptr, bifurc_may_setup, bifurcation,
        NO_BAIL_OUT
    },

    {
        s_t_bifmay,
        {s_filt, s_seed, "Beta >= 2", ""},
        {300.0, 0.9, 5, 0},
        HelpLabels::HT_BIF, HelpLabels::HF_BIFMAY, FractalFlags::NOGUESS|FractalFlags::NOTRACE|FractalFlags::NOROTATE,
        -3.5F, -0.9F, -0.5F, 3.2F,
        0, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::LBIFMAY, SymmetryType::NONE,
        bifurc_may, nullptr, bifurc_may_setup, bifurcation,
        NO_BAIL_OUT
    },

    {
        s_t_halley+1,
        {s_order, s_real_relax, s_epsilon, s_imag_relax},
        {6, 1.0, 0.0001, 0},
        HelpLabels::HT_HALLEY, HelpLabels::HF_HALLEY, FractalFlags::NONE,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::HALLEY, SymmetryType::XY_AXIS,
        mpc_halley_fractal, mpc_halley_per_pixel, halley_setup, standard_fractal,
        NO_BAIL_OUT
    },

    {
        s_t_halley,
        {s_order, s_real_relax, s_epsilon, s_imag_relax},
        {6, 1.0, 0.0001, 0},
        HelpLabels::HT_HALLEY, HelpLabels::HF_HALLEY, FractalFlags::NONE,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::MPHALLEY, SymmetryType::XY_AXIS,
        halley_fractal, halley_per_pixel, halley_setup, standard_fractal,
        NO_BAIL_OUT
    },

    {
        "dynamic",
        {"+# of intervals (<0 = connect)", "time step (<0 = Euler)", "a", "b"},
        {50, .1, 1, 3},
        HelpLabels::HT_DYNAM, HelpLabels::HF_DYNAM, FractalFlags::NOGUESS|FractalFlags::NOTRACE|FractalFlags::TRIG1,
        -20.0F, 20.0F, -20.0F, 20.0F,
        0, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::NOFRACTAL, SymmetryType::NONE,
        (VF)dynam_float, nullptr, dynam2d_float_setup, dynam2d_float,
        NO_BAIL_OUT
    },

    {
        "quat",
        {"notused", "notused", "cj", "ck"},
        {0, 0, 0, 0},
        HelpLabels::HT_QUAT, HelpLabels::HF_QUAT, FractalFlags::OKJB,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, FractalType::QUATJULFP, FractalType::NOFRACTAL, FractalType::NOFRACTAL, SymmetryType::X_AXIS,
        quaternion_fp_fractal, quaternion_fp_per_pixel, mandel_fp_setup,
        standard_fractal,
        LTRIG_BAIL_OUT
    },

    {
        "quatjul",
        {"c1", "ci", "cj", "ck"},
        {-.745, 0, .113, .05},
        HelpLabels::HT_QUAT, HelpLabels::HF_QUATJ, FractalFlags::OKJB|FractalFlags::MORE,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, FractalType::NOFRACTAL, FractalType::QUATFP, FractalType::NOFRACTAL, SymmetryType::ORIGIN,
        quaternion_fp_fractal, quaternion_jul_fp_per_pixel, julia_fp_setup,
        standard_fractal,
        LTRIG_BAIL_OUT
    },

    {
        "cellular",
        {s_cell_init, s_cell_rule, s_cell_type, s_cell_strt},
        {11.0, 3311100320.0, 41.0, 0},
        HelpLabels::HT_CELLULAR, HelpLabels::HF_CELLULAR, FractalFlags::NOGUESS|FractalFlags::NOTRACE|FractalFlags::NOZOOM,
        -1.0F, 1.0F, -1.0F, 1.0F,
        0, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::NOFRACTAL, SymmetryType::NONE,
        nullptr, nullptr, cellular_setup, cellular,
        NO_BAIL_OUT
    },

    {
        s_t_julibrot,
        {"", "", "", ""},
        {0, 0, 0, 0},
        HelpLabels::HT_JULIBROT, HelpLabels::NONE, FractalFlags::NOGUESS|FractalFlags::NOTRACE|FractalFlags::NOROTATE|FractalFlags::NORESUME,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::JULIBROT, SymmetryType::NONE,
        julia_fp_fractal, jb_fp_per_pixel, julibrot_setup, std_4d_fp_fractal,
        STD_BAIL_OUT
    },

#ifdef RANDOM_RUN
    {
        t_julia_inverse+1,
        {realparm, imagparm, s_maxhits, "Random Run Interval"},
        {-0.11, 0.6557, 4, 1024},
        HelpLabels::HT_INVERSE, HelpLabels::HF_INVERSE, fractal_flags::NOGUESS | fractal_flags::NOTRACE | fractal_flags::INFCALC | fractal_flags::NORESUME,
        -2.0F, 2.0F, -1.5F, 1.5F,
        24, fractal_type::NOFRACTAL, fractal_type::MANDEL, fractal_type::INVERSEJULIAFP, symmetry_type::NONE,
        l_inverse_julia_orbit, nullptr, orbit3d_long_setup, inverse_julia_per_image,
        NOBAILOUT
    },

    {
        t_julia_inverse,
        {realparm, imagparm, s_maxhits, "Random Run Interval"},
        {-0.11, 0.6557, 4, 1024},
        HelpLabels::HT_INVERSE, HelpLabels::HF_INVERSE, fractal_flags::NOGUESS | fractal_flags::NOTRACE | fractal_flags::INFCALC | fractal_flags::NORESUME,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, fractal_type::NOFRACTAL, fractal_type::MANDEL, fractal_type::INVERSEJULIA, symmetry_type::NONE,
        m_inverse_julia_orbit, nullptr, orbit3d_float_setup, inverse_julia_per_image,
        NOBAILOUT
    },
#else
    {
        s_t_julia_inverse+1,
        {s_real_param, s_imag_param, s_max_hits, ""},
        {-0.11, 0.6557, 4, 1024},
        HelpLabels::HT_INVERSE, HelpLabels::HF_INVERSE, FractalFlags::NOGUESS|FractalFlags::NOTRACE|FractalFlags::INFCALC|FractalFlags::NORESUME,
        -2.0F, 2.0F, -1.5F, 1.5F,
        24, FractalType::NOFRACTAL, FractalType::MANDEL, FractalType::INVERSEJULIAFP, SymmetryType::NONE,
        l_inverse_julia_orbit, nullptr, orbit3d_long_setup, inverse_julia_per_image,
        NO_BAIL_OUT
    },

    {
        s_t_julia_inverse,
        {s_real_param, s_imag_param, s_max_hits, ""},
        {-0.11, 0.6557, 4, 1024},
        HelpLabels::HT_INVERSE, HelpLabels::HF_INVERSE, FractalFlags::NOGUESS|FractalFlags::NOTRACE|FractalFlags::INFCALC|FractalFlags::NORESUME,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, FractalType::NOFRACTAL, FractalType::MANDEL, FractalType::INVERSEJULIA, SymmetryType::NONE,
        m_inverse_julia_orbit, nullptr, orbit3d_float_setup, inverse_julia_per_image,
        NO_BAIL_OUT
    },

#endif

    {
        "mandelcloud",
        {"+# of intervals (<0 = connect)", "", "", ""},
        {50, 0, 0, 0},
        HelpLabels::HT_MANDELCLOUD, HelpLabels::HF_MANDELCLOUD, FractalFlags::NOGUESS|FractalFlags::NOTRACE,
        -2.5F, 1.5F, -1.5F, 1.5F,
        0, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::NOFRACTAL, SymmetryType::NONE,
        (VF)mandel_cloud_float, nullptr, dynam2d_float_setup, dynam2d_float,
        NO_BAIL_OUT
    },

    {
        s_t_phoenix+1,
        {s_p1_real, s_p2_real, s_degree_z, ""},
        {0.56667, -0.5, 0, 0},
        HelpLabels::HT_PHOENIX, HelpLabels::HF_PHOENIX, FractalFlags::BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        1, FractalType::NOFRACTAL, FractalType::MANDPHOENIX, FractalType::PHOENIXFP, SymmetryType::X_AXIS,
        long_phoenix_fractal, long_phoenix_per_pixel, phoenix_setup,
        standard_fractal,
        STD_BAIL_OUT
    },

    {
        s_t_phoenix,
        {s_p1_real, s_p2_real, s_degree_z, ""},
        {0.56667, -0.5, 0, 0},
        HelpLabels::HT_PHOENIX, HelpLabels::HF_PHOENIX, FractalFlags::BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, FractalType::NOFRACTAL, FractalType::MANDPHOENIXFP, FractalType::PHOENIX, SymmetryType::X_AXIS,
        phoenix_fractal, phoenix_per_pixel, phoenix_setup, standard_fractal,
        STD_BAIL_OUT
    },

    {
        s_t_mandphoenix+1,
        {s_realz0, s_imagz0, s_degree_z, ""},
        {0.0, 0.0, 0, 0},
        HelpLabels::HT_PHOENIX, HelpLabels::HF_MANDPHOENIX, FractalFlags::BAILTEST,
        -2.5F, 1.5F, -1.5F, 1.5F,
        1, FractalType::PHOENIX, FractalType::NOFRACTAL, FractalType::MANDPHOENIXFP, SymmetryType::NONE,
        long_phoenix_fractal, long_mand_phoenix_per_pixel, mand_phoenix_setup,
        standard_fractal,
        STD_BAIL_OUT
    },

    {
        s_t_mandphoenix,
        {s_realz0, s_imagz0, s_degree_z, ""},
        {0.0, 0.0, 0, 0},
        HelpLabels::HT_PHOENIX, HelpLabels::HF_MANDPHOENIX, FractalFlags::BAILTEST,
        -2.5F, 1.5F, -1.5F, 1.5F,
        0, FractalType::PHOENIXFP, FractalType::NOFRACTAL, FractalType::MANDPHOENIX, SymmetryType::NONE,
        phoenix_fractal, mand_phoenix_per_pixel, mand_phoenix_setup,
        standard_fractal,
        STD_BAIL_OUT
    },

    {
        "hypercomplex",
        {"notused", "notused", "cj", "ck"},
        {0, 0, 0, 0},
        HelpLabels::HT_HYPERC, HelpLabels::HF_HYPERC, FractalFlags::OKJB|FractalFlags::TRIG1,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, FractalType::HYPERCMPLXJFP, FractalType::NOFRACTAL, FractalType::NOFRACTAL, SymmetryType::X_AXIS,
        hyper_complex_fp_fractal, quaternion_fp_per_pixel, mandel_fp_setup,
        standard_fractal,
        LTRIG_BAIL_OUT
    },

    {
        "hypercomplexj",
        {"c1", "ci", "cj", "ck"},
        {-.745, 0, .113, .05},
        HelpLabels::HT_HYPERC, HelpLabels::HF_HYPERCJ, FractalFlags::OKJB|FractalFlags::TRIG1|FractalFlags::MORE,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, FractalType::NOFRACTAL, FractalType::HYPERCMPLXFP, FractalType::NOFRACTAL, SymmetryType::ORIGIN,
        hyper_complex_fp_fractal, quaternion_jul_fp_per_pixel, julia_fp_setup,
        standard_fractal,
        LTRIG_BAIL_OUT
    },

    {
        s_t_frothybasin+1,
        {s_froth_mapping, s_froth_shade, s_froth_a_value, ""},
        {1, 0, 1.028713768218725, 0},
        HelpLabels::HT_FROTH, HelpLabels::HF_FROTH, FractalFlags::NOTRACE,
        -2.8F, 2.8F, -2.355F, 1.845F,
        28, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::FROTHFP, SymmetryType::NONE,
        froth_per_orbit, froth_per_pixel, froth_setup, calc_froth,
        FROTH_BAIL_OUT
    },

    {
        s_t_frothybasin,
        {s_froth_mapping, s_froth_shade, s_froth_a_value, ""},
        {1, 0, 1.028713768218725, 0},
        HelpLabels::HT_FROTH, HelpLabels::HF_FROTH, FractalFlags::NOTRACE,
        -2.8F, 2.8F, -2.355F, 1.845F,
        0, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::FROTH, SymmetryType::NONE,
        froth_per_orbit, froth_per_pixel, froth_setup, calc_froth,
        FROTH_BAIL_OUT
    },

    {
        s_t_mandel4,
        {s_realz0, s_imagz0, "", ""},
        {0, 0, 0, 0},
        HelpLabels::HT_MANDJUL4, HelpLabels::HF_MANDEL4, FractalFlags::BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, FractalType::JULIA4FP, FractalType::NOFRACTAL, FractalType::MANDEL4, SymmetryType::X_AXIS_NO_PARAM,
        mandel4_fp_fractal, mandel_fp_per_pixel, mandel_fp_setup, standard_fractal,
        STD_BAIL_OUT
    },

    {
        s_t_julia4,
        {s_real_param, s_imag_param, "", ""},
        {0.6, 0.55, 0, 0},
        HelpLabels::HT_MANDJUL4, HelpLabels::HF_JULIA4, FractalFlags::OKJB|FractalFlags::BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, FractalType::NOFRACTAL, FractalType::MANDEL4FP, FractalType::JULIA4, SymmetryType::ORIGIN,
        mandel4_fp_fractal, julia_fp_per_pixel, julia_fp_setup, standard_fractal,
        STD_BAIL_OUT
    },

    {
        s_t_marksmandel,
        {s_realz0, s_imagz0, s_exponent, ""},
        {0, 0, 1, 0},
        HelpLabels::HT_MARKS, HelpLabels::HF_MARKSMAND, FractalFlags::BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, FractalType::MARKSJULIAFP, FractalType::NOFRACTAL, FractalType::MARKSMANDEL, SymmetryType::NONE,
        marks_lambda_fp_fractal, marks_mandel_fp_per_pixel, mandel_fp_setup,
        standard_fractal,
        STD_BAIL_OUT
    },

    {
        s_t_marksjulia,
        {s_real_param, s_imag_param, s_exponent, ""},
        {0.1, 0.9, 1, 0},
        HelpLabels::HT_MARKS, HelpLabels::HF_MARKSJULIA, FractalFlags::BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, FractalType::NOFRACTAL, FractalType::MARKSMANDELFP, FractalType::MARKSJULIA, SymmetryType::ORIGIN,
        marks_lambda_fp_fractal, julia_fp_per_pixel, marks_julia_fp_setup,
        standard_fractal,
        STD_BAIL_OUT
    },

    {
        "icons",
        {"Lambda", "Alpha", "Beta", "Gamma"},
        {-2.34, 2.0, 0.2, 0.1},
        HelpLabels::HT_ICON, HelpLabels::HF_ICON, FractalFlags::NOGUESS|FractalFlags::NOTRACE|FractalFlags::INFCALC|FractalFlags::MORE,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::NOFRACTAL, SymmetryType::NONE,
        (VF)icon_float_orbit, nullptr, orbit3d_float_setup, orbit2d_float,
        NO_BAIL_OUT
    },

    {
        "icons3d",
        {"Lambda", "Alpha", "Beta", "Gamma"},
        {-2.34, 2.0, 0.2, 0.1},
        HelpLabels::HT_ICON, HelpLabels::HF_ICON, FractalFlags::NOGUESS|FractalFlags::NOTRACE|FractalFlags::INFCALC|FractalFlags::PARMS3D|FractalFlags::MORE,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::NOFRACTAL, SymmetryType::NONE,
        (VF)icon_float_orbit, nullptr, orbit3d_float_setup, orbit3d_float,
        NO_BAIL_OUT
    },

    {
        s_t_phoenixcplx+1,
        {s_p1_real, s_p1_imag, s_p2_real, s_p2_imag},
        {0.2, 0, 0.3, 0},
        HelpLabels::HT_PHOENIX, HelpLabels::HF_PHOENIXCPLX, FractalFlags::MORE|FractalFlags::BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        1, FractalType::NOFRACTAL, FractalType::MANDPHOENIXCPLX, FractalType::PHOENIXFPCPLX, SymmetryType::ORIGIN,
        long_phoenix_fractal_cplx, long_phoenix_per_pixel, phoenix_cplx_setup,
        standard_fractal,
        STD_BAIL_OUT
    },

    {
        s_t_phoenixcplx,
        {s_p1_real, s_p1_imag, s_p2_real, s_p2_imag},
        {0.2, 0, 0.3, 0},
        HelpLabels::HT_PHOENIX, HelpLabels::HF_PHOENIXCPLX, FractalFlags::MORE|FractalFlags::BAILTEST,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, FractalType::NOFRACTAL, FractalType::MANDPHOENIXFPCPLX, FractalType::PHOENIXCPLX, SymmetryType::ORIGIN,
        phoenix_fractal_cplx, phoenix_per_pixel, phoenix_cplx_setup,
        standard_fractal,
        STD_BAIL_OUT
    },

    {
        s_t_mandphoenixcplx+1,
        {s_realz0, s_imagz0, s_p2_real, s_p2_imag},
        {0, 0, 0.5, 0},
        HelpLabels::HT_PHOENIX, HelpLabels::HF_MANDPHOENIXCPLX, FractalFlags::MORE|FractalFlags::BAILTEST,
        -2.5F, 1.5F, -1.5F, 1.5F,
        1, FractalType::PHOENIXCPLX, FractalType::NOFRACTAL, FractalType::MANDPHOENIXFPCPLX, SymmetryType::X_AXIS,
        long_phoenix_fractal_cplx, long_mand_phoenix_per_pixel,
        mand_phoenix_cplx_setup, standard_fractal,
        STD_BAIL_OUT
    },

    {
        s_t_mandphoenixcplx,
        {s_realz0, s_imagz0, s_p2_real, s_p2_imag},
        {0, 0, 0.5, 0},
        HelpLabels::HT_PHOENIX, HelpLabels::HF_MANDPHOENIXCPLX, FractalFlags::MORE|FractalFlags::BAILTEST,
        -2.5F, 1.5F, -1.5F, 1.5F,
        0, FractalType::PHOENIXFPCPLX, FractalType::NOFRACTAL, FractalType::MANDPHOENIXCPLX, SymmetryType::X_AXIS,
        phoenix_fractal_cplx, mand_phoenix_per_pixel, mand_phoenix_cplx_setup,
        standard_fractal,
        STD_BAIL_OUT
    },

    {
        "ant",
        {
            "#Rule String (1's and non-1's, 0 rand)",
            "#Maxpts",
            "+Numants (max 256)",
            "+Ant type (1 or 2)"
        },
        {1100, 1.0E9, 1, 1},
        HelpLabels::HT_ANT, HelpLabels::HF_ANT, FractalFlags::NOZOOM|FractalFlags::NOGUESS|FractalFlags::NOTRACE|FractalFlags::NORESUME|FractalFlags::MORE,
        -1.0F, 1.0F, -1.0F, 1.0F,
        0, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::NOFRACTAL, SymmetryType::NONE,
        nullptr, nullptr, standalone_setup, ant,
        NO_BAIL_OUT
    },

    {
        "chip",
        {"a", "b", "c", ""},
        {-15, -19, 1, 0},
        HelpLabels::HT_MARTIN, HelpLabels::HF_CHIP, FractalFlags::NOGUESS|FractalFlags::NOTRACE|FractalFlags::INFCALC,
        -760.0F, 760.0F, -570.0F, 570.0F,
        0, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::NOFRACTAL, SymmetryType::NONE,
        (VF)chip2d_float_orbit, nullptr, orbit3d_float_setup, orbit2d_float,
        NO_BAIL_OUT
    },

    {
        "quadruptwo",
        {"a", "b", "c", ""},
        {34, 1, 5, 0},
        HelpLabels::HT_MARTIN, HelpLabels::HF_QUADRUPTWO, FractalFlags::NOGUESS|FractalFlags::NOTRACE|FractalFlags::INFCALC,
        -82.93367F, 112.2749F, -55.76383F, 90.64257F,
        0, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::NOFRACTAL, SymmetryType::NONE,
        (VF)quadrup_two2d_float_orbit, nullptr, orbit3d_float_setup, orbit2d_float,
        NO_BAIL_OUT
    },

    {
        "threeply",
        {"a", "b", "c", ""},
        {-55, -1, -42, 0},
        HelpLabels::HT_MARTIN, HelpLabels::HF_THREEPLY, FractalFlags::NOGUESS|FractalFlags::NOTRACE|FractalFlags::INFCALC,
        -8000.0F, 8000.0F, -6000.0F, 6000.0F,
        0, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::NOFRACTAL, SymmetryType::NONE,
        (VF)three_ply2d_float_orbit, nullptr, orbit3d_float_setup, orbit2d_float,
        NO_BAIL_OUT
    },

    {
        "volterra-lotka",
        {"h", "p", "", ""},
        {0.739, 0.739, 0, 0},
        HelpLabels::HT_VL, HelpLabels::HF_VL, FractalFlags::NONE,
        0.0F, 6.0F, 0.0F, 4.5F,
        0, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::NOFRACTAL, SymmetryType::NONE,
        vl_fp_fractal, other_julia_fp_per_pixel, vl_setup, standard_fractal,
        256
    },

    {
        "escher_julia",
        {s_real_param, s_imag_param, "", ""},
        {0.32, 0.043, 0, 0},
        HelpLabels::HT_ESCHER, HelpLabels::HF_ESCHER, FractalFlags::NONE,
        -1.6F, 1.6F, -1.2F, 1.2F,
        0, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::NOFRACTAL, SymmetryType::ORIGIN,
        escher_fp_fractal, julia_fp_per_pixel, standard_setup,
        standard_fractal,
        STD_BAIL_OUT
    },

    // From Pickovers' "Chaos in Wonderland"
    // included by Humberto R. Baptista
    // code adapted from king.cpp bt James Rankin

    {
        "latoocarfian",
        {"a", "b", "c", "d"},
        {-0.966918, 2.879879, 0.765145, 0.744728},
        HelpLabels::HT_LATOO, HelpLabels::HF_LATOO, FractalFlags::NOGUESS|FractalFlags::NOTRACE|FractalFlags::INFCALC|FractalFlags::TRIG4,
        -2.0F, 2.0F, -1.5F, 1.5F,
        0, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::NOFRACTAL, SymmetryType::NONE,
        (VF)latoo_float_orbit, nullptr, orbit3d_float_setup, orbit2d_float,
        NO_BAIL_OUT
    },
    {
        "dividebrot5",
        {"a", "b", "", ""},
        {2.0, 0.0, 0.0, 0.0},
        HelpLabels::HT_DIVIDEBROT5, HelpLabels::HF_DIVIDEBROT5, FractalFlags::BAILTEST|FractalFlags::BF_MATH,
        -2.5f, 1.5f, -1.5f, 1.5f,
        0, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::NOFRACTAL, SymmetryType::NONE,
        divide_brot5_fp_fractal, divide_brot5_fp_per_pixel, divide_brot5_setup, standard_fractal,
        16
    },
    {
        "mandelbrotmix4",
        {s_p1_real, s_p1_imag, s_p2_real, s_p2_imag},
        {0.05, 3, -1.5, -2},
        HelpLabels::HT_MANDELBROTMIX4, HelpLabels::HF_MANDELBROTMIX4, FractalFlags::BAILTEST|FractalFlags::TRIG1|FractalFlags::MORE,
        -2.5F, 1.5F, -1.5F, 1.5F,
        0, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::NOFRACTAL, SymmetryType::NONE,
        mandelbrot_mix4_fp_fractal, mandelbrot_mix4_fp_per_pixel, mandelbrot_mix4_setup, standard_fractal,
        STD_BAIL_OUT
    },
    {
        "burningship",
        {s_p1_real, s_p1_imag, "degree", ""},
        {0, 0, 2, 0},
        HelpLabels::HT_MANDEL, HelpLabels::HF_MANDEL, FractalFlags::BAILTEST|FractalFlags::PERTURB|FractalFlags::BF_MATH,
        -2.5F, 1.5F, -1.2F, 1.8F,
        0, FractalType::BURNING_SHIP, FractalType::NOFRACTAL, FractalType::BURNING_SHIP, SymmetryType::NONE,
        burning_ship_fp_fractal, other_mandel_fp_per_pixel, mandel_setup, standard_fractal,
        STD_BAIL_OUT
    },

    {
        nullptr,            // marks the END of the list
        {nullptr, nullptr, nullptr, nullptr},
        {0, 0, 0, 0},
        HelpLabels::NONE, HelpLabels::NONE, FractalFlags::NONE,
        0.0F, 0.0F, 0.0F, 0.0F,
        0, FractalType::NOFRACTAL, FractalType::NOFRACTAL, FractalType::NOFRACTAL, SymmetryType::NONE,
        nullptr, nullptr, nullptr, nullptr,
        0
    }
};

int g_num_fractal_types = (sizeof(g_fractal_specific) / sizeof(FractalSpecific)) - 1;
