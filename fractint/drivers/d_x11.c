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
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
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
#include "helpdefs.h"
#include "port.h"
#include "prototyp.h"
#include "drivers.h"

#ifdef LINUX
#define FNDELAY O_NDELAY
#endif
#ifdef __SVR4
# include <sys/filio.h>
# define FNDELAY O_NONBLOCK
#endif
#include <assert.h>

/* external variables (set in the FRACTINT.CFG file, but findable here */

extern int slowdisplay;
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

/* the video-palette array (named after the VGA adapter's video-DAC) */

extern unsigned char dacbox[256][3];

extern void drawbox();

extern int text_type;
extern int helpmode;
extern int rotate_hi;

extern void fpe_handler();

typedef unsigned long XPixel;

enum {
  TEXT_WIDTH = 80,
  TEXT_HEIGHT = 25,
  MOUSE_SCALE = 1
};

typedef struct tagDriverX11 DriverX11;
struct tagDriverX11 {
  Driver pub;
  int onroot;				/* = 0; */
  int fullscreen;			/* = 0; */
  int sharecolor;			/* = 0; */
  int privatecolor;			/* = 0; */
  int fixcolors;			/* = 0; */
  /* Run X events synchronously (debugging) */
  int sync;				/* = 0; */
  char *Xdisplay;			/* = ""; */
  char *Xgeometry;			/* = NULL; */
  int doesBacking;

  /*
   * The pixtab stuff is so we can map from fractint pixel values 0-n to
   * the actual color table entries which may be anything.
   */
  int usepixtab;			/* = 0; */
  unsigned long pixtab[256];
  int ipixtab[256];
  XPixel cmap_pixtab[256]; /* for faking a LUTs on non-LUT visuals */
  int cmap_pixtab_alloced;
  int fake_lut;

  int fastmode; /* = 0; */ /* Don't draw pixels 1 at a time */
  int alarmon; /* = 0; */ /* 1 if the refresh alarm is on */
  int doredraw; /* = 0; */ /* 1 if we have a redraw waiting */

  Display *Xdp;				/* = NULL; */
  Window Xw;
  GC Xgc;				/* = NULL; */
  Visual *Xvi;
  Screen *Xsc;
  Colormap Xcmap;
  int Xdepth;
  XImage *Ximage;
  char *Xdata;
  int Xdscreen;
  Pixmap Xpixmap;			/* = None; */
  int Xwinwidth, Xwinheight;
  Window Xroot;
  int xlastcolor;			/* = -1; */
  int xlastfcn;				/* = GXcopy; */
  BYTE *pixbuf;				/* = NULL; */
  XColor cols[256];

  int XZoomWaiting;			/* = 0; */

  const char *x_font_name;		/* = FONT; */
  XFontStruct *font_info;		/* = NULL; */

  int xbufkey; /* = 0; */		/* Buffered X key */
  
  unsigned char *fontPtr;		/* = NULL; */

  char text_screen[TEXT_HEIGHT][TEXT_WIDTH];
  int text_attr[TEXT_HEIGHT][TEXT_WIDTH];

  BYTE *font_table;			/* = NULL; */

  Bool text_modep;			/* True when displaying text */

  /* rubber banding and event processing data */
  int ctl_mode;
  int shift_mode;
  int button_num;
  int last_x, last_y;
  int dx, dy;
};

/* convenience macro to declare local variable di pointing to private
   structure from public pointer */
#define DIX11(arg_) DriverX11 *di = (DriverX11 *) arg_

#ifdef FPUERR
static void continue_hdl(int sig, int code, struct sigcontext *scp,
	char *addr);
#endif

static const int mousefkey[4][4] /* [button][dir] */ = {
    {RIGHT_ARROW, LEFT_ARROW, DOWN_ARROW, UP_ARROW},
    {0, 0, PAGE_DOWN, PAGE_UP},
    {CTL_PLUS, CTL_MINUS, CTL_DEL, CTL_INSERT},
    {CTL_END, CTL_HOME, CTL_PAGE_DOWN, CTL_PAGE_UP}
};


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

#define FONT "-*-*-medium-r-*-*-9-*-*-*-*-*-iso8859-*"
#define DRAW_INTERVAL 6

extern void (*dotwrite)(int, int, int);	/* write-a-dot routine */
extern int (*dotread)(int, int); 	/* read-a-dot routine */
extern void (*linewrite)();		/* write-a-line routine */
extern void (*lineread)();		/* read-a-line routine */

static const VIDEOINFO x11_info = {
  "xfractint mode           ","                         ",
  999, 0,	 0,   0,   0,	19, 640, 480,  256
};

static unsigned long
do_fake_lut(DriverX11 *di, int idx)
{
  return di->fake_lut ? di->cmap_pixtab[idx] : idx;
}
#define FAKE_LUT(_di,_idx) do_fake_lut(_di,_idx)

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
check_arg(DriverX11 *di, int argc, char **argv, int *i)
{
  if (strcmp(argv[*i], "-display") == 0 && (*i)+1 < argc) {
    di->Xdisplay = argv[(*i)+1];
    (*i)++;
    return 1;
  } else if (strcmp(argv[*i], "-fullscreen") == 0) {
    di->fullscreen = 1;
    return 1;
  } else if (strcmp(argv[*i], "-onroot") == 0) {
    di->onroot = 1;
    return 1;
  } else if (strcmp(argv[*i], "-share") == 0) {
    di->sharecolor = 1;
    return 1;
  } else if (strcmp(argv[*i], "-fast") == 0) {
    di->fastmode = 1;
    return 1;
  } else if (strcmp(argv[*i], "-slowdisplay") == 0) {
    slowdisplay = 1;
    return 1;
  } else if (strcmp(argv[*i], "-sync") == 0) {
    di->sync = 1;
    return 1;
  } else if (strcmp(argv[*i], "-private") == 0) {
    di->privatecolor = 1;
    return 1;
  } else if (strcmp(argv[*i], "-fixcolors") == 0 && *i+1 < argc) {
    di->fixcolors = atoi(argv[(*i)+1]);
    (*i)++;
    return 1;
  } else if (strcmp(argv[*i], "-geometry") == 0 && *i+1 < argc) {
    di->Xgeometry = argv[(*i)+1];
    (*i)++;
    return 1;
  } else if (strcmp(argv[*i], "-fn") == 0 && *i+1 < argc) {
    di->x_font_name = argv[(*i)+1];
    (*i)++;
    return 1;
  } else {
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
static void
doneXwindow(DriverX11 *di)
{
  if (di->Xdp == NULL)
    return;

  if (di->Xgc)
    XFreeGC(di->Xdp, di->Xgc);

  if (di->Xpixmap) {
    XFreePixmap(di->Xdp, di->Xpixmap);
    di->Xpixmap = None;
  }
  XFlush(di->Xdp);
#if 0
  XCloseDisplay(di->Xdp);
#endif
  di->Xdp = NULL;
}

/*----------------------------------------------------------------------
 *
 * x11_terminate --
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
x11_terminate(Driver *drv)
{
  DIX11(drv);
  doneXwindow(di);
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

static void
erase_text_screen(DriverX11 *di)
{
  int r, c;
  for (r = 0; r < TEXT_HEIGHT; r++)
    for (c = 0; c < TEXT_WIDTH; c++) {
      di->text_attr[r][c] = 0;
      di->text_screen[r][c] = ' ';
    }
}

/*----------------------------------------------------------------------
 *
 * x11_init --
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
x11_init(Driver *drv, int *argc, char **argv)
{
  DIX11(drv);
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

  initdacbox();

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

  /* filter out x11 arguments */
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

  di->Xdp = XOpenDisplay(di->Xdisplay);
  if (di->Xdp == NULL) {
    x11_terminate(drv);
    return 0;
  }
  di->Xdscreen = XDefaultScreen(di->Xdp);

  erase_text_screen(di);

  /* should enumerate visuals here and build video modes for each */
  memcpy(&vidtbl[vidtbllen++], &x11_info, sizeof(vidtbl[0]));
  return 1;
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
static int errhand(dp, xe)
Display *dp;
XErrorEvent *xe;
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

static void
select_visual(DriverX11 *di)
{
  di->Xvi = XDefaultVisualOfScreen(di->Xsc);
  di->Xdepth = DefaultDepth(di->Xdp, di->Xdscreen);

  switch (di->Xvi->class) {
  case StaticGray:
  case StaticColor:
    colors = (di->Xdepth <= 8) ? di->Xvi->map_entries : 256;
    gotrealdac = 0;
    di->fake_lut = 0;
    break;

  case GrayScale:
  case PseudoColor:
    colors = (di->Xdepth <= 8) ? di->Xvi->map_entries : 256;
    gotrealdac = 1;
    di->fake_lut = 0;
    break;

  case TrueColor:
  case DirectColor:
    colors = 256;
    gotrealdac = 0;
    di->fake_lut = 1;
    break;

  default:
    /* those should be all the visual classes */
    assert(1);
    break;
  }
  if (colors > 256)
    colors = 256;
}

/*----------------------------------------------------------------------
 *
 * x11_flush --
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
static void
x11_flush(Driver *drv)
{
  DIX11(drv);
  XSync(di->Xdp, False);
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
static void
clearXwindow(DriverX11 *di)
{
  char *ptr;
  int i, len;
  if (di->fake_lut) {
    int j;
    for (j = 0; j < di->Ximage->height; j++)
      for (i = 0; i < di->Ximage->width; i++)
	XPutPixel(di->Ximage, i, j, di->cmap_pixtab[di->pixtab[0]]);
  } else if (di->pixtab[0] != 0) {
    /*
     * Initialize image to di->pixtab[0].
     */
    if (colors == 2) {
      for (i = 0; i < di->Ximage->bytes_per_line; i++) {
	di->Ximage->data[i] = 0xff;
      }
    } else {
      for (i = 0; i < di->Ximage->bytes_per_line; i++) {
	di->Ximage->data[i] = di->pixtab[0];
      }
    }
    for (i = 1; i < di->Ximage->height; i++) {
      bcopy(di->Ximage->data, di->Ximage->data+i*di->Ximage->bytes_per_line, 
	    di->Ximage->bytes_per_line);
    }
  } else {
    /*
     * Initialize image to 0's.
     */
    bzero(di->Ximage->data, di->Ximage->bytes_per_line*di->Ximage->height);
  }
  di->xlastcolor = -1;
  XSetForeground(di->Xdp, di->Xgc, FAKE_LUT(di, di->pixtab[0]));
  if (di->onroot)
    XFillRectangle(di->Xdp, di->Xpixmap, di->Xgc,
		   0, 0, di->Xwinwidth, di->Xwinheight);
  XFillRectangle(di->Xdp, di->Xw, di->Xgc,
		 0, 0, di->Xwinwidth, di->Xwinheight);
  x11_flush(&di->pub);
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
static int
x11_resize(Driver *drv)
{
  DIX11(drv);
  static int oldx = -1, oldy = -1;
  int junki;
  unsigned int junkui;
  Window junkw;
  unsigned int width, height;
  int Xmwidth;
  Status status;

  XGetGeometry(di->Xdp, di->Xw, &junkw, &junki, &junki, &width, &height,
	       &junkui, &junkui);

  if (oldx != width || oldy != height) {
    sxdots = width;
    sydots = height;
    vidtbl[0].xdots = sxdots;
    vidtbl[0].ydots = sydots;
    oldx = sxdots;
    oldy = sydots;
    di->Xwinwidth = sxdots;
    di->Xwinheight = sydots;
    screenaspect = sydots/(float) sxdots;
    finalaspectratio = screenaspect;
    Xmwidth = (di->Xdepth > 1) ? sxdots: (1 + sxdots/8);
    if (di->pixbuf != NULL)
      free(di->pixbuf);
    di->pixbuf = (BYTE *) malloc(di->Xwinwidth *sizeof(BYTE));
    if (di->Ximage != NULL) {
      free(di->Ximage->data);
      XDestroyImage(di->Ximage);
    }
    di->Ximage = XCreateImage(di->Xdp, di->Xvi, di->Xdepth, ZPixmap, 0, NULL, sxdots, 
			  sydots, di->Xdepth, 0);
    if (di->Ximage == NULL) {
      printf("XCreateImage failed\n");
      x11_terminate(drv);
      exit(-1);
    }
    di->Ximage->data = malloc(di->Ximage->bytes_per_line * di->Ximage->height);
    if (di->Ximage->data == NULL) {
      fprintf(stderr, "Malloc failed: %d\n", di->Ximage->bytes_per_line *
	      di->Ximage->height);
      exit(-1);
    }
    clearXwindow(di);
    return 1;
  } else {
    return 0;
  }
}


/*
 *----------------------------------------------------------------------
 *
 * x11_read_palette --
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
x11_read_palette(Driver *drv)
{
  DIX11(drv);
  int i;
  if (gotrealdac == 0)
    return -1;
  for (i = 0; i < 256; i++) {
    dacbox[i][0] = di->cols[i].red/1024;
    dacbox[i][1] = di->cols[i].green/1024;
    dacbox[i][2] = di->cols[i].blue/1024;
  }
  return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * x11_write_palette --
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
x11_write_palette(Driver *drv)
{
  DIX11(drv);
  int i;

  if (!gotrealdac) {
    if (di->fake_lut) {
      /* !gotrealdac, fake_lut => truecolor, directcolor displays */
      static unsigned char last_dac[256][3];
      static int last_dac_inited = False;

      for (i = 0; i < 256; i++) {
	if (!last_dac_inited ||
	    last_dac[i][0] != dacbox[i][0] ||
	    last_dac[i][1] != dacbox[i][1] ||
	    last_dac[i][2] != dacbox[i][2]) {
	  di->cols[i].flags = DoRed | DoGreen | DoBlue;
	  di->cols[i].red = dacbox[i][0]*1024;
	  di->cols[i].green = dacbox[i][1]*1024;
	  di->cols[i].blue = dacbox[i][2]*1024;

	  if (di->cmap_pixtab_alloced) {
	    XFreeColors(di->Xdp, di->Xcmap, di->cmap_pixtab + i, 1, None);
	  }
	  if (XAllocColor(di->Xdp, di->Xcmap, &di->cols[i])) {
	    di->cmap_pixtab[i] = di->cols[i].pixel;
	  } else {
	    assert(1);
	    printf("Allocating color %d failed.\n", i);
	  }

	  last_dac[i][0] = dacbox[i][0];
	  last_dac[i][1] = dacbox[i][1];
	  last_dac[i][2] = dacbox[i][2];
	}
      }
      di->cmap_pixtab_alloced = True;
      last_dac_inited = True;
    } else {
      /* !gotrealdac, !fake_lut => static color, static gray displays */
      assert(1);
    }
  } else {
    /* gotrealdac => grayscale or pseudocolor displays */
    for (i = 0; i < 256; i++) {
      di->cols[i].pixel = di->pixtab[i];
      di->cols[i].flags = DoRed | DoGreen | DoBlue;
      di->cols[i].red = dacbox[i][0]*1024;
      di->cols[i].green = dacbox[i][1]*1024;
      di->cols[i].blue = dacbox[i][2]*1024;
    }
    XStoreColors(di->Xdp, di->Xcmap, di->cols, colors);
    XFlush(di->Xdp);
  }

  return 0;
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
static int
xcmapstuff(DriverX11 *di)
{
  int ncells, i;

  if (di->onroot) {
    di->privatecolor = 0;
  }
  for (i = 0; i < colors; i++) {
    di->pixtab[i] = i;
    di->ipixtab[i] = 999;
  }
  if (!gotrealdac) {
    di->Xcmap = DefaultColormapOfScreen(di->Xsc);
    if (di->fake_lut)
      x11_write_palette(&di->pub);
  } else if (di->sharecolor) {
    gotrealdac = 0;
  } else if (di->privatecolor) {
    di->Xcmap = XCreateColormap(di->Xdp, di->Xw, di->Xvi, AllocAll);
    XSetWindowColormap(di->Xdp, di->Xw, di->Xcmap);
  } else {
    int powr;

    di->Xcmap = DefaultColormap(di->Xdp, di->Xdscreen);
    for (powr = di->Xdepth; powr >= 1; powr--) {
      ncells = 1 << powr;
      if (ncells > colors)
	continue;
      if (XAllocColorCells(di->Xdp, di->Xcmap, False, NULL, 0, di->pixtab, 
			   (unsigned int) ncells)) {
	colors = ncells;
	di->usepixtab = 1;
	break;
      }
    }
    if (!di->usepixtab) {
      printf("Couldn't allocate any colors\n");
      gotrealdac = 0;
    }
  }
  for (i = 0; i < colors; i++) {
    di->ipixtab[di->pixtab[i]] = i;
  }
  /* We must make sure if any color uses position 0, that it is 0.
   * This is so we can clear the image with bzero.
   * So, suppose fractint 0 = cmap 42, cmap 0 = fractint 55.
   * Then want fractint 0 = cmap 0, cmap 42 = fractint 55.
   * I.e. pixtab[55] = 42, ipixtab[42] = 55.
   */
  if (di->ipixtab[0] == 999) {
    di->ipixtab[0] = 0;
  } else if (di->ipixtab[0] != 0) {
    int other;
    other = di->ipixtab[0];
    di->pixtab[other] = di->pixtab[0];
    di->ipixtab[di->pixtab[other]] = other;
    di->pixtab[0] = 0;
    di->ipixtab[0] = 0;
  }

  if (!gotrealdac && colors == 2 && BlackPixelOfScreen(di->Xsc) != 0) {
    di->pixtab[0] = di->ipixtab[0] = 1;
    di->pixtab[1] = di->ipixtab[1] = 0;
    di->usepixtab = 1;
  }

  return colors;
}

static int
x11_start_video(Driver *drv)
{
  DIX11(drv);
  clearXwindow(di);
  return 0;
}

static int
x11_end_video(Driver *drv)
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
  ((DriverX11 *) display)->doredraw = 1;
}

/*
 *----------------------------------------------------------------------
 *
 * x11_schedule_alarm --
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
x11_schedule_alarm(Driver *drv, int soon)
{
  DIX11(drv);

  if (!di->fastmode)
    return;

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
 * x11_write_pixel --
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
x11_write_pixel(Driver *drv, int x, int y, int color)
{
  DIX11(drv);
#ifdef DEBUG /* Debugging checks */
    if (color >= colors || color < 0) {
	printf("Color %d too big %d\n", color, colors);
    }
    if (x >= sxdots || x < 0 || y >= sydots || y < 0) {
	printf("Bad coord %d %d\n", x, y);
    }
#endif
    if (di->xlastcolor != color) {
	XSetForeground(di->Xdp, di->Xgc, FAKE_LUT(di, di->pixtab[color]));
	di->xlastcolor = color;
    }
    XPutPixel(di->Ximage, x, y, FAKE_LUT(di, di->pixtab[color]));
    if (di->fastmode == 1 && helpmode != HELPXHAIR) {
	if (!di->alarmon) {
	    x11_schedule_alarm(drv, 0);
	}
    } else {
	XDrawPoint(di->Xdp, di->Xw, di->Xgc, x, y);
	if (di->onroot) {
	    XDrawPoint(di->Xdp, di->Xpixmap, di->Xgc, x, y);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * x11_read_pixel --
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
x11_read_pixel(Driver *drv, int x, int y)
{
  DIX11(drv);
  if (di->fake_lut) {
    int i;
    XPixel pixel = XGetPixel(di->Ximage, x, y);
    for (i = 0; i < 256; i++)
      if (di->cmap_pixtab[i] == pixel)
	return i;
    return 0;
  } else
    return di->ipixtab[XGetPixel(di->Ximage, x, y)];
}

/*
 *----------------------------------------------------------------------
 *
 * x11_write_span --
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
x11_write_span(Driver *drv, int y, int x, int lastx, BYTE *pixels)
{
  int width;
  int i;
  BYTE *pixline;
  DIX11(drv);

#if 1
  if (x == lastx) {
    x11_write_pixel(drv, x, y, pixels[0]);
    return;
  }
  width = lastx-x+1;
  if (di->usepixtab) {
    for (i=0;i < width;i++) {
      di->pixbuf[i] = di->pixtab[pixels[i]];
    }
    pixline = di->pixbuf;
  } else {
    pixline = pixels;
  }
  for (i = 0; i < width; i++) {
    XPutPixel(di->Ximage, x+i, y, FAKE_LUT(di, pixline[i]));
  }
  if (di->fastmode == 1 && helpmode != HELPXHAIR) {
    if (!di->alarmon) {
      x11_schedule_alarm(drv, 0);
    }
  } else {
    XPutImage(di->Xdp, di->Xw, di->Xgc, di->Ximage, x, y, x, y, width, 1);
    if (di->onroot) {
      XPutImage(di->Xdp, di->Xpixmap, di->Xgc, di->Ximage, x, y, x, y, width, 1);
    }
  }
#else
  width = lastx-x+1;
  for (i=0;i < width;i++) {
    x11_write_pixel(x+i, y, pixels[i]);
  }
#endif
}

/*
 *----------------------------------------------------------------------
 *
 * x11_read_span --
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
x11_read_span(Driver *drv, int y, int x, int lastx, BYTE *pixels)
{
  int i, width;
  width = lastx-x+1;
  for (i = 0; i < width; i++) {
    pixels[i] = x11_read_pixel(drv, x+i, y);
  }
}

/*
 *----------------------------------------------------------------------
 *
 * x11_set_line_mode --
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
static void
x11_set_line_mode(Driver *drv, int mode)
{
  DIX11(drv);
  di->xlastcolor = -1;
  if (mode == 0) {
    XSetFunction(di->Xdp, di->Xgc, GXcopy);
    di->xlastfcn = GXcopy;
  } else {
    XSetForeground(di->Xdp, di->Xgc, FAKE_LUT(di, colors-1));
    di->xlastcolor = -1;
    XSetFunction(di->Xdp, di->Xgc, GXxor);
    di->xlastfcn = GXxor;
  }
}

/*
 *----------------------------------------------------------------------
 *
 * x11_draw_line --
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
static void
x11_draw_line(Driver *drv, int x1, int y1, int x2, int y2)
{
  DIX11(drv);
  XDrawLine(di->Xdp, di->Xw, di->Xgc, x1, y1, x2, y2);
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
getachar(DriverX11 *di)
{
  if (0) {
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
static int
translate_key(int ch)
{
  if (ch >= 'a' && ch <= 'z')
    return ch;
  else {
    switch (ch) {
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
static int
handle_esc(DriverX11 *di)
{
  int ch1, ch2, ch3;

#ifdef __hpux
  /* HP escape key sequences. */
  ch1 = getachar();
  if (ch1 == -1) {
    delay(250); /* Wait 1/4 sec to see if a control sequence follows */
    ch1 = getachar();
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
  ch1 = getachar();
  if (ch1 == -1) {
    delay(250); /* Wait 1/4 sec to see if a control sequence follows */
    ch1 = getachar();
  }
  if (ch1 == -1 || !isdigit(ch1))
    return ESC;
  ch2 = getachar();
  if (ch2 == -1) {
    delay(250); /* Wait 1/4 sec to see if a control sequence follows */
    ch2 = getachar();
  }
  if (ch2 == -1)
    return ESC;
  if (isdigit(ch2)) {
    ch3 = getachar();
    if (ch3 == -1) {
      delay(250); /* Wait 1/4 sec to see if a control sequence follows */
      ch3 = getachar();
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
    delay(250); /* Wait 1/4 sec to see if a control sequence follows */
    ch1 = getachar(di);
  }
  if (ch1 != '[')		/* See if we have esc [ */
    return ESC;
  ch1 = getachar(di);
  if (ch1 == -1) {
    delay(250); /* Wait 1/4 sec to see if a control sequence follows */
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
    delay(250); /* Wait 1/4 sec to see if a control sequence follows */
    ch2 = getachar(di);
  }
  if (ch2 == '~') {		/* esc [ ch1 ~ */
    switch (ch1) {
    case '2':		/* esc [ 2 ~ */
      return INSERT;
    case '3':		/* esc [ 3 ~ */
      return FIK_DELETE;
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
      delay(250); /* Wait 1/4 sec to see if a control sequence follows */
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

/* ev_key_press
 *
 * Translate keypress into appropriate fractint character code,
 * according to defines in fractint.h
 */
static int
ev_key_press(DriverX11 *di, XKeyEvent *xevent)
{
  int charcount;
  char buffer[1];
  KeySym keysym;
  int compose;
  charcount = XLookupString(xevent, buffer, 1, &keysym, NULL);
  switch (keysym) {
  case XK_Control_L:
  case XK_Control_R:
    di->ctl_mode = 1;
    return 1;

  case XK_Shift_L:
  case XK_Shift_R:
    di->shift_mode = 1;
    break;
  case XK_Home:
  case XK_R7:
    di->xbufkey = di->ctl_mode ? CTL_HOME : HOME;
    return 1;
  case XK_Left:
  case XK_R10:
    di->xbufkey = di->ctl_mode ? LEFT_ARROW_2 : LEFT_ARROW;
    return 1;
  case XK_Right:
  case XK_R12:
    di->xbufkey = di->ctl_mode ? RIGHT_ARROW_2 : RIGHT_ARROW;
    return 1;
  case XK_Down:
  case XK_R14:
    di->xbufkey = di->ctl_mode ? DOWN_ARROW_2 : DOWN_ARROW;
    return 1;
  case XK_Up:
  case XK_R8:
    di->xbufkey = di->ctl_mode ? UP_ARROW_2 : UP_ARROW;
    return 1;
  case XK_Insert:
    di->xbufkey = di->ctl_mode ? CTL_INSERT : INSERT;
    return 1;
  case XK_Delete:
    di->xbufkey = di->ctl_mode ? CTL_DEL : FIK_DELETE;
    return 1;
  case XK_End:
  case XK_R13:
    di->xbufkey = di->ctl_mode ? CTL_END : END;
    return 1;
  case XK_Help:
    di->xbufkey = F1;
    return 1;
  case XK_Prior:
  case XK_R9:
    di->xbufkey = di->ctl_mode ? CTL_PAGE_UP : PAGE_UP;
    return 1;
  case XK_Next:
  case XK_R15:
    di->xbufkey = di->ctl_mode ? CTL_PAGE_DOWN : PAGE_DOWN;
    return 1;
  case XK_F1:
  case XK_L1:
    di->xbufkey = di->shift_mode ? SF1: F1;
    return 1;
  case XK_F2:
  case XK_L2:
    di->xbufkey = di->shift_mode ? SF2: F2;
    return 1;
  case XK_F3:
  case XK_L3:
    di->xbufkey = di->shift_mode ? SF3: F3;
    return 1;
  case XK_F4:
  case XK_L4:
    di->xbufkey = di->shift_mode ? SF4: F4;
    return 1;
  case XK_F5:
  case XK_L5:
    di->xbufkey = di->shift_mode ? SF5: F5;
    return 1;
  case XK_F6:
  case XK_L6:
    di->xbufkey = di->shift_mode ? SF6: F6;
    return 1;
  case XK_F7:
  case XK_L7:
    di->xbufkey = di->shift_mode ? SF7: F7;
    return 1;
  case XK_F8:
  case XK_L8:
    di->xbufkey = di->shift_mode ? SF8: F8;
    return 1;
  case XK_F9:
  case XK_L9:
    di->xbufkey = di->shift_mode ? SF9: F9;
    return 1;
  case XK_F10:
  case XK_L10:
    di->xbufkey = di->shift_mode ? SF10: F10;
    return 1;
  case '+':
    di->xbufkey = di->ctl_mode ? CTL_PLUS : '+';
    return 1;
  case '-':
    di->xbufkey = di->ctl_mode ? CTL_MINUS : '-';
    return 1;
    break;
  case XK_Return:
  case XK_KP_Enter:
    di->xbufkey = di->ctl_mode ? CTL('T') : '\n';
    return 1;
  }
  if (charcount == 1) {
    di->xbufkey = buffer[0];
    if (di->xbufkey == '\003') {
      goodbye();
    }
  }
}

/* ev_key_release
 *
 * toggle modifier key state for shit and contol keys, otherwise ignore
 */
static void
ev_key_release(DriverX11 *di, XKeyEvent *xevent)
{
  char buffer[1];
  KeySym keysym;
  (void) XLookupString(xevent, buffer, 1, &keysym, NULL);
  switch (keysym) {
  case XK_Control_L:
  case XK_Control_R:
    di->ctl_mode = 0;
    break;
  case XK_Shift_L:
  case XK_Shift_R:
    di->shift_mode = 0;
    break;
  }
}

static void
ev_expose(DriverX11 *di, XExposeEvent *xevent)
{
  if (di->text_modep) {
    /* if text window, refresh text */
  } else {
    /* refresh graphics */
    int x, y, w, h;
    x = xevent->x;
    y = xevent->y;
    w = xevent->width;
    h = xevent->height;
    if (x+w > sxdots) {
      w = sxdots-x;
    }
    if (y+h > sydots) {
      h = sydots-y;
    }
    if (x < sxdots && y < sydots && w > 0 && h > 0) {
      XPutImage(di->Xdp, di->Xw, di->Xgc, di->Ximage, x, y, x, y,
		xevent->width, xevent->height);
    }
  }
}


/*
 *----------------------------------------------------------------------
 *
 * x11_redraw --
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
x11_redraw(Driver *drv)
{
  DIX11(drv);
  if (di->alarmon) {
    XPutImage(di->Xdp, di->Xw, di->Xgc, di->Ximage, 0, 0, 0, 0,
	      sxdots, sydots);
    if (di->onroot)
      XPutImage(di->Xdp, di->Xpixmap, di->Xgc, di->Ximage, 0, 0, 0, 0,
		sxdots, sydots);
    di->alarmon = 0;
  }
  di->doredraw = 0;
}

static void
ev_button_press(DriverX11 *di, XEvent *xevent)
{
  int done = 0;
  int banding = 0;
  int bandx0, bandy0, bandx1, bandy1;

  if (lookatmouse == 3 || zoomoff == 0) {
    di->last_x = xevent->xbutton.x;
    di->last_y = xevent->xbutton.y;
    return;
  }

  bandx1 = bandx0 = xevent->xbutton.x;
  bandy1 = bandy0 = xevent->xbutton.y;
  while (!done) {
    XNextEvent(di->Xdp, xevent);
    switch (xevent->type) {
    case MotionNotify:
      while (XCheckWindowEvent(di->Xdp, di->Xw, PointerMotionMask, xevent))
	1;
      if (banding)
	XDrawRectangle(di->Xdp, di->Xw, di->Xgc, MIN(bandx0, bandx1), 
		       MIN(bandy0, bandy1), ABS(bandx1-bandx0), 
		       ABS(bandy1-bandy0));
      bandx1 = xevent->xmotion.x;
      bandy1 = xevent->xmotion.y;
      if (ABS(bandx1-bandx0)*finalaspectratio > ABS(bandy1-bandy0))
	bandy1 =
	  SIGN(bandy1-bandy0)*ABS(bandx1-bandx0)*finalaspectratio + bandy0;
      else
	bandx1 =
	  SIGN(bandx1-bandx0)*ABS(bandy1-bandy0)/finalaspectratio + bandx0;

      if (!banding) {
	/* Don't start rubber-banding until the mouse
	   gets moved.  Otherwise a click messes up the
	   window */
	if (ABS(bandx1-bandx0) > 10 || ABS(bandy1-bandy0) > 10) {
	  banding = 1;
	  XSetForeground(di->Xdp, di->Xgc, FAKE_LUT(di, colors-1));
	  XSetFunction(di->Xdp, di->Xgc, GXxor);
	}
      }
      if (banding) {
	XDrawRectangle(di->Xdp, di->Xw, di->Xgc, MIN(bandx0, bandx1), 
		       MIN(bandy0, bandy1), ABS(bandx1-bandx0), 
		       ABS(bandy1-bandy0));
      }
      XFlush(di->Xdp);
      break;

    case ButtonRelease:
      done = 1;
      break;
    }
  }

  if (!banding)
    return;

  XDrawRectangle(di->Xdp, di->Xw, di->Xgc, MIN(bandx0, bandx1), 
		 MIN(bandy0, bandy1), ABS(bandx1-bandx0), 
		 ABS(bandy1-bandy0));
  if (bandx1 == bandx0)
    bandx1 = bandx0+1;
  if (bandy1 == bandy0)
    bandy1 = bandy0+1;
  zrotate = 0;
  zskew = 0;
  zbx = (MIN(bandx0, bandx1)-sxoffs)/dxsize;
  zby = (MIN(bandy0, bandy1)-syoffs)/dysize;
  zwidth = ABS(bandx1-bandx0)/dxsize;
  zdepth = zwidth;
  if (!inside_help)
    di->xbufkey = ENTER;
  if (di->xlastcolor != -1)
    XSetForeground(di->Xdp, di->Xgc, FAKE_LUT(di, di->xlastcolor));
  XSetFunction(di->Xdp, di->Xgc, di->xlastfcn);
  di->XZoomWaiting = 1;
  drawbox(0);
}

static void
ev_motion_notify(DriverX11 *di, XEvent *xevent)
{
  if (editpal_cursor && !inside_help) {
    while (XCheckWindowEvent(di->Xdp, di->Xw, PointerMotionMask, xevent))
      1;
	  
    if (xevent->xmotion.state & Button2Mask ||
	(xevent->xmotion.state & (Button1Mask | Button3Mask))) {
      di->button_num = 3;
    } else if (xevent->xmotion.state & Button1Mask) {
      di->button_num = 1;
    } else if (xevent->xmotion.state & Button3Mask) {
      di->button_num = 2;
    } else {
      di->button_num = 0;
    }

    if (lookatmouse == 3 && di->button_num != 0) {
      di->dx += (xevent->xmotion.x-di->last_x)/MOUSE_SCALE;
      di->dy += (xevent->xmotion.y-di->last_y)/MOUSE_SCALE;
      di->last_x = xevent->xmotion.x;
      di->last_y = xevent->xmotion.y;
    } else {
      Cursor_SetPos(xevent->xmotion.x, xevent->xmotion.y);
      di->xbufkey = ENTER;
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
static int
handle_events(DriverX11 *di)
{
  XEvent xevent;

  if (di->doredraw)
    x11_redraw(&di->pub);

  while (XPending(di->Xdp) && !di->xbufkey) {
    XNextEvent(di->Xdp, &xevent);
    switch (xevent.type) {
    case KeyRelease:
      ev_key_release(di, &xevent.xkey);
      break;

    case KeyPress:
      if (ev_key_press(di, &xevent.xkey))
	return;
      break;

    case MotionNotify:
      ev_motion_notify(di, &xevent);
      break;

    case ButtonPress:
      ev_button_press(di, &xevent);
      break;

    case Expose:
      ev_expose(di, &xevent.xexpose);
      break;
    }
  }
  
  if (!di->xbufkey && editpal_cursor && !inside_help && lookatmouse == 3 &&
      (di->dx != 0 || di->dy != 0)) {
    if (ABS(di->dx) > ABS(di->dy)) {
      if (di->dx > 0) {
	di->xbufkey = mousefkey[di->button_num][0]; /* right */
	di->dx--;
      } else if (di->dx < 0) {
	di->xbufkey = mousefkey[di->button_num][1]; /* left */
	di->dx++;
      }
    } else {
      if (di->dy > 0) {
	di->xbufkey = mousefkey[di->button_num][2]; /* down */
	di->dy--;
      } else if (di->dy < 0) {
	di->xbufkey = mousefkey[di->button_num][3]; /* up */
	di->dy++;
      }
    }
  }
}

/* Check if there is a character waiting for us.  */
static int
input_pending(DriverX11 *di)
{
  return XPending(di->Xdp);
#if 0
  struct timeval now;
  fd_set read_fds;

  memset(&now, 0, sizeof(now));
  FD_ZERO(&read_fds);
  FD_SET(ConnectionNumber(di->Xdp), &read_fds);

  return select(1, &read_fds, NULL, NULL, &now) > 0;
#endif
}

/*----------------------------------------------------------------------
 *
 * x11_get_key --
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
x11_get_key(Driver *drv, int block)
{
  static int skipcount = 0;
  DIX11(drv);

  while (1) {
    /* Don't check X events every time, since that is expensive */
    skipcount++;
    if (block == 0 && skipcount < 25)
      break;
    skipcount = 0;

    handle_events(di);

    if (di->xbufkey) {
      int ch = di->xbufkey;
      di->xbufkey = 0;
      skipcount = 9999; /* If we got a key, check right away next time */
      return translate_key(ch);
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

      FD_SET(ConnectionNumber(di->Xdp), &reads);
      status = select(ConnectionNumber(di->Xdp) + 1, &reads, NULL, NULL, &tout);

      if (status <= 0)
	return 0;
    }
  }

  return 0;
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
static Window
pr_dwmroot(DriverX11 *di, Display *dpy, Window pwin)
{
  /* search for DEC Window Manager root */
  XWindowAttributes pxwa, cxwa;
  Window  root, parent, *child;
  unsigned int     i, nchild;

  if (!XGetWindowAttributes(dpy, pwin, &pxwa)) {
    printf("Search for root: XGetWindowAttributes failed\n");
    return RootWindow(dpy, di->Xdscreen);
  }
  if (XQueryTree(dpy, pwin, &root, &parent, &child, &nchild)) {
    for (i = 0; i < nchild; i++) {
      if (!XGetWindowAttributes(dpy, child[i], &cxwa)) {
	printf("Search for root: XGetWindowAttributes failed\n");
	return RootWindow(dpy, di->Xdscreen);
      }
      if (pxwa.width == cxwa.width && pxwa.height == cxwa.height)
	return pr_dwmroot(di, dpy, child[i]);
    }
    return(pwin);
  } else {
    printf("xfractint: failed to find root window\n");
    return RootWindow(dpy, di->Xdscreen);
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
static Window
FindRootWindow(DriverX11 *di)
{
  int i;
  di->Xroot = RootWindow(di->Xdp, di->Xdscreen);
  di->Xroot = pr_dwmroot(di, di->Xdp, di->Xroot); /* search for DEC wm root */

  {  /* search for swm/tvtwm root (from ssetroot by Tom LaStrange) */
    Atom __SWM_VROOT = None;
    Window rootReturn, parentReturn, *children;
    unsigned int numChildren;

    __SWM_VROOT = XInternAtom(di->Xdp, "__SWM_VROOT", False);
    XQueryTree(di->Xdp, di->Xroot, &rootReturn, &parentReturn,
	       &children, &numChildren);
    for (i = 0; i < numChildren; i++) {
      Atom actual_type;
      int actual_format;
      unsigned long nitems, bytesafter;
      Window *newRoot = NULL;

      if (XGetWindowProperty (di->Xdp, children[i], __SWM_VROOT,
			      (long) 0, (long) 1,
			      False, XA_WINDOW,
			      &actual_type, &actual_format,
			      &nitems, &bytesafter,
			      (unsigned char **) &newRoot) == Success &&
	  newRoot) {
	di->Xroot = *newRoot; break;
      }
    }
  }
  return di->Xroot;
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
static void
RemoveRootPixmap(DriverX11 *di)
{
  Atom prop, type;
  int format;
  unsigned long nitems, after;
  Pixmap *pm;

  prop = XInternAtom(di->Xdp, "_XSETROOT_ID", False);
  if (XGetWindowProperty(di->Xdp, di->Xroot, prop, (long) 0, (long) 1, 1,
			 AnyPropertyType, &type, &format, &nitems, &after,
			 (unsigned char **) &pm) == Success && nitems == 1) {
    if (type == XA_PIXMAP && format == 32 && after == 0) {
      XKillClient(di->Xdp, (XID)*pm);
      XFree((char *)pm);
    }
  }
}

/*----------------------------------------------------------------------
 *
 * x11_window --
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
static void
x11_window(Driver *drv)
{
  XSetWindowAttributes Xwatt;
  XGCValues Xgcvals;
  int Xwinx = 0, Xwiny = 0;
  int i;
  DIX11(drv);

  adapter = 0;

  /* We have to do some X stuff even for disk video, to parse the geometry
   * string */

  if (di->Xgeometry && !di->onroot)
    XGeometry(di->Xdp, di->Xdscreen, di->Xgeometry, DEFXY, 0, 1, 1, 0, 0,
	      &Xwinx, &Xwiny, &di->Xwinwidth, &di->Xwinheight);
  if (di->sync)
    XSynchronize(di->Xdp, True);
  XSetErrorHandler(errhand);
  di->Xsc = ScreenOfDisplay(di->Xdp, di->Xdscreen);
  select_visual(di);
  if (di->fixcolors > 0)
    colors = di->fixcolors;

  if (di->fullscreen || di->onroot) {
    di->Xwinwidth = DisplayWidth(di->Xdp, di->Xdscreen);
    di->Xwinheight = DisplayHeight(di->Xdp, di->Xdscreen);
  }
  sxdots = di->Xwinwidth;
  sydots = di->Xwinheight;

  Xwatt.background_pixel = BlackPixelOfScreen(di->Xsc);
  Xwatt.bit_gravity = StaticGravity;
  di->doesBacking = DoesBackingStore(di->Xsc);
  if (di->doesBacking)
    Xwatt.backing_store = Always;
  else
    Xwatt.backing_store = NotUseful;
  if (di->onroot) {
    di->Xroot = FindRootWindow(di);
    RemoveRootPixmap(di);
    di->Xgc = XCreateGC(di->Xdp, di->Xroot, 0, &Xgcvals);
    di->Xpixmap = XCreatePixmap(di->Xdp, di->Xroot,
				di->Xwinwidth, di->Xwinheight, di->Xdepth);
    di->Xw = di->Xroot;
    XFillRectangle(di->Xdp, di->Xpixmap, di->Xgc, 0, 0, di->Xwinwidth, di->Xwinheight);
    XSetWindowBackgroundPixmap(di->Xdp, di->Xroot, di->Xpixmap);
  } else {
    di->Xroot = DefaultRootWindow(di->Xdp);
    di->Xw = XCreateWindow(di->Xdp, di->Xroot, Xwinx, Xwiny,
			   di->Xwinwidth, di->Xwinheight, 0, di->Xdepth,
			   InputOutput, CopyFromParent,
			   CWBackPixel | CWBitGravity | CWBackingStore,
			   &Xwatt);
    XStoreName(di->Xdp, di->Xw, "xfractint");
    di->Xgc = XCreateGC(di->Xdp, di->Xw, 0, &Xgcvals);
  }
  colors = xcmapstuff(di);
  if (rotate_hi == 255) rotate_hi = colors-1;

  {
    unsigned long event_mask = KeyPressMask | KeyReleaseMask | ExposureMask;
    if (! di->onroot)
      event_mask |= ButtonPressMask | ButtonReleaseMask | PointerMotionMask;
    XSelectInput(di->Xdp, di->Xw, event_mask);
  }

  if (!di->onroot) {
    XSetBackground(di->Xdp, di->Xgc, FAKE_LUT(di, di->pixtab[0]));
    XSetForeground(di->Xdp, di->Xgc, FAKE_LUT(di, di->pixtab[1]));
    Xwatt.background_pixel = FAKE_LUT(di, di->pixtab[0]);
    XChangeWindowAttributes(di->Xdp, di->Xw, CWBackPixel, &Xwatt);
    XMapWindow(di->Xdp, di->Xw);
  }

  x11_resize(drv);
  x11_flush(drv);
  x11_write_palette(drv);

  vidtbl[0].xdots = sxdots;
  vidtbl[0].ydots = sydots;
  vidtbl[0].colors = colors;
  vidtbl[0].dotmode = 19;
}

static void
load_font(DriverX11 *di)
{
  di->font_info = XLoadQueryFont(di->Xdp, di->x_font_name);
  if (di->font_info == NULL)
    di->font_info = XLoadQueryFont(di->Xdp, "6x12");
}

/*
 *----------------------------------------------------------------------
 *
 * xgetfont --
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
static BYTE *
x11_find_font(Driver *drv, int parm)
{
  DIX11(drv);

  if (!di->font_table) {
    XImage *font_image;
    char str[8];
    int i, j, k, l;
    int width;
    Pixmap font_pixmap;
    XGCValues values;
    GC font_gc;

    di->font_table = (unsigned char *) malloc(128*8);
    bzero(di->font_table, 128*8);

    di->xlastcolor = -1;
    if (! di->font_info)
      load_font(di);
    if (di->font_info == NULL)
      return NULL;
    width = di->font_info->max_bounds.width;
    if (di->font_info->max_bounds.width > 8 ||
	di->font_info->max_bounds.width != di->font_info->min_bounds.width) {
      fprintf(stderr, "Bad font\n");
      free(di->font_table);
      di->font_table = NULL;
      return NULL;
    }

    font_pixmap = XCreatePixmap(di->Xdp, di->Xw, 64, 8, 1);
    assert(font_pixmap);
    values.background = 0;
    values.foreground = 1;
    values.font = di->font_info->fid;
    font_gc = XCreateGC(di->Xdp, font_pixmap,
			GCForeground | GCBackground | GCFont, &values);
    assert(font_gc);

    for (i = 0; i < 128; i += 8) {
      for (j = 0; j < 8; j++)
	str[j] = i+j;

      XDrawImageString(di->Xdp, font_pixmap, di->Xgc, 0, 8, str, 8);
      font_image =
	XGetImage(di->Xdp, font_pixmap, 0, 0, 64, 8, AllPlanes, XYPixmap);
      assert(font_image);
      for (j = 0; j < 8; j++) {
	for (k = 0; k < 8; k++) {
	  for (l = 0; l < width; l++) {
	    if (XGetPixel(font_image, j*width+l, k)) {
	      di->font_table[(i+j)*8+k] = (di->font_table[(i+j)*8+k] << 1) | 1;
	    } else {
	      di->font_table[(i+j)*8+k] = (di->font_table[(i+j)*8+k] << 1);
	    }
	  }
	}
      }
      XDestroyImage(font_image);
    }
    
    XFreeGC(di->Xdp, font_gc);
    XFreePixmap(di->Xdp, font_pixmap);
  }
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
x11_shell(Driver *drv)
{
  DIX11(drv);
  SignalHandler sigint;
  char *shell;
  char *argv[2];
  int pid, donepid;

  sigint = (SignalHandler) signal(SIGINT, SIG_IGN);
  shell = getenv("SHELL");
  if (shell == NULL)
    shell = SHELL;

  argv[0] = shell;
  argv[1] = NULL;

  /* Clean up the window */

  /* Fork the shell; it should be something like an xterm */
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

  signal(SIGINT, (SignalHandler) sigint);
  putchar('\n');
}

static void
x11_put_string(Driver *drv, int row, int col, int attr, const char *msg)
{
  DIX11(drv);
  int r, c;

  fprintf(stderr, "x11_put_string(%d,%d, %u): ``%s''\n", row, col, attr, msg);

  if (row != -1)
    textrow = row;
  if (col != -1)
    textcol = col;

  r = textrow + textrbase;
  c = textcol + textcbase;

  while (*msg) {
    if ('\n' == *msg) {
      textrow++;
      r++;
      textcol = 0;
      c = textcbase;
    } else {
      assert(r < TEXT_HEIGHT);
      assert(c < TEXT_WIDTH);
      di->text_screen[r][c] = *msg;
      di->text_attr[r][c] = attr;
      textcol++;
      c++;
    }
    msg++;
  }
}


static void
x11_set_for_text(Driver *drv)
{
  DIX11(drv);
  if (! di->font_info)
    load_font(di);
  /* map text screen child window */
  /* allocate text colors in window's colormap, save window's colors */
  /* refresh window with last contents of text_screen */
  di->text_modep = True;
  fprintf(stderr, "x11_set_for_text\n");
}

static void
x11_set_for_graphics(Driver *drv)
{
  DIX11(drv);
  /* unmap text screen child window */
  /* restore colormap from saved copy */
  /* expose will be sent for newly visible area */
  di->text_modep = False;
  fprintf(stderr, "x11_set_for_graphics\n");
}

static void
x11_set_clear(Driver *drv)
{
  DIX11(drv);
  erase_text_screen(di);
  x11_set_for_text(drv);
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
x11_set_video_mode(Driver *drv, int ax, int bx, int cx, int dx)
{
  if (diskflag)
    enddisk();
  driver_end_video();
  goodmode = 1;
  switch (dotmode) {
  case 0:				/* text */
#if 0
    clear();
#endif
    break;

  case 19: /* X window */
    dotwrite = driver_write_pixel;
    dotread = driver_read_pixel;
    lineread = driver_read_span;
    linewrite = driver_write_span;
    x11_start_video(drv);
    x11_set_for_graphics(drv);
    break;

  default:
    printf("Bad mode %d\n", dotmode);
    exit(-1);
  } 
  if (dotmode !=0) {
    x11_read_palette(drv);
    andcolor = colors-1;
    boxcount =0;
  }
}

static void
x11_move_cursor(Driver *drv, int row, int col)
{
  /* draw reverse video text cursor at new position */
  fprintf(stderr, "x11_move_cursor(%d,%d)\n", row, col);
}

static void
x11_hide_text_cursor(Driver *drv)
{
  /* erase cursor if currently drawn */
  fprintf(stderr, "x11_hide_text_cursor\n");
}

static void
x11_set_attr(Driver *drv, int row, int col, int attr, int count)
{
  DIX11(drv);
  int i = col;

  while (count) {
    assert(row < TEXT_HEIGHT);
    assert(i < TEXT_WIDTH);
    di->text_attr[row][i] = attr;
    if (++i == TEXT_WIDTH) {
      i = 0;
      row++;
    }
    count--;
  }
  /* refresh text */
  fprintf(stderr, "x11_set_attr(%d,%d, %d): %d\n", row, col, count, attr);
}

static void
x11_scroll_up(Driver *drv, int top, int bot)
{
  DIX11(drv);
  int r, c;
  assert(bot <= TEXT_HEIGHT);
  for (r = top; r < bot; r++)
    for (c = 0; c < TEXT_WIDTH; c++) {
      di->text_attr[r][c] = di->text_attr[r+1][c];
      di->text_screen[r][c] = di->text_screen[r+1][c];
    }
  for (c = 0; c < TEXT_WIDTH; c++) {
    di->text_attr[bot][c] = 0;
    di->text_screen[bot][c] = ' ';
  }
  /* draw text */
  fprintf(stderr, "x11_scroll_up(%d, %d)\n", top, bot);
}

static void
x11_stack_screen(Driver *drv)
{
  fprintf(stderr, "x11_stack_screen\n");
}

static void
x11_unstack_screen(Driver *drv)
{
  fprintf(stderr, "x11_unstack_screen\n");
}

static void
x11_discard_screen(Driver *drv)
{
  fprintf(stderr, "x11_discard_screen\n");
}

static int
x11_init_fm(Driver *drv)
{
  return 0;
}

static void
x11_buzzer(Driver *drv, int kind)
{
  fprintf(stderr, "x11_buzzer(%d)\n", kind);
}

static int
x11_sound_on(Driver *drv, int freq)
{
  fprintf(stderr, "x11_sound_on(%d)\n", freq);
  return 0;
}

static void
x11_sound_off(Driver *drv)
{
  fprintf(stderr, "x11_sound_off\n");
}

static int
x11_diskp(Driver *drv)
{
  return 0;
}

static void
x11_mute()
{
}

/*
 * place this last in the file to avoid having to forward declare routines
 */
static DriverX11 x11_driver_info = {
  STD_DRIVER_STRUCT(x11),
  0,					/* onroot */
  0,					/* fullscreen */
  0,					/* sharecolor */
  0,					/* privatecolor */
  0,					/* fixcolors */
  0,					/* sync */
  "",					/* Xdisplay */
  NULL,					/* Xgeometry */
  0,					/* doesBacking */
  0,					/* usepixtab */
  { 0L },				/* pixtab */
  { 0 },				/* ipixtab */
  { 0L },				/* cmap_pixtab */
  0,					/* cmap_pixtab_alloced */
  0,					/* fake_lut */
  0,					/* fastmode */
  0,					/* alarmon */
  0,					/* doredraw */
  NULL,					/* Xdp */
  None,					/* Xw */
  None,					/* Xgc */
  NULL,					/* Xvi */
  NULL,					/* Xsc */
  None,					/* Xcmap */
  0,					/* Xdepth */
  NULL,					/* Ximage */
  NULL,					/* Xdata */
  0,					/* Xdscreen */
  None,					/* Xpixmap */
  DEFX, DEFY,				/* Xwinwidth, Xwinheight */
  None,					/* Xroot */
  -1,					/* xlastcolor */
  GXcopy,				/* xlastfcn */
  NULL,					/* pixbuf */
  { 0 },				/* cols */
  0,					/* XZoomWaiting */
  FONT,					/* x_font_name */
  NULL,					/* font_info */
  0,					/* xbufkey */
  NULL,					/* fontPtr */
  { 0 },				/* text_screen */
  { 0 },				/* text_attr */
  NULL,					/* font_table */
  False,				/* text_modep */
  0,					/* ctl_mode */
  0,					/* shift_mode */
  0,					/* button_num */
  0, 0,					/* last_x, last_y */
  0, 0					/* dx, dy */
};

Driver *x11_driver = &x11_driver_info.pub;
