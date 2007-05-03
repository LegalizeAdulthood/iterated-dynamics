/* d_x11.c
 * This file contains routines for the Unix port of fractint.
 * It uses the current window for text and creates an X window for graphics.
 *
 * This file Copyright 1991 Ken Shirriff.  It may be used according to the
 * fractint license conditions, blah blah blah.
 *
 * Some of the X stuff is based on xloadimage by Jim Frost.
 * The FindWindowRoot routine is from ssetroot by Tom LaStrange.
 * Other root window stuff is based on xmartin, by Ed Kubaitis.
 * Some of the colormap stuff is from Mike Yang (mikey@sgi.com).
 * Some of the zoombox code is from Bill Broadley.
 * David Sanderson straightened out a bunch of include file problems.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <signal.h>
#include <sys/types.h>
#ifdef _AIX
#include <sys/select.h>
#endif
#include <sys/time.h>
#include <sys/ioctl.h>
#ifdef FPUERR
#include <floatingpoint.h>
#endif
#ifdef __hpux
#include <sys/file.h>
#endif
#include <sys/wait.h>

#include "helpdefs.h"
#include "port.h"
#include "prototyp.h"
#include "externs.h"
#include "drivers.h"
#include "fihelp.h"
#include "SoundState.h"

#ifdef LINUX
#define FNDELAY O_NDELAY
#endif
#ifdef __SVR4
# include <sys/filio.h>
# define FNDELAY O_NONBLOCK
#endif
#include <assert.h>

extern int slowdisplay;
extern int inside_help;
extern VIDEOINFO x11_video_table[];

extern void fpe_handler(int);

typedef unsigned long XPixel;

enum
{
	TEXT_WIDTH = 80,
	TEXT_HEIGHT = 25,
	MOUSE_SCALE = 1
};

class X11Driver : public NamedDriver
{
public:
	X11Driver(const char *name, const char *description);

	virtual BYTE *find_font(int parm);
	virtual int initialize(int &argc, char **argv);
	virtual int validate_mode(const VIDEOINFO &mode);
	virtual void flush();
	virtual void terminate();
	virtual void pause();
	virtual void resume();
	virtual void schedule_alarm(int soon);
	virtual void window();
	virtual int resize();
	virtual void put_char_attr_rowcol(int row, int col, int char_attr);
	virtual int get_char_attr_rowcol(int row, int col);
	virtual void set_keyboard_timeout(int ms);
	virtual void get_max_screen(int &width, int &height) const;
	virtual void delay(int ms);
	virtual void put_char_attr(int char_attr);
	virtual int get_char_attr();
	virtual int diskp();
	virtual void mute();
	virtual void sound_off();
	virtual int sound_on(int freq);
	virtual void buzzer(int buzzer_type);
	virtual int init_fm();
	virtual void discard_screen();
	virtual void unstack_screen();
	virtual void stack_screen();
	virtual void scroll_up(int top, int bot);
	virtual void set_attr(int row, int col, int attr, int count);
	virtual void hide_text_cursor();
	virtual void move_cursor(int row, int col);
	virtual void set_clear();
	virtual void set_for_graphics();
	virtual void set_for_text();
	virtual void put_string(int row, int col, int attr, const char *msg);
	virtual void set_video_mode(const VIDEOINFO &mode);
	virtual void shell();
	virtual void unget_key(int key);
	virtual int wait_key_pressed(int timeout);
	virtual int key_pressed();
	virtual int key_cursor(int row, int col);
	virtual int get_key();
	virtual void restore_graphics();
	virtual void save_graphics();
	virtual void display_string(int x, int y, int fg, int bg, const char *text);
	virtual void draw_line(int x1, int y1, int x2, int y2, int color);
	virtual void set_line_mode(int mode);
	virtual void put_truecolor(int x, int y, int r, int g, int b, int a);
	virtual void get_truecolor(int x, int y, int &r, int &g, int &b, int &a);
	virtual void write_span(int y, int x, int lastx, const BYTE *pixels);
	virtual void read_span(int y, int x, int lastx, BYTE *pixels);
	virtual void  write_pixel(int x, int y, int color);
	virtual int read_pixel(int x, int y);
	virtual int write_palette();
	virtual int read_palette();
	virtual void redraw();
	virtual void set_mouse_mode(int new_mode)
	{
		m_look_at_mouse = new_mode;
	}

	virtual int get_mouse_mode() const
	{
		return m_look_at_mouse;
	}

private:
	unsigned long do_fake_lut(int idx);
	int check_arg(int i, int argc, char **argv);
	void doneXwindow();
	void initdacbox();
	void erase_text_screen();
	void select_visual();
	void clearXwindow();
	int xcmapstuff();
	int start_video();
	int end_video();
	void setredrawscreen();
	int getachar();
	int translate_key(int ch);
	int handle_esc();
	int ev_key_press(XKeyEvent *xevent);
	void ev_key_release(XKeyEvent *xevent);
	void ev_expose(XExposeEvent *xevent);
	void ev_button_press(XEvent *xevent);
	void ev_motion_notify(XEvent *xevent);
	void handle_events();
	int input_pending();
	Window pr_dwmroot(Display *dpy, Window pwin);
	Window FindRootWindow();
	void RemoveRootPixmap();
	void load_font();

	int m_look_at_mouse;
	int m_on_root;				/* = 0; */
	int m_fullscreen;			/* = 0; */
	int m_sharecolor;			/* = 0; */
	int m_privatecolor;			/* = 0; */
	int m_fixcolors;			/* = 0; */
	/* Run X events synchronously (debugging) */
	int m_sync;				/* = 0; */
	char *m_Xdisplay;			/* = ""; */
	char *m_Xgeometry;			/* = NULL; */
	int m_doesBacking;

	/*
	* The pixtab stuff is so we can map from fractint pixel values 0-n to
	* the actual color table entries which may be anything.
	*/
	int m_usepixtab;			/* = 0; */
	unsigned long m_pixtab[256];
	int m_ipixtab[256];
	XPixel m_cmap_pixtab[256]; /* for faking a LUTs on non-LUT visuals */
	int m_cmap_pixtab_alloced;
	int m_fake_lut;

	int m_fastmode; /* = 0; */ /* Don't draw pixels 1 at a time */
	int m_alarmon; /* = 0; */ /* 1 if the refresh alarm is on */
	int m_doredraw; /* = 0; */ /* 1 if we have a redraw waiting */

	Display *m_Xdp;				/* = NULL; */
	Window m_Xw;
	GC m_Xgc;				/* = NULL; */
	Visual *m_Xvi;
	Screen *m_Xsc;
	Colormap m_Xcmap;
	int m_Xdepth;
	XImage *m_Ximage;
	char *m_Xdata;
	int m_Xdscreen;
	Pixmap m_Xpixmap;			/* = None; */
	int m_Xwinwidth, m_Xwinheight;
	Window m_Xroot;
	int m_xlastcolor;			/* = -1; */
	int m_xlastfcn;				/* = GXcopy; */
	BYTE *m_pixbuf;				/* = NULL; */
	XColor m_cols[256];

	int m_XZoomWaiting;			/* = 0; */

	const char *m_x_font_name;		/* = FONT; */
	XFontStruct *m_font_info;		/* = NULL; */

	int m_xbufkey; /* = 0; */		/* Buffered X key */

	unsigned char *m_fontPtr;		/* = NULL; */

	char m_text_screen[TEXT_HEIGHT][TEXT_WIDTH];
	int m_text_attr[TEXT_HEIGHT][TEXT_WIDTH];

	BYTE *m_font_table;			/* = NULL; */

	Bool m_text_modep;			/* True when displaying text */

	/* rubber banding and event processing data */
	int m_ctl_mode;
	int m_shift_mode;
	int m_button_num;
	int m_last_x, m_last_y;
	int m_dx, m_dy;
	int m_keyboard_timeout;
};

#ifdef FPUERR
static void continue_hdl(int sig, int code, struct sigcontext *scp,
	char *addr);
#endif

static const int mousefkey[4][4] /* [button][dir] */ =
{
	{ FIK_RIGHT_ARROW, FIK_LEFT_ARROW, FIK_DOWN_ARROW, FIK_UP_ARROW },
	{ 0, 0, FIK_PAGE_DOWN, FIK_PAGE_UP },
	{ FIK_CTL_PLUS, FIK_CTL_MINUS, FIK_CTL_DEL, FIK_CTL_INSERT },
	{ FIK_CTL_END, FIK_CTL_HOME, FIK_CTL_PAGE_DOWN, FIK_CTL_PAGE_UP }
};


#define DEFX 640
#define DEFY 480
#define DEFXY "640x480+0+0"

extern int g_edit_pal_cursor;
extern void cursor_set_position();

#define SENS 1
#define ABS(x) ((x) > 0 ? (x) : -(x))
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define SIGN(x) ((x) > 0 ? 1 : -1)

#define SHELL "/bin/csh"

#define FONT "-*-*-medium-r-*-*-9-*-*-*-*-*-iso8859-*"
#define DRAW_INTERVAL 6

extern void (*g_dot_write)(int, int, int);	/* write-a-dot routine */
extern int (*g_dot_read)(int, int); 	/* read-a-dot routine */
extern void (*g_line_write)(int, int, int, const BYTE *);	/* write-a-line routine */
extern void (*g_line_read)(int, int, int, BYTE *);	/* read-a-line routine */

static const VIDEOINFO x11_info =
{
	"xfractint mode           ","                         ",
	999, 0,	 0,   0,   0,	19, 640, 480,  256
};

unsigned long X11Driver::do_fake_lut(int idx)
{
	return m_fake_lut ? m_cmap_pixtab[idx] : idx;
}

/*
 *----------------------------------------------------------------------
 *
 * check_arg --
 *
 *	See if we want to do something with the argument.
 *
 * Results:
 *	Returns 1 if we parsed the argument.
 *
 * Side effects:
 *	Increments i if we use more than 1 argument.
 *
 *----------------------------------------------------------------------
 */
int X11Driver::check_arg(int i, int argc, char **argv)
{
	if (strcmp(argv[i], "-display") == 0 && i < argc-1)
	{
		m_Xdisplay = argv[i+1];
		return 2;
	}
	else if (strcmp(argv[i], "-fullscreen") == 0)
	{
		m_fullscreen = 1;
		return 1;
	}
	else if (strcmp(argv[i], "-onroot") == 0)
	{
		m_on_root = 1;
		return 1;
	}
	else if (strcmp(argv[i], "-share") == 0)
	{
		m_sharecolor = 1;
		return 1;
	}
	else if (strcmp(argv[i], "-fast") == 0)
	{
		m_fastmode = 1;
		return 1;
	}
	else if (strcmp(argv[i], "-slowdisplay") == 0)
	{
		slowdisplay = 1;
		return 1;
	}
	else if (strcmp(argv[i], "-sync") == 0)
	{
		m_sync = 1;
		return 1;
	}
	else if (strcmp(argv[i], "-private") == 0)
	{
		m_privatecolor = 1;
		return 1;
	}
	else if (strcmp(argv[i], "-fixcolors") == 0 && i < argc-1)
	{
		m_fixcolors = atoi(argv[i+1]);
		return 2;
	}
	else if (strcmp(argv[i], "-geometry") == 0 && i < argc-1)
	{
		m_Xgeometry = argv[i+1];
		return 2;
	}
	else if (strcmp(argv[i], "-fn") == 0 && i < argc-1)
	{
		m_x_font_name = argv[i+1];
		return 2;
	}
	else
	{
		return 0;
	}
}

/*----------------------------------------------------------------------
 *
 * doneXwindow --
 *
 *	Clean up the X stuff.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Frees window, etc.
 *
 *----------------------------------------------------------------------
 */
void X11Driver::doneXwindow()
{
	if (m_Xdp == NULL)
	{
		return;
	}

	if (m_Xgc)
	{
		XFreeGC(m_Xdp, m_Xgc);
	}

	if (m_Xpixmap)
	{
		XFreePixmap(m_Xdp, m_Xpixmap);
		m_Xpixmap = None;
	}
	XFlush(m_Xdp);
	m_Xdp = NULL;
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
 *	The color map should be good if only 128 colors are used.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Loads the dac.
 *
 *----------------------------------------------------------------------
 */
void X11Driver::initdacbox()
{
	int i;
	for (i=0;i < 256;i++)
	{
		g_dac_box[i][0] = (i >> 5)*8+7;
		g_dac_box[i][1] = (((i+16) & 28) >> 2)*8+7;
		g_dac_box[i][2] = (((i+2) & 3))*16+15;
	}
	g_dac_box[0][0] = g_dac_box[0][1] = g_dac_box[0][2] = 0;
	g_dac_box[1][0] = g_dac_box[1][1] = g_dac_box[1][2] = 63;
	g_dac_box[2][0] = 47; g_dac_box[2][1] = g_dac_box[2][2] = 63;
}

void X11Driver::erase_text_screen()
{
	int r, c;
	for (r = 0; r < TEXT_HEIGHT; r++)
	{
		for (c = 0; c < TEXT_WIDTH; c++)
		{
			m_text_attr[r][c] = 0;
			m_text_screen[r][c] = ' ';
		}
	}
}

/*
 *----------------------------------------------------------------------
 *
 * errhand --
 *
 *	Called on an X server error.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Prints the error message.
 *
 *----------------------------------------------------------------------
 */
static int errhand(Display *dp, XErrorEvent *xe)
{
	char buf[200];
	fflush(stdout);
	printf("X Error: %d %d %d %d\n", xe->type, xe->error_code,
		xe->request_code, xe->minor_code);
	XGetErrorText(dp, xe->error_code, buf, 200);
	printf("%s\n", buf);
}

#ifdef FPUERR
/*
 *----------------------------------------------------------------------
 *
 * continue_hdl --
 *
 *	Handle an IEEE fpu error.
 *	This routine courtesy of Ulrich Hermes
 *	<hermes@olymp.informatik.uni-dortmund.de>
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Clears flag.
 *
 *----------------------------------------------------------------------
 */
static void continue_hdl(int sig, int code, struct sigcontext *scp, char *addr)
{
	int i;
	char out[20];
	/*		if you want to get all messages enable this statement.    */
	/*  printf("ieee exception code %x occurred at pc %X\n", code, scp->sc_pc); */
	/*	clear all excaption flags					  */
	i = ieee_flags("clear", "exception", "all", out);
}
#endif

void X11Driver::select_visual()
{
	m_Xvi = XDefaultVisualOfScreen(m_Xsc);
	m_Xdepth = DefaultDepth(m_Xdp, m_Xdscreen);

	switch (m_Xvi->c_class)
	{
	case StaticGray:
	case StaticColor:
		g_colors = (m_Xdepth <= 8) ? m_Xvi->map_entries : 256;
		g_got_real_dac = 0;
		m_fake_lut = 0;
		break;

	case GrayScale:
	case PseudoColor:
		g_colors = (m_Xdepth <= 8) ? m_Xvi->map_entries : 256;
		g_got_real_dac = 1;
		m_fake_lut = 0;
		break;

	case TrueColor:
	case DirectColor:
		g_colors = 256;
		g_got_real_dac = 0;
		m_fake_lut = 1;
		break;

	default:
		/* those should be all the visual classes */
		assert(1);
		break;
	}
	if (g_colors > 256)
	{
		g_colors = 256;
	}
}

/*----------------------------------------------------------------------
 *
 * clearXwindow --
 *
 *	Clears X window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Clears window.
 *
 *----------------------------------------------------------------------
 */
void X11Driver::clearXwindow()
{
	char *ptr;
	int i, len;
	if (m_fake_lut)
	{
		int j;
		for (j = 0; j < m_Ximage->height; j++)
		{
			for (i = 0; i < m_Ximage->width; i++)
			{
				XPutPixel(m_Ximage, i, j, m_cmap_pixtab[m_pixtab[0]]);
			}
		}
	}
	else if (m_pixtab[0] != 0)
	{
		/*
		 * Initialize image to m_pixtab[0].
		 */
		if (g_colors == 2)
		{
			for (i = 0; i < m_Ximage->bytes_per_line; i++)
			{
				m_Ximage->data[i] = 0xff;
			}
		}
		else
		{
			for (i = 0; i < m_Ximage->bytes_per_line; i++)
			{
				m_Ximage->data[i] = m_pixtab[0];
			}
		}
		for (i = 1; i < m_Ximage->height; i++)
		{
			bcopy(m_Ximage->data,
				m_Ximage->data+i*m_Ximage->bytes_per_line, 
				m_Ximage->bytes_per_line);
		}
	}
	else
	{
		/*
		 * Initialize image to 0's.
		 */
		bzero(m_Ximage->data, m_Ximage->bytes_per_line*m_Ximage->height);
	}
	m_xlastcolor = -1;
	XSetForeground(m_Xdp, m_Xgc, do_fake_lut(m_pixtab[0]));
	if (m_on_root)
	{
		XFillRectangle(m_Xdp, m_Xpixmap, m_Xgc,
			0, 0, m_Xwinwidth, m_Xwinheight);
	}
	XFillRectangle(m_Xdp, m_Xw, m_Xgc,
		0, 0, m_Xwinwidth, m_Xwinheight);
	flush();
}

/*----------------------------------------------------------------------
 *
 * xcmapstuff --
 *
 *	Set up the colormap appropriately
 *
 * Results:
 *	Number of colors.
 *
 * Side effects:
 *	Sets colormap.
 *
 *----------------------------------------------------------------------
 */
int X11Driver::xcmapstuff()
{
	int ncells, i;

	if (m_on_root)
	{
		m_privatecolor = 0;
	}
	for (i = 0; i < g_colors; i++)
	{
		m_pixtab[i] = i;
		m_ipixtab[i] = 999;
	}
	if (!g_got_real_dac)
	{
		m_Xcmap = DefaultColormapOfScreen(m_Xsc);
		if (m_fake_lut)
		{
			write_palette();
		}
	}
	else if (m_sharecolor)
	{
		g_got_real_dac = 0;
	}
	else if (m_privatecolor)
	{
		m_Xcmap = XCreateColormap(m_Xdp, m_Xw, m_Xvi, AllocAll);
		XSetWindowColormap(m_Xdp, m_Xw, m_Xcmap);
	}
	else
	{
		int powr;

		m_Xcmap = DefaultColormap(m_Xdp, m_Xdscreen);
		for (powr = m_Xdepth; powr >= 1; powr--)
		{
			ncells = 1 << powr;
			if (ncells > g_colors)
			{
				continue;
			}
			if (XAllocColorCells(m_Xdp, m_Xcmap, False, NULL, 0,
				m_pixtab, (unsigned int) ncells))
			{
				g_colors = ncells;
				m_usepixtab = 1;
				break;
			}
		}
		if (!m_usepixtab)
		{
			printf("Couldn't allocate any colors\n");
			g_got_real_dac = 0;
		}
	}
	for (i = 0; i < g_colors; i++)
	{
		m_ipixtab[m_pixtab[i]] = i;
	}
	/* We must make sure if any color uses position 0, that it is 0.
	* This is so we can clear the image with bzero.
	* So, suppose fractint 0 = cmap 42, cmap 0 = fractint 55.
	* Then want fractint 0 = cmap 0, cmap 42 = fractint 55.
	* I.e. pixtab[55] = 42, ipixtab[42] = 55.
	*/
	if (m_ipixtab[0] == 999)
	{
		m_ipixtab[0] = 0;
	}
	else if (m_ipixtab[0] != 0)
	{
		int other;
		other = m_ipixtab[0];
		m_pixtab[other] = m_pixtab[0];
		m_ipixtab[m_pixtab[other]] = other;
		m_pixtab[0] = 0;
		m_ipixtab[0] = 0;
	}

	if (!g_got_real_dac && g_colors == 2 && BlackPixelOfScreen(m_Xsc) != 0)
	{
		m_pixtab[0] = m_ipixtab[0] = 1;
		m_pixtab[1] = m_ipixtab[1] = 0;
		m_usepixtab = 1;
	}

	return g_colors;
}

int X11Driver::start_video()
{
	clearXwindow();
	return 0;
}

int X11Driver::end_video()
{
	return 0;				/* set flag: video ended */
}


/*
 *----------------------------------------------------------------------
 *
 * setredrawscreen --
 *
 *	Set the screen refresh flag
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sets the flag.
 *
 *----------------------------------------------------------------------
 */
void X11Driver::setredrawscreen()
{
	m_doredraw = 1;
}

/*
 *----------------------------------------------------------------------
 *
 * getachar --
 *
 *	Gets a character.
 *
 * Results:
 *	Key.
 *
 * Side effects:
 *	Reads key.
 *
 *----------------------------------------------------------------------
 */
int X11Driver::getachar()
{
	char ch;
	int status;
	status = read(0, &ch, 1);
	if (status < 0)
	{
		return -1;
	}
	else
	{
		return ch;
	}
}

/*----------------------------------------------------------------------
 *
 * translate_key --
 *
 *	Translate an input key into MSDOS format.  The purpose of this
 *	routine is to do the mappings like U -> PAGE_UP.
 *
 * Results:
 *	New character;
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
int X11Driver::translate_key(int ch)
{
	if (ch >= 'a' && ch <= 'z')
	{
		return ch;
	}
	else
	{
		switch (ch)
		{
		case 'I':		return FIK_INSERT;
		case 'D':		return FIK_DELETE;
		case 'U':		return FIK_PAGE_UP;
		case 'N':		return FIK_PAGE_DOWN;
		case CTL('O'):	return FIK_CTL_HOME;
		case CTL('E'):	return FIK_CTL_END;
		case 'H':		return FIK_LEFT_ARROW;
		case 'L':		return FIK_RIGHT_ARROW;
		case 'K':		return FIK_UP_ARROW;
		case 'J':		return FIK_DOWN_ARROW;
		case 1115:		return FIK_CTL_LEFT_ARROW;
		case 1116:		return FIK_CTL_RIGHT_ARROW;
		case 1141:		return FIK_CTL_UP_ARROW;
		case 1145:		return FIK_CTL_DOWN_ARROW;
		case 'O':		return FIK_HOME;
		case 'E':		return FIK_END;
		case '\n':		return FIK_ENTER;
		case CTL('T'):	return FIK_CTL_ENTER;
		case -2:		return FIK_CTL_ENTER_2;
		case CTL('U'):	return FIK_CTL_PAGE_UP;
		case CTL('N'):	return FIK_CTL_PAGE_DOWN;
		case '{':		return FIK_CTL_MINUS;
		case '}':		return FIK_CTL_PLUS;
#if 0
			/* we need ^I for tab */
		case CTL('I'):	return FIK_CTL_INSERT;
#endif
		case CTL('D'):	return FIK_CTL_DEL;
		case '!':		return FIK_F1;
		case '@':		return FIK_F2;
		case '#':		return FIK_F3;
		case '$':		return FIK_F4;
		case '%':		return FIK_F5;
		case '^':		return FIK_F6;
		case '&':		return FIK_F7;
		case '*':		return FIK_F8;
		case '(':		return FIK_F9;
		case ')':		return FIK_F10;
		default:
			return ch;
		}
	}
}

/*----------------------------------------------------------------------
 *
 * handle_esc --
 *
 *	Handle an escape key.  This may be an escape key sequence
 *	indicating a function key was pressed.
 *
 * Results:
 *	Key.
 *
 * Side effects:
 *	Reads keys.
 *
 *----------------------------------------------------------------------
 */
int X11Driver::handle_esc()
{
	int ch1, ch2, ch3;

#ifdef __hpux
	/* HP escape key sequences. */
	ch1 = getachar();
	if (ch1 == -1)
	{
		driver_delay(250); /* Wait 1/4 sec to see if a control sequence follows */
		ch1 = getachar();
	}
	if (ch1 == -1)
	{
		return FIK_ESC;
	}

	switch (ch1)
	{
	case 'A': return FIK_UP_ARROW;
	case 'B': return FIK_DOWN_ARROW;
	case 'D': return FIK_LEFT_ARROW;
	case 'C': return FIK_RIGHT_ARROW;
	case 'd': return FIK_HOME;
	}
	if (ch1 != '[')
	{
		return FIK_ESC;
	}
	ch1 = getachar();
	if (ch1 == -1)
	{
		driver_delay(250); /* Wait 1/4 sec to see if a control sequence follows */
		ch1 = getachar();
	}
	if (ch1 == -1 || !isdigit(ch1))
	{
		return FIK_ESC;
	}
	ch2 = getachar();
	if (ch2 == -1)
	{
		driver_delay(250); /* Wait 1/4 sec to see if a control sequence follows */
		ch2 = getachar();
	}
	if (ch2 == -1)
	{
		return FIK_ESC;
	}
	if (isdigit(ch2))
	{
		ch3 = getachar();
		if (ch3 == -1)
		{
			driver_delay(250); /* Wait 1/4 sec to see if a control sequence follows */
			ch3 = getachar();
		}
		if (ch3 != '~')
		{
			return FIK_ESC;
		}
		ch2 = (ch2-'0')*10+ch3-'0';
	}
	else if (ch3 != '~')
	{
		return FIK_ESC;
	}
	else
	{
		ch2 = ch2-'0';
	}
	switch (ch2)
	{
	case 5: return FIK_PAGE_UP;
	case 6: return FIK_PAGE_DOWN;
	case 29: return FIK_F1; /* help */
	case 11: return FIK_F1;
	case 12: return FIK_F2;
	case 13: return FIK_F3;
	case 14: return FIK_F4;
	case 15: return FIK_F5;
	case 17: return FIK_F6;
	case 18: return FIK_F7;
	case 19: return FIK_F8;
	default:
		return FIK_ESC;
	}
#else
	/* SUN escape key sequences */
	ch1 = getachar();
	if (ch1 == -1)
	{
		driver_delay(250); /* Wait 1/4 sec to see if a control sequence follows */
		ch1 = getachar();
	}
	if (ch1 != '[')		/* See if we have esc [ */
	{
		return FIK_ESC;
	}
	ch1 = getachar();
	if (ch1 == -1)
	{
		driver_delay(250); /* Wait 1/4 sec to see if a control sequence follows */
		ch1 = getachar();
	}
	if (ch1 == -1)
	{
		return FIK_ESC;
	}
	switch (ch1)
	{
	case 'A':		/* esc [ A */ return FIK_UP_ARROW;
	case 'B':		/* esc [ B */ return FIK_DOWN_ARROW;
	case 'C':		/* esc [ C */ return FIK_RIGHT_ARROW;
	case 'D':		/* esc [ D */ return FIK_LEFT_ARROW;
	default:
		break;
	}
	ch2 = getachar();
	if (ch2 == -1)
	{
		driver_delay(250); /* Wait 1/4 sec to see if a control sequence follows */
		ch2 = getachar();
	}
	if (ch2 == '~')
	{		/* esc [ ch1 ~ */
		switch (ch1)
		{
		case '2':		/* esc [ 2 ~ */ return FIK_INSERT;
		case '3':		/* esc [ 3 ~ */ return FIK_DELETE;
		case '5':		/* esc [ 5 ~ */ return FIK_PAGE_UP;
		case '6':		/* esc [ 6 ~ */ return FIK_PAGE_DOWN;
		default:
			return FIK_ESC;
		}
	}
	else if (ch2 == -1)
	{
		return FIK_ESC;
	}
	else
	{
		ch3 = getachar();
		if (ch3 == -1)
		{
			driver_delay(250); /* Wait 1/4 sec to see if a control sequence follows */
			ch3 = getachar();
		}
		if (ch3 != '~')
		{	/* esc [ ch1 ch2 ~ */
			return FIK_ESC;
		}
		if (ch1 == '1')
		{
			switch (ch2)
			{
			case '1':	/* esc [ 1 1 ~ */ return FIK_F1;
			case '2':	/* esc [ 1 2 ~ */ return FIK_F2;
			case '3':	/* esc [ 1 3 ~ */ return FIK_F3;
			case '4':	/* esc [ 1 4 ~ */ return FIK_F4;
			case '5':	/* esc [ 1 5 ~ */ return FIK_F5;
			case '6':	/* esc [ 1 6 ~ */ return FIK_F6;
			case '7':	/* esc [ 1 7 ~ */ return FIK_F7;
			case '8':	/* esc [ 1 8 ~ */ return FIK_F8;
			case '9':	/* esc [ 1 9 ~ */ return FIK_F9;
			default:
				return FIK_ESC;
			}
		}
		else if (ch1 == '2')
		{
			switch (ch2)
			{
			case '0':	/* esc [ 2 0 ~ */ return FIK_F10;
			case '8':	/* esc [ 2 8 ~ */ return FIK_F1;  /* HELP */
			default:
				return FIK_ESC;
			}
		}
		else
		{
			return FIK_ESC;
		}
	}
#endif
}

/* ev_key_press
 *
 * Translate keypress into appropriate fractint character code,
 * according to defines in fractint.h
 */
int X11Driver::ev_key_press(XKeyEvent *xevent)
{
	int charcount;
	char buffer[1];
	KeySym keysym;
	int compose;
	charcount = XLookupString(xevent, buffer, 1, &keysym, NULL);
	switch (keysym)
	{
	case XK_Control_L:
	case XK_Control_R:
		m_ctl_mode = 1;
		return 1;

	case XK_Shift_L:
	case XK_Shift_R:
		m_shift_mode = 1;
		break;
	case XK_Home:
	case XK_R7:
		m_xbufkey = m_ctl_mode ? FIK_CTL_HOME : FIK_HOME;
		return 1;
	case XK_Left:
	case XK_R10:
		m_xbufkey = m_ctl_mode ? FIK_CTL_LEFT_ARROW : FIK_LEFT_ARROW;
		return 1;
	case XK_Right:
	case XK_R12:
		m_xbufkey = m_ctl_mode ? FIK_CTL_RIGHT_ARROW : FIK_RIGHT_ARROW;
		return 1;
	case XK_Down:
	case XK_R14:
		m_xbufkey = m_ctl_mode ? FIK_CTL_DOWN_ARROW : FIK_DOWN_ARROW;
		return 1;
	case XK_Up:
	case XK_R8:
		m_xbufkey = m_ctl_mode ? FIK_CTL_UP_ARROW : FIK_UP_ARROW;
		return 1;
	case XK_Insert:
		m_xbufkey = m_ctl_mode ? FIK_CTL_INSERT : FIK_INSERT;
		return 1;
	case XK_Delete:
		m_xbufkey = m_ctl_mode ? FIK_CTL_DEL : FIK_DELETE;
		return 1;
	case XK_End:
	case XK_R13:
		m_xbufkey = m_ctl_mode ? FIK_CTL_END : FIK_END;
		return 1;
	case XK_Help:
		m_xbufkey = FIK_F1;
		return 1;
	case XK_Prior:
	case XK_R9:
		m_xbufkey = m_ctl_mode ? FIK_CTL_PAGE_UP : FIK_PAGE_UP;
		return 1;
	case XK_Next:
	case XK_R15:
		m_xbufkey = m_ctl_mode ? FIK_CTL_PAGE_DOWN : FIK_PAGE_DOWN;
		return 1;
	case XK_F1:
	case XK_L1:
		m_xbufkey = m_shift_mode ? FIK_SF1: FIK_F1;
		return 1;
	case XK_F2:
	case XK_L2:
		m_xbufkey = m_shift_mode ? FIK_SF2: FIK_F2;
		return 1;
	case XK_F3:
	case XK_L3:
		m_xbufkey = m_shift_mode ? FIK_SF3: FIK_F3;
		return 1;
	case XK_F4:
	case XK_L4:
		m_xbufkey = m_shift_mode ? FIK_SF4: FIK_F4;
		return 1;
	case XK_F5:
	case XK_L5:
		m_xbufkey = m_shift_mode ? FIK_SF5: FIK_F5;
		return 1;
	case XK_F6:
	case XK_L6:
		m_xbufkey = m_shift_mode ? FIK_SF6: FIK_F6;
		return 1;
	case XK_F7:
	case XK_L7:
		m_xbufkey = m_shift_mode ? FIK_SF7: FIK_F7;
		return 1;
	case XK_F8:
	case XK_L8:
		m_xbufkey = m_shift_mode ? FIK_SF8: FIK_F8;
		return 1;
	case XK_F9:
	case XK_L9:
		m_xbufkey = m_shift_mode ? FIK_SF9: FIK_F9;
		return 1;
	case XK_F10:
	case XK_L10:
		m_xbufkey = m_shift_mode ? FIK_SF10: FIK_F10;
		return 1;
	case '+':
		m_xbufkey = m_ctl_mode ? FIK_CTL_PLUS : '+';
		return 1;
	case '-':
		m_xbufkey = m_ctl_mode ? FIK_CTL_MINUS : '-';
		return 1;
		break;
	case XK_Return:
	case XK_KP_Enter:
		m_xbufkey = m_ctl_mode ? CTL('T') : '\n';
		return 1;
	}
	if (charcount == 1)
	{
		m_xbufkey = buffer[0];
		if (m_xbufkey == '\003')
		{
			goodbye();
		}
	}
}

/* ev_key_release
 *
 * toggle modifier key state for shit and contol keys, otherwise ignore
 */
void X11Driver::ev_key_release(XKeyEvent *xevent)
{
	char buffer[1];
	KeySym keysym;
	XLookupString(xevent, buffer, 1, &keysym, NULL);
	switch (keysym)
	{
	case XK_Control_L:
	case XK_Control_R:
		m_ctl_mode = 0;
		break;
	case XK_Shift_L:
	case XK_Shift_R:
		m_shift_mode = 0;
		break;
	}
}

void X11Driver::ev_expose(XExposeEvent *xevent)
{
	if (m_text_modep)
	{
		/* if text window, refresh text */
	}
	else
	{
		/* refresh graphics */
		int x, y, w, h;
		x = xevent->x;
		y = xevent->y;
		w = xevent->width;
		h = xevent->height;
		if (x+w > g_screen_width)
		{
			w = g_screen_width-x;
		}
		if (y+h > g_screen_height)
		{
			h = g_screen_height-y;
		}
		if (x < g_screen_width && y < g_screen_height && w > 0 && h > 0)
		{
			XPutImage(m_Xdp, m_Xw, m_Xgc, m_Ximage, x, y, x, y,
		xevent->width, xevent->height);
		}
	}
}

void X11Driver::ev_button_press(XEvent *xevent)
{
	int done = 0;
	int banding = 0;
	int bandx0;
	int bandy0;
	int bandx1;
	int bandy1;

	if (m_look_at_mouse == LOOK_MOUSE_ZOOM_BOX || g_zoom_off == 0)
	{
		m_last_x = xevent->xbutton.x;
		m_last_y = xevent->xbutton.y;
		return;
	}

	bandx1 = bandx0 = xevent->xbutton.x;
	bandy1 = bandy0 = xevent->xbutton.y;
	while (!done)
	{
		XNextEvent(m_Xdp, xevent);
		switch (xevent->type)
		{
		case MotionNotify:
			while (XCheckWindowEvent(m_Xdp, m_Xw, PointerMotionMask, xevent))
			{
				1;
			}
			if (banding)
			{
				XDrawRectangle(m_Xdp, m_Xw, m_Xgc, MIN(bandx0, bandx1), 
					MIN(bandy0, bandy1), ABS(bandx1-bandx0), 
					ABS(bandy1-bandy0));
			}
			bandx1 = xevent->xmotion.x;
			bandy1 = xevent->xmotion.y;
			if (ABS(bandx1-bandx0)*g_final_aspect_ratio > ABS(bandy1-bandy0))
			{
				bandy1 = SIGN(bandy1-bandy0)*ABS(bandx1-bandx0)*
					static_cast<int>(g_final_aspect_ratio) + bandy0;
			}
			else
			{
				bandx1 = SIGN(bandx1-bandx0)*ABS(bandy1-bandy0)/
					static_cast<int>(g_final_aspect_ratio) + bandx0;
			}

			if (!banding)
			{
				/* Don't start rubber-banding until the mouse
					gets moved.  Otherwise a click messes up the
					window */
				if (ABS(bandx1-bandx0) > 10 || ABS(bandy1-bandy0) > 10)
				{
					banding = 1;
					XSetForeground(m_Xdp, m_Xgc, do_fake_lut(g_colors-1));
					XSetFunction(m_Xdp, m_Xgc, GXxor);
				}
			}
			if (banding)
			{
				XDrawRectangle(m_Xdp, m_Xw, m_Xgc, MIN(bandx0, bandx1), 
					MIN(bandy0, bandy1), ABS(bandx1-bandx0), 
					ABS(bandy1-bandy0));
			}
			XFlush(m_Xdp);
			break;

		case ButtonRelease:
			done = 1;
			break;
		}
	}

	if (!banding)
	{
		return;
	}

	XDrawRectangle(m_Xdp, m_Xw, m_Xgc, MIN(bandx0, bandx1), 
		MIN(bandy0, bandy1), ABS(bandx1-bandx0), 
		ABS(bandy1-bandy0));
	if (bandx1 == bandx0)
	{
		bandx1 = bandx0+1;
	}
	if (bandy1 == bandy0)
	{
		bandy1 = bandy0+1;
	}
	g_z_rotate = 0;
	g_z_skew = 0;
	g_zbx = (MIN(bandx0, bandx1)-g_sx_offset)/g_dx_size;
	g_zby = (MIN(bandy0, bandy1)-g_sy_offset)/g_dy_size;
	g_z_width = ABS(bandx1-bandx0)/g_dx_size;
	g_z_depth = g_z_width;
	if (!inside_help)
	{
		m_xbufkey = FIK_ENTER;
	}
	if (m_xlastcolor != -1)
	{
		XSetForeground(m_Xdp, m_Xgc, do_fake_lut(m_xlastcolor));
	}
	XSetFunction(m_Xdp, m_Xgc, m_xlastfcn);
	m_XZoomWaiting = 1;
	zoom_box_draw(0);
}

void X11Driver::ev_motion_notify(XEvent *xevent)
{
	if (g_edit_pal_cursor && !inside_help)
	{
		while (XCheckWindowEvent(m_Xdp, m_Xw, PointerMotionMask, xevent))
		{
			1;
		}

		if (xevent->xmotion.state & Button2Mask ||
			(xevent->xmotion.state & (Button1Mask | Button3Mask)))
		{
			m_button_num = 3;
		}
		else if (xevent->xmotion.state & Button1Mask)
		{
			m_button_num = 1;
		}
		else if (xevent->xmotion.state & Button3Mask)
		{
			m_button_num = 2;
		}
		else
		{
			m_button_num = 0;
		}

		if (m_look_at_mouse == LOOK_MOUSE_ZOOM_BOX && m_button_num != 0)
		{
			m_dx += (xevent->xmotion.x-m_last_x)/MOUSE_SCALE;
			m_dy += (xevent->xmotion.y-m_last_y)/MOUSE_SCALE;
			m_last_x = xevent->xmotion.x;
			m_last_y = xevent->xmotion.y;
		}
		else
		{
			cursor_set_position(xevent->xmotion.x, xevent->xmotion.y);
			m_xbufkey = FIK_ENTER;
		}
	}
}

/*----------------------------------------------------------------------
 *
 * handle_events --
 *
 *	Handles X events.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Does event action.
 *
 *----------------------------------------------------------------------
 */
void X11Driver::handle_events()
{
	XEvent xevent;

	if (m_doredraw)
	{
		redraw();
	}

	while (XPending(m_Xdp) && !m_xbufkey)
	{
		XNextEvent(m_Xdp, &xevent);
		switch (xevent.type)
		{
		case KeyRelease:
			ev_key_release(&xevent.xkey);
			break;

		case KeyPress:
			if (ev_key_press(&xevent.xkey))
			{
				return;
			}
			break;

		case MotionNotify:
			ev_motion_notify(&xevent);
			break;

		case ButtonPress:
			ev_button_press(&xevent);
			break;

		case Expose:
			ev_expose(&xevent.xexpose);
			break;
		}
	}

	if (!m_xbufkey && g_edit_pal_cursor && !inside_help
		&& m_look_at_mouse == LOOK_MOUSE_ZOOM_BOX && (m_dx != 0 || m_dy != 0))
	{
		if (ABS(m_dx) > ABS(m_dy))
		{
			if (m_dx > 0)
			{
				m_xbufkey = mousefkey[m_button_num][0]; /* right */
				m_dx--;
			}
			else if (m_dx < 0)
			{
				m_xbufkey = mousefkey[m_button_num][1]; /* left */
				m_dx++;
			}
		}
		else
		{
			if (m_dy > 0)
			{
				m_xbufkey = mousefkey[m_button_num][2]; /* down */
				m_dy--;
			}
			else if (m_dy < 0)
			{
				m_xbufkey = mousefkey[m_button_num][3]; /* up */
				m_dy++;
			}
		}
	}
}

/* Check if there is a character waiting for us.  */
int X11Driver::input_pending()
{
	return XPending(m_Xdp);
#if 0
	struct timeval now;
	fd_set read_fds;

	memset(&now, 0, sizeof(now));
	FD_ZERO(&read_fds);
	FD_SET(ConnectionNumber(m_Xdp), &read_fds);

	return select(1, &read_fds, NULL, NULL, &now) > 0;
#endif
}

/*----------------------------------------------------------------------
 *
 * pr_dwmroot --
 *
 *	Search for a dec window manager root window.
 *
 * Results:
 *	Returns the root window.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
Window X11Driver::pr_dwmroot(Display *dpy, Window pwin)
{
	/* search for DEC Window Manager root */
	XWindowAttributes pxwa, cxwa;
	Window  root, parent, *child;
	unsigned int     i, nchild;

	if (!XGetWindowAttributes(dpy, pwin, &pxwa))
	{
		printf("Search for root: XGetWindowAttributes failed\n");
		return RootWindow(dpy, m_Xdscreen);
	}
	if (XQueryTree(dpy, pwin, &root, &parent, &child, &nchild))
	{
		for (i = 0; i < nchild; i++)
		{
			if (!XGetWindowAttributes(dpy, child[i], &cxwa))
			{
				printf("Search for root: XGetWindowAttributes failed\n");
				return RootWindow(dpy, m_Xdscreen);
			}
			if (pxwa.width == cxwa.width && pxwa.height == cxwa.height)
			{
				return pr_dwmroot(dpy, child[i]);
			}
		}
		return(pwin);
	}
	else
	{
		printf("xfractint: failed to find root window\n");
		return RootWindow(dpy, m_Xdscreen);
	}
}

/*
 *----------------------------------------------------------------------
 *
 * FindRootWindow --
 *
 *	Find the root or virtual root window.
 *
 * Results:
 *	Returns the root window.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
Window X11Driver::FindRootWindow()
{
	int i;
	m_Xroot = RootWindow(m_Xdp, m_Xdscreen);
	m_Xroot = pr_dwmroot(m_Xdp, m_Xroot); /* search for DEC wm root */


 {  /* search for swm/tvtwm root (from ssetroot by Tom LaStrange) */
		Atom __SWM_VROOT = None;
		Window rootReturn, parentReturn, *children;
		unsigned int numChildren;

		__SWM_VROOT = XInternAtom(m_Xdp, "__SWM_VROOT", False);
		XQueryTree(m_Xdp, m_Xroot, &rootReturn, &parentReturn,
			&children, &numChildren);
		for (i = 0; i < numChildren; i++)
		{
			Atom actual_type;
			int actual_format;
			unsigned long nitems, bytesafter;
			Window *newRoot = NULL;

			if (XGetWindowProperty (m_Xdp, children[i], __SWM_VROOT,
				(long) 0, (long) 1,
				False, XA_WINDOW,
				&actual_type, &actual_format,
				&nitems, &bytesafter,
				(unsigned char **) &newRoot) == Success &&
				newRoot)
			{
				m_Xroot = *newRoot; break;
			}
		}
	}
	return m_Xroot;
}

/*
 *----------------------------------------------------------------------
 *
 * RemoveRootPixmap --
 *
 *	Clean up old pixmap on the root window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Pixmap is cleaned up.
 *
 *----------------------------------------------------------------------
 */
void X11Driver::RemoveRootPixmap()
{
	Atom prop, type;
	int format;
	unsigned long nitems, after;
	Pixmap *pm;

	prop = XInternAtom(m_Xdp, "_XSETROOT_ID", False);
	if (XGetWindowProperty(m_Xdp, m_Xroot, prop, (long) 0, (long) 1, 1,
			AnyPropertyType, &type, &format, &nitems, &after,
			(unsigned char **) &pm) == Success
		&& nitems == 1)
	{
		if (type == XA_PIXMAP && format == 32 && after == 0)
		{
			XKillClient(m_Xdp, (XID)*pm);
			XFree((char *)pm);
		}
	}
}

void X11Driver::load_font()
{
	m_font_info = XLoadQueryFont(m_Xdp, m_x_font_name);
	if (m_font_info == NULL)
	{
		m_font_info = XLoadQueryFont(m_Xdp, "6x12");
	}
}

/*
 *----------------------------------------------------------------------
 *
 * find_font --
 *
 *	Get an 8x8 font.
 *
 * Results:
 *	Returns a pointer to the bits.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
BYTE *X11Driver::find_font(int parm)
{
	if (!m_font_table)
	{
		XImage *font_image;
		char str[8];
		int i, j, k, l;
		int width;
		Pixmap font_pixmap;
		XGCValues values;
		GC font_gc;

		m_font_table = (unsigned char *) malloc(128*8);
		bzero(m_font_table, 128*8);

		m_xlastcolor = -1;
		if (! m_font_info)
		{
			load_font();
		}
		if (m_font_info == NULL)
		{
			return NULL;
		}
		width = m_font_info->max_bounds.width;
		if (m_font_info->max_bounds.width > 8 ||
			m_font_info->max_bounds.width != m_font_info->min_bounds.width)
		{
			fprintf(stderr, "Bad font\n");
			free(m_font_table);
			m_font_table = NULL;
			return NULL;
		}

		font_pixmap = XCreatePixmap(m_Xdp, m_Xw, 64, 8, 1);
		assert(font_pixmap);
		values.background = 0;
		values.foreground = 1;
		values.font = m_font_info->fid;
		font_gc = XCreateGC(m_Xdp, font_pixmap,
			GCForeground | GCBackground | GCFont, &values);
		assert(font_gc);

		for (i = 0; i < 128; i += 8)
		{
			for (j = 0; j < 8; j++)
			{
				str[j] = i+j;
			}

			XDrawImageString(m_Xdp, font_pixmap, m_Xgc, 0, 8, str, 8);
			font_image =
	XGetImage(m_Xdp, font_pixmap, 0, 0, 64, 8, AllPlanes, XYPixmap);
			assert(font_image);
			for (j = 0; j < 8; j++)
			{
				for (k = 0; k < 8; k++)
				{
					for (l = 0; l < width; l++)
					{
						if (XGetPixel(font_image, j*width+l, k))
						{
							m_font_table[(i+j)*8+k] = (m_font_table[(i+j)*8+k] << 1) | 1;
						}
						else
						{
							m_font_table[(i+j)*8+k] = (m_font_table[(i+j)*8+k] << 1);
						}
					}
				}
			}
			XDestroyImage(font_image);
		}

		XFreeGC(m_Xdp, font_gc);
		XFreePixmap(m_Xdp, font_pixmap);
	}
}

/*----------------------------------------------------------------------
 *
 * flush --
 *
 *	Sync the x server
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Modifies window.
 *
 *----------------------------------------------------------------------
 */
void X11Driver::flush()
{
	XSync(m_Xdp, False);
}

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
int X11Driver::initialize(int &argc, char **argv)
{
	/*
	* Check a bunch of important conditions
	*/
	if (sizeof(short) != 2)
	{
		fprintf(stderr, "Error: need short to be 2 bytes\n");
		exit(-1);
	}
	if (sizeof(long) < sizeof(FLOAT4))
	{
		fprintf(stderr, "Error: need sizeof(long) >= sizeof(FLOAT4)\n");
		exit(-1);
	}

	initdacbox();

	signal(SIGFPE, fpe_handler);
#ifdef FPUERR
	signal(SIGABRT, SIG_IGN);
	/*
		setup the IEEE-handler to forget all common ( invalid,
		divide by zero, overflow ) signals. Here we test, if 
		such ieee trapping is supported.
	*/
	if (ieee_handler("set", "common", continue_hdl) != 0 )
	{
		printf("ieee trapping not supported here \n");
	}
#endif

	/* filter out x11 arguments */
	for (int i = 0; i < argc; i++)
	{
		int count = check_arg(i, argc, argv);
		if (count)
		{
			for (int j = i; j < argc - count; j++)
			{
				argv[j] = argv[j + count];
			}
			argc -= count;
		}
	}

	m_Xdp = XOpenDisplay(m_Xdisplay);
	if (m_Xdp == NULL)
	{
		terminate();
		return 0;
	}
	m_Xdscreen = XDefaultScreen(m_Xdp);

	erase_text_screen();

	/* should enumerate visuals here and build video modes for each */
	add_video_mode(this, x11_video_table[0]);

	return 1;
}

int X11Driver::validate_mode(const VIDEOINFO &mode)
{
	return 0;
}

/*----------------------------------------------------------------------
 *
 * terminate --
 *
 *	Cleanup windows and stuff.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Cleans up.
 *
 *----------------------------------------------------------------------
 */
void X11Driver::terminate()
{
	doneXwindow();
}

void X11Driver::pause()
{
}

void X11Driver::resume()
{
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
void X11Driver::schedule_alarm(int soon)
{
	if (!m_fastmode)
	{
		return;
	}

	// TODO: use X timer functionality?
#if 0
	signal(SIGALRM, (SignalHandler) setredrawscreen);
	if (soon)
	{
		alarm(1);
	}
	else
	{
		alarm(DRAW_INTERVAL);
	}
#endif

	m_alarmon = 1;
}

/*----------------------------------------------------------------------
 *
 * window --
 *
 *	Make the X window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Makes window.
 *
 *----------------------------------------------------------------------
 */
void X11Driver::window()
{
	XSetWindowAttributes Xwatt;
	XGCValues Xgcvals;
	int Xwinx = 0, Xwiny = 0;
	int i;

	g_adapter = 0;

	/* We have to do some X stuff even for disk video, to parse the geometry
	* string */

	if (m_Xgeometry && !m_on_root)
	{
		XGeometry(m_Xdp, m_Xdscreen, m_Xgeometry, DEFXY, 0, 1, 1, 0, 0,
			&Xwinx, &Xwiny, &m_Xwinwidth, &m_Xwinheight);
	}
	if (m_sync)
	{
		XSynchronize(m_Xdp, True);
	}
	XSetErrorHandler(errhand);
	m_Xsc = ScreenOfDisplay(m_Xdp, m_Xdscreen);
	select_visual();
	if (m_fixcolors > 0)
	{
		g_colors = m_fixcolors;
	}

	if (m_fullscreen || m_on_root)
	{
		m_Xwinwidth = DisplayWidth(m_Xdp, m_Xdscreen);
		m_Xwinheight = DisplayHeight(m_Xdp, m_Xdscreen);
	}
	g_screen_width = m_Xwinwidth;
	g_screen_height = m_Xwinheight;

	Xwatt.background_pixel = BlackPixelOfScreen(m_Xsc);
	Xwatt.bit_gravity = StaticGravity;
	m_doesBacking = DoesBackingStore(m_Xsc);
	if (m_doesBacking)
	{
		Xwatt.backing_store = Always;
	}
	else
	{
		Xwatt.backing_store = NotUseful;
	}
	if (m_on_root)
	{
		m_Xroot = FindRootWindow();
		RemoveRootPixmap();
		m_Xgc = XCreateGC(m_Xdp, m_Xroot, 0, &Xgcvals);
		m_Xpixmap = XCreatePixmap(m_Xdp, m_Xroot,
				m_Xwinwidth, m_Xwinheight, m_Xdepth);
		m_Xw = m_Xroot;
		XFillRectangle(m_Xdp, m_Xpixmap, m_Xgc, 0, 0, m_Xwinwidth, m_Xwinheight);
		XSetWindowBackgroundPixmap(m_Xdp, m_Xroot, m_Xpixmap);
	}
	else
	{
		m_Xroot = DefaultRootWindow(m_Xdp);
		m_Xw = XCreateWindow(m_Xdp, m_Xroot, Xwinx, Xwiny,
			m_Xwinwidth, m_Xwinheight, 0, m_Xdepth,
			InputOutput, CopyFromParent,
			CWBackPixel | CWBitGravity | CWBackingStore,
			&Xwatt);
		XStoreName(m_Xdp, m_Xw, "xfractint");
		m_Xgc = XCreateGC(m_Xdp, m_Xw, 0, &Xgcvals);
	}
	g_colors = xcmapstuff();
	if (g_rotate_hi == 255)
	{
		g_rotate_hi = g_colors-1;
	}
	{
		unsigned long event_mask = KeyPressMask | KeyReleaseMask | ExposureMask;
		if (!m_on_root)
		{
			event_mask |= ButtonPressMask | ButtonReleaseMask
				| PointerMotionMask;
		}
		XSelectInput(m_Xdp, m_Xw, event_mask);
	}

	if (!m_on_root)
	{
		XSetBackground(m_Xdp, m_Xgc, do_fake_lut(m_pixtab[0]));
		XSetForeground(m_Xdp, m_Xgc, do_fake_lut(m_pixtab[1]));
		Xwatt.background_pixel = do_fake_lut(m_pixtab[0]);
		XChangeWindowAttributes(m_Xdp, m_Xw, CWBackPixel, &Xwatt);
		XMapWindow(m_Xdp, m_Xw);
	}

	resize();
	flush();
	write_palette();

	x11_video_table[0].x_dots = g_screen_width;
	x11_video_table[0].y_dots = g_screen_height;
	x11_video_table[0].colors = g_colors;
	x11_video_table[0].dotmode = 19;
}

/*----------------------------------------------------------------------
 *
 * x11_resize --
 *
 *	Look after resizing the window if necessary.
 *
 * Results:
 *	Returns 1 for resize, 0 for no resize.
 *
 * Side effects:
 *	May reallocate data structures.
 *
 *----------------------------------------------------------------------
 */
int X11Driver::resize()
{
	static int oldx = -1, oldy = -1;
	int junki;
	unsigned int junkui;
	Window junkw;
	unsigned int width, height;
	int Xmwidth;
	Status status;

	XGetGeometry(m_Xdp, m_Xw, &junkw, &junki, &junki, &width, &height,
		&junkui, &junkui);

	if (oldx != width || oldy != height)
	{
		g_screen_width = width;
		g_screen_height = height;
		x11_video_table[0].x_dots = g_screen_width;
		x11_video_table[0].y_dots = g_screen_height;
		oldx = g_screen_width;
		oldy = g_screen_height;
		m_Xwinwidth = g_screen_width;
		m_Xwinheight = g_screen_height;
		g_screen_aspect_ratio = g_screen_height/(float) g_screen_width;
		g_final_aspect_ratio = g_screen_aspect_ratio;
		Xmwidth = (m_Xdepth > 1) ? g_screen_width: (1 + g_screen_width/8);
		if (m_pixbuf != NULL)
		{
			free(m_pixbuf);
		}
		m_pixbuf = (BYTE *) malloc(m_Xwinwidth *sizeof(BYTE));
		if (m_Ximage != NULL)
		{
			free(m_Ximage->data);
			XDestroyImage(m_Ximage);
		}
		m_Ximage = XCreateImage(m_Xdp, m_Xvi, m_Xdepth, ZPixmap, 0, NULL, g_screen_width, 
			g_screen_height, m_Xdepth, 0);
		if (m_Ximage == NULL)
		{
			printf("XCreateImage failed\n");
			terminate();
			exit(-1);
		}
		m_Ximage->data = (char *) malloc(m_Ximage->bytes_per_line * m_Ximage->height);
		if (m_Ximage->data == NULL)
		{
			fprintf(stderr, "Malloc failed: %d\n", m_Ximage->bytes_per_line *
				m_Ximage->height);
			exit(-1);
		}
		clearXwindow();
		return 1;
	}
	else
	{
		return 0;
	}
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
void X11Driver::redraw()
{
	if (m_alarmon)
	{
		XPutImage(m_Xdp, m_Xw, m_Xgc, m_Ximage, 0, 0, 0, 0,
			g_screen_width, g_screen_height);
		if (m_on_root)
		{
			XPutImage(m_Xdp, m_Xpixmap, m_Xgc, m_Ximage, 0, 0, 0, 0,
				g_screen_width, g_screen_height);
		}
		m_alarmon = 0;
	}
	m_doredraw = 0;
}

/*
 *----------------------------------------------------------------------
 *
 * read_palette --
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
int X11Driver::read_palette()
{
	int i;
	if (g_got_real_dac == 0)
	{
		return -1;
	}
	for (i = 0; i < 256; i++)
	{
		g_dac_box[i][0] = m_cols[i].red/1024;
		g_dac_box[i][1] = m_cols[i].green/1024;
		g_dac_box[i][2] = m_cols[i].blue/1024;
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
 *	Changes the displayed g_colors.
 *
 *----------------------------------------------------------------------
 */
int X11Driver::write_palette()
{
	int i;

	if (!g_got_real_dac)
	{
		if (m_fake_lut)
		{
			/* !g_got_real_dac, fake_lut => truecolor, directcolor displays */
			static unsigned char last_dac[256][3];
			static int last_dac_inited = False;

			for (i = 0; i < 256; i++)
			{
				if (!last_dac_inited ||
					last_dac[i][0] != g_dac_box[i][0] ||
					last_dac[i][1] != g_dac_box[i][1] ||
					last_dac[i][2] != g_dac_box[i][2])
				{
					m_cols[i].flags = DoRed | DoGreen | DoBlue;
					m_cols[i].red = g_dac_box[i][0]*1024;
					m_cols[i].green = g_dac_box[i][1]*1024;
					m_cols[i].blue = g_dac_box[i][2]*1024;

					if (m_cmap_pixtab_alloced)
					{
						XFreeColors(m_Xdp, m_Xcmap, m_cmap_pixtab + i, 1, None);
					}
					if (XAllocColor(m_Xdp, m_Xcmap, &m_cols[i]))
					{
						m_cmap_pixtab[i] = m_cols[i].pixel;
					}
					else
					{
						assert(1);
						printf("Allocating color %d failed.\n", i);
					}

					last_dac[i][0] = g_dac_box[i][0];
					last_dac[i][1] = g_dac_box[i][1];
					last_dac[i][2] = g_dac_box[i][2];
				}
			}
			m_cmap_pixtab_alloced = True;
			last_dac_inited = True;
		}
		else
		{
			/* !g_got_real_dac, !fake_lut => static color, static gray displays */
			assert(1);
		}
	}
	else
	{
		/* g_got_real_dac => grayscale or pseudocolor displays */
		for (i = 0; i < 256; i++)
		{
			m_cols[i].pixel = m_pixtab[i];
			m_cols[i].flags = DoRed | DoGreen | DoBlue;
			m_cols[i].red = g_dac_box[i][0]*1024;
			m_cols[i].green = g_dac_box[i][1]*1024;
			m_cols[i].blue = g_dac_box[i][2]*1024;
		}
		XStoreColors(m_Xdp, m_Xcmap, m_cols, g_colors);
		XFlush(m_Xdp);
	}

	return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * read_pixel --
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
int X11Driver::read_pixel(int x, int y)
{
	if (m_fake_lut)
	{
		int i;
		XPixel pixel = XGetPixel(m_Ximage, x, y);
		for (i = 0; i < 256; i++)
		{
			if (m_cmap_pixtab[i] == pixel)
			{
				return i;
			}
		}
		return 0;
	}
	else
	{
		return m_ipixtab[XGetPixel(m_Ximage, x, y)];
	}
}

/*
 *----------------------------------------------------------------------
 *
 * write_pixel --
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
void  X11Driver::write_pixel(int x, int y, int color)
{
#ifdef DEBUG /* Debugging checks */
	if (color >= g_colors || color < 0)
	{
		printf("Color %d too big %d\n", color, g_colors);
	}
	if (x >= g_screen_width || x < 0 || y >= g_screen_height || y < 0)
	{
		printf("Bad coord %d %d\n", x, y);
	}
#endif
	if (m_xlastcolor != color)
	{
		XSetForeground(m_Xdp, m_Xgc, do_fake_lut(m_pixtab[color]));
		m_xlastcolor = color;
	}
	XPutPixel(m_Ximage, x, y, do_fake_lut(m_pixtab[color]));
	if (m_fastmode == 1 && get_help_mode() != HELPXHAIR)
	{
		if (!m_alarmon)
		{
			schedule_alarm(0);
		}
	}
	else
	{
		XDrawPoint(m_Xdp, m_Xw, m_Xgc, x, y);
		if (m_on_root)
		{
			XDrawPoint(m_Xdp, m_Xpixmap, m_Xgc, x, y);
		}
	}
}

/*
 *----------------------------------------------------------------------
 *
 * read_span --
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
void X11Driver::read_span(int y, int x, int lastx, BYTE *pixels)
{
	int i, width;
	width = lastx-x+1;
	for (i = 0; i < width; i++)
	{
		pixels[i] = read_pixel(x+i, y);
	}
}

/*
 *----------------------------------------------------------------------
 *
 * write_span --
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
void X11Driver::write_span(int y, int x, int lastx, const BYTE *pixels)
{
	int width;
	int i;
	const BYTE *pixline;

#if 1
	if (x == lastx)
	{
		write_pixel(x, y, pixels[0]);
		return;
	}
	width = lastx-x+1;
	if (m_usepixtab)
	{
		for (i=0;i < width;i++)
		{
			m_pixbuf[i] = m_pixtab[pixels[i]];
		}
		pixline = m_pixbuf;
	}
	else
	{
		pixline = pixels;
	}
	for (i = 0; i < width; i++)
	{
		XPutPixel(m_Ximage, x+i, y, do_fake_lut(pixline[i]));
	}
	if (m_fastmode == 1 && get_help_mode() != HELPXHAIR)
	{
		if (!m_alarmon)
		{
			schedule_alarm(0);
		}
	}
	else
	{
		XPutImage(m_Xdp, m_Xw, m_Xgc, m_Ximage, x, y, x, y, width, 1);
		if (m_on_root)
		{
			XPutImage(m_Xdp, m_Xpixmap, m_Xgc, m_Ximage, x, y, x, y, width, 1);
		}
	}
#else
	width = lastx-x+1;
	for (i=0;i < width;i++)
	{
		write_pixel(x+i, y, pixels[i]);
	}
#endif
}

void X11Driver::get_truecolor(int x, int y, int &r, int &g, int &b, int &a)
{
}

void X11Driver::put_truecolor(int x, int y, int r, int g, int b, int a)
{
}

/*
 *----------------------------------------------------------------------
 *
 * X11Driver::set_line_mode --
 *
 *	Set line mode to 0=draw or 1=xor.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sets mode.
 *
 *----------------------------------------------------------------------
 */
void X11Driver::set_line_mode(int mode)
{
	m_xlastcolor = -1;
	if (mode == 0)
	{
		XSetFunction(m_Xdp, m_Xgc, GXcopy);
		m_xlastfcn = GXcopy;
	}
	else
	{
		XSetForeground(m_Xdp, m_Xgc, do_fake_lut(g_colors-1));
		m_xlastcolor = -1;
		XSetFunction(m_Xdp, m_Xgc, GXxor);
		m_xlastfcn = GXxor;
	}
}

/*
 *----------------------------------------------------------------------
 *
 * draw_line --
 *
 *	Draw a line.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Modifies window.
 *
 *----------------------------------------------------------------------
 */
void X11Driver::draw_line(int x1, int y1, int x2, int y2, int color)
{
	XDrawLine(m_Xdp, m_Xw, m_Xgc, x1, y1, x2, y2);
}

void X11Driver::display_string(int x, int y, int fg, int bg, const char *text)
{
}

void X11Driver::save_graphics()
{
}

void X11Driver::restore_graphics()
{
}

/*----------------------------------------------------------------------
 *
 * get_key --
 *
 *	Get a key from the keyboard or the X server.
 *	Blocks if block = 1.
 *
 * Results:
 *	Key, or 0 if no key and not blocking.
 *	Times out after .5 second.
 *
 * Side effects:
 *	Processes X events.
 *
 *----------------------------------------------------------------------
 */
int X11Driver::get_key()
{
	int block = 1;
	static int skipcount = 0;

	while (1)
	{
		/* Don't check X events every time, since that is expensive */
		skipcount++;
		if (block == 0 && skipcount < 25)
		{
			break;
		}
		skipcount = 0;

		handle_events();

		if (m_xbufkey)
		{
			int ch = m_xbufkey;
			m_xbufkey = 0;
			skipcount = 9999; /* If we got a key, check right away next time */
			return translate_key(ch);
		}

		if (!block)
		{
			break;
		}

		{
			fd_set reads;
			struct timeval tout;
			int status;

			FD_ZERO(&reads);
			FD_SET(0, &reads);
			tout.tv_sec = 0;
			tout.tv_usec = 500000;

			FD_SET(ConnectionNumber(m_Xdp), &reads);
			status = select(ConnectionNumber(m_Xdp) + 1, &reads, NULL, NULL, &tout);

			if (status <= 0)
			{
				return 0;
			}
		}
	}

	return 0;
}

int X11Driver::key_cursor(int row, int col)
{
	return 0;
}

int X11Driver::key_pressed()
{
	return 0;
}

int X11Driver::wait_key_pressed(int timeout)
{
	return 0;
}

void X11Driver::unget_key(int key)
{
}

/*
 *----------------------------------------------------------------------
 *
 * shell
 *
 *	Exit to a unix shell.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Goes to shell
 *
 *----------------------------------------------------------------------
 */
void X11Driver::shell()
{
	SignalHandler sigint;
	char *shell;
	char *argv[2];
	int pid, donepid;

	sigint = (SignalHandler) signal(SIGINT, SIG_IGN);
	shell = getenv("SHELL");
	if (shell == NULL)
	{
		shell = SHELL;
	}

	argv[0] = shell;
	argv[1] = NULL;

	/* Clean up the window */

	/* Fork the shell; it should be something like an xterm */
	pid = fork();
	if (pid < 0)
	{
		perror("fork to shell");
	}
	if (pid == 0)
	{
		execvp(shell, argv);
		perror("fork to shell");
		exit(1);
	}

	/* Wait for the shell to finish */
	while (1)
	{
		donepid = wait(0);
		if (donepid < 0 || donepid == pid)
		{
			break;
		}
	}

	signal(SIGINT, (SignalHandler) sigint);
	putchar('\n');
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

; Unix: We ignore ax,bx,cx,dx.  g_dot_mode is the "mode" field in the video
; table.  We use mode 19 for the X window.
*/
void X11Driver::set_video_mode(const VIDEOINFO &mode)
{
	if (g_disk_flag)
	{
		disk_end();
	}
	end_video();
	g_good_mode = 1;
	switch (g_dot_mode)
	{
	case 0:				/* text */
#if 0
		clear();
#endif
		break;

	case 19: /* X window */
		g_dot_write = driver_write_pixel;
		g_dot_read = driver_read_pixel;
		g_line_read = driver_read_span;
		g_line_write = driver_write_span;
		start_video();
		set_for_graphics();
		break;

	default:
		printf("Bad mode %d\n", g_dot_mode);
		exit(-1);
	} 
	if (g_dot_mode !=0)
	{
		read_palette();
		g_and_color = g_colors-1;
		g_box_count =0;
	}
}

void X11Driver::put_string(int row, int col, int attr, const char *msg)
{
	int r, c;

	fprintf(stderr, "x11_put_string(%d,%d, %u): ``%s''\n", row, col, attr, msg);

	if (row != -1)
	{
		g_text_row = row;
	}
	if (col != -1)
	{
		g_text_col = col;
	}

	r = g_text_row + g_text_rbase;
	c = g_text_col + g_text_cbase;

	while (*msg)
	{
		if ('\n' == *msg)
		{
			g_text_row++;
			r++;
			g_text_col = 0;
			c = g_text_cbase;
		}
		else
		{
			assert(r < TEXT_HEIGHT);
			assert(c < TEXT_WIDTH);
			m_text_screen[r][c] = *msg;
			m_text_attr[r][c] = attr;
			g_text_col++;
			c++;
		}
		msg++;
	}
}

void X11Driver::set_for_text()
{
	if (! m_font_info)
	{
		load_font();
	}
	/* map text screen child window */
	/* allocate text colors in window's colormap, save window's colors */
	/* refresh window with last contents of text_screen */
	m_text_modep = True;
	fprintf(stderr, "x11_set_for_text\n");
}

void X11Driver::set_for_graphics()
{
	/* unmap text screen child window */
	/* restore colormap from saved copy */
	/* expose will be sent for newly visible area */
	m_text_modep = False;
	fprintf(stderr, "x11_set_for_graphics\n");
}

void X11Driver::set_clear()
{
	erase_text_screen();
	set_for_text();
}

void X11Driver::move_cursor(int row, int col)
{
	/* draw reverse video text cursor at new position */
	fprintf(stderr, "x11_move_cursor(%d,%d)\n", row, col);
}

void X11Driver::hide_text_cursor()
{
	/* erase cursor if currently drawn */
	fprintf(stderr, "x11_hide_text_cursor\n");
}

void X11Driver::set_attr(int row, int col, int attr, int count)
{
	int i = col;

	while (count)
	{
		assert(row < TEXT_HEIGHT);
		assert(i < TEXT_WIDTH);
		m_text_attr[row][i] = attr;
		if (++i == TEXT_WIDTH)
		{
			i = 0;
			row++;
		}
		count--;
	}
	/* refresh text */
	fprintf(stderr, "x11_set_attr(%d,%d, %d): %d\n", row, col, count, attr);
}

void X11Driver::scroll_up(int top, int bot)
{
	int r, c;
	assert(bot <= TEXT_HEIGHT);
	for (r = top; r < bot; r++)
	{
		for (c = 0; c < TEXT_WIDTH; c++)
		{
			m_text_attr[r][c] = m_text_attr[r+1][c];
			m_text_screen[r][c] = m_text_screen[r+1][c];
		}
	}
	for (c = 0; c < TEXT_WIDTH; c++)
	{
		m_text_attr[bot][c] = 0;
		m_text_screen[bot][c] = ' ';
	}
	/* draw text */
	fprintf(stderr, "x11_scroll_up(%d, %d)\n", top, bot);
}

void X11Driver::stack_screen()
{
	fprintf(stderr, "x11_stack_screen\n");
}

void X11Driver::unstack_screen()
{
	fprintf(stderr, "x11_unstack_screen\n");
}

void X11Driver::discard_screen()
{
	fprintf(stderr, "x11_discard_screen\n");
}

int X11Driver::init_fm()
{
	return 0;
}

/*
	Sound a tone based on the value of the parameter

       0 = normal completion of task
       1 = interrupted task
       2 = error contition
*/
void X11Driver::buzzer(int buzzer_type)
{
	if ((g_sound_state.m_flags & SOUNDFLAG_ORBITMASK) != 0)
	{
		printf("\007");
		fflush(stdout);
	}
	if (buzzer_type == 0)
	{
		redrawscreen();
	}
}

int X11Driver::sound_on(int freq)
{
	fprintf(stderr, "x11_sound_on(%d)\n", freq);
	return 0;
}

void X11Driver::sound_off()
{
	fprintf(stderr, "x11_sound_off\n");
}

void X11Driver::mute()
{
}

int X11Driver::diskp()
{
	return 0;
}

int X11Driver::get_char_attr()
{
	return 0;
}

void X11Driver::put_char_attr(int char_attr)
{
}

void X11Driver::delay(int ms)
{
	static struct timeval delay;
	delay.tv_sec = ms/1000;
	delay.tv_usec = (ms % 1000)*1000;
#if defined( __SVR4) || defined(LINUX)
	select(0, (fd_set *) 0, (fd_set *) 0, (fd_set *) 0, &delay);
#else
	select(0, (int *) 0, (int *) 0, (int *) 0, &delay);
#endif
}

void X11Driver::get_max_screen(int &width, int &height) const
{
	width = m_Xsc->width;
	height = m_Xsc->height;
}

void X11Driver::set_keyboard_timeout(int ms)
{
	m_keyboard_timeout = ms;
}

int X11Driver::get_char_attr_rowcol(int row, int col)
{
	return 0;
}

void X11Driver::put_char_attr_rowcol(int row, int col, int char_attr)
{
}

/*
 * place this last in the file to avoid having to forward declare routines
 */
static X11Driver x11_driver_info("x11", "An X Window System driver");

X11Driver::X11Driver(const char *name, const char *description)
	: NamedDriver(name, description),
	m_look_at_mouse(LOOK_MOUSE_NONE),
	m_on_root(0),
	m_fullscreen(0),
	m_sharecolor(0),
	m_privatecolor(0),
	m_fixcolors(0),
	m_sync(0),
	m_Xdisplay(""),
	m_Xgeometry(NULL),
	m_doesBacking(0),
	m_usepixtab(0),
	m_cmap_pixtab_alloced(0),
	m_fake_lut(0),
	m_fastmode(0),
	m_alarmon(0),
	m_doredraw(0),
	m_Xdp(NULL),
	m_Xw(None),
	m_Xgc(None),
	m_Xvi(NULL),
	m_Xsc(NULL),
	m_Xcmap(None),
	m_Xdepth(0),
	m_Ximage(NULL),
	m_Xdata(NULL),
	m_Xdscreen(0),
	m_Xpixmap(None),
	m_Xwinwidth(DEFX),
	m_Xwinheight(DEFY),
	m_Xroot(None),
	m_xlastcolor(-1),
	m_xlastfcn(GXcopy),
	m_pixbuf(NULL),
	m_XZoomWaiting(0),
	m_x_font_name(FONT),
	m_font_info(NULL),
	m_xbufkey(0),
	m_fontPtr(NULL),
	m_font_table(NULL),
	m_text_modep(False),
	m_ctl_mode(0),
	m_shift_mode(0),
	m_button_num(0),
	m_last_x(0),
	m_last_y(0),
	m_dx(0),
	m_dy(0),
	m_keyboard_timeout(0)
{
}

AbstractDriver *x11_driver = &x11_driver_info;
