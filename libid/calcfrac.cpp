// SPDX-License-Identifier: GPL-3.0-only
//
// Contains the high level ("engine") code for calculating the
// fractal images (well, SOMEBODY had to do it!).
// The following modules work very closely with:
//   - the fractal-specific code for escape-time fractals.
//   - assorted subroutines belonging mainly to calcfrac.
//   - fast Mandelbrot/Julia integer implementation
// Additional fractal-specific modules are also invoked from calcfrac:
//   - engine level and fractal specific code for attractors.
//   - julibrot logic
//   - formula fractals
//   and more
//
#include "calcfrac.h"

#include "port.h"
#include "bailout_formula.h"
#include "biginit.h"
#include "boundary_trace.h"
#include "calcmand.h"
#include "calmanfp.h"
#include "check_key.h"
#include "check_write_file.h"
#include "cmdfiles.h"
#include "cmplx.h"
#include "debug_flags.h"
#include "diffusion_scan.h"
#include "diskvid.h"
#include "drivers.h"
#include "engine_timer.h"
#include "find_special_colors.h"
#include "fractalb.h"
#include "fractalp.h"
#include "fractals.h"
#include "fractype.h"
#include "frothy_basin.h"
#include "id.h"
#include "id_data.h"
#include "line3d.h"
#include "miscfrac.h"
#include "mpmath_c.h"
#include "newton.h"
#include "one_or_two_pass.h"
#include "orbit.h"
#include "parser.h"
#include "pixel_grid.h"
#include "resume.h"
#include "sign.h"
#include "soi.h"
#include "solid_guess.h"
#include "sound.h"
#include "spindac.h"
#include "sticky_orbits.h"
#include "stop_msg.h"
#include "tesseral.h"
#include "update_save_name.h"
#include "video.h"
#include "wait_until.h"
#include "work_list.h"

#include <algorithm>
#include <cfloat>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

// routines in this module
static void perform_worklist();
static int  potential(double, long);
static void decomposition();
static void setsymmetry(symmetry_type sym, bool uselist);
static bool xsym_split(int xaxis_row, bool xaxis_between);
static bool ysym_split(int yaxis_col, bool yaxis_between);
static void put_truecolor_disk(int, int, int);

// added for testing autologmap()
static long autologmap();

static DComplex s_saved{};     //
static double rqlim_save{};    //
static int (*calctypetmp)(){}; //
static unsigned long lm{};     // magnitude limit (CALCMAND)
int g_xx_begin{};              // these are same as worklist,
int g_yy_begin{};              // declared as separate items
static double dem_delta{};     //
static double dem_width{};     // distance estimator variables
static double dem_toobig{};    //
static bool dem_mandel{};      //

#define DEM_BAILOUT 535.5
// next has a skip bit for each maxblock unit;
//   1st pass sets bit  [1]... off only if block's contents guessed;
//   at end of 1st pass [0]... bits are set if any surrounding block not guessed;
//   bits are numbered [..][y/16+1][x+1]&(1<<(y&15))
// size of next puts a limit of MAX_PIXELS pixels across on solid guessing logic

// variables exported from this file
long g_l_at_rad{};                              // finite attractor radius
double g_f_at_rad{};                            // finite attractor radius
LComplex g_l_init_orbit{};                      //
long g_l_magnitude{};                           //
long g_l_magnitude_limit{};                     //
long g_l_magnitude_limit2{};                    //
long g_l_close_enough{};                        //
long g_l_init_x{};                              //
long g_l_init_y{};                              //
DComplex g_init{};                              //
DComplex g_tmp_z{};                             //
DComplex g_old_z{};                             //
DComplex g_new_z{};                             //
int g_color{};                                  //
long g_color_iter{};                            //
long g_old_color_iter{};                        //
long g_real_color_iter{};                       //
int g_row{};                                    //
int g_col{};                                    //
int g_invert{};                                 //
double g_f_radius{};                            //
double g_f_x_center{};                          //
double g_f_y_center{};                          // for inversion
void (*g_put_color)(int, int, int){putcolor_a}; //
void (*g_plot)(int, int, int){putcolor_a};      //
double g_magnitude{};                           //
double g_magnitude_limit{};                     //
double g_magnitude_limit2{};                    //
bool g_magnitude_calc{true};                    //
bool g_use_old_periodicity{};                   //
bool g_use_old_distance_estimator{};            //
bool g_old_demm_colors{};                       //
int (*g_calc_type)(){};                         //
bool g_quick_calc{};                            //
double g_close_proximity{0.01};                 //
double g_close_enough{};                        //
int g_pi_in_pixels{};                           // value of pi in pixels
                                                // ORBIT variables
bool g_show_orbit{};                            // flag to turn on and off
int g_orbit_save_index{};                       // index into save_orbit array
int g_orbit_color{15};                          // XOR color
int g_i_x_start{};                              //
int g_i_x_stop{};                               //
int g_i_y_start{};                              //
int g_i_y_stop{};                               // start, stop here
symmetry_type g_symmetry{};                     // symmetry flag
symmetry_type g_force_symmetry{};               // force symmetry
bool g_reset_periodicity{};                     // true if escape time pixel rtn to reset
int g_keyboard_check_interval{};                //
int g_max_keyboard_check_interval{};            // avoids checking keyboard too often
int g_xx_start{};                               //
int g_xx_stop{};                                //
int g_yy_start{};                               //
int g_yy_stop{};                                //
int g_work_pass{};                              //
int g_work_symmetry{};                          // for the sake of calcmand
status_values g_got_status{status_values::NONE}; // variables which must be visible for tab_display
int g_current_pass{};                            //
int g_total_passes{};                            //
int g_current_row{};                             //
int g_current_column{};                          //
bool g_three_pass{};                             // for solid_guess & its subroutines
int g_attractors{};                              // number of finite attractors
DComplex g_attractor[MAX_NUM_ATTRACTORS]{};      // finite attractor vals (f.p)
LComplex g_l_attractor[MAX_NUM_ATTRACTORS]{};    // finite attractor vals (int)
int g_attractor_period[MAX_NUM_ATTRACTORS]{};    // period of the finite attractor
int g_inside_color{};                            // inside color: 1=blue
int g_outside_color{};                           // outside color

// --------------------------------------------------------------------
//              These variables are external for speed's sake only
// --------------------------------------------------------------------

int g_periodicity_check{};           //
int g_periodicity_next_saved_incr{}; // For periodicity testing, only in standard_fractal()
long g_first_saved_and{};            //
int g_atan_colors{180};              //

static std::vector<BYTE> s_save_dots;
static BYTE *s_fill_buff{};
static int s_save_dots_len{};
static int s_show_dot_color{};
static int s_show_dot_width{};

enum class show_dot_action
{
    SAVE = 1,
    RESTORE = 2
};

enum class show_dot_direction
{
    JUST_A_POINT = 0,
    LOWER_RIGHT = 1,
    UPPER_RIGHT = 2,
    LOWER_LEFT = 3,
    UPPER_LEFT = 4
};

int g_and_color {};        // "and" value used for color selection

double fmodtest_bailout_or()
{
    double const tmpx = sqr(g_new_z.x);
    double const tmpy = sqr(g_new_z.y);
    if (tmpx > tmpy)
    {
        return tmpx;
    }

    return tmpy;
}

// FMODTEST routine.
// Makes the test condition for the FMOD coloring type
//   that of the current bailout method. 'or' and 'and'
//   methods are not used - in these cases a normal
//   modulus test is used
//
double fmodtest()
{
    double result;

    switch (g_bail_out_test)
    {
    case bailouts::Mod:
        if (g_magnitude == 0.0 || g_magnitude_calc || g_integer_fractal)
        {
            result = sqr(g_new_z.x)+sqr(g_new_z.y);
        }
        else
        {
            result = g_magnitude; // don't recalculate
        }
        break;

    case bailouts::Real:
        result = sqr(g_new_z.x);
        break;

    case bailouts::Imag:
        result = sqr(g_new_z.y);
        break;

    case bailouts::Or:
        result = fmodtest_bailout_or();
        break;

    case bailouts::Manh:
        result = sqr(std::fabs(g_new_z.x) + std::fabs(g_new_z.y));
        break;

    case bailouts::Manr:
        result = sqr(g_new_z.x+g_new_z.y);
        break;

    default:
        result = sqr(g_new_z.x)+sqr(g_new_z.y);
        break;
    }

    return result;
}

// The sym_fill_line() routine was pulled out of the boundary tracing
// code for re-use with show dot. It's purpose is to fill a line with a
// solid color. This assumes that BYTE *str is already filled
// with the color. The routine does write the line using symmetry
// in all cases, however the symmetry logic assumes that the line
// is one color; it is not general enough to handle a row of
// pixels of different colors.
void sym_fill_line(int row, int left, int right, BYTE *str)
{
    int length;
    length = right-left+1;
    write_span(row, left, right, str);
    // here's where all the symmetry goes
    if (g_plot == g_put_color)
    {
        g_keyboard_check_interval -= length >> 4; // seems like a reasonable value
    }
    else if (g_plot == symplot2)   // X-axis symmetry
    {
        int i = g_yy_stop-(row-g_yy_start);
        if (i > g_i_y_stop && i < g_logical_screen_y_dots)
        {
            write_span(i, left, right, str);
            g_keyboard_check_interval -= length >> 3;
        }
    }
    else if (g_plot == symplot2Y) // Y-axis symmetry
    {
        write_span(row, g_xx_stop-(right-g_xx_start), g_xx_stop-(left-g_xx_start), str);
        g_keyboard_check_interval -= length >> 3;
    }
    else if (g_plot == symplot2J)  // Origin symmetry
    {
        int i = g_yy_stop-(row-g_yy_start);
        int j = std::min(g_xx_stop-(right-g_xx_start), g_logical_screen_x_dots-1);
        int k = std::min(g_xx_stop-(left -g_xx_start), g_logical_screen_x_dots-1);
        if (i > g_i_y_stop && i < g_logical_screen_y_dots && j <= k)
        {
            write_span(i, j, k, str);
        }
        g_keyboard_check_interval -= length >> 3;
    }
    else if (g_plot == symplot4) // X-axis and Y-axis symmetry
    {
        int i = g_yy_stop-(row-g_yy_start);
        int j = std::min(g_xx_stop-(right-g_xx_start), g_logical_screen_x_dots-1);
        int k = std::min(g_xx_stop-(left -g_xx_start), g_logical_screen_x_dots-1);
        if (i > g_i_y_stop && i < g_logical_screen_y_dots)
        {
            write_span(i, left, right, str);
            if (j <= k)
            {
                write_span(i, j, k, str);
            }
        }
        if (j <= k)
        {
            write_span(row, j, k, str);
        }
        g_keyboard_check_interval -= length >> 2;
    }
    else    // cheap and easy way out
    {
        for (int i = left; i <= right; i++)
        {
            (*g_plot)(i, row, str[i-left]);
        }
        g_keyboard_check_interval -= length >> 1;
    }
}

// The sym_put_line() routine is the symmetry-aware version of put_line().
// It only works efficiently in the no symmetry or X_AXIS symmetry case,
// otherwise it just writes the pixels one-by-one.
static void sym_put_line(int row, int left, int right, BYTE *str)
{
    int length = right-left+1;
    write_span(row, left, right, str);
    if (g_plot == g_put_color)
    {
        g_keyboard_check_interval -= length >> 4; // seems like a reasonable value
    }
    else if (g_plot == symplot2)   // X-axis symmetry
    {
        int i = g_yy_stop-(row-g_yy_start);
        if (i > g_i_y_stop && i < g_logical_screen_y_dots)
        {
            write_span(i, left, right, str);
        }
        g_keyboard_check_interval -= length >> 3;
    }
    else
    {
        for (int i = left; i <= right; i++)
        {
            (*g_plot)(i, row, str[i-left]);
        }
        g_keyboard_check_interval -= length >> 1;
    }
}

void showdotsaverestore(
    int startx,
    int stopx,
    int starty,
    int stopy,
    show_dot_direction direction,
    show_dot_action action)
{
    int ct = 0;
    if (direction != show_dot_direction::JUST_A_POINT)
    {
        if (s_save_dots.empty())
        {
            stopmsg("savedots empty");
            exit(0);
        }
        if (s_fill_buff == nullptr)
        {
            stopmsg("fillbuff NULL");
            exit(0);
        }
    }
    switch (direction)
    {
    case show_dot_direction::LOWER_RIGHT:
        for (int j = starty; j <= stopy; startx++, j++)
        {
            if (action == show_dot_action::SAVE)
            {
                read_span(j, startx, stopx, &s_save_dots[0] + ct);
                sym_fill_line(j, startx, stopx, s_fill_buff);
            }
            else
            {
                sym_put_line(j, startx, stopx, &s_save_dots[0] + ct);
            }
            ct += stopx-startx+1;
        }
        break;
    case show_dot_direction::UPPER_RIGHT:
        for (int j = starty; j >= stopy; startx++, j--)
        {
            if (action == show_dot_action::SAVE)
            {
                read_span(j, startx, stopx, &s_save_dots[0] + ct);
                sym_fill_line(j, startx, stopx, s_fill_buff);
            }
            else
            {
                sym_put_line(j, startx, stopx, &s_save_dots[0] + ct);
            }
            ct += stopx-startx+1;
        }
        break;
    case show_dot_direction::LOWER_LEFT:
        for (int j = starty; j <= stopy; stopx--, j++)
        {
            if (action == show_dot_action::SAVE)
            {
                read_span(j, startx, stopx, &s_save_dots[0] + ct);
                sym_fill_line(j, startx, stopx, s_fill_buff);
            }
            else
            {
                sym_put_line(j, startx, stopx, &s_save_dots[0] + ct);
            }
            ct += stopx-startx+1;
        }
        break;
    case show_dot_direction::UPPER_LEFT:
        for (int j = starty; j >= stopy; stopx--, j--)
        {
            if (action == show_dot_action::SAVE)
            {
                read_span(j, startx, stopx, &s_save_dots[0] + ct);
                sym_fill_line(j, startx, stopx, s_fill_buff);
            }
            else
            {
                sym_put_line(j, startx, stopx, &s_save_dots[0] + ct);
            }
            ct += stopx-startx+1;
        }
        break;
    case show_dot_direction::JUST_A_POINT:
        break;
    }
    if (action == show_dot_action::SAVE)
    {
        (*g_plot)(g_col, g_row, s_show_dot_color);
    }
}

int calctypeshowdot()
{
    int out;
    int startx;
    int starty;
    int stopx;
    int stopy;
    int width;
    show_dot_direction direction = show_dot_direction::JUST_A_POINT;
    stopx = g_col;
    startx = g_col;
    stopy = g_row;
    starty = g_row;
    width = s_show_dot_width+1;
    if (width > 0)
    {
        if (g_col+width <= g_i_x_stop && g_row+width <= g_i_y_stop)
        {
            // preferred show dot shape
            direction = show_dot_direction::UPPER_LEFT;
            startx = g_col;
            stopx  = g_col+width;
            starty = g_row+width;
            stopy  = g_row+1;
        }
        else if (g_col-width >= g_i_x_start && g_row+width <= g_i_y_stop)
        {
            // second choice
            direction = show_dot_direction::UPPER_RIGHT;
            startx = g_col-width;
            stopx  = g_col;
            starty = g_row+width;
            stopy  = g_row+1;
        }
        else if (g_col-width >= g_i_x_start && g_row-width >= g_i_y_start)
        {
            direction = show_dot_direction::LOWER_RIGHT;
            startx = g_col-width;
            stopx  = g_col;
            starty = g_row-width;
            stopy  = g_row-1;
        }
        else if (g_col+width <= g_i_x_stop && g_row-width >= g_i_y_start)
        {
            direction = show_dot_direction::LOWER_LEFT;
            startx = g_col;
            stopx  = g_col+width;
            starty = g_row-width;
            stopy  = g_row-1;
        }
    }
    showdotsaverestore(startx, stopx, starty, stopy, direction, show_dot_action::SAVE);
    if (g_orbit_delay > 0)
    {
        sleepms(g_orbit_delay);
    }
    out = (*calctypetmp)();
    showdotsaverestore(startx, stopx, starty, stopy, direction, show_dot_action::RESTORE);
    return out;
}

static void fix_inversion(double *x) // make double converted from string look ok
{
    char buf[30];
    std::sprintf(buf, "%-1.15lg", *x);
    *x = std::atof(buf);
}

// calcfract - the top level routine for generating an image
int calcfract()
{
    g_attractors = 0;          // default to no known finite attractors
    g_display_3d = display_3d_modes::NONE;
    g_basin = 0;
    g_put_color = putcolor_a;
    if (g_is_true_color && g_true_mode != true_color_mode::default_color)
    {
        // Have to force passes = 1
        g_std_calc_mode = '1';
        g_user_std_calc_mode = g_std_calc_mode;
    }
    if (g_truecolor)
    {
        check_writefile(g_light_name, ".tga");
        if (!startdisk1(g_light_name, nullptr, false))
        {
            // Have to force passes = 1
            g_std_calc_mode = '1';
            g_user_std_calc_mode = g_std_calc_mode;
            g_put_color = put_truecolor_disk;
        }
        else
        {
            g_truecolor = false;
        }
    }
    if (!g_use_grid)
    {
        if (g_user_std_calc_mode != 'o')
        {
            g_std_calc_mode = '1';
            g_user_std_calc_mode = '1';
        }
    }

    init_misc();  // set up some variables in parser.c
    reset_clock();

    // following delta values useful only for types with rotation disabled
    // currently used only by bifurcation
    if (g_integer_fractal)
    {
        g_distance_estimator = 0;
    }
    g_param_z1.x   = g_params[0];
    g_param_z1.y   = g_params[1];
    g_param_z2.x  = g_params[2];
    g_param_z2.y  = g_params[3];

    if (g_log_map_flag && g_colors < 16)
    {
        stopmsg("Need at least 16 colors to use logmap");
        g_log_map_flag = 0;
    }

    if (g_use_old_periodicity)
    {
        g_periodicity_next_saved_incr = 1;
        g_first_saved_and = 1;
    }
    else
    {
        g_periodicity_next_saved_incr = (int)std::log10(static_cast<double>(g_max_iterations)); // works better than log()
        if (g_periodicity_next_saved_incr < 4)
        {
            g_periodicity_next_saved_incr = 4; // maintains image with low iterations
        }
        g_first_saved_and = (long)((g_periodicity_next_saved_incr*2) + 1);
    }

    g_log_map_table.clear();
    g_log_map_table_max_size = g_max_iterations;
    g_log_map_calculate = false;
    // below, 32767 is used as the allowed value for maximum iteration count for
    // historical reasons.  TODO: increase this limit
    if (g_log_map_flag
        && ((g_max_iterations > 32767) || g_log_map_fly_calculate == 1))
    {
        g_log_map_calculate = true; // calculate on the fly
        SetupLogTable();
    }
    else if (g_log_map_flag && (g_log_map_fly_calculate == 2))
    {
        g_log_map_table_max_size = 32767;
        g_log_map_calculate = false; // use logtable
    }
    else if (g_iteration_ranges_len && (g_max_iterations >= 32767))
    {
        g_log_map_table_max_size = 32766;
    }

    if ((g_log_map_flag || g_iteration_ranges_len) && !g_log_map_calculate)
    {
        bool resized = false;
        try
        {
            g_log_map_table.resize(g_log_map_table_max_size + 1);
            resized = true;
        }
        catch (std::bad_alloc const&)
        {
        }

        if (!resized)
        {
            if (g_iteration_ranges_len || g_log_map_fly_calculate == 2)
            {
                stopmsg("Insufficient memory for logmap/ranges with this maxiter");
            }
            else
            {
                stopmsg("Insufficient memory for logTable, using on-the-fly routine");
                g_log_map_fly_calculate = 1;
                g_log_map_calculate = true; // calculate on the fly
                SetupLogTable();
            }
        }
        else if (g_iteration_ranges_len)
        {
            // Can't do ranges if MaxLTSize > 32767
            int m;
            int numval;
            int flip;
            int altern;
            int l = 0;
            int k = 0;
            int i = 0;
            g_log_map_flag = 0; // ranges overrides logmap
            while (i < g_iteration_ranges_len)
            {
                flip = 0;
                m = 0;
                altern = 32767;
                numval = g_iteration_ranges[i++];
                if (numval < 0)
                {
                    altern = g_iteration_ranges[i++];    // sub-range iterations
                    numval = g_iteration_ranges[i++];
                }
                if (numval > (int)g_log_map_table_max_size || i >= g_iteration_ranges_len)
                {
                    numval = (int)g_log_map_table_max_size;
                }
                while (l <= numval)
                {
                    g_log_map_table[l++] = (BYTE)(k + flip);
                    if (++m >= altern)
                    {
                        flip ^= 1;            // Alternate colors
                        m = 0;
                    }
                }
                ++k;
                if (altern != 32767)
                {
                    ++k;
                }
            }
        }
        else
        {
            SetupLogTable();
        }
    }
    lm = 4L << g_bit_shift;                 // CALCMAND magnitude limit

    g_atan_colors = g_colors;

    // ORBIT stuff
    g_show_orbit = g_start_show_orbit;
    g_orbit_save_index = 0;
    g_orbit_color = 15;
    if (g_colors < 16)
    {
        g_orbit_color = 1;
    }

    if (g_inversion[0] != 0.0)
    {
        g_f_radius    = g_inversion[0];
        g_f_x_center   = g_inversion[1];
        g_f_y_center   = g_inversion[2];

        if (g_inversion[0] == AUTO_INVERT)  //  auto calc radius 1/6 screen
        {
            g_inversion[0] = std::min(std::fabs(g_x_max - g_x_min),
                                    std::fabs(g_y_max - g_y_min)) / 6.0;
            fix_inversion(&g_inversion[0]);
            g_f_radius = g_inversion[0];
        }

        if (g_invert < 2 || g_inversion[1] == AUTO_INVERT)  // xcenter not already set
        {
            g_inversion[1] = (g_x_min + g_x_max) / 2.0;
            fix_inversion(&g_inversion[1]);
            g_f_x_center = g_inversion[1];
            if (std::fabs(g_f_x_center) < std::fabs(g_x_max-g_x_min) / 100)
            {
                g_f_x_center = 0.0;
                g_inversion[1] = 0.0;
            }
        }

        if (g_invert < 3 || g_inversion[2] == AUTO_INVERT)  // ycenter not already set
        {
            g_inversion[2] = (g_y_min + g_y_max) / 2.0;
            fix_inversion(&g_inversion[2]);
            g_f_y_center = g_inversion[2];
            if (std::fabs(g_f_y_center) < std::fabs(g_y_max-g_y_min) / 100)
            {
                g_f_y_center = 0.0;
                g_inversion[2] = 0.0;
            }
        }

        g_invert = 3; // so values will not be changed if we come back
    }

    g_close_enough = g_delta_min*std::pow(2.0, -(double)(std::abs(g_periodicity_check)));
    rqlim_save = g_magnitude_limit;
    g_magnitude_limit2 = std::sqrt(g_magnitude_limit);
    if (g_integer_fractal)          // for integer routines (lambda)
    {
        g_l_param.x = (long)(g_param_z1.x * g_fudge_factor);    // real portion of Lambda
        g_l_param.y = (long)(g_param_z1.y * g_fudge_factor);    // imaginary portion of Lambda
        g_l_param2.x = (long)(g_param_z2.x * g_fudge_factor);  // real portion of Lambda2
        g_l_param2.y = (long)(g_param_z2.y * g_fudge_factor);  // imaginary portion of Lambda2
        g_l_magnitude_limit = (long)(g_magnitude_limit * g_fudge_factor);      // stop if magnitude exceeds this
        if (g_l_magnitude_limit <= 0)
        {
            g_l_magnitude_limit = 0x7fffffffL; // klooge for integer math
        }
        g_l_magnitude_limit2 = (long)(g_magnitude_limit2 * g_fudge_factor);    // stop if magnitude exceeds this
        g_l_close_enough = (long)(g_close_enough * g_fudge_factor); // "close enough" value
        g_l_init_orbit.x = (long)(g_init_orbit.x * g_fudge_factor);
        g_l_init_orbit.y = (long)(g_init_orbit.y * g_fudge_factor);
    }
    g_resuming = (g_calc_status == calc_status_value::RESUMABLE);
    if (!g_resuming) // free resume_info memory if any is hanging around
    {
        end_resume();
        if (g_resave_flag)
        {
            update_save_name(g_save_filename); // do the pending increment
            g_resave_flag = 0;
            g_started_resaves = false;
        }
        g_calc_time = 0;
    }

    if (g_cur_fractal_specific->calctype != standard_fractal
        && g_cur_fractal_specific->calctype != calcmand
        && g_cur_fractal_specific->calctype != calcmandfp
        && g_cur_fractal_specific->calctype != lyapunov
        && g_cur_fractal_specific->calctype != calcfroth)
    {
        g_calc_type = g_cur_fractal_specific->calctype; // per_image can override
        g_symmetry = g_cur_fractal_specific->symmetry; //   calctype & symmetry
        g_plot = g_put_color; // defaults when setsymmetry not called or does nothing
        g_xx_begin = 0;
        g_yy_begin = 0;
        g_xx_start = 0;
        g_yy_start = 0;
        g_i_x_start = 0;
        g_i_y_start = 0;
        g_yy_stop = g_logical_screen_y_dots-1;
        g_i_y_stop = g_logical_screen_y_dots-1;
        g_xx_stop = g_logical_screen_x_dots-1;
        g_i_x_stop = g_logical_screen_x_dots-1;
        g_calc_status = calc_status_value::IN_PROGRESS; // mark as in-progress
        g_distance_estimator = 0; // only standard escape time engine supports distest
        // per_image routine is run here
        if (g_cur_fractal_specific->per_image())
        {
            // not a stand-alone
            // next two lines in case periodicity changed
            g_close_enough = g_delta_min*std::pow(2.0, -(double)(std::abs(g_periodicity_check)));
            g_l_close_enough = (long)(g_close_enough * g_fudge_factor); // "close enough" value
            setsymmetry(g_symmetry, false);
            timer(timer_type::ENGINE, g_calc_type); // non-standard fractal engine
        }
        if (check_key())
        {
            if (g_calc_status == calc_status_value::IN_PROGRESS)   // calctype didn't set this itself,
            {
                g_calc_status = calc_status_value::NON_RESUMABLE;   // so mark it interrupted, non-resumable
            }
        }
        else
        {
            g_calc_status = calc_status_value::COMPLETED; // no key, so assume it completed
        }
    }
    else // standard escape-time engine
    {
        if (g_std_calc_mode == '3')  // convoluted 'g' + '2' hybrid
        {
            const int old_calc_mode = g_std_calc_mode;
            if (!g_resuming || g_three_pass)
            {
                g_std_calc_mode = 'g';
                g_three_pass = true;
                timer(timer_type::ENGINE, (int(*)())perform_worklist);
                if (g_calc_status == calc_status_value::COMPLETED)
                {
                    if (g_logical_screen_x_dots >= 640)    // '2' is silly after 'g' for low rez
                    {
                        g_std_calc_mode = '2';
                    }
                    else
                    {
                        g_std_calc_mode = '1';
                    }
                    timer(timer_type::ENGINE, (int(*)())perform_worklist);
                    g_three_pass = false;
                }
            }
            else // resuming '2' pass
            {
                if (g_logical_screen_x_dots >= 640)
                {
                    g_std_calc_mode = '2';
                }
                else
                {
                    g_std_calc_mode = '1';
                }
                timer(timer_type::ENGINE, (int(*)())perform_worklist);
            }
            g_std_calc_mode = (char)old_calc_mode;
        }
        else // main case, much nicer!
        {
            g_three_pass = false;
            timer(timer_type::ENGINE, (int(*)())perform_worklist);
        }
    }
    g_calc_time += g_timer_interval;

    if (!g_log_map_table.empty() && !g_log_map_calculate)
    {
        g_log_map_table.clear();
    }
    free_workarea();

    if (g_cur_fractal_specific->calctype == calcfroth)
    {
        froth_cleanup();
    }
    if ((g_sound_flag & SOUNDFLAG_ORBITMASK) > SOUNDFLAG_BEEP)   // close sound write file
    {
        close_snd();
    }
    if (g_truecolor)
    {
        enddisk();
    }
    return g_calc_status == calc_status_value::COMPLETED ? 0 : -1;
}

// locate alternate math record
int find_alternate_math(fractal_type type, bf_math_type math)
{
    if (math == bf_math_type::NONE)
    {
        return -1;
    }
    int i = -1;
    fractal_type curtype;
    while ((curtype = g_alternate_math[++i].type) != type && curtype != fractal_type::NOFRACTAL)
    {
    }
    int ret = -1;
    if (curtype == type && g_alternate_math[i].math != bf_math_type::NONE)
    {
        ret = i;
    }
    return ret;
}

// general escape-time engine routines
static void perform_worklist()
{
    int (*sv_orbitcalc)() = nullptr;  // function that calculates one orbit
    int (*sv_per_pixel)() = nullptr;  // once-per-pixel init
    bool (*sv_per_image)() = nullptr;  // once-per-image setup
    int alt = find_alternate_math(g_fractal_type, g_bf_math);

    if (alt > -1)
    {
        sv_orbitcalc = g_cur_fractal_specific->orbitcalc;
        sv_per_pixel = g_cur_fractal_specific->per_pixel;
        sv_per_image = g_cur_fractal_specific->per_image;
        g_cur_fractal_specific->orbitcalc = g_alternate_math[alt].orbitcalc;
        g_cur_fractal_specific->per_pixel = g_alternate_math[alt].per_pixel;
        g_cur_fractal_specific->per_image = g_alternate_math[alt].per_image;
    }
    else
    {
        g_bf_math = bf_math_type::NONE;
    }

    if (g_potential_flag && g_potential_16bit)
    {
        int tmpcalcmode = g_std_calc_mode;

        g_std_calc_mode = '1'; // force 1 pass
        if (!g_resuming)
        {
            if (pot_startdisk() < 0)
            {
                g_potential_16bit = false;       // startdisk failed or cancelled
                g_std_calc_mode = (char)tmpcalcmode;    // maybe we can carry on???
            }
        }
    }
    if (g_std_calc_mode == 'b' && bit_set(g_cur_fractal_specific->flags, fractal_flags::NOTRACE))
    {
        g_std_calc_mode = '1';
    }
    if (g_std_calc_mode == 'g' && bit_set(g_cur_fractal_specific->flags, fractal_flags::NOGUESS))
    {
        g_std_calc_mode = '1';
    }
    if (g_std_calc_mode == 'o' && (g_cur_fractal_specific->calctype != standard_fractal))
    {
        g_std_calc_mode = '1';
    }

    // default setup a new worklist
    g_num_work_list = 1;
    g_work_list[0].xxbegin = 0;
    g_work_list[0].xxstart = 0;
    g_work_list[0].yybegin = 0;
    g_work_list[0].yystart = 0;
    g_work_list[0].xxstop = g_logical_screen_x_dots - 1;
    g_work_list[0].yystop = g_logical_screen_y_dots - 1;
    g_work_list[0].sym = 0;
    g_work_list[0].pass = 0;
    if (g_resuming) // restore worklist, if we can't the above will stay in place
    {
        int vsn;
        vsn = start_resume();
        get_resume(sizeof(g_num_work_list), &g_num_work_list, sizeof(g_work_list), g_work_list, 0);
        end_resume();
        if (vsn < 2)
        {
            g_xx_begin = 0;
        }
    }

    if (g_distance_estimator) // setup stuff for distance estimator
    {
        double ftemp;
        double ftemp2;
        double delxx;
        double delyy2;
        double delyy;
        double delxx2;
        double d_x_size;
        double d_y_size;
        double aspect;
        if (g_distance_estimator_x_dots && g_distance_estimator_y_dots)
        {
            aspect = (double)g_distance_estimator_y_dots/(double)g_distance_estimator_x_dots;
            d_x_size = g_distance_estimator_x_dots-1;
            d_y_size = g_distance_estimator_y_dots-1;
        }
        else
        {
            aspect = (double)g_logical_screen_y_dots/(double)g_logical_screen_x_dots;
            d_x_size = g_logical_screen_x_dots-1;
            d_y_size = g_logical_screen_y_dots-1;
        }

        delxx  = (g_x_max - g_x_3rd) / d_x_size; // calculate stepsizes
        delyy  = (g_y_max - g_y_3rd) / d_y_size;
        delxx2 = (g_x_3rd - g_x_min) / d_y_size;
        delyy2 = (g_y_3rd - g_y_min) / d_x_size;

        g_use_old_distance_estimator = false;
        g_magnitude_limit = rqlim_save; // just in case changed to DEM_BAILOUT earlier
        if (g_distance_estimator != 1 || g_colors == 2)   // not doing regular outside colors
        {
            if (g_magnitude_limit < DEM_BAILOUT)           // so go straight for dem bailout
            {
                g_magnitude_limit = DEM_BAILOUT;
            }
        }
        // must be mandel type, formula, or old PAR/GIF
        dem_mandel = g_cur_fractal_specific->tojulia != fractal_type::NOFRACTAL
            || g_use_old_distance_estimator
            || g_fractal_type == fractal_type::FORMULA
            || g_fractal_type == fractal_type::FFORMULA;
        dem_delta = sqr(delxx) + sqr(delyy2);
        ftemp = sqr(delyy) + sqr(delxx2);
        if (ftemp > dem_delta)
        {
            dem_delta = ftemp;
        }
        if (g_distance_estimator_width_factor == 0)
        {
            g_distance_estimator_width_factor = 1;
        }
        ftemp = g_distance_estimator_width_factor;
        if (g_distance_estimator_width_factor > 0)
        {
            dem_delta *= sqr(ftemp)/10000; // multiply by thickness desired
        }
        else
        {
            dem_delta *= 1/(sqr(ftemp)*10000); // multiply by thickness desired
        }
        dem_width = (std::sqrt(sqr(g_x_max-g_x_min) + sqr(g_x_3rd-g_x_min)) * aspect
                     + std::sqrt(sqr(g_y_max-g_y_min) + sqr(g_y_3rd-g_y_min))) / g_distance_estimator;
        ftemp = (g_magnitude_limit < DEM_BAILOUT) ? DEM_BAILOUT : g_magnitude_limit;
        ftemp += 3; // bailout plus just a bit
        ftemp2 = std::log(ftemp);
        if (g_use_old_distance_estimator)
        {
            dem_toobig = sqr(ftemp) * sqr(ftemp2) * 4 / dem_delta;
        }
        else
        {
            dem_toobig = std::fabs(ftemp) * std::fabs(ftemp2) * 2 / std::sqrt(dem_delta);
        }
    }

    while (g_num_work_list > 0)
    {
        // per_image can override
        g_calc_type = g_cur_fractal_specific->calctype;
        g_symmetry = g_cur_fractal_specific->symmetry; //   calctype & symmetry
        g_plot = g_put_color; // defaults when setsymmetry not called or does nothing

        // pull top entry off worklist
        g_xx_start = g_work_list[0].xxstart;
        g_i_x_start = g_work_list[0].xxstart;
        g_xx_stop  = g_work_list[0].xxstop;
        g_i_x_stop  = g_work_list[0].xxstop;
        g_xx_begin  = g_work_list[0].xxbegin;
        g_yy_start = g_work_list[0].yystart;
        g_i_y_start = g_work_list[0].yystart;
        g_yy_stop  = g_work_list[0].yystop;
        g_i_y_stop  = g_work_list[0].yystop;
        g_yy_begin  = g_work_list[0].yybegin;
        g_work_pass = g_work_list[0].pass;
        g_work_symmetry  = g_work_list[0].sym;
        --g_num_work_list;
        for (int i = 0; i < g_num_work_list; ++i)
        {
            g_work_list[i] = g_work_list[i+1];
        }

        g_calc_status = calc_status_value::IN_PROGRESS; // mark as in-progress

        g_cur_fractal_specific->per_image();
        if (g_show_dot >= 0)
        {
            find_special_colors();
            switch (g_auto_show_dot)
            {
            case 'd':
                s_show_dot_color = g_color_dark % g_colors;
                break;
            case 'm':
                s_show_dot_color = g_color_medium % g_colors;
                break;
            case 'b':
            case 'a':
                s_show_dot_color = g_color_bright % g_colors;
                break;
            default:
                s_show_dot_color = g_show_dot % g_colors;
                break;
            }
            if (g_size_dot <= 0)
            {
                s_show_dot_width = -1;
            }
            else
            {
                double dshowdot_width;
                dshowdot_width = (double)g_size_dot*g_logical_screen_x_dots/1024.0;

                // Arbitrary sanity limit, however showdot_width will
                // overflow if dshowdot width gets near 256.
                if (dshowdot_width > 150.0)
                {
                    s_show_dot_width = 150;
                }
                else if (dshowdot_width > 0.0)
                {
                    s_show_dot_width = (int)dshowdot_width;
                }
                else
                {
                    s_show_dot_width = -1;
                }
            }
            while (s_show_dot_width >= 0)
            {
                // We're using near memory, so get the amount down
                // to something reasonable. The polynomial used to
                // calculate savedotslen is exactly right for the
                // triangular-shaped shotdot cursor. The that cursor
                // is changed, this formula must match.
                while ((s_save_dots_len = sqr(s_show_dot_width) + 5*s_show_dot_width + 4) > 1000)
                {
                    s_show_dot_width--;
                }
                bool resized = false;
                try
                {
                    s_save_dots.resize(s_save_dots_len);
                    resized = true;
                }
                catch (std::bad_alloc const&)
                {
                }

                if (resized)
                {
                    s_save_dots_len /= 2;
                    s_fill_buff = &s_save_dots[0] + s_save_dots_len;
                    std::memset(s_fill_buff, s_show_dot_color, s_save_dots_len);
                    break;
                }
                // There's even less free memory than we thought, so reduce
                // showdot_width still more
                s_show_dot_width--;
            }
            if (s_save_dots.empty())
            {
                s_show_dot_width = -1;
            }
            calctypetmp = g_calc_type;
            g_calc_type    = calctypeshowdot;
        }

        // some common initialization for escape-time pixel level routines
        g_close_enough = g_delta_min*std::pow(2.0, -(double)(std::abs(g_periodicity_check)));
        g_l_close_enough = (long)(g_close_enough * g_fudge_factor); // "close enough" value
        g_keyboard_check_interval = g_max_keyboard_check_interval;

        setsymmetry(g_symmetry, true);

        if (!g_resuming && (labs(g_log_map_flag) == 2 || (g_log_map_flag && g_log_map_auto_calculate)))
        {
            // calculate round screen edges to work out best start for logmap
            g_log_map_flag = (autologmap() * (g_log_map_flag / labs(g_log_map_flag)));
            SetupLogTable();
        }

        // call the appropriate escape-time engine
        switch (g_std_calc_mode)
        {
        case 's':
            soi();
            break;
        case 't':
            tesseral();
            break;
        case 'b':
            boundary_trace();
            break;
        case 'g':
            solid_guess();
            break;
        case 'd':
            diffusion_scan();
            break;
        case 'o':
            sticky_orbits();
            break;
        default:
            one_or_two_pass();
        }
        if (!s_save_dots.empty())
        {
            s_save_dots.clear();
            s_fill_buff = nullptr;
        }
        if (check_key())   // interrupted?
        {
            break;
        }
    }

    if (g_num_work_list > 0)
    {
        // interrupted, resumable
        alloc_resume(sizeof(g_work_list)+20, 2);
        put_resume(sizeof(g_num_work_list), &g_num_work_list, sizeof(g_work_list), g_work_list, 0);
    }
    else
    {
        g_calc_status = calc_status_value::COMPLETED; // completed
    }
    if (sv_orbitcalc != nullptr)
    {
        g_cur_fractal_specific->orbitcalc = sv_orbitcalc;
        g_cur_fractal_specific->per_pixel = sv_per_pixel;
        g_cur_fractal_specific->per_image = sv_per_image;
    }
}

int calcmand()              // fast per pixel 1/2/b/g, called with row & col set
{
    // setup values from array to avoid using es reg in calcmand.asm
    g_l_init_x = g_l_x_pixel();
    g_l_init_y = g_l_y_pixel();
    if (calcmandasm() >= 0)
    {
        if ((!g_log_map_table.empty() || g_log_map_calculate) // map color, but not if maxit & adjusted for inside,etc
            && (g_real_color_iter < g_max_iterations
                || (g_inside_color < COLOR_BLACK && g_color_iter == g_max_iterations)))
        {
            g_color_iter = logtablecalc(g_color_iter);
        }
        g_color = std::abs((int)g_color_iter);
        if (g_color_iter >= g_colors)
        {
            // don't use color 0 unless from inside/outside
            if (g_colors < 16)
            {
                g_color = (int)(g_color_iter & g_and_color);
            }
            else
            {
                g_color = (int)(((g_color_iter - 1) % g_and_color) + 1);
            }
        }
        if (g_debug_flag != debug_flags::force_boundary_trace_error)
        {
            if (g_color <= 0 && g_std_calc_mode == 'b')
            {
                g_color = 1;
            }
        }
        (*g_plot)(g_col, g_row, g_color);
    }
    else
    {
        g_color = (int)g_color_iter;
    }
    return g_color;
}

// sort of a floating point version of calcmand()
// can also handle invert, any rqlim, potflag, zmag, epsilon cross,
// and all the current outside options
int calcmandfp()
{
    if (g_invert != 0)
    {
        invertz2(&g_init);
    }
    else
    {
        g_init.x = g_dx_pixel();
        g_init.y = g_dy_pixel();
    }
    if (calcmandfpasm() >= 0)
    {
        if (g_potential_flag)
        {
            g_color_iter = potential(g_magnitude, g_real_color_iter);
        }
        if ((!g_log_map_table.empty() || g_log_map_calculate) // map color, but not if maxit & adjusted for inside,etc
            && (g_real_color_iter < g_max_iterations
                || (g_inside_color < COLOR_BLACK && g_color_iter == g_max_iterations)))
        {
            g_color_iter = logtablecalc(g_color_iter);
        }
        g_color = std::abs((int)g_color_iter);
        if (g_color_iter >= g_colors)
        {
            // don't use color 0 unless from inside/outside
            if (g_colors < 16)
            {
                g_color = (int)(g_color_iter & g_and_color);
            }
            else
            {
                g_color = (int)(((g_color_iter - 1) % g_and_color) + 1);
            }
        }
        if (g_debug_flag != debug_flags::force_boundary_trace_error)
        {
            if (g_color == 0 && g_std_calc_mode == 'b')
            {
                g_color = 1;
            }
        }
        (*g_plot)(g_col, g_row, g_color);
    }
    else
    {
        g_color = (int)g_color_iter;
    }
    return g_color;
}
#define STARTRAILMAX FLT_MAX   // just a convenient large number

int standard_fractal()       // per pixel 1/2/b/g, called with row & col set
{
    int const green = 2;
    int const yellow = 6;

    long savemaxit = 0;
    double tantable[16] = { 0.0 };
    int hooper = 0;
    long lcloseprox = 0;
    double memvalue = 0.0;
    double min_orbit = 100000.0;        // orbit value closest to origin
    long   min_index = 0;               // iteration of min_orbit
    long cyclelen = -1;
    long savedcoloriter = 0;
    bool caught_a_cycle = false;
    long savedand = 0;
    int savedincr = 0;                  // for periodicity checking
    LComplex lsaved = { 0 };
    bool attracted = false;
    LComplex lat = { 0 };
    DComplex  at = { 0.0 };
    DComplex deriv = { 0.0 };
    long dem_color = -1;
    DComplex dem_new = { 0 };
    int check_freq = 0;
    double totaldist = 0.0;
    DComplex lastz = { 0.0 };

    lcloseprox = (long)(g_close_proximity*g_fudge_factor);
    savemaxit = g_max_iterations;
    if (g_inside_color == STARTRAIL)
    {
        std::fill(std::begin(tantable), std::end(tantable), 0.0);
        g_max_iterations = 16;
    }
    if (g_periodicity_check == 0 || g_inside_color == ZMAG || g_inside_color == STARTRAIL)
    {
        g_old_color_iter = 2147483647L;       // don't check periodicity at all
    }
    else if (g_inside_color == PERIOD)       // for display-periodicity
    {
        g_old_color_iter = (g_max_iterations/5)*4;       // don't check until nearly done
    }
    else if (g_reset_periodicity)
    {
        g_old_color_iter = 255;               // don't check periodicity 1st 250 iterations
    }

    // Jonathan - how about this idea ? skips first saved value which never works
#ifdef MINSAVEDAND
    if (oldcoloriter < MINSAVEDAND)
    {
        oldcoloriter = MINSAVEDAND;
    }
#else
    if (g_old_color_iter < g_first_saved_and)   // I like it!
    {
        g_old_color_iter = g_first_saved_and;
    }
#endif
    // really fractal specific, but we'll leave it here
    if (!g_integer_fractal)
    {
        if (g_use_init_orbit == init_orbit_mode::value)
        {
            s_saved = g_init_orbit;
        }
        else
        {
            s_saved.x = 0;
            s_saved.y = 0;
        }
        if (g_bf_math != bf_math_type::NONE)
        {
            if (g_decimals > 200)
            {
                g_keyboard_check_interval = -1;
            }
            if (g_bf_math == bf_math_type::BIGNUM)
            {
                clear_bn(g_saved_z_bn.x);
                clear_bn(g_saved_z_bn.y);
            }
            else if (g_bf_math == bf_math_type::BIGFLT)
            {
                clear_bf(g_saved_z_bf.x);
                clear_bf(g_saved_z_bf.y);
            }
        }
        g_init.y = g_dy_pixel();
        if (g_distance_estimator)
        {
            if (g_use_old_distance_estimator)
            {
                g_magnitude_limit = rqlim_save;
                if (g_distance_estimator != 1 || g_colors == 2)   // not doing regular outside colors
                {
                    if (g_magnitude_limit < DEM_BAILOUT)     // so go straight for dem bailout
                    {
                        g_magnitude_limit = DEM_BAILOUT;
                    }
                }
                dem_color = -1;
            }
            deriv.x = 1;
            deriv.y = 0;
            g_magnitude = 0;
        }
    }
    else
    {
        if (g_use_init_orbit == init_orbit_mode::value)
        {
            lsaved = g_l_init_orbit;
        }
        else
        {
            lsaved.x = 0;
            lsaved.y = 0;
        }
        g_l_init.y = g_l_y_pixel();
    }
    g_orbit_save_index = 0;
    g_color_iter = 0;
    if (g_fractal_type == fractal_type::JULIAFP || g_fractal_type == fractal_type::JULIA)
    {
        g_color_iter = -1;
    }
    caught_a_cycle = false;
    if (g_inside_color == PERIOD)
    {
        savedand = 16;           // begin checking every 16th cycle
    }
    else
    {
#ifdef MINSAVEDAND
        savedand = MINSAVEDAND;
#else
        savedand = g_first_saved_and;                // begin checking every other cycle
#endif
    }
    savedincr = 1;               // start checking the very first time

    if (g_inside_color <= BOF60 && g_inside_color >= BOF61)
    {
        g_l_magnitude = 0;
        g_magnitude = 0;
        min_orbit = 100000.0;
    }
    g_overflow = false;           // reset integer math overflow flag

    g_cur_fractal_specific->per_pixel(); // initialize the calculations

    attracted = false;

    if (g_outside_color == TDIS)
    {
        if (g_integer_fractal)
        {
            g_old_z.x = ((double)g_l_old_z.x) / g_fudge_factor;
            g_old_z.y = ((double)g_l_old_z.y) / g_fudge_factor;
        }
        else if (g_bf_math == bf_math_type::BIGNUM)
        {
            g_old_z = cmplxbntofloat(&g_old_z_bn);
        }
        else if (g_bf_math == bf_math_type::BIGFLT)
        {
            g_old_z = cmplxbftofloat(&g_old_z_bf);
        }
        lastz.x = g_old_z.x;
        lastz.y = g_old_z.y;
    }

    if (((g_sound_flag & SOUNDFLAG_ORBITMASK) > SOUNDFLAG_X || g_show_dot >= 0) && g_orbit_delay > 0)
    {
        check_freq = 16;
    }
    else
    {
        check_freq = 2048;
    }

    if (g_show_orbit)
    {
        snd_time_write();
    }
    while (++g_color_iter < g_max_iterations)
    {
        // calculation of one orbit goes here
        // input in "old" -- output in "new"
        if (g_color_iter % check_freq == 0)
        {
            if (check_key())
            {
                return -1;
            }
        }

        if (g_distance_estimator)
        {
            double ftemp;
            // Distance estimator for points near Mandelbrot set
            // Algorithms from Peitgen & Saupe, Science of Fractal Images, p.198
            if (dem_mandel)
            {
                ftemp = 2 * (g_old_z.x * deriv.x - g_old_z.y * deriv.y) + 1;
            }
            else
            {
                ftemp = 2 * (g_old_z.x * deriv.x - g_old_z.y * deriv.y);
            }
            deriv.y = 2 * (g_old_z.y * deriv.x + g_old_z.x * deriv.y);
            deriv.x = ftemp;
            if (g_use_old_distance_estimator)
            {
                if (sqr(deriv.x)+sqr(deriv.y) > dem_toobig)
                {
                    break;
                }
            }
            else
            {
                if (std::max(std::fabs(deriv.x), std::fabs(deriv.y)) > dem_toobig)
                {
                    break;
                }
            }
            // if above exit taken, the later test vs dem_delta will place this
            // point on the boundary, because mag(old)<bailout just now

            if (g_cur_fractal_specific->orbitcalc() || g_overflow)
            {
                if (g_use_old_distance_estimator)
                {
                    if (dem_color < 0)
                    {
                        dem_color = g_color_iter;
                        dem_new = g_new_z;
                    }
                    if (g_magnitude_limit >= DEM_BAILOUT
                        || g_magnitude >= (g_magnitude_limit = DEM_BAILOUT)
                        || g_magnitude == 0)
                    {
                        break;
                    }
                }
                else
                {
                    break;
                }
            }
            g_old_z = g_new_z;
        }

        // the usual case
        else if ((g_cur_fractal_specific->orbitcalc() && g_inside_color != STARTRAIL) || g_overflow)
        {
            break;
        }
        if (g_show_orbit)
        {
            if (!g_integer_fractal)
            {
                if (g_bf_math == bf_math_type::BIGNUM)
                {
                    g_new_z = cmplxbntofloat(&g_new_z_bn);
                }
                else if (g_bf_math == bf_math_type::BIGFLT)
                {
                    g_new_z = cmplxbftofloat(&g_new_z_bf);
                }
                plot_orbit(g_new_z.x, g_new_z.y, -1);
            }
            else
            {
                iplot_orbit(g_l_new_z.x, g_l_new_z.y, -1);
            }
        }
        if (g_inside_color < ITER)
        {
            if (g_bf_math == bf_math_type::BIGNUM)
            {
                g_new_z = cmplxbntofloat(&g_new_z_bn);
            }
            else if (g_bf_math == bf_math_type::BIGFLT)
            {
                g_new_z = cmplxbftofloat(&g_new_z_bf);
            }
            if (g_inside_color == STARTRAIL)
            {
                if (0 < g_color_iter && g_color_iter < 16)
                {
                    if (g_integer_fractal)
                    {
                        g_new_z.x = g_l_new_z.x;
                        g_new_z.x /= g_fudge_factor;
                        g_new_z.y = g_l_new_z.y;
                        g_new_z.y /= g_fudge_factor;
                    }

                    if (g_new_z.x > STARTRAILMAX)
                    {
                        g_new_z.x = STARTRAILMAX;
                    }
                    if (g_new_z.x < -STARTRAILMAX)
                    {
                        g_new_z.x = -STARTRAILMAX;
                    }
                    if (g_new_z.y > STARTRAILMAX)
                    {
                        g_new_z.y = STARTRAILMAX;
                    }
                    if (g_new_z.y < -STARTRAILMAX)
                    {
                        g_new_z.y = -STARTRAILMAX;
                    }
                    g_temp_sqr_x = g_new_z.x * g_new_z.x;
                    g_temp_sqr_y = g_new_z.y * g_new_z.y;
                    g_magnitude = g_temp_sqr_x + g_temp_sqr_y;
                    g_old_z = g_new_z;
                    {
                        int tmpcolor;
                        tmpcolor = (int)(((g_color_iter - 1) % g_and_color) + 1);
                        tantable[tmpcolor-1] = g_new_z.y/(g_new_z.x+.000001);
                    }
                }
            }
            else if (g_inside_color == EPSCROSS)
            {
                hooper = 0;
                if (g_integer_fractal)
                {
                    if (labs(g_l_new_z.x) < labs(lcloseprox))
                    {
                        hooper = (lcloseprox > 0? 1 : -1); // close to y axis
                        goto plot_inside;
                    }
                    else if (labs(g_l_new_z.y) < labs(lcloseprox))
                    {
                        hooper = (lcloseprox > 0 ? 2: -2); // close to x axis
                        goto plot_inside;
                    }
                }
                else
                {
                    if (std::fabs(g_new_z.x) < std::fabs(g_close_proximity))
                    {
                        hooper = (g_close_proximity > 0? 1 : -1); // close to y axis
                        goto plot_inside;
                    }
                    else if (std::fabs(g_new_z.y) < std::fabs(g_close_proximity))
                    {
                        hooper = (g_close_proximity > 0? 2 : -2); // close to x axis
                        goto plot_inside;
                    }
                }
            }
            else if (g_inside_color == FMODI)
            {
                double mag;
                if (g_integer_fractal)
                {
                    g_new_z.x = ((double)g_l_new_z.x) / g_fudge_factor;
                    g_new_z.y = ((double)g_l_new_z.y) / g_fudge_factor;
                }
                mag = fmodtest();
                if (mag < g_close_proximity)
                {
                    memvalue = mag;
                }
            }
            else if (g_inside_color <= BOF60 && g_inside_color >= BOF61)
            {
                if (g_integer_fractal)
                {
                    if (g_l_magnitude == 0 || g_magnitude_calc)
                    {
                        g_l_magnitude = lsqr(g_l_new_z.x) + lsqr(g_l_new_z.y);
                    }
                    g_magnitude = g_l_magnitude;
                    g_magnitude = g_magnitude / g_fudge_factor;
                }
                else if (g_magnitude == 0.0 || g_magnitude_calc)
                {
                    g_magnitude = sqr(g_new_z.x) + sqr(g_new_z.y);
                }
                if (g_magnitude < min_orbit)
                {
                    min_orbit = g_magnitude;
                    min_index = g_color_iter + 1;
                }
            }
        }

        if (g_outside_color == TDIS || g_outside_color == FMOD)
        {
            if (g_bf_math == bf_math_type::BIGNUM)
            {
                g_new_z = cmplxbntofloat(&g_new_z_bn);
            }
            else if (g_bf_math == bf_math_type::BIGFLT)
            {
                g_new_z = cmplxbftofloat(&g_new_z_bf);
            }
            if (g_outside_color == TDIS)
            {
                if (g_integer_fractal)
                {
                    g_new_z.x = ((double)g_l_new_z.x) / g_fudge_factor;
                    g_new_z.y = ((double)g_l_new_z.y) / g_fudge_factor;
                }
                totaldist += std::sqrt(sqr(lastz.x-g_new_z.x)+sqr(lastz.y-g_new_z.y));
                lastz.x = g_new_z.x;
                lastz.y = g_new_z.y;
            }
            else if (g_outside_color == FMOD)
            {
                double mag;
                if (g_integer_fractal)
                {
                    g_new_z.x = ((double)g_l_new_z.x) / g_fudge_factor;
                    g_new_z.y = ((double)g_l_new_z.y) / g_fudge_factor;
                }
                mag = fmodtest();
                if (mag < g_close_proximity)
                {
                    memvalue = mag;
                }
            }
        }

        if (g_attractors > 0)       // finite attractor in the list
        {
            // NOTE: Integer code is UNTESTED
            if (g_integer_fractal)
            {
                for (int i = 0; i < g_attractors; i++)
                {
                    lat.x = g_l_new_z.x - g_l_attractor[i].x;
                    lat.x = lsqr(lat.x);
                    if (lat.x < g_l_at_rad)
                    {
                        lat.y = g_l_new_z.y - g_l_attractor[i].y;
                        lat.y = lsqr(lat.y);
                        if (lat.y < g_l_at_rad)
                        {
                            if ((lat.x + lat.y) < g_l_at_rad)
                            {
                                attracted = true;
                                if (g_finite_attractor)
                                {
                                    g_color_iter = (g_color_iter % g_attractor_period[i]) + 1;
                                }
                                break;
                            }
                        }
                    }
                }
            }
            else
            {
                for (int i = 0; i < g_attractors; i++)
                {
                    at.x = g_new_z.x - g_attractor[i].x;
                    at.x = sqr(at.x);
                    if (at.x < g_f_at_rad)
                    {
                        at.y = g_new_z.y - g_attractor[i].y;
                        at.y = sqr(at.y);
                        if (at.y < g_f_at_rad)
                        {
                            if ((at.x + at.y) < g_f_at_rad)
                            {
                                attracted = true;
                                if (g_finite_attractor)
                                {
                                    g_color_iter = (g_color_iter % g_attractor_period[i]) + 1;
                                }
                                break;
                            }
                        }
                    }
                }
            }
            if (attracted)
            {
                break;              // AHA! Eaten by an attractor
            }
        }

        if (g_color_iter > g_old_color_iter) // check periodicity
        {
            if ((g_color_iter & savedand) == 0)            // time to save a new value
            {
                savedcoloriter = g_color_iter;
                if (g_integer_fractal)
                {
                    lsaved = g_l_new_z;// integer fractals
                }
                else if (g_bf_math == bf_math_type::BIGNUM)
                {
                    copy_bn(g_saved_z_bn.x, g_new_z_bn.x);
                    copy_bn(g_saved_z_bn.y, g_new_z_bn.y);
                }
                else if (g_bf_math == bf_math_type::BIGFLT)
                {
                    copy_bf(g_saved_z_bf.x, g_new_z_bf.x);
                    copy_bf(g_saved_z_bf.y, g_new_z_bf.y);
                }
                else
                {
                    s_saved = g_new_z;  // floating pt fractals
                }
                if (--savedincr == 0)    // time to lengthen the periodicity?
                {
                    savedand = (savedand << 1) + 1;       // longer periodicity
                    savedincr = g_periodicity_next_saved_incr;// restart counter
                }
            }
            else                // check against an old save
            {
                if (g_integer_fractal)     // floating-pt periodicity chk
                {
                    if (labs(lsaved.x - g_l_new_z.x) < g_l_close_enough)
                    {
                        if (labs(lsaved.y - g_l_new_z.y) < g_l_close_enough)
                        {
                            caught_a_cycle = true;
                        }
                    }
                }
                else if (g_bf_math == bf_math_type::BIGNUM)
                {
                    if (cmp_bn(abs_a_bn(sub_bn(g_bn_tmp, g_saved_z_bn.x, g_new_z_bn.x)), g_close_enough_bn) < 0)
                    {
                        if (cmp_bn(abs_a_bn(sub_bn(g_bn_tmp, g_saved_z_bn.y, g_new_z_bn.y)), g_close_enough_bn) < 0)
                        {
                            caught_a_cycle = true;
                        }
                    }
                }
                else if (g_bf_math == bf_math_type::BIGFLT)
                {
                    if (cmp_bf(abs_a_bf(sub_bf(g_bf_tmp, g_saved_z_bf.x, g_new_z_bf.x)), g_close_enough_bf) < 0)
                    {
                        if (cmp_bf(abs_a_bf(sub_bf(g_bf_tmp, g_saved_z_bf.y, g_new_z_bf.y)), g_close_enough_bf) < 0)
                        {
                            caught_a_cycle = true;
                        }
                    }
                }
                else
                {
                    if (std::fabs(s_saved.x - g_new_z.x) < g_close_enough)
                    {
                        if (std::fabs(s_saved.y - g_new_z.y) < g_close_enough)
                        {
                            caught_a_cycle = true;
                        }
                    }
                }
                if (caught_a_cycle)
                {
                    cyclelen = g_color_iter-savedcoloriter;
                    g_color_iter = g_max_iterations - 1;
                }
            }
        }
    }  // end while (g_color_iter++ < maxit)

    if (g_show_orbit)
    {
        scrub_orbit();
    }

    g_real_color_iter = g_color_iter;           // save this before we start adjusting it
    if (g_color_iter >= g_max_iterations)
    {
        g_old_color_iter = 0;         // check periodicity immediately next time
    }
    else
    {
        g_old_color_iter = g_color_iter + 10;    // check when past this + 10 next time
        if (g_color_iter == 0)
        {
            g_color_iter = 1;         // needed to make same as calcmand
        }
    }

    if (g_potential_flag)
    {
        if (g_integer_fractal)       // adjust integer fractals
        {
            g_new_z.x = ((double)g_l_new_z.x) / g_fudge_factor;
            g_new_z.y = ((double)g_l_new_z.y) / g_fudge_factor;
        }
        else if (g_bf_math == bf_math_type::BIGNUM)
        {
            g_new_z.x = (double)bntofloat(g_new_z_bn.x);
            g_new_z.y = (double)bntofloat(g_new_z_bn.y);
        }
        else if (g_bf_math == bf_math_type::BIGFLT)
        {
            g_new_z.x = (double)bftofloat(g_new_z_bf.x);
            g_new_z.y = (double)bftofloat(g_new_z_bf.y);
        }
        g_magnitude = sqr(g_new_z.x) + sqr(g_new_z.y);
        g_color_iter = potential(g_magnitude, g_color_iter);
        if (!g_log_map_table.empty() || g_log_map_calculate)
        {
            g_color_iter = logtablecalc(g_color_iter);
        }
        goto plot_pixel;          // skip any other adjustments
    }

    if (g_color_iter >= g_max_iterations)                // an "inside" point
    {
        goto plot_inside;         // distest, decomp, biomorph don't apply
    }

    if (g_outside_color < ITER)
    {
        if (g_integer_fractal)
        {
            g_new_z.x = ((double)g_l_new_z.x) / g_fudge_factor;
            g_new_z.y = ((double)g_l_new_z.y) / g_fudge_factor;
        }
        else if (g_bf_math ==  bf_math_type::BIGNUM)
        {
            g_new_z.x = (double)bntofloat(g_new_z_bn.x);
            g_new_z.y = (double)bntofloat(g_new_z_bn.y);
        }
        // Add 7 to overcome negative values on the MANDEL
        if (g_outside_color == REAL)                 // "real"
        {
            g_color_iter += (long)g_new_z.x + 7;
        }
        else if (g_outside_color == IMAG)              // "imag"
        {
            g_color_iter += (long)g_new_z.y + 7;
        }
        else if (g_outside_color == MULT  && g_new_z.y)      // "mult"
        {
            g_color_iter = (long)((double)g_color_iter * (g_new_z.x/g_new_z.y));
        }
        else if (g_outside_color == SUM)               // "sum"
        {
            g_color_iter += (long)(g_new_z.x + g_new_z.y);
        }
        else if (g_outside_color == ATAN)              // "atan"
        {
            g_color_iter = (long)std::fabs(std::atan2(g_new_z.y, g_new_z.x)*g_atan_colors/PI);
        }
        else if (g_outside_color == FMOD)
        {
            g_color_iter = (long)(memvalue * g_colors / g_close_proximity);
        }
        else if (g_outside_color == TDIS)
        {
            g_color_iter = (long)(totaldist);
        }

        // eliminate negative colors & wrap arounds
        if ((g_color_iter <= 0 || g_color_iter > g_max_iterations) && g_outside_color != FMOD)
        {
            g_color_iter = 1;
        }
    }

    if (g_distance_estimator)
    {
        double dist;
        dist = sqr(g_new_z.x) + sqr(g_new_z.y);
        if (dist == 0 || g_overflow)
        {
            dist = 0;
        }
        else
        {
            double temp;
            temp = std::log(dist);
            dist = dist * sqr(temp) / (sqr(deriv.x) + sqr(deriv.y));
        }
        if (dist < dem_delta)     // point is on the edge
        {
            if (g_distance_estimator > 0)
            {
                goto plot_inside;   // show it as an inside point
            }
            g_color_iter = 0 - g_distance_estimator;       // show boundary as specified color
            goto plot_pixel;       // no further adjustments apply
        }
        if (g_colors == 2)
        {
            g_color_iter = !g_inside_color;   // the only useful distest 2 color use
            goto plot_pixel;       // no further adjustments apply
        }
        if (g_distance_estimator > 1)          // pick color based on distance
        {
            if (g_old_demm_colors)   // this one is needed for old color scheme
            {
                g_color_iter = (long)std::sqrt(sqrt(dist) / dem_width + 1);
            }
            else if (g_use_old_distance_estimator)
            {
                g_color_iter = (long)std::sqrt(dist / dem_width + 1);
            }
            else
            {
                g_color_iter = (long)(dist / dem_width + 1);
            }
            g_color_iter &= LONG_MAX;  // oops - color can be negative
            goto plot_pixel;       // no further adjustments apply
        }
        if (g_use_old_distance_estimator)
        {
            g_color_iter = dem_color;
            g_new_z = dem_new;
        }
        // use pixel's "regular" color
    }

    if (g_decomp[0] > 0)
    {
        decomposition();
    }
    else if (g_biomorph != -1)
    {
        if (g_integer_fractal)
        {
            if (labs(g_l_new_z.x) < g_l_magnitude_limit2 || labs(g_l_new_z.y) < g_l_magnitude_limit2)
            {
                g_color_iter = g_biomorph;
            }
        }
        else if (std::fabs(g_new_z.x) < g_magnitude_limit2 || std::fabs(g_new_z.y) < g_magnitude_limit2)
        {
            g_color_iter = g_biomorph;
        }
    }

    if (g_outside_color >= COLOR_BLACK && !attracted)       // merge escape-time stripes
    {
        g_color_iter = g_outside_color;
    }
    else if (!g_log_map_table.empty() || g_log_map_calculate)
    {
        g_color_iter = logtablecalc(g_color_iter);
    }
    goto plot_pixel;

plot_inside: // we're "inside"
    if (g_periodicity_check < 0 && caught_a_cycle)
    {
        g_color_iter = 7;           // show periodicity
    }
    else if (g_inside_color >= COLOR_BLACK)
    {
        g_color_iter = g_inside_color;              // set to specified color, ignore logpal
    }
    else
    {
        if (g_inside_color == STARTRAIL)
        {
            double diff;
            g_color_iter = 0;
            for (int i = 1; i < 16; i++)
            {
                diff = tantable[0] - tantable[i];
                if (std::fabs(diff) < .05)
                {
                    g_color_iter = i;
                    break;
                }
            }
        }
        else if (g_inside_color == PERIOD)
        {
            if (cyclelen > 0)
            {
                g_color_iter = cyclelen;
            }
            else
            {
                g_color_iter = g_max_iterations;
            }
        }
        else if (g_inside_color == EPSCROSS)
        {
            if (hooper == 1)
            {
                g_color_iter = green;
            }
            else if (hooper == 2)
            {
                g_color_iter = yellow;
            }
            else if (hooper == 0)
            {
                g_color_iter = g_max_iterations;
            }
            if (g_show_orbit)
            {
                scrub_orbit();
            }
        }
        else if (g_inside_color == FMODI)
        {
            g_color_iter = (long)(memvalue * g_colors / g_close_proximity);
        }
        else if (g_inside_color == ATANI)            // "atan"
        {
            if (g_integer_fractal)
            {
                g_new_z.x = ((double)g_l_new_z.x) / g_fudge_factor;
                g_new_z.y = ((double)g_l_new_z.y) / g_fudge_factor;
                g_color_iter = (long)std::fabs(std::atan2(g_new_z.y, g_new_z.x)*g_atan_colors/PI);
            }
            else
            {
                g_color_iter = (long)std::fabs(std::atan2(g_new_z.y, g_new_z.x)*g_atan_colors/PI);
            }
        }
        else if (g_inside_color == BOF60)
        {
            g_color_iter = (long)(std::sqrt(min_orbit) * 75);
        }
        else if (g_inside_color == BOF61)
        {
            g_color_iter = min_index;
        }
        else if (g_inside_color == ZMAG)
        {
            if (g_integer_fractal)
            {
                g_color_iter = (long)(((double)g_l_magnitude/g_fudge_factor) * (g_max_iterations >> 1) + 1);
            }
            else
            {
                g_color_iter = (long)((sqr(g_new_z.x) + sqr(g_new_z.y)) * (g_max_iterations >> 1) + 1);
            }
        }
        else   // inside == -1
        {
            g_color_iter = g_max_iterations;
        }
        if (!g_log_map_table.empty() || g_log_map_calculate)
        {
            g_color_iter = logtablecalc(g_color_iter);
        }
    }

plot_pixel:

    g_color = std::abs((int)g_color_iter);
    if (g_color_iter >= g_colors)
    {
        // don't use color 0 unless from inside/outside
        if (g_colors < 16)
        {
            g_color = (int)(g_color_iter & g_and_color);
        }
        else
        {
            g_color = (int)(((g_color_iter - 1) % g_and_color) + 1);
        }
    }
    if (g_debug_flag != debug_flags::force_boundary_trace_error)
    {
        if (g_color <= 0 && g_std_calc_mode == 'b')
        {
            g_color = 1;
        }
    }
    (*g_plot)(g_col, g_row, g_color);

    g_max_iterations = savemaxit;
    if ((g_keyboard_check_interval -= std::abs((int)g_real_color_iter)) <= 0)
    {
        if (check_key())
        {
            return -1;
        }
        g_keyboard_check_interval = g_max_keyboard_check_interval;
    }
    return g_color;
}

#define cos45  sin45
#define lcos45 lsin45

// standardfractal doodad subroutines
static void decomposition()
{
    // static double cos45     = 0.70710678118654750; // cos 45  degrees
    static double sin45     = 0.70710678118654750; // sin 45     degrees
    static double cos22_5   = 0.92387953251128670; // cos 22.5   degrees
    static double sin22_5   = 0.38268343236508980; // sin 22.5   degrees
    static double cos11_25  = 0.98078528040323040; // cos 11.25  degrees
    static double sin11_25  = 0.19509032201612820; // sin 11.25  degrees
    static double cos5_625  = 0.99518472667219690; // cos 5.625  degrees
    static double sin5_625  = 0.09801714032956060; // sin 5.625  degrees
    static double tan22_5   = 0.41421356237309500; // tan 22.5   degrees
    static double tan11_25  = 0.19891236737965800; // tan 11.25  degrees
    static double tan5_625  = 0.09849140335716425; // tan 5.625  degrees
    static double tan2_8125 = 0.04912684976946725; // tan 2.8125 degrees
    static double tan1_4063 = 0.02454862210892544; // tan 1.4063 degrees
    // static long lcos45;      // cos 45   degrees
    static long lsin45;         // sin 45     degrees
    static long lcos22_5;       // cos 22.5   degrees
    static long lsin22_5;       // sin 22.5   degrees
    static long lcos11_25;      // cos 11.25  degrees
    static long lsin11_25;      // sin 11.25  degrees
    static long lcos5_625;      // cos 5.625  degrees
    static long lsin5_625;      // sin 5.625  degrees
    static long ltan22_5;       // tan 22.5   degrees
    static long ltan11_25;      // tan 11.25  degrees
    static long ltan5_625;      // tan 5.625  degrees
    static long ltan2_8125;     // tan 2.8125 degrees
    static long ltan1_4063;     // tan 1.4063 degrees
    static long reset_fudge = -1;
    int temp = 0;
    int save_temp = 0;
    LComplex lalt;
    DComplex alt;
    g_color_iter = 0;
    if (g_integer_fractal) // the only case
    {
        if (reset_fudge != g_fudge_factor)
        {
            reset_fudge = g_fudge_factor;
            // lcos45     = (long)(cos45 * fudge);
            lsin45     = (long)(sin45 * g_fudge_factor);
            lcos22_5   = (long)(cos22_5 * g_fudge_factor);
            lsin22_5   = (long)(sin22_5 * g_fudge_factor);
            lcos11_25  = (long)(cos11_25 * g_fudge_factor);
            lsin11_25  = (long)(sin11_25 * g_fudge_factor);
            lcos5_625  = (long)(cos5_625 * g_fudge_factor);
            lsin5_625  = (long)(sin5_625 * g_fudge_factor);
            ltan22_5   = (long)(tan22_5 * g_fudge_factor);
            ltan11_25  = (long)(tan11_25 * g_fudge_factor);
            ltan5_625  = (long)(tan5_625 * g_fudge_factor);
            ltan2_8125 = (long)(tan2_8125 * g_fudge_factor);
            ltan1_4063 = (long)(tan1_4063 * g_fudge_factor);
        }
        if (g_l_new_z.y < 0)
        {
            temp = 2;
            g_l_new_z.y = -g_l_new_z.y;
        }

        if (g_l_new_z.x < 0)
        {
            ++temp;
            g_l_new_z.x = -g_l_new_z.x;
        }
        if (g_decomp[0] == 2)
        {
            save_temp = temp;
            if (temp == 2)
            {
                save_temp = 3;
            }
            else if (temp == 3)
            {
                save_temp = 2;
            }
        }

        if (g_decomp[0] >= 8)
        {
            temp <<= 1;
            if (g_l_new_z.x < g_l_new_z.y)
            {
                ++temp;
                lalt.x = g_l_new_z.x; // just
                g_l_new_z.x = g_l_new_z.y; // swap
                g_l_new_z.y = lalt.x; // them
            }

            if (g_decomp[0] >= 16)
            {
                temp <<= 1;
                if (multiply(g_l_new_z.x, ltan22_5, g_bit_shift) < g_l_new_z.y)
                {
                    ++temp;
                    lalt = g_l_new_z;
                    g_l_new_z.x = multiply(lalt.x, lcos45, g_bit_shift) +
                             multiply(lalt.y, lsin45, g_bit_shift);
                    g_l_new_z.y = multiply(lalt.x, lsin45, g_bit_shift) -
                             multiply(lalt.y, lcos45, g_bit_shift);
                }

                if (g_decomp[0] >= 32)
                {
                    temp <<= 1;
                    if (multiply(g_l_new_z.x, ltan11_25, g_bit_shift) < g_l_new_z.y)
                    {
                        ++temp;
                        lalt = g_l_new_z;
                        g_l_new_z.x = multiply(lalt.x, lcos22_5, g_bit_shift) +
                                 multiply(lalt.y, lsin22_5, g_bit_shift);
                        g_l_new_z.y = multiply(lalt.x, lsin22_5, g_bit_shift) -
                                 multiply(lalt.y, lcos22_5, g_bit_shift);
                    }

                    if (g_decomp[0] >= 64)
                    {
                        temp <<= 1;
                        if (multiply(g_l_new_z.x, ltan5_625, g_bit_shift) < g_l_new_z.y)
                        {
                            ++temp;
                            lalt = g_l_new_z;
                            g_l_new_z.x = multiply(lalt.x, lcos11_25, g_bit_shift) +
                                     multiply(lalt.y, lsin11_25, g_bit_shift);
                            g_l_new_z.y = multiply(lalt.x, lsin11_25, g_bit_shift) -
                                     multiply(lalt.y, lcos11_25, g_bit_shift);
                        }

                        if (g_decomp[0] >= 128)
                        {
                            temp <<= 1;
                            if (multiply(g_l_new_z.x, ltan2_8125, g_bit_shift) < g_l_new_z.y)
                            {
                                ++temp;
                                lalt = g_l_new_z;
                                g_l_new_z.x = multiply(lalt.x, lcos5_625, g_bit_shift) +
                                         multiply(lalt.y, lsin5_625, g_bit_shift);
                                g_l_new_z.y = multiply(lalt.x, lsin5_625, g_bit_shift) -
                                         multiply(lalt.y, lcos5_625, g_bit_shift);
                            }

                            if (g_decomp[0] == 256)
                            {
                                temp <<= 1;
                                if (multiply(g_l_new_z.x, ltan1_4063, g_bit_shift) < g_l_new_z.y)
                                {
                                    if ((g_l_new_z.x*ltan1_4063 < g_l_new_z.y))
                                    {
                                        ++temp;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    else // double case
    {
        if (g_new_z.y < 0)
        {
            temp = 2;
            g_new_z.y = -g_new_z.y;
        }
        if (g_new_z.x < 0)
        {
            ++temp;
            g_new_z.x = -g_new_z.x;
        }
        if (g_decomp[0] == 2)
        {
            save_temp = temp;
            if (temp == 2)
            {
                save_temp = 3;
            }
            else if (temp == 3)
            {
                save_temp = 2;
            }
        }
        if (g_decomp[0] >= 8)
        {
            temp <<= 1;
            if (g_new_z.x < g_new_z.y)
            {
                ++temp;
                alt.x = g_new_z.x; // just
                g_new_z.x = g_new_z.y; // swap
                g_new_z.y = alt.x; // them
            }
            if (g_decomp[0] >= 16)
            {
                temp <<= 1;
                if (g_new_z.x*tan22_5 < g_new_z.y)
                {
                    ++temp;
                    alt = g_new_z;
                    g_new_z.x = alt.x*cos45 + alt.y*sin45;
                    g_new_z.y = alt.x*sin45 - alt.y*cos45;
                }

                if (g_decomp[0] >= 32)
                {
                    temp <<= 1;
                    if (g_new_z.x*tan11_25 < g_new_z.y)
                    {
                        ++temp;
                        alt = g_new_z;
                        g_new_z.x = alt.x*cos22_5 + alt.y*sin22_5;
                        g_new_z.y = alt.x*sin22_5 - alt.y*cos22_5;
                    }

                    if (g_decomp[0] >= 64)
                    {
                        temp <<= 1;
                        if (g_new_z.x*tan5_625 < g_new_z.y)
                        {
                            ++temp;
                            alt = g_new_z;
                            g_new_z.x = alt.x*cos11_25 + alt.y*sin11_25;
                            g_new_z.y = alt.x*sin11_25 - alt.y*cos11_25;
                        }

                        if (g_decomp[0] >= 128)
                        {
                            temp <<= 1;
                            if (g_new_z.x*tan2_8125 < g_new_z.y)
                            {
                                ++temp;
                                alt = g_new_z;
                                g_new_z.x = alt.x*cos5_625 + alt.y*sin5_625;
                                g_new_z.y = alt.x*sin5_625 - alt.y*cos5_625;
                            }

                            if (g_decomp[0] == 256)
                            {
                                temp <<= 1;
                                if ((g_new_z.x*tan1_4063 < g_new_z.y))
                                {
                                    ++temp;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    for (int i = 1; temp > 0; ++i)
    {
        if (temp & 1)
        {
            g_color_iter = (1 << i) - 1 - g_color_iter;
        }
        temp >>= 1;
    }
    if (g_decomp[0] == 2)
    {
        if (save_temp & 2)
        {
            g_color_iter = 1;
        }
        else
        {
            g_color_iter = 0;
        }
        if (g_colors == 2)
        {
            g_color_iter++;
        }
    }
    if (g_colors > g_decomp[0])
    {
        g_color_iter++;
    }
}

// Continuous potential calculation for Mandelbrot and Julia
// Reference: Science of Fractal Images p. 190.
// Special thanks to Mark Peterson for his "MtMand" program that
// beautifully approximates plate 25 (same reference) and spurred
// on the inclusion of similar capabilities.
//
// The purpose of this function is to calculate a color value
// for a fractal that varies continuously with the screen pixels
// locations for better rendering in 3D.
//
// Here "magnitude" is the modulus of the orbit value at
// "iterations". The potparms[] are user-entered paramters
// controlling the level and slope of the continuous potential
// surface. Returns color.
//
static int potential(double mag, long iterations)
{
    float f_mag;
    float f_tmp;
    float pot;
    int i_pot;
    long l_pot;

    if (iterations < g_max_iterations)
    {
        l_pot = iterations+2;
        pot = (float) l_pot;
        if (l_pot <= 0 || mag <= 1.0)
        {
            pot = 0.0F;
        }
        else   // pot = log(mag) / pow(2.0, (double)pot);
        {
            if (l_pot < 120 && !g_float_flag) // empirically determined limit of fShift
            {
                f_mag = (float)mag;
                fLog14(f_mag, f_tmp); // this SHOULD be non-negative
                fShift(f_tmp, (char)-l_pot, pot);
            }
            else
            {
                double d_tmp = std::log(mag)/(double)std::pow(2.0, (double)pot);
                if (d_tmp > FLT_MIN)   // prevent float type underflow
                {
                    pot = (float)d_tmp;
                }
                else
                {
                    pot = 0.0F;
                }
            }
        }

        // following transformation strictly for aesthetic reasons
        // meaning of parameters:
        //    potparam[0] -- zero potential level - highest color -
        //    potparam[1] -- slope multiplier -- higher is steeper
        //    potparam[2] -- rqlim value if changeable (bailout for modulus)
        if (pot > 0.0)
        {
            if (g_float_flag)
            {
                pot = (float)std::sqrt((double)pot);
            }
            else
            {
                fSqrt14(pot, f_tmp);
                pot = f_tmp;
            }
            pot = (float)(g_potential_params[0] - pot*g_potential_params[1] - 1.0);
        }
        else
        {
            pot = (float)(g_potential_params[0] - 1.0);
        }
        if (pot < 1.0F)
        {
            pot = 1.0F; // avoid color 0
        }
    }
    else if (g_inside_color >= COLOR_BLACK)
    {
        pot = (float) g_inside_color;
    }
    else     // inside < 0 implies inside = maxit, so use 1st pot param instead
    {
        pot = (float)g_potential_params[0];
    }

    l_pot = (long) (pot * 256);
    i_pot = (int) (l_pot >> 8);
    if (i_pot >= g_colors)
    {
        i_pot = g_colors - 1;
        l_pot = 255;
    }

    if (g_potential_16bit)
    {
        if (!driver_diskp())   // if putcolor won't be doing it for us
        {
            disk_write_pixel(g_col+g_logical_screen_x_offset, g_row+g_logical_screen_y_offset, i_pot);
        }
        disk_write_pixel(g_col+g_logical_screen_x_offset, g_row+g_screen_y_dots+g_logical_screen_y_offset, (int)l_pot);
    }

    return i_pot;
}

// symmetry plot setup
static bool xsym_split(int xaxis_row, bool xaxis_between)
{
    if ((g_work_symmetry&0x11) == 0x10)   // already decided not sym
    {
        return true;
    }
    if ((g_work_symmetry&1) != 0)   // already decided on sym
    {
        g_i_y_stop = (g_yy_start+g_yy_stop)/2;
    }
    else   // new window, decide
    {
        g_work_symmetry |= 0x10;
        if (xaxis_row <= g_yy_start || xaxis_row >= g_yy_stop)
        {
            return true; // axis not in window
        }
        int i = xaxis_row + (xaxis_row - g_yy_start);
        if (xaxis_between)
        {
            ++i;
        }
        if (i > g_yy_stop) // split into 2 pieces, bottom has the symmetry
        {
            if (g_num_work_list >= MAX_CALC_WORK-1)   // no room to split
            {
                return true;
            }
            g_i_y_stop = xaxis_row - (g_yy_stop - xaxis_row);
            if (!xaxis_between)
            {
                --g_i_y_stop;
            }
            add_worklist(g_xx_start, g_xx_stop, g_xx_start, g_i_y_stop+1, g_yy_stop, g_i_y_stop+1, g_work_pass, 0);
            g_yy_stop = g_i_y_stop;
            return true; // tell set_symmetry no sym for current window
        }
        if (i < g_yy_stop) // split into 2 pieces, top has the symmetry
        {
            if (g_num_work_list >= MAX_CALC_WORK-1)   // no room to split
            {
                return true;
            }
            add_worklist(g_xx_start, g_xx_stop, g_xx_start, i+1, g_yy_stop, i+1, g_work_pass, 0);
            g_yy_stop = i;
        }
        g_i_y_stop = xaxis_row;
        g_work_symmetry |= 1;
    }
    g_symmetry = symmetry_type::NONE;
    return false; // tell set_symmetry its a go
}

static bool ysym_split(int yaxis_col, bool yaxis_between)
{
    if ((g_work_symmetry&0x22) == 0x20)   // already decided not sym
    {
        return true;
    }
    if ((g_work_symmetry&2) != 0)   // already decided on sym
    {
        g_i_x_stop = (g_xx_start+g_xx_stop)/2;
    }
    else   // new window, decide
    {
        g_work_symmetry |= 0x20;
        if (yaxis_col <= g_xx_start || yaxis_col >= g_xx_stop)
        {
            return true; // axis not in window
        }
        int i = yaxis_col + (yaxis_col - g_xx_start);
        if (yaxis_between)
        {
            ++i;
        }
        if (i > g_xx_stop) // split into 2 pieces, right has the symmetry
        {
            if (g_num_work_list >= MAX_CALC_WORK-1)   // no room to split
            {
                return true;
            }
            g_i_x_stop = yaxis_col - (g_xx_stop - yaxis_col);
            if (!yaxis_between)
            {
                --g_i_x_stop;
            }
            add_worklist(g_i_x_stop+1, g_xx_stop, g_i_x_stop+1, g_yy_start, g_yy_stop, g_yy_start, g_work_pass, 0);
            g_xx_stop = g_i_x_stop;
            return true; // tell set_symmetry no sym for current window
        }
        if (i < g_xx_stop) // split into 2 pieces, left has the symmetry
        {
            if (g_num_work_list >= MAX_CALC_WORK-1)   // no room to split
            {
                return true;
            }
            add_worklist(i+1, g_xx_stop, i+1, g_yy_start, g_yy_stop, g_yy_start, g_work_pass, 0);
            g_xx_stop = i;
        }
        g_i_x_stop = yaxis_col;
        g_work_symmetry |= 2;
    }
    g_symmetry = symmetry_type::NONE;
    return false; // tell set_symmetry its a go
}

static void setsymmetry(symmetry_type sym, bool uselist) // set up proper symmetrical plot functions
{
    int i;
    int xaxis_row;
    int yaxis_col; // pixel number for origin
    bool xaxis_between = false;
    bool yaxis_between = false;         // if axis between 2 pixels, not on one
    bool xaxis_on_screen = false;
    bool yaxis_on_screen = false;
    double ftemp;
    bf_t bft1;
    int saved = 0;
    g_symmetry = symmetry_type::X_AXIS;
    if (g_std_calc_mode == 's' || g_std_calc_mode == 'o')
    {
        return;
    }
    if (sym == symmetry_type::NO_PLOT && g_force_symmetry == symmetry_type::NOT_FORCED)
    {
        g_plot = noplot;
        return;
    }
    // NOTE: 16-bit potential disables symmetry
    // also any decomp= option and any inversion not about the origin
    // also any rotation other than 180deg and any off-axis stretch
    if (g_bf_math != bf_math_type::NONE)
    {
        if (cmp_bf(g_bf_x_min, g_bf_x_3rd) || cmp_bf(g_bf_y_min, g_bf_y_3rd))
        {
            return;
        }
    }
    if ((g_potential_flag && g_potential_16bit) || ((g_invert != 0) && g_inversion[2] != 0.0)
        || g_decomp[0] != 0
        || g_x_min != g_x_3rd || g_y_min != g_y_3rd)
    {
        return;
    }
    if (sym != symmetry_type::X_AXIS
        && sym != symmetry_type::X_AXIS_NO_PARAM
        && g_inversion[1] != 0.0
        && g_force_symmetry == symmetry_type::NOT_FORCED)
    {
        return;
    }
    if (g_force_symmetry < symmetry_type::NOT_FORCED)
    {
        sym = g_force_symmetry;
    }
    else if (g_force_symmetry == static_cast<symmetry_type>(1000))
    {
        g_force_symmetry = sym;  // for backwards compatibility
    }
    else if (g_outside_color == REAL
        || g_outside_color == IMAG
        || g_outside_color == MULT
        || g_outside_color == SUM
        || g_outside_color == ATAN
        || g_bail_out_test == bailouts::Manr
        || g_outside_color == FMOD)
    {
        return;
    }
    else if (g_inside_color == FMODI || g_outside_color == TDIS)
    {
        return;
    }
    bool parmszero = (g_param_z1.x == 0.0 && g_param_z1.y == 0.0 && g_use_init_orbit != init_orbit_mode::value);
    bool parmsnoreal = (g_param_z1.x == 0.0 && g_use_init_orbit != init_orbit_mode::value);
    bool parmsnoimag = (g_param_z1.y == 0.0 && g_use_init_orbit != init_orbit_mode::value);
    switch (g_fractal_type)
    {
    case fractal_type::LMANLAMFNFN:      // These need only P1 checked.
    case fractal_type::FPMANLAMFNFN:     // P2 is used for a switch value
    case fractal_type::LMANFNFN:         // These have NOPARM set in fractalp.c,
    case fractal_type::FPMANFNFN:        // but it only applies to P1.
    case fractal_type::FPMANDELZPOWER:   // or P2 is an exponent
    case fractal_type::LMANDELZPOWER:
    case fractal_type::FPMANZTOZPLUSZPWR:
    case fractal_type::MARKSMANDEL:
    case fractal_type::MARKSMANDELFP:
    case fractal_type::MARKSJULIA:
    case fractal_type::MARKSJULIAFP:
        break;
    case fractal_type::FORMULA:  // Check P2, P3, P4 and P5
    case fractal_type::FFORMULA:
        parmszero = (parmszero && g_params[2] == 0.0 && g_params[3] == 0.0
            && g_params[4] == 0.0 && g_params[5] == 0.0
            && g_params[6] == 0.0 && g_params[7] == 0.0
            && g_params[8] == 0.0 && g_params[9] == 0.0);
        parmsnoreal = (parmsnoreal && g_params[2] == 0.0 && g_params[4] == 0.0
            && g_params[6] == 0.0 && g_params[8] == 0.0);
        parmsnoimag = (parmsnoimag && g_params[3] == 0.0 && g_params[5] == 0.0
            && g_params[7] == 0.0 && g_params[9] == 0.0);
        break;
    default:   // Check P2 for the rest
        parmszero = (parmszero && g_param_z2.x == 0.0 && g_param_z2.y == 0.0);
    }
    yaxis_col = -1;
    xaxis_row = -1;
    if (g_bf_math != bf_math_type::NONE)
    {
        saved = save_stack();
        bft1    = alloc_stack(g_r_bf_length+2);
        xaxis_on_screen = (sign_bf(g_bf_y_min) != sign_bf(g_bf_y_max));
        yaxis_on_screen = (sign_bf(g_bf_x_min) != sign_bf(g_bf_x_max));
    }
    else
    {
        xaxis_on_screen = (sign(g_y_min) != sign(g_y_max));
        yaxis_on_screen = (sign(g_x_min) != sign(g_x_max));
    }
    if (xaxis_on_screen) // axis is on screen
    {
        if (g_bf_math != bf_math_type::NONE)
        {
            sub_bf(bft1, g_bf_y_min, g_bf_y_max);
            div_bf(bft1, g_bf_y_max, bft1);
            neg_a_bf(bft1);
            ftemp = (double)bftofloat(bft1);
        }
        else
        {
            ftemp = (0.0-g_y_max) / (g_y_min-g_y_max);
        }
        ftemp *= (g_logical_screen_y_dots-1);
        ftemp += 0.25;
        xaxis_row = (int)ftemp;
        xaxis_between = (ftemp - xaxis_row >= 0.5);
        if (!uselist && (!xaxis_between || (xaxis_row+1)*2 != g_logical_screen_y_dots))
        {
            xaxis_row = -1; // can't split screen, so dead center or not at all
        }
    }
    if (yaxis_on_screen) // axis is on screen
    {
        if (g_bf_math != bf_math_type::NONE)
        {
            sub_bf(bft1, g_bf_x_max, g_bf_x_min);
            div_bf(bft1, g_bf_x_min, bft1);
            neg_a_bf(bft1);
            ftemp = (double)bftofloat(bft1);
        }
        else
        {
            ftemp = (0.0-g_x_min) / (g_x_max-g_x_min);
        }
        ftemp *= (g_logical_screen_x_dots-1);
        ftemp += 0.25;
        yaxis_col = (int)ftemp;
        yaxis_between = (ftemp - yaxis_col >= 0.5);
        if (!uselist && (!yaxis_between || (yaxis_col+1)*2 != g_logical_screen_x_dots))
        {
            yaxis_col = -1; // can't split screen, so dead center or not at all
        }
    }
    switch (sym)       // symmetry switch
    {
    case symmetry_type::X_AXIS_NO_REAL:    // X-axis Symmetry (no real param)
        if (!parmsnoreal)
        {
            break;
        }
        goto xsym;
    case symmetry_type::X_AXIS_NO_IMAG:    // X-axis Symmetry (no imag param)
        if (!parmsnoimag)
        {
            break;
        }
        goto xsym;
    case symmetry_type::X_AXIS_NO_PARAM:                        // X-axis Symmetry  (no params)
        if (!parmszero)
        {
            break;
        }
xsym:
    case symmetry_type::X_AXIS:                       // X-axis Symmetry
        if (!xsym_split(xaxis_row, xaxis_between))
        {
            if (g_basin)
            {
                g_plot = symplot2basin;
            }
            else
            {
                g_plot = symplot2;
            }
        }
        break;
    case symmetry_type::Y_AXIS_NO_PARAM:                        // Y-axis Symmetry (No Parms)
        if (!parmszero)
        {
            break;
        }
    case symmetry_type::Y_AXIS:                       // Y-axis Symmetry
        if (!ysym_split(yaxis_col, yaxis_between))
        {
            g_plot = symplot2Y;
        }
        break;
    case symmetry_type::XY_AXIS_NO_PARAM:                       // X-axis AND Y-axis Symmetry (no parms)
        if (!parmszero)
        {
            break;
        }
    case symmetry_type::XY_AXIS:                      // X-axis AND Y-axis Symmetry
        xsym_split(xaxis_row, xaxis_between);
        ysym_split(yaxis_col, yaxis_between);
        switch (g_work_symmetry & 3)
        {
        case 1: // just xaxis symmetry
            if (g_basin)
            {
                g_plot = symplot2basin;
            }
            else
            {
                g_plot = symplot2 ;
            }
            break;
        case 2: // just yaxis symmetry
            if (g_basin) // got no routine for this case
            {
                g_i_x_stop = g_xx_stop; // fix what split should not have done
                g_symmetry = symmetry_type::X_AXIS;
            }
            else
            {
                g_plot = symplot2Y;
            }
            break;
        case 3: // both axes
            if (g_basin)
            {
                g_plot = symplot4basin;
            }
            else
            {
                g_plot = symplot4 ;
            }
        }
        break;
    case symmetry_type::ORIGIN_NO_PARAM:                       // Origin Symmetry (no parms)
        if (!parmszero)
        {
            break;
        }
    case symmetry_type::ORIGIN:                      // Origin Symmetry
originsym:
        if (!xsym_split(xaxis_row, xaxis_between)
            && !ysym_split(yaxis_col, yaxis_between))
        {
            g_plot = symplot2J;
            g_i_x_stop = g_xx_stop; // didn't want this changed
        }
        else
        {
            g_i_y_stop = g_yy_stop; // in case first split worked
            g_symmetry = symmetry_type::X_AXIS;
            g_work_symmetry = 0x30; // let it recombine with others like it
        }
        break;
    case symmetry_type::PI_SYM_NO_PARAM:
        if (!parmszero)
        {
            break;
        }
    case symmetry_type::PI_SYM:                      // PI symmetry
        if (g_bf_math != bf_math_type::NONE)
        {
            if ((double)bftofloat(abs_a_bf(sub_bf(bft1, g_bf_x_max, g_bf_x_min))) < PI/4)
            {
                break; // no point in pi symmetry if values too close
            }
        }
        else
        {
            if (std::fabs(g_x_max - g_x_min) < PI/4)
            {
                break; // no point in pi symmetry if values too close
            }
        }
        if ((g_invert != 0) && g_force_symmetry == symmetry_type::NOT_FORCED)
        {
            goto originsym;
        }
        g_plot = symPIplot ;
        g_symmetry = symmetry_type::NONE;
        if (!xsym_split(xaxis_row, xaxis_between)
            && !ysym_split(yaxis_col, yaxis_between))
        {
            if (g_param_z1.y == 0.0)
            {
                g_plot = symPIplot4J; // both axes
            }
            else
            {
                g_plot = symPIplot2J; // origin
            }
        }
        else
        {
            g_i_y_stop = g_yy_stop; // in case first split worked
            g_work_symmetry = 0x30;  // don't mark pisym as ysym, just do it unmarked
        }
        if (g_bf_math != bf_math_type::NONE)
        {
            sub_bf(bft1, g_bf_x_max, g_bf_x_min);
            abs_a_bf(bft1);
            g_pi_in_pixels = (int)((PI/(double)bftofloat(bft1)*g_logical_screen_x_dots)); // PI in pixels
        }
        else
        {
            g_pi_in_pixels = (int)((PI/std::fabs(g_x_max-g_x_min))*g_logical_screen_x_dots); // PI in pixels
        }

        g_i_x_stop = g_xx_start+g_pi_in_pixels-1;
        if (g_i_x_stop > g_xx_stop)
        {
            g_i_x_stop = g_xx_stop;
        }
        i = (g_xx_start+g_xx_stop)/2;
        if (g_plot == symPIplot4J && g_i_x_stop > i)
        {
            g_i_x_stop = i;
        }
        break;
    default:                  // no symmetry
        break;
    }
    if (g_bf_math != bf_math_type::NONE)
    {
        restore_stack(saved);
    }
}

// added for testing autologmap()
// insert at end of CALCFRAC.C

static long autologmap()
{
    // calculate round screen edges to avoid wasted colours in logmap
    long mincolour;
    int xstop = g_logical_screen_x_dots - 1; // don't use symetry
    int ystop = g_logical_screen_y_dots - 1; // don't use symetry
    long old_maxit;
    mincolour = LONG_MAX;
    g_row = 0;
    g_reset_periodicity = false;
    old_maxit = g_max_iterations;
    for (g_col = 0; g_col < xstop; g_col++) // top row
    {
        g_color = (*g_calc_type)();
        if (g_color == -1)
        {
            goto ack; // key pressed, bailout
        }
        if (g_real_color_iter < mincolour)
        {
            mincolour = g_real_color_iter ;
            g_max_iterations = std::max(2L, mincolour); // speedup for when edges overlap lakes
        }
        if (g_col >=32)
        {
            (*g_plot)(g_col-32, g_row, 0);
        }
    }                                    // these lines tidy up for BTM etc
    for (int lag = 32; lag > 0; lag--)
    {
        (*g_plot)(g_col-lag, g_row, 0);
    }

    g_col = xstop;
    for (g_row = 0; g_row < ystop; g_row++) // right  side
    {
        g_color = (*g_calc_type)();
        if (g_color == -1)
        {
            goto ack; // key pressed, bailout
        }
        if (g_real_color_iter < mincolour)
        {
            mincolour = g_real_color_iter ;
            g_max_iterations = std::max(2L, mincolour); // speedup for when edges overlap lakes
        }
        if (g_row >=32)
        {
            (*g_plot)(g_col, g_row-32, 0);
        }
    }
    for (int lag = 32; lag > 0; lag--)
    {
        (*g_plot)(g_col, g_row-lag, 0);
    }

    g_col = 0;
    for (g_row = 0; g_row < ystop; g_row++) // left  side
    {
        g_color = (*g_calc_type)();
        if (g_color == -1)
        {
            goto ack; // key pressed, bailout
        }
        if (g_real_color_iter < mincolour)
        {
            mincolour = g_real_color_iter ;
            g_max_iterations = std::max(2L, mincolour); // speedup for when edges overlap lakes
        }
        if (g_row >=32)
        {
            (*g_plot)(g_col, g_row-32, 0);
        }
    }
    for (int lag = 32; lag > 0; lag--)
    {
        (*g_plot)(g_col, g_row-lag, 0);
    }

    g_row = ystop ;
    for (g_col = 0; g_col < xstop; g_col++) // bottom row
    {
        g_color = (*g_calc_type)();
        if (g_color == -1)
        {
            goto ack; // key pressed, bailout
        }
        if (g_real_color_iter < mincolour)
        {
            mincolour = g_real_color_iter ;
            g_max_iterations = std::max(2L, mincolour); // speedup for when edges overlap lakes
        }
        if (g_col >=32)
        {
            (*g_plot)(g_col-32, g_row, 0);
        }
    }
    for (int lag = 32; lag > 0; lag--)
    {
        (*g_plot)(g_col-lag, g_row, 0);
    }

ack: // bailout here if key is pressed
    if (mincolour == 2)      // insure autologmap not called again
    {
        g_resuming = true;
    }
    g_max_iterations = old_maxit;

    return mincolour ;
}

// Symmetry plot for period PI
void symPIplot(int x, int y, int color)
{
    while (x <= g_xx_stop)
    {
        g_put_color(x, y, color) ;
        x += g_pi_in_pixels;
    }
}

// Symmetry plot for period PI plus Origin Symmetry
void symPIplot2J(int x, int y, int color)
{
    int i;
    int j;
    while (x <= g_xx_stop)
    {
        g_put_color(x, y, color) ;
        i = g_yy_stop-(y-g_yy_start);
        if (i > g_i_y_stop && i < g_logical_screen_y_dots
            && (j = g_xx_stop-(x-g_xx_start)) < g_logical_screen_x_dots)
        {
            g_put_color(j, i, color) ;
        }
        x += g_pi_in_pixels;
    }
}

// Symmetry plot for period PI plus Both Axis Symmetry
void symPIplot4J(int x, int y, int color)
{
    int i;
    int j;
    while (x <= (g_xx_start+g_xx_stop)/2)
    {
        j = g_xx_stop-(x-g_xx_start);
        g_put_color(x , y , color) ;
        if (j < g_logical_screen_x_dots)
        {
            g_put_color(j , y , color) ;
        }
        i = g_yy_stop-(y-g_yy_start);
        if (i > g_i_y_stop && i < g_logical_screen_y_dots)
        {
            g_put_color(x , i , color) ;
            if (j < g_logical_screen_x_dots)
            {
                g_put_color(j , i , color) ;
            }
        }
        x += g_pi_in_pixels;
    }
}

// Symmetry plot for X Axis Symmetry
void symplot2(int x, int y, int color)
{
    int i;
    g_put_color(x, y, color) ;
    i = g_yy_stop-(y-g_yy_start);
    if (i > g_i_y_stop && i < g_logical_screen_y_dots)
    {
        g_put_color(x, i, color) ;
    }
}

// Symmetry plot for Y Axis Symmetry
void symplot2Y(int x, int y, int color)
{
    int i;
    g_put_color(x, y, color) ;
    i = g_xx_stop-(x-g_xx_start);
    if (i < g_logical_screen_x_dots)
    {
        g_put_color(i, y, color) ;
    }
}

// Symmetry plot for Origin Symmetry
void symplot2J(int x, int y, int color)
{
    int i;
    int j;
    g_put_color(x, y, color) ;
    i = g_yy_stop-(y-g_yy_start);
    if (i > g_i_y_stop && i < g_logical_screen_y_dots
        && (j = g_xx_stop-(x-g_xx_start)) < g_logical_screen_x_dots)
    {
        g_put_color(j, i, color) ;
    }
}

// Symmetry plot for Both Axis Symmetry
void symplot4(int x, int y, int color)
{
    int i;
    int j;
    j = g_xx_stop-(x-g_xx_start);
    g_put_color(x , y, color) ;
    if (j < g_logical_screen_x_dots)
    {
        g_put_color(j , y, color) ;
    }
    i = g_yy_stop-(y-g_yy_start);
    if (i > g_i_y_stop && i < g_logical_screen_y_dots)
    {
        g_put_color(x , i, color) ;
        if (j < g_logical_screen_x_dots)
        {
            g_put_color(j , i, color) ;
        }
    }
}

// Symmetry plot for X Axis Symmetry - Striped Newtbasin version
void symplot2basin(int x, int y, int color)
{
    int i;
    int stripe;
    g_put_color(x, y, color) ;
    if (g_basin == 2 && color > 8)
    {
        stripe = 8;
    }
    else
    {
        stripe = 0;
    }
    i = g_yy_stop-(y-g_yy_start);
    if (i > g_i_y_stop && i < g_logical_screen_y_dots)
    {
        color -= stripe;                    // reconstruct unstriped color
        color = (g_degree+1-color)%g_degree+1;  // symmetrical color
        color += stripe;                    // add stripe
        g_put_color(x, i, color)  ;
    }
}

// Symmetry plot for Both Axis Symmetry  - Newtbasin version
void symplot4basin(int x, int y, int color)
{
    int i;
    int j;
    int color1;
    int stripe;
    if (color == 0) // assumed to be "inside" color
    {
        symplot4(x, y, color);
        return;
    }
    if (g_basin == 2 && color > 8)
    {
        stripe = 8;
    }
    else
    {
        stripe = 0;
    }
    color -= stripe;               // reconstruct unstriped color
    if (color < g_degree/2+2)
    {
        color1 = g_degree/2+2 - color;
    }
    else
    {
        color1 = g_degree/2+g_degree+2 - color;
    }
    j = g_xx_stop-(x-g_xx_start);
    g_put_color(x, y, color+stripe) ;
    if (j < g_logical_screen_x_dots)
    {
        g_put_color(j, y, color1+stripe) ;
    }
    i = g_yy_stop-(y-g_yy_start);
    if (i > g_i_y_stop && i < g_logical_screen_y_dots)
    {
        g_put_color(x, i, stripe + (g_degree+1 - color)%g_degree+1) ;
        if (j < g_logical_screen_x_dots)
        {
            g_put_color(j, i, stripe + (g_degree+1 - color1)%g_degree+1) ;
        }
    }
}

static void put_truecolor_disk(int x, int y, int color)
{
    putcolor_a(x, y, color);
    targa_color(x, y, color);
}

// Do nothing plot!!!
void noplot(int, int, int)
{
}
