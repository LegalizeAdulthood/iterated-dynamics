/* Unixscr.c
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
#include <curses.h>
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
#include <fcntl.h>
#include <string.h>
#include "helpdefs.h"
#include "port.h"
#include "prototyp.h"

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

extern	int	dotmode;		/* video access method (= 19)	   */
extern	int	sxdots, sydots; 	/* total # of dots on the screen   */
extern	int	sxoffs, syoffs; 	/* offset of drawing area          */
extern	int	colors; 		/* maximum colors available	   */
extern	int	initmode;
extern	int	adapter;
extern	int	gotrealdac;
extern	int	inside_help;
extern  float	finalaspectratio;
extern  float	screenaspect;
extern	int	lookatmouse;

extern VIDEOINFO videotable[];

/* the video-palette array (named after the VGA adapter's video-DAC) */

extern unsigned char dacbox[256][3];

extern void drawbox();

extern int text_type;
extern int helpmode;
extern int rotate_hi;

extern void fpe_handler();

#ifdef FPUERR
static void continue_hdl(int sig, int code, struct sigcontext *scp,
	char *addr);
#endif

#define DEFX 640
#define DEFY 480
#define DEFXY "640x480+0+0"

extern int editpal_cursor;
extern void Cursor_SetPos();

#define SENS 1
#define ABS(x) ((x) > 0 ? (x) : -(x))
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define SIGN(x) ((x) > 0 ? 1 : -1)

#define SHELL "/bin/csh"

#define DRAW_INTERVAL 6

extern void (*dotwrite)(int, int, int);	/* write-a-dot routine */
extern int (*dotread)(int, int); 	/* read-a-dot routine */
extern void (*linewrite)(void);		/* write-a-line routine */
extern void (*lineread)(void);		/* read-a-line routine */

extern void normalineread(void);
extern void normaline(void);

static VIDEOINFO disk_info = {
  "disk mode      ","                         ",
  999, 0, 0, 0, 0, 8, 19, 640, 480, 256
};

#define MAXSCREENS 3

typedef struct tagDriverDisk DriverDisk;
struct tagDriverDisk {
  Driver pub;
  SCREEN *term;
  WINDOW *curwin;

  int simple_input; /* Use simple input (debugging) */
  char *Xgeometry;

  int old_fcntl;

  int alarmon; /* 1 if the refresh alarm is on */
  int doredraw; /* 1 if we have a redraw waiting */

  int width, height;
  int xlastcolor;
  BYTE *pixbuf;
  unsigned char cols[256][3];
  int pixtab[256];
  int ipixtab[256];

  int xbufkey;		/* Buffered X key */

  unsigned char *fontPtr;

  int screenctr;

  BYTE *savescreen[MAXSCREENS];
  int saverc[MAXSCREENS+1];
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
check_arg(DriverDisk *di, int argc, char **argv, int *i)
{
  if (strcmp(argv[*i], "-disk") == 0) {
    return 1;
  } else if (strcmp(argv[*i], "-simple") == 0) {
    di->simple_input = 1;
    return 1;
  } else if (strcmp(argv[*i], "-geometry") == 0 && *i+1 < argc) {
    di->Xgeometry = argv[(*i)+1];
    (*i)++;
    return 1;
  } else {
    return 0;
  }
}

/*----------------------------------------------------------------------
 *
 * disk_terminate --
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
disk_terminate(Driver *drv)
{
  DriverDisk *di = (DriverDisk *) drv;
  if (!di->simple_input) {
    fcntl(0, F_SETFL, di->old_fcntl);
  }
  mvcur(0, COLS-1, LINES-1, 0);
  nocbreak();
  echo();
  endwin();
  delscreen(di->term);
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
  for (i=0;i < 256;i++) {
    dacbox[i][0] = (i >> 5)*8+7;
    dacbox[i][1] = (((i+16) & 28) >> 2)*8+7;
    dacbox[i][2] = (((i+2) & 3))*16+15;
  }
  dacbox[0][0] = dacbox[0][1] = dacbox[0][2] = 0;
  dacbox[1][0] = dacbox[1][1] = dacbox[1][2] = 63;
  dacbox[2][0] = 47; dacbox[2][1] = dacbox[2][2] = 63;
}

static int
test_video_mode(Driver *drv, VIDEOINFO *mode)
{
  int result = 1;

/* FIXME need to work out how to test JCO */
/* see d_allegro.c */

  return (result);
}

/*----------------------------------------------------------------------
 *
 * disk_init --
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
disk_init(Driver *drv, int *argc, char **argv)
{
  DriverDisk *di = (DriverDisk *) drv;

  /*
   * Check a bunch of important conditions
   */
  if (sizeof(short) != 2) {
    fprintf(stderr, "Error: need short to be 2 bytes\n");
    exit(-1);
  }
  if (sizeof(long) < sizeof(FLOAT4)) {
    fprintf(stderr, "Error: need sizeof(long) >= sizeof(FLOAT4)\n");
    exit(-1);
  }

  di->term = newterm(NULL, stdout, stdin);
  if (!di->term)
    return 0;

  di->curwin = stdscr;
  cbreak();
  noecho();

  if (standout()) {
    text_type = 1;
    standend();
  } else {
    text_type = 1;
  }

  initdacbox();

  if (!di->simple_input) {
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

  /* should enumerate visuals here and build video modes for each */
  /* test_video_mode(drv, &disk_info); put this in a loop FIXME */
  /* add_video_mode(&disk_info); */
  return 1;
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
 * disk_flush
 */
static void
disk_flush(Driver *drv)
{
  DriverDisk *di = (DriverDisk *) drv;
  wrefresh(di->curwin);
}

/*----------------------------------------------------------------------
 * disk_resize
 */
static int
disk_resize(Driver *drv)
{
  return 0;
}


/*----------------------------------------------------------------------
 * disk_read_palette
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
disk_read_palette(Driver *drv)
{
  DriverDisk *di = (DriverDisk *) drv;
  int i;
  if (gotrealdac == 0)
    return -1;
  for (i = 0; i < colors; i++) {
    dacbox[i][0] = di->cols[i][0];
    dacbox[i][1] = di->cols[i][1];
    dacbox[i][2] = di->cols[i][2];
  }
  return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * disk_write_palette --
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
disk_write_palette(Driver *drv)
{
  DriverDisk *di = (DriverDisk *) drv;
  int i;

  for (i = 0; i < colors; i++) {
    di->cols[i][0] = dacbox[i][0];
    di->cols[i][1] = dacbox[i][1];
    di->cols[i][2] = dacbox[i][2];
  }

  return 0;
}

static int
disk_start_video(Driver *drv)
{
  return 0;
}

static int
disk_end_video(Driver *drv)
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
static void
setredrawscreen(void)
{
  ((DriverDisk *) display)->doredraw = 1;
}

/*
 *----------------------------------------------------------------------
 *
 * disk_schedule_alarm --
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
disk_schedule_alarm(Driver *drv, int soon)
{
  DriverDisk *di = (DriverDisk *) drv;
  signal(SIGALRM, (SignalHandler) setredrawscreen);
  if (soon)
    alarm(1);
  else
    alarm(DRAW_INTERVAL);
  di->alarmon = 1;
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
static void 
disk_write_pixel(Driver *drv, int x, int y, int color)
{
  fprintf(stderr, "disk_write_pixel(%d,%d): %d\n", x, y, color);
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
static int
disk_read_pixel(Driver *drv, int x, int y)
{
  fprintf(stderr, "disk_read_pixel(%d,%d)\n", x, y);
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
static void
disk_write_span(Driver *drv, int y, int x, int lastx, BYTE *pixels)
{
  int i;
  int width = lastx-x+1;

  for (i = 0; i < width; i++)
    disk_write_pixel(drv, x+i, y, pixels[i]);
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
static void
disk_read_span(Driver *drv, int y, int x, int lastx, BYTE *pixels)
{
  int i, width;
  width = lastx-x+1;
  for (i = 0; i < width; i++) {
    pixels[i] = disk_read_pixel(drv, x+i, y);
  }
}

static void
disk_set_line_mode(Driver *drv, int mode)
{
  fprintf(stderr, "disk_set_line_mode(%d)\n", mode);
}

static void
disk_draw_line(Driver *drv, int x1, int y1, int x2, int y2)
{
  fprintf(stderr, "disk_draw_line(%d,%d, %d,%d)\n", x1, y1, x2, y2);
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
getachar(DriverDisk *di)
{
  if (di->simple_input) {
    return getchar();
  } else {
    char ch;
    int status;
    status = read(0, &ch, 1);
    if (status < 0) {
      return -1;
    } else {
      return ch;
    }
  }
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
  else {
    switch (ch) {
    case 'I':		return INSERT;
    case 'D':		return DELETE;
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

/*
; ***************** Function ddelay(int delaytime) ************************
;
;       performs a delay loop for 'delaytime' milliseconds
*/
void
ddelay(int time)
{
    static struct timeval delay;
    delay.tv_sec = time/1000;
    delay.tv_usec = (time%1000)*1000;
#if defined( __SVR4) || defined(LINUX)
    (void) select(0, (fd_set *) 0, (fd_set *) 0, (fd_set *) 0, &delay);
#else
    (void) select(0, (int *) 0, (int *) 0, (int *) 0, &delay);
#endif
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
handleesc(DriverDisk *di)
{
  int ch1, ch2, ch3;
  if (di->simple_input)
    return ESC;

#ifdef __hpux
  /* HP escape key sequences. */
  ch1 = getachar(di);
  if (ch1 == -1) {
    ddelay(250); /* Wait 1/4 sec to see if a control sequence follows */
    ch1 = getachar(di);
  }
  if (ch1 == -1)
    return ESC;

  switch (ch1) {
  case 'A':
    return UP_ARROW;
  case 'B':
    return DOWN_ARROW;
  case 'D':
    return LEFT_ARROW;
  case 'C':
    return RIGHT_ARROW;
  case 'd':
    return HOME;
  }
  if (ch1 != '[')
    return ESC;
  ch1 = getachar(di);
  if (ch1 == -1) {
    ddelay(250); /* Wait 1/4 sec to see if a control sequence follows */
    ch1 = getachar(di);
  }
  if (ch1 == -1 || !isdigit(ch1))
    return ESC;
  ch2 = getachar(di);
  if (ch2 == -1) {
    ddelay(250); /* Wait 1/4 sec to see if a control sequence follows */
    ch2 = getachar(di);
  }
  if (ch2 == -1)
    return ESC;
  if (isdigit(ch2)) {
    ch3 = getachar(di);
    if (ch3 == -1) {
      ddelay(250); /* Wait 1/4 sec to see if a control sequence follows */
      ch3 = getachar(di);
    }
    if (ch3 != '~')
      return ESC;
    ch2 = (ch2-'0')*10+ch3-'0';
  } else if (ch3 != '~') {
    return ESC;
  } else {
    ch2 = ch2-'0';
  }
  switch (ch2) {
  case 5:
    return PAGE_UP;
  case 6:
    return PAGE_DOWN;
  case 29:
    return F1; /* help */
  case 11:
    return F1;
  case 12:
    return F2;
  case 13:
    return F3;
  case 14:
    return F4;
  case 15:
    return F5;
  case 17:
    return F6;
  case 18:
    return F7;
  case 19:
    return F8;
  default:
    return ESC;
  }
#else
  /* SUN escape key sequences */
  ch1 = getachar(di);
  if (ch1 == -1) {
    ddelay(250); /* Wait 1/4 sec to see if a control sequence follows */
    ch1 = getachar(di);
  }
  if (ch1 != '[')		/* See if we have esc [ */
    return ESC;
  ch1 = getachar(di);
  if (ch1 == -1) {
    ddelay(250); /* Wait 1/4 sec to see if a control sequence follows */
    ch1 = getachar(di);
  }
  if (ch1 == -1)
    return ESC;
  switch (ch1) {
  case 'A':		/* esc [ A */
    return UP_ARROW;
  case 'B':		/* esc [ B */
    return DOWN_ARROW;
  case 'C':		/* esc [ C */
    return RIGHT_ARROW;
  case 'D':		/* esc [ D */
    return LEFT_ARROW;
  default:
    break;
  }
  ch2 = getachar(di);
  if (ch2 == -1) {
    ddelay(250); /* Wait 1/4 sec to see if a control sequence follows */
    ch2 = getachar(di);
  }
  if (ch2 == '~') {		/* esc [ ch1 ~ */
    switch (ch1) {
    case '2':		/* esc [ 2 ~ */
      return INSERT;
    case '3':		/* esc [ 3 ~ */
      return DELETE;
    case '5':		/* esc [ 5 ~ */
      return PAGE_UP;
    case '6':		/* esc [ 6 ~ */
      return PAGE_DOWN;
    default:
      return ESC;
    }
  } else if (ch2 == -1) {
    return ESC;
  } else {
    ch3 = getachar(di);
    if (ch3 == -1) {
      ddelay(250); /* Wait 1/4 sec to see if a control sequence follows */
      ch3 = getachar(di);
    }
    if (ch3 != '~') {	/* esc [ ch1 ch2 ~ */
      return ESC;
    }
    if (ch1 == '1') {
      switch (ch2) {
      case '1':	/* esc [ 1 1 ~ */
	return F1;
      case '2':	/* esc [ 1 2 ~ */
	return F2;
      case '3':	/* esc [ 1 3 ~ */
	return F3;
      case '4':	/* esc [ 1 4 ~ */
	return F4;
      case '5':	/* esc [ 1 5 ~ */
	return F5;
      case '6':	/* esc [ 1 6 ~ */
	return F6;
      case '7':	/* esc [ 1 7 ~ */
	return F7;
      case '8':	/* esc [ 1 8 ~ */
	return F8;
      case '9':	/* esc [ 1 9 ~ */
	return F9;
      default:
	return ESC;
      }
    } else if (ch1 == '2') {
      switch (ch2) {
      case '0':	/* esc [ 2 0 ~ */
	return F10;
      case '8':	/* esc [ 2 8 ~ */
	return F1;  /* HELP */
      default:
	return ESC;
      }
    } else {
      return ESC;
    }
  }
#endif
}

/*
 *----------------------------------------------------------------------
 *
 * disk_redraw --
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
disk_redraw(Driver *drv)
{
  fprintf(stderr, "disk_redraw\n");
}

/*----------------------------------------------------------------------
 *
 * disk_get_key --
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
disk_get_key(Driver *drv, int block)
{
  static int skipcount = 0;
  DriverDisk *di = (DriverDisk *) drv;

  while (1) {
    if (input_pending()) {
      int ch = getachar(di);
      return (ch == ESC) ? handleesc(di) : translatekey(ch);
    }

    /* Don't check X events every time, since that is expensive */
    skipcount++;
    if (block == 0 && skipcount < 25)
      break;
    skipcount = 0;

    if (di->xbufkey) {
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

  return 0;
}

static void
parse_geometry(const char *spec, int *x, int *y, int *width, int *height)
{
  char *plus, *minus;

  /* do something like XParseGeometry() */
  if (2 == sscanf(spec, "%dx%d", width, height)) {
    /* all we care about is width and height for disk output */
    *x = 0;
    *y = 0;
  }
}

static void
disk_window(Driver *drv)
{
  int offx, offy;
  int i;
  DriverDisk *di = (DriverDisk *) drv;

  if (!di->simple_input) {
    di->old_fcntl = fcntl(0, F_GETFL);
    fcntl(0, F_SETFL, FNDELAY);
  }

  adapter = 0;

  /* We have to do some X stuff even for disk video, to parse the geometry
   * string */
  gotrealdac = 1;
  colors = 256;
  for (i = 0; i < colors; i++) {
    di->pixtab[i] = i;
    di->ipixtab[i] = i;
  }
  if (di->Xgeometry)
    parse_geometry(di->Xgeometry, &offx, &offy, &di->width, &di->height);
  sxdots = di->width;
  sydots = di->height;
  disk_flush(drv);
  disk_write_palette(drv);

  videotable[0].xdots = sxdots;
  videotable[0].ydots = sydots;
  videotable[0].colors = colors;
  videotable[0].dotmode = 19;
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
disk_shell(Driver *drv)
{
  SignalHandler sigint;
  char *shell;
  char *argv[2];
  int pid, donepid;
  DriverDisk *di = (DriverDisk *) drv;

  sigint = (SignalHandler) signal(SIGINT, SIG_IGN);
  shell = getenv("SHELL");
  if (shell == NULL)
    shell = SHELL;

  argv[0] = shell;
  argv[1] = NULL;

  /* Clean up the window */
  if (!di->simple_input)
    fcntl(0, F_SETFL, di->old_fcntl);

  mvcur(0, COLS-1, LINES-1, 0);
  nocbreak();
  echo();
  
  endwin();

  /* Fork the shell */
  pid = fork();
  if (pid < 0)
    perror("fork to shell");
  if (pid == 0) {
    execvp(shell, argv);
    perror("fork to shell");
    exit(1);
  }

  /* Wait for the shell to finish */
  while (1) {
    donepid = wait(0);
    if (donepid < 0 || donepid == pid)
      break;
  }

  /* Go back to curses mode */
  
  initscr();
  di->curwin = stdscr;
  cbreak();
  noecho();
  if (!di->simple_input) {
    di->old_fcntl = fcntl(0, F_GETFL);
    fcntl(0, F_SETFL, FNDELAY);
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

; Unix: We ignore ax,bx,cx,dx.  dotmode is the "mode" field in the video
; table.  We use mode 19 for the X window.
*/
static void
disk_set_video_mode(Driver *drv, int ax, int bx, int cx, int dx)
{
  DriverDisk *di = (DriverDisk *) drv;
  if (diskflag) {
    enddisk();
  }
  goodmode = 1;
  if (driver_diskp()) {
    startdisk();
    dotwrite = writedisk;
    dotread = readdisk;
    lineread = normalineread;
    linewrite = normaline;
  } else if (dotmode == 0) {
    clear();
    wrefresh(di->curwin);
  } else {
    fprintf(stderr,"Bad mode %d\n", dotmode);
    exit(-1);
  } 
  if (dotmode !=0) {
    driver_read_palette();
    andcolor = colors-1;
    boxcount = 0;
  }
}

/*
; PUTSTR.asm puts a string directly to video display memory. Called from C by:
;    putstring(row, col, attr, string) where
;         row, col = row and column to start printing.
;         attr = color attribute.
;         string = far pointer to the null terminated string to print.
;    Written for the A86 assembler (which has much less 'red tape' than MASM)
;    by Bob Montgomery, Orlando, Fla.             7-11-88
;    Adapted for MASM 5.1 by Tim Wegner          12-11-89
;    Furthur mucked up to handle graphics
;       video modes by Bert Tyler                 1-07-90
;    Reworked for:  row,col update/inherit;
;       620x200x2 inverse video;  far ptr to string;
;       fix to avoid scrolling when last posn chgd;
;       divider removed;  newline ctl chars;  PB  9-25-90
*/
static void
disk_put_string(Driver *drv, int row, int col, int attr, const char *msg)
{
  DriverDisk *di = (DriverDisk *) drv;
  int so = 0;

  if (row != -1)
    textrow = row;
  if (col != -1)
    textcol = col;

  if (attr & INVERSE || attr & BRIGHT) {
    wstandout(di->curwin);
    so = 1;
  }
  wmove(di->curwin,textrow+textrbase, textcol+textcbase);
  while (1) {
    if (*msg == '\0') break;
    if (*msg == '\n') {
      textcol = 0;
      textrow++;
      wmove(di->curwin,textrow+textrbase, textcol+textcbase);
    } else {
      char *ptr;
      ptr = strchr(msg,'\n');
      if (ptr == NULL) {
	waddstr(di->curwin,msg);
	break;
      } else
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
}

static void
disk_set_for_text(Driver *drv)
{
}

static void
disk_set_for_graphics(Driver *drv)
{
}

static void
disk_set_clear(Driver *drv)
{
  DriverDisk *di = (DriverDisk *) drv;
  wclear(di->curwin);
  wrefresh(di->curwin);
}

/************** Function scrollup(toprow, botrow) ******************
 *
 *       Scroll the screen up (from toprow to botrow)
 */
static void
disk_scroll_up(Driver *drv, int top, int bot)
{
  DriverDisk *di = (DriverDisk *) drv;
  wmove(di->curwin, top, 0);
  wdeleteln(di->curwin);
  wmove(di->curwin, bot, 0);
  winsertln(di->curwin);
  wrefresh(di->curwin);
}

static BYTE *
disk_find_font(Driver *drv, int parm)
{
  return NULL;
}

static void
disk_move_cursor(Driver *drv, int row, int col)
{
  DriverDisk *di = (DriverDisk *) drv;
  if (row == -1) {
    row = textrow;
  } else {
    textrow = row;
  }
  if (col == -1) {
    col = textcol;
  } else {
    textcol = col;
  }
  wmove(di->curwin,row,col);
}

static void
disk_set_attr(Driver *drv, int row, int col, int attr, int count)
{
}

static void
disk_hide_text_cursor(Driver *drv)
{
}

/*
 * Implement stack and unstack window functions by using multiple curses
 * windows.
 */
static void
disk_stack_screen(Driver *drv)
{
  DriverDisk *di = (DriverDisk *) drv;
  int i;

  di->saverc[di->screenctr+1] = textrow*80 + textcol;
  if (++di->screenctr) { /* already have some stacked */
    static char far msg[] = { "stackscreen overflow" };
    if ((i = di->screenctr - 1) >= MAXSCREENS) { /* bug, missing unstack? */
      stopmsg(1,msg);
      exit(1);
    }
    {
      WINDOW **ptr = (WINDOW **) malloc(sizeof(WINDOW *));
      if (ptr) {
	*ptr = di->curwin;
	di->savescreen[i] = (BYTE *) ptr;
	di->curwin = newwin(0, 0, 0, 0);
	touchwin(di->curwin);
	wrefresh(di->curwin);
      } else {
	stopmsg(1,msg);
	exit(1);
      }
    }
    disk_set_clear(drv);
  }
  else
    disk_set_for_text(drv);
}

static void
disk_unstack_screen(Driver *drv)
{
  DriverDisk *di = (DriverDisk *) drv;

  textrow = di->saverc[di->screenctr] / 80;
  textcol = di->saverc[di->screenctr] % 80;
  if (--di->screenctr >= 0) { /* unstack */
    WINDOW **ptr = (WINDOW **) di->savescreen[di->screenctr];

    delwin(di->curwin);
    di->curwin = *ptr;
    touchwin(di->curwin);
    wrefresh(di->curwin);

    free(ptr);
  }
  else
    disk_set_for_graphics(drv);
  disk_move_cursor(drv, -1, -1);
}

static void
disk_discard_screen(Driver *drv)
{
  DriverDisk *di = (DriverDisk *) drv;
  if (--di->screenctr >= 0) { /* unstack */
    if (di->savescreen[di->screenctr]) {
      farmemfree(di->savescreen[di->screenctr]);
    }
  }
}

static int
disk_init_fm(Driver *drv)
{
}

static void
disk_delay(Driver *drv, long time)
{
  ddelay((int)time);
}

static void
disk_buzzer(Driver *drv, int kind)
{
  if ((soundflag & 7) != 0) {
    printf("\007");
    fflush(stdout);
  }
  if (kind==0) {
    disk_redraw(drv);
  }
}

static int
disk_sound_on(Driver *drv, int freq)
{
  fprintf(stderr, "disk_sound_on(%d)\n", freq);
}

static void
disk_sound_off(Driver *drv)
{
  fprintf(stderr, "disk_sound_off\n");
}

static int
disk_diskp(Driver *drv)
{
  return 1;
}

static DriverDisk disk_driver_info = {
  STD_DRIVER_STRUCT(disk),
  NULL,					/* term */
  NULL,					/* curwin */
  0,					/* simple_input */
  NULL,					/* Xgeometry */
  0,					/* old_fcntl */
  0,					/* alarmon */
  0,					/* doredraw */
  DEFX, DEFY,				/* width, height */
  -1,					/* xlastcolor */
  NULL,					/* pixbuf */
  { 0 },				/* cols */
  { 0 },				/* pixtab */
  { 0 },				/* ipixtab */
  0,					/* xbufkey */
  NULL,					/* fontPtr */
  0,					/* screenctr */
  { 0 },				/* savescreen */
  { 0 }					/* saverc */
};

Driver *disk_driver = &disk_driver_info.pub;
