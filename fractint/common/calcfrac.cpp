/*
	CALCFRAC.C contains the high level ("engine") code for calculating the
	fractal images (well, SOMEBODY had to do it!).
	Original author Tim Wegner, but just about ALL the authors have contributed
	SOME code to this routine at one time or another, or contributed to one of
	the many massive restructurings.
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
#include <assert.h>
#include <string.h>
#include <limits.h>
#include <time.h>

/* see Fractint.cpp for a description of the include hierarchy */
#include "port.h"
#include "prototyp.h"
#include "fractype.h"

#include "calcfrac.h"
#include "calcmand.h"
#include "drivers.h"
#include "filesystem.h"
#include "fpu.h"
#include "calcfrac.h"
#include "diskvid.h"
#include "fracsubr.h"
#include "fractalp.h"
#include "fractals.h"
#include "framain2.h"
#include "line3d.h"
#include "lorenz.h"
#include "miscfrac.h"
#include "miscres.h"
#include "prompts2.h"
#include "realdos.h"
#include "soi.h"

#include "EscapeTime.h"
#include "SoundState.h"
#include "MathUtil.h"
#include "Formula.h"
#include "WorkList.h"
#include "SolidGuess.h"
#include "BoundaryTrace.h"
#include "Tesseral.h"
#include "DiffusionScan.h"

#define SHOWDOT_SAVE    1
#define SHOWDOT_RESTORE 2

#define JUST_A_POINT 0
#define LOWER_RIGHT  1
#define UPPER_RIGHT  2
#define LOWER_LEFT   3
#define UPPER_LEFT   4

/* variables exported from this file */
int g_orbit_draw_mode = ORBITDRAW_RECTANGLE;
ComplexL g_init_orbit_l = { 0, 0 };

int g_and_color;

// magnitude of current orbit z
long g_magnitude_l = 0;

long g_limit_l = 0;
long g_limit2_l = 0;
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
int g_passes;
int g_invert;
double g_f_radius;
double g_f_x_center;
double g_f_y_center; /* for inversion */
void (_fastcall *g_plot_color_put_color)(int, int, int) = putcolor_a;
void (_fastcall *g_plot_color)(int, int, int) = putcolor_a;
double g_magnitude;
double g_rq_limit;
double g_rq_limit2;
bool g_no_magnitude_calculation = false;
bool g_use_old_periodicity = false;
bool g_use_old_distance_test = false;
bool g_old_demm_colors = false;
int (*g_calculate_type)() = NULL;
int (*g_calculate_type_temp)();
bool g_quick_calculate = false;
double g_proximity = 0.01;
double g_close_enough;
unsigned long g_magnitude_limit;               /* magnitude limit (CALCMAND) */
/* ORBIT variables */
bool g_show_orbit;                     /* flag to turn on and off */
int g_orbit_index;                      /* pointer into g_save_orbit array */
int g_orbit_color = 15;                 /* XOR color */
int g_x_stop;
int g_y_stop;							/* stop here */
SymmetryType g_symmetry;
int g_reset_periodicity; /* nonzero if escape time pixel rtn to reset */
int g_input_counter;
int g_max_input_counter;    /* avoids checking keyboard too often */
char *g_resume_info = NULL;                    /* resume info if allocated */
bool g_resuming;                           /* true if resuming after interrupt */
/* variables which must be visible for tab_display */
int g_got_status; /* -1 if not, 0 for 1or2pass, 1 for ssg, */
			  /* 2 for btm, 3 for 3d, 4 for tesseral, 5 for diffusion_scan */
              /* 6 for orbits */
int g_current_pass;
int g_total_passes;
int g_current_row;
int g_current_col;
int g_three_pass;
bool g_next_screen_flag; /* for cellular next screen generation */
int     g_num_attractors;                 /* number of finite attractors  */
ComplexD  g_attractors[N_ATTR];       /* finite attractor vals (f.p)  */
ComplexL g_attractors_l[N_ATTR];      /* finite attractor vals (int)  */
int    g_attractor_period[N_ATTR];          /* period of the finite attractor */
int g_periodicity_check;
/* next has a skip bit for each s_max_block unit;
	1st pass sets bit  [1]... off only if g_block's contents guessed;
	at end of 1st pass [0]... bits are set if any surrounding g_block not guessed;
	bits are numbered [..][y/16 + 1][x + 1] & (1<<(y&15)) */
typedef int (*TPREFIX)[2][MAX_Y_BLOCK][MAX_X_BLOCK];
/* size of next puts a limit of MAX_PIXELS pixels across on solid guessing logic */
BYTE g_stack[4096];              /* common temp, two put_line calls */
/* For periodicity testing, only in standard_fractal() */
int g_next_saved_incr;
int g_first_saved_and;
int g_atan_colors = 180;
long (*g_calculate_mandelbrot_asm_fp)();

/* routines in this module      */
//static void perform_work_list();
static int one_or_two_pass();
static int  _fastcall standard_calculate(int);
static int  _fastcall potential(double, long);
static void decomposition();
static void _fastcall set_symmetry(int symmetry, bool use_list);
static int  _fastcall x_symmetry_split(int, int);
static int  _fastcall y_symmetry_split(int, int);
static void _fastcall put_truecolor_disk(int, int, int);
static int draw_orbits();
/* added for testing automatic_log_map() */
static long automatic_log_map();
static void _fastcall plot_color_symmetry_pi(int x, int y, int color);
static void _fastcall plot_color_symmetry_pi_origin(int x, int y, int color);
static void _fastcall plot_color_symmetry_pi_xy_axis(int x, int y, int color);
static void _fastcall plot_color_symmetry_y_axis(int x, int y, int color);
static void _fastcall plot_color_symmetry_xy_axis(int x, int y, int color);
static void _fastcall plot_color_symmetry_x_axis_basin(int x, int y, int color);
static void _fastcall plot_color_symmetry_xy_axis_basin(int x, int y, int color);

int g_ix_start;
int g_iy_start;						/* start here */
int g_work_pass;
int g_work_sym;                   /* for the sake of calculate_mandelbrot    */

static double s_dem_delta;
static double s_dem_width;     /* distance estimator variables */
static double s_dem_too_big;
static bool s_dem_mandelbrot;
/* static vars for solid_guess & its subroutines */
static ComplexD s_saved_z;
static double s_rq_limit_save;
static int s_pixel_pi; /* value of pi in pixels */
static BYTE *s_save_dots = NULL;
static BYTE *s_fill_buffer;
static int s_save_dots_len;
static int s_show_dot_color;
static int s_show_dot_width = 0;

/* FMODTEST routine. */
/* Makes the test condition for the FMOD coloring type
	that of the current bailout method. 'or' and 'and'
	methods are not used - in these cases a normal
	modulus test is used
*/
static double fmod_test()
{
	if (g_inside == FMODI && g_save_release <= 2000) /* for backwards compatibility */
	{
		return (g_magnitude == 0.0 || !g_no_magnitude_calculation || g_integer_fractal) ?
			sqr(g_new_z.x) + sqr(g_new_z.y) : g_magnitude;
	}

	double result;
	switch (g_bail_out_test)
	{
	case Mod:
		result = (g_magnitude == 0.0 || !g_no_magnitude_calculation || g_integer_fractal) ?
			sqr(g_new_z.x) + sqr(g_new_z.y) : g_magnitude;
		break;
	case Real:
		result = sqr(g_new_z.x);
		break;
	case Imag:
		result = sqr(g_new_z.y);
		break;
	case Or:
		{
			double tmpx = sqr(g_new_z.x);
			double tmpy = sqr(g_new_z.y);
			result = (tmpx > tmpy) ? tmpx : tmpy;
		}
		break;
	case Manh:
		result = sqr(fabs(g_new_z.x) + fabs(g_new_z.y));
		break;
	case Manr:
		result = sqr(g_new_z.x + g_new_z.y);
		break;
	default:
		result = sqr(g_new_z.x) + sqr(g_new_z.y);
		break;
	}
	return result;
}

/*
	The sym_fill_line() routine was pulled out of the boundary tracing
	code for re-use with g_show_dot. It's purpose is to fill a line with a
	solid color. This assumes that BYTE *str is already filled
	with the color. The routine does write the line using symmetry
	in all cases, however the symmetry logic assumes that the line
	is one color; it is not general enough to handle a row of
	pixels of different colors.
*/
void sym_fill_line(int row, int left, int right, BYTE *str)
{
	put_line(row, left, right, str);

	/* here's where all the symmetry goes */
	int length = right-left + 1;
	if (g_plot_color == g_plot_color_put_color)
	{
		g_input_counter -= length >> 4; /* seems like a reasonable value */
	}
	else if (g_plot_color == plot_color_symmetry_x_axis) /* X-axis symmetry */
	{
		int i = g_WorkList.yy_stop() - (row - g_WorkList.yy_start());
		if (i > g_y_stop && i < g_y_dots)
		{
			put_line(i, left, right, str);
			g_input_counter -= length >> 3;
		}
	}
	else if (g_plot_color == plot_color_symmetry_y_axis) /* Y-axis symmetry */
	{
		put_line(row, g_WorkList.xx_stop()-(right-g_WorkList.xx_start()), g_WorkList.xx_stop()-(left-g_WorkList.xx_start()), str);
		g_input_counter -= length >> 3;
	}
	else if (g_plot_color == plot_color_symmetry_origin)  /* Origin symmetry */
	{
		int i = g_WorkList.yy_stop()-(row-g_WorkList.yy_start());
		int j = min(g_WorkList.xx_stop()-(right-g_WorkList.xx_start()), g_x_dots-1);
		int k = min(g_WorkList.xx_stop()-(left -g_WorkList.xx_start()), g_x_dots-1);
		if (i > g_y_stop && i < g_y_dots && j <= k)
		{
			put_line(i, j, k, str);
		}
		g_input_counter -= length >> 3;
	}
	else if (g_plot_color == plot_color_symmetry_xy_axis) /* X-axis and Y-axis symmetry */
	{
		int i = g_WorkList.yy_stop()-(row-g_WorkList.yy_start());
		int j = min(g_WorkList.xx_stop()-(right-g_WorkList.xx_start()), g_x_dots-1);
		int k = min(g_WorkList.xx_stop()-(left -g_WorkList.xx_start()), g_x_dots-1);
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
	else    /* cheap and easy way out */
	{
		for (int i = left; i <= right; i++)  /* DG */
		{
			(*g_plot_color)(i, row, str[i-left]);
		}
		g_input_counter -= length >> 1;
	}
}

/*
  The sym_put_line() routine is the symmetry-aware version of put_line().
  It only works efficiently in the no symmetry or SYMMETRY_X_AXIS symmetry case,
  otherwise it just writes the pixels one-by-one.
*/
static void sym_put_line(int row, int left, int right, BYTE *str)
{
	put_line(row, left, right, str);
	int length = right-left + 1;
	if (g_plot_color == g_plot_color_put_color)
	{
		g_input_counter -= length >> 4; /* seems like a reasonable value */
	}
	else if (g_plot_color == plot_color_symmetry_x_axis) /* X-axis symmetry */
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
		for (int i = left; i <= right; i++)  /* DG */
		{
			(*g_plot_color)(i, row, str[i-left]);
		}
		g_input_counter -= length >> 1;
	}
}

static void show_dot_save_restore(int startx, int stopx, int starty, int stopy, int direction, int action)
{
	if (direction != JUST_A_POINT)
	{
		if (s_save_dots == NULL)
		{
			stop_message(0, "s_save_dots NULL");
			exit(0);
		}
		if (s_fill_buffer == NULL)
		{
			stop_message(0, "s_fill_buffer NULL");
			exit(0);
		}
	}
	int ct = 0;
	switch (direction)
	{
	case LOWER_RIGHT:
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
		break;
	case UPPER_RIGHT:
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
		break;
	case LOWER_LEFT:
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
		break;
	case UPPER_LEFT:
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
		break;
	}
	if (action == SHOWDOT_SAVE)
	{
		(*g_plot_color)(g_col, g_row, s_show_dot_color);
	}
}

static int calculate_type_show_dot()
{
	int width = s_show_dot_width + 1;
	int startx = g_col;
	int stopx = g_col;
	int starty = g_row;
	int stopy = g_row;
	int direction = JUST_A_POINT;
	if (width > 0)
	{
		if (g_col + width <= g_x_stop && g_row + width <= g_y_stop)
		{
			/* preferred g_show_dot shape */
			direction = UPPER_LEFT;
			startx = g_col;
			stopx  = g_col + width;
			starty = g_row + width;
			stopy  = g_row + 1;
		}
		else if (g_col-width >= g_ix_start && g_row + width <= g_y_stop)
		{
			/* second choice */
			direction = UPPER_RIGHT;
			startx = g_col-width;
			stopx  = g_col;
			starty = g_row + width;
			stopy  = g_row + 1;
		}
		else if (g_col-width >= g_ix_start && g_row-width >= g_iy_start)
		{
			direction = LOWER_RIGHT;
			startx = g_col-width;
			stopx  = g_col;
			starty = g_row-width;
			stopy  = g_row-1;
		}
		else if (g_col + width <= g_x_stop && g_row-width >= g_iy_start)
		{
			direction = LOWER_LEFT;
			startx = g_col;
			stopx  = g_col + width;
			starty = g_row-width;
			stopy  = g_row-1;
		}
	}
	show_dot_save_restore(startx, stopx, starty, stopy, direction, SHOWDOT_SAVE);
	if (g_orbit_delay > 0)
	{
		sleep_ms(g_orbit_delay);
	}
	int out = (*g_calculate_type_temp)();
	show_dot_save_restore(startx, stopx, starty, stopy, direction, SHOWDOT_RESTORE);
	return out;
}

/******* calculate_fractal - the top level routine for generating an image *******/
int calculate_fractal()
{
	g_math_error_count = 0;
	g_num_attractors = 0;          /* default to no known finite attractors  */
	g_display_3d = DISPLAY3D_NONE;
	g_basin = 0;
	/* added yet another level of indirection to g_plot_color_put_color!!! TW */
	g_plot_color_put_color = putcolor_a;
	if (g_is_true_color && g_true_mode)
	{
		/* Have to force passes = 1 */
		g_user_standard_calculation_mode = '1';
		g_standard_calculation_mode = '1';
	}
	if (g_true_color)
	{
		check_write_file(g_light_name, ".tga");
		if (start_disk1(g_light_name, NULL, 0) == 0)
		{
			/* Have to force passes = 1 */
			g_user_standard_calculation_mode = '1';
			g_standard_calculation_mode = '1';
			g_plot_color_put_color = put_truecolor_disk;
		}
		else
		{
			g_true_color = 0;
		}
	}
	if (!g_escape_time_state.m_use_grid)
	{
		if (g_user_standard_calculation_mode != 'o')
		{
			g_user_standard_calculation_mode = '1';
			g_standard_calculation_mode = '1';
		}
	}

	g_formula_state.init_misc();  /* set up some variables in parser.c */
	reset_clock();

	/* following delta values useful only for types with rotation disabled */
	/* currently used only by bifurcation */
	if (g_integer_fractal)
	{
		g_distance_test = 0;
	}
	g_parameter.x   = g_parameters[0];
	g_parameter.y   = g_parameters[1];
	g_parameter2.x  = g_parameters[2];
	g_parameter2.y  = g_parameters[3];

	if (g_log_palette_mode && g_colors < 16)
	{
		stop_message(0, "Need at least 16 colors to use logmap");
		g_log_palette_mode = LOGPALETTE_NONE;
	}

	if (g_use_old_periodicity)
	{
		g_next_saved_incr = 1;
		g_first_saved_and = 1;
	}
	else
	{
		g_next_saved_incr = (int) log10((double) g_max_iteration); /* works better than log() */
		if (g_next_saved_incr < 4)
		{
			g_next_saved_incr = 4; /* maintains image with low iterations */
		}
		g_first_saved_and = (long) (g_next_saved_incr*2 + 1);
	}

	g_log_table = NULL;
	g_max_log_table_size = g_max_iteration;
	g_log_calculation = 0;

	/* below, INT_MAX = 32767 only when an integer is two bytes.  Which is not true for Xfractint. */
	/* Since 32767 is what was meant, replaced the instances of INT_MAX with 32767. */
	if (g_log_palette_mode
		&& (((g_max_iteration > 32767) && true) || g_log_dynamic_calculate == LOGDYNAMIC_DYNAMIC))
	{
		if (g_save_release > 1920)
		{
			g_log_calculation = 1; /* calculate on the fly */
			SetupLogTable();
		}
		else
		{
			g_max_log_table_size = 32767;
			g_log_calculation = 0; /* use logtable */
		}
	}
	else if (g_ranges_length && (g_max_iteration >= 32767))
	{
		g_max_log_table_size = 32766;
	}

	if ((g_log_palette_mode || g_ranges_length) && !g_log_calculation)
	{
		g_log_table = (BYTE *) malloc(g_max_log_table_size + 1);

		if (g_log_table == NULL)
		{
			if (g_ranges_length || g_log_dynamic_calculate == LOGDYNAMIC_TABLE)
			{
				stop_message(0, "Insufficient memory for logmap/ranges with this maxiter");
			}
			else
			{
				stop_message(0, "Insufficient memory for logTable, using on-the-fly routine");
				g_log_dynamic_calculate = LOGDYNAMIC_DYNAMIC;
				g_log_calculation = 1; /* calculate on the fly */
				SetupLogTable();
			}
		}
		else if (g_ranges_length)  /* Can't do ranges if g_max_log_table_size > 32767 */
		{
			int i = 0;
			int k = 0;
			int l = 0;
			g_log_palette_mode = LOGPALETTE_NONE; /* ranges overrides logmap */
			while (i < g_ranges_length)
			{
				int m = 0;
				int flip = 0;
				int altern = 32767;
				int numval = g_ranges[i++];
				if (numval < 0)
				{
					altern = g_ranges[i++];    /* sub-range iterations */
					numval = g_ranges[i++];
				}
				if ((numval > (int) g_max_log_table_size) || (i >= g_ranges_length))
				{
					numval = (int) g_max_log_table_size;
				}
				while (l <= numval)
				{
					g_log_table[l++] = (BYTE) (k + flip);
					if (++m >= altern)
					{
						flip ^= 1;            /* Alternate colors */
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
	g_magnitude_limit = 4L << g_bit_shift;                 /* CALCMAND magnitude limit */

	g_atan_colors = (g_save_release > 2002) ? g_colors : 180;

	/* ORBIT stuff */
	g_show_orbit = g_start_show_orbit;
	g_orbit_index = 0;
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

		if (g_inversion[0] == AUTOINVERT)  /*  auto calc radius 1/6 screen */
		{
			g_inversion[0] = min(fabs(g_escape_time_state.m_grid_fp.width()),
								fabs(g_escape_time_state.m_grid_fp.height()))/6.0;
			fix_inversion(&g_inversion[0]);
			g_f_radius = g_inversion[0];
		}

		if (g_invert < 2 || g_inversion[1] == AUTOINVERT)  /* xcenter not already set */
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

		if (g_invert < 3 || g_inversion[2] == AUTOINVERT)  /* ycenter not already set */
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

		g_invert = 3; /* so values will not be changed if we come back */
	}

	g_close_enough = g_delta_min_fp*pow(2.0, -(double)(abs(g_periodicity_check)));
	s_rq_limit_save = g_rq_limit;
	g_rq_limit2 = sqrt(g_rq_limit);
	if (g_integer_fractal)          /* for integer routines (lambda) */
	{
		g_parameter_l.x = (long) (g_parameter.x*g_fudge);    /* real portion of Lambda */
		g_parameter_l.y = (long) (g_parameter.y*g_fudge);    /* imaginary portion of Lambda */
		g_parameter2_l.x = (long) (g_parameter2.x*g_fudge);  /* real portion of Lambda2 */
		g_parameter2_l.y = (long) (g_parameter2.y*g_fudge);  /* imaginary portion of Lambda2 */
		g_limit_l = (long) (g_rq_limit*g_fudge);      /* stop if magnitude exceeds this */
		if (g_limit_l <= 0)
		{
			g_limit_l = 0x7fffffffL; /* klooge for integer math */
		}
		g_limit2_l = (long) (g_rq_limit2*g_fudge);    /* stop if magnitude exceeds this */
		g_close_enough_l = (long) (g_close_enough*g_fudge); /* "close enough" value */
		g_init_orbit_l.x = (long) (g_initial_orbit_z.x*g_fudge);
		g_init_orbit_l.y = (long) (g_initial_orbit_z.y*g_fudge);
	}
	g_resuming = (g_calculation_status == CALCSTAT_RESUMABLE);
	if (!g_resuming) /* free resume_info memory if any is hanging around */
	{
		end_resume();
		if (g_resave_mode)
		{
			update_save_name(g_save_name); /* do the pending increment */
			g_resave_mode = RESAVE_NO;
			g_started_resaves = false;
		}
		g_calculation_time = 0;
	}

	if (g_current_fractal_specific->calculate_type != standard_fractal
		&& g_current_fractal_specific->calculate_type != calculate_mandelbrot
		&& g_current_fractal_specific->calculate_type != calculate_mandelbrot_fp
		&& g_current_fractal_specific->calculate_type != lyapunov
		&& g_current_fractal_specific->calculate_type != froth_calc)
	{
		/* per_image can override calculate_type & symmetry */
		g_calculate_type = g_current_fractal_specific->calculate_type; 
		g_symmetry = g_current_fractal_specific->symmetry;
		/* defaults when setsymmetry not called or does nothing */
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
		g_calculation_status = CALCSTAT_IN_PROGRESS;
		/* only standard escape time engine supports distest */
		g_distance_test = 0;
		if (g_current_fractal_specific->per_image())
		{
			/* not a stand-alone */
			/* next two lines in case periodicity changed */
			g_close_enough = g_delta_min_fp*pow(2.0, -(double)(abs(g_periodicity_check)));
			g_close_enough_l = (long) (g_close_enough*g_fudge); /* "close enough" value */
			set_symmetry(g_symmetry, false);
			timer(TIMER_ENGINE, g_calculate_type); /* non-standard fractal engine */
		}
		if (check_key())
		{
			if (g_calculation_status == CALCSTAT_IN_PROGRESS) /* calctype didn't set this itself, */
			{
				g_calculation_status = CALCSTAT_NON_RESUMABLE;   /* so mark it interrupted, non-resumable */
			}
		}
		else
		{
			g_calculation_status = CALCSTAT_COMPLETED; /* no key, so assume it completed */
		}
	}
	else /* standard escape-time engine */
	{
		if (g_standard_calculation_mode == '3')  /* convoluted 'g' + '2' hybrid */
		{
			int oldcalcmode;
			oldcalcmode = g_standard_calculation_mode;
			if (!g_resuming || g_three_pass)
			{
				g_standard_calculation_mode = 'g';
				g_three_pass = 1;
				timer(TIMER_ENGINE, (int(*)()) perform_work_list);
				if (g_calculation_status == CALCSTAT_COMPLETED)
				{
					/* '2' is silly after 'g' for low rez */
					g_standard_calculation_mode = (g_x_dots >= 640) ? '2' : '1';
					timer(TIMER_ENGINE, (int(*)()) perform_work_list);
					g_three_pass = 0;
				}
			}
			else /* resuming '2' pass */
			{
				g_standard_calculation_mode = (g_x_dots >= 640) ? '2' : '1';
				timer(TIMER_ENGINE, (int (*)()) perform_work_list);
			}
			g_standard_calculation_mode = (char)oldcalcmode;
		}
		else /* main case, much nicer! */
		{
			g_three_pass = 0;
			timer(TIMER_ENGINE, (int(*)()) perform_work_list);
		}
	}
	g_calculation_time += g_timer_interval;

	if (g_log_table && !g_log_calculation)
	{
		free(g_log_table);   /* free if not using extraseg */
		g_log_table = NULL;
	}
	g_formula_state.free_work_area();

	g_sound_state.close();

	if (g_true_color)
	{
		disk_end();
	}
	return (g_calculation_status == CALCSTAT_COMPLETED) ? 0 : -1;
}

/**************** general escape-time engine routines *********************/

static int draw_rectangle_orbits()
{
	/* draw a rectangle */
	g_row = g_WorkList.yy_begin();
	g_col = g_WorkList.xx_begin();

	while (g_row <= g_y_stop)
	{
		g_current_row = g_row;
		while (g_col <= g_x_stop)
		{
			if (plotorbits2dfloat() == -1)
			{
				g_WorkList.add(g_WorkList.xx_start(), g_WorkList.xx_stop(), g_col, g_WorkList.yy_start(), g_WorkList.yy_stop(), g_row, 0, g_work_sym);
				return -1; /* interrupted */
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
	int dY;                     /* vector components */
	int final,                      /* final row or column number */
		G,                  /* used to test for new row or column */
		inc1,           /* G increment when row or column doesn't change */
		inc2;               /* G increment when row or column changes */
	char pos_slope;

	dX = g_x_stop - g_ix_start;                   /* find vector components */
	dY = g_y_stop - g_iy_start;
	pos_slope = (char)(dX > 0);                   /* is slope positive? */
	if (dY < 0)
	{
		pos_slope = (char)!pos_slope;
	}
	if (abs(dX) > abs(dY))                /* shallow line case */
	{
		if (dX > 0)         /* determine start point and last column */
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
		inc1 = 2*abs(dY);            /* determine increments and initial G */
		G = inc1 - abs(dX);
		inc2 = 2*(abs(dY) - abs(dX));
		if (pos_slope)
		{
			while (g_col <= final)    /* step through columns checking for new row */
			{
				if (plotorbits2dfloat() == -1)
				{
					g_WorkList.add(g_WorkList.xx_start(), g_WorkList.xx_stop(), g_col, g_WorkList.yy_start(), g_WorkList.yy_stop(), g_row, 0, g_work_sym);
					return -1; /* interrupted */
				}
				g_col++;
				if (G >= 0)             /* it's time to change rows */
				{
					g_row++;      /* positive slope so increment through the rows */
					G += inc2;
				}
				else                        /* stay at the same row */
				{
					G += inc1;
				}
			}
		}
		else
		{
			while (g_col <= final)    /* step through columns checking for new row */
			{
				if (plotorbits2dfloat() == -1)
				{
					g_WorkList.add(g_WorkList.xx_start(), g_WorkList.xx_stop(), g_col, g_WorkList.yy_start(), g_WorkList.yy_stop(), g_row, 0, g_work_sym);
					return -1; /* interrupted */
				}
				g_col++;
				if (G > 0)              /* it's time to change rows */
				{
					g_row--;      /* negative slope so decrement through the rows */
					G += inc2;
				}
				else                        /* stay at the same row */
				{
					G += inc1;
				}
			}
		}
	}   /* if |dX| > |dY| */
	else                            /* steep line case */
	{
		if (dY > 0)             /* determine start point and last row */
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
		inc1 = 2*abs(dX);            /* determine increments and initial G */
		G = inc1 - abs(dY);
		inc2 = 2*(abs(dX) - abs(dY));
		if (pos_slope)
		{
			while (g_row <= final)    /* step through rows checking for new column */
			{
				if (plotorbits2dfloat() == -1)
				{
					g_WorkList.add(g_WorkList.xx_start(), g_WorkList.xx_stop(), g_col, g_WorkList.yy_start(), g_WorkList.yy_stop(), g_row, 0, g_work_sym);
					return -1; /* interrupted */
				}
				g_row++;
				if (G >= 0)                 /* it's time to change columns */
				{
					g_col++;  /* positive slope so increment through the columns */
					G += inc2;
				}
				else                    /* stay at the same column */
				{
					G += inc1;
				}
			}
		}
		else
		{
			while (g_row <= final)    /* step through rows checking for new column */
			{
				if (plotorbits2dfloat() == -1)
				{
					g_WorkList.add(g_WorkList.xx_start(), g_WorkList.xx_stop(), g_col, g_WorkList.yy_start(), g_WorkList.yy_stop(), g_row, 0, g_work_sym);
					return -1; /* interrupted */
				}
				g_row++;
				if (G > 0)                  /* it's time to change columns */
				{
					g_col--;  /* negative slope so decrement through the columns */
					G += inc2;
				}
				else                    /* stay at the same column */
				{
					G += inc1;
				}
			}
		}
	}
	return 0;
}

/* TODO: this code does not yet work??? */
static int draw_function_orbits()
{
	double Xctr;
	double Yctr;
	LDBL Magnification; /* LDBL not really needed here, but used to match function parameters */
	double Xmagfactor;
	double Rotation;
	double Skew;
	int angle;
	double theta;
	double xfactor = g_x_dots/2.0;
	double yfactor = g_y_dots/2.0;

	angle = g_WorkList.xx_begin();  /* save angle in x parameter */

	convert_center_mag(&Xctr, &Yctr, &Magnification, &Xmagfactor, &Rotation, &Skew);
	if (Rotation <= 0)
	{
		Rotation += 360;
	}

	while (angle < Rotation)
	{
		theta = MathUtil::DegreesToRadians(angle);
		g_col = (int) (xfactor + (Xctr + Xmagfactor*cos(theta)));
		g_row = (int) (yfactor + (Yctr + Xmagfactor*sin(theta)));
		if (plotorbits2dfloat() == -1)
		{
			g_WorkList.add(angle, 0, 0, 0, 0, 0, 0, g_work_sym);
			return -1; /* interrupted */
		}
		angle++;
	}
	return 0;
}

static int draw_orbits()
{
	g_got_status = GOT_STATUS_ORBITS; /* for <tab> screen */
	g_total_passes = 1;

	if (plot_orbits_2d_setup() == -1)
	{
		g_standard_calculation_mode = 'g';
		return -1;
	}

	switch (g_orbit_draw_mode)
	{
	case ORBITDRAW_RECTANGLE:	return draw_rectangle_orbits();	break;
	case ORBITDRAW_LINE:		return draw_line_orbits();		break;
	case ORBITDRAW_FUNCTION:	return draw_function_orbits();	break;

	default:
		assert(false);
	}

	return 0;
}

static int one_or_two_pass()
{
	int i;

	g_total_passes = 1;
	if (g_standard_calculation_mode == '2')
	{
		g_total_passes = 2;
	}
	if (g_standard_calculation_mode == '2' && g_work_pass == 0) /* do 1st pass of two */
	{
		if (standard_calculate(1) == -1)
		{
			g_WorkList.add(g_WorkList.xx_start(), g_WorkList.xx_stop(), g_col, g_WorkList.yy_start(), g_WorkList.yy_stop(), g_row, 0, g_work_sym);
			return -1;
		}
		if (g_WorkList.num_items() > 0) /* g_work_list not empty, defer 2nd pass */
		{
			g_WorkList.add(g_WorkList.xx_start(), g_WorkList.xx_stop(), g_WorkList.xx_start(), g_WorkList.yy_start(), g_WorkList.yy_stop(), g_WorkList.yy_start(), 1, g_work_sym);
			return 0;
		}
		g_work_pass = 1;
		g_WorkList.set_xx_begin(g_WorkList.xx_start());
		g_WorkList.set_yy_begin(g_WorkList.yy_start());
	}
	/* second or only pass */
	if (standard_calculate(2) == -1)
	{
		i = g_WorkList.yy_stop();
		if (g_y_stop != g_WorkList.yy_stop()) /* must be due to symmetry */
		{
			i -= g_row - g_iy_start;
		}
		g_WorkList.add(g_WorkList.xx_start(), g_WorkList.xx_stop(), g_col, g_row, i, g_row, g_work_pass, g_work_sym);
		return -1;
	}

	return 0;
}

static int _fastcall standard_calculate(int passnum)
{
	g_got_status = GOT_STATUS_12PASS;
	g_current_pass = passnum;
	g_row = g_WorkList.yy_begin();
	g_col = g_WorkList.xx_begin();

	while (g_row <= g_y_stop)
	{
		g_current_row = g_row;
		g_reset_periodicity = 1;
		while (g_col <= g_x_stop)
		{
			/* on 2nd pass of two, skip even pts */
			if (g_quick_calculate && !g_resuming)
			{
				g_color = getcolor(g_col, g_row);
				if (g_color != g_inside)
				{
					++g_col;
					continue;
				}
			}
			if (passnum == 1 || g_standard_calculation_mode == '1' || (g_row&1) != 0 || (g_col&1) != 0)
			{
				if ((*g_calculate_type)() == -1) /* standard_fractal(), calculate_mandelbrot() or calculate_mandelbrot_fp() */
				{
					return -1; /* interrupted */
				}
				g_resuming = false; /* reset so g_quick_calculate works */
				g_reset_periodicity = 0;
				if (passnum == 1) /* first pass, copy pixel and bump col */
				{
					if ((g_row&1) == 0 && g_row < g_y_stop)
					{
						(*g_plot_color)(g_col, g_row + 1, g_color);
						if ((g_col&1) == 0 && g_col < g_x_stop)
						{
							(*g_plot_color)(g_col + 1, g_row + 1, g_color);
						}
					}
					if ((g_col&1) == 0 && g_col < g_x_stop)
					{
						(*g_plot_color)(++g_col, g_row, g_color);
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


int calculate_mandelbrot()              /* fast per pixel 1/2/b/g, called with row & col set */
{
	/* setup values from array to avoid using es reg in calcmand.asm */
	g_initial_x_l = g_lx_pixel();
	g_initial_y_l = g_ly_pixel();
	if (calculate_mandelbrot_asm() >= 0)
	{
		if ((g_log_table || g_log_calculation) /* map color, but not if maxit & adjusted for inside, etc */
				&& (g_real_color_iter < g_max_iteration || (g_inside < 0 && g_color_iter == g_max_iteration)))
			g_color_iter = logtablecalc(g_color_iter);
		g_color = abs((int)g_color_iter);
		if (g_color_iter >= g_colors)  /* don't use color 0 unless from inside/outside */
		{
			if (g_save_release <= 1950)
			{
				if (g_colors < 16)
				{
					g_color &= g_and_color;
				}
				else
				{
					g_color = ((g_color - 1) % g_and_color) + 1;  /* skip color zero */
				}
			}
			else
			{
				g_color = (g_colors < 16)
					? (int) (g_color_iter & g_and_color)
					: (int) (((g_color_iter - 1) % g_and_color) + 1);
			}
		}
		if (g_debug_mode != DEBUGMODE_BNDTRACE_NONZERO)
		{
			if (g_color <= 0 && g_standard_calculation_mode == 'b')   /* fix BTM bug */
			{
				g_color = 1;
			}
		}
		(*g_plot_color)(g_col, g_row, g_color);
	}
	else
	{
		g_color = (int)g_color_iter;
	}
	return g_color;
}

/************************************************************************/
/* added by Wes Loewer - sort of a floating point version of calculate_mandelbrot() */
/* can also handle invert, any g_rq_limit, g_potential_flag, zmag, epsilon cross,     */
/* and all the current outside options    -Wes Loewer 11/03/91          */
/************************************************************************/
int calculate_mandelbrot_fp()
{
	if (g_invert)
	{
		invert_z(&g_initial_z);
	}
	else
	{
		g_initial_z.x = g_dx_pixel();
		g_initial_z.y = g_dy_pixel();
	}
	if (g_calculate_mandelbrot_asm_fp() >= 0)
	{
		if (g_potential_flag)
		{
			g_color_iter = potential(g_magnitude, g_real_color_iter);
		}
		if ((g_log_table || g_log_calculation) /* map color, but not if maxit & adjusted for inside, etc */
				&& (g_real_color_iter < g_max_iteration || (g_inside < 0 && g_color_iter == g_max_iteration)))
		{
			g_color_iter = logtablecalc(g_color_iter);
		}
		g_color = abs((int)g_color_iter);
		if (g_color_iter >= g_colors)  /* don't use color 0 unless from inside/outside */
		{
			if (g_save_release <= 1950)
			{
				if (g_colors < 16)
				{
					g_color &= g_and_color;
				}
				else
				{
					g_color = ((g_color - 1) % g_and_color) + 1;  /* skip color zero */
				}
			}
			else
			{
				g_color = (g_colors < 16)
					? (int) (g_color_iter & g_and_color)
					: (int) (((g_color_iter - 1) % g_and_color) + 1);
			}
		}
		if (g_debug_mode != DEBUGMODE_BNDTRACE_NONZERO)
		{
			if (g_color == 0 && g_standard_calculation_mode == 'b' )   /* fix BTM bug */
			{
				g_color = 1;
			}
		}
		(*g_plot_color)(g_col, g_row, g_color);
	}
	else
	{
		g_color = (int)g_color_iter;
	}
	return g_color;
}
#define STARTRAILMAX FLT_MAX   /* just a convenient large number */
#define green 2
#define yellow 6
#if 1
#define NUMSAVED 40     /* define this to save periodicity analysis to file */
#endif
#if 0
#define MINSAVEDAND 3   /* if not defined, old method used */
#endif
int standard_fractal()       /* per pixel 1/2/b/g, called with row & col set */
{
#ifdef NUMSAVED
	ComplexD savedz[NUMSAVED];
	long caught[NUMSAVED];
	long changed[NUMSAVED];
	int zctr = 0;
#endif
	double tantable[16];
	int hooper = 0;
	long lcloseprox;
	double memvalue = 0.0;
	double min_orbit = 100000.0; /* orbit value closest to origin */
	long   min_index = 0;        /* iteration of min_orbit */
	long cyclelen = -1;
	long savedcoloriter = 0;
	int caught_a_cycle;
	long savedand;
	int savedincr;       /* for periodicity checking */
	ComplexL lsaved;
	bool attracted;
	ComplexL lat;
	ComplexD  at;
	ComplexD deriv;
	long dem_color = -1;
	ComplexD dem_new;
	int check_freq;
	double totaldist = 0.0;
	lcloseprox = (long) (g_proximity*g_fudge);
	long saved_max_iterations = g_max_iteration;
#ifdef NUMSAVED
	for (int i = 0; i < NUMSAVED; i++)
	{
		caught[i] = 0L;
		changed[i] = 0L;
	}
#endif
	if (g_inside == STARTRAIL)
	{
		for (int i = 0; i < 16; i++)
		{
			tantable[i] = 0.0;
		}
		if (g_save_release > 1824)
		{
			g_max_iteration = 16;
		}
	}
	if (g_periodicity_check == 0 || g_inside == COLORMODE_Z_MAGNITUDE || g_inside == STARTRAIL)
	{
		g_old_color_iter = 2147483647L;       /* don't check periodicity at all */
	}
	else if (g_inside == PERIOD)   /* for display-periodicity */
	{
		g_old_color_iter = (g_max_iteration/5)*4;       /* don't check until nearly done */
	}
	else if (g_reset_periodicity)
	{
		g_old_color_iter = 255;               /* don't check periodicity 1st 250 iterations */
	}

	/* Jonathan - how about this idea ? skips first saved value which never works */
#ifdef MINSAVEDAND
	if (g_old_color_iter < MINSAVEDAND)
	{
		g_old_color_iter = MINSAVEDAND;
	}
#else
	if (g_old_color_iter < g_first_saved_and) /* I like it! */
	{
		g_old_color_iter = g_first_saved_and;
	}
#endif
	/* really fractal specific, but we'll leave it here */
	if (!g_integer_fractal)
	{
		if (g_use_initial_orbit_z == INITIALZ_ORBIT)
		{
			s_saved_z = g_initial_orbit_z;
		}
		else
		{
			s_saved_z.x = 0;
			s_saved_z.y = 0;
		}
#ifdef NUMSAVED
		savedz[zctr++] = s_saved_z;
#endif
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
		g_initial_z.y = g_dy_pixel();
		if (g_distance_test)
		{
			if (g_use_old_distance_test)
			{
				g_rq_limit = s_rq_limit_save;
				if (g_distance_test != 1 || g_colors == 2) /* not doing regular outside colors */
				{
					if (g_rq_limit < DEM_BAILOUT)   /* so go straight for dem bailout */
					{
						g_rq_limit = DEM_BAILOUT;
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
		if (g_use_initial_orbit_z == INITIALZ_ORBIT)
		{
			lsaved = g_init_orbit_l;
		}
		else
		{
			lsaved.x = 0;
			lsaved.y = 0;
		}
		g_initial_z_l.y = g_ly_pixel();
	}
	g_orbit_index = 0;
	g_color_iter = 0;
	if (g_fractal_type == FRACTYPE_JULIA_FP || g_fractal_type == FRACTYPE_JULIA)
	{
		g_color_iter = -1;
	}
	caught_a_cycle = 0;
	if (g_inside == PERIOD)
	{
		savedand = 16;           /* begin checking every 16th cycle */
	}
	else
	{
		/* Jonathan - don't understand such a low savedand -- how about this? */
#ifdef MINSAVEDAND
		savedand = MINSAVEDAND;
#else
		savedand = g_first_saved_and;                /* begin checking every other cycle */
#endif
	}
	savedincr = 1;               /* start checking the very first time */

	if (g_inside <= COLORMODE_BEAUTY_OF_FRACTALS_60 && g_inside >= COLORMODE_BEAUTY_OF_FRACTALS_61)
	{
		g_magnitude = 0;
		g_magnitude_l = 0;
		min_orbit = 100000.0;
	}
	g_overflow = 0;                /* reset integer math overflow flag */

	g_current_fractal_specific->per_pixel(); /* initialize the calculations */

	attracted = false;

	ComplexD lastz;
	if (g_outside == TDIS)
	{
		if (g_integer_fractal)
		{
			g_old_z.x = ((double)g_old_z_l.x)/g_fudge;
			g_old_z.y = ((double)g_old_z_l.y)/g_fudge;
		}
		else if (g_bf_math == BIGNUM)
		{
			g_old_z = complex_bn_to_float(&bnold);
		}
		else if (g_bf_math == BIGFLT)
		{
			g_old_z = complex_bf_to_float(&bfold);
		}
		lastz.x = g_old_z.x;
		lastz.y = g_old_z.y;
	}

	check_freq = (((g_sound_state.flags() & SOUNDFLAG_ORBITMASK) > SOUNDFLAG_X || g_show_dot >= 0) && g_orbit_delay > 0)
		? 16 : 2048;

	if (g_show_orbit)
	{
		g_sound_state.write_time();
	}
	while (++g_color_iter < g_max_iteration)
	{
		/* calculation of one orbit goes here */
		/* input in "g_old_z" -- output in "g_new_z" */
		if (g_color_iter % check_freq == 0)
		{
			if (check_key())
			{
				return -1;
			}
		}

		if (g_distance_test)
		{
			double ftemp;
			/* Distance estimator for points near Mandelbrot set */
			/* Original code by Phil Wilson, hacked around by PB */
			/* Algorithms from Peitgen & Saupe, Science of Fractal Images, p.198 */
			ftemp = s_dem_mandelbrot
				? 2*(g_old_z.x*deriv.x - g_old_z.y*deriv.y) + 1
				: 2*(g_old_z.x*deriv.x - g_old_z.y*deriv.y);
			deriv.y = 2*(g_old_z.y*deriv.x + g_old_z.x*deriv.y);
			deriv.x = ftemp;
			if (g_use_old_distance_test)
			{
				if (sqr(deriv.x) + sqr(deriv.y) > s_dem_too_big)
				{
					break;
				}
			}
			else if (g_save_release > 1950)
			{
				if (max(fabs(deriv.x), fabs(deriv.y)) > s_dem_too_big)
				{
					break;
				}
			}
			/* if above exit taken, the later test vs s_dem_delta will place this
				point on the boundary, because mag(g_old_z) < bailout just now */

			if (g_current_fractal_specific->orbitcalc() || (g_overflow && g_save_release > 1826))
			{
				if (g_use_old_distance_test)
				{
					if (dem_color < 0)
					{
						dem_color = g_color_iter;
						dem_new = g_new_z;
					}
					if (g_rq_limit >= DEM_BAILOUT
						|| g_magnitude >= (g_rq_limit = DEM_BAILOUT)
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
		/* the usual case */
		else if ((g_current_fractal_specific->orbitcalc() && g_inside != STARTRAIL)
				|| g_overflow)
		{
			break;
		}
		if (g_show_orbit)
		{
			if (!g_integer_fractal)
			{
				if (g_bf_math == BIGNUM)
				{
					g_new_z = complex_bn_to_float(&bnnew);
				}
				else if (g_bf_math == BIGFLT)
				{
					g_new_z = complex_bf_to_float(&bfnew);
				}
				plot_orbit(g_new_z.x, g_new_z.y, -1);
			}
			else
			{
				plot_orbit_i(g_new_z_l.x, g_new_z_l.y, -1);
			}
		}
		if (g_inside < -1)
		{
			if (g_bf_math == BIGNUM)
			{
				g_new_z = complex_bn_to_float(&bnnew);
			}
			else if (g_bf_math == BIGFLT)
			{
				g_new_z = complex_bf_to_float(&bfnew);
			}
			if (g_inside == STARTRAIL)
			{
				if (0 < g_color_iter && g_color_iter < 16)
				{
					if (g_integer_fractal)
					{
						g_new_z.x = g_new_z_l.x;
						g_new_z.x /= g_fudge;
						g_new_z.y = g_new_z_l.y;
						g_new_z.y /= g_fudge;
					}

					if (g_save_release > 1824)
					{
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
						g_temp_sqr_x = g_new_z.x*g_new_z.x;
						g_temp_sqr_y = g_new_z.y*g_new_z.y;
						g_magnitude = g_temp_sqr_x + g_temp_sqr_y;
						g_old_z = g_new_z;
					}
					{
						int tmpcolor;
						tmpcolor = (int) (((g_color_iter - 1) % g_and_color) + 1);
						tantable[tmpcolor-1] = g_new_z.y/(g_new_z.x + .000001);
					}
				}
			}
			else if (g_inside == COLORMODE_EPSILON_CROSS)
			{
				hooper = 0;
				if (g_integer_fractal)
				{
					if (labs(g_new_z_l.x) < labs(lcloseprox))
					{
						hooper = (lcloseprox > 0? 1 : -1); /* close to y axis */
						goto plot_inside;
					}
					else if (labs(g_new_z_l.y) < labs(lcloseprox))
					{
						hooper = (lcloseprox > 0 ? 2: -2); /* close to x axis */
						goto plot_inside;
					}
				}
				else
				{
					if (fabs(g_new_z.x) < fabs(g_proximity))
					{
						hooper = (g_proximity > 0? 1 : -1); /* close to y axis */
						goto plot_inside;
					}
					else if (fabs(g_new_z.y) < fabs(g_proximity))
					{
						hooper = (g_proximity > 0? 2 : -2); /* close to x axis */
						goto plot_inside;
					}
				}
			}
			else if (g_inside == FMODI)
			{
				double mag;
				if (g_integer_fractal)
				{
					g_new_z.x = ((double)g_new_z_l.x)/g_fudge;
					g_new_z.y = ((double)g_new_z_l.y)/g_fudge;
				}
				mag = fmod_test();
				if (mag < g_proximity)
				{
					memvalue = mag;
				}
			}
			else if (g_inside <= COLORMODE_BEAUTY_OF_FRACTALS_60 && g_inside >= COLORMODE_BEAUTY_OF_FRACTALS_61)
			{
				if (g_integer_fractal)
				{
					if (g_magnitude_l == 0 || !g_no_magnitude_calculation)
					{
						g_magnitude_l = lsqr(g_new_z_l.x) + lsqr(g_new_z_l.y);
					}
					g_magnitude = g_magnitude_l;
					g_magnitude /= g_fudge;
				}
				else if (g_magnitude == 0.0 || !g_no_magnitude_calculation)
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

		if (g_outside == TDIS || g_outside == FMOD)
		{
			if (g_bf_math == BIGNUM)
			{
				g_new_z = complex_bn_to_float(&bnnew);
			}
			else if (g_bf_math == BIGFLT)
			{
				g_new_z = complex_bf_to_float(&bfnew);
			}
			if (g_outside == TDIS)
			{
				if (g_integer_fractal)
				{
					g_new_z.x = ((double)g_new_z_l.x)/g_fudge;
					g_new_z.y = ((double)g_new_z_l.y)/g_fudge;
				}
				totaldist += sqrt(sqr(lastz.x-g_new_z.x) + sqr(lastz.y-g_new_z.y));
				lastz.x = g_new_z.x;
				lastz.y = g_new_z.y;
			}
			else if (g_outside == FMOD)
			{
				double mag;
				if (g_integer_fractal)
				{
					g_new_z.x = ((double)g_new_z_l.x)/g_fudge;
					g_new_z.y = ((double)g_new_z_l.y)/g_fudge;
				}
				mag = fmod_test();
				if (mag < g_proximity)
				{
					memvalue = mag;
				}
			}
		}

		if (g_num_attractors > 0)       /* finite attractor in the list   */
		{                         /* NOTE: Integer code is UNTESTED */
			if (g_integer_fractal)
			{
				for (int i = 0; i < g_num_attractors; i++)
				{
					lat.x = g_new_z_l.x - g_attractors_l[i].x;
					lat.x = lsqr(lat.x);
					if (lat.x < g_attractor_radius_l)
					{
						lat.y = g_new_z_l.y - g_attractors_l[i].y;
						lat.y = lsqr(lat.y);
						if (lat.y < g_attractor_radius_l)
						{
							if ((lat.x + lat.y) < g_attractor_radius_l)
							{
								attracted = true;
								if (g_finite_attractor < 0)
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
				for (int i = 0; i < g_num_attractors; i++)
				{
					at.x = g_new_z.x - g_attractors[i].x;
					at.x = sqr(at.x);
					if (at.x < g_attractor_radius_fp)
					{
						at.y = g_new_z.y - g_attractors[i].y;
						at.y = sqr(at.y);
						if (at.y < g_attractor_radius_fp)
						{
							if ((at.x + at.y) < g_attractor_radius_fp)
							{
								attracted = true;
								if (g_finite_attractor < 0)
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
				break;              /* AHA! Eaten by an attractor */
			}
		}

		if (g_color_iter > g_old_color_iter) /* check periodicity */
		{
			if ((g_color_iter & savedand) == 0)            /* time to save a new value */
			{
				savedcoloriter = g_color_iter;
				if (g_integer_fractal)
				{
					lsaved = g_new_z_l; /* integer fractals */
				}
				else if (g_bf_math == BIGNUM)
				{
					copy_bn(bnsaved.x, bnnew.x);
					copy_bn(bnsaved.y, bnnew.y);
				}
				else if (g_bf_math == BIGFLT)
				{
					copy_bf(bfsaved.x, bfnew.x);
					copy_bf(bfsaved.y, bfnew.y);
				}
				else
				{
					s_saved_z = g_new_z;  /* floating pt fractals */
#ifdef NUMSAVED
					if (zctr < NUMSAVED)
					{
						changed[zctr]  = g_color_iter;
						savedz[zctr++] = s_saved_z;
					}
#endif
				}
				if (--savedincr == 0)    /* time to lengthen the periodicity? */
				{
					savedand = (savedand << 1) + 1;       /* longer periodicity */
					savedincr = g_next_saved_incr; /* restart counter */
				}
			}
			else                /* check against an old save */
			{
				if (g_integer_fractal)     /* floating-pt periodicity chk */
				{
					if (labs(lsaved.x - g_new_z_l.x) < g_close_enough_l)
					{
						if (labs(lsaved.y - g_new_z_l.y) < g_close_enough_l)
						{
							caught_a_cycle = 1;
						}
					}
				}
				else if (g_bf_math == BIGNUM)
				{
					if (cmp_bn(abs_a_bn(sub_bn(bntmp, bnsaved.x, bnnew.x)), bnclosenuff) < 0)
					{
						if (cmp_bn(abs_a_bn(sub_bn(bntmp, bnsaved.y, bnnew.y)), bnclosenuff) < 0)
						{
							caught_a_cycle = 1;
						}
					}
				}
				else if (g_bf_math == BIGFLT)
				{
					if (cmp_bf(abs_a_bf(sub_bf(bftmp, bfsaved.x, bfnew.x)), bfclosenuff) < 0)
					{
						if (cmp_bf(abs_a_bf(sub_bf(bftmp, bfsaved.y, bfnew.y)), bfclosenuff) < 0)
						{
							caught_a_cycle = 1;
						}
					}
				}
				else
				{
					if (fabs(s_saved_z.x - g_new_z.x) < g_close_enough)
					{
						if (fabs(s_saved_z.y - g_new_z.y) < g_close_enough)
						{
							caught_a_cycle = 1;
						}
					}
#ifdef NUMSAVED
					{
						for (int i = 0; i <= zctr; i++)
						{
							if (caught[i] == 0)
							{
								if (fabs(savedz[i].x - g_new_z.x) < g_close_enough)
								{
									if (fabs(savedz[i].y - g_new_z.y) < g_close_enough)
									{
										caught[i] = g_color_iter;
									}
								}
							}
						}
					}
#endif
				}
				if (caught_a_cycle)
				{
#ifdef NUMSAVED
					static FILE *fp = NULL;
					static char c;
					if (fp == NULL)
					{
						fp = dir_fopen(g_work_dir, "cycles.txt", "w");
					}
#endif
					cyclelen = g_color_iter - savedcoloriter;
#ifdef NUMSAVED
					fprintf(fp, "row %3d col %3d len %6ld iter %6ld savedand %6ld\n",
						g_row, g_col, cyclelen, g_color_iter, savedand);
					if (zctr > 1 && zctr < NUMSAVED)
					{
						for (int i = 0; i < zctr; i++)
						{
							fprintf(fp, "   caught %2d saved %6ld iter %6ld\n", i, changed[i], caught[i]);
						}
					}
					fflush(fp);
#endif
					g_color_iter = g_max_iteration - 1;
				}
			}
		}
	}  /* end while (g_color_iter++ < g_max_iteration) */

	if (g_show_orbit)
	{
		orbit_scrub();
	}

	g_real_color_iter = g_color_iter;           /* save this before we start adjusting it */
	if (g_color_iter >= g_max_iteration)
	{
		g_old_color_iter = 0;         /* check periodicity immediately next time */
	}
	else
	{
		g_old_color_iter = g_color_iter + 10;    /* check when past this + 10 next time */
		if (g_color_iter == 0)
		{
			g_color_iter = 1;         /* needed to make same as calculate_mandelbrot */
		}
	}

	if (g_potential_flag)
	{
		if (g_integer_fractal)       /* adjust integer fractals */
		{
			g_new_z.x = ((double) g_new_z_l.x)/g_fudge;
			g_new_z.y = ((double) g_new_z_l.y)/g_fudge;
		}
		else if (g_bf_math == BIGNUM)
		{
			g_new_z.x = (double) bntofloat(bnnew.x);
			g_new_z.y = (double) bntofloat(bnnew.y);
		}
		else if (g_bf_math == BIGFLT)
		{
			g_new_z.x = (double) bftofloat(bfnew.x);
			g_new_z.y = (double) bftofloat(bfnew.y);
		}
		g_magnitude = sqr(g_new_z.x) + sqr(g_new_z.y);
		g_color_iter = potential(g_magnitude, g_color_iter);
		if (g_log_table || g_log_calculation)
		{
			g_color_iter = logtablecalc(g_color_iter);
		}
		goto plot_pixel;          /* skip any other adjustments */
	}

	if (g_color_iter >= g_max_iteration)              /* an "inside" point */
	{
		goto plot_inside;         /* distest, decomp, biomorph don't apply */
	}


	if (g_outside < -1)  /* these options by Richard Hughes modified by TW */
	{
		if (g_integer_fractal)
		{
			g_new_z.x = ((double)g_new_z_l.x)/g_fudge;
			g_new_z.y = ((double)g_new_z_l.y)/g_fudge;
		}
		else if (g_bf_math == BIGNUM)
		{
			g_new_z.x = (double)bntofloat(bnnew.x);
			g_new_z.y = (double)bntofloat(bnnew.y);
		}
		/* Add 7 to overcome negative values on the MANDEL    */
		if (g_outside == REAL)               /* "real" */
		{
			g_color_iter += (long)g_new_z.x + 7;
		}
		else if (g_outside == IMAG)          /* "imag" */
		{
			g_color_iter += (long)g_new_z.y + 7;
		}
		else if (g_outside == MULT  && g_new_z.y)  /* "mult" */
		{
			g_color_iter = (long)((double)g_color_iter*(g_new_z.x/g_new_z.y));
		}
		else if (g_outside == SUM)           /* "sum" */
		{
			g_color_iter += (long)(g_new_z.x + g_new_z.y);
		}
		else if (g_outside == ATAN)          /* "atan" */
		{
			g_color_iter = (long)fabs(atan2(g_new_z.y, g_new_z.x)*g_atan_colors/MathUtil::Pi);
		}
		else if (g_outside == FMOD)
		{
			g_color_iter = (long)(memvalue*g_colors/g_proximity);
		}
		else if (g_outside == TDIS)
		{
			g_color_iter = (long)(totaldist);
		}

		/* eliminate negative colors & wrap arounds */
		if ((g_color_iter <= 0 || g_color_iter > g_max_iteration) && g_outside != FMOD)
		{
			g_color_iter = (g_save_release < 1961) ? 0 : 1;
		}
	}

	if (g_distance_test)
	{
		double dist = (g_new_z.x) + sqr(g_new_z.y);
		if (dist == 0 || g_overflow)
		{
			dist = 0;
		}
		else
		{
			dist *= sqr(log(dist))/(sqr(deriv.x) + sqr(deriv.y));
		}
		if (dist < s_dem_delta)     /* point is on the edge */
		{
			if (g_distance_test > 0)
			{
				goto plot_inside;   /* show it as an inside point */
			}
			g_color_iter = -g_distance_test;       /* show boundary as specified color */
			goto plot_pixel;       /* no further adjustments apply */
		}
		if (g_colors == 2)
		{
			g_color_iter = !g_inside;   /* the only useful distest 2 color use */
			goto plot_pixel;       /* no further adjustments apply */
		}
		if (g_distance_test > 1)          /* pick color based on distance */
		{
			if (g_old_demm_colors) /* this one is needed for old color scheme */
			{
				g_color_iter = (long)sqrt(sqrt(dist)/s_dem_width + 1);
			}
			else if (g_use_old_distance_test)
			{
				g_color_iter = (long)sqrt(dist/s_dem_width + 1);
			}
			else
			{
				g_color_iter = (long)(dist/s_dem_width + 1);
			}
			g_color_iter &= LONG_MAX;  /* oops - color can be negative */
			goto plot_pixel;       /* no further adjustments apply */
		}
		if (g_use_old_distance_test)
		{
			g_color_iter = dem_color;
			g_new_z = dem_new;
		}
		/* use pixel's "regular" color */
	}

	if (g_decomposition[0] > 0)
	{
		decomposition();
	}
	else if (g_biomorph != -1)
	{
		if (g_integer_fractal)
		{
			if (labs(g_new_z_l.x) < g_limit2_l || labs(g_new_z_l.y) < g_limit2_l)
			{
				g_color_iter = g_biomorph;
			}
		}
		else if (fabs(g_new_z.x) < g_rq_limit2 || fabs(g_new_z.y) < g_rq_limit2)
		{
			g_color_iter = g_biomorph;
		}
	}

	if (g_outside >= 0 && !attracted) /* merge escape-time stripes */
	{
		g_color_iter = g_outside;
	}
	else if (g_log_table || g_log_calculation)
	{
		g_color_iter = logtablecalc(g_color_iter);
	}
	goto plot_pixel;

plot_inside: /* we're "inside" */
	if (g_periodicity_check < 0 && caught_a_cycle)
	{
		g_color_iter = 7;           /* show periodicity */
	}
	else if (g_inside >= 0)
	{
		g_color_iter = g_inside;              /* set to specified color, ignore logpal */
	}
	else
	{
		if (g_inside == STARTRAIL)
		{
			g_color_iter = 0;
			for (int i = 1; i < 16; i++)
			{
				if (fabs(tantable[0] - tantable[i]) < .05)
				{
					g_color_iter = i;
					break;
				}
			}
		}
		else if (g_inside == PERIOD)
		{
			g_color_iter = (cyclelen > 0) ? cyclelen : g_max_iteration;
		}
		else if (g_inside == COLORMODE_EPSILON_CROSS)
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
				g_color_iter = g_max_iteration;
			}
			if (g_show_orbit)
			{
				orbit_scrub();
			}
		}
		else if (g_inside == FMODI)
		{
			g_color_iter = (long) (memvalue*g_colors/g_proximity);
		}
		else if (g_inside == ATANI)          /* "atan" */
		{
			if (g_integer_fractal)
			{
				g_new_z.x = ((double) g_new_z_l.x)/g_fudge;
				g_new_z.y = ((double) g_new_z_l.y)/g_fudge;
			}
			g_color_iter = (long) fabs(atan2(g_new_z.y, g_new_z.x)*g_atan_colors/MathUtil::Pi);
		}
		else if (g_inside == COLORMODE_BEAUTY_OF_FRACTALS_60)
		{
			g_color_iter = (long) (sqrt(min_orbit)*75);
		}
		else if (g_inside == COLORMODE_BEAUTY_OF_FRACTALS_61)
		{
			g_color_iter = min_index;
		}
		else if (g_inside == COLORMODE_Z_MAGNITUDE)
		{
			g_color_iter = (long) (g_integer_fractal ?
				(((double) g_magnitude_l/g_fudge)*(g_max_iteration/2) + 1)
				: ((sqr(g_new_z.x) + sqr(g_new_z.y))*(g_max_iteration/2) + 1));
		}
		else /* inside == -1 */
		{
			g_color_iter = g_max_iteration;
		}
		if (g_log_table || g_log_calculation)
		{
			g_color_iter = logtablecalc(g_color_iter);
		}
	}

plot_pixel:
	g_color = abs((int)g_color_iter);
	if (g_color_iter >= g_colors)  /* don't use color 0 unless from inside/outside */
	{
		if (g_save_release <= 1950)
		{
			if (g_colors < 16)
			{
				g_color &= g_and_color;
			}
			else
			{
				g_color = ((g_color - 1) % g_and_color) + 1;  /* skip color zero */
			}
		}
		else
		{
			g_color = (g_colors < 16)
				? (int) (g_color_iter & g_and_color)
				: (int) (((g_color_iter - 1) % g_and_color) + 1);
		}
	}
	if (g_debug_mode != DEBUGMODE_BNDTRACE_NONZERO)
	{
		if (g_color <= 0 && g_standard_calculation_mode == 'b' )   /* fix BTM bug */
		{
			g_color = 1;
		}
	}
	(*g_plot_color)(g_col, g_row, g_color);

	g_max_iteration = saved_max_iterations;
	g_input_counter -= abs((int)g_real_color_iter);
	if (g_input_counter <= 0)
	{
		if (check_key())
		{
			return -1;
		}
		g_input_counter = g_max_input_counter;
	}
	return g_color;
}
#undef green
#undef yellow

#define cos45  sin45
#define lcos45 lsin45

/**************** standardfractal doodad subroutines *********************/
static void decomposition()
{
	/* static double cos45     = 0.70710678118654750; */ /* cos 45  degrees */
	static double sin45     = 0.70710678118654750; /* sin 45     degrees */
	static double cos22_5   = 0.92387953251128670; /* cos 22.5   degrees */
	static double sin22_5   = 0.38268343236508980; /* sin 22.5   degrees */
	static double cos11_25  = 0.98078528040323040; /* cos 11.25  degrees */
	static double sin11_25  = 0.19509032201612820; /* sin 11.25  degrees */
	static double cos5_625  = 0.99518472667219690; /* cos 5.625  degrees */
	static double sin5_625  = 0.09801714032956060; /* sin 5.625  degrees */
	static double tan22_5   = 0.41421356237309500; /* tan 22.5   degrees */
	static double tan11_25  = 0.19891236737965800; /* tan 11.25  degrees */
	static double tan5_625  = 0.09849140335716425; /* tan 5.625  degrees */
	static double tan2_8125 = 0.04912684976946725; /* tan 2.8125 degrees */
	static double tan1_4063 = 0.02454862210892544; /* tan 1.4063 degrees */
	/* static long lcos45     ;*/ /* cos 45   degrees */
	static long lsin45; /* sin 45     degrees */
	static long lcos22_5; /* cos 22.5   degrees */
	static long lsin22_5; /* sin 22.5   degrees */
	static long lcos11_25; /* cos 11.25  degrees */
	static long lsin11_25; /* sin 11.25  degrees */
	static long lcos5_625; /* cos 5.625  degrees */
	static long lsin5_625; /* sin 5.625  degrees */
	static long ltan22_5; /* tan 22.5   degrees */
	static long ltan11_25; /* tan 11.25  degrees */
	static long ltan5_625; /* tan 5.625  degrees */
	static long ltan2_8125; /* tan 2.8125 degrees */
	static long ltan1_4063; /* tan 1.4063 degrees */
	static long reset_fudge = -1;
	int temp = 0;
	int save_temp = 0;
	ComplexL lalt;
	ComplexD alt;
	g_color_iter = 0;
	if (g_integer_fractal) /* the only case */
	{
		if (reset_fudge != g_fudge)
		{
			reset_fudge = g_fudge;
			/* lcos45     = (long)(cos45*g_fudge); */
			lsin45     = (long)(sin45*g_fudge);
			lcos22_5   = (long)(cos22_5*g_fudge);
			lsin22_5   = (long)(sin22_5*g_fudge);
			lcos11_25  = (long)(cos11_25*g_fudge);
			lsin11_25  = (long)(sin11_25*g_fudge);
			lcos5_625  = (long)(cos5_625*g_fudge);
			lsin5_625  = (long)(sin5_625*g_fudge);
			ltan22_5   = (long)(tan22_5*g_fudge);
			ltan11_25  = (long)(tan11_25*g_fudge);
			ltan5_625  = (long)(tan5_625*g_fudge);
			ltan2_8125 = (long)(tan2_8125*g_fudge);
			ltan1_4063 = (long)(tan1_4063*g_fudge);
		}
		if (g_new_z_l.y < 0)
		{
			temp = 2;
			g_new_z_l.y = -g_new_z_l.y;
		}

		if (g_new_z_l.x < 0)
		{
			++temp;
			g_new_z_l.x = -g_new_z_l.x;
		}
		if (g_decomposition[0] == 2 && g_save_release >= 1827)
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
			if (g_new_z_l.x < g_new_z_l.y)
			{
				++temp;
				lalt.x = g_new_z_l.x; /* just */
				g_new_z_l.x = g_new_z_l.y; /* swap */
				g_new_z_l.y = lalt.x; /* them */
			}

			if (g_decomposition[0] >= 16)
			{
				temp <<= 1;
				if (multiply(g_new_z_l.x, ltan22_5, g_bit_shift) < g_new_z_l.y)
				{
					++temp;
					lalt = g_new_z_l;
					g_new_z_l.x = multiply(lalt.x, lcos45, g_bit_shift) +
						multiply(lalt.y, lsin45, g_bit_shift);
					g_new_z_l.y = multiply(lalt.x, lsin45, g_bit_shift) -
						multiply(lalt.y, lcos45, g_bit_shift);
				}

				if (g_decomposition[0] >= 32)
				{
					temp <<= 1;
					if (multiply(g_new_z_l.x, ltan11_25, g_bit_shift) < g_new_z_l.y)
					{
						++temp;
						lalt = g_new_z_l;
						g_new_z_l.x = multiply(lalt.x, lcos22_5, g_bit_shift) +
							multiply(lalt.y, lsin22_5, g_bit_shift);
						g_new_z_l.y = multiply(lalt.x, lsin22_5, g_bit_shift) -
							multiply(lalt.y, lcos22_5, g_bit_shift);
					}

					if (g_decomposition[0] >= 64)
					{
						temp <<= 1;
						if (multiply(g_new_z_l.x, ltan5_625, g_bit_shift) < g_new_z_l.y)
						{
							++temp;
							lalt = g_new_z_l;
							g_new_z_l.x = multiply(lalt.x, lcos11_25, g_bit_shift) +
								multiply(lalt.y, lsin11_25, g_bit_shift);
							g_new_z_l.y = multiply(lalt.x, lsin11_25, g_bit_shift) -
								multiply(lalt.y, lcos11_25, g_bit_shift);
						}

						if (g_decomposition[0] >= 128)
						{
							temp <<= 1;
							if (multiply(g_new_z_l.x, ltan2_8125, g_bit_shift) < g_new_z_l.y)
							{
								++temp;
								lalt = g_new_z_l;
								g_new_z_l.x = multiply(lalt.x, lcos5_625, g_bit_shift) +
									multiply(lalt.y, lsin5_625, g_bit_shift);
								g_new_z_l.y = multiply(lalt.x, lsin5_625, g_bit_shift) -
									multiply(lalt.y, lcos5_625, g_bit_shift);
							}

							if (g_decomposition[0] == 256)
							{
								temp <<= 1;
								if (multiply(g_new_z_l.x, ltan1_4063, g_bit_shift) < g_new_z_l.y)
								{
									if ((g_new_z_l.x*ltan1_4063 < g_new_z_l.y))
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
	else /* double case */
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
		if (g_decomposition[0] == 2 && g_save_release >= 1827)
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
			if (g_new_z.x < g_new_z.y)
			{
				++temp;
				alt.x = g_new_z.x; /* just */
				g_new_z.x = g_new_z.y; /* swap */
				g_new_z.y = alt.x; /* them */
			}
			if (g_decomposition[0] >= 16)
			{
				temp <<= 1;
				if (g_new_z.x*tan22_5 < g_new_z.y)
				{
					++temp;
					alt = g_new_z;
					g_new_z.x = alt.x*cos45 + alt.y*sin45;
					g_new_z.y = alt.x*sin45 - alt.y*cos45;
				}

				if (g_decomposition[0] >= 32)
				{
					temp <<= 1;
					if (g_new_z.x*tan11_25 < g_new_z.y)
					{
						++temp;
						alt = g_new_z;
						g_new_z.x = alt.x*cos22_5 + alt.y*sin22_5;
						g_new_z.y = alt.x*sin22_5 - alt.y*cos22_5;
					}

					if (g_decomposition[0] >= 64)
					{
						temp <<= 1;
						if (g_new_z.x*tan5_625 < g_new_z.y)
						{
							++temp;
							alt = g_new_z;
							g_new_z.x = alt.x*cos11_25 + alt.y*sin11_25;
							g_new_z.y = alt.x*sin11_25 - alt.y*cos11_25;
						}

						if (g_decomposition[0] >= 128)
						{
							temp <<= 1;
							if (g_new_z.x*tan2_8125 < g_new_z.y)
							{
								++temp;
								alt = g_new_z;
								g_new_z.x = alt.x*cos5_625 + alt.y*sin5_625;
								g_new_z.y = alt.x*sin5_625 - alt.y*cos5_625;
							}

							if (g_decomposition[0] == 256)
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
	if (g_decomposition[0] == 2 && g_save_release >= 1827)
	{
		g_color_iter = (save_temp & 2) ? 1 : 0;
		if (g_colors == 2)
		{
			g_color_iter++;
		}
	}
	else if (g_decomposition[0] == 2 && g_save_release < 1827)
	{
		g_color_iter &= 1;
	}
	if (g_colors > g_decomposition[0])
	{
		g_color_iter++;
	}
}

/******************************************************************/
/* Continuous potential calculation for Mandelbrot and Julia      */
/* Reference: Science of Fractal Images p. 190.                   */
/* Special thanks to Mark Peterson for his "MtMand" program that  */
/* beautifully approximates plate 25 (same reference) and spurred */
/* on the inclusion of similar capabilities in FRACTINT.          */
/*                                                                */
/* The purpose of this function is to calculate a color value     */
/* for a fractal that varies continuously with the screen pixels  */
/* locations for better rendering in 3D.                          */
/*                                                                */
/* Here "magnitude" is the modulus of the orbit value at          */
/* "iterations". The potparms[] are user-entered paramters        */
/* controlling the level and slope of the continuous potential    */
/* surface. Returns color.  - Tim Wegner 6/25/89                  */
/*                                                                */
/*                     -- Change history --                       */
/*                                                                */
/* 09/12/89   - added g_float_flag support and fixed float underflow */
/*                                                                */
/******************************************************************/

static int _fastcall potential(double mag, long iterations)
{
	float f_mag;
	float f_tmp;
	float pot;
	double d_tmp;
	int i_pot;
	long l_pot;

	if (iterations < g_max_iteration)
	{
		l_pot = iterations + 2;
		pot = (float) l_pot;
		if (l_pot <= 0 || mag <= 1.0)
		{
			pot = 0.0f;
		}
		else
		{
			 /* pot = log(mag)/pow(2.0, (double)pot); */
			if (l_pot < 120 && !g_float_flag) /* empirically determined limit of fShift */
			{
				f_mag = (float)mag;
				fLog14(f_mag, f_tmp); /* this SHOULD be non-negative */
				fShift(f_tmp, (char)-l_pot, pot);
			}
			else
			{
				d_tmp = log(mag)/(double)pow(2.0, (double)pot);
				/* prevent float type underflow */
				pot = (d_tmp > FLT_MIN) ? (float) d_tmp : 0.0f;
			}
		}
		/* following transformation strictly for aesthetic reasons */
		/* meaning of parameters:
				g_potential_parameter[0] -- zero potential level - highest color -
				g_potential_parameter[1] -- slope multiplier -- higher is steeper
				g_potential_parameter[2] -- g_rq_limit value if changeable (bailout for modulus) */

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
			pot = (float)(g_potential_parameter[0] - pot*g_potential_parameter[1] - 1.0);
		}
		else
		{
			pot = (float)(g_potential_parameter[0] - 1.0);
		}
		if (pot < 1.0)
		{
			pot = 1.0f; /* avoid color 0 */
		}
	}
	else if (g_inside >= 0)
	{
		pot = (float) g_inside;
	}
	else /* inside < 0 implies inside = maxit, so use 1st pot param instead */
	{
		pot = (float)g_potential_parameter[0];
	}

	l_pot = (long) pot*256;
	i_pot = (int) (l_pot >> 8);
	if (i_pot >= g_colors)
	{
		i_pot = g_colors - 1;
		l_pot = 255;
	}

	if (g_potential_16bit)
	{
		if (!driver_diskp()) /* if g_plot_color_put_color won't be doing it for us */
		{
			disk_write(g_col + g_sx_offset, g_row + g_sy_offset, i_pot);
		}
		disk_write(g_col + g_sx_offset, g_row + g_screen_height + g_sy_offset, (int)l_pot);
	}

	return i_pot;
}


/************************* symmetry plot setup ************************/

static int _fastcall x_symmetry_split(int xaxis_row, int xaxis_between)
{
	int i;
	if ((g_work_sym&0x11) == 0x10) /* already decided not sym */
	{
		return 1;
	}
	if ((g_work_sym&1) != 0) /* already decided on sym */
	{
		g_y_stop = (g_WorkList.yy_start() + g_WorkList.yy_stop())/2;
	}
	else /* new window, decide */
	{
		g_work_sym |= 0x10;
		if (xaxis_row <= g_WorkList.yy_start() || xaxis_row >= g_WorkList.yy_stop())
		{
			return 1; /* axis not in window */
		}
		i = xaxis_row + (xaxis_row - g_WorkList.yy_start());
		if (xaxis_between)
		{
			++i;
		}
		if (i > g_WorkList.yy_stop()) /* split into 2 pieces, bottom has the symmetry */
		{
			if (g_WorkList.num_items() >= MAX_WORK_LIST-1) /* no room to split */
			{
				return 1;
			}
			g_y_stop = xaxis_row - (g_WorkList.yy_stop() - xaxis_row);
			if (!xaxis_between)
			{
				--g_y_stop;
			}
			g_WorkList.add(g_WorkList.xx_start(), g_WorkList.xx_stop(), g_WorkList.xx_start(), g_y_stop + 1, g_WorkList.yy_stop(), g_y_stop + 1, g_work_pass, 0);
			g_WorkList.set_yy_stop(g_y_stop);
			return 1; /* tell set_symmetry no sym for current window */
		}
		if (i < g_WorkList.yy_stop()) /* split into 2 pieces, top has the symmetry */
		{
			if (g_WorkList.num_items() >= MAX_WORK_LIST-1) /* no room to split */
			{
				return 1;
			}
			g_WorkList.add(g_WorkList.xx_start(), g_WorkList.xx_stop(), g_WorkList.xx_start(), i + 1, g_WorkList.yy_stop(), i + 1, g_work_pass, 0);
			g_WorkList.set_yy_stop(i);
		}
		g_y_stop = xaxis_row;
		g_work_sym |= 1;
	}
	g_symmetry = SYMMETRY_NONE;
	return 0; /* tell set_symmetry its a go */
}

static int _fastcall y_symmetry_split(int yaxis_col, int yaxis_between)
{
	int i;
	if ((g_work_sym&0x22) == 0x20) /* already decided not sym */
	{
		return 1;
	}
	if ((g_work_sym&2) != 0) /* already decided on sym */
	{
		g_x_stop = (g_WorkList.xx_start() + g_WorkList.xx_stop())/2;
	}
	else /* new window, decide */
	{
		g_work_sym |= 0x20;
		if (yaxis_col <= g_WorkList.xx_start() || yaxis_col >= g_WorkList.xx_stop())
		{
			return 1; /* axis not in window */
		}
		i = yaxis_col + (yaxis_col - g_WorkList.xx_start());
		if (yaxis_between)
		{
			++i;
		}
		if (i > g_WorkList.xx_stop()) /* split into 2 pieces, right has the symmetry */
		{
			if (g_WorkList.num_items() >= MAX_WORK_LIST-1) /* no room to split */
			{
				return 1;
			}
			g_x_stop = yaxis_col - (g_WorkList.xx_stop() - yaxis_col);
			if (!yaxis_between)
			{
				--g_x_stop;
			}
			g_WorkList.add(g_x_stop + 1, g_WorkList.xx_stop(), g_x_stop + 1, g_WorkList.yy_start(), g_WorkList.yy_stop(), g_WorkList.yy_start(), g_work_pass, 0);
			g_WorkList.set_xx_stop(g_x_stop);
			return 1; /* tell set_symmetry no sym for current window */
		}
		if (i < g_WorkList.xx_stop()) /* split into 2 pieces, left has the symmetry */
		{
			if (g_WorkList.num_items() >= MAX_WORK_LIST-1) /* no room to split */
			{
				return 1;
			}
			g_WorkList.add(i + 1, g_WorkList.xx_stop(), i + 1, g_WorkList.yy_start(), g_WorkList.yy_stop(), g_WorkList.yy_start(), g_work_pass, 0);
			g_WorkList.set_xx_stop(i);
		}
		g_x_stop = yaxis_col;
		g_work_sym |= 2;
	}
	g_symmetry = SYMMETRY_NONE;
	return 0; /* tell set_symmetry its a go */
}

#ifdef _MSC_VER
#pragma optimize ("ea", off)
#endif

static void _fastcall set_symmetry(int symmetry, bool use_list) /* set up proper symmetrical plot functions */
{
	int i;
	int parmszero;
	int parmsnoreal;
	int parmsnoimag;
	int xaxis_row;
	int yaxis_col;         /* pixel number for origin */
	int xaxis_between = 0;
	int yaxis_between = 0; /* if axis between 2 pixels, not on one */
	int xaxis_on_screen = 0;
	int yaxis_on_screen = 0;
	double ftemp;
	bf_t bft1;
	int saved = 0;
	g_symmetry = SYMMETRY_X_AXIS;
	if (g_standard_calculation_mode == 's' || g_standard_calculation_mode == 'o')
	{
		return;
	}
	if (symmetry == SYMMETRY_NO_PLOT && g_force_symmetry == FORCESYMMETRY_NONE)
	{
		g_plot_color = plot_color_none;
		return;
	}
	/* NOTE: 16-bit potential disables symmetry */
	/* also any decomp= option and any inversion not about the origin */
	/* also any rotation other than 180deg and any off-axis stretch */
	if (g_bf_math)
	{
		if (cmp_bf(g_escape_time_state.m_grid_bf.x_min(), g_escape_time_state.m_grid_bf.x_3rd()) || cmp_bf(g_escape_time_state.m_grid_bf.y_min(), g_escape_time_state.m_grid_bf.y_3rd()))
		{
			return;
		}
	}
	if ((g_potential_flag && g_potential_16bit) || (g_invert && g_inversion[2] != 0.0)
			|| g_decomposition[0] != 0
			|| g_escape_time_state.m_grid_fp.x_min() != g_escape_time_state.m_grid_fp.x_3rd() || g_escape_time_state.m_grid_fp.y_min() != g_escape_time_state.m_grid_fp.y_3rd())
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
		g_force_symmetry = symmetry;  /* for backwards compatibility */
	}
	else if (g_outside == REAL || g_outside == IMAG || g_outside == MULT || g_outside == SUM
			|| g_outside == ATAN || g_bail_out_test == Manr || g_outside == FMOD)
	{
		return;
	}
	else if (g_inside == FMODI || g_outside == TDIS)
	{
		return;
	}
	parmszero = (g_parameter.x == 0.0 && g_parameter.y == 0.0 && g_use_initial_orbit_z != INITIALZ_ORBIT);
	parmsnoreal = (g_parameter.x == 0.0 && g_use_initial_orbit_z != INITIALZ_ORBIT);
	parmsnoimag = (g_parameter.y == 0.0 && g_use_initial_orbit_z != INITIALZ_ORBIT);
	switch (g_fractal_type)
	{
	case FRACTYPE_MANDELBROT_LAMBDA_FUNC_OR_FUNC_L:      /* These need only P1 checked. */
	case FRACTYPE_MANDELBROT_LAMBDA_FUNC_OR_FUNC_FP:     /* P2 is used for a switch value */
	case FRACTYPE_MANDELBROT_FUNC_OR_FUNC_L:         /* These have NOPARM set in fractalp.c, */
	case FRACTYPE_MANDELBROT_FUNC_OR_FUNC_FP:        /* but it only applies to P1. */
	case FRACTYPE_MANDELBROT_Z_POWER_FP:   /* or P2 is an exponent */
	case FRACTYPE_MANDELBROT_Z_POWER_L:
	case FRACTYPE_MANDELBROT_Z_TO_Z_PLUS_Z_POWER_FP:
	case FRACTYPE_MARKS_MANDELBROT:
	case FRACTYPE_MARKS_MANDELBROT_FP:
	case FRACTYPE_MARKS_JULIA:
	case FRACTYPE_MARKS_JULIA_FP:
		break;
	case FRACTYPE_FORMULA:  /* Check P2, P3, P4 and P5 */
	case FRACTYPE_FORMULA_FP:
		parmszero = (parmszero && g_parameters[2] == 0.0 && g_parameters[3] == 0.0
						&& g_parameters[4] == 0.0 && g_parameters[5] == 0.0
						&& g_parameters[6] == 0.0 && g_parameters[7] == 0.0
						&& g_parameters[8] == 0.0 && g_parameters[9] == 0.0);
		parmsnoreal = (parmsnoreal && g_parameters[2] == 0.0 && g_parameters[4] == 0.0
						&& g_parameters[6] == 0.0 && g_parameters[8] == 0.0);
		parmsnoimag = (parmsnoimag && g_parameters[3] == 0.0 && g_parameters[5] == 0.0
						&& g_parameters[7] == 0.0 && g_parameters[9] == 0.0);
		break;
	default:   /* Check P2 for the rest */
		parmszero = (parmszero && g_parameter2.x == 0.0 && g_parameter2.y == 0.0);
	}
	xaxis_row = yaxis_col = -1;
	if (g_bf_math)
	{
		saved = save_stack();
		bft1    = alloc_stack(rbflength + 2);
		xaxis_on_screen = (sign_bf(g_escape_time_state.m_grid_bf.y_min()) != sign_bf(g_escape_time_state.m_grid_bf.y_max()));
		yaxis_on_screen = (sign_bf(g_escape_time_state.m_grid_bf.x_min()) != sign_bf(g_escape_time_state.m_grid_bf.x_max()));
	}
	else
	{
		xaxis_on_screen = (sign(g_escape_time_state.m_grid_fp.y_min()) != sign(g_escape_time_state.m_grid_fp.y_max()));
		yaxis_on_screen = (sign(g_escape_time_state.m_grid_fp.x_min()) != sign(g_escape_time_state.m_grid_fp.x_max()));
	}
	if (xaxis_on_screen) /* axis is on screen */
	{
		if (g_bf_math)
		{
			/* ftemp = -g_yy_max/(g_yy_min-g_yy_max); */
			sub_bf(bft1, g_escape_time_state.m_grid_bf.y_min(), g_escape_time_state.m_grid_bf.y_max());
			div_bf(bft1, g_escape_time_state.m_grid_bf.y_max(), bft1);
			neg_a_bf(bft1);
			ftemp = (double)bftofloat(bft1);
		}
		else
		{
			ftemp = -g_escape_time_state.m_grid_fp.y_max()/(g_escape_time_state.m_grid_fp.y_min()-g_escape_time_state.m_grid_fp.y_max());
		}
		ftemp *= (g_y_dots-1);
		ftemp += 0.25;
		xaxis_row = (int)ftemp;
		xaxis_between = (ftemp - xaxis_row >= 0.5);
		if (!use_list && (!xaxis_between || (xaxis_row + 1)*2 != g_y_dots))
		{
			xaxis_row = -1; /* can't split screen, so dead center or not at all */
		}
	}
	if (yaxis_on_screen) /* axis is on screen */
	{
		if (g_bf_math)
		{
			/* ftemp = -g_xx_min/(g_xx_max-g_xx_min); */
			sub_bf(bft1, g_escape_time_state.m_grid_bf.x_max(), g_escape_time_state.m_grid_bf.x_min());
			div_bf(bft1, g_escape_time_state.m_grid_bf.x_min(), bft1);
			neg_a_bf(bft1);
			ftemp = (double)bftofloat(bft1);
		}
		else
		{
			ftemp = -g_escape_time_state.m_grid_fp.x_min()/g_escape_time_state.m_grid_fp.height();
		}
		ftemp *= (g_x_dots-1);
		ftemp += 0.25;
		yaxis_col = (int)ftemp;
		yaxis_between = (ftemp - yaxis_col >= 0.5);
		if (!use_list && (!yaxis_between || (yaxis_col + 1)*2 != g_x_dots))
		{
			yaxis_col = -1; /* can't split screen, so dead center or not at all */
		}
	}
	switch (symmetry)
	{
	case SYMMETRY_X_AXIS_NO_REAL:
		if (!parmsnoreal)
		{
			break;
		}
		goto xsym;
	case SYMMETRY_X_AXIS_NO_IMAGINARY:
		if (!parmsnoimag)
		{
			break;
		}
		goto xsym;
	case SYMMETRY_X_AXIS_NO_PARAMETER:
		if (!parmszero)
		{
			break;
		}
		xsym:
	case SYMMETRY_X_AXIS:
		if (x_symmetry_split(xaxis_row, xaxis_between) == 0)
		{
			g_plot_color = g_basin ? plot_color_symmetry_x_axis_basin : plot_color_symmetry_x_axis;
		}
		break;
	case SYMMETRY_Y_AXIS_NO_PARAMETER:
		if (!parmszero)
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
		if (!parmszero)
		{
			break;
		}
	case SYMMETRY_XY_AXIS:
		x_symmetry_split(xaxis_row, xaxis_between);
		y_symmetry_split(yaxis_col, yaxis_between);
		switch (g_work_sym & 3)
		{
		case SYMMETRY_X_AXIS: /* just xaxis symmetry */
			g_plot_color = g_basin ? plot_color_symmetry_x_axis_basin : plot_color_symmetry_x_axis;
			break;
		case SYMMETRY_Y_AXIS: /* just yaxis symmetry */
			if (g_basin) /* got no routine for this case */
			{
				g_x_stop = g_WorkList.xx_stop(); /* fix what split should not have done */
				g_symmetry = SYMMETRY_X_AXIS;
			}
			else
			{
				g_plot_color = plot_color_symmetry_y_axis;
			}
			break;
		case SYMMETRY_XY_AXIS:
			g_plot_color = g_basin ? plot_color_symmetry_xy_axis_basin : plot_color_symmetry_xy_axis;
		}
		break;
	case SYMMETRY_ORIGIN_NO_PARAMETER:
		if (!parmszero)
		{
			break;
		}
	case SYMMETRY_ORIGIN:
		originsym:
		if (x_symmetry_split(xaxis_row, xaxis_between) == 0
			&& y_symmetry_split(yaxis_col, yaxis_between) == 0)
		{
			g_plot_color = plot_color_symmetry_origin;
			g_x_stop = g_WorkList.xx_stop(); /* didn't want this changed */
		}
		else
		{
			g_y_stop = g_WorkList.yy_stop(); /* in case first split worked */
			g_symmetry = SYMMETRY_X_AXIS;
			g_work_sym = 0x30; /* let it recombine with others like it */
		}
		break;
	case SYMMETRY_PI_NO_PARAMETER:
		if (!parmszero)
		{
			break;
		}
	case SYMMETRY_PI:                      /* PI symmetry */
		if (g_bf_math)
		{
			if ((double)bftofloat(abs_a_bf(sub_bf(bft1, g_escape_time_state.m_grid_bf.x_max(), g_escape_time_state.m_grid_bf.x_min()))) < MathUtil::Pi/4)
			{
				break; /* no point in pi symmetry if values too close */
			}
		}
		else
		{
			if (fabs(g_escape_time_state.m_grid_fp.width()) < MathUtil::Pi/4)
			{
				break; /* no point in pi symmetry if values too close */
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
			/* both axes or origin*/
			g_plot_color = (g_parameter.y == 0.0) ? plot_color_symmetry_pi_xy_axis : plot_color_symmetry_pi_origin; 
		}
		else
		{
			g_y_stop = g_WorkList.yy_stop(); /* in case first split worked */
			g_work_sym = 0x30;  /* don't mark pisym as ysym, just do it unmarked */
		}
		if (g_bf_math)
		{
			sub_bf(bft1, g_escape_time_state.m_grid_bf.x_max(), g_escape_time_state.m_grid_bf.x_min());
			abs_a_bf(bft1);
			s_pixel_pi = (int) (MathUtil::Pi/(double) bftofloat(bft1)*g_x_dots); /* PI in pixels */
		}
		else
		{
			s_pixel_pi = (int) (MathUtil::Pi/fabs(g_escape_time_state.m_grid_fp.width())*g_x_dots); /* PI in pixels */
		}

		g_x_stop = g_WorkList.xx_start() + s_pixel_pi-1;
		if (g_x_stop > g_WorkList.xx_stop())
		{
			g_x_stop = g_WorkList.xx_stop();
		}
		if (g_plot_color == plot_color_symmetry_pi_xy_axis)
		{
			i = (g_WorkList.xx_start() + g_WorkList.xx_stop())/2;
			if (g_x_stop > i)
			{
				g_x_stop = i;
			}
		}
		break;
	default:                  /* no symmetry */
		break;
	}
	if (g_bf_math)
	{
		restore_stack(saved);
	}
}

#ifdef _MSC_VER
#pragma optimize ("ea", on)
#endif

/* added for testing automatic_log_map() */ /* CAE 9211 fixed missing comment */
/* insert at end of CALCFRAC.C */

static long automatic_log_map()   /*RB*/
{  /* calculate round screen edges to avoid wasted colours in logmap */
	long mincolour;
	int xstop = g_x_dots - 1; /* don't use symetry */
	int ystop = g_y_dots - 1; /* don't use symetry */
	long old_maxit;
	mincolour = LONG_MAX;
	g_row = 0;
	g_reset_periodicity = 0;
	old_maxit = g_max_iteration;
	for (g_col = 0; g_col < xstop; g_col++) /* top row */
	{
		g_color = (*g_calculate_type)();
		if (g_color == -1)
		{
			goto ack; /* key pressed, bailout */
		}
		if (g_real_color_iter < mincolour)
		{
			mincolour = g_real_color_iter;
			g_max_iteration = max(2, mincolour); /*speedup for when edges overlap lakes */
		}
		if (g_col >= 32)
		{
			(*g_plot_color)(g_col-32, g_row, 0);
		}
	}                                    /* these lines tidy up for BTM etc */
	for (int lag = 32; lag > 0; lag--)
	{
		(*g_plot_color)(g_col-lag, g_row, 0);
	}

	g_col = xstop;
	for (g_row = 0; g_row < ystop; g_row++) /* right  side */
	{
		g_color = (*g_calculate_type)();
		if (g_color == -1)
		{
			goto ack; /* key pressed, bailout */
		}
		if (g_real_color_iter < mincolour)
		{
			mincolour = g_real_color_iter;
			g_max_iteration = max(2, mincolour); /*speedup for when edges overlap lakes */
		}
		if (g_row >= 32)
		{
			(*g_plot_color)(g_col, g_row-32, 0);
		}
	}
	for (int lag = 32; lag > 0; lag--)
	{
		(*g_plot_color)(g_col, g_row-lag, 0);
	}

	g_col = 0;
	for (g_row = 0; g_row < ystop; g_row++) /* left  side */
	{
		g_color = (*g_calculate_type)();
		if (g_color == -1)
		{
			goto ack; /* key pressed, bailout */
		}
		if (g_real_color_iter < mincolour)
		{
			mincolour = g_real_color_iter;
			g_max_iteration = max(2, mincolour); /*speedup for when edges overlap lakes */
		}
		if (g_row >= 32)
		{
			(*g_plot_color)(g_col, g_row-32, 0);
		}
	}
	for (int lag = 32; lag > 0; lag--)
	{
		(*g_plot_color)(g_col, g_row-lag, 0);
	}

	g_row = ystop;
	for (g_col = 0; g_col < xstop; g_col++) /* bottom row */
	{
		g_color = (*g_calculate_type)();
		if (g_color == -1)
		{
			goto ack; /* key pressed, bailout */
		}
		if (g_real_color_iter < mincolour)
		{
			mincolour = g_real_color_iter;
			g_max_iteration = max(2, mincolour); /*speedup for when edges overlap lakes */
		}
		if (g_col >= 32)
		{
			(*g_plot_color)(g_col-32, g_row, 0);
		}
	}
	for (int lag = 32; lag > 0; lag--)
	{
		(*g_plot_color)(g_col-lag, g_row, 0);
	}

ack: /* bailout here if key is pressed */
	if (mincolour == 2)
	{
		g_resuming = true; /* insure automatic_log_map not called again */
	}
	g_max_iteration = old_maxit;

	return mincolour;
}

/* Symmetry plot for period PI */
static void _fastcall plot_color_symmetry_pi(int x, int y, int color)
{
	while (x <= g_WorkList.xx_stop())
	{
		g_plot_color_put_color(x, y, color);
		x += s_pixel_pi;
	}
}
/* Symmetry plot for period PI plus Origin Symmetry */
static void _fastcall plot_color_symmetry_pi_origin(int x, int y, int color)
{
	int i;
	int j;
	while (x <= g_WorkList.xx_stop())
	{
		g_plot_color_put_color(x, y, color);
		i = g_WorkList.yy_stop() - (y - g_WorkList.yy_start());
		if (i > g_y_stop && i < g_y_dots)
		{
			j = g_WorkList.xx_stop() - (x - g_WorkList.xx_start());
			if (j < g_x_dots)
			{
				g_plot_color_put_color(j, i, color);
			}
		}
		x += s_pixel_pi;
	}
}
/* Symmetry plot for period PI plus Both Axis Symmetry */
static void _fastcall plot_color_symmetry_pi_xy_axis(int x, int y, int color)
{
	int i;
	int j;
	while (x <= (g_WorkList.xx_start() + g_WorkList.xx_stop())/2)
	{
		j = g_WorkList.xx_stop()-(x-g_WorkList.xx_start());
		g_plot_color_put_color(x , y , color);
		if (j < g_x_dots)
		{
			g_plot_color_put_color(j , y , color);
		}
		i = g_WorkList.yy_stop()-(y-g_WorkList.yy_start());
		if (i > g_y_stop && i < g_y_dots)
		{
			g_plot_color_put_color(x , i , color);
			if (j < g_x_dots)
			{
				g_plot_color_put_color(j , i , color);
			}
		}
		x += s_pixel_pi;
	}
}

/* Symmetry plot for X Axis Symmetry */
void _fastcall plot_color_symmetry_x_axis(int x, int y, int color)
{
	int i;
	g_plot_color_put_color(x, y, color);
	i = g_WorkList.yy_stop()-(y-g_WorkList.yy_start());
	if (i > g_y_stop && i < g_y_dots)
	{
		g_plot_color_put_color(x, i, color);
	}
}

/* Symmetry plot for Y Axis Symmetry */
static void _fastcall plot_color_symmetry_y_axis(int x, int y, int color)
{
	int i;
	g_plot_color_put_color(x, y, color);
	i = g_WorkList.xx_stop()-(x-g_WorkList.xx_start());
	if (i < g_x_dots)
	{
		g_plot_color_put_color(i, y, color);
	}
}

/* Symmetry plot for Origin Symmetry */
void _fastcall plot_color_symmetry_origin(int x, int y, int color)
{
	int i;
	g_plot_color_put_color(x, y, color);
	i = g_WorkList.yy_stop()-(y-g_WorkList.yy_start());
	if (i > g_y_stop && i < g_y_dots)
	{
		int j = g_WorkList.xx_stop()-(x-g_WorkList.xx_start());
		if (j < g_x_dots)
		{
			g_plot_color_put_color(j, i, color);
		}
	}
}

/* Symmetry plot for Both Axis Symmetry */
static void _fastcall plot_color_symmetry_xy_axis(int x, int y, int color)
{
	int i;
	int j;
	j = g_WorkList.xx_stop()-(x-g_WorkList.xx_start());
	g_plot_color_put_color(x , y, color);
	if (j < g_x_dots)
	{
		g_plot_color_put_color(j , y, color);
	}
	i = g_WorkList.yy_stop()-(y-g_WorkList.yy_start());
	if (i > g_y_stop && i < g_y_dots)
	{
		g_plot_color_put_color(x , i, color);
		if (j < g_x_dots)
		{
			g_plot_color_put_color(j , i, color);
		}
	}
}

/* Symmetry plot for X Axis Symmetry - Striped Newtbasin version */
static void _fastcall plot_color_symmetry_x_axis_basin(int x, int y, int color)
{
	int i;
	int stripe;
	g_plot_color_put_color(x, y, color);
	stripe = (g_basin == 2 && color > 8) ? 8 : 0;
	i = g_WorkList.yy_stop()-(y-g_WorkList.yy_start());
	if (i > g_y_stop && i < g_y_dots)
	{
		color -= stripe;                    /* reconstruct unstriped color */
		color = (g_degree + 1-color) % g_degree + 1;  /* symmetrical color */
		color += stripe;                    /* add stripe */
		g_plot_color_put_color(x, i, color);
	}
}

/* Symmetry plot for Both Axis Symmetry  - Newtbasin version */
static void _fastcall plot_color_symmetry_xy_axis_basin(int x, int y, int color)
{
	int i;
	int j;
	int color1;
	int stripe;
	if (color == 0) /* assumed to be "inside" color */
	{
		plot_color_symmetry_xy_axis(x, y, color);
		return;
	}
	stripe = (g_basin == 2 && color > 8) ? 8 : 0;
	color -= stripe;               /* reconstruct unstriped color */
	color1 = (color < g_degree/2 + 2) ?
		(g_degree/2 + 2 - color) : (g_degree/2 + g_degree + 2 - color);
	j = g_WorkList.xx_stop()-(x-g_WorkList.xx_start());
	g_plot_color_put_color(x, y, color + stripe);
	if (j < g_x_dots)
	{
		g_plot_color_put_color(j, y, color1 + stripe);
	}
	i = g_WorkList.yy_stop()-(y-g_WorkList.yy_start());
	if (i > g_y_stop && i < g_y_dots)
	{
		g_plot_color_put_color(x, i, stripe + (g_degree + 1 - color) % g_degree + 1);
		if (j < g_x_dots)
		{
			g_plot_color_put_color(j, i, stripe + (g_degree + 1 - color1) % g_degree + 1);
		}
	}
}

static void _fastcall put_truecolor_disk(int x, int y, int color)
{
	putcolor_a(x, y, color);
	targa_color(x, y, color);
}

/* Do nothing plot!!! */
#ifdef __CLINT__
#pragma argsused
#endif

void _fastcall plot_color_none(int x, int y, int color)
{
	x = y = color = 0;  /* just for warning */
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

	int (*m_save_orbit_calc)();  /* function that calculates one orbit */
	int (*m_save_per_pixel)();  /* once-per-pixel init */
	int (*m_save_per_image)();  /* once-per-image setup */
};

PerformWorkList::PerformWorkList()
	: m_save_orbit_calc(NULL),
	m_save_per_pixel(NULL),
	m_save_per_image(NULL)
{
}

PerformWorkList::~PerformWorkList()
{
}

void PerformWorkList::setup_alternate_math()
{
	m_save_orbit_calc = NULL;  /* function that calculates one orbit */
	m_save_per_pixel = NULL;  /* once-per-pixel init */
	m_save_per_image = NULL;  /* once-per-image setup */
	alternate_math *alt = find_alternate_math(g_bf_math);
	if (alt != NULL)
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
	if (m_save_orbit_calc != NULL)
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
		char tmpcalcmode = g_standard_calculation_mode;

		g_standard_calculation_mode = '1'; /* force 1 pass */
		if (!g_resuming)
		{
			if (disk_start_potential() < 0)
			{
				g_potential_16bit = false;       /* disk_start failed or cancelled */
				g_standard_calculation_mode = tmpcalcmode;    /* maybe we can carry on??? */
			}
		}
	}
}

void PerformWorkList::setup_standard_calculation_mode()
{
	if (g_standard_calculation_mode == 'b' && (g_current_fractal_specific->flags & FRACTALFLAG_NO_BOUNDARY_TRACING))
	{
		g_standard_calculation_mode = '1';
	}
	if (g_standard_calculation_mode == 'g' && (g_current_fractal_specific->flags & FRACTALFLAG_NO_SOLID_GUESSING))
	{
		g_standard_calculation_mode = '1';
	}
	if (g_standard_calculation_mode == 'o' && (g_current_fractal_specific->calculate_type != standard_fractal))
	{
		g_standard_calculation_mode = '1';
	}
}

void PerformWorkList::setup_initial_work_list()
{
	g_WorkList.setup_initial_work_list();
}

void PerformWorkList::setup_distance_estimator()
{
	double dxsize;
	double dysize;
	double aspect;
	if (g_pseudo_x && g_pseudo_y)
	{
		aspect = (double) g_pseudo_y/(double) g_pseudo_x;
		dxsize = g_pseudo_x-1;
		dysize = g_pseudo_y-1;
	}
	else
	{
		aspect = (double) g_y_dots/(double) g_x_dots;
		dxsize = g_x_dots-1;
		dysize = g_y_dots-1;
	}

	double delta_x_fp = (g_escape_time_state.m_grid_fp.x_max() - g_escape_time_state.m_grid_fp.x_3rd())/dxsize; /* calculate stepsizes */
	double delta_y_fp = (g_escape_time_state.m_grid_fp.y_max() - g_escape_time_state.m_grid_fp.y_3rd())/dysize;
	double delta_x2_fp = (g_escape_time_state.m_grid_fp.x_3rd() - g_escape_time_state.m_grid_fp.x_min())/dysize;
	double delta_y2_fp = (g_escape_time_state.m_grid_fp.y_3rd() - g_escape_time_state.m_grid_fp.y_min())/dxsize;

	/* in case it's changed with <G> */
	g_use_old_distance_test = (g_save_release < 1827) ? 1 : 0;

	g_rq_limit = s_rq_limit_save; /* just in case changed to DEM_BAILOUT earlier */
	if (g_distance_test != 1 || g_colors == 2) /* not doing regular outside colors */
	{
		if (g_rq_limit < DEM_BAILOUT)         /* so go straight for dem bailout */
		{
			g_rq_limit = DEM_BAILOUT;
		}
	}
	/* must be mandel type, formula, or old PAR/GIF */
	s_dem_mandelbrot =
		(g_current_fractal_specific->tojulia != FRACTYPE_NO_FRACTAL
		|| g_use_old_distance_test
		|| g_fractal_type == FRACTYPE_FORMULA
		|| g_fractal_type == FRACTYPE_FORMULA_FP);

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
	/* multiply by thickness desired */
	s_dem_delta *= (g_distance_test_width > 0) ? sqr(ftemp)/10000 : 1/(sqr(ftemp)*10000);
	s_dem_width = (sqrt(sqr(g_escape_time_state.m_grid_fp.width()) + sqr(g_escape_time_state.m_grid_fp.x_3rd()-g_escape_time_state.m_grid_fp.x_min()) )*aspect
		+ sqrt(sqr(g_escape_time_state.m_grid_fp.height()) + sqr(g_escape_time_state.m_grid_fp.y_3rd()-g_escape_time_state.m_grid_fp.y_min()) ) )/g_distance_test;
	ftemp = (g_rq_limit < DEM_BAILOUT) ? DEM_BAILOUT : g_rq_limit;
	ftemp += 3; /* bailout plus just a bit */
	double ftemp2 = log(ftemp);
	s_dem_too_big = g_use_old_distance_test ?
		sqr(ftemp)*sqr(ftemp2)*4/s_dem_delta : fabs(ftemp)*fabs(ftemp2)*2/sqrt(s_dem_delta);
}

void PerformWorkList::setup_per_image()
{
	/* per_image can override */
	g_calculate_type = g_current_fractal_specific->calculate_type;
	g_symmetry = g_current_fractal_specific->symmetry; /*   calctype & symmetry  */
	g_plot_color = g_plot_color_put_color; /* defaults when setsymmetry not called or does nothing */
}

void PerformWorkList::get_top_work_list_item()
{
	g_WorkList.get_top_item();
}

void PerformWorkList::show_dot_start()
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
		double dshowdot_width = (double) g_size_dot*g_x_dots/1024.0;
		/*
			Arbitrary sanity limit, however s_show_dot_width will
			overflow if dshowdot width gets near 256.
		*/
		if (dshowdot_width > 150.0)
		{
			s_show_dot_width = 150;
		}
		else if (dshowdot_width > 0.0)
		{
			s_show_dot_width = (int) dshowdot_width;
		}
		else
		{
			s_show_dot_width = -1;
		}
	}
	while (s_show_dot_width >= 0)
	{
		/*
		We're using near memory, so get the amount down
		to something reasonable. The polynomial used to
		calculate s_save_dots_len is exactly right for the
		triangular-shaped shotdot cursor. The that cursor
		is changed, this formula must match.
		*/
		while ((s_save_dots_len = sqr(s_show_dot_width) + 5*s_show_dot_width + 4) > 1000)
		{
			s_show_dot_width--;
		}
		s_save_dots = (BYTE *)malloc(s_save_dots_len);
		if (s_save_dots != NULL)
		{
			s_save_dots_len /= 2;
			s_fill_buffer = s_save_dots + s_save_dots_len;
			memset(s_fill_buffer, s_show_dot_color, s_save_dots_len);
			break;
		}
		/*
		There's even less free memory than we thought, so reduce
		s_show_dot_width still more
		*/
		s_show_dot_width--;
	}
	if (s_save_dots == NULL)
	{
		s_show_dot_width = -1;
	}
	g_calculate_type_temp = g_calculate_type;
	g_calculate_type    = calculate_type_show_dot;
}

void PerformWorkList::show_dot_finish()
{
	if (s_save_dots != NULL)
	{
		free(s_save_dots);
		s_save_dots = NULL;
		s_fill_buffer = NULL;
	}
}

void PerformWorkList::common_escape_time_initialization()
{
	/* some common initialization for escape-time pixel level routines */
	g_close_enough = g_delta_min_fp*pow(2.0, (double) -abs(g_periodicity_check));
	g_close_enough_l = (long) (g_close_enough*g_fudge); /* "close enough" value */
	g_input_counter = g_max_input_counter;

	set_symmetry(g_symmetry, true);

	if (!g_resuming && (labs(g_log_palette_mode) == 2 || (g_log_palette_mode && g_log_automatic_flag)))
	{
		/* calculate round screen edges to work out best start for logmap */
		g_log_palette_mode = (automatic_log_map()*(g_log_palette_mode/labs(g_log_palette_mode)));
		SetupLogTable();
	}
}
void PerformWorkList::call_escape_time_engine()
{
	/* call the appropriate escape-time engine */
	switch (g_standard_calculation_mode)
	{
	case 's':
		soi();
		break;
	case 't':
		tesseral();
		break;
	case 'b':
		boundary_trace_main();
		break;
	case 'g':
		solid_guess();
		break;
	case 'd':
		diffusion_scan();
		break;
	case 'o':
		draw_orbits();
		break;
	default:
		one_or_two_pass();
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
		g_calculation_status = CALCSTAT_COMPLETED; /* completed */
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
		g_calculation_status = CALCSTAT_IN_PROGRESS;

		g_current_fractal_specific->per_image();
		if (g_show_dot >= 0)
		{
			show_dot_start();
		}

		common_escape_time_initialization();
		call_escape_time_engine();
		show_dot_finish();
		if (check_key()) /* interrupted? */
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
