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

#define DI(name_) DriverWin32Disk *name_ = (DriverWin32Disk *) drv

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
extern void (*dotwrite)(int, int, int);	/* write-a-dot routine */
extern int (*dotread)(int, int); 		/* read-a-dot routine */
extern void (*linewrite)(void);			/* write-a-line routine */
extern void (*lineread)(void);			/* read-a-line routine */
extern void normalineread(void);
extern void normaline(void);
#endif
/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/

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

typedef struct tagDriverWin32Disk DriverWin32Disk;
struct tagDriverWin32Disk
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
check_arg(DriverWin32Disk *di, int argc, char **argv, int *i)
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
#define ODS(text_)						ods(__FILE__, __LINE__, text_)
#define ODS1(fmt_, arg_)				ods(__FILE__, __LINE__, fmt_, arg_)
#define ODS2(fmt_, a1_, a2_)			ods(__FILE__, __LINE__, fmt_, a1_, a2_)
#define ODS3(fmt_, a1_, a2_, a3_)		ods(__FILE__, __LINE__, fmt_, a1_, a2_, a3_)
#define ODS4(fmt_, _1, _2, _3, _4)		ods(__FILE__, __LINE__, fmt_, _1, _2, _3, _4)
#define ODS5(fmt_, _1, _2, _3, _4, _5)	ods(__FILE__, __LINE__, fmt_, _1, _2, _3, _4, _5)

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
	DriverWin32Disk *di = (DriverWin32Disk *) drv;
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
	DriverWin32Disk *di = (DriverWin32Disk *) drv;

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
		for (m = 0; m < NUM_OF(modes); m++)
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
	DriverWin32Disk *di = (DriverWin32Disk *) drv;
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
	((DriverWin32Disk *) display)->doredraw = 1;
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
	DriverWin32Disk *di = (DriverWin32Disk *) drv;
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

	ODS3("win32_disk_write_pixel (%d,%d)=%d", x, y, color);
	ASSERT(di->pixels);
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

	ODS2("win32_disk_read_pixel (%d,%d)", x, y);
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
getachar(DriverWin32Disk *di)
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
handleesc(DriverWin32Disk *di)
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
win32_disk_window(Driver *drv)
{
	DI(di);
	ODS("win32_disk_window");

	wintext_texton();
	if (di->pixels != NULL)
	{
		free(di->pixels);
	}
	di->pixels_len = di->width * di->height * sizeof(BYTE);
	di->pixels = (BYTE *) malloc(di->pixels_len);
	memset(di->pixels, 0, di->pixels_len);
#if 0
	int offx, offy;
	int i;
	DriverWin32Disk *di = (DriverWin32Disk *) drv;

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
	g_video_table[0].dotmode = 19;
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
	int setting_text = 0;
	ODS4("win32_disk_set_video_mode %d,%d,%d,%d", ax, bx, cx, dx);

#if 0
	int videoax, videobx, videocx, videodx;

	/*
	setvideomode    proc    uses di si es,argax:word,argbx:word,argcx:word,argdx:word
			mov     ax,sxdots               ; initially, set the virtual line
			mov     g_vxdots,ax               ; to be the scan line length
	*/
	g_vxdots = sxdots;
	/*
			xor     ax,ax
			mov     g_is_true_color,ax          ; assume not truecolor
			mov     g_vesa_x_res,ax            ; reset indicators used for
			mov     vesa_yres,ax            ;  virtual screen limits estimation
	*/
	g_is_true_color = 0;
	g_vesa_x_res = 0;
	g_vesa_y_res = 0;
	/*
			cmp     dotmode,0
			je      its_text
			mov     setting_text,0          ; try to set virtual stuff
			jmp     short its_graphics
	its_text:
			mov     setting_text,1          ; don't set virtual stuff
	*/
	setting_text = (0 == dotmode) ? TRUE : FALSE;
	/*
	its_graphics:
			cmp     dotmode, 29		; Targa truecolor mode?
			jne     NotTrueColorMode
			jmp     TrueColorAuto		; yup.
	*/
	if (29 != dotmode)
	{
		/*
		NotTrueColorMode:
				cmp     g_disk_flag,1              ; is disk video active?
				jne     nodiskvideo             ;  nope.
				call    far ptr enddisk         ; yup, external disk-video end routine
		*/
		if (1 == g_disk_flag)
		{
			enddisk();
		}

		/*
		nodiskvideo:
				cmp     videoflag,1             ; say, was the last video your-own?
				jne     novideovideo            ;  nope.
				call    far ptr endvideo        ; yup, external your-own end routine
				mov     videoflag,0             ; set flag: no your-own-video
				jmp     short notarga
		*/
		if (1 == videoflag)
		{
			endvideo();
			videoflag = 0;
		}
		else
		{
			/*
			novideovideo:
					cmp     tgaflag,1               ; TARGA MODIFIED 2 June 89 j mclain
					jne     notarga
					call    far ptr EndTGA          ; endTGA( void )
					mov     tgaflag,0               ; set flag: targa cleaned up
			*/
			if (1 == tgaflag)
			{
				EndTGA();
				tgaflag = 0;
			}
		}

		/*
		notarga:
				cmp     xga_isinmode,0          ; XGA in graphics mode?
				je      noxga                   ; nope
				mov     ax,0                    ; pull it out of graphics mode
				push    ax
				mov     xga_clearvideo,al
				call    far ptr xga_mode
				pop     ax
		*/
		if (0 != xga_isinmode)
		{
			xga_clearvideo = 0;
			xga_mode(0);
		}

		/*
		noxga:
				cmp     f85flag, 1              ; was the last video 8514?
				jne     no8514                  ; nope.
				cmp     ai_8514, 0              ;check afi flag, JCO 4/11/92
				jne     f85endafi
				call    far ptr close8514hw     ;use registers, JCO 4/11/92
				jmp     f85enddone
		f85endafi:
				call    far ptr close8514       ;use afi, JCO 4/11/92
		;       call    f85end          ;use afi
		f85enddone:
				mov     f85flag, 0
		*/
		if (1 == f85flag)
		{
			if (0 == ai_8514)
			{
				close8514hw();
			}
			else
			{
				close8514();
				f85end();
			}
			f85flag = 0;
		}
		/*
		no8514:
				cmp     HGCflag, 1              ; was last video Hercules
				jne     noHGC                   ; nope
				call    hgcend
				mov     HGCflag, 0
		*/
		if (1 == HGCflag)
		{
			hgcend();
			HGCflag = 0;
		}
		/*
		noHGC:
				mov     oktoprint,1             ; say it's OK to use printf()
				mov     g_good_mode,1              ; assume a good video mode
				mov     xga_loaddac,1           ; tell the XGA to fake a 'loaddac'
				mov     ax,video_bankadr        ; restore the results of 'whichvga()'
				mov     [bankadr],ax            ;  ...
				mov     ax,video_bankseg        ;  ...
				mov     [bankseg],ax            ;  ...

				mov     ax,argax                ; load up for the interrupt call
				mov     bx,argbx                ;  ...
				mov     cx,argcx                ;  ...
				mov     dx,argdx                ;  ...

				mov     videoax,ax              ; save the values for future use
				mov     videobx,bx              ;  ...
				mov     videocx,cx              ;  ...
				mov     videodx,dx              ;  ...

				call    setvideo                ; call the internal routine first

				cmp     g_good_mode,0              ; is it still a good video mode?
				jne     videomodeisgood         ; yup.
				mov     ax,offset nullwrite     ; set up null write-a-dot routine
				mov     bx,offset mcgaread      ; set up null read-a-dot  routine
				mov     cx,offset normaline     ; set up normal linewrite routine
				mov     dx,offset mcgareadline  ; set up normal linewrite routine
				mov     si,offset swapnormread  ; set up the normal swap routine
				jmp     videomode               ; return to common code
		*/
		oktoprint = 1;
		g_good_mode = 1;
		xga_loaddac = 1;
		*bankadr = video_bankadr;
		*bankseg = video_bankseg;
		videoax = ax;
		videobx = bx;
		videocx = cx;
		videodx = dx;
		/*
		videomodeisgood:
				mov     bx,dotmode              ; set up for a video table jump
				cmp     bx,30                   ; are we within the range of dotmodes?
				jbe     videomodesetup          ; yup.  all is OK
				mov     bx,0                    ; nope.  use dullnormalmode
		*/
		int i = dotmode;
		if (30 >= i)
		{
			i = 0;
		}
		/*
		videomodesetup:
				shl     bx,1                    ; switch to a word offset
				mov     bx,cs:videomodetable[bx]        ; get the next step
				jmp     bx                      ; and go there

		videomodetable  dw      offset dullnormalmode   ; mode 0
				dw      offset dullnormalmode   ; mode 1
				dw      offset vgamode          ; mode 2
				dw      offset mcgamode         ; mode 3
				dw      offset tseng256mode     ; mode 4
				dw      offset paradise256mode  ; mode 5
				dw      offset video7256mode    ; mode 6
				dw      offset tweak256mode     ; mode 7
				dw      offset everex16mode     ; mode 8
				dw      offset targaMode        ; mode 9
				dw      offset hgcmode          ; mode 10
				dw      offset diskmode         ; mode 11
				dw      offset f8514mode        ; mode 12
				dw      offset cgamode          ; mode 13
				dw      offset tandymode        ; mode 14
				dw      offset trident256mode   ; mode 15
				dw      offset chipstech256mode ; mode 16
				dw      offset ati256mode       ; mode 17
				dw      offset everex256mode    ; mode 18
				dw      offset yourownmode      ; mode 19
				dw      offset ati1024mode      ; mode 20
				dw      offset tseng16mode      ; mode 21
				dw      offset trident16mode    ; mode 22
				dw      offset video716mode     ; mode 23
				dw      offset paradise16mode   ; mode 24
				dw      offset chipstech16mode  ; mode 25
				dw      offset everex16mode     ; mode 26
				dw      offset VGAautomode      ; mode 27
				dw      offset VESAmode         ; mode 28
				dw      offset TrueColorAuto    ; mode 29
				dw      offset dullnormalmode   ; mode 30
				dw      offset dullnormalmode   ; mode 31

		tandymode:      ; from Joseph Albrecht
				mov     tandyseg,0b800h         ; set video segment address
				mov     tandyofs,0              ; set video offset address
				mov     ax,offset plottandy16   ; set up write-a-dot
				mov     bx,offset gettandy16    ; set up read-a-dot
				mov     cx,offset normaline     ; set up the normal linewrite routine
				mov     dx,offset normalineread ; set up the normal lineread  routine
				mov     si,offset swapnormread  ; set up the normal swap routine
				cmp     videoax,8               ; check for 160x200x16 color mode
				je      tandy16low              ; ..
				cmp     videoax,9               ; check for 320x200x16 color mode
				je      tandy16med              ; ..
				cmp     videoax,0ah             ; check for 640x200x4 color mode
				je      tandy4high              ; ..
				cmp     videoax,0bh             ; check for 640x200x16 color mode
				je      tandy16high             ; ..
		tandy16low:
				mov     tandyscan,offset scan16k; set scan line address table
				jmp     videomode               ; return to common code
		tandy16med:
				mov     tandyscan,offset scan32k; set scan line address table
				jmp     videomode               ; return to common code
		tandy4high:
				mov     ax,offset plottandy4    ; set up write-a-dot
				mov     bx,offset gettandy4     ; set up read-a-dot
				jmp     videomode               ; return to common code
		tandy16high:
				mov     tandyseg,0a000h         ; set video segment address
				mov     tandyofs,8000h          ; set video offset address
				mov     tandyscan,offset scan64k; set scan line address table
				jmp     videomode               ; return to common code
		dullnormalmode:
				mov     ax,offset normalwrite   ; set up the BIOS write-a-dot routine
				mov     bx,offset normalread    ; set up the BIOS read-a-dot  routine
				mov     cx,offset normaline     ; set up the normal linewrite routine
				mov     dx,offset normalineread ; set up the normal lineread  routine
				mov     si,offset swapnormread  ; set up the normal swap routine
				jmp     videomode               ; return to common code
		mcgamode:
				mov     ax,offset mcgawrite     ; set up MCGA write-a-dot routine
				mov     bx,offset mcgaread      ; set up MCGA read-a-dot  routine
				mov     cx,offset mcgaline      ; set up the MCGA linewrite routine
				mov     dx,offset mcgareadline  ; set up the MCGA lineread  routine
				mov     si,offset swap256       ; set up the MCGA swap routine
				jmp     videomode               ; return to common code
		tseng16mode:
				mov     tseng,1                 ; set chipset flag
				mov     [bankadr],offset $tseng
				mov     [bankseg],seg $tseng
				jmp     vgamode         ; set ega/vga functions
		trident16mode:
				mov     trident,1               ; set chipset flag
				mov     [bankadr],offset $trident
				mov     [bankseg],seg $trident
				jmp     vgamode
		video716mode:
				mov     video7,1                ; set chipset flag
				mov     [bankadr],offset $video7
				mov     [bankseg],seg $video7
				jmp     vgamode
		paradise16mode:
				mov     paradise,1              ; set chipset flag
				mov     [bankadr],offset $paradise
				mov     [bankseg],seg $paradise
				jmp     vgamode
		chipstech16mode:
				mov     chipstech,1             ; set chipset flag
				mov     [bankadr],offset $chipstech
				mov     [bankseg],seg $chipstech
				jmp     vgamode
		everex16mode:
				mov     everex,1                ; set chipset flag
				mov     [bankadr],offset $everex
				mov     [bankseg],seg $everex
				jmp     vgamode
		VESAmode:                               ; set VESA 16-color mode
				mov     ax,word ptr vesa_mapper
				mov     [bankadr],ax
				mov     ax,word ptr vesa_mapper+2
				mov     [bankseg],ax
				cmp     vesa_bitsppixel,15
				jae     VESAtruecolormode
		VGAautomode:                            ; set VGA auto-detect mode
				cmp     colors,256              ; 256 colors?
				je      VGAauto256mode          ; just like SuperVGA
				cmp     xga_isinmode,0          ; in an XGA mode?
				jne     xgamode
				cmp     colors,16               ; 16 colors?
				je      vgamode                 ; just like a VGA
			jmp     dullnormalmode          ; otherwise, use the BIOS

		VESAtruecolormode:
				mov     g_is_true_color,1
			mov	ax, offset VESAtruewrite   ; set up VESA true-color write-a-dot routine	
			mov	bx, offset VESAtrueread    ; set up VESA true-color read-a-dot routine	
			mov	cx, offset normaline       ; set up dullnormal linewrite routine	
			mov	dx, offset normalineread   ; set up dullnormal lineread routine	
				mov     si,offset swap256          ; set up the swap routine
				jmp     videomode                  ; return to common code

		xgamode:
				mov     ax,offset xga_16write      ; set up XGA write-a-dot routine
				mov     bx,offset xga_16read       ; set up XGA read-a-dot  routine
				mov     cx,offset xga_16linewrite  ; set up the XGA linewrite routine
				mov     dx,offset normalineread    ; set up the XGA lineread  routine
				mov     si,offset swap256          ; set up the swap routine
				jmp     videomode                  ; return to common code
			
		VGAauto256mode:
				jmp     super256mode            ; just like a SuperVGA
		egamode:
		vgamode:
		;;;     shr     g_vxdots,1                ; scan line increment is in bytes...
		;;;     shr     g_vxdots,1
		;;;     shr     g_vxdots,1
				mov     ax,offset vgawrite      ; set up EGA/VGA write-a-dot routine.
				mov     bx,offset vgaread       ; set up EGA/VGA read-a-dot  routine
				mov     cx,offset vgaline       ; set up the EGA/VGA linewrite routine
				mov     dx,offset vgareadline   ; set up the EGA/VGA lineread  routine
				mov     si,offset swapvga       ; set up the EGA/VGA swap routine
				jmp     videomode               ; return to common code
		tseng256mode:
				mov     tseng,1                 ; set chipset flag
				mov     [bankadr],offset $tseng
				mov     [bankseg],seg $tseng
				jmp     super256mode            ; set super VGA linear memory functions
		paradise256mode:
				mov     paradise,1              ; set chipset flag
				mov     [bankadr],offset $paradise
				mov     [bankseg],seg $paradise
				jmp     super256mode            ; set super VGA linear memory functions
		video7256mode:
				mov     video7, 1               ; set chipset flag
				mov     [bankadr],offset $video7
				mov     [bankseg],seg $video7
				jmp     super256mode            ; set super VGA linear memory functions
		trident256mode:
				mov     trident,1               ; set chipset flag
				mov     [bankadr],offset $trident
				mov     [bankseg],seg $trident
				jmp     super256mode            ; set super VGA linear memory functions
		chipstech256mode:
				mov     chipstech,1             ; set chipset flag
				mov     [bankadr],offset $chipstech
				mov     [bankseg],seg $chipstech
				jmp     super256mode            ; set super VGA linear memory functions
		ati256mode:
				mov     ativga,1                ; set chipset flag
				mov     [bankadr],offset $ativga
				mov     [bankseg],seg $ativga
				jmp     super256mode            ; set super VGA linear memory functions
		everex256mode:
				mov     everex,1                ; set chipset flag
				mov     [bankadr],offset $everex
				mov     [bankseg],seg $everex
				jmp     super256mode            ; set super VGA linear memory functions
		VGA256automode:                         ; Auto-detect SuperVGA
				jmp     super256mode            ; set super VGA linear memory functions
		VESA256mode:                            ; set VESA 256-color mode
				mov     ax,word ptr vesa_mapper
				mov     [bankadr],ax
				mov     ax,word ptr vesa_mapper+2
				mov     [bankseg],ax
				jmp     super256mode            ; set super VGA linear memory functions
		super256mode:
				mov     ax,offset super256write ; set up superVGA write-a-dot routine
				mov     bx,offset super256read  ; set up superVGA read-a-dot  routine
				mov     cx,offset super256line  ; set up the  linewrite routine
				mov     dx,offset super256readline ; set up the normal lineread  routine
				mov     si,offset swap256       ; set up the swap routine
				jmp     videomode               ; return to common code
		tweak256mode:
				shr     g_vxdots,1                ; scan line increment is in bytes...
				shr     g_vxdots,1
				mov     oktoprint,0             ; NOT OK to printf() in this mode
				mov     ax,offset tweak256write ; set up tweaked-256 write-a-dot
				mov     bx,offset tweak256read  ; set up tweaked-256 read-a-dot
				mov     cx,offset tweak256line  ; set up tweaked-256 read-a-line
				mov     dx,offset tweak256readline ; set up the normal lineread  routine
				mov     si,offset swapvga       ; set up the swap routine
				jmp     videomode               ; return to common code
		cgamode:
				mov     cx,offset normaline     ; set up the normal linewrite routine
				mov     dx,offset normalineread ; set up the normal lineread  routine
				mov     si,offset swapnormread  ; set up the normal swap routine
				cmp     videoax,4               ; check for 320x200x4 color mode
				je      cga4med                 ; ..
				cmp     videoax,5               ; ..
				je      cga4med                 ; ..
				cmp     videoax,6               ; check for 640x200x2 color mode
				je      cga2high                ; ..
		cga4med:
				mov     ax,offset plotcga4      ; set up CGA write-a-dot
				mov     bx,offset getcga4       ; set up CGA read-a-dot
				jmp     videomode               ; return to common code
		cga2high:
				mov     ax,offset plotcga2      ; set up CGA write-a-dot
				mov     bx,offset getcga2       ; set up CGA read-a-dot
				jmp     videomode               ; return to common code
		ati1024mode:
				mov     ativga,1                ; set ATI flag.
				mov     ax,offset ati1024write  ; set up ATI1024 write-a-dot
				mov     bx,offset ati1024read   ; set up ATI1024 read-a-dot
				mov     cx,offset normaline     ; set up the normal linewrite routine
				mov     dx,offset normalineread ; set up the normal lineread  routine
				mov     si,offset swap256       ; set up the swap routine
				jmp     videomode               ; return to common code
		diskmode:
				call    far ptr startdisk       ; external disk-video start routine
				mov     ax,offset diskwrite     ; set up disk-vid write-a-dot routine
				mov     bx,offset diskread      ; set up disk-vid read-a-dot routine
				mov     cx,offset normaline     ; set up the normal linewrite routine
				mov     dx,offset normalineread ; set up the normal lineread  routine
				mov     si,offset swapnormread  ; set up the normal swap routine
				jmp     videomode               ; return to common code
		yourownmode:
				call    far ptr startvideo      ; external your-own start routine
				mov     ax,offset videowrite    ; set up ur-own-vid write-a-dot routine
				mov     bx,offset videoread     ; set up ur-own-vid read-a-dot routine
				mov     cx,offset normaline     ; set up the normal linewrite routine
				mov     dx,offset normalineread ; set up the normal lineread  routine
				mov     si,offset swapnormread  ; set up the normal swap routine
				mov     videoflag,1             ; flag "your-own-end" needed.
				jmp     videomode               ; return to common code
		targaMode:                              ; TARGA MODIFIED 2 June 89 - j mclain
				call    far ptr StartTGA
				mov     ax,offset tgawrite      ;
				mov     bx,offset tgaread       ;
				mov     cx,offset normaline     ; set up the normal linewrite routine
				mov     dx,offset normalineread ; set up the normal lineread  routine
				mov     si,offset swapnormread  ; set up the normal swap routine
				mov     tgaflag,1               ;
				jmp     videomode               ; return to common code
		f8514mode:                              ; 8514 modes
				cmp     ai_8514, 0              ; check if afi flag is set, JCO 4/11/92
				jne     f85afi          ; yes, try afi
				call    far ptr open8514hw      ; start the 8514a, try registers first JCO
				jnc     f85ok
				mov     ai_8514, 1              ; set afi flag
		f85afi:
				call    far ptr open8514        ; start the 8514a, try afi
				jnc     f85ok
				mov     ai_8514, 0              ; clear afi flag, JCO 4/11/92
				mov     g_good_mode,0              ; oops - problems.
				mov     dotmode, 0              ; if problem starting use normal mode
				jmp     dullnormalmode
		hgcmode:
				mov     oktoprint,0             ; NOT OK to printf() in this mode
				call    hgcstart                ; Initialize the HGC card
				mov     ax,offset hgcwrite      ; set up HGC write-a-dot routine
				mov     bx,offset hgcread       ; set up HGC read-a-dot  routine
				mov     cx,offset normaline     ; set up normal linewrite routine
				mov     dx,offset normalineread ; set up the normal lineread  routine
				mov     si,offset swapnormread  ; set up the normal swap routine
				mov     HGCflag,1               ; flag "HGC-end" needed.
				jmp     videomode               ; return to common code
		f85ok:
				cmp     ai_8514, 0
				jne     f85okafi                        ; afi flag is set JCO 4/11/92
				mov     ax,offset f85hwwrite    ;use register routines
				mov     bx,offset f85hwread    ;changed to near calls
				mov     cx,offset f85hwline    ;
				mov     dx,offset f85hwreadline    ;
				mov     si,offset swapnormread  ; set up the normal swap routine
				mov     f85flag,1               ;
				mov     oktoprint,0             ; NOT OK to printf() in this mode
				jmp     videomode               ; return to common code
		f85okafi:
				mov     ax,offset f85write      ;use afi routines, JCO 4/11/92
				mov     bx,offset f85read      ;changed to near calls
				mov     cx,offset f85line      ;
				mov     dx,offset f85readline      ;
				mov     si,offset swapnormread  ; set up the normal swap routine
				mov     f85flag,1               ;
				mov     oktoprint,0             ; NOT OK to printf() in this mode
				jmp     videomode               ; return to common code
		*/
	}
	else
	{
		/*
		TrueColorAuto:
				cmp     TPlusInstalled, 1
				jne     NoTPlus
				push    NonInterlaced
				push    PixelZoom
				push    MaxColorRes
				push    ydots
				push    xdots
				call    far ptr MatchTPlusMode
				add     sp, 10
				or      ax, ax
				jz      NoTrueColorCard

				cmp     ax, 1                     ; Are we limited to 256 colors or less?
				jne     SetTPlusRoutines          ; All right! True color mode!

				mov     cx, MaxColorRes           ; Aw well, give'm what they want.
				shl     ax, cl
				mov     colors, ax

		SetTPlusRoutines:
				mov     g_good_mode, 1
				mov     oktoprint, 1
				mov     ax, offset TPlusWrite
				mov     bx, offset TPlusRead
				mov     cx, offset normaline
				mov     dx, offset normalineread
				mov     si, offset swapnormread
				jmp     videomode

		NoTPlus:
		NoTrueColorCard:
				mov     g_good_mode, 0
				jmp     videomode
		*/
	}

	/*
	videomode:
			mov     dotwrite,ax             ; save the results
			mov     dotread,bx              ;  ...
			mov     linewrite,cx            ;  ...
			mov     lineread,dx             ;  ...
			mov     word ptr g_swap_setup,si   ;  ...
			mov     ax,cs                   ;  ...
			mov     word ptr g_swap_setup+2,ax ;  ...

			mov     ax,colors               ; calculate the "and" value
			dec     ax                      ; to use for eventual color
			mov     g_and_color,ax             ; selection

			mov     boxcount,0              ; clear the zoom-box counter

			mov     g_dac_learn,0              ; set the DAC rotates to learn mode
			mov     g_dac_count,6              ; initialize the DAC counter
			cmp     cpu,88                  ; say, are we on a 186/286/386?
			jbe     setvideoslow            ;  boo!  hiss!
			mov     g_dac_learn,1              ; yup.  bypass learn mode
			mov     ax,cyclelimit           ;  and go as fast as he wants
			mov     g_dac_count,ax             ;  ...
	setvideoslow:
			call    far ptr loaddac         ; load the video dac, if we can
			ret
	setvideomode    endp
	*/
#endif
#if 0
	DriverWin32Disk *di = (DriverWin32Disk *) drv;
	if (g_disk_flag)
	{
		enddisk();
	}
	g_good_mode = 1;
	if (driver_diskp())
	{
		startdisk();
		dotwrite = writedisk;
		dotread = readdisk;
		lineread = normalineread;
		linewrite = normaline;
	}
	else if (dotmode == 0)
	{
		clear();
		wrefresh(di->curwin);
	}
	else
	{
		printf("Bad mode %d\n", dotmode);
		exit(-1);
	}

	if (dotmode !=0)
	{
		driver_read_palette();
		g_and_color = colors-1;
		boxcount = 0;
	}
#endif
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
	wintext_putstring(g_text_cbase + col, g_text_rbase + row, attr, msg);
#if 0
	getyx(di->curwin,textrow,textcol);
	textrow -= textrbase;
	textcol -= textcbase;
#endif
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
	}
	if (col != -1)
	{
		di->cursor_col = col;
	}
	row = di->cursor_row;
	col = di->cursor_col;
	wintext_cursor(g_text_cbase + col, g_text_rbase + row, 1);
	di->cursor_shown = TRUE;

#if 0
	DriverWin32Disk *di = (DriverWin32Disk *) drv;
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
	wintext_set_attr(g_text_rbase + row, g_text_cbase + col, attr, count);
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
	DriverWin32Disk *di = (DriverWin32Disk *) drv;
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
	DriverWin32Disk *di = (DriverWin32Disk *) drv;

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
	DriverWin32Disk *di = (DriverWin32Disk *) drv;

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

	di->cursor_col = col;
	di->cursor_row = row;
	if (0 == result)
	{
		wintext_cursor(col, row, 1);
		result = win32_disk_get_key(drv);
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

static DriverWin32Disk win32_disk_driver_info =
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
