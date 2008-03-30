// frothy basin routines 
#include <string>

#include "port.h"
#include "id.h"
#include "cmplx.h"
#include "externs.h"
#include "prototyp.h"
#include "drivers.h"

#include "fracsubr.h"
#include "fractals.h"
#include "framain2.h"
#include "FrothyBasin.h"
#include "loadmap.h"

// frothy basin type 
static int const FROTH_BITSHIFT = 28;

inline long FROTH_D_TO_L(double x)
{
	return (long((x)*(1L << FROTH_BITSHIFT)));
}

static double const FROTH_CLOSE = 1e-6;      // seems like a good value 
static long const FROTH_LCLOSE = FROTH_D_TO_L(FROTH_CLOSE);
static double const SQRT3 = 1.732050807568877193;
static double const FROTH_SLOPE = SQRT3;
static long const FROTH_LSLOPE = FROTH_D_TO_L(FROTH_SLOPE);
static double const FROTH_CRITICAL_A = 1.028713768218725;  // 1.0287137682187249127 

struct froth_double_struct
{
	double a;
	double halfa;
	double top_x1;
	double top_x2;
	double top_x3;
	double top_x4;
	double left_x1;
	double left_x2;
	double left_x3;
	double left_x4;
	double right_x1;
	double right_x2;
	double right_x3;
	double right_x4;
};

struct froth_long_struct
{
	long a;
	long halfa;
	long top_x1;
	long top_x2;
	long top_x3;
	long top_x4;
	long left_x1;
	long left_x2;
	long left_x3;
	long left_x4;
	long right_x1;
	long right_x2;
	long right_x3;
	long right_x4;
};

struct froth_struct
{
	int repeat_mapping;
	int altcolor;
	int attractors;
	int shades;
	froth_double_struct f;
	froth_long_struct l;
};

static froth_struct s_frothy_data = { 0 };

inline double froth_top_x_mapping(double x)
{
	return ((x)*(x) - (x) - 3.0*s_frothy_data.f.a*s_frothy_data.f.a/4.0);
}

// color maps which attempt to replicate the images of James Alexander. 
static void set_froth_palette()
{
	if ((g_.ColorState() == COLORSTATE_DEFAULT) && (g_colors >= 16))
	{
		const std::string mapname = (s_frothy_data.attractors == 6) ? "froth6.map" : "froth3.map";
		if (validate_luts(mapname))
		{
			return;
		}
		g_.SetColorState(COLORSTATE_DEFAULT); // treat map as default 
		load_dac();
	}
}

bool froth_setup()
{
	double sin_theta;
	double cos_theta;
	double x0;
	double y0;

	sin_theta = SQRT3/2; // sin(2*PI/3) 
	cos_theta = -0.5;    // cos(2*PI/3) 

	// for the all important backwards compatibility 
	{
		if (g_parameters[0] != 2)
		{
			g_parameters[0] = 1;
		}
		s_frothy_data.repeat_mapping = int(g_parameters[0]) == 2;
		if (g_parameters[1] != 0)
		{
			g_parameters[1] = 1;
		}
		s_frothy_data.altcolor = int(g_parameters[1]);
		s_frothy_data.f.a = g_parameters[2];

		s_frothy_data.attractors = fabs(s_frothy_data.f.a) <= FROTH_CRITICAL_A ? (!s_frothy_data.repeat_mapping ? 3 : 6)
																: (!s_frothy_data.repeat_mapping ? 2 : 3);

		// new improved values 
		// 0.5 is the value that causes the mapping to reach a minimum 
		x0 = 0.5;
		// a/2 is the value that causes the y value to be invariant over the mappings 
		y0 = s_frothy_data.f.halfa = s_frothy_data.f.a/2;
		s_frothy_data.f.top_x1 = froth_top_x_mapping(x0);
		s_frothy_data.f.top_x2 = froth_top_x_mapping(s_frothy_data.f.top_x1);
		s_frothy_data.f.top_x3 = froth_top_x_mapping(s_frothy_data.f.top_x2);
		s_frothy_data.f.top_x4 = froth_top_x_mapping(s_frothy_data.f.top_x3);

		// rotate 120 degrees counter-clock-wise 
		s_frothy_data.f.left_x1 = s_frothy_data.f.top_x1*cos_theta - y0*sin_theta;
		s_frothy_data.f.left_x2 = s_frothy_data.f.top_x2*cos_theta - y0*sin_theta;
		s_frothy_data.f.left_x3 = s_frothy_data.f.top_x3*cos_theta - y0*sin_theta;
		s_frothy_data.f.left_x4 = s_frothy_data.f.top_x4*cos_theta - y0*sin_theta;

		// rotate 120 degrees clock-wise 
		s_frothy_data.f.right_x1 = s_frothy_data.f.top_x1*cos_theta + y0*sin_theta;
		s_frothy_data.f.right_x2 = s_frothy_data.f.top_x2*cos_theta + y0*sin_theta;
		s_frothy_data.f.right_x3 = s_frothy_data.f.top_x3*cos_theta + y0*sin_theta;
		s_frothy_data.f.right_x4 = s_frothy_data.f.top_x4*cos_theta + y0*sin_theta;
	}

	// if 2 attractors, use same shades as 3 attractors 
	s_frothy_data.shades = (g_colors-1)/std::max(3, s_frothy_data.attractors);

	// g_rq_limit needs to be at least sq(1 + sqrt(1 + sq(a))), 
	// which is never bigger than 6.93..., so we'll call it 7.0 
	if (g_rq_limit < 7.0)
	{
		g_rq_limit = 7.0;
	}
	set_froth_palette();
	// make the best of the .map situation 
	g_orbit_color = s_frothy_data.attractors != 6 && g_colors >= 16 ? (s_frothy_data.shades << 1) + 1 : g_colors-1;

	if (g_integer_fractal)
	{
		froth_long_struct tmp_l;

		tmp_l.a = FROTH_D_TO_L(s_frothy_data.f.a);
		tmp_l.halfa = FROTH_D_TO_L(s_frothy_data.f.halfa);

		tmp_l.top_x1 = FROTH_D_TO_L(s_frothy_data.f.top_x1);
		tmp_l.top_x2 = FROTH_D_TO_L(s_frothy_data.f.top_x2);
		tmp_l.top_x3 = FROTH_D_TO_L(s_frothy_data.f.top_x3);
		tmp_l.top_x4 = FROTH_D_TO_L(s_frothy_data.f.top_x4);

		tmp_l.left_x1 = FROTH_D_TO_L(s_frothy_data.f.left_x1);
		tmp_l.left_x2 = FROTH_D_TO_L(s_frothy_data.f.left_x2);
		tmp_l.left_x3 = FROTH_D_TO_L(s_frothy_data.f.left_x3);
		tmp_l.left_x4 = FROTH_D_TO_L(s_frothy_data.f.left_x4);

		tmp_l.right_x1 = FROTH_D_TO_L(s_frothy_data.f.right_x1);
		tmp_l.right_x2 = FROTH_D_TO_L(s_frothy_data.f.right_x2);
		tmp_l.right_x3 = FROTH_D_TO_L(s_frothy_data.f.right_x3);
		tmp_l.right_x4 = FROTH_D_TO_L(s_frothy_data.f.right_x4);

		s_frothy_data.l = tmp_l;
	}
	return true;
}

// Froth Fractal type 
int froth_calc()   // per pixel 1/2/g, called with row & col set 
{
	int found_attractor = 0;

	if (check_key())
	{
		return -1;
	}

	g_orbit_index = 0;
	g_color_iter = 0;
	if (g_show_dot > 0)
	{
		g_plot_color(g_col, g_row, g_show_dot % g_colors);
	}
	if (!g_integer_fractal) // fp mode 
	{
		if (g_invert)
		{
			invert_z(&g_temp_z);
			g_old_z = g_temp_z;
		}
		else
		{
			g_old_z = g_externs.DPixel();
		}

		while (!found_attractor)
		{
			g_temp_sqr_x = sqr(g_old_z.real());
			g_temp_sqr_y = sqr(g_old_z.imag());
			if ((g_temp_sqr_x + g_temp_sqr_y < g_rq_limit) && (g_color_iter < g_max_iteration))
			{
				break;
			}

			// simple formula: z = z^2 + conj(z*(-1 + ai)) 
			// but it's the attractor that makes this so interesting 
			g_new_z.real(g_temp_sqr_x - g_temp_sqr_y - g_old_z.real() - s_frothy_data.f.a*g_old_z.imag());
			g_old_z.y += (g_old_z.real() + g_old_z.real())*g_old_z.imag() - s_frothy_data.f.a*g_old_z.real();
			g_old_z.real(g_new_z.x);
			if (s_frothy_data.repeat_mapping)
			{
				g_new_z.real(sqr(g_old_z.real()) - sqr(g_old_z.imag()) - g_old_z.real() - s_frothy_data.f.a*g_old_z.imag());
				g_old_z.y += (g_old_z.real() + g_old_z.real())*g_old_z.imag() - s_frothy_data.f.a*g_old_z.real();
				g_old_z.real(g_new_z.x);
			}

			g_color_iter++;

			if (g_show_orbit)
			{
				if (driver_key_pressed())
				{
					break;
				}
				plot_orbit(g_old_z.real(), g_old_z.imag(), -1);
			}

			if (fabs(s_frothy_data.f.halfa-g_old_z.imag()) < FROTH_CLOSE
					&& g_old_z.real() >= s_frothy_data.f.top_x1 && g_old_z.real() <= s_frothy_data.f.top_x2)
			{
				if ((!s_frothy_data.repeat_mapping && s_frothy_data.attractors == 2)
					|| (s_frothy_data.repeat_mapping && s_frothy_data.attractors == 3))
				{
					found_attractor = 1;
				}
				else if (g_old_z.real() <= s_frothy_data.f.top_x3)
				{
					found_attractor = 1;
				}
				else if (g_old_z.real() >= s_frothy_data.f.top_x4)
				{
					found_attractor = !s_frothy_data.repeat_mapping ? 1 : 2;
				}
			}
			else if (fabs(FROTH_SLOPE*g_old_z.real() - s_frothy_data.f.a - g_old_z.imag()) < FROTH_CLOSE
						&& g_old_z.real() <= s_frothy_data.f.right_x1 && g_old_z.real() >= s_frothy_data.f.right_x2)
			{
				if (!s_frothy_data.repeat_mapping && s_frothy_data.attractors == 2)
				{
					found_attractor = 2;
				}
				else if (s_frothy_data.repeat_mapping && s_frothy_data.attractors == 3)
				{
					found_attractor = 3;
				}
				else if (g_old_z.real() >= s_frothy_data.f.right_x3)
				{
					found_attractor = !s_frothy_data.repeat_mapping ? 2 : 4;
				}
				else if (g_old_z.real() <= s_frothy_data.f.right_x4)
				{
					found_attractor = !s_frothy_data.repeat_mapping ? 3 : 6;
				}
			}
			else if (fabs(-FROTH_SLOPE*g_old_z.real() - s_frothy_data.f.a - g_old_z.imag()) < FROTH_CLOSE
						&& g_old_z.real() <= s_frothy_data.f.left_x1 && g_old_z.real() >= s_frothy_data.f.left_x2)
			{
				if (!s_frothy_data.repeat_mapping && s_frothy_data.attractors == 2)
				{
					found_attractor = 2;
				}
				else if (s_frothy_data.repeat_mapping && s_frothy_data.attractors == 3)
				{
					found_attractor = 2;
				}
				else if (g_old_z.real() >= s_frothy_data.f.left_x3)
				{
					found_attractor = !s_frothy_data.repeat_mapping ? 3 : 5;
				}
				else if (g_old_z.real() <= s_frothy_data.f.left_x4)
				{
					found_attractor = !s_frothy_data.repeat_mapping ? 2 : 3;
				}
			}
		}
	}
	else // integer mode 
	{
		if (g_invert)
		{
			invert_z(&g_temp_z);
			g_old_z_l = ComplexDoubleToFudge(g_temp_z);
		}
		else
		{
			g_old_z_l = g_externs.LPixel();
		}

		while (!found_attractor)
		{
			g_temp_sqr_x_l = lsqr(g_old_z_l.x);
			g_temp_sqr_y_l = lsqr(g_old_z_l.y);
			g_magnitude_l = g_temp_sqr_x_l + g_temp_sqr_y_l;
			if ((g_magnitude_l < g_rq_limit_l) && (g_magnitude_l >= 0) && (g_color_iter < g_max_iteration))
			{
				break;
			}

			// simple formula: z = z^2 + conj(z*(-1 + ai)) 
			// but it's the attractor that makes this so interesting 
			g_new_z_l.real(g_temp_sqr_x_l - g_temp_sqr_y_l - g_old_z_l.x - multiply(s_frothy_data.l.a, g_old_z_l.y, g_bit_shift));
			g_old_z_l.y += (multiply(g_old_z_l.x, g_old_z_l.y, g_bit_shift) << 1) - multiply(s_frothy_data.l.a, g_old_z_l.x, g_bit_shift);
			g_old_z_l.x = g_new_z_l.x;
			if (s_frothy_data.repeat_mapping)
			{
				g_temp_sqr_x_l = lsqr(g_old_z_l.x);
				g_temp_sqr_y_l = lsqr(g_old_z_l.y);
				g_magnitude_l = g_temp_sqr_x_l + g_temp_sqr_y_l;
				if ((g_magnitude_l > g_rq_limit_l) || (g_magnitude_l < 0))
				{
					break;
				}
				g_new_z_l.real(g_temp_sqr_x_l - g_temp_sqr_y_l - g_old_z_l.x - multiply(s_frothy_data.l.a, g_old_z_l.y, g_bit_shift));
				g_old_z_l.y += (multiply(g_old_z_l.x, g_old_z_l.y, g_bit_shift) << 1) - multiply(s_frothy_data.l.a, g_old_z_l.x, g_bit_shift);
				g_old_z_l.x = g_new_z_l.x;
			}
			g_color_iter++;

			if (g_show_orbit)
			{
				if (driver_key_pressed())
				{
					break;
				}
				plot_orbit_i(g_old_z_l.x, g_old_z_l.y, -1);
			}

			if (labs(s_frothy_data.l.halfa-g_old_z_l.y) < FROTH_LCLOSE
				&& g_old_z_l.x > s_frothy_data.l.top_x1 && g_old_z_l.x < s_frothy_data.l.top_x2)
			{
				if ((!s_frothy_data.repeat_mapping && s_frothy_data.attractors == 2)
					|| (s_frothy_data.repeat_mapping && s_frothy_data.attractors == 3))
				{
					found_attractor = 1;
				}
				else if (g_old_z_l.x <= s_frothy_data.l.top_x3)
				{
					found_attractor = 1;
				}
				else if (g_old_z_l.x >= s_frothy_data.l.top_x4)
				{
					found_attractor = !s_frothy_data.repeat_mapping ? 1 : 2;
				}
			}
			else if (labs(multiply(FROTH_LSLOPE, g_old_z_l.x, g_bit_shift)-s_frothy_data.l.a-g_old_z_l.y) < FROTH_LCLOSE
						&& g_old_z_l.x <= s_frothy_data.l.right_x1 && g_old_z_l.x >= s_frothy_data.l.right_x2)
			{
				if (!s_frothy_data.repeat_mapping && s_frothy_data.attractors == 2)
				{
					found_attractor = 2;
				}
				else if (s_frothy_data.repeat_mapping && s_frothy_data.attractors == 3)
				{
					found_attractor = 3;
				}
				else if (g_old_z_l.x >= s_frothy_data.l.right_x3)
				{
					found_attractor = !s_frothy_data.repeat_mapping ? 2 : 4;
				}
				else if (g_old_z_l.x <= s_frothy_data.l.right_x4)
				{
					found_attractor = !s_frothy_data.repeat_mapping ? 3 : 6;
				}
			}
			else if (labs(multiply(-FROTH_LSLOPE, g_old_z_l.x, g_bit_shift)-s_frothy_data.l.a-g_old_z_l.y) < FROTH_LCLOSE)
			{
				if (!s_frothy_data.repeat_mapping && s_frothy_data.attractors == 2)
				{
					found_attractor = 2;
				}
				else if (s_frothy_data.repeat_mapping && s_frothy_data.attractors == 3)
				{
					found_attractor = 2;
				}
				else if (g_old_z_l.x >= s_frothy_data.l.left_x3)
				{
					found_attractor = !s_frothy_data.repeat_mapping ? 3 : 5;
				}
				else if (g_old_z_l.x <= s_frothy_data.l.left_x4)
				{
					found_attractor = !s_frothy_data.repeat_mapping ? 2 : 3;
				}
			}
		}
	}
	if (g_show_orbit)
	{
		orbit_scrub();
	}

	g_real_color_iter = g_color_iter;
	g_input_counter -= abs(int(g_real_color_iter));
	if (g_input_counter <= 0)
	{
		if (check_key())
		{
			return -1;
		}
		g_input_counter = g_max_input_counter;
	}

	// inside - Here's where non-palette based images would be nice.  Instead, 
	// we'll use blocks of (g_colors-1)/3 or (g_colors-1)/6 and use special froth  
	// color maps in attempt to replicate the images of James Alexander.       
	if (found_attractor)
	{
		if (!s_frothy_data.altcolor)
		{
			if (g_color_iter > s_frothy_data.shades)
			{
				g_color_iter = s_frothy_data.shades;
			}
		}
		else
		{
			g_color_iter = s_frothy_data.shades*g_color_iter/g_max_iteration;
		}
		if (g_color_iter == 0)
		{
			g_color_iter = 1;
		}
		g_color_iter += s_frothy_data.shades*(found_attractor-1);
		g_old_color_iter = g_color_iter;
	}
	else // outside, or inside but didn't get sucked in by attractor. 
	{
		g_color_iter = 0;
	}

	g_color = abs(int(g_color_iter));

	g_plot_color(g_col, g_row, g_color);

	return g_color;
}

//
//	These last two froth functions are for the orbit-in-window feature.
//	Normally, this feature requires standard_fractal, but since it is the
//	attractor that makes the frothybasin type so unique, it is worth
//	putting in as a stand-alone.
//
int froth_per_pixel()
{
	if (!g_integer_fractal) // fp mode 
	{
		g_old_z = g_externs.DPixel();
		g_temp_sqr_x = sqr(g_old_z.real());
		g_temp_sqr_y = sqr(g_old_z.imag());
	}
	else  // integer mode 
	{
		g_old_z_l = g_externs.LPixel();
		g_temp_sqr_x_l = multiply(g_old_z_l.x, g_old_z_l.x, g_bit_shift);
		g_temp_sqr_y_l = multiply(g_old_z_l.y, g_old_z_l.y, g_bit_shift);
	}
	return 0;
}

int froth_per_orbit()
{
	if (!g_integer_fractal) // fp mode 
	{
		g_new_z.real(g_temp_sqr_x - g_temp_sqr_y - g_old_z.real() - s_frothy_data.f.a*g_old_z.imag());
		g_new_z.imag(2.0*g_old_z.real()*g_old_z.imag() - s_frothy_data.f.a*g_old_z.real() + g_old_z.imag());
		if (s_frothy_data.repeat_mapping)
		{
			g_old_z = g_new_z;
			g_new_z.real(sqr(g_old_z.real()) - sqr(g_old_z.imag()) - g_old_z.real() - s_frothy_data.f.a*g_old_z.imag());
			g_new_z.imag(2.0*g_old_z.real()*g_old_z.imag() - s_frothy_data.f.a*g_old_z.real() + g_old_z.imag());
		}

		g_temp_sqr_x = sqr(g_new_z.x);
		g_temp_sqr_y = sqr(g_new_z.y);
		if (g_temp_sqr_x + g_temp_sqr_y >= g_rq_limit)
		{
			return 1;
		}
		g_old_z = g_new_z;
	}
	else  // integer mode 
	{
		g_new_z_l.real(g_temp_sqr_x_l - g_temp_sqr_y_l - g_old_z_l.x - multiply(s_frothy_data.l.a, g_old_z_l.y, g_bit_shift));
		g_new_z_l.imag(g_old_z_l.y + (multiply(g_old_z_l.x, g_old_z_l.y, g_bit_shift) << 1) - multiply(s_frothy_data.l.a, g_old_z_l.x, g_bit_shift));
		if (s_frothy_data.repeat_mapping)
		{
			g_temp_sqr_x_l = lsqr(g_new_z_l.x);
			g_temp_sqr_y_l = lsqr(g_new_z_l.imag());
			if (g_temp_sqr_x_l + g_temp_sqr_y_l >= g_rq_limit_l)
			{
				return 1;
			}
			g_old_z_l = g_new_z_l;
			g_new_z_l.real(g_temp_sqr_x_l - g_temp_sqr_y_l - g_old_z_l.x - multiply(s_frothy_data.l.a, g_old_z_l.y, g_bit_shift));
			g_new_z_l.imag(g_old_z_l.y + (multiply(g_old_z_l.x, g_old_z_l.y, g_bit_shift) << 1) - multiply(s_frothy_data.l.a, g_old_z_l.x, g_bit_shift));
		}
		g_temp_sqr_x_l = lsqr(g_new_z_l.x);
		g_temp_sqr_y_l = lsqr(g_new_z_l.imag());
		if (g_temp_sqr_x_l + g_temp_sqr_y_l >= g_rq_limit_l)
		{
			return 1;
		}
		g_old_z_l = g_new_z_l;
	}
	return 0;
}
