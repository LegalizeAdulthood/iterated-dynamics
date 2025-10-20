// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "math/cmplx.h"
#include "math/big.h"
#include "math/Point.h"

#include <config/port.h>

namespace id::fractals
{
enum class FractalType;
}

namespace id::engine
{

constexpr double AUTO_INVERT = -123456.789;
constexpr int MAX_PARAMS{10};        // maximum number of parameters

enum
{
    MAX_NUM_ATTRACTORS = 8
};

enum class SymmetryType
{
    NONE = 0,              //  0: no symmetry
    X_AXIS_NO_PARAM = -1,  // -1: x-axis symmetry (if no parameters)
    X_AXIS = 1,            //  1: x-axis symmetry
    Y_AXIS_NO_PARAM = -2,  // -2: y-axis symmetry (if no parameters)
    Y_AXIS = 2,            //  2: y-axis symmetry
    XY_AXIS_NO_PARAM = -3, // -3: y-axis AND x-axis (if no parameters)
    XY_AXIS = 3,           //  3: y-axis AND x-axis symmetry
    ORIGIN_NO_PARAM = -4,  // -4: polar symmetry (if no parameters)
    ORIGIN = 4,            //  4: polar symmetry
    PI_SYM_NO_PARAM = -5,  // -5: PI (sin/cos) symmetry (if no parameters)
    PI_SYM = 5,            //  5: PI (sin/cos) symmetry
    X_AXIS_NO_IMAG = -6,   //
    X_AXIS_NO_REAL = 6,    //
    NO_PLOT = 99,          //
    SETUP = 100,           //
    NOT_FORCED = 999       //
};

// values for inside/outside
enum
{
    COLOR_BLACK = 0,
    ITER = -1,
    REAL = -2,
    IMAG = -3,
    MULT = -4,
    SUM = -5,
    ATAN = -6,
    FMOD = -7,
    TDIS = -8,
    ZMAG = -59,
    BOF60 = -60,
    BOF61 = -61,
    EPS_CROSS = -100,
    STAR_TRAIL = -101,
    PERIOD = -102,
    FMODI = -103,
    ATANI = -104
};

enum class Passes
{
    NONE = -1,
    SEQUENTIAL_SCAN = 0,
    SOLID_GUESS = 1,
    BOUNDARY_TRACE = 2,
    THREE_D = 3,
    TESSERAL = 4,
    DIFFUSION = 5,
    ORBITS = 6,
};

enum class CalcMode
{
    NONE = 0,
    ONE_PASS = '1',
    TWO_PASS = '2',
    THREE_PASS = '3',
    SOLID_GUESS = 'g',
    BOUNDARY_TRACE = 'b',
    TESSERAL = 't',
    SYNCHRONOUS_ORBIT = 's',
    DIFFUSION = 'd',
    ORBIT = 'o',
    PERTURBATION = 'p',
};

// -1 no fractal
//  0 params changed, recalculation required
//  1 actively calculating
//  2 interrupted, resumable
//  3 interrupted, not resumable
//  4 completed
enum class CalcStatus
{
    NO_FRACTAL = -1,
    PARAMS_CHANGED = 0,
    IN_PROGRESS = 1,
    RESUMABLE = 2,
    NON_RESUMABLE = 3,
    COMPLETED = 4
};

extern int                   g_and_color;           // AND mask for iteration to get color index
extern  double               g_f_at_rad;            // finite attractor radius
extern int                   g_atan_colors;
extern math::DComplex        g_attractor[];
extern int                   g_attractor_period[];
extern int                   g_attractors;
extern CalcStatus            g_calc_status;
extern long                  g_calc_time;
extern int                 (*g_calc_type)();
extern double                g_close_enough;
extern double                g_close_proximity;
extern int                   g_col;
extern int                   g_color;
extern long                  g_color_iter;
extern int                   g_current_column;
extern int                   g_current_pass;
extern int                   g_current_row;
extern long                  g_first_saved_and;
extern math::DComplex        g_init;
extern int                   g_keyboard_check_interval;
extern double                g_magnitude;
extern bool                  g_magnitude_calc;
extern double                g_magnitude_limit;
extern double                g_magnitude_limit2;
extern long                  g_max_iterations;      // try this many iterations
extern int                   g_max_keyboard_check_interval;
extern math::DComplex        g_new_z;
extern long                  g_old_color_iter;
extern bool                  g_old_demm_colors;
extern math::DComplex        g_old_z;

extern math::DComplex        g_init_orbit;          // initial orbit value
extern int                   g_orbit_color;         // XOR color
extern int                   g_orbit_save_index;    // index into save_orbit array
extern bool                  g_show_orbit;          // flag to turn on and off
extern bool                  g_start_show_orbit;    // show orbits on at start of fractal

extern double                g_params[MAX_PARAMS];
extern Passes                g_passes;
extern int                   g_periodicity_check;
extern int                   g_periodicity_next_saved_incr;
extern int                   g_pi_in_pixels;
extern void                (*g_plot)(int x, int y, int color);
extern void                (*g_put_color)(int x, int y, int color);
extern bool                  g_quick_calc;
extern long                  g_real_color_iter;
extern bool                  g_reset_periodicity;
extern int                   g_row;
extern CalcMode              g_old_std_calc_mode;
extern CalcMode              g_std_calc_mode;
extern SymmetryType          g_force_symmetry;
extern SymmetryType          g_symmetry;
extern bool                  g_three_pass;
extern int                   g_total_passes;
extern math::DComplex        g_tmp_z;
extern bool                  g_use_old_periodicity;
extern bool                  g_use_old_distance_estimator;

extern math::Point2i         g_start_pt; // current work list entry being computed
extern math::Point2i         g_i_start_pt;
extern math::Point2i         g_stop_pt;
extern math::Point2i         g_i_stop_pt;
extern math::Point2i         g_begin_pt;
extern int                   g_work_pass;
extern int                   g_work_symmetry;

extern int                   g_inside_color;
extern int                   g_outside_color;

int calc_fract();
int calc_mandelbrot_type();
int standard_fractal_type();
int find_alternate_math(fractals::FractalType type, math::BFMathType math);
int potential(double mag, long iterations);
void sym_pi_plot(int x, int y, int color);
void sym_pi_plot2j(int x, int y, int color);
void sym_pi_plot4j(int x, int y, int color);
void sym_plot2(int x, int y, int color);
void sym_plot2y(int x, int y, int color);
void sym_plot2j(int x, int y, int color);
void sym_plot4(int x, int y, int color);
void sym_plot2_basin(int x, int y, int color);
void sym_plot4_basin(int x, int y, int color);
void no_plot(int x, int y, int color);
void sym_fill_line(int row, int left, int right, const Byte *str);

} // namespace id::engine
