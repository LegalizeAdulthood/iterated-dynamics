/* d_win32_disk.c
 *
 * Routines for a Win32 disk video mode driver for fractint.
 */
#include <assert.h>
#include <stdio.h>

#define WIN32_LEAN_AND_MEAN
#define STRICT
#include <windows.h>

#include "helpdefs.h"
#include "port.h"
#include "prototyp.h"
#include "drivers.h"
#include "WinText.h"

#if !defined(ASSERT)
#if defined(_DEBUG)
#define ASSERT(x_) assert(x_)
#else
#define ASSERT(x_)
#endif
#endif

#define MAXSCREENS 3

extern HINSTANCE g_instance;

#define NUM_OF(ary_) (sizeof(ary_)/sizeof(ary_[0]))

#define DI(name_) Win32DiskDriver *name_ = (Win32DiskDriver *) drv

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
#if 0
/* external variables (set in the FRACTINT.CFG file, but findable here */
extern int dotmode;  /* video access method (= 19)    */
extern int sxdots, sydots;  /* total # of dots on the screen   */
extern int sxoffs, syoffs;  /* offset of drawing area          */
extern int colors;   /* maximum colors available    */
extern int g_init_mode;
extern int g_adapter;
extern int g_got_real_dac;
extern int inside_help;
extern  float finalaspectratio;
extern  float screenaspect;
extern int lookatmouse;
extern VIDEOINFO g_video_table[];
/* the video-palette array (named after the VGA adapter's video-DAC) */
extern unsigned char g_dac_box[256][3];
extern void drawbox();
extern int g_text_type;
extern int helpmode;
extern int rotate_hi;
extern void fpe_handler();
#ifdef FPUERR
static void continue_hdl(int sig, int code, struct sigcontext *scp, char *addr);
#endif
#define DEFX 640
#define DEFY 480
#define DEFXY "640x480+0+0"
extern int editpal_cursor;
extern void Cursor_SetPos();
#define SENS 1
#define ABS(x)		((x) > 0   ? (x) : -(x))
#define MIN(x, y)	((x) < (y) ? (x) : (y))
#define SIGN(x)		((x) > 0   ? 1   : -1)
#define SHELL "/bin/csh"
#define DRAW_INTERVAL 6
#endif
/*  /\  /\  /\  /\  /\  /\  /\  /\  /\  /\  /\  /\  /\  /\  /\  /\  /\  /\  /\  /\
/* /  \/  \/  \/  \/  \/  \/  \/  \/  \/  \/  \/  \/  \/  \/  \/  \/  \/  \/  \/  \
/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/

typedef void t_dotwriter(int, int, int);
typedef int  t_dotreader(int, int);
typedef void t_linewriter(int y, int x, int lastx, BYTE *pixels);
typedef void t_linereader(int y, int x, int lastx, BYTE *pixels);
typedef void t_swapper(void);

/* read/write-a-dot/line routines */
extern t_dotwriter *dotwrite;
extern t_dotreader *dotread;
extern t_linewriter *linewrite;
extern t_linereader *lineread;
extern t_linewriter *normaline;
extern t_linereader *normalineread;

extern int startvideo();
extern int endvideo();
extern void writevideo(int x, int y, int color);
extern int readvideo(int x, int y);
extern int readvideopalette(void);
extern int writevideopalette(void);

/* VIDEOINFO:															*/
/*         char    name[26];       Adapter name (IBM EGA, etc)          */
/*         char    comment[26];    Comments (UNTESTED, etc)             */
/*         int     keynum;         key number used to invoked this mode */
/*                                 2-10 = F2-10, 11-40 = S,C,A{F1-F10}  */
/*         int     videomodeax;    begin with INT 10H, AX=(this)        */
/*         int     videomodebx;                 ...and BX=(this)        */
/*         int     videomodecx;                 ...and CX=(this)        */
/*         int     videomodedx;                 ...and DX=(this)        */
/*                                 NOTE:  IF AX==BX==CX==0, SEE BELOW   */
/*         int     dotmode;        video access method used by asm code */
/*                                      1 == BIOS 10H, AH=12,13 (SLOW)  */
/*                                      2 == access like EGA/VGA        */
/*                                      3 == access like MCGA           */
/*                                      4 == Tseng-like  SuperVGA*256   */
/*                                      5 == P'dise-like SuperVGA*256   */
/*                                      6 == Vega-like   SuperVGA*256   */
/*                                      7 == "Tweaked" IBM-VGA ...*256  */
/*                                      8 == "Tweaked" SuperVGA ...*256 */
/*                                      9 == Targa Format               */
/*                                      10 = Hercules                   */
/*                                      11 = "disk video" (no screen)   */
/*                                      12 = 8514/A                     */
/*                                      13 = CGA 320x200x4, 640x200x2   */
/*                                      14 = Tandy 1000                 */
/*                                      15 = TRIDENT  SuperVGA*256      */
/*                                      16 = Chips&Tech SuperVGA*256    */
/*         int     xdots;          number of dots across the screen     */
/*         int     ydots;          number of dots down the screen       */
/*         int     colors;         number of colors available           */

static VIDEOINFO modes[] =
{
	{
		"IBM 256-Color VGA/MCGA	  ", "Quick and LOTS of colors ",
		F2, 0, 0, 0, 0,				19, 320, 200, 256
	},
	{
		"IBM VGA (non-std)        ", "Quick and LOTS of colors ",
		F3, 0, 0, 0, 0,				19, 320, 400, 256
	},
	{
		"IBM VGA (non-std)        ", "Quick and LOTS of colors ",
		F4, 0, 0, 0, 0,				19, 360, 480, 256
	},
	{
		"SuperVGA/VESA Autodetect ", "Works with most SuperVGA ",
		F5, 0, 0, 0, 0,				19, 640, 400, 256
	},
	{
		"SuperVGA/VESA Autodetect ", "Works with most SuperVGA ",
		F6, 0, 0, 0, 0,				19, 640, 480, 256
	},
	{
		"SuperVGA/VESA Autodetect ", "Works with most SuperVGA",
		F7 , 0, 0, 0, 0,				  27,   800,   600, 256
	},
	{
		"SuperVGA/VESA Autodetect ", "Works with most SuperVGA",
		F8, 0, 0, 0, 0,				  27,  1024,   768, 256
	},
	{
		"VESA Standard interface  ", "OK: Andy Fu - Chips&Tech",
		F9, 0, 0, 0, 0,				  28,  1280,  1024, 256
	},
	{
		"Disk video               ", "                        ",
		SF1, 0, 0, 0, 0,				  28,  1024,  1024, 256
	},
	{
		"Disk video               ", "                        ",
		SF2, 0, 0, 0, 0,				  28,  1600,  1200, 256
	},
	{
		"Disk video               ", "                        ",
		SF3, 0, 0, 0, 0,				  28,  2048,  2048, 256
	},
	{
		"Disk video               ", "                        ",
		SF4, 0, 0, 0, 0,				  28,  4096,  4096, 256
	},
	{
		"Disk video               ", "                        ",
		SF5, 0, 0, 0, 0,				  28,  8192,  8192, 256
	}
};

typedef struct tagWin32DiskDriver Win32DiskDriver;
struct tagWin32DiskDriver
{
	Driver pub;

	BYTE *pixels;
	size_t pixels_len;
	int width;
	int height;
	unsigned char cols[256][3];

	int screen_count;
	BYTE *saved_screens[MAXSCREENS];
	int saved_cursor[MAXSCREENS+1];
	BOOL cursor_shown;
	int cursor_row;
	int cursor_col;

#if 0
	int simple_input; /* Use simple input (debugging) */
	int old_fcntl;
	int alarmon; /* 1 if the refresh alarm is on */
	int doredraw; /* 1 if we have a redraw waiting */
	int width, height;
	int xlastcolor;
	unsigned char *fontPtr;
#endif
};

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
static int
check_arg(Win32DiskDriver *di, int argc, char **argv, int *i)
{
#if 0
	if (strcmp(argv[*i], "-disk") == 0)
	{
		return 1;
	}
	else if (strcmp(argv[*i], "-simple") == 0)
	{
		di->simple_input = 1;
		return 1;
	}
	else if (strcmp(argv[*i], "-geometry") == 0 && *i+1 < argc)
	{
		di->Xgeometry = argv[(*i)+1];
		(*i)++;
		return 1;
	}
#endif
	return 0;
}

void
function_called(const char *fn, const char *file, unsigned line, int error)
{
	char buffer[512];
	sprintf(buffer, "%s(%d): %c%s called\n", file, line, error ? '!' : '?', fn);
	OutputDebugString(buffer);
	if (error)
	{
		ASSERT(FALSE);
	}
}

void
ods(const char *file, unsigned int line, const char *format, ...)
{
	char header[MAX_PATH+1] = { 0 };
	char buffer[MAX_PATH+1] = { 0 };
	va_list args;

	va_start(args, format);
	_vsnprintf(buffer, MAX_PATH, format, args);
	_snprintf(header, MAX_PATH, "%s(%d): %s\n", file, line, buffer);
	va_end(args);

	OutputDebugString(header);
}
#if RT_VERBOSE
#define ODS(text_)						ods(__FILE__, __LINE__, text_)
#define ODS1(fmt_, arg_)				ods(__FILE__, __LINE__, fmt_, arg_)
#define ODS2(fmt_, a1_, a2_)			ods(__FILE__, __LINE__, fmt_, a1_, a2_)
#define ODS3(fmt_, a1_, a2_, a3_)		ods(__FILE__, __LINE__, fmt_, a1_, a2_, a3_)
#define ODS4(fmt_, _1, _2, _3, _4)		ods(__FILE__, __LINE__, fmt_, _1, _2, _3, _4)
#define ODS5(fmt_, _1, _2, _3, _4, _5)	ods(__FILE__, __LINE__, fmt_, _1, _2, _3, _4, _5)
#else
#define ODS(text_)
#define ODS1(fmt_, arg_)
#define ODS2(fmt_, a1_, a2_)
#define ODS3(fmt_, a1_, a2_, a3_)
#define ODS4(fmt_, _1, _2, _3, _4)
#define ODS5(fmt_, _1, _2, _3, _4, _5)
#endif

/*----------------------------------------------------------------------
*
* win32_disk_terminate --
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
static void
win32_disk_terminate(Driver *drv)
{
	DI(di);
	ODS("win32_disk_terminate");

	if (di->pixels)
	{
		free(di->pixels);
		di->pixels = NULL;
	}
#if 0
	Win32DiskDriver *di = (Win32DiskDriver *) drv;
	if (!di->simple_input)
	{
		fcntl(0, F_SETFL, di->old_fcntl);
	}
	mvcur(0, COLS-1, LINES-1, 0);
	nocbreak();
	echo();
	endwin();
	delscreen(di->term);
#endif
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
static void
initdacbox()
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

/*----------------------------------------------------------------------
*
* win32_disk_init --
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
static int
win32_disk_init(Driver *drv, int *argc, char **argv)
{
	Win32DiskDriver *di = (Win32DiskDriver *) drv;

	ODS1("win32_disk_init %d", *argc);
	if (!wintext_initialize(g_instance, NULL, "FractInt for Windows"))
	{
		return FALSE;
	}

	initdacbox();

#if 0
	if (!di->simple_input)
	{
		signal(SIGINT, (SignalHandler) goodbye);
	}
	signal(SIGFPE, fpe_handler);
	/*
	signal(SIGTSTP, goodbye);
	*/
#ifdef FPUERR
	signal(SIGABRT, SIG_IGN);
	/*
	setup the IEEE-handler to forget all common ( invalid,
	divide by zero, overflow ) signals. Here we test, if 
	such ieee trapping is supported.
	*/
	if (ieee_handler("set", "common", continue_hdl) != 0 )
		printf("ieee trapping not supported here \n");
#endif

	/* filter out driver arguments */
	{
		int count = *argc;
		char **argv_copy = (char **) malloc(sizeof(char *)*count);
		int i;
		int copied;

		for (i = 0; i < count; i++)
		argv_copy[i] = argv[i];

		copied = 0;
		for (i = 0; i < count; i++)
		if (! check_arg(di, i, argv, &i))
			argv[copied++] = argv_copy[i];
		*argc = copied;
	}
#endif

	{
		int m;
		int c = NUM_OF(modes);
		for (m = 0; m < c; m++)
		{
			add_video_mode(drv, &modes[m]);
		}
	}

	return TRUE;
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
static void
continue_hdl(int sig, int code, struct sigcontext *scp, char *addr)
{
	int i;
	char out[20];
	/*		if you want to get all messages enable this statement.    */
	/*  printf("ieee exception code %x occurred at pc %X\n", code, scp->sc_pc); */
	/*	clear all excaption flags					  */
	i = ieee_flags("clear", "exception", "all", out);
}
#endif


/*----------------------------------------------------------------------
* win32_disk_flush
*/
static void
win32_disk_flush(Driver *drv)
{
	ODS("win32_disk_flush");
#if 0
	Win32DiskDriver *di = (Win32DiskDriver *) drv;
	wrefresh(di->curwin);
#endif
}

/*----------------------------------------------------------------------
* win32_disk_resize
*/
static int
win32_disk_resize(Driver *drv)
{
	ODS("win32_disk_resize");
	return 0;
}


/*----------------------------------------------------------------------
* win32_disk_read_palette
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
static int
win32_disk_read_palette(Driver *drv)
{
	DI(di);
	int i;

	ODS("win32_disk_read_palette");
	if (g_got_real_dac == 0)
		return -1;
	for (i = 0; i < 256; i++)
	{
		g_dac_box[i][0] = di->cols[i][0];
		g_dac_box[i][1] = di->cols[i][1];
		g_dac_box[i][2] = di->cols[i][2];
	}
	return 0;
}

/*
*----------------------------------------------------------------------
*
* win32_disk_write_palette --
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
static int
win32_disk_write_palette(Driver *drv)
{
	DI(di);
	int i;

	ODS("win32_disk_write_palette");
	for (i = 0; i < 256; i++)
	{
		di->cols[i][0] = g_dac_box[i][0];
		di->cols[i][1] = g_dac_box[i][1];
		di->cols[i][2] = g_dac_box[i][2];
	}

	return 0;
}

static int
win32_disk_start_video(Driver *drv)
{
	ODS("win32_disk_start_video");
	return 0;
}

static int
win32_disk_end_video(Driver *drv)
{
	ODS("win32_disk_end_video");
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
static void
setredrawscreen(void)
{
#if 0
	((Win32DiskDriver *) display)->doredraw = 1;
#endif
}

/*
*----------------------------------------------------------------------
*
* win32_disk_schedule_alarm --
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
static void
win32_disk_schedule_alarm(Driver *drv, int soon)
{
	Win32DiskDriver *di = (Win32DiskDriver *) drv;
	ODS1("win32_disk_schedule_alarm %d", soon);
#if 0
	signal(SIGALRM, (SignalHandler) setredrawscreen);
	if (soon)
		alarm(1);
	else
		alarm(DRAW_INTERVAL);
	di->alarmon = 1;
#endif
}

/*
*----------------------------------------------------------------------
*
* win32_disk_write_pixel --
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
static void 
win32_disk_write_pixel(Driver *drv, int x, int y, int color)
{
	DI(di);
	ASSERT(di->pixels);
	ASSERT(x >= 0 && x < di->width);
	ASSERT(y >= 0 && y < di->height);
	di->pixels[y*di->width + x] = (BYTE) (color & 0xFF);
}

/*
*----------------------------------------------------------------------
*
* win32_disk_read_pixel --
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
static int
win32_disk_read_pixel(Driver *drv, int x, int y)
{
	DI(di);
	ASSERT(di->pixels);
	ASSERT(x >= 0 && x < di->width);
	ASSERT(y >= 0 && y < di->height);
	return (int) di->pixels[y*di->width + x];
}

/*
*----------------------------------------------------------------------
*
* win32_disk_write_span --
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
static void
win32_disk_write_span(Driver *drv, int y, int x, int lastx, BYTE *pixels)
{
	int i;
	int width = lastx-x+1;
	ODS3("win32_disk_write_span (%d,%d,%d)", y, x, lastx);

	for (i = 0; i < width; i++)
	{
		win32_disk_write_pixel(drv, x+i, y, pixels[i]);
	}
}

/*
*----------------------------------------------------------------------
*
* win32_disk_read_span --
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
static void
win32_disk_read_span(Driver *drv, int y, int x, int lastx, BYTE *pixels)
{
	int i, width;
	ODS3("win32_disk_read_span (%d,%d,%d)", y, x, lastx);
	width = lastx-x+1;
	for (i = 0; i < width; i++)
	{
		pixels[i] = win32_disk_read_pixel(drv, x+i, y);
	}
}

static void
win32_disk_set_line_mode(Driver *drv, int mode)
{
	ODS1("win32_disk_set_line_mode %d", mode);
}

static void
win32_disk_draw_line(Driver *drv, int x1, int y1, int x2, int y2, int color)
{
	ODS5("win32_disk_draw_line (%d,%d) (%d,%d) %d", x1, y1, x2, y2, color);
	draw_line(x1, y1, x2, y2, color);
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
static int
getachar(Win32DiskDriver *di)
{
#if 0
	if (di->simple_input)
	{
		return getchar();
	}
	else
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
#endif
}

/*----------------------------------------------------------------------
*
* translatekey --
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
static int
translatekey(int ch)
{
	if (ch >= 'a' && ch <= 'z')
		return ch;
	else
	{
		switch (ch)
		{
		case 'I':		return INSERT;
		case 'D':		return FIK_DELETE;
		case 'U':		return PAGE_UP;
		case 'N':		return PAGE_DOWN;
		case CTL('O'):	return CTL_HOME;
		case CTL('E'):	return CTL_END;
		case 'H':		return LEFT_ARROW;
		case 'L':		return RIGHT_ARROW;
		case 'K':		return UP_ARROW;
		case 'J':		return DOWN_ARROW;
		case 1115:		return LEFT_ARROW_2;
		case 1116:		return RIGHT_ARROW_2;
		case 1141:		return UP_ARROW_2;
		case 1145:		return DOWN_ARROW_2;
		case 'O':		return HOME;
		case 'E':		return END;
		case '\n':		return ENTER;
		case CTL('T'):	return CTL_ENTER;
		case -2:		return CTL_ENTER_2;
		case CTL('U'):	return CTL_PAGE_UP;
		case CTL('N'):	return CTL_PAGE_DOWN;
		case '{':		return CTL_MINUS;
		case '}':		return CTL_PLUS;
#if 0
		/* we need ^I for tab */
		case CTL('I'):	return CTL_INSERT;
#endif
		case CTL('D'):	return CTL_DEL;
		case '!':		return F1;
		case '@':		return F2;
		case '#':		return F3;
		case '$':		return F4;
		case '%':		return F5;
		case '^':		return F6;
		case '&':		return F7;
		case '*':		return F8;
		case '(':		return F9;
		case ')':		return F10;
		default:
			return ch;
		}
	}
}

/*----------------------------------------------------------------------
*
* handleesc --
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
static int
handleesc(Win32DiskDriver *di)
{
	return ESC;
}

/*
*----------------------------------------------------------------------
*
* win32_disk_redraw --
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
static void
win32_disk_redraw(Driver *drv)
{
	ODS("win32_disk_redraw");
	wintext_paintscreen(0, 80, 0, 24);
}

/*----------------------------------------------------------------------
*
* win32_disk_get_key --
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
static int
win32_disk_get_key(Driver *drv)
{
	int ch = wintext_getkeypress(1);

	if (ch)
	{
		extern int inside_help;
		keybuffer = ch;
		if (F1 == ch && helpmode && !inside_help)
		{
			keybuffer = 0;
			inside_help = 1;
			help(0);
			inside_help = 0;
			ch = 0;
		}
		else if (TAB == ch && tabmode)
		{
			keybuffer = 0;
			tab_display();
			ch = 0;
		}
	}

	return ch;
}

static void
parse_geometry(const char *spec, int *x, int *y, int *width, int *height)
{
	/* do something like XParseGeometry() */
	if (2 == sscanf(spec, "%dx%d", width, height))
	{
		/* all we care about is width and height for disk output */
		*x = 0;
		*y = 0;
	}
}

static void
init_pixels(Win32DiskDriver *di)
{
	if (di->pixels != NULL)
	{
		free(di->pixels);
	}
	di->width = sxdots;
	di->height = sydots;
	di->pixels_len = di->width * di->height * sizeof(BYTE);
	ASSERT(di->pixels_len > 0);
	di->pixels = (BYTE *) malloc(di->pixels_len);
	memset(di->pixels, 0, di->pixels_len);
}

static void
win32_disk_window(Driver *drv)
{
	DI(di);
	ODS("win32_disk_window");

	wintext_texton();
#if 0
	int offx, offy;
	int i;
	Win32DiskDriver *di = (Win32DiskDriver *) drv;

	if (!di->simple_input)
	{
		di->old_fcntl = fcntl(0, F_GETFL);
		fcntl(0, F_SETFL, FNDELAY);
	}

	g_adapter = 0;

	/* We have to do some X stuff even for disk video, to parse the geometry
	* string */
	g_got_real_dac = 1;
	colors = 256;
	for (i = 0; i < colors; i++)
	{
		di->pixtab[i] = i;
		di->ipixtab[i] = i;
	}
	if (di->Xgeometry)
		parse_geometry(di->Xgeometry, &offx, &offy, &di->width, &di->height);
	sxdots = di->width;
	sydots = di->height;
	win32_disk_flush(drv);
	win32_disk_write_palette(drv);

	g_video_table[0].xdots = sxdots;
	g_video_table[0].ydots = sydots;
	g_video_table[0].colors = colors;
	g_video_table[0].dotmode = DOTMODE_ROLL_YOUR_OWN;
#endif
}

/*
*----------------------------------------------------------------------
*
* shell_to_dos --
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
static void
win32_disk_shell(Driver *drv)
{
	windows_shell_to_dos();
}

static int s_video_flag = 0;

void videomode(t_dotwriter dot_write, t_dotreader dot_read,
			   t_linewriter line_write, t_linereader line_read,
			   t_swapper swapper)
{
	dotwrite = dot_write;
	dotread = dot_read;
	linewrite = line_write;
	lineread = line_read;
	g_swap_setup = swapper;
	g_and_color = colors-1;
	boxcount = 0;
	g_dac_learn = 1;
	g_dac_count = cyclelimit;
}

t_dotwriter win32_dot_writer;
t_dotreader win32_dot_reader;
t_linewriter win32_line_writer;
t_linereader win32_line_reader;
t_swapper win32_swap_setup;

void win32_dot_writer(int x, int y, int color)
{
	driver_write_pixel(x, y, color);
}
int win32_dot_reader(int x, int y)
{
	return driver_read_pixel(x, y);
}
void win32_line_writer(int row, int col, int lastcol, BYTE *pixels)
{
	driver_write_span(row, col, lastcol, pixels);
}
void win32_line_reader(int row, int col, int lastcol, BYTE *pixels)
{
	driver_read_span(row, col, lastcol, pixels);
}
void win32_swap_setup(void) {}

/*
; **************** Function setvideomode(ax, bx, cx, dx) ****************
;       This function sets the (alphanumeric or graphic) video mode
;       of the monitor.   Called with the proper values of AX thru DX.
;       No returned values, as there is no particular standard to
;       adhere to in this case.

;       (SPECIAL "TWEAKED" VGA VALUES:  if AX==BX==CX==0, assume we have a
;       genuine VGA or register compatable adapter and program the registers
;       directly using the coded value in DX)

; Unix: We ignore ax,bx,cx,dx.  dotmode is the "mode" field in the video
; table.  We use mode 19 for the X window.
*/
static void
win32_disk_set_video_mode(Driver *drv, int ax, int bx, int cx, int dx)
{
	DI(di);

	init_pixels(di);

	/* initially, set the virtual line to be the scan line length */
	g_vxdots = sxdots;
	g_is_true_color = 0;				/* assume not truecolor */
	g_vesa_x_res = 0;					/* reset indicators used for */
	g_vesa_y_res = 0;					/* virtual screen limits estimation */

	if (g_disk_flag)
	{
		enddisk();
	}

	if (s_video_flag)
	{
		endvideo();
		s_video_flag = 0;
	}

	g_ok_to_print = 1;
	g_good_mode = 1;

	if (driver_diskp())
	{
		extern t_swapper null_swap;
		startdisk();
		dotwrite = writedisk;
		dotread = readdisk;
		lineread = normalineread;
		linewrite = normaline;
	}
	else if (0 == dotmode)
	{
		videomode(win32_dot_writer, win32_dot_reader,
			win32_line_writer, win32_line_reader,
			win32_swap_setup);
	}
	else
	{
		/* bad video mode 'dotmode' */
		ASSERT(FALSE);
	}

	if (dotmode !=0)
	{
		driver_read_palette();
		g_and_color = colors-1;
		boxcount = 0;
	}
}

/*
; PUTSTR.asm puts a string directly to video display memory. Called from C by:
;    putstring(row, col, attr, string) where
;         row, col = row and column to start printing.
;         attr = color attribute.
;         string = pointer to the null terminated string to print.
;    Written for the A86 assembler (which has much less 'red tape' than MASM)
;    by Bob Montgomery, Orlando, Fla.             7-11-88
;    Adapted for MASM 5.1 by Tim Wegner          12-11-89
;    Furthur mucked up to handle graphics
;       video modes by Bert Tyler                 1-07-90
;    Reworked for:  row,col update/inherit;
;       620x200x2 inverse video;  ptr to string;
;       fix to avoid scrolling when last posn chgd;
;       divider removed;  newline ctl chars;  PB  9-25-90
*/
static void
win32_disk_put_string(Driver *drv, int row, int col, int attr, const char *msg)
{
	if (-1 != row)
	{
		g_text_row = row;
	}
	if (-1 != col)
	{
		g_text_col = col;
	}
	{
		int abs_row = g_text_rbase + g_text_row;
		int abs_col = g_text_cbase + g_text_col;
		ASSERT(abs_row >= 0 && abs_row < WINTEXT_MAX_ROW);
		ASSERT(abs_col >= 0 && abs_col < WINTEXT_MAX_COL);
		wintext_putstring(abs_col, abs_row, attr, msg, &g_text_row, &g_text_col);
	}
}

static void
win32_disk_set_for_text(Driver *drv)
{
	ODS("win32_disk_set_for_text: TODO: hide graphics window, show text window");
}

static void
win32_disk_set_for_graphics(Driver *drv)
{
	ODS("win32_disk_set_for_graphics: TODO: hide text window, show graphics window");
}

static void
win32_disk_set_clear(Driver *drv)
{
	wintext_clear();
}

/************** Function scrollup(toprow, botrow) ******************
*
*       Scroll the screen up (from toprow to botrow)
*/
static void
win32_disk_scroll_up(Driver *drv, int top, int bot)
{
	ODS2("win32_disk_scroll_up %d, %d", top, bot);
	wintext_scroll_up(top, bot);
}

static BYTE *
win32_disk_find_font(Driver *drv, int parm)
{
	ODS1("win32_disk_find_font %d", parm);
	return NULL;
}

static void
win32_disk_move_cursor(Driver *drv, int row, int col)
{
	DI(di);
	ODS2("win32_disk_move_cursor %d,%d", row, col);

	if (row != -1)
	{
		di->cursor_row = row;
		g_text_row = row;
	}
	if (col != -1)
	{
		di->cursor_col = col;
		g_text_col = col;
	}
	row = di->cursor_row;
	col = di->cursor_col;
	wintext_cursor(g_text_cbase + col, g_text_rbase + row, 1);
	di->cursor_shown = TRUE;

#if 0
	Win32DiskDriver *di = (Win32DiskDriver *) drv;
	if (row == -1)
	{
		row = textrow;
	}
	else
	{
		textrow = row;
	}
	if (col == -1)
	{
		col = textcol;
	}
	else
	{
		textcol = col;
	}
	wmove(di->curwin,row,col);
#endif
}

static void
win32_disk_set_attr(Driver *drv, int row, int col, int attr, int count)
{
	if (-1 != row)
	{
		g_text_row = row;
	}
	if (-1 != col)
	{
		g_text_col = col;
	}
	wintext_set_attr(g_text_rbase + g_text_row, g_text_cbase + g_text_col, attr, count);
}

static void
win32_disk_hide_text_cursor(Driver *drv)
{
	DI(di);
	if (TRUE == di->cursor_shown)
	{
		di->cursor_shown = FALSE;
		wintext_hide_cursor();
	}
	ODS("win32_disk_hide_text_cursor");
}

/*
* Implement stack and unstack window functions by using multiple curses
* windows.
*/
static void
win32_disk_stack_screen(Driver *drv)
{
	Win32DiskDriver *di = (Win32DiskDriver *) drv;
	int i;

	ODS("win32_disk_stack_screen");
	di->saved_cursor[di->screen_count+1] = g_text_row*80 + g_text_col;
	if (++di->screen_count)
	{ /* already have some stacked */
		static char msg[] =
		{ "stackscreen overflow" };
		if ((i = di->screen_count - 1) >= MAXSCREENS)
		{ /* bug, missing unstack? */
			stopmsg(STOPMSG_NO_STACK, msg);
			exit(1);
		}
		di->saved_screens[i] = wintext_screen_get();
		win32_disk_set_clear(drv);
	}
	else
	{
		win32_disk_set_for_text(drv);
	}
}

static void
win32_disk_unstack_screen(Driver *drv)
{
	Win32DiskDriver *di = (Win32DiskDriver *) drv;

	ODS("win32_disk_unstack_screen");
	g_text_row = di->saved_cursor[di->screen_count] / 80;
	g_text_col = di->saved_cursor[di->screen_count] % 80;
	if (--di->screen_count >= 0)
	{ /* unstack */
		wintext_screen_set(di->saved_screens[di->screen_count]);
		free(di->saved_screens[di->screen_count]);
		di->saved_screens[di->screen_count] = NULL;
	}
	else
	{
		win32_disk_set_for_graphics(drv);
	}
	win32_disk_move_cursor(drv, -1, -1);
}

static void
win32_disk_discard_screen(Driver *drv)
{
	Win32DiskDriver *di = (Win32DiskDriver *) drv;

	ODS("win32_disk_discard_screen");
	if (--di->screen_count >= 0)
	{ /* unstack */
		if (di->saved_screens[di->screen_count])
		{
			free(di->saved_screens[di->screen_count]);
			di->saved_screens[di->screen_count] = NULL;
		}
	}
}

static int
win32_disk_init_fm(Driver *drv)
{
	ODS("win32_disk_init_fm");
	return 0;
}

static void
win32_disk_buzzer(Driver *drv, int kind)
{
	ODS1("win32_disk_buzzer %d", kind);
	MessageBeep(MB_OK);
}

static int
win32_disk_sound_on(Driver *drv, int freq)
{
	ODS1("win32_disk_sound_on %d", freq);
	return 0;
}

static void
win32_disk_sound_off(Driver *drv)
{
	ODS("win32_disk_sound_off");
}

static void
win32_disk_mute(Driver *drv)
{
	ODS("win32_disk_mute");
}

static int
win32_disk_diskp(Driver *drv)
{
	return 1;
}

static int
win32_disk_key_pressed(Driver *drv)
{
	extern int keypressed(void);
	return keypressed();
}

static int
win32_disk_key_cursor(Driver *drv, int row, int col)
{
	DI(di);
	int result = win32_disk_key_pressed(drv);
	ODS2("win32_disk_key_cursor %d,%d", row, col);

	if (-1 != row)
	{
		di->cursor_row = row;
		g_text_row = row;
	}
	if (-1 != col)
	{
		di->cursor_col = col;
		g_text_col = col;
	}

	col = di->cursor_col;
	row = di->cursor_row;
	if (0 == result)
	{
		di->cursor_shown = TRUE;
		wintext_cursor(col, row, 1);
		result = win32_disk_get_key(drv);
		win32_disk_hide_text_cursor(drv);
		di->cursor_shown = FALSE;
	}
	return result;
}

static int
win32_disk_wait_key_pressed(Driver *drv, int timeout)
{
	extern int waitkeypressed(int timeout);
	return waitkeypressed(timeout);
}

static int
win32_disk_get_char_attr(Driver *drv)
{
	return 0;
}

static void
win32_disk_put_char_attr(Driver *drv, int char_attr)
{
}

static Win32DiskDriver win32_disk_driver_info =
{
	STD_DRIVER_STRUCT(win32_disk),

#if 0
	NULL,				/* term */
	NULL,				/* curwin */
	0,					/* simple_input */
	NULL,				/* Xgeometry */
	0,					/* old_fcntl */
	0,					/* alarmon */
	0,					/* doredraw */
	DEFX, DEFY,			/* width, height */
	-1,					/* xlastcolor */
	NULL,				/* pixbuf */
	{ 0 },				/* cols */
	{ 0 },				/* pixtab */
	{ 0 },				/* ipixtab */
	0,					/* xbufkey */
	NULL,				/* fontPtr */
	0,					/* screen_count */
	{ 0 },				/* saved_screens */
	{ 0 }				/* saved_cursor */
#endif
};

Driver *win32_disk_driver = &win32_disk_driver_info.pub;
