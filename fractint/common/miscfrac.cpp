/*
	Miscellaneous fractal-specific code
*/
#include <algorithm>
#include <sstream>
#include <string>

#include <string.h>
#include <limits.h>

#include <boost/format.hpp>

#include "port.h"
#include "prototyp.h"
#include "fractype.h"

#include "calcfrac.h"
#include "diskvid.h"
#include "drivers.h"
#include "fpu.h"
#include "fracsubr.h"
#include "fractals.h"
#include "framain2.h"
#include "loadmap.h"
#include "miscfrac.h"
#include "miscres.h"
#include "realdos.h"
#include "testpt.h"

#include "EscapeTime.h"
#include "MathUtil.h"
#include "Formula.h"

#define RANDOM(x)  (rand() % (x))

/* for diffusion type */
#define DIFFUSION_CENTRAL	0
#define DIFFUSION_LINE		1
#define DIFFUSION_SQUARE	2

/* for bifurcation type: */
#define DEFAULT_FILTER 1000     /* "Beauty of Fractals" recommends using 5000
								(p.25), but that seems unnecessary. Can
								override this value with a nonzero param1 */
#define SEED 0.66               /* starting value for population */

typedef void (*PLOT)(int, int, int);

/* global data */

/* data local to this module */
static int s_iparm_x;      /* s_iparm_x = g_parameter.x*8 */
static int s_shift_value;  /* shift based on #colors */
static int s_recur1 = 1;
static int s_plasma_colors;
static int s_recur_level = 0;
static int s_plasma_check;                        /* to limit kbd checking */
static U16 (*s_get_pixels)(int, int)  = (U16(*)(int, int))getcolor;
static U16 s_max_plasma;
static int *s_verhulst_array = 0;
static unsigned long s_filter_cycles;
static bool s_half_time_check;
static long s_population_l;
static long s_rate_l;
static double s_population;
static double s_rate;
static bool s_mono = false;
static int s_outside_x;
static long s_pi_l;
static long s_bifurcation_close_enough_l;
static long s_bifurcation_saved_population_l; /* poss future use */
static double s_bifurcation_close_enough;
static double s_bifurcation_saved_population;
static int s_bifurcation_saved_increment;
static long s_bifurcation_saved_mask;
static long s_beta;
static int s_lyapunov_length;
static int s_lyapunov_r_xy[34];

/* routines local to this module */
static void set_plasma_palette();
static U16 adjust(int xa, int ya, int x, int y, int xb, int yb);
static void subdivide(int x1, int y1, int x2, int y2);
static int new_subdivision(int x1, int y1, int x2, int y2, int recur);
static void verhulst();
static void bifurcation_period_init();
static int bifurcation_periodic(long time);
static int lyapunov_cycles(long, double, double);


/***************** standalone engine for "test" ********************/

int test()
{
	int startrow;
	int startpass;
	int numpasses;
	startrow = 0;
	startpass = 0;
	if (g_resuming)
	{
		start_resume();
		get_resume(sizeof(startrow), &startrow, sizeof(startpass), &startpass, 0);
		end_resume();
	}
	if (test_start()) /* assume it was stand-alone, doesn't want passes logic */
	{
		return 0;
	}
	numpasses = (g_standard_calculation_mode == '1') ? 0 : 1;
	for (g_passes = startpass; g_passes <= numpasses; g_passes++)
	{
		for (g_row = startrow; g_row <= g_y_stop; g_row = g_row + 1 + numpasses)
		{
			for (g_col = 0; g_col <= g_x_stop; g_col++)       /* look at each point on screen */
			{
				register int color;
				g_initial_z.x = g_dx_pixel();
				g_initial_z.y = g_dy_pixel();
				if (driver_key_pressed())
				{
					test_end();
					alloc_resume(20, 1);
					put_resume(sizeof(g_row), &g_row, sizeof(g_passes), &g_passes, 0);
					return -1;
				}
				color = test_per_pixel(g_initial_z.x, g_initial_z.y, g_parameter.x, g_parameter.y, g_max_iteration, g_inside);
				if (color >= g_colors)  /* avoid trouble if color is 0 */
				{
					color = ((color-1) % g_and_color) + 1; /* skip color zero */
				}
				(*g_plot_color)(g_col, g_row, color);
				if (numpasses && (g_passes == 0))
				{
					(*g_plot_color)(g_col, g_row + 1, color);
				}
			}
		}
		startrow = g_passes + 1;
	}
	test_end();
	return 0;
}

/***************** standalone engine for "plasma" ********************/

/* returns a random 16 bit value that is never 0 */
static U16 rand16()
{
	U16 value;
	value = U16(rand15());
	value <<= 1;
	value = U16(value + (rand15() & 1));
	if (value < 1)
	{
		value = 1;
	}
	return value;
}

static void put_potential(int x, int y, U16 color)
{
	if (color < 1)
	{
		color = 1;
	}
	g_plot_color_put_color(x, y, color >> 8 ? color >> 8 : 1);  /* don't write 0 */
	/* we don't write this if driver_diskp() because the above g_plot_color_put_color
			was already a "disk_write" in that case */
	if (!driver_diskp())
	{
		disk_write(x + g_sx_offset, y + g_sy_offset, color >> 8);    /* upper 8 bits */
	}
	disk_write(x + g_sx_offset, y + g_screen_height + g_sy_offset, color&255); /* lower 8 bits */
}

/* fixes border */
static void put_potential_border(int x, int y, U16 color)
{
	if ((x == 0) || (y == 0) || (x == g_x_dots-1) || (y == g_y_dots-1))
	{
		color = U16(g_outside);
	}
	put_potential(x, y, color);
}

/* fixes border */
static void put_color_border(int x, int y, int color)
{
	if ((x == 0) || (y == 0) || (x == g_x_dots-1) || (y == g_y_dots-1))
	{
		color = g_outside;
	}
	if (color < 1)
	{
		color = 1;
	}
	g_plot_color_put_color(x, y, color);
}

static U16 get_potential(int x, int y)
{
	U16 color = U16(disk_read(x + g_sx_offset, y + g_sy_offset));
	color = U16((color << 8) + U16(disk_read(x + g_sx_offset, y + g_screen_height + g_sy_offset)));
	return color;
}

static U16 adjust(int xa, int ya, int x, int y, int xb, int yb)
{
	S32 pseudorandom;
	pseudorandom = ((S32)s_iparm_x)*((rand15()-16383));
	/* pseudorandom = pseudorandom*(abs(xa-xb) + abs(ya-yb)); */
	pseudorandom *= s_recur1;
	pseudorandom >>= s_shift_value;
	pseudorandom = (((S32) s_get_pixels(xa, ya) + (S32) s_get_pixels(xb, yb) + 1)/2) + pseudorandom;
	if (s_max_plasma == 0)
	{
		if (pseudorandom >= s_plasma_colors)
		{
			pseudorandom = s_plasma_colors-1;
		}
	}
	else if (pseudorandom >= (S32)s_max_plasma)
	{
		pseudorandom = s_max_plasma;
	}
	if (pseudorandom < 1)
	{
		pseudorandom = 1;
	}
	g_plot_color(x, y, U16(pseudorandom));
	return U16(pseudorandom);
}


static int new_subdivision(int x1, int y1, int x2, int y2, int recur)
{
	int x;
	int y;
	int nx1;
	int nx;
	int ny1;
	int ny;
	S32 i, v;

	struct sub
	{
		BYTE t; /* top of stack */
		int v[16]; /* subdivided value */
		BYTE r[16];  /* recursion level */
	};

	static struct sub subx, suby;

	s_recur1 = int(320L >> recur);
	suby.t = 2;
	ny = y2;
	ny1 = y1;
	suby.v[0] = y2;
	suby.v[2] = y1;
	suby.r[0] = 0;
	suby.r[2] = 0;
	suby.r[1] = 1;
	suby.v[1] = (ny1 + ny)/2;
	y = suby.v[1];

	while (suby.t >= 1)
	{
		if ((++s_plasma_check & 0x0f) == 1)
		{
			if (driver_key_pressed())
			{
				s_plasma_check--;
				return 1;
			}
		}
		while (suby.r[suby.t-1] < (BYTE)recur)
		{
			/*     1.  Create new entry at top of the stack  */
			/*     2.  Copy old top value to new top value.  */
			/*            This is largest y value.           */
			/*     3.  Smallest y is now old mid point       */
			/*     4.  Set new mid point recursion level     */
			/*     5.  New mid point value is average        */
			/*            of largest and smallest            */
			suby.t++;
			ny1  = suby.v[suby.t] = suby.v[suby.t-1];
			ny   = suby.v[suby.t-2];
			suby.r[suby.t] = suby.r[suby.t-1];
			y    = suby.v[suby.t-1]   = (ny1 + ny)/2;
			suby.r[suby.t-1]   = BYTE(std::max(suby.r[suby.t], suby.r[suby.t-2]) + 1);
		}
		subx.t = 2;
		nx  = x2;
		nx1 = x1;
		subx.v[0] = x2;
		subx.v[2] = x1;
		subx.r[0] = 0;
		subx.r[2] = 0;
		subx.r[1] = 1;
		x = subx.v[1] = (nx1 + nx)/2;

		while (subx.t >= 1)
		{
			while (subx.r[subx.t-1] < (BYTE)recur)
			{
				subx.t++; /* move the top ofthe stack up 1 */
				nx1  = subx.v[subx.t] = subx.v[subx.t-1];
				nx   = subx.v[subx.t-2];
				subx.r[subx.t] = subx.r[subx.t-1];
				x    = subx.v[subx.t-1]   = (nx1 + nx)/2;
				subx.r[subx.t-1]   = BYTE(std::max(subx.r[subx.t],
				subx.r[subx.t-2]) + 1);
			}

			i = s_get_pixels(nx, y);
			if (i == 0)
			{
				i = adjust(nx, ny1, nx, y , nx, ny);
			}
			v = i;
			i = s_get_pixels(x, ny);
			if (i == 0)
			{
				i = adjust(nx1, ny, x , ny, nx, ny);
			}
			v += i;
			if (s_get_pixels(x, y) == 0)
			{
				i = s_get_pixels(x, ny1);
				if (i == 0)
				{
					i = adjust(nx1, ny1, x , ny1, nx, ny1);
				}
				v += i;
				i = s_get_pixels(nx1, y);
				if (i == 0)
				{
					i = adjust(nx1, ny1, nx1, y , nx1, ny);
				}
				v += i;
				g_plot_color(x, y, U16((v + 2)/4));
			}

			if (subx.r[subx.t-1] == (BYTE)recur)
			{
				subx.t = BYTE(subx.t - 2);
			}
		}

		if (suby.r[suby.t-1] == (BYTE)recur)
		{
			suby.t = BYTE(suby.t - 2);
		}
	}
	return 0;
}

static void subdivide(int x1, int y1, int x2, int y2)
{
	int x;
	int y;
	S32 v;
	S32 i;
	if ((++s_plasma_check & 0x7f) == 1)
	{
		if (driver_key_pressed())
		{
			s_plasma_check--;
			return;
		}
	}
	if (x2-x1 < 2 && y2-y1 < 2)
	{
		return;
	}
	s_recur_level++;
	s_recur1 = int(320L >> s_recur_level);

	x = (x1 + x2)/2;
	y = (y1 + y2)/2;
	v = s_get_pixels(x, y1);
	if (v == 0)
	{
		v = adjust(x1, y1, x , y1, x2, y1);
	}
	i = v;
	v = s_get_pixels(x2, y);
	if (v == 0)
	{
		v = adjust(x2, y1, x2, y , x2, y2);
	}
	i += v;
	v = s_get_pixels(x, y2);
	if (v == 0)
	{
		v = adjust(x1, y2, x , y2, x2, y2);
	}
	i += v;
	v = s_get_pixels(x1, y);
	if (v == 0)
	{
		v = adjust(x1, y1, x1, y , x1, y2);
	}
	i += v;

	if (s_get_pixels(x, y) == 0)
	{
		g_plot_color(x, y, U16((i + 2)/4));
	}

	subdivide(x1, y1, x , y);
	subdivide(x , y1, x2, y);
	subdivide(x , y , x2, y2);
	subdivide(x1, y , x , y2);
	s_recur_level--;
}

int plasma()
{
	int i;
	int k;
	int n;
	U16 rnd[4];
	bool OldPotFlag;
	bool OldPot16bit;

	OldPotFlag = false;
	OldPot16bit = false;
	s_plasma_check = 0;

	s_iparm_x = int(g_parameters[0]*8);
	if (g_parameter.x <= 0.0)
	{
		s_iparm_x = 0;
	}
	if (g_parameter.x >= 100)
	{
		s_iparm_x = 800;
	}
	g_parameters[0] = double(s_iparm_x)/8.0;  /* let user know what was used */
	if (g_parameters[1] < 0) /* limit parameter values  */
	{
		g_parameters[1] = 0;
	}
	if (g_parameters[1] > 1)
	{
		g_parameters[1] = 1;
	}
	if (g_parameters[2] < 0) /* limit parameter values  */
	{
		g_parameters[2] = 0;
	}
	if (g_parameters[2] > 1)
	{
		g_parameters[2] = 1;
	}
	if (g_parameters[3] < 0) /* limit parameter values  */
	{
		g_parameters[3] = 0;
	}
	if (g_parameters[3] > 1)
	{
		g_parameters[3] = 1;
	}

	if (!g_use_fixed_random_seed && (g_parameters[2] == 1))
	{
		--g_random_seed;
	}
	if (g_parameters[2] != 0 && g_parameters[2] != 1)
	{
		g_random_seed = int(g_parameters[2]);
	}
	s_max_plasma = U16(g_parameters[3]);  /* s_max_plasma is used as a flag for potential */

	if (s_max_plasma != 0)
	{
		if (disk_start_potential() >= 0)
		{
			/* s_max_plasma = (U16)(1L << 16) -1; */
			s_max_plasma = 0xFFFF;
			g_plot_color = (g_outside >= 0) ? PLOT(put_potential_border) : PLOT(put_potential);
			s_get_pixels =  get_potential;
			OldPotFlag = g_potential_flag;
			OldPot16bit = g_potential_16bit;
		}
		else
		{
			s_max_plasma = 0;        /* can't do potential (disk_start failed) */
			g_parameters[3]   = 0;
			g_plot_color = (g_outside >= 0) ? put_color_border : g_plot_color_put_color;
			s_get_pixels  = (U16(*)(int, int))getcolor;
		}
	}
	else
	{
		g_plot_color = (g_outside >= 0) ? put_color_border : g_plot_color_put_color;
		s_get_pixels  = (U16(*)(int, int))getcolor;
	}
	srand(g_random_seed);
	if (!g_use_fixed_random_seed)
	{
		++g_random_seed;
	}

	set_plasma_palette();

	s_shift_value = 18;
	if (s_max_plasma != 0)
	{
		s_shift_value = 10;
	}

	if (s_max_plasma == 0)
	{
		s_plasma_colors = std::min(g_colors, g_max_colors);
		for (n = 0; n < 4; n++)
		{
			rnd[n] = U16(1 + (((rand15()/s_plasma_colors)*(s_plasma_colors-1)) >> (s_shift_value-11)));
		}
	}
	else
	{
		for (n = 0; n < 4; n++)
		{
			rnd[n] = rand16();
		}
	}
	if (DEBUGMODE_PIN_CORNERS_ONE == g_debug_mode)
	{
		for (n = 0; n < 4; n++)
		{
			rnd[n] = 1;
		}
	}

	g_plot_color(0,      0,  rnd[0]);
	g_plot_color(g_x_dots-1,      0,  rnd[1]);
	g_plot_color(g_x_dots-1, g_y_dots-1,  rnd[2]);
	g_plot_color(0, g_y_dots-1,  rnd[3]);

	s_recur_level = 0;
	if (g_parameters[1] == 0)
	{
		subdivide(0, 0, g_x_dots-1, g_y_dots-1);
	}
	else
	{
		s_recur1 = 1;
		i = 1;
		k = 1;
		while (new_subdivision(0, 0, g_x_dots-1, g_y_dots-1, i) == 0)
		{
			k *= 2;
			if (k  >int(std::max(g_x_dots-1, g_y_dots-1)))
			{
				break;
			}
			if (driver_key_pressed())
			{
				n = 1;
				goto done;
			}
			i++;
		}
	}
	n = !driver_key_pressed() ? 0 : 1;
done:
	if (s_max_plasma != 0)
	{
		g_potential_flag = OldPotFlag;
		g_potential_16bit = OldPot16bit;
	}
	g_plot_color    = g_plot_color_put_color;
	s_get_pixels  = (U16(*)(int, int))getcolor;
	return n;
}

static BYTE mix(int i, BYTE first, BYTE second)
{
	return BYTE((i*first + (86 - i)*second)/85);
}

static void set_mix(int idx, int i, const BYTE first[3], const BYTE second[3])
{
	g_.DAC().Set(idx,
		mix(i, first[0], second[0]),
		mix(i, first[1], second[1]),
		mix(i, first[2], second[2]));
}

static void set_plasma_palette()
{
	if (!g_.MapDAC() && !g_color_preloaded) /* map= not specified  */
	{
		BYTE red[3]		= { COLOR_CHANNEL_MAX, 0, 0 };
		BYTE green[3]	= { 0, COLOR_CHANNEL_MAX, 0 };
		BYTE blue[3]	= { 0,  0, COLOR_CHANNEL_MAX };

		g_.DAC().Set(0, 0, 0, 0);
		for (int i = 1; i <= 85; i++)
		{
			set_mix(i, i, green, blue);
			set_mix(i + 85, i, red, green);
			set_mix(i + 170, i, blue, red);
		}
		spindac(0, 1);
	}
}

/***************** standalone engine for "diffusion" ********************/

int diffusion()
{
	int x_max;
	int y_max;
	int x_min;
	int y_min;     /* Current maximum coordinates */
	int border;   /* Distance between release point and fractal */
	int mode;     /* Determines diffusion type:  0 = central (classic) */
					/*                             1 = falling particles */
					/*                             2 = square cavity     */
	int colorshift; /* If zero, select colors at random, otherwise shift */
					/* the color every colorshift points */
	int colorcount;
	int currentcolor;
	int i;
	double cosine;
	double sine;
	double angle;
	int x;
	int y;
	float r;
	float radius;

	if (driver_diskp())
	{
		not_disk_message();
	}

	x = -1;
	y = -1;
	g_bit_shift = 16;
	g_fudge = 1L << 16;

	border = int(g_parameters[0]);
	mode = int(g_parameters[1]);
	colorshift = int(g_parameters[2]);

	colorcount = colorshift; /* Counts down from colorshift */
	currentcolor = 1;  /* Start at color 1 (color 0 is probably invisible)*/

	if ((mode > DIFFUSION_SQUARE) || (mode < DIFFUSION_CENTRAL))
	{
		mode = DIFFUSION_CENTRAL;
	}

	if (border <= 0)
	{
		border = 10;
	}

	srand(g_random_seed);
	if (!g_use_fixed_random_seed)
	{
		++g_random_seed;
	}

	switch (mode)
	{
	case DIFFUSION_CENTRAL:
		x_max = g_x_dots/2 + border;  /* Initial box */
		x_min = g_x_dots/2 - border;
		y_max = g_y_dots/2 + border;
		y_min = g_y_dots/2 - border;
		break;

	case DIFFUSION_LINE:
		x_max = g_x_dots/2 + border;  /* Initial box */
		x_min = g_x_dots/2 - border;
		y_min = g_y_dots - border;
		break;

	case DIFFUSION_SQUARE:
		radius = (g_x_dots > g_y_dots) ? float(g_y_dots - border) : float(g_x_dots - border);
		break;
	}

	if (g_resuming) /* restore g_work_list, if we can't the above will stay in place */
	{
		start_resume();
		if (mode != DIFFUSION_SQUARE)
		{
			get_resume(sizeof(x_max), &x_max, sizeof(x_min), &x_min,
				sizeof(y_max), &y_max, sizeof(y_min), &y_min, 0);
		}
		else
		{
			get_resume(sizeof(x_max), &x_max, sizeof(x_min), &x_min,
				sizeof(y_max), &y_max, sizeof(radius), &radius, 0);
		}
		end_resume();
	}

	switch (mode)
	{
	case DIFFUSION_CENTRAL: /* Single seed point in the center */
		g_plot_color_put_color(g_x_dots/2, g_y_dots/2, currentcolor);
		break;
	case DIFFUSION_LINE: /* Line along the bottom */
		for (i = 0; i <= g_x_dots; i++)
		{
			g_plot_color_put_color(i, g_y_dots-1, currentcolor);
		}
		break;
	case DIFFUSION_SQUARE: /* Large square that fills the screen */
		if (g_x_dots > g_y_dots)
		{
			for (i = 0; i < g_y_dots; i++)
			{
				g_plot_color_put_color(g_x_dots/2-g_y_dots/2 , i , currentcolor);
				g_plot_color_put_color(g_x_dots/2 + g_y_dots/2 , i , currentcolor);
				g_plot_color_put_color(g_x_dots/2-g_y_dots/2 + i , 0 , currentcolor);
				g_plot_color_put_color(g_x_dots/2-g_y_dots/2 + i , g_y_dots-1 , currentcolor);
			}
		}
		else
		{
			for (i = 0; i < g_x_dots; i++)
			{
				g_plot_color_put_color(0 , g_y_dots/2-g_x_dots/2 + i , currentcolor);
				g_plot_color_put_color(g_x_dots-1 , g_y_dots/2-g_x_dots/2 + i , currentcolor);
				g_plot_color_put_color(i , g_y_dots/2-g_x_dots/2 , currentcolor);
				g_plot_color_put_color(i , g_y_dots/2 + g_x_dots/2 , currentcolor);
			}
		}
		break;
	}

	while (true)
	{
		switch (mode)
		{
		case DIFFUSION_CENTRAL: /* Release new point on a circle inside the box */
			angle = 2*double(rand())/(RAND_MAX/MathUtil::Pi);
			FPUsincos(&angle, &sine, &cosine);
			x = int(cosine*(x_max-x_min) + g_x_dots);
			y = int(sine  *(y_max-y_min) + g_y_dots);
			x /= 2;
			y /= 2;
			break;
		case DIFFUSION_LINE: /* Release new point on the line y_min somewhere between x_min and x_max */
			y = y_min;
			x = RANDOM(x_max-x_min) + (g_x_dots-x_max + x_min)/2;
			break;
		case DIFFUSION_SQUARE: /* Release new point on a circle inside the box with radius
					given by the radius variable */
			angle = 2*double(rand())/(RAND_MAX/MathUtil::Pi);
			FPUsincos(&angle, &sine, &cosine);
			x = int(cosine*radius + g_x_dots);
			y = int(sine  *radius + g_y_dots);
			x /= 2;
			y /= 2;
			break;
		}

		/* Loop as long as the point (x, y) is surrounded by color 0 */
		/* on all eight sides                                       */

		while ((getcolor(x + 1, y + 1) == 0) && (getcolor(x + 1, y) == 0) &&
			(getcolor(x + 1, y-1) == 0) && (getcolor(x  , y + 1) == 0) &&
			(getcolor(x  , y-1) == 0) && (getcolor(x-1, y + 1) == 0) &&
			(getcolor(x-1, y) == 0) && (getcolor(x-1, y-1) == 0))
		{
			/* Erase moving point */
			if (g_show_orbit)
			{
				g_plot_color_put_color(x, y, 0);
			}

			if (mode == DIFFUSION_CENTRAL) /* Make sure point is inside the box */
			{
				if (x == x_max)
				{
					x--;
				}
				else if (x == x_min)
				{
					x++;
				}
				if (y == y_max)
				{
					y--;
				}
				else if (y == y_min)
				{
					y++;
				}
			}

			if (mode == DIFFUSION_LINE) /* Make sure point is on the screen below g_y_min, but
							we need a 1 pixel margin because of the next random step.*/
			{
				if (x >= g_x_dots-1)
				{
					x--;
				}
				else if (x <= 1)
				{
					x++;
				}
				if (y < y_min)
				{
					y++;
				}
			}

			/* Take one random step */
			x += RANDOM(3) - 1;
			y += RANDOM(3) - 1;

			/* Check keyboard */
			if ((++s_plasma_check & 0x7f) == 1)
			{
				if (check_key())
				{
					alloc_resume(20, 1);
					if (mode != DIFFUSION_SQUARE)
					{
						put_resume(sizeof(x_max), &x_max, sizeof(x_min), &x_min,
							sizeof(y_max), &y_max, sizeof(y_min), &y_min, 0);
					}
					else
					{
						put_resume(sizeof(x_max), &x_max, sizeof(x_min), &x_min,
							sizeof(y_max), &y_max, sizeof(radius), &radius, 0);
					}
					s_plasma_check--;
					return 1;
				}
			}

			/* Show the moving point */
			if (g_show_orbit)
			{
				g_plot_color_put_color(x, y, RANDOM(g_colors-1) + 1);
			}

		} /* End of loop, now fix the point */

		/* If we're doing colorshifting then use currentcolor, otherwise
			pick one at random */
		g_plot_color_put_color(x, y, colorshift ? currentcolor : RANDOM(g_colors-1) + 1);

		/* If we're doing colorshifting then check to see if we need to shift*/
		if (colorshift)
		{
			if (!--colorcount) /* If the counter reaches zero then shift*/
			{
				currentcolor++;      /* Increase the current color and wrap */
				currentcolor %= g_colors;  /* around skipping zero */
				if (!currentcolor)
				{
					currentcolor++;
				}
				colorcount = colorshift;  /* and reset the counter */
			}
		}

		/* If the new point is close to an edge, we may need to increase
			some limits so that the limits expand to match the growing
			fractal. */

		switch (mode)
		{
		case DIFFUSION_CENTRAL:
			if (((x + border) > x_max) || ((x-border) < x_min)
				|| ((y-border) < y_min) || ((y + border) > y_max))
			{
				/* Increase box size, but not past the edge of the screen */
				y_min--;
				y_max++;
				x_min--;
				x_max++;
				if ((y_min == 0) || (x_min == 0))
				{
					return 0;
				}
			}
			break;
		case DIFFUSION_LINE: /* Decrease g_y_min, but not past top of screen */
			if (y-border < y_min)
			{
				y_min--;
			}
			if (y_min == 0)
			{
				return 0;
			}
			break;
		case DIFFUSION_SQUARE: /* Decrease the radius where points are released to stay away
					from the fractal.  It might be decreased by 1 or 2 */
			r = sqr(float(x)-g_x_dots/2) + sqr(float(y)-g_y_dots/2);
			if (r <= border*border)
			{
				return 0;
			}
			while ((radius-border)*(radius-border) > r)
			{
				radius--;
			}
			break;
		}
	}
}



/************ standalone engine for "bifurcation" types ***************/

/***************************************************************/
/* The following code now forms a generalised Fractal Engine   */
/* for Bifurcation fractal typeS.  By rights it now belongs in */
/* CALCFRACT.C, but it's easier for me to leave it here !      */

/* Original code by Phil Wilson, hacked around by Kev Allen.   */

/* Besides generalisation, enhancements include Periodicity    */
/* Checking during the plotting phase (AND halfway through the */
/* filter cycle, if possible, to halve calc times), quicker    */
/* floating-point calculations for the standard Verhulst type, */
/* and new bifurcation types (integer bifurcation, f.p & int   */
/* biflambda - the real equivalent of complex Lambda sets -    */
/* and f.p renditions of bifurcations of r*sin(Pi*p), which    */
/* spurred Mitchel Feigenbaum on to discover his Number).      */

/* To add further types, extend the g_fractal_specific[] array in */
/* usual way, with Bifurcation as the engine, and the name of  */
/* the routine that calculates the next bifurcation generation */
/* as the "orbitcalc" routine in the g_fractal_specific[] entry.  */

/* Bifurcation "orbitcalc" routines get called once per screen */
/* pixel column.  They should calculate the next generation    */
/* from the doubles s_rate & s_population (or the longs s_rate_l &    */
/* s_population_l if they use integer math), placing the result   */
/* back in s_population (or s_population_l).  They should return 0  */
/* if all is ok, or any non-zero value if calculation bailout  */
/* is desirable (eg in case of errors, or the series tending   */
/* to infinity).                Have fun !                     */
/***************************************************************/

int bifurcation()
{
	unsigned long array_size;
	int row;
	int column;
	column = 0;
	if (g_resuming)
	{
		start_resume();
		get_resume(sizeof(column), &column, 0);
		end_resume();
	}
	array_size =  (g_y_stop + 1)*sizeof(int); /* should be g_y_stop + 1 */
	s_verhulst_array = new int[array_size];
	if (s_verhulst_array == 0)
	{
		stop_message(STOPMSG_NORMAL, "Insufficient free memory for calculation.");
		return -1;
	}

	s_pi_l = long(MathUtil::Pi*g_fudge);

	for (row = 0; row <= g_y_stop; row++) /* should be g_y_stop */
	{
		s_verhulst_array[row] = 0;
	}

	s_mono = false;
	if (s_mono)
	{
		if (g_inside)
		{
			s_outside_x = 0;
			g_inside = 1;
		}
		else
		{
			s_outside_x = 1;
		}
	}

	s_filter_cycles = (g_parameter.x <= 0) ? DEFAULT_FILTER : long(g_parameter.x);
	s_half_time_check = false;
	if (g_periodicity_check && (unsigned long)g_max_iteration < s_filter_cycles)
	{
		s_filter_cycles = (s_filter_cycles - g_max_iteration + 1)/2;
		s_half_time_check = true;
	}

	if (g_integer_fractal)
	{
		g_initial_z_l.y = g_escape_time_state.m_grid_l.y_max() - g_y_stop*g_escape_time_state.m_grid_l.delta_y();            /* Y-value of    */
	}
	else
	{
		g_initial_z.y = double(g_escape_time_state.m_grid_fp.y_max() - g_y_stop*g_escape_time_state.m_grid_fp.delta_y()); /* bottom pixels */
	}

	while (column <= g_x_stop)
	{
		if (driver_key_pressed())
		{
			delete[] s_verhulst_array;
			alloc_resume(10, 1);
			put_resume(sizeof(column), &column, 0);
			return -1;
		}

		if (g_integer_fractal)
		{
			s_rate_l = g_escape_time_state.m_grid_l.x_min() + column*g_escape_time_state.m_grid_l.delta_x();
		}
		else
		{
			s_rate = double(g_escape_time_state.m_grid_fp.x_min() + column*g_escape_time_state.m_grid_fp.delta_x());
		}
		verhulst();        /* calculate array once per column */

		for (row = g_y_stop; row >= 0; row--) /* should be g_y_stop & >= 0 */
		{
			int color;
			color = s_verhulst_array[row];
			if (color && s_mono)
			{
				color = g_inside;
			}
			else if ((!color) && s_mono)
			{
				color = s_outside_x;
			}
			else if (color >= g_colors)
			{
				color = g_colors-1;
			}
			s_verhulst_array[row] = 0;
			(*g_plot_color)(column, row, color); /* was row-1, but that's not right? */
		}
		column++;
	}
	delete[] s_verhulst_array;
	return 0;
}

static void verhulst()          /* P. F. Verhulst (1845) */
{
	unsigned int pixel_row;
	bool errors;
	unsigned long counter;

	if (g_integer_fractal)
	{
		s_population_l = (g_parameter.y == 0) ? long(SEED*g_fudge) : long(g_parameter.y*g_fudge);
	}
	else
	{
		s_population = (g_parameter.y == 0) ? SEED : g_parameter.y;
	}

	errors = false;
	g_overflow = false;

	for (counter = 0; counter < s_filter_cycles; counter++)
	{
		errors = (g_current_fractal_specific->orbitcalc() != 0);
		if (errors)
		{
			return;
		}
	}
	if (s_half_time_check) /* check for periodicity at half-time */
	{
		bifurcation_period_init();
		for (counter = 0; counter < (unsigned long)g_max_iteration; counter++)
		{
			errors = (g_current_fractal_specific->orbitcalc() != 0);
			if (errors)
			{
				return;
			}
			if (g_periodicity_check && bifurcation_periodic(counter))
			{
				break;
			}
		}
		if (counter >= (unsigned long)g_max_iteration)   /* if not periodic, go the distance */
		{
			for (counter = 0; counter < s_filter_cycles; counter++)
			{
				errors = (g_current_fractal_specific->orbitcalc() != 0);
				if (errors)
				{
					return;
				}
			}
		}
	}

	if (g_periodicity_check)
	{
		bifurcation_period_init();
	}
	for (counter = 0; counter < (unsigned long)g_max_iteration; counter++)
	{
		errors = (g_current_fractal_specific->orbitcalc() != 0);
		if (errors)
		{
			return;
		}

		/* assign population value to Y coordinate in pixels */
		pixel_row = g_integer_fractal
			? (g_y_stop - int((s_population_l - g_initial_z_l.y)/g_escape_time_state.m_grid_l.delta_y()))
			: (g_y_stop - int((s_population - g_initial_z.y)/g_escape_time_state.m_grid_fp.delta_y()));

		/* if it's visible on the screen, save it in the column array */
		if (pixel_row <= (unsigned int)g_y_stop)
		{
			s_verhulst_array[pixel_row] ++;
		}
		if (g_periodicity_check && bifurcation_periodic(counter))
		{
			if (pixel_row <= (unsigned int)g_y_stop)
			{
				s_verhulst_array[pixel_row] --;
			}
			break;
		}
	}
}

static void bifurcation_period_init()
{
	s_bifurcation_saved_increment = 1;
	s_bifurcation_saved_mask = 1;
	if (g_integer_fractal)
	{
		s_bifurcation_saved_population_l = -1;
		s_bifurcation_close_enough_l = g_escape_time_state.m_grid_l.delta_y()/8;
	}
	else
	{
		s_bifurcation_saved_population = -1.0;
		s_bifurcation_close_enough = double(g_escape_time_state.m_grid_fp.delta_y())/8.0;
	}
}

/* Bifurcation s_population Periodicity Check */
/* Returns : 1 if periodicity found, else 0 */
static int bifurcation_periodic(long time)
{
	if ((time & s_bifurcation_saved_mask) == 0)      /* time to save a new value */
	{
		if (g_integer_fractal)
		{
			s_bifurcation_saved_population_l = s_population_l;
		}
		else                   
		{
			s_bifurcation_saved_population =  s_population;
		}
		if (--s_bifurcation_saved_increment == 0)
		{
			s_bifurcation_saved_mask = (s_bifurcation_saved_mask << 1) + 1;
			s_bifurcation_saved_increment = 4;
		}
	}
	else                         /* check against an old save */
	{
		if (g_integer_fractal)
		{
			if (labs(s_bifurcation_saved_population_l-s_population_l) <= s_bifurcation_close_enough_l)
			{
				return 1;
			}
		}
		else
		{
			if (fabs(s_bifurcation_saved_population-s_population) <= s_bifurcation_close_enough)
			{
				return 1;
			}
		}
	}
	return 0;
}

/**********************************************************************/
/*                                                                                                    */
/* The following are Bifurcation "orbitcalc" routines...              */
/*                                                                                                    */
/**********************************************************************/
int bifurcation_lambda() /* Used by lyanupov */
{
	s_population = s_rate*s_population*(1 - s_population);
	return fabs(s_population) > BIG;
}

int bifurcation_verhulst_trig_fp()
{
	g_temp_z.x = s_population;
	g_temp_z.y = 0;
	CMPLXtrig0(g_temp_z, g_temp_z);
	s_population += s_rate*g_temp_z.x*(1 - g_temp_z.x);
	return fabs(s_population) > BIG;
}

int bifurcation_verhulst_trig()
{
#if !defined(XFRACT)
	g_tmp_z_l.x = s_population_l;
	g_tmp_z_l.y = 0;
	LCMPLXtrig0(g_tmp_z_l, g_tmp_z_l);
	g_tmp_z_l.y = g_tmp_z_l.x - multiply(g_tmp_z_l.x, g_tmp_z_l.x, g_bit_shift);
	s_population_l += multiply(s_rate_l, g_tmp_z_l.y, g_bit_shift);
#endif
	return g_overflow;
}

int bifurcation_stewart_trig_fp()
{
	g_temp_z.x = s_population;
	g_temp_z.y = 0;
	CMPLXtrig0(g_temp_z, g_temp_z);
	s_population = (s_rate*g_temp_z.x*g_temp_z.x) - 1.0;
	return fabs(s_population) > BIG;
}

int bifurcation_stewart_trig()
{
#if !defined(XFRACT)
	g_tmp_z_l.x = s_population_l;
	g_tmp_z_l.y = 0;
	LCMPLXtrig0(g_tmp_z_l, g_tmp_z_l);
	s_population_l = multiply(g_tmp_z_l.x, g_tmp_z_l.x, g_bit_shift);
	s_population_l = multiply(s_population_l, s_rate_l,      g_bit_shift);
	s_population_l -= g_fudge;
#endif
	return g_overflow;
}

int bifurcation_set_trig_pi_fp()
{
	g_temp_z.x = s_population*MathUtil::Pi;
	g_temp_z.y = 0;
	CMPLXtrig0(g_temp_z, g_temp_z);
	s_population = s_rate*g_temp_z.x;
	return fabs(s_population) > BIG;
}

int bifurcation_set_trig_pi()
{
#if !defined(XFRACT)
	g_tmp_z_l.x = multiply(s_population_l, s_pi_l, g_bit_shift);
	g_tmp_z_l.y = 0;
	LCMPLXtrig0(g_tmp_z_l, g_tmp_z_l);
	s_population_l = multiply(s_rate_l, g_tmp_z_l.x, g_bit_shift);
#endif
	return g_overflow;
}

int bifurcation_add_trig_pi_fp()
{
	g_temp_z.x = s_population*MathUtil::Pi;
	g_temp_z.y = 0;
	CMPLXtrig0(g_temp_z, g_temp_z);
	s_population += s_rate*g_temp_z.x;
	return fabs(s_population) > BIG;
}

int bifurcation_add_trig_pi()
{
#if !defined(XFRACT)
	g_tmp_z_l.x = multiply(s_population_l, s_pi_l, g_bit_shift);
	g_tmp_z_l.y = 0;
	LCMPLXtrig0(g_tmp_z_l, g_tmp_z_l);
	s_population_l += multiply(s_rate_l, g_tmp_z_l.x, g_bit_shift);
#endif
	return g_overflow;
}

int bifurcation_lambda_trig_fp()
{
	/* s_population = s_rate*fn(s_population)*(1 - fn(s_population)) */
	g_temp_z.x = s_population;
	g_temp_z.y = 0;
	CMPLXtrig0(g_temp_z, g_temp_z);
	s_population = s_rate*g_temp_z.x*(1 - g_temp_z.x);
	return fabs(s_population) > BIG;
}

int bifurcation_lambda_trig()
{
#if !defined(XFRACT)
	g_tmp_z_l.x = s_population_l;
	g_tmp_z_l.y = 0;
	LCMPLXtrig0(g_tmp_z_l, g_tmp_z_l);
	g_tmp_z_l.y = g_tmp_z_l.x - multiply(g_tmp_z_l.x, g_tmp_z_l.x, g_bit_shift);
	s_population_l = multiply(s_rate_l, g_tmp_z_l.y, g_bit_shift);
#endif
	return g_overflow;
}

int bifurcation_may_fp()
{
	/* X = (lambda * X)/(1 + X)^s_beta, from R.May as described in Pickover,
				Computers, Pattern, Chaos, and Beauty, page 153 */
	g_temp_z.x = 1.0 + s_population;
	g_temp_z.x = pow(g_temp_z.x, -s_beta); /* pow in math.h included with mpmath.h */
	s_population = (s_rate*s_population)*g_temp_z.x;
	return fabs(s_population) > BIG;
}

int bifurcation_may()
{
#if !defined(XFRACT)
	g_tmp_z_l.x = s_population_l + g_fudge;
	g_tmp_z_l.y = 0;
	g_parameter2_l.x = s_beta*g_fudge;
	LCMPLXpwr(g_tmp_z_l, g_parameter2_l, g_tmp_z_l);
	s_population_l = multiply(s_rate_l, s_population_l, g_bit_shift);
	s_population_l = divide(s_population_l, g_tmp_z_l.x, g_bit_shift);
#endif
	return g_overflow;
}

bool bifurcation_may_setup()
{
	s_beta = long(g_parameters[2]);
	if (s_beta < 2)
	{
		s_beta = 2;
	}
	g_parameters[2] = double(s_beta);

	timer_engine(g_current_fractal_specific->calculate_type);
	return false;
}

/******************* standalone engine for "popcorn" ********************/
int popcorn()   /* subset of std engine */
{
	int start_row;
	start_row = 0;
	if (g_resuming)
	{
		start_resume();
		get_resume(sizeof(start_row), &start_row, 0);
		end_resume();
	}
	g_input_counter = g_max_input_counter;
	g_plot_color = plot_color_none;
	g_temp_sqr_x = g_temp_sqr_x_l = 0; /* PB added this to cover weird BAILOUTs */
	for (g_row = start_row; g_row <= g_y_stop; g_row++)
	{
		g_reset_periodicity = true;
		for (g_col = 0; g_col <= g_x_stop; g_col++)
		{
			if (standard_fractal() == -1) /* interrupted */
			{
				alloc_resume(10, 1);
				put_resume(sizeof(g_row), &g_row, 0);
				return -1;
			}
			g_reset_periodicity = false;
		}
	}
	g_calculation_status = CALCSTAT_COMPLETED;
	return 0;
}

/******************* standalone engine for "lyapunov" *********************/
/*** Roy Murphy [76376, 721]                                             ***/
/*** revision history:                                                  ***/
/*** initial version: Winter '91                                        ***/
/***    Fall '92 integration of Nicholas Wilt's ASM speedups            ***/
/***    Jan 93' integration with calcfrac() yielding boundary tracing,  ***/
/***    tesseral, and solid guessing, and inversion, inside=nnn         ***/
/**************************************************************************/
int lyapunov()
{
	double a;
	double b;

	if (driver_key_pressed())
	{
		return -1;
	}
	g_overflow = false;
	if (g_parameters[1] == 1)
	{
		s_population = (1.0 + rand())/(2.0 + RAND_MAX);
	}
	else if (g_parameters[1] == 0)
	{
		if (fabs(s_population) > BIG || s_population == 0 || s_population == 1)
		{
			s_population = (1.0 + rand())/(2.0 + RAND_MAX);
		}
	}
	else
	{
		s_population = g_parameters[1];
	}
	(*g_plot_color)(g_col, g_row, 1);
	if (g_invert)
	{
		invert_z(&g_initial_z);
		a = g_initial_z.y;
		b = g_initial_z.x;
	}
	else
	{
		a = g_dy_pixel();
		b = g_dx_pixel();
	}
	g_color = lyapunov_cycles(s_filter_cycles, a, b);
	if (g_inside > 0 && g_color == 0)
	{
		g_color = g_inside;
	}
	else if (g_color >= g_colors)
	{
		g_color = g_colors-1;
	}
	(*g_plot_color)(g_col, g_row, g_color);
	return g_color;
}

bool lyapunov_setup()
{
	/*
		This routine sets up the sequence for forcing the s_rate parameter
		to vary between the two values.  It fills the array s_lyapunov_r_xy[] and
		sets s_lyapunov_length to the length of the sequence.

		The sequence is coded in the bit pattern in an integer.
		Briefly, the sequence starts with an A the leading zero bits
		are ignored and the remaining bit sequence is decoded.  The
		sequence ends with a B.  Not all possible sequences can be
		represented in this manner, but every possible sequence is
		either represented as itself, as a rotation of one of the
		representable sequences, or as the inverse of a representable
		sequence (swapping 0s and 1s in the array.)  Sequences that
		are the rotation and/or inverses of another sequence will generate
		the same lyapunov exponents.

		A few examples follow:
				number    sequence
					0       ab
					1       aab
					2       aabb
					3       aaab
					4       aabbb
					5       aabab
					6       aaabb (this is a duplicate of 4, a rotated inverse)
					7       aaaab
					8       aabbbb  etc.
	*/
	long i;
	int t;

	s_filter_cycles = long(g_parameters[2]);
	if (s_filter_cycles == 0)
	{
		s_filter_cycles = g_max_iteration/2;
	}
	s_lyapunov_length = 1;

	i = long(g_parameters[0]);
	s_lyapunov_r_xy[0] = 1;
	for (t = 31; t >= 0; t--)
	{
		if (i & (1 << t))
		{
			break;
		}
	}
	for (; t >= 0; t--)
	{
		s_lyapunov_r_xy[s_lyapunov_length++] = (i & (1 << t)) != 0;
	}
	s_lyapunov_r_xy[s_lyapunov_length++] = 0;
	if (g_inside < 0)
	{
		stop_message(STOPMSG_NORMAL, "Sorry, inside options other than inside=nnn are not supported by the lyapunov");
		g_inside = 1;
	}
	if (g_user_standard_calculation_mode == CALCMODE_ORBITS)  /* Oops, lyapunov type */
	{
		g_user_standard_calculation_mode = CALCMODE_SINGLE_PASS;  /* doesn't use new & breaks orbits */
		g_standard_calculation_mode = CALCMODE_SINGLE_PASS;
	}
	return true;
}

static int lyapunov_cycles(long filter_cycles, double a, double b)
{
	int color;
	int count;
	int lnadjust;
	long i;
	double lyap;
	double total;
	double temp;
	/* e10 = 22026.4657948  e-10 = 0.0000453999297625 */

	total = 1.0;
	lnadjust = 0;
	for (i = 0; i < filter_cycles; i++)
	{
		for (count = 0; count < s_lyapunov_length; count++)
		{
			s_rate = s_lyapunov_r_xy[count] ? a : b;
			if (g_current_fractal_specific->orbitcalc())
			{
				g_overflow = true;
				goto jumpout;
			}
		}
	}
	for (i = 0; i < g_max_iteration/2; i++)
	{
		for (count = 0; count < s_lyapunov_length; count++)
		{
			s_rate = s_lyapunov_r_xy[count] ? a : b;
			if (g_current_fractal_specific->orbitcalc())
			{
				g_overflow = true;
				goto jumpout;
			}
			temp = fabs(s_rate-2.0*s_rate*s_population);
			total *= temp;
			if (total == 0)
			{
				g_overflow = true;
				goto jumpout;
			}
		}
		while (total > 22026.4657948)
		{
			total *= 0.0000453999297625;
			lnadjust += 10;
		}
		while (total < 0.0000453999297625)
		{
			total *= 22026.4657948;
			lnadjust -= 10;
		}
	}

jumpout:
	temp = log(total) + lnadjust;
	if (g_overflow || total <= 0 || temp > 0)
	{
		color = 0;
	}
	else
	{
		lyap = g_log_palette_mode
			? -temp/(double(s_lyapunov_length)*i)
			: 1 - exp(temp/(double(s_lyapunov_length)*i));
		color = 1 + int(lyap*(g_colors-1));
	}
	return color;
}

std::string precision_format(const char *specifier, int precision)
{
	return str(boost::format("%%.%d%s") % precision % specifier);
}

