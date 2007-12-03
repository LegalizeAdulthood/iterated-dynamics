/*
 * jiim.cpp
 *
 * Generates Inverse Julia in real time, lets move a cursor which determines
 * the J-set.
 *
 *  The J-set is generated in a fixed-size window, a third of the screen.
 *
 * The routines used to set/move the cursor and to save/restore the
 * window were "borrowed" from editpal.c (TW - now we *use* the editpal code)
 *     (if you don't know how to write good code, look for someone who does)
 *
 *    JJB  [jbuhler@gidef.edu.ar]
 *    TIW  Tim Wegner
 *    MS   Michael Snyder
 *    KS   Ken Shirriff
 * Revision History:
 *
 *        7-28-92       JJB  Initial release out of editpal.c
 *        7-29-92       JJB  Added SaveRect() & RestoreRect() - now the
 *                           screen is restored after leaving.
 *        7-30-92       JJB  Now, if the cursor goes into the window, the
 *                           window is moved to the other side of the screen.
 *                           Worked from the first time!
 *        10-09-92      TIW  A major rewrite that merged cut routines duplicated
 *                           in EDITPAL.C and added orbits feature.
 *        11-02-92      KS   Made cursor blink
 *        11-18-92      MS   Altered Inverse Julia to use MIIM method.
 *        11-25-92      MS   Modified MIIM support routines to better be
 *                           shared with stand-alone inverse julia in
 *                           LORENZ.C, and to use DISKVID for swap space.
 *        05-05-93      TIW  Boy this change file really got out of date.
 *                           Added orbits capability, various commands, some
 *                           of Dan Farmer's ideas like circles and lines
 *                           connecting orbits points.
 *        12-18-93      TIW  Removed use of float only for orbits, fixed a
 *                           help mode bug.
 *
 */
#include <string.h>
#include <stdarg.h>
#include <string>

#include "port.h"
#include "prototyp.h"
#include "helpdefs.h"
#include "fractype.h"

#include "calcfrac.h"
#include "diskvid.h"
#include "drivers.h"
#include "editpal.h"
#include "fihelp.h"
#include "filesystem.h"
#include "jiim.h"
#include "lorenz.h"
#include "miscfrac.h"
#include "prompts2.h"
#include "realdos.h"

#include "EscapeTime.h"

#define MAXRECT         1024      /* largest width of SaveRect/RestoreRect */

#define SECRETMODE_RANDOM_WALK			0
#define SECRETMODE_ONE_DIRECTION		1
#define SECRETMODE_ONE_DIR_DRAW_OTHER	2
#define SECRETMODE_NEGATIVE_MAX_COLOR	4
#define SECRETMODE_POSITIVE_MAX_COLOR	5
#define SECRETMODE_7					7
#define SECRETMODE_ZIGZAG				8
#define SECRETMODE_RANDOM_RUN			9

static int s_show_numbers = 0;              /* toggle for display of coords */
static char *s_rect_buff = 0;
static int s_windows = 0;               /* windows management system */

static int s_window_corner_x;
static int s_window_corner_y;                       /* corners of the window */
static int s_window_dots_x;
static int s_window_dots_y;                       /* dots in the window    */

double g_julia_c_x = BIG;
double g_julia_c_y = BIG;

/* circle routines from Dr. Dobbs June 1990 */
static int s_x_base;
static int s_y_base;
static unsigned int s_x_aspect;
static unsigned int s_y_aspect;

void SetAspect(double aspect)
{
	s_x_aspect = 0;
	s_y_aspect = 0;
	aspect = fabs(aspect);
	if (aspect != 1.0)
	{
		if (aspect > 1.0)
		{
			s_y_aspect = (unsigned int)(65536.0/aspect);
		}
		else
		{
			s_x_aspect = (unsigned int)(65536.0*aspect);
		}
	}
}

static void plot_color_clip(int x, int y, int color)
{
	/* avoid writing outside window */
	if (x < s_window_corner_x || y < s_window_corner_y || x >= s_window_corner_x + s_window_dots_x || y >= s_window_corner_y + s_window_dots_y)
	{
		return;
	}
	if (y >= g_screen_height - s_show_numbers) /* avoid overwriting coords */
	{
		return;
	}
	if (s_windows == 2) /* avoid overwriting fractal */
	{
		if (0 <= x && x < g_x_dots && 0 <= y && y < g_y_dots)
		{
			return;
		}
	}
	g_plot_color_put_color(x, y, color);
}


int  c_getcolor(int x, int y)
{
	/* avoid reading outside window */
	if (x < s_window_corner_x || y < s_window_corner_y || x >= s_window_corner_x + s_window_dots_x || y >= s_window_corner_y + s_window_dots_y)
	{
		return 1000;
	}
	if (y >= g_screen_height - s_show_numbers) /* avoid overreading coords */
	{
		return 1000;
	}
	if (s_windows == 2) /* avoid overreading fractal */
	{
		if (0 <= x && x < g_x_dots && 0 <= y && y < g_y_dots)
		{
			return 1000;
		}
	}
	return getcolor(x, y);
}

void circleplot(int x, int y, int color)
{
	if (s_x_aspect == 0)
	{
		if (s_y_aspect == 0)
		{
			plot_color_clip(x + s_x_base, y + s_y_base, color);
		}
		else
		{
			plot_color_clip(x + s_x_base, short(s_y_base + ((long(y)*long(s_y_aspect)) >> 16)), color);
		}
	}
	else
	{
		plot_color_clip(int(s_x_base + ((long(x)*long(s_x_aspect)) >> 16)), y + s_y_base, color);
	}
}

void plot8(int x, int y, int color)
{
	circleplot(x, y, color);
	circleplot(-x, y, color);
	circleplot(x, -y, color);
	circleplot(-x, -y, color);
	circleplot(y, x, color);
	circleplot(-y, x, color);
	circleplot(y, -x, color);
	circleplot(-y, -x, color);
}

void circle(int radius, int color)
{
	int x;
	int y;
	int sum;

	x = 0;
	y = radius << 1;
	sum = 0;

	while (x <= y)
	{
		if (!(x & 1))   /* plot if x is even */
		{
			plot8(x >> 1, (y + 1) >> 1, color);
		}
		sum += (x << 1) + 1;
		x++;
		if (sum > 0)
		{
			sum -= (y << 1) - 1;
			y--;
		}
	}
}


/*
 * MIIM section:
 *
 * Global variables and service functions used for computing
 * MIIM Julias will be grouped here (and shared by code in LORENZ.C)
 *
 */


static long s_list_front;
static long s_list_back;
static long s_list_size;  /* head, tail, size of MIIM Queue */
static long s_l_size;
static long s_l_max;                    /* how many in queue (now, ever) */
static int s_max_hits = 1;
static bool s_ok_to_miim;
static int s_secret_experimental_mode;
static float s_lucky_x = 0;
static float s_lucky_y = 0;

static void fillrect(int x, int y, int width, int height, int color)
{
	/* fast version of fillrect */
	if (!g_has_inverse)
	{
		return;
	}
	memset(g_stack, color % g_colors, width);
	while (height-- > 0)
	{
		if (driver_key_pressed()) /* we could do this less often when in fast modes */
		{
			return;
		}
		put_row(x, y++, width, (char *)g_stack);
	}
}

/*
 * Queue/Stack Section:
 *
 * Defines a buffer that can be used as a FIFO queue or LIFO stack.
 */

int
QueueEmpty()            /* True if NO points remain in queue */
{
	return s_list_front == s_list_back;
}

#if 0 /* not used */
int
QueueFull()             /* True if room for NO more points in queue */
{
	return ((ListFront + 1) % ListSize) == ListBack;
}
#endif

int
QueueFullAlmost()       /* True if room for ONE more point in queue */
{
	return ((s_list_front + 2) % s_list_size) == s_list_back;
}

void
ClearQueue()
{
	s_list_front = 0;
	s_list_back = 0;
	s_l_size = 0;
	s_l_max = 0;
}


/*
 * Queue functions for MIIM julia:
 * move to jiim.cpp when done
 */

bool Init_Queue(unsigned long request)
{
	if (driver_diskp())
	{
		stop_message(0, "Don't try this in disk video mode, kids...\n");
		s_list_size = 0;
		return false;
	}

#if 0
	if (xmmquery() && g_debug_mode != DEBUGMODE_USE_DISK)  /* use LARGEST extended mem */
	{
		largest = xmmlongest();
		if (largest > request/128)
		{
			request   = (unsigned long) largest*128L;
		}
	}
#endif

	for (s_list_size = request; s_list_size > 1024; s_list_size /= 2)
	{
		switch (disk_start_common(s_list_size*8, 1, 256))
		{
		case 0:                        /* success */
			s_list_front = 0;
			s_list_back = 0;
			s_l_size = 0;
			s_l_max = 0;
			return true;
		case -1:
			continue;                   /* try smaller queue size */
		case -2:
			s_list_size = 0;               /* cancelled by user      */
			return false;
		}
	}

	/* failed to get memory for MIIM Queue */
	s_list_size = 0;
	return false;
}

void
Free_Queue()
{
	disk_end();
	s_list_front = 0;
	s_list_back = 0;
	s_list_size = 0;
	s_l_size = 0;
	s_l_max = 0;
}

int
PushLong(long x, long y)
{
	if (((s_list_front + 1) % s_list_size) != s_list_back)
	{
		if (disk_to_memory(8*s_list_front, sizeof(x), &x) &&
			disk_to_memory(8*s_list_front +sizeof(x), sizeof(y), &y))
		{
			s_list_front = (s_list_front + 1) % s_list_size;
			if (++s_l_size > s_l_max)
			{
				s_l_max   = s_l_size;
				s_lucky_x = float(x);
				s_lucky_y = float(y);
			}
			return 1;
		}
	}
	return 0;                    /* fail */
}

int
PushFloat(float x, float y)
{
	if (((s_list_front + 1) % s_list_size) != s_list_back)
	{
		if (disk_to_memory(8*s_list_front, sizeof(x), &x) &&
			disk_to_memory(8*s_list_front +sizeof(x), sizeof(y), &y))
		{
			s_list_front = (s_list_front + 1) % s_list_size;
			if (++s_l_size > s_l_max)
			{
				s_l_max   = s_l_size;
				s_lucky_x = x;
				s_lucky_y = y;
			}
			return 1;
		}
	}
	return 0;                    /* fail */
}

ComplexD
PopFloat()
{
	ComplexD pop;
	float popx;
	float popy;

	if (!QueueEmpty())
	{
		s_list_front--;
		if (s_list_front < 0)
		{
			s_list_front = s_list_size - 1;
		}
		if (disk_from_memory(8*s_list_front, sizeof(popx), &popx) &&
			disk_from_memory(8*s_list_front +sizeof(popx), sizeof(popy), &popy))
		{
			pop.x = popx;
			pop.y = popy;
			--s_l_size;
		}
		return pop;
	}
	pop.x = 0;
	pop.y = 0;
	return pop;
}

ComplexL
PopLong()
{
	ComplexL pop;

	if (!QueueEmpty())
	{
		s_list_front--;
		if (s_list_front < 0)
		{
			s_list_front = s_list_size - 1;
		}
		if (disk_from_memory(8*s_list_front, sizeof(pop.x), &pop.x) &&
			disk_from_memory(8*s_list_front +sizeof(pop.x), sizeof(pop.y), &pop.y))
		{
			--s_l_size;
		}
		return pop;
	}
	pop.x = 0;
	pop.y = 0;
	return pop;
}

int
EnQueueFloat(float x, float y)
{
	return PushFloat(x, y);
}

int
EnQueueLong(long x, long y)
{
	return PushLong(x, y);
}

ComplexD
DeQueueFloat()
{
	ComplexD out;
	float outx;
	float outy;

	if (s_list_back != s_list_front)
	{
		if (disk_from_memory(8*s_list_back, sizeof(outx), &outx) &&
			disk_from_memory(8*s_list_back +sizeof(outx), sizeof(outy), &outy))
		{
			s_list_back = (s_list_back + 1) % s_list_size;
			out.x = outx;
			out.y = outy;
			s_l_size--;
		}
		return out;
	}
	out.x = 0;
	out.y = 0;
	return out;
}

ComplexL
DeQueueLong()
{
	ComplexL out;
	out.x = 0;
	out.y = 0;

	if (s_list_back != s_list_front)
	{
		if (disk_from_memory(8*s_list_back, sizeof(out.x), &out.x) &&
			disk_from_memory(8*s_list_back +sizeof(out.x), sizeof(out.y), &out.y))
		{
			s_list_back = (s_list_back + 1) % s_list_size;
			s_l_size--;
		}
		return out;
	}
	out.x = 0;
	out.y = 0;
	return out;
}



static void SaveRect(int x, int y, int width, int height)
{
	if (!g_has_inverse)
	{
		return;
	}
	s_rect_buff = new char[width*height];
	if (s_rect_buff != 0)
	{
		char *buff = s_rect_buff;
		int yoff;

		cursor_hide();
		for (yoff = 0; yoff < height; yoff++)
		{
			get_row(x, y + yoff, width, buff);
			put_row(x, y + yoff, width, (char *) g_stack);
			buff += width;
		}
		cursor_show();
	}
}


static void RestoreRect(int x, int y, int width, int height)
{
	char *buff = s_rect_buff;
	int  yoff;

	if (!g_has_inverse)
	{
		return;
	}

	cursor_hide();
	for (yoff = 0; yoff < height; yoff++)
	{
		put_row(x, y + yoff, width, buff);
		buff += width;
	}
	cursor_show();
}

/*
 * interface to FRACTINT
 */

ComplexD g_save_c = {-3000.0, -3000.0};

class JIIM
{
public:
	JIIM(bool which)
		: m_orbits(which)
	{
	}
	~JIIM()
	{
	}
	void execute();

private:
	bool m_orbits;
};

void Jiim(bool which)
{
	JIIM(which).execute();
}

void JIIM::execute()
{
	affine m_cvt;
	int exact = 0;
	int count = 0;            /* coloring julia */
	static int mode = 0;      /* point, circle, ... */
	double cr;
	double ci;
	double r;
	int x_factor;
	int y_factor;             /* aspect ratio          */
	int x_offset;
	int y_offset;                   /* center of the window  */
	int x;
	int y;
	int still;
	int kbdchar = -1;
	long iter;
	int color;
	float zoom;
	int old_sx_offset;
	int old_sy_offset;
	bool save_has_inverse;
	int (*old_calculate_type)();
	int old_x;
	int old_y;
	double aspect;
	static int random_direction = 0;
	static int random_count = 0;
	bool actively_computing = true;
	bool first_time = true;
	int old_debugflag = g_debug_mode;

	/* must use standard fractal or be froth_calc */
	if (g_fractal_specific[g_fractal_type].calculate_type != standard_fractal
		&& g_fractal_specific[g_fractal_type].calculate_type != froth_calc)
	{
		return;
	}
	HelpModeSaver saved_help(!m_orbits ? HELP_JIIM : HELP_ORBITS);
	if (m_orbits)
	{
		g_has_inverse = true;
	}
	old_sx_offset = g_sx_offset;
	old_sy_offset = g_sy_offset;
	old_calculate_type = g_calculate_type;
	s_show_numbers = 0;
	g_using_jiim = true;
	g_line_buffer = new BYTE[max(g_screen_width, g_screen_height)];
	aspect = (double(g_x_dots)*3)/(double(g_y_dots)*4);  /* assumes 4:3 */
	actively_computing = true;
	SetAspect(aspect);
	MouseModeSaver saved_mouse(LOOK_MOUSE_ZOOM_BOX);

	if (m_orbits)
	{
		(*g_fractal_specific[g_fractal_type].per_image)();
	}
	else
	{
		color = g_color_bright;
	}

	cursor_new();

	/* Grab memory for Queue/Stack before SaveRect gets it. */
	s_ok_to_miim  = false;
	if (!m_orbits && !(g_debug_mode == DEBUGMODE_NO_MIIM_QUEUE))
	{
		s_ok_to_miim = Init_Queue(8*1024); /* Queue Set-up Successful? */
	}

	s_max_hits = 1;
	if (m_orbits)
	{
		g_plot_color = plot_color_clip;                /* for line with clipping */
	}

	if (g_sx_offset != 0 || g_sy_offset != 0) /* we're in view windows */
	{
		save_has_inverse = g_has_inverse;
		g_has_inverse = true;
		SaveRect(0, 0, g_x_dots, g_y_dots);
		g_sx_offset = 0;
		g_sy_offset = 0;
		RestoreRect(0, 0, g_x_dots, g_y_dots);
		g_has_inverse = save_has_inverse;
	}

	if (g_x_dots == g_screen_width || g_y_dots == g_screen_height ||
		g_screen_width-g_x_dots < g_screen_width/3 ||
		g_screen_height-g_y_dots < g_screen_height/3 ||
		g_x_dots >= MAXRECT)
	{
		/* this mode puts orbit/julia in an overlapping window 1/3 the size of
			the physical screen */
		s_windows = 0; /* full screen or large view window */
		s_window_dots_x = g_screen_width/3;
		s_window_dots_y = g_screen_height/3;
		s_window_corner_x = s_window_dots_x*2;
		s_window_corner_y = s_window_dots_y*2;
		x_offset = s_window_dots_x*5/2;
		y_offset = s_window_dots_y*5/2;
	}
	else if (g_x_dots > g_screen_width/3 && g_y_dots > g_screen_height/3)
	{
		/* Julia/orbit and fractal don't overlap */
		s_windows = 1;
		s_window_dots_x = g_screen_width-g_x_dots;
		s_window_dots_y = g_screen_height-g_y_dots;
		s_window_corner_x = g_x_dots;
		s_window_corner_y = g_y_dots;
		x_offset = s_window_corner_x + s_window_dots_x/2;
		y_offset = s_window_corner_y + s_window_dots_y/2;
	}
	else
	{
		/* Julia/orbit takes whole screen */
		s_windows = 2;
		s_window_dots_x = g_screen_width;
		s_window_dots_y = g_screen_height;
		s_window_corner_x = 0;
		s_window_corner_y = 0;
		x_offset = s_window_dots_x/2;
		y_offset = s_window_dots_y/2;
	}

	x_factor = int(s_window_dots_x/5.33);
	y_factor = int(-s_window_dots_y/4);

	if (s_windows == 0)
	{
		SaveRect(s_window_corner_x, s_window_corner_y, s_window_dots_x, s_window_dots_y);
	}
	else if (s_windows == 2)  /* leave the fractal */
	{
		fillrect(g_x_dots, s_window_corner_y, s_window_dots_x-g_x_dots, s_window_dots_y, g_color_dark);
		fillrect(s_window_corner_x   , g_y_dots, g_x_dots, s_window_dots_y-g_y_dots, g_color_dark);
	}
	else  /* blank whole window */
	{
		fillrect(s_window_corner_x, s_window_corner_y, s_window_dots_x, s_window_dots_y, g_color_dark);
	}

	setup_convert_to_screen(&m_cvt);

	/* reuse last location if inside window */
	g_col = int(m_cvt.a*g_save_c.x + m_cvt.b*g_save_c.y + m_cvt.e + .5);
	g_row = int(m_cvt.c*g_save_c.x + m_cvt.d*g_save_c.y + m_cvt.f + .5);
	if (g_col < 0 || g_col >= g_x_dots ||
		g_row < 0 || g_row >= g_y_dots)
	{
		cr = g_escape_time_state.m_grid_fp.x_center();
		ci = g_escape_time_state.m_grid_fp.y_center();
	}
	else
	{
		cr = g_save_c.x;
		ci = g_save_c.y;
	}

	old_x = -1;
	old_y = -1;

	g_col = int(m_cvt.a*cr + m_cvt.b*ci + m_cvt.e + .5);
	g_row = int(m_cvt.c*cr + m_cvt.d*ci + m_cvt.f + .5);

	/* possible extraseg arrays have been trashed, so set up again */
	// TODO: is this necessary anymore?  extraseg is dead!
	g_integer_fractal ? g_escape_time_state.fill_grid_l() : g_escape_time_state.fill_grid_fp();

	cursor_set_position(g_col, g_row);
	cursor_show();
	color = g_color_bright;

	iter = 1;
	still = 1;
	zoom = 1;

#ifdef XFRACT
	cursor_start_mouse_tracking();
#endif

	while (still)
	{
		int dcol;
		int drow;

		if (actively_computing)
		{
			cursor_check_blink();
		}
		else
		{
			cursor_wait_key();
		}
		if (driver_key_pressed() || first_time) /* prevent burning up UNIX CPU */
		{
			first_time = false;
			while (driver_key_pressed())
			{
				cursor_wait_key();
				kbdchar = driver_get_key();

				dcol = 0;
				drow = 0;
				g_julia_c_x = BIG;
				g_julia_c_y = BIG;
				switch (kbdchar)
				{
				case 1143:    /* ctrl - keypad 5 */
				case 1076:    /* keypad 5        */
					break;     /* do nothing */
				case FIK_CTL_PAGE_UP:
					dcol = 4;
					drow = -4;
					break;
				case FIK_CTL_PAGE_DOWN:
					dcol = 4;
					drow = 4;
					break;
				case FIK_CTL_HOME:
					dcol = -4;
					drow = -4;
					break;
				case FIK_CTL_END:
					dcol = -4;
					drow = 4;
					break;
				case FIK_PAGE_UP:
					dcol = 1;
					drow = -1;
					break;
				case FIK_PAGE_DOWN:
					dcol = 1;
					drow = 1;
					break;
				case FIK_HOME:
					dcol = -1;
					drow = -1;
					break;
				case FIK_END:
					dcol = -1;
					drow = 1;
					break;
				case FIK_UP_ARROW:
					drow = -1;
					break;
				case FIK_DOWN_ARROW:
					drow = 1;
					break;
				case FIK_LEFT_ARROW:
					dcol = -1;
					break;
				case FIK_RIGHT_ARROW:
					dcol = 1;
					break;
				case FIK_CTL_UP_ARROW:
					drow = -4;
					break;
				case FIK_CTL_DOWN_ARROW:
					drow = 4;
					break;
				case FIK_CTL_LEFT_ARROW:
					dcol = -4;
					break;
				case FIK_CTL_RIGHT_ARROW:
					dcol = 4;
					break;
				case 'z':
				case 'Z':
					zoom = 1.0f;
					break;
				case '<':
				case ',':
					zoom /= 1.15f;
					break;
				case '>':
				case '.':
					zoom *= 1.15f;
					break;
				case FIK_SPACE:
					g_julia_c_x = cr;
					g_julia_c_y = ci;
					goto finish;
					/* break; */
				case 'c':   /* circle toggle */
				case 'C':   /* circle toggle */
					mode ^= 1;
					break;
				case 'l':
				case 'L':
					mode ^= 2;
					break;
				case 'n':
				case 'N':
					s_show_numbers = 8 - s_show_numbers;
					if (s_windows == 0 && s_show_numbers == 0)
					{
						cursor_hide();
						clear_temp_message();
						cursor_show();
					}
					break;
				case 'p':
				case 'P':
					get_a_number(&cr, &ci);
					exact = 1;
					g_col = int(m_cvt.a*cr + m_cvt.b*ci + m_cvt.e + .5);
					g_row = int(m_cvt.c*cr + m_cvt.d*ci + m_cvt.f + .5);
					dcol = 0;
					drow = 0;
					break;
				case 'h':   /* hide fractal toggle */
				case 'H':   /* hide fractal toggle */
					if (s_windows == 2)
					{
						s_windows = 3;
					}
					else if (s_windows == 3 && s_window_dots_x == g_screen_width)
					{
						RestoreRect(0, 0, g_x_dots, g_y_dots);
						s_windows = 2;
					}
					break;
#ifdef XFRACT
				case FIK_ENTER:
					break;
#endif
				case '0':
				case '1':
				case '2':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
					if (!m_orbits)
					{
						s_secret_experimental_mode = kbdchar - '0';
						break;
					}
				default:
					still = 0;
				}  /* switch */
				if (kbdchar == 's' || kbdchar == 'S')
				{
					goto finish;
				}
				if (dcol > 0 || drow > 0)
				{
					exact = 0;
				}
				g_col += dcol;
				g_row += drow;
#ifdef XFRACT
				if (kbdchar == FIK_ENTER)
				{
					/* We want to use the position of the cursor */
					exact = 0;
					g_col = cursor_get_x();
					g_row = cursor_get_y();
				}
#endif

				/* keep cursor in logical screen */
				if (g_col >= g_x_dots)
				{
					g_col = g_x_dots -1;
					exact = 0;
				}
				if (g_row >= g_y_dots)
				{
					g_row = g_y_dots -1;
					exact = 0;
				}
				if (g_col < 0)
				{
					g_col = 0;
					exact = 0;
				}
				if (g_row < 0)
				{
					g_row = 0;
					exact = 0;
				}

				cursor_set_position(g_col, g_row);
			}  /* end while (driver_key_pressed) */

			if (exact == 0)
			{
				if (g_integer_fractal)
				{
					cr = g_lx_pixel();
					ci = g_ly_pixel();
					cr /= (1L << g_bit_shift);
					ci /= (1L << g_bit_shift);
				}
				else
				{
					cr = g_dx_pixel();
					ci = g_dy_pixel();
				}
			}
			actively_computing = true;
			if (s_show_numbers) /* write coordinates on screen */
			{
				char str[41];
				sprintf(str, "%16.14f %16.14f %3d", cr, ci, getcolor(g_col, g_row));
				if (s_windows == 0)
				{
					/* show temp msg will clear self if new msg is a
						different length - pad to length 40*/
					while (int(strlen(str)) < 40)
					{
						strcat(str, " ");
					}
					str[40] = 0;
					cursor_hide();
					actively_computing = true;
					show_temp_message(str);
					cursor_show();
				}
				else
				{
					driver_display_string(5, g_screen_height-s_show_numbers, WHITE, BLACK, str);
				}
			}
			iter = 1;
			g_old_z.x = 0;
			g_old_z.y = 0;
			g_old_z_l.x = 0;
			g_old_z_l.y = 0;
			g_save_c.x = cr;
			g_save_c.y = ci;
			g_initial_z.y =  ci;
			g_initial_z.x =  cr;
			g_initial_z_l.x = long(g_initial_z.x*g_fudge);
			g_initial_z_l.y = long(g_initial_z.y*g_fudge);

			old_x = -1;
			old_y = -1;
			/* compute fixed points and use them as starting points of JIIM */
			if (!m_orbits && s_ok_to_miim)
			{
				ComplexD f1;
				ComplexD f2;
				ComplexD Sqrt;        /* Fixed points of Julia */

				Sqrt = ComplexSqrtFloat(1 - 4*cr, -4*ci);
				f1.x = (1 + Sqrt.x)/2;
				f2.x = (1 - Sqrt.x)/2;
				f1.y =  Sqrt.y/2;
				f2.y = -Sqrt.y/2;

				ClearQueue();
				s_max_hits = 1;
				EnQueueFloat(float(f1.x), float(f1.y));
				EnQueueFloat(float(f2.x), float(f2.y));
			}
			if (m_orbits)
			{
				g_fractal_specific[g_fractal_type].per_pixel();
			}
			/* move window if bumped */
			if (s_windows == 0 && g_col > s_window_corner_x && g_col < s_window_corner_x + s_window_dots_x && g_row > s_window_corner_y && g_row < s_window_corner_y + s_window_dots_y)
			{
				RestoreRect(s_window_corner_x, s_window_corner_y, s_window_dots_x, s_window_dots_y);
				s_window_corner_x = (s_window_corner_x == s_window_dots_x*2) ? 2 : s_window_dots_x*2;
				x_offset = s_window_corner_x + s_window_dots_x/2;
				SaveRect(s_window_corner_x, s_window_corner_y, s_window_dots_x, s_window_dots_y);
			}
			if (s_windows == 2)
			{
				fillrect(g_x_dots, s_window_corner_y, s_window_dots_x-g_x_dots, s_window_dots_y-s_show_numbers, g_color_dark);
				fillrect(s_window_corner_x   , g_y_dots, g_x_dots, s_window_dots_y-g_y_dots-s_show_numbers, g_color_dark);
			}
			else
			{
				fillrect(s_window_corner_x, s_window_corner_y, s_window_dots_x, s_window_dots_y, g_color_dark);
			}
		} /* end if (driver_key_pressed) */

		if (!m_orbits)
		{
			if (!g_has_inverse)
			{
				continue;
			}
			/* If we have MIIM queue allocated, then use MIIM method. */
			if (s_ok_to_miim)
			{
				if (QueueEmpty())
				{
					if (s_max_hits < g_colors - 1 && s_max_hits < 5 &&
						(s_lucky_x != 0.0 || s_lucky_y != 0.0))
					{
						int i;

						s_l_size = 0;
						s_l_max = 0;
						g_old_z.x = s_lucky_x;
						g_old_z.y = s_lucky_y;
						g_new_z.x = s_lucky_x;
						g_new_z.y = s_lucky_y;
						s_lucky_x = 0.0f;
						s_lucky_y = 0.0f;
						for (i = 0; i < 199; i++)
						{
							g_old_z = ComplexSqrtFloat(g_old_z.x - cr, g_old_z.y - ci);
							g_new_z = ComplexSqrtFloat(g_new_z.x - cr, g_new_z.y - ci);
							EnQueueFloat(float(g_new_z.x),  float(g_new_z.y));
							EnQueueFloat(float(-g_old_z.x), float(-g_old_z.y));
						}
						s_max_hits++;
					}
					else
					{
						continue;             /* loop while (still) */
					}
				}

				g_old_z = DeQueueFloat();
				x = int(g_old_z.x*x_factor*zoom + x_offset);
				y = int(g_old_z.y*y_factor*zoom + y_offset);
				color = c_getcolor(x, y);
				if (color < s_max_hits)
				{
					plot_color_clip(x, y, color + 1);
					g_new_z = ComplexSqrtFloat(g_old_z.x - cr, g_old_z.y - ci);
					EnQueueFloat(float(g_new_z.x),  float(g_new_z.y));
					EnQueueFloat(float(-g_new_z.x), float(-g_new_z.y));
				}
			}
			else
			{
				/* if not MIIM */
				g_old_z.x -= cr;
				g_old_z.y -= ci;
				r = g_old_z.x*g_old_z.x + g_old_z.y*g_old_z.y;
				if (r > 10.0)
				{
					g_old_z.x = 0.0;
					g_old_z.y = 0.0; /* avoids math error */
					iter = 1;
					r = 0;
				}
				iter++;
				color = ((count++) >> 5) % g_colors; /* chg color every 32 pts */
				if (color == 0)
				{
					color = 1;
				}

				/* r = sqrt(g_old_z.x*g_old_z.x + g_old_z.y*g_old_z.y); calculated above */
				r = sqrt(r);
				g_new_z.x = sqrt(fabs((r + g_old_z.x)/2));
				if (g_old_z.y < 0)
				{
					g_new_z.x = -g_new_z.x;
				}

				g_new_z.y = sqrt(fabs((r - g_old_z.x)/2));

				switch (s_secret_experimental_mode)
				{
				case SECRETMODE_RANDOM_WALK:                     /* unmodified random walk */
				default:
					if (rand() % 2)
					{
						g_new_z.x = -g_new_z.x;
						g_new_z.y = -g_new_z.y;
					}
					x = int(g_new_z.x*x_factor*zoom + x_offset);
					y = int(g_new_z.y*y_factor*zoom + y_offset);
					break;

				case SECRETMODE_ONE_DIRECTION:                     /* always go one direction */
					if (g_save_c.y < 0)
					{
						g_new_z.x = -g_new_z.x;
						g_new_z.y = -g_new_z.y;
					}
					x = int(g_new_z.x*x_factor*zoom + x_offset);
					y = int(g_new_z.y*y_factor*zoom + y_offset);
					break;
				case SECRETMODE_ONE_DIR_DRAW_OTHER:                     /* go one dir, draw the other */
					if (g_save_c.y < 0)
					{
						g_new_z.x = -g_new_z.x;
						g_new_z.y = -g_new_z.y;
					}
					x = int(-g_new_z.x*x_factor*zoom + x_offset);
					y = int(-g_new_z.y*y_factor*zoom + y_offset);
					break;
				case SECRETMODE_NEGATIVE_MAX_COLOR:                     /* go negative if max color */
					x = int(g_new_z.x*x_factor*zoom + x_offset);
					y = int(g_new_z.y*y_factor*zoom + y_offset);
					if (c_getcolor(x, y) == g_colors - 1)
					{
						g_new_z.x = -g_new_z.x;
						g_new_z.y = -g_new_z.y;
						x = int(g_new_z.x*x_factor*zoom + x_offset);
						y = int(g_new_z.y*y_factor*zoom + y_offset);
					}
					break;
				case SECRETMODE_POSITIVE_MAX_COLOR:                     /* go positive if max color */
					g_new_z.x = -g_new_z.x;
					g_new_z.y = -g_new_z.y;
					x = int(g_new_z.x*x_factor*zoom + x_offset);
					y = int(g_new_z.y*y_factor*zoom + y_offset);
					if (c_getcolor(x, y) == g_colors - 1)
					{
						x = int(g_new_z.x*x_factor*zoom + x_offset);
						y = int(g_new_z.y*y_factor*zoom + y_offset);
					}
					break;
				case SECRETMODE_7:
					if (g_save_c.y < 0)
					{
						g_new_z.x = -g_new_z.x;
						g_new_z.y = -g_new_z.y;
					}
					x = int(-g_new_z.x*x_factor*zoom + x_offset);
					y = int(-g_new_z.y*y_factor*zoom + y_offset);
					if (iter > 10)
					{
						if (mode == 0)                        /* pixels  */
						{
							plot_color_clip(x, y, color);
						}
						else if (mode & 1)            /* circles */
						{
							s_x_base = x;
							s_y_base = y;
							circle(int(zoom*(s_window_dots_x >> 1)/iter), color);
						}
						if ((mode & 2) && x > 0 && y > 0 && old_x > 0 && old_y > 0)
						{
							driver_draw_line(x, y, old_x, old_y, color);
						}
						old_x = x;
						old_y = y;
					}
					x = int(g_new_z.x*x_factor*zoom + x_offset);
					y = int(g_new_z.y*y_factor*zoom + y_offset);
					break;
				case SECRETMODE_ZIGZAG:                     /* go in long zig zags */
					if (random_count >= 300)
					{
						random_count = -300;
					}
					if (random_count < 0)
					{
						g_new_z.x = -g_new_z.x;
						g_new_z.y = -g_new_z.y;
					}
					x = int(g_new_z.x*x_factor*zoom + x_offset);
					y = int(g_new_z.y*y_factor*zoom + y_offset);
					break;
				case SECRETMODE_RANDOM_RUN:                     /* "random run" */
					switch (random_direction)
					{
					case 0:             /* go random direction for a while */
						if (rand() % 2)
						{
							g_new_z.x = -g_new_z.x;
							g_new_z.y = -g_new_z.y;
						}
						if (++random_count > 1024)
						{
							random_count = 0;
							random_direction = (rand() % 2) ? 1 : -1;
						}
						break;
					case 1:             /* now go negative dir for a while */
						g_new_z.x = -g_new_z.x;
						g_new_z.y = -g_new_z.y;
						/* fall through */
					case -1:            /* now go positive dir for a while */
						if (++random_count > 512)
						{
							random_direction = 0;
							random_count = 0;
						}
						break;
					}
					x = int(g_new_z.x*x_factor*zoom + x_offset);
					y = int(g_new_z.y*y_factor*zoom + y_offset);
					break;
				} /* end switch SecretMode (sorry about the indentation) */
			} /* end if not MIIM */
		}
		else /* orbits */
		{
			if (iter < g_max_iteration)
			{
				color = int(iter) % g_colors;
				if (g_integer_fractal)
				{
					g_old_z.x = g_old_z_l.x;
					g_old_z.x /= g_fudge;
					g_old_z.y = g_old_z_l.y;
					g_old_z.y /= g_fudge;
				}
				x = int((g_old_z.x - g_initial_z.x)*x_factor*3*zoom + x_offset);
				y = int((g_old_z.y - g_initial_z.y)*y_factor*3*zoom + y_offset);
				if ((*g_fractal_specific[g_fractal_type].orbitcalc)())
				{
					iter = g_max_iteration;
				}
				else
				{
					iter++;
				}
			}
			else
			{
				x = -1;
				y = -1;
				actively_computing = false;
			}
		}
		if (m_orbits || iter > 10)
		{
			if (mode == 0)                  /* pixels  */
			{
				plot_color_clip(x, y, color);
			}
			else if (mode & 1)            /* circles */
			{
				s_x_base = x;
				s_y_base = y;
				circle(int(zoom*(s_window_dots_x >> 1)/iter), color);
			}
			if ((mode & 2) && x > 0 && y > 0 && old_x > 0 && old_y > 0)
			{
				driver_draw_line(x, y, old_x, old_y, color);
			}
			old_x = x;
			old_y = y;
		}
		g_old_z = g_new_z;
		g_old_z_l = g_new_z_l;
	} /* end while (still) */

finish:
	Free_Queue();

	if (kbdchar != 's' && kbdchar != 'S')
	{
		cursor_hide();
		if (s_windows == 0)
		{
			RestoreRect(s_window_corner_x, s_window_corner_y, s_window_dots_x, s_window_dots_y);
		}
		else if (s_windows >= 2)
		{
			if (s_windows == 2)
			{
				fillrect(g_x_dots, s_window_corner_y, s_window_dots_x-g_x_dots, s_window_dots_y, g_color_dark);
				fillrect(s_window_corner_x   , g_y_dots, g_x_dots, s_window_dots_y-g_y_dots, g_color_dark);
			}
			else
			{
				fillrect(s_window_corner_x, s_window_corner_y, s_window_dots_x, s_window_dots_y, g_color_dark);
			}
			if (s_windows == 3 && s_window_dots_x == g_screen_width) /* unhide */
			{
				RestoreRect(0, 0, g_x_dots, g_y_dots);
				s_windows = 2;
			}
			cursor_hide();
			save_has_inverse = g_has_inverse;
			g_has_inverse = true;
			SaveRect(0, 0, g_x_dots, g_y_dots);
			g_sx_offset = old_sx_offset;
			g_sy_offset = old_sy_offset;
			RestoreRect(0, 0, g_x_dots, g_y_dots);
			g_has_inverse = save_has_inverse;
		}
	}
	cursor_destroy();
#ifdef XFRACT
	cursor_end_mouse_tracking();
#endif
	if (g_line_buffer)
	{
		delete[] g_line_buffer;
		g_line_buffer = 0;
	}

	if (s_rect_buff)
	{
		delete[] s_rect_buff;
		s_rect_buff = 0;
	}

	g_using_jiim = false;
	g_calculate_type = old_calculate_type;
	g_debug_mode = old_debugflag; /* yo Chuck! */
	if (kbdchar == 's' || kbdchar == 'S')
	{
		g_view_window = false;
		g_view_x_dots = 0;
		g_view_y_dots = 0;
		g_view_reduction = 4.2f;
		g_view_crop = true;
		g_final_aspect_ratio = g_screen_aspect_ratio;
		g_x_dots = g_screen_width;
		g_y_dots = g_screen_height;
		g_dx_size = g_x_dots - 1;
		g_dy_size = g_y_dots - 1;
		g_sx_offset = 0;
		g_sy_offset = 0;
		free_temp_message();
	}
	else
	{
		clear_temp_message();
	}
	s_show_numbers = 0;
	driver_unget_key(kbdchar);
}
