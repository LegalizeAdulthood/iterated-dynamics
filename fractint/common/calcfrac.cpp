//
//	calcfrac.cpp contains the high level ("engine") code for calculating the
//	fractal images (well, SOMEBODY had to do it!).
//	Original author Tim Wegner, but just about ALL the authors have contributed
//	SOME code to this routine at one time or another, or contributed to one of
//	the many massive restructurings.
//	The following modules work very closely with CALCFRAC.C:
//	  fractals.cpp    the fractal-specific code for escape-time fractals.
//	  fracsubr.cpp    assorted subroutines belonging mainly to calcfrac.
//	  calcmand.asm    fast Mandelbrot/Julia integer implementation
//	Additional fractal-specific modules are also invoked from CALCFRAC:
//	  lorenz.cpp      engine level and fractal specific code for attractors.
//	  jb.cpp          julibrot logic
//	  parser.cpp      formula fractals
//	  and more
//
#include <algorithm>
#include <string>

#include <assert.h>
#include <string.h>
#include <limits.h>
#include <time.h>

#include "port.h"
#include "prototyp.h"
#include "fractype.h"

#include "calcfrac.h"
#include "calcmand.h"
#include "drivers.h"
#include "filesystem.h"
#include "fpu.h"

#include "BoundaryTrace.h"
#include "calcfrac.h"
#include "DiffusionScan.h"
#include "diskvid.h"
#include "EscapeTime.h"
#include "Externals.h"
#include "FiniteAttractor.h"
#include "fmath.h"
#include "Formula.h"
#include "fracsubr.h"
#include "fractalp.h"
#include "fractals.h"
#include "framain2.h"
#include "FrothyBasin.h"
#include "line3d.h"
#include "lorenz.h"
#include "MathUtil.h"
#include "miscfrac.h"
#include "miscres.h"
#include "mpmath.h"
#include "prompts2.h"
#include "realdos.h"
#include "resume.h"
#include "soi.h"
#include "SolidGuess.h"
#include "SoundState.h"
#include "Tesseral.h"
#include "WorkList.h"

enum ShowDotType
{
	SHOWDOT_SAVE	= 1,
	SHOWDOT_RESTORE = 2
};

enum DotDirection
{
	JUST_A_POINT	= 0,
	LOWER_RIGHT		= 1,
	UPPER_RIGHT		= 2,
	LOWER_LEFT		= 3,
	UPPER_LEFT		= 4
};

class OrbitScanner : public WorkListScanner
{
public:
	virtual ~OrbitScanner() { }

	virtual void Scan();
};

// variables exported from this file
int g_orbit_draw_mode = ORBITDRAW_RECTANGLE;
ComplexL g_initial_orbit_z_l = { 0, 0 };

int g_and_color;

// magnitude of current orbit z
long g_magnitude_l = 0;

long g_rq_limit_l = 0;
long g_rq_limit2_l = 0;
long g_close_enough_l = 0;
ComplexD g_initial_z = { 0, 0 };
ComplexD g_old_z = { 0, 0 };
ComplexD g_new_z = { 0, 0 };
ComplexD g_temp_z = { 0, 0 };
int g_color = 0;
long g_color_iter;
long g_old_color_iter;
long g_real_color_iter;
int g_row;
int g_col;
int g_invert;
double g_f_radius;
double g_f_x_center;
double g_f_y_center; // for inversion
void (*g_plot_color_put_color)(int, int, int) = putcolor_a;
void (*g_plot_color)(int, int, int) = putcolor_a;
double g_magnitude;
double g_rq_limit;
double g_rq_limit2;
bool g_no_magnitude_calculation = false;
bool g_use_old_periodicity = false;
bool g_old_demm_colors = false;
int (*g_calculate_type)() = 0;
int (*g_calculate_type_temp)();
bool g_quick_calculate = false;
double g_proximity = 0.01;
double g_close_enough;
unsigned long g_magnitude_limit;               // magnitude limit (CALCMAND)
// orbit variables
bool g_show_orbit;                     // flag to turn on and off
int g_orbit_index;                      // pointer into g_save_orbit array
int g_orbit_color = 15;                 // XOR color
int g_x_stop;
int g_y_stop;							// stop here
SymmetryType g_symmetry;
bool g_reset_periodicity; // nonzero if escape time pixel rtn to reset
int g_input_counter;
int g_max_input_counter;    // avoids checking keyboard too often
int g_current_row;
int g_current_col;
bool g_three_pass;
bool g_next_screen_flag; // for cellular next screen generation
int     g_num_attractors;                 // number of finite attractors
ComplexD  g_attractors[MAX_NUM_ATTRACTORS];       // finite attractor vals (f.p)
ComplexL g_attractors_l[MAX_NUM_ATTRACTORS];      // finite attractor vals (int)
int    g_attractor_period[MAX_NUM_ATTRACTORS];          // period of the finite attractor
int g_periodicity_check;
// next has a skip bit for each s_max_block unit;
//	1st pass sets bit  [1]... off only if g_block's contents guessed;
//	at end of 1st pass [0]... bits are set if any surrounding g_block not guessed;
//	bits are numbered [..][y/16 + 1][x + 1] & (1<<(y&15))
typedef int (*TPREFIX)[2][MAX_Y_BLOCK][MAX_X_BLOCK];
// size of next puts a limit of MAX_PIXELS pixels across on solid guessing logic
BYTE g_stack[4096];              // common temp, two put_line calls
// For periodicity testing, only in standard_fractal()
int g_next_saved_incr;
int g_first_saved_and;
long (*g_calculate_mandelbrot_asm_fp)();

int g_ix_start;
int g_iy_start;						// start here
int g_work_pass;
int g_work_sym;                   // for the sake of calculate_mandelbrot_l

// routines in this module
static int one_or_two_pass();
static int  standard_calculate(int passnum);
static int  potential(double, long);
static void decomposition();
static void set_symmetry(int symmetry, bool use_list);
static int  x_symmetry_split(int, int);
static int  y_symmetry_split(int, int);
static void put_truecolor_disk(int, int, int);
// added for testing automatic_log_map()
static long automatic_log_map();
static void plot_color_symmetry_pi(int x, int y, int color);
static void plot_color_symmetry_pi_origin(int x, int y, int color);
static void plot_color_symmetry_pi_xy_axis(int x, int y, int color);
static void plot_color_symmetry_y_axis(int x, int y, int color);
static void plot_color_symmetry_xy_axis(int x, int y, int color);
static void plot_color_symmetry_x_axis_basin(int x, int y, int color);
static void plot_color_symmetry_xy_axis_basin(int x, int y, int color);

static double s_dem_delta;
static double s_dem_width;     // distance estimator variables
static double s_dem_too_big;
static bool s_dem_mandelbrot;
static ComplexD s_saved_z;
static double s_rq_limit_save;
static int s_pi_in_pixels; // value of pi in pixels
static BYTE *s_save_dots = 0;
static BYTE *s_fill_buffer = 0;
static int s_save_dots_len;
static int s_show_dot_color;
static int s_show_dot_width = 0;

static double const DEM_BAILOUT = 535.5;  // (pb: not sure if this is special or arbitrary)

static OrbitScanner s_orbitScanner;


// FMODTEST routine.
// Makes the test condition for the COLORMODE_FLOAT_MODULUS coloring type
//	that of the current bailout method. 'or' and 'and'
//	methods are not used - in these cases a normal
//	modulus test is used
//
static double fmod_test()
{
	double result;
	switch (g_externs.BailOutTest())
	{
	case BAILOUT_MODULUS:
		result = (g_magnitude == 0.0 || !g_no_magnitude_calculation || g_integer_fractal) ?
			sqr(g_new_z.real()) + sqr(g_new_z.imag()) : g_magnitude;
		break;
	case BAILOUT_REAL:
		result = sqr(g_new_z.real());
		break;
	case BAILOUT_IMAGINARY:
		result = sqr(g_new_z.imag());
		break;
	case BAILOUT_OR:
		{
			double tmpx = sqr(g_new_z.real());
			double tmpy = sqr(g_new_z.imag());
			result = (tmpx > tmpy) ? tmpx : tmpy;
		}
		break;
	case BAILOUT_MANHATTAN:
		result = sqr(fabs(g_new_z.real()) + fabs(g_new_z.imag()));
		break;
	case BAILOUT_MANHATTAN_R:
		result = sqr(g_new_z.real() + g_new_z.imag());
		break;
	default:
		result = sqr(g_new_z.real()) + sqr(g_new_z.imag());
		break;
	}
	return result;
}

//
//	The sym_fill_line() routine was pulled out of the boundary tracing
//	code for re-use with g_show_dot. It's purpose is to fill a line with a
//	solid color. This assumes that BYTE *str is already filled
//	with the color. The routine does write the line using symmetry
//	in all cases, however the symmetry logic assumes that the line
//	is one color; it is not general enough to handle a row of
//	pixels of different colors.
//
void sym_fill_line(int row, int left, int right, BYTE const *str)
{
	put_line(row, left, right, str);

	// here's where all the symmetry goes
	int length = right-left + 1;
	if (g_plot_color == g_plot_color_put_color)
	{
		g_input_counter -= length >> 4; // seems like a reasonable value
	}
	else if (g_plot_color == plot_color_symmetry_x_axis) // X-axis symmetry
	{
		int i = g_WorkList.yy_stop() - (row - g_WorkList.yy_start());
		if (i > g_y_stop && i < g_y_dots)
		{
			put_line(i, left, right, str);
			g_input_counter -= length >> 3;
		}
	}
	else if (g_plot_color == plot_color_symmetry_y_axis) // Y-axis symmetry
	{
		put_line(row,
			g_WorkList.xx_stop() - (right - g_WorkList.xx_start()),
			g_WorkList.xx_stop() - (left - g_WorkList.xx_start()),
			str);
		g_input_counter -= length >> 3;
	}
	else if (g_plot_color == plot_color_symmetry_origin)  // Origin symmetry
	{
		int i = g_WorkList.yy_stop()-(row-g_WorkList.yy_start());
		int j = std::min(g_WorkList.xx_stop()-(right-g_WorkList.xx_start()), g_x_dots-1);
		int k = std::min(g_WorkList.xx_stop()-(left -g_WorkList.xx_start()), g_x_dots-1);
		if (i > g_y_stop && i < g_y_dots && j <= k)
		{
			put_line(i, j, k, str);
		}
		g_input_counter -= length >> 3;
	}
	else if (g_plot_color == plot_color_symmetry_xy_axis) // X-axis and Y-axis symmetry
	{
		int i = g_WorkList.yy_stop()-(row-g_WorkList.yy_start());
		int j = std::min(g_WorkList.xx_stop()-(right-g_WorkList.xx_start()), g_x_dots-1);
		int k = std::min(g_WorkList.xx_stop()-(left -g_WorkList.xx_start()), g_x_dots-1);
		if (i > g_y_stop && i < g_y_dots)
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
		g_input_counter -= length >> 2;
	}
	else    // cheap and easy way out
	{
		for (int i = left; i <= right; i++)  // DG
		{
			g_plot_color(i, row, str[i-left]);
		}
		g_input_counter -= length >> 1;
	}
}

//
// The sym_put_line() routine is the symmetry-aware version of put_line().
// It only works efficiently in the no symmetry or SYMMETRY_X_AXIS symmetry case,
// otherwise it just writes the pixels one-by-one.
//
static void sym_put_line(int row, int left, int right, BYTE const *str)
{
	put_line(row, left, right, str);
	int length = right-left + 1;
	if (g_plot_color == g_plot_color_put_color)
	{
		g_input_counter -= length >> 4; // seems like a reasonable value
	}
	else if (g_plot_color == plot_color_symmetry_x_axis) // X-axis symmetry
	{
		int i = g_WorkList.yy_stop()-(row-g_WorkList.yy_start());
		if (i > g_y_stop && i < g_y_dots)
		{
			put_line(i, left, right, str);
		}
		g_input_counter -= length >> 3;
	}
	else
	{
		for (int i = left; i <= right; i++)  // DG
		{
			g_plot_color(i, row, str[i-left]);
		}
		g_input_counter -= length >> 1;
	}
}

static void show_dot_save_restore_upper_left(int startx, int &stopx, int starty, int stopy, int action)
{
	{
		int ct = 0;
		for (int j = starty; j >= stopy; stopx--, j--)
		{
			if (action == SHOWDOT_SAVE)
			{
				get_line(j, startx, stopx, s_save_dots + ct);
				sym_fill_line(j, startx, stopx, s_fill_buffer);
			}
			else
			{
				sym_put_line(j, startx, stopx, s_save_dots + ct);
			}
			ct += stopx-startx + 1;
		}
	}
}

static void show_dot_save_restore_lower_left(int startx, int &stopx, int starty, int stopy, int action)
{
	{
		int ct = 0;
		for (int j = starty; j <= stopy; stopx--, j++)
		{
			if (action == SHOWDOT_SAVE)
			{
				get_line(j, startx, stopx, s_save_dots + ct);
				sym_fill_line(j, startx, stopx, s_fill_buffer);
			}
			else
			{
				sym_put_line(j, startx, stopx, s_save_dots + ct);
			}
			ct += stopx-startx + 1;
		}
	}
}

static void show_dot_save_restore_upper_right(int &startx, int stopx, int starty, int stopy, int action)
{
	{
		int ct = 0;
		for (int j = starty; j >= stopy; startx++, j--)
		{
			if (action == SHOWDOT_SAVE)
			{
				get_line(j, startx, stopx, s_save_dots + ct);
				sym_fill_line(j, startx, stopx, s_fill_buffer);
			}
			else
			{
				sym_put_line(j, startx, stopx, s_save_dots + ct);
			}
			ct += stopx-startx + 1;
		}
	}
}

static void show_dot_save_restore_lower_right(int &startx, int stopx, int starty, int stopy, int action)
{
	{
		int ct = 0;
		for (int j = starty; j <= stopy; startx++, j++)
		{
			if (action == SHOWDOT_SAVE)
			{
				get_line(j, startx, stopx, s_save_dots + ct);
				sym_fill_line(j, startx, stopx, s_fill_buffer);
			}
			else
			{
				sym_put_line(j, startx, stopx, s_save_dots + ct);
			}
			ct += stopx-startx + 1;
		}
	}
}

static void show_dot_save_restore(int startx, int stopx, int starty, int stopy, DotDirection direction, int action)
{
	if (direction != JUST_A_POINT)
	{
		if (s_save_dots == 0)
		{
			stop_message(STOPMSG_NORMAL, "s_save_dots 0");
			exit(0);
		}
		if (s_fill_buffer == 0)
		{
			stop_message(STOPMSG_NORMAL, "s_fill_buffer 0");
			exit(0);
		}
	}
	switch (direction)
	{
	case LOWER_RIGHT:
		show_dot_save_restore_lower_right(startx, stopx, starty, stopy, action);
		break;
	case UPPER_RIGHT:
		show_dot_save_restore_upper_right(startx, stopx, starty, stopy, action);
		break;
	case LOWER_LEFT:
		show_dot_save_restore_lower_left(startx, stopx, starty, stopy, action);
		break;
	case UPPER_LEFT:
		show_dot_save_restore_upper_left(startx, stopx, starty, stopy, action);
		break;
	}
	if (action == SHOWDOT_SAVE)
	{
		g_plot_color(g_col, g_row, s_show_dot_color);
	}
}

static int calculate_type_show_dot()
{
	int width = s_show_dot_width + 1;
	int startx = g_col;
	int stopx = g_col;
	int starty = g_row;
	int stopy = g_row;
	DotDirection direction = JUST_A_POINT;
	if (width > 0)
	{
		if (g_col + width <= g_x_stop && g_row + width <= g_y_stop)
		{
			// preferred g_show_dot shape
			direction = UPPER_LEFT;
			startx = g_col;
			stopx = g_col + width;
			starty = g_row + width;
			stopy = g_row + 1;
		}
		else if (g_col-width >= g_ix_start && g_row + width <= g_y_stop)
		{
			// second choice
			direction = UPPER_RIGHT;
			startx = g_col-width;
			stopx = g_col;
			starty = g_row + width;
			stopy = g_row + 1;
		}
		else if (g_col-width >= g_ix_start && g_row-width >= g_iy_start)
		{
			direction = LOWER_RIGHT;
			startx = g_col-width;
			stopx = g_col;
			starty = g_row-width;
			stopy = g_row-1;
		}
		else if (g_col + width <= g_x_stop && g_row-width >= g_iy_start)
		{
			direction = LOWER_LEFT;
			startx = g_col;
			stopx = g_col + width;
			starty = g_row-width;
			stopy = g_row-1;
		}
	}
	show_dot_save_restore(startx, stopx, starty, stopy, direction, SHOWDOT_SAVE);
	if (g_orbit_delay > 0)
	{
		sleep_ms(g_orbit_delay);
	}
	int out = g_calculate_type_temp();
	show_dot_save_restore(startx, stopx, starty, stopy, direction, SHOWDOT_RESTORE);
	return out;
}

void ForceOnePass()
{
	// Have to force passes = 1
	g_externs.SetUserStandardCalculationMode(CALCMODE_SINGLE_PASS);
	g_externs.SetStandardCalculationMode(CALCMODE_SINGLE_PASS);
}

// calculate_fractal - the top level routine for generating an image
int calculate_fractal()
{
	g_math_error_count = 0;
	g_num_attractors = 0;          // default to no known finite attractors
	g_display_3d = DISPLAY3D_NONE;
	g_externs.SetBasin(0);
	// added yet another level of indirection to g_plot_color_put_color!!! TW
	g_plot_color_put_color = putcolor_a;
	if (g_is_true_color && g_true_mode_iterates)
	{
		ForceOnePass();
	}
	if (g_true_color)
	{
		check_write_file(g_light_name, ".tga");
		if (start_disk_targa(g_light_name, 0, false) == 0)
		{
			ForceOnePass();
			g_plot_color_put_color = put_truecolor_disk;
		}
		else
		{
			g_true_color = false;
		}
	}
	if (!g_escape_time_state.m_use_grid)
	{
		if (g_externs.UserStandardCalculationMode() != CALCMODE_ORBITS)
		{
			ForceOnePass();
		}
	}

	g_formula_state.init_misc();  // set up some variables in parser.c
	reset_clock();

	// following delta values useful only for types with rotation disabled
	// currently used only by bifurcation
	if (g_integer_fractal)
	{
		g_distance_test = 0;
	}
	g_parameter.real(g_parameters[0]);
	g_parameter.imag(g_parameters[1]);
	g_parameter2.real(g_parameters[2]);
	g_parameter2.imag(g_parameters[3]);

	if (g_use_old_periodicity)
	{
		g_next_saved_incr = 1;
		g_first_saved_and = 1;
	}
	else
	{
		g_next_saved_incr = int(log10(double(g_max_iteration))); // works better than log()
		if (g_next_saved_incr < 4)
		{
			g_next_saved_incr = 4; // maintains image with low iterations
		}
		g_first_saved_and = long(g_next_saved_incr*2 + 1);
	}

	g_log_table = 0;
	g_max_log_table_size = g_max_iteration;
	g_log_calculation = false;

	// below, INT_MAX = 32767 only when an integer is two bytes.  Which is not true for Xfractint.
	// Since 32767 is what was meant, replaced the instances of INT_MAX with 32767.
	if (g_log_palette_mode
		&& (((g_max_iteration > 32767) && true) || g_log_dynamic_calculate == LOGDYNAMIC_DYNAMIC))
	{
		g_log_calculation = true; // calculate on the fly
		SetupLogTable();
	}
	else if (g_ranges_length && (g_max_iteration >= 32767))
	{
		g_max_log_table_size = 32766;
	}

	if ((g_log_palette_mode || g_ranges_length) && !g_log_calculation)
	{
		g_log_table = new BYTE[g_max_log_table_size + 1];

		if (g_log_table == 0)
		{
			if (g_ranges_length || g_log_dynamic_calculate == LOGDYNAMIC_TABLE)
			{
				stop_message(STOPMSG_NORMAL, "Insufficient memory for logmap/ranges with this maxiter");
			}
			else
			{
				stop_message(STOPMSG_NORMAL, "Insufficient memory for logTable, using on-the-fly routine");
				g_log_dynamic_calculate = LOGDYNAMIC_DYNAMIC;
				g_log_calculation = true; // calculate on the fly
				SetupLogTable();
			}
		}
		else if (g_ranges_length)  // Can't do ranges if g_max_log_table_size > 32767
		{
			int i = 0;
			int k = 0;
			int l = 0;
			g_log_palette_mode = LOGPALETTE_NONE; // ranges overrides logmap
			while (i < g_ranges_length)
			{
				int m = 0;
				int flip = 0;
				int altern = 32767;
				int numval = g_ranges[i++];
				if (numval < 0)
				{
					altern = g_ranges[i++];    // sub-range iterations
					numval = g_ranges[i++];
				}
				if ((numval > int(g_max_log_table_size)) || (i >= g_ranges_length))
				{
					numval = int(g_max_log_table_size);
				}
				while (l <= numval)
				{
					g_log_table[l++] = BYTE(k + flip);
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
	g_magnitude_limit = 4L << g_bit_shift;                 // CALCMAND magnitude limit

	g_externs.SetAtanColors(g_colors);

	// orbit stuff
	g_show_orbit = g_start_show_orbit;
	g_orbit_index = 0;
	g_orbit_color = 15;

	if (g_inversion[0] != 0.0)
	{
		g_f_radius = g_inversion[0];
		g_f_x_center = g_inversion[1];
		g_f_y_center = g_inversion[2];

		if (g_inversion[0] == AUTO_INVERT)  // auto calc radius 1/6 screen
		{
			g_inversion[0] = std::min(fabs(g_escape_time_state.m_grid_fp.width()),
								fabs(g_escape_time_state.m_grid_fp.height()))/6.0;
			fix_inversion(&g_inversion[0]);
			g_f_radius = g_inversion[0];
		}

		if (g_invert < 2 || g_inversion[1] == AUTO_INVERT)  // xcenter not already set
		{
			g_inversion[1] = g_escape_time_state.m_grid_fp.x_center();
			fix_inversion(&g_inversion[1]);
			g_f_x_center = g_inversion[1];
			if (fabs(g_f_x_center) < fabs(g_escape_time_state.m_grid_fp.width())/100)
			{
				g_f_x_center = 0.0;
				g_inversion[1] = 0.0;
			}
		}

		if (g_invert < 3 || g_inversion[2] == AUTO_INVERT)  // ycenter not already set
		{
			g_inversion[2] = g_escape_time_state.m_grid_fp.y_center();
			fix_inversion(&g_inversion[2]);
			g_f_y_center = g_inversion[2];
			if (fabs(g_f_y_center) < fabs(g_escape_time_state.m_grid_fp.height())/100)
			{
				g_f_y_center = 0.0;
				g_inversion[2] = 0.0;
			}
		}

		g_invert = 3; // so values will not be changed if we come back
	}

	g_close_enough = g_delta_min_fp*pow(2.0, -double(abs(g_periodicity_check)));
	s_rq_limit_save = g_rq_limit;
	g_rq_limit2 = sqrt(g_rq_limit);
	if (g_integer_fractal)          // for integer routines (lambda)
	{
		g_parameter_l = ComplexDoubleToFudge(g_parameter); // Lambda
		g_parameter2_l = ComplexDoubleToFudge(g_parameter2); // Lambda2
		g_rq_limit_l = DoubleToFudge(g_rq_limit);      // stop if magnitude exceeds this
		if (g_rq_limit_l <= 0)
		{
			g_rq_limit_l = 0x7fffffffL; // klooge for integer math
		}
		g_rq_limit2_l = DoubleToFudge(g_rq_limit2);    // stop if magnitude exceeds this
		g_close_enough_l = DoubleToFudge(g_close_enough); // "close enough" value
		g_initial_orbit_z_l = ComplexDoubleToFudge(g_initial_orbit_z);
	}
	g_resuming = (g_externs.CalculationStatus() == CALCSTAT_RESUMABLE);
	if (!g_resuming) // free resume_info memory if any is hanging around
	{
		end_resume();
		if (g_resave_mode)
		{
			update_save_name(g_save_name); // do the pending increment
			g_resave_mode = RESAVE_NO;
			g_started_resaves = false;
		}
		g_calculation_time = 0;
	}

	if (g_current_fractal_specific->calculate_type != standard_fractal
		&& g_current_fractal_specific->calculate_type != calculate_mandelbrot_l
		&& g_current_fractal_specific->calculate_type != calculate_mandelbrot_fp
		&& g_current_fractal_specific->calculate_type != lyapunov
		&& g_current_fractal_specific->calculate_type != froth_calc)
	{
		// per_image can override calculate_type & symmetry
		g_calculate_type = g_current_fractal_specific->calculate_type;
		g_symmetry = g_current_fractal_specific->symmetry;
		// defaults when setsymmetry not called or does nothing
		g_plot_color = g_plot_color_put_color;
		g_iy_start = 0;
		g_ix_start = 0;
		g_WorkList.set_yy_start(0);
		g_WorkList.set_xx_start(0);
		g_WorkList.set_yy_begin(0);
		g_WorkList.set_xx_begin(0);
		g_y_stop = g_y_dots - 1;
		g_WorkList.set_yy_stop(g_y_dots - 1);
		g_x_stop = g_x_dots - 1;
		g_WorkList.set_xx_stop(g_x_dots - 1);
		g_externs.SetCalculationStatus(CALCSTAT_IN_PROGRESS);
		// only standard escape time engine supports distest
		g_distance_test = 0;
		if (g_current_fractal_specific->per_image())
		{
			// not a stand-alone
			// next two lines in case periodicity changed
			g_close_enough = g_delta_min_fp*pow(2.0, -double(abs(g_periodicity_check)));
			g_close_enough_l = DoubleToFudge(g_close_enough); // "close enough" value
			set_symmetry(g_symmetry, false);
			timer_engine(g_calculate_type); // non-standard fractal engine
		}
		if (check_key())
		{
			if (g_externs.CalculationStatus() == CALCSTAT_IN_PROGRESS) // calctype didn't set this itself,
			{
				g_externs.SetCalculationStatus(CALCSTAT_NON_RESUMABLE);   // so mark it interrupted, non-resumable
			}
		}
		else
		{
			g_externs.SetCalculationStatus(CALCSTAT_COMPLETED); // no key, so assume it completed
		}
	}
	else // standard escape-time engine
	{
		// convoluted solid guessing plus two-pass ('g' + '2') hybrid
		if (g_externs.StandardCalculationMode() == CALCMODE_TRIPLE_PASS)
		{
			CalculationMode oldcalcmode = g_externs.StandardCalculationMode();
			if (!g_resuming || g_three_pass)
			{
				g_externs.SetStandardCalculationMode(CALCMODE_SOLID_GUESS);
				g_three_pass = true;
				timer_engine((int (*)()) perform_work_list);
				if (g_externs.CalculationStatus() == CALCSTAT_COMPLETED)
				{
					// '2' is silly after 'g' for low rez
					g_externs.SetStandardCalculationMode((g_x_dots >= 640) ? CALCMODE_DUAL_PASS : CALCMODE_SINGLE_PASS);
					timer_engine((int (*)()) perform_work_list);
					g_three_pass = false;
				}
			}
			else // resuming '2' pass
			{
				g_externs.SetStandardCalculationMode((g_x_dots >= 640) ? CALCMODE_DUAL_PASS : CALCMODE_SINGLE_PASS);
				timer_engine((int (*)()) perform_work_list);
			}
			g_externs.SetStandardCalculationMode(oldcalcmode);
		}
		else // main case, much nicer!
		{
			g_three_pass = false;
			timer_engine((int (*)()) perform_work_list);
		}
	}
	g_calculation_time += g_timer_interval;

	if (!g_log_calculation)
	{
		delete[] g_log_table;
		g_log_table = 0;
	}
	g_formula_state.free_work_area();

	g_sound_state.close();

	if (g_true_color)
	{
		disk_end();
	}
	return (g_externs.CalculationStatus() == CALCSTAT_COMPLETED) ? 0 : -1;
}

// general escape-time engine routines

static int draw_rectangle_orbits()
{
	// draw a rectangle
	g_row = g_WorkList.yy_begin();
	g_col = g_WorkList.xx_begin();

	while (g_row <= g_y_stop)
	{
		g_current_row = g_row;
		while (g_col <= g_x_stop)
		{
			if (plotorbits2dfloat() == -1)
			{
				g_WorkList.add(g_WorkList.xx_start(), g_WorkList.xx_stop(), g_col,
					g_WorkList.yy_start(), g_WorkList.yy_stop(), g_row,
					0, g_work_sym);
				return -1; // interrupted
			}
			++g_col;
		}
		g_col = g_ix_start;
		++g_row;
	}

	return 0;
}

static int draw_line_orbits()
{
	int dX;
	int dY;                     // vector components
	int final,                      // final row or column number
		G,                  // used to test for new row or column
		inc1,           // G increment when row or column doesn't change
		inc2;               // G increment when row or column changes

	dX = g_x_stop - g_ix_start;                   // find vector components
	dY = g_y_stop - g_iy_start;
	bool positive_slope = (dX > 0);                   // is slope positive?
	if (dY < 0)
	{
		positive_slope = !positive_slope;
	}
	if (abs(dX) > abs(dY))                // shallow line case
	{
		if (dX > 0)         // determine start point and last column
		{
			g_col = g_WorkList.xx_begin();
			g_row = g_WorkList.yy_begin();
			final = g_x_stop;
		}
		else
		{
			g_col = g_x_stop;
			g_row = g_y_stop;
			final = g_WorkList.xx_begin();
		}
		inc1 = 2*abs(dY);            // determine increments and initial G
		G = inc1 - abs(dX);
		inc2 = 2*(abs(dY) - abs(dX));
		if (positive_slope)
		{
			while (g_col <= final)    // step through columns checking for new row
			{
				if (plotorbits2dfloat() == -1)
				{
					g_WorkList.add(g_WorkList.xx_start(), g_WorkList.xx_stop(), g_col,
						g_WorkList.yy_start(), g_WorkList.yy_stop(), g_row,
						0, g_work_sym);
					return -1; // interrupted
				}
				g_col++;
				if (G >= 0)             // it's time to change rows
				{
					g_row++;      // positive slope so increment through the rows
					G += inc2;
				}
				else                        // stay at the same row
				{
					G += inc1;
				}
			}
		}
		else
		{
			while (g_col <= final)    // step through columns checking for new row
			{
				if (plotorbits2dfloat() == -1)
				{
					g_WorkList.add(g_WorkList.xx_start(), g_WorkList.xx_stop(), g_col,
						g_WorkList.yy_start(), g_WorkList.yy_stop(), g_row,
						0, g_work_sym);
					return -1; // interrupted
				}
				g_col++;
				if (G > 0)              // it's time to change rows
				{
					g_row--;      // negative slope so decrement through the rows
					G += inc2;
				}
				else                        // stay at the same row
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
			g_col = g_WorkList.xx_begin();
			g_row = g_WorkList.yy_begin();
			final = g_y_stop;
		}
		else
		{
			g_col = g_x_stop;
			g_row = g_y_stop;
			final = g_WorkList.yy_begin();
		}
		inc1 = 2*abs(dX);            // determine increments and initial G
		G = inc1 - abs(dY);
		inc2 = 2*(abs(dX) - abs(dY));
		if (positive_slope)
		{
			while (g_row <= final)    // step through rows checking for new column
			{
				if (plotorbits2dfloat() == -1)
				{
					g_WorkList.add(g_WorkList.xx_start(), g_WorkList.xx_stop(), g_col,
						g_WorkList.yy_start(), g_WorkList.yy_stop(), g_row,
						0, g_work_sym);
					return -1; // interrupted
				}
				g_row++;
				if (G >= 0)                 // it's time to change columns
				{
					g_col++;  // positive slope so increment through the columns
					G += inc2;
				}
				else                    // stay at the same column
				{
					G += inc1;
				}
			}
		}
		else
		{
			while (g_row <= final)    // step through rows checking for new column
			{
				if (plotorbits2dfloat() == -1)
				{
					g_WorkList.add(g_WorkList.xx_start(), g_WorkList.xx_stop(), g_col,
						g_WorkList.yy_start(), g_WorkList.yy_stop(), g_row,
						0, g_work_sym);
					return -1; // interrupted
				}
				g_row++;
				if (G > 0)                  // it's time to change columns
				{
					g_col--;  // negative slope so decrement through the columns
					G += inc2;
				}
				else                    // stay at the same column
				{
					G += inc1;
				}
			}
		}
	}
	return 0;
}

// TODO: this code does not yet work???
static int draw_function_orbits()
{
	double Xctr;
	double Yctr;
	LDBL Magnification; // LDBL not really needed here, but used to match function parameters
	double Xmagfactor;
	double Rotation;
	double Skew;
	int angle;
	double theta;
	double xfactor = g_x_dots/2.0;
	double yfactor = g_y_dots/2.0;

	angle = g_WorkList.xx_begin();  // save angle in x parameter

	convert_center_mag(&Xctr, &Yctr, &Magnification, &Xmagfactor, &Rotation, &Skew);
	if (Rotation <= 0)
	{
		Rotation += 360;
	}

	while (angle < Rotation)
	{
		theta = MathUtil::DegreesToRadians(angle);
		g_col = int(xfactor + (Xctr + Xmagfactor*cos(theta)));
		g_row = int(yfactor + (Yctr + Xmagfactor*sin(theta)));
		if (plotorbits2dfloat() == -1)
		{
			g_WorkList.add(angle, 0, 0,
				0, 0, 0,
				0, g_work_sym);
			return -1; // interrupted
		}
		angle++;
	}
	return 0;
}

void OrbitScanner::Scan()
{
	g_externs.SetTabStatus(TAB_STATUS_ORBITS); // for <tab> screen
	g_externs.SetTotalPasses(1);

	if (plot_orbits_2d_setup() == -1)
	{
		g_externs.SetStandardCalculationMode(CALCMODE_SOLID_GUESS);
	}

	switch (g_orbit_draw_mode)
	{
	case ORBITDRAW_RECTANGLE:	draw_rectangle_orbits();	break;
	case ORBITDRAW_LINE:		draw_line_orbits();			break;
	case ORBITDRAW_FUNCTION:	draw_function_orbits();		break;

	default:
		assert(!"bad orbit draw mode");
	}
}

class MultiPassScanner : public WorkListScanner
{
public:
	virtual ~MultiPassScanner() { }

	virtual void Scan();
};

static MultiPassScanner s_multiPassScanner;

void MultiPassScanner::Scan()
{
	int i;

	g_externs.SetTotalPasses(1);
	if (g_externs.StandardCalculationMode() == CALCMODE_DUAL_PASS)
	{
		g_externs.SetTotalPasses(2);

		if (g_work_pass == 0) // do 1st pass of two
		{
			if (standard_calculate(1) == -1)
			{
				g_WorkList.add(g_WorkList.xx_start(), g_WorkList.xx_stop(), g_col,
					g_WorkList.yy_start(), g_WorkList.yy_stop(), g_row,
					0, g_work_sym);
				return;
			}
			if (g_WorkList.num_items() > 0) // g_work_list not empty, defer 2nd pass
			{
				g_WorkList.add(g_WorkList.xx_start(), g_WorkList.xx_stop(), g_WorkList.xx_start(),
					g_WorkList.yy_start(), g_WorkList.yy_stop(), g_WorkList.yy_start(),
					1, g_work_sym);
				return;
			}
			g_work_pass = 1;
			g_WorkList.set_xx_begin(g_WorkList.xx_start());
			g_WorkList.set_yy_begin(g_WorkList.yy_start());
		}
	}

	// second or only pass
	if (standard_calculate(2) == -1)
	{
		i = g_WorkList.yy_stop();
		if (g_y_stop != g_WorkList.yy_stop()) // must be due to symmetry
		{
			i -= g_row - g_iy_start;
		}
		g_WorkList.add(g_WorkList.xx_start(), g_WorkList.xx_stop(), g_col,
			g_row, i, g_row,
			g_work_pass, g_work_sym);
	}
}

static int standard_calculate(int passnum)
{
	g_externs.SetTabStatus(TAB_STATUS_12PASS);
	g_externs.SetCurrentPass(passnum);
	g_row = g_WorkList.yy_begin();
	g_col = g_WorkList.xx_begin();

	while (g_row <= g_y_stop)
	{
		g_current_row = g_row;
		g_reset_periodicity = true;
		while (g_col <= g_x_stop)
		{
			// on 2nd pass of two, skip even pts
			if (g_quick_calculate && !g_resuming)
			{
				g_color = get_color(g_col, g_row);
				if (g_color != g_externs.Inside())
				{
					++g_col;
					continue;
				}
			}
			if (passnum == 1 || g_externs.StandardCalculationMode() == CALCMODE_SINGLE_PASS || (g_row&1) != 0 || (g_col&1) != 0)
			{
				if (g_calculate_type() == -1) // standard_fractal(), calculate_mandelbrot_l() or calculate_mandelbrot_fp()
				{
					return -1; // interrupted
				}
				g_resuming = false; // reset so g_quick_calculate works
				g_reset_periodicity = false;
				if (passnum == 1) // first pass, copy pixel and bump col
				{
					if ((g_row&1) == 0 && g_row < g_y_stop)
					{
						g_plot_color(g_col, g_row + 1, g_color);
						if ((g_col & 1) == 0 && g_col < g_x_stop)
						{
							g_plot_color(g_col + 1, g_row + 1, g_color);
						}
					}
					if ((g_col&1) == 0 && g_col < g_x_stop)
					{
						g_plot_color(++g_col, g_row, g_color);
					}
				}
			}
			++g_col;
		}
		g_col = g_ix_start;
		if (passnum == 1 && (g_row&1) == 0)
		{
			++g_row;
		}
		++g_row;
	}
	return 0;
}

static void SetBoundaryTraceDebugColor()
{
	if (g_debug_mode != DEBUGMODE_BNDTRACE_NONZERO)
	{
		if (g_color <= 0 && g_externs.StandardCalculationMode() == CALCMODE_BOUNDARY_TRACE)
		{
			g_color = 1;
		}
	}
}

int calculate_mandelbrot_l()              // fast per pixel 1/2/b/g, called with row & col set
{
	// setup values from array to avoid using es reg in calcmand.asm
	g_initial_x_l = g_externs.LxPixel();
	g_initial_y_l = g_externs.LyPixel();
	if (calculate_mandelbrot_asm() >= 0)
	{
		if ((g_log_table || g_log_calculation) // map color, but not if maxit & adjusted for inside, etc
			&& (g_real_color_iter < g_max_iteration
				|| (g_externs.Inside() < 0 && g_color_iter == g_max_iteration)))
		{
			g_color_iter = logtablecalc(g_color_iter);
		}
		g_color = abs(int(g_color_iter));
		if (g_color_iter >= g_colors)  // don't use color 0 unless from inside/outside
		{
			g_color = int(((g_color_iter - 1) % g_and_color) + 1);
		}
		SetBoundaryTraceDebugColor();
		g_plot_color(g_col, g_row, g_color);
	}
	else
	{
		g_color = int(g_color_iter);
	}
	return g_color;
}

// NOTE: Integer code is UNTESTED
bool detect_finite_attractor_l()
{
	bool attracted = false;

	ComplexL attractor_l;
	for (int i = 0; i < g_num_attractors; i++)
	{
		attractor_l.real(lsqr(g_new_z_l.real() - g_attractors_l[i].real()));
		if (attractor_l.real() < g_attractor_radius_l)
		{
			attractor_l.imag(lsqr(g_new_z_l.imag() - g_attractors_l[i].imag()));
			if (attractor_l.imag() < g_attractor_radius_l)
			{
				if ((attractor_l.real() + attractor_l.imag()) < g_attractor_radius_l)
				{
					attracted = true;
					if (g_finite_attractor == FINITE_ATTRACTOR_PHASE)
					{
						g_color_iter = (g_color_iter % g_attractor_period[i]) + 1;
					}
				}
			}
		}
	}

	return attracted;
}

bool detect_finite_attractor_fp()
{
	bool attracted = false;

	ComplexD attractor;
	for (int i = 0; i < g_num_attractors; i++)
	{
		attractor.x = g_new_z.real() - g_attractors[i].x;
		attractor.x = sqr(attractor.x);
		if (attractor.x < g_attractor_radius_fp)
		{
			attractor.y = g_new_z.imag() - g_attractors[i].y;
			attractor.y = sqr(attractor.y);
			if (attractor.y < g_attractor_radius_fp)
			{
				if ((attractor.x + attractor.y) < g_attractor_radius_fp)
				{
					attracted = true;
					if (g_finite_attractor == FINITE_ATTRACTOR_PHASE)
					{
						g_color_iter = (g_color_iter % g_attractor_period[i]) + 1;
					}
				}
			}
		}
	}

	return attracted;
}

bool detect_finite_attractor()
{
	if (g_num_attractors == 0)       // no finite attractor
	{
		return false;
	}

	return g_integer_fractal ? detect_finite_attractor_l() : detect_finite_attractor_fp();
}

// added by Wes Loewer - sort of a floating point version of calculate_mandelbrot_l()
// can also handle invert, any g_rq_limit, g_potential_flag, zmag, epsilon cross,
// and all the current outside options    -Wes Loewer 11/03/91
//
int calculate_mandelbrot_fp()
{
	if (g_invert)
	{
		invert_z(&g_initial_z);
	}
	else
	{
		g_initial_z = g_externs.DPixel();
	}
	if (g_calculate_mandelbrot_asm_fp() >= 0)
	{
		if (g_potential_flag)
		{
			g_color_iter = potential(g_magnitude, g_real_color_iter);
		}
		if ((g_log_table || g_log_calculation) // map color, but not if maxit & adjusted for inside, etc
				&& (g_real_color_iter < g_max_iteration || (g_externs.Inside() < 0 && g_color_iter == g_max_iteration)))
		{
			g_color_iter = logtablecalc(g_color_iter);
		}
		g_color = abs(int(g_color_iter));
		if (g_color_iter >= g_colors)  // don't use color 0 unless from inside/outside
		{
			g_color = int(((g_color_iter - 1) % g_and_color) + 1);
		}
		SetBoundaryTraceDebugColor();
		g_plot_color(g_col, g_row, g_color);
	}
	else
	{
		g_color = int(g_color_iter);
	}
	return g_color;
}

float const STARTRAILMAX = FLT_MAX;   // just a convenient large number
enum
{
	green = 2,
	yellow = 6
};

#if 0
#define MINSAVEDAND 3   // if not defined, old method used
#endif

enum EpsilonCrossHooperType
{
	HOOPER_NEGATIVE_X_AXIS = -2,
	HOOPER_NEGATIVE_Y_AXIS = -1,
	HOOPER_NONE = 0,
	HOOPER_POSITIVE_Y_AXIS = 1,
	HOOPER_POSITIVE_X_AXIS = 2
};

class ColorModeStarTrail
{
public:
	ColorModeStarTrail()
	{
	}
	~ColorModeStarTrail()
	{
	}

	void initialize();
	void update();
	void final();

private:
	double m_tangent_table[16];

	void clamp(double x)
	{
		if (x > STARTRAILMAX)
		{
			x = STARTRAILMAX;
		}
		else if (x < -STARTRAILMAX)
		{
			x = -STARTRAILMAX;
		}
	}
};

void ColorModeStarTrail::initialize()
{
	for (int i = 0; i < 16; i++)
	{
		m_tangent_table[i] = 0.0;
	}
	g_max_iteration = 16;
}

void ColorModeStarTrail::update()
{
	if (0 < g_color_iter && g_color_iter < 16)
	{
		if (g_integer_fractal)
		{
			g_new_z = ComplexFudgeToDouble(g_new_z_l);
		}

		clamp(g_new_z.x);
		clamp(g_new_z.y);
		g_temp_sqr.real(g_new_z.real()*g_new_z.real());
		g_temp_sqr.imag(g_new_z.imag()*g_new_z.imag());
		g_magnitude = g_temp_sqr.real() + g_temp_sqr.imag();
		g_old_z = g_new_z;
		{
			int tmpcolor = int(((g_color_iter - 1) % g_and_color) + 1);
			m_tangent_table[tmpcolor-1] = g_new_z.imag()/(g_new_z.real() + .000001);
		}
	}
}

void ColorModeStarTrail::final()
{
	g_color_iter = 0;
	for (int i = 1; i < 16; i++)
	{
		if (fabs(m_tangent_table[0] - m_tangent_table[i]) < .05)
		{
			g_color_iter = i;
			break;
		}
	}
}

class StandardFractal
{
public:
	StandardFractal()
	{
	}
	~StandardFractal()
	{
	}

	int Execute();

private:
	int check_if_interrupted();
	void set_final_color_and_plot();
	void inside_colormode_final();
	void adjust_color_log_map();
	void inside_colormode_z_magnitude_final();
	void inside_colormode_beauty_of_fractals_61_final();
	void inside_colormode_beauty_of_fractals_60_final();
	void inside_colormode_float_modulus_final();
	void inside_colormode_inverse_tangent_final();
	void inside_colormode_epsilon_cross_final();
	void inside_colormode_period_final();
	void inside_colormode_star_trail_final();
	void merge_escape_time_stripes();
	void compute_decomposition_and_biomorph();
	bool distance_get_color(double dist);
	double distance_compute();
	void outside_colormode_final();
	void eliminate_negative_colors_and_wrap_arounds();
	void outside_colormode_total_distance_final();
	void outside_colormode_float_modulus_final();
	void outside_colormode_inverse_tangent_final();
	void outside_colormode_sum_final();
	void outside_colormode_multiply_final();
	void outside_colormode_imaginary_final();
	void outside_colormode_real_final();
	void outside_colormode_set_new_z_update();
	bool distance_test_compute();
	void potential_compute();
	void potential_set_new_z();
	void outside_colormode_update();
	void outside_colormode_float_modulus_update();
	void outside_colormode_total_distance_update();
	void outside_colormode_set_new_z_final();
	bool inside_colormode_update();
	void colormode_beauty_of_fractals_update();
	void colormode_float_modulus_integer_update();
	bool colormode_epsilon_cross_update();
	void colormode_star_trail_update();
	void colormode_star_trail_clamp(double &x);
	void check_periodicity();
	void set_new_z_if_bigmath();
	void show_orbit();
	bool interrupted();
	void initialize();
	void outside_colormode_total_distance_initialize();
	void inside_colormode_beauty_of_fractals_initialize();
	void initialize_periodicity_cycle_check_size();
	void initialize_integer();
	void initialize_float();
	void initialize_periodicity();
	void inside_colormode_star_trail_initialize();

	double m_tangent_table[16];
	EpsilonCrossHooperType m_colormode_epsilon_cross_hooper;
	double m_colormode_modulus_value;
	double m_colormode_bof60_min_magnitude;
	long m_colormode_bof61_min_index;
	long m_colormode_period_cycle_length;
	long m_colormode_period_cycle_start_iteration;
	ComplexD m_dem_new_z;
	double m_colormode_total_distance;
	bool m_attracted;
	ComplexL m_saved_z_l;
	ComplexD m_distance_test_derivative;
	long m_dem_color;
	bool m_caught_a_cycle;
	long m_periodicity_cycle_check_size;
	ComplexD m_colormode_total_distance_last_z;
	int m_check_freq;
	long m_colormode_epsilon_cross_proximity_l;
	int m_periodicity_cycle_check_counter;
};

int standard_fractal()       // per pixel 1/2/b/g, called with row & col set
{
	return StandardFractal().Execute();
}

void StandardFractal::inside_colormode_star_trail_initialize()
{
	if (g_externs.Inside() == COLORMODE_STAR_TRAIL)
	{
		for (int i = 0; i < 16; i++)
		{
			m_tangent_table[i] = 0.0;
		}
		g_max_iteration = 16;
	}
}
void StandardFractal::initialize_periodicity()
{
	if (g_periodicity_check == 0 || g_externs.Inside() == COLORMODE_Z_MAGNITUDE || g_externs.Inside() == COLORMODE_STAR_TRAIL)
	{
		g_old_color_iter = 2147483647L;       // don't check periodicity at all
	}
	else if (g_externs.Inside() == COLORMODE_PERIOD)   // for display-periodicity
	{
		g_old_color_iter = (g_max_iteration/5)*4;       // don't check until nearly done
	}
	else if (g_reset_periodicity)
	{
		g_old_color_iter = 255;               // don't check periodicity 1st 250 iterations
	}
}
void StandardFractal::initialize_float()
{
	if (g_externs.UseInitialOrbitZ() == INITIALZ_ORBIT)
	{
		s_saved_z = g_initial_orbit_z;
	}
	else
	{
		s_saved_z.x = 0;
		s_saved_z.y = 0;
	}
	if (g_bf_math)
	{
		if (g_decimals > 200)
		{
			g_input_counter = -1;
		}
		if (g_bf_math == BIGNUM)
		{
			clear_bn(bnsaved.x);
			clear_bn(bnsaved.y);
		}
		else if (g_bf_math == BIGFLT)
		{
			clear_bf(bfsaved.x);
			clear_bf(bfsaved.y);
		}
	}
	g_initial_z.imag(g_externs.DyPixel());
	if (g_distance_test)
	{
		m_distance_test_derivative.x = 1;
		m_distance_test_derivative.y = 0;
		g_magnitude = 0;
	}
}
void StandardFractal::initialize_integer()
{
	if (g_externs.UseInitialOrbitZ() == INITIALZ_ORBIT)
	{
		m_saved_z_l = g_initial_orbit_z_l;
	}
	else
	{
		m_saved_z_l.x = 0;
		m_saved_z_l.y = 0;
	}
	g_initial_z_l.imag(g_externs.LyPixel());
}
void StandardFractal::initialize_periodicity_cycle_check_size()
{
	if (g_externs.Inside() == COLORMODE_PERIOD)
	{
		m_periodicity_cycle_check_size = 16;           // begin checking every 16th cycle
	}
	else
	{
		// Jonathan - don't understand such a low savedand -- how about this?
#ifdef MINSAVEDAND
		savedand = MINSAVEDAND;
#else
		m_periodicity_cycle_check_size = g_first_saved_and;                // begin checking every other cycle
#endif
	}
	m_periodicity_cycle_check_counter = 1;               // start checking the very first time
}
void StandardFractal::inside_colormode_beauty_of_fractals_initialize()
{
	if (inside_coloring_beauty_of_fractals())
	{
		g_magnitude = 0;
		g_magnitude_l = 0;
	}
}
void StandardFractal::outside_colormode_total_distance_initialize()
{
	if (g_externs.Outside() == COLORMODE_TOTAL_DISTANCE)
	{
		if (g_integer_fractal)
		{
			g_old_z = ComplexFudgeToDouble(g_old_z_l);
		}
		else if (g_bf_math == BIGNUM)
		{
			g_old_z = complex_bn_to_float(&g_old_z_bn);
		}
		else if (g_bf_math == BIGFLT)
		{
			g_old_z = complex_bf_to_float(&g_old_z_bf);
		}
		m_colormode_total_distance_last_z.x = g_old_z.real();
		m_colormode_total_distance_last_z.y = g_old_z.imag();
	}
}
void StandardFractal::initialize()
{
	inside_colormode_star_trail_initialize();
	initialize_periodicity();
	// Jonathan - how about this idea ? skips first saved value which never works
#ifdef MINSAVEDAND
	if (g_old_color_iter < MINSAVEDAND)
	{
		g_old_color_iter = MINSAVEDAND;
	}
#else
	if (g_old_color_iter < g_first_saved_and) // I like it!
	{
		g_old_color_iter = g_first_saved_and;
	}
#endif
	// really fractal specific, but we'll leave it here
	m_dem_color = -1;
	if (!g_integer_fractal)
	{
		initialize_float();
	}
	else
	{
		initialize_integer();
	}
	g_orbit_index = 0;
	g_color_iter = 0;
	if (fractal_type_julia(g_fractal_type))
	{
		g_color_iter = -1;
	}
	m_caught_a_cycle = false;
	initialize_periodicity_cycle_check_size();
	inside_colormode_beauty_of_fractals_initialize();
	g_overflow = false;                // reset integer math overflow flag

	g_current_fractal_specific->per_pixel(); // initialize the calculations

	m_attracted = false;
	outside_colormode_total_distance_initialize();
	m_check_freq = (((g_sound_state.flags() & SOUNDFLAG_ORBITMASK) > SOUNDFLAG_X || g_show_dot >= 0)
			&& g_orbit_delay > 0) ? 16 : 2048;

	if (g_show_orbit)
	{
		g_sound_state.write_time();
	}
	m_colormode_modulus_value = 0.0;
	m_colormode_bof60_min_magnitude = 100000.0;		// orbit value closest to origin
	m_colormode_bof61_min_index = 0;					// iteration of min_orbit
	m_colormode_period_cycle_length = -1;
	m_colormode_period_cycle_start_iteration = 0;
	m_colormode_total_distance = 0.0;
	m_colormode_epsilon_cross_hooper = HOOPER_NONE;
	m_colormode_epsilon_cross_proximity_l = DoubleToFudge(g_proximity);
}
bool StandardFractal::interrupted()
{
	return (g_color_iter % m_check_freq == 0) && check_key();
}
void StandardFractal::show_orbit()
{
	if (!g_integer_fractal)
	{
		if (g_bf_math == BIGNUM)
		{
			g_new_z = complex_bn_to_float(&g_new_z_bn);
		}
		else if (g_bf_math == BIGFLT)
		{
			g_new_z = complex_bf_to_float(&g_new_z_bf);
		}
		plot_orbit(g_new_z.real(), g_new_z.imag(), -1);
	}
	else
	{
		plot_orbit_i(g_new_z_l.real(), g_new_z_l.imag(), -1);
	}
}
void StandardFractal::set_new_z_if_bigmath()
{
	if (g_bf_math == BIGNUM)
	{
		g_new_z = complex_bn_to_float(&g_new_z_bn);
	}
	else if (g_bf_math == BIGFLT)
	{
		g_new_z = complex_bf_to_float(&g_new_z_bf);
	}
}
void StandardFractal::check_periodicity()
{
	if (g_color_iter > g_old_color_iter) // check periodicity
	{
		if ((g_color_iter & m_periodicity_cycle_check_size) == 0)            // time to save a new value
		{
			m_colormode_period_cycle_start_iteration = g_color_iter;
			if (g_integer_fractal)
			{
				m_saved_z_l = g_new_z_l; // integer fractals
			}
			else if (g_bf_math == BIGNUM)
			{
				copy_bn(bnsaved.x, g_new_z_bn.x);
				copy_bn(bnsaved.y, g_new_z_bn.y);
			}
			else if (g_bf_math == BIGFLT)
			{
				copy_bf(bfsaved.x, g_new_z_bf.x);
				copy_bf(bfsaved.y, g_new_z_bf.y);
			}
			else
			{
				s_saved_z = g_new_z;  // floating pt fractals
			}
			if (--m_periodicity_cycle_check_counter == 0)    // time to lengthen the periodicity?
			{
				m_periodicity_cycle_check_size = (m_periodicity_cycle_check_size << 1) + 1;       // longer periodicity
				m_periodicity_cycle_check_counter = g_next_saved_incr; // restart counter
			}
		}
		else                // check against an old save
		{
			if (g_integer_fractal)     // floating-pt periodicity chk
			{
				if (labs(m_saved_z_l.x - g_new_z_l.real()) < g_close_enough_l)
				{
					if (labs(m_saved_z_l.y - g_new_z_l.imag()) < g_close_enough_l)
					{
						m_caught_a_cycle = true;
					}
				}
			}
			else if (g_bf_math == BIGNUM)
			{
				if (cmp_bn(abs_a_bn(sub_bn(bntmp, bnsaved.x, g_new_z_bn.x)), bnclosenuff) < 0)
				{
					if (cmp_bn(abs_a_bn(sub_bn(bntmp, bnsaved.y, g_new_z_bn.y)), bnclosenuff) < 0)
					{
						m_caught_a_cycle = true;
					}
				}
			}
			else if (g_bf_math == BIGFLT)
			{
				if (cmp_bf(abs_a_bf(sub_bf(bftmp, bfsaved.x, g_new_z_bf.x)), bfclosenuff) < 0)
				{
					if (cmp_bf(abs_a_bf(sub_bf(bftmp, bfsaved.y, g_new_z_bf.y)), bfclosenuff) < 0)
					{
						m_caught_a_cycle = true;
					}
				}
			}
			else
			{
				if (fabs(s_saved_z.x - g_new_z.real()) < g_close_enough)
				{
					if (fabs(s_saved_z.y - g_new_z.imag()) < g_close_enough)
					{
						m_caught_a_cycle = true;
					}
				}
			}
			if (m_caught_a_cycle)
			{
				m_colormode_period_cycle_length = g_color_iter - m_colormode_period_cycle_start_iteration;
				g_color_iter = g_max_iteration - 1;
			}
		}
	}
}
void StandardFractal::colormode_star_trail_clamp(double &x)
{
	if (x > STARTRAILMAX)
	{
		x = STARTRAILMAX;
	}
	else if (x < -STARTRAILMAX)
	{
		x = -STARTRAILMAX;
	}
}
void StandardFractal::colormode_star_trail_update()
{
	if (0 < g_color_iter && g_color_iter < 16)
	{
		if (g_integer_fractal)
		{
			g_new_z = ComplexFudgeToDouble(g_new_z_l);
		}

		colormode_star_trail_clamp(g_new_z.x);
		colormode_star_trail_clamp(g_new_z.y);
		g_temp_sqr.real(g_new_z.real()*g_new_z.real());
		g_temp_sqr.imag(g_new_z.imag()*g_new_z.imag());
		g_magnitude = g_temp_sqr.real() + g_temp_sqr.imag();
		g_old_z = g_new_z;
		{
			int tmpcolor = int(((g_color_iter - 1) % g_and_color) + 1);
			m_tangent_table[tmpcolor-1] = g_new_z.imag()/(g_new_z.real() + .000001);
		}
	}
}
bool StandardFractal::colormode_epsilon_cross_update()
{
	bool go_plot_inside;
	go_plot_inside = false;
	m_colormode_epsilon_cross_hooper = HOOPER_NONE;
	if (g_integer_fractal)
	{
		if (labs(g_new_z_l.real()) < labs(m_colormode_epsilon_cross_proximity_l))
		{
			// close to y axis
			m_colormode_epsilon_cross_hooper = (m_colormode_epsilon_cross_proximity_l > 0) ?
				HOOPER_POSITIVE_Y_AXIS : HOOPER_NEGATIVE_Y_AXIS;
			go_plot_inside = true;
		}
		else if (labs(g_new_z_l.imag()) < labs(m_colormode_epsilon_cross_proximity_l))
		{
			// close to x axis
			m_colormode_epsilon_cross_hooper = (m_colormode_epsilon_cross_proximity_l > 0) ?
				HOOPER_POSITIVE_X_AXIS : HOOPER_NEGATIVE_X_AXIS;
			go_plot_inside = true;
		}
	}
	else
	{
		if (fabs(g_new_z.real()) < fabs(g_proximity))
		{
			// close to y axis
			m_colormode_epsilon_cross_hooper = (g_proximity > 0) ?
				HOOPER_POSITIVE_Y_AXIS : HOOPER_NEGATIVE_Y_AXIS;
			go_plot_inside = true;
		}
		else if (fabs(g_new_z.imag()) < fabs(g_proximity))
		{
			// close to x axis
			m_colormode_epsilon_cross_hooper = (g_proximity > 0) ?
				HOOPER_POSITIVE_X_AXIS : HOOPER_NEGATIVE_X_AXIS;
			go_plot_inside = true;
		}
	}
	return go_plot_inside;
}
void StandardFractal::colormode_float_modulus_integer_update()
{
	if (g_integer_fractal)
	{
		g_new_z = ComplexFudgeToDouble(g_new_z_l);
	}
	double mag = fmod_test();
	if (mag < g_proximity)
	{
		m_colormode_modulus_value = mag;
	}
}
void StandardFractal::colormode_beauty_of_fractals_update()
{
	if (g_integer_fractal)
	{
		if (g_magnitude_l == 0 || !g_no_magnitude_calculation)
		{
			g_magnitude_l = lsqr(g_new_z_l.real()) + lsqr(g_new_z_l.imag());
		}
		g_magnitude = FudgeToDouble(g_magnitude_l);
	}
	else if (g_magnitude == 0.0 || !g_no_magnitude_calculation)
	{
		g_magnitude = sqr(g_new_z.real()) + sqr(g_new_z.imag());
	}
	if (g_magnitude < m_colormode_bof60_min_magnitude)
	{
		m_colormode_bof60_min_magnitude = g_magnitude;
		m_colormode_bof61_min_index = g_color_iter + 1;
	}
}
bool StandardFractal::inside_colormode_update()
{
	bool go_plot_inside = false;
	if (g_externs.Inside() < COLORMODE_ITERATION)
	{
		set_new_z_if_bigmath();
		if (g_externs.Inside() == COLORMODE_STAR_TRAIL)
		{
			colormode_star_trail_update();
		}
		else if (g_externs.Inside() == COLORMODE_EPSILON_CROSS)
		{
			go_plot_inside = colormode_epsilon_cross_update();
		}
		else if (g_externs.Inside() == COLORMODE_FLOAT_MODULUS_INTEGER)
		{
			colormode_float_modulus_integer_update();
		}
		else if (inside_coloring_beauty_of_fractals())
		{
			colormode_beauty_of_fractals_update();
		}
	}
	return go_plot_inside;
}
void StandardFractal::outside_colormode_set_new_z_update()
{
	if (g_integer_fractal)
	{
		g_new_z = ComplexFudgeToDouble(g_new_z_l);
	}
	else if (g_bf_math == BIGNUM)
	{
		g_new_z = complex_bn_to_float(&g_new_z_bn);
	}
	else if (g_bf_math == BIGFLT)
	{
		g_new_z = complex_bf_to_float(&g_new_z_bf);
	}
}
void StandardFractal::outside_colormode_total_distance_update()
{
	m_colormode_total_distance += sqrt(sqr(m_colormode_total_distance_last_z.x-g_new_z.real()) + sqr(m_colormode_total_distance_last_z.y-g_new_z.imag()));
	m_colormode_total_distance_last_z.x = g_new_z.real();
	m_colormode_total_distance_last_z.y = g_new_z.imag();
}
void StandardFractal::outside_colormode_float_modulus_update()
{
	double mag = fmod_test();
	if (mag < g_proximity)
	{
		m_colormode_modulus_value = mag;
	}
}
void StandardFractal::outside_colormode_update()
{
	if (g_externs.Outside() == COLORMODE_TOTAL_DISTANCE || g_externs.Outside() == COLORMODE_FLOAT_MODULUS)
	{
		outside_colormode_set_new_z_update();
		if (g_externs.Outside() == COLORMODE_TOTAL_DISTANCE)
		{
			outside_colormode_total_distance_update();
		}
		else if (g_externs.Outside() == COLORMODE_FLOAT_MODULUS)
		{
			outside_colormode_float_modulus_update();
		}
	}
}
void StandardFractal::potential_set_new_z()
{
	if (g_integer_fractal)       // adjust integer fractals
	{
		g_new_z = ComplexFudgeToDouble(g_new_z_l);
	}
	else if (g_bf_math == BIGNUM)
	{
		g_new_z.real(double(bntofloat(g_new_z_bn.x)));
		g_new_z.imag(double(bntofloat(g_new_z_bn.y)));
	}
	else if (g_bf_math == BIGFLT)
	{
		g_new_z.real(double(bftofloat(g_new_z_bf.x)));
		g_new_z.imag(double(bftofloat(g_new_z_bf.y)));
	}
}
void StandardFractal::potential_compute()
{
	potential_set_new_z();
	g_magnitude = sqr(g_new_z.real()) + sqr(g_new_z.imag());
	g_color_iter = potential(g_magnitude, g_color_iter);
	if (g_log_table || g_log_calculation)
	{
		g_color_iter = logtablecalc(g_color_iter);
	}
}
bool StandardFractal::distance_test_compute()
{
	// Distance estimator for points near Mandelbrot set
	// Original code by Phil Wilson, hacked around by PB
	// Algorithms from Peitgen & Saupe, Science of Fractal Images, p.198
	double ftemp = (s_dem_mandelbrot ? 1 : 0)
		+ 2*(g_old_z.real()*m_distance_test_derivative.x - g_old_z.imag()*m_distance_test_derivative.y);
	m_distance_test_derivative.y = 2*(g_old_z.imag()*m_distance_test_derivative.x + g_old_z.real()*m_distance_test_derivative.y);
	m_distance_test_derivative.x = ftemp;
	if (std::max(fabs(m_distance_test_derivative.x), fabs(m_distance_test_derivative.y)) > s_dem_too_big)
	{
		return true;
	}
	// if above exit taken, the later test vs s_dem_delta will place this
	// point on the boundary, because mag(g_old_z) < bailout just now
	if (g_current_fractal_specific->orbitcalc() || g_overflow)
	{
		return true;
	}
	g_old_z = g_new_z;
	return false;
}
void StandardFractal::outside_colormode_set_new_z_final()
{
	if (g_integer_fractal)
	{
		g_new_z = ComplexFudgeToDouble(g_new_z_l);
	}
	else if (g_bf_math == BIGNUM)
	{
		g_new_z.real(double(bntofloat(g_new_z_bn.x)));
		g_new_z.imag(double(bntofloat(g_new_z_bn.y)));
	}
}
void StandardFractal::outside_colormode_real_final()
{
	// Add 7 to overcome negative values on the MANDEL
	g_color_iter += long(g_new_z.real()) + 7;
}
void StandardFractal::outside_colormode_imaginary_final()
{
	// Add 7 to overcome negative values on the MANDEL
	g_color_iter += long(g_new_z.imag()) + 7;
}
void StandardFractal::outside_colormode_multiply_final()
{
	g_color_iter = long(double(g_color_iter)*(g_new_z.real()/g_new_z.imag()));
}
void StandardFractal::outside_colormode_sum_final()
{
	g_color_iter += long(g_new_z.real() + g_new_z.imag());
}
void StandardFractal::outside_colormode_inverse_tangent_final()
{
	g_color_iter = long(fabs(atan2(g_new_z.imag(), g_new_z.real())*g_externs.AtanColors()/MathUtil::Pi));
}
void StandardFractal::outside_colormode_float_modulus_final()
{
	g_color_iter = long(m_colormode_modulus_value*g_colors/g_proximity);
}
void StandardFractal::outside_colormode_total_distance_final()
{
	g_color_iter = long(m_colormode_total_distance);
}
void StandardFractal::eliminate_negative_colors_and_wrap_arounds()
{
	// eliminate negative colors & wrap arounds
	if ((g_color_iter <= 0 || g_color_iter > g_max_iteration) && g_externs.Outside() != COLORMODE_FLOAT_MODULUS)
	{
		g_color_iter = 1;
	}
}
void StandardFractal::outside_colormode_final()
{
	outside_colormode_set_new_z_final();
	if (g_externs.Outside() == COLORMODE_REAL)
	{
		outside_colormode_real_final();
	}
	else if (g_externs.Outside() == COLORMODE_IMAGINARY)
	{
		outside_colormode_imaginary_final();
	}
	else if (g_externs.Outside() == COLORMODE_MULTIPLY && g_new_z.imag())
	{
		outside_colormode_multiply_final();
	}
	else if (g_externs.Outside() == COLORMODE_SUM)
	{
		outside_colormode_sum_final();
	}
	else if (g_externs.Outside() == COLORMODE_INVERSE_TANGENT)
	{
		outside_colormode_inverse_tangent_final();
	}
	else if (g_externs.Outside() == COLORMODE_FLOAT_MODULUS)
	{
		outside_colormode_float_modulus_final();
	}
	else if (g_externs.Outside() == COLORMODE_TOTAL_DISTANCE)
	{
		outside_colormode_total_distance_final();
	}

	eliminate_negative_colors_and_wrap_arounds();
}
double StandardFractal::distance_compute()
{
	double dist;
	dist = (g_new_z.real()) + sqr(g_new_z.imag());
	if (dist == 0 || g_overflow)
	{
		dist = 0;
	}
	else
	{
		dist *= sqr(log(dist))/(sqr(m_distance_test_derivative.x) + sqr(m_distance_test_derivative.y));
	}
	return dist;
}
bool StandardFractal::distance_get_color(double dist)
{
	if (dist < s_dem_delta)
	{
		g_color_iter = -g_distance_test;       // show boundary as specified color
		return true;       // no further adjustments apply
	}
	if (g_distance_test > 1)          // pick color based on distance
	{
		if (g_old_demm_colors) // this one is needed for old color scheme
		{
			g_color_iter = long(sqrt(sqrt(dist)/s_dem_width + 1));
		}
		else
		{
			g_color_iter = long(dist/s_dem_width + 1);
		}
		g_color_iter &= LONG_MAX;  // oops - color can be negative
		return true;       // no further adjustments apply
	}
	return false;
}
void StandardFractal::compute_decomposition_and_biomorph()
{
	if (g_decomposition[0] > 0)
	{
		decomposition();
	}
	else if (g_externs.Biomorph() != BIOMORPH_NONE)
	{
		if (g_integer_fractal)
		{
			if (labs(g_new_z_l.real()) < g_rq_limit2_l || labs(g_new_z_l.imag()) < g_rq_limit2_l)
			{
				g_color_iter = g_externs.Biomorph();
			}
		}
		else if (fabs(g_new_z.real()) < g_rq_limit2 || fabs(g_new_z.imag()) < g_rq_limit2)
		{
			g_color_iter = g_externs.Biomorph();
		}
	}
}
void StandardFractal::merge_escape_time_stripes()
{
	if (g_externs.Outside() >= 0 && !m_attracted) // merge escape-time stripes
	{
		g_color_iter = g_externs.Outside();
	}
	else if (g_log_table || g_log_calculation)
	{
		g_color_iter = logtablecalc(g_color_iter);
	}
}
void StandardFractal::inside_colormode_star_trail_final()
{
	g_color_iter = 0;
	for (int i = 1; i < 16; i++)
	{
		if (fabs(m_tangent_table[0] - m_tangent_table[i]) < .05)
		{
			g_color_iter = i;
			break;
		}
	}
}
void StandardFractal::inside_colormode_period_final()
{
	g_color_iter = (m_colormode_period_cycle_length > 0) ? m_colormode_period_cycle_length : g_max_iteration;
}
void StandardFractal::inside_colormode_epsilon_cross_final()
{
	if (m_colormode_epsilon_cross_hooper == HOOPER_POSITIVE_Y_AXIS)
	{
		g_color_iter = green;
	}
	else if (m_colormode_epsilon_cross_hooper == HOOPER_POSITIVE_X_AXIS)
	{
		g_color_iter = yellow;
	}
	else if (m_colormode_epsilon_cross_hooper == HOOPER_NONE)
	{
		g_color_iter = g_max_iteration;
	}
	if (g_show_orbit)
	{
		orbit_scrub();
	}
}
void StandardFractal::inside_colormode_inverse_tangent_final()
{
	if (g_integer_fractal)
	{
		g_new_z = ComplexFudgeToDouble(g_new_z_l);
	}
	g_color_iter = long(fabs(atan2(g_new_z.imag(), g_new_z.real())*g_externs.AtanColors()/MathUtil::Pi));
}
void StandardFractal::inside_colormode_float_modulus_final()
{
	g_color_iter = long(m_colormode_modulus_value*g_colors/g_proximity);
}
void StandardFractal::inside_colormode_beauty_of_fractals_60_final()
{
	g_color_iter = long(sqrt(m_colormode_bof60_min_magnitude)*75);
}
void StandardFractal::inside_colormode_beauty_of_fractals_61_final()
{
	g_color_iter = m_colormode_bof61_min_index;
}
void StandardFractal::inside_colormode_z_magnitude_final()
{
	g_color_iter = long(g_integer_fractal ?
		(FudgeToDouble(g_magnitude_l)*(g_max_iteration/2) + 1)
		: ((sqr(g_new_z.real()) + sqr(g_new_z.imag()))*(g_max_iteration/2) + 1));
}
void StandardFractal::adjust_color_log_map()
{
	if (g_log_table || g_log_calculation)
	{
		g_color_iter = logtablecalc(g_color_iter);
	}
}
void StandardFractal::inside_colormode_final()
{
	if (g_periodicity_check < 0 && m_caught_a_cycle)
	{
		g_color_iter = 7;           // show periodicity
	}
	else if (g_externs.Inside() >= 0)
	{
		g_color_iter = g_externs.Inside();              // set to specified color, ignore logpal
	}
	else
	{
		if (g_externs.Inside() == COLORMODE_STAR_TRAIL)
		{
			inside_colormode_star_trail_final();
		}
		else if (g_externs.Inside() == COLORMODE_PERIOD)
		{
			inside_colormode_period_final();
		}
		else if (g_externs.Inside() == COLORMODE_EPSILON_CROSS)
		{
			inside_colormode_epsilon_cross_final();
		}
		else if (g_externs.Inside() == COLORMODE_FLOAT_MODULUS_INTEGER)
		{
			inside_colormode_float_modulus_final();
		}
		else if (g_externs.Inside() == COLORMODE_INVERSE_TANGENT_INTEGER)
		{
			inside_colormode_inverse_tangent_final();
		}
		else if (g_externs.Inside() == COLORMODE_BEAUTY_OF_FRACTALS_60)
		{
			inside_colormode_beauty_of_fractals_60_final();
		}
		else if (g_externs.Inside() == COLORMODE_BEAUTY_OF_FRACTALS_61)
		{
			inside_colormode_beauty_of_fractals_61_final();
		}
		else if (g_externs.Inside() == COLORMODE_Z_MAGNITUDE)
		{
			inside_colormode_z_magnitude_final();
		}
		else if (g_externs.Inside() == COLORMODE_ITERATION)
		{
			g_color_iter = g_max_iteration;
		}
		adjust_color_log_map();
	}
}
void StandardFractal::set_final_color_and_plot()
{
	g_color = abs(int(g_color_iter));
	if (g_color_iter >= g_colors)  // don't use color 0 unless from inside/outside
	{
		g_color = int(((g_color_iter - 1) % g_and_color) + 1);
	}
	SetBoundaryTraceDebugColor();
	g_plot_color(g_col, g_row, g_color);
}
int StandardFractal::check_if_interrupted()
{
	int result = g_color;
	g_input_counter -= abs(int(g_real_color_iter));
	if (g_input_counter <= 0)
	{
		if (check_key())
		{
			result = -1;
		}
		g_input_counter = g_max_input_counter;
	}
	return result;
}
int StandardFractal::Execute()
{
	ValueSaver<long> saved_max_iterations(g_max_iteration);

	initialize();

	while (++g_color_iter < g_max_iteration)
	{
		// calculation of one orbit goes here
		// input in "g_old_z" -- output in "g_new_z"
		if (interrupted())
		{
			return -1;
		}

		if (g_distance_test && distance_test_compute())
		{
			break;
		}
		// the usual case
		else if ((g_current_fractal_specific->orbitcalc() && g_externs.Inside() != COLORMODE_STAR_TRAIL)
				|| g_overflow)
		{
			break;
		}
		if (g_show_orbit)
		{
			show_orbit();
		}
		if (inside_colormode_update())
		{
			goto plot_inside;
		}
		outside_colormode_update();

		m_attracted = detect_finite_attractor();
		if (m_attracted)
		{
			break;
		}

		check_periodicity();
	}

	if (g_show_orbit)
	{
		orbit_scrub();
	}

	g_real_color_iter = g_color_iter;           // save this before we start adjusting it
	if (g_color_iter >= g_max_iteration)
	{
		g_old_color_iter = 0;         // check periodicity immediately next time
	}
	else
	{
		g_old_color_iter = g_color_iter + 10;    // check when past this + 10 next time
		if (g_color_iter == 0)
		{
			g_color_iter = 1;         // needed to make same as calculate_mandelbrot_l
		}
	}

	if (g_potential_flag)
	{
		potential_compute();
		goto plot_pixel;          // skip any other adjustments
	}

	if (g_color_iter >= g_max_iteration)              // an "inside" point
	{
		goto plot_inside;         // distest, decomp, biomorph don't apply
	}

	if (g_externs.Outside() < COLORMODE_ITERATION)
	{
		outside_colormode_final();
	}

	if (g_distance_test)
	{
		double dist = distance_compute();
		if ((dist < s_dem_delta) && (g_distance_test > 0))
		{
			goto plot_inside;
		}
		if (distance_get_color(dist))
		{
			goto plot_pixel;
		}
		// use pixel's "regular" color
	}

	compute_decomposition_and_biomorph();
	merge_escape_time_stripes();

	goto plot_pixel;

plot_inside: // we're "inside"
	inside_colormode_final();

plot_pixel:
	set_final_color_and_plot();

	return check_if_interrupted();
}

// standardfractal doodad subroutines
static void decomposition()
	{
	static double cos45		= 0.70710678118654750;	// cos 45     degrees
	static double sin45		= 0.70710678118654750;	// sin 45     degrees
	static double cos22_5	= 0.92387953251128670;	// cos 22.5   degrees
	static double sin22_5	= 0.38268343236508980;	// sin 22.5   degrees
	static double cos11_25	= 0.98078528040323040;	// cos 11.25  degrees
	static double sin11_25	= 0.19509032201612820;	// sin 11.25  degrees
	static double cos5_625	= 0.99518472667219690;	// cos 5.625  degrees
	static double sin5_625	= 0.09801714032956060;	// sin 5.625  degrees
	static double tan22_5	= 0.41421356237309500;	// tan 22.5   degrees
	static double tan11_25	= 0.19891236737965800;	// tan 11.25  degrees
	static double tan5_625	= 0.09849140335716425;	// tan 5.625  degrees
	static double tan2_8125	= 0.04912684976946725;	// tan 2.8125 degrees
	static double tan1_4063	= 0.02454862210892544;	// tan 1.4063 degrees
	static long lcos45;								// cos 45     degrees
	static long lsin45;								// sin 45     degrees
	static long lcos22_5;							// cos 22.5   degrees
	static long lsin22_5;							// sin 22.5   degrees
	static long lcos11_25;							// cos 11.25  degrees
	static long lsin11_25;							// sin 11.25  degrees
	static long lcos5_625;							// cos 5.625  degrees
	static long lsin5_625;							// sin 5.625  degrees
	static long ltan22_5;							// tan 22.5   degrees
	static long ltan11_25;							// tan 11.25  degrees
	static long ltan5_625;							// tan 5.625  degrees
	static long ltan2_8125;							// tan 2.8125 degrees
	static long ltan1_4063;							// tan 1.4063 degrees
	static long reset_fudge = -1;

	int temp = 0;
	int save_temp = 0;
	ComplexL lalt;
	ComplexD alt;
	g_color_iter = 0;
	if (g_integer_fractal) // the only case
	{
		if (reset_fudge != g_externs.Fudge())
		{
			reset_fudge = g_externs.Fudge();
			// lcos45	= DoubleToFudge(cos45);
			lsin45		= DoubleToFudge(sin45);
			lcos22_5	= DoubleToFudge(cos22_5);
			lsin22_5	= DoubleToFudge(sin22_5);
			lcos11_25	= DoubleToFudge(cos11_25);
			lsin11_25	= DoubleToFudge(sin11_25);
			lcos5_625	= DoubleToFudge(cos5_625);
			lsin5_625	= DoubleToFudge(sin5_625);
			ltan22_5	= DoubleToFudge(tan22_5);
			ltan11_25	= DoubleToFudge(tan11_25);
			ltan5_625	= DoubleToFudge(tan5_625);
			ltan2_8125	= DoubleToFudge(tan2_8125);
			ltan1_4063	= DoubleToFudge(tan1_4063);
		}
		if (g_new_z_l.imag() < 0)
		{
			temp = 2;
			g_new_z_l.imag(-g_new_z_l.imag());
		}

		if (g_new_z_l.real() < 0)
		{
			++temp;
			g_new_z_l.real(-g_new_z_l.real());
		}
		if (g_decomposition[0] == 2)
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

		if (g_decomposition[0] >= 8)
		{
			temp <<= 1;
			if (g_new_z_l.real() < g_new_z_l.imag())
			{
				++temp;
				lalt.x = g_new_z_l.real(); // just
				g_new_z_l.real(g_new_z_l.imag()); // swap
				g_new_z_l.imag(lalt.x); // them
			}

			if (g_decomposition[0] >= 16)
			{
				temp <<= 1;
				if (multiply(g_new_z_l.real(), ltan22_5, g_bit_shift) < g_new_z_l.imag())
				{
					++temp;
					lalt = g_new_z_l;
					g_new_z_l.real(multiply(lalt.x, lcos45, g_bit_shift) +
						multiply(lalt.y, lsin45, g_bit_shift));
					g_new_z_l.imag(multiply(lalt.x, lsin45, g_bit_shift) -
						multiply(lalt.y, lcos45, g_bit_shift));
				}

				if (g_decomposition[0] >= 32)
				{
					temp <<= 1;
					if (multiply(g_new_z_l.real(), ltan11_25, g_bit_shift) < g_new_z_l.imag())
					{
						++temp;
						lalt = g_new_z_l;
						g_new_z_l.real(multiply(lalt.x, lcos22_5, g_bit_shift) +
							multiply(lalt.y, lsin22_5, g_bit_shift));
						g_new_z_l.imag(multiply(lalt.x, lsin22_5, g_bit_shift) -
							multiply(lalt.y, lcos22_5, g_bit_shift));
					}

					if (g_decomposition[0] >= 64)
					{
						temp <<= 1;
						if (multiply(g_new_z_l.real(), ltan5_625, g_bit_shift) < g_new_z_l.imag())
						{
							++temp;
							lalt = g_new_z_l;
							g_new_z_l.real(multiply(lalt.x, lcos11_25, g_bit_shift) +
								multiply(lalt.y, lsin11_25, g_bit_shift));
							g_new_z_l.imag(multiply(lalt.x, lsin11_25, g_bit_shift) -
								multiply(lalt.y, lcos11_25, g_bit_shift));
						}

						if (g_decomposition[0] >= 128)
						{
							temp <<= 1;
							if (multiply(g_new_z_l.real(), ltan2_8125, g_bit_shift) < g_new_z_l.imag())
							{
								++temp;
								lalt = g_new_z_l;
								g_new_z_l.real(multiply(lalt.x, lcos5_625, g_bit_shift) +
									multiply(lalt.y, lsin5_625, g_bit_shift));
								g_new_z_l.imag(multiply(lalt.x, lsin5_625, g_bit_shift) -
									multiply(lalt.y, lcos5_625, g_bit_shift));
							}

							if (g_decomposition[0] == 256)
							{
								temp <<= 1;
								if (multiply(g_new_z_l.real(), ltan1_4063, g_bit_shift) < g_new_z_l.imag())
								{
									if ((g_new_z_l.real()*ltan1_4063 < g_new_z_l.imag()))
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
		if (g_new_z.imag() < 0)
		{
			temp = 2;
			g_new_z.imag(-g_new_z.imag());
		}
		if (g_new_z.real() < 0)
		{
			++temp;
			g_new_z.real(-g_new_z.real());
		}
		if (g_decomposition[0] == 2)
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
		if (g_decomposition[0] >= 8)
		{
			temp <<= 1;
			if (g_new_z.real() < g_new_z.imag())
			{
				++temp;
				alt.x = g_new_z.real(); // just
				g_new_z.real(g_new_z.imag()); // swap
				g_new_z.imag(alt.x); // them
			}
			if (g_decomposition[0] >= 16)
			{
				temp <<= 1;
				if (g_new_z.real()*tan22_5 < g_new_z.imag())
				{
					++temp;
					alt = g_new_z;
					g_new_z.real(alt.x*cos45 + alt.y*sin45);
					g_new_z.imag(alt.x*sin45 - alt.y*cos45);
				}

				if (g_decomposition[0] >= 32)
				{
					temp <<= 1;
					if (g_new_z.real()*tan11_25 < g_new_z.imag())
					{
						++temp;
						alt = g_new_z;
						g_new_z.real(alt.x*cos22_5 + alt.y*sin22_5);
						g_new_z.imag(alt.x*sin22_5 - alt.y*cos22_5);
					}

					if (g_decomposition[0] >= 64)
					{
						temp <<= 1;
						if (g_new_z.real()*tan5_625 < g_new_z.imag())
						{
							++temp;
							alt = g_new_z;
							g_new_z.real(alt.x*cos11_25 + alt.y*sin11_25);
							g_new_z.imag(alt.x*sin11_25 - alt.y*cos11_25);
						}

						if (g_decomposition[0] >= 128)
						{
							temp <<= 1;
							if (g_new_z.real()*tan2_8125 < g_new_z.imag())
							{
								++temp;
								alt = g_new_z;
								g_new_z.real(alt.x*cos5_625 + alt.y*sin5_625);
								g_new_z.imag(alt.x*sin5_625 - alt.y*cos5_625);
							}

							if (g_decomposition[0] == 256)
							{
								temp <<= 1;
								if ((g_new_z.real()*tan1_4063 < g_new_z.imag()))
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
	if (g_decomposition[0] == 2)
	{
		g_color_iter = (save_temp & 2) ? 1 : 0;
	}
	if (g_colors > g_decomposition[0])
	{
		g_color_iter++;
	}
}

//
// Continuous potential calculation for Mandelbrot and Julia
// Reference: Science of Fractal Images p. 190.
// Special thanks to Mark Peterson for his "MtMand" program that
// beautifully approximates plate 25 (same reference) and spurred
// on the inclusion of similar capabilities in Iterated Dynamics.
//
// The purpose of this function is to calculate a color value
// for a fractal that varies continuously with the screen pixels
// locations for better rendering in 3D.
//
// Here "magnitude" is the modulus of the orbit value at
// "iterations". The potparms[] are user-entered paramters
// controlling the level and slope of the continuous potential
// surface. Returns color.  - Tim Wegner 6/25/89
//
static int potential(double mag, long iterations)
{
	float f_mag;
	float f_tmp;
	float pot;
	double d_tmp;
	int i_pot;
	long potential_l;

	if (iterations < g_max_iteration)
	{
		potential_l = iterations + 2;
		pot = float(potential_l);
		if (potential_l <= 0 || mag <= 1.0)
		{
			pot = 0.0f;
		}
		else
		{
			 // pot = log(mag)/pow(2.0, double(pot));
			if (potential_l < 120 && !g_float_flag) // empirically determined limit of fShift
			{
				f_mag = float(mag);
				fLog14(f_mag, f_tmp); // this SHOULD be non-negative
				fShift(f_tmp, char(-potential_l), pot);
			}
			else
			{
				d_tmp = log(mag)/double(pow(2.0, double(pot)));
				// prevent float type underflow
				pot = (d_tmp > FLT_MIN) ? float(d_tmp) : 0.0f;
			}
		}
		// following transformation strictly for aesthetic reasons
		// meaning of parameters:
		//		g_potential_parameter[0] -- zero potential level - highest color -
		//		g_potential_parameter[1] -- slope multiplier -- higher is steeper
		//		g_potential_parameter[2] -- g_rq_limit value if changeable (bailout for modulus)

		if (pot > 0.0)
		{
			if (g_float_flag)
			{
				pot = float(sqrt(double(pot)));
			}
			else
			{
				fSqrt14(pot, f_tmp);
				pot = f_tmp;
			}
			pot = float(g_potential_parameter[0] - pot*g_potential_parameter[1] - 1.0);
		}
		else
		{
			pot = float(g_potential_parameter[0] - 1.0);
		}
		if (pot < 1.0)
		{
			pot = 1.0f; // avoid color 0
		}
	}
	else if (g_externs.Inside() >= 0)
	{
		pot = float(g_externs.Inside());
	}
	else // inside < 0 implies inside = maxit, so use 1st pot param instead
	{
		pot = float(g_potential_parameter[0]);
	}

	potential_l = long(pot)*256;
	i_pot = int(potential_l >> 8);
	if (i_pot >= g_colors)
	{
		i_pot = g_colors - 1;
		potential_l = 255;
	}

	if (g_potential_16bit)
	{
		if (!driver_diskp()) // if g_plot_color_put_color won't be doing it for us
		{
			disk_write(g_col + g_screen_x_offset, g_row + g_screen_y_offset, i_pot);
		}
		disk_write(g_col + g_screen_x_offset, g_row + g_screen_height + g_screen_y_offset, int(potential_l));
	}

	return i_pot;
}


// symmetry plot setup

static int x_symmetry_split(int xaxis_row, bool xaxis_between)
{
	int i;
	if ((g_work_sym&0x11) == 0x10) // already decided not sym
	{
		return 1;
	}
	if ((g_work_sym&1) != 0) // already decided on sym
	{
		g_y_stop = (g_WorkList.yy_start() + g_WorkList.yy_stop())/2;
	}
	else // new window, decide
	{
		g_work_sym |= 0x10;
		if (xaxis_row <= g_WorkList.yy_start() || xaxis_row >= g_WorkList.yy_stop())
		{
			return 1; // axis not in window
		}
		i = xaxis_row + (xaxis_row - g_WorkList.yy_start());
		if (xaxis_between)
		{
			++i;
		}
		if (i > g_WorkList.yy_stop()) // split into 2 pieces, bottom has the symmetry
		{
			if (g_WorkList.num_items() >= MAX_WORK_LIST-1) // no room to split
			{
				return 1;
			}
			g_y_stop = xaxis_row - (g_WorkList.yy_stop() - xaxis_row);
			if (!xaxis_between)
			{
				--g_y_stop;
			}
			g_WorkList.add(g_WorkList.xx_start(), g_WorkList.xx_stop(), g_WorkList.xx_start(),
				g_y_stop + 1, g_WorkList.yy_stop(), g_y_stop + 1,
				g_work_pass, 0);
			g_WorkList.set_yy_stop(g_y_stop);
			return 1; // tell set_symmetry no sym for current window
		}
		if (i < g_WorkList.yy_stop()) // split into 2 pieces, top has the symmetry
		{
			if (g_WorkList.num_items() >= MAX_WORK_LIST-1) // no room to split
			{
				return 1;
			}
			g_WorkList.add(g_WorkList.xx_start(), g_WorkList.xx_stop(), g_WorkList.xx_start(),
				i + 1, g_WorkList.yy_stop(), i + 1,
				g_work_pass, 0);
			g_WorkList.set_yy_stop(i);
		}
		g_y_stop = xaxis_row;
		g_work_sym |= 1;
	}
	g_symmetry = SYMMETRY_NONE;
	return 0; // tell set_symmetry its a go
}

static int y_symmetry_split(int yaxis_col, bool yaxis_between)
{
	int i;
	if ((g_work_sym&0x22) == 0x20) // already decided not sym
	{
		return 1;
	}
	if ((g_work_sym&2) != 0) // already decided on sym
	{
		g_x_stop = (g_WorkList.xx_start() + g_WorkList.xx_stop())/2;
	}
	else // new window, decide
	{
		g_work_sym |= 0x20;
		if (yaxis_col <= g_WorkList.xx_start() || yaxis_col >= g_WorkList.xx_stop())
		{
			return 1; // axis not in window
		}
		i = yaxis_col + (yaxis_col - g_WorkList.xx_start());
		if (yaxis_between)
		{
			++i;
		}
		if (i > g_WorkList.xx_stop()) // split into 2 pieces, right has the symmetry
		{
			if (g_WorkList.num_items() >= MAX_WORK_LIST-1) // no room to split
			{
				return 1;
			}
			g_x_stop = yaxis_col - (g_WorkList.xx_stop() - yaxis_col);
			if (!yaxis_between)
			{
				--g_x_stop;
			}
			g_WorkList.add(g_x_stop + 1, g_WorkList.xx_stop(), g_x_stop + 1,
				g_WorkList.yy_start(), g_WorkList.yy_stop(), g_WorkList.yy_start(),
				g_work_pass, 0);
			g_WorkList.set_xx_stop(g_x_stop);
			return 1; // tell set_symmetry no sym for current window
		}
		if (i < g_WorkList.xx_stop()) // split into 2 pieces, left has the symmetry
		{
			if (g_WorkList.num_items() >= MAX_WORK_LIST-1) // no room to split
			{
				return 1;
			}
			g_WorkList.add(i + 1, g_WorkList.xx_stop(), i + 1,
				g_WorkList.yy_start(), g_WorkList.yy_stop(), g_WorkList.yy_start(),
				g_work_pass, 0);
			g_WorkList.set_xx_stop(i);
		}
		g_x_stop = yaxis_col;
		g_work_sym |= 2;
	}
	g_symmetry = SYMMETRY_NONE;
	return 0; // tell set_symmetry its a go
}

static void set_symmetry(int symmetry, bool use_list) // set up proper symmetrical plot functions
{
	// pixel number for origin
	// if axis between 2 pixels, not on one
	g_symmetry = SYMMETRY_X_AXIS;
	if (g_externs.StandardCalculationMode() == CALCMODE_SYNCHRONOUS_ORBITS
		|| g_externs.StandardCalculationMode() == CALCMODE_ORBITS)
	{
		return;
	}
	if (symmetry == SYMMETRY_NO_PLOT && g_force_symmetry == FORCESYMMETRY_NONE)
	{
		g_plot_color = plot_color_none;
		return;
	}
	// NOTE: 16-bit potential disables symmetry
	// also any decomp= option and any inversion not about the origin
	// also any rotation other than 180deg and any off-axis stretch
	if (g_bf_math)
	{
		if (cmp_bf(g_escape_time_state.m_grid_bf.x_min(), g_escape_time_state.m_grid_bf.x_3rd())
			|| cmp_bf(g_escape_time_state.m_grid_bf.y_min(), g_escape_time_state.m_grid_bf.y_3rd()))
		{
			return;
		}
	}
	if ((g_potential_flag && g_potential_16bit)
		|| (g_invert && g_inversion[2] != 0.0)
		|| g_decomposition[0] != 0
		|| g_escape_time_state.m_grid_fp.x_min() != g_escape_time_state.m_grid_fp.x_3rd()
		|| g_escape_time_state.m_grid_fp.y_min() != g_escape_time_state.m_grid_fp.y_3rd())
	{
		return;
	}
	if (symmetry != SYMMETRY_X_AXIS && symmetry != SYMMETRY_X_AXIS_NO_PARAMETER && g_inversion[1] != 0.0 && g_force_symmetry == FORCESYMMETRY_NONE)
	{
		return;
	}
	if (g_force_symmetry < FORCESYMMETRY_NONE)
	{
		symmetry = g_force_symmetry;
	}
	else if (g_force_symmetry == FORCESYMMETRY_SEARCH)
	{
		g_force_symmetry = symmetry;  // for backwards compatibility
	}
	else if (g_externs.Outside() == COLORMODE_REAL
			|| g_externs.Outside() == COLORMODE_IMAGINARY
			|| g_externs.Outside() == COLORMODE_MULTIPLY
			|| g_externs.Outside() == COLORMODE_SUM
			|| g_externs.Outside() == COLORMODE_INVERSE_TANGENT
			|| g_externs.Outside() == COLORMODE_FLOAT_MODULUS
			|| g_externs.Outside() == COLORMODE_TOTAL_DISTANCE
			|| g_externs.Inside() == COLORMODE_FLOAT_MODULUS_INTEGER
			|| g_externs.BailOutTest() == BAILOUT_MANHATTAN_R)
	{
		return;
	}
	bool parameters_have_zero_real = (g_parameter.real() == 0.0 && g_externs.UseInitialOrbitZ() != INITIALZ_ORBIT);
	bool parameters_have_zero_imaginary = (g_parameter.imag() == 0.0 && g_externs.UseInitialOrbitZ() != INITIALZ_ORBIT);
	bool parameters_are_zero = parameters_have_zero_real && parameters_have_zero_imaginary;
	switch (g_fractal_type)
	{
	case FRACTYPE_MANDELBROT_LAMBDA_FUNC_OR_FUNC_L:      // These need only P1 checked.
	case FRACTYPE_MANDELBROT_LAMBDA_FUNC_OR_FUNC_FP:     // P2 is used for a switch value
	case FRACTYPE_MANDELBROT_FUNC_OR_FUNC_L:         // These have NOPARM set in fractalp.c,
	case FRACTYPE_MANDELBROT_FUNC_OR_FUNC_FP:        // but it only applies to P1.
	case FRACTYPE_MANDELBROT_Z_POWER_FP:   // or P2 is an exponent
	case FRACTYPE_MANDELBROT_Z_POWER_L:
	case FRACTYPE_MANDELBROT_Z_TO_Z_PLUS_Z_POWER_FP:
	case FRACTYPE_MARKS_MANDELBROT:
	case FRACTYPE_MARKS_MANDELBROT_FP:
	case FRACTYPE_MARKS_JULIA:
	case FRACTYPE_MARKS_JULIA_FP:
		break;
	case FRACTYPE_FORMULA:  // Check P2, P3, P4 and P5
	case FRACTYPE_FORMULA_FP:
		parameters_are_zero = (parameters_are_zero && g_parameters[2] == 0.0 && g_parameters[3] == 0.0
						&& g_parameters[4] == 0.0 && g_parameters[5] == 0.0
						&& g_parameters[6] == 0.0 && g_parameters[7] == 0.0
						&& g_parameters[8] == 0.0 && g_parameters[9] == 0.0);
		parameters_have_zero_real = (parameters_have_zero_real && g_parameters[2] == 0.0 && g_parameters[4] == 0.0
						&& g_parameters[6] == 0.0 && g_parameters[8] == 0.0);
		parameters_have_zero_imaginary = (parameters_have_zero_imaginary && g_parameters[3] == 0.0 && g_parameters[5] == 0.0
						&& g_parameters[7] == 0.0 && g_parameters[9] == 0.0);
		break;
	default:   // Check P2 for the rest
		parameters_are_zero = (parameters_are_zero && g_parameter2.real() == 0.0 && g_parameter2.imag() == 0.0);
	}
	bool xaxis_on_screen = false;
	bool yaxis_on_screen = false;
	bf_t bft1;
	int saved = 0;
	if (g_bf_math)
	{
		saved = save_stack();
		bft1 = alloc_stack(g_rbf_length + 2);
		xaxis_on_screen = (sign_bf(g_escape_time_state.m_grid_bf.y_min()) != sign_bf(g_escape_time_state.m_grid_bf.y_max()));
		yaxis_on_screen = (sign_bf(g_escape_time_state.m_grid_bf.x_min()) != sign_bf(g_escape_time_state.m_grid_bf.x_max()));
	}
	else
	{
		xaxis_on_screen = (sign(g_escape_time_state.m_grid_fp.y_min()) != sign(g_escape_time_state.m_grid_fp.y_max()));
		yaxis_on_screen = (sign(g_escape_time_state.m_grid_fp.x_min()) != sign(g_escape_time_state.m_grid_fp.x_max()));
	}
	bool xaxis_between = false;
	int xaxis_row = -1;
	if (xaxis_on_screen) // axis is on screen
	{
		double ftemp;
		if (g_bf_math)
		{
			// ftemp = -g_yy_max/(g_yy_min-g_yy_max);
			sub_bf(bft1, g_escape_time_state.m_grid_bf.y_min(), g_escape_time_state.m_grid_bf.y_max());
			div_bf(bft1, g_escape_time_state.m_grid_bf.y_max(), bft1);
			neg_a_bf(bft1);
			ftemp = double(bftofloat(bft1));
		}
		else
		{
			ftemp = -g_escape_time_state.m_grid_fp.y_max()/(g_escape_time_state.m_grid_fp.y_min()
				- g_escape_time_state.m_grid_fp.y_max());
		}
		ftemp *= (g_y_dots-1);
		ftemp += 0.25;
		xaxis_row = int(ftemp);
		xaxis_between = (ftemp - xaxis_row >= 0.5);
		if (!use_list && (!xaxis_between || (xaxis_row + 1)*2 != g_y_dots))
		{
			xaxis_row = -1; // can't split screen, so dead center or not at all
		}
	}
	bool yaxis_between = false;
	int yaxis_col = -1;
	if (yaxis_on_screen) // axis is on screen
	{
		double ftemp;
		if (g_bf_math)
		{
			// ftemp = -g_xx_min/(g_xx_max-g_xx_min);
			sub_bf(bft1, g_escape_time_state.m_grid_bf.x_max(), g_escape_time_state.m_grid_bf.x_min());
			div_bf(bft1, g_escape_time_state.m_grid_bf.x_min(), bft1);
			neg_a_bf(bft1);
			ftemp = double(bftofloat(bft1));
		}
		else
		{
			ftemp = -g_escape_time_state.m_grid_fp.x_min()/g_escape_time_state.m_grid_fp.height();
		}
		ftemp *= (g_x_dots-1);
		ftemp += 0.25;
		yaxis_col = int(ftemp);
		yaxis_between = (ftemp - yaxis_col >= 0.5);
		if (!use_list && (!yaxis_between || (yaxis_col + 1)*2 != g_x_dots))
		{
			yaxis_col = -1; // can't split screen, so dead center or not at all
		}
	}
	switch (symmetry)
	{
	case SYMMETRY_X_AXIS_NO_REAL:
		if (!parameters_have_zero_real)
		{
			break;
		}
		goto xsym;
	case SYMMETRY_X_AXIS_NO_IMAGINARY:
		if (!parameters_have_zero_imaginary)
		{
			break;
		}
		goto xsym;
	case SYMMETRY_X_AXIS_NO_PARAMETER:
		if (!parameters_are_zero)
		{
			break;
		}
xsym:
	case SYMMETRY_X_AXIS:
		if (x_symmetry_split(xaxis_row, xaxis_between) == 0)
		{
			g_plot_color = g_externs.Basin() ? plot_color_symmetry_x_axis_basin : plot_color_symmetry_x_axis;
		}
		break;
	case SYMMETRY_Y_AXIS_NO_PARAMETER:
		if (!parameters_are_zero)
		{
			break;
		}
	case SYMMETRY_Y_AXIS:
		if (y_symmetry_split(yaxis_col, yaxis_between) == 0)
		{
			g_plot_color = plot_color_symmetry_y_axis;
		}
		break;
	case SYMMETRY_XY_AXIS_NO_PARAMETER:
		if (!parameters_are_zero)
		{
			break;
		}
	case SYMMETRY_XY_AXIS:
		x_symmetry_split(xaxis_row, xaxis_between);
		y_symmetry_split(yaxis_col, yaxis_between);
		switch (g_work_sym & 3)
		{
		case SYMMETRY_X_AXIS: // just xaxis symmetry
			g_plot_color = g_externs.Basin() ? plot_color_symmetry_x_axis_basin : plot_color_symmetry_x_axis;
			break;
		case SYMMETRY_Y_AXIS: // just yaxis symmetry
			if (g_externs.Basin()) // got no routine for this case
			{
				g_x_stop = g_WorkList.xx_stop(); // fix what split should not have done
				g_symmetry = SYMMETRY_X_AXIS;
			}
			else
			{
				g_plot_color = plot_color_symmetry_y_axis;
			}
			break;
		case SYMMETRY_XY_AXIS:
			g_plot_color = g_externs.Basin() ? plot_color_symmetry_xy_axis_basin : plot_color_symmetry_xy_axis;
		}
		break;
	case SYMMETRY_ORIGIN_NO_PARAMETER:
		if (!parameters_are_zero)
		{
			break;
		}
	case SYMMETRY_ORIGIN:
originsym:
		if (x_symmetry_split(xaxis_row, xaxis_between) == 0
			&& y_symmetry_split(yaxis_col, yaxis_between) == 0)
		{
			g_plot_color = plot_color_symmetry_origin;
			g_x_stop = g_WorkList.xx_stop(); // didn't want this changed
		}
		else
		{
			g_y_stop = g_WorkList.yy_stop(); // in case first split worked
			g_symmetry = SYMMETRY_X_AXIS;
			g_work_sym = 0x30; // let it recombine with others like it
		}
		break;
	case SYMMETRY_PI_NO_PARAMETER:
		if (!parameters_are_zero)
		{
			break;
		}
	case SYMMETRY_PI:                      // PI symmetry
		if (g_bf_math)
		{
			if (double(bftofloat(abs_a_bf(sub_bf(bft1,
					g_escape_time_state.m_grid_bf.x_max(),
					g_escape_time_state.m_grid_bf.x_min())))) < MathUtil::Pi/4)
			{
				break; // no point in pi symmetry if values too close
			}
		}
		else
		{
			if (fabs(g_escape_time_state.m_grid_fp.width()) < MathUtil::Pi/4)
			{
				break; // no point in pi symmetry if values too close
			}
		}
		if (g_invert && g_force_symmetry == FORCESYMMETRY_NONE)
		{
			goto originsym;
		}
		g_plot_color = plot_color_symmetry_pi;
		g_symmetry = SYMMETRY_NONE;
		if (x_symmetry_split(xaxis_row, xaxis_between) == 0
			&& y_symmetry_split(yaxis_col, yaxis_between) == 0)
		{
			// both axes or origin
			g_plot_color = (g_parameter.imag() == 0.0) ? plot_color_symmetry_pi_xy_axis : plot_color_symmetry_pi_origin;
		}
		else
		{
			g_y_stop = g_WorkList.yy_stop(); // in case first split worked
			g_work_sym = 0x30;  // don't mark pisym as ysym, just do it unmarked
		}
		if (g_bf_math)
		{
			sub_bf(bft1, g_escape_time_state.m_grid_bf.x_max(), g_escape_time_state.m_grid_bf.x_min());
			abs_a_bf(bft1);
			s_pi_in_pixels = int(MathUtil::Pi/double(bftofloat(bft1))*g_x_dots);
		}
		else
		{
			s_pi_in_pixels = int(MathUtil::Pi/fabs(g_escape_time_state.m_grid_fp.width())*g_x_dots);
		}

		g_x_stop = g_WorkList.xx_start() + s_pi_in_pixels-1;
		if (g_x_stop > g_WorkList.xx_stop())
		{
			g_x_stop = g_WorkList.xx_stop();
		}
		if (g_plot_color == plot_color_symmetry_pi_xy_axis)
		{
			int i = (g_WorkList.xx_start() + g_WorkList.xx_stop())/2;
			if (g_x_stop > i)
			{
				g_x_stop = i;
			}
		}
		break;
	default:                  // no symmetry
		break;
	}
	if (g_bf_math)
	{
		restore_stack(saved);
	}
}

static long automatic_log_map()
{
	// calculate round screen edges to avoid wasted colours in logmap
	// don't use symetry
	// don't use symetry
	g_row = 0;
	g_reset_periodicity = false;
	long old_maxit = g_max_iteration;
	int xstop = g_x_dots - 1;
	long mincolour = LONG_MAX;
	for (g_col = 0; g_col < xstop; g_col++) // top row
	{
		g_color = g_calculate_type();
		if (g_color == -1)
		{
			goto ack; // key pressed, bailout
		}
		if (g_real_color_iter < mincolour)
		{
			mincolour = g_real_color_iter;
			g_max_iteration = std::max(2L, mincolour); // speedup for when edges overlap lakes
		}
		if (g_col >= 32)
		{
			g_plot_color(g_col-32, g_row, 0);
		}
	}                                    // these lines tidy up for BTM etc
	for (int lag = 32; lag > 0; lag--)
	{
		g_plot_color(g_col-lag, g_row, 0);
	}

	g_col = xstop;
	{
		int ystop = g_y_dots - 1;
		for (g_row = 0; g_row < ystop; g_row++) // right  side
		{
			g_color = g_calculate_type();
			if (g_color == -1)
			{
				goto ack; // key pressed, bailout
			}
			if (g_real_color_iter < mincolour)
			{
				mincolour = g_real_color_iter;
				g_max_iteration = std::max(2L, mincolour); // speedup for when edges overlap lakes
			}
			if (g_row >= 32)
			{
				g_plot_color(g_col, g_row-32, 0);
			}
		}
		for (int lag = 32; lag > 0; lag--)
		{
			g_plot_color(g_col, g_row-lag, 0);
		}

		g_col = 0;
		for (g_row = 0; g_row < ystop; g_row++) // left  side
		{
			g_color = g_calculate_type();
			if (g_color == -1)
			{
				goto ack; // key pressed, bailout
			}
			if (g_real_color_iter < mincolour)
			{
				mincolour = g_real_color_iter;
				g_max_iteration = std::max(2L, mincolour); // speedup for when edges overlap lakes
			}
			if (g_row >= 32)
			{
				g_plot_color(g_col, g_row-32, 0);
			}
		}
		for (int lag = 32; lag > 0; lag--)
		{
			g_plot_color(g_col, g_row-lag, 0);
		}

		g_row = ystop;
		for (g_col = 0; g_col < xstop; g_col++) // bottom row
		{
			g_color = g_calculate_type();
			if (g_color == -1)
			{
				goto ack; // key pressed, bailout
			}
			if (g_real_color_iter < mincolour)
			{
				mincolour = g_real_color_iter;
				g_max_iteration = std::max(2L, mincolour); // speedup for when edges overlap lakes
			}
			if (g_col >= 32)
			{
				g_plot_color(g_col-32, g_row, 0);
			}
		}
		for (int lag = 32; lag > 0; lag--)
		{
			g_plot_color(g_col-lag, g_row, 0);
		}
	}

ack: // bailout here if key is pressed
	if (mincolour == 2)
	{
		g_resuming = true; // insure automatic_log_map not called again
	}
	g_max_iteration = old_maxit;

	return mincolour;
}

// Symmetry plot for period PI
static void plot_color_symmetry_pi(int x, int y, int color)
{
	while (x <= g_WorkList.xx_stop())
	{
		g_plot_color_put_color(x, y, color);
		x += s_pi_in_pixels;
	}
}
// Symmetry plot for period PI plus Origin Symmetry
static void plot_color_symmetry_pi_origin(int x, int y, int color)
{
	while (x <= g_WorkList.xx_stop())
	{
		g_plot_color_put_color(x, y, color);
		int i = g_WorkList.yy_stop() - (y - g_WorkList.yy_start());
		if (i > g_y_stop && i < g_y_dots)
		{
			int j = g_WorkList.xx_stop() - (x - g_WorkList.xx_start());
			if (j < g_x_dots)
			{
				g_plot_color_put_color(j, i, color);
			}
		}
		x += s_pi_in_pixels;
	}
}
// Symmetry plot for period PI plus Both Axis Symmetry
static void plot_color_symmetry_pi_xy_axis(int x, int y, int color)
{
	while (x <= (g_WorkList.xx_start() + g_WorkList.xx_stop())/2)
	{
		int j = g_WorkList.xx_stop()-(x-g_WorkList.xx_start());
		g_plot_color_put_color(x , y , color);
		if (j < g_x_dots)
		{
			g_plot_color_put_color(j , y , color);
		}
		int i = g_WorkList.yy_stop()-(y-g_WorkList.yy_start());
		if (i > g_y_stop && i < g_y_dots)
		{
			g_plot_color_put_color(x , i , color);
			if (j < g_x_dots)
			{
				g_plot_color_put_color(j , i , color);
			}
		}
		x += s_pi_in_pixels;
	}
}

// Symmetry plot for X Axis Symmetry
void plot_color_symmetry_x_axis(int x, int y, int color)
{
	g_plot_color_put_color(x, y, color);
	int i = g_WorkList.yy_stop()-(y-g_WorkList.yy_start());
	if (i > g_y_stop && i < g_y_dots)
	{
		g_plot_color_put_color(x, i, color);
	}
}

// Symmetry plot for Y Axis Symmetry
static void plot_color_symmetry_y_axis(int x, int y, int color)
{
	g_plot_color_put_color(x, y, color);
	int i = g_WorkList.xx_stop()-(x-g_WorkList.xx_start());
	if (i < g_x_dots)
	{
		g_plot_color_put_color(i, y, color);
	}
}

// Symmetry plot for Origin Symmetry
void plot_color_symmetry_origin(int x, int y, int color)
{
	g_plot_color_put_color(x, y, color);
	int i = g_WorkList.yy_stop()-(y-g_WorkList.yy_start());
	if (i > g_y_stop && i < g_y_dots)
	{
		int j = g_WorkList.xx_stop()-(x-g_WorkList.xx_start());
		if (j < g_x_dots)
		{
			g_plot_color_put_color(j, i, color);
		}
	}
}

// Symmetry plot for Both Axis Symmetry
static void plot_color_symmetry_xy_axis(int x, int y, int color)
{
	int j = g_WorkList.xx_stop()-(x-g_WorkList.xx_start());
	g_plot_color_put_color(x , y, color);
	if (j < g_x_dots)
	{
		g_plot_color_put_color(j , y, color);
	}
	int i = g_WorkList.yy_stop()-(y-g_WorkList.yy_start());
	if (i > g_y_stop && i < g_y_dots)
	{
		g_plot_color_put_color(x , i, color);
		if (j < g_x_dots)
		{
			g_plot_color_put_color(j , i, color);
		}
	}
}

static int Stripe(int color)
{
	return (g_externs.Basin() == 2 && color > 8) ? 8 : 0;
}

// Symmetry plot for X Axis Symmetry - Striped Newtbasin version
static void plot_color_symmetry_x_axis_basin(int x, int y, int color)
{
	g_plot_color_put_color(x, y, color);
	int stripe = Stripe(color);
	int i = g_WorkList.yy_stop()-(y-g_WorkList.yy_start());
	if (i > g_y_stop && i < g_y_dots)
	{
		color -= stripe;                    // reconstruct unstriped color
		color = (g_degree + 1-color) % g_degree + 1;  // symmetrical color
		color += stripe;                    // add stripe
		g_plot_color_put_color(x, i, color);
	}
}

// Symmetry plot for Both Axis Symmetry  - Newtbasin version
static void plot_color_symmetry_xy_axis_basin(int x, int y, int color)
{
	if (color == 0) // assumed to be "inside" color
	{
		plot_color_symmetry_xy_axis(x, y, color);
		return;
	}
	int stripe = Stripe(color);
	color -= stripe;               // reconstruct unstriped color
	int color1 = (color < g_degree/2 + 2) ?
		(g_degree/2 + 2 - color) : (g_degree/2 + g_degree + 2 - color);
	int j = g_WorkList.xx_stop()-(x-g_WorkList.xx_start());
	g_plot_color_put_color(x, y, color + stripe);
	if (j < g_x_dots)
	{
		g_plot_color_put_color(j, y, color1 + stripe);
	}
	int i = g_WorkList.yy_stop()-(y-g_WorkList.yy_start());
	if (i > g_y_stop && i < g_y_dots)
	{
		g_plot_color_put_color(x, i, stripe + (g_degree + 1 - color) % g_degree + 1);
		if (j < g_x_dots)
		{
			g_plot_color_put_color(j, i, stripe + (g_degree + 1 - color1) % g_degree + 1);
		}
	}
}

static void put_truecolor_disk(int x, int y, int color)
{
	putcolor_a(x, y, color);
	targa_color(x, y, color);
}

// Do nothing plot!!!
#ifdef __CLINT__
#pragma argsused
#endif

void plot_color_none(int x, int y, int color)
{
	x = 0;
	y = 0;
	color = 0;  // just for warning
}


class PerformWorkList
{
public:
	PerformWorkList();
	~PerformWorkList();

	void calculate();

private:
	void setup_alternate_math();
	void cleanup_alternate_math();
	void interrupted_or_completed();
	void show_dot_finish();
	void call_escape_time_engine();
	void common_escape_time_initialization();
	void show_dot_start();
	void get_top_work_list_item();
	void setup_per_image();
	void setup_distance_estimator();
	void setup_initial_work_list();
	void setup_standard_calculation_mode();
	void setup_potential();

	int (*m_save_orbit_calc)();  // function that calculates one orbit
	int (*m_save_per_pixel)();  // once-per-pixel init
	bool (*m_save_per_image)();  // once-per-image setup
};

PerformWorkList::PerformWorkList()
	: m_save_orbit_calc(0),
	m_save_per_pixel(0),
	m_save_per_image(0)
{
}

PerformWorkList::~PerformWorkList()
{
}

void PerformWorkList::setup_alternate_math()
{
	m_save_orbit_calc = 0;  // function that calculates one orbit
	m_save_per_pixel = 0;  // once-per-pixel init
	m_save_per_image = 0;  // once-per-image setup
	alternate_math *alt = find_alternate_math(g_bf_math);
	if (alt != 0)
	{
		m_save_orbit_calc = g_current_fractal_specific->orbitcalc;
		m_save_per_pixel = g_current_fractal_specific->per_pixel;
		m_save_per_image = g_current_fractal_specific->per_image;
		g_current_fractal_specific->orbitcalc = alt->orbitcalc;
		g_current_fractal_specific->per_pixel = alt->per_pixel;
		g_current_fractal_specific->per_image = alt->per_image;
	}
	else
	{
		g_bf_math = 0;
	}
}

void PerformWorkList::cleanup_alternate_math()
{
	if (m_save_orbit_calc != 0)
	{
		g_current_fractal_specific->orbitcalc = m_save_orbit_calc;
		g_current_fractal_specific->per_pixel = m_save_per_pixel;
		g_current_fractal_specific->per_image = m_save_per_image;
	}
}

void PerformWorkList::setup_potential()
{
	if (g_potential_flag && g_potential_16bit)
	{
		CalculationMode tmpcalcmode = g_externs.StandardCalculationMode();

		g_externs.SetStandardCalculationMode(CALCMODE_SINGLE_PASS);
		if (!g_resuming)
		{
			if (disk_start_potential() < 0)
			{
				g_potential_16bit = false;       // disk_start failed or cancelled
				g_externs.SetStandardCalculationMode(tmpcalcmode);    // maybe we can carry on???
			}
		}
	}
}

bool CanPerformRequestedMode(FractalTypeSpecificData const *fractal, CalculationMode mode)
{
	return (mode == CALCMODE_BOUNDARY_TRACE && !fractal->no_boundary_tracing())
		|| (mode == CALCMODE_SOLID_GUESS && !fractal->no_solid_guessing())
		|| (mode == CALCMODE_ORBITS && (fractal->calculate_type == standard_fractal));
}

void PerformWorkList::setup_standard_calculation_mode()
{
	if (!CanPerformRequestedMode(g_current_fractal_specific, g_externs.StandardCalculationMode()))
	{
		g_externs.SetStandardCalculationMode(CALCMODE_SINGLE_PASS);
	}
}

void PerformWorkList::setup_initial_work_list()
{
	g_WorkList.setup_initial_work_list();
}

void PerformWorkList::setup_distance_estimator()
{
	double dx_size;
	double dy_size;
	double aspect;
	if (g_pseudo_x && g_pseudo_y)
	{
		aspect = double(g_pseudo_y)/double(g_pseudo_x);
		dx_size = g_pseudo_x-1;
		dy_size = g_pseudo_y-1;
	}
	else
	{
		aspect = double(g_y_dots)/double(g_x_dots);
		dx_size = g_x_dots-1;
		dy_size = g_y_dots-1;
	}

	double delta_x_fp = (g_escape_time_state.m_grid_fp.x_max() - g_escape_time_state.m_grid_fp.x_3rd())/dx_size; // calculate stepsizes
	double delta_y_fp = (g_escape_time_state.m_grid_fp.y_max() - g_escape_time_state.m_grid_fp.y_3rd())/dy_size;
	double delta_x2_fp = (g_escape_time_state.m_grid_fp.x_3rd() - g_escape_time_state.m_grid_fp.x_min())/dy_size;
	double delta_y2_fp = (g_escape_time_state.m_grid_fp.y_3rd() - g_escape_time_state.m_grid_fp.y_min())/dx_size;

	g_rq_limit = s_rq_limit_save; // just in case changed to DEM_BAILOUT earlier
	if (g_distance_test != 1 || g_colors == 2) // not doing regular outside colors
	{
		if (g_rq_limit < DEM_BAILOUT)         // so go straight for dem bailout
		{
			g_rq_limit = DEM_BAILOUT;
		}
	}
	// must be mandel type, formula, or old PAR/GIF
	s_dem_mandelbrot =
		(!fractal_type_none(g_current_fractal_specific->tojulia)
		|| fractal_type_formula(g_fractal_type));

	s_dem_delta = sqr(g_escape_time_state.m_grid_fp.delta_x()) + sqr(delta_y2_fp);
	double ftemp = sqr(delta_y_fp) + sqr(delta_x2_fp);
	if (ftemp > s_dem_delta)
	{
		s_dem_delta = ftemp;
	}
	if (g_distance_test_width == 0)
	{
		g_distance_test_width = 1;
	}
	ftemp = g_distance_test_width;
	// multiply by thickness desired
	s_dem_delta *= (g_distance_test_width > 0) ? sqr(ftemp)/10000 : 1/(sqr(ftemp)*10000);
	s_dem_width = (sqrt(sqr(g_escape_time_state.m_grid_fp.width()) + sqr(g_escape_time_state.m_grid_fp.x_3rd() - g_escape_time_state.m_grid_fp.x_min()) )*aspect
		+ sqrt(sqr(g_escape_time_state.m_grid_fp.height()) + sqr(g_escape_time_state.m_grid_fp.y_3rd() - g_escape_time_state.m_grid_fp.y_min()) ) )/g_distance_test;
	ftemp = (g_rq_limit < DEM_BAILOUT) ? DEM_BAILOUT : g_rq_limit;
	ftemp += 3; // bailout plus just a bit
	double ftemp2 = log(ftemp);
	s_dem_too_big = fabs(ftemp)*fabs(ftemp2)*2/sqrt(s_dem_delta);
}

void PerformWorkList::setup_per_image()
{
	// per_image can override
	g_calculate_type = g_current_fractal_specific->calculate_type;
	g_symmetry = g_current_fractal_specific->symmetry; // calctype & symmetry
	g_plot_color = g_plot_color_put_color; // defaults when setsymmetry not called or does nothing
}

void PerformWorkList::get_top_work_list_item()
{
	g_WorkList.get_top_item();
}

void PerformWorkList::show_dot_start()
{
	g_.DAC().FindSpecialColors();
	switch (g_auto_show_dot)
	{
	case AUTOSHOWDOT_DARK:
		s_show_dot_color = g_.DAC().Dark() % g_colors;
		break;
	case AUTOSHOWDOT_MEDIUM:
		s_show_dot_color = g_.DAC().Medium() % g_colors;
		break;
	case AUTOSHOWDOT_BRIGHT:
	case AUTOSHOWDOT_AUTO:
		s_show_dot_color = g_.DAC().Bright() % g_colors;
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
		double dshowdot_width = double(g_size_dot)*g_x_dots/1024.0;
		// Arbitrary sanity limit, however s_show_dot_width will
		// overflow if dshowdot width gets near 256.
		if (dshowdot_width > 150.0)
		{
			s_show_dot_width = 150;
		}
		else if (dshowdot_width > 0.0)
		{
			s_show_dot_width = int(dshowdot_width);
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
		// calculate s_save_dots_len is exactly right for the
		// triangular-shaped shotdot cursor. The that cursor
		// is changed, this formula must match.
		while ((s_save_dots_len = sqr(s_show_dot_width) + 5*s_show_dot_width + 4) > 1000)
		{
			s_show_dot_width--;
		}
		s_save_dots = new BYTE[s_save_dots_len];
		if (s_save_dots != 0)
		{
			s_save_dots_len /= 2;
			s_fill_buffer = s_save_dots + s_save_dots_len;
			memset(s_fill_buffer, s_show_dot_color, s_save_dots_len);
			break;
		}
		// There's even less free memory than we thought, so reduce
		// s_show_dot_width still more
		s_show_dot_width--;
	}
	if (s_save_dots == 0)
	{
		s_show_dot_width = -1;
	}
	g_calculate_type_temp = g_calculate_type;
	g_calculate_type = calculate_type_show_dot;
}

void PerformWorkList::show_dot_finish()
{
	delete[] s_save_dots;
	s_save_dots = 0;
	s_fill_buffer = 0;
}

void PerformWorkList::common_escape_time_initialization()
{
	// some common initialization for escape-time pixel level routines
	g_close_enough = g_delta_min_fp*pow(2.0, double(-abs(g_periodicity_check)));
	g_close_enough_l = DoubleToFudge(g_close_enough); // "close enough" value
	g_input_counter = g_max_input_counter;

	set_symmetry(g_symmetry, true);

	if (!g_resuming && (labs(g_log_palette_mode) == 2 || (g_log_palette_mode && g_log_automatic_flag)))
	{
		// calculate round screen edges to work out best start for logmap
		g_log_palette_mode = (automatic_log_map()*(g_log_palette_mode/labs(g_log_palette_mode)));
		SetupLogTable();
	}
}

void PerformWorkList::call_escape_time_engine()
{
	// call the appropriate escape-time engine
	switch (g_externs.StandardCalculationMode())
	{
	case CALCMODE_SINGLE_PASS:
	case CALCMODE_DUAL_PASS:
	case CALCMODE_TRIPLE_PASS:
		s_multiPassScanner.Scan();
		break;
	case CALCMODE_SOLID_GUESS:
		g_solidGuessScanner.Scan();
		break;
	case CALCMODE_BOUNDARY_TRACE:
		g_boundaryTraceScan.Scan();
		break;
	case CALCMODE_DIFFUSION:
		g_diffusionScan.Scan();
		break;
	case CALCMODE_TESSERAL:
		g_tesseralScan.Scan();
		break;
	case CALCMODE_SYNCHRONOUS_ORBITS:
		g_synchronousOrbitScanner.Scan();
		break;
	case CALCMODE_ORBITS:
		s_orbitScanner.Scan();
		break;

	default:
		assert(!"bad calculation mode");
	}
}

void PerformWorkList::interrupted_or_completed()
{
	if (g_WorkList.num_items() > 0)
	{
		g_WorkList.put_resume();
	}
	else
	{
		g_externs.SetCalculationStatus(CALCSTAT_COMPLETED); // completed
	}
}

void PerformWorkList::calculate()
{
	setup_alternate_math();
	setup_potential();
	setup_standard_calculation_mode();
	setup_initial_work_list();
	if (g_distance_test)
	{
		setup_distance_estimator();
	}

	while (g_WorkList.num_items() > 0)
	{
		setup_per_image();
		get_top_work_list_item();
		g_externs.SetCalculationStatus(CALCSTAT_IN_PROGRESS);

		g_current_fractal_specific->per_image();
		if (g_show_dot >= 0)
		{
			show_dot_start();
		}

		common_escape_time_initialization();
		call_escape_time_engine();
		show_dot_finish();
		if (check_key()) // interrupted?
		{
			break;
		}
	}

	interrupted_or_completed();
	cleanup_alternate_math();
}

static PerformWorkList s_perform_work_list;

void perform_work_list()
{
	s_perform_work_list.calculate();
}
