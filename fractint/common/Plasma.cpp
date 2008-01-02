#include <algorithm>
#include <string>

#include "port.h"
#include "cmplx.h"
#include "id.h"
#include "externs.h"
#include "prototyp.h"
#include "drivers.h"

#include "diskvid.h"
#include "Plasma.h"

typedef void (*PLOT)(int, int, int);

static void set_plasma_palette();
static U16 adjust(int xa, int ya, int x, int y, int xb, int yb);
static void subdivide(int x1, int y1, int x2, int y2);
static int new_subdivision(int x1, int y1, int x2, int y2, int recur);

static int s_iparm_x;      /* s_iparm_x = g_parameter.x*8 */
static int s_shift_value;  /* shift based on #colors */
static int s_recur1 = 1;
static int s_plasma_colors;
static int s_recur_level = 0;
static int s_plasma_check;                        /* to limit kbd checking */
static U16 (*s_get_pixels)(int, int)  = (U16(*)(int, int))getcolor;
static U16 s_max_plasma;

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

