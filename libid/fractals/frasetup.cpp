// SPDX-License-Identifier: GPL-3.0-only
//
#include "fractals/frasetup.h"

#include "engine/bailout_formula.h"
#include "engine/calcfrac.h"
#include "engine/calmanfp.h"
#include "engine/cmdfiles.h"
#include "engine/engine_timer.h"
#include "engine/fractals.h"
#include "engine/get_julia_attractor.h"
#include "engine/id_data.h"
#include "engine/perturbation.h"
#include "fractals/fractalp.h"
#include "fractals/fractype.h"
#include "fractals/magnet.h"
#include "fractals/peterson_variations.h"
#include "fractals/pickover_mandelbrot.h"
#include "fractals/popcorn.h"
#include "misc/debug_flags.h"
#include "ui/editpal.h"
#include "ui/trig_fns.h"

#include <cmath>

// --------------------------------------------------------------------
//              Setup (once per fractal image) routines
// --------------------------------------------------------------------

// Mandelbrot Routine
bool burning_ship_per_image()
{
    if (g_std_calc_mode == CalcMode::PERTURBATION &&
        bit_set(g_cur_fractal_specific->flags, FractalFlags::PERTURB))
    {
        return mandel_perturbation_per_image();
    }
    // use the main processing loop
    g_calc_type = standard_fractal_type;
    return true;
}

bool standalone_per_image()
{
    engine_timer(g_cur_fractal_specific->calc_type);
    return false;               // effectively disable solid-guessing
}

bool mandel_perturbation_per_image()
{
    return perturbation();
}

bool mandel_z_power_perturbation_per_image()
{
    constexpr int MAX_POWER{28};
    g_c_exponent = std::min(std::max(g_c_exponent, 2), MAX_POWER);
    return perturbation();
}

bool mandel_per_image()
{
    g_bf_math = BFMathType::NONE;
    g_c_exponent = (int)g_params[2];
    g_power_z.x = g_params[2] - 1.0;
    g_power_z.y = g_params[3];
    g_float_param = &g_init;
    switch (g_fractal_type)
    {
    case FractalType::MARKS_MANDEL:
        if (g_c_exponent < 1)
        {
            g_c_exponent = 1;
            g_params[2] = 1;
        }
        if (!(g_c_exponent & 1))
        {
            g_symmetry = SymmetryType::XY_AXIS_NO_PARAM;    // odd exponents
        }
        if (g_c_exponent & 1)
        {
            g_symmetry = SymmetryType::X_AXIS_NO_PARAM;
        }
        break;

    case FractalType::MANDEL:
        /*
           floating point code could probably be altered to handle many of
           the situations that otherwise are using standard_fractal().
           calcmandfp() can currently handle invert, any rqlim, potflag
           zmag, epsilon cross, and all the current outside options
        */
        if (g_std_calc_mode == CalcMode::PERTURBATION &&
            bit_set(g_cur_fractal_specific->flags, FractalFlags::PERTURB))
        {
            return mandel_perturbation_per_image();
        }
        if (g_debug_flag != DebugFlags::FORCE_STANDARD_FRACTAL     //
            && !g_distance_estimator                               //
            && g_decomp[0] == 0                                    //
            && g_biomorph == -1                                    //
            && g_inside_color >= ITER                              //
            && g_outside_color >= ATAN                             //
            && g_use_init_orbit != InitOrbitMode::VALUE            //
            && (g_sound_flag & SOUNDFLAG_ORBIT_MASK) < SOUNDFLAG_X //
            && !g_using_jiim                                       //
            && g_bailout_test == Bailout::MOD                      //
            && (g_orbit_save_flags & OSF_MIDI) == 0)
        {
            g_calc_type = calc_mandelbrot_type; // the normal case
            calc_mandelbrot_init();
        }
        else
        {
            // special case: use the main processing loop
            g_calc_type = standard_fractal_type;
        }
        break;

    case FractalType::MANDEL_Z_POWER:
        if (g_std_calc_mode == CalcMode::PERTURBATION &&
            bit_set(g_cur_fractal_specific->flags, FractalFlags::PERTURB))
        {
            if (g_c_exponent == 2)
            {
                return mandel_perturbation_per_image();
            }
            if (g_c_exponent > 2)
            {
                return mandel_z_power_perturbation_per_image();
            }
        }
        if ((double)g_c_exponent == g_params[2] && (g_c_exponent & 1))   // odd exponents
        {
            g_symmetry = SymmetryType::XY_AXIS_NO_PARAM;
        }
        if (g_params[3] != 0)
        {
            g_symmetry = SymmetryType::NONE;
        }
        if (g_params[3] == 0.0 && g_debug_flag != DebugFlags::FORCE_COMPLEX_POWER && (double)g_c_exponent == g_params[2])
        {
            get_fractal_specific(g_fractal_type)->orbit_calc = mandel_z_power_orbit;
        }
        else
        {
            get_fractal_specific(g_fractal_type)->orbit_calc = mandel_z_power_cmplx_orbit;
        }
        break;
    case FractalType::MAGNET_1M:
    case FractalType::MAGNET_2M:
        g_attractor[0].x = 1.0;      // 1.0 + 0.0i always attracts
        g_attractor[0].y = 0.0;      // - both MAGNET1 and MAGNET2
        g_attractor_period[0] = 1;
        g_attractors = 1;
        break;
    case FractalType::SPIDER:
        if (g_periodicity_check == 1)   // if not user set
        {
            g_periodicity_check = 4;
        }
        break;
    case FractalType::FN_PLUS_FN:
        g_symmetry = SymmetryType::X_AXIS_NO_PARAM;
        break;
    case FractalType::MANDEL_FN_PLUS_EXP:
    case FractalType::MANDEL_FN_PLUS_Z_SQRD:
        if (g_param_z1.y == 0.0)
        {
            g_symmetry = SymmetryType::X_AXIS;
        }
        else
        {
            g_symmetry = SymmetryType::NONE;
        }
        if ((g_trig_index[0] == TrigFn::LOG) || (g_trig_index[0] == TrigFn::FLIP))
        {
            g_symmetry = SymmetryType::NONE;
        }
        break;
    case FractalType::QUAT:
        g_float_param = &g_tmp_z;
        g_attractors = 0;
        g_periodicity_check = 0;
        break;
    case FractalType::HYPER_CMPLX:
        g_float_param = &g_tmp_z;
        g_attractors = 0;
        g_periodicity_check = 0;
        if (g_params[2] != 0)
        {
            g_symmetry = SymmetryType::NONE;
        }
        if (g_trig_index[0] == TrigFn::FLIP)
        {
            g_symmetry = SymmetryType::NONE;
        }
        break;
    case FractalType::TIMS_ERROR:
        if (g_trig_index[0] == TrigFn::FLIP)
        {
            g_symmetry = SymmetryType::NONE;
        }
        break;
    case FractalType::MARKS_MANDEL_PWR:
        if (g_trig_index[0] == TrigFn::FLIP)
        {
            g_symmetry = SymmetryType::NONE;
        }
        break;
    default:
        break;
    }
    return true;
}

bool julia_per_image()
{
    g_c_exponent = (int)g_params[2];
    g_float_param = &g_param_z1;
    if (g_fractal_type == FractalType::COMPLEX_MARKS_JUL)
    {
        g_power_z.x = g_params[2] - 1.0;
        g_power_z.y = g_params[3];
        g_marks_coefficient = complex_power(*g_float_param, g_power_z);
    }
    switch (g_fractal_type)
    {
    case FractalType::JULIA:
        /*
           floating point code could probably be altered to handle many of
           the situations that otherwise are using standard_fractal().
           calcmandfp() can currently handle invert, any rqlim, potflag
           zmag, epsilon cross, and all the current outside options
        */
        if (g_debug_flag != DebugFlags::FORCE_STANDARD_FRACTAL
            && !g_distance_estimator
            && g_decomp[0] == 0
            && g_biomorph == -1
            && (g_inside_color >= ITER)
            && g_outside_color >= ATAN
            && g_use_init_orbit != InitOrbitMode::VALUE
            && (g_sound_flag & SOUNDFLAG_ORBIT_MASK) < SOUNDFLAG_X
            && !g_finite_attractor
            && !g_using_jiim
            && g_bailout_test == Bailout::MOD
            && (g_orbit_save_flags & OSF_MIDI) == 0)
        {
            g_calc_type = calc_mandelbrot_type; // the normal case
            calc_mandelbrot_init();
        }
        else
        {
            // special case: use the main processing loop
            g_calc_type = standard_fractal_type;
            get_julia_attractor(0.0, 0.0);    // another attractor?
        }
        break;
    case FractalType::JULIA_Z_POWER:
        if ((g_c_exponent & 1) || g_params[3] != 0.0 || (double)g_c_exponent != g_params[2])
        {
            g_symmetry = SymmetryType::NONE;
        }
        if (g_params[3] == 0.0 && g_debug_flag != DebugFlags::FORCE_COMPLEX_POWER && (double)g_c_exponent == g_params[2])
        {
            get_fractal_specific(g_fractal_type)->orbit_calc = mandel_z_power_orbit;
        }
        else
        {
            get_fractal_specific(g_fractal_type)->orbit_calc = mandel_z_power_cmplx_orbit;
        }
        get_julia_attractor(g_params[0], g_params[1]);  // another attractor?
        break;
    case FractalType::MAGNET_2J:
        float_pre_calc_magnet2();
    case FractalType::MAGNET_1J:
        g_attractor[0].x = 1.0;      // 1.0 + 0.0i always attracts
        g_attractor[0].y = 0.0;      // - both MAGNET1 and MAGNET2
        g_attractor_period[0] = 1;
        g_attractors = 1;
        get_julia_attractor(0.0, 0.0);    // another attractor?
        break;
    case FractalType::LAMBDA:
        get_julia_attractor(0.0, 0.0);    // another attractor?
        get_julia_attractor(0.5, 0.0);    // another attractor?
        break;
    case FractalType::MAN_O_WAR:
        if (g_param_z1.y == 0.0)
        {
            g_symmetry = SymmetryType::X_AXIS;
        }
        get_julia_attractor(0.0, 0.0);    // another attractor?
        break;
    case FractalType::JULIA_FN_PLUS_EXP:
    case FractalType::JULIA_FN_PLUS_Z_SQRD:
        if (g_param_z1.y == 0.0)
        {
            g_symmetry = SymmetryType::X_AXIS;
        }
        else
        {
            g_symmetry = SymmetryType::NONE;
        }
        if ((g_trig_index[0] == TrigFn::LOG) || (g_trig_index[0] == TrigFn::FLIP))
        {
            g_symmetry = SymmetryType::NONE;
        }
        get_julia_attractor(0.0, 0.0);    // another attractor?
        break;
    case FractalType::HYPER_CMPLX_J:
        if (g_params[2] != 0)
        {
            g_symmetry = SymmetryType::NONE;
        }
        if (g_trig_index[0] != TrigFn::SQR)
        {
            g_symmetry = SymmetryType::NONE;
        }
    case FractalType::QUAT_JUL:
        g_attractors = 0;   // attractors broken since code checks r,i not j,k
        g_periodicity_check = 0;
        if (g_params[4] != 0.0 || g_params[5] != 0)
        {
            g_symmetry = SymmetryType::NONE;
        }
        break;
    case FractalType::POPCORN:
    case FractalType::POPCORN_JUL:
    {
        bool default_functions = false;
        if (g_trig_index[0] == TrigFn::SIN
            && g_trig_index[1] == TrigFn::TAN
            && g_trig_index[2] == TrigFn::SIN
            && g_trig_index[3] == TrigFn::TAN
            && std::abs(g_param_z2.x - 3.0) < .0001
            && g_param_z2.y == 0
            && g_param_z1.y == 0)
        {
            default_functions = true;
            if (g_fractal_type == FractalType::POPCORN_JUL)
            {
                g_symmetry = SymmetryType::ORIGIN;
            }
        }
        if (default_functions && g_debug_flag == DebugFlags::FORCE_REAL_POPCORN)
        {
            g_cur_fractal_specific->orbit_calc = popcorn_fractal;
        }
        else
        {
            g_cur_fractal_specific->orbit_calc = popcorn_orbit;
        }
        get_julia_attractor(0.0, 0.0);    // another attractor?
        break;
    }

    case FractalType::CIRCLE:
        if (g_inside_color == STAR_TRAIL)   // CIRCLE locks up when used with STAR_TRAIL
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

bool standard_per_image()
{
    if (g_fractal_type == FractalType::UNITY)
    {
        g_periodicity_check = 0;
    }
    return true;
}
