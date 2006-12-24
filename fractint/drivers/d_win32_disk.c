/* d_win32_disk.c
 *
 * Routines for a Win32 disk video mode driver for fractint.
 */
#include <assert.h>

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

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
#if 0
#ifdef LINUX
#define FNDELAY O_NDELAY
#endif
#ifdef __SVR4
# include <sys/filio.h>
# define FNDELAY O_NONBLOCK
#endif
#include <assert.h>
/* Check if there is a character waiting for us.  */
#define input_pending() (ioctl(0, FIONREAD, &iocount), (int) iocount)
/* external variables (set in the FRACTINT.CFG file, but findable here */
extern int dotmode;  /* video access method (= 19)    */
extern int sxdots, sydots;  /* total # of dots on the screen   */
extern int sxoffs, syoffs;  /* offset of drawing area          */
extern int colors;   /* maximum colors available    */
extern int initmode;
extern int adapter;
extern int gotrealdac;
extern int inside_help;
extern  float finalaspectratio;
extern  float screenaspect;
extern int lookatmouse;
extern VIDEOINFO videotable[];
/* the video-palette array (named after the VGA adapter's video-DAC) */
extern unsigned char dacbox[256][3];
extern void drawbox();
extern int text_type;
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

static VIDEOINFO win32_disk_info =
{
	"xfractint mode           ","                         ",
	999, 0,	 0,   0,   0,	19, 640, 480,  256
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
#if 0
	SCREEN *term;
	WINDOW *curwin;

	int simple_input; /* Use simple input (debugging) */
	char *Xgeometry;

	int old_fcntl;

	int alarmon; /* 1 if the refresh alarm is on */
	int doredraw; /* 1 if we have a redraw waiting */

	int width, height;
	int xlastcolor;
	int pixtab[256];
	int ipixtab[256];

	int xbufkey;		/* Buffered X key */

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
function_called(const char *fn, const char *file, unsigned line)
{
	char buffer[512];
	sprintf(buffer, "%s(%d): !%s called\n", file, line, fn);
	OutputDebugString(buffer);
}
#define CALLED(fn_) function_called(fn_, __FILE__, __LINE__)

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
	CALLED("win32_disk_terminate");
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
#if 0
	int i;
	for (i=0;i < 256;i++)
	{
		dacbox[i][0] = (i >> 5)*8+7;
		dacbox[i][1] = (((i+16) & 28) >> 2)*8+7;
		dacbox[i][2] = (((i+2) & 3))*16+15;
	}
	dacbox[0][0] = dacbox[0][1] = dacbox[0][2] = 0;
	dacbox[1][0] = dacbox[1][1] = dacbox[1][2] = 63;
	dacbox[2][0] = 47; dacbox[2][1] = dacbox[2][2] = 63;
#endif
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

	CALLED("win32_disk_init");
	if (wintext_initialize(g_instance, NULL, "FractInt for Windows"))
	{
		return TRUE;
	}

	return FALSE;
#if 0

	/*
	* Check a bunch of important conditions
	*/
	if (sizeof(short) != 2)
	{
		OutputDebugString("Error: need short to be 2 bytes\n");
		exit(-1);
	}
	if (sizeof(long) < sizeof(FLOAT4))
	{
		OutputDebugString("Error: need sizeof(long) >= sizeof(FLOAT4)\n");
		exit(-1);
	}

	di->term = newterm(NULL, stdout, stdin);
	if (!di->term)
		return 0;

	di->curwin = stdscr;
	cbreak();
	noecho();

	if (standout())
	{
		text_type = 1;
		standend();
	}
	else
	{
		text_type = 1;
	}

	initdacbox();

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

	/* add_video_mode(&win32_disk_info); */
#endif
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
	CALLED("win32_disk_flush");
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
	CALLED("win32_disk_resize");
	return 0;
}


/*----------------------------------------------------------------------
* win32_disk_read_palette
*
*	Reads the current video palette into dacbox.
*	
*
* Results:
*	None.
*
* Side effects:
*	Fills in dacbox.
*
*----------------------------------------------------------------------
*/
static int
win32_disk_read_palette(Driver *drv)
{
	DriverWin32Disk *di = (DriverWin32Disk *) drv;
	int i;

	CALLED("win32_disk_read_palette");
	if (gotrealdac == 0)
		return -1;
	for (i = 0; i < 256; i++)
	{
		dacbox[i][0] = di->cols[i][0];
		dacbox[i][1] = di->cols[i][1];
		dacbox[i][2] = di->cols[i][2];
	}
	return 0;
}

/*
*----------------------------------------------------------------------
*
* win32_disk_write_palette --
*	Writes dacbox into the video palette.
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
	DriverWin32Disk *di = (DriverWin32Disk *) drv;
	int i;

	CALLED("win32_disk_write_palette");
	for (i = 0; i < 256; i++)
	{
		di->cols[i][0] = dacbox[i][0];
		di->cols[i][1] = dacbox[i][1];
		di->cols[i][2] = dacbox[i][2];
	}

	return 0;
}

static int
win32_disk_start_video(Driver *drv)
{
	CALLED("win32_disk_start_video");
	return 0;
}

static int
win32_disk_end_video(Driver *drv)
{
	CALLED("win32_disk_end_video");
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
	CALLED("win32_disk_schedule_alarm");
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
	DriverWin32Disk *di = (DriverWin32Disk *) drv;

	CALLED("win32_disk_write_pixel");
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
	DriverWin32Disk *di = (DriverWin32Disk *) drv;

	CALLED("win32_disk_read_pixel");
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
	CALLED("win32_disk_write_span");

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
	CALLED("win32_disk_read_span");
	width = lastx-x+1;
	for (i = 0; i < width; i++)
	{
		pixels[i] = win32_disk_read_pixel(drv, x+i, y);
	}
}

static void
win32_disk_set_line_mode(Driver *drv, int mode)
{
	CALLED("win32_disk_set_line_mode");
}

static void
win32_disk_draw_line(Driver *drv, int x1, int y1, int x2, int y2)
{
	CALLED("win32_disk_draw_line");
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
	CALLED("win32_disk_redraw");
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
win32_disk_get_key(Driver *drv, int block)
{
	CALLED("win32_disk_get_key");
	return wintext_getkeypress(block);
#if 0
	static int skipcount = 0;
	DriverWin32Disk *di = (DriverWin32Disk *) drv;

	while (1)
	{
		if (input_pending())
		{
			int ch = getachar(di);
			return (ch == ESC) ? handleesc(di) : translatekey(ch);
		}

		/* Don't check X events every time, since that is expensive */
		skipcount++;
		if (block == 0 && skipcount < 25)
			break;
		skipcount = 0;

		if (di->xbufkey)
		{
			int ch = di->xbufkey;
			di->xbufkey = 0;
			skipcount = 9999; /* If we got a key, check right away next time */
			return translatekey(ch);
		}

		if (!block)
			break;

		{
			fd_set reads;
			struct timeval tout;
			int status;

			FD_ZERO(&reads);
			FD_SET(0, &reads);
			tout.tv_sec = 0;
			tout.tv_usec = 500000;

			status = select(1, &reads, NULL, NULL, &tout);
			if (status <= 0)
				return 0;
		}
	}
#endif

	return 0;
}

static void
parse_geometry(const char *spec, int *x, int *y, int *width, int *height)
{
#if 0
	char *plus, *minus;

	/* do something like XParseGeometry() */
	if (2 == sscanf(spec, "%dx%d", width, height))
	{
		/* all we care about is width and height for disk output */
		*x = 0;
		*y = 0;
	}
#endif
}

static void
win32_disk_window(Driver *drv)
{
	DriverWin32Disk *di = (DriverWin32Disk *) drv;

	CALLED("win32_disk_window");
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

	adapter = 0;

	/* We have to do some X stuff even for disk video, to parse the geometry
	* string */
	gotrealdac = 1;
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

	videotable[0].xdots = sxdots;
	videotable[0].ydots = sydots;
	videotable[0].colors = colors;
	videotable[0].dotmode = 19;
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
	CALLED("win32_disk_set_video_mode");
#if 0
	DriverWin32Disk *di = (DriverWin32Disk *) drv;
	if (diskflag)
	{
		enddisk();
	}
	goodmode = 1;
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
		andcolor = colors-1;
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
	DriverWin32Disk *di = (DriverWin32Disk *) drv;

	CALLED("win32_disk_put_string");
	wintext_putstring(col, row, attr, msg);
#if 0
	DriverWin32Disk *di = (DriverWin32Disk *) drv;
	int so = 0;

	if (row != -1)
		textrow = row;
	if (col != -1)
		textcol = col;

	if (attr & INVERSE || attr & BRIGHT)
	{
		wstandout(di->curwin);
		so = 1;
	}
	wmove(di->curwin,textrow+textrbase, textcol+textcbase);
	while (1)
	{
		if (*msg == '\0') break;
		if (*msg == '\n')
		{
			textcol = 0;
			textrow++;
			wmove(di->curwin,textrow+textrbase, textcol+textcbase);
		}
		else
		{
			char *ptr;
			ptr = strchr(msg,'\n');
			if (ptr == NULL)
			{
				waddstr(di->curwin,msg);
				break;
			}
			else
				waddch(di->curwin,*msg);
		}
		msg++;
	}
	if (so)
		wstandend(di->curwin);

	wrefresh(di->curwin);
	fflush(stdout);
	getyx(di->curwin,textrow,textcol);
	textrow -= textrbase;
	textcol -= textcbase;
#endif
}

static void
win32_disk_set_for_text(Driver *drv)
{
	CALLED("win32_disk_set_for_text");
}

static void
win32_disk_set_for_graphics(Driver *drv)
{
	CALLED("win32_disk_set_for_graphics");
}

static void
win32_disk_set_clear(Driver *drv)
{
	CALLED("win32_disk_set_clear");
	wintext_clear();
}

/************** Function scrollup(toprow, botrow) ******************
*
*       Scroll the screen up (from toprow to botrow)
*/
static void
win32_disk_scroll_up(Driver *drv, int top, int bot)
{
	CALLED("win32_disk_scroll_up");
	wintext_scroll_up(top, bot);
#if 0
	DriverWin32Disk *di = (DriverWin32Disk *) drv;

	wmove(di->curwin, top, 0);
	wdeleteln(di->curwin);
	wmove(di->curwin, bot, 0);
	winsertln(di->curwin);
	wrefresh(di->curwin);
#endif
}

static BYTE *
win32_disk_find_font(Driver *drv, int parm)
{
	CALLED("win32_disk_find_font");
	return NULL;
}

static void
win32_disk_move_cursor(Driver *drv, int row, int col)
{
	CALLED("win32_disk_move_cursor");
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
	CALLED("win32_disk_set_attr");
	wintext_set_attr(row, col, attr, count);
}

static void
win32_disk_hide_text_cursor(Driver *drv)
{
	CALLED("win32_disk_hide_text_cursor");
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

	CALLED("win32_disk_stack_screen");
	di->saved_cursor[di->screen_count+1] = textrow*80 + textcol;
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

	CALLED("win32_disk_unstack_screen");
	textrow = di->saved_cursor[di->screen_count] / 80;
	textcol = di->saved_cursor[di->screen_count] % 80;
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

	CALLED("win32_disk_discard_screen");
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
	CALLED("win32_disk_init_fm");
	return 0;
}

static void
win32_disk_buzzer(Driver *drv, int kind)
{
	CALLED("win32_disk_buzzer");
	MessageBeep(MB_OK);
}

static int
win32_disk_sound_on(Driver *drv, int freq)
{
	CALLED("win32_disk_sound_on");
	return 0;
}

static void
win32_disk_sound_off(Driver *drv)
{
	CALLED("win32_disk_sound_off");
}

static void
win32_disk_mute(Driver *drv)
{
	CALLED("win32_disk_mute");
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
