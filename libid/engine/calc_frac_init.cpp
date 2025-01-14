// SPDX-License-Identifier: GPL-3.0-only
//
#include "engine/calc_frac_init.h"

#include "engine/bailout_formula.h"
#include "engine/calcfrac.h"
#include "engine/convert_center_mag.h"
#include "engine/convert_corners.h"
#include "engine/fractalb.h"
#include "engine/get_prec_big_float.h"
#include "engine/id_data.h"
#include "engine/pixel_grid.h"
#include "engine/pixel_limits.h"
#include "engine/soi.h"
#include "engine/type_has_param.h"
#include "fractals/fractalp.h"
#include "fractals/jb.h"
#include "math/biginit.h"
#include "math/fixed_pt.h"
#include "math/sign.h"
#include "misc/debug_flags.h"
#include "misc/id.h"
#include "ui/cmdfiles.h"
#include "ui/stop_msg.h"

#include <config/port.h>

#include <algorithm>
#include <array>
#include <cfloat>
#include <cmath>
#include <cstdio>

enum
{
    FUDGE_FACTOR = 29, // fudge all values up by 2**this
    FUDGE_FACTOR2 = 24 // (or maybe this)
};

static long   fudge_to_long(double d);
static double fudge_to_double(long l);
static void   adjust_to_limits(double expand);
static void   smallest_add(double *num);
static int    ratio_bad(double actual, double desired);
static void   adjust_to_limits_bf(double expand);
static void   smallest_add_bf(bf_t num);

/* This function calculates the precision needed to distinguish adjacent
   pixels at the maximum resolution of MAX_PIXELS by MAX_PIXELS
   (if res==Resolution::MAX) or at current resolution (if res==Resolution::CURRENT)    */
static int get_prec_dbl(ResolutionFlag flag)
{
    LDouble res;
    if (flag == ResolutionFlag::MAX)
    {
        res = OLD_MAX_PIXELS -1;
    }
    else
    {
        res = g_logical_screen_x_dots-1;
    }

    LDouble x_delta = ((LDouble) g_x_max - (LDouble) g_x_3rd) / res;
    LDouble y_delta2 = ((LDouble) g_y_3rd - (LDouble) g_y_min) / res;

    if (flag == ResolutionFlag::CURRENT)
    {
        res = g_logical_screen_y_dots-1;
    }

    LDouble y_delta = ((LDouble) g_y_max - (LDouble) g_y_3rd) / res;
    LDouble x_delta2 = ((LDouble) g_x_3rd - (LDouble) g_x_min) / res;

    LDouble del1 = std::abs(x_delta) + std::abs(x_delta2);
    LDouble del2 = std::abs(y_delta) + std::abs(y_delta2);
    del1 = std::min(del2, del1);
    if (del1 == 0)
    {
#ifndef NDEBUG
        show_corners_dbl("getprecdbl");
#endif
        return -1;
    }
    int digits = 1;
    while (del1 < 1.0)
    {
        digits++;
        del1 *= 10;
    }
    digits = std::max(digits, 3);
    return digits;
}

void fractal_float_to_bf()
{
    init_bf_dec(get_prec_dbl(ResolutionFlag::CURRENT));
    float_to_bf(g_bf_x_min, g_x_min);
    float_to_bf(g_bf_x_max, g_x_max);
    float_to_bf(g_bf_y_min, g_y_min);
    float_to_bf(g_bf_y_max, g_y_max);
    float_to_bf(g_bf_x_3rd, g_x_3rd);
    float_to_bf(g_bf_y_3rd, g_y_3rd);

    for (int i = 0; i < MAX_PARAMS; i++)
    {
        if (type_has_param(g_fractal_type, i, nullptr))
        {
            float_to_bf(g_bf_params[i], g_params[i]);
        }
    }
    g_calc_status = CalcStatus::PARAMS_CHANGED;
}

void calc_frac_init() // initialize a *pile* of stuff for fractal calculation
{
    g_old_color_iter = 0L;
    g_color_iter = 0L;
    for (int i = 0; i < 10; i++)
    {
        g_rhombus_stack[i] = 0;
    }

    // set up grid array compactly leaving space at end
    // space req for grid is 2(xdots+ydots)*sizeof(long or double)
    // space available in extraseg is 65536 Bytes
    long xy_temp = g_logical_screen_x_dots + g_logical_screen_y_dots;
    if ((!g_user_float_flag && (xy_temp*sizeof(long) > 32768))
        || (g_user_float_flag && (xy_temp*sizeof(double) > 32768))
        || g_debug_flag == DebugFlags::PREVENT_COORDINATE_GRID)
    {
        g_use_grid = false;
        g_float_flag = true;
        g_user_float_flag = true;
    }
    else
    {
        g_use_grid = true;
    }

    set_grid_pointers();

    if (bit_clear(g_cur_fractal_specific->flags, FractalFlags::BF_MATH))
    {
        FractalType to_float = g_cur_fractal_specific->to_float;
        if (to_float == FractalType::NO_FRACTAL ||
            bit_clear(g_fractal_specific[+to_float].flags, FractalFlags::BF_MATH))
        {
            g_bf_math = BFMathType::NONE;
        }
        else if (g_bf_math != BFMathType::NONE)
        {
            g_cur_fractal_specific = &g_fractal_specific[+to_float];
            g_fractal_type = to_float;
        }
    }

    // switch back to double when zooming out if using arbitrary precision
    if (g_bf_math != BFMathType::NONE)
    {
        int got_prec = get_prec_bf(ResolutionFlag::CURRENT);
        if ((got_prec <= DBL_DIG+1 && g_debug_flag != DebugFlags::FORCE_ARBITRARY_PRECISION_MATH) || g_math_tol[1] >= 1.0)
        {
            bf_corners_to_float();
            g_bf_math = BFMathType::NONE;
        }
        else
        {
            init_bf_dec(got_prec);
        }
    }
    else if ((g_fractal_type == FractalType::MANDEL || g_fractal_type == FractalType::MANDEL_FP)
        && g_debug_flag == DebugFlags::FORCE_ARBITRARY_PRECISION_MATH)
    {
        g_fractal_type = FractalType::MANDEL_FP;
        g_cur_fractal_specific = &g_fractal_specific[+FractalType::MANDEL_FP];
        fractal_float_to_bf();
        g_user_float_flag = true;
    }
    else if ((g_fractal_type == FractalType::JULIA || g_fractal_type == FractalType::JULIA_FP)
        && g_debug_flag == DebugFlags::FORCE_ARBITRARY_PRECISION_MATH)
    {
        g_fractal_type = FractalType::JULIA_FP;
        g_cur_fractal_specific = &g_fractal_specific[+FractalType::JULIA_FP];
        fractal_float_to_bf();
        g_user_float_flag = true;
    }
    else if ((g_fractal_type == FractalType::MANDEL_Z_POWER_L || g_fractal_type == FractalType::MANDEL_Z_POWER_FP)
        && g_debug_flag == DebugFlags::FORCE_ARBITRARY_PRECISION_MATH)
    {
        g_fractal_type = FractalType::MANDEL_Z_POWER_FP;
        g_cur_fractal_specific = &g_fractal_specific[+FractalType::MANDEL_Z_POWER_FP];
        fractal_float_to_bf();
        g_user_float_flag = true;
    }
    else if ((g_fractal_type == FractalType::JULIA_Z_POWER_L || g_fractal_type == FractalType::JULIA_Z_POWER_FP)
        && g_debug_flag == DebugFlags::FORCE_ARBITRARY_PRECISION_MATH)
    {
        g_fractal_type = FractalType::JULIA_Z_POWER_FP;
        g_cur_fractal_specific = &g_fractal_specific[+FractalType::JULIA_Z_POWER_FP];
        fractal_float_to_bf();
        g_user_float_flag = true;
    }
    else if (g_fractal_type == FractalType::DIVIDE_BROT5 //
        && g_debug_flag == DebugFlags::FORCE_ARBITRARY_PRECISION_MATH)
    {
        g_cur_fractal_specific = &g_fractal_specific[+FractalType::DIVIDE_BROT5];
        fractal_float_to_bf();
        g_user_float_flag = true;
    }
    else
    {
        free_bf_vars();
    }
    if (g_bf_math != BFMathType::NONE)
    {
        g_float_flag = true;
    }
    else
    {
        g_float_flag = g_user_float_flag;
    }
    if (g_calc_status == CalcStatus::RESUMABLE)
    {
        // on resume, ensure floatflag correct
        g_float_flag = g_cur_fractal_specific->is_integer == 0;
    }
    // if floating pt only, set floatflag for TAB screen
    if (!g_cur_fractal_specific->is_integer && g_cur_fractal_specific->to_float == FractalType::NO_FRACTAL)
    {
        g_float_flag = true;
    }
    if (g_user_std_calc_mode == 's')
    {
        if (g_fractal_type == FractalType::MANDEL || g_fractal_type == FractalType::MANDEL_FP)
        {
            g_float_flag = true;
        }
        else
        {
            g_user_std_calc_mode = '1';
        }
    }

    // cppcheck-suppress variableScope
    int tries = 0;
init_restart:
#if defined(_WIN32)
    _ASSERTE(_CrtCheckMemory());
#endif

    /* the following variables may be forced to a different setting due to
       calc routine constraints;  usr_xxx is what the user last said is wanted,
       xxx is what we actually do in the current situation */
    g_std_calc_mode      = g_user_std_calc_mode;
    g_periodicity_check = g_user_periodicity_value;
    g_distance_estimator          = g_user_distance_estimator_value;
    g_biomorph         = g_user_biomorph_value;

    g_potential_flag = false;
    if (g_potential_params[0] != 0.0
        && g_colors >= 64
        && (g_cur_fractal_specific->calc_type == standard_fractal
            || g_cur_fractal_specific->calc_type == calc_mand
            || g_cur_fractal_specific->calc_type == calc_mand_fp))
    {
        g_potential_flag = true;
        g_user_distance_estimator_value = 0;
        g_distance_estimator = 0;    // can't do distest too
    }

    if (g_distance_estimator)
    {
        g_float_flag = true;  // force floating point for dist est
    }

    if (g_float_flag)
    {
        // ensure type matches floatflag
        if (g_cur_fractal_specific->is_integer != 0
            && g_cur_fractal_specific->to_float != FractalType::NO_FRACTAL)
        {
            g_fractal_type = g_cur_fractal_specific->to_float;
        }
    }
    else
    {
        if (g_cur_fractal_specific->is_integer == 0
            && g_cur_fractal_specific->to_float != FractalType::NO_FRACTAL)
        {
            g_fractal_type = g_cur_fractal_specific->to_float;
        }
    }
    // match Julibrot with integer mode of orbit
    if (g_fractal_type == FractalType::JULIBROT_FP && g_fractal_specific[+g_new_orbit_type].is_integer)
    {
        FractalType i = g_fractal_specific[+g_new_orbit_type].to_float;
        if (i != FractalType::NO_FRACTAL)
        {
            g_new_orbit_type = i;
        }
        else
        {
            g_fractal_type = FractalType::JULIBROT;
        }
    }
    else if (g_fractal_type == FractalType::JULIBROT && g_fractal_specific[+g_new_orbit_type].is_integer == 0)
    {
        FractalType i = g_fractal_specific[+g_new_orbit_type].to_float;
        if (i != FractalType::NO_FRACTAL)
        {
            g_new_orbit_type = i;
        }
        else
        {
            g_fractal_type = FractalType::JULIBROT_FP;
        }
    }

    g_cur_fractal_specific = &g_fractal_specific[+g_fractal_type];

    g_integer_fractal = g_cur_fractal_specific->is_integer;

    if (g_potential_flag && g_potential_params[2] != 0.0)
    {
        g_magnitude_limit = g_potential_params[2];
    }
    else if (g_bailout)     // user input bailout
    {
        g_magnitude_limit = g_bailout;
    }
    else if (g_biomorph != -1)     // biomorph benefits from larger bailout
    {
        g_magnitude_limit = 100;
    }
    else
    {
        g_magnitude_limit = g_cur_fractal_specific->orbit_bailout;
    }
    if (g_integer_fractal)   // the bailout limit mustn't be too high here
    {
        g_magnitude_limit = std::min(g_magnitude_limit, 127.0);
    }

    if (bit_set(g_cur_fractal_specific->flags, FractalFlags::NO_ROTATE))
    {
        // ensure min<max and unrotated rectangle
        if (g_x_min > g_x_max)
        {
            std::swap(g_x_min, g_x_max);
        }
        if (g_y_min > g_y_max)
        {
            std::swap(g_y_min, g_y_max);
        }
        g_x_3rd = g_x_min;
        g_y_3rd = g_y_min;
    }

    // set up bitshift for integer math
    g_bit_shift = FUDGE_FACTOR2; // by default, the smaller shift
    if (g_integer_fractal > 1)    // use specific override from table
    {
        g_bit_shift = g_integer_fractal;
    }
    if (g_integer_fractal == 0)
    {
        // float?
        FractalType i = g_cur_fractal_specific->to_float;
        if (i != FractalType::NO_FRACTAL) // -> int?
        {
            if (g_fractal_specific[+i].is_integer > 1)   // specific shift?
            {
                g_bit_shift = g_fractal_specific[+i].is_integer;
            }
        }
        else
        {
            g_bit_shift = 16;  // to allow larger corners
        }
    }
    // We want this code if we're using the assembler calcmand
    if (g_fractal_type == FractalType::MANDEL || g_fractal_type == FractalType::JULIA)
    {
        // adjust shift bits if..
        if (!g_potential_flag                                    // not using potential
            && (g_params[0] > -2.0 && g_params[0] < 2.0)  // parameters not too large
            && (g_params[1] > -2.0 && g_params[1] < 2.0)
            && (g_invert == 0)                        // and not inverting
            && g_biomorph == -1                     // and not biomorphing
            && g_magnitude_limit <= 4.0                         // and bailout not too high
            && (g_outside_color > REAL || g_outside_color < ATAN)   // and no funny outside stuff
            && g_debug_flag != DebugFlags::FORCE_SMALLER_BIT_SHIFT // and not debugging
            && g_close_proximity <= 2.0             // and g_close_proximity not too large
            && g_bailout_test == Bailout::MOD)    // and bailout test = mod
        {
            g_bit_shift = FUDGE_FACTOR;                     // use the larger bitshift
        }
    }

    g_fudge_factor = 1L << g_bit_shift;

    g_l_at_rad = g_fudge_factor/32768L;
    g_f_at_rad = 1.0/32768L;

    // now setup arrays of real coordinates corresponding to each pixel
    if (g_bf_math != BFMathType::NONE)
    {
        adjust_to_limits_bf(1.0); // make sure all corners in valid range
    }
    else
    {
        adjust_to_limits(1.0); // make sure all corners in valid range
        g_delta_x  = (LDouble)(g_x_max - g_x_3rd) / (LDouble)g_logical_screen_x_size_dots; // calculate stepsizes
        g_delta_y  = (LDouble)(g_y_max - g_y_3rd) / (LDouble)g_logical_screen_y_size_dots;
        g_delta_x2 = (LDouble)(g_x_3rd - g_x_min) / (LDouble)g_logical_screen_y_size_dots;
        g_delta_y2 = (LDouble)(g_y_3rd - g_y_min) / (LDouble)g_logical_screen_x_size_dots;
        fill_dx_array();
    }

    if (g_fractal_type != FractalType::CELLULAR && g_fractal_type != FractalType::ANT)  // fudgetolong fails w >10 digits in double
    {
        g_l_x_min  = fudge_to_long(g_x_min);
        g_l_x_max  = fudge_to_long(g_x_max);
        g_l_x_3rd  = fudge_to_long(g_x_3rd);
        g_l_y_min  = fudge_to_long(g_y_min);
        g_l_y_max  = fudge_to_long(g_y_max);
        g_l_y_3rd  = fudge_to_long(g_y_3rd);
        g_l_delta_x  = fudge_to_long((double)g_delta_x);
        g_l_delta_y  = fudge_to_long((double)g_delta_y);
        g_l_delta_x2 = fudge_to_long((double)g_delta_x2);
        g_l_delta_y2 = fudge_to_long((double)g_delta_y2);
    }

    // skip this if plasma to avoid 3d problems
    // skip if g_bf_math to avoid extraseg conflict with dx0 arrays
    // skip if ifs, ifs3d, or lsystem to avoid crash when mathtolerance
    // is set.  These types don't auto switch between float and integer math
    if (g_fractal_type != FractalType::PLASMA
        && g_bf_math == BFMathType::NONE
        && g_fractal_type != FractalType::IFS
        && g_fractal_type != FractalType::IFS_3D
        && g_fractal_type != FractalType::L_SYSTEM)
    {
        if (g_integer_fractal && (g_invert == 0) && g_use_grid)
        {
            if ((g_l_delta_x  == 0 && g_delta_x  != 0.0)
                || (g_l_delta_x2 == 0 && g_delta_x2 != 0.0)
                || (g_l_delta_y  == 0 && g_delta_y  != 0.0)
                || (g_l_delta_y2 == 0 && g_delta_y2 != 0.0))
            {
                goto expand_retry;
            }

            fill_lx_array();   // fill up the x,y grids
            // past max res?  check corners within 10% of expected
            if (ratio_bad((double)g_l_x0[g_logical_screen_x_dots-1]-g_l_x_min, (double)g_l_x_max-g_l_x_3rd)
                || ratio_bad((double)g_l_y0[g_logical_screen_y_dots-1]-g_l_y_max, (double)g_l_y_3rd-g_l_y_max)
                || ratio_bad((double)g_l_x1[(g_logical_screen_y_dots >> 1)-1], ((double)g_l_x_3rd-g_l_x_min)/2)
                || ratio_bad((double)g_l_y1[(g_logical_screen_x_dots >> 1)-1], ((double)g_l_y_min-g_l_y_3rd)/2))
            {
expand_retry:
                if (g_integer_fractal          // integer fractal type?
                    && g_cur_fractal_specific->to_float != FractalType::NO_FRACTAL)
                {
                    g_float_flag = true;       // switch to floating pt
                }
                else
                {
                    adjust_to_limits(2.0);   // double the size
                }
                if (g_calc_status == CalcStatus::RESUMABLE)         // due to restore of an old file?
                {
                    g_calc_status = CalcStatus::PARAMS_CHANGED;         //   whatever, it isn't resumable
                }
                goto init_restart;
            } // end if ratio bad

            // re-set corners to match reality
            g_l_x_max = g_l_x0[g_logical_screen_x_dots-1] + g_l_x1[g_logical_screen_y_dots-1];
            g_l_y_min = g_l_y0[g_logical_screen_y_dots-1] + g_l_y1[g_logical_screen_x_dots-1];
            g_l_x_3rd = g_l_x_min + g_l_x1[g_logical_screen_y_dots-1];
            g_l_y_3rd = g_l_y0[g_logical_screen_y_dots-1];
            g_x_min = fudge_to_double(g_l_x_min);
            g_x_max = fudge_to_double(g_l_x_max);
            g_x_3rd = fudge_to_double(g_l_x_3rd);
            g_y_min = fudge_to_double(g_l_y_min);
            g_y_max = fudge_to_double(g_l_y_max);
            g_y_3rd = fudge_to_double(g_l_y_3rd);
        } // end if (integerfractal && !invert && use_grid)
        else
        {
            // set up dx0 and dy0 analogs of lx0 and ly0
            // put fractal parameters in doubles
            double dx0 = g_x_min;                // fill up the x, y grids
            double dy0 = g_y_max;
            double dy1 = 0;
            double dx1 = 0;
            /* this way of defining the dx and dy arrays is not the most
               accurate, but it is kept because it is used to determine
               the limit of resolution */
            for (int i = 1; i < g_logical_screen_x_dots; i++)
            {
                dx0 = (double)(dx0 + (double)g_delta_x);
                dy1 = (double)(dy1 - (double)g_delta_y2);
            }
            for (int i = 1; i < g_logical_screen_y_dots; i++)
            {
                dy0 = (double)(dy0 - (double)g_delta_y);
                dx1 = (double)(dx1 + (double)g_delta_x2);
            }
            if (g_bf_math == BFMathType::NONE) // redundant test, leave for now
            {
                /* Following is the old logic for detecting failure of double
                   precision. It has two advantages: it is independent of the
                   representation of numbers, and it is sensitive to resolution
                   (allows deeper zooms at lower resolution. However it fails
                   for rotations of exactly 90 degrees, so we added a safety net
                   by using the magnification.  */
                if (++tries < 2) // for safety
                {
                    if (tries > 1)
                    {
                        stop_msg("precision-detection error");
                    }
                    /* Previously there were four tests of distortions in the
                       zoom box used to detect precision limitations. In some
                       cases of rotated/skewed zoom boxes, this causes the algorithm
                       to bail out to arbitrary precision too soon. The logic
                       now only tests the larger of the two deltas in an attempt
                       to repair this bug. This should never make the transition
                       to arbitrary precision sooner, but always later.*/
                    double test_x_try;
                    double test_x_exact;
                    if (std::abs(g_x_max-g_x_3rd) > std::abs(g_x_3rd-g_x_min))
                    {
                        test_x_exact  = g_x_max-g_x_3rd;
                        test_x_try    = dx0-g_x_min;
                    }
                    else
                    {
                        test_x_exact  = g_x_3rd-g_x_min;
                        test_x_try    = dx1;
                    }
                    double testy_try;
                    double testy_exact;
                    if (std::abs(g_y_3rd-g_y_max) > std::abs(g_y_min-g_y_3rd))
                    {
                        testy_exact = g_y_3rd-g_y_max;
                        testy_try   = dy0-g_y_max;
                    }
                    else
                    {
                        testy_exact = g_y_min-g_y_3rd;
                        testy_try   = dy1;
                    }
                    if (ratio_bad(test_x_try, test_x_exact)
                        || ratio_bad(testy_try, testy_exact))
                    {
                        if (bit_set(g_cur_fractal_specific->flags, FractalFlags::BF_MATH))
                        {
                            fractal_float_to_bf();
                            goto init_restart;
                        }
                        goto expand_retry;
                    } // end if ratio_bad etc.
                } // end if tries < 2
            } // end if bf_math == 0

            // if long double available, this is more accurate
            fill_dx_array();       // fill up the x, y grids

            // re-set corners to match reality
            g_x_max = (double)(g_x_min + (g_logical_screen_x_dots-1)*g_delta_x + (g_logical_screen_y_dots-1)*g_delta_x2);
            g_y_min = (double)(g_y_max - (g_logical_screen_y_dots-1)*g_delta_y - (g_logical_screen_x_dots-1)*g_delta_y2);
            g_x_3rd = (double)(g_x_min + (g_logical_screen_y_dots-1)*g_delta_x2);
            g_y_3rd = (double)(g_y_max - (g_logical_screen_y_dots-1)*g_delta_y);
        } // end else
    } // end if not plasma

    // for periodicity close-enough, and for unity:
    //     min(max(delx,delx2),max(dely,dely2))
    g_delta_min = std::abs((double)g_delta_x);
    g_delta_min = std::max(std::abs((double) g_delta_x2), g_delta_min);
    if (std::abs((double)g_delta_y) > std::abs((double)g_delta_y2))
    {
        g_delta_min = std::min(std::abs((double) g_delta_y), g_delta_min);
    }
    else if (std::abs((double)g_delta_y2) < g_delta_min)
    {
        g_delta_min = std::abs((double)g_delta_y2);
    }
    g_l_delta_min = fudge_to_long(g_delta_min);

    // calculate factors which plot real values to screen co-ords
    // calcfrac.c plot_orbit routines have comments about this
    double tmp = (double)((0.0-g_delta_y2) * g_delta_x2 * g_logical_screen_x_size_dots * g_logical_screen_y_size_dots
                     - (g_x_max-g_x_3rd) * (g_y_3rd-g_y_max));
    if (tmp != 0)
    {
        g_plot_mx1 = (double)(g_delta_x2 * g_logical_screen_x_size_dots * g_logical_screen_y_size_dots / tmp);
        g_plot_mx2 = (g_y_3rd-g_y_max) * g_logical_screen_x_size_dots / tmp;
        g_plot_my1 = (double)((0.0-g_delta_y2) * g_logical_screen_x_size_dots * g_logical_screen_y_size_dots / tmp);
        g_plot_my2 = (g_x_max-g_x_3rd) * g_logical_screen_y_size_dots / tmp;
    }
    if (g_bf_math == BFMathType::NONE)
    {
        free_bf_vars();
    }
}

static long fudge_to_long(double d)
{
    if ((d *= g_fudge_factor) > 0)
    {
        d += 0.5;
    }
    else
    {
        d -= 0.5;
    }
    return (long)d;
}

static double fudge_to_double(long l)
{
    char buf[30];
    double d;
    std::snprintf(buf, std::size(buf), "%.9g", (double)l / g_fudge_factor);
    std::sscanf(buf, "%lg", &d);
    return d;
}

void adjust_corner_bf()
{
    // make edges very near vert/horiz exact, to ditch rounding errs and
    // to avoid problems when delta per axis makes too large a ratio
    double x_mag_factor;
    double rotation;
    double skew;
    LDouble magnification;

    int saved = save_stack();
    bf_t bf_temp = alloc_stack(g_r_bf_length + 2);
    bf_t bf_temp2 = alloc_stack(g_r_bf_length + 2);
    bf_t b_tmp1 = alloc_stack(g_r_bf_length + 2);

    // While we're at it, let's adjust the x_mag_factor as well
    // use bf_temp, bftemp2 as bfXctr, bfYctr
    cvt_center_mag_bf(bf_temp, bf_temp2, magnification, x_mag_factor, rotation, skew);
    double f_temp = std::abs(x_mag_factor);
    if (f_temp != 1 && f_temp >= (1-g_aspect_drift) && f_temp <= (1+g_aspect_drift))
    {
        x_mag_factor = sign(x_mag_factor);
        cvt_corners_bf(bf_temp, bf_temp2, magnification, x_mag_factor, rotation, skew);
    }

    // ftemp=fabs(xx3rd-xxmin);
    abs_a_bf(sub_bf(bf_temp, g_bf_x_3rd, g_bf_x_min));

    // ftemp2=fabs(xxmax-xx3rd);
    abs_a_bf(sub_bf(bf_temp2, g_bf_x_max, g_bf_x_3rd));

    // if ( (ftemp=fabs(xx3rd-xxmin)) < (ftemp2=fabs(xxmax-xx3rd)) )
    if (cmp_bf(bf_temp, bf_temp2) < 0)
    {
        // if (ftemp*10000 < ftemp2 && yy3rd != yymax)
        if (cmp_bf(mult_bf_int(b_tmp1, bf_temp, 10000), bf_temp2) < 0
            && cmp_bf(g_bf_y_3rd, g_bf_y_max) != 0)
        {
            // xx3rd = xxmin;
            copy_bf(g_bf_x_3rd, g_bf_x_min);
        }
    }

    // else if (ftemp2*10000 < ftemp && yy3rd != yymin)
    if (cmp_bf(mult_bf_int(b_tmp1, bf_temp2, 10000), bf_temp) < 0
        && cmp_bf(g_bf_y_3rd, g_bf_y_min) != 0)
    {
        // xx3rd = xxmax;
        copy_bf(g_bf_x_3rd, g_bf_x_max);
    }

    // ftemp=fabs(yy3rd-yymin);
    abs_a_bf(sub_bf(bf_temp, g_bf_y_3rd, g_bf_y_min));

    // ftemp2=fabs(yymax-yy3rd);
    abs_a_bf(sub_bf(bf_temp2, g_bf_y_max, g_bf_y_3rd));

    // if ( (ftemp=fabs(yy3rd-yymin)) < (ftemp2=fabs(yymax-yy3rd)) )
    if (cmp_bf(bf_temp, bf_temp2) < 0)
    {
        // if (ftemp*10000 < ftemp2 && xx3rd != xxmax)
        if (cmp_bf(mult_bf_int(b_tmp1, bf_temp, 10000), bf_temp2) < 0
            && cmp_bf(g_bf_x_3rd, g_bf_x_max) != 0)
        {
            // yy3rd = yymin;
            copy_bf(g_bf_y_3rd, g_bf_y_min);
        }
    }

    // else if (ftemp2*10000 < ftemp && xx3rd != xxmin)
    if (cmp_bf(mult_bf_int(b_tmp1, bf_temp2, 10000), bf_temp) < 0
        && cmp_bf(g_bf_x_3rd, g_bf_x_min) != 0)
    {
        // yy3rd = yymax;
        copy_bf(g_bf_y_3rd, g_bf_y_max);
    }

    restore_stack(saved);
}

void adjust_corner()
{
    // make edges very near vert/horiz exact, to ditch rounding errs and
    // to avoid problems when delta per axis makes too large a ratio
    double f_temp;

    if (!g_integer_fractal)
    {
        double x_ctr;
        double y_ctr;
        double x_mag_factor;
        double rotation;
        double skew;
        LDouble magnification;
        // While we're at it, let's adjust the x_mag_factor as well
        cvt_center_mag(x_ctr, y_ctr, magnification, x_mag_factor, rotation, skew);
        f_temp = std::abs(x_mag_factor);
        if (f_temp != 1 && f_temp >= (1-g_aspect_drift) && f_temp <= (1+g_aspect_drift))
        {
            x_mag_factor = sign(x_mag_factor);
            cvt_corners(x_ctr, y_ctr, magnification, x_mag_factor, rotation, skew);
        }
    }

    f_temp = std::abs(g_x_3rd-g_x_min);
    double f_temp2 = std::abs(g_x_max - g_x_3rd);
    if (f_temp < f_temp2)
    {
        if (f_temp*10000 < f_temp2 && g_y_3rd != g_y_max)
        {
            g_x_3rd = g_x_min;
        }
    }

    if (f_temp2*10000 < f_temp && g_y_3rd != g_y_min)
    {
        g_x_3rd = g_x_max;
    }

    f_temp = std::abs(g_y_3rd-g_y_min);
    f_temp2 = std::abs(g_y_max-g_y_3rd);
    if (f_temp < f_temp2)
    {
        if (f_temp*10000 < f_temp2 && g_x_3rd != g_x_max)
        {
            g_y_3rd = g_y_min;
        }
    }

    if (f_temp2*10000 < f_temp && g_x_3rd != g_x_min)
    {
        g_y_3rd = g_y_max;
    }
}

static void adjust_to_limits_bf(double expand)
{
    bf_t b_corner_x[4];
    bf_t b_corner_y[4];
    int saved = save_stack();
    b_corner_x[0] = alloc_stack(g_r_bf_length+2);
    b_corner_x[1] = alloc_stack(g_r_bf_length+2);
    b_corner_x[2] = alloc_stack(g_r_bf_length+2);
    b_corner_x[3] = alloc_stack(g_r_bf_length+2);
    b_corner_y[0] = alloc_stack(g_r_bf_length+2);
    b_corner_y[1] = alloc_stack(g_r_bf_length+2);
    b_corner_y[2] = alloc_stack(g_r_bf_length+2);
    b_corner_y[3] = alloc_stack(g_r_bf_length+2);
    bf_t b_low_x = alloc_stack(g_r_bf_length + 2);
    bf_t b_high_x = alloc_stack(g_r_bf_length + 2);
    bf_t b_low_y = alloc_stack(g_r_bf_length + 2);
    bf_t b_high_y = alloc_stack(g_r_bf_length + 2);
    bf_t b_limit = alloc_stack(g_r_bf_length + 2);
    bf_t bf_temp = alloc_stack(g_r_bf_length + 2);
    bf_t b_center_x = alloc_stack(g_r_bf_length + 2);
    bf_t b_center_y = alloc_stack(g_r_bf_length + 2);
    bf_t b_adj_x = alloc_stack(g_r_bf_length + 2);
    bf_t b_adj_y = alloc_stack(g_r_bf_length + 2);
    bf_t b_tmp1 = alloc_stack(g_r_bf_length + 2);
    bf_t b_tmp2 = alloc_stack(g_r_bf_length + 2);
    bf_t b_expand = alloc_stack(g_r_bf_length + 2);

    LDouble limit = 32767.99;

    /*   if (bitshift >= 24) limit = 31.99;
       if (bitshift >= 29) limit = 3.99; */
    float_to_bf(b_limit, limit);
    float_to_bf(b_expand, expand);

    add_bf(b_center_x, g_bf_x_min, g_bf_x_max);
    half_a_bf(b_center_x);

    // center_y = (yymin+yymax)/2;
    add_bf(b_center_y, g_bf_y_min, g_bf_y_max);
    half_a_bf(b_center_y);

    // if (xxmin == center_x) {
    if (cmp_bf(g_bf_x_min, b_center_x) == 0)
    {
        // ohoh, infinitely thin, fix it
        smallest_add_bf(g_bf_x_max);
        // bfxmin -= bfxmax-center_x;
        sub_a_bf(g_bf_x_min, sub_bf(b_tmp1, g_bf_x_max, b_center_x));
    }

    // if (bfymin == center_y)
    if (cmp_bf(g_bf_y_min, b_center_y) == 0)
    {
        smallest_add_bf(g_bf_y_max);
        // bfymin -= bfymax-center_y;
        sub_a_bf(g_bf_y_min, sub_bf(b_tmp1, g_bf_y_max, b_center_y));
    }

    // if (bfx3rd == center_x)
    if (cmp_bf(g_bf_x_3rd, b_center_x) == 0)
    {
        smallest_add_bf(g_bf_x_3rd);
    }

    // if (bfy3rd == center_y)
    if (cmp_bf(g_bf_y_3rd, b_center_y) == 0)
    {
        smallest_add_bf(g_bf_y_3rd);
    }

    // setup array for easier manipulation
    // corner_x[0] = xxmin;
    copy_bf(b_corner_x[0], g_bf_x_min);

    // corner_x[1] = xxmax;
    copy_bf(b_corner_x[1], g_bf_x_max);

    // corner_x[2] = xx3rd;
    copy_bf(b_corner_x[2], g_bf_x_3rd);

    // corner_x[3] = xxmin+(xxmax-xx3rd);
    sub_bf(b_corner_x[3], g_bf_x_max, g_bf_x_3rd);
    add_a_bf(b_corner_x[3], g_bf_x_min);

    // corner_y[0] = yymax;
    copy_bf(b_corner_y[0], g_bf_y_max);

    // corner_y[1] = yymin;
    copy_bf(b_corner_y[1], g_bf_y_min);

    // corner_y[2] = yy3rd;
    copy_bf(b_corner_y[2], g_bf_y_3rd);

    // corner_y[3] = yymin+(yymax-yy3rd);
    sub_bf(b_corner_y[3], g_bf_y_max, g_bf_y_3rd);
    add_a_bf(b_corner_y[3], g_bf_y_min);

    // if caller wants image size adjusted, do that first
    if (expand != 1.0)
    {
        for (int i = 0; i < 4; ++i)
        {
            // corner_x[i] = center_x + (corner_x[i]-center_x)*expand;
            sub_bf(b_tmp1, b_corner_x[i], b_center_x);
            mult_bf(b_corner_x[i], b_tmp1, b_expand);
            add_a_bf(b_corner_x[i], b_center_x);

            // corner_y[i] = center_y + (corner_y[i]-center_y)*expand;
            sub_bf(b_tmp1, b_corner_y[i], b_center_y);
            mult_bf(b_corner_y[i], b_tmp1, b_expand);
            add_a_bf(b_corner_y[i], b_center_y);
        }
    }

    // get min/max x/y values
    // low_x = high_x = corner_x[0];
    copy_bf(b_low_x, b_corner_x[0]);
    copy_bf(b_high_x, b_corner_x[0]);

    // low_y = high_y = corner_y[0];
    copy_bf(b_low_y, b_corner_y[0]);
    copy_bf(b_high_y, b_corner_y[0]);

    for (int i = 1; i < 4; ++i)
    {
        // if (corner_x[i] < low_x)               low_x  = corner_x[i];
        if (cmp_bf(b_corner_x[i], b_low_x) < 0)
        {
            copy_bf(b_low_x, b_corner_x[i]);
        }

        // if (corner_x[i] > high_x)              high_x = corner_x[i];
        if (cmp_bf(b_corner_x[i], b_high_x) > 0)
        {
            copy_bf(b_high_x, b_corner_x[i]);
        }

        // if (corner_y[i] < low_y)               low_y  = corner_y[i];
        if (cmp_bf(b_corner_y[i], b_low_y) < 0)
        {
            copy_bf(b_low_y, b_corner_y[i]);
        }

        // if (corner_y[i] > high_y)              high_y = corner_y[i];
        if (cmp_bf(b_corner_y[i], b_high_y) > 0)
        {
            copy_bf(b_high_y, b_corner_y[i]);
        }
    }

    // if image is too large, downsize it maintaining center
    // ftemp = high_x-low_x;
    sub_bf(bf_temp, b_high_x, b_low_x);

    // if (high_y-low_y > ftemp) ftemp = high_y-low_y;
    if (cmp_bf(sub_bf(b_tmp1, b_high_y, b_low_y), bf_temp) > 0)
    {
        copy_bf(bf_temp, b_tmp1);
    }

    // if image is too large, downsize it maintaining center

    float_to_bf(b_tmp1, limit*2.0);
    copy_bf(b_tmp2, bf_temp);
    div_bf(bf_temp, b_tmp1, b_tmp2);
    float_to_bf(b_tmp1, 1.0);
    if (cmp_bf(bf_temp, b_tmp1) < 0)
    {
        for (int i = 0; i < 4; ++i)
        {
            // corner_x[i] = center_x + (corner_x[i]-center_x)*ftemp;
            sub_bf(b_tmp1, b_corner_x[i], b_center_x);
            mult_bf(b_corner_x[i], b_tmp1, bf_temp);
            add_a_bf(b_corner_x[i], b_center_x);

            // corner_y[i] = center_y + (corner_y[i]-center_y)*ftemp;
            sub_bf(b_tmp1, b_corner_y[i], b_center_y);
            mult_bf(b_corner_y[i], b_tmp1, bf_temp);
            add_a_bf(b_corner_y[i], b_center_y);
        }
    }

    // if any corner has x or y past limit, move the image
    // adj_x = adj_y = 0;
    clear_bf(b_adj_x);
    clear_bf(b_adj_y);

    for (int i = 0; i < 4; ++i)
    {
        /* if (corner_x[i] > limit && (ftemp = corner_x[i] - limit) > adj_x)
           adj_x = ftemp; */
        if (cmp_bf(b_corner_x[i], b_limit) > 0
            && cmp_bf(sub_bf(bf_temp, b_corner_x[i], b_limit), b_adj_x) > 0)
        {
            copy_bf(b_adj_x, bf_temp);
        }

        /* if (corner_x[i] < 0.0-limit && (ftemp = corner_x[i] + limit) < adj_x)
           adj_x = ftemp; */
        if (cmp_bf(b_corner_x[i], neg_bf(b_tmp1, b_limit)) < 0
            && cmp_bf(add_bf(bf_temp, b_corner_x[i], b_limit), b_adj_x) < 0)
        {
            copy_bf(b_adj_x, bf_temp);
        }

        /* if (corner_y[i] > limit  && (ftemp = corner_y[i] - limit) > adj_y)
           adj_y = ftemp; */
        if (cmp_bf(b_corner_y[i], b_limit) > 0
            && cmp_bf(sub_bf(bf_temp, b_corner_y[i], b_limit), b_adj_y) > 0)
        {
            copy_bf(b_adj_y, bf_temp);
        }

        /* if (corner_y[i] < 0.0-limit && (ftemp = corner_y[i] + limit) < adj_y)
           adj_y = ftemp; */
        if (cmp_bf(b_corner_y[i], neg_bf(b_tmp1, b_limit)) < 0
            && cmp_bf(add_bf(bf_temp, b_corner_y[i], b_limit), b_adj_y) < 0)
        {
            copy_bf(b_adj_y, bf_temp);
        }
    }

    /* if (g_calc_status == calc_status_value::RESUMABLE && (adj_x != 0 || adj_y != 0) && (zoom_box_width == 1.0))
       g_calc_status = calc_status_value::PARAMS_CHANGED; */
    if (g_calc_status == CalcStatus::RESUMABLE
        && (is_bf_not_zero(b_adj_x)|| is_bf_not_zero(b_adj_y))
        && (g_zoom_box_width == 1.0))
    {
        g_calc_status = CalcStatus::PARAMS_CHANGED;
    }

    // xxmin = corner_x[0] - adj_x;
    sub_bf(g_bf_x_min, b_corner_x[0], b_adj_x);
    // xxmax = corner_x[1] - adj_x;
    sub_bf(g_bf_x_max, b_corner_x[1], b_adj_x);
    // xx3rd = corner_x[2] - adj_x;
    sub_bf(g_bf_x_3rd, b_corner_x[2], b_adj_x);
    // yymax = corner_y[0] - adj_y;
    sub_bf(g_bf_y_max, b_corner_y[0], b_adj_y);
    // yymin = corner_y[1] - adj_y;
    sub_bf(g_bf_y_min, b_corner_y[1], b_adj_y);
    // yy3rd = corner_y[2] - adj_y;
    sub_bf(g_bf_y_3rd, b_corner_y[2], b_adj_y);

    adjust_corner_bf(); // make 3rd corner exact if very near other co-ords
    restore_stack(saved);
}

static void adjust_to_limits(double expand)
{
    double corner_x[4];
    double corner_y[4];

    double limit = 32767.99;

    if (g_integer_fractal)
    {
        // let user reproduce old GIF's and PAR's
        limit = 1023.99;
        if (g_bit_shift >= 24)
        {
            limit = 31.99;
        }
        if (g_bit_shift >= 29)
        {
            limit = 3.99;
        }
    }

    double center_x = (g_x_min + g_x_max) / 2;
    double center_y = (g_y_min + g_y_max) / 2;

    if (g_x_min == center_x)
    {
        // ohoh, infinitely thin, fix it
        smallest_add(&g_x_max);
        g_x_min -= g_x_max-center_x;
    }

    if (g_y_min == center_y)
    {
        smallest_add(&g_y_max);
        g_y_min -= g_y_max-center_y;
    }

    if (g_x_3rd == center_x)
    {
        smallest_add(&g_x_3rd);
    }

    if (g_y_3rd == center_y)
    {
        smallest_add(&g_y_3rd);
    }

    // setup array for easier manipulation
    corner_x[0] = g_x_min;
    corner_x[1] = g_x_max;
    corner_x[2] = g_x_3rd;
    corner_x[3] = g_x_min+(g_x_max-g_x_3rd);

    corner_y[0] = g_y_max;
    corner_y[1] = g_y_min;
    corner_y[2] = g_y_3rd;
    corner_y[3] = g_y_min+(g_y_max-g_y_3rd);

    // if caller wants image size adjusted, do that first
    if (expand != 1.0)
    {
        for (int i = 0; i < 4; ++i)
        {
            corner_x[i] = center_x + (corner_x[i]-center_x)*expand;
            corner_y[i] = center_y + (corner_y[i]-center_y)*expand;
        }
    }
    // get min/max x/y values
    double high_x = corner_x[0];
    double low_x = corner_x[0];
    double high_y = corner_y[0];
    double low_y = corner_y[0];

    for (int i = 1; i < 4; ++i)
    {
        low_x = std::min(corner_x[i], low_x);
        high_x = std::max(corner_x[i], high_x);
        low_y = std::min(corner_y[i], low_y);
        high_y = std::max(corner_y[i], high_y);
    }

    // if image is too large, downsize it maintaining center
    double f_temp = high_x - low_x;
    f_temp = std::max(high_y - low_y, f_temp);

    // if image is too large, downsize it maintaining center
    f_temp = limit*2/f_temp;
    if (f_temp < 1.0)
    {
        for (int i = 0; i < 4; ++i)
        {
            corner_x[i] = center_x + (corner_x[i]-center_x)*f_temp;
            corner_y[i] = center_y + (corner_y[i]-center_y)*f_temp;
        }
    }

    // if any corner has x or y past limit, move the image
    double adj_y = 0;
    double adj_x = 0;

    for (int i = 0; i < 4; ++i)
    {
        if (corner_x[i] > limit && (f_temp = corner_x[i] - limit) > adj_x)
        {
            adj_x = f_temp;
        }
        if (corner_x[i] < 0.0-limit && (f_temp = corner_x[i] + limit) < adj_x)
        {
            adj_x = f_temp;
        }
        if (corner_y[i] > limit     && (f_temp = corner_y[i] - limit) > adj_y)
        {
            adj_y = f_temp;
        }
        if (corner_y[i] < 0.0-limit && (f_temp = corner_y[i] + limit) < adj_y)
        {
            adj_y = f_temp;
        }
    }
    if (g_calc_status == CalcStatus::RESUMABLE && (adj_x != 0 || adj_y != 0) && (g_zoom_box_width == 1.0))
    {
        g_calc_status = CalcStatus::PARAMS_CHANGED;
    }
    g_x_min = corner_x[0] - adj_x;
    g_x_max = corner_x[1] - adj_x;
    g_x_3rd = corner_x[2] - adj_x;
    g_y_max = corner_y[0] - adj_y;
    g_y_min = corner_y[1] - adj_y;
    g_y_3rd = corner_y[2] - adj_y;

    adjust_corner(); // make 3rd corner exact if very near other co-ords
}

static void smallest_add(double *num)
{
    *num += *num * 5.0e-16;
}

static void smallest_add_bf(bf_t num)
{
    int saved = save_stack();
    bf_t b_tmp1 = alloc_stack(g_bf_length + 2);
    mult_bf(b_tmp1, float_to_bf(b_tmp1, 5.0e-16), num);
    add_a_bf(num, b_tmp1);
    restore_stack(saved);
}

static int ratio_bad(double actual, double desired)
{
    double tol;
    if (g_integer_fractal)
    {
        tol = g_math_tol[0];
    }
    else
    {
        tol = g_math_tol[1];
    }
    if (tol <= 0.0)
    {
        return 1;
    }
    else if (tol >= 1.0)
    {
        return 0;
    }
    if (desired != 0 && g_debug_flag != DebugFlags::PREVENT_ARBITRARY_PRECISION_MATH)
    {
        double f_temp = actual / desired;
        if (f_temp < (1.0-tol) || f_temp > (1.0+tol))
        {
            return 1;
        }
    }
    return 0;
}