#include <string>

#include "port.h"
#include "prototyp.h"
#include "helpdefs.h"
#include "fractype.h"
#include "drivers.h"
#include "fracsubr.h"
#include "fractals.h"
#include "jb.h"
#include "loadmap.h"
#include "prompts1.h"
#include "prompts2.h"
#include "realdos.h"
#include "StopMessage.h"

#include "EscapeTime.h"
#include "QuaternionEngine.h"

// these need to be accessed elsewhere for saving data
double g_m_x_min_fp = -.83;
double g_m_y_min_fp = -.25;
double g_m_x_max_fp = -.83;
double g_m_y_max_fp =  .25;
int g_z_dots = 128;
float g_origin_fp = 8.0f;
float g_height_fp = 7.0f;
float g_width_fp = 10.0f;
float g_screen_distance_fp = 24.0f;
float g_eyes_fp = 2.5f;
float g_depth_fp = 8.0f;
int g_juli_3d_mode = JULI3DMODE_MONOCULAR;
int g_new_orbit_type = FRACTYPE_JULIA;

template <typename T>
struct PerspectiveT
{
	T x;
	T y;
	T zx;
	T zy;
};

typedef PerspectiveT<long> Perspective;
typedef PerspectiveT<double> Perspective_fp;

static long s_m_x_min;
static long s_m_y_min;
static long s_x_per_inch;
static long s_y_per_inch;
static long s_inch_per_x_dot;
static long s_inch_per_y_dot;
static double s_x_per_inch_fp;
static double s_y_per_inch_fp;
static double s_inch_per_x_dot_fp;
static double s_inch_per_y_dot_fp;
static int s_b_base;
static long s_x_pixel;
static long s_y_pixel;
static double s_x_pixel_fp;
static double s_y_pixel_fp;
static long s_init_z;
static long s_djx;
static long s_djy;
static long s_dmx;
static long s_dmy;
static double s_init_z_fp;
static double s_djx_fp;
static double s_djy_fp;
static double s_dmx_fp;
static double s_dmy_fp;
static long s_jx;
static long s_jy;
static long s_mx;
static long s_my;
static long s_x_offset;
static long s_y_offset;
static double s_jx_fp;
static double s_jy_fp;
static double s_mx_fp;
static double s_my_fp;
static double s_x_offset_fp;
static double s_y_offset_fp;
static Perspective s_left_eye;
static Perspective s_right_eye;
static Perspective *s_per;
static Perspective_fp s_left_eye_fp;
static Perspective_fp s_right_eye_fp;
static Perspective_fp *s_per_fp;
static ComplexL s_jbc;
static ComplexD s_jbc_fp;
static long s_width;
static long s_dist;
static long s_depth;
static long s_br_ratio;
#ifndef XFRACT
static double s_fg;
static double s_fg16;
static long s_eyes;
#endif

bool julibrot_setup()
{
#ifndef NO_FIXED_POINT_MATH
	long origin;
#endif
	int r = 0;
	s_x_offset_fp = g_escape_time_state.m_grid_fp.x_center();     // Calculate average
	s_y_offset_fp = g_escape_time_state.m_grid_fp.y_center();     // Calculate average
	s_dmx_fp = (g_m_x_max_fp - g_m_x_min_fp)/g_z_dots;
	s_dmy_fp = (g_m_y_max_fp - g_m_y_min_fp)/g_z_dots;
	g_float_parameter = &s_jbc_fp;
	s_x_per_inch_fp = -g_escape_time_state.m_grid_fp.width()/g_width_fp;
	s_y_per_inch_fp = g_escape_time_state.m_grid_fp.height()/g_height_fp;
	s_inch_per_x_dot_fp = g_width_fp/g_x_dots;
	s_inch_per_y_dot_fp = g_height_fp/g_y_dots;
	s_init_z_fp = g_origin_fp - (g_depth_fp/2);
	s_right_eye_fp.x = (g_juli_3d_mode == JULI3DMODE_MONOCULAR) ? 0.0 : (g_eyes_fp/2);
	s_left_eye_fp.x = -s_right_eye_fp.x;
	s_left_eye_fp.y = 0;
	s_right_eye_fp.y = 0;
	s_left_eye_fp.zx = g_screen_distance_fp;
	s_right_eye_fp.zx = g_screen_distance_fp;
	s_left_eye_fp.zy = g_screen_distance_fp;
	s_right_eye_fp.zy = g_screen_distance_fp;
	s_b_base = 128;

#ifndef NO_FIXED_POINT_MATH
	if (g_fractal_specific[g_fractal_type].isinteger > 0)
	{
		long jxmin;
		long jxmax;
		long jymin;
		long jymax;
		long mxmax;
		long mymax;
		if (g_fractal_specific[g_new_orbit_type].isinteger == 0)
		{
			stop_message(STOPMSG_NORMAL, "Julibrot orbit type isinteger mismatch");
		}
		if (g_fractal_specific[g_new_orbit_type].isinteger > 1)
		{
			g_bit_shift = g_fractal_specific[g_new_orbit_type].isinteger;
		}
		s_fg = double(1L << g_bit_shift);
		s_fg16 = double(1L << 16);
		jxmin = long(g_escape_time_state.m_grid_fp.x_min()*s_fg);
		jxmax = long(g_escape_time_state.m_grid_fp.x_max()*s_fg);
		s_x_offset = (jxmax + jxmin)/2;    // Calculate average
		jymin = long(g_escape_time_state.m_grid_fp.y_min()*s_fg);
		jymax = long(g_escape_time_state.m_grid_fp.y_max()*s_fg);
		s_y_offset = (jymax + jymin)/2;    // Calculate average
		s_m_x_min = long(g_m_x_min_fp*s_fg);
		mxmax = long(g_m_x_max_fp*s_fg);
		s_m_y_min = long(g_m_y_min_fp*s_fg);
		mymax = long(g_m_y_max_fp*s_fg);
		origin = long(g_origin_fp*s_fg16);
		s_depth = long(g_depth_fp*s_fg16);
		s_width = long(g_width_fp*s_fg16);
		s_dist = long(g_screen_distance_fp*s_fg16);
		s_eyes = long(g_eyes_fp*s_fg16);
		s_br_ratio = long(s_fg16);
		s_dmx = (mxmax - s_m_x_min)/g_z_dots;
		s_dmy = (mymax - s_m_y_min)/g_z_dots;
		g_long_parameter = &s_jbc;

		s_x_per_inch = long(-g_escape_time_state.m_grid_fp.width()/g_width_fp*s_fg);
		s_y_per_inch = long(g_escape_time_state.m_grid_fp.height()/g_height_fp*s_fg);
		s_inch_per_x_dot = long((g_width_fp/g_x_dots)*s_fg16);
		s_inch_per_y_dot = long((g_height_fp/g_y_dots)*s_fg16);
		s_init_z = origin - (s_depth/2);
		s_right_eye.x = (g_juli_3d_mode == JULI3DMODE_MONOCULAR) ? 0L : (s_eyes/2);
		s_left_eye.x = -s_right_eye.x;
		s_left_eye.y = 0l;
		s_right_eye.y = 0l;
		s_left_eye.zx = s_dist;
		s_right_eye.zx = s_dist;
		s_left_eye.zy = s_dist;
		s_right_eye.zy = s_dist;
		s_b_base = 128;
	}
#endif

	std::string map_name;
	if (g_juli_3d_mode == JULI3DMODE_RED_BLUE)
	{
		g_.SetSaveDAC(SAVEDAC_NO);
		map_name = GLASSES1_MAP;
	}
	else
	{
		map_name = GREY_MAP;
	}
	if (g_.SaveDAC() != SAVEDAC_YES)
	{
		if (validate_luts(map_name))
		{
			return 0;
		}
		load_dac();
		if (g_.SaveDAC() == SAVEDAC_NEXT)
		{
			g_.SetSaveDAC(SAVEDAC_YES);
		}
	}
	return r >= 0;
}


int julibrot_per_pixel()
{
	s_jx = multiply(s_per->x - s_x_pixel, s_init_z, 16);
	s_jx = divide(s_jx, s_dist, 16) - s_x_pixel;
	s_jx = multiply(s_jx << (g_bit_shift - 16), s_x_per_inch, g_bit_shift);
	s_jx += s_x_offset;
	s_djx = divide(s_depth, s_dist, 16);
	s_djx = multiply(s_djx, s_per->x - s_x_pixel, 16) << (g_bit_shift - 16);
	s_djx = multiply(s_djx, s_x_per_inch, g_bit_shift)/g_z_dots;

	s_jy = multiply(s_per->y - s_y_pixel, s_init_z, 16);
	s_jy = divide(s_jy, s_dist, 16) - s_y_pixel;
	s_jy = multiply(s_jy << (g_bit_shift - 16), s_y_per_inch, g_bit_shift);
	s_jy += s_y_offset;
	s_djy = divide(s_depth, s_dist, 16);
	s_djy = multiply(s_djy, s_per->y - s_y_pixel, 16) << (g_bit_shift - 16);
	s_djy = multiply(s_djy, s_y_per_inch, g_bit_shift)/g_z_dots;

	return 1;
}

int julibrot_per_pixel_fp()
{
	s_jx_fp = ((s_per_fp->x - s_x_pixel_fp)*s_init_z_fp/g_screen_distance_fp - s_x_pixel_fp)*s_x_per_inch_fp;
	s_jx_fp += s_x_offset_fp;
	s_djx_fp = (g_depth_fp/g_screen_distance_fp)*(s_per_fp->x - s_x_pixel_fp)*s_x_per_inch_fp/g_z_dots;

	s_jy_fp = ((s_per_fp->y - s_y_pixel_fp)*s_init_z_fp/g_screen_distance_fp - s_y_pixel_fp)*s_y_per_inch_fp;
	s_jy_fp += s_y_offset_fp;
	s_djy_fp = g_depth_fp/g_screen_distance_fp*(s_per_fp->y - s_y_pixel_fp)*s_y_per_inch_fp/g_z_dots;

	return 1;
}

static bool s_plotted;

static int z_line(long x, long y)
{
	int n;
	int z_pixel;

	s_x_pixel = x;
	s_y_pixel = y;
	s_mx = s_m_x_min;
	s_my = s_m_y_min;
	switch (g_juli_3d_mode)
	{
	case JULI3DMODE_MONOCULAR:
	case JULI3DMODE_LEFT_EYE:
		s_per = &s_left_eye;
		break;
	case JULI3DMODE_RIGHT_EYE:
		s_per = &s_right_eye;
		break;
	case JULI3DMODE_RED_BLUE:
		s_per = ((g_row + g_col) & 1) ? &s_left_eye : &s_right_eye;
		break;
	}
	julibrot_per_pixel();
	for (z_pixel = 0; z_pixel < g_z_dots; z_pixel++)
	{
		g_old_z_l.real(s_jx);
		g_old_z_l.imag(s_jy);
		s_jbc.real(s_mx);
		s_jbc.imag(s_my);
		if (driver_key_pressed())
		{
			return -1;
		}
		g_temp_sqr_l.real(multiply(g_old_z_l.real(), g_old_z_l.real(), g_bit_shift));
		g_temp_sqr_l.imag(multiply(g_old_z_l.imag(), g_old_z_l.imag(), g_bit_shift));
		for (n = 0; n < g_max_iteration; n++)
		{
			if (g_fractal_specific[g_new_orbit_type].orbitcalc())
			{
				break;
			}
		}
		if (n == g_max_iteration)
		{
			if (g_juli_3d_mode == JULI3DMODE_RED_BLUE)
			{
				g_color = int(128l*z_pixel/g_z_dots);
				if ((g_row + g_col) & 1)
				{

					g_plot_color(g_col, g_row, 127 - g_color);
				}
				else
				{
					g_color = int(multiply(long(g_color) << 16, s_br_ratio, 16) >> 16);
					if (g_color < 1)
					{
						g_color = 1;
					}
					if (g_color > 127)
					{
						g_color = 127;
					}
					g_plot_color(g_col, g_row, 127 + s_b_base - g_color);
				}
			}
			else
			{
				g_color = int(254l*z_pixel/g_z_dots);
				g_plot_color(g_col, g_row, g_color + 1);
			}
			s_plotted = true;
			break;
		}
		s_mx += s_dmx;
		s_my += s_dmy;
		s_jx += s_djx;
		s_jy += s_djy;
	}
	return 0;
}

static int z_line_fp(double x, double y)
{
	int n;
	int z_pixel;
#ifdef XFRACT
	static int keychk = 0;
#endif
	s_x_pixel_fp = x;
	s_y_pixel_fp = y;
	s_mx_fp = g_m_x_min_fp;
	s_my_fp = g_m_y_min_fp;
	switch (g_juli_3d_mode)
	{
	case JULI3DMODE_MONOCULAR:
	case JULI3DMODE_LEFT_EYE:
		s_per_fp = &s_left_eye_fp;
		break;
	case JULI3DMODE_RIGHT_EYE:
		s_per_fp = &s_right_eye_fp;
		break;
	case JULI3DMODE_RED_BLUE:
		s_per_fp = ((g_row + g_col) & 1) ? &s_left_eye_fp : &s_right_eye_fp;
		break;
	}
	julibrot_per_pixel_fp();
	for (z_pixel = 0; z_pixel < g_z_dots; z_pixel++)
	{
		// Special initialization for Mandelbrot types
		if (g_new_orbit_type == FRACTYPE_QUATERNION_FP || g_new_orbit_type == FRACTYPE_HYPERCOMPLEX_FP)
		{
			g_old_z.real(0.0);
			g_old_z.imag(0.0);
			s_jbc_fp.real(0.0);
			s_jbc_fp.imag(0.0);
			g_c_quaternion = QuaternionD(s_jx_fp, s_jy_fp, s_mx_fp, s_my_fp);
		}
#ifdef XFRACT
		if (keychk++ > 500)
		{
			keychk = 0;
			if (driver_key_pressed())
			{
				return -1;
			}
		}
#else
		if (driver_key_pressed())
		{
			return -1;
		}
#endif
		g_temp_sqr.real(sqr(g_old_z.real()));
		g_temp_sqr.imag(sqr(g_old_z.imag()));

		for (n = 0; n < g_max_iteration; n++)
		{
			if (g_fractal_specific[g_new_orbit_type].orbitcalc())
			{
				break;
			}
		}
		if (n == g_max_iteration)
		{
			if (g_juli_3d_mode == 3)
			{
				g_color = int(128l*z_pixel/g_z_dots);
				if ((g_row + g_col) & 1)
				{
					g_plot_color(g_col, g_row, 127 - g_color);
				}
				else
				{
					if (g_color < 1)
					{
						g_color = 1;
					}
					if (g_color > 127)
					{
						g_color = 127;
					}
					g_plot_color(g_col, g_row, 127 + s_b_base - g_color);
				}
			}
			else
			{
				g_color = int(254l*z_pixel/g_z_dots);
				g_plot_color(g_col, g_row, g_color + 1);
			}
			s_plotted = true;
			break;
		}
		s_mx_fp += s_dmx_fp;
		s_my_fp += s_dmy_fp;
		s_jx_fp += s_djx_fp;
		s_jy_fp += s_djy_fp;
	}
	return 0;
}

void standard_4d_fractal_set_orbit_calc(
	int (*orbit_function)(void), int (*complex_orbit_function)(void))
{
	// TODO: do not write to g_fractal_specific
	g_fractal_specific[g_new_orbit_type].orbitcalc =
			(g_parameters[P2_IMAG] == 0.0
			&& g_debug_mode != DEBUGMODE_UNOPT_POWER
			&& double(g_c_exp) == g_parameters[P2_REAL])
		? orbit_function : complex_orbit_function;
}

int standard_4d_fractal()
{
	g_c_exp = int(g_parameters[P2_REAL]);
	if (g_new_orbit_type == FRACTYPE_JULIA_Z_POWER_L)
	{
		if (g_c_exp < 1)
		{
			g_c_exp = 1;
		}
		standard_4d_fractal_set_orbit_calc(z_power_orbit, complex_z_power_orbit);
	}

	long y = 0;
	for (int ydot = (g_y_dots >> 1) - 1; ydot >= 0; ydot--, y -= s_inch_per_y_dot)
	{
		s_plotted = false;
		long x = -(s_width >> 1);
		for (int xdot = 0; xdot < g_x_dots; xdot++, x += s_inch_per_x_dot)
		{
			g_col = xdot;
			g_row = ydot;
			if (z_line(x, y) < 0)
			{
				return -1;
			}
			g_col = g_x_dots - g_col - 1;
			g_row = g_y_dots - g_row - 1;
			if (z_line(-x, -y) < 0)
			{
				return -1;
			}
		}
		if (!s_plotted)
		{
			if (y == 0)
			{
				s_plotted = true;  // no points first pass; don't give up
			}
			else
			{
				break;
			}
		}
	}
	return 0;
}

int standard_4d_fractal_fp()
{
	g_c_exp = int(g_parameters[P2_REAL]);

	if (g_new_orbit_type == FRACTYPE_JULIA_Z_POWER_FP)
	{
		if (g_c_exp < 1)
		{
			g_c_exp = 1;
		}
		standard_4d_fractal_set_orbit_calc(z_power_orbit_fp, complex_z_power_orbit_fp);
		get_julia_attractor (g_parameters[P1_REAL], g_parameters[P1_IMAG]); // another attractor?
	}

	double y = 0;
	for (int ydot = (g_y_dots >> 1) - 1; ydot >= 0; ydot--, y -= s_inch_per_y_dot_fp)
	{
		s_plotted = false;
		double x = -g_width_fp/2;
		for (int xdot = 0; xdot < g_x_dots; xdot++, x += s_inch_per_x_dot_fp)
		{
			g_col = xdot;
			g_row = ydot;
			if (z_line_fp(x, y) < 0)
			{
				return -1;
			}
			g_col = g_x_dots - g_col - 1;
			g_row = g_y_dots - g_row - 1;
			if (z_line_fp(-x, -y) < 0)
			{
				return -1;
			}
		}
		if (!s_plotted)
		{
			if (y == 0)
			{
				s_plotted = true;  // no points first pass; don't give up
			}
			else
			{
				break;
			}
		}
	}
	return 0;
}
