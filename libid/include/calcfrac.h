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
    NONE                = 0,
    X_AXIS_NO_PARAM     = -1,
    X_AXIS              = 1,
    Y_AXIS_NO_PARAM     = -2,
    Y_AXIS              = 2,
    XY_AXIS_NO_PARAM    = -3,
    XY_AXIS             = 3,
    ORIGIN_NO_PARAM     = -4,
    ORIGIN              = 4,
    PI_SYM_NO_PARAM     = -5,
    PI_SYM              = 5,
    X_AXIS_NO_IMAG      = -6,
    X_AXIS_NO_REAL      = 6,
    NO_PLOT             = 99,
    SETUP               = 100,
    NOT_FORCED          = 999
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
extern unsigned int          g_diffusion_bits;
extern unsigned long         g_diffusion_counter;
extern unsigned long         g_diffusion_limit;
extern char                  g_draw_mode;
extern double                g_f_radius;
extern double                g_f_x_center;
extern double                g_f_y_center;
extern long                  g_first_saved_and;
extern int                   g_got_status;
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
extern int                   g_xx_start;
extern int                   g_xx_stop;
extern int                   g_yy_start;
extern int                   g_yy_stop;

int calcfract();
int calcmand();
int calcmandfp();
int standard_fractal();
int find_alternate_math(fractal_type type, bf_math_type math);
void symPIplot(int x, int y, int color);
void symPIplot2J(int x, int y, int color);
void symPIplot4J(int x, int y, int color);
void symplot2(int x, int y, int color);
void symplot2Y(int x, int y, int color);
void symplot2J(int x, int y, int color);
void symplot4(int x, int y, int color);
void symplot2basin(int x, int y, int color);
void symplot4basin(int x, int y, int color);
void noplot(int x, int y, int color);
