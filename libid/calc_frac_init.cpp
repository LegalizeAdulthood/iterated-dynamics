#include "calc_frac_init.h"

#include "port.h"
#include "prototyp.h"

#include "bailout_formula.h"
#include "biginit.h"
#include "calcfrac.h"
#include "cmdfiles.h"
#include "convert_center_mag.h"
#include "convert_corners.h"
#include "debug_flags.h"
#include "fractalb.h"
#include "fractalp.h"
#include "get_prec_big_float.h"
#include "id_data.h"
#include "jb.h"
#include "pixel_grid.h"
#include "soi.h"
#include "stop_msg.h"
#include "type_has_param.h"

#include <array>
#include <cfloat>
#include <cmath>
#include <cstdio>

enum
{
    FUDGEFACTOR = 29, // fudge all values up by 2**this
    FUDGEFACTOR2 = 24 // (or maybe this)
};

static long   fudgetolong(double d);
static double fudgetodouble(long l);
static void   adjust_to_limits(double expand);
static void   smallest_add(double *num);
static int    ratio_bad(double actual, double desired);
static void   adjust_to_limitsbf(double expand);
static void   smallest_add_bf(bf_t num);

/* This function calculates the precision needed to distinguish adjacent
   pixels at Fractint's maximum resolution of MAX_PIXELS by MAX_PIXELS
   (if rez==MAXREZ) or at current resolution (if rez==CURRENTREZ)    */
static int getprecdbl(int rezflag)
{
    LDBL del1, del2, xdel, xdel2, ydel, ydel2;
    int digits;
    LDBL rez;
    if (rezflag == MAXREZ)
    {
        rez = OLD_MAX_PIXELS -1;
    }
    else
    {
        rez = g_logical_screen_x_dots-1;
    }

    xdel = ((LDBL)g_x_max - (LDBL)g_x_3rd)/rez;
    ydel2 = ((LDBL)g_y_3rd - (LDBL)g_y_min)/rez;

    if (rezflag == CURRENTREZ)
    {
        rez = g_logical_screen_y_dots-1;
    }

    ydel = ((LDBL)g_y_max - (LDBL)g_y_3rd)/rez;
    xdel2 = ((LDBL)g_x_3rd - (LDBL)g_x_min)/rez;

    del1 = std::fabs(xdel) + std::fabs(xdel2);
    del2 = std::fabs(ydel) + std::fabs(ydel2);
    if (del2 < del1)
    {
        del1 = del2;
    }
    if (del1 == 0)
    {
#ifdef DEBUG
        showcornersdbl("getprecdbl");
#endif
        return -1;
    }
    digits = 1;
    while (del1 < 1.0)
    {
        digits++;
        del1 *= 10;
    }
    digits = std::max(digits, 3);
    return digits;
}

void fractal_floattobf()
{
    init_bf_dec(getprecdbl(CURRENTREZ));
    floattobf(g_bf_x_min, g_x_min);
    floattobf(g_bf_x_max, g_x_max);
    floattobf(g_bf_y_min, g_y_min);
    floattobf(g_bf_y_max, g_y_max);
    floattobf(g_bf_x_3rd, g_x_3rd);
    floattobf(g_bf_y_3rd, g_y_3rd);

    for (int i = 0; i < MAX_PARAMS; i++)
    {
        if (typehasparm(g_fractal_type, i, nullptr))
        {
            floattobf(bfparms[i], g_params[i]);
        }
    }
    g_calc_status = calc_status_value::PARAMS_CHANGED;
}

void calcfracinit() // initialize a *pile* of stuff for fractal calculation
{
    g_old_color_iter = 0L;
    g_color_iter = g_old_color_iter;
    for (int i = 0; i < 10; i++)
    {
        g_rhombus_stack[i] = 0;
    }

    // set up grid array compactly leaving space at end
    // space req for grid is 2(xdots+ydots)*sizeof(long or double)
    // space available in extraseg is 65536 Bytes
    long xytemp = g_logical_screen_x_dots + g_logical_screen_y_dots;
    if ((!g_user_float_flag && (xytemp*sizeof(long) > 32768))
        || (g_user_float_flag && (xytemp*sizeof(double) > 32768))
        || g_debug_flag == debug_flags::prevent_coordinate_grid)
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

    if (!(g_cur_fractal_specific->flags & BF_MATH))
    {
        fractal_type tofloat = g_cur_fractal_specific->tofloat;
        if (tofloat == fractal_type::NOFRACTAL)
        {
            bf_math = bf_math_type::NONE;
        }
        else if (!(g_fractal_specific[static_cast<int>(tofloat)].flags & BF_MATH))
        {
            bf_math = bf_math_type::NONE;
        }
        else if (bf_math != bf_math_type::NONE)
        {
            g_cur_fractal_specific = &g_fractal_specific[static_cast<int>(tofloat)];
            g_fractal_type = tofloat;
        }
    }

    // switch back to double when zooming out if using arbitrary precision
    if (bf_math != bf_math_type::NONE)
    {
        int gotprec = getprecbf(CURRENTREZ);
        if ((gotprec <= DBL_DIG+1 && g_debug_flag != debug_flags::force_arbitrary_precision_math) || g_math_tol[1] >= 1.0)
        {
            bfcornerstofloat();
            bf_math = bf_math_type::NONE;
        }
        else
        {
            init_bf_dec(gotprec);
        }
    }
    else if ((g_fractal_type == fractal_type::MANDEL || g_fractal_type == fractal_type::MANDELFP)
        && g_debug_flag == debug_flags::force_arbitrary_precision_math)
    {
        g_fractal_type = fractal_type::MANDELFP;
        g_cur_fractal_specific = &g_fractal_specific[static_cast<int>(fractal_type::MANDELFP)];
        fractal_floattobf();
        g_user_float_flag = true;
    }
    else if ((g_fractal_type == fractal_type::JULIA || g_fractal_type == fractal_type::JULIAFP)
        && g_debug_flag == debug_flags::force_arbitrary_precision_math)
    {
        g_fractal_type = fractal_type::JULIAFP;
        g_cur_fractal_specific = &g_fractal_specific[static_cast<int>(fractal_type::JULIAFP)];
        fractal_floattobf();
        g_user_float_flag = true;
    }
    else if ((g_fractal_type == fractal_type::LMANDELZPOWER || g_fractal_type == fractal_type::FPMANDELZPOWER)
        && g_debug_flag == debug_flags::force_arbitrary_precision_math)
    {
        g_fractal_type = fractal_type::FPMANDELZPOWER;
        g_cur_fractal_specific = &g_fractal_specific[static_cast<int>(fractal_type::FPMANDELZPOWER)];
        fractal_floattobf();
        g_user_float_flag = true;
    }
    else if ((g_fractal_type == fractal_type::LJULIAZPOWER || g_fractal_type == fractal_type::FPJULIAZPOWER)
        && g_debug_flag == debug_flags::force_arbitrary_precision_math)
    {
        g_fractal_type = fractal_type::FPJULIAZPOWER;
        g_cur_fractal_specific = &g_fractal_specific[static_cast<int>(fractal_type::FPJULIAZPOWER)];
        fractal_floattobf();
        g_user_float_flag = true;
    }
    else
    {
        free_bf_vars();
    }
    if (bf_math != bf_math_type::NONE)
    {
        g_float_flag = true;
    }
    else
    {
        g_float_flag = g_user_float_flag;
    }
    if (g_calc_status == calc_status_value::RESUMABLE)
    {
        // on resume, ensure floatflag correct
        g_float_flag = g_cur_fractal_specific->isinteger == 0;
    }
    // if floating pt only, set floatflag for TAB screen
    if (!g_cur_fractal_specific->isinteger && g_cur_fractal_specific->tofloat == fractal_type::NOFRACTAL)
    {
        g_float_flag = true;
    }
    if (g_user_std_calc_mode == 's')
    {
        if (g_fractal_type == fractal_type::MANDEL || g_fractal_type == fractal_type::MANDELFP)
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
        && (g_cur_fractal_specific->calctype == standard_fractal
            || g_cur_fractal_specific->calctype == calcmand
            || g_cur_fractal_specific->calctype == calcmandfp))
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
        if (g_cur_fractal_specific->isinteger != 0
            && g_cur_fractal_specific->tofloat != fractal_type::NOFRACTAL)
        {
            g_fractal_type = g_cur_fractal_specific->tofloat;
        }
    }
    else
    {
        if (g_cur_fractal_specific->isinteger == 0
            && g_cur_fractal_specific->tofloat != fractal_type::NOFRACTAL)
        {
            g_fractal_type = g_cur_fractal_specific->tofloat;
        }
    }
    // match Julibrot with integer mode of orbit
    if (g_fractal_type == fractal_type::JULIBROTFP && g_fractal_specific[static_cast<int>(g_new_orbit_type)].isinteger)
    {
        fractal_type i = g_fractal_specific[static_cast<int>(g_new_orbit_type)].tofloat;
        if (i != fractal_type::NOFRACTAL)
        {
            g_new_orbit_type = i;
        }
        else
        {
            g_fractal_type = fractal_type::JULIBROT;
        }
    }
    else if (g_fractal_type == fractal_type::JULIBROT && g_fractal_specific[static_cast<int>(g_new_orbit_type)].isinteger == 0)
    {
        fractal_type i = g_fractal_specific[static_cast<int>(g_new_orbit_type)].tofloat;
        if (i != fractal_type::NOFRACTAL)
        {
            g_new_orbit_type = i;
        }
        else
        {
            g_fractal_type = fractal_type::JULIBROTFP;
        }
    }

    g_cur_fractal_specific = &g_fractal_specific[static_cast<int>(g_fractal_type)];

    g_integer_fractal = g_cur_fractal_specific->isinteger;

    if (g_potential_flag && g_potential_params[2] != 0.0)
    {
        g_magnitude_limit = g_potential_params[2];
    }
    else if (g_bail_out)     // user input bailout
    {
        g_magnitude_limit = g_bail_out;
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
        if (g_magnitude_limit > 127.0)
        {
            g_magnitude_limit = 127.0;
        }
    }

    if ((g_cur_fractal_specific->flags&NOROTATE) != 0)
    {
        // ensure min<max and unrotated rectangle
        if (g_x_min > g_x_max)
        {
            double ftemp = g_x_max;
            g_x_max = g_x_min;
            g_x_min = ftemp;
        }
        if (g_y_min > g_y_max)
        {
            double ftemp = g_y_max;
            g_y_max = g_y_min;
            g_y_min = ftemp;
        }
        g_x_3rd = g_x_min;
        g_y_3rd = g_y_min;
    }

    // set up bitshift for integer math
    g_bit_shift = FUDGEFACTOR2; // by default, the smaller shift
    if (g_integer_fractal > 1)    // use specific override from table
    {
        g_bit_shift = g_integer_fractal;
    }
    if (g_integer_fractal == 0)
    {
        // float?
        fractal_type i = g_cur_fractal_specific->tofloat;
        if (i != fractal_type::NOFRACTAL) // -> int?
        {
            if (g_fractal_specific[static_cast<int>(i)].isinteger > 1)   // specific shift?
            {
                g_bit_shift = g_fractal_specific[static_cast<int>(i)].isinteger;
            }
        }
        else
        {
            g_bit_shift = 16;  // to allow larger corners
        }
    }
    // We want this code if we're using the assembler calcmand
    if (g_fractal_type == fractal_type::MANDEL || g_fractal_type == fractal_type::JULIA)
    {
        // adjust shift bits if..
        if (!g_potential_flag                                    // not using potential
            && (g_params[0] > -2.0 && g_params[0] < 2.0)  // parameters not too large
            && (g_params[1] > -2.0 && g_params[1] < 2.0)
            && (g_invert == 0)                        // and not inverting
            && g_biomorph == -1                     // and not biomorphing
            && g_magnitude_limit <= 4.0                         // and bailout not too high
            && (g_outside_color > REAL || g_outside_color < ATAN)   // and no funny outside stuff
            && g_debug_flag != debug_flags::force_smaller_bitshift // and not debugging
            && g_close_proximity <= 2.0             // and g_close_proximity not too large
            && g_bail_out_test == bailouts::Mod)    // and bailout test = mod
        {
            g_bit_shift = FUDGEFACTOR;                     // use the larger bitshift
        }
    }

    g_fudge_factor = 1L << g_bit_shift;

    g_l_at_rad = g_fudge_factor/32768L;
    g_f_at_rad = 1.0/32768L;

    // now setup arrays of real coordinates corresponding to each pixel
    if (bf_math != bf_math_type::NONE)
    {
        adjust_to_limitsbf(1.0); // make sure all corners in valid range
    }
    else
    {
        adjust_to_limits(1.0); // make sure all corners in valid range
        g_delta_x  = (LDBL)(g_x_max - g_x_3rd) / (LDBL)g_logical_screen_x_size_dots; // calculate stepsizes
        g_delta_y  = (LDBL)(g_y_max - g_y_3rd) / (LDBL)g_logical_screen_y_size_dots;
        g_delta_x2 = (LDBL)(g_x_3rd - g_x_min) / (LDBL)g_logical_screen_y_size_dots;
        g_delta_y2 = (LDBL)(g_y_3rd - g_y_min) / (LDBL)g_logical_screen_x_size_dots;
        fill_dx_array();
    }

    if (g_fractal_type != fractal_type::CELLULAR && g_fractal_type != fractal_type::ANT)  // fudgetolong fails w >10 digits in double
    {
        g_l_x_min  = fudgetolong(g_x_min);
        g_l_x_max  = fudgetolong(g_x_max);
        g_l_x_3rd  = fudgetolong(g_x_3rd);
        g_l_y_min  = fudgetolong(g_y_min);
        g_l_y_max  = fudgetolong(g_y_max);
        g_l_y_3rd  = fudgetolong(g_y_3rd);
        g_l_delta_x  = fudgetolong((double)g_delta_x);
        g_l_delta_y  = fudgetolong((double)g_delta_y);
        g_l_delta_x2 = fudgetolong((double)g_delta_x2);
        g_l_delta_y2 = fudgetolong((double)g_delta_y2);
    }

    // skip this if plasma to avoid 3d problems
    // skip if bf_math to avoid extraseg conflict with dx0 arrays
    // skip if ifs, ifs3d, or lsystem to avoid crash when mathtolerance
    // is set.  These types don't auto switch between float and integer math
    if (g_fractal_type != fractal_type::PLASMA
        && bf_math == bf_math_type::NONE
        && g_fractal_type != fractal_type::IFS
        && g_fractal_type != fractal_type::IFS3D
        && g_fractal_type != fractal_type::LSYSTEM)
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
                    && g_cur_fractal_specific->tofloat != fractal_type::NOFRACTAL)
                {
                    g_float_flag = true;       // switch to floating pt
                }
                else
                {
                    adjust_to_limits(2.0);   // double the size
                }
                if (g_calc_status == calc_status_value::RESUMABLE)         // due to restore of an old file?
                {
                    g_calc_status = calc_status_value::PARAMS_CHANGED;         //   whatever, it isn't resumable
                }
                goto init_restart;
            } // end if ratio bad

            // re-set corners to match reality
            g_l_x_max = g_l_x0[g_logical_screen_x_dots-1] + g_l_x1[g_logical_screen_y_dots-1];
            g_l_y_min = g_l_y0[g_logical_screen_y_dots-1] + g_l_y1[g_logical_screen_x_dots-1];
            g_l_x_3rd = g_l_x_min + g_l_x1[g_logical_screen_y_dots-1];
            g_l_y_3rd = g_l_y0[g_logical_screen_y_dots-1];
            g_x_min = fudgetodouble(g_l_x_min);
            g_x_max = fudgetodouble(g_l_x_max);
            g_x_3rd = fudgetodouble(g_l_x_3rd);
            g_y_min = fudgetodouble(g_l_y_min);
            g_y_max = fudgetodouble(g_l_y_max);
            g_y_3rd = fudgetodouble(g_l_y_3rd);
        } // end if (integerfractal && !invert && use_grid)
        else
        {
            double dx0, dy0, dx1, dy1;
            // set up dx0 and dy0 analogs of lx0 and ly0
            // put fractal parameters in doubles
            dx0 = g_x_min;                // fill up the x, y grids
            dy0 = g_y_max;
            dy1 = 0;
            dx1 = dy1;
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
            if (bf_math == bf_math_type::NONE) // redundant test, leave for now
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
                        stopmsg(STOPMSG_NONE, "precision-detection error");
                    }
                    /* Previously there were four tests of distortions in the
                       zoom box used to detect precision limitations. In some
                       cases of rotated/skewed zoom boxes, this causes the algorithm
                       to bail out to arbitrary precision too soon. The logic
                       now only tests the larger of the two deltas in an attempt
                       to repair this bug. This should never make the transition
                       to arbitrary precision sooner, but always later.*/
                    double testx_try;
                    double testx_exact;
                    if (std::fabs(g_x_max-g_x_3rd) > std::fabs(g_x_3rd-g_x_min))
                    {
                        testx_exact  = g_x_max-g_x_3rd;
                        testx_try    = dx0-g_x_min;
                    }
                    else
                    {
                        testx_exact  = g_x_3rd-g_x_min;
                        testx_try    = dx1;
                    }
                    double testy_try;
                    double testy_exact;
                    if (std::fabs(g_y_3rd-g_y_max) > std::fabs(g_y_min-g_y_3rd))
                    {
                        testy_exact = g_y_3rd-g_y_max;
                        testy_try   = dy0-g_y_max;
                    }
                    else
                    {
                        testy_exact = g_y_min-g_y_3rd;
                        testy_try   = dy1;
                    }
                    if (ratio_bad(testx_try, testx_exact)
                        || ratio_bad(testy_try, testy_exact))
                    {
                        if (g_cur_fractal_specific->flags & BF_MATH)
                        {
                            fractal_floattobf();
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
    g_delta_min = std::fabs((double)g_delta_x);
    if (std::fabs((double)g_delta_x2) > g_delta_min)
    {
        g_delta_min = std::fabs((double)g_delta_x2);
    }
    if (std::fabs((double)g_delta_y) > std::fabs((double)g_delta_y2))
    {
        if (std::fabs((double)g_delta_y) < g_delta_min)
        {
            g_delta_min = std::fabs((double)g_delta_y);
        }
    }
    else if (std::fabs((double)g_delta_y2) < g_delta_min)
    {
        g_delta_min = std::fabs((double)g_delta_y2);
    }
    g_l_delta_min = fudgetolong(g_delta_min);

    // calculate factors which plot real values to screen co-ords
    // calcfrac.c plot_orbit routines have comments about this
    double ftemp = (double)((0.0-g_delta_y2) * g_delta_x2 * g_logical_screen_x_size_dots * g_logical_screen_y_size_dots
                     - (g_x_max-g_x_3rd) * (g_y_3rd-g_y_max));
    if (ftemp != 0)
    {
        g_plot_mx1 = (double)(g_delta_x2 * g_logical_screen_x_size_dots * g_logical_screen_y_size_dots / ftemp);
        g_plot_mx2 = (g_y_3rd-g_y_max) * g_logical_screen_x_size_dots / ftemp;
        g_plot_my1 = (double)((0.0-g_delta_y2) * g_logical_screen_x_size_dots * g_logical_screen_y_size_dots / ftemp);
        g_plot_my2 = (g_x_max-g_x_3rd) * g_logical_screen_y_size_dots / ftemp;
    }
    if (bf_math == bf_math_type::NONE)
    {
        free_bf_vars();
    }
}

static long fudgetolong(double d)
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

static double fudgetodouble(long l)
{
    char buf[30];
    double d;
    std::snprintf(buf, std::size(buf), "%.9g", (double)l / g_fudge_factor);
    std::sscanf(buf, "%lg", &d);
    return d;
}

void adjust_cornerbf()
{
    // make edges very near vert/horiz exact, to ditch rounding errs and
    // to avoid problems when delta per axis makes too large a ratio
    double ftemp;
    double Xmagfactor, Rotation, Skew;
    LDBL Magnification;

    bf_t bftemp, bftemp2;
    bf_t btmp1;
    int saved;
    saved = save_stack();
    bftemp  = alloc_stack(rbflength+2);
    bftemp2 = alloc_stack(rbflength+2);
    btmp1  =  alloc_stack(rbflength+2);

    // While we're at it, let's adjust the Xmagfactor as well
    // use bftemp, bftemp2 as bfXctr, bfYctr
    cvtcentermagbf(bftemp, bftemp2, &Magnification, &Xmagfactor, &Rotation, &Skew);
    ftemp = std::fabs(Xmagfactor);
    if (ftemp != 1 && ftemp >= (1-g_aspect_drift) && ftemp <= (1+g_aspect_drift))
    {
        Xmagfactor = sign(Xmagfactor);
        cvtcornersbf(bftemp, bftemp2, Magnification, Xmagfactor, Rotation, Skew);
    }

    // ftemp=fabs(xx3rd-xxmin);
    abs_a_bf(sub_bf(bftemp, g_bf_x_3rd, g_bf_x_min));

    // ftemp2=fabs(xxmax-xx3rd);
    abs_a_bf(sub_bf(bftemp2, g_bf_x_max, g_bf_x_3rd));

    // if ( (ftemp=fabs(xx3rd-xxmin)) < (ftemp2=fabs(xxmax-xx3rd)) )
    if (cmp_bf(bftemp, bftemp2) < 0)
    {
        // if (ftemp*10000 < ftemp2 && yy3rd != yymax)
        if (cmp_bf(mult_bf_int(btmp1, bftemp, 10000), bftemp2) < 0
            && cmp_bf(g_bf_y_3rd, g_bf_y_max) != 0)
        {
            // xx3rd = xxmin;
            copy_bf(g_bf_x_3rd, g_bf_x_min);
        }
    }

    // else if (ftemp2*10000 < ftemp && yy3rd != yymin)
    if (cmp_bf(mult_bf_int(btmp1, bftemp2, 10000), bftemp) < 0
        && cmp_bf(g_bf_y_3rd, g_bf_y_min) != 0)
    {
        // xx3rd = xxmax;
        copy_bf(g_bf_x_3rd, g_bf_x_max);
    }

    // ftemp=fabs(yy3rd-yymin);
    abs_a_bf(sub_bf(bftemp, g_bf_y_3rd, g_bf_y_min));

    // ftemp2=fabs(yymax-yy3rd);
    abs_a_bf(sub_bf(bftemp2, g_bf_y_max, g_bf_y_3rd));

    // if ( (ftemp=fabs(yy3rd-yymin)) < (ftemp2=fabs(yymax-yy3rd)) )
    if (cmp_bf(bftemp, bftemp2) < 0)
    {
        // if (ftemp*10000 < ftemp2 && xx3rd != xxmax)
        if (cmp_bf(mult_bf_int(btmp1, bftemp, 10000), bftemp2) < 0
            && cmp_bf(g_bf_x_3rd, g_bf_x_max) != 0)
        {
            // yy3rd = yymin;
            copy_bf(g_bf_y_3rd, g_bf_y_min);
        }
    }

    // else if (ftemp2*10000 < ftemp && xx3rd != xxmin)
    if (cmp_bf(mult_bf_int(btmp1, bftemp2, 10000), bftemp) < 0
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
    double ftemp, ftemp2;
    double Xctr, Yctr, Xmagfactor, Rotation, Skew;
    LDBL Magnification;

    if (!g_integer_fractal)
    {
        // While we're at it, let's adjust the Xmagfactor as well
        cvtcentermag(&Xctr, &Yctr, &Magnification, &Xmagfactor, &Rotation, &Skew);
        ftemp = std::fabs(Xmagfactor);
        if (ftemp != 1 && ftemp >= (1-g_aspect_drift) && ftemp <= (1+g_aspect_drift))
        {
            Xmagfactor = sign(Xmagfactor);
            cvtcorners(Xctr, Yctr, Magnification, Xmagfactor, Rotation, Skew);
        }
    }

    ftemp = std::fabs(g_x_3rd-g_x_min);
    ftemp2 = std::fabs(g_x_max-g_x_3rd);
    if (ftemp < ftemp2)
    {
        if (ftemp*10000 < ftemp2 && g_y_3rd != g_y_max)
        {
            g_x_3rd = g_x_min;
        }
    }

    if (ftemp2*10000 < ftemp && g_y_3rd != g_y_min)
    {
        g_x_3rd = g_x_max;
    }

    ftemp = std::fabs(g_y_3rd-g_y_min);
    ftemp2 = std::fabs(g_y_max-g_y_3rd);
    if (ftemp < ftemp2)
    {
        if (ftemp*10000 < ftemp2 && g_x_3rd != g_x_max)
        {
            g_y_3rd = g_y_min;
        }
    }

    if (ftemp2*10000 < ftemp && g_x_3rd != g_x_min)
    {
        g_y_3rd = g_y_max;
    }

}

static void adjust_to_limitsbf(double expand)
{
    LDBL limit;
    bf_t bcornerx[4], bcornery[4];
    bf_t blowx, bhighx, blowy, bhighy, blimit, bftemp;
    bf_t bcenterx, bcentery, badjx, badjy, btmp1, btmp2;
    bf_t bexpand;
    int saved;
    saved = save_stack();
    bcornerx[0] = alloc_stack(rbflength+2);
    bcornerx[1] = alloc_stack(rbflength+2);
    bcornerx[2] = alloc_stack(rbflength+2);
    bcornerx[3] = alloc_stack(rbflength+2);
    bcornery[0] = alloc_stack(rbflength+2);
    bcornery[1] = alloc_stack(rbflength+2);
    bcornery[2] = alloc_stack(rbflength+2);
    bcornery[3] = alloc_stack(rbflength+2);
    blowx       = alloc_stack(rbflength+2);
    bhighx      = alloc_stack(rbflength+2);
    blowy       = alloc_stack(rbflength+2);
    bhighy      = alloc_stack(rbflength+2);
    blimit      = alloc_stack(rbflength+2);
    bftemp      = alloc_stack(rbflength+2);
    bcenterx    = alloc_stack(rbflength+2);
    bcentery    = alloc_stack(rbflength+2);
    badjx       = alloc_stack(rbflength+2);
    badjy       = alloc_stack(rbflength+2);
    btmp1       = alloc_stack(rbflength+2);
    btmp2       = alloc_stack(rbflength+2);
    bexpand     = alloc_stack(rbflength+2);

    limit = 32767.99;

    /*   if (bitshift >= 24) limit = 31.99;
       if (bitshift >= 29) limit = 3.99; */
    floattobf(blimit, limit);
    floattobf(bexpand, expand);

    add_bf(bcenterx, g_bf_x_min, g_bf_x_max);
    half_a_bf(bcenterx);

    // centery = (yymin+yymax)/2;
    add_bf(bcentery, g_bf_y_min, g_bf_y_max);
    half_a_bf(bcentery);

    // if (xxmin == centerx) {
    if (cmp_bf(g_bf_x_min, bcenterx) == 0)
    {
        // ohoh, infinitely thin, fix it
        smallest_add_bf(g_bf_x_max);
        // bfxmin -= bfxmax-centerx;
        sub_a_bf(g_bf_x_min, sub_bf(btmp1, g_bf_x_max, bcenterx));
    }

    // if (bfymin == centery)
    if (cmp_bf(g_bf_y_min, bcentery) == 0)
    {
        smallest_add_bf(g_bf_y_max);
        // bfymin -= bfymax-centery;
        sub_a_bf(g_bf_y_min, sub_bf(btmp1, g_bf_y_max, bcentery));
    }

    // if (bfx3rd == centerx)
    if (cmp_bf(g_bf_x_3rd, bcenterx) == 0)
    {
        smallest_add_bf(g_bf_x_3rd);
    }

    // if (bfy3rd == centery)
    if (cmp_bf(g_bf_y_3rd, bcentery) == 0)
    {
        smallest_add_bf(g_bf_y_3rd);
    }

    // setup array for easier manipulation
    // cornerx[0] = xxmin;
    copy_bf(bcornerx[0], g_bf_x_min);

    // cornerx[1] = xxmax;
    copy_bf(bcornerx[1], g_bf_x_max);

    // cornerx[2] = xx3rd;
    copy_bf(bcornerx[2], g_bf_x_3rd);

    // cornerx[3] = xxmin+(xxmax-xx3rd);
    sub_bf(bcornerx[3], g_bf_x_max, g_bf_x_3rd);
    add_a_bf(bcornerx[3], g_bf_x_min);

    // cornery[0] = yymax;
    copy_bf(bcornery[0], g_bf_y_max);

    // cornery[1] = yymin;
    copy_bf(bcornery[1], g_bf_y_min);

    // cornery[2] = yy3rd;
    copy_bf(bcornery[2], g_bf_y_3rd);

    // cornery[3] = yymin+(yymax-yy3rd);
    sub_bf(bcornery[3], g_bf_y_max, g_bf_y_3rd);
    add_a_bf(bcornery[3], g_bf_y_min);

    // if caller wants image size adjusted, do that first
    if (expand != 1.0)
    {
        for (int i = 0; i < 4; ++i)
        {
            // cornerx[i] = centerx + (cornerx[i]-centerx)*expand;
            sub_bf(btmp1, bcornerx[i], bcenterx);
            mult_bf(bcornerx[i], btmp1, bexpand);
            add_a_bf(bcornerx[i], bcenterx);

            // cornery[i] = centery + (cornery[i]-centery)*expand;
            sub_bf(btmp1, bcornery[i], bcentery);
            mult_bf(bcornery[i], btmp1, bexpand);
            add_a_bf(bcornery[i], bcentery);
        }
    }

    // get min/max x/y values
    // lowx = highx = cornerx[0];
    copy_bf(blowx, bcornerx[0]);
    copy_bf(bhighx, bcornerx[0]);

    // lowy = highy = cornery[0];
    copy_bf(blowy, bcornery[0]);
    copy_bf(bhighy, bcornery[0]);

    for (int i = 1; i < 4; ++i)
    {
        // if (cornerx[i] < lowx)               lowx  = cornerx[i];
        if (cmp_bf(bcornerx[i], blowx) < 0)
        {
            copy_bf(blowx, bcornerx[i]);
        }

        // if (cornerx[i] > highx)              highx = cornerx[i];
        if (cmp_bf(bcornerx[i], bhighx) > 0)
        {
            copy_bf(bhighx, bcornerx[i]);
        }

        // if (cornery[i] < lowy)               lowy  = cornery[i];
        if (cmp_bf(bcornery[i], blowy) < 0)
        {
            copy_bf(blowy, bcornery[i]);
        }

        // if (cornery[i] > highy)              highy = cornery[i];
        if (cmp_bf(bcornery[i], bhighy) > 0)
        {
            copy_bf(bhighy, bcornery[i]);
        }
    }

    // if image is too large, downsize it maintaining center
    // ftemp = highx-lowx;
    sub_bf(bftemp, bhighx, blowx);

    // if (highy-lowy > ftemp) ftemp = highy-lowy;
    if (cmp_bf(sub_bf(btmp1, bhighy, blowy), bftemp) > 0)
    {
        copy_bf(bftemp, btmp1);
    }

    // if image is too large, downsize it maintaining center

    floattobf(btmp1, limit*2.0);
    copy_bf(btmp2, bftemp);
    div_bf(bftemp, btmp1, btmp2);
    floattobf(btmp1, 1.0);
    if (cmp_bf(bftemp, btmp1) < 0)
    {
        for (int i = 0; i < 4; ++i)
        {
            // cornerx[i] = centerx + (cornerx[i]-centerx)*ftemp;
            sub_bf(btmp1, bcornerx[i], bcenterx);
            mult_bf(bcornerx[i], btmp1, bftemp);
            add_a_bf(bcornerx[i], bcenterx);

            // cornery[i] = centery + (cornery[i]-centery)*ftemp;
            sub_bf(btmp1, bcornery[i], bcentery);
            mult_bf(bcornery[i], btmp1, bftemp);
            add_a_bf(bcornery[i], bcentery);
        }
    }

    // if any corner has x or y past limit, move the image
    // adjx = adjy = 0;
    clear_bf(badjx);
    clear_bf(badjy);

    for (int i = 0; i < 4; ++i)
    {
        /* if (cornerx[i] > limit && (ftemp = cornerx[i] - limit) > adjx)
           adjx = ftemp; */
        if (cmp_bf(bcornerx[i], blimit) > 0
            && cmp_bf(sub_bf(bftemp, bcornerx[i], blimit), badjx) > 0)
        {
            copy_bf(badjx, bftemp);
        }

        /* if (cornerx[i] < 0.0-limit && (ftemp = cornerx[i] + limit) < adjx)
           adjx = ftemp; */
        if (cmp_bf(bcornerx[i], neg_bf(btmp1, blimit)) < 0
            && cmp_bf(add_bf(bftemp, bcornerx[i], blimit), badjx) < 0)
        {
            copy_bf(badjx, bftemp);
        }

        /* if (cornery[i] > limit  && (ftemp = cornery[i] - limit) > adjy)
           adjy = ftemp; */
        if (cmp_bf(bcornery[i], blimit) > 0
            && cmp_bf(sub_bf(bftemp, bcornery[i], blimit), badjy) > 0)
        {
            copy_bf(badjy, bftemp);
        }

        /* if (cornery[i] < 0.0-limit && (ftemp = cornery[i] + limit) < adjy)
           adjy = ftemp; */
        if (cmp_bf(bcornery[i], neg_bf(btmp1, blimit)) < 0
            && cmp_bf(add_bf(bftemp, bcornery[i], blimit), badjy) < 0)
        {
            copy_bf(badjy, bftemp);
        }
    }

    /* if (g_calc_status == calc_status_value::RESUMABLE && (adjx != 0 || adjy != 0) && (zoom_box_width == 1.0))
       g_calc_status = calc_status_value::PARAMS_CHANGED; */
    if (g_calc_status == calc_status_value::RESUMABLE
        && (is_bf_not_zero(badjx)|| is_bf_not_zero(badjy))
        && (g_zoom_box_width == 1.0))
    {
        g_calc_status = calc_status_value::PARAMS_CHANGED;
    }

    // xxmin = cornerx[0] - adjx;
    sub_bf(g_bf_x_min, bcornerx[0], badjx);
    // xxmax = cornerx[1] - adjx;
    sub_bf(g_bf_x_max, bcornerx[1], badjx);
    // xx3rd = cornerx[2] - adjx;
    sub_bf(g_bf_x_3rd, bcornerx[2], badjx);
    // yymax = cornery[0] - adjy;
    sub_bf(g_bf_y_max, bcornery[0], badjy);
    // yymin = cornery[1] - adjy;
    sub_bf(g_bf_y_min, bcornery[1], badjy);
    // yy3rd = cornery[2] - adjy;
    sub_bf(g_bf_y_3rd, bcornery[2], badjy);

    adjust_cornerbf(); // make 3rd corner exact if very near other co-ords
    restore_stack(saved);
}

static void adjust_to_limits(double expand)
{
    double cornerx[4], cornery[4];
    double lowx, highx, lowy, highy, limit, ftemp;
    double centerx, centery, adjx, adjy;

    limit = 32767.99;

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

    centerx = (g_x_min+g_x_max)/2;
    centery = (g_y_min+g_y_max)/2;

    if (g_x_min == centerx)
    {
        // ohoh, infinitely thin, fix it
        smallest_add(&g_x_max);
        g_x_min -= g_x_max-centerx;
    }

    if (g_y_min == centery)
    {
        smallest_add(&g_y_max);
        g_y_min -= g_y_max-centery;
    }

    if (g_x_3rd == centerx)
    {
        smallest_add(&g_x_3rd);
    }

    if (g_y_3rd == centery)
    {
        smallest_add(&g_y_3rd);
    }

    // setup array for easier manipulation
    cornerx[0] = g_x_min;
    cornerx[1] = g_x_max;
    cornerx[2] = g_x_3rd;
    cornerx[3] = g_x_min+(g_x_max-g_x_3rd);

    cornery[0] = g_y_max;
    cornery[1] = g_y_min;
    cornery[2] = g_y_3rd;
    cornery[3] = g_y_min+(g_y_max-g_y_3rd);

    // if caller wants image size adjusted, do that first
    if (expand != 1.0)
    {
        for (int i = 0; i < 4; ++i)
        {
            cornerx[i] = centerx + (cornerx[i]-centerx)*expand;
            cornery[i] = centery + (cornery[i]-centery)*expand;
        }
    }
    // get min/max x/y values
    highx = cornerx[0];
    lowx = highx;
    highy = cornery[0];
    lowy = highy;

    for (int i = 1; i < 4; ++i)
    {
        if (cornerx[i] < lowx)
        {
            lowx  = cornerx[i];
        }
        if (cornerx[i] > highx)
        {
            highx = cornerx[i];
        }
        if (cornery[i] < lowy)
        {
            lowy  = cornery[i];
        }
        if (cornery[i] > highy)
        {
            highy = cornery[i];
        }
    }

    // if image is too large, downsize it maintaining center
    ftemp = highx-lowx;

    if (highy-lowy > ftemp)
    {
        ftemp = highy-lowy;
    }

    // if image is too large, downsize it maintaining center
    ftemp = limit*2/ftemp;
    if (ftemp < 1.0)
    {
        for (int i = 0; i < 4; ++i)
        {
            cornerx[i] = centerx + (cornerx[i]-centerx)*ftemp;
            cornery[i] = centery + (cornery[i]-centery)*ftemp;
        }
    }

    // if any corner has x or y past limit, move the image
    adjy = 0;
    adjx = adjy;

    for (int i = 0; i < 4; ++i)
    {
        if (cornerx[i] > limit && (ftemp = cornerx[i] - limit) > adjx)
        {
            adjx = ftemp;
        }
        if (cornerx[i] < 0.0-limit && (ftemp = cornerx[i] + limit) < adjx)
        {
            adjx = ftemp;
        }
        if (cornery[i] > limit     && (ftemp = cornery[i] - limit) > adjy)
        {
            adjy = ftemp;
        }
        if (cornery[i] < 0.0-limit && (ftemp = cornery[i] + limit) < adjy)
        {
            adjy = ftemp;
        }
    }
    if (g_calc_status == calc_status_value::RESUMABLE && (adjx != 0 || adjy != 0) && (g_zoom_box_width == 1.0))
    {
        g_calc_status = calc_status_value::PARAMS_CHANGED;
    }
    g_x_min = cornerx[0] - adjx;
    g_x_max = cornerx[1] - adjx;
    g_x_3rd = cornerx[2] - adjx;
    g_y_max = cornery[0] - adjy;
    g_y_min = cornery[1] - adjy;
    g_y_3rd = cornery[2] - adjy;

    adjust_corner(); // make 3rd corner exact if very near other co-ords
}

static void smallest_add(double *num)
{
    *num += *num * 5.0e-16;
}

static void smallest_add_bf(bf_t num)
{
    bf_t btmp1;
    int saved;
    saved = save_stack();
    btmp1 = alloc_stack(bflength+2);
    mult_bf(btmp1, floattobf(btmp1, 5.0e-16), num);
    add_a_bf(num, btmp1);
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
    if (desired != 0 && g_debug_flag != debug_flags::prevent_arbitrary_precision_math)
    {
        double ftemp = actual / desired;
        if (ftemp < (1.0-tol) || ftemp > (1.0+tol))
        {
            return 1;
        }
    }
    return 0;
}
