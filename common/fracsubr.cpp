/*
FRACSUBR.C contains subroutines which belong primarily to CALCFRAC.C and
FRACTALS.C, i.e. which are non-fractal-specific fractal engine subroutines.
*/
#include <vector>

#include <float.h>
#include <limits.h>
#include <memory.h>
#include <stdarg.h>
#include <stdlib.h>
#ifndef XFRACT
#include <sys/timeb.h>
#endif
#include <sys/types.h>
#include <time.h>

// see Fractint.c for a description of the "include"  hierarchy
#include "port.h"
#include "prototyp.h"
#include "fractype.h"
#include "drivers.h"

#if defined(_WIN32)
#define ftimex ftime
#define timebx timeb
#endif

// routines in this module

static long   fudgetolong(double d);
static double fudgetodouble(long l);
static void   adjust_to_limits(double);
static void   smallest_add(double *);
static int    ratio_bad(double, double);
static void   plotdorbit(double, double, int);
static int    combine_worklist();

static void   adjust_to_limitsbf(double);
static void   smallest_add_bf(bf_t);
int    g_resume_len;               // length of resume info
static int    resume_offset;            // offset in resume info gets
bool    g_tab_or_help = false;    // kludge for sound and tab or help key press

namespace
{

enum
{
    NUM_SAVE_ORBIT = 1500
};

int save_orbit[NUM_SAVE_ORBIT] = { 0 };           // array to save orbit values

}

#define FUDGEFACTOR     29      // fudge all values up by 2**this
#define FUDGEFACTOR2    24      // (or maybe this)

void free_grid_pointers()
{
    g_grid_x0.clear();
    g_grid_y0.clear();
    g_grid_x1.clear();
    g_grid_y1.clear();
    g_l_x0.clear();
    g_l_y0.clear();
    g_l_x1.clear();
    g_l_y1.clear();
}

void set_grid_pointers()
{
    free_grid_pointers();
    g_grid_x0.resize(xdots);
    g_grid_y1.resize(xdots);

    g_grid_y0.resize(ydots);
    g_grid_x1.resize(ydots);

    g_l_x0.resize(xdots);
    g_l_y1.resize(xdots);

    g_l_y0.resize(ydots);
    g_l_x1.resize(ydots);
    set_pixel_calc_functions();
}

void fill_dx_array()
{
    if (g_use_grid)
    {
        g_grid_x0[0] = xxmin;              // fill up the x, y grids
        g_grid_y0[0] = yymax;
        g_grid_y1[0] = 0;
        g_grid_x1[0] = g_grid_y1[0];
        for (int i = 1; i < xdots; i++)
        {
            g_grid_x0[i] = (double)(g_grid_x0[0] + i*g_delta_x);
            g_grid_y1[i] = (double)(g_grid_y1[0] - i*g_delta_y2);
        }
        for (int i = 1; i < ydots; i++)
        {
            g_grid_y0[i] = (double)(g_grid_y0[0] - i*g_delta_y);
            g_grid_x1[i] = (double)(g_grid_x1[0] + i*g_delta_x2);
        }
    }
}

void fill_lx_array()
{
    // note that lx1 & ly1 values can overflow into sign bit; since
    // they're used only to add to lx0/ly0, 2s comp straightens it out
    if (g_use_grid)
    {
        g_l_x0[0] = xmin;               // fill up the x, y grids
        g_l_y0[0] = ymax;
        g_l_y1[0] = 0;
        g_l_x1[0] = g_l_y1[0];
        for (int i = 1; i < xdots; i++)
        {
            g_l_x0[i] = g_l_x0[i-1] + g_l_delta_x;
            g_l_y1[i] = g_l_y1[i-1] - g_l_delta_y2;
        }
        for (int i = 1; i < ydots; i++)
        {
            g_l_y0[i] = g_l_y0[i-1] - g_l_delta_y;
            g_l_x1[i] = g_l_x1[i-1] + g_l_delta_x2;
        }
    }
}

void fractal_floattobf()
{
    init_bf_dec(getprecdbl(CURRENTREZ));
    floattobf(bfxmin, xxmin);
    floattobf(bfxmax, xxmax);
    floattobf(bfymin, yymin);
    floattobf(bfymax, yymax);
    floattobf(bfx3rd, xx3rd);
    floattobf(bfy3rd, yy3rd);

    for (int i = 0; i < MAXPARAMS; i++)
    {
        if (typehasparm(fractype, i, nullptr))
        {
            floattobf(bfparms[i], g_params[i]);
        }
    }
    g_calc_status = calc_status_value::PARAMS_CHANGED;
}


bool g_use_grid = false;

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
    long xytemp = xdots + ydots;
    if ((!g_user_float_flag && (xytemp*sizeof(long) > 32768)) ||
            (g_user_float_flag && (xytemp*sizeof(double) > 32768)) ||
            g_debug_flag == debug_flags::prevent_coordinate_grid)
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

    if (!(curfractalspecific->flags & BF_MATH))
    {
        fractal_type tofloat = curfractalspecific->tofloat;
        if (tofloat == fractal_type::NOFRACTAL)
        {
            bf_math = bf_math_type::NONE;
        }
        else if (!(fractalspecific[static_cast<int>(tofloat)].flags & BF_MATH))
        {
            bf_math = bf_math_type::NONE;
        }
        else if (bf_math != bf_math_type::NONE)
        {
            curfractalspecific = &fractalspecific[static_cast<int>(tofloat)];
            fractype = tofloat;
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
    else if ((fractype == fractal_type::MANDEL || fractype == fractal_type::MANDELFP) && g_debug_flag == debug_flags::force_arbitrary_precision_math)
    {
        fractype = fractal_type::MANDELFP;
        curfractalspecific = &fractalspecific[static_cast<int>(fractal_type::MANDELFP)];
        fractal_floattobf();
        g_user_float_flag = true;
    }
    else if ((fractype == fractal_type::JULIA || fractype == fractal_type::JULIAFP) && g_debug_flag == debug_flags::force_arbitrary_precision_math)
    {
        fractype = fractal_type::JULIAFP;
        curfractalspecific = &fractalspecific[static_cast<int>(fractal_type::JULIAFP)];
        fractal_floattobf();
        g_user_float_flag = true;
    }
    else if ((fractype == fractal_type::LMANDELZPOWER || fractype == fractal_type::FPMANDELZPOWER) && g_debug_flag == debug_flags::force_arbitrary_precision_math)
    {
        fractype = fractal_type::FPMANDELZPOWER;
        curfractalspecific = &fractalspecific[static_cast<int>(fractal_type::FPMANDELZPOWER)];
        fractal_floattobf();
        g_user_float_flag = true;
    }
    else if ((fractype == fractal_type::LJULIAZPOWER || fractype == fractal_type::FPJULIAZPOWER) && g_debug_flag == debug_flags::force_arbitrary_precision_math)
    {
        fractype = fractal_type::FPJULIAZPOWER;
        curfractalspecific = &fractalspecific[static_cast<int>(fractal_type::FPJULIAZPOWER)];
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
        g_float_flag = curfractalspecific->isinteger == 0;
    }
    // if floating pt only, set floatflag for TAB screen
    if (!curfractalspecific->isinteger && curfractalspecific->tofloat == fractal_type::NOFRACTAL)
    {
        g_float_flag = true;
    }
    if (g_user_std_calc_mode == 's')
    {
        if (fractype == fractal_type::MANDEL || fractype == fractal_type::MANDELFP)
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
            && (curfractalspecific->calctype == standard_fractal
                || curfractalspecific->calctype == calcmand
                || curfractalspecific->calctype == calcmandfp))
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
        if (curfractalspecific->isinteger != 0
                && curfractalspecific->tofloat != fractal_type::NOFRACTAL)
        {
            fractype = curfractalspecific->tofloat;
        }
    }
    else
    {
        if (curfractalspecific->isinteger == 0
                && curfractalspecific->tofloat != fractal_type::NOFRACTAL)
        {
            fractype = curfractalspecific->tofloat;
        }
    }
    // match Julibrot with integer mode of orbit
    if (fractype == fractal_type::JULIBROTFP && fractalspecific[static_cast<int>(g_new_orbit_type)].isinteger)
    {
        fractal_type i = fractalspecific[static_cast<int>(g_new_orbit_type)].tofloat;
        if (i != fractal_type::NOFRACTAL)
        {
            g_new_orbit_type = i;
        }
        else
        {
            fractype = fractal_type::JULIBROT;
        }
    }
    else if (fractype == fractal_type::JULIBROT && fractalspecific[static_cast<int>(g_new_orbit_type)].isinteger == 0)
    {
        fractal_type i = fractalspecific[static_cast<int>(g_new_orbit_type)].tofloat;
        if (i != fractal_type::NOFRACTAL)
        {
            g_new_orbit_type = i;
        }
        else
        {
            fractype = fractal_type::JULIBROTFP;
        }
    }

    curfractalspecific = &fractalspecific[static_cast<int>(fractype)];

    g_integer_fractal = curfractalspecific->isinteger;

    if (g_potential_flag && g_potential_params[2] != 0.0)
    {
        rqlim = g_potential_params[2];
    }
    else if (g_bail_out)     // user input bailout
    {
        rqlim = g_bail_out;
    }
    else if (g_biomorph != -1)     // biomorph benefits from larger bailout
    {
        rqlim = 100;
    }
    else
    {
        rqlim = curfractalspecific->orbit_bailout;
    }
    if (g_integer_fractal)   // the bailout limit mustn't be too high here
    {
        if (rqlim > 127.0)
        {
            rqlim = 127.0;
        }
    }

    if ((curfractalspecific->flags&NOROTATE) != 0)
    {
        // ensure min<max and unrotated rectangle
        if (xxmin > xxmax)
        {
            double ftemp = xxmax;
            xxmax = xxmin;
            xxmin = ftemp;
        }
        if (yymin > yymax)
        {
            double ftemp = yymax;
            yymax = yymin;
            yymin = ftemp;
        }
        xx3rd = xxmin;
        yy3rd = yymin;
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
        fractal_type i = curfractalspecific->tofloat;
        if (i != fractal_type::NOFRACTAL) // -> int?
        {
            if (fractalspecific[static_cast<int>(i)].isinteger > 1)   // specific shift?
            {
                g_bit_shift = fractalspecific[static_cast<int>(i)].isinteger;
            }
        }
        else
        {
            g_bit_shift = 16;  // to allow larger corners
        }
    }
    // We want this code if we're using the assembler calcmand
    if (fractype == fractal_type::MANDEL || fractype == fractal_type::JULIA)
    {
        // adjust shift bits if..
        if (!g_potential_flag                                    // not using potential
                && (g_params[0] > -2.0 && g_params[0] < 2.0)  // parameters not too large
                && (g_params[1] > -2.0 && g_params[1] < 2.0)
                && (g_invert == 0)                        // and not inverting
                && g_biomorph == -1                     // and not biomorphing
                && rqlim <= 4.0                         // and bailout not too high
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
        g_delta_x  = (LDBL)(xxmax - xx3rd) / (LDBL)g_x_size_dots; // calculate stepsizes
        g_delta_y  = (LDBL)(yymax - yy3rd) / (LDBL)g_y_size_dots;
        g_delta_x2 = (LDBL)(xx3rd - xxmin) / (LDBL)g_y_size_dots;
        g_delta_y2 = (LDBL)(yy3rd - yymin) / (LDBL)g_x_size_dots;
        fill_dx_array();
    }

    if (fractype != fractal_type::CELLULAR && fractype != fractal_type::ANT)  // fudgetolong fails w >10 digits in double
    {
        xmin  = fudgetolong(xxmin);
        xmax  = fudgetolong(xxmax);
        x3rd  = fudgetolong(xx3rd);
        ymin  = fudgetolong(yymin);
        ymax  = fudgetolong(yymax);
        y3rd  = fudgetolong(yy3rd);
        g_l_delta_x  = fudgetolong((double)g_delta_x);
        g_l_delta_y  = fudgetolong((double)g_delta_y);
        g_l_delta_x2 = fudgetolong((double)g_delta_x2);
        g_l_delta_y2 = fudgetolong((double)g_delta_y2);
    }

    // skip this if plasma to avoid 3d problems
    // skip if bf_math to avoid extraseg conflict with dx0 arrays
    // skip if ifs, ifs3d, or lsystem to avoid crash when mathtolerance
    // is set.  These types don't auto switch between float and integer math
    if (fractype != fractal_type::PLASMA && bf_math == bf_math_type::NONE
            && fractype != fractal_type::IFS && fractype != fractal_type::IFS3D && fractype != fractal_type::LSYSTEM)
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
            if (ratio_bad((double)g_l_x0[xdots-1]-xmin, (double)xmax-x3rd)
                    || ratio_bad((double)g_l_y0[ydots-1]-ymax, (double)y3rd-ymax)
                    || ratio_bad((double)g_l_x1[(ydots >> 1)-1], ((double)x3rd-xmin)/2)
                    || ratio_bad((double)g_l_y1[(xdots >> 1)-1], ((double)ymin-y3rd)/2))
            {
expand_retry:
                if (g_integer_fractal          // integer fractal type?
                        && curfractalspecific->tofloat != fractal_type::NOFRACTAL)
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
            xmax = g_l_x0[xdots-1] + g_l_x1[ydots-1];
            ymin = g_l_y0[ydots-1] + g_l_y1[xdots-1];
            x3rd = xmin + g_l_x1[ydots-1];
            y3rd = g_l_y0[ydots-1];
            xxmin = fudgetodouble(xmin);
            xxmax = fudgetodouble(xmax);
            xx3rd = fudgetodouble(x3rd);
            yymin = fudgetodouble(ymin);
            yymax = fudgetodouble(ymax);
            yy3rd = fudgetodouble(y3rd);
        } // end if (integerfractal && !invert && use_grid)
        else
        {
            double dx0, dy0, dx1, dy1;
            // set up dx0 and dy0 analogs of lx0 and ly0
            // put fractal parameters in doubles
            dx0 = xxmin;                // fill up the x, y grids
            dy0 = yymax;
            dy1 = 0;
            dx1 = dy1;
            /* this way of defining the dx and dy arrays is not the most
               accurate, but it is kept because it is used to determine
               the limit of resolution */
            for (int i = 1; i < xdots; i++)
            {
                dx0 = (double)(dx0 + (double)g_delta_x);
                dy1 = (double)(dy1 - (double)g_delta_y2);
            }
            for (int i = 1; i < ydots; i++)
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
                    if (fabs(xxmax-xx3rd) > fabs(xx3rd-xxmin))
                    {
                        testx_exact  = xxmax-xx3rd;
                        testx_try    = dx0-xxmin;
                    }
                    else
                    {
                        testx_exact  = xx3rd-xxmin;
                        testx_try    = dx1;
                    }
                    double testy_try;
                    double testy_exact;
                    if (fabs(yy3rd-yymax) > fabs(yymin-yy3rd))
                    {
                        testy_exact = yy3rd-yymax;
                        testy_try   = dy0-yymax;
                    }
                    else
                    {
                        testy_exact = yymin-yy3rd;
                        testy_try   = dy1;
                    }
                    if (ratio_bad(testx_try, testx_exact) ||
                            ratio_bad(testy_try, testy_exact))
                    {
                        if (curfractalspecific->flags & BF_MATH)
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
            xxmax = (double)(xxmin + (xdots-1)*g_delta_x + (ydots-1)*g_delta_x2);
            yymin = (double)(yymax - (ydots-1)*g_delta_y - (xdots-1)*g_delta_y2);
            xx3rd = (double)(xxmin + (ydots-1)*g_delta_x2);
            yy3rd = (double)(yymax - (ydots-1)*g_delta_y);

        } // end else
    } // end if not plasma

    // for periodicity close-enough, and for unity:
    //     min(max(delx,delx2),max(dely,dely2))
    g_delta_min = fabs((double)g_delta_x);
    if (fabs((double)g_delta_x2) > g_delta_min)
    {
        g_delta_min = fabs((double)g_delta_x2);
    }
    if (fabs((double)g_delta_y) > fabs((double)g_delta_y2))
    {
        if (fabs((double)g_delta_y) < g_delta_min)
        {
            g_delta_min = fabs((double)g_delta_y);
        }
    }
    else if (fabs((double)g_delta_y2) < g_delta_min)
    {
        g_delta_min = fabs((double)g_delta_y2);
    }
    g_l_delta_min = fudgetolong(g_delta_min);

    // calculate factors which plot real values to screen co-ords
    // calcfrac.c plot_orbit routines have comments about this
    double ftemp = (double)((0.0-g_delta_y2) * g_delta_x2 * g_x_size_dots * g_y_size_dots
                     - (xxmax-xx3rd) * (yy3rd-yymax));
    if (ftemp != 0)
    {
        g_plot_mx1 = (double)(g_delta_x2 * g_x_size_dots * g_y_size_dots / ftemp);
        g_plot_mx2 = (yy3rd-yymax) * g_x_size_dots / ftemp;
        g_plot_my1 = (double)((0.0-g_delta_y2) * g_x_size_dots * g_y_size_dots / ftemp);
        g_plot_my2 = (xxmax-xx3rd) * g_y_size_dots / ftemp;
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
    sprintf(buf, "%.9g", (double)l / g_fudge_factor);
#ifndef XFRACT
    sscanf(buf, "%lg", &d);
#else
    sscanf(buf, "%lf", &d);
#endif
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
    ftemp = fabs(Xmagfactor);
    if (ftemp != 1 && ftemp >= (1-g_aspect_drift) && ftemp <= (1+g_aspect_drift))
    {
        Xmagfactor = sign(Xmagfactor);
        cvtcornersbf(bftemp, bftemp2, Magnification, Xmagfactor, Rotation, Skew);
    }

    // ftemp=fabs(xx3rd-xxmin);
    abs_a_bf(sub_bf(bftemp, bfx3rd, bfxmin));

    // ftemp2=fabs(xxmax-xx3rd);
    abs_a_bf(sub_bf(bftemp2, bfxmax, bfx3rd));

    // if ( (ftemp=fabs(xx3rd-xxmin)) < (ftemp2=fabs(xxmax-xx3rd)) )
    if (cmp_bf(bftemp, bftemp2) < 0)
    {
        // if (ftemp*10000 < ftemp2 && yy3rd != yymax)
        if (cmp_bf(mult_bf_int(btmp1, bftemp, 10000), bftemp2) < 0
                && cmp_bf(bfy3rd, bfymax) != 0)
        {
            // xx3rd = xxmin;
            copy_bf(bfx3rd, bfxmin);
        }
    }

    // else if (ftemp2*10000 < ftemp && yy3rd != yymin)
    if (cmp_bf(mult_bf_int(btmp1, bftemp2, 10000), bftemp) < 0
            && cmp_bf(bfy3rd, bfymin) != 0)
    {
        // xx3rd = xxmax;
        copy_bf(bfx3rd, bfxmax);
    }

    // ftemp=fabs(yy3rd-yymin);
    abs_a_bf(sub_bf(bftemp, bfy3rd, bfymin));

    // ftemp2=fabs(yymax-yy3rd);
    abs_a_bf(sub_bf(bftemp2, bfymax, bfy3rd));

    // if ( (ftemp=fabs(yy3rd-yymin)) < (ftemp2=fabs(yymax-yy3rd)) )
    if (cmp_bf(bftemp, bftemp2) < 0)
    {
        // if (ftemp*10000 < ftemp2 && xx3rd != xxmax)
        if (cmp_bf(mult_bf_int(btmp1, bftemp, 10000), bftemp2) < 0
                && cmp_bf(bfx3rd, bfxmax) != 0)
        {
            // yy3rd = yymin;
            copy_bf(bfy3rd, bfymin);
        }
    }

    // else if (ftemp2*10000 < ftemp && xx3rd != xxmin)
    if (cmp_bf(mult_bf_int(btmp1, bftemp2, 10000), bftemp) < 0
            && cmp_bf(bfx3rd, bfxmin) != 0)
    {
        // yy3rd = yymax;
        copy_bf(bfy3rd, bfymax);
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
        ftemp = fabs(Xmagfactor);
        if (ftemp != 1 && ftemp >= (1-g_aspect_drift) && ftemp <= (1+g_aspect_drift))
        {
            Xmagfactor = sign(Xmagfactor);
            cvtcorners(Xctr, Yctr, Magnification, Xmagfactor, Rotation, Skew);
        }
    }

    ftemp = fabs(xx3rd-xxmin);
    ftemp2 = fabs(xxmax-xx3rd);
    if (ftemp < ftemp2)
    {
        if (ftemp*10000 < ftemp2 && yy3rd != yymax)
        {
            xx3rd = xxmin;
        }
    }

    if (ftemp2*10000 < ftemp && yy3rd != yymin)
    {
        xx3rd = xxmax;
    }

    ftemp = fabs(yy3rd-yymin);
    ftemp2 = fabs(yymax-yy3rd);
    if (ftemp < ftemp2)
    {
        if (ftemp*10000 < ftemp2 && xx3rd != xxmax)
        {
            yy3rd = yymin;
        }
    }

    if (ftemp2*10000 < ftemp && xx3rd != xxmin)
    {
        yy3rd = yymax;
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

    add_bf(bcenterx, bfxmin, bfxmax);
    half_a_bf(bcenterx);

    // centery = (yymin+yymax)/2;
    add_bf(bcentery, bfymin, bfymax);
    half_a_bf(bcentery);

    // if (xxmin == centerx) {
    if (cmp_bf(bfxmin, bcenterx) == 0)
    {
        // ohoh, infinitely thin, fix it
        smallest_add_bf(bfxmax);
        // bfxmin -= bfxmax-centerx;
        sub_a_bf(bfxmin, sub_bf(btmp1, bfxmax, bcenterx));
    }

    // if (bfymin == centery)
    if (cmp_bf(bfymin, bcentery) == 0)
    {
        smallest_add_bf(bfymax);
        // bfymin -= bfymax-centery;
        sub_a_bf(bfymin, sub_bf(btmp1, bfymax, bcentery));
    }

    // if (bfx3rd == centerx)
    if (cmp_bf(bfx3rd, bcenterx) == 0)
    {
        smallest_add_bf(bfx3rd);
    }

    // if (bfy3rd == centery)
    if (cmp_bf(bfy3rd, bcentery) == 0)
    {
        smallest_add_bf(bfy3rd);
    }

    // setup array for easier manipulation
    // cornerx[0] = xxmin;
    copy_bf(bcornerx[0], bfxmin);

    // cornerx[1] = xxmax;
    copy_bf(bcornerx[1], bfxmax);

    // cornerx[2] = xx3rd;
    copy_bf(bcornerx[2], bfx3rd);

    // cornerx[3] = xxmin+(xxmax-xx3rd);
    sub_bf(bcornerx[3], bfxmax, bfx3rd);
    add_a_bf(bcornerx[3], bfxmin);

    // cornery[0] = yymax;
    copy_bf(bcornery[0], bfymax);

    // cornery[1] = yymin;
    copy_bf(bcornery[1], bfymin);

    // cornery[2] = yy3rd;
    copy_bf(bcornery[2], bfy3rd);

    // cornery[3] = yymin+(yymax-yy3rd);
    sub_bf(bcornery[3], bfymax, bfy3rd);
    add_a_bf(bcornery[3], bfymin);

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
        if (cmp_bf(bcornerx[i], blimit) > 0 &&
                cmp_bf(sub_bf(bftemp, bcornerx[i], blimit), badjx) > 0)
        {
            copy_bf(badjx, bftemp);
        }

        /* if (cornerx[i] < 0.0-limit && (ftemp = cornerx[i] + limit) < adjx)
           adjx = ftemp; */
        if (cmp_bf(bcornerx[i], neg_bf(btmp1, blimit)) < 0 &&
                cmp_bf(add_bf(bftemp, bcornerx[i], blimit), badjx) < 0)
        {
            copy_bf(badjx, bftemp);
        }

        /* if (cornery[i] > limit  && (ftemp = cornery[i] - limit) > adjy)
           adjy = ftemp; */
        if (cmp_bf(bcornery[i], blimit) > 0 &&
                cmp_bf(sub_bf(bftemp, bcornery[i], blimit), badjy) > 0)
        {
            copy_bf(badjy, bftemp);
        }

        /* if (cornery[i] < 0.0-limit && (ftemp = cornery[i] + limit) < adjy)
           adjy = ftemp; */
        if (cmp_bf(bcornery[i], neg_bf(btmp1, blimit)) < 0 &&
                cmp_bf(add_bf(bftemp, bcornery[i], blimit), badjy) < 0)
        {
            copy_bf(badjy, bftemp);
        }
    }

    /* if (g_calc_status == calc_status_value::RESUMABLE && (adjx != 0 || adjy != 0) && (zoom_box_width == 1.0))
       g_calc_status = calc_status_value::PARAMS_CHANGED; */
    if (g_calc_status == calc_status_value::RESUMABLE && (is_bf_not_zero(badjx)|| is_bf_not_zero(badjy)) && (zoom_box_width == 1.0))
    {
        g_calc_status = calc_status_value::PARAMS_CHANGED;
    }

    // xxmin = cornerx[0] - adjx;
    sub_bf(bfxmin, bcornerx[0], badjx);
    // xxmax = cornerx[1] - adjx;
    sub_bf(bfxmax, bcornerx[1], badjx);
    // xx3rd = cornerx[2] - adjx;
    sub_bf(bfx3rd, bcornerx[2], badjx);
    // yymax = cornery[0] - adjy;
    sub_bf(bfymax, bcornery[0], badjy);
    // yymin = cornery[1] - adjy;
    sub_bf(bfymin, bcornery[1], badjy);
    // yy3rd = cornery[2] - adjy;
    sub_bf(bfy3rd, bcornery[2], badjy);

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
        if (g_save_release > 1940)   // let user reproduce old GIF's and PAR's
        {
            limit = 1023.99;
        }
        if (g_bit_shift >= 24)
        {
            limit = 31.99;
        }
        if (g_bit_shift >= 29)
        {
            limit = 3.99;
        }
    }

    centerx = (xxmin+xxmax)/2;
    centery = (yymin+yymax)/2;

    if (xxmin == centerx)
    {
        // ohoh, infinitely thin, fix it
        smallest_add(&xxmax);
        xxmin -= xxmax-centerx;
    }

    if (yymin == centery)
    {
        smallest_add(&yymax);
        yymin -= yymax-centery;
    }

    if (xx3rd == centerx)
    {
        smallest_add(&xx3rd);
    }

    if (yy3rd == centery)
    {
        smallest_add(&yy3rd);
    }

    // setup array for easier manipulation
    cornerx[0] = xxmin;
    cornerx[1] = xxmax;
    cornerx[2] = xx3rd;
    cornerx[3] = xxmin+(xxmax-xx3rd);

    cornery[0] = yymax;
    cornery[1] = yymin;
    cornery[2] = yy3rd;
    cornery[3] = yymin+(yymax-yy3rd);

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
    if (g_calc_status == calc_status_value::RESUMABLE && (adjx != 0 || adjy != 0) && (zoom_box_width == 1.0))
    {
        g_calc_status = calc_status_value::PARAMS_CHANGED;
    }
    xxmin = cornerx[0] - adjx;
    xxmax = cornerx[1] - adjx;
    xx3rd = cornerx[2] - adjx;
    yymax = cornery[0] - adjy;
    yymin = cornery[1] - adjy;
    yy3rd = cornery[2] - adjy;

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
        return (1);
    }
    else if (tol >= 1.0)
    {
        return (0);
    }
    if (desired != 0 && g_debug_flag != debug_flags::prevent_arbitrary_precision_math)
    {
        double ftemp = actual / desired;
        if (ftemp < (1.0-tol) || ftemp > (1.0+tol))
        {
            return (1);
        }
    }
    return (0);
}


/* Save/resume stuff:

   Engines which aren't resumable can simply ignore all this.

   Before calling the (per_image,calctype) routines (engine), calcfract sets:
      "resuming" to false if new image, true if resuming a partially done image
      "g_calc_status" to IN_PROGRESS
   If an engine is interrupted and wants to be able to resume it must:
      store whatever status info it needs to be able to resume later
      set g_calc_status to RESUMABLE and return
   If subsequently called with resuming true, the engine must restore status
   info and continue from where it left off.

   Since the info required for resume can get rather large for some types,
   it is not stored directly in save_info.  Instead, memory is dynamically
   allocated as required, and stored in .fra files as a separate block.
   To save info for later resume, an engine routine can use:
      alloc_resume(maxsize,version)
         Maxsize must be >= max bytes subsequently saved + 2; over-allocation
         is harmless except for possibility of insufficient mem available;
         undersize is not checked and probably causes serious misbehaviour.
         Version is an arbitrary number so that subsequent revisions of the
         engine can be made backward compatible.
         Alloc_resume sets g_calc_status to RESUMABLE if it succeeds;
         to NON_RESUMABLE if it cannot allocate memory
         (and issues warning to user).
      put_resume({bytes,&argument,} ... 0)
         Can be called as often as required to store the info.
         Is not protected against calls which use more space than allocated.
   To reload info when resuming, use:
      start_resume()
         initializes and returns version number
      get_resume({bytes,&argument,} ... 0)
         inverse of store_resume
      end_resume()
         optional, frees the memory area sooner than would happen otherwise

   Example, save info:
      alloc_resume(sizeof(parmarray)+100,2);
      put_resume(sizeof(row),&row, sizeof(col),&col,
                 sizeof(parmarray),parmarray, 0);
    restore info:
      vsn = start_resume();
      get_resume(sizeof(row),&row, sizeof(col),&col, 0);
      if (vsn >= 2)
         get_resume(sizeof(parmarray),parmarray,0);
      end_resume();

   Engines which allocate a large memory chunk of their own might
   directly set resume_info, resume_len, g_calc_status to avoid doubling
   transient memory needs by using these routines.

   standard_fractal, calcmand, solid_guess, and bound_trace_main are a related
   set of engines for escape-time fractals.  They use a common worklist
   structure for save/resume.  Fractals using these must specify calcmand
   or standard_fractal as the engine in fractalspecificinfo.
   Other engines don't get btm nor ssg, don't get off-axis symmetry nor
   panning (the worklist stuff), and are on their own for save/resume.

   */

int put_resume(int len, ...)
{
    va_list arg_marker;

    if (g_resume_data.empty())
    {
        return (-1);
    }

    va_start(arg_marker, len);
    while (len)
    {
        BYTE const *source_ptr = va_arg(arg_marker, BYTE *);
        std::copy(&source_ptr[0], &source_ptr[len], &g_resume_data[g_resume_len]);
        g_resume_len += len;
        len = va_arg(arg_marker, int);
    }
    va_end(arg_marker);
    return (0);
}

int alloc_resume(int alloclen, int version)
{
    g_resume_data.clear();
    g_resume_data.resize(sizeof(int)*alloclen);
    g_resume_len = 0;
    put_resume(sizeof(version), &version, 0);
    g_calc_status = calc_status_value::RESUMABLE;
    return (0);
}

int get_resume(int len, ...)
{
    va_list arg_marker;

    if (g_resume_data.empty())
    {
        return (-1);
    }
    va_start(arg_marker, len);
    while (len)
    {
        BYTE *dest_ptr = va_arg(arg_marker, BYTE *);
        std::copy(&g_resume_data[resume_offset], &g_resume_data[resume_offset + len], &dest_ptr[0]);
        resume_offset += len;
        len = va_arg(arg_marker, int);
    }
    va_end(arg_marker);
    return (0);
}

int start_resume()
{
    int version;
    if (g_resume_data.empty())
    {
        return (-1);
    }
    resume_offset = 0;
    get_resume(sizeof(version), &version, 0);
    return (version);
}

void end_resume()
{
    g_resume_data.clear();
}


/* Showing orbit requires converting real co-ords to screen co-ords.
   Define:
       Xs == xxmax-xx3rd               Ys == yy3rd-yymax
       W  == xdots-1                   D  == ydots-1
   We know that:
       realx == lx0[col] + lx1[row]
       realy == ly0[row] + ly1[col]
       lx0[col] == (col/width) * Xs + xxmin
       lx1[row] == row * delxx
       ly0[row] == (row/D) * Ys + yymax
       ly1[col] == col * (0-delyy)
  so:
       realx == (col/W) * Xs + xxmin + row * delxx
       realy == (row/D) * Ys + yymax + col * (0-delyy)
  and therefore:
       row == (realx-xxmin - (col/W)*Xs) / Xv    (1)
       col == (realy-yymax - (row/D)*Ys) / Yv    (2)
  substitute (2) into (1) and solve for row:
       row == ((realx-xxmin)*(0-delyy2)*W*D - (realy-yymax)*Xs*D)
                      / ((0-delyy2)*W*delxx2*D-Ys*Xs)
  */

// sleep N * a tenth of a millisecond

void sleepms_old(long ms)
{
    static long scalems = 0L;
    int old_help_mode;
    timebx t1, t2;
#define SLEEPINIT 250 // milliseconds for calibration
    bool const save_tab_mode = g_tab_mode;
    old_help_mode = g_help_mode;
    g_tab_mode = false;
    g_help_mode = -1;
    if (scalems == 0L) // calibrate
    {
        /* selects a value of scalems that makes the units
           10000 per sec independent of CPU speed */
        int i, elapsed;
        scalems = 1L;
        if (driver_key_pressed())   // check at start, hope to get start of timeslice
        {
            goto sleepexit;
        }
        // calibrate, assume slow computer first
        showtempmsg("Calibrating timer");
        do
        {
            scalems *= 2;
            ftimex(&t2);
            do
            {
                // wait for the start of a new tick
                ftimex(&t1);
            }
            while (t2.time == t1.time && t2.millitm == t1.millitm);
            sleepms_old(10L * SLEEPINIT); // about 1/4 sec
            ftimex(&t2);
            if (driver_key_pressed())
            {
                scalems = 0L;
                cleartempmsg();
                goto sleepexit;
            }
        }
        while ((elapsed = (int)(t2.time - t1.time)*1000 + t2.millitm - t1.millitm) < SLEEPINIT);
        // once more to see if faster (eg multi-tasking)
        do
        {
            // wait for the start of a new tick
            ftimex(&t1);
        }
        while (t2.time == t1.time && t2.millitm == t1.millitm);
        sleepms_old(10L * SLEEPINIT);
        ftimex(&t2);
        i = (int)(t2.time-t1.time)*1000 + t2.millitm-t1.millitm;
        if (i < elapsed)
        {
            elapsed = (i == 0) ? 1 : i;
        }
        scalems = (long)((float)SLEEPINIT/(float)(elapsed) * scalems);
        cleartempmsg();
    }
    if (ms > 10L * SLEEPINIT)
    {
        // using ftime is probably more accurate
        ms /= 10;
        ftimex(&t1);
        while (1)
        {
            if (driver_key_pressed())
            {
                break;
            }
            ftimex(&t2);
            if ((long)((t2.time-t1.time)*1000 + t2.millitm-t1.millitm) >= ms)
            {
                break;
            }
        }
    }
    else if (!driver_key_pressed())
    {
        ms *= scalems;
        while (ms-- >= 0)
        {
        }
    }
sleepexit:
    g_tab_mode = save_tab_mode;
    g_help_mode = old_help_mode;
}

static void sleepms_new(long ms)
{
    uclock_t next_time;
    uclock_t now = usec_clock();
    next_time = now + ms*100;
    while ((now = usec_clock()) < next_time)
    {
        if (driver_key_pressed())
        {
            break;
        }
    }
}

void sleepms(long ms)
{
    if (g_debug_flag == debug_flags::force_old_sleep)
    {
        sleepms_old(ms);
    }
    else
    {
        sleepms_new(ms);
    }
}

/*
 * wait until wait_time microseconds from the
 * last call has elapsed.
 */
#define MAX_INDEX 2
static uclock_t next_time[MAX_INDEX];
void wait_until(int index, uclock_t wait_time)
{
    if (g_debug_flag == debug_flags::force_old_sleep)
    {
        sleepms_old(wait_time);
    }
    else
    {
        uclock_t now;
        while ((now = usec_clock()) < next_time[index])
        {
            if (driver_key_pressed())
            {
                break;
            }
        }
        next_time[index] = now + wait_time*100; // wait until this time next call
    }
}

void reset_clock()
{
    restart_uclock();
    for (auto &elem : next_time)
    {
        elem = 0;
    }
}

#define LOG2  0.693147180F
#define LOG32 3.465735902F

static FILE *snd_fp = nullptr;

// open sound file
bool snd_open()
{
    static char soundname[] = {"sound001.txt"};
    if ((g_orbit_save_flags & osf_midi) != 0 && snd_fp == nullptr)
    {
        snd_fp = fopen(soundname, "w");
        if (snd_fp == nullptr)
        {
            stopmsg(STOPMSG_NONE, "Can't open sound*.txt");
        }
        else
        {
            updatesavename(soundname);
        }
    }
    return (snd_fp != nullptr);
}

/* This routine plays a tone in the speaker and optionally writes a file
   if the orbitsave variable is turned on */
void w_snd(int tone)
{
    if ((g_orbit_save_flags & osf_midi) != 0)
    {
        // cppcheck-suppress leakNoVarFunctionCall
        if (snd_open())
        {
            fprintf(snd_fp, "%-d\n", tone);
        }
    }
    g_tab_or_help = false;
    if (!driver_key_pressed())
    {
        // driver_key_pressed calls driver_sound_off() if TAB or F1 pressed
        // must not then call driver_sound_off(), else indexes out of synch
        //   if (20 < tone && tone < 15000)  better limits?
        //   if (10 < tone && tone < 5000)  better limits?
        if (driver_sound_on(tone))
        {
            wait_until(0, g_orbit_delay);
            if (!g_tab_or_help)   // kludge because wait_until() calls driver_key_pressed
            {
                driver_sound_off();
            }
        }
    }
}

void snd_time_write()
{
    // cppcheck-suppress leakNoVarFunctionCall
    if (snd_open())
    {
        fprintf(snd_fp, "time=%-ld\n", (long)clock()*1000/CLOCKS_PER_SEC);
    }
}

void close_snd()
{
    if (snd_fp)
    {
        fclose(snd_fp);
    }
    snd_fp = nullptr;
}

static void plotdorbit(double dx, double dy, int color)
{
    int i;
    int j;
    int save_sxoffs, save_syoffs;
    if (g_orbit_save_index >= NUM_SAVE_ORBIT-3)
    {
        return;
    }
    i = (int)(dy * g_plot_mx1 - dx * g_plot_mx2);
    i += sxoffs;
    if (i < 0 || i >= sxdots)
    {
        return;
    }
    j = (int)(dx * g_plot_my1 - dy * g_plot_my2);
    j += syoffs;
    if (j < 0 || j >= sydots)
    {
        return;
    }
    save_sxoffs = sxoffs;
    save_syoffs = syoffs;
    syoffs = 0;
    sxoffs = syoffs;
    // save orbit value
    if (color == -1)
    {
        save_orbit[g_orbit_save_index++] = i;
        save_orbit[g_orbit_save_index++] = j;
        int const c = getcolor(i, j);
        save_orbit[g_orbit_save_index++] = c;
        g_put_color(i, j, c^g_orbit_color);
    }
    else
    {
        g_put_color(i, j, color);
    }
    sxoffs = save_sxoffs;
    syoffs = save_syoffs;
    if (g_debug_flag == debug_flags::force_scaled_sound_formula)
    {
        if ((g_sound_flag & SOUNDFLAG_ORBITMASK) == SOUNDFLAG_X)   // sound = x
        {
            w_snd((int)(i*1000/xdots+g_base_hertz));
        }
        else if ((g_sound_flag & SOUNDFLAG_ORBITMASK) > SOUNDFLAG_X)     // sound = y or z
        {
            w_snd((int)(j*1000/ydots+g_base_hertz));
        }
        else if (g_orbit_delay > 0)
        {
            wait_until(0, g_orbit_delay);
        }
    }
    else
    {
        if ((g_sound_flag & SOUNDFLAG_ORBITMASK) == SOUNDFLAG_X)   // sound = x
        {
            w_snd((int)(i+g_base_hertz));
        }
        else if ((g_sound_flag & SOUNDFLAG_ORBITMASK) == SOUNDFLAG_Y)     // sound = y
        {
            w_snd((int)(j+g_base_hertz));
        }
        else if ((g_sound_flag & SOUNDFLAG_ORBITMASK) == SOUNDFLAG_Z)     // sound = z
        {
            w_snd((int)(i+j+g_base_hertz));
        }
        else if (g_orbit_delay > 0)
        {
            wait_until(0, g_orbit_delay);
        }
    }

    // placing sleepms here delays each dot
}

void iplot_orbit(long ix, long iy, int color)
{
    plotdorbit((double)ix/g_fudge_factor-xxmin, (double)iy/g_fudge_factor-yymax, color);
}

void plot_orbit(double real, double imag, int color)
{
    plotdorbit(real-xxmin, imag-yymax, color);
}

void scrub_orbit()
{
    int i, j, c;
    int save_sxoffs, save_syoffs;
    driver_mute();
    save_sxoffs = sxoffs;
    save_syoffs = syoffs;
    syoffs = 0;
    sxoffs = syoffs;
    while (g_orbit_save_index >= 3)
    {
        c = save_orbit[--g_orbit_save_index];
        j = save_orbit[--g_orbit_save_index];
        i = save_orbit[--g_orbit_save_index];
        g_put_color(i, j, c);
    }
    sxoffs = save_sxoffs;
    syoffs = save_syoffs;
}


int add_worklist(int xfrom, int xto, int xbegin,
                 int yfrom, int yto, int ybegin,
                 int pass, int sym)
{
    if (g_num_work_list >= MAXCALCWORK)
    {
        return (-1);
    }
    g_work_list[g_num_work_list].xxstart = xfrom;
    g_work_list[g_num_work_list].xxstop  = xto;
    g_work_list[g_num_work_list].xxbegin = xbegin;
    g_work_list[g_num_work_list].yystart = yfrom;
    g_work_list[g_num_work_list].yystop  = yto;
    g_work_list[g_num_work_list].yybegin = ybegin;
    g_work_list[g_num_work_list].pass    = pass;
    g_work_list[g_num_work_list].sym     = sym;
    ++g_num_work_list;
    tidy_worklist();
    return (0);
}

static int combine_worklist() // look for 2 entries which can freely merge
{
    for (int i = 0; i < g_num_work_list; ++i)
    {
        if (g_work_list[i].yystart == g_work_list[i].yybegin)
        {
            for (int j = i+1; j < g_num_work_list; ++j)
            {
                if (g_work_list[j].sym == g_work_list[i].sym
                        && g_work_list[j].yystart == g_work_list[j].yybegin
                        && g_work_list[j].xxstart == g_work_list[j].xxbegin
                        && g_work_list[i].pass == g_work_list[j].pass)
                {
                    if (g_work_list[i].xxstart == g_work_list[j].xxstart
                            && g_work_list[i].xxbegin == g_work_list[j].xxbegin
                            && g_work_list[i].xxstop  == g_work_list[j].xxstop)
                    {
                        if (g_work_list[i].yystop+1 == g_work_list[j].yystart)
                        {
                            g_work_list[i].yystop = g_work_list[j].yystop;
                            return (j);
                        }
                        if (g_work_list[j].yystop+1 == g_work_list[i].yystart)
                        {
                            g_work_list[i].yystart = g_work_list[j].yystart;
                            g_work_list[i].yybegin = g_work_list[j].yybegin;
                            return (j);
                        }
                    }
                    if (g_work_list[i].yystart == g_work_list[j].yystart
                            && g_work_list[i].yybegin == g_work_list[j].yybegin
                            && g_work_list[i].yystop  == g_work_list[j].yystop)
                    {
                        if (g_work_list[i].xxstop+1 == g_work_list[j].xxstart)
                        {
                            g_work_list[i].xxstop = g_work_list[j].xxstop;
                            return (j);
                        }
                        if (g_work_list[j].xxstop+1 == g_work_list[i].xxstart)
                        {
                            g_work_list[i].xxstart = g_work_list[j].xxstart;
                            g_work_list[i].xxbegin = g_work_list[j].xxbegin;
                            return (j);
                        }
                    }
                }
            }
        }
    }
    return (0); // nothing combined
}

// combine mergeable entries, resort
void tidy_worklist()
{
    {
        int i;
        while ((i = combine_worklist()) != 0)
        {
            // merged two, delete the gone one
            while (++i < g_num_work_list)
            {
                g_work_list[i-1] = g_work_list[i];
            }
            --g_num_work_list;
        }
    }
    for (int i = 0; i < g_num_work_list; ++i)
    {
        for (int j = i+1; j < g_num_work_list; ++j)
        {
            if (g_work_list[j].pass < g_work_list[i].pass
                    || (g_work_list[j].pass == g_work_list[i].pass
                        && (g_work_list[j].yystart < g_work_list[i].yystart
                            || (g_work_list[j].yystart == g_work_list[i].yystart
                                && g_work_list[j].xxstart <  g_work_list[i].xxstart))))
            {
                // dumb sort, swap 2 entries to correct order
                WORKLIST tempwork = g_work_list[i];
                g_work_list[i] = g_work_list[j];
                g_work_list[j] = tempwork;
            }
        }
    }
}


void get_julia_attractor(double real, double imag)
{
    LComplex lresult = { 0 };
    DComplex result = { 0.0 };
    int savper;
    long savmaxit;

    if (g_attractors == 0 && !g_finite_attractor)   // not magnet & not requested
    {
        return;
    }

    if (g_attractors >= MAX_NUM_ATTRACTORS)       // space for more attractors ?
    {
        return;                  // Bad luck - no room left !
    }

    savper = g_periodicity_check;
    savmaxit = g_max_iterations;
    g_periodicity_check = 0;
    g_old_z.x = real;                    // prepare for f.p orbit calc
    g_old_z.y = imag;
    tempsqrx = sqr(g_old_z.x);
    tempsqry = sqr(g_old_z.y);

    g_l_old_z.x = (long)real;     // prepare for int orbit calc
    g_l_old_z.y = (long)imag;
    g_l_temp_sqr_x = (long)tempsqrx;
    g_l_temp_sqr_y = (long)tempsqry;

    g_l_old_z.x = g_l_old_z.x << g_bit_shift;
    g_l_old_z.y = g_l_old_z.y << g_bit_shift;
    g_l_temp_sqr_x = g_l_temp_sqr_x << g_bit_shift;
    g_l_temp_sqr_y = g_l_temp_sqr_y << g_bit_shift;

    if (g_max_iterations < 500)           // we're going to try at least this hard
    {
        g_max_iterations = 500;
    }
    g_color_iter = 0;
    g_overflow = false;
    while (++g_color_iter < g_max_iterations)
    {
        if (curfractalspecific->orbitcalc() || g_overflow)
        {
            break;
        }
    }
    if (g_color_iter >= g_max_iterations)      // if orbit stays in the lake
    {
        if (g_integer_fractal)     // remember where it went to
        {
            lresult = g_l_new_z;
        }
        else
        {
            result =  g_new_z;
        }
        for (int i = 0; i < 10; i++)
        {
            g_overflow = false;
            if (!curfractalspecific->orbitcalc() && !g_overflow) // if it stays in the lake
            {
                // and doesn't move far, probably
                if (g_integer_fractal)   //   found a finite attractor
                {
                    if (labs(lresult.x-g_l_new_z.x) < g_l_close_enough
                            && labs(lresult.y-g_l_new_z.y) < g_l_close_enough)
                    {
                        g_l_attractor[g_attractors] = g_l_new_z;
                        g_attractor_period[g_attractors] = i+1;
                        g_attractors++;   // another attractor - coloured lakes !
                        break;
                    }
                }
                else
                {
                    if (fabs(result.x-g_new_z.x) < g_close_enough
                            && fabs(result.y-g_new_z.y) < g_close_enough)
                    {
                        g_attractor[g_attractors] = g_new_z;
                        g_attractor_period[g_attractors] = i+1;
                        g_attractors++;   // another attractor - coloured lakes !
                        break;
                    }
                }
            }
            else
            {
                break;
            }
        }
    }
    if (g_attractors == 0)
    {
        g_periodicity_check = savper;
    }
    g_max_iterations = savmaxit;
}


#define maxyblk 7    // must match calcfrac.c
#define maxxblk 202  // must match calcfrac.c
int ssg_blocksize() // used by solidguessing and by zoom panning
{
    int blocksize, i;
    // blocksize 4 if <300 rows, 8 if 300-599, 16 if 600-1199, 32 if >=1200
    blocksize = 4;
    i = 300;
    while (i <= ydots)
    {
        blocksize += blocksize;
        i += i;
    }
    // increase blocksize if prefix array not big enough
    while (blocksize*(maxxblk-2) < xdots || blocksize*(maxyblk-2)*16 < ydots)
    {
        blocksize += blocksize;
    }
    return (blocksize);
}
