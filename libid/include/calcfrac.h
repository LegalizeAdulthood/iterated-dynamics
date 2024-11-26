// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "big.h"
#include "cmplx.h"

enum class fractal_type;

constexpr double AUTO_INVERT = -123456.789;

enum
{
    MAX_NUM_ATTRACTORS = 8
};

enum class symmetry_type
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
    EPSCROSS = -100,
    STARTRAIL = -101,
    PERIOD = -102,
    FMODI = -103,
    ATANI = -104
};

enum class status_values
{
    NONE = -1,
    ONE_OR_TWO_PASS = 0,
    SOLID_GUESS = 1,
    BOUNDARY_TRACE = 2,
    THREE_D = 3,
    TESSERAL = 4,
    DIFFUSION = 5,
    ORBITS =6,
};

extern int                   g_and_color;           // AND mask for iteration to get color index
extern  long                 g_l_at_rad;            // finite attractor radius
extern  double               g_f_at_rad;            // finite attractor radius
extern int                   g_atan_colors;
extern DComplex              g_attractor[];
extern int                   g_attractor_period[];
extern int                   g_attractors;
extern int                 (*g_calc_type)();
extern double                g_close_enough;
extern double                g_close_proximity;
extern int                   g_col;
extern int                   g_color;
extern long                  g_color_iter;
extern int                   g_current_column;
extern int                   g_current_pass;
extern int                   g_current_row;
extern double                g_f_radius;
extern double                g_f_x_center;
extern double                g_f_y_center;
extern long                  g_first_saved_and;
extern status_values         g_got_status;
extern int                   g_i_x_start;
extern int                   g_i_x_stop;
extern int                   g_i_y_start;
extern int                   g_i_y_stop;
extern DComplex              g_init;
extern int                   g_invert;
extern int                   g_keyboard_check_interval;
extern LComplex              g_l_attractor[];
extern long                  g_l_close_enough;
extern LComplex              g_l_init_orbit;
extern long                  g_l_init_x;
extern long                  g_l_init_y;
extern long                  g_l_magnitude;
extern long                  g_l_magnitude_limit;
extern long                  g_l_magnitude_limit2;
extern double                g_magnitude;
extern bool                  g_magnitude_calc;
extern double                g_magnitude_limit;
extern double                g_magnitude_limit2;
extern int                   g_max_keyboard_check_interval;
extern DComplex              g_new_z;
extern long                  g_old_color_iter;
extern bool                  g_old_demm_colors;
extern DComplex              g_old_z;
extern int                   g_orbit_color;
extern int                   g_orbit_save_index;
extern int                   g_periodicity_check;
extern int                   g_periodicity_next_saved_incr;
extern int                   g_pi_in_pixels;
extern void                (*g_plot)(int x, int y, int color);
extern void                (*g_put_color)(int x, int y, int color);
extern bool                  g_quick_calc;
extern long                  g_real_color_iter;
extern bool                  g_reset_periodicity;
extern int                   g_row;
extern bool                  g_show_orbit;
extern symmetry_type         g_force_symmetry;
extern symmetry_type         g_symmetry;
extern bool                  g_three_pass;
extern int                   g_total_passes;
extern DComplex              g_tmp_z;
extern bool                  g_use_old_periodicity;
extern bool                  g_use_old_distance_estimator;
extern int                   g_work_pass;
extern int                   g_work_symmetry;
extern int                   g_xx_begin;
extern int                   g_xx_start;
extern int                   g_xx_stop;
extern int                   g_yy_begin;
extern int                   g_yy_start;
extern int                   g_yy_stop;

extern int                   g_inside_color;
extern int                   g_outside_color;

int calc_fract();
int calc_mand();
int calc_mand_fp();
int standard_fractal();
int find_alternate_math(fractal_type type, bf_math_type math);
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
void sym_fill_line(int row, int left, int right, BYTE *str);
