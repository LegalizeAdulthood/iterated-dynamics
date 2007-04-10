/* see Fractint.c for a description of the "include"  hierarchy */
#include "port.h"
#include "prototyp.h"
#include "helpdefs.h"
#include "fractype.h"
#include "drivers.h"

/* these need to be accessed elsewhere for saving data */
double g_m_x_min_fp = -.83;
double g_m_y_min_fp = -.25;
double g_m_x_max_fp = -.83;
double g_m_y_max_fp =  .25;
int g_z_dots = 128;
float g_origin_fp  = 8.0f;
float g_height_fp  = 7.0f;
float g_width_fp   = 10.0f;
float g_screen_distance_fp    = 24.0f;
float g_eyes_fp    = 2.5f;
float g_depth_fp   = 8.0f;
int g_juli_3d_mode = JULI3DMODE_MONOCULAR;
int g_new_orbit_type = JULIA;

struct perspective
{
	long x, y, zx, zy;
};

struct perspective_fp
{
	double x, y, zx, zy;
};

static long s_m_x_min, s_m_y_min;
static long s_x_per_inch, s_y_per_inch, s_inch_per_x_dot, s_inch_per_y_dot;
static double s_x_per_inch_fp, s_y_per_inch_fp, s_inch_per_x_dot_fp, s_inch_per_y_dot_fp;
static int s_b_base;
static long s_x_pixel, s_y_pixel;
static double s_x_pixel_fp, s_y_pixel_fp;
static long s_init_z, s_djx, s_djy, s_dmx, s_dmy;
static double s_init_z_fp, s_djx_fp, s_djy_fp, s_dmx_fp, s_dmy_fp;
static long s_jx, s_jy, s_mx, s_my, s_x_offset, s_y_offset;
static double s_jx_fp, s_jy_fp, s_mx_fp, s_my_fp, s_x_offset_fp, s_y_offset_fp;
static struct perspective s_left_eye, s_right_eye, *s_per;
static struct perspective_fp s_left_eye_fp, s_right_eye_fp, *s_per_fp;
static _LCMPLX s_jbc;
static _CMPLX s_jbc_fp;
#ifndef XFRACT
static double s_fg, s_fg16;
#endif
static long s_width, s_dist, s_depth, s_br_ratio;
#ifndef XFRACT
static long s_eyes;
#endif

int julibrot_setup(void)
{
#ifndef XFRACT
	long origin;
#endif
	int r = 0;
	char *mapname;

#ifndef XFRACT
	if (g_colors < 255)
	{
		stop_message(0, "Sorry, but Julibrots require a 256-color video mode");
		return 0;
	}
#endif

	s_x_offset_fp = (g_xx_max + g_xx_min) / 2;     /* Calculate average */
	s_y_offset_fp = (g_yy_max + g_yy_min) / 2;     /* Calculate average */
	s_dmx_fp = (g_m_x_max_fp - g_m_x_min_fp) / g_z_dots;
	s_dmy_fp = (g_m_y_max_fp - g_m_y_min_fp) / g_z_dots;
	g_float_parameter = &s_jbc_fp;
	s_x_per_inch_fp = (g_xx_min - g_xx_max) / g_width_fp;
	s_y_per_inch_fp = (g_yy_max - g_yy_min) / g_height_fp;
	s_inch_per_x_dot_fp = g_width_fp / g_x_dots;
	s_inch_per_y_dot_fp = g_height_fp / g_y_dots;
	s_init_z_fp = g_origin_fp - (g_depth_fp / 2);
	s_right_eye_fp.x = (g_juli_3d_mode == JULI3DMODE_MONOCULAR) ? 0.0 : (g_eyes_fp / 2);
	s_left_eye_fp.x = -s_right_eye_fp.x;
	s_left_eye_fp.y = s_right_eye_fp.y = 0;
	s_left_eye_fp.zx = s_right_eye_fp.zx = g_screen_distance_fp;
	s_left_eye_fp.zy = s_right_eye_fp.zy = g_screen_distance_fp;
	s_b_base = 128;

#ifndef XFRACT
	if (g_fractal_specific[g_fractal_type].isinteger > 0)
	{
		long jxmin, jxmax, jymin, jymax, mxmax, mymax;
		if (g_fractal_specific[g_new_orbit_type].isinteger == 0)
		{
			stop_message(0, "Julibrot orbit type isinteger mismatch");
		}
		if (g_fractal_specific[g_new_orbit_type].isinteger > 1)
		{
			g_bit_shift = g_fractal_specific[g_new_orbit_type].isinteger;
		}
		s_fg = (double) (1L << g_bit_shift);
		s_fg16 = (double) (1L << 16);
		jxmin = (long) (g_xx_min*s_fg);
		jxmax = (long) (g_xx_max*s_fg);
		s_x_offset = (jxmax + jxmin) / 2;    /* Calculate average */
		jymin = (long) (g_yy_min*s_fg);
		jymax = (long) (g_yy_max*s_fg);
		s_y_offset = (jymax + jymin) / 2;    /* Calculate average */
		s_m_x_min = (long) (g_m_x_min_fp*s_fg);
		mxmax = (long) (g_m_x_max_fp*s_fg);
		s_m_y_min = (long) (g_m_y_min_fp*s_fg);
		mymax = (long) (g_m_y_max_fp*s_fg);
		origin = (long) (g_origin_fp*s_fg16);
		s_depth = (long) (g_depth_fp*s_fg16);
		s_width = (long) (g_width_fp*s_fg16);
		s_dist = (long) (g_screen_distance_fp*s_fg16);
		s_eyes = (long) (g_eyes_fp*s_fg16);
		s_br_ratio = (long) s_fg16;
		s_dmx = (mxmax - s_m_x_min) / g_z_dots;
		s_dmy = (mymax - s_m_y_min) / g_z_dots;
		g_long_parameter = &s_jbc;

		s_x_per_inch = (long) ((g_xx_min - g_xx_max) / g_width_fp*s_fg);
		s_y_per_inch = (long) ((g_yy_max - g_yy_min) / g_height_fp*s_fg);
		s_inch_per_x_dot = (long) ((g_width_fp / g_x_dots)*s_fg16);
		s_inch_per_y_dot = (long) ((g_height_fp / g_y_dots)*s_fg16);
		s_init_z = origin - (s_depth / 2);
		s_right_eye.x = (g_juli_3d_mode == JULI3DMODE_MONOCULAR) ? 0L : (s_eyes/2);
		s_left_eye.x = -s_right_eye.x;
		s_left_eye.y = s_right_eye.y = 0l;
		s_left_eye.zx = s_right_eye.zx = s_dist;
		s_left_eye.zy = s_right_eye.zy = s_dist;
		s_b_base = 128;
	}
#endif

	if (g_juli_3d_mode == JULI3DMODE_RED_BLUE)
	{
		g_save_dac = SAVEDAC_NO;
		mapname = g_glasses1_map;
	}
	else
	{
		mapname = g_grey_file;
	}
	if (g_save_dac != SAVEDAC_YES)
	{
		if (validate_luts(mapname) != 0)
		{
			return 0;
		}
		spindac(0, 1);               /* load it, but don't spin */
		if (g_save_dac == SAVEDAC_NEXT)
		{
			g_save_dac = SAVEDAC_YES;
		}
	}
	return r >= 0;
}


int julibrot_per_pixel(void)
{
	s_jx = multiply(s_per->x - s_x_pixel, s_init_z, 16);
	s_jx = divide(s_jx, s_dist, 16) - s_x_pixel;
	s_jx = multiply(s_jx << (g_bit_shift - 16), s_x_per_inch, g_bit_shift);
	s_jx += s_x_offset;
	s_djx = divide(s_depth, s_dist, 16);
	s_djx = multiply(s_djx, s_per->x - s_x_pixel, 16) << (g_bit_shift - 16);
	s_djx = multiply(s_djx, s_x_per_inch, g_bit_shift) / g_z_dots;

	s_jy = multiply(s_per->y - s_y_pixel, s_init_z, 16);
	s_jy = divide(s_jy, s_dist, 16) - s_y_pixel;
	s_jy = multiply(s_jy << (g_bit_shift - 16), s_y_per_inch, g_bit_shift);
	s_jy += s_y_offset;
	s_djy = divide(s_depth, s_dist, 16);
	s_djy = multiply(s_djy, s_per->y - s_y_pixel, 16) << (g_bit_shift - 16);
	s_djy = multiply(s_djy, s_y_per_inch, g_bit_shift) / g_z_dots;

	return 1;
}

int julibrot_per_pixel_fp(void)
{
	s_jx_fp = ((s_per_fp->x - s_x_pixel_fp)*s_init_z_fp / g_screen_distance_fp - s_x_pixel_fp)*s_x_per_inch_fp;
	s_jx_fp += s_x_offset_fp;
	s_djx_fp = (g_depth_fp / g_screen_distance_fp)*(s_per_fp->x - s_x_pixel_fp)*s_x_per_inch_fp / g_z_dots;

	s_jy_fp = ((s_per_fp->y - s_y_pixel_fp)*s_init_z_fp / g_screen_distance_fp - s_y_pixel_fp)*s_y_per_inch_fp;
	s_jy_fp += s_y_offset_fp;
	s_djy_fp = g_depth_fp / g_screen_distance_fp*(s_per_fp->y - s_y_pixel_fp)*s_y_per_inch_fp / g_z_dots;

	return 1;
}

static int s_plotted;

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
		g_old_z_l.x = s_jx;
		g_old_z_l.y = s_jy;
		s_jbc.x = s_mx;
		s_jbc.y = s_my;
		if (driver_key_pressed())
		{
			return -1;
		}
		g_temp_sqr_x_l = multiply(g_old_z_l.x, g_old_z_l.x, g_bit_shift);
		g_temp_sqr_y_l = multiply(g_old_z_l.y, g_old_z_l.y, g_bit_shift);
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
				g_color = (int) (128l*z_pixel / g_z_dots);
				if ((g_row + g_col) & 1)
				{

					(*g_plot_color)(g_col, g_row, 127 - g_color);
				}
				else
				{
					g_color = (int) (multiply((long) g_color << 16, s_br_ratio, 16) >> 16);
					if (g_color < 1)
					{
						g_color = 1;
					}
					if (g_color > 127)
					{
						g_color = 127;
					}
					(*g_plot_color)(g_col, g_row, 127 + s_b_base - g_color);
				}
			}
			else
			{
				g_color = (int) (254l*z_pixel / g_z_dots);
				(*g_plot_color)(g_col, g_row, g_color + 1);
			}
			s_plotted = 1;
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
		/* Special initialization for Mandelbrot types */
		if ((g_new_orbit_type == QUATFP || g_new_orbit_type == HYPERCMPLXFP)
			&& g_save_release > 2002)
		{
			g_old_z.x = 0.0;
			g_old_z.y = 0.0;
			s_jbc_fp.x = 0.0;
			s_jbc_fp.y = 0.0;
			g_quaternion_c = s_jx_fp;
			g_quaternion_ci = s_jy_fp;
			g_quaternion_cj = s_mx_fp;
			g_quaternion_ck = s_my_fp;
		}
		else
		{
			g_old_z.x = s_jx_fp;
			g_old_z.y = s_jy_fp;
			s_jbc_fp.x = s_mx_fp;
			s_jbc_fp.y = s_my_fp;
			g_quaternion_c = g_parameters[0];
			g_quaternion_ci = g_parameters[1];
			g_quaternion_cj = g_parameters[2];
			g_quaternion_ck = g_parameters[3];
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
		g_temp_sqr_x = sqr(g_old_z.x);
		g_temp_sqr_y = sqr(g_old_z.y);

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
				g_color = (int) (128l*z_pixel / g_z_dots);
				if ((g_row + g_col) & 1)
				{
					(*g_plot_color)(g_col, g_row, 127 - g_color);
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
					(*g_plot_color)(g_col, g_row, 127 + s_b_base - g_color);
				}
			}
			else
			{
				g_color = (int) (254l*z_pixel / g_z_dots);
				(*g_plot_color)(g_col, g_row, g_color + 1);
			}
			s_plotted = 1;
			break;
		}
		s_mx_fp += s_dmx_fp;
		s_my_fp += s_dmy_fp;
		s_jx_fp += s_djx_fp;
		s_jy_fp += s_djy_fp;
	}
	return 0;
}

int standard_4d_fractal(void)
{
	long x, y;
	int xdot, ydot;
	g_c_exp = (int)g_parameters[2];
	if (g_new_orbit_type == LJULIAZPOWER)
	{
		if (g_c_exp < 1)
		{
			g_c_exp = 1;
		}
		g_fractal_specific[g_new_orbit_type].orbitcalc =
			(g_parameters[3] == 0.0 && g_debug_flag != DEBUGFLAG_UNOPT_POWER && (double)g_c_exp == g_parameters[2])
			? z_power_orbit : complex_z_power_orbit;
	}

	for (y = 0, ydot = (g_y_dots >> 1) - 1; ydot >= 0; ydot--, y -= s_inch_per_y_dot)
	{
		s_plotted = 0;
		x = -(s_width >> 1);
		for (xdot = 0; xdot < g_x_dots; xdot++, x += s_inch_per_x_dot)
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
		if (s_plotted == 0)
		{
			if (y == 0)
			{
				s_plotted = -1;  /* no points first pass; don't give up */
			}
			else
			{
				break;
			}
		}
	}
	return 0;
}

int standard_4d_fractal_fp(void)
{
	double x, y;
	int xdot, ydot;
	g_c_exp = (int)g_parameters[2];

	if (g_new_orbit_type == FPJULIAZPOWER)
	{
		g_fractal_specific[g_new_orbit_type].orbitcalc =
			(g_parameters[3] == 0.0 && g_debug_flag != DEBUGFLAG_UNOPT_POWER && (double)g_c_exp == g_parameters[2])
			? z_power_orbit_fp : complex_z_power_orbit_fp;
		get_julia_attractor (g_parameters[0], g_parameters[1]); /* another attractor? */
	}

	for (y = 0, ydot = (g_y_dots >> 1) - 1; ydot >= 0; ydot--, y -= s_inch_per_y_dot_fp)
	{
		s_plotted = 0;
		x = -g_width_fp / 2;
		for (xdot = 0; xdot < g_x_dots; xdot++, x += s_inch_per_x_dot_fp)
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
		if (s_plotted == 0)
		{
			if (y == 0)
			{
				s_plotted = -1;  /* no points first pass; don't give up */
			}
			else
			{
				break;
			}
		}
	}
	return 0;
}
