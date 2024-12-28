// SPDX-License-Identifier: GPL-3.0-only
//
#include "frasetup.h"

#include "bailout_formula.h"
#include "calcfrac.h"
#include "calmanfp.h"
#include "cmdfiles.h"
#include "debug_flags.h"
#include "editpal.h"
#include "engine_timer.h"
#include "fractalp.h"
#include "fractals.h"
#include "fractype.h"
#include "get_julia_attractor.h"
#include "id_data.h"
#include "magnet.h"
#include "mpmath.h"
#include "mpmath_c.h"
#include "perturbation.h"
#include "peterson_variations.h"
#include "pickover_mandelbrot.h"
#include "popcorn.h"
#include "prototyp.h"
#include "trig_fns.h"

#include <cmath>

// --------------------------------------------------------------------
//              Setup (once per fractal image) routines
// --------------------------------------------------------------------

bool
mandel_setup()           // Mandelbrot Routine
{
    if (g_std_calc_mode == 'p' && bit_set(g_cur_fractal_specific->flags, fractal_flags::PERTURB))
    {
        return mandel_perturbation_setup();
    }
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
        && g_bail_out_test == Bailout::MOD
        && (g_orbit_save_flags & osf_midi) == 0)
    {
        g_calc_type = calc_mand; // the normal case - use CALCMAND
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
julia_setup()            // Julia Routine
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
        && g_bail_out_test == Bailout::MOD
        && (g_orbit_save_flags & osf_midi) == 0)
    {
        g_calc_type = calc_mand; // the normal case - use CALCMAND
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
standalone_setup()
{
    timer(timer_type::ENGINE, g_cur_fractal_specific->calctype);
    return false;               // effectively disable solid-guessing
}

bool mandel_perturbation_setup()
{
    return perturbation();
}

bool mandel_z_power_perturbation_setup()
{
    constexpr int MAX_POWER{28};
    g_c_exponent = std::min(std::max(g_c_exponent, 2), MAX_POWER);
    return perturbation();
}

bool
mandel_fp_setup()
{
    g_bf_math = BFMathType::NONE;
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
        if (g_std_calc_mode == 'p' && bit_set(g_cur_fractal_specific->flags, fractal_flags::PERTURB))
        {
            return mandel_perturbation_setup();
        }
        if (g_debug_flag != debug_flags::force_standard_fractal
            && !g_distance_estimator
            && g_decomp[0] == 0
            && g_biomorph == -1
            && (g_inside_color >= ITER)
            && g_outside_color >= ATAN
            && g_use_init_orbit != init_orbit_mode::value
            && (g_sound_flag & SOUNDFLAG_ORBITMASK) < SOUNDFLAG_X
            && !g_using_jiim
            && g_bail_out_test == Bailout::MOD
            && (g_orbit_save_flags & osf_midi) == 0)
        {
            g_calc_type = calc_mand_fp; // the normal case - use calcmandfp
            calc_mand_fp_asm_start();
        }
        else
        {
            // special case: use the main processing loop
            g_calc_type = standard_fractal;
        }
        break;

    case fractal_type::FPMANDELZPOWER:
        if (g_std_calc_mode == 'p' && bit_set(g_cur_fractal_specific->flags, fractal_flags::PERTURB))
        {
            if (g_c_exponent == 2)
            {
                return mandel_perturbation_setup();
            }
            if (g_c_exponent > 2)
            {
                return mandel_z_power_perturbation_setup();
            }
        }
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
            g_fractal_specific[+g_fractal_type].orbitcalc = float_z_power_fractal;
        }
        else
        {
            g_fractal_specific[+g_fractal_type].orbitcalc = float_cmplx_z_power_fractal;
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
julia_fp_setup()
{
    g_c_exponent = (int)g_params[2];
    g_float_param = &g_param_z1;
    if (g_fractal_type == fractal_type::COMPLEXMARKSJUL)
    {
        g_power_z.x = g_params[2] - 1.0;
        g_power_z.y = g_params[3];
        g_marks_coefficient = complex_power(*g_float_param, g_power_z);
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
            && g_bail_out_test == Bailout::MOD
            && (g_orbit_save_flags & osf_midi) == 0)
        {
            g_calc_type = calc_mand_fp; // the normal case - use calcmandfp
            calc_mand_fp_asm_start();
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
            g_fractal_specific[+g_fractal_type].orbitcalc = float_z_power_fractal;
        }
        else
        {
            g_fractal_specific[+g_fractal_type].orbitcalc = float_cmplx_z_power_fractal;
        }
        get_julia_attractor(g_params[0], g_params[1]);  // another attractor?
        break;
    case fractal_type::MAGNET2J:
        float_pre_calc_magnet2();
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
            g_cur_fractal_specific->orbitcalc = popcorn_fractal;
        }
        else
        {
            g_cur_fractal_specific->orbitcalc = popcorn_fractal_fn;
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
mandel_long_setup()
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
            g_fractal_specific[+g_fractal_type].orbitcalc = long_z_power_fractal;
        }
        else
        {
            g_fractal_specific[+g_fractal_type].orbitcalc = long_cmplx_z_power_fractal;
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
julia_long_setup()
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
            g_fractal_specific[+g_fractal_type].orbitcalc = long_z_power_fractal;
        }
        else
        {
            g_fractal_specific[+g_fractal_type].orbitcalc = long_cmplx_z_power_fractal;
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
            g_cur_fractal_specific->orbitcalc = long_popcorn_fractal;
        }
        else
        {
            g_cur_fractal_specific->orbitcalc = long_popcorn_fractal_fn;
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
standard_setup()
{
    if (g_fractal_type == fractal_type::UNITYFP)
    {
        g_periodicity_check = 0;
    }
    return true;
}
