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
#include <boost/format.hpp>

#include "port.h"
#include "prototyp.h"
#include "helpdefs.h"
#include "fractype.h"

#include "calcfrac.h"
#include "diskvid.h"
#include "drivers.h"
#include "editpal.h"
#include "EscapeTime.h"
#include "Externals.h"
#include "filesystem.h"
#include "FrothyBasin.h"
#include "idhelp.h"
#include "jiim.h"
#include "lorenz.h"
#include "miscfrac.h"
#include "prompts2.h"
#include "realdos.h"
#include "StopMessage.h"
#include "TextColors.h"
#include "ViewWindow.h"

class JIIM
{
public:
	JIIM(bool which)
		: _orbits(which),
		_exact(false),
		_xCenter(0),
		_yCenter(0),
		_dColumn(0),
		_dRow(0),
		_convert(),
		_zoom(1.0f),
		_cReal(0.0),
		_cImag(0.0),
		_again(false)
	{
	}
	void Execute();
	void WriteCoordinatesOnScreen(bool &actively_computing);

	void UpdateColumnRow();

	bool ProcessKeyPress(int kbdchar);

	void UpdateCursor(bool actively_computing);

	void SizeWindow();


private:
	enum
	{
		MAXRECT = 1024      // largest width of SaveRect/RestoreRect
	};
	enum SecretMode
	{
		SECRETMODE_RANDOM_WALK			= 0,
		SECRETMODE_ONE_DIRECTION		= 1,
		SECRETMODE_ONE_DIR_DRAW_OTHER	= 2,
		SECRETMODE_NEGATIVE_MAX_COLOR	= 4,
		SECRETMODE_POSITIVE_MAX_COLOR	= 5,
		SECRETMODE_7					= 7,
		SECRETMODE_ZIGZAG				= 8,
		SECRETMODE_RANDOM_RUN			= 9
	};
	enum JIIMMode
	{
		JIIM_PIXEL = 0,
		JIIM_CIRCLE = 1,
		JIIM_LINE = 2
	};

	bool _orbits;
	bool _exact;
	static int s_mode;
	int _xCenter;
	int _yCenter; // center of window
	int _dColumn;
	int _dRow;
	affine _convert;
	float _zoom;
	double _cReal;
	double _cImag;
	bool _again;

	bool UsesStandardFractalOrFrothCalc();
};

int JIIM::s_mode = JIIM_PIXEL;

static int s_show_numbers = 0;              // toggle for display of coords
static std::vector<BYTE> s_rect_buff;
static int s_windows = 0;               // windows management system

static int s_window_corner_x;
static int s_window_corner_y;                       // corners of the window
static int s_window_dots_x;
static int s_window_dots_y;                       // dots in the window

StdComplexD g_julia_c(BIG, BIG);

// circle routines from Dr. Dobbs June 1990
static int s_x_base;
static int s_y_base;
static unsigned int s_x_aspect;
static unsigned int s_y_aspect;

static void SetAspect(double aspect)
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
	// avoid writing outside window
	if (x < s_window_corner_x || y < s_window_corner_y || x >= s_window_corner_x + s_window_dots_x || y >= s_window_corner_y + s_window_dots_y)
	{
		return;
	}
	if (y >= g_screen_height - s_show_numbers) // avoid overwriting coords
	{
		return;
	}
	if (s_windows == 2) // avoid overwriting fractal
	{
		if (0 <= x && x < g_x_dots && 0 <= y && y < g_y_dots)
		{
			return;
		}
	}
	g_plot_color_put_color(x, y, color);
}


static int c_getcolor(int x, int y)
{
	// avoid reading outside window
	if (x < s_window_corner_x || y < s_window_corner_y || x >= s_window_corner_x + s_window_dots_x || y >= s_window_corner_y + s_window_dots_y)
	{
		return 1000;
	}
	if (y >= g_screen_height - s_show_numbers) // avoid overreading coords
	{
		return 1000;
	}
	if (s_windows == 2) // avoid overreading fractal
	{
		if (0 <= x && x < g_x_dots && 0 <= y && y < g_y_dots)
		{
			return 1000;
		}
	}
	return get_color(x, y);
}

static void circle_plot(int x, int y, int color)
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

static void plot8(int x, int y, int color)
{
	circle_plot(x, y, color);
	circle_plot(-x, y, color);
	circle_plot(x, -y, color);
	circle_plot(-x, -y, color);
	circle_plot(y, x, color);
	circle_plot(-y, x, color);
	circle_plot(y, -x, color);
	circle_plot(-y, -x, color);
}

static void circle(int radius, int color)
{
	int x;
	int y;
	int sum;

	x = 0;
	y = radius << 1;
	sum = 0;

	while (x <= y)
	{
		if (!(x & 1))   // plot if x is even
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
static long s_list_size;  // head, tail, size of MIIM Queue
static long s_l_size;
static long s_l_max;                    // how many in queue (now, ever)
static int s_max_hits = 1;
static bool s_ok_to_miim;
static int s_secret_experimental_mode;
static float s_lucky_x = 0;
static float s_lucky_y = 0;

static void fillrect(int x, int y, int width, int height, int color)
{
	// fast version of fillrect
	if (!g_has_inverse)
	{
		return;
	}
	std::vector<BYTE> scanline;
	scanline.resize(width, BYTE(color % g_colors));
	while (height-- > 0)
	{
		if (driver_key_pressed()) // we could do this less often when in fast modes
		{
			return;
		}
		put_row(x, y++, width, &scanline[0]);
	}
}

/*
 * Queue/Stack Section:
 *
 * Defines a buffer that can be used as a FIFO queue or LIFO stack.
 */

int QueueEmpty()            // True if NO points remain in queue
{
	return s_list_front == s_list_back;
}

int QueueFullAlmost()       // True if room for ONE more point in queue
{
	return ((s_list_front + 2) % s_list_size) == s_list_back;
}

static void ClearQueue()
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
		stop_message(STOPMSG_NORMAL, "Don't try this in disk video mode, kids...\n");
		s_list_size = 0;
		return false;
	}

#if 0
	if (xmmquery() && g_debug_mode != DEBUGMODE_USE_DISK)  // use LARGEST extended mem
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
		case 0:                        // success
			s_list_front = 0;
			s_list_back = 0;
			s_l_size = 0;
			s_l_max = 0;
			return true;
		case -1:
			continue;                   // try smaller queue size
		case -2:
			s_list_size = 0;               // cancelled by user
			return false;
		}
	}

	// failed to get memory for MIIM Queue
	s_list_size = 0;
	return false;
}

void Free_Queue()
{
	disk_end();
	s_list_front = 0;
	s_list_back = 0;
	s_list_size = 0;
	s_l_size = 0;
	s_l_max = 0;
}

int PushLong(long x, long y)
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
	return 0;                    // fail
}

int PushFloat(float x, float y)
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
	return 0;                    // fail
}

ComplexD PopFloat()
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
			pop.real(popx);
			pop.imag(popy);
			--s_l_size;
		}
		return pop;
	}
	pop.real(0);
	pop.imag(0);
	return pop;
}

template <typename T>
int disk_from_memory(long offset, ComplexT<T> &dest)
{
	T real;
	int result = disk_from_memory(offset, sizeof(real), &real);
	if (result)
	{
		T imag;
		result = disk_from_memory(offset + sizeof(imag), sizeof(imag), &imag);
		if (result)
		{
			dest.real(real);
			dest.imag(imag);
		}
	}
	return result;
}

ComplexL PopLong()
{
	ComplexL pop;

	if (!QueueEmpty())
	{
		s_list_front--;
		if (s_list_front < 0)
		{
			s_list_front = s_list_size - 1;
		}
		if (disk_from_memory(8*s_list_front, pop))
		{
			--s_l_size;
		}
		return pop;
	}
	pop.real(0L);
	pop.imag(0L);
	return pop;
}

int EnQueueFloat(float x, float y)
{
	return PushFloat(x, y);
}

int EnQueueLong(long x, long y)
{
	return PushLong(x, y);
}

ComplexD DeQueueFloat()
{
	ComplexD out;

	if (s_list_back != s_list_front)
	{
		if (disk_from_memory(8*s_list_back, out))
		{
			s_list_back = (s_list_back + 1) % s_list_size;
			s_l_size--;
		}
		return out;
	}
	out.real(0);
	out.imag(0);
	return out;
}

ComplexL DeQueueLong()
{
	ComplexL out;
	out.real(0);
	out.imag(0);

	if (s_list_back != s_list_front)
	{
		if (disk_from_memory(8*s_list_back, out))
		{
			s_list_back = (s_list_back + 1) % s_list_size;
			s_l_size--;
		}
		return out;
	}
	out.real(0);
	out.imag(0);
	return out;
}

static void SaveRect(int x, int y, int width, int height)
{
	if (!g_has_inverse)
	{
		return;
	}
	s_rect_buff.resize(width*height);
	BYTE *buff = &s_rect_buff[0];

	cursor::cursor_hide();
	for (int yoff = 0; yoff < height; yoff++)
	{
		get_row(x, y + yoff, width, buff);
		put_row(x, y + yoff, width, g_stack);
		buff += width;
	}
	cursor::cursor_show();
}

static void RestoreRect(int x, int y, int width, int height)
{
	BYTE *buff = &s_rect_buff[0];

	if (!g_has_inverse)
	{
		return;
	}

	cursor::cursor_hide();
	for (int yoff = 0; yoff < height; yoff++)
	{
		put_row(x, y + yoff, width, buff);
		buff += width;
	}
	cursor::cursor_show();
}

InitializedComplexD g_save_c(-3000.0, -3000.0);

void Jiim(bool which)
{
	JIIM(which).Execute();
}

bool JIIM::UsesStandardFractalOrFrothCalc()
{
	return (g_fractal_specific[g_fractal_type].calculate_type == standard_fractal
		|| g_fractal_specific[g_fractal_type].calculate_type == froth_calc);
}

void JIIM::Execute()
{
	int count = 0;            // coloring julia
	s_mode = JIIM_PIXEL;
	int x;
	int y;
	int kbdchar = -1;
	long iter;
	int color;
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
	int old_debug_mode = g_debug_mode;

	// must use standard fractal or be froth_calc
	if (!UsesStandardFractalOrFrothCalc())
	{
		return;
	}
	HelpModeSaver saved_help(!_orbits ? FIHELP_JIIM : FIHELP_ORBITS);
	if (_orbits)
	{
		g_has_inverse = true;
	}
	old_sx_offset = g_screen_x_offset;
	old_sy_offset = g_screen_y_offset;
	old_calculate_type = g_calculate_type;
	s_show_numbers = 0;
	g_using_jiim = true;
	g_line_buffer = new BYTE[std::max(g_screen_width, g_screen_height)];
	aspect = (double(g_x_dots)*3)/(double(g_y_dots)*4);  // assumes 4:3
	actively_computing = true;
	SetAspect(aspect);
	MouseModeSaver saved_mouse(LOOK_MOUSE_ZOOM_BOX);

	if (_orbits)
	{
		g_fractal_specific[g_fractal_type].per_image();
	}
	else
	{
		color = g_.DAC().Bright();
	}

	cursor::create();

	// Grab memory for Queue/Stack before SaveRect gets it.
	s_ok_to_miim  = false;
	if (!_orbits && !(g_debug_mode == DEBUGMODE_NO_MIIM_QUEUE))
	{
		s_ok_to_miim = Init_Queue(8*1024); // Queue Set-up Successful?
	}

	s_max_hits = 1;
	if (_orbits)
	{
		g_plot_color = plot_color_clip;                // for line with clipping
	}

	if (g_screen_x_offset != 0 || g_screen_y_offset != 0) // we're in view windows
	{
		save_has_inverse = g_has_inverse;
		g_has_inverse = true;
		SaveRect(0, 0, g_x_dots, g_y_dots);
		g_screen_x_offset = 0;
		g_screen_y_offset = 0;
		RestoreRect(0, 0, g_x_dots, g_y_dots);
		g_has_inverse = save_has_inverse;
	}

	SizeWindow();
	int x_factor = int(s_window_dots_x/5.33);
	int y_factor = int(-s_window_dots_y/4);

	if (s_windows == 0)
	{
		SaveRect(s_window_corner_x, s_window_corner_y, s_window_dots_x, s_window_dots_y);
	}
	else if (s_windows == 2)  // leave the fractal
	{
		fillrect(g_x_dots, s_window_corner_y, s_window_dots_x-g_x_dots, s_window_dots_y, g_.DAC().Dark());
		fillrect(s_window_corner_x   , g_y_dots, g_x_dots, s_window_dots_y-g_y_dots, g_.DAC().Dark());
	}
	else  // blank whole window
	{
		fillrect(s_window_corner_x, s_window_corner_y, s_window_dots_x, s_window_dots_y, g_.DAC().Dark());
	}

	setup_convert_to_screen(&_convert);

	// reuse last location if inside window
	g_col = int(_convert.a*g_save_c.real() + _convert.b*g_save_c.imag() + _convert.e + .5);
	g_row = int(_convert.c*g_save_c.real() + _convert.d*g_save_c.imag() + _convert.f + .5);
	if (g_col < 0 || g_col >= g_x_dots ||
		g_row < 0 || g_row >= g_y_dots)
	{
		_cReal = g_escape_time_state.m_grid_fp.x_center();
		_cImag = g_escape_time_state.m_grid_fp.y_center();
	}
	else
	{
		_cReal = g_save_c.real();
		_cImag = g_save_c.imag();
	}

	old_x = -1;
	old_y = -1;

	g_col = int(_convert.a*_cReal + _convert.b*_cImag + _convert.e + .5);
	g_row = int(_convert.c*_cReal + _convert.d*_cImag + _convert.f + .5);

	// possible extraseg arrays have been trashed, so set up again
	// TODO: is this necessary anymore?  extraseg is dead!
	g_integer_fractal ? g_escape_time_state.fill_grid_l() : g_escape_time_state.fill_grid_fp();

	cursor::cursor_set_position(g_col, g_row);
	cursor::cursor_show();
	color = g_.DAC().Bright();

	iter = 1;
	_again = true;
	_zoom = 1;

#ifdef XFRACT
	cursor_start_mouse_tracking();
#endif

	double r;
	while (_again)
	{
		UpdateCursor(actively_computing);
		if (driver_key_pressed() || first_time) // prevent burning up UNIX CPU
		{
			first_time = false;
			while (driver_key_pressed())
			{
				cursor::cursor_wait_key();
				kbdchar = driver_get_key();

				_dColumn = 0;
				_dRow = 0;
				g_julia_c = StdComplexD(BIG, BIG);
				if (ProcessKeyPress(kbdchar))
				{
					goto finish;
				}

				UpdateColumnRow();
				cursor::cursor_set_position(g_col, g_row);
			}

			if (!_exact)
			{
				if (g_integer_fractal)
				{
					_cReal = FudgeToDouble(g_externs.LxPixel());
					_cImag = FudgeToDouble(g_externs.LyPixel());
				}
				else
				{
					_cReal = g_externs.DxPixel();
					_cImag = g_externs.DyPixel();
				}
			}
			actively_computing = true;
			WriteCoordinatesOnScreen(actively_computing);
			iter = 1;
			g_old_z.real(0);
			g_old_z.imag(0);
			g_old_z_l.real(0);
			g_old_z_l.imag(0);
			g_save_c.real(_cReal);
			g_save_c.imag(_cImag);
			g_initial_z.imag( _cImag);
			g_initial_z.real( _cReal);
			g_initial_z_l = ComplexDoubleToFudge(g_initial_z);

			old_x = -1;
			old_y = -1;
			// compute fixed points and use them as starting points of JIIM
			if (!_orbits && s_ok_to_miim)
			{
				ComplexD f1;
				ComplexD f2;
				ComplexD Sqrt;        // Fixed points of Julia

				Sqrt = ComplexSqrtFloat(1 - 4*_cReal, -4*_cImag);
				f1.real((1 + Sqrt.real())/2);
				f2.real((1 - Sqrt.real())/2);
				f1.imag(Sqrt.imag()/2);
				f2.imag(-Sqrt.imag()/2);

				ClearQueue();
				s_max_hits = 1;
				EnQueueFloat(float(f1.real()), float(f1.imag()));
				EnQueueFloat(float(f2.real()), float(f2.imag()));
			}
			if (_orbits)
			{
				g_fractal_specific[g_fractal_type].per_pixel();
			}
			// move window if bumped
			if (s_windows == 0
				&& g_col > s_window_corner_x
				&& g_col < s_window_corner_x + s_window_dots_x
				&& g_row > s_window_corner_y
				&& g_row < s_window_corner_y + s_window_dots_y)
			{
				RestoreRect(s_window_corner_x, s_window_corner_y, s_window_dots_x, s_window_dots_y);
				s_window_corner_x = (s_window_corner_x == s_window_dots_x*2) ? 2 : s_window_dots_x*2;
				_xCenter = s_window_corner_x + s_window_dots_x/2;
				SaveRect(s_window_corner_x, s_window_corner_y, s_window_dots_x, s_window_dots_y);
			}
			if (s_windows == 2)
			{
				fillrect(g_x_dots, s_window_corner_y, s_window_dots_x-g_x_dots, s_window_dots_y-s_show_numbers, g_.DAC().Dark());
				fillrect(s_window_corner_x   , g_y_dots, g_x_dots, s_window_dots_y-g_y_dots-s_show_numbers, g_.DAC().Dark());
			}
			else
			{
				fillrect(s_window_corner_x, s_window_corner_y, s_window_dots_x, s_window_dots_y, g_.DAC().Dark());
			}
		} // end if (driver_key_pressed)

		if (!_orbits)
		{
			if (!g_has_inverse)
			{
				continue;
			}
			// If we have MIIM queue allocated, then use MIIM method.
			if (s_ok_to_miim)
			{
				if (QueueEmpty())
				{
					if (s_max_hits < g_colors - 1 && s_max_hits < 5 &&
						(s_lucky_x != 0.0 || s_lucky_y != 0.0))
					{
						s_l_size = 0;
						s_l_max = 0;
						g_old_z.real(s_lucky_x);
						g_old_z.imag(s_lucky_y);
						g_new_z.real(s_lucky_x);
						g_new_z.imag(s_lucky_y);
						s_lucky_x = 0.0f;
						s_lucky_y = 0.0f;
						for (int i = 0; i < 199; i++)
						{
							g_old_z = ComplexSqrtFloat(g_old_z.real() - _cReal, g_old_z.imag() - _cImag);
							g_new_z = ComplexSqrtFloat(g_new_z.real() - _cReal, g_new_z.imag() - _cImag);
							EnQueueFloat(float(g_new_z.real()),  float(g_new_z.imag()));
							EnQueueFloat(float(-g_old_z.real()), float(-g_old_z.imag()));
						}
						s_max_hits++;
					}
					else
					{
						continue;             // loop while (still)
					}
				}

				g_old_z = DeQueueFloat();
				x = int(g_old_z.real()*x_factor*_zoom + _xCenter);
				y = int(g_old_z.imag()*y_factor*_zoom + _yCenter);
				color = c_getcolor(x, y);
				if (color < s_max_hits)
				{
					plot_color_clip(x, y, color + 1);
					g_new_z = ComplexSqrtFloat(g_old_z.real() - _cReal, g_old_z.imag() - _cImag);
					EnQueueFloat(float(g_new_z.real()),  float(g_new_z.imag()));
					EnQueueFloat(float(-g_new_z.real()), float(-g_new_z.imag()));
				}
			}
			else
			{
				// if not MIIM
				g_old_z.real(g_old_z.real() - _cReal);
				g_old_z.imag(g_old_z.imag() - _cImag);
				r = g_old_z.real()*g_old_z.real() + g_old_z.imag()*g_old_z.imag();
				if (r > 10.0)
				{
					g_old_z.real(0.0);
					g_old_z.imag(0.0); // avoids math error
					iter = 1;
					r = 0;
				}
				iter++;
				color = ((count++) >> 5) % g_colors; // chg color every 32 pts
				if (color == 0)
				{
					color = 1;
				}

				// r = sqrt(g_old_z.real()*g_old_z.real() + g_old_z.imag()*g_old_z.imag()); calculated above
				r = sqrt(r);
				g_new_z.real(sqrt(fabs((r + g_old_z.real())/2)));
				if (g_old_z.imag() < 0)
				{
					g_new_z.real(-g_new_z.real());
				}

				g_new_z.imag(sqrt(fabs((r - g_old_z.real())/2)));

				switch (s_secret_experimental_mode)
				{
				case SECRETMODE_RANDOM_WALK:                     // unmodified random walk
				default:
					if (rand() % 2)
					{
						g_new_z.real(-g_new_z.real());
						g_new_z.imag(-g_new_z.imag());
					}
					x = int(g_new_z.real()*x_factor*_zoom + _xCenter);
					y = int(g_new_z.imag()*y_factor*_zoom + _yCenter);
					break;

				case SECRETMODE_ONE_DIRECTION:                     // always go one direction
					if (g_save_c.imag() < 0)
					{
						g_new_z.real(-g_new_z.real());
						g_new_z.imag(-g_new_z.imag());
					}
					x = int(g_new_z.real()*x_factor*_zoom + _xCenter);
					y = int(g_new_z.imag()*y_factor*_zoom + _yCenter);
					break;
				case SECRETMODE_ONE_DIR_DRAW_OTHER:                     // go one dir, draw the other
					if (g_save_c.imag() < 0)
					{
						g_new_z.real(-g_new_z.real());
						g_new_z.imag(-g_new_z.imag());
					}
					x = int(-g_new_z.real()*x_factor*_zoom + _xCenter);
					y = int(-g_new_z.imag()*y_factor*_zoom + _yCenter);
					break;
				case SECRETMODE_NEGATIVE_MAX_COLOR:                     // go negative if max color
					x = int(g_new_z.real()*x_factor*_zoom + _xCenter);
					y = int(g_new_z.imag()*y_factor*_zoom + _yCenter);
					if (c_getcolor(x, y) == g_colors - 1)
					{
						g_new_z.real(-g_new_z.real());
						g_new_z.imag(-g_new_z.imag());
						x = int(g_new_z.real()*x_factor*_zoom + _xCenter);
						y = int(g_new_z.imag()*y_factor*_zoom + _yCenter);
					}
					break;
				case SECRETMODE_POSITIVE_MAX_COLOR:                     // go positive if max color
					g_new_z.real(-g_new_z.real());
					g_new_z.imag(-g_new_z.imag());
					x = int(g_new_z.real()*x_factor*_zoom + _xCenter);
					y = int(g_new_z.imag()*y_factor*_zoom + _yCenter);
					if (c_getcolor(x, y) == g_colors - 1)
					{
						x = int(g_new_z.real()*x_factor*_zoom + _xCenter);
						y = int(g_new_z.imag()*y_factor*_zoom + _yCenter);
					}
					break;
				case SECRETMODE_7:
					if (g_save_c.imag() < 0)
					{
						g_new_z.real(-g_new_z.real());
						g_new_z.imag(-g_new_z.imag());
					}
					x = int(-g_new_z.real()*x_factor*_zoom + _xCenter);
					y = int(-g_new_z.imag()*y_factor*_zoom + _yCenter);
					if (iter > 10)
					{
						if (s_mode == JIIM_PIXEL)
						{
							plot_color_clip(x, y, color);
						}
						else if (s_mode & JIIM_CIRCLE)
						{
							s_x_base = x;
							s_y_base = y;
							circle(int(_zoom*(s_window_dots_x >> 1)/iter), color);
						}
						if ((s_mode & JIIM_LINE) && x > 0 && y > 0 && old_x > 0 && old_y > 0)
						{
							driver_draw_line(x, y, old_x, old_y, color);
						}
						old_x = x;
						old_y = y;
					}
					x = int(g_new_z.real()*x_factor*_zoom + _xCenter);
					y = int(g_new_z.imag()*y_factor*_zoom + _yCenter);
					break;
				case SECRETMODE_ZIGZAG:                     // go in long zig zags
					if (random_count >= 300)
					{
						random_count = -300;
					}
					if (random_count < 0)
					{
						g_new_z.real(-g_new_z.real());
						g_new_z.imag(-g_new_z.imag());
					}
					x = int(g_new_z.real()*x_factor*_zoom + _xCenter);
					y = int(g_new_z.imag()*y_factor*_zoom + _yCenter);
					break;
				case SECRETMODE_RANDOM_RUN:                     // "random run"
					switch (random_direction)
					{
					case 0:             // go random direction for a while
						if (rand() % 2)
						{
							g_new_z.real(-g_new_z.real());
							g_new_z.imag(-g_new_z.imag());
						}
						if (++random_count > 1024)
						{
							random_count = 0;
							random_direction = (rand() % 2) ? 1 : -1;
						}
						break;
					case 1:             // now go negative dir for a while
						g_new_z.real(-g_new_z.real());
						g_new_z.imag(-g_new_z.imag());
						// fall through
					case -1:            // now go positive dir for a while
						if (++random_count > 512)
						{
							random_direction = 0;
							random_count = 0;
						}
						break;
					}
					x = int(g_new_z.real()*x_factor*_zoom + _xCenter);
					y = int(g_new_z.imag()*y_factor*_zoom + _yCenter);
					break;
				} // end switch SecretMode (sorry about the indentation)
			} // end if not MIIM
		}
		else // orbits
		{
			if (iter < g_max_iteration)
			{
				color = int(iter) % g_colors;
				if (g_integer_fractal)
				{
					g_old_z = ComplexFudgeToDouble(g_old_z_l);
				}
				x = int((g_old_z.real() - g_initial_z.real())*x_factor*3*_zoom + _xCenter);
				y = int((g_old_z.imag() - g_initial_z.imag())*y_factor*3*_zoom + _yCenter);
				if (g_fractal_specific[g_fractal_type].orbitcalc())
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
		if (_orbits || iter > 10)
		{
			if (s_mode == JIIM_PIXEL)
			{
				plot_color_clip(x, y, color);
			}
			else if (s_mode & JIIM_CIRCLE)
			{
				s_x_base = x;
				s_y_base = y;
				circle(int(_zoom*(s_window_dots_x >> 1)/iter), color);
			}
			if ((s_mode & JIIM_LINE) && x > 0 && y > 0 && old_x > 0 && old_y > 0)
			{
				driver_draw_line(x, y, old_x, old_y, color);
			}
			old_x = x;
			old_y = y;
		}
		g_old_z = g_new_z;
		g_old_z_l = g_new_z_l;
	} // end while (still)

finish:
	Free_Queue();

	if (kbdchar != 's' && kbdchar != 'S')
	{
		cursor::cursor_hide();
		if (s_windows == 0)
		{
			RestoreRect(s_window_corner_x, s_window_corner_y, s_window_dots_x, s_window_dots_y);
		}
		else if (s_windows >= 2)
		{
			if (s_windows == 2)
			{
				fillrect(g_x_dots, s_window_corner_y, s_window_dots_x-g_x_dots, s_window_dots_y, g_.DAC().Dark());
				fillrect(s_window_corner_x   , g_y_dots, g_x_dots, s_window_dots_y-g_y_dots, g_.DAC().Dark());
			}
			else
			{
				fillrect(s_window_corner_x, s_window_corner_y, s_window_dots_x, s_window_dots_y, g_.DAC().Dark());
			}
			if (s_windows == 3 && s_window_dots_x == g_screen_width) // unhide
			{
				RestoreRect(0, 0, g_x_dots, g_y_dots);
				s_windows = 2;
			}
			cursor::cursor_hide();
			save_has_inverse = g_has_inverse;
			g_has_inverse = true;
			SaveRect(0, 0, g_x_dots, g_y_dots);
			g_screen_x_offset = old_sx_offset;
			g_screen_y_offset = old_sy_offset;
			RestoreRect(0, 0, g_x_dots, g_y_dots);
			g_has_inverse = save_has_inverse;
		}
	}
	cursor::destroy();
#ifdef XFRACT
	cursor_end_mouse_tracking();
#endif
	delete[] g_line_buffer;
	g_line_buffer = 0;
	{
		std::vector<BYTE> local;
		s_rect_buff.swap(local);
	}

	g_using_jiim = false;
	g_calculate_type = old_calculate_type;
	g_debug_mode = old_debug_mode;
	if (kbdchar == 's' || kbdchar == 'S')
	{
		g_viewWindow.InitializeRestart();
		g_x_dots = g_screen_width;
		g_y_dots = g_screen_height;
		g_dx_size = g_x_dots - 1;
		g_dy_size = g_y_dots - 1;
		g_screen_x_offset = 0;
		g_screen_y_offset = 0;
		free_temp_message();
	}
	else
	{
		clear_temp_message();
	}
	s_show_numbers = 0;
	driver_unget_key(kbdchar);
}

void JIIM::WriteCoordinatesOnScreen(bool &actively_computing)
{
	if (s_show_numbers) // write coordinates on screen
	{
		std::string text =
			str(boost::format("%16.14f %16.14f %3d") % _cReal % _cImag % get_color(g_col, g_row));
		if (s_windows == 0)
		{
			/* show temp msg will clear self if new msg is a
			different length - pad to length 40*/
			if (text.length() < 40)
			{
				text.resize(40, ' ');
			}
			cursor::cursor_hide();
			actively_computing = true;
			show_temp_message(text);
			cursor::cursor_show();
		}
		else
		{
			driver_display_string(5, g_screen_height-s_show_numbers, TEXTCOLOR_WHITE, TEXTCOLOR_BLACK, text.c_str());
		}
	}
}

void JIIM::UpdateColumnRow()
{
	if (_dColumn > 0 || _dRow > 0)
	{
		_exact = false;
	}
	g_col += _dColumn;
	g_row += _dRow;

	// keep cursor in logical screen
	if (g_col >= g_x_dots)
	{
		g_col = g_x_dots -1;
		_exact = false;
	}
	if (g_row >= g_y_dots)
	{
		g_row = g_y_dots -1;
		_exact = false;
	}
	if (g_col < 0)
	{
		g_col = 0;
		_exact = false;
	}
	if (g_row < 0)
	{
		g_row = 0;
		_exact = false;
	}
}

bool JIIM::ProcessKeyPress(int kbdchar)
{
	switch (kbdchar)
	{
	case 1143:    // ctrl - keypad 5
	case 1076:    // keypad 5
		break;     // do nothing
	case IDK_CTL_PAGE_UP:
		_dColumn = 4;
		_dRow = -4;
		break;
	case IDK_CTL_PAGE_DOWN:
		_dColumn = 4;
		_dRow = 4;
		break;
	case IDK_CTL_HOME:
		_dColumn = -4;
		_dRow = -4;
		break;
	case IDK_CTL_END:
		_dColumn = -4;
		_dRow = 4;
		break;
	case IDK_PAGE_UP:
		_dColumn = 1;
		_dRow = -1;
		break;
	case IDK_PAGE_DOWN:
		_dColumn = 1;
		_dRow = 1;
		break;
	case IDK_HOME:
		_dColumn = -1;
		_dRow = -1;
		break;
	case IDK_END:
		_dColumn = -1;
		_dRow = 1;
		break;
	case IDK_UP_ARROW:
		_dRow = -1;
		break;
	case IDK_DOWN_ARROW:
		_dRow = 1;
		break;
	case IDK_LEFT_ARROW:
		_dColumn = -1;
		break;
	case IDK_RIGHT_ARROW:
		_dColumn = 1;
		break;
	case IDK_CTL_UP_ARROW:
		_dRow = -4;
		break;
	case IDK_CTL_DOWN_ARROW:
		_dRow = 4;
		break;
	case IDK_CTL_LEFT_ARROW:
		_dColumn = -4;
		break;
	case IDK_CTL_RIGHT_ARROW:
		_dColumn = 4;
		break;
	case 'z':
	case 'Z':
		_zoom = 1.0f;
		break;
	case '<':
	case ',':
		_zoom /= 1.15f;
		break;
	case '>':
	case '.':
		_zoom *= 1.15f;
		break;
	case IDK_SPACE:
		g_julia_c = StdComplexD(_cReal, _cImag);
		return true;

	case 'c':   // circle toggle
	case 'C':   // circle toggle
		s_mode ^= JIIM_CIRCLE;
		break;
	case 'l':
	case 'L':
		s_mode ^= JIIM_LINE;
		break;
	case 'n':
	case 'N':
		s_show_numbers = 8 - s_show_numbers;
		if (s_windows == 0 && s_show_numbers == 0)
		{
			cursor::cursor_hide();
			clear_temp_message();
			cursor::cursor_show();
		}
		break;
	case 'p':
	case 'P':
		get_a_number(&_cReal, &_cImag);
		_exact = true;
		g_col = int(_convert.a*_cReal + _convert.b*_cImag + _convert.e + .5);
		g_row = int(_convert.c*_cReal + _convert.d*_cImag + _convert.f + .5);
		_dColumn = 0;
		_dRow = 0;
		break;
	case 'h':   // hide fractal toggle
	case 'H':   // hide fractal toggle
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
	case '0':
	case '1':
	case '2':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		if (!_orbits)
		{
			s_secret_experimental_mode = kbdchar - '0';
			break;
		}
	default:
		_again = false;
	}
	return (kbdchar == 's' || kbdchar == 'S');
}

void JIIM::UpdateCursor(bool actively_computing)
{
	if (actively_computing)
	{
		cursor::cursor_check_blink();
	}
	else
	{
		cursor::cursor_wait_key();
	}
}

void JIIM::SizeWindow()
{
	if (g_x_dots == g_screen_width || g_y_dots == g_screen_height ||
		g_screen_width-g_x_dots < g_screen_width/3 ||
		g_screen_height-g_y_dots < g_screen_height/3 ||
		g_x_dots >= MAXRECT)
	{
		/* this mode puts orbit/julia in an overlapping window 1/3 the size of
		the physical screen */
		s_windows = 0; // full screen or large view window
		s_window_dots_x = g_screen_width/3;
		s_window_dots_y = g_screen_height/3;
		s_window_corner_x = s_window_dots_x*2;
		s_window_corner_y = s_window_dots_y*2;
		_xCenter = s_window_dots_x*5/2;
		_yCenter = s_window_dots_y*5/2;
	}
	else if (g_x_dots > g_screen_width/3 && g_y_dots > g_screen_height/3)
	{
		// Julia/orbit and fractal don't overlap
		s_windows = 1;
		s_window_dots_x = g_screen_width-g_x_dots;
		s_window_dots_y = g_screen_height-g_y_dots;
		s_window_corner_x = g_x_dots;
		s_window_corner_y = g_y_dots;
		_xCenter = s_window_corner_x + s_window_dots_x/2;
		_yCenter = s_window_corner_y + s_window_dots_y/2;
	}
	else
	{
		// Julia/orbit takes whole screen
		s_windows = 2;
		s_window_dots_x = g_screen_width;
		s_window_dots_y = g_screen_height;
		s_window_corner_x = 0;
		s_window_corner_y = 0;
		_xCenter = s_window_dots_x/2;
		_yCenter = s_window_dots_y/2;
	}
}

