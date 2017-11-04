/*
CALCFRAC.C contains the high level ("engine") code for calculating the
fractal images (well, SOMEBODY had to do it!).
The following modules work very closely with CALCFRAC.C:
  FRACTALS.C    the fractal-specific code for escape-time fractals.
  FRACSUBR.C    assorted subroutines belonging mainly to calcfrac.
  CALCMAND.ASM  fast Mandelbrot/Julia integer implementation
Additional fractal-specific modules are also invoked from CALCFRAC:
  LORENZ.C      engine level and fractal specific code for attractors.
  JB.C          julibrot logic
  PARSER.C      formula fractals
  and more
 -------------------------------------------------------------------- */
#include <algorithm>
#include <vector>

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// see Fractint.c for a description of the "include"  hierarchy
#include "port.h"
#include "prototyp.h"
#include "fractype.h"
#include "targa_lc.h"
#include "drivers.h"

// routines in this module
static void perform_worklist();
static int  one_or_two_pass();
static int  standard_calc(int);
static int  potential(double, long);
static void decomposition();
static int  bound_trace_main();
static void step_col_row();
static int  solid_guess();
static bool guessrow(bool firstpass, int y, int blocksize);
static void plotblock(int, int, int, int);
static void setsymmetry(symmetry_type sym, bool uselist);
static bool xsym_split(int xaxis_row, bool xaxis_between);
static bool ysym_split(int yaxis_col, bool yaxis_between);
static void put_truecolor_disk(int, int, int);
static int diffusion_engine();
static int sticky_orbits();

static int tesseral();
static int tesschkcol(int, int, int);
static int tesschkrow(int, int, int);
static int tesscol(int, int, int);
static int tessrow(int, int, int);

static int diffusion_scan();

// lookup tables to avoid too much bit fiddling :
static char dif_la[] =
{
    0, 8, 0, 8,4,12,4,12,0, 8, 0, 8,4,12,4,12, 2,10, 2,10,6,14,6,14,2,10,
    2,10, 6,14,6,14,0, 8,0, 8, 4,12,4,12,0, 8, 0, 8, 4,12,4,12,2,10,2,10,
    6,14, 6,14,2,10,2,10,6,14, 6,14,1, 9,1, 9, 5,13, 5,13,1, 9,1, 9,5,13,
    5,13, 3,11,3,11,7,15,7,15, 3,11,3,11,7,15, 7,15, 1, 9,1, 9,5,13,5,13,
    1, 9, 1, 9,5,13,5,13,3,11, 3,11,7,15,7,15, 3,11, 3,11,7,15,7,15,0, 8,
    0, 8, 4,12,4,12,0, 8,0, 8, 4,12,4,12,2,10, 2,10, 6,14,6,14,2,10,2,10,
    6,14, 6,14,0, 8,0, 8,4,12, 4,12,0, 8,0, 8, 4,12, 4,12,2,10,2,10,6,14,
    6,14, 2,10,2,10,6,14,6,14, 1, 9,1, 9,5,13, 5,13, 1, 9,1, 9,5,13,5,13,
    3,11, 3,11,7,15,7,15,3,11, 3,11,7,15,7,15, 1, 9, 1, 9,5,13,5,13,1, 9,
    1, 9, 5,13,5,13,3,11,3,11, 7,15,7,15,3,11, 3,11, 7,15,7,15
};

static char dif_lb[] =
{
    0, 8, 8, 0, 4,12,12, 4, 4,12,12, 4, 8, 0, 0, 8, 2,10,10, 2, 6,14,14,
    6, 6,14,14, 6,10, 2, 2,10, 2,10,10, 2, 6,14,14, 6, 6,14,14, 6,10, 2,
    2,10, 4,12,12, 4, 8, 0, 0, 8, 8, 0, 0, 8,12, 4, 4,12, 1, 9, 9, 1, 5,
    13,13, 5, 5,13,13, 5, 9, 1, 1, 9, 3,11,11, 3, 7,15,15, 7, 7,15,15, 7,
    11, 3, 3,11, 3,11,11, 3, 7,15,15, 7, 7,15,15, 7,11, 3, 3,11, 5,13,13,
    5, 9, 1, 1, 9, 9, 1, 1, 9,13, 5, 5,13, 1, 9, 9, 1, 5,13,13, 5, 5,13,
    13, 5, 9, 1, 1, 9, 3,11,11, 3, 7,15,15, 7, 7,15,15, 7,11, 3, 3,11, 3,
    11,11, 3, 7,15,15, 7, 7,15,15, 7,11, 3, 3,11, 5,13,13, 5, 9, 1, 1, 9,
    9, 1, 1, 9,13, 5, 5,13, 2,10,10, 2, 6,14,14, 6, 6,14,14, 6,10, 2, 2,
    10, 4,12,12, 4, 8, 0, 0, 8, 8, 0, 0, 8,12, 4, 4,12, 4,12,12, 4, 8, 0,
    0, 8, 8, 0, 0, 8,12, 4, 4,12, 6,14,14, 6,10, 2, 2,10,10, 2, 2,10,14,
    6, 6,14
};

// added for testing autologmap()
static long autologmap();

// variables exported from this file
LComplex g_l_init_orbit = { 0 };
long g_l_magnitude = 0;
long g_l_limit = 0;
long g_l_limit2 = 0;
long g_l_close_enough = 0;
DComplex g_init = { 0.0 };
DComplex tmp = { 0.0 };
DComplex old = { 0.0 };
DComplex g_new = { 0.0 };
DComplex saved = { 0.0 };
int g_color = 0;
long g_color_iter = 0;
long oldcoloriter = 0;
long realcoloriter = 0;
int row = 0;
int col = 0;
int passes = 0;
int g_invert = 0;
double g_f_radius = 0.0;
double g_f_x_center = 0.0;
double g_f_y_center = 0.0;                 // for inversion
void (*putcolor)(int, int, int) = putcolor_a;
void (*plot)(int, int, int) = putcolor_a;

double magnitude = 0.0;
double rqlim = 0.0;
double rqlim2 = 0.0;
double rqlim_save = 0.0;
bool no_mag_calc = false;
bool use_old_period = false;
bool use_old_distest = false;
bool old_demm_colors = false;
int (*calctype)() = nullptr;
int (*calctypetmp)() = nullptr;
bool quick_calc = false;
double g_close_proximity = 0.01;

double g_close_enough = 0.0;
int pixelpi = 0;                        // value of pi in pixels
unsigned long lm = 0;                   // magnitude limit (CALCMAND)

// ORBIT variables
bool show_orbit = false;                // flag to turn on and off
int     orbit_ptr = 0;                  // pointer into save_orbit array
int     orbit_color = 15;                 // XOR color

int g_i_x_start = 0;
int g_i_x_stop = 0;
int g_i_y_start = 0;
int g_i_y_stop = 0;                         // start, stop here
symmetry_type symmetry = symmetry_type::NONE; // symmetry flag
bool reset_periodicity = false;         // true if escape time pixel rtn to reset
int g_keyboard_check_interval = 0;
int max_keyboard_check_interval = 0;                   // avoids checking keyboard too often

std::vector<BYTE> resume_data;          // resume info
bool resuming = false;                  // true if resuming after interrupt
int num_worklist = 0;                   // resume worklist for standard engine
WORKLIST worklist[MAXCALCWORK] = { 0 };
int xxstart = 0;
int xxstop = 0;
int xxbegin = 0;                        // these are same as worklist,
int yystart = 0;
int yystop = 0;
int yybegin = 0;                        // declared as separate items
int workpass = 0;
int worksym = 0;                        // for the sake of calcmand

static double dem_delta = 0.0;
static double dem_width = 0.0;          // distance estimator variables
static double dem_toobig = 0.0;
static bool dem_mandel = false;
#define DEM_BAILOUT 535.5

// variables which must be visible for tab_display
int g_got_status = -1;                    // -1 if not, 0 for 1or2pass, 1 for ssg,
                                        // 2 for btm, 3 for 3d, 4 for tesseral, 5 for diffusion_scan
                                        // 6 for orbits
int g_current_pass = 0;
int totpasses = 0;
int g_current_row = 0;
int g_current_column = 0;

// static vars for diffusion scan
unsigned g_diffusion_bits = 0;        // number of bits in the counter
unsigned long g_diffusion_counter = 0;  // the diffusion counter
unsigned long g_diffusion_limit = 0;    // the diffusion counter

// static vars for solid_guess & its subroutines
bool three_pass = false;
static int maxblock = 0;
static int halfblock = 0;
static bool guessplot = false;          // paint 1st pass row at a time?
static bool right_guess = false;
static bool bottom_guess = false;
#define maxyblk 7    // maxxblk*maxyblk*2 <= 4096, the size of "prefix"
#define maxxblk 202  // each maxnblk is oversize by 2 for a "border"
// maxxblk defn must match fracsubr.c
/* next has a skip bit for each maxblock unit;
   1st pass sets bit  [1]... off only if block's contents guessed;
   at end of 1st pass [0]... bits are set if any surrounding block not guessed;
   bits are numbered [..][y/16+1][x+1]&(1<<(y&15)) */

// size of next puts a limit of MAXPIXELS pixels across on solid guessing logic
namespace
{
BYTE dstack[4096] = { 0 };              // common temp, two put_line calls
}
unsigned int tprefix[2][maxyblk][maxxblk] = { 0 }; // common temp

bool nxtscreenflag = false;             // for cellular next screen generation
int g_attractors = 0;                     // number of finite attractors
DComplex g_attractor[MAX_NUM_ATTRACTORS] = { 0.0 };        // finite attractor vals (f.p)
LComplex g_l_attractor[MAX_NUM_ATTRACTORS] = { 0 };         // finite attractor vals (int)
int g_attractor_period[MAX_NUM_ATTRACTORS] = { 0 };         // period of the finite attractor

/***** vars for new btm *****/
enum class direction
{
    North,
    East,
    South,
    West
};
direction going_to;
int trail_row = 0;
int trail_col = 0;

// --------------------------------------------------------------------
//              These variables are external for speed's sake only
// --------------------------------------------------------------------

int periodicitycheck = 0;

// For periodicity testing, only in standard_fractal()
int nextsavedincr = 0;
long firstsavedand = 0;

static std::vector<BYTE> savedots;
static BYTE *fillbuff = nullptr;
static int savedotslen = 0;
static int showdotcolor = 0;
int atan_colors = 180;

static int showdot_width = 0;

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

int g_and_color = 0;        // "and" value used for color selection

/*
;
;       32-bit integer divide routine with an 'n'-bit shift.
;       Overflow condition returns 0x7fffh with overflow = 1;
;
;       z = divide(x, y, n);       z = x / y;
*/
long divide(long x, long y, int n)
{
    return (long)(((float) x) / ((float) y)*(float)(1 << n));
}

/*
 * 32 bit integer multiply with n bit shift.
 * Note that we fake integer multiplication with floating point
 * multiplication.
 * Overflow condition returns 0x7fffffffh with overflow = 1;
 */
long multiply(long x, long y, int n)
{
    long l = (long)(((float) x) * ((float) y)/(float)(1 << n));
    if (l == 0x7fffffff)
    {
        overflow = true;
    }
    return l;
}


double fmodtest_bailout_or()
{
    double const tmpx = sqr(g_new.x);
    double const tmpy = sqr(g_new.y);
    if (tmpx > tmpy)
    {
        return tmpx;
    }


    return tmpy;

}

// FMODTEST routine.
/* Makes the test condition for the FMOD coloring type
   that of the current bailout method. 'or' and 'and'
   methods are not used - in these cases a normal
   modulus test is used                              */

double fmodtest()
{
    double result;
    if (g_inside == FMODI && save_release <= 2000) // for backwards compatibility
    {
        if (magnitude == 0.0 || !no_mag_calc || g_integer_fractal)
        {
            result = sqr(g_new.x)+sqr(g_new.y);
        }
        else
        {
            result = magnitude; // don't recalculate
        }
        return result;
    }

    switch (g_bail_out_test)
    {
    case bailouts::Mod:
        if (magnitude == 0.0 || !no_mag_calc || g_integer_fractal)
        {
            result = sqr(g_new.x)+sqr(g_new.y);
        }
        else
        {
            result = magnitude; // don't recalculate
        }
        break;

    case bailouts::Real:
        result = sqr(g_new.x);
        break;

    case bailouts::Imag:
        result = sqr(g_new.y);
        break;

    case bailouts::Or:
        result = fmodtest_bailout_or();
        break;

    case bailouts::Manh:
        result = sqr(fabs(g_new.x)+fabs(g_new.y));
        break;

    case bailouts::Manr:
        result = sqr(g_new.x+g_new.y);
        break;

    default:
        result = sqr(g_new.x)+sqr(g_new.y);
        break;
    }

    return result;
}

/*
   The sym_fill_line() routine was pulled out of the boundary tracing
   code for re-use with show_dot. It's purpose is to fill a line with a
   solid color. This assumes that BYTE *str is already filled
   with the color. The routine does write the line using symmetry
   in all cases, however the symmetry logic assumes that the line
   is one color; it is not general enough to handle a row of
   pixels of different colors.
*/
static void sym_fill_line(int row, int left, int right, BYTE *str)
{
    int length;
    length = right-left+1;
    put_line(row, left, right, str);
    // here's where all the symmetry goes
    if (plot == putcolor)
    {
        g_keyboard_check_interval -= length >> 4; // seems like a reasonable value
    }
    else if (plot == symplot2)   // X-axis symmetry
    {
        int i = yystop-(row-yystart);
        if (i > g_i_y_stop && i < ydots)
        {
            put_line(i, left, right, str);
            g_keyboard_check_interval -= length >> 3;
        }
    }
    else if (plot == symplot2Y) // Y-axis symmetry
    {
        put_line(row, xxstop-(right-xxstart), xxstop-(left-xxstart), str);
        g_keyboard_check_interval -= length >> 3;
    }
    else if (plot == symplot2J)  // Origin symmetry
    {
        int i = yystop-(row-yystart);
        int j = std::min(xxstop-(right-xxstart), xdots-1);
        int k = std::min(xxstop-(left -xxstart), xdots-1);
        if (i > g_i_y_stop && i < ydots && j <= k)
        {
            put_line(i, j, k, str);
        }
        g_keyboard_check_interval -= length >> 3;
    }
    else if (plot == symplot4) // X-axis and Y-axis symmetry
    {
        int i = yystop-(row-yystart);
        int j = std::min(xxstop-(right-xxstart), xdots-1);
        int k = std::min(xxstop-(left -xxstart), xdots-1);
        if (i > g_i_y_stop && i < ydots)
        {
            put_line(i, left, right, str);
            if (j <= k)
            {
                put_line(i, j, k, str);
            }
        }
        if (j <= k)
        {
            put_line(row, j, k, str);
        }
        g_keyboard_check_interval -= length >> 2;
    }
    else    // cheap and easy way out
    {
        for (int i = left; i <= right; i++)
        {
            (*plot)(i, row, str[i-left]);
        }
        g_keyboard_check_interval -= length >> 1;
    }
}

/*
  The sym_put_line() routine is the symmetry-aware version of put_line().
  It only works efficiently in the no symmetry or X_AXIS symmetry case,
  otherwise it just writes the pixels one-by-one.
*/
static void sym_put_line(int row, int left, int right, BYTE *str)
{
    int length = right-left+1;
    put_line(row, left, right, str);
    if (plot == putcolor)
    {
        g_keyboard_check_interval -= length >> 4; // seems like a reasonable value
    }
    else if (plot == symplot2)   // X-axis symmetry
    {
        int i = yystop-(row-yystart);
        if (i > g_i_y_stop && i < ydots)
        {
            put_line(i, left, right, str);
        }
        g_keyboard_check_interval -= length >> 3;
    }
    else
    {
        for (int i = left; i <= right; i++)
        {
            (*plot)(i, row, str[i-left]);
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
        if (savedots.empty())
        {
            stopmsg(STOPMSG_NONE, "savedots empty");
            exit(0);
        }
        if (fillbuff == nullptr)
        {
            stopmsg(STOPMSG_NONE, "fillbuff NULL");
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
                get_line(j, startx, stopx, &savedots[0] + ct);
                sym_fill_line(j, startx, stopx, fillbuff);
            }
            else
            {
                sym_put_line(j, startx, stopx, &savedots[0] + ct);
            }
            ct += stopx-startx+1;
        }
        break;
    case show_dot_direction::UPPER_RIGHT:
        for (int j = starty; j >= stopy; startx++, j--)
        {
            if (action == show_dot_action::SAVE)
            {
                get_line(j, startx, stopx, &savedots[0] + ct);
                sym_fill_line(j, startx, stopx, fillbuff);
            }
            else
            {
                sym_put_line(j, startx, stopx, &savedots[0] + ct);
            }
            ct += stopx-startx+1;
        }
        break;
    case show_dot_direction::LOWER_LEFT:
        for (int j = starty; j <= stopy; stopx--, j++)
        {
            if (action == show_dot_action::SAVE)
            {
                get_line(j, startx, stopx, &savedots[0] + ct);
                sym_fill_line(j, startx, stopx, fillbuff);
            }
            else
            {
                sym_put_line(j, startx, stopx, &savedots[0] + ct);
            }
            ct += stopx-startx+1;
        }
        break;
    case show_dot_direction::UPPER_LEFT:
        for (int j = starty; j >= stopy; stopx--, j--)
        {
            if (action == show_dot_action::SAVE)
            {
                get_line(j, startx, stopx, &savedots[0] + ct);
                sym_fill_line(j, startx, stopx, fillbuff);
            }
            else
            {
                sym_put_line(j, startx, stopx, &savedots[0] + ct);
            }
            ct += stopx-startx+1;
        }
        break;
    case show_dot_direction::JUST_A_POINT:
        break;
    }
    if (action == show_dot_action::SAVE)
    {
        (*plot)(col, row, showdotcolor);
    }
}

int calctypeshowdot()
{
    int out, startx, starty, stopx, stopy, width;
    show_dot_direction direction = show_dot_direction::JUST_A_POINT;
    stopx = col;
    startx = stopx;
    stopy = row;
    starty = stopy;
    width = showdot_width+1;
    if (width > 0)
    {
        if (col+width <= g_i_x_stop && row+width <= g_i_y_stop)
        {
            // preferred show_dot shape
            direction = show_dot_direction::UPPER_LEFT;
            startx = col;
            stopx  = col+width;
            starty = row+width;
            stopy  = row+1;
        }
        else if (col-width >= g_i_x_start && row+width <= g_i_y_stop)
        {
            // second choice
            direction = show_dot_direction::UPPER_RIGHT;
            startx = col-width;
            stopx  = col;
            starty = row+width;
            stopy  = row+1;
        }
        else if (col-width >= g_i_x_start && row-width >= g_i_y_start)
        {
            direction = show_dot_direction::LOWER_RIGHT;
            startx = col-width;
            stopx  = col;
            starty = row-width;
            stopy  = row-1;
        }
        else if (col+width <= g_i_x_stop && row-width >= g_i_y_start)
        {
            direction = show_dot_direction::LOWER_LEFT;
            startx = col;
            stopx  = col+width;
            starty = row-width;
            stopy  = row-1;
        }
    }
    showdotsaverestore(startx, stopx, starty, stopy, direction, show_dot_action::SAVE);
    if (orbit_delay > 0)
    {
        sleepms(orbit_delay);
    }
    out = (*calctypetmp)();
    showdotsaverestore(startx, stopx, starty, stopy, direction, show_dot_action::RESTORE);
    return out;
}

/******* calcfract - the top level routine for generating an image *******/

int calcfract()
{
    g_attractors = 0;          // default to no known finite attractors
    g_display_3d = display_3d_modes::NONE;
    g_basin = 0;
    putcolor = putcolor_a;
    if (g_is_true_color && truemode)
    {
        // Have to force passes = 1
        stdcalcmode = '1';
        usr_stdcalcmode = stdcalcmode;
    }
    if (truecolor)
    {
        check_writefile(g_light_name, ".tga");
        if (!startdisk1(g_light_name.c_str(), nullptr, false))
        {
            // Have to force passes = 1
            stdcalcmode = '1';
            usr_stdcalcmode = stdcalcmode;
            putcolor = put_truecolor_disk;
        }
        else
        {
            truecolor = false;
        }
    }
    if (!use_grid)
    {
        if (usr_stdcalcmode != 'o')
        {
            stdcalcmode = '1';
            usr_stdcalcmode = stdcalcmode;
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
    parm.x   = param[0];
    parm.y   = param[1];
    parm2.x  = param[2];
    parm2.y  = param[3];

    if (g_log_map_flag && g_colors < 16)
    {
        stopmsg(STOPMSG_NONE, "Need at least 16 colors to use logmap");
        g_log_map_flag = 0;
    }

    if (use_old_period)
    {
        nextsavedincr = 1;
        firstsavedand = 1;
    }
    else
    {
        nextsavedincr = (int)log10(static_cast<double>(maxit)); // works better than log()
        if (nextsavedincr < 4)
        {
            nextsavedincr = 4; // maintains image with low iterations
        }
        firstsavedand = (long)((nextsavedincr*2) + 1);
    }

    g_log_map_table.clear();
    MaxLTSize = maxit;
    g_log_map_calculate = false;
    // below, INT_MAX = 32767 only when an integer is two bytes.  Which is not true for Xfractint.
    // Since 32767 is what was meant, replaced the instances of INT_MAX with 32767.
    if (g_log_map_flag && (((maxit > 32767) && (save_release > 1920))
                    || g_log_map_fly_calculate == 1))
    {
        g_log_map_calculate = true; // calculate on the fly
        SetupLogTable();
    }
    else if (g_log_map_flag && (((maxit > 32767) && (save_release <= 1920))
                         || g_log_map_fly_calculate == 2))
    {
        MaxLTSize = 32767;
        g_log_map_calculate = false; // use logtable
    }
    else if (rangeslen && (maxit >= 32767))
    {
        MaxLTSize = 32766;
    }

    if ((g_log_map_flag || rangeslen) && !g_log_map_calculate)
    {
        bool resized = false;
        try
        {
            g_log_map_table.resize(MaxLTSize + 1);
            resized = true;
        }
        catch (std::bad_alloc const&)
        {
        }

        if (!resized)
        {
            if (rangeslen || g_log_map_fly_calculate == 2)
            {
                stopmsg(STOPMSG_NONE, "Insufficient memory for logmap/ranges with this maxiter");
            }
            else
            {
                stopmsg(STOPMSG_NONE, "Insufficient memory for logTable, using on-the-fly routine");
                g_log_map_fly_calculate = 1;
                g_log_map_calculate = true; // calculate on the fly
                SetupLogTable();
            }
        }
        else if (rangeslen)
        {
            // Can't do ranges if MaxLTSize > 32767
            int m;
            int numval;
            int flip;
            int altern;
            int l = 0;
            int k = l;
            int i = k;
            g_log_map_flag = 0; // ranges overrides logmap
            while (i < rangeslen)
            {
                flip = 0;
                m = flip;
                altern = 32767;
                numval = ranges[i++];
                if (numval < 0)
                {
                    altern = ranges[i++];    // sub-range iterations
                    numval = ranges[i++];
                }
                if (numval > (int)MaxLTSize || i >= rangeslen)
                {
                    numval = (int)MaxLTSize;
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
    lm = 4L << bitshift;                 // CALCMAND magnitude limit

    if (save_release > 2002)
    {
        atan_colors = g_colors;
    }
    else
    {
        atan_colors = 180;
    }

    // ORBIT stuff
    show_orbit = start_show_orbit;
    orbit_ptr = 0;
    orbit_color = 15;
    if (g_colors < 16)
    {
        orbit_color = 1;
    }

    if (g_inversion[0] != 0.0)
    {
        g_f_radius    = g_inversion[0];
        g_f_x_center   = g_inversion[1];
        g_f_y_center   = g_inversion[2];

        if (g_inversion[0] == AUTOINVERT)  //  auto calc radius 1/6 screen
        {
            g_inversion[0] = std::min(fabs(xxmax - xxmin),
                                    fabs(yymax - yymin)) / 6.0;
            fix_inversion(&g_inversion[0]);
            g_f_radius = g_inversion[0];
        }

        if (g_invert < 2 || g_inversion[1] == AUTOINVERT)  // xcenter not already set
        {
            g_inversion[1] = (xxmin + xxmax) / 2.0;
            fix_inversion(&g_inversion[1]);
            g_f_x_center = g_inversion[1];
            if (fabs(g_f_x_center) < fabs(xxmax-xxmin) / 100)
            {
                g_f_x_center = 0.0;
                g_inversion[1] = g_f_x_center;
            }
        }

        if (g_invert < 3 || g_inversion[2] == AUTOINVERT)  // ycenter not already set
        {
            g_inversion[2] = (yymin + yymax) / 2.0;
            fix_inversion(&g_inversion[2]);
            g_f_y_center = g_inversion[2];
            if (fabs(g_f_y_center) < fabs(yymax-yymin) / 100)
            {
                g_f_y_center = 0.0;
                g_inversion[2] = g_f_y_center;
            }
        }

        g_invert = 3; // so values will not be changed if we come back
    }

    g_close_enough = ddelmin*pow(2.0, -(double)(abs(periodicitycheck)));
    rqlim_save = rqlim;
    rqlim2 = sqrt(rqlim);
    if (g_integer_fractal)          // for integer routines (lambda)
    {
        g_l_param.x = (long)(parm.x * g_fudge_factor);    // real portion of Lambda
        g_l_param.y = (long)(parm.y * g_fudge_factor);    // imaginary portion of Lambda
        g_l_param2.x = (long)(parm2.x * g_fudge_factor);  // real portion of Lambda2
        g_l_param2.y = (long)(parm2.y * g_fudge_factor);  // imaginary portion of Lambda2
        g_l_limit = (long)(rqlim * g_fudge_factor);      // stop if magnitude exceeds this
        if (g_l_limit <= 0)
        {
            g_l_limit = 0x7fffffffL; // klooge for integer math
        }
        g_l_limit2 = (long)(rqlim2 * g_fudge_factor);    // stop if magnitude exceeds this
        g_l_close_enough = (long)(g_close_enough * g_fudge_factor); // "close enough" value
        g_l_init_orbit.x = (long)(g_init_orbit.x * g_fudge_factor);
        g_l_init_orbit.y = (long)(g_init_orbit.y * g_fudge_factor);
    }
    resuming = (g_calc_status == calc_status_value::RESUMABLE);
    if (!resuming) // free resume_info memory if any is hanging around
    {
        end_resume();
        if (resave_flag)
        {
            updatesavename(savename); // do the pending increment
            resave_flag = 0;
            started_resaves = false;
        }
        g_calc_time = 0;
    }

    if (curfractalspecific->calctype != standard_fractal
            && curfractalspecific->calctype != calcmand
            && curfractalspecific->calctype != calcmandfp
            && curfractalspecific->calctype != lyapunov
            && curfractalspecific->calctype != calcfroth)
    {
        calctype = curfractalspecific->calctype; // per_image can override
        symmetry = curfractalspecific->symmetry; //   calctype & symmetry
        plot = putcolor; // defaults when setsymmetry not called or does nothing
        xxbegin = 0;
        yybegin = xxbegin;
        xxstart = yybegin;
        yystart = xxstart;
        g_i_x_start = yystart;
        g_i_y_start = g_i_x_start;
        yystop = ydots-1;
        g_i_y_stop = yystop;
        xxstop = xdots-1;
        g_i_x_stop = xxstop;
        g_calc_status = calc_status_value::IN_PROGRESS; // mark as in-progress
        g_distance_estimator = 0; // only standard escape time engine supports distest
        // per_image routine is run here
        if (curfractalspecific->per_image())
        {
            // not a stand-alone
            // next two lines in case periodicity changed
            g_close_enough = ddelmin*pow(2.0, -(double)(abs(periodicitycheck)));
            g_l_close_enough = (long)(g_close_enough * g_fudge_factor); // "close enough" value
            setsymmetry(symmetry, false);
            timer(0, calctype); // non-standard fractal engine
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
        if (stdcalcmode == '3')  // convoluted 'g' + '2' hybrid
        {
            int oldcalcmode;
            oldcalcmode = stdcalcmode;
            if (!resuming || three_pass)
            {
                stdcalcmode = 'g';
                three_pass = true;
                timer(0, (int(*)())perform_worklist);
                if (g_calc_status == calc_status_value::COMPLETED)
                {
                    if (xdots >= 640)    // '2' is silly after 'g' for low rez
                    {
                        stdcalcmode = '2';
                    }
                    else
                    {
                        stdcalcmode = '1';


                    }
                    timer(0, (int(*)())perform_worklist);
                    three_pass = false;
                }
            }
            else // resuming '2' pass
            {
                if (xdots >= 640)
                {
                    stdcalcmode = '2';
                }
                else
                {
                    stdcalcmode = '1';

                }
                timer(0, (int(*)())perform_worklist);
            }
            stdcalcmode = (char)oldcalcmode;
        }
        else // main case, much nicer!
        {
            three_pass = false;
            timer(0, (int(*)())perform_worklist);
        }
    }
    g_calc_time += timer_interval;

    if (!g_log_map_table.empty() && !g_log_map_calculate)
    {
        g_log_map_table.clear();
    }
    free_workarea();

    if (curfractalspecific->calctype == calcfroth)
    {
        froth_cleanup();
    }
    if ((soundflag & SOUNDFLAG_ORBITMASK) > SOUNDFLAG_BEEP)   // close sound write file
    {
        close_snd();
    }
    if (truecolor)
    {
        enddisk();
    }
    return (g_calc_status == calc_status_value::COMPLETED) ? 0 : -1;
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
        ;
    }
    int ret = -1;
    if (curtype == type && g_alternate_math[i].math != bf_math_type::NONE)
    {
        ret = i;
    }
    return ret;
}


/**************** general escape-time engine routines *********************/

static void perform_worklist()
{
    int (*sv_orbitcalc)() = nullptr;  // function that calculates one orbit
    int (*sv_per_pixel)() = nullptr;  // once-per-pixel init
    bool (*sv_per_image)() = nullptr;  // once-per-image setup
    int alt = find_alternate_math(fractype, bf_math);

    if (alt > -1)
    {
        sv_orbitcalc = curfractalspecific->orbitcalc;
        sv_per_pixel = curfractalspecific->per_pixel;
        sv_per_image = curfractalspecific->per_image;
        curfractalspecific->orbitcalc = g_alternate_math[alt].orbitcalc;
        curfractalspecific->per_pixel = g_alternate_math[alt].per_pixel;
        curfractalspecific->per_image = g_alternate_math[alt].per_image;
    }
    else
    {
        bf_math = bf_math_type::NONE;
    }

    if (potflag && pot16bit)
    {
        int tmpcalcmode = stdcalcmode;

        stdcalcmode = '1'; // force 1 pass
        if (!resuming)
        {
            if (pot_startdisk() < 0)
            {
                pot16bit = false;       // startdisk failed or cancelled
                stdcalcmode = (char)tmpcalcmode;    // maybe we can carry on???
            }
        }
    }
    if (stdcalcmode == 'b' && (curfractalspecific->flags & NOTRACE))
    {
        stdcalcmode = '1';

    }
    if (stdcalcmode == 'g' && (curfractalspecific->flags & NOGUESS))
    {
        stdcalcmode = '1';
    }
    if (stdcalcmode == 'o' && (curfractalspecific->calctype != standard_fractal))
    {
        stdcalcmode = '1';
    }

    // default setup a new worklist
    num_worklist = 1;
    worklist[0].xxbegin = 0;
    worklist[0].xxstart = worklist[0].xxbegin;
    worklist[0].yybegin = 0;
    worklist[0].yystart = worklist[0].yybegin;
    worklist[0].xxstop = xdots - 1;
    worklist[0].yystop = ydots - 1;
    worklist[0].sym = 0;
    worklist[0].pass = worklist[0].sym;
    if (resuming) // restore worklist, if we can't the above will stay in place
    {
        int vsn;
        vsn = start_resume();
        get_resume(sizeof(num_worklist), &num_worklist, sizeof(worklist), worklist, 0);
        end_resume();
        if (vsn < 2)
        {
            xxbegin = 0;
        }
    }

    if (g_distance_estimator) // setup stuff for distance estimator
    {
        double ftemp, ftemp2, delxx, delyy2, delyy, delxx2, d_x_size, d_y_size;
        double aspect;
        if (pseudox && pseudoy)
        {
            aspect = (double)pseudoy/(double)pseudox;
            d_x_size = pseudox-1;
            d_y_size = pseudoy-1;
        }
        else
        {
            aspect = (double)ydots/(double)xdots;
            d_x_size = xdots-1;
            d_y_size = ydots-1;
        }

        delxx  = (xxmax - xx3rd) / d_x_size; // calculate stepsizes
        delyy  = (yymax - yy3rd) / d_y_size;
        delxx2 = (xx3rd - xxmin) / d_y_size;
        delyy2 = (yy3rd - yymin) / d_x_size;

        use_old_distest = save_release < 1827;
        rqlim = rqlim_save; // just in case changed to DEM_BAILOUT earlier
        if (g_distance_estimator != 1 || g_colors == 2)   // not doing regular outside colors
        {
            if (rqlim < DEM_BAILOUT)           // so go straight for dem bailout
            {
                rqlim = DEM_BAILOUT;
            }
        }
        // must be mandel type, formula, or old PAR/GIF
        dem_mandel = curfractalspecific->tojulia != fractal_type::NOFRACTAL || use_old_distest
                     || fractype == fractal_type::FORMULA || fractype == fractal_type::FFORMULA;
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
        dem_width = (sqrt(sqr(xxmax-xxmin) + sqr(xx3rd-xxmin)) * aspect
                     + sqrt(sqr(yymax-yymin) + sqr(yy3rd-yymin))) / g_distance_estimator;
        ftemp = (rqlim < DEM_BAILOUT) ? DEM_BAILOUT : rqlim;
        ftemp += 3; // bailout plus just a bit
        ftemp2 = log(ftemp);
        if (use_old_distest)
        {
            dem_toobig = sqr(ftemp) * sqr(ftemp2) * 4 / dem_delta;
        }
        else
        {
            dem_toobig = fabs(ftemp) * fabs(ftemp2) * 2 / sqrt(dem_delta);
        }
    }

    while (num_worklist > 0)
    {
        // per_image can override
        calctype = curfractalspecific->calctype;
        symmetry = curfractalspecific->symmetry; //   calctype & symmetry
        plot = putcolor; // defaults when setsymmetry not called or does nothing

        // pull top entry off worklist
        xxstart = worklist[0].xxstart;
        g_i_x_start = xxstart;
        xxstop  = worklist[0].xxstop;
        g_i_x_stop  = xxstop;
        xxbegin  = worklist[0].xxbegin;
        yystart = worklist[0].yystart;
        g_i_y_start = yystart;
        yystop  = worklist[0].yystop;
        g_i_y_stop  = yystop;
        yybegin  = worklist[0].yybegin;
        workpass = worklist[0].pass;
        worksym  = worklist[0].sym;
        --num_worklist;
        for (int i = 0; i < num_worklist; ++i)
        {
            worklist[i] = worklist[i+1];
        }

        g_calc_status = calc_status_value::IN_PROGRESS; // mark as in-progress

        curfractalspecific->per_image();
        if (show_dot >= 0)
        {
            find_special_colors();
            switch (g_auto_show_dot)
            {
            case 'd':
                showdotcolor = g_color_dark % g_colors;
                break;
            case 'm':
                showdotcolor = g_color_medium % g_colors;
                break;
            case 'b':
            case 'a':
                showdotcolor = g_color_bright % g_colors;
                break;
            default:
                showdotcolor = show_dot % g_colors;
                break;
            }
            if (sizedot <= 0)
            {
                showdot_width = -1;
            }
            else
            {
                double dshowdot_width;
                dshowdot_width = (double)sizedot*xdots/1024.0;
                /*
                   Arbitrary sanity limit, however showdot_width will
                   overflow if dshowdot width gets near 256.
                */
                if (dshowdot_width > 150.0)
                {
                    showdot_width = 150;
                }
                else if (dshowdot_width > 0.0)
                {
                    showdot_width = (int)dshowdot_width;
                }
                else
                {
                    showdot_width = -1;
                }
            }
            while (showdot_width >= 0)
            {
                /*
                   We're using near memory, so get the amount down
                   to something reasonable. The polynomial used to
                   calculate savedotslen is exactly right for the
                   triangular-shaped shotdot cursor. The that cursor
                   is changed, this formula must match.
                */
                while ((savedotslen = sqr(showdot_width) + 5*showdot_width + 4) > 1000)
                {
                    showdot_width--;
                }
                bool resized = false;
                try
                {
                    savedots.resize(savedotslen);
                    resized = true;
                }
                catch (std::bad_alloc const&)
                {
                }

                if (resized)
                {
                    savedotslen /= 2;
                    fillbuff = &savedots[0] + savedotslen;
                    memset(fillbuff, showdotcolor, savedotslen);
                    break;
                }
                /*
                   There's even less free memory than we thought, so reduce
                   showdot_width still more
                */
                showdot_width--;
            }
            if (savedots.empty())
            {
                showdot_width = -1;
            }
            calctypetmp = calctype;
            calctype    = calctypeshowdot;
        }

        // some common initialization for escape-time pixel level routines
        g_close_enough = ddelmin*pow(2.0, -(double)(abs(periodicitycheck)));
        g_l_close_enough = (long)(g_close_enough * g_fudge_factor); // "close enough" value
        g_keyboard_check_interval = max_keyboard_check_interval;

        setsymmetry(symmetry, true);

        if (!resuming && (labs(g_log_map_flag) == 2 || (g_log_map_flag && g_log_map_auto_calculate)))
        {
            // calculate round screen edges to work out best start for logmap
            g_log_map_flag = (autologmap() * (g_log_map_flag / labs(g_log_map_flag)));
            SetupLogTable();
        }

        // call the appropriate escape-time engine
        switch (stdcalcmode)
        {
        case 's':
            if (g_debug_flag == debug_flags::use_soi_long_double)
            {
                soi_ldbl();
            }
            else
            {
                soi();
            }
            break;
        case 't':
            tesseral();
            break;
        case 'b':
            bound_trace_main();
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
        if (!savedots.empty())
        {
            savedots.clear();
            fillbuff = nullptr;
        }
        if (check_key())   // interrupted?
        {
            break;
        }
    }

    if (num_worklist > 0)
    {
        // interrupted, resumable
        alloc_resume(sizeof(worklist)+20, 2);
        put_resume(sizeof(num_worklist), &num_worklist, sizeof(worklist), worklist, 0);
    }
    else
    {
        g_calc_status = calc_status_value::COMPLETED; // completed
    }
    if (sv_orbitcalc != nullptr)
    {
        curfractalspecific->orbitcalc = sv_orbitcalc;
        curfractalspecific->per_pixel = sv_per_pixel;
        curfractalspecific->per_image = sv_per_image;
    }
}

static int diffusion_scan()
{
    double log2;

    log2 = (double) log(2.0);

    g_got_status = 5;

    // note: the max size of 2048x2048 gives us a 22 bit counter that will
    // fit any 32 bit architecture, the maxinum limit for this case would
    // be 65536x65536

    g_diffusion_bits = (unsigned)(std::min(log(static_cast<double>(g_i_y_stop-g_i_y_start+1)), log(static_cast<double>(g_i_x_stop-g_i_x_start+1)))/log2);
    g_diffusion_bits <<= 1; // double for two axes
    g_diffusion_limit = 1UL << g_diffusion_bits;

    if (diffusion_engine() == -1)
    {
        add_worklist(xxstart, xxstop, xxstart, yystart, yystop,
                     (int)(g_diffusion_counter >> 16),            // high,
                     (int)(g_diffusion_counter & 0xffff),         // low order words
                     worksym);
        return -1;
    }

    return 0;
}

/* little macro that plots a filled square of color c, size s with
   top left cornet at (x,y) with optimization from sym_fill_line */
#define plot_block(x, y, s, c) \
    memset(dstack, (c), (s)); \
    for (int ty = (y); ty < (y)+(s); ty++) \
       sym_fill_line(ty, (x), (x)+(s)-1, dstack)

// macro that does the same as above, but checks the limits in x and y
#define plot_block_lim(x, y, s, c) \
    memset(dstack, (c), (s)); \
    for (int ty = (y); ty < std::min((y)+(s), g_i_y_stop+1); ty++) \
       sym_fill_line(ty, (x), std::min((x)+(s)-1, g_i_x_stop), dstack)

// macro: count_to_int(dif_counter, colo, rowo)
#define count_to_int(C, x, y)     \
    tC = C;                     \
    x = dif_la[tC&0xFF];        \
    y = dif_lb[tC&0xFF];        \
    tC >>= 8;                   \
    x <<= 4;                    \
    x += dif_la[tC&0xFF];       \
    y <<= 4;                    \
    y += dif_lb[tC&0xFF];       \
    tC >>= 8;                   \
    x <<= 4;                    \
    x += dif_la[tC&0xFF];       \
    y <<= 4;                    \
    y += dif_lb[tC&0xFF];       \
    tC >>= 8;                   \
    x >>= dif_offset;           \
    y >>= dif_offset

// Calculate the point
#define calculate               \
    reset_periodicity = true;   \
    if ((*calctype)() == -1)    \
        return -1;              \
    reset_periodicity = false

static int diffusion_engine()
{
    double log2 = (double) log(2.0);

    int i, j;

    int nx, ny; // number of tyles to build in x and y dirs
    // made this to complete the area that is not
    // a square with sides like 2 ** n
    int rem_x, rem_y; // what is left on the last tile to draw

    long unsigned tC; // temp for dif_counter

    int dif_offset; // offset for adjusting looked-up values

    int sqsz;  // size of the block being filled

    int colo, rowo; // original col and row

    int s = 1 << (g_diffusion_bits/2); // size of the square

    nx = (int) floor(static_cast<double>((g_i_x_stop-g_i_x_start+1)/s));
    ny = (int) floor(static_cast<double>((g_i_y_stop-g_i_y_start+1)/s));

    rem_x = (g_i_x_stop-g_i_x_start+1) - nx * s;
    rem_y = (g_i_y_stop-g_i_y_start+1) - ny * s;

    if (yybegin == g_i_y_start && workpass == 0)
    {
        // if restarting on pan:
        g_diffusion_counter =0l;
    }
    else
    {
        // yybegin and passes contain data for resuming the type:
        g_diffusion_counter = (((long)((unsigned)yybegin)) << 16) | ((unsigned)workpass);
    }

    dif_offset = 12-(g_diffusion_bits/2); // offset to adjust coordinates
    // (*) for 4 bytes use 16 for 3 use 12 etc.

    /*************************************/
    // only the points (dithering only) :
    if (g_fill_color == 0)
    {
        while (g_diffusion_counter < (g_diffusion_limit >> 1))
        {
            count_to_int(g_diffusion_counter, colo, rowo);

            i = 0;
            col = g_i_x_start + colo; // get the right tiles
            do
            {
                j = 0;
                row = g_i_y_start + rowo ;
                do
                {
                    calculate;
                    (*plot)(col, row, g_color);
                    j++;
                    row += s;                  // next tile
                }
                while (j < ny);
                /* in the last y tile we may not need to plot the point
                 */
                if (rowo < rem_y)
                {
                    calculate;
                    (*plot)(col, row, g_color);
                }
                i++;
                col += s;
            }
            while (i < nx);
            /* in the last x tiles we may not need to plot the point */
            if (colo < rem_x)
            {
                row = g_i_y_start + rowo;
                j = 0;
                do
                {
                    calculate;
                    (*plot)(col, row, g_color);
                    j++;
                    row += s; // next tile
                }
                while (j < ny);
                if (rowo < rem_y)
                {
                    calculate;
                    (*plot)(col, row, g_color);
                }
            }
            g_diffusion_counter++;
        }
    }
    else
    {
        /*********************************/
        // with progressive filling :
        while (g_diffusion_counter < (g_diffusion_limit >> 1))
        {
            sqsz = 1 << ((int)(g_diffusion_bits-(int)(log(g_diffusion_counter+0.5)/log2)-1)/2);

            count_to_int(g_diffusion_counter, colo, rowo);

            i = 0;
            do
            {
                j = 0;
                do
                {
                    col = g_i_x_start + colo + i * s; // get the right tiles
                    row = g_i_y_start + rowo + j * s;

                    calculate;
                    plot_block(col, row, sqsz, g_color);
                    j++;
                }
                while (j < ny);
                // in the last tile we may not need to plot the point
                if (rowo < rem_y)
                {
                    row = g_i_y_start + rowo + ny * s;

                    calculate;
                    plot_block_lim(col, row, sqsz, g_color);
                }
                i++;
            }
            while (i < nx);
            // in the last tile we may not need to plot the point
            if (colo < rem_x)
            {
                col = g_i_x_start + colo + nx * s;
                j = 0;
                do
                {
                    row = g_i_y_start + rowo + j * s; // get the right tiles

                    calculate;
                    plot_block_lim(col, row, sqsz, g_color);
                    j++;
                }
                while (j < ny);
                if (rowo < rem_y)
                {
                    row = g_i_y_start + rowo + ny * s;

                    calculate;
                    plot_block_lim(col, row, sqsz, g_color);
                }
            }

            g_diffusion_counter++;
        }
    }
    // from half dif_limit on we only plot 1x1 points :-)
    while (g_diffusion_counter < g_diffusion_limit)
    {
        count_to_int(g_diffusion_counter, colo, rowo);

        i = 0;
        do
        {
            j = 0;
            do
            {
                col = g_i_x_start + colo + i * s; // get the right tiles
                row = g_i_y_start + rowo + j * s;

                calculate;
                (*plot)(col, row, g_color);
                j++;
            }
            while (j < ny);
            // in the last tile we may not need to plot the point
            if (rowo < rem_y)
            {
                row = g_i_y_start + rowo + ny * s;

                calculate;
                (*plot)(col, row, g_color);
            }
            i++;
        }
        while (i < nx);
        // in the last tile we may nnt need to plot the point
        if (colo < rem_x)
        {
            col = g_i_x_start + colo + nx * s;
            j = 0;
            do
            {
                row = g_i_y_start + rowo + j * s; // get the right tiles

                calculate;
                (*plot)(col, row, g_color);
                j++;
            }
            while (j < ny);
            if (rowo < rem_y)
            {
                row = g_i_y_start + rowo + ny * s;

                calculate;
                (*plot)(col, row, g_color);
            }
        }
        g_diffusion_counter++;
    }
    return 0;
}

char g_draw_mode = 'r';

static int sticky_orbits()
{
    g_got_status = 6; // for <tab> screen
    totpasses = 1;

    if (plotorbits2dsetup() == -1)
    {
        stdcalcmode = 'g';
        return -1;
    }

    switch (g_draw_mode)
    {
    case 'r':
    default:
        // draw a rectangle
        row = yybegin;
        col = xxbegin;

        while (row <= g_i_y_stop)
        {
            g_current_row = row;
            while (col <= g_i_x_stop)
            {
                if (plotorbits2dfloat() == -1)
                {
                    add_worklist(xxstart, xxstop, col, yystart, yystop, row, 0, worksym);
                    return -1; // interrupted
                }
                ++col;
            }
            col = g_i_x_start;
            ++row;
        }
        break;
    case 'l':
    {
        int dX, dY;                     // vector components
        int final,                      // final row or column number
            G,                  // used to test for new row or column
            inc1,           // G increment when row or column doesn't change
            inc2;               // G increment when row or column changes
        char pos_slope;

        dX = g_i_x_stop - g_i_x_start;                   // find vector components
        dY = g_i_y_stop - g_i_y_start;
        pos_slope = (char)(dX > 0);                   // is slope positive?
        if (dY < 0)
        {
            pos_slope = (char)!pos_slope;
        }
        if (abs(dX) > abs(dY))                  // shallow line case
        {
            if (dX > 0)         // determine start point and last column
            {
                col = xxbegin;
                row = yybegin;
                final = g_i_x_stop;
            }
            else
            {
                col = g_i_x_stop;
                row = g_i_y_stop;
                final = xxbegin;
            }
            inc1 = 2 * abs(dY);             // determine increments and initial G
            G = inc1 - abs(dX);
            inc2 = 2 * (abs(dY) - abs(dX));
            if (pos_slope)
            {
                while (col <= final)    // step through columns checking for new row
                {
                    if (plotorbits2dfloat() == -1)
                    {
                        add_worklist(xxstart, xxstop, col, yystart, yystop, row, 0, worksym);
                        return -1; // interrupted
                    }
                    col++;
                    if (G >= 0)             // it's time to change rows
                    {
                        row++;      // positive slope so increment through the rows
                        G += inc2;
                    }
                    else                          // stay at the same row
                    {
                        G += inc1;
                    }
                }
            }
            else
            {
                while (col <= final)    // step through columns checking for new row
                {
                    if (plotorbits2dfloat() == -1)
                    {
                        add_worklist(xxstart, xxstop, col, yystart, yystop, row, 0, worksym);
                        return -1; // interrupted
                    }
                    col++;
                    if (G > 0)              // it's time to change rows
                    {
                        row--;      // negative slope so decrement through the rows
                        G += inc2;
                    }
                    else                          // stay at the same row
                    {
                        G += inc1;
                    }
                }
            }
        }   // if |dX| > |dY|
        else                            // steep line case
        {
            if (dY > 0)             // determine start point and last row
            {
                col = xxbegin;
                row = yybegin;
                final = g_i_y_stop;
            }
            else
            {
                col = g_i_x_stop;
                row = g_i_y_stop;
                final = yybegin;
            }
            inc1 = 2 * abs(dX);             // determine increments and initial G
            G = inc1 - abs(dY);
            inc2 = 2 * (abs(dX) - abs(dY));
            if (pos_slope)
            {
                while (row <= final)    // step through rows checking for new column
                {
                    if (plotorbits2dfloat() == -1)
                    {
                        add_worklist(xxstart, xxstop, col, yystart, yystop, row, 0, worksym);
                        return -1; // interrupted
                    }
                    row++;
                    if (G >= 0)                 // it's time to change columns
                    {
                        col++;  // positive slope so increment through the columns
                        G += inc2;
                    }
                    else                      // stay at the same column
                    {
                        G += inc1;
                    }
                }
            }
            else
            {
                while (row <= final)    // step through rows checking for new column
                {
                    if (plotorbits2dfloat() == -1)
                    {
                        add_worklist(xxstart, xxstop, col, yystart, yystop, row, 0, worksym);
                        return -1; // interrupted
                    }
                    row++;
                    if (G > 0)                  // it's time to change columns
                    {
                        col--;  // negative slope so decrement through the columns
                        G += inc2;
                    }
                    else                      // stay at the same column
                    {
                        G += inc1;
                    }
                }
            }
        }
        break;
    } // end case 'l'

    case 'f':  // this code does not yet work???
    {
        double Xctr, Yctr;
        LDBL Magnification; // LDBL not really needed here, but used to match function parameters
        double Xmagfactor, Rotation, Skew;
        int angle;
        double factor = PI / 180.0;
        double theta;
        double xfactor = xdots / 2.0;
        double yfactor = ydots / 2.0;

        angle = xxbegin;  // save angle in x parameter

        cvtcentermag(&Xctr, &Yctr, &Magnification, &Xmagfactor, &Rotation, &Skew);
        if (Rotation <= 0)
        {
            Rotation += 360;
        }

        while (angle < Rotation)
        {
            theta = (double)angle * factor;
            col = (int)(xfactor + (Xctr + Xmagfactor * cos(theta)));
            row = (int)(yfactor + (Yctr + Xmagfactor * sin(theta)));
            if (plotorbits2dfloat() == -1)
            {
                add_worklist(angle, 0, 0, 0, 0, 0, 0, worksym);
                return -1; // interrupted
            }
            angle++;
        }
        break;
    }  // end case 'f'
    }  // end switch

    return 0;
}

static int one_or_two_pass()
{
    int i;

    totpasses = 1;
    if (stdcalcmode == '2')
    {
        totpasses = 2;
    }
    if (stdcalcmode == '2' && workpass == 0) // do 1st pass of two
    {
        if (standard_calc(1) == -1)
        {
            add_worklist(xxstart, xxstop, col, yystart, yystop, row, 0, worksym);
            return -1;
        }
        if (num_worklist > 0) // worklist not empty, defer 2nd pass
        {
            add_worklist(xxstart, xxstop, xxstart, yystart, yystop, yystart, 1, worksym);
            return 0;
        }
        workpass = 1;
        xxbegin = xxstart;
        yybegin = yystart;
    }
    // second or only pass
    if (standard_calc(2) == -1)
    {
        i = yystop;
        if (g_i_y_stop != yystop)   // must be due to symmetry
        {
            i -= row - g_i_y_start;
        }
        add_worklist(xxstart, xxstop, col, row, i, row, workpass, worksym);
        return -1;
    }

    return 0;
}

static int standard_calc(int passnum)
{
    g_got_status = 0;
    g_current_pass = passnum;
    row = yybegin;
    col = xxbegin;

    while (row <= g_i_y_stop)
    {
        g_current_row = row;
        reset_periodicity = true;
        while (col <= g_i_x_stop)
        {
            // on 2nd pass of two, skip even pts
            if (quick_calc && !resuming)
            {
                g_color = getcolor(col, row);
                if (g_color != g_inside)
                {
                    ++col;
                    continue;
                }
            }
            if (passnum == 1 || stdcalcmode == '1' || (row&1) != 0 || (col&1) != 0)
            {
                if ((*calctype)() == -1)   // standard_fractal(), calcmand() or calcmandfp()
                {
                    return -1;          // interrupted
                }
                resuming = false;       // reset so quick_calc works
                reset_periodicity = false;
                if (passnum == 1)       // first pass, copy pixel and bump col
                {
                    if ((row&1) == 0 && row < g_i_y_stop)
                    {
                        (*plot)(col, row+1, g_color);
                        if ((col&1) == 0 && col < g_i_x_stop)
                        {
                            (*plot)(col+1, row+1, g_color);
                        }
                    }
                    if ((col&1) == 0 && col < g_i_x_stop)
                    {
                        (*plot)(++col, row, g_color);
                    }
                }
            }
            ++col;
        }
        col = g_i_x_start;
        if (passnum == 1 && (row&1) == 0)
        {
            ++row;
        }
        ++row;
    }
    return 0;
}


int calcmand()              // fast per pixel 1/2/b/g, called with row & col set
{
    // setup values from array to avoid using es reg in calcmand.asm
    g_l_init_x = g_l_x_pixel();
    g_l_init_y = lypixel();
    if (calcmandasm() >= 0)
    {
        if ((!g_log_map_table.empty() || g_log_map_calculate) // map color, but not if maxit & adjusted for inside,etc
                && (realcoloriter < maxit || (g_inside < COLOR_BLACK && g_color_iter == maxit)))
        {
            g_color_iter = logtablecalc(g_color_iter);
        }
        g_color = abs((int)g_color_iter);
        if (g_color_iter >= g_colors)
        {
            // don't use color 0 unless from inside/outside
            if (save_release <= 1950)
            {
                if (g_colors < 16)
                {
                    g_color &= g_and_color;
                }
                else
                {
                    g_color = ((g_color - 1) % g_and_color) + 1;  // skip color zero
                }
            }
            else
            {
                if (g_colors < 16)
                {
                    g_color = (int)(g_color_iter & g_and_color);
                }
                else
                {
                    g_color = (int)(((g_color_iter - 1) % g_and_color) + 1);
                }
            }
        }
        if (g_debug_flag != debug_flags::force_boundary_trace_error)
        {
            if (g_color <= 0 && stdcalcmode == 'b')
            {
                g_color = 1;
            }
        }
        (*plot)(col, row, g_color);
    }
    else
    {
        g_color = (int)g_color_iter;
    }
    return g_color;
}

/************************************************************************/
// sort of a floating point version of calcmand()
// can also handle invert, any rqlim, potflag, zmag, epsilon cross,
// and all the current outside options
/************************************************************************/
int calcmandfp()
{
    if (g_invert != 0)
    {
        invertz2(&g_init);
    }
    else
    {
        g_init.x = dxpixel();
        g_init.y = dypixel();
    }
    if (calcmandfpasm() >= 0)
    {
        if (potflag)
        {
            g_color_iter = potential(magnitude, realcoloriter);
        }
        if ((!g_log_map_table.empty() || g_log_map_calculate) // map color, but not if maxit & adjusted for inside,etc
                && (realcoloriter < maxit || (g_inside < COLOR_BLACK && g_color_iter == maxit)))
        {
            g_color_iter = logtablecalc(g_color_iter);
        }
        g_color = abs((int)g_color_iter);
        if (g_color_iter >= g_colors)
        {
            // don't use color 0 unless from inside/outside
            if (save_release <= 1950)
            {
                if (g_colors < 16)
                {
                    g_color &= g_and_color;
                }
                else
                {
                    g_color = ((g_color - 1) % g_and_color) + 1;  // skip color zero
                }
            }
            else
            {
                if (g_colors < 16)
                {
                    g_color = (int)(g_color_iter & g_and_color);
                }
                else
                {
                    g_color = (int)(((g_color_iter - 1) % g_and_color) + 1);
                }
            }
        }
        if (g_debug_flag != debug_flags::force_boundary_trace_error)
        {
            if (g_color == 0 && stdcalcmode == 'b')
            {
                g_color = 1;
            }
        }
        (*plot)(col, row, g_color);
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

#ifdef NUMSAVED
    DComplex savedz[NUMSAVED] = { 0.0 };
    long caught[NUMSAVED] = { 0 };
    long changed[NUMSAVED] = { 0 };
    int zctr = 0;
#endif
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
    savemaxit = maxit;
#ifdef NUMSAVED
    for (int i = 0; i < NUMSAVED; i++)
    {
        caught[i] = 0L;
        changed[i] = 0L;
    }
#endif
    if (g_inside == STARTRAIL)
    {
        for (auto &elem : tantable)
        {
            elem = 0.0;
        }
        if (save_release > 1824)
        {
            maxit = 16;
        }
    }
    if (periodicitycheck == 0 || g_inside == ZMAG || g_inside == STARTRAIL)
    {
        oldcoloriter = 2147483647L;       // don't check periodicity at all
    }
    else if (g_inside == PERIOD)       // for display-periodicity
    {
        oldcoloriter = (maxit/5)*4;       // don't check until nearly done
    }
    else if (reset_periodicity)
    {
        oldcoloriter = 255;               // don't check periodicity 1st 250 iterations
    }

    // Jonathan - how about this idea ? skips first saved value which never works
#ifdef MINSAVEDAND
    if (oldcoloriter < MINSAVEDAND)
    {
        oldcoloriter = MINSAVEDAND;
    }
#else
    if (oldcoloriter < firstsavedand)   // I like it!
    {
        oldcoloriter = firstsavedand;
    }
#endif
    // really fractal specific, but we'll leave it here
    if (!g_integer_fractal)
    {
        if (useinitorbit == 1)
        {
            saved = g_init_orbit;
        }
        else
        {
            saved.x = 0;
            saved.y = 0;
        }
#ifdef NUMSAVED
        savedz[zctr++] = saved;
#endif
        if (bf_math != bf_math_type::NONE)
        {
            if (g_decimals > 200)
            {
                g_keyboard_check_interval = -1;
            }
            if (bf_math == bf_math_type::BIGNUM)
            {
                clear_bn(bnsaved.x);
                clear_bn(bnsaved.y);
            }
            else if (bf_math == bf_math_type::BIGFLT)
            {
                clear_bf(bfsaved.x);
                clear_bf(bfsaved.y);
            }
        }
        g_init.y = dypixel();
        if (g_distance_estimator)
        {
            if (use_old_distest)
            {
                rqlim = rqlim_save;
                if (g_distance_estimator != 1 || g_colors == 2)   // not doing regular outside colors
                {
                    if (rqlim < DEM_BAILOUT)     // so go straight for dem bailout
                    {
                        rqlim = DEM_BAILOUT;
                    }
                }
                dem_color = -1;
            }
            deriv.x = 1;
            deriv.y = 0;
            magnitude = 0;
        }
    }
    else
    {
        if (useinitorbit == 1)
        {
            lsaved = g_l_init_orbit;
        }
        else
        {
            lsaved.x = 0;
            lsaved.y = 0;
        }
        g_l_init.y = lypixel();
    }
    orbit_ptr = 0;
    g_color_iter = 0;
    if (fractype == fractal_type::JULIAFP || fractype == fractal_type::JULIA)
    {
        g_color_iter = -1;
    }
    caught_a_cycle = false;
    if (g_inside == PERIOD)
    {
        savedand = 16;           // begin checking every 16th cycle
    }
    else
    {
#ifdef MINSAVEDAND
        savedand = MINSAVEDAND;
#else
        savedand = firstsavedand;                // begin checking every other cycle
#endif
    }
    savedincr = 1;               // start checking the very first time

    if (g_inside <= BOF60 && g_inside >= BOF61)
    {
        g_l_magnitude = 0;
        magnitude = g_l_magnitude;
        min_orbit = 100000.0;
    }
    overflow = false;           // reset integer math overflow flag

    curfractalspecific->per_pixel(); // initialize the calculations

    attracted = false;

    if (outside == TDIS)
    {
        if (g_integer_fractal)
        {
            old.x = ((double)g_l_old.x) / g_fudge_factor;
            old.y = ((double)g_l_old.y) / g_fudge_factor;
        }
        else if (bf_math == bf_math_type::BIGNUM)
        {
            old = cmplxbntofloat(&bnold);
        }
        else if (bf_math == bf_math_type::BIGFLT)
        {
            old = cmplxbftofloat(&bfold);
        }
        lastz.x = old.x;
        lastz.y = old.y;
    }

    if (((soundflag & SOUNDFLAG_ORBITMASK) > SOUNDFLAG_X || show_dot >= 0) && orbit_delay > 0)
    {
        check_freq = 16;
    }
    else
    {
        check_freq = 2048;
    }

    if (show_orbit)
    {
        snd_time_write();
    }
    while (++g_color_iter < maxit)
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
                ftemp = 2 * (old.x * deriv.x - old.y * deriv.y) + 1;
            }
            else
            {
                ftemp = 2 * (old.x * deriv.x - old.y * deriv.y);
            }
            deriv.y = 2 * (old.y * deriv.x + old.x * deriv.y);
            deriv.x = ftemp;
            if (use_old_distest)
            {
                if (sqr(deriv.x)+sqr(deriv.y) > dem_toobig)
                {
                    break;
                }
            }
            else if (save_release > 1950)
            {
                if (std::max(fabs(deriv.x), fabs(deriv.y)) > dem_toobig)
                {
                    break;
                }
            }
            /* if above exit taken, the later test vs dem_delta will place this
                       point on the boundary, because mag(old)<bailout just now */

            if (curfractalspecific->orbitcalc() || (overflow && save_release > 1826))
            {
                if (use_old_distest)
                {
                    if (dem_color < 0)
                    {
                        dem_color = g_color_iter;
                        dem_new = g_new;
                    }
                    if (rqlim >= DEM_BAILOUT
                            || magnitude >= (rqlim = DEM_BAILOUT)
                            || magnitude == 0)
                    {
                        break;
                    }
                }
                else
                {
                    break;
                }
            }
            old = g_new;
        }

        // the usual case
        else if ((curfractalspecific->orbitcalc() && g_inside != STARTRAIL)
                 || overflow)
        {
            break;
        }
        if (show_orbit)
        {
            if (!g_integer_fractal)
            {
                if (bf_math == bf_math_type::BIGNUM)
                {
                    g_new = cmplxbntofloat(&bnnew);
                }
                else if (bf_math == bf_math_type::BIGFLT)
                {
                    g_new = cmplxbftofloat(&bfnew);
                }
                plot_orbit(g_new.x, g_new.y, -1);
            }
            else
            {
                iplot_orbit(g_l_new.x, g_l_new.y, -1);
            }
        }
        if (g_inside < ITER)
        {
            if (bf_math == bf_math_type::BIGNUM)
            {
                g_new = cmplxbntofloat(&bnnew);
            }
            else if (bf_math == bf_math_type::BIGFLT)
            {
                g_new = cmplxbftofloat(&bfnew);
            }
            if (g_inside == STARTRAIL)
            {
                if (0 < g_color_iter && g_color_iter < 16)
                {
                    if (g_integer_fractal)
                    {
                        g_new.x = g_l_new.x;
                        g_new.x /= g_fudge_factor;
                        g_new.y = g_l_new.y;
                        g_new.y /= g_fudge_factor;
                    }

                    if (save_release > 1824)
                    {
                        if (g_new.x > STARTRAILMAX)
                        {
                            g_new.x = STARTRAILMAX;
                        }
                        if (g_new.x < -STARTRAILMAX)
                        {
                            g_new.x = -STARTRAILMAX;
                        }
                        if (g_new.y > STARTRAILMAX)
                        {
                            g_new.y = STARTRAILMAX;
                        }
                        if (g_new.y < -STARTRAILMAX)
                        {
                            g_new.y = -STARTRAILMAX;
                        }
                        tempsqrx = g_new.x * g_new.x;
                        tempsqry = g_new.y * g_new.y;
                        magnitude = tempsqrx + tempsqry;
                        old = g_new;
                    }
                    {
                        int tmpcolor;
                        tmpcolor = (int)(((g_color_iter - 1) % g_and_color) + 1);
                        tantable[tmpcolor-1] = g_new.y/(g_new.x+.000001);
                    }
                }
            }
            else if (g_inside == EPSCROSS)
            {
                hooper = 0;
                if (g_integer_fractal)
                {
                    if (labs(g_l_new.x) < labs(lcloseprox))
                    {
                        hooper = (lcloseprox > 0? 1 : -1); // close to y axis
                        goto plot_inside;
                    }
                    else if (labs(g_l_new.y) < labs(lcloseprox))
                    {
                        hooper = (lcloseprox > 0 ? 2: -2); // close to x axis
                        goto plot_inside;
                    }
                }
                else
                {
                    if (fabs(g_new.x) < fabs(g_close_proximity))
                    {
                        hooper = (g_close_proximity > 0? 1 : -1); // close to y axis
                        goto plot_inside;
                    }
                    else if (fabs(g_new.y) < fabs(g_close_proximity))
                    {
                        hooper = (g_close_proximity > 0? 2 : -2); // close to x axis
                        goto plot_inside;
                    }
                }
            }
            else if (g_inside == FMODI)
            {
                double mag;
                if (g_integer_fractal)
                {
                    g_new.x = ((double)g_l_new.x) / g_fudge_factor;
                    g_new.y = ((double)g_l_new.y) / g_fudge_factor;
                }
                mag = fmodtest();
                if (mag < g_close_proximity)
                {
                    memvalue = mag;
                }
            }
            else if (g_inside <= BOF60 && g_inside >= BOF61)
            {
                if (g_integer_fractal)
                {
                    if (g_l_magnitude == 0 || !no_mag_calc)
                    {
                        g_l_magnitude = lsqr(g_l_new.x) + lsqr(g_l_new.y);
                    }
                    magnitude = g_l_magnitude;
                    magnitude = magnitude / g_fudge_factor;
                }
                else if (magnitude == 0.0 || !no_mag_calc)
                {
                    magnitude = sqr(g_new.x) + sqr(g_new.y);
                }
                if (magnitude < min_orbit)
                {
                    min_orbit = magnitude;
                    min_index = g_color_iter + 1;
                }
            }
        }

        if (outside == TDIS || outside == FMOD)
        {
            if (bf_math == bf_math_type::BIGNUM)
            {
                g_new = cmplxbntofloat(&bnnew);
            }
            else if (bf_math == bf_math_type::BIGFLT)
            {
                g_new = cmplxbftofloat(&bfnew);
            }
            if (outside == TDIS)
            {
                if (g_integer_fractal)
                {
                    g_new.x = ((double)g_l_new.x) / g_fudge_factor;
                    g_new.y = ((double)g_l_new.y) / g_fudge_factor;
                }
                totaldist += sqrt(sqr(lastz.x-g_new.x)+sqr(lastz.y-g_new.y));
                lastz.x = g_new.x;
                lastz.y = g_new.y;
            }
            else if (outside == FMOD)
            {
                double mag;
                if (g_integer_fractal)
                {
                    g_new.x = ((double)g_l_new.x) / g_fudge_factor;
                    g_new.y = ((double)g_l_new.y) / g_fudge_factor;
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
                    lat.x = g_l_new.x - g_l_attractor[i].x;
                    lat.x = lsqr(lat.x);
                    if (lat.x < l_at_rad)
                    {
                        lat.y = g_l_new.y - g_l_attractor[i].y;
                        lat.y = lsqr(lat.y);
                        if (lat.y < l_at_rad)
                        {
                            if ((lat.x + lat.y) < l_at_rad)
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
                    at.x = g_new.x - g_attractor[i].x;
                    at.x = sqr(at.x);
                    if (at.x < g_f_at_rad)
                    {
                        at.y = g_new.y - g_attractor[i].y;
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

        if (g_color_iter > oldcoloriter) // check periodicity
        {
            if ((g_color_iter & savedand) == 0)            // time to save a new value
            {
                savedcoloriter = g_color_iter;
                if (g_integer_fractal)
                {
                    lsaved = g_l_new;// integer fractals
                }
                else if (bf_math == bf_math_type::BIGNUM)
                {
                    copy_bn(bnsaved.x, bnnew.x);
                    copy_bn(bnsaved.y, bnnew.y);
                }
                else if (bf_math == bf_math_type::BIGFLT)
                {
                    copy_bf(bfsaved.x, bfnew.x);
                    copy_bf(bfsaved.y, bfnew.y);
                }
                else
                {
                    saved = g_new;  // floating pt fractals
#ifdef NUMSAVED
                    if (zctr < NUMSAVED)
                    {
                        changed[zctr]  = g_color_iter;
                        savedz[zctr++] = saved;
                    }
#endif
                }
                if (--savedincr == 0)    // time to lengthen the periodicity?
                {
                    savedand = (savedand << 1) + 1;       // longer periodicity
                    savedincr = nextsavedincr;// restart counter
                }
            }
            else                // check against an old save
            {
                if (g_integer_fractal)     // floating-pt periodicity chk
                {
                    if (labs(lsaved.x - g_l_new.x) < g_l_close_enough)
                    {
                        if (labs(lsaved.y - g_l_new.y) < g_l_close_enough)
                        {
                            caught_a_cycle = true;
                        }
                    }
                }
                else if (bf_math == bf_math_type::BIGNUM)
                {
                    if (cmp_bn(abs_a_bn(sub_bn(bntmp, bnsaved.x, bnnew.x)), bnclosenuff) < 0)
                    {
                        if (cmp_bn(abs_a_bn(sub_bn(bntmp, bnsaved.y, bnnew.y)), bnclosenuff) < 0)
                        {
                            caught_a_cycle = true;
                        }
                    }
                }
                else if (bf_math == bf_math_type::BIGFLT)
                {
                    if (cmp_bf(abs_a_bf(sub_bf(bftmp, bfsaved.x, bfnew.x)), bfclosenuff) < 0)
                    {
                        if (cmp_bf(abs_a_bf(sub_bf(bftmp, bfsaved.y, bfnew.y)), bfclosenuff) < 0)
                        {
                            caught_a_cycle = true;
                        }
                    }
                }
                else
                {
                    if (fabs(saved.x - g_new.x) < g_close_enough)
                    {
                        if (fabs(saved.y - g_new.y) < g_close_enough)
                        {
                            caught_a_cycle = true;
                        }
                    }
#ifdef NUMSAVED
                    for (int i = 0; i <= zctr; i++)
                    {
                        if (caught[i] == 0)
                        {
                            if (fabs(savedz[i].x - g_new.x) < g_close_enough)
                                if (fabs(savedz[i].y - g_new.y) < g_close_enough)
                                {
                                    caught[i] = g_color_iter;
                                }
                        }
                    }
#endif
                }
                if (caught_a_cycle)
                {
#ifdef NUMSAVED
                    static FILE *fp = dir_fopen(workdir.c_str(), "cycles.txt", "w");
#endif
                    cyclelen = g_color_iter-savedcoloriter;
#ifdef NUMSAVED
                    fprintf(fp, "row %3d col %3d len %6ld iter %6ld savedand %6ld\n",
                            row, col, cyclelen, g_color_iter, savedand);
                    if (zctr > 1 && zctr < NUMSAVED)
                    {
                        for (int i = 0; i < zctr; i++)
                        {
                            fprintf(fp, "   caught %2d saved %6ld iter %6ld\n", i, changed[i], caught[i]);
                        }
                    }
                    fflush(fp);
#endif
                    g_color_iter = maxit - 1;
                }

            }
        }
    }  // end while (g_color_iter++ < maxit)

    if (show_orbit)
    {
        scrub_orbit();
    }

    realcoloriter = g_color_iter;           // save this before we start adjusting it
    if (g_color_iter >= maxit)
    {
        oldcoloriter = 0;         // check periodicity immediately next time
    }
    else
    {
        oldcoloriter = g_color_iter + 10;    // check when past this + 10 next time
        if (g_color_iter == 0)
        {
            g_color_iter = 1;         // needed to make same as calcmand
        }
    }

    if (potflag)
    {
        if (g_integer_fractal)       // adjust integer fractals
        {
            g_new.x = ((double)g_l_new.x) / g_fudge_factor;
            g_new.y = ((double)g_l_new.y) / g_fudge_factor;
        }
        else if (bf_math == bf_math_type::BIGNUM)
        {
            g_new.x = (double)bntofloat(bnnew.x);
            g_new.y = (double)bntofloat(bnnew.y);
        }
        else if (bf_math == bf_math_type::BIGFLT)
        {
            g_new.x = (double)bftofloat(bfnew.x);
            g_new.y = (double)bftofloat(bfnew.y);
        }
        magnitude = sqr(g_new.x) + sqr(g_new.y);
        g_color_iter = potential(magnitude, g_color_iter);
        if (!g_log_map_table.empty() || g_log_map_calculate)
        {
            g_color_iter = logtablecalc(g_color_iter);
        }
        goto plot_pixel;          // skip any other adjustments
    }

    if (g_color_iter >= maxit)                // an "inside" point
    {
        goto plot_inside;         // distest, decomp, biomorph don't apply
    }


    if (outside < ITER)
    {
        if (g_integer_fractal)
        {
            g_new.x = ((double)g_l_new.x) / g_fudge_factor;
            g_new.y = ((double)g_l_new.y) / g_fudge_factor;
        }
        else if (bf_math ==  bf_math_type::BIGNUM)
        {
            g_new.x = (double)bntofloat(bnnew.x);
            g_new.y = (double)bntofloat(bnnew.y);
        }
        // Add 7 to overcome negative values on the MANDEL
        if (outside == REAL)                 // "real"
        {
            g_color_iter += (long)g_new.x + 7;
        }
        else if (outside == IMAG)              // "imag"
        {
            g_color_iter += (long)g_new.y + 7;
        }
        else if (outside == MULT  && g_new.y)      // "mult"
        {
            g_color_iter = (long)((double)g_color_iter * (g_new.x/g_new.y));
        }
        else if (outside == SUM)               // "sum"
        {
            g_color_iter += (long)(g_new.x + g_new.y);
        }
        else if (outside == ATAN)              // "atan"
        {
            g_color_iter = (long)fabs(atan2(g_new.y, g_new.x)*atan_colors/PI);
        }
        else if (outside == FMOD)
        {
            g_color_iter = (long)(memvalue * g_colors / g_close_proximity);
        }
        else if (outside == TDIS)
        {
            g_color_iter = (long)(totaldist);
        }


        // eliminate negative colors & wrap arounds
        if ((g_color_iter <= 0 || g_color_iter > maxit) && outside != FMOD)
        {
            if (save_release < 1961)
            {
                g_color_iter = 0;
            }
            else
            {
                g_color_iter = 1;
            }
        }
    }

    if (g_distance_estimator)
    {
        double dist;
        dist = sqr(g_new.x) + sqr(g_new.y);
        if (dist == 0 || overflow)
        {
            dist = 0;
        }
        else
        {
            double temp;
            temp = log(dist);
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
            g_color_iter = !g_inside;   // the only useful distest 2 color use
            goto plot_pixel;       // no further adjustments apply
        }
        if (g_distance_estimator > 1)          // pick color based on distance
        {
            if (old_demm_colors)   // this one is needed for old color scheme
            {
                g_color_iter = (long)sqrt(sqrt(dist) / dem_width + 1);
            }
            else if (use_old_distest)
            {
                g_color_iter = (long)sqrt(dist / dem_width + 1);
            }
            else
            {
                g_color_iter = (long)(dist / dem_width + 1);
            }
            g_color_iter &= LONG_MAX;  // oops - color can be negative
            goto plot_pixel;       // no further adjustments apply
        }
        if (use_old_distest)
        {
            g_color_iter = dem_color;
            g_new = dem_new;
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
            if (labs(g_l_new.x) < g_l_limit2 || labs(g_l_new.y) < g_l_limit2)
            {
                g_color_iter = g_biomorph;
            }
        }
        else if (fabs(g_new.x) < rqlim2 || fabs(g_new.y) < rqlim2)
        {
            g_color_iter = g_biomorph;
        }
    }

    if (outside >= COLOR_BLACK && !attracted)       // merge escape-time stripes
    {
        g_color_iter = outside;
    }
    else if (!g_log_map_table.empty() || g_log_map_calculate)
    {
        g_color_iter = logtablecalc(g_color_iter);
    }
    goto plot_pixel;

plot_inside: // we're "inside"
    if (periodicitycheck < 0 && caught_a_cycle)
    {
        g_color_iter = 7;           // show periodicity
    }
    else if (g_inside >= COLOR_BLACK)
    {
        g_color_iter = g_inside;              // set to specified color, ignore logpal
    }
    else
    {
        if (g_inside == STARTRAIL)
        {
            double diff;
            g_color_iter = 0;
            for (int i = 1; i < 16; i++)
            {
                diff = tantable[0] - tantable[i];
                if (fabs(diff) < .05)
                {
                    g_color_iter = i;
                    break;
                }
            }
        }
        else if (g_inside == PERIOD)
        {
            if (cyclelen > 0)
            {
                g_color_iter = cyclelen;
            }
            else
            {
                g_color_iter = maxit;
            }
        }
        else if (g_inside == EPSCROSS)
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
                g_color_iter = maxit;
            }
            if (show_orbit)
            {
                scrub_orbit();
            }
        }
        else if (g_inside == FMODI)
        {
            g_color_iter = (long)(memvalue * g_colors / g_close_proximity);
        }
        else if (g_inside == ATANI)            // "atan"
        {
            if (g_integer_fractal)
            {
                g_new.x = ((double)g_l_new.x) / g_fudge_factor;
                g_new.y = ((double)g_l_new.y) / g_fudge_factor;
                g_color_iter = (long)fabs(atan2(g_new.y, g_new.x)*atan_colors/PI);
            }
            else
            {
                g_color_iter = (long)fabs(atan2(g_new.y, g_new.x)*atan_colors/PI);
            }
        }
        else if (g_inside == BOF60)
        {
            g_color_iter = (long)(sqrt(min_orbit) * 75);
        }
        else if (g_inside == BOF61)
        {
            g_color_iter = min_index;
        }
        else if (g_inside == ZMAG)
        {
            if (g_integer_fractal)
            {
                /*
                g_new.x = ((double)lnew.x) / fudge;
                g_new.y = ((double)lnew.y) / fudge;
                g_color_iter = (long)((((double)lsqr(lnew.x))/fudge + ((double)lsqr(lnew.y))/fudge) * (maxit>>1) + 1);
                */
                g_color_iter = (long)(((double)g_l_magnitude/g_fudge_factor) * (maxit >> 1) + 1);
            }
            else
            {
                g_color_iter = (long)((sqr(g_new.x) + sqr(g_new.y)) * (maxit >> 1) + 1);
            }
        }
        else   // inside == -1
        {
            g_color_iter = maxit;
        }
        if (!g_log_map_table.empty() || g_log_map_calculate)
        {
            g_color_iter = logtablecalc(g_color_iter);
        }
    }

plot_pixel:

    g_color = abs((int)g_color_iter);
    if (g_color_iter >= g_colors)
    {
        // don't use color 0 unless from inside/outside
        if (save_release <= 1950)
        {
            if (g_colors < 16)
            {
                g_color &= g_and_color;
            }
            else
            {
                g_color = ((g_color - 1) % g_and_color) + 1;  // skip color zero
            }
        }
        else
        {
            if (g_colors < 16)
            {
                g_color = (int)(g_color_iter & g_and_color);
            }
            else
            {
                g_color = (int)(((g_color_iter - 1) % g_and_color) + 1);
            }
        }
    }
    if (g_debug_flag != debug_flags::force_boundary_trace_error)
    {
        if (g_color <= 0 && stdcalcmode == 'b')
        {
            g_color = 1;
        }
    }
    (*plot)(col, row, g_color);

    maxit = savemaxit;
    if ((g_keyboard_check_interval -= abs((int)realcoloriter)) <= 0)
    {
        if (check_key())
        {
            return -1;
        }
        g_keyboard_check_interval = max_keyboard_check_interval;
    }
    return g_color;
}


#define cos45  sin45
#define lcos45 lsin45

/**************** standardfractal doodad subroutines *********************/
static void decomposition()
{
    // static double cos45     = 0.70710678118654750; */ /* cos 45  degrees
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
    // static long lcos45     ;*/ /* cos 45   degrees
    static long lsin45     ; // sin 45     degrees
    static long lcos22_5   ; // cos 22.5   degrees
    static long lsin22_5   ; // sin 22.5   degrees
    static long lcos11_25  ; // cos 11.25  degrees
    static long lsin11_25  ; // sin 11.25  degrees
    static long lcos5_625  ; // cos 5.625  degrees
    static long lsin5_625  ; // sin 5.625  degrees
    static long ltan22_5   ; // tan 22.5   degrees
    static long ltan11_25  ; // tan 11.25  degrees
    static long ltan5_625  ; // tan 5.625  degrees
    static long ltan2_8125 ; // tan 2.8125 degrees
    static long ltan1_4063 ; // tan 1.4063 degrees
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
        if (g_l_new.y < 0)
        {
            temp = 2;
            g_l_new.y = -g_l_new.y;
        }

        if (g_l_new.x < 0)
        {
            ++temp;
            g_l_new.x = -g_l_new.x;
        }
        if (g_decomp[0] == 2 && save_release >= 1827)
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
            if (g_l_new.x < g_l_new.y)
            {
                ++temp;
                lalt.x = g_l_new.x; // just
                g_l_new.x = g_l_new.y; // swap
                g_l_new.y = lalt.x; // them
            }

            if (g_decomp[0] >= 16)
            {
                temp <<= 1;
                if (multiply(g_l_new.x, ltan22_5, bitshift) < g_l_new.y)
                {
                    ++temp;
                    lalt = g_l_new;
                    g_l_new.x = multiply(lalt.x, lcos45, bitshift) +
                             multiply(lalt.y, lsin45, bitshift);
                    g_l_new.y = multiply(lalt.x, lsin45, bitshift) -
                             multiply(lalt.y, lcos45, bitshift);
                }

                if (g_decomp[0] >= 32)
                {
                    temp <<= 1;
                    if (multiply(g_l_new.x, ltan11_25, bitshift) < g_l_new.y)
                    {
                        ++temp;
                        lalt = g_l_new;
                        g_l_new.x = multiply(lalt.x, lcos22_5, bitshift) +
                                 multiply(lalt.y, lsin22_5, bitshift);
                        g_l_new.y = multiply(lalt.x, lsin22_5, bitshift) -
                                 multiply(lalt.y, lcos22_5, bitshift);
                    }

                    if (g_decomp[0] >= 64)
                    {
                        temp <<= 1;
                        if (multiply(g_l_new.x, ltan5_625, bitshift) < g_l_new.y)
                        {
                            ++temp;
                            lalt = g_l_new;
                            g_l_new.x = multiply(lalt.x, lcos11_25, bitshift) +
                                     multiply(lalt.y, lsin11_25, bitshift);
                            g_l_new.y = multiply(lalt.x, lsin11_25, bitshift) -
                                     multiply(lalt.y, lcos11_25, bitshift);
                        }

                        if (g_decomp[0] >= 128)
                        {
                            temp <<= 1;
                            if (multiply(g_l_new.x, ltan2_8125, bitshift) < g_l_new.y)
                            {
                                ++temp;
                                lalt = g_l_new;
                                g_l_new.x = multiply(lalt.x, lcos5_625, bitshift) +
                                         multiply(lalt.y, lsin5_625, bitshift);
                                g_l_new.y = multiply(lalt.x, lsin5_625, bitshift) -
                                         multiply(lalt.y, lcos5_625, bitshift);
                            }

                            if (g_decomp[0] == 256)
                            {
                                temp <<= 1;
                                if (multiply(g_l_new.x, ltan1_4063, bitshift) < g_l_new.y)
                                {
                                    if ((g_l_new.x*ltan1_4063 < g_l_new.y))
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
        if (g_new.y < 0)
        {
            temp = 2;
            g_new.y = -g_new.y;
        }
        if (g_new.x < 0)
        {
            ++temp;
            g_new.x = -g_new.x;
        }
        if (g_decomp[0] == 2 && save_release >= 1827)
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
            if (g_new.x < g_new.y)
            {
                ++temp;
                alt.x = g_new.x; // just
                g_new.x = g_new.y; // swap
                g_new.y = alt.x; // them
            }
            if (g_decomp[0] >= 16)
            {
                temp <<= 1;
                if (g_new.x*tan22_5 < g_new.y)
                {
                    ++temp;
                    alt = g_new;
                    g_new.x = alt.x*cos45 + alt.y*sin45;
                    g_new.y = alt.x*sin45 - alt.y*cos45;
                }

                if (g_decomp[0] >= 32)
                {
                    temp <<= 1;
                    if (g_new.x*tan11_25 < g_new.y)
                    {
                        ++temp;
                        alt = g_new;
                        g_new.x = alt.x*cos22_5 + alt.y*sin22_5;
                        g_new.y = alt.x*sin22_5 - alt.y*cos22_5;
                    }

                    if (g_decomp[0] >= 64)
                    {
                        temp <<= 1;
                        if (g_new.x*tan5_625 < g_new.y)
                        {
                            ++temp;
                            alt = g_new;
                            g_new.x = alt.x*cos11_25 + alt.y*sin11_25;
                            g_new.y = alt.x*sin11_25 - alt.y*cos11_25;
                        }

                        if (g_decomp[0] >= 128)
                        {
                            temp <<= 1;
                            if (g_new.x*tan2_8125 < g_new.y)
                            {
                                ++temp;
                                alt = g_new;
                                g_new.x = alt.x*cos5_625 + alt.y*sin5_625;
                                g_new.y = alt.x*sin5_625 - alt.y*cos5_625;
                            }

                            if (g_decomp[0] == 256)
                            {
                                temp <<= 1;
                                if ((g_new.x*tan1_4063 < g_new.y))
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
    if (g_decomp[0] == 2 && save_release >= 1827)
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
    else if (g_decomp[0] == 2 && save_release < 1827)
    {
        g_color_iter &= 1;
    }
    if (g_colors > g_decomp[0])
    {
        g_color_iter++;
    }
}

/******************************************************************/
// Continuous potential calculation for Mandelbrot and Julia
// Reference: Science of Fractal Images p. 190.
// Special thanks to Mark Peterson for his "MtMand" program that
// beautifully approximates plate 25 (same reference) and spurred
// on the inclusion of similar capabilities in FRACTINT.
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
/******************************************************************/

static int potential(double mag, long iterations)
{
    float f_mag, f_tmp, pot;
    int i_pot;
    long l_pot;

    if (iterations < maxit)
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
                double d_tmp = log(mag)/(double)pow(2.0, (double)pot);
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
        /* meaning of parameters:
              potparam[0] -- zero potential level - highest color -
              potparam[1] -- slope multiplier -- higher is steeper
              potparam[2] -- rqlim value if changeable (bailout for modulus) */

        if (pot > 0.0)
        {
            if (g_float_flag)
            {
                pot = (float)sqrt((double)pot);
            }
            else
            {
                fSqrt14(pot, f_tmp);
                pot = f_tmp;
            }
            pot = (float)(potparam[0] - pot*potparam[1] - 1.0);
        }
        else
        {
            pot = (float)(potparam[0] - 1.0);
        }
        if (pot < 1.0F)
        {
            pot = 1.0F; // avoid color 0
        }
    }
    else if (g_inside >= COLOR_BLACK)
    {
        pot = (float) g_inside;
    }
    else     // inside < 0 implies inside = maxit, so use 1st pot param instead
    {
        pot = (float)potparam[0];
    }

    l_pot = (long) (pot * 256);
    i_pot = (int) (l_pot >> 8);
    if (i_pot >= g_colors)
    {
        i_pot = g_colors - 1;
        l_pot = 255;
    }

    if (pot16bit)
    {
        if (!driver_diskp())   // if putcolor won't be doing it for us
        {
            writedisk(col+sxoffs, row+syoffs, i_pot);
        }
        writedisk(col+sxoffs, row+sydots+syoffs, (int)l_pot);
    }

    return i_pot;
}


/******************* boundary trace method ***************************/
#define bkcolor 0

inline direction advance(direction dir, int increment)
{
    return static_cast<direction>((static_cast<int>(dir) + increment) % 4);
}

inline void advance_match(direction &coming_from)
{
    going_to = advance(going_to, -1);
    coming_from = advance(going_to, -1);
}

inline void advance_no_match()
{
    going_to = advance(going_to, 1);
}

static
int  bound_trace_main()
{
    direction coming_from;
    unsigned int matches_found;
    int trail_color, fillcolor_used, last_fillcolor_used = -1;
    int max_putline_length;
    int right, left, length;
    if (g_inside == COLOR_BLACK || outside == COLOR_BLACK)
    {
        stopmsg(STOPMSG_NONE, "Boundary tracing cannot be used with inside=0 or outside=0");
        return -1;
    }
    if (g_colors < 16)
    {
        stopmsg(STOPMSG_NONE, "Boundary tracing cannot be used with < 16 colors");
        return -1;
    }

    g_got_status = 2;
    max_putline_length = 0; // reset max_putline_length
    for (int currow = g_i_y_start; currow <= g_i_y_stop; currow++)
    {
        reset_periodicity = true; // reset for a new row
        g_color = bkcolor;
        for (int curcol = g_i_x_start; curcol <= g_i_x_stop; curcol++)
        {
            if (getcolor(curcol, currow) != bkcolor)
            {
                continue;
            }

            trail_color = g_color;
            row = currow;
            col = curcol;
            if ((*calctype)() == -1) // color, row, col are global
            {
                if (show_dot != bkcolor)   // remove show_dot pixel
                {
                    (*plot)(col, row, bkcolor);
                }
                if (g_i_y_stop != yystop)
                {
                    g_i_y_stop = yystop - (currow - yystart); // allow for sym
                }
                add_worklist(xxstart, xxstop, curcol, currow, g_i_y_stop, currow, 0, worksym);
                return -1;
            }
            reset_periodicity = false; // normal periodicity checking

            /*
            This next line may cause a few more pixels to be calculated,
            but at the savings of quite a bit of overhead
            */
            if (g_color != trail_color)
            {
                continue;
            }

            // sweep clockwise to trace outline
            trail_row = currow;
            trail_col = curcol;
            trail_color = g_color;
            fillcolor_used = g_fill_color > 0 ? g_fill_color : trail_color;
            coming_from = direction::West;
            going_to = direction::East;
            matches_found = 0;
            bool continue_loop = true;
            do
            {
                step_col_row();
                if (row >= currow
                        && col >= g_i_x_start
                        && col <= g_i_x_stop
                        && row <= g_i_y_stop)
                {
                    // the order of operations in this next line is critical
                    g_color = getcolor(col, row);
                    if (g_color == bkcolor && (*calctype)() == -1)
                        // color, row, col are global for (*calctype)()
                    {
                        if (show_dot != bkcolor)   // remove show_dot pixel
                        {
                            (*plot)(col, row, bkcolor);
                        }
                        if (g_i_y_stop != yystop)
                        {
                            g_i_y_stop = yystop - (currow - yystart); // allow for sym
                        }
                        add_worklist(xxstart, xxstop, curcol, currow, g_i_y_stop, currow, 0, worksym);
                        return -1;
                    }
                    if (g_color == trail_color)
                    {
                        if (matches_found < 4)   // to keep it from overflowing
                        {
                            matches_found++;
                        }
                        trail_row = row;
                        trail_col = col;
                        advance_match(coming_from);
                    }
                    else
                    {
                        advance_no_match();
                        continue_loop = (going_to != coming_from) || (matches_found > 0);
                    }
                }
                else
                {
                    advance_no_match();
                    continue_loop = (going_to != coming_from) || (matches_found > 0);
                }
            }
            while (continue_loop && (col != curcol || row != currow));

            if (matches_found <= 3)
            {
                // no hole
                g_color = bkcolor;
                reset_periodicity = true;
                continue;
            }

            /*
            Fill in region by looping around again, filling lines to the left
            whenever going_to is South or West
            */
            trail_row = currow;
            trail_col = curcol;
            coming_from = direction::West;
            going_to = direction::East;
            do
            {
                bool match_found = false;
                do
                {
                    step_col_row();
                    if (row >= currow
                            && col >= g_i_x_start
                            && col <= g_i_x_stop
                            && row <= g_i_y_stop
                            && getcolor(col, row) == trail_color)
                        // getcolor() must be last
                    {
                        if (going_to == direction::South
                                || (going_to == direction::West && coming_from != direction::East))
                        {
                            // fill a row, but only once
                            right = col;
                            while (--right >= g_i_x_start)
                            {
                                g_color = getcolor(right, row);
                                if (g_color == trail_color)
                                {
                                    break;
                                }
                            }
                            if (g_color == bkcolor) // check last color
                            {
                                left = right;
                                while (getcolor(--left, row) == bkcolor)
                                {
                                    // Should NOT be possible for left < ixstart
                                    ; // do nothing
                                }
                                left++; // one pixel too far
                                if (right == left)   // only one hole
                                {
                                    (*plot)(left, row, fillcolor_used);
                                }
                                else
                                {
                                    // fill the line to the left
                                    length = right-left+1;
                                    if (fillcolor_used != last_fillcolor_used || length > max_putline_length)
                                    {
                                        // only reset dstack if necessary
                                        memset(dstack, fillcolor_used, length);
                                        last_fillcolor_used = fillcolor_used;
                                        max_putline_length = length;
                                    }
                                    sym_fill_line(row, left, right, dstack);
                                }
                            } // end of fill line
                        }
                        trail_row = row;
                        trail_col = col;
                        advance_match(coming_from);
                        match_found = true;
                    }
                    else
                    {
                        advance_no_match();
                    }
                }
                while (!match_found && going_to != coming_from);

                if (!match_found)
                {
                    // next one has to be a match
                    step_col_row();
                    trail_row = row;
                    trail_col = col;
                    advance_match(coming_from);
                }
            }
            while (trail_col != curcol || trail_row != currow);
            reset_periodicity = true; // reset after a trace/fill
            g_color = bkcolor;
        }
    }
    return 0;
}

/*******************************************************************/
// take one step in the direction of going_to
static void step_col_row()
{
    switch (going_to)
    {
    case direction::North:
        col = trail_col;
        row = trail_row - 1;
        break;
    case direction::East:
        col = trail_col + 1;
        row = trail_row;
        break;
    case direction::South:
        col = trail_col;
        row = trail_row + 1;
        break;
    case direction::West:
        col = trail_col - 1;
        row = trail_row;
        break;
    }
}

/******************* end of boundary trace method *******************/


/************************ super solid guessing *****************************/

/*
   I, Timothy Wegner, invented this solidguessing idea and implemented it in
   more or less the overall framework you see here.  I am adding this note
   now in a possibly vain attempt to secure my place in history, because
   Pieter Branderhorst has totally rewritten this routine, incorporating
   a *MUCH* more sophisticated algorithm.  His revised code is not only
   faster, but is also more accurate. Harrumph!
*/

static int solid_guess()
{
    int i;
    int xlim;
    int ylim;
    int blocksize;
    unsigned int *pfxp0, *pfxp1;
    unsigned int u;

    guessplot = (plot != putcolor && plot != symplot2 && plot != symplot2J);
    // check if guessing at bottom & right edges is ok
    bottom_guess = (plot == symplot2 || (plot == putcolor && g_i_y_stop+1 == ydots));
    right_guess  = (plot == symplot2J
                    || ((plot == putcolor || plot == symplot2) && g_i_x_stop+1 == xdots));

    // there seems to be a bug in solid guessing at bottom and side
    if (g_debug_flag != debug_flags::force_solid_guess_error)
    {
        bottom_guess = false;
        right_guess = false;
    }

    blocksize = ssg_blocksize();
    maxblock = blocksize;
    i = maxblock;
    totpasses = 1;
    while ((i >>= 1) > 1)
    {
        ++totpasses;
    }

    /* ensure window top and left are on required boundary, treat window
          as larger than it really is if necessary (this is the reason symplot
          routines must check for > xdots/ydots before plotting sym points) */
    g_i_x_start &= -1 - (maxblock-1);
    g_i_y_start = yybegin;
    g_i_y_start &= -1 - (maxblock-1);

    g_got_status = 1;

    if (workpass == 0) // otherwise first pass already done
    {
        // first pass, calc every blocksize**2 pixel, quarter result & paint it
        g_current_pass = 1;
        if (g_i_y_start <= yystart) // first time for this window, init it
        {
            g_current_row = 0;
            memset(&tprefix[1][0][0], 0, maxxblk*maxyblk*2); // noskip flags off
            reset_periodicity = true;
            row = g_i_y_start;
            for (col = g_i_x_start; col <= g_i_x_stop; col += maxblock)
            {
                // calc top row
                if ((*calctype)() == -1)
                {
                    add_worklist(xxstart, xxstop, xxbegin, yystart, yystop, yybegin, 0, worksym);
                    goto exit_solidguess;
                }
                reset_periodicity = false;
            }
        }
        else
        {
            memset(&tprefix[1][0][0], -1, maxxblk*maxyblk*2); // noskip flags on
        }
        for (int y = g_i_y_start; y <= g_i_y_stop; y += blocksize)
        {
            g_current_row = y;
            i = 0;
            if (y+blocksize <= g_i_y_stop)
            {
                // calc the row below
                row = y+blocksize;
                reset_periodicity = true;
                for (col = g_i_x_start; col <= g_i_x_stop; col += maxblock)
                {
                    i = (*calctype)();
                    if (i == -1)
                    {
                        break;
                    }
                    reset_periodicity = false;
                }
            }
            reset_periodicity = false;
            if (i == -1 || guessrow(true, y, blocksize)) // interrupted?
            {
                if (y < yystart)
                {
                    y = yystart;
                }
                add_worklist(xxstart, xxstop, xxstart, yystart, yystop, y, 0, worksym);
                goto exit_solidguess;
            }
        }

        if (num_worklist) // work list not empty, just do 1st pass
        {
            add_worklist(xxstart, xxstop, xxstart, yystart, yystop, yystart, 1, worksym);
            goto exit_solidguess;
        }
        ++workpass;
        g_i_y_start = yystart & (-1 - (maxblock-1));

        // calculate skip flags for skippable blocks
        xlim = (g_i_x_stop+maxblock)/maxblock+1;
        ylim = ((g_i_y_stop+maxblock)/maxblock+15)/16+1;
        if (!right_guess)         // no right edge guessing, zap border
        {
            for (int y = 0; y <= ylim; ++y)
            {
                tprefix[1][y][xlim] = 0xffff;
            }
        }
        if (!bottom_guess)      // no bottom edge guessing, zap border
        {
            i = (g_i_y_stop+maxblock)/maxblock+1;
            int y = i/16+1;
            i = 1 << (i&15);
            for (int x = 0; x <= xlim; ++x)
            {
                tprefix[1][y][x] |= i;
            }
        }
        // set each bit in tprefix[0] to OR of it & surrounding 8 in tprefix[1]
        for (int y = 0; ++y < ylim;)
        {
            pfxp0 = (unsigned int *)&tprefix[0][y][0];
            pfxp1 = (unsigned int *)&tprefix[1][y][0];
            for (int x = 0; ++x < xlim;)
            {
                ++pfxp1;
                u = *(pfxp1-1)|*pfxp1|*(pfxp1+1);
                *(++pfxp0) = u|(u >> 1)|(u << 1)
                             |((*(pfxp1-(maxxblk+1))|*(pfxp1-maxxblk)|*(pfxp1-(maxxblk-1))) >> 15)
                             |((*(pfxp1+(maxxblk-1))|*(pfxp1+maxxblk)|*(pfxp1+(maxxblk+1))) << 15);
            }
        }
    }
    else   // first pass already done
    {
        memset(&tprefix[0][0][0], -1, maxxblk*maxyblk*2); // noskip flags on
    }
    if (three_pass)
    {
        goto exit_solidguess;
    }

    // remaining pass(es), halve blocksize & quarter each blocksize**2
    i = workpass;
    while (--i > 0)   // allow for already done passes
    {
        blocksize = blocksize >> 1;
    }
    reset_periodicity = false;
    while ((blocksize = blocksize >> 1) >= 2)
    {
        if (stoppass > 0)
        {
            if (workpass >= stoppass)
            {
                goto exit_solidguess;
            }
        }
        g_current_pass = workpass + 1;
        for (int y = g_i_y_start; y <= g_i_y_stop; y += blocksize)
        {
            g_current_row = y;
            if (guessrow(false, y, blocksize))
            {
                if (y < yystart)
                {
                    y = yystart;
                }
                add_worklist(xxstart, xxstop, xxstart, yystart, yystop, y, workpass, worksym);
                goto exit_solidguess;
            }
        }
        ++workpass;
        if (num_worklist // work list not empty, do one pass at a time
                && blocksize > 2) // if 2, we just did last pass
        {
            add_worklist(xxstart, xxstop, xxstart, yystart, yystop, yystart, workpass, worksym);
            goto exit_solidguess;
        }
        g_i_y_start = yystart & (-1 - (maxblock-1));
    }

exit_solidguess:
    return 0;
}

#define calcadot(c, x, y)     \
{                           \
    col = x;                  \
    row = y;                  \
    c = (*calctype)();        \
    if (c == -1)            \
        return true;        \
}

static bool guessrow(bool firstpass, int y, int blocksize)
{
    int j;
    int color;
    int xplushalf, xplusblock;
    int ylessblock, ylesshalf, yplushalf, yplusblock;
    int     c21, c31, c41;         // cxy is the color of pixel at (x,y)
    int c12, c22, c32, c42;         // where c22 is the topleft corner of
    int c13, c23, c33;             // the block being handled in current
    int     c24,    c44;         // iteration
    int guessed23, guessed32, guessed33, guessed12, guessed13;
    int prev11, fix21, fix31;
    unsigned int *pfxptr, pfxmask;

    c42 = 0;  // just for warning
    c41 = c42;
    c44 = c41;

    halfblock = blocksize >> 1;
    int i = y/maxblock;
    pfxptr = (unsigned int *) &tprefix[firstpass ? 1 : 0][(i >> 4) + 1][g_i_x_start/maxblock];
    pfxmask = 1 << (i & 15);
    ylesshalf = y - halfblock;
    ylessblock = y - blocksize; // constants, for speed
    yplushalf = y + halfblock;
    yplusblock = y + blocksize;
    prev11 = -1;
    c22 = getcolor(g_i_x_start, y);
    c13 = c22;
    c12 = c13;
    c24 = c12;
    c21 = getcolor(g_i_x_start, (y > 0)?ylesshalf:0);
    c31 = c21;
    if (yplusblock <= g_i_y_stop)
    {
        c24 = getcolor(g_i_x_start, yplusblock);
    }
    else if (!bottom_guess)
    {
        c24 = -1;
    }
    guessed13 = 0;
    guessed12 = guessed13;

    for (int x = g_i_x_start; x <= g_i_x_stop;)   // increment at end, or when doing continue
    {
        if ((x&(maxblock-1)) == 0)  // time for skip flag stuff
        {
            ++pfxptr;
            if (!firstpass && (*pfxptr&pfxmask) == 0)  // check for fast skip
            {
                x += maxblock;
                c13 = c22;
                c12 = c13;
                c24 = c12;
                c21 = c24;
                c31 = c21;
                prev11 = c31;
                guessed13 = 0;
                guessed12 = guessed13;
                continue;
            }
        }

        if (firstpass)    // 1st pass, paint topleft corner
        {
            plotblock(0, x, y, c22);
        }
        // setup variables
        xplushalf = x + halfblock;
        xplusblock = xplushalf + halfblock;
        if (xplushalf > g_i_x_stop)
        {
            if (!right_guess)
            {
                c31 = -1;
            }
        }
        else if (y > 0)
        {
            c31 = getcolor(xplushalf, ylesshalf);
        }
        if (xplusblock <= g_i_x_stop)
        {
            if (yplusblock <= g_i_y_stop)
            {
                c44 = getcolor(xplusblock, yplusblock);
            }
            c41 = getcolor(xplusblock, (y > 0)?ylesshalf:0);
            c42 = getcolor(xplusblock, y);
        }
        else if (!right_guess)
        {
            c44 = -1;
            c42 = c44;
            c41 = c42;
        }
        if (yplusblock > g_i_y_stop)
        {
            c44 = bottom_guess ? c42 : -1;
        }

        // guess or calc the remaining 3 quarters of current block
        guessed33 = 1;
        guessed32 = guessed33;
        guessed23 = guessed32;
        c33 = c22;
        c32 = c33;
        c23 = c32;
        if (yplushalf > g_i_y_stop)
        {
            if (!bottom_guess)
            {
                c33 = -1;
                c23 = c33;
            }
            guessed33 = -1;
            guessed23 = guessed33;
            guessed13 = 0;
        }
        if (xplushalf > g_i_x_stop)
        {
            if (!right_guess)
            {
                c33 = -1;
                c32 = c33;
            }
            guessed33 = -1;
            guessed32 = guessed33;
        }
        while (1) // go around till none of 23,32,33 change anymore
        {
            if (guessed33 > 0
                    && (c33 != c44 || c33 != c42 || c33 != c24 || c33 != c32 || c33 != c23))
            {
                calcadot(c33, xplushalf, yplushalf);
                guessed33 = 0;
            }
            if (guessed32 > 0
                    && (c32 != c33 || c32 != c42 || c32 != c31 || c32 != c21
                        || c32 != c41 || c32 != c23))
            {
                calcadot(c32, xplushalf, y);
                guessed32 = 0;
                continue;
            }
            if (guessed23 > 0
                    && (c23 != c33 || c23 != c24 || c23 != c13 || c23 != c12 || c23 != c32))
            {
                calcadot(c23, x, yplushalf);
                guessed23 = 0;
                continue;
            }
            break;
        }

        if (firstpass)   // note whether any of block's contents were calculated
        {
            if (guessed23 == 0 || guessed32 == 0 || guessed33 == 0)
            {
                *pfxptr |= pfxmask;
            }
        }

        if (halfblock > 1)
        {
            // not last pass, check if something to display
            if (firstpass)  // display guessed corners, fill in block
            {
                if (guessplot)
                {
                    if (guessed23 > 0)
                    {
                        (*plot)(x, yplushalf, c23);
                    }
                    if (guessed32 > 0)
                    {
                        (*plot)(xplushalf, y, c32);
                    }
                    if (guessed33 > 0)
                    {
                        (*plot)(xplushalf, yplushalf, c33);
                    }
                }
                plotblock(1, x, yplushalf, c23);
                plotblock(0, xplushalf, y, c32);
                plotblock(1, xplushalf, yplushalf, c33);
            }
            else  // repaint changed blocks
            {
                if (c23 != c22)
                {
                    plotblock(-1, x, yplushalf, c23);
                }
                if (c32 != c22)
                {
                    plotblock(-1, xplushalf, y, c32);
                }
                if (c33 != c22)
                {
                    plotblock(-1, xplushalf, yplushalf, c33);
                }
            }
        }

        // check if some calcs in this block mean earlier guesses need fixing
        fix21 = ((c22 != c12 || c22 != c32)
                 && c21 == c22 && c21 == c31 && c21 == prev11
                 && y > 0
                 && (x == g_i_x_start || c21 == getcolor(x-halfblock, ylessblock))
                 && (xplushalf > g_i_x_stop || c21 == getcolor(xplushalf, ylessblock))
                 && c21 == getcolor(x, ylessblock));
        fix31 = (c22 != c32
                 && c31 == c22 && c31 == c42 && c31 == c21 && c31 == c41
                 && y > 0 && xplushalf <= g_i_x_stop
                 && c31 == getcolor(xplushalf, ylessblock)
                 && (xplusblock > g_i_x_stop || c31 == getcolor(xplusblock, ylessblock))
                 && c31 == getcolor(x, ylessblock));
        prev11 = c31; // for next time around
        if (fix21)
        {
            calcadot(c21, x, ylesshalf);
            if (halfblock > 1 && c21 != c22)
            {
                plotblock(-1, x, ylesshalf, c21);
            }
        }
        if (fix31)
        {
            calcadot(c31, xplushalf, ylesshalf);
            if (halfblock > 1 && c31 != c22)
            {
                plotblock(-1, xplushalf, ylesshalf, c31);
            }
        }
        if (c23 != c22)
        {
            if (guessed12)
            {
                calcadot(c12, x-halfblock, y);
                if (halfblock > 1 && c12 != c22)
                {
                    plotblock(-1, x-halfblock, y, c12);
                }
            }
            if (guessed13)
            {
                calcadot(c13, x-halfblock, yplushalf);
                if (halfblock > 1 && c13 != c22)
                {
                    plotblock(-1, x-halfblock, yplushalf, c13);
                }
            }
        }
        c22 = c42;
        c24 = c44;
        c13 = c33;
        c21 = c41;
        c31 = c21;
        c12 = c32;
        guessed12 = guessed32;
        guessed13 = guessed33;
        x += blocksize;
    } // end x loop

    if (!firstpass || guessplot)
    {
        return false;
    }

    // paint rows the fast way
    for (int i = 0; i < halfblock; ++i)
    {
        j = y+i;
        if (j <= g_i_y_stop)
        {
            put_line(j, xxstart, g_i_x_stop, &dstack[xxstart]);
        }
        j = y+i+halfblock;
        if (j <= g_i_y_stop)
        {
            put_line(j, xxstart, g_i_x_stop, &dstack[xxstart+OLDMAXPIXELS]);
        }
        if (driver_key_pressed())
        {
            return true;
        }
    }
    if (plot != putcolor)  // symmetry, just vertical & origin the fast way
    {
        if (plot == symplot2J)   // origin sym, reverse lines
        {
            for (int i = (g_i_x_stop+xxstart+1)/2; --i >= xxstart;)
            {
                color = dstack[i];
                j = g_i_x_stop - (i - xxstart);
                dstack[i] = dstack[j];
                dstack[j] = (BYTE)color;
                j += OLDMAXPIXELS;
                color = dstack[i + OLDMAXPIXELS];
                dstack[i + OLDMAXPIXELS] = dstack[j];
                dstack[j] = (BYTE)color;
            }
        }
        for (int i = 0; i < halfblock; ++i)
        {
            j = yystop-(y+i-yystart);
            if (j > g_i_y_stop && j < ydots)
            {
                put_line(j, xxstart, g_i_x_stop, &dstack[xxstart]);
            }
            j = yystop-(y+i+halfblock-yystart);
            if (j > g_i_y_stop && j < ydots)
            {
                put_line(j, xxstart, g_i_x_stop, &dstack[xxstart+OLDMAXPIXELS]);
            }
            if (driver_key_pressed())
            {
                return true;
            }
        }
    }
    return false;
}
#undef calcadot

static void plotblock(int buildrow, int x, int y, int color)
{
    int xlim, ylim;
    xlim = x+halfblock;
    if (xlim > g_i_x_stop)
    {
        xlim = g_i_x_stop+1;
    }
    if (buildrow >= 0 && !guessplot) // save it for later put_line
    {
        if (buildrow == 0)
        {
            for (int i = x; i < xlim; ++i)
            {
                dstack[i] = (BYTE)color;
            }
        }
        else
        {
            for (int i = x; i < xlim; ++i)
            {
                dstack[i+OLDMAXPIXELS] = (BYTE)color;
            }
        }
        if (x >= xxstart)   // when x reduced for alignment, paint those dots too
        {
            return; // the usual case
        }
    }
    // paint it
    ylim = y+halfblock;
    if (ylim > g_i_y_stop)
    {
        if (y > g_i_y_stop)
        {
            return;
        }
        ylim = g_i_y_stop+1;
    }
    for (int i = x; ++i < xlim;)
    {
        (*plot)(i, y, color); // skip 1st dot on 1st row
    }
    while (++y < ylim)
    {
        for (int i = x; i < xlim; ++i)
        {
            (*plot)(i, y, color);
        }
    }
}


/************************* symmetry plot setup ************************/

static bool xsym_split(int xaxis_row, bool xaxis_between)
{
    if ((worksym&0x11) == 0x10)   // already decided not sym
    {
        return true;
    }
    if ((worksym&1) != 0)   // already decided on sym
    {
        g_i_y_stop = (yystart+yystop)/2;
    }
    else   // new window, decide
    {
        worksym |= 0x10;
        if (xaxis_row <= yystart || xaxis_row >= yystop)
        {
            return true; // axis not in window
        }
        int i = xaxis_row + (xaxis_row - yystart);
        if (xaxis_between)
        {
            ++i;
        }
        if (i > yystop) // split into 2 pieces, bottom has the symmetry
        {
            if (num_worklist >= MAXCALCWORK-1)   // no room to split
            {
                return true;
            }
            g_i_y_stop = xaxis_row - (yystop - xaxis_row);
            if (!xaxis_between)
            {
                --g_i_y_stop;
            }
            add_worklist(xxstart, xxstop, xxstart, g_i_y_stop+1, yystop, g_i_y_stop+1, workpass, 0);
            yystop = g_i_y_stop;
            return true; // tell set_symmetry no sym for current window
        }
        if (i < yystop) // split into 2 pieces, top has the symmetry
        {
            if (num_worklist >= MAXCALCWORK-1)   // no room to split
            {
                return true;
            }
            add_worklist(xxstart, xxstop, xxstart, i+1, yystop, i+1, workpass, 0);
            yystop = i;
        }
        g_i_y_stop = xaxis_row;
        worksym |= 1;
    }
    symmetry = symmetry_type::NONE;
    return false; // tell set_symmetry its a go
}

static bool ysym_split(int yaxis_col, bool yaxis_between)
{
    if ((worksym&0x22) == 0x20)   // already decided not sym
    {
        return true;
    }
    if ((worksym&2) != 0)   // already decided on sym
    {
        g_i_x_stop = (xxstart+xxstop)/2;
    }
    else   // new window, decide
    {
        worksym |= 0x20;
        if (yaxis_col <= xxstart || yaxis_col >= xxstop)
        {
            return true; // axis not in window
        }
        int i = yaxis_col + (yaxis_col - xxstart);
        if (yaxis_between)
        {
            ++i;
        }
        if (i > xxstop) // split into 2 pieces, right has the symmetry
        {
            if (num_worklist >= MAXCALCWORK-1)   // no room to split
            {
                return true;
            }
            g_i_x_stop = yaxis_col - (xxstop - yaxis_col);
            if (!yaxis_between)
            {
                --g_i_x_stop;
            }
            add_worklist(g_i_x_stop+1, xxstop, g_i_x_stop+1, yystart, yystop, yystart, workpass, 0);
            xxstop = g_i_x_stop;
            return true; // tell set_symmetry no sym for current window
        }
        if (i < xxstop) // split into 2 pieces, left has the symmetry
        {
            if (num_worklist >= MAXCALCWORK-1)   // no room to split
            {
                return true;
            }
            add_worklist(i+1, xxstop, i+1, yystart, yystop, yystart, workpass, 0);
            xxstop = i;
        }
        g_i_x_stop = yaxis_col;
        worksym |= 2;
    }
    symmetry = symmetry_type::NONE;
    return false; // tell set_symmetry its a go
}

static void setsymmetry(symmetry_type sym, bool uselist) // set up proper symmetrical plot functions
{
    int i;
    int xaxis_row, yaxis_col;           // pixel number for origin
    bool xaxis_between = false;
    bool yaxis_between = false;         // if axis between 2 pixels, not on one
    bool xaxis_on_screen = false;
    bool yaxis_on_screen = false;
    double ftemp;
    bf_t bft1;
    int saved = 0;
    symmetry = symmetry_type::X_AXIS;
    if (stdcalcmode == 's' || stdcalcmode == 'o')
    {
        return;
    }
    if (sym == symmetry_type::NO_PLOT && g_force_symmetry == symmetry_type::NOT_FORCED)
    {
        plot = noplot;
        return;
    }
    // NOTE: 16-bit potential disables symmetry
    // also any decomp= option and any inversion not about the origin
    // also any rotation other than 180deg and any off-axis stretch
    if (bf_math != bf_math_type::NONE)
    {
        if (cmp_bf(bfxmin, bfx3rd) || cmp_bf(bfymin, bfy3rd))
        {
            return;
        }
    }
    if ((potflag && pot16bit) || ((g_invert != 0) && g_inversion[2] != 0.0)
            || g_decomp[0] != 0
            || xxmin != xx3rd || yymin != yy3rd)
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
    else if (outside == REAL || outside == IMAG || outside == MULT || outside == SUM
             || outside == ATAN || g_bail_out_test == bailouts::Manr || outside == FMOD)
    {
        return;
    }
    else if (g_inside == FMODI || outside == TDIS)
    {
        return;
    }
    bool parmszero = (parm.x == 0.0 && parm.y == 0.0 && useinitorbit != 1);
    bool parmsnoreal = (parm.x == 0.0 && useinitorbit != 1);
    bool parmsnoimag = (parm.y == 0.0 && useinitorbit != 1);
    switch (fractype)
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
        parmszero = (parmszero && param[2] == 0.0 && param[3] == 0.0
                     && param[4] == 0.0 && param[5] == 0.0
                     && param[6] == 0.0 && param[7] == 0.0
                     && param[8] == 0.0 && param[9] == 0.0);
        parmsnoreal = (parmsnoreal && param[2] == 0.0 && param[4] == 0.0
                       && param[6] == 0.0 && param[8] == 0.0);
        parmsnoimag = (parmsnoimag && param[3] == 0.0 && param[5] == 0.0
                       && param[7] == 0.0 && param[9] == 0.0);
        break;
    default:   // Check P2 for the rest
        parmszero = (parmszero && parm2.x == 0.0 && parm2.y == 0.0);
    }
    yaxis_col = -1;
    xaxis_row = yaxis_col;
    if (bf_math != bf_math_type::NONE)
    {
        saved = save_stack();
        bft1    = alloc_stack(rbflength+2);
        xaxis_on_screen = (sign_bf(bfymin) != sign_bf(bfymax));
        yaxis_on_screen = (sign_bf(bfxmin) != sign_bf(bfxmax));
    }
    else
    {
        xaxis_on_screen = (sign(yymin) != sign(yymax));
        yaxis_on_screen = (sign(xxmin) != sign(xxmax));
    }
    if (xaxis_on_screen) // axis is on screen
    {
        if (bf_math != bf_math_type::NONE)
        {
            sub_bf(bft1, bfymin, bfymax);
            div_bf(bft1, bfymax, bft1);
            neg_a_bf(bft1);
            ftemp = (double)bftofloat(bft1);
        }
        else
        {
            ftemp = (0.0-yymax) / (yymin-yymax);
        }
        ftemp *= (ydots-1);
        ftemp += 0.25;
        xaxis_row = (int)ftemp;
        xaxis_between = (ftemp - xaxis_row >= 0.5);
        if (!uselist && (!xaxis_between || (xaxis_row+1)*2 != ydots))
        {
            xaxis_row = -1; // can't split screen, so dead center or not at all
        }
    }
    if (yaxis_on_screen) // axis is on screen
    {
        if (bf_math != bf_math_type::NONE)
        {
            sub_bf(bft1, bfxmax, bfxmin);
            div_bf(bft1, bfxmin, bft1);
            neg_a_bf(bft1);
            ftemp = (double)bftofloat(bft1);
        }
        else
        {
            ftemp = (0.0-xxmin) / (xxmax-xxmin);
        }
        ftemp *= (xdots-1);
        ftemp += 0.25;
        yaxis_col = (int)ftemp;
        yaxis_between = (ftemp - yaxis_col >= 0.5);
        if (!uselist && (!yaxis_between || (yaxis_col+1)*2 != xdots))
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
                plot = symplot2basin;
            }
            else
            {
                plot = symplot2;
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
            plot = symplot2Y;
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
        switch (worksym & 3)
        {
        case 1: // just xaxis symmetry
            if (g_basin)
            {
                plot = symplot2basin;
            }
            else
            {
                plot = symplot2 ;
            }
            break;
        case 2: // just yaxis symmetry
            if (g_basin) // got no routine for this case
            {
                g_i_x_stop = xxstop; // fix what split should not have done
                symmetry = symmetry_type::X_AXIS;
            }
            else
            {
                plot = symplot2Y;
            }
            break;
        case 3: // both axes
            if (g_basin)
            {
                plot = symplot4basin;
            }
            else
            {
                plot = symplot4 ;
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
            plot = symplot2J;
            g_i_x_stop = xxstop; // didn't want this changed
        }
        else
        {
            g_i_y_stop = yystop; // in case first split worked
            symmetry = symmetry_type::X_AXIS;
            worksym = 0x30; // let it recombine with others like it
        }
        break;
    case symmetry_type::PI_SYM_NO_PARAM:
        if (!parmszero)
        {
            break;
        }
    case symmetry_type::PI_SYM:                      // PI symmetry
        if (bf_math != bf_math_type::NONE)
        {
            if ((double)bftofloat(abs_a_bf(sub_bf(bft1, bfxmax, bfxmin))) < PI/4)
            {
                break; // no point in pi symmetry if values too close
            }
        }
        else
        {
            if (fabs(xxmax - xxmin) < PI/4)
            {
                break; // no point in pi symmetry if values too close
            }
        }
        if ((g_invert != 0) && g_force_symmetry == symmetry_type::NOT_FORCED)
        {
            goto originsym;
        }
        plot = symPIplot ;
        symmetry = symmetry_type::NONE;
        if (!xsym_split(xaxis_row, xaxis_between)
                && !ysym_split(yaxis_col, yaxis_between))
        {
            if (parm.y == 0.0)
            {
                plot = symPIplot4J; // both axes
            }
            else
            {
                plot = symPIplot2J; // origin
            }
        }
        else
        {
            g_i_y_stop = yystop; // in case first split worked
            worksym = 0x30;  // don't mark pisym as ysym, just do it unmarked
        }
        if (bf_math != bf_math_type::NONE)
        {
            sub_bf(bft1, bfxmax, bfxmin);
            abs_a_bf(bft1);
            pixelpi = (int)((PI/(double)bftofloat(bft1)*xdots)); // PI in pixels
        }
        else
        {
            pixelpi = (int)((PI/fabs(xxmax-xxmin))*xdots); // PI in pixels
        }

        g_i_x_stop = xxstart+pixelpi-1;
        if (g_i_x_stop > xxstop)
        {
            g_i_x_stop = xxstop;
        }
        i = (xxstart+xxstop)/2;
        if (plot == symPIplot4J && g_i_x_stop > i)
        {
            g_i_x_stop = i;
        }
        break;
    default:                  // no symmetry
        break;
    }
    if (bf_math != bf_math_type::NONE)
    {
        restore_stack(saved);
    }
}

/**************** tesseral method by CJLT begins here*********************/

struct tess             // one of these per box to be done gets stacked
{
    int x1, x2, y1, y2;      // left/right top/bottom x/y coords
    int top, bot, lft, rgt;  // edge colors, -1 mixed, -2 unknown
};

static int tesseral()
{
    tess *tp;

    guessplot = (plot != putcolor && plot != symplot2);
    tp = (tess *)&dstack[0];
    tp->x1 = g_i_x_start;                              // set up initial box
    tp->x2 = g_i_x_stop;
    tp->y1 = g_i_y_start;
    tp->y2 = g_i_y_stop;

    if (workpass == 0) // not resuming
    {
        tp->top = tessrow(g_i_x_start, g_i_x_stop, g_i_y_start);     // Do top row
        tp->bot = tessrow(g_i_x_start, g_i_x_stop, g_i_y_stop);      // Do bottom row
        tp->lft = tesscol(g_i_x_start, g_i_y_start+1, g_i_y_stop-1); // Do left column
        tp->rgt = tesscol(g_i_x_stop, g_i_y_start+1, g_i_y_stop-1);  // Do right column
        if (check_key())
        {
            // interrupt before we got properly rolling
            add_worklist(xxstart, xxstop, xxstart, yystart, yystop, yystart, 0, worksym);
            return -1;
        }
    }
    else // resuming, rebuild work stack
    {
        int i, mid, curx, cury, xsize, ysize;
        tess *tp2;
        tp->rgt = -2;
        tp->lft = tp->rgt;
        tp->bot = tp->lft;
        tp->top = tp->bot;
        cury = yybegin & 0xfff;
        ysize = 1;
        i = (unsigned)yybegin >> 12;
        while (--i >= 0)
        {
            ysize <<= 1;
        }
        curx = workpass & 0xfff;
        xsize = 1;
        i = (unsigned)workpass >> 12;
        while (--i >= 0)
        {
            xsize <<= 1;
        }
        while (1)
        {
            tp2 = tp;
            if (tp->x2 - tp->x1 > tp->y2 - tp->y1)
            {
                // next divide down middle
                if (tp->x1 == curx && (tp->x2 - tp->x1 - 2) < xsize)
                {
                    break;
                }
                mid = (tp->x1 + tp->x2) >> 1;                // Find mid point
                if (mid > curx)
                {
                    // stack right part
                    memcpy(++tp, tp2, sizeof(*tp));
                    tp->x2 = mid;
                }
                tp2->x1 = mid;
            }
            else
            {
                // next divide across
                if (tp->y1 == cury && (tp->y2 - tp->y1 - 2) < ysize)
                {
                    break;
                }
                mid = (tp->y1 + tp->y2) >> 1;                // Find mid point
                if (mid > cury)
                {
                    // stack bottom part
                    memcpy(++tp, tp2, sizeof(*tp));
                    tp->y2 = mid;
                }
                tp2->y1 = mid;
            }
        }
    }

    g_got_status = 4; // for tab_display

    while (tp >= (tess *)&dstack[0])
    {
        // do next box
        g_current_column = tp->x1; // for tab_display
        g_current_row = tp->y1;

        if (tp->top == -1 || tp->bot == -1 || tp->lft == -1 || tp->rgt == -1)
        {
            goto tess_split;
        }
        // for any edge whose color is unknown, set it
        if (tp->top == -2)
        {
            tp->top = tesschkrow(tp->x1, tp->x2, tp->y1);
        }
        if (tp->top == -1)
        {
            goto tess_split;
        }
        if (tp->bot == -2)
        {
            tp->bot = tesschkrow(tp->x1, tp->x2, tp->y2);
        }
        if (tp->bot != tp->top)
        {
            goto tess_split;
        }
        if (tp->lft == -2)
        {
            tp->lft = tesschkcol(tp->x1, tp->y1, tp->y2);
        }
        if (tp->lft != tp->top)
        {
            goto tess_split;
        }
        if (tp->rgt == -2)
        {
            tp->rgt = tesschkcol(tp->x2, tp->y1, tp->y2);
        }
        if (tp->rgt != tp->top)
        {
            goto tess_split;
        }

        {
            int mid, midcolor;
            if (tp->x2 - tp->x1 > tp->y2 - tp->y1)
            {
                // divide down the middle
                mid = (tp->x1 + tp->x2) >> 1;           // Find mid point
                midcolor = tesscol(mid, tp->y1+1, tp->y2-1); // Do mid column
                if (midcolor != tp->top)
                {
                    goto tess_split;
                }
            }
            else
            {
                // divide across the middle
                mid = (tp->y1 + tp->y2) >> 1;           // Find mid point
                midcolor = tessrow(tp->x1+1, tp->x2-1, mid); // Do mid row
                if (midcolor != tp->top)
                {
                    goto tess_split;
                }
            }
        }

        {
            // all 4 edges are the same color, fill in
            int i, j;
            i = 0;
            if (g_fill_color != 0)
            {
                if (g_fill_color > 0)
                {
                    tp->top = g_fill_color %g_colors;
                }
                if (guessplot || (j = tp->x2 - tp->x1 - 1) < 2)
                {
                    // paint dots
                    for (col = tp->x1 + 1; col < tp->x2; col++)
                    {
                        for (row = tp->y1 + 1; row < tp->y2; row++)
                        {
                            (*plot)(col, row, tp->top);
                            if (++i > 500)
                            {
                                if (check_key())
                                {
                                    goto tess_end;
                                }
                                i = 0;
                            }
                        }
                    }
                }
                else
                {
                    // use put_line for speed
                    memset(&dstack[OLDMAXPIXELS], tp->top, j);
                    for (row = tp->y1 + 1; row < tp->y2; row++)
                    {
                        put_line(row, tp->x1+1, tp->x2-1, &dstack[OLDMAXPIXELS]);
                        if (plot != putcolor) // symmetry
                        {
                            j = yystop-(row-yystart);
                            if (j > g_i_y_stop && j < ydots)
                            {
                                put_line(j, tp->x1+1, tp->x2-1, &dstack[OLDMAXPIXELS]);
                            }
                        }
                        if (++i > 25)
                        {
                            if (check_key())
                            {
                                goto tess_end;
                            }
                            i = 0;
                        }
                    }
                }
            }
            --tp;
        }
        continue;

tess_split:
        {
            // box not surrounded by same color, sub-divide
            int mid, midcolor;
            tess *tp2;
            if (tp->x2 - tp->x1 > tp->y2 - tp->y1)
            {
                // divide down the middle
                mid = (tp->x1 + tp->x2) >> 1;                // Find mid point
                midcolor = tesscol(mid, tp->y1+1, tp->y2-1); // Do mid column
                if (midcolor == -3)
                {
                    goto tess_end;
                }
                if (tp->x2 - mid > 1)
                {
                    // right part >= 1 column
                    if (tp->top == -1)
                    {
                        tp->top = -2;
                    }
                    if (tp->bot == -1)
                    {
                        tp->bot = -2;
                    }
                    tp2 = tp;
                    if (mid - tp->x1 > 1)
                    {
                        // left part >= 1 col, stack right
                        memcpy(++tp, tp2, sizeof(*tp));
                        tp->x2 = mid;
                        tp->rgt = midcolor;
                    }
                    tp2->x1 = mid;
                    tp2->lft = midcolor;
                }
                else
                {
                    --tp;
                }
            }
            else
            {
                // divide across the middle
                mid = (tp->y1 + tp->y2) >> 1;                // Find mid point
                midcolor = tessrow(tp->x1+1, tp->x2-1, mid); // Do mid row
                if (midcolor == -3)
                {
                    goto tess_end;
                }
                if (tp->y2 - mid > 1)
                {
                    // bottom part >= 1 column
                    if (tp->lft == -1)
                    {
                        tp->lft = -2;
                    }
                    if (tp->rgt == -1)
                    {
                        tp->rgt = -2;
                    }
                    tp2 = tp;
                    if (mid - tp->y1 > 1)
                    {
                        // top also >= 1 col, stack bottom
                        memcpy(++tp, tp2, sizeof(*tp));
                        tp->y2 = mid;
                        tp->bot = midcolor;
                    }
                    tp2->y1 = mid;
                    tp2->top = midcolor;
                }
                else
                {
                    --tp;
                }
            }
        }

    }

tess_end:
    if (tp >= (tess *)&dstack[0])
    {
        // didn't complete
        int i, xsize, ysize;
        ysize = 1;
        xsize = ysize;
        i = 2;
        while (tp->x2 - tp->x1 - 2 >= i)
        {
            i <<= 1;
            ++xsize;
        }
        i = 2;
        while (tp->y2 - tp->y1 - 2 >= i)
        {
            i <<= 1;
            ++ysize;
        }
        add_worklist(xxstart, xxstop, xxstart, yystart, yystop,
                     (ysize << 12)+tp->y1, (xsize << 12)+tp->x1, worksym);
        return -1;
    }
    return 0;

} // tesseral

static int tesschkcol(int x, int y1, int y2)
{
    int i;
    i = getcolor(x, ++y1);
    while (--y2 > y1)
    {
        if (getcolor(x, y2) != i)
        {
            return -1;
        }
    }
    return i;
}

static int tesschkrow(int x1, int x2, int y)
{
    int i;
    i = getcolor(x1, y);
    while (x2 > x1)
    {
        if (getcolor(x2, y) != i)
        {
            return -1;
        }
        --x2;
    }
    return i;
}

static int tesscol(int x, int y1, int y2)
{
    int colcolor, i;
    col = x;
    row = y1;
    reset_periodicity = true;
    colcolor = (*calctype)();
    reset_periodicity = false;
    while (++row <= y2)
    {
        // generate the column
        i = (*calctype)();
        if (i < 0)
        {
            return -3;
        }
        if (i != colcolor)
        {
            colcolor = -1;
        }
    }
    return colcolor;
}

static int tessrow(int x1, int x2, int y)
{
    int rowcolor, i;
    row = y;
    col = x1;
    reset_periodicity = true;
    rowcolor = (*calctype)();
    reset_periodicity = false;
    while (++col <= x2)
    {
        // generate the row
        i = (*calctype)();
        if (i < 0)
        {
            return -3;
        }
        if (i != rowcolor)
        {
            rowcolor = -1;
        }
    }
    return rowcolor;
}

// added for testing autologmap()
// insert at end of CALCFRAC.C

static long autologmap()
{
    // calculate round screen edges to avoid wasted colours in logmap
    long mincolour;
    int xstop = xdots - 1; // don't use symetry
    int ystop = ydots - 1; // don't use symetry
    long old_maxit;
    mincolour = LONG_MAX;
    row = 0;
    reset_periodicity = false;
    old_maxit = maxit;
    for (col = 0; col < xstop; col++) // top row
    {
        g_color = (*calctype)();
        if (g_color == -1)
        {
            goto ack; // key pressed, bailout
        }
        if (realcoloriter < mincolour)
        {
            mincolour = realcoloriter ;
            maxit = std::max(2L, mincolour); /*speedup for when edges overlap lakes */
        }
        if (col >=32)
        {
            (*plot)(col-32, row, 0);
        }
    }                                    // these lines tidy up for BTM etc
    for (int lag = 32; lag > 0; lag--)
    {
        (*plot)(col-lag, row, 0);
    }

    col = xstop;
    for (row = 0; row < ystop; row++) // right  side
    {
        g_color = (*calctype)();
        if (g_color == -1)
        {
            goto ack; // key pressed, bailout
        }
        if (realcoloriter < mincolour)
        {
            mincolour = realcoloriter ;
            maxit = std::max(2L, mincolour); /*speedup for when edges overlap lakes */
        }
        if (row >=32)
        {
            (*plot)(col, row-32, 0);
        }
    }
    for (int lag = 32; lag > 0; lag--)
    {
        (*plot)(col, row-lag, 0);
    }

    col = 0;
    for (row = 0; row < ystop; row++) // left  side
    {
        g_color = (*calctype)();
        if (g_color == -1)
        {
            goto ack; // key pressed, bailout
        }
        if (realcoloriter < mincolour)
        {
            mincolour = realcoloriter ;
            maxit = std::max(2L, mincolour); /*speedup for when edges overlap lakes */
        }
        if (row >=32)
        {
            (*plot)(col, row-32, 0);
        }
    }
    for (int lag = 32; lag > 0; lag--)
    {
        (*plot)(col, row-lag, 0);
    }

    row = ystop ;
    for (col = 0; col < xstop; col++) // bottom row
    {
        g_color = (*calctype)();
        if (g_color == -1)
        {
            goto ack; // key pressed, bailout
        }
        if (realcoloriter < mincolour)
        {
            mincolour = realcoloriter ;
            maxit = std::max(2L, mincolour); /*speedup for when edges overlap lakes */
        }
        if (col >=32)
        {
            (*plot)(col-32, row, 0);
        }
    }
    for (int lag = 32; lag > 0; lag--)
    {
        (*plot)(col-lag, row, 0);
    }

ack: // bailout here if key is pressed
    if (mincolour == 2)      // insure autologmap not called again
    {
        resuming = true;
    }
    maxit = old_maxit;

    return mincolour ;
}

// Symmetry plot for period PI
void symPIplot(int x, int y, int color)
{
    while (x <= xxstop)
    {
        putcolor(x, y, color) ;
        x += pixelpi;
    }
}
// Symmetry plot for period PI plus Origin Symmetry
void symPIplot2J(int x, int y, int color)
{
    int i, j;
    while (x <= xxstop)
    {
        putcolor(x, y, color) ;
        i = yystop-(y-yystart);
        if (i > g_i_y_stop && i < ydots
                && (j = xxstop-(x-xxstart)) < xdots)
        {
            putcolor(j, i, color) ;
        }
        x += pixelpi;
    }
}
// Symmetry plot for period PI plus Both Axis Symmetry
void symPIplot4J(int x, int y, int color)
{
    int i, j;
    while (x <= (xxstart+xxstop)/2)
    {
        j = xxstop-(x-xxstart);
        putcolor(x , y , color) ;
        if (j < xdots)
        {
            putcolor(j , y , color) ;
        }
        i = yystop-(y-yystart);
        if (i > g_i_y_stop && i < ydots)
        {
            putcolor(x , i , color) ;
            if (j < xdots)
            {
                putcolor(j , i , color) ;
            }
        }
        x += pixelpi;
    }
}

// Symmetry plot for X Axis Symmetry
void symplot2(int x, int y, int color)
{
    int i;
    putcolor(x, y, color) ;
    i = yystop-(y-yystart);
    if (i > g_i_y_stop && i < ydots)
    {
        putcolor(x, i, color) ;
    }
}

// Symmetry plot for Y Axis Symmetry
void symplot2Y(int x, int y, int color)
{
    int i;
    putcolor(x, y, color) ;
    i = xxstop-(x-xxstart);
    if (i < xdots)
    {
        putcolor(i, y, color) ;
    }
}

// Symmetry plot for Origin Symmetry
void symplot2J(int x, int y, int color)
{
    int i, j;
    putcolor(x, y, color) ;
    i = yystop-(y-yystart);
    if (i > g_i_y_stop && i < ydots
            && (j = xxstop-(x-xxstart)) < xdots)
    {
        putcolor(j, i, color) ;
    }
}

// Symmetry plot for Both Axis Symmetry
void symplot4(int x, int y, int color)
{
    int i, j;
    j = xxstop-(x-xxstart);
    putcolor(x , y, color) ;
    if (j < xdots)
    {
        putcolor(j , y, color) ;
    }
    i = yystop-(y-yystart);
    if (i > g_i_y_stop && i < ydots)
    {
        putcolor(x , i, color) ;
        if (j < xdots)
        {
            putcolor(j , i, color) ;
        }
    }
}

// Symmetry plot for X Axis Symmetry - Striped Newtbasin version
void symplot2basin(int x, int y, int color)
{
    int i, stripe;
    putcolor(x, y, color) ;
    if (g_basin == 2 && color > 8)
    {
        stripe = 8;
    }
    else
    {
        stripe = 0;
    }
    i = yystop-(y-yystart);
    if (i > g_i_y_stop && i < ydots)
    {
        color -= stripe;                    // reconstruct unstriped color
        color = (degree+1-color)%degree+1;  // symmetrical color
        color += stripe;                    // add stripe
        putcolor(x, i, color)  ;
    }
}

// Symmetry plot for Both Axis Symmetry  - Newtbasin version
void symplot4basin(int x, int y, int color)
{
    int i, j, color1, stripe;
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
    if (color < degree/2+2)
    {
        color1 = degree/2+2 - color;
    }
    else
    {
        color1 = degree/2+degree+2 - color;
    }
    j = xxstop-(x-xxstart);
    putcolor(x, y, color+stripe) ;
    if (j < xdots)
    {
        putcolor(j, y, color1+stripe) ;
    }
    i = yystop-(y-yystart);
    if (i > g_i_y_stop && i < ydots)
    {
        putcolor(x, i, stripe + (degree+1 - color)%degree+1) ;
        if (j < xdots)
        {
            putcolor(j, i, stripe + (degree+1 - color1)%degree+1) ;
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
