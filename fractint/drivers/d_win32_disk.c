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
#include "WinText.h"

#define MAXSCREENS 10
#define DRAW_INTERVAL 6
#define TIMER_ID 1

extern HINSTANCE g_instance;

#define NUM_OF(ary_) (sizeof(ary_)/sizeof(ary_[0]))

#define DI(name_) Win32DiskDriver *name_ = (Win32DiskDriver *) drv

/* read/write-a-dot/line routines */
typedef void t_dotwriter(int, int, int);
typedef int  t_dotreader(int, int);
typedef void t_linewriter(int y, int x, int lastx, BYTE *pixels);
typedef void t_linereader(int y, int x, int lastx, BYTE *pixels);
typedef void t_swapper(void);

t_linewriter normaline;
t_linereader normalineread;
t_swapper null_swap;

t_swapper *g_swap_setup = null_swap;			/* setfortext/graphics setup routine */
static t_dotwriter *dotwrite = NULL;
static t_dotreader *dotread = NULL;
static t_linewriter *linewrite = NULL;
static t_linereader *lineread = NULL;

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

#define DRIVER_MODE(name_, comment_, key_, width_, height_, mode_) \
	{ name_, comment_, key_, 0, 0, 0, 0, mode_, width_, height_, 256 }
#define MODE19(n_, c_, k_, w_, h_) DRIVER_MODE(n_, c_, k_, w_, h_, 19)
#define MODE27(n_, c_, k_, w_, h_) DRIVER_MODE(n_, c_, k_, w_, h_, 27)
#define MODE28(n_, c_, k_, w_, h_) DRIVER_MODE(n_, c_, k_, w_, h_, 28)
static VIDEOINFO modes[] =
{
	MODE19("Win32 Disk Video         ", "                        ", FIK_F2,  320,  200),
	MODE19("Win32 Disk Video         ", "                        ", FIK_F3,  320,  400),
	MODE19("Win32 Disk Video         ", "                        ", FIK_F4,  360,  480),
	MODE19("Win32 Disk Video         ", "                        ", FIK_F5,  640,  400),
	MODE19("Win32 Disk Video         ", "                        ", FIK_F6,  640,  480),
	MODE27("Win32 Disk Video         ", "                        ", FIK_F7,  800,  600),
	MODE27("Win32 Disk Video         ", "                        ", FIK_F8,  1024, 768),
	MODE28("Win32 Disk Video         ", "                        ", FIK_F9,  1280, 1024),
	MODE28("Win32 Disk Video         ", "                        ", FIK_SF1, 1024, 1024),
	MODE28("Win32 Disk Video         ", "                        ", FIK_SF2, 1600, 1200),
	MODE28("Win32 Disk Video         ", "                        ", FIK_SF3, 2048, 2048),
	MODE28("Win32 Disk Video         ", "                        ", FIK_SF4, 4096, 4096),
	MODE28("Win32 Disk Video         ", "                        ", FIK_SF5, 8192, 8192)
};
#undef MODE28
#undef MODE27
#undef MODE19
#undef DRIVER_MODE

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
	{
		int i;
		for (i = 0; i < NUM_OF(di->saved_screens); i++)
		{
			if (NULL != di->saved_screens[i])
			{
				free(di->saved_screens[i]);
				di->saved_screens[i] = NULL;
			}
		}
	}
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
	int delay = (soon ? 1 : DRAW_INTERVAL)*1000;
	UINT_PTR result = SetTimer(wintext_hWndCopy, TIMER_ID, delay, wintext_timer_redraw);
	if (!result)
	{
		DWORD error = GetLastError();
		_ASSERTE(result);
	}
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
	_ASSERTE(di->pixels);
	_ASSERTE(x >= 0 && x < di->width);
	_ASSERTE(y >= 0 && y < di->height);
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
	_ASSERTE(di->pixels);
	_ASSERTE(x >= 0 && x < di->width);
	_ASSERTE(y >= 0 && y < di->height);
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
		case 1115:		return FIK_LEFT_ARROW_2;
		case 1116:		return FIK_RIGHT_ARROW_2;
		case 1141:		return FIK_UP_ARROW_2;
		case 1145:		return FIK_DOWN_ARROW_2;
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
	wintext_paintscreen(0, 80, 0, 25);
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
	int ch = keybuffer;
	if (ch)
	{
		keybuffer = 0;
		return ch;
	}

	ch = wintext_getkeypress(1);
	if (ch)
	{
		extern int inside_help;
		keybuffer = ch;
		if (FIK_F1 == ch && helpmode && !inside_help)
		{
			keybuffer = 0;
			inside_help = 1;
			help(0);
			inside_help = 0;
			ch = 0;
		}
		else if (FIK_TAB == ch && tabmode)
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
	_ASSERTE(di->pixels_len > 0);
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

t_dotwriter win32_dot_writer;
t_dotreader win32_dot_reader;
t_linewriter win32_line_writer;
t_linereader win32_line_reader;

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

void null_swap(void)
{
}

t_dotwriter nullwrite;
t_dotreader nullread;

static void nullwrite(int a, int b, int c)
{
	_ASSERTE(FALSE);
}

static int nullread(int a, int b)
{
	_ASSERTE(FALSE);
	return 0;
}

/* from video.asm */
void setnullvideo(void)
{
	ODS("setnullvideo");
	dotwrite = nullwrite;
	dotread = nullread;
}

/*
; **************** Function getcolor(xdot, ydot) *******************

;       Return the color on the screen at the (xdot, ydot) point
*/
int getcolor(int xdot, int ydot)
{
	int x1, y1;
	x1 = xdot + sxoffs;
	y1 = ydot + syoffs;
	_ASSERTE(x1 >= 0 && x1 <= sxdots);
	_ASSERTE(y1 >= 0 && y1 <= sydots);
	if (x1 < 0 || y1 < 0 || x1 >= sxdots || y1 >= sydots)
		return 0;
	return (*dotread)(x1, y1);
}

/*
; ************** Function putcolor_a(xdot, ydot, color) *******************

;       write the color on the screen at the (xdot, ydot) point
*/
void putcolor_a(int xdot, int ydot, int color)
{
	int x1 = xdot + sxoffs;
	int y1 = ydot + syoffs;
	_ASSERTE(x1 >= 0 && x1 <= sxdots);
	_ASSERTE(y1 >= 0 && y1 <= sydots);
	(*dotwrite)(x1, y1, color & g_and_color);
}

/*
; ***Function get_line(int row, int startcol, int stopcol, unsigned char *pixels)

;       This routine is a 'line' analog of 'getcolor()', and gets a segment
;       of a line from the screen and stores it in pixels[] at one byte per
;       pixel
;       Called by the GIF decoder
*/

void get_line(int row, int startcol, int stopcol, BYTE *pixels)
{
	_ASSERTE(_CrtCheckMemory());
	if (startcol + sxoffs >= sxdots || row + syoffs >= sydots)
		return;
	(*lineread)(row + syoffs, startcol + sxoffs, stopcol + sxoffs, pixels);
}

/*
; ***Function put_line(int row, int startcol, int stopcol, unsigned char *pixels)

;       This routine is a 'line' analog of 'putcolor()', and puts a segment
;       of a line from the screen and stores it in pixels[] at one byte per
;       pixel
;       Called by the GIF decoder
*/

void put_line(int row, int startcol, int stopcol, BYTE *pixels)
{
	_ASSERTE(_CrtCheckMemory());
	if (startcol + sxoffs >= sxdots || row + syoffs > sydots)
		return;
	(*linewrite)(row + syoffs, startcol + sxoffs, stopcol + sxoffs, pixels);
}

/*
; ***************Function out_line(pixels, linelen) *********************

;       This routine is a 'line' analog of 'putcolor()', and sends an
;       entire line of pixels to the screen (0 <= xdot < xdots) at a clip
;       Called by the GIF decoder
*/
int out_line(BYTE *pixels, int linelen)
{
	_ASSERTE(_CrtCheckMemory());
	if (g_row_count + syoffs >= sydots)
	{
		return 0;
	}
	(*linewrite)(g_row_count + syoffs, sxoffs, linelen + sxoffs - 1, pixels);
	g_row_count++;
	return 0;
}

/*
; **************** internal Read/Write-a-line routines *********************
;
;       These routines are called by out_line(), put_line() and get_line().
*/
void normaline(int y, int x, int lastx, BYTE *pixels)
{
	int i, width;
	width = lastx - x + 1;
	for (i = 0; i < width; i++)
	{
		dotwrite(x + i, y, pixels[i]);
	}
}

void normalineread(int y, int x, int lastx, BYTE *pixels)
{
	int i, width;
	width = lastx - x + 1;
	for (i = 0; i < width; i++)
	{
		pixels[i] = dotread(x + i, y);
	}
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

	g_ok_to_print = FALSE;
	g_good_mode = 1;

	startdisk();
	dotwrite = writedisk;
	dotread = readdisk;
	lineread = normalineread;
	linewrite = normaline;

	if (dotmode !=0)
	{
		driver_read_palette();
		g_and_color = colors-1;
		boxcount = 0;
		g_dac_learn = 1;
		g_dac_count = cyclelimit;
		g_got_real_dac = TRUE;
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
		_ASSERTE(abs_row >= 0 && abs_row < WINTEXT_MAX_ROW);
		_ASSERTE(abs_col >= 0 && abs_col < WINTEXT_MAX_COL);
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
	ODS("win32_disk_stack_screen");

	di->saved_cursor[di->screen_count+1] = g_text_row*80 + g_text_col;
	if (++di->screen_count)
	{
		/* already have some stacked */
		int i = di->screen_count - 1;

		_ASSERTE(i < MAXSCREENS);
		if (i >= MAXSCREENS)
		{
			/* bug, missing unstack? */
			stopmsg(STOPMSG_NO_STACK, "stackscreen overflow");
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
	_ASSERTE(di->screen_count > 0);
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

	_ASSERTE(di->screen_count > 0);
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

static int
win32_disk_validate_mode(Driver *drv, VIDEOINFO *mode)
{
	return (mode->colors == 256) ? 1 : 0;
}

static Win32DiskDriver win32_disk_driver_info =
{
	STD_DRIVER_STRUCT(win32_disk, "A disk video driver for 32-bit Windows.")
};

Driver *win32_disk_driver = &win32_disk_driver_info.pub;
