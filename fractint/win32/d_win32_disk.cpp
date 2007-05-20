/* d_win32_disk.c
 *
 * Routines for a Win32 disk video mode driver for fractint.
 */
#include <assert.h>
#include <stdio.h>

#define WIN32_LEAN_AND_MEAN
#define STRICT
#include <windows.h>

#include "port.h"
#include "prototyp.h"
#include "fractype.h"
#include "helpdefs.h"
#include "drivers.h"
#include "fihelp.h"
#include "miscres.h"
#include "plot3d.h"

#include "WinText.h"
#include "frame.h"
#include "d_win32.h"
#include "ods.h"

#define DRAW_INTERVAL 6
#define TIMER_ID 1

/* read/write-a-dot/line routines */
typedef void t_dotwriter(int, int, int);
typedef int  t_dotreader(int, int);
typedef void t_linewriter(int y, int x, int lastx, BYTE *pixels);
typedef void t_linereader(int y, int x, int lastx, BYTE *pixels);

extern HINSTANCE g_instance;

extern void set_normal_line();
extern void set_disk_dot();

class Win32DiskDriver : public Win32BaseDriver
{
public:
	Win32DiskDriver(const char *name, const char *description);

	/* initialize the driver */			virtual bool initialize(int &argc, char **argv);

	/* validate a fractint.cfg mode */	virtual int validate_mode(const VIDEOINFO &mode);
										virtual void set_video_mode(const VIDEOINFO &mode);
	/* find max screen extents */		virtual void get_max_screen(int &x_max, int &y_max) const;

	/* creates a window */				virtual void window();
	/* handles window resize.  */		virtual int resize();
	/* redraws the screen */			virtual void redraw();

	/* read palette into g_dac_box */	virtual int read_palette();
	/* write g_dac_box into palette */	virtual int write_palette();

	/* reads a single pixel */			virtual int read_pixel(int x, int y);
	/* writes a single pixel */			virtual void write_pixel(int x, int y, int color);
	/* reads a span of pixel */			virtual void read_span(int y, int x, int lastx, BYTE *pixels);
	/* writes a span of pixels */		virtual void write_span(int y, int x, int lastx, const BYTE *pixels);
										virtual void get_truecolor(int x, int y, int &r, int &g, int &b, int &a);
										virtual void put_truecolor(int x, int y, int r, int g, int b, int a);
	/* set copy/xor line */				virtual void set_line_mode(int mode);
	/* draw line */						virtual void draw_line(int x1, int y1, int x2, int y2, int color);
	/* draw string in graphics mode */	virtual void display_string(int x, int y, int fg, int bg, const char *text);
	/* save graphics */					virtual void save_graphics();
	/* restore graphics */				virtual void restore_graphics();
	/* set for text mode & save gfx */	virtual void set_for_text();
	/* restores graphics and data */	virtual void set_for_graphics();

	/* returns true if disk video */	virtual int diskp();

										virtual void flush();
	/* refresh alarm */					virtual void schedule_alarm(int secs);

private:
	int check_arg(char *argv);

	int m_width;
	int m_height;
	unsigned char m_clut[256][3];

	static VIDEOINFO s_modes[];
};

Win32DiskDriver::Win32DiskDriver(const char *name, const char *description)
	: Win32BaseDriver(name, description),
	m_width(0),
	m_height(0)
{
	for (int i = 0; i < 256; i++)
	{
		m_clut[i][0] = m_clut[i][1] = m_clut[i][2] = 0;
	}
}
	
/* VIDEOINFO:															*/
/*         char    name[26];       Adapter name (IBM EGA, etc)          */
/*         char    comment[26];    Comments (UNTESTED, etc)             */
/*         int     keynum;         key number used to invoked this mode */
/*                                 2-10 = F2-10, 11-40 = S, C, A{F1-F10}  */
/*         int     dot_mode;        video access method used by asm code */
/*         int     x_dots;          number of dots across the screen     */
/*         int     y_dots;          number of dots down the screen       */
/*         int     colors;         number of g_colors available           */

#define DRIVER_MODE(name_, comment_, key_, width_, height_, mode_) \
	{ name_, comment_, key_, /* 0, 0, 0, 0, */ mode_, width_, height_, 256 }
#define MODE19(n_, c_, k_, w_, h_) DRIVER_MODE(n_, c_, k_, w_, h_, 19)
VIDEOINFO Win32DiskDriver::s_modes[] =
{
	MODE19("Win32 Disk Video         ", "                        ", 0,  320,  240),
	MODE19("Win32 Disk Video         ", "                        ", 0,  400,  300),
	MODE19("Win32 Disk Video         ", "                        ", 0,  480,  360),
	MODE19("Win32 Disk Video         ", "                        ", 0,  600,  450),
	MODE19("Win32 Disk Video         ", "                        ", 0,  640,  480),
	MODE19("Win32 Disk Video         ", "                        ", 0,  800,  600),
	MODE19("Win32 Disk Video         ", "                        ", 0, 1024,  768),
	MODE19("Win32 Disk Video         ", "                        ", 0, 1200,  900),
	MODE19("Win32 Disk Video         ", "                        ", 0, 1280,  960),
	MODE19("Win32 Disk Video         ", "                        ", 0, 1400, 1050),
	MODE19("Win32 Disk Video         ", "                        ", 0, 1500, 1125),
	MODE19("Win32 Disk Video         ", "                        ", 0, 1600, 1200)
};
#undef MODE19
#undef DRIVER_MODE

/* check_arg
 *
 *	See if we want to do something with the argument.
 *
 * Results:
 *	Returns 1 if we parsed the argument.
 *
 * Side effects:
 *	Increments i if we use more than 1 argument.
 */
int Win32DiskDriver::check_arg(char *arg)
{
	return 0;
}

/*----------------------------------------------------------------------
*
* initdacbox --
*
* Put something nice in the dac.
*
* The conditions are:
*	Colors 1 and 2 should be bright so ifs fractals show up.
*	Color 15 should be bright for lsystem.
*	Color 1 should be bright for bifurcation.
*	Colors 1, 2, 3 should be distinct for periodicity.
*	The color map should look good for mandelbrot.
*
* Results:
*	None.
*
* Side effects:
*	Loads the dac.
*
*----------------------------------------------------------------------
*/
/* TODO: review case when COLOR_CHANNEL_MAX != 63 */
static void initdacbox()
{
	int i;
	for (i = 0; i < 256; i++)
	{
		g_dac_box[i][0] = (i >> 5)*8 + 7;
		g_dac_box[i][1] = (((i + 16) & 28) >> 2)*8 + 7;
		g_dac_box[i][2] = (((i + 2) & 3))*16 + 15;
	}
	g_dac_box[0][0] = g_dac_box[0][1] = g_dac_box[0][2] = 0;
	g_dac_box[1][0] = g_dac_box[1][1] = g_dac_box[1][2] = 255;
	g_dac_box[2][0] = 190; g_dac_box[2][1] = g_dac_box[2][2] = 255;
}

/* handle_help_tab
 *
 * Because we want context sensitive help to work everywhere, with the
 * help to display indicated by a non-zero value, we need
 * to trap the F1 key at a very low level.  The same is true of the
 * TAB display.
 *
 * What we do here is check for these keys and invoke their displays.
 * To avoid a recursive invoke of help(), a static is used to avoid
 * recursing on ourselves as help will invoke get key!
 */
static int handle_help_tab(int ch)
{
	static int inside_help = 0;

	if (FIK_F1 == ch && get_help_mode() && !inside_help)
	{
		inside_help = 1;
		help(0);
		inside_help = 0;
		ch = 0;
	}
	else if (FIK_TAB == ch && g_tab_display_enabled)
	{
		bool save_tab_display_enabled = g_tab_display_enabled;
		g_tab_display_enabled = false;
		tab_display();
		g_tab_display_enabled = save_tab_display_enabled;
		ch = 0;
	}

	return ch;
}

static void parse_geometry(const char *spec, int *x, int *y, int *width, int *height)
{
	/* do something like XParseGeometry() */
	if (2 == sscanf(spec, "%dx%d", width, height))
	{
		/* all we care about is width and height for disk output */
		*x = 0;
		*y = 0;
	}
}

/***********************************************************************
////////////////////////////////////////////////////////////////////////
\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
***********************************************************************/

/*----------------------------------------------------------------------
*
* init --
*
*	Initialize the windows and stuff.
*
* Results:
*	None.
*
* Side effects:
*	Initializes windows.
*
*----------------------------------------------------------------------
*/
bool Win32DiskDriver::initialize(int &argc, char **argv)
{
	LPCTSTR title = "FractInt for Windows";

	m_frame.init(g_instance, title);
	if (!m_wintext.initialize(g_instance, NULL, title))
	{
		return false;
	}

	initdacbox();

	/* add default list of video modes */
	for (int m = 0; m < NUM_OF(s_modes); m++)
	{
		add_video_mode(this, s_modes[m]);
	}

	return true;
}

/* resize
 *
 * Check if we need resizing.  If no, return 0.
 * If yes, resize internal buffers and return 1.
 */
int Win32DiskDriver::resize()
{
	m_frame.resize(m_wintext.max_width(), m_wintext.max_height());
	if ((g_video_table[g_adapter].x_dots == m_width)
		&& (g_video_table[g_adapter].y_dots == m_height))
	{
		return 0;
	}

	if (g_disk_flag)
	{
		disk_end();
	}
	disk_start();

	return !0;
}


/*----------------------------------------------------------------------
* read_palette
*
*	Reads the current video palette into g_dac_box.
*
*
* Results:
*	None.
*
* Side effects:
*	Fills in g_dac_box.
*
*----------------------------------------------------------------------
*/
int Win32DiskDriver::read_palette()
{
	ODS("disk_read_palette");
	if (g_got_real_dac == 0)
	{
		return -1;
	}
	for (int i = 0; i < 256; i++)
	{
		g_dac_box[i][0] = m_clut[i][0];
		g_dac_box[i][2] = m_clut[i][2];
	}
	return 0;
}

/*
*----------------------------------------------------------------------
*
* write_palette --
*	Writes g_dac_box into the video palette.
*
*
* Results:
*	None.
*
* Side effects:
*	Changes the displayed colors.
*
*----------------------------------------------------------------------
*/
int Win32DiskDriver::write_palette()
{
	int i;

	ODS("disk_write_palette");
	for (i = 0; i < 256; i++)
	{
		m_clut[i][0] = g_dac_box[i][0];
		m_clut[i][1] = g_dac_box[i][1];
		m_clut[i][2] = g_dac_box[i][2];
	}

	return 0;
}

/*
*----------------------------------------------------------------------
*
* schedule_alarm --
*
*	Start the refresh alarm
*
* Results:
*	None.
*
* Side effects:
*	Starts the alarm.
*
*----------------------------------------------------------------------
*/
void Win32DiskDriver::schedule_alarm(int soon)
{
	m_wintext.schedule_alarm((soon ? 1 : DRAW_INTERVAL)*1000);
}

/*
*----------------------------------------------------------------------
*
* disk_write_pixel --
*
*	Write a point to the screen
*
* Results:
*	None.
*
* Side effects:
*	Draws point.
*
*----------------------------------------------------------------------
*/
void Win32DiskDriver::write_pixel(int x, int y, int color)
{
	putcolor_a(x, y, color);
}

/*
*----------------------------------------------------------------------
*
* disk_read_pixel --
*
*	Read a point from the screen
*
* Results:
*	Value of point.
*
* Side effects:
*	None.
*
*----------------------------------------------------------------------
*/
int Win32DiskDriver::read_pixel(int x, int y)
{
	return getcolor(x, y);
}

/*
*----------------------------------------------------------------------
*
* disk_write_span --
*
*	Write a line of pixels to the screen.
*
* Results:
*	None.
*
* Side effects:
*	Draws pixels.
*
*----------------------------------------------------------------------
*/
void Win32DiskDriver::write_span(int y, int x, int lastx, const BYTE *pixels)
{
	int i;
	int width = lastx-x + 1;
	ODS3("disk_write_span (%d,%d,%d)", y, x, lastx);

	for (i = 0; i < width; i++)
	{
		write_pixel(x + i, y, pixels[i]);
	}
}

/*
*----------------------------------------------------------------------
*
* disk_read_span --
*
*	Reads a line of pixels from the screen.
*
* Results:
*	None.
*
* Side effects:
*	Gets pixels
*
*----------------------------------------------------------------------
*/
void Win32DiskDriver::read_span(int y, int x, int lastx, BYTE *pixels)
{
	int i, width;
	ODS3("disk_read_span (%d,%d,%d)", y, x, lastx);
	width = lastx-x + 1;
	for (i = 0; i < width; i++)
	{
		pixels[i] = read_pixel(x + i, y);
	}
}

void Win32DiskDriver::set_line_mode(int mode)
{
	ODS1("disk_set_line_mode %d", mode);
}

void Win32DiskDriver::draw_line(int x1, int y1, int x2, int y2, int color)
{
	ODS5("disk_draw_line (%d,%d) (%d,%d) %d", x1, y1, x2, y2, color);
	::draw_line(x1, y1, x2, y2, color);
}

/*
*----------------------------------------------------------------------
*
* redraw --
*
*	Refresh the screen.
*
* Results:
*	None.
*
* Side effects:
*	Redraws the screen.
*
*----------------------------------------------------------------------
*/
void Win32DiskDriver::redraw()
{
	ODS("disk_redraw");
	m_wintext.paintscreen(0, 80, 0, 25);
}

void Win32DiskDriver::window()
{
	m_frame.create(m_wintext.max_width(), m_wintext.max_height());
	m_wintext.create(m_frame.window());
}

/*
; **************** Function setvideomode(ax, bx, cx, dx) ****************
;       This function sets the (alphanumeric or graphic) video mode
;       of the monitor.   Called with the proper values of AX thru DX.
;       No returned values, as there is no particular standard to
;       adhere to in this case.

;       (SPECIAL "TWEAKED" VGA VALUES:  if AX==BX==CX==0, assume we have a
;       genuine VGA or register compatable adapter and program the registers
;       directly using the coded value in DX)

; Unix: We ignore ax, bx, cx, dx.  g_dot_mode is the "mode" field in the video
; table.  We use mode 19 for the X window.
*/
void Win32DiskDriver::set_video_mode(const VIDEOINFO &mode)
{
	/* initially, set the virtual line to be the scan line length */
	g_vx_dots = g_screen_width;
	g_is_true_color = 0;				/* assume not truecolor */
	g_ok_to_print = false;
	g_good_mode = 1;
	if (g_dot_mode != 0)
	{
		g_and_color = g_colors-1;
		g_box_count = 0;
		g_dac_learn = 1;
		g_dac_count = g_cycle_limit;
		g_got_real_dac = true;

		read_palette();
	}

	resize();

	set_disk_dot();
	set_normal_line();
}

void Win32DiskDriver::display_string(int x, int y, int fg, int bg, const char *text)
{
}

void Win32DiskDriver::set_for_text()
{
}

void Win32DiskDriver::set_for_graphics()
{
	hide_text_cursor();
}

int Win32DiskDriver::diskp()
{
	return 1;
}

int Win32DiskDriver::validate_mode(const VIDEOINFO &mode)
{
	/* allow modes of any size with 256 colors and g_dot_mode = 19
	   ax/bx/cx/dx must be zero. */
	return (mode.colors == 256) &&
		(mode.dotmode == 19);
}

void Win32DiskDriver::get_truecolor(int x, int y, int &r, int &g, int &b, int &a)
{
}

void Win32DiskDriver::put_truecolor(int x, int y, int r, int g, int b, int a)
{
}

void Win32DiskDriver::save_graphics()
{
}

void Win32DiskDriver::restore_graphics()
{
}

void Win32DiskDriver::get_max_screen(int &x_max, int &y_max) const
{
	x_max = -1;
	y_max = -1;
}

void Win32DiskDriver::flush()
{
}

Win32DiskDriver disk_driver_info("disk", "A disk video driver for 32-bit Windows.");

AbstractDriver *disk_driver = &disk_driver_info;
