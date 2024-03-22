#include "port.h"
#include "prototyp.h"

#include "frasetup.h"

#include "bailout_formula.h"
#include "calcfrac.h"
#include "calmanfp.h"
#include "cmdfiles.h"
#include "editpal.h"
#include "fracsubr.h"
#include "fractalp.h"
#include "fractals.h"
#include "fractint.h"
#include "fractype.h"
#include "id_data.h"
#include "mpmath.h"
#include "mpmath_c.h"

#include <cmath>

// --------------------------------------------------------------------
//              Setup (once per fractal image) routines
// --------------------------------------------------------------------

bool
MandelSetup()           // Mandelbrot Routine
{
    if (g_debug_flag != debug_flags::force_standard_fractal
        && (g_invert == 0)
        && g_decomp[0] == 0
        && g_magnitude_limit == 4.0
        && g_bit_shift == 29
        && !g_potential_flag
        && g_biomorph == -1
        && g_inside_color > ZMAG
        && g_outside_color >= ITER
        && g_use_init_orbit != init_orbit_mode::value
        && !g_using_jiim
        && g_bail_out_test == bailouts::Mod
        && (g_orbit_save_flags & osf_midi) == 0)
    {
        g_calc_type = calcmand; // the normal case - use CALCMAND
    }
    else
    {
        // special case: use the main processing loop
        g_calc_type = standard_fractal;
        g_long_param = &g_l_init;
    }
    return true;
}

bool
JuliaSetup()            // Julia Routine
{
    if (g_debug_flag != debug_flags::force_standard_fractal
        && (g_invert == 0)
        && g_decomp[0] == 0
        && g_magnitude_limit == 4.0
        && g_bit_shift == 29
        && !g_potential_flag
        && g_biomorph == -1
        && g_inside_color > ZMAG
        && g_outside_color >= ITER
        && !g_finite_attractor
        && !g_using_jiim
        && g_bail_out_test == bailouts::Mod
        && (g_orbit_save_flags & osf_midi) == 0)
    {
        g_calc_type = calcmand; // the normal case - use CALCMAND
    }
    else
    {
        // special case: use the main processing loop
        g_calc_type = standard_fractal;
        g_long_param = &g_l_param;
        get_julia_attractor(0.0, 0.0);    // another attractor?
    }
    return true;
}

bool
NewtonSetup()           // Newton/NewtBasin Routines
{
    if (g_debug_flag != debug_flags::allow_mp_newton_type)
    {
        if (g_fractal_type == fractal_type::MPNEWTON)
        {
            g_fractal_type = fractal_type::NEWTON;
        }
        else if (g_fractal_type == fractal_type::MPNEWTBASIN)
        {
            g_fractal_type = fractal_type::NEWTBASIN;
        }
        g_cur_fractal_specific = &g_fractal_specific[static_cast<int>(g_fractal_type)];
    }
    // set up table of roots of 1 along unit circle
    g_degree = (int)g_param_z1.x;
    if (g_degree < 2)
    {
        g_degree = 3;   // defaults to 3, but 2 is possible
    }

    // precalculated values
    g_newton_r_over_d       = 1.0 / (double)g_degree;
    g_degree_minus_1_over_degree      = (double)(g_degree - 1) / (double)g_degree;
    g_max_color     = 0;
    g_threshold    = .3*PI/g_degree; // less than half distance between roots
    if (g_fractal_type == fractal_type::MPNEWTON || g_fractal_type == fractal_type::MPNEWTBASIN)
    {
        g_newton_mp_r_over_d     = *pd2MP(g_newton_r_over_d);
        g_mp_degree_minus_1_over_degree    = *pd2MP(g_degree_minus_1_over_degree);
        g_mp_threshold  = *pd2MP(g_threshold);
        g_mp_one        = *pd2MP(1.0);
    }

    g_basin = 0;
    g_roots.resize(16);
    if (g_fractal_type == fractal_type::NEWTBASIN)
    {
        if (g_param_z1.y)
        {
            g_basin = 2; //stripes
        }
        else
        {
            g_basin = 1;
        }
        g_roots.resize(g_degree);

        // list of roots to discover where we converged for newtbasin
        for (int i = 0; i < g_degree; i++)
        {
            g_roots[i].x = std::cos(i*PI*2.0/(double)g_degree);
            g_roots[i].y = std::sin(i*PI*2.0/(double)g_degree);
        }
    }
    else if (g_fractal_type == fractal_type::MPNEWTBASIN)
    {
        if (g_param_z1.y)
        {
            g_basin = 2;    //stripes
        }
        else
        {
            g_basin = 1;
        }

        g_mpc_roots.resize(g_degree);

        // list of roots to discover where we converged for newtbasin
        for (int i = 0; i < g_degree; i++)
        {
            g_mpc_roots[i].x = *pd2MP(std::cos(i*PI*2.0/(double)g_degree));
            g_mpc_roots[i].y = *pd2MP(std::sin(i*PI*2.0/(double)g_degree));
        }
    }

    g_params[0] = (double)g_degree;
    if (g_degree%4 == 0)
    {
        g_symmetry = symmetry_type::XY_AXIS;
    }
    else
    {
        g_symmetry = symmetry_type::X_AXIS;
    }

    g_calc_type = standard_fractal;
    if (g_fractal_type == fractal_type::MPNEWTON || g_fractal_type == fractal_type::MPNEWTBASIN)
    {
        setMPfunctions();
    }
    return true;
}


bool
StandaloneSetup()
{
    timer(0, g_cur_fractal_specific->calctype);
    return false;               // effectively disable solid-guessing
}

bool
UnitySetup()
{
    g_periodicity_check = 0;
    g_fudge_one = (1L << g_bit_shift);
    g_fudge_two = g_fudge_one + g_fudge_one;
    return true;
}

bool
MandelfpSetup()
{
    bf_math = bf_math_type::NONE;
    g_c_exponent = (int)g_params[2];
    g_power_z.x = g_params[2] - 1.0;
    g_power_z.y = g_params[3];
    g_float_param = &g_init;
    switch (g_fractal_type)
    {
    case fractal_type::MARKSMANDELFP:
        if (g_c_exponent < 1)
        {
            g_c_exponent = 1;
            g_params[2] = 1;
        }
        if (!(g_c_exponent & 1))
        {
            g_symmetry = symmetry_type::XY_AXIS_NO_PARAM;    // odd exponents
        }
        if (g_c_exponent & 1)
        {
            g_symmetry = symmetry_type::X_AXIS_NO_PARAM;
        }
        break;
    case fractal_type::MANDELFP:
        /*
           floating point code could probably be altered to handle many of
           the situations that otherwise are using standard_fractal().
           calcmandfp() can currently handle invert, any rqlim, potflag
           zmag, epsilon cross, and all the current outside options
        */
        if (g_debug_flag != debug_flags::force_standard_fractal
            && !g_distance_estimator
            && g_decomp[0] == 0
            && g_biomorph == -1
            && (g_inside_color >= ITER)
            && g_outside_color >= ATAN
            && g_use_init_orbit != init_orbit_mode::value
            && (g_sound_flag & SOUNDFLAG_ORBITMASK) < SOUNDFLAG_X
            && !g_using_jiim
            && g_bail_out_test == bailouts::Mod
            && (g_orbit_save_flags & osf_midi) == 0)
        {
            g_calc_type = calcmandfp; // the normal case - use calcmandfp
            calcmandfpasmstart();
        }
        else
        {
            // special case: use the main processing loop
            g_calc_type = standard_fractal;
        }
        break;
    case fractal_type::FPMANDELZPOWER:
        if ((double)g_c_exponent == g_params[2] && (g_c_exponent & 1))   // odd exponents
        {
            g_symmetry = symmetry_type::XY_AXIS_NO_PARAM;
        }
        if (g_params[3] != 0)
        {
            g_symmetry = symmetry_type::NONE;
        }
        if (g_params[3] == 0.0 && g_debug_flag != debug_flags::force_complex_power && (double)g_c_exponent == g_params[2])
        {
            g_fractal_specific[static_cast<int>(g_fractal_type)].orbitcalc = floatZpowerFractal;
        }
        else
        {
            g_fractal_specific[static_cast<int>(g_fractal_type)].orbitcalc = floatCmplxZpowerFractal;
        }
        break;
    case fractal_type::MAGNET1M:
    case fractal_type::MAGNET2M:
        g_attractor[0].x = 1.0;      // 1.0 + 0.0i always attracts
        g_attractor[0].y = 0.0;      // - both MAGNET1 and MAGNET2
        g_attractor_period[0] = 1;
        g_attractors = 1;
        break;
    case fractal_type::SPIDERFP:
        if (g_periodicity_check == 1)   // if not user set
        {
            g_periodicity_check = 4;
        }
        break;
    case fractal_type::MANDELEXP:
        g_symmetry = symmetry_type::X_AXIS_NO_PARAM;
        break;
    case fractal_type::FPMANTRIGPLUSEXP:
    case fractal_type::FPMANTRIGPLUSZSQRD:
        if (g_param_z1.y == 0.0)
        {
            g_symmetry = symmetry_type::X_AXIS;
        }
        else
        {
            g_symmetry = symmetry_type::NONE;
        }
        if ((g_trig_index[0] == trig_fn::LOG) || (g_trig_index[0] == trig_fn::FLIP))
        {
            g_symmetry = symmetry_type::NONE;
        }
        break;
    case fractal_type::QUATFP:
        g_float_param = &g_tmp_z;
        g_attractors = 0;
        g_periodicity_check = 0;
        break;
    case fractal_type::HYPERCMPLXFP:
        g_float_param = &g_tmp_z;
        g_attractors = 0;
        g_periodicity_check = 0;
        if (g_params[2] != 0)
        {
            g_symmetry = symmetry_type::NONE;
        }
        if (g_trig_index[0] == trig_fn::FLIP)
        {
            g_symmetry = symmetry_type::NONE;
        }
        break;
    case fractal_type::TIMSERRORFP:
        if (g_trig_index[0] == trig_fn::FLIP)
        {
            g_symmetry = symmetry_type::NONE;
        }
        break;
    case fractal_type::MARKSMANDELPWRFP:
        if (g_trig_index[0] == trig_fn::FLIP)
        {
            g_symmetry = symmetry_type::NONE;
        }
        break;
    default:
        break;
    }
    return true;
}

bool
JuliafpSetup()
{
    g_c_exponent = (int)g_params[2];
    g_float_param = &g_param_z1;
    if (g_fractal_type == fractal_type::COMPLEXMARKSJUL)
    {
        g_power_z.x = g_params[2] - 1.0;
        g_power_z.y = g_params[3];
        g_marks_coefficient = ComplexPower(*g_float_param, g_power_z);
    }
    switch (g_fractal_type)
    {
    case fractal_type::JULIAFP:
        /*
           floating point code could probably be altered to handle many of
           the situations that otherwise are using standard_fractal().
           calcmandfp() can currently handle invert, any rqlim, potflag
           zmag, epsilon cross, and all the current outside options
        */
        if (g_debug_flag != debug_flags::force_standard_fractal
            && !g_distance_estimator
            && g_decomp[0] == 0
            && g_biomorph == -1
            && (g_inside_color >= ITER)
            && g_outside_color >= ATAN
            && g_use_init_orbit != init_orbit_mode::value
            && (g_sound_flag & SOUNDFLAG_ORBITMASK) < SOUNDFLAG_X
            && !g_finite_attractor
            && !g_using_jiim
            && g_bail_out_test == bailouts::Mod
            && (g_orbit_save_flags & osf_midi) == 0)
        {
            g_calc_type = calcmandfp; // the normal case - use calcmandfp
            calcmandfpasmstart();
        }
        else
        {
            // special case: use the main processing loop
            g_calc_type = standard_fractal;
            get_julia_attractor(0.0, 0.0);    // another attractor?
        }
        break;
    case fractal_type::FPJULIAZPOWER:
        if ((g_c_exponent & 1) || g_params[3] != 0.0 || (double)g_c_exponent != g_params[2])
        {
            g_symmetry = symmetry_type::NONE;
        }
        if (g_params[3] == 0.0 && g_debug_flag != debug_flags::force_complex_power && (double)g_c_exponent == g_params[2])
        {
            g_fractal_specific[static_cast<int>(g_fractal_type)].orbitcalc = floatZpowerFractal;
        }
        else
        {
            g_fractal_specific[static_cast<int>(g_fractal_type)].orbitcalc = floatCmplxZpowerFractal;
        }
        get_julia_attractor(g_params[0], g_params[1]);  // another attractor?
        break;
    case fractal_type::MAGNET2J:
        FloatPreCalcMagnet2();
    case fractal_type::MAGNET1J:
        g_attractor[0].x = 1.0;      // 1.0 + 0.0i always attracts
        g_attractor[0].y = 0.0;      // - both MAGNET1 and MAGNET2
        g_attractor_period[0] = 1;
        g_attractors = 1;
        get_julia_attractor(0.0, 0.0);    // another attractor?
        break;
    case fractal_type::LAMBDAFP:
        get_julia_attractor(0.0, 0.0);    // another attractor?
        get_julia_attractor(0.5, 0.0);    // another attractor?
        break;
    case fractal_type::LAMBDAEXP:
        if (g_param_z1.y == 0.0)
        {
            g_symmetry = symmetry_type::X_AXIS;
        }
        get_julia_attractor(0.0, 0.0);    // another attractor?
        break;
    case fractal_type::FPJULTRIGPLUSEXP:
    case fractal_type::FPJULTRIGPLUSZSQRD:
        if (g_param_z1.y == 0.0)
        {
            g_symmetry = symmetry_type::X_AXIS;
        }
        else
        {
            g_symmetry = symmetry_type::NONE;
        }
        if ((g_trig_index[0] == trig_fn::LOG) || (g_trig_index[0] == trig_fn::FLIP))
        {
            g_symmetry = symmetry_type::NONE;
        }
        get_julia_attractor(0.0, 0.0);    // another attractor?
        break;
    case fractal_type::HYPERCMPLXJFP:
        if (g_params[2] != 0)
        {
            g_symmetry = symmetry_type::NONE;
        }
        if (g_trig_index[0] != trig_fn::SQR)
        {
            g_symmetry = symmetry_type::NONE;
        }
    case fractal_type::QUATJULFP:
        g_attractors = 0;   // attractors broken since code checks r,i not j,k
        g_periodicity_check = 0;
        if (g_params[4] != 0.0 || g_params[5] != 0)
        {
            g_symmetry = symmetry_type::NONE;
        }
        break;
    case fractal_type::FPPOPCORN:
    case fractal_type::FPPOPCORNJUL:
    {
        bool default_functions = false;
        if (g_trig_index[0] == trig_fn::SIN
            && g_trig_index[1] == trig_fn::TAN
            && g_trig_index[2] == trig_fn::SIN
            && g_trig_index[3] == trig_fn::TAN
            && std::fabs(g_param_z2.x - 3.0) < .0001
            && g_param_z2.y == 0
            && g_param_z1.y == 0)
        {
            default_functions = true;
            if (g_fractal_type == fractal_type::FPPOPCORNJUL)
            {
                g_symmetry = symmetry_type::ORIGIN;
            }
        }
        if (default_functions && g_debug_flag == debug_flags::force_real_popcorn)
        {
            g_cur_fractal_specific->orbitcalc = PopcornFractal;
        }
        else
        {
            g_cur_fractal_specific->orbitcalc = PopcornFractalFn;
        }
        get_julia_attractor(0.0, 0.0);    // another attractor?
        break;
    }

    case fractal_type::FPCIRCLE:
        if (g_inside_color == STARTRAIL)   // FPCIRCLE locks up when used with STARTRAIL
        {
            g_inside_color = COLOR_BLACK; // arbitrarily set inside = NUMB
        }
        get_julia_attractor(0.0, 0.0);    // another attractor?
        break;
    default:
        get_julia_attractor(0.0, 0.0);    // another attractor?
        break;
    }
    return true;
}

bool
MandellongSetup()
{
    g_fudge_half = g_fudge_factor/2;
    g_c_exponent = (int)g_params[2];
    if (g_fractal_type == fractal_type::MARKSMANDEL && g_c_exponent < 1)
    {
        g_c_exponent = 1;
        g_params[2] = 1;
    }
    if ((g_fractal_type == fractal_type::MARKSMANDEL && !(g_c_exponent & 1))
        || (g_fractal_type == fractal_type::LMANDELZPOWER && (g_c_exponent & 1)))
    {
        g_symmetry = symmetry_type::XY_AXIS_NO_PARAM;    // odd exponents
    }
    if ((g_fractal_type == fractal_type::MARKSMANDEL && (g_c_exponent & 1)) || g_fractal_type == fractal_type::LMANDELEXP)
    {
        g_symmetry = symmetry_type::X_AXIS_NO_PARAM;
    }
    if (g_fractal_type == fractal_type::SPIDER && g_periodicity_check == 1)
    {
        g_periodicity_check = 4;
    }
    g_long_param = &g_l_init;
    if (g_fractal_type == fractal_type::LMANDELZPOWER)
    {
        if (g_params[3] == 0.0 && g_debug_flag != debug_flags::force_complex_power && (double)g_c_exponent == g_params[2])
        {
            g_fractal_specific[static_cast<int>(g_fractal_type)].orbitcalc = longZpowerFractal;
        }
        else
        {
            g_fractal_specific[static_cast<int>(g_fractal_type)].orbitcalc = longCmplxZpowerFractal;
        }
        if (g_params[3] != 0 || (double)g_c_exponent != g_params[2])
        {
            g_symmetry = symmetry_type::NONE;
        }
    }
    if ((g_fractal_type == fractal_type::LMANTRIGPLUSEXP) || (g_fractal_type == fractal_type::LMANTRIGPLUSZSQRD))
    {
        if (g_param_z1.y == 0.0)
        {
            g_symmetry = symmetry_type::X_AXIS;
        }
        else
        {
            g_symmetry = symmetry_type::NONE;
        }
        if ((g_trig_index[0] == trig_fn::LOG) || (g_trig_index[0] == trig_fn::FLIP))
        {
            g_symmetry = symmetry_type::NONE;
        }
    }
    if (g_fractal_type == fractal_type::TIMSERROR)
    {
        if (g_trig_index[0] == trig_fn::FLIP)
        {
            g_symmetry = symmetry_type::NONE;
        }
    }
    if (g_fractal_type == fractal_type::MARKSMANDELPWR)
    {
        if (g_trig_index[0] == trig_fn::FLIP)
        {
            g_symmetry = symmetry_type::NONE;
        }
    }
    return true;
}

bool
JulialongSetup()
{
    g_c_exponent = (int)g_params[2];
    g_long_param = &g_l_param;
    switch (g_fractal_type)
    {
    case fractal_type::LJULIAZPOWER:
        if ((g_c_exponent & 1) || g_params[3] != 0.0 || (double)g_c_exponent != g_params[2])
        {
            g_symmetry = symmetry_type::NONE;
        }
        if (g_params[3] == 0.0 && g_debug_flag != debug_flags::force_complex_power && (double)g_c_exponent == g_params[2])
        {
            g_fractal_specific[static_cast<int>(g_fractal_type)].orbitcalc = longZpowerFractal;
        }
        else
        {
            g_fractal_specific[static_cast<int>(g_fractal_type)].orbitcalc = longCmplxZpowerFractal;
        }
        break;
    case fractal_type::LAMBDA:
        get_julia_attractor(0.0, 0.0);    // another attractor?
        get_julia_attractor(0.5, 0.0);    // another attractor?
        break;
    case fractal_type::LLAMBDAEXP:
        if (g_l_param.y == 0)
        {
            g_symmetry = symmetry_type::X_AXIS;
        }
        break;
    case fractal_type::LJULTRIGPLUSEXP:
    case fractal_type::LJULTRIGPLUSZSQRD:
        if (g_param_z1.y == 0.0)
        {
            g_symmetry = symmetry_type::X_AXIS;
        }
        else
        {
            g_symmetry = symmetry_type::NONE;
        }
        if ((g_trig_index[0] == trig_fn::LOG) || (g_trig_index[0] == trig_fn::FLIP))
        {
            g_symmetry = symmetry_type::NONE;
        }
        get_julia_attractor(0.0, 0.0);    // another attractor?
        break;
    case fractal_type::LPOPCORN:
    case fractal_type::LPOPCORNJUL:
    {
        bool default_functions = false;
        if (g_trig_index[0] == trig_fn::SIN
            && g_trig_index[1] == trig_fn::TAN
            && g_trig_index[2] == trig_fn::SIN
            && g_trig_index[3] == trig_fn::TAN
            && std::fabs(g_param_z2.x - 3.0) < .0001
            && g_param_z2.y == 0
            && g_param_z1.y == 0)
        {
            default_functions = true;
            if (g_fractal_type == fractal_type::LPOPCORNJUL)
            {
                g_symmetry = symmetry_type::ORIGIN;
            }
        }
        if (default_functions && g_debug_flag == debug_flags::force_real_popcorn)
        {
            g_cur_fractal_specific->orbitcalc = LPopcornFractal;
        }
        else
        {
            g_cur_fractal_specific->orbitcalc = LPopcornFractalFn;
        }
        get_julia_attractor(0.0, 0.0);    // another attractor?
        break;
    }

    default:
        get_julia_attractor(0.0, 0.0);    // another attractor?
        break;
    }
    return true;
}

bool
TrigPlusSqrlongSetup()
{
    g_cur_fractal_specific->per_pixel =  julia_per_pixel;
    g_cur_fractal_specific->orbitcalc =  TrigPlusSqrFractal;
    if (g_l_param.x == g_fudge_factor && g_l_param.y == 0L && g_l_param2.y == 0L && g_debug_flag != debug_flags::force_standard_fractal)
    {
        if (g_l_param2.x == g_fudge_factor)          // Scott variant
        {
            g_cur_fractal_specific->orbitcalc =  ScottTrigPlusSqrFractal;
        }
        else if (g_l_param2.x == -g_fudge_factor)      // Skinner variant
        {
            g_cur_fractal_specific->orbitcalc =  SkinnerTrigSubSqrFractal;
        }
    }
    return JulialongSetup();
}

bool
TrigPlusSqrfpSetup()
{
    g_cur_fractal_specific->per_pixel =  juliafp_per_pixel;
    g_cur_fractal_specific->orbitcalc =  TrigPlusSqrfpFractal;
    if (g_param_z1.x == 1.0 && g_param_z1.y == 0.0 && g_param_z2.y == 0.0 && g_debug_flag != debug_flags::force_standard_fractal)
    {
        if (g_param_z2.x == 1.0)          // Scott variant
        {
            g_cur_fractal_specific->orbitcalc =  ScottTrigPlusSqrfpFractal;
        }
        else if (g_param_z2.x == -1.0)      // Skinner variant
        {
            g_cur_fractal_specific->orbitcalc =  SkinnerTrigSubSqrfpFractal;
        }
    }
    return JuliafpSetup();
}

bool
TrigPlusTriglongSetup()
{
    FnPlusFnSym();
    if (g_trig_index[1] == trig_fn::SQR)
    {
        return TrigPlusSqrlongSetup();
    }
    g_cur_fractal_specific->per_pixel =  long_julia_per_pixel;
    g_cur_fractal_specific->orbitcalc =  TrigPlusTrigFractal;
    if (g_l_param.x == g_fudge_factor && g_l_param.y == 0L && g_l_param2.y == 0L && g_debug_flag != debug_flags::force_standard_fractal)
    {
        if (g_l_param2.x == g_fudge_factor)          // Scott variant
        {
            g_cur_fractal_specific->orbitcalc =  ScottTrigPlusTrigFractal;
        }
        else if (g_l_param2.x == -g_fudge_factor)      // Skinner variant
        {
            g_cur_fractal_specific->orbitcalc =  SkinnerTrigSubTrigFractal;
        }
    }
    return JulialongSetup();
}

bool
TrigPlusTrigfpSetup()
{
    FnPlusFnSym();
    if (g_trig_index[1] == trig_fn::SQR)
    {
        return TrigPlusSqrfpSetup();
    }
    g_cur_fractal_specific->per_pixel =  otherjuliafp_per_pixel;
    g_cur_fractal_specific->orbitcalc =  TrigPlusTrigfpFractal;
    if (g_param_z1.x == 1.0 && g_param_z1.y == 0.0 && g_param_z2.y == 0.0 && g_debug_flag != debug_flags::force_standard_fractal)
    {
        if (g_param_z2.x == 1.0)          // Scott variant
        {
            g_cur_fractal_specific->orbitcalc =  ScottTrigPlusTrigfpFractal;
        }
        else if (g_param_z2.x == -1.0)      // Skinner variant
        {
            g_cur_fractal_specific->orbitcalc =  SkinnerTrigSubTrigfpFractal;
        }
    }
    return JuliafpSetup();
}

bool
FnPlusFnSym() // set symmetry matrix for fn+fn type
{
    static symmetry_type fnplusfn[7][7] =
    {
        // fn2 ->sin     cos    sinh    cosh   exp    log    sqr
        // fn1
        /* sin */ {symmetry_type::PI_SYM,  symmetry_type::X_AXIS,  symmetry_type::XY_AXIS, symmetry_type::X_AXIS,  symmetry_type::X_AXIS, symmetry_type::X_AXIS, symmetry_type::X_AXIS},
        /* cos */ {symmetry_type::X_AXIS,  symmetry_type::PI_SYM,  symmetry_type::X_AXIS,  symmetry_type::XY_AXIS, symmetry_type::X_AXIS, symmetry_type::X_AXIS, symmetry_type::X_AXIS},
        /* sinh*/ {symmetry_type::XY_AXIS, symmetry_type::X_AXIS,  symmetry_type::XY_AXIS, symmetry_type::X_AXIS,  symmetry_type::X_AXIS, symmetry_type::X_AXIS, symmetry_type::X_AXIS},
        /* cosh*/ {symmetry_type::X_AXIS,  symmetry_type::XY_AXIS, symmetry_type::X_AXIS,  symmetry_type::XY_AXIS, symmetry_type::X_AXIS, symmetry_type::X_AXIS, symmetry_type::X_AXIS},
        /* exp */ {symmetry_type::X_AXIS,  symmetry_type::XY_AXIS, symmetry_type::X_AXIS,  symmetry_type::X_AXIS,  symmetry_type::XY_AXIS, symmetry_type::X_AXIS, symmetry_type::X_AXIS},
        /* log */ {symmetry_type::X_AXIS,  symmetry_type::X_AXIS,  symmetry_type::X_AXIS,  symmetry_type::X_AXIS,  symmetry_type::X_AXIS, symmetry_type::X_AXIS, symmetry_type::X_AXIS},
        /* sqr */ {symmetry_type::X_AXIS,  symmetry_type::X_AXIS,  symmetry_type::X_AXIS,  symmetry_type::X_AXIS,  symmetry_type::X_AXIS, symmetry_type::X_AXIS, symmetry_type::XY_AXIS}
    };
    if (g_param_z1.y == 0.0 && g_param_z2.y == 0.0)
    {
        if (g_trig_index[0] <= trig_fn::SQR && g_trig_index[1] < trig_fn::SQR)    // bounds of array
        {
            g_symmetry = fnplusfn[static_cast<int>(g_trig_index[0])][static_cast<int>(g_trig_index[1])];
        }
        if (g_trig_index[0] == trig_fn::FLIP || g_trig_index[1] == trig_fn::FLIP)
        {
            g_symmetry = symmetry_type::NONE;
        }
    }                 // defaults to X_AXIS symmetry
    else
    {
        g_symmetry = symmetry_type::NONE;
    }
    return false;
}

bool
LambdaTrigOrTrigSetup()
{
    // default symmetry is ORIGIN
    g_long_param = &g_l_param;
    g_float_param = &g_param_z1;
    if ((g_trig_index[0] == trig_fn::EXP) || (g_trig_index[1] == trig_fn::EXP))
    {
        g_symmetry = symmetry_type::NONE;
    }
    if ((g_trig_index[0] == trig_fn::LOG) || (g_trig_index[1] == trig_fn::LOG))
    {
        g_symmetry = symmetry_type::X_AXIS;
    }
    get_julia_attractor(0.0, 0.0);       // an attractor?
    return true;
}

bool
JuliaTrigOrTrigSetup()
{
    // default symmetry is X_AXIS
    g_long_param = &g_l_param;
    g_float_param = &g_param_z1;
    if (g_param_z1.y != 0.0)
    {
        g_symmetry = symmetry_type::NONE;
    }
    if (g_trig_index[0] == trig_fn::FLIP || g_trig_index[1] == trig_fn::FLIP)
    {
        g_symmetry = symmetry_type::NONE;
    }
    get_julia_attractor(0.0, 0.0);       // an attractor?
    return true;
}

bool
ManlamTrigOrTrigSetup()
{
    // psuedo
    // default symmetry is X_AXIS
    g_long_param = &g_l_init;
    g_float_param = &g_init;
    if (g_trig_index[0] == trig_fn::SQR)
    {
        g_symmetry = symmetry_type::NONE;
    }
    if ((g_trig_index[0] == trig_fn::LOG) || (g_trig_index[1] == trig_fn::LOG))
    {
        g_symmetry = symmetry_type::NONE;
    }
    return true;
}

bool
MandelTrigOrTrigSetup()
{
    // default symmetry is X_AXIS_NO_PARAM
    g_long_param = &g_l_init;
    g_float_param = &g_init;
    if ((g_trig_index[0] == trig_fn::FLIP) || (g_trig_index[1] == trig_fn::FLIP))
    {
        g_symmetry = symmetry_type::NONE;
    }
    return true;
}


bool
ZXTrigPlusZSetup()
{
    //   static char ZXTrigPlusZSym1[] =
    // fn1 ->  sin   cos    sinh  cosh exp   log   sqr
    //           {X_AXIS, XY_AXIS, X_AXIS, XY_AXIS, X_AXIS, NONE, XY_AXIS};
    //   static char ZXTrigPlusZSym2[] =
    // fn1 ->  sin   cos    sinh  cosh exp   log   sqr
    //           {NONE, ORIGIN, NONE, ORIGIN, NONE, NONE, ORIGIN};

    if (g_params[1] == 0.0 && g_params[3] == 0.0)
    {
        //      symmetry = ZXTrigPlusZSym1[g_trig_index[0]];
        switch (g_trig_index[0])
        {
        case trig_fn::COSXX:
        case trig_fn::COSH:
        case trig_fn::SQR:
        case trig_fn::COS:
            g_symmetry = symmetry_type::XY_AXIS;
            break;
        case trig_fn::FLIP:
            g_symmetry = symmetry_type::Y_AXIS;
            break;
        case trig_fn::LOG:
            g_symmetry = symmetry_type::NONE;
            break;
        default:
            g_symmetry = symmetry_type::X_AXIS;
            break;
        }
    }
    else
    {
        //      symmetry = ZXTrigPlusZSym2[g_trig_index[0]];
        switch (g_trig_index[0])
        {
        case trig_fn::COSXX:
        case trig_fn::COSH:
        case trig_fn::SQR:
        case trig_fn::COS:
            g_symmetry = symmetry_type::ORIGIN;
            break;
        case trig_fn::FLIP:
            g_symmetry = symmetry_type::NONE;
            break;
        default:
            g_symmetry = symmetry_type::NONE;
            break;
        }
    }
    if (g_cur_fractal_specific->isinteger)
    {
        g_cur_fractal_specific->orbitcalc =  ZXTrigPlusZFractal;
        if (g_l_param.x == g_fudge_factor && g_l_param.y == 0L && g_l_param2.y == 0L && g_debug_flag != debug_flags::force_standard_fractal)
        {
            if (g_l_param2.x == g_fudge_factor)       // Scott variant
            {
                g_cur_fractal_specific->orbitcalc =  ScottZXTrigPlusZFractal;
            }
            else if (g_l_param2.x == -g_fudge_factor)      // Skinner variant
            {
                g_cur_fractal_specific->orbitcalc =  SkinnerZXTrigSubZFractal;
            }
        }
        return JulialongSetup();
    }
    else
    {
        g_cur_fractal_specific->orbitcalc =  ZXTrigPlusZfpFractal;
        if (g_param_z1.x == 1.0 && g_param_z1.y == 0.0 && g_param_z2.y == 0.0 && g_debug_flag != debug_flags::force_standard_fractal)
        {
            if (g_param_z2.x == 1.0)       // Scott variant
            {
                g_cur_fractal_specific->orbitcalc =  ScottZXTrigPlusZfpFractal;
            }
            else if (g_param_z2.x == -1.0)           // Skinner variant
            {
                g_cur_fractal_specific->orbitcalc =  SkinnerZXTrigSubZfpFractal;
            }
        }
    }
    return JuliafpSetup();
}

bool
LambdaTrigSetup()
{
    bool const isinteger = g_cur_fractal_specific->isinteger != 0;
    if (isinteger)
    {
        g_cur_fractal_specific->orbitcalc =  LambdaTrigFractal;
    }
    else
    {
        g_cur_fractal_specific->orbitcalc =  LambdaTrigfpFractal;
    }
    switch (g_trig_index[0])
    {
    case trig_fn::SIN:
    case trig_fn::COSXX:
    case trig_fn::COS:
        g_symmetry = symmetry_type::PI_SYM;
        if (isinteger)
        {
            g_cur_fractal_specific->orbitcalc =  LambdaTrigFractal1;
        }
        else
        {
            g_cur_fractal_specific->orbitcalc =  LambdaTrigfpFractal1;
        }
        break;
    case trig_fn::SINH:
    case trig_fn::COSH:
        g_symmetry = symmetry_type::ORIGIN;
        if (isinteger)
        {
            g_cur_fractal_specific->orbitcalc =  LambdaTrigFractal2;
        }
        else
        {
            g_cur_fractal_specific->orbitcalc =  LambdaTrigfpFractal2;
        }
        break;
    case trig_fn::SQR:
        g_symmetry = symmetry_type::ORIGIN;
        break;
    case trig_fn::EXP:
        if (isinteger)
        {
            g_cur_fractal_specific->orbitcalc =  LongLambdaexponentFractal;
        }
        else
        {
            g_cur_fractal_specific->orbitcalc =  LambdaexponentFractal;
        }
        g_symmetry = symmetry_type::NONE;
        break;
    case trig_fn::LOG:
        g_symmetry = symmetry_type::NONE;
        break;
    default:   // default for additional functions
        g_symmetry = symmetry_type::ORIGIN;
        break;
    }
    get_julia_attractor(0.0, 0.0);       // an attractor?
    if (isinteger)
    {
        return JulialongSetup();
    }
    else
    {
        return JuliafpSetup();
    }
}

bool
JuliafnPlusZsqrdSetup()
{
    //   static char fnpluszsqrd[] =
    // fn1 ->  sin   cos    sinh  cosh   sqr    exp   log
    // sin    {NONE, ORIGIN, NONE, ORIGIN, ORIGIN, NONE, NONE};

    switch (g_trig_index[0]) // fix sqr symmetry & add additional functions
    {
    case trig_fn::COSXX: // cosxx
    case trig_fn::COSH:
    case trig_fn::SQR:
    case trig_fn::COS:
    case trig_fn::TAN:
    case trig_fn::TANH:
        g_symmetry = symmetry_type::ORIGIN;
        break;
        // default is for NONE symmetry

    default:
        break;
    }
    if (g_cur_fractal_specific->isinteger)
    {
        return JulialongSetup();
    }
    else
    {
        return JuliafpSetup();
    }
}

bool
SqrTrigSetup()
{
    //   static char SqrTrigSym[] =
    // fn1 ->  sin    cos    sinh   cosh   sqr    exp   log
    //           {PI_SYM, PI_SYM, XY_AXIS, XY_AXIS, XY_AXIS, X_AXIS, X_AXIS};
    switch (g_trig_index[0]) // fix sqr symmetry & add additional functions
    {
    case trig_fn::SIN:
    case trig_fn::COSXX: // cosxx
    case trig_fn::COS:   // 'real' cos
        g_symmetry = symmetry_type::PI_SYM;
        break;
        // default is for X_AXIS symmetry

    default:
        break;
    }
    if (g_cur_fractal_specific->isinteger)
    {
        return JulialongSetup();
    }
    else
    {
        return JuliafpSetup();
    }
}

bool
FnXFnSetup()
{
    static symmetry_type fnxfn[7][7] =
    {
        // fn2 ->sin     cos    sinh    cosh  exp    log    sqr
        // fn1
        /* sin */ {symmetry_type::PI_SYM,  symmetry_type::Y_AXIS,  symmetry_type::XY_AXIS, symmetry_type::XY_AXIS, symmetry_type::X_AXIS,  symmetry_type::NONE,   symmetry_type::XY_AXIS},
        /* cos */ {symmetry_type::Y_AXIS,  symmetry_type::PI_SYM,  symmetry_type::XY_AXIS, symmetry_type::XY_AXIS, symmetry_type::X_AXIS,  symmetry_type::NONE,   symmetry_type::XY_AXIS},
        /* sinh*/ {symmetry_type::XY_AXIS, symmetry_type::XY_AXIS, symmetry_type::XY_AXIS, symmetry_type::XY_AXIS, symmetry_type::X_AXIS,  symmetry_type::NONE,   symmetry_type::XY_AXIS},
        /* cosh*/ {symmetry_type::XY_AXIS, symmetry_type::XY_AXIS, symmetry_type::XY_AXIS, symmetry_type::XY_AXIS, symmetry_type::X_AXIS,  symmetry_type::NONE,   symmetry_type::XY_AXIS},
        /* exp */ {symmetry_type::X_AXIS,  symmetry_type::X_AXIS,  symmetry_type::X_AXIS,  symmetry_type::X_AXIS,  symmetry_type::X_AXIS,  symmetry_type::NONE,   symmetry_type::XY_AXIS},
        /* log */ {symmetry_type::NONE,    symmetry_type::NONE,    symmetry_type::NONE,    symmetry_type::NONE,    symmetry_type::NONE,    symmetry_type::X_AXIS, symmetry_type::NONE},
        /* sqr */ {symmetry_type::XY_AXIS, symmetry_type::XY_AXIS, symmetry_type::XY_AXIS, symmetry_type::XY_AXIS, symmetry_type::XY_AXIS, symmetry_type::NONE,   symmetry_type::XY_AXIS},
    };
    if (g_trig_index[0] <= trig_fn::SQR && g_trig_index[1] <= trig_fn::SQR)    // bounds of array
    {
        g_symmetry = fnxfn[static_cast<int>(g_trig_index[0])][static_cast<int>(g_trig_index[1])];
        // defaults to X_AXIS symmetry
    }
    else
    {
        if (g_trig_index[0] == trig_fn::LOG || g_trig_index[1] == trig_fn::LOG)
        {
            g_symmetry = symmetry_type::NONE;
        }
        if (g_trig_index[0] == trig_fn::COS || g_trig_index[1] == trig_fn::COS)
        {
            if (g_trig_index[0] == trig_fn::SIN || g_trig_index[1] == trig_fn::SIN)
            {
                g_symmetry = symmetry_type::PI_SYM;
            }
            if (g_trig_index[0] == trig_fn::COSXX || g_trig_index[1] == trig_fn::COSXX)
            {
                g_symmetry = symmetry_type::PI_SYM;
            }
        }
        if (g_trig_index[0] == trig_fn::COS && g_trig_index[1] == trig_fn::COS)
        {
            g_symmetry = symmetry_type::PI_SYM;
        }
    }
    if (g_cur_fractal_specific->isinteger)
    {
        return JulialongSetup();
    }
    else
    {
        return JuliafpSetup();
    }
}

bool
MandelTrigSetup()
{
    bool const isinteger = g_cur_fractal_specific->isinteger != 0;
    if (isinteger)
    {
        g_cur_fractal_specific->orbitcalc =  LambdaTrigFractal;
    }
    else
    {
        g_cur_fractal_specific->orbitcalc =  LambdaTrigfpFractal;
    }
    g_symmetry = symmetry_type::XY_AXIS_NO_PARAM;
    switch (g_trig_index[0])
    {
    case trig_fn::SIN:
    case trig_fn::COSXX:
        if (isinteger)
        {
            g_cur_fractal_specific->orbitcalc =  LambdaTrigFractal1;
        }
        else
        {
            g_cur_fractal_specific->orbitcalc =  LambdaTrigfpFractal1;
        }
        break;
    case trig_fn::SINH:
    case trig_fn::COSH:
        if (isinteger)
        {
            g_cur_fractal_specific->orbitcalc =  LambdaTrigFractal2;
        }
        else
        {
            g_cur_fractal_specific->orbitcalc =  LambdaTrigfpFractal2;
        }
        break;
    case trig_fn::EXP:
        g_symmetry = symmetry_type::X_AXIS_NO_PARAM;
        if (isinteger)
        {
            g_cur_fractal_specific->orbitcalc =  LongLambdaexponentFractal;
        }
        else
        {
            g_cur_fractal_specific->orbitcalc =  LambdaexponentFractal;
        }
        break;
    case trig_fn::LOG:
        g_symmetry = symmetry_type::X_AXIS_NO_PARAM;
        break;
    default:
        g_symmetry = symmetry_type::XY_AXIS_NO_PARAM;
        break;
    }
    if (isinteger)
    {
        return MandellongSetup();
    }
    else
    {
        return MandelfpSetup();
    }
}

bool
MarksJuliaSetup()
{
    if (g_params[2] < 1)
    {
        g_params[2] = 1;
    }
    g_c_exponent = (int)g_params[2];
    g_long_param = &g_l_param;
    g_l_old_z = *g_long_param;
    if (g_c_exponent > 3)
    {
        lcpower(&g_l_old_z, g_c_exponent-1, &g_l_coefficient, g_bit_shift);
    }
    else if (g_c_exponent == 3)
    {
        g_l_coefficient.x = multiply(g_l_old_z.x, g_l_old_z.x, g_bit_shift) - multiply(g_l_old_z.y, g_l_old_z.y, g_bit_shift);
        g_l_coefficient.y = multiply(g_l_old_z.x, g_l_old_z.y, g_bit_shift_less_1);
    }
    else if (g_c_exponent == 2)
    {
        g_l_coefficient = g_l_old_z;
    }
    else if (g_c_exponent < 2)
    {
        g_l_coefficient.x = 1L << g_bit_shift;
        g_l_coefficient.y = 0L;
    }
    get_julia_attractor(0.0, 0.0);       // an attractor?
    return true;
}

bool
MarksJuliafpSetup()
{
    if (g_params[2] < 1)
    {
        g_params[2] = 1;
    }
    g_c_exponent = (int)g_params[2];
    g_float_param = &g_param_z1;
    g_old_z = *g_float_param;
    if (g_c_exponent > 3)
    {
        cpower(&g_old_z, g_c_exponent-1, &g_marks_coefficient);
    }
    else if (g_c_exponent == 3)
    {
        g_marks_coefficient.x = sqr(g_old_z.x) - sqr(g_old_z.y);
        g_marks_coefficient.y = g_old_z.x * g_old_z.y * 2;
    }
    else if (g_c_exponent == 2)
    {
        g_marks_coefficient = g_old_z;
    }
    else if (g_c_exponent < 2)
    {
        g_marks_coefficient.x = 1.0;
        g_marks_coefficient.y = 0.0;
    }
    get_julia_attractor(0.0, 0.0);       // an attractor?
    return true;
}

bool
SierpinskiSetup()
{
    // sierpinski
    g_periodicity_check = 0;                // disable periodicity checks
    g_l_temp.x = 1;
    g_l_temp.x = g_l_temp.x << g_bit_shift; // ltmp.x = 1
    g_l_temp.y = g_l_temp.x >> 1;                        // ltmp.y = .5
    return true;
}

bool
SierpinskiFPSetup()
{
    // sierpinski
    g_periodicity_check = 0;                // disable periodicity checks
    g_tmp_z.x = 1;
    g_tmp_z.y = 0.5;
    return true;
}

bool
HalleySetup()
{
    // Halley
    g_periodicity_check = 0;

    if (g_user_float_flag)
    {
        g_fractal_type = fractal_type::HALLEY; // float on
    }
    else
    {
        g_fractal_type = fractal_type::MPHALLEY;
    }

    g_cur_fractal_specific = &g_fractal_specific[static_cast<int>(g_fractal_type)];

    g_degree = (int)g_param_z1.x;
    if (g_degree < 2)
    {
        g_degree = 2;
    }
    g_params[0] = (double)g_degree;

    //  precalculated values
    g_halley_a_plus_one = g_degree + 1; // a+1
    g_halley_a_plus_one_times_degree = g_halley_a_plus_one * g_degree;

    if (g_fractal_type == fractal_type::MPHALLEY)
    {
        setMPfunctions();
        g_halley_mp_a_plus_one = *pd2MP((double)g_halley_a_plus_one);
        g_halley_mp_a_plus_one_times_degree = *pd2MP((double)g_halley_a_plus_one_times_degree);
        g_mpc_temp_param.x = *pd2MP(g_param_z1.y);
        g_mpc_temp_param.y = *pd2MP(g_param_z2.y);
        g_mp_temp_param2_x = *pd2MP(g_param_z2.x);
        g_mp_one        = *pd2MP(1.0);
    }

    if (g_degree % 2)
    {
        g_symmetry = symmetry_type::X_AXIS;   // odd
    }
    else
    {
        g_symmetry = symmetry_type::XY_AXIS; // even
    }
    return true;
}

bool
StandardSetup()
{
    if (g_fractal_type == fractal_type::UNITYFP)
    {
        g_periodicity_check = 0;
    }
    return true;
}
