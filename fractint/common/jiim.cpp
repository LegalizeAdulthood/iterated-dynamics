/*
 * JIIM.C
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

#ifndef USE_VARARGS
#include <stdarg.h>
#else
#include <varargs.h>
#endif

/* see Fractint.cpp for a description of the include hierarchy */
#include "port.h"
#include "prototyp.h"
#include "helpdefs.h"
#include "fractype.h"
#include "drivers.h"
#include "fihelp.h"
#include "calcfrac.h"
#include "diskvid.h"
#include "editpal.h"
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

static int show_numbers = 0;              /* toggle for display of coords */
static char *rect_buff = NULL;
static FILE *file;
static int windows = 0;               /* windows management system */

static int xc, yc;                       /* corners of the window */
static int xd, yd;                       /* dots in the window    */

double g_julia_c_x = BIG;
double g_julia_c_y = BIG;

/* circle routines from Dr. Dobbs June 1990 */
static int xbase, ybase;
static unsigned int xAspect, yAspect;

void SetAspect(double aspect)
{
	xAspect = 0;
	yAspect = 0;
	aspect = fabs(aspect);
	if (aspect != 1.0)
	{
		if (aspect > 1.0)
		{
			yAspect = (unsigned int)(65536.0 / aspect);
		}
		else
		{
			xAspect = (unsigned int)(65536.0*aspect);
		}
	}
}

void _fastcall c_putcolor(int x, int y, int color)
	{
	/* avoid writing outside window */
	if (x < xc || y < yc || x >= xc + xd || y >= yc + yd)
	{
		return;
	}
	if (y >= g_screen_height - show_numbers) /* avoid overwriting coords */
	{
		return;
	}
	if (windows == 2) /* avoid overwriting fractal */
		if (0 <= x && x < g_x_dots && 0 <= y && y < g_y_dots)
		{
			return;
		}
	g_put_color(x, y, color);
	}


int  c_getcolor(int x, int y)
	{
	/* avoid reading outside window */
	if (x < xc || y < yc || x >= xc + xd || y >= yc + yd)
	{
		return 1000;
	}
	if (y >= g_screen_height - show_numbers) /* avoid overreading coords */
	{
		return 1000;
	}
	if (windows == 2) /* avoid overreading fractal */
		if (0 <= x && x < g_x_dots && 0 <= y && y < g_y_dots)
		{
			return 1000;
		}
	return getcolor(x, y);
	}

void circleplot(int x, int y, int color)
{
	if (xAspect == 0)
	{
		if (yAspect == 0)
		{
			c_putcolor(x + xbase, y + ybase, color);
		}
		else
		{
			c_putcolor(x + xbase, (short)(ybase + (((long) y*(long) yAspect) >> 16)), color);
		}
	}
	else
	{
		c_putcolor((int)(xbase + (((long) x*(long) xAspect) >> 16)), y + ybase, color);
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


long ListFront;
long ListBack;
long ListSize;  /* head, tail, size of MIIM Queue */
long lsize;
long lmax;                    /* how many in queue (now, ever) */
int    maxhits = 1;
int    OKtoMIIM;
static int    SecretExperimentalMode;
float luckyx = 0;
float luckyy = 0;

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
	return ListFront == ListBack;
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
	return ((ListFront + 2) % ListSize) == ListBack;
}

void
ClearQueue()
{
	ListFront = ListBack = lsize = lmax = 0;
}


/*
 * Queue functions for MIIM julia:
 * move to JIIM.C when done
 */

int Init_Queue(unsigned long request)
{
	if (driver_diskp())
	{
		stop_message(0, "Don't try this in disk video mode, kids...\n");
		ListSize = 0;
		return 0;
	}

#if 0
	if (xmmquery() && g_debug_mode != DEBUGMODE_USE_DISK)  /* use LARGEST extended mem */
	{
		largest = xmmlongest();
		if (largest > request / 128)
		{
			request   = (unsigned long) largest*128L;
		}
	}
#endif

	for (ListSize = request; ListSize > 1024; ListSize /= 2)
	{
		switch (disk_start_common(ListSize*8, 1, 256))
		{
		case 0:                        /* success */
			ListFront = ListBack = 0;
			lsize = lmax = 0;
			return 1;
		case -1:
			continue;                   /* try smaller queue size */
		case -2:
			ListSize = 0;               /* cancelled by user      */
			return 0;
		}
	}

	/* failed to get memory for MIIM Queue */
	ListSize = 0;
	return 0;
}

void
Free_Queue()
{
	disk_end();
	ListFront = ListBack = ListSize = lsize = lmax = 0;
}

int
PushLong(long x, long y)
{
	if (((ListFront + 1) % ListSize) != ListBack)
	{
		if (disk_to_memory(8*ListFront, sizeof(x), &x) &&
			disk_to_memory(8*ListFront +sizeof(x), sizeof(y), &y))
		{
			ListFront = (ListFront + 1) % ListSize;
			if (++lsize > lmax)
			{
				lmax   = lsize;
				luckyx = (float)x;
				luckyy = (float)y;
			}
			return 1;
		}
	}
	return 0;                    /* fail */
}

int
PushFloat(float x, float y)
{
	if (((ListFront + 1) % ListSize) != ListBack)
	{
		if (disk_to_memory(8*ListFront, sizeof(x), &x) &&
			disk_to_memory(8*ListFront +sizeof(x), sizeof(y), &y))
		{
			ListFront = (ListFront + 1) % ListSize;
			if (++lsize > lmax)
			{
				lmax   = lsize;
				luckyx = x;
				luckyy = y;
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
		ListFront--;
		if (ListFront < 0)
		{
			ListFront = ListSize - 1;
		}
		if (disk_from_memory(8*ListFront, sizeof(popx), &popx) &&
			disk_from_memory(8*ListFront +sizeof(popx), sizeof(popy), &popy))
		{
			pop.x = popx;
			pop.y = popy;
			--lsize;
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
		ListFront--;
		if (ListFront < 0)
		{
			ListFront = ListSize - 1;
		}
		if (disk_from_memory(8*ListFront, sizeof(pop.x), &pop.x) &&
			disk_from_memory(8*ListFront +sizeof(pop.x), sizeof(pop.y), &pop.y))
		{
			--lsize;
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

	if (ListBack != ListFront)
	{
		if (disk_from_memory(8*ListBack, sizeof(outx), &outx) &&
			disk_from_memory(8*ListBack +sizeof(outx), sizeof(outy), &outy))
		{
			ListBack = (ListBack + 1) % ListSize;
			out.x = outx;
			out.y = outy;
			lsize--;
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

	if (ListBack != ListFront)
	{
		if (disk_from_memory(8*ListBack, sizeof(out.x), &out.x) &&
			disk_from_memory(8*ListBack +sizeof(out.x), sizeof(out.y), &out.y))
		{
			ListBack = (ListBack + 1) % ListSize;
			lsize--;
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
	rect_buff = (char *) malloc(width*height);
	if (rect_buff != NULL)
	{
		char *buff = rect_buff;
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
	char *buff = rect_buff;
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

void Jiim(int which)         /* called by fractint */
{
	struct affine cvt;
	int exact = 0;
	int count = 0;            /* coloring julia */
	static int mode = 0;      /* point, circle, ... */
	double cr;
	double ci;
	double r;
	int xfactor;
	int yfactor;             /* aspect ratio          */
	int xoff;
	int yoff;                   /* center of the window  */
	int x;
	int y;
	int still;
	int kbdchar = -1;
	long iter;
	int color;
	float zoom;
	int oldsxoffs;
	int oldsyoffs;
	bool savehasinverse;
	int (*oldcalctype)();
	int old_x;
	int old_y;
	double aspect;
	static int randir = 0;
	static int rancnt = 0;
	int actively_computing = 1;
	int first_time = 1;
	int old_debugflag = g_debug_mode;

	/* must use standard fractal or be froth_calc */
	if (g_fractal_specific[g_fractal_type].calculate_type != standard_fractal
			&& g_fractal_specific[g_fractal_type].calculate_type != froth_calc)
	{
		return;
	}
	HelpModeSaver saved_help(JIIM == which ? HELP_JIIM : HELP_ORBITS);
	if (which != JIIM)
	{
		g_has_inverse = true;
	}
	oldsxoffs = g_sx_offset;
	oldsyoffs = g_sy_offset;
	oldcalctype = g_calculate_type;
	show_numbers = 0;
	g_using_jiim = 1;
	g_line_buffer = (BYTE *) malloc(max(g_screen_width, g_screen_height));
	aspect = ((double)g_x_dots*3)/((double)g_y_dots*4);  /* assumes 4:3 */
	actively_computing = 1;
	SetAspect(aspect);
	MouseModeSaver saved_mouse(LOOK_MOUSE_ZOOM_BOX);

	if (which == ORBIT)
	{
		(*PER_IMAGE)();
	}
	else
	{
		color = g_color_bright;
	}

	cursor_new();

	/* Grab memory for Queue/Stack before SaveRect gets it. */
	OKtoMIIM  = 0;
	if (which == JIIM && !(g_debug_mode == DEBUGMODE_NO_MIIM_QUEUE))
	{
		OKtoMIIM = Init_Queue((long)8*1024); /* Queue Set-up Successful? */
	}

	maxhits = 1;
	if (which == ORBIT)
	{
		g_plot_color = c_putcolor;                /* for line with clipping */
	}

	if (g_sx_offset != 0 || g_sy_offset != 0) /* we're in view windows */
	{
		savehasinverse = g_has_inverse;
		g_has_inverse = true;
		SaveRect(0, 0, g_x_dots, g_y_dots);
		g_sx_offset = 0;
		g_sy_offset = 0;
		RestoreRect(0, 0, g_x_dots, g_y_dots);
		g_has_inverse = savehasinverse;
	}

	if (g_x_dots == g_screen_width || g_y_dots == g_screen_height ||
		g_screen_width-g_x_dots < g_screen_width/3 ||
		g_screen_height-g_y_dots < g_screen_height/3 ||
		g_x_dots >= MAXRECT)
	{
		/* this mode puts orbit/julia in an overlapping window 1/3 the size of
			the physical screen */
		windows = 0; /* full screen or large view window */
		xd = g_screen_width / 3;
		yd = g_screen_height / 3;
		xc = xd*2;
		yc = yd*2;
		xoff = xd*5 / 2;
		yoff = yd*5 / 2;
	}
	else if (g_x_dots > g_screen_width/3 && g_y_dots > g_screen_height/3)
	{
		/* Julia/orbit and fractal don't overlap */
		windows = 1;
		xd = g_screen_width-g_x_dots;
		yd = g_screen_height-g_y_dots;
		xc = g_x_dots;
		yc = g_y_dots;
		xoff = xc + xd/2;
		yoff = yc + yd/2;
	}
	else
	{
		/* Julia/orbit takes whole screen */
		windows = 2;
		xd = g_screen_width;
		yd = g_screen_height;
		xc = 0;
		yc = 0;
		xoff = xd/2;
		yoff = yd/2;
	}

	xfactor = (int)(xd/5.33);
	yfactor = (int)(-yd/4);

	if (windows == 0)
	{
		SaveRect(xc, yc, xd, yd);
	}
	else if (windows == 2)  /* leave the fractal */
	{
		fillrect(g_x_dots, yc, xd-g_x_dots, yd, g_color_dark);
		fillrect(xc   , g_y_dots, g_x_dots, yd-g_y_dots, g_color_dark);
	}
	else  /* blank whole window */
	{
		fillrect(xc, yc, xd, yd, g_color_dark);
	}

	setup_convert_to_screen(&cvt);

	/* reuse last location if inside window */
	g_col = (int)(cvt.a*g_save_c.x + cvt.b*g_save_c.y + cvt.e + .5);
	g_row = (int)(cvt.c*g_save_c.x + cvt.d*g_save_c.y + cvt.f + .5);
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

	old_x = old_y = -1;

	g_col = (int)(cvt.a*cr + cvt.b*ci + cvt.e + .5);
	g_row = (int)(cvt.c*cr + cvt.d*ci + cvt.f + .5);

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
			first_time = 0;
			while (driver_key_pressed())
			{
				cursor_wait_key();
				kbdchar = driver_get_key();

				dcol = drow = 0;
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
					mode = mode ^ 1;
					break;
				case 'l':
				case 'L':
					mode = mode ^ 2;
					break;
				case 'n':
				case 'N':
					show_numbers = 8 - show_numbers;
					if (windows == 0 && show_numbers == 0)
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
					g_col = (int)(cvt.a*cr + cvt.b*ci + cvt.e + .5);
					g_row = (int)(cvt.c*cr + cvt.d*ci + cvt.f + .5);
					dcol = drow = 0;
					break;
				case 'h':   /* hide fractal toggle */
				case 'H':   /* hide fractal toggle */
					if (windows == 2)
					{
						windows = 3;
					}
					else if (windows == 3 && xd == g_screen_width)
					{
						RestoreRect(0, 0, g_x_dots, g_y_dots);
						windows = 2;
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
					if (which == JIIM)
					{
						SecretExperimentalMode = kbdchar - '0';
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
			actively_computing = 1;
			if (show_numbers) /* write coordinates on screen */
			{
				char str[41];
				sprintf(str, "%16.14f %16.14f %3d", cr, ci, getcolor(g_col, g_row));
				if (windows == 0)
				{
					/* show temp msg will clear self if new msg is a
						different length - pad to length 40*/
					while ((int)strlen(str) < 40)
					{
						strcat(str, " ");
					}
					str[40] = 0;
					cursor_hide();
					actively_computing = 1;
					show_temp_message(str);
					cursor_show();
				}
				else
				{
					driver_display_string(5, g_screen_height-show_numbers, WHITE, BLACK, str);
				}
			}
			iter = 1;
			g_old_z.x = g_old_z.y = g_old_z_l.x = g_old_z_l.y = 0;
			g_save_c.x = g_initial_z.x =  cr;
			g_save_c.y = g_initial_z.y =  ci;
			g_initial_z_l.x = (long)(g_initial_z.x*g_fudge);
			g_initial_z_l.y = (long)(g_initial_z.y*g_fudge);

			old_x = old_y = -1;
			/* compute fixed points and use them as starting points of JIIM */
			if (which == JIIM && OKtoMIIM)
			{
				ComplexD f1;
				ComplexD f2;
				ComplexD Sqrt;        /* Fixed points of Julia */

				Sqrt = ComplexSqrtFloat(1 - 4*cr, -4*ci);
				f1.x = (1 + Sqrt.x) / 2;
				f2.x = (1 - Sqrt.x) / 2;
				f1.y =  Sqrt.y / 2;
				f2.y = -Sqrt.y / 2;

				ClearQueue();
				maxhits = 1;
				EnQueueFloat((float)f1.x, (float)f1.y);
				EnQueueFloat((float)f2.x, (float)f2.y);
			}
			if (which == ORBIT)
			{
				PER_PIXEL();
			}
			/* move window if bumped */
			if (windows == 0 && g_col > xc && g_col < xc + xd && g_row > yc && g_row < yc + yd)
			{
				RestoreRect(xc, yc, xd, yd);
				xc = (xc == xd*2) ? 2 : xd*2;
				xoff = xc + xd /  2;
				SaveRect(xc, yc, xd, yd);
			}
			if (windows == 2)
			{
				fillrect(g_x_dots, yc, xd-g_x_dots, yd-show_numbers, g_color_dark);
				fillrect(xc   , g_y_dots, g_x_dots, yd-g_y_dots-show_numbers, g_color_dark);
			}
			else
			{
				fillrect(xc, yc, xd, yd, g_color_dark);
			}
		} /* end if (driver_key_pressed) */

		if (which == JIIM)
		{
			if (!g_has_inverse)
			{
				continue;
			}
			/* If we have MIIM queue allocated, then use MIIM method. */
			if (OKtoMIIM)
			{
				if (QueueEmpty())
				{
					if (maxhits < g_colors - 1 && maxhits < 5 &&
						(luckyx != 0.0 || luckyy != 0.0))
					{
						int i;

						lsize  = lmax   = 0;
						g_old_z.x  = g_new_z.x  = luckyx;
						g_old_z.y  = g_new_z.y  = luckyy;
						luckyx = luckyy = 0.0f;
						for (i = 0; i < 199; i++)
						{
							g_old_z = ComplexSqrtFloat(g_old_z.x - cr, g_old_z.y - ci);
							g_new_z = ComplexSqrtFloat(g_new_z.x - cr, g_new_z.y - ci);
							EnQueueFloat((float)g_new_z.x,  (float)g_new_z.y);
							EnQueueFloat((float)-g_old_z.x, (float)-g_old_z.y);
						}
						maxhits++;
					}
					else
					{
						continue;             /* loop while (still) */
					}
				}

				g_old_z = DeQueueFloat();
				x = (int)(g_old_z.x*xfactor*zoom + xoff);
				y = (int)(g_old_z.y*yfactor*zoom + yoff);
				color = c_getcolor(x, y);
				if (color < maxhits)
				{
					c_putcolor(x, y, color + 1);
					g_new_z = ComplexSqrtFloat(g_old_z.x - cr, g_old_z.y - ci);
					EnQueueFloat((float)g_new_z.x,  (float)g_new_z.y);
					EnQueueFloat((float)-g_new_z.x, (float)-g_new_z.y);
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
					g_old_z.x = g_old_z.y = 0.0; /* avoids math error */
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

				switch (SecretExperimentalMode)
				{
				case SECRETMODE_RANDOM_WALK:                     /* unmodified random walk */
				default:
					if (rand() % 2)
					{
						g_new_z.x = -g_new_z.x;
						g_new_z.y = -g_new_z.y;
					}
					x = (int)(g_new_z.x*xfactor*zoom + xoff);
					y = (int)(g_new_z.y*yfactor*zoom + yoff);
					break;

				case SECRETMODE_ONE_DIRECTION:                     /* always go one direction */
					if (g_save_c.y < 0)
					{
						g_new_z.x = -g_new_z.x;
						g_new_z.y = -g_new_z.y;
					}
					x = (int)(g_new_z.x*xfactor*zoom + xoff);
					y = (int)(g_new_z.y*yfactor*zoom + yoff);
					break;
				case SECRETMODE_ONE_DIR_DRAW_OTHER:                     /* go one dir, draw the other */
					if (g_save_c.y < 0)
					{
						g_new_z.x = -g_new_z.x;
						g_new_z.y = -g_new_z.y;
					}
					x = (int)(-g_new_z.x*xfactor*zoom + xoff);
					y = (int)(-g_new_z.y*yfactor*zoom + yoff);
					break;
				case SECRETMODE_NEGATIVE_MAX_COLOR:                     /* go negative if max color */
					x = (int)(g_new_z.x*xfactor*zoom + xoff);
					y = (int)(g_new_z.y*yfactor*zoom + yoff);
					if (c_getcolor(x, y) == g_colors - 1)
					{
						g_new_z.x = -g_new_z.x;
						g_new_z.y = -g_new_z.y;
						x = (int)(g_new_z.x*xfactor*zoom + xoff);
						y = (int)(g_new_z.y*yfactor*zoom + yoff);
					}
					break;
				case SECRETMODE_POSITIVE_MAX_COLOR:                     /* go positive if max color */
					g_new_z.x = -g_new_z.x;
					g_new_z.y = -g_new_z.y;
					x = (int)(g_new_z.x*xfactor*zoom + xoff);
					y = (int)(g_new_z.y*yfactor*zoom + yoff);
					if (c_getcolor(x, y) == g_colors - 1)
					{
						x = (int)(g_new_z.x*xfactor*zoom + xoff);
						y = (int)(g_new_z.y*yfactor*zoom + yoff);
					}
					break;
				case SECRETMODE_7:
					if (g_save_c.y < 0)
					{
						g_new_z.x = -g_new_z.x;
						g_new_z.y = -g_new_z.y;
					}
					x = (int)(-g_new_z.x*xfactor*zoom + xoff);
					y = (int)(-g_new_z.y*yfactor*zoom + yoff);
					if (iter > 10)
					{
						if (mode == 0)                        /* pixels  */
						{
							c_putcolor(x, y, color);
						}
						else if (mode & 1)            /* circles */
						{
							xbase = x;
							ybase = y;
							circle((int)(zoom*(xd >> 1)/iter), color);
						}
						if ((mode & 2) && x > 0 && y > 0 && old_x > 0 && old_y > 0)
						{
							driver_draw_line(x, y, old_x, old_y, color);
						}
						old_x = x;
						old_y = y;
					}
					x = (int)(g_new_z.x*xfactor*zoom + xoff);
					y = (int)(g_new_z.y*yfactor*zoom + yoff);
					break;
				case SECRETMODE_ZIGZAG:                     /* go in long zig zags */
					if (rancnt >= 300)
					{
						rancnt = -300;
					}
					if (rancnt < 0)
					{
						g_new_z.x = -g_new_z.x;
						g_new_z.y = -g_new_z.y;
					}
					x = (int)(g_new_z.x*xfactor*zoom + xoff);
					y = (int)(g_new_z.y*yfactor*zoom + yoff);
					break;
				case SECRETMODE_RANDOM_RUN:                     /* "random run" */
					switch (randir)
					{
					case 0:             /* go random direction for a while */
						if (rand() % 2)
						{
							g_new_z.x = -g_new_z.x;
							g_new_z.y = -g_new_z.y;
						}
						if (++rancnt > 1024)
						{
							rancnt = 0;
							randir = (rand() % 2) ? 1 : -1;
						}
						break;
					case 1:             /* now go negative dir for a while */
						g_new_z.x = -g_new_z.x;
						g_new_z.y = -g_new_z.y;
						/* fall through */
					case -1:            /* now go positive dir for a while */
						if (++rancnt > 512)
						{
							randir = rancnt = 0;
						}
						break;
					}
					x = (int)(g_new_z.x*xfactor*zoom + xoff);
					y = (int)(g_new_z.y*yfactor*zoom + yoff);
					break;
				} /* end switch SecretMode (sorry about the indentation) */
			} /* end if not MIIM */
		}
		else /* orbits */
		{
			if (iter < g_max_iteration)
			{
				color = (int)iter % g_colors;
				if (g_integer_fractal)
				{
					g_old_z.x = g_old_z_l.x;
					g_old_z.x /= g_fudge;
					g_old_z.y = g_old_z_l.y;
					g_old_z.y /= g_fudge;
				}
				x = (int)((g_old_z.x - g_initial_z.x)*xfactor*3*zoom + xoff);
				y = (int)((g_old_z.y - g_initial_z.y)*yfactor*3*zoom + yoff);
				if ((*ORBITCALC)())
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
				x = y = -1;
				actively_computing = 0;
			}
		}
		if (which == ORBIT || iter > 10)
		{
			if (mode == 0)                  /* pixels  */
			{
				c_putcolor(x, y, color);
			}
			else if (mode & 1)            /* circles */
			{
				xbase = x;
				ybase = y;
				circle((int)(zoom*(xd >> 1)/iter), color);
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
		if (windows == 0)
		{
			RestoreRect(xc, yc, xd, yd);
		}
		else if (windows >= 2)
		{
			if (windows == 2)
			{
				fillrect(g_x_dots, yc, xd-g_x_dots, yd, g_color_dark);
				fillrect(xc   , g_y_dots, g_x_dots, yd-g_y_dots, g_color_dark);
			}
			else
			{
				fillrect(xc, yc, xd, yd, g_color_dark);
			}
			if (windows == 3 && xd == g_screen_width) /* unhide */
			{
				RestoreRect(0, 0, g_x_dots, g_y_dots);
				windows = 2;
			}
			cursor_hide();
			savehasinverse = g_has_inverse;
			g_has_inverse = true;
			SaveRect(0, 0, g_x_dots, g_y_dots);
			g_sx_offset = oldsxoffs;
			g_sy_offset = oldsyoffs;
			RestoreRect(0, 0, g_x_dots, g_y_dots);
			g_has_inverse = savehasinverse;
		}
	}
	cursor_destroy();
#ifdef XFRACT
	cursor_end_mouse_tracking();
#endif
	if (g_line_buffer)
	{
		free(g_line_buffer);
		g_line_buffer = NULL;
	}

	if (rect_buff)
	{
		free(rect_buff);
		rect_buff = NULL;
	}

	g_using_jiim = 0;
	g_calculate_type = oldcalctype;
	g_debug_mode = old_debugflag; /* yo Chuck! */
	if (kbdchar == 's' || kbdchar == 'S')
	{
		g_view_window = false;
		g_view_x_dots = g_view_y_dots = 0;
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
	if (file != NULL)
	{
		fclose(file);
		file = NULL;
		dir_remove(g_temp_dir, g_screen_file);
	}
	show_numbers = 0;
	driver_unget_key(kbdchar);
}
