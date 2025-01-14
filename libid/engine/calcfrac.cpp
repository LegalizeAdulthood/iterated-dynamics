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
#include "engine/calcfrac.h"

#include "3d/line3d.h"
#include "engine/bailout_formula.h"
#include "engine/boundary_trace.h"
#include "engine/calmanfp.h"
#include "engine/check_key.h"
#include "engine/diffusion_scan.h"
#include "engine/engine_timer.h"
#include "engine/fractalb.h"
#include "engine/fractals.h"
#include "engine/id_data.h"
#include "engine/log_map.h"
#include "engine/one_or_two_pass.h"
#include "engine/orbit.h"
#include "engine/pixel_grid.h"
#include "engine/resume.h"
#include "engine/soi.h"
#include "engine/solid_guess.h"
#include "engine/sticky_orbits.h"
#include "engine/tesseral.h"
#include "engine/wait_until.h"
#include "engine/work_list.h"
#include "fractals/fractalp.h"
#include "fractals/frothy_basin.h"
#include "fractals/lyapunov.h"
#include "fractals/newton.h"
#include "fractals/parser.h"
#include "io/check_write_file.h"
#include "io/update_save_name.h"
#include "math/biginit.h"
#include "math/mpmath.h"
#include "math/sign.h"
#include "misc/debug_flags.h"
#include "misc/drivers.h"
#include "ui/cmdfiles.h"
#include "ui/diskvid.h"
#include "ui/find_special_colors.h"
#include "ui/sound.h"
#include "ui/spindac.h"
#include "ui/stop_msg.h"
#include "ui/video.h"

#include <algorithm>
#include <cfloat>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <new>
#include <vector>

constexpr double DEM_BAILOUT{535.5};

enum class ShowDotAction
{
    SAVE = 1,
    RESTORE = 2
};

enum class ShowDotDirection
{
    JUST_A_POINT = 0,
    LOWER_RIGHT = 1,
    UPPER_RIGHT = 2,
    LOWER_LEFT = 3,
    UPPER_LEFT = 4
};

static void perform_work_list();
static void decomposition();
static void set_symmetry(SymmetryType sym, bool use_list);
static bool x_sym_split(int x_axis_row, bool x_axis_between);
static bool y_sym_split(int y_axis_col, bool y_axis_between);
static void put_true_color_disk(int, int, int);
static long auto_log_map();

static DComplex s_saved{};            //
static double s_rq_lim_save{};        //
static int (*s_calc_type_tmp)(){};    //
static unsigned long s_lm{};          // magnitude limit (CALCMAND)
static double s_dem_delta{};          //
static double s_dem_width{};          // distance estimator variables
static double s_dem_too_big{};        //
static bool s_dem_mandel{};           //
static std::vector<Byte> s_save_dots; //
static Byte *s_fill_buff{};           //
static int s_save_dots_len{};         //
static int s_show_dot_color{};        //
static int s_show_dot_width{};        //

// next has a skip bit for each maxblock unit;
//   1st pass sets bit  [1]... off only if block's contents guessed;
//   at end of 1st pass [0]... bits are set if any surrounding block not guessed;
//   bits are numbered [..][y/16+1][x+1]&(1<<(y&15))
// size of next puts a limit of MAX_PIXELS pixels across on solid guessing logic

int g_xx_begin{};                               // these are same as work list,
int g_yy_begin{};                               // declared as separate items
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
void (*g_put_color)(int, int, int){put_color_a}; //
void (*g_plot)(int, int, int){put_color_a};      //
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
SymmetryType g_symmetry{};                     // symmetry flag
SymmetryType g_force_symmetry{};               // force symmetry
bool g_reset_periodicity{};                     // true if escape time pixel rtn to reset
int g_keyboard_check_interval{};                //
int g_max_keyboard_check_interval{};            // avoids checking keyboard too often
int g_xx_start{};                               //
int g_xx_stop{};                                //
int g_yy_start{};                               //
int g_yy_stop{};                                //
int g_work_pass{};                              //
int g_work_symmetry{};                          // for the sake of calcmand
StatusValues g_got_status{StatusValues::NONE}; // variables which must be visible for tab_display
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
int g_periodicity_check{};                       //
int g_periodicity_next_saved_incr{};             // For periodicity testing, only in standard_fractal()
long g_first_saved_and{};                        //
int g_atan_colors{180};                          //
int g_and_color{};                               // "and" value used for color selection

static double fmod_test_bailout_or()
{
    double const tmp_x = sqr(g_new_z.x);
    double const tmp_y = sqr(g_new_z.y);
    if (tmp_x > tmp_y)
    {
        return tmp_x;
    }

    return tmp_y;
}

// Makes the test condition for the FMOD coloring type
//   that of the current bailout method. 'or' and 'and'
//   methods are not used - in these cases a normal
//   modulus test is used
//
static double fmod_test()
{
    double result;

    switch (g_bailout_test)
    {
    case Bailout::MOD:
        if (g_magnitude == 0.0 || g_magnitude_calc || g_integer_fractal)
        {
            result = sqr(g_new_z.x)+sqr(g_new_z.y);
        }
        else
        {
            result = g_magnitude; // don't recalculate
        }
        break;

    case Bailout::REAL:
        result = sqr(g_new_z.x);
        break;

    case Bailout::IMAG:
        result = sqr(g_new_z.y);
        break;

    case Bailout::OR:
        result = fmod_test_bailout_or();
        break;

    case Bailout::MANH:
        result = sqr(std::abs(g_new_z.x) + std::abs(g_new_z.y));
        break;

    case Bailout::MANR:
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
// solid color. This assumes that Byte *str is already filled
// with the color. The routine does write the line using symmetry
// in all cases, however the symmetry logic assumes that the line
// is one color; it is not general enough to handle a row of
// pixels of different colors.
void sym_fill_line(int row, int left, int right, Byte *str)
{
    int length = right - left + 1;
    write_span(row, left, right, str);
    // here's where all the symmetry goes
    if (g_plot == g_put_color)
    {
        g_keyboard_check_interval -= length >> 4; // seems like a reasonable value
    }
    else if (g_plot == sym_plot2)   // X-axis symmetry
    {
        int i = g_yy_stop-(row-g_yy_start);
        if (i > g_i_y_stop && i < g_logical_screen_y_dots)
        {
            write_span(i, left, right, str);
            g_keyboard_check_interval -= length >> 3;
        }
    }
    else if (g_plot == sym_plot2y) // Y-axis symmetry
    {
        write_span(row, g_xx_stop-(right-g_xx_start), g_xx_stop-(left-g_xx_start), str);
        g_keyboard_check_interval -= length >> 3;
    }
    else if (g_plot == sym_plot2j)  // Origin symmetry
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
    else if (g_plot == sym_plot4) // X-axis and Y-axis symmetry
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
static void sym_put_line(int row, int left, int right, Byte *str)
{
    int length = right-left+1;
    write_span(row, left, right, str);
    if (g_plot == g_put_color)
    {
        g_keyboard_check_interval -= length >> 4; // seems like a reasonable value
    }
    else if (g_plot == sym_plot2)   // X-axis symmetry
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

static void show_dot_save_restore(
    int start_x, int stop_x, int start_y, int stop_y, ShowDotDirection direction, ShowDotAction action)
{
    int ct = 0;
    if (direction != ShowDotDirection::JUST_A_POINT)
    {
        if (s_save_dots.empty())
        {
            stop_msg("savedots empty");
            exit(0);
        }
        if (s_fill_buff == nullptr)
        {
            stop_msg("fillbuff NULL");
            exit(0);
        }
    }
    switch (direction)
    {
    case ShowDotDirection::LOWER_RIGHT:
        for (int j = start_y; j <= stop_y; start_x++, j++)
        {
            if (action == ShowDotAction::SAVE)
            {
                read_span(j, start_x, stop_x, s_save_dots.data() + ct);
                sym_fill_line(j, start_x, stop_x, s_fill_buff);
            }
            else
            {
                sym_put_line(j, start_x, stop_x, s_save_dots.data() + ct);
            }
            ct += stop_x-start_x+1;
        }
        break;
    case ShowDotDirection::UPPER_RIGHT:
        for (int j = start_y; j >= stop_y; start_x++, j--)
        {
            if (action == ShowDotAction::SAVE)
            {
                read_span(j, start_x, stop_x, s_save_dots.data() + ct);
                sym_fill_line(j, start_x, stop_x, s_fill_buff);
            }
            else
            {
                sym_put_line(j, start_x, stop_x, s_save_dots.data() + ct);
            }
            ct += stop_x-start_x+1;
        }
        break;
    case ShowDotDirection::LOWER_LEFT:
        for (int j = start_y; j <= stop_y; stop_x--, j++)
        {
            if (action == ShowDotAction::SAVE)
            {
                read_span(j, start_x, stop_x, s_save_dots.data() + ct);
                sym_fill_line(j, start_x, stop_x, s_fill_buff);
            }
            else
            {
                sym_put_line(j, start_x, stop_x, s_save_dots.data() + ct);
            }
            ct += stop_x-start_x+1;
        }
        break;
    case ShowDotDirection::UPPER_LEFT:
        for (int j = start_y; j >= stop_y; stop_x--, j--)
        {
            if (action == ShowDotAction::SAVE)
            {
                read_span(j, start_x, stop_x, s_save_dots.data() + ct);
                sym_fill_line(j, start_x, stop_x, s_fill_buff);
            }
            else
            {
                sym_put_line(j, start_x, stop_x, s_save_dots.data() + ct);
            }
            ct += stop_x-start_x+1;
        }
        break;
    case ShowDotDirection::JUST_A_POINT:
        break;
    }
    if (action == ShowDotAction::SAVE)
    {
        (*g_plot)(g_col, g_row, s_show_dot_color);
    }
}

static int calc_type_show_dot()
{
    ShowDotDirection direction = ShowDotDirection::JUST_A_POINT;
    int stop_x = g_col;
    int start_x = g_col;
    int stop_y = g_row;
    int start_y = g_row;
    int width = s_show_dot_width + 1;
    if (width > 0)
    {
        if (g_col+width <= g_i_x_stop && g_row+width <= g_i_y_stop)
        {
            // preferred show dot shape
            direction = ShowDotDirection::UPPER_LEFT;
            start_x = g_col;
            stop_x  = g_col+width;
            start_y = g_row+width;
            stop_y  = g_row+1;
        }
        else if (g_col-width >= g_i_x_start && g_row+width <= g_i_y_stop)
        {
            // second choice
            direction = ShowDotDirection::UPPER_RIGHT;
            start_x = g_col-width;
            stop_x  = g_col;
            start_y = g_row+width;
            stop_y  = g_row+1;
        }
        else if (g_col-width >= g_i_x_start && g_row-width >= g_i_y_start)
        {
            direction = ShowDotDirection::LOWER_RIGHT;
            start_x = g_col-width;
            stop_x  = g_col;
            start_y = g_row-width;
            stop_y  = g_row-1;
        }
        else if (g_col+width <= g_i_x_stop && g_row-width >= g_i_y_start)
        {
            direction = ShowDotDirection::LOWER_LEFT;
            start_x = g_col;
            stop_x  = g_col+width;
            start_y = g_row-width;
            stop_y  = g_row-1;
        }
    }
    show_dot_save_restore(start_x, stop_x, start_y, stop_y, direction, ShowDotAction::SAVE);
    if (g_orbit_delay > 0)
    {
        sleep_ms(g_orbit_delay);
    }
    int out = (*s_calc_type_tmp)();
    show_dot_save_restore(start_x, stop_x, start_y, stop_y, direction, ShowDotAction::RESTORE);
    return out;
}

static void fix_inversion(double *x) // make double converted from string look ok
{
    char buf[30];
    std::sprintf(buf, "%-1.15lg", *x);
    *x = std::atof(buf);
}

// calcfract - the top level routine for generating an image
int calc_fract()
{
    g_attractors = 0;          // default to no known finite attractors
    g_display_3d = Display3DMode::NONE;
    g_basin = 0;
    g_put_color = put_color_a;
    if (g_is_true_color && g_true_mode != TrueColorMode::DEFAULT_COLOR)
    {
        // Have to force passes = 1
        g_std_calc_mode = '1';
        g_user_std_calc_mode = g_std_calc_mode;
    }
    if (g_true_color)
    {
        check_write_file(g_light_name, ".tga");
        if (!start_disk1(g_light_name, nullptr, false))
        {
            // Have to force passes = 1
            g_std_calc_mode = '1';
            g_user_std_calc_mode = g_std_calc_mode;
            g_put_color = put_true_color_disk;
        }
        else
        {
            g_true_color = false;
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
        stop_msg("Need at least 16 colors to use logmap");
        g_log_map_flag = 0;
    }

    if (g_use_old_periodicity)
    {
        g_periodicity_next_saved_incr = 1;
        g_first_saved_and = 1;
    }
    else
    {
        // works better than log()
        g_periodicity_next_saved_incr = (int) std::log10(static_cast<double>(g_max_iterations));
        // maintains image with low iterations
        g_periodicity_next_saved_incr = std::max(g_periodicity_next_saved_incr, 4);
        g_first_saved_and = (long) (g_periodicity_next_saved_incr * 2 + 1);
    }

    g_log_map_table.clear();
    g_log_map_table_max_size = g_max_iterations;
    g_log_map_calculate = false;
    // below, 32767 is used as the allowed value for maximum iteration count for
    // historical reasons.  TODO: increase this limit
    if (g_log_map_flag && (g_max_iterations > 32767 || g_log_map_fly_calculate == 1))
    {
        g_log_map_calculate = true; // calculate on the fly
        setup_log_table();
    }
    else if (g_log_map_flag && g_log_map_fly_calculate == 2)
    {
        g_log_map_table_max_size = 32767;
        g_log_map_calculate = false; // use logtable
    }
    else if (g_iteration_ranges_len && g_max_iterations >= 32767)
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
        catch (const std::bad_alloc &)
        {
        }

        if (!resized)
        {
            if (g_iteration_ranges_len || g_log_map_fly_calculate == 2)
            {
                stop_msg("Insufficient memory for logmap/ranges with this maxiter");
            }
            else
            {
                stop_msg("Insufficient memory for logTable, using on-the-fly routine");
                g_log_map_fly_calculate = 1;
                g_log_map_calculate = true; // calculate on the fly
                setup_log_table();
            }
        }
        else if (g_iteration_ranges_len)
        {
            // Can't do ranges if MaxLTSize > 32767
            int l = 0;
            int k = 0;
            int i = 0;
            g_log_map_flag = 0; // ranges overrides logmap
            while (i < g_iteration_ranges_len)
            {
                int flip = 0;
                int m = 0;
                int altern = 32767;
                int num_val = g_iteration_ranges[i++];
                if (num_val < 0)
                {
                    altern = g_iteration_ranges[i++];    // sub-range iterations
                    num_val = g_iteration_ranges[i++];
                }
                if (num_val > (int)g_log_map_table_max_size || i >= g_iteration_ranges_len)
                {
                    num_val = (int)g_log_map_table_max_size;
                }
                while (l <= num_val)
                {
                    g_log_map_table[l++] = (Byte)(k + flip);
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
            setup_log_table();
        }
    }
    s_lm = 4L << g_bit_shift;                 // CALCMAND magnitude limit

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
            g_inversion[0] = std::min(std::abs(g_x_max - g_x_min), std::abs(g_y_max - g_y_min)) / 6.0;
            fix_inversion(&g_inversion[0]);
            g_f_radius = g_inversion[0];
        }

        if (g_invert < 2 || g_inversion[1] == AUTO_INVERT)  // xcenter not already set
        {
            g_inversion[1] = (g_x_min + g_x_max) / 2.0;
            fix_inversion(&g_inversion[1]);
            g_f_x_center = g_inversion[1];
            if (std::abs(g_f_x_center) < std::abs(g_x_max-g_x_min) / 100)
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
            if (std::abs(g_f_y_center) < std::abs(g_y_max-g_y_min) / 100)
            {
                g_f_y_center = 0.0;
                g_inversion[2] = 0.0;
            }
        }

        g_invert = 3; // so values will not be changed if we come back
    }

    g_close_enough = g_delta_min*std::pow(2.0, -(double)(std::abs(g_periodicity_check)));
    s_rq_lim_save = g_magnitude_limit;
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
    g_resuming = (g_calc_status == CalcStatus::RESUMABLE);
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

    if (g_cur_fractal_specific->calc_type != standard_fractal
        && g_cur_fractal_specific->calc_type != calc_mand
        && g_cur_fractal_specific->calc_type != calc_mand_fp
        && g_cur_fractal_specific->calc_type != lyapunov
        && g_cur_fractal_specific->calc_type != calc_froth)
    {
        g_calc_type = g_cur_fractal_specific->calc_type; // per_image can override
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
        g_calc_status = CalcStatus::IN_PROGRESS; // mark as in-progress
        g_distance_estimator = 0; // only standard escape time engine supports distest
        // per_image routine is run here
        if (g_cur_fractal_specific->per_image())
        {
            // not a stand-alone
            // next two lines in case periodicity changed
            g_close_enough = g_delta_min*std::pow(2.0, -(double)(std::abs(g_periodicity_check)));
            g_l_close_enough = (long)(g_close_enough * g_fudge_factor); // "close enough" value
            set_symmetry(g_symmetry, false);
            engine_timer(g_calc_type); // non-standard fractal engine
        }
        if (check_key())
        {
            if (g_calc_status == CalcStatus::IN_PROGRESS)   // calctype didn't set this itself,
            {
                g_calc_status = CalcStatus::NON_RESUMABLE;   // so mark it interrupted, non-resumable
            }
        }
        else
        {
            g_calc_status = CalcStatus::COMPLETED; // no key, so assume it completed
        }
    }
    else // standard escape-time engine
    {
        auto timer_work_list{[]
            {
                perform_work_list();
                return 0;
            }};
        if (g_std_calc_mode == '3')  // convoluted 'g' + '2' hybrid
        {
            const int old_calc_mode = g_std_calc_mode;
            if (!g_resuming || g_three_pass)
            {
                g_std_calc_mode = 'g';
                g_three_pass = true;
                engine_timer(timer_work_list);
                if (g_calc_status == CalcStatus::COMPLETED)
                {
                    if (g_logical_screen_x_dots >= 640)    // '2' is silly after 'g' for low res
                    {
                        g_std_calc_mode = '2';
                    }
                    else
                    {
                        g_std_calc_mode = '1';
                    }
                    engine_timer(timer_work_list);
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
                engine_timer(timer_work_list);
            }
            g_std_calc_mode = (char)old_calc_mode;
        }
        else // main case, much nicer!
        {
            g_three_pass = false;
            engine_timer(timer_work_list);
        }
    }
    g_calc_time += g_timer_interval;

    if (!g_log_map_table.empty() && !g_log_map_calculate)
    {
        g_log_map_table.clear();
    }
    free_work_area();

    if (g_cur_fractal_specific->calc_type == calc_froth)
    {
        froth_cleanup();
    }
    if ((g_sound_flag & SOUNDFLAG_ORBIT_MASK) > SOUNDFLAG_BEEP)   // close sound write file
    {
        close_sound();
    }
    if (g_true_color)
    {
        end_disk();
    }
    return g_calc_status == CalcStatus::COMPLETED ? 0 : -1;
}

// locate alternate math record
int find_alternate_math(FractalType type, BFMathType math)
{
    if (math == BFMathType::NONE)
    {
        return -1;
    }
    int i = -1;
    FractalType current;
    while ((current = g_alternate_math[++i].type) != type && current != FractalType::NO_FRACTAL)
    {
    }
    int ret = -1;
    if (current == type && g_alternate_math[i].math != BFMathType::NONE)
    {
        ret = i;
    }
    return ret;
}

// general escape-time engine routines
static void perform_work_list()
{
    int (*sv_orbit_calc)() = nullptr;  // function that calculates one orbit
    int (*sv_per_pixel)() = nullptr;  // once-per-pixel init
    bool (*sv_per_image)() = nullptr;  // once-per-image setup
    int alt = find_alternate_math(g_fractal_type, g_bf_math);

    if (alt > -1)
    {
        sv_orbit_calc = g_cur_fractal_specific->orbit_calc;
        sv_per_pixel = g_cur_fractal_specific->per_pixel;
        sv_per_image = g_cur_fractal_specific->per_image;
        g_cur_fractal_specific->orbit_calc = g_alternate_math[alt].orbit_calc;
        g_cur_fractal_specific->per_pixel = g_alternate_math[alt].per_pixel;
        g_cur_fractal_specific->per_image = g_alternate_math[alt].per_image;
    }
    else
    {
        g_bf_math = BFMathType::NONE;
    }

    if (g_potential_flag && g_potential_16bit)
    {
        int tmp_calc_mode = g_std_calc_mode;

        g_std_calc_mode = '1'; // force 1 pass
        if (!g_resuming)
        {
            if (pot_start_disk() < 0)
            {
                g_potential_16bit = false;       // startdisk failed or cancelled
                g_std_calc_mode = (char)tmp_calc_mode;    // maybe we can carry on???
            }
        }
    }
    if (g_std_calc_mode == 'b' && bit_set(g_cur_fractal_specific->flags, FractalFlags::NO_TRACE))
    {
        g_std_calc_mode = '1';
    }
    if (g_std_calc_mode == 'g' && bit_set(g_cur_fractal_specific->flags, FractalFlags::NO_GUESS))
    {
        g_std_calc_mode = '1';
    }
    if (g_std_calc_mode == 'o' && (g_cur_fractal_specific->calc_type != standard_fractal))
    {
        g_std_calc_mode = '1';
    }

    // default setup a new worklist
    g_num_work_list = 1;
    g_work_list[0].xx_begin = 0;
    g_work_list[0].xx_start = 0;
    g_work_list[0].yy_begin = 0;
    g_work_list[0].yy_start = 0;
    g_work_list[0].xx_stop = g_logical_screen_x_dots - 1;
    g_work_list[0].yy_stop = g_logical_screen_y_dots - 1;
    g_work_list[0].symmetry = 0;
    g_work_list[0].pass = 0;
    if (g_resuming) // restore worklist, if we can't the above will stay in place
    {
        int version = start_resume();
        get_resume_len(sizeof(g_num_work_list), &g_num_work_list, sizeof(g_work_list), g_work_list, 0);
        end_resume();
        if (version < 2)
        {
            g_xx_begin = 0;
        }
    }

    if (g_distance_estimator) // setup stuff for distance estimator
    {
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

        double del_xx = (g_x_max - g_x_3rd) / d_x_size; // calculate stepsizes
        double del_yy = (g_y_max - g_y_3rd) / d_y_size;
        double del_xx2 = (g_x_3rd - g_x_min) / d_y_size;
        double del_yy2 = (g_y_3rd - g_y_min) / d_x_size;

        g_use_old_distance_estimator = false;
        g_magnitude_limit = s_rq_lim_save; // just in case changed to DEM_BAILOUT earlier
        if (g_distance_estimator != 1 || g_colors == 2)   // not doing regular outside colors
        {
            g_magnitude_limit = std::max(g_magnitude_limit, DEM_BAILOUT);
        }
        // must be mandel type, formula, or old PAR/GIF
        s_dem_mandel = g_cur_fractal_specific->to_julia != FractalType::NO_FRACTAL
            || g_use_old_distance_estimator
            || g_fractal_type == FractalType::FORMULA
            || g_fractal_type == FractalType::FORMULA_FP;
        s_dem_delta = sqr(del_xx) + sqr(del_yy2);
        double f_temp = sqr(del_yy) + sqr(del_xx2);
        s_dem_delta = std::max(f_temp, s_dem_delta);
        if (g_distance_estimator_width_factor == 0)
        {
            g_distance_estimator_width_factor = 1;
        }
        f_temp = g_distance_estimator_width_factor;
        if (g_distance_estimator_width_factor > 0)
        {
            s_dem_delta *= sqr(f_temp)/10000; // multiply by thickness desired
        }
        else
        {
            s_dem_delta *= 1/(sqr(f_temp)*10000); // multiply by thickness desired
        }
        s_dem_width = (std::sqrt(sqr(g_x_max-g_x_min) + sqr(g_x_3rd-g_x_min)) * aspect
                     + std::sqrt(sqr(g_y_max-g_y_min) + sqr(g_y_3rd-g_y_min))) / g_distance_estimator;
        f_temp = (g_magnitude_limit < DEM_BAILOUT) ? DEM_BAILOUT : g_magnitude_limit;
        f_temp += 3; // bailout plus just a bit
        double f_temp2 = std::log(f_temp);
        if (g_use_old_distance_estimator)
        {
            s_dem_too_big = sqr(f_temp) * sqr(f_temp2) * 4 / s_dem_delta;
        }
        else
        {
            s_dem_too_big = std::abs(f_temp) * std::abs(f_temp2) * 2 / std::sqrt(s_dem_delta);
        }
    }

    while (g_num_work_list > 0)
    {
        // per_image can override
        g_calc_type = g_cur_fractal_specific->calc_type;
        g_symmetry = g_cur_fractal_specific->symmetry; //   calctype & symmetry
        g_plot = g_put_color; // defaults when setsymmetry not called or does nothing

        // pull top entry off worklist
        g_xx_start = g_work_list[0].xx_start;
        g_i_x_start = g_work_list[0].xx_start;
        g_xx_stop  = g_work_list[0].xx_stop;
        g_i_x_stop  = g_work_list[0].xx_stop;
        g_xx_begin  = g_work_list[0].xx_begin;
        g_yy_start = g_work_list[0].yy_start;
        g_i_y_start = g_work_list[0].yy_start;
        g_yy_stop  = g_work_list[0].yy_stop;
        g_i_y_stop  = g_work_list[0].yy_stop;
        g_yy_begin  = g_work_list[0].yy_begin;
        g_work_pass = g_work_list[0].pass;
        g_work_symmetry  = g_work_list[0].symmetry;
        --g_num_work_list;
        for (int i = 0; i < g_num_work_list; ++i)
        {
            g_work_list[i] = g_work_list[i+1];
        }

        g_calc_status = CalcStatus::IN_PROGRESS; // mark as in-progress

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
                double d_show_dot_width = (double) g_size_dot * g_logical_screen_x_dots / 1024.0;

                // Arbitrary sanity limit, however showdot_width will
                // overflow if dshowdot width gets near 256.
                if (d_show_dot_width > 150.0)
                {
                    s_show_dot_width = 150;
                }
                else if (d_show_dot_width > 0.0)
                {
                    s_show_dot_width = (int)d_show_dot_width;
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
                catch (const std::bad_alloc &)
                {
                }

                if (resized)
                {
                    s_save_dots_len /= 2;
                    s_fill_buff = s_save_dots.data() + s_save_dots_len;
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
            s_calc_type_tmp = g_calc_type;
            g_calc_type    = calc_type_show_dot;
        }

        // some common initialization for escape-time pixel level routines
        g_close_enough = g_delta_min*std::pow(2.0, -(double)(std::abs(g_periodicity_check)));
        g_l_close_enough = (long)(g_close_enough * g_fudge_factor); // "close enough" value
        g_keyboard_check_interval = g_max_keyboard_check_interval;

        set_symmetry(g_symmetry, true);

        if (!g_resuming && (std::abs(g_log_map_flag) == 2 || (g_log_map_flag && g_log_map_auto_calculate)))
        {
            // calculate round screen edges to work out best start for logmap
            g_log_map_flag = (auto_log_map() * (g_log_map_flag / std::abs(g_log_map_flag)));
            setup_log_table();
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
            // TODO: fix this
            // horrible cludge preventing crash when coming back from perturbation and math = bignum/bigflt
            if (g_calc_status != CalcStatus::COMPLETED)
            {
                solid_guess();
            }
            break;
            
        case 'd':
            diffusion_scan();
            break;
            
        case 'o':
            sticky_orbits();
            break;
            
        case 'p':
            // we already finished perturbation
            if (bit_set(g_cur_fractal_specific->flags, FractalFlags::PERTURB))
            {
                return;
            }
            break;
            
        default:
            one_or_two_pass();
            break;
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
        put_resume_len(sizeof(g_num_work_list), &g_num_work_list, sizeof(g_work_list), g_work_list, 0);
    }
    else
    {
        g_calc_status = CalcStatus::COMPLETED; // completed
    }
    if (sv_orbit_calc != nullptr)
    {
        g_cur_fractal_specific->orbit_calc = sv_orbit_calc;
        g_cur_fractal_specific->per_pixel = sv_per_pixel;
        g_cur_fractal_specific->per_image = sv_per_image;
    }
}

static long calc_mand_asm()
{
    static bool been_here = false;
    if (!been_here)
    {
        stop_msg("This integer fractal type is unimplemented;\n"
                "Use float=yes to get a real image.");
        been_here = true;
    }
    return 0;
}

int calc_mand()              // fast per pixel 1/2/b/g, called with row & col set
{
    // setup values from array to avoid using es reg in calcmand.asm
    g_l_init_x = g_l_x_pixel();
    g_l_init_y = g_l_y_pixel();
    if (calc_mand_asm() >= 0)
    {
        if ((!g_log_map_table.empty() || g_log_map_calculate) // map color, but not if maxit & adjusted for inside,etc
            && (g_real_color_iter < g_max_iterations
                || (g_inside_color < COLOR_BLACK && g_color_iter == g_max_iterations)))
        {
            g_color_iter = log_table_calc(g_color_iter);
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
        if (g_debug_flag != DebugFlags::FORCE_BOUNDARY_TRACE_ERROR)
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
int calc_mand_fp()
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
    if (calc_mand_fp_asm() >= 0)
    {
        if (g_potential_flag)
        {
            g_color_iter = potential(g_magnitude, g_real_color_iter);
        }
        if ((!g_log_map_table.empty() || g_log_map_calculate) // map color, but not if maxit & adjusted for inside,etc
            && (g_real_color_iter < g_max_iterations
                || (g_inside_color < COLOR_BLACK && g_color_iter == g_max_iterations)))
        {
            g_color_iter = log_table_calc(g_color_iter);
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
        if (g_debug_flag != DebugFlags::FORCE_BOUNDARY_TRACE_ERROR)
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
#define STAR_TRAIL_MAX FLT_MAX   // just a convenient large number

int standard_fractal()       // per pixel 1/2/b/g, called with row & col set
{
    long save_max_it = 0;
    double tan_table[16]{};
    int hooper = 0;
    long l_close_prox = 0;
    double mem_value = 0.0;
    double min_orbit = 100000.0;        // orbit value closest to origin
    long   min_index = 0;               // iteration of min_orbit
    long cycle_len = -1;
    long saved_color_iter = 0;
    bool caught_a_cycle = false;
    long saved_and = 0;
    int saved_incr = 0;                  // for periodicity checking
    LComplex l_saved{};
    bool attracted = false;
    LComplex lat{};
    DComplex  at{};
    DComplex deriv{};
    long dem_color = -1;
    DComplex dem_new{};
    int check_freq = 0;
    double total_dist = 0.0;
    DComplex last_z{};

    l_close_prox = (long)(g_close_proximity*g_fudge_factor);
    save_max_it = g_max_iterations;
    if (g_inside_color == STAR_TRAIL)
    {
        std::fill(std::begin(tan_table), std::end(tan_table), 0.0);
        g_max_iterations = 16;
    }
    if (g_periodicity_check == 0 || g_inside_color == ZMAG || g_inside_color == STAR_TRAIL)
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
    g_old_color_iter = std::max<long>(g_old_color_iter, MINSAVEDAND);
#else
    g_old_color_iter = std::max(g_old_color_iter, g_first_saved_and);
#endif
    // really fractal specific, but we'll leave it here
    if (!g_integer_fractal)
    {
        if (g_use_init_orbit == InitOrbitMode::VALUE)
        {
            s_saved = g_init_orbit;
        }
        else
        {
            s_saved.x = 0;
            s_saved.y = 0;
        }
        if (g_bf_math != BFMathType::NONE)
        {
            if (g_decimals > 200)
            {
                g_keyboard_check_interval = -1;
            }
            if (g_bf_math == BFMathType::BIG_NUM)
            {
                clear_bn(g_saved_z_bn.x);
                clear_bn(g_saved_z_bn.y);
            }
            else if (g_bf_math == BFMathType::BIG_FLT)
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
                g_magnitude_limit = s_rq_lim_save;
                if (g_distance_estimator != 1 || g_colors == 2)   // not doing regular outside colors
                {
                    // so go straight for dem bailout
                    g_magnitude_limit = std::max(g_magnitude_limit, DEM_BAILOUT);
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
        if (g_use_init_orbit == InitOrbitMode::VALUE)
        {
            l_saved = g_l_init_orbit;
        }
        else
        {
            l_saved.x = 0;
            l_saved.y = 0;
        }
        g_l_init.y = g_l_y_pixel();
    }
    g_orbit_save_index = 0;
    g_color_iter = 0;
    if (g_fractal_type == FractalType::JULIA_FP || g_fractal_type == FractalType::JULIA)
    {
        g_color_iter = -1;
    }
    caught_a_cycle = false;
    if (g_inside_color == PERIOD)
    {
        saved_and = 16;           // begin checking every 16th cycle
    }
    else
    {
#ifdef MINSAVEDAND
        savedand = MINSAVEDAND;
#else
        saved_and = g_first_saved_and;                // begin checking every other cycle
#endif
    }
    saved_incr = 1;               // start checking the very first time

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
        else if (g_bf_math == BFMathType::BIG_NUM)
        {
            g_old_z = cmplx_bn_to_float(g_old_z_bn);
        }
        else if (g_bf_math == BFMathType::BIG_FLT)
        {
            g_old_z = cmplx_bf_to_float(g_old_z_bf);
        }
        last_z.x = g_old_z.x;
        last_z.y = g_old_z.y;
    }

    if (((g_sound_flag & SOUNDFLAG_ORBIT_MASK) > SOUNDFLAG_X || g_show_dot >= 0) && g_orbit_delay > 0)
    {
        check_freq = 16;
    }
    else
    {
        check_freq = 2048;
    }

    if (g_show_orbit)
    {
        sound_time_write();
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
            double f_temp;
            // Distance estimator for points near Mandelbrot set
            // Algorithms from Peitgen & Saupe, Science of Fractal Images, p.198
            if (s_dem_mandel)
            {
                f_temp = 2 * (g_old_z.x * deriv.x - g_old_z.y * deriv.y) + 1;
            }
            else
            {
                f_temp = 2 * (g_old_z.x * deriv.x - g_old_z.y * deriv.y);
            }
            deriv.y = 2 * (g_old_z.y * deriv.x + g_old_z.x * deriv.y);
            deriv.x = f_temp;
            if (g_use_old_distance_estimator)
            {
                if (sqr(deriv.x)+sqr(deriv.y) > s_dem_too_big)
                {
                    break;
                }
            }
            else
            {
                if (std::max(std::abs(deriv.x), std::abs(deriv.y)) > s_dem_too_big)
                {
                    break;
                }
            }
            // if above exit taken, the later test vs dem_delta will place this
            // point on the boundary, because mag(old)<bailout just now

            if (g_cur_fractal_specific->orbit_calc() || g_overflow)
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
        else if ((g_cur_fractal_specific->orbit_calc() && g_inside_color != STAR_TRAIL) || g_overflow)
        {
            break;
        }
        if (g_show_orbit)
        {
            if (!g_integer_fractal)
            {
                if (g_bf_math == BFMathType::BIG_NUM)
                {
                    g_new_z = cmplx_bn_to_float(g_new_z_bn);
                }
                else if (g_bf_math == BFMathType::BIG_FLT)
                {
                    g_new_z = cmplx_bf_to_float(g_new_z_bf);
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
            if (g_bf_math == BFMathType::BIG_NUM)
            {
                g_new_z = cmplx_bn_to_float(g_new_z_bn);
            }
            else if (g_bf_math == BFMathType::BIG_FLT)
            {
                g_new_z = cmplx_bf_to_float(g_new_z_bf);
            }
            if (g_inside_color == STAR_TRAIL)
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

                    g_new_z.x = std::min<double>(g_new_z.x, STAR_TRAIL_MAX);
                    g_new_z.x = std::max<double>(g_new_z.x, -STAR_TRAIL_MAX);
                    g_new_z.y = std::min<double>(g_new_z.y, STAR_TRAIL_MAX);
                    g_new_z.y = std::max<double>(g_new_z.y, -STAR_TRAIL_MAX);
                    g_temp_sqr_x = g_new_z.x * g_new_z.x;
                    g_temp_sqr_y = g_new_z.y * g_new_z.y;
                    g_magnitude = g_temp_sqr_x + g_temp_sqr_y;
                    g_old_z = g_new_z;
                    {
                        int tmp_color;
                        tmp_color = (int)(((g_color_iter - 1) % g_and_color) + 1);
                        tan_table[tmp_color-1] = g_new_z.y/(g_new_z.x+.000001);
                    }
                }
            }
            else if (g_inside_color == EPS_CROSS)
            {
                hooper = 0;
                if (g_integer_fractal)
                {
                    if (std::abs(g_l_new_z.x) < std::abs(l_close_prox))
                    {
                        hooper = (l_close_prox > 0? 1 : -1); // close to y axis
                        goto plot_inside;
                    }
                    else if (std::abs(g_l_new_z.y) < std::abs(l_close_prox))
                    {
                        hooper = (l_close_prox > 0 ? 2: -2); // close to x axis
                        goto plot_inside;
                    }
                }
                else
                {
                    if (std::abs(g_new_z.x) < std::abs(g_close_proximity))
                    {
                        hooper = (g_close_proximity > 0? 1 : -1); // close to y axis
                        goto plot_inside;
                    }
                    else if (std::abs(g_new_z.y) < std::abs(g_close_proximity))
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
                mag = fmod_test();
                if (mag < g_close_proximity)
                {
                    mem_value = mag;
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
            if (g_bf_math == BFMathType::BIG_NUM)
            {
                g_new_z = cmplx_bn_to_float(g_new_z_bn);
            }
            else if (g_bf_math == BFMathType::BIG_FLT)
            {
                g_new_z = cmplx_bf_to_float(g_new_z_bf);
            }
            if (g_outside_color == TDIS)
            {
                if (g_integer_fractal)
                {
                    g_new_z.x = ((double)g_l_new_z.x) / g_fudge_factor;
                    g_new_z.y = ((double)g_l_new_z.y) / g_fudge_factor;
                }
                total_dist += std::sqrt(sqr(last_z.x-g_new_z.x)+sqr(last_z.y-g_new_z.y));
                last_z.x = g_new_z.x;
                last_z.y = g_new_z.y;
            }
            else if (g_outside_color == FMOD)
            {
                double mag;
                if (g_integer_fractal)
                {
                    g_new_z.x = ((double)g_l_new_z.x) / g_fudge_factor;
                    g_new_z.y = ((double)g_l_new_z.y) / g_fudge_factor;
                }
                mag = fmod_test();
                if (mag < g_close_proximity)
                {
                    mem_value = mag;
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
            if ((g_color_iter & saved_and) == 0)            // time to save a new value
            {
                saved_color_iter = g_color_iter;
                if (g_integer_fractal)
                {
                    l_saved = g_l_new_z;// integer fractals
                }
                else if (g_bf_math == BFMathType::BIG_NUM)
                {
                    copy_bn(g_saved_z_bn.x, g_new_z_bn.x);
                    copy_bn(g_saved_z_bn.y, g_new_z_bn.y);
                }
                else if (g_bf_math == BFMathType::BIG_FLT)
                {
                    copy_bf(g_saved_z_bf.x, g_new_z_bf.x);
                    copy_bf(g_saved_z_bf.y, g_new_z_bf.y);
                }
                else
                {
                    s_saved = g_new_z;  // floating pt fractals
                }
                if (--saved_incr == 0)    // time to lengthen the periodicity?
                {
                    saved_and = (saved_and << 1) + 1;       // longer periodicity
                    saved_incr = g_periodicity_next_saved_incr;// restart counter
                }
            }
            else                // check against an old save
            {
                if (g_integer_fractal)     // floating-pt periodicity chk
                {
                    if (std::abs(l_saved.x - g_l_new_z.x) < g_l_close_enough)
                    {
                        if (std::abs(l_saved.y - g_l_new_z.y) < g_l_close_enough)
                        {
                            caught_a_cycle = true;
                        }
                    }
                }
                else if (g_bf_math == BFMathType::BIG_NUM)
                {
                    if (cmp_bn(abs_a_bn(sub_bn(g_bn_tmp, g_saved_z_bn.x, g_new_z_bn.x)), g_close_enough_bn) < 0)
                    {
                        if (cmp_bn(abs_a_bn(sub_bn(g_bn_tmp, g_saved_z_bn.y, g_new_z_bn.y)), g_close_enough_bn) < 0)
                        {
                            caught_a_cycle = true;
                        }
                    }
                }
                else if (g_bf_math == BFMathType::BIG_FLT)
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
                    if (std::abs(s_saved.x - g_new_z.x) < g_close_enough)
                    {
                        if (std::abs(s_saved.y - g_new_z.y) < g_close_enough)
                        {
                            caught_a_cycle = true;
                        }
                    }
                }
                if (caught_a_cycle)
                {
                    cycle_len = g_color_iter-saved_color_iter;
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
        else if (g_bf_math == BFMathType::BIG_NUM)
        {
            g_new_z.x = (double)bn_to_float(g_new_z_bn.x);
            g_new_z.y = (double)bn_to_float(g_new_z_bn.y);
        }
        else if (g_bf_math == BFMathType::BIG_FLT)
        {
            g_new_z.x = (double)bf_to_float(g_new_z_bf.x);
            g_new_z.y = (double)bf_to_float(g_new_z_bf.y);
        }
        g_magnitude = sqr(g_new_z.x) + sqr(g_new_z.y);
        g_color_iter = potential(g_magnitude, g_color_iter);
        if (!g_log_map_table.empty() || g_log_map_calculate)
        {
            g_color_iter = log_table_calc(g_color_iter);
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
        else if (g_bf_math ==  BFMathType::BIG_NUM)
        {
            g_new_z.x = (double)bn_to_float(g_new_z_bn.x);
            g_new_z.y = (double)bn_to_float(g_new_z_bn.y);
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
            g_color_iter = (long)std::abs(std::atan2(g_new_z.y, g_new_z.x)*g_atan_colors/PI);
        }
        else if (g_outside_color == FMOD)
        {
            g_color_iter = (long)(mem_value * g_colors / g_close_proximity);
        }
        else if (g_outside_color == TDIS)
        {
            g_color_iter = (long)(total_dist);
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
        if (dist < s_dem_delta)     // point is on the edge
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
                g_color_iter = (long)std::sqrt(sqrt(dist) / s_dem_width + 1);
            }
            else if (g_use_old_distance_estimator)
            {
                g_color_iter = (long)std::sqrt(dist / s_dem_width + 1);
            }
            else
            {
                g_color_iter = (long)(dist / s_dem_width + 1);
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
            if (std::abs(g_l_new_z.x) < g_l_magnitude_limit2 || std::abs(g_l_new_z.y) < g_l_magnitude_limit2)
            {
                g_color_iter = g_biomorph;
            }
        }
        else if (std::abs(g_new_z.x) < g_magnitude_limit2 || std::abs(g_new_z.y) < g_magnitude_limit2)
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
        g_color_iter = log_table_calc(g_color_iter);
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
        if (g_inside_color == STAR_TRAIL)
        {
            double diff;
            g_color_iter = 0;
            for (int i = 1; i < 16; i++)
            {
                diff = tan_table[0] - tan_table[i];
                if (std::abs(diff) < .05)
                {
                    g_color_iter = i;
                    break;
                }
            }
        }
        else if (g_inside_color == PERIOD)
        {
            if (cycle_len > 0)
            {
                g_color_iter = cycle_len;
            }
            else
            {
                g_color_iter = g_max_iterations;
            }
        }
        else if (g_inside_color == EPS_CROSS)
        {
            if (hooper == 1)
            {
                int const green = 2;
                g_color_iter = green;
            }
            else if (hooper == 2)
            {
                int const yellow = 6;
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
            g_color_iter = (long)(mem_value * g_colors / g_close_proximity);
        }
        else if (g_inside_color == ATANI)            // "atan"
        {
            if (g_integer_fractal)
            {
                g_new_z.x = ((double)g_l_new_z.x) / g_fudge_factor;
                g_new_z.y = ((double)g_l_new_z.y) / g_fudge_factor;
                g_color_iter = (long)std::abs(std::atan2(g_new_z.y, g_new_z.x)*g_atan_colors/PI);
            }
            else
            {
                g_color_iter = (long)std::abs(std::atan2(g_new_z.y, g_new_z.x)*g_atan_colors/PI);
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
            g_color_iter = log_table_calc(g_color_iter);
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
    if (g_debug_flag != DebugFlags::FORCE_BOUNDARY_TRACE_ERROR)
    {
        if (g_color <= 0 && g_std_calc_mode == 'b')
        {
            g_color = 1;
        }
    }
    (*g_plot)(g_col, g_row, g_color);

    g_max_iterations = save_max_it;
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
#define l_cos45 l_sin45

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
    // static long l_cos45;      // cos 45   degrees
    static long l_sin45;         // sin 45     degrees
    static long l_cos22_5;       // cos 22.5   degrees
    static long l_sin22_5;       // sin 22.5   degrees
    static long l_cos11_25;      // cos 11.25  degrees
    static long l_sin11_25;      // sin 11.25  degrees
    static long l_cos5_625;      // cos 5.625  degrees
    static long l_sin5_625;      // sin 5.625  degrees
    static long l_tan22_5;       // tan 22.5   degrees
    static long l_tan11_25;      // tan 11.25  degrees
    static long l_tan5_625;      // tan 5.625  degrees
    static long l_tan2_8125;     // tan 2.8125 degrees
    static long l_tan1_4063;     // tan 1.4063 degrees
    static long reset_fudge = -1;
    int temp = 0;
    int save_temp = 0;
    g_color_iter = 0;
    if (g_integer_fractal) // the only case
    {
        if (reset_fudge != g_fudge_factor)
        {
            reset_fudge = g_fudge_factor;
            // l_cos45     = (long)(cos45 * fudge);
            l_sin45     = (long)(sin45 * g_fudge_factor);
            l_cos22_5   = (long)(cos22_5 * g_fudge_factor);
            l_sin22_5   = (long)(sin22_5 * g_fudge_factor);
            l_cos11_25  = (long)(cos11_25 * g_fudge_factor);
            l_sin11_25  = (long)(sin11_25 * g_fudge_factor);
            l_cos5_625  = (long)(cos5_625 * g_fudge_factor);
            l_sin5_625  = (long)(sin5_625 * g_fudge_factor);
            l_tan22_5   = (long)(tan22_5 * g_fudge_factor);
            l_tan11_25  = (long)(tan11_25 * g_fudge_factor);
            l_tan5_625  = (long)(tan5_625 * g_fudge_factor);
            l_tan2_8125 = (long)(tan2_8125 * g_fudge_factor);
            l_tan1_4063 = (long)(tan1_4063 * g_fudge_factor);
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
            LComplex l_alt;
            temp <<= 1;
            if (g_l_new_z.x < g_l_new_z.y)
            {
                ++temp;
                l_alt.x = g_l_new_z.x; // just
                g_l_new_z.x = g_l_new_z.y; // swap
                g_l_new_z.y = l_alt.x; // them
            }

            if (g_decomp[0] >= 16)
            {
                temp <<= 1;
                if (multiply(g_l_new_z.x, l_tan22_5, g_bit_shift) < g_l_new_z.y)
                {
                    ++temp;
                    l_alt = g_l_new_z;
                    g_l_new_z.x = multiply(l_alt.x, l_cos45, g_bit_shift) +
                             multiply(l_alt.y, l_sin45, g_bit_shift);
                    g_l_new_z.y = multiply(l_alt.x, l_sin45, g_bit_shift) -
                             multiply(l_alt.y, l_cos45, g_bit_shift);
                }

                if (g_decomp[0] >= 32)
                {
                    temp <<= 1;
                    if (multiply(g_l_new_z.x, l_tan11_25, g_bit_shift) < g_l_new_z.y)
                    {
                        ++temp;
                        l_alt = g_l_new_z;
                        g_l_new_z.x = multiply(l_alt.x, l_cos22_5, g_bit_shift) +
                                 multiply(l_alt.y, l_sin22_5, g_bit_shift);
                        g_l_new_z.y = multiply(l_alt.x, l_sin22_5, g_bit_shift) -
                                 multiply(l_alt.y, l_cos22_5, g_bit_shift);
                    }

                    if (g_decomp[0] >= 64)
                    {
                        temp <<= 1;
                        if (multiply(g_l_new_z.x, l_tan5_625, g_bit_shift) < g_l_new_z.y)
                        {
                            ++temp;
                            l_alt = g_l_new_z;
                            g_l_new_z.x = multiply(l_alt.x, l_cos11_25, g_bit_shift) +
                                     multiply(l_alt.y, l_sin11_25, g_bit_shift);
                            g_l_new_z.y = multiply(l_alt.x, l_sin11_25, g_bit_shift) -
                                     multiply(l_alt.y, l_cos11_25, g_bit_shift);
                        }

                        if (g_decomp[0] >= 128)
                        {
                            temp <<= 1;
                            if (multiply(g_l_new_z.x, l_tan2_8125, g_bit_shift) < g_l_new_z.y)
                            {
                                ++temp;
                                l_alt = g_l_new_z;
                                g_l_new_z.x = multiply(l_alt.x, l_cos5_625, g_bit_shift) +
                                         multiply(l_alt.y, l_sin5_625, g_bit_shift);
                                g_l_new_z.y = multiply(l_alt.x, l_sin5_625, g_bit_shift) -
                                         multiply(l_alt.y, l_cos5_625, g_bit_shift);
                            }

                            if (g_decomp[0] == 256)
                            {
                                temp <<= 1;
                                if (multiply(g_l_new_z.x, l_tan1_4063, g_bit_shift) < g_l_new_z.y)
                                {
                                    if ((g_l_new_z.x*l_tan1_4063 < g_l_new_z.y))
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
            DComplex alt;
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
int potential(double mag, long iterations)
{
    float pot;
    long l_pot;

    if (iterations < g_max_iterations)
    {
        float f_tmp;
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
                float f_mag = (float) mag;
                f_log14(f_mag, f_tmp); // this SHOULD be non-negative
                f_shift(f_tmp, (char)-l_pot, pot);
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
                f_sqrt14(pot, f_tmp);
                pot = f_tmp;
            }
            pot = (float)(g_potential_params[0] - pot*g_potential_params[1] - 1.0);
        }
        else
        {
            pot = (float)(g_potential_params[0] - 1.0);
        }
        pot = std::max(pot, 1.0f); // avoid color 0
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
    int i_pot = (int) (l_pot >> 8);
    if (i_pot >= g_colors)
    {
        i_pot = g_colors - 1;
        l_pot = 255;
    }

    if (g_potential_16bit)
    {
        if (!driver_is_disk())   // if putcolor won't be doing it for us
        {
            disk_write_pixel(g_col+g_logical_screen_x_offset, g_row+g_logical_screen_y_offset, i_pot);
        }
        disk_write_pixel(g_col+g_logical_screen_x_offset, g_row+g_screen_y_dots+g_logical_screen_y_offset, (int)l_pot);
    }

    return i_pot;
}

// symmetry plot setup
static bool x_sym_split(int x_axis_row, bool x_axis_between)
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
        if (x_axis_row <= g_yy_start || x_axis_row >= g_yy_stop)
        {
            return true; // axis not in window
        }
        int i = x_axis_row + (x_axis_row - g_yy_start);
        if (x_axis_between)
        {
            ++i;
        }
        if (i > g_yy_stop) // split into 2 pieces, bottom has the symmetry
        {
            if (g_num_work_list >= MAX_CALC_WORK-1)   // no room to split
            {
                return true;
            }
            g_i_y_stop = x_axis_row - (g_yy_stop - x_axis_row);
            if (!x_axis_between)
            {
                --g_i_y_stop;
            }
            add_work_list(g_xx_start, g_xx_stop, g_xx_start, g_i_y_stop+1, g_yy_stop, g_i_y_stop+1, g_work_pass, 0);
            g_yy_stop = g_i_y_stop;
            return true; // tell set_symmetry no sym for current window
        }
        if (i < g_yy_stop) // split into 2 pieces, top has the symmetry
        {
            if (g_num_work_list >= MAX_CALC_WORK-1)   // no room to split
            {
                return true;
            }
            add_work_list(g_xx_start, g_xx_stop, g_xx_start, i+1, g_yy_stop, i+1, g_work_pass, 0);
            g_yy_stop = i;
        }
        g_i_y_stop = x_axis_row;
        g_work_symmetry |= 1;
    }
    g_symmetry = SymmetryType::NONE;
    return false; // tell set_symmetry its a go
}

static bool y_sym_split(int y_axis_col, bool y_axis_between)
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
        if (y_axis_col <= g_xx_start || y_axis_col >= g_xx_stop)
        {
            return true; // axis not in window
        }
        int i = y_axis_col + (y_axis_col - g_xx_start);
        if (y_axis_between)
        {
            ++i;
        }
        if (i > g_xx_stop) // split into 2 pieces, right has the symmetry
        {
            if (g_num_work_list >= MAX_CALC_WORK-1)   // no room to split
            {
                return true;
            }
            g_i_x_stop = y_axis_col - (g_xx_stop - y_axis_col);
            if (!y_axis_between)
            {
                --g_i_x_stop;
            }
            add_work_list(g_i_x_stop+1, g_xx_stop, g_i_x_stop+1, g_yy_start, g_yy_stop, g_yy_start, g_work_pass, 0);
            g_xx_stop = g_i_x_stop;
            return true; // tell set_symmetry no sym for current window
        }
        if (i < g_xx_stop) // split into 2 pieces, left has the symmetry
        {
            if (g_num_work_list >= MAX_CALC_WORK-1)   // no room to split
            {
                return true;
            }
            add_work_list(i+1, g_xx_stop, i+1, g_yy_start, g_yy_stop, g_yy_start, g_work_pass, 0);
            g_xx_stop = i;
        }
        g_i_x_stop = y_axis_col;
        g_work_symmetry |= 2;
    }
    g_symmetry = SymmetryType::NONE;
    return false; // tell set_symmetry its a go
}

static void set_symmetry(SymmetryType sym, bool use_list) // set up proper symmetrical plot functions
{
    int i;
    // pixel number for origin
    bool x_axis_between = false;
    bool y_axis_between = false;         // if axis between 2 pixels, not on one
    bool x_axis_on_screen = false;
    bool y_axis_on_screen = false;
    double f_temp;
    bf_t bft1;
    int saved = 0;
    g_symmetry = SymmetryType::X_AXIS;
    if (g_std_calc_mode == 's' || g_std_calc_mode == 'o')
    {
        return;
    }
    if (sym == SymmetryType::NO_PLOT && g_force_symmetry == SymmetryType::NOT_FORCED)
    {
        g_plot = no_plot;
        return;
    }
    // NOTE: 16-bit potential disables symmetry
    // also any decomp= option and any inversion not about the origin
    // also any rotation other than 180deg and any off-axis stretch
    if (g_bf_math != BFMathType::NONE)
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
    if (sym != SymmetryType::X_AXIS
        && sym != SymmetryType::X_AXIS_NO_PARAM
        && g_inversion[1] != 0.0
        && g_force_symmetry == SymmetryType::NOT_FORCED)
    {
        return;
    }
    if (g_force_symmetry < SymmetryType::NOT_FORCED)
    {
        sym = g_force_symmetry;
    }
    else if (g_force_symmetry == static_cast<SymmetryType>(1000))
    {
        g_force_symmetry = sym;  // for backwards compatibility
    }
    else if (g_outside_color == REAL
        || g_outside_color == IMAG
        || g_outside_color == MULT
        || g_outside_color == SUM
        || g_outside_color == ATAN
        || g_bailout_test == Bailout::MANR
        || g_outside_color == FMOD)
    {
        return;
    }
    else if (g_inside_color == FMODI || g_outside_color == TDIS)
    {
        return;
    }
    bool params_zero = (g_param_z1.x == 0.0 && g_param_z1.y == 0.0 && g_use_init_orbit != InitOrbitMode::VALUE);
    bool params_no_real = (g_param_z1.x == 0.0 && g_use_init_orbit != InitOrbitMode::VALUE);
    bool params_no_imag = (g_param_z1.y == 0.0 && g_use_init_orbit != InitOrbitMode::VALUE);
    switch (g_fractal_type)
    {
    case FractalType::MAN_LAM_FN_FN_L:      // These need only P1 checked.
    case FractalType::MAN_LAM_FN_FN_FP:     // P2 is used for a switch value
    case FractalType::MAN_FN_FN_L:         // These have NOPARM set in fractalp.c,
    case FractalType::MAN_FN_FN_FP:        // but it only applies to P1.
    case FractalType::MANDEL_Z_POWER_FP:   // or P2 is an exponent
    case FractalType::MANDEL_Z_POWER_L:
    case FractalType::MAN_Z_TO_Z_PLUS_Z_PWR_FP:
    case FractalType::MARKS_MANDEL:
    case FractalType::MARKS_MANDEL_FP:
    case FractalType::MARKS_JULIA:
    case FractalType::MARKS_JULIA_FP:
        break;
    case FractalType::FORMULA:  // Check P2, P3, P4 and P5
    case FractalType::FORMULA_FP:
        params_zero = (params_zero && g_params[2] == 0.0 && g_params[3] == 0.0
            && g_params[4] == 0.0 && g_params[5] == 0.0
            && g_params[6] == 0.0 && g_params[7] == 0.0
            && g_params[8] == 0.0 && g_params[9] == 0.0);
        params_no_real = (params_no_real && g_params[2] == 0.0 && g_params[4] == 0.0
            && g_params[6] == 0.0 && g_params[8] == 0.0);
        params_no_imag = (params_no_imag && g_params[3] == 0.0 && g_params[5] == 0.0
            && g_params[7] == 0.0 && g_params[9] == 0.0);
        break;
    default:   // Check P2 for the rest
        params_zero = (params_zero && g_param_z2.x == 0.0 && g_param_z2.y == 0.0);
    }
    int y_axis_col = -1;
    int x_axis_row = -1;
    if (g_bf_math != BFMathType::NONE)
    {
        saved = save_stack();
        bft1    = alloc_stack(g_r_bf_length+2);
        x_axis_on_screen = (sign_bf(g_bf_y_min) != sign_bf(g_bf_y_max));
        y_axis_on_screen = (sign_bf(g_bf_x_min) != sign_bf(g_bf_x_max));
    }
    else
    {
        x_axis_on_screen = (sign(g_y_min) != sign(g_y_max));
        y_axis_on_screen = (sign(g_x_min) != sign(g_x_max));
    }
    if (x_axis_on_screen) // axis is on screen
    {
        if (g_bf_math != BFMathType::NONE)
        {
            sub_bf(bft1, g_bf_y_min, g_bf_y_max);
            div_bf(bft1, g_bf_y_max, bft1);
            neg_a_bf(bft1);
            f_temp = (double)bf_to_float(bft1);
        }
        else
        {
            f_temp = (0.0-g_y_max) / (g_y_min-g_y_max);
        }
        f_temp *= (g_logical_screen_y_dots-1);
        f_temp += 0.25;
        x_axis_row = (int)f_temp;
        x_axis_between = (f_temp - x_axis_row >= 0.5);
        if (!use_list && (!x_axis_between || (x_axis_row+1)*2 != g_logical_screen_y_dots))
        {
            x_axis_row = -1; // can't split screen, so dead center or not at all
        }
    }
    if (y_axis_on_screen) // axis is on screen
    {
        if (g_bf_math != BFMathType::NONE)
        {
            sub_bf(bft1, g_bf_x_max, g_bf_x_min);
            div_bf(bft1, g_bf_x_min, bft1);
            neg_a_bf(bft1);
            f_temp = (double)bf_to_float(bft1);
        }
        else
        {
            f_temp = (0.0-g_x_min) / (g_x_max-g_x_min);
        }
        f_temp *= (g_logical_screen_x_dots-1);
        f_temp += 0.25;
        y_axis_col = (int)f_temp;
        y_axis_between = (f_temp - y_axis_col >= 0.5);
        if (!use_list && (!y_axis_between || (y_axis_col+1)*2 != g_logical_screen_x_dots))
        {
            y_axis_col = -1; // can't split screen, so dead center or not at all
        }
    }
    switch (sym)       // symmetry switch
    {
    case SymmetryType::X_AXIS_NO_REAL:    // X-axis Symmetry (no real param)
        if (!params_no_real)
        {
            break;
        }
        goto xsym;
    case SymmetryType::X_AXIS_NO_IMAG:    // X-axis Symmetry (no imag param)
        if (!params_no_imag)
        {
            break;
        }
        goto xsym;
    case SymmetryType::X_AXIS_NO_PARAM:                        // X-axis Symmetry  (no params)
        if (!params_zero)
        {
            break;
        }
xsym:
    case SymmetryType::X_AXIS:                       // X-axis Symmetry
        if (!x_sym_split(x_axis_row, x_axis_between))
        {
            if (g_basin)
            {
                g_plot = sym_plot2_basin;
            }
            else
            {
                g_plot = sym_plot2;
            }
        }
        break;
    case SymmetryType::Y_AXIS_NO_PARAM:                        // Y-axis Symmetry (No Parms)
        if (!params_zero)
        {
            break;
        }
    case SymmetryType::Y_AXIS:                       // Y-axis Symmetry
        if (!y_sym_split(y_axis_col, y_axis_between))
        {
            g_plot = sym_plot2y;
        }
        break;
    case SymmetryType::XY_AXIS_NO_PARAM:                       // X-axis AND Y-axis Symmetry (no parms)
        if (!params_zero)
        {
            break;
        }
    case SymmetryType::XY_AXIS:                      // X-axis AND Y-axis Symmetry
        x_sym_split(x_axis_row, x_axis_between);
        y_sym_split(y_axis_col, y_axis_between);
        switch (g_work_symmetry & 3)
        {
        case 1: // just xaxis symmetry
            if (g_basin)
            {
                g_plot = sym_plot2_basin;
            }
            else
            {
                g_plot = sym_plot2 ;
            }
            break;
        case 2: // just yaxis symmetry
            if (g_basin) // got no routine for this case
            {
                g_i_x_stop = g_xx_stop; // fix what split should not have done
                g_symmetry = SymmetryType::X_AXIS;
            }
            else
            {
                g_plot = sym_plot2y;
            }
            break;
        case 3: // both axes
            if (g_basin)
            {
                g_plot = sym_plot4_basin;
            }
            else
            {
                g_plot = sym_plot4 ;
            }
        }
        break;
    case SymmetryType::ORIGIN_NO_PARAM:                       // Origin Symmetry (no parms)
        if (!params_zero)
        {
            break;
        }
    case SymmetryType::ORIGIN:                      // Origin Symmetry
originsym:
        if (!x_sym_split(x_axis_row, x_axis_between)
            && !y_sym_split(y_axis_col, y_axis_between))
        {
            g_plot = sym_plot2j;
            g_i_x_stop = g_xx_stop; // didn't want this changed
        }
        else
        {
            g_i_y_stop = g_yy_stop; // in case first split worked
            g_symmetry = SymmetryType::X_AXIS;
            g_work_symmetry = 0x30; // let it recombine with others like it
        }
        break;
    case SymmetryType::PI_SYM_NO_PARAM:
        if (!params_zero)
        {
            break;
        }
    case SymmetryType::PI_SYM:                      // PI symmetry
        if (g_bf_math != BFMathType::NONE)
        {
            if ((double)bf_to_float(abs_a_bf(sub_bf(bft1, g_bf_x_max, g_bf_x_min))) < PI/4)
            {
                break; // no point in pi symmetry if values too close
            }
        }
        else
        {
            if (std::abs(g_x_max - g_x_min) < PI/4)
            {
                break; // no point in pi symmetry if values too close
            }
        }
        if ((g_invert != 0) && g_force_symmetry == SymmetryType::NOT_FORCED)
        {
            goto originsym;
        }
        g_plot = sym_pi_plot ;
        g_symmetry = SymmetryType::NONE;
        if (!x_sym_split(x_axis_row, x_axis_between)
            && !y_sym_split(y_axis_col, y_axis_between))
        {
            if (g_param_z1.y == 0.0)
            {
                g_plot = sym_pi_plot4j; // both axes
            }
            else
            {
                g_plot = sym_pi_plot2j; // origin
            }
        }
        else
        {
            g_i_y_stop = g_yy_stop; // in case first split worked
            g_work_symmetry = 0x30;  // don't mark pisym as ysym, just do it unmarked
        }
        if (g_bf_math != BFMathType::NONE)
        {
            sub_bf(bft1, g_bf_x_max, g_bf_x_min);
            abs_a_bf(bft1);
            g_pi_in_pixels = (int)((PI/(double)bf_to_float(bft1)*g_logical_screen_x_dots)); // PI in pixels
        }
        else
        {
            g_pi_in_pixels = (int)((PI/std::abs(g_x_max-g_x_min))*g_logical_screen_x_dots); // PI in pixels
        }

        g_i_x_stop = g_xx_start + g_pi_in_pixels - 1;
        g_i_x_stop = std::min(g_i_x_stop, g_xx_stop);
        i = (g_xx_start+g_xx_stop)/2;
        if (g_plot == sym_pi_plot4j && g_i_x_stop > i)
        {
            g_i_x_stop = i;
        }
        break;
    default:                  // no symmetry
        break;
    }
    if (g_bf_math != BFMathType::NONE)
    {
        restore_stack(saved);
    }
}

// added for testing autologmap()
// insert at end of CALCFRAC.C

static long auto_log_map()
{
    // calculate round screen edges to avoid wasted colours in logmap
    int x_stop = g_logical_screen_x_dots - 1; // don't use symmetry
    int y_stop = g_logical_screen_y_dots - 1; // don't use symmetry
    long min_color = LONG_MAX;
    g_row = 0;
    g_reset_periodicity = false;
    long old_max_it = g_max_iterations;
    for (g_col = 0; g_col < x_stop; g_col++) // top row
    {
        g_color = (*g_calc_type)();
        if (g_color == -1)
        {
            goto ack; // key pressed, bailout
        }
        if (g_real_color_iter < min_color)
        {
            min_color = g_real_color_iter ;
            g_max_iterations = std::max(2L, min_color); // speedup for when edges overlap lakes
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

    g_col = x_stop;
    for (g_row = 0; g_row < y_stop; g_row++) // right  side
    {
        g_color = (*g_calc_type)();
        if (g_color == -1)
        {
            goto ack; // key pressed, bailout
        }
        if (g_real_color_iter < min_color)
        {
            min_color = g_real_color_iter ;
            g_max_iterations = std::max(2L, min_color); // speedup for when edges overlap lakes
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
    for (g_row = 0; g_row < y_stop; g_row++) // left  side
    {
        g_color = (*g_calc_type)();
        if (g_color == -1)
        {
            goto ack; // key pressed, bailout
        }
        if (g_real_color_iter < min_color)
        {
            min_color = g_real_color_iter ;
            g_max_iterations = std::max(2L, min_color); // speedup for when edges overlap lakes
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

    g_row = y_stop ;
    for (g_col = 0; g_col < x_stop; g_col++) // bottom row
    {
        g_color = (*g_calc_type)();
        if (g_color == -1)
        {
            goto ack; // key pressed, bailout
        }
        if (g_real_color_iter < min_color)
        {
            min_color = g_real_color_iter ;
            g_max_iterations = std::max(2L, min_color); // speedup for when edges overlap lakes
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
    if (min_color == 2)      // insure autologmap not called again
    {
        g_resuming = true;
    }
    g_max_iterations = old_max_it;

    return min_color ;
}

// Symmetry plot for period PI
void sym_pi_plot(int x, int y, int color)
{
    while (x <= g_xx_stop)
    {
        g_put_color(x, y, color) ;
        x += g_pi_in_pixels;
    }
}

// Symmetry plot for period PI plus Origin Symmetry
void sym_pi_plot2j(int x, int y, int color)
{
    int j;
    while (x <= g_xx_stop)
    {
        g_put_color(x, y, color) ;
        int i = g_yy_stop - (y - g_yy_start);
        if (i > g_i_y_stop && i < g_logical_screen_y_dots
            && (j = g_xx_stop-(x-g_xx_start)) < g_logical_screen_x_dots)
        {
            g_put_color(j, i, color) ;
        }
        x += g_pi_in_pixels;
    }
}

// Symmetry plot for period PI plus Both Axis Symmetry
void sym_pi_plot4j(int x, int y, int color)
{
    while (x <= (g_xx_start+g_xx_stop)/2)
    {
        int j = g_xx_stop - (x - g_xx_start);
        g_put_color(x , y , color) ;
        if (j < g_logical_screen_x_dots)
        {
            g_put_color(j , y , color) ;
        }
        int i = g_yy_stop - (y - g_yy_start);
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
void sym_plot2(int x, int y, int color)
{
    g_put_color(x, y, color) ;
    int i = g_yy_stop - (y - g_yy_start);
    if (i > g_i_y_stop && i < g_logical_screen_y_dots)
    {
        g_put_color(x, i, color) ;
    }
}

// Symmetry plot for Y Axis Symmetry
void sym_plot2y(int x, int y, int color)
{
    g_put_color(x, y, color) ;
    int i = g_xx_stop - (x - g_xx_start);
    if (i < g_logical_screen_x_dots)
    {
        g_put_color(i, y, color) ;
    }
}

// Symmetry plot for Origin Symmetry
void sym_plot2j(int x, int y, int color)
{
    int j;
    g_put_color(x, y, color) ;
    int i = g_yy_stop - (y - g_yy_start);
    if (i > g_i_y_stop && i < g_logical_screen_y_dots
        && (j = g_xx_stop-(x-g_xx_start)) < g_logical_screen_x_dots)
    {
        g_put_color(j, i, color) ;
    }
}

// Symmetry plot for Both Axis Symmetry
void sym_plot4(int x, int y, int color)
{
    int j = g_xx_stop - (x - g_xx_start);
    g_put_color(x , y, color) ;
    if (j < g_logical_screen_x_dots)
    {
        g_put_color(j , y, color) ;
    }
    int i = g_yy_stop - (y - g_yy_start);
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
void sym_plot2_basin(int x, int y, int color)
{
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
    int i = g_yy_stop - (y - g_yy_start);
    if (i > g_i_y_stop && i < g_logical_screen_y_dots)
    {
        color -= stripe;                    // reconstruct unstriped color
        color = (g_degree+1-color)%g_degree+1;  // symmetrical color
        color += stripe;                    // add stripe
        g_put_color(x, i, color)  ;
    }
}

// Symmetry plot for Both Axis Symmetry  - Newtbasin version
void sym_plot4_basin(int x, int y, int color)
{
    int color1;
    int stripe;
    if (color == 0) // assumed to be "inside" color
    {
        sym_plot4(x, y, color);
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
    int j = g_xx_stop - (x - g_xx_start);
    g_put_color(x, y, color+stripe) ;
    if (j < g_logical_screen_x_dots)
    {
        g_put_color(j, y, color1+stripe) ;
    }
    int i = g_yy_stop - (y - g_yy_start);
    if (i > g_i_y_stop && i < g_logical_screen_y_dots)
    {
        g_put_color(x, i, stripe + (g_degree+1 - color)%g_degree+1) ;
        if (j < g_logical_screen_x_dots)
        {
            g_put_color(j, i, stripe + (g_degree+1 - color1)%g_degree+1) ;
        }
    }
}

static void put_true_color_disk(int x, int y, int color)
{
    put_color_a(x, y, color);
    targa_color(x, y, color);
}

// Do nothing plot!!!
void no_plot(int, int, int)
{
}