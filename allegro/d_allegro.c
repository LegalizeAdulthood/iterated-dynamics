/* d_allegro.c
 * This file contains routines for the Allegro port of the Unix port of fractint.
 * It uses an X window for both text and graphics.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
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
#include <allegro.h>
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

/* external variables (set in the FRACTINT.CFG file, but findable here */

extern int slowdisplay;
extern int dotmode;		/* video access method (= 19)	   */
extern int sxdots, sydots; 	/* total # of dots on the screen   */
extern int sxoffs, syoffs; 	/* offset of drawing area          */
extern int colors; 		/* maximum colors available	   */
extern int initmode;
extern int adapter;
extern int gotrealdac;
extern int inside_help;
extern float finalaspectratio;
extern float screenaspect;
extern int lookatmouse;
extern int calc_status;
extern int badconfig;

extern struct videoinfo videotable[];

/* the video-palette array (named after the VGA adapter's video-DAC) */

extern unsigned char dacbox[256][3];
extern void drawbox();

extern int text_type;
extern int helpmode;
extern int rotate_hi;
extern long coloriter;

extern void fpe_handler();

typedef unsigned long XPixel;

enum {
  TEXT_WIDTH = 80,
  TEXT_HEIGHT = 25,
  MOUSE_SCALE = 1
};

typedef struct tagDriverAlgro DriverAlgro;
struct tagDriverAlgro {
  Driver pub;
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
  unsigned long cmap_pixtab[256]; /* for faking a LUTs on non-LUT visuals */
  int cmap_pixtab_alloced;
  int fake_lut;

  int fastmode; /* = 0; */ /* Don't draw pixels 1 at a time */
  int alarmon; /* = 0; */ /* 1 if the refresh alarm is on */
  int doredraw; /* = 0; */ /* 1 if we have a redraw waiting */

  struct BITMAP *bmp;                   /* bitmap for graphics display */
  struct BITMAP *txt;                   /* bitmap for text display */
  struct BITMAP *stack_txt;             /* bitmap for stacked text display */
  int depth;
  char *Xdata;
  int Xdscreen;
  int gfx_mode;
  int winwidth, winheight;
  int lastcolor;			/* = -1; */
  BYTE *pixbuf;				/* = NULL; */
  RGB cols[256];
  PALETTE pal;

  int XZoomWaiting;			/* = 0; */

  const char *x_font_name;		/* = FONT; */

  int xbufkey; /* = 0; */		/* Buffered X key */
  
  unsigned char *fontPtr;		/* = NULL; */

  int txt_ht;                           /* text letter height = 2^txt_ht pixels */
  int txt_wt;                           /* text letter width = 2^txt_wt pixels */

  char text_screen[TEXT_HEIGHT][TEXT_WIDTH];
  int text_attr[TEXT_HEIGHT][TEXT_WIDTH];
  char stack_text_screen[TEXT_HEIGHT][TEXT_WIDTH];
  int stack_text_attr[TEXT_HEIGHT][TEXT_WIDTH];

  BYTE *font_table;			/* = NULL; */

  int text_modep;			/* >= 1 when displaying text */

  /* rubber banding and event processing data */
  int ctl_mode;
  int shift_mode;
  int button_num;
  int last_x, last_y;
  int dx, dy;
};

/* convenience macro to declare local variable di pointing to private
   structure from public pointer */
#define DIALGRO(arg_) DriverAlgro *di = (DriverAlgro *) arg_

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
#ifndef ABS
#define ABS(x) ((x) > 0 ? (x) : -(x))
#endif
#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif
#define SIGN(x) ((x) > 0 ? 1 : -1)

#define SHELL "/bin/csh"

#define FONT "-*-*-medium-r-*-*-9-*-*-*-*-*-iso8859-*"
#define DRAW_INTERVAL 6

extern void (*dotwrite)(int, int, int);	/* write-a-dot routine */
extern int (*dotread)(int, int); 	/* read-a-dot routine */
extern void (*linewrite)();		/* write-a-line routine */
extern void (*lineread)();		/* read-a-line routine */

VIDEOINFO algro_info = {
  "Allegro driver ","                         ",
  999, 0, 0, 0, 0, 8, 19, 640, 480, 256
};
#if 0
static unsigned long
do_fake_lut(DriverAlgro *di, int idx)
{
  return di->fake_lut ? di->cmap_pixtab[idx] : idx;
}
#define FAKE_LUT(_di,_idx) do_fake_lut(_di,_idx)
#endif
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
check_arg(DriverAlgro *di, int argc, char **argv, int *i)
{
  if (strcmp(argv[*i], "-display") == 0 && (*i)+1 < argc) {
    di->Xdisplay = argv[(*i)+1];
    (*i)++;
    return 1;
  } else if (strcmp(argv[*i], "-fullscreen") == 0) {
    di->fullscreen = 1;
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
 * Algro_terminate --
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
algro_terminate(Driver *drv)
{
  allegro_exit();
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
erase_text_screen(DriverAlgro *di)
{
  int r, c;

  clear_to_color(di->txt, palette_color[7]); /* white background */
/*
  acquire_screen();
  blit(di->txt,screen,0,0,0,0,TEXT_WIDTH<<di->txt_wt,TEXT_HEIGHT<<di->txt_ht);
  release_screen();
*/
  for (r = 0; r < TEXT_HEIGHT; r++)
    for (c = 0; c < TEXT_WIDTH; c++) {
      di->text_attr[r][c] = 0;
      di->text_screen[r][c] = ' ';
    }
}

static void
get_default_mode(Driver *drv, VIDEOINFO *mode)
{
  DIALGRO(drv);

  di->depth = desktop_color_depth();
  set_color_depth(di->depth);
  mode->truecolorbits = di->depth;

  switch (di->depth) {
    default:
    case 8:
       mode->colors = 256;
       mode->videomodeR = 0;
       mode->videomodeG = 0;
       mode->videomodeB = 0;
       mode->videomodeA = 0;
       break;
    case 15:
       mode->colors = 32768;
       mode->videomodeR = 5;
       mode->videomodeG = 5;
       mode->videomodeB = 5;
       mode->videomodeA = 0;
       break;
    case 16:
       mode->colors = 65536;
       mode->videomodeR = 5;
       mode->videomodeG = 6;
       mode->videomodeB = 5;
       mode->videomodeA = 0;
       break;
    case 24:
       mode->colors = 16777216;
       mode->videomodeR = 8;
       mode->videomodeG = 8;
       mode->videomodeB = 8;
       mode->videomodeA = 0;
       break;
    case 32:
       mode->colors = 16777216;
       mode->videomodeR = 8;
       mode->videomodeG = 8;
       mode->videomodeB = 8;
       mode->videomodeA = 8;
       break;
   }
   mode->colors = 256; /* For now, we can't handle more */
#if 0
fprintf(stderr, "colors=%i, R=%i, G=%i, B=%i, A=%i, depth=%i\n", mode->colors, mode->videomodeR,
              mode->videomodeG, mode->videomodeB, mode->videomodeA, mode->truecolorbits);
#endif
   return;
}

#if 0
static int
test_video_mode(Driver *drv, VIDEOINFO *mode)
{
  int result = 1;
  int old_color_depth;
  DIALGRO(drv);

  old_color_depth = desktop_color_depth();
  set_color_depth(mode->truecolorbits);

  if(set_gfx_mode(di->gfx_mode, mode->xdots, mode->ydots, 0, 0) < 0) {
     result = 0; /* setting graphics mode failed */
     fprintf(stderr, "%s, x=%i, y=%i, depth=%i\n", allegro_error, mode->xdots,
              mode->ydots, mode->truecolorbits);
  }

  set_color_depth(old_color_depth);
  return (result);
}
#endif

static void
generate_video_table(Driver *drv)
{
  int viddepths[] = {8, 15, 16, 24, 32};
  int numdepths = 5;
  long colornums[] = {256, 32768, 65536, 16777216, 16777216};
  int R[]         = {0,  5,  5,  8, 8};
  int G[]         = {0,  5,  6,  8, 8};
  int B[]         = {0,  5,  5,  8, 8};
  int A[]         = {0,  0,  0,  0, 8};
  int vidxresol[] = {640, 800, 1024, 1600};
  int vidyresol[] = {480, 600,  768, 1200};
  int numresols = 4;
  int i, j;

  for (i = 0; i < numdepths; i++) {
    algro_info.truecolorbits = viddepths[i];
    algro_info.colors = colornums[i];
    algro_info.videomodeR = R[i];
    algro_info.videomodeG = G[i];
    algro_info.videomodeB = B[i];
    algro_info.videomodeA = A[i];
    for (j = 0; j < numresols; j++) {
      algro_info.xdots = vidxresol[j];
      algro_info.ydots = vidyresol[j];
      add_video_mode(drv, &algro_info);
    }
  }
}

/*----------------------------------------------------------------------
 *
 * algro_init --
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
algro_init(Driver *drv, int *argc, char **argv)
{
  DIALGRO(drv);
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
  text_type = 2; /* graphics text */

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

   allegro_init();  /* ***Need to set up configuration file here*** */

   install_timer();

   install_mouse(); /* returns -1 on failure, otherwise number of buttons */

   install_keyboard();

   install_sound(DIGI_AUTODETECT,MIDI_NONE,"dummy");

  /* filter out allegro arguments */
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
 
   set_display_switch_mode(SWITCH_PAUSE); /* pause when in the background */
   text_mode(palette_color[7]);

   get_default_mode(drv, &algro_info);    /* determine default mode */
   add_video_mode(drv, &algro_info);      /* set the default mode */
   generate_video_table(drv);
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
 *
 * algro_flush --
 *
 *	Sync the ? server
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
algro_flush(Driver *drv)
{
  return;
}

/*----------------------------------------------------------------------
 *
 * clearAllegrowindow --
 *
 *	Clears Allegro window.
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
clearAllegrowindow(Driver *drv)
{
  DIALGRO(drv);
   clear_bitmap(di->bmp);
   scare_mouse();
   acquire_screen();
   blit(di->bmp,screen,0,0,0,0,di->winwidth,di->winheight);
   release_screen();
   unscare_mouse();
}

/*----------------------------------------------------------------------
 *
 * algro_resize --
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
algro_resize(Driver *drv)
{
  DIALGRO(drv);
  static int oldx = -1, oldy = -1, oldbpp = -1;

  if (oldx != videotable[adapter].xdots || oldy != videotable[adapter].ydots
      || oldbpp != videotable[adapter].truecolorbits) {

    oldx = videotable[adapter].xdots;
    oldy = videotable[adapter].ydots;
    colors = videotable[adapter].colors;
    if (colors > 256 || colors <= 0)
      colors = 256;
/*    videotable[adapter].dotmode = 19; */
    oldbpp = videotable[adapter].truecolorbits;

    di->winwidth = oldx;
    di->winheight = oldy;
    di->depth = oldbpp;

    screenaspect = oldy/(float) oldx;
    finalaspectratio = screenaspect;
    set_color_depth(oldbpp);
    if (di->bmp != NULL)
      destroy_bitmap(di->bmp);
    if (di->txt != NULL)
      destroy_bitmap(di->txt);
    if (di->stack_txt != NULL)
      destroy_bitmap(di->stack_txt);
    di->bmp = (struct BITMAP *)create_bitmap(oldx,oldy);
    di->txt = (struct BITMAP *)create_bitmap(oldx,oldy);
    di->stack_txt = (struct BITMAP *)create_bitmap(oldx,oldy);
    if (di->bmp == NULL || di->txt == NULL || di->stack_txt == NULL) {
      fprintf(stderr,"create_bitmap failed\n");
      algro_terminate(drv); /* FIXME we don't need to bailout here. JCO */
      exit(-1);
    }

    if (di->fullscreen)
       di->gfx_mode = GFX_AUTODETECT_FULLSCREEN;

    if (set_gfx_mode(di->gfx_mode,oldx,oldy,0,0) < 0) {
      return -1;
    }

    clearAllegrowindow(drv);
    return 1;
  } else {
/*    clearAllegrowindow(drv); JCO why were we doing this???? */
    return 0;
  }
}


/*
 *----------------------------------------------------------------------
 *
 * algro_read_palette --
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
algro_read_palette(Driver *drv)
{
  DIALGRO(drv);
  int i;

  if (gotrealdac == 0 && istruecolor)
    generate_332_palette(di->pal);
  else
    get_palette(di->pal);
  for (i = 0; i < colors; i++) {
    dacbox[i][0] = di->pal[i].r /* >>10 */;
    dacbox[i][1] = di->pal[i].g /* >>10 */;
    dacbox[i][2] = di->pal[i].b /* >>10 */;
  }
  return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * algro_write_palette --
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
algro_write_palette(Driver *drv)
{
  DIALGRO(drv);
  int i;

#if 0
  fprintf(stderr,"colors = %i\n",colors);
#endif
  for (i = 0; i < colors; i++) {
    di->pal[i].r = dacbox[i][0] /*<<10 */;
    di->pal[i].g = dacbox[i][1] /*<<10 */;
    di->pal[i].b = dacbox[i][2] /*<<10 */;
  }
  set_palette(di->pal);
  return 0;
}

#if 0  /* not used! JCO */
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
xcmapstuff(DriverAlgro *di)
{
  int ncells, i;

/*  if (di->onroot) {
    di->privatecolor = 0;
  }
*/
  for (i = 0; i < colors; i++) {
    di->pixtab[i] = i;
    di->ipixtab[i] = 999;
  }
  if (!gotrealdac) {
    di->Xcmap = DefaultColormapOfScreen(di->Xsc);
    if (di->fake_lut)
      algro_write_palette(&di->pub);
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
      fprintf(stderr,"Couldn't allocate any colors\n");
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
#endif

static int
algro_start_video(Driver *drv)
{
  DIALGRO(drv);
  scare_mouse();
  clear_to_color(di->bmp, palette_color[0]);
  clear_to_color(di->txt, palette_color[7]);  /* white */
  clear_to_color(di->stack_txt, palette_color[7]);  /* white */
  acquire_screen();
  blit(di->bmp,screen,0,0,0,0,di->winwidth,di->winheight);
  release_screen();
  unscare_mouse();
  return 0;
}

static int
algro_end_video(Driver *drv)
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
  ((DriverAlgro *) display)->doredraw = 1;
}

/*
 *----------------------------------------------------------------------
 *
 * algro_schedule_alarm --
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
algro_schedule_alarm(Driver *drv, int soon)
{
  DIALGRO(drv);

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
 * algro_write_pixel --
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
algro_write_pixel(Driver *drv, int x, int y, int color)
{
  DIALGRO(drv);
/*  static int from_y = 0; */
#ifdef DEBUG /* Debugging checks */
    if (color >= colors || color < 0) {
	fprintf(stderr,"Color %d too big %d\n", color, colors);
    }
    if (x >= sxdots || x < 0 || y >= sydots || y < 0) {
	fprintf(stderr,"Bad coord %d %d\n", x, y);
    }
#endif

    if (0) /* (di->depth != 8) */
        di->lastcolor = coloriter;
    else
        di->lastcolor = palette_color[color];

    if (di->fastmode == 1 && helpmode != HELPXHAIR) {
      if (!di->alarmon) {
         algro_schedule_alarm(drv, 0);
         putpixel(di->bmp, x, y, di->lastcolor);
      }
/* else {
         scare_mouse();
         acquire_screen();
         blit(di->bmp, screen, 0, from_y, 0, from_y, di->winwidth, y-from_y-1);
         release_screen();
         unscare_mouse();
         from_y = y;
      }
*/
    } else {
/*    putpixel(di->bmp, x, y, FAKE_LUT(di, di->pixtab[color])); */
/*    putpixel(screen, x, y, FAKE_LUT(di, di->pixtab[color])); */
    putpixel(di->bmp, x, y, di->lastcolor);
    scare_mouse();
    acquire_screen();
    putpixel(screen, x, y, di->lastcolor);
    release_screen();
    unscare_mouse();
    }
}

/*
 *----------------------------------------------------------------------
 *
 * algro_read_pixel --
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
algro_read_pixel(Driver *drv, int x, int y)
{
  DIALGRO(drv);
#if 0
  fprintf(stderr, "algro_read_pixel(%i,%i): %i\n", x, y, getpixel(di->bmp, x, y));
#endif
      return getpixel(screen, x, y);
}

/*
 *----------------------------------------------------------------------
 *
 * algro_write_span --
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
algro_write_span(Driver *drv, int y, int x, int lastx, BYTE *pixels)
{
  int width;
  int i;
/*  BYTE *pixline; */
  DIALGRO(drv);

#if 0
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
      algro_schedule_alarm(drv, 0);
    }
  } else {
    XPutImage(di->Xdp, di->Xw, di->Xgc, di->Ximage, x, y, x, y, width, 1);
/*    if (di->onroot) {
      XPutImage(di->Xdp, di->Xpixmap, di->Xgc, di->Ximage, x, y, x, y, width, 1);
    }
*/
  }
#else
  width = lastx-x+1;
  for (i=0;i < width;i++) {
    algro_write_pixel(drv, x+i, y, (int)pixels[i]);
  }
  if (di->fastmode == 1 && helpmode != HELPXHAIR) {
    if (!di->alarmon) {
      algro_schedule_alarm(drv, 0);
    }
  } else {
    scare_mouse();
    acquire_screen();
    blit(di->bmp, screen, x, y, x, y, width, 1);
    release_screen();
    unscare_mouse();
  }
#endif
}

/*
 *----------------------------------------------------------------------
 *
 * algro_read_span --
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
algro_read_span(Driver *drv, int y, int x, int lastx, BYTE *pixels)
{
  int i, width;
  width = lastx-x+1;
  for (i = 0; i < width; i++) {
    pixels[i] = (BYTE)algro_read_pixel(drv, x+i, y);
  }
}

/*
 *----------------------------------------------------------------------
 *
 * algro_set_line_mode --
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
algro_set_line_mode(Driver *drv, int mode)
{
  DIALGRO(drv);
  if (mode == 0) {
    xor_mode(FALSE);
  } else {
    xor_mode(TRUE);
    di->lastcolor = -1;
  }
}

/*
 *----------------------------------------------------------------------
 *
 * algro_draw_line --
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
algro_draw_line(Driver *drv, int x1, int y1, int x2, int y2)
{
  DIALGRO(drv);

  line(screen, x1, y1, x2, y2, di->lastcolor);
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
  int tmp = ch & 0xff;
#if 0
  fprintf(stderr, "translate_key(%i): ``%c''\n", ch, ch);
#endif

/* perhaps put key mapping in a text file.  JCO */      

  if (tmp >= 'a' && tmp <= 'z')
    return tmp;

#ifdef XLATE
  else {
    switch (tmp) {
    case 'I':		return INSERT;
    case 'D':		return DELETE;
    case 'U':		return PAGE_UP;
    case 'N':		return PAGE_DOWN;
    case CTL('O'):	return CTL_HOME;
#if 0
    case CTL('E'):	return CTL_END;
#endif
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
#if 0
    case '#':		return F3;
#endif
    case '$':		return F4;
    case '%':		return F5;
    case '^':		return F6;
    case '&':		return F7;
    case '*':		return F8;
    case '(':		return F9;
    case ')':		return F10;
    default:
      return tmp;
    }
  }

#else
/* This is the Allegro key mapping */
  else if (tmp == 0){
    switch (ch>>8) {
    case 0x2f:		return F1;
    case 0x30:		return F2;
    case 0x31:		return F3;
    case 0x32:		return F4;
    case 0x33:		return F5;
    case 0x34:		return F6;
    case 0x35:		return F7;
    case 0x36:		return F8;
    case 0x37:		return F9;
    case 0x38:		return F10;
    case 0x4c:		return INSERT;
    case 0x4d:		return DELETE;
    case 0x4e:		return HOME;
    case 0x4f:		return END;
    case 0x50:		return PAGE_UP;
    case 0x51:		return PAGE_DOWN;
    case 0x52:		return LEFT_ARROW;
    case 0x53:		return RIGHT_ARROW;
    case 0x54:		return UP_ARROW;
    case 0x55:		return DOWN_ARROW;
    case 0x59:		return ESC;
    case 0x5b:		return ENTER_2;
    case 0x67:		return ENTER;
    case -2:		return CTL_ENTER_2;
    default:
      return tmp;
    }
  }
  else if (tmp == 1){ /* shift key held down */
    switch (ch>>8) {
    case 0x2f:		return SF1;
    case 0x30:		return SF2;
    case 0x31:		return SF3;
    case 0x32:		return SF4;
    case 0x33:		return SF5;
    case 0x34:		return SF6;
    case 0x35:		return SF7;
    case 0x36:		return SF8;
    case 0x37:		return SF9;
    case 0x38:		return SF10;
    case 0x40:		return BACK_TAB;
    default:
      return tmp;
    }
  }  else if (tmp == 2){ /* control key held down */
    switch (ch>>8) {
    case 0x3d:		return CTL_MINUS;
    case 0x3e:		return CTL_PLUS;
    case 0x43:		return CTL_ENTER;
    case 0x4c:		return CTL_INSERT;
    case 0x4d:		return CTL_DEL;
    case 0x4e:		return CTL_HOME;
    case 0x4f:		return CTL_END;
    case 0x50:		return CTL_PAGE_UP;
    case 0x51:		return CTL_PAGE_DOWN;
    case 0x52:		return LEFT_ARROW_2;
    case 0x53:		return RIGHT_ARROW_2;
    case 0x54:		return UP_ARROW_2;
    case 0x55:		return DOWN_ARROW_2;
    default:
      return tmp;
    }
  }
  else
    return tmp;
#endif
}

#if 0
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
handle_esc(DriverAlgro *di)
{
  int ch1, ch2, ch3;

#ifdef __hpux
  /* HP escape key sequences. */
  ch1 = getachar();
  if (ch1 == -1) {
    rest(250); /* Wait 1/4 sec to see if a control sequence follows */
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
    rest(250); /* Wait 1/4 sec to see if a control sequence follows */
    ch1 = getachar();
  }
  if (ch1 == -1 || !isdigit(ch1))
    return ESC;
  ch2 = getachar();
  if (ch2 == -1) {
    rest(250); /* Wait 1/4 sec to see if a control sequence follows */
    ch2 = getachar();
  }
  if (ch2 == -1)
    return ESC;
  if (isdigit(ch2)) {
    ch3 = getachar();
    if (ch3 == -1) {
      rest(250); /* Wait 1/4 sec to see if a control sequence follows */
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
    rest(250); /* Wait 1/4 sec to see if a control sequence follows */
    ch1 = getachar(di);
  }
  if (ch1 != '[')		/* See if we have esc [ */
    return ESC;
  ch1 = getachar(di);
  if (ch1 == -1) {
    rest(250); /* Wait 1/4 sec to see if a control sequence follows */
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
    rest(250); /* Wait 1/4 sec to see if a control sequence follows */
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
      rest(250); /* Wait 1/4 sec to see if a control sequence follows */
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
#endif

static void
ev_expose(DriverAlgro *di)
{
  if (di->text_modep >= 1) {
    /* if text window, refresh text */
  } else {
    /* refresh graphics */
    int x, y;
/*    int w, h; */
    x = mouse_x;
    y = mouse_y;
#if 0
    w = xevent->width;
    h = xevent->height;
    if (x+w > sxdots) {
      w = sxdots-x;
    }
    if (y+h > sydots) {
      h = sydots-y;
    }
    if (x < sxdots && y < sydots && w > 0 && h > 0) {
      acquire_screen();
      blit(di->bmp,screen,x,y,x,y,xevent->width,xevent->height);
      release_screen();
    }
#endif
  }
}


/*
 *----------------------------------------------------------------------
 *
 * algro_redraw --
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
algro_redraw(Driver *drv)
{
  DIALGRO(drv);
  if (di->alarmon) {
    scare_mouse();
    acquire_screen();
    blit(di->bmp,screen,0,0,0,0,di->winwidth,di->winheight);
    release_screen();
    unscare_mouse();
    di->alarmon = 0;
  }
  di->doredraw = 0;
}

static void
ev_button_press(DriverAlgro *di)
{
  int done = 0;
  int banding = 0;
  int bandx0, bandy0, bandx1, bandy1;

  if (lookatmouse == 3 || zoomoff == 0) {
    di->last_x = mouse_x;
    di->last_y = mouse_y;
    return;
  }

  bandx1 = bandx0 = mouse_x;
  bandy1 = bandy0 = mouse_y;

  while (!done) {

    switch (mouse_b) {
    case 1:  /* left button is pressed */
      if (banding) {
         scare_mouse();
         acquire_screen();
         rect(screen, MIN(bandx0, bandx1), MIN(bandy0, bandy1),
                      MAX(bandx0, bandx1), MAX(bandy0, bandy1), 255);
         release_screen();
         unscare_mouse();
      }
      bandx1 = mouse_x;
      bandy1 = mouse_y;
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
	  drawing_mode(DRAW_MODE_XOR, screen, 0, 0);
	}
      }
      if (banding) {
         scare_mouse();
         acquire_screen();
         rect(screen, MIN(bandx0, bandx1), MIN(bandy0, bandy1),
                      MAX(bandx0, bandx1), MAX(bandy0, bandy1), 255);
         release_screen();
         unscare_mouse();
      }
      break;

    default:
      done = 1;
      break;
    }
  }

  if (!banding)
    return;

  scare_mouse();
  acquire_screen();
  rect(screen, MIN(bandx0, bandx1), MIN(bandy0, bandy1),
               MAX(bandx0, bandx1), MAX(bandy0, bandy1), 255);
  release_screen();
  unscare_mouse();
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
/*
  if (di->lastcolor != -1)
    XSetForeground(di->Xdp, di->Xgc, FAKE_LUT(di, di->lastcolor));
  XSetFunction(di->Xdp, di->Xgc, di->xlastfcn);
*/
  di->XZoomWaiting = 1;
  drawbox(0);

}

static void
ev_motion_notify(DriverAlgro *di)
{
  /* left button is bit 0
     right button is bit 1
     middle button is bit 2 */

  if (mouse_b & 4 ||
     (mouse_b & (1 | 2))) {
    di->button_num = 3;
  } else if (mouse_b & 1) {
    di->button_num = 1;
  } else if (mouse_b & 4) {
    di->button_num = 2;
  } else {
    di->button_num = 0;
  }

  if (lookatmouse == 3 && di->button_num != 0) {
    di->dx += (mouse_x-di->last_x)/MOUSE_SCALE;
    di->dy += (mouse_y-di->last_y)/MOUSE_SCALE;
    di->last_x = mouse_x;
    di->last_y = mouse_y;
  } else {
    Cursor_SetPos(mouse_x, mouse_y);
    di->xbufkey = ENTER;
  }
}

/*----------------------------------------------------------------------
 *
 * handle_events --
 *
 *	Handles events.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Does event action.
 *
 *----------------------------------------------------------------------
 */
static void
handle_events(Driver *drv, int events)
{
  DIALGRO(drv);
  if (events == 1 && !di->xbufkey)
    di->xbufkey = readkey();

  if (events == 2 || events == 4) {

    if (editpal_cursor && !inside_help) {
      ev_motion_notify(di);
    }

    ev_button_press(di);

    ev_expose(di); /* ???? what is this? JCO */
  }

    if (di->doredraw)
      algro_redraw(drv);

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

/* Check if there is a character or mouse movement/button waiting for us.  */
static int
input_pending(void)
{
  static int last_mouse_pos = 0;
  int result = 0;

  if (keyboard_needs_poll())
    poll_keyboard();
  if (keypressed()) /* a key was pressed */
    result = 1;
  if (mouse_needs_poll())
     poll_mouse();
  if (mouse_b != 0) /* a mouse button was pressed */
    result |= 2;
  if (last_mouse_pos != mouse_pos) { /* the mouse was moved */
    result |= 4;
    last_mouse_pos = mouse_pos;
  }

  return (result);
}

/*----------------------------------------------------------------------
 *
 * algro_get_key --
 *
 *	Get a key from the keyboard or .
 *	Blocks if block = 1.
 *
 * Results:
 *	Key, or 0 if no key and not blocking.
 *	Times out after .5 second.
 *
 * Side effects:
 *	Processes events.
 *
 *----------------------------------------------------------------------
 */
static int
algro_get_key(Driver *drv, int block)
{
/*  static int skipcount = 0; */
  static int mousevisible = 0;
  int got_one;
  DIALGRO(drv);

  while (1) {
    /* Don't check events every time, since that is expensive */
#if 0
    skipcount++;
    if (block == 0 && skipcount < 25)
      break;
    skipcount = 0;
#endif
    got_one = input_pending();
    if ((got_one) != 0) {
      handle_events(drv, got_one);
  
      if (di->xbufkey) {
        int ch = di->xbufkey;
        di->xbufkey = 0;
/*        skipcount = 9999; */ /* If we got a key, check right away next time */
        return translate_key(ch);
      }
    }

    if (calc_status == 1) {
      show_mouse(NULL);
      mousevisible = 0;
      break;
    }
    else {
      if (!mousevisible) {
        show_mouse(screen);
        mousevisible = 1;
      }
      yield_timeslice();
      break;
    }

    if (!block)
      break;

#if 0
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
#endif
  }

  return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * getfont --
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
algro_find_font(Driver *drv, int parm)
{
  DIALGRO(drv);
/* Allegro already has a font in the variable font */
  di->txt_ht = (text_height(font) / 2) - 1; /* text height in pixels -> bit shift */
  di->txt_wt = (text_length(font,"H") / 2) - 1; /* text width in pixels -> bit shift */
  return(NULL);
  
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
algro_shell(Driver *drv)
{
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
algro_put_string(Driver *drv, int row, int col, int attr, const char *msg)
{
  DIALGRO(drv);
  int r, c, i, k, s_r, s_c;
  int foregnd = attr & 15;
  int backgnd = (attr >> 4) & 15;
  int tmp_attr;
  int max_c = 0;

#if 0
  fprintf(stderr, "algro_put_string(%d,%d, %u): ``%s''\n", row, col, attr, msg);
#endif
  if (attr & BRIGHT && !(attr & INVERSE)) { /* bright */
    foregnd += 8;
  }
  if (attr & INVERSE) { /* inverse video */
    text_mode(palette_color[foregnd]);
    tmp_attr = backgnd;
  }
  else {
    text_mode(palette_color[backgnd]);
    tmp_attr = foregnd;
  }


  if (row != -1)
    textrow = row;
  if (col != -1)
    textcol = col;

  s_r = r = textrow + textrbase;
  s_c = c = textcol + textcbase;

  while (*msg) {
    if ('\n' == *msg) {
      textrow++;
      r++;
      if (c > max_c)
        max_c = c;
      textcol = 0;
      c = textcbase;
    } else {
#if 0
      if (c >= TEXT_WIDTH) c = TEXT_WIDTH - 1; /* keep going, but truncate */
      if (r >= TEXT_HEIGHT) r = TEXT_HEIGHT - 1;
#endif
      assert(r < TEXT_HEIGHT);
      assert(c < TEXT_WIDTH);
      di->text_screen[r][c] = *msg;
      di->text_attr[r][c] = attr;
      textout(di->txt,font,&di->text_screen[r][c],
              c<<di->txt_wt,r<<di->txt_ht,palette_color[tmp_attr]);
      textcol++;
      c++;
    }
    msg++;
  }

  if (c > max_c)
    max_c = c;

  i = s_r<<di->txt_ht; /* reuse i for blit */
  k = s_c<<di->txt_wt;
  if (r == 0)
     r = 1;
  if (max_c > TEXT_WIDTH)
    max_c = TEXT_WIDTH;
  c = max_c - s_c;     /* reuse c for blit, now it's max width of msg */
  if (r > TEXT_HEIGHT - s_r)
    r = TEXT_HEIGHT - s_r;
  scare_mouse();
  acquire_screen();
  blit(di->txt,screen,k,i,k,i,c<<di->txt_wt,r<<di->txt_ht);
  release_screen();
  unscare_mouse();
}

static void
algro_set_for_text(Driver *drv)
{
  /* map text screen child window */
  /* allocate text colors in window's colormap, save window's colors */
  /* refresh window with last contents of text_screen */
  set_palette(default_palette);  /* the default IBM BIOS palette */
                     /* the image's colormap is already saved in di->pal */

/*  fprintf(stderr, "algro_set_for_text\n"); */
}

static void
algro_set_for_graphics(Driver *drv)
{
  DIALGRO(drv);
  /* unmap text screen child window */
  /* restore colormap from saved copy */
  /* expose will be sent for newly visible area */
  set_palette(di->pal);  /* need to set in algro_unstack_screen() also */
  clearAllegrowindow(drv);

/*  fprintf(stderr, "algro_set_for_graphics\n"); */
}

static void
algro_set_clear(Driver *drv)
{
  DIALGRO(drv);
  erase_text_screen(di);
  algro_set_for_text(drv);
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
algro_set_video_mode(Driver *drv, int ax, int bx, int cx, int dx)
{
  DIALGRO(drv);
  if (diskflag)
    enddisk();
  algro_end_video(drv);

  set_color_depth(di->depth);

  if (algro_resize(drv) < 0) {
     set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
     allegro_message("Error setting graphics mode\n%s\n", allegro_error);
     goodmode = 0;
     algro_terminate(drv);
     exit (-1);
  } else {
     dotmode = 19;
  }

  goodmode = 1;
  switch (dotmode) {
  case 0:				/* text */
    algro_set_for_text(drv);
    break;

  case 19: /* Allegro window */
    dotwrite = driver_write_pixel;
    dotread = driver_read_pixel;
    lineread = driver_read_span;
    linewrite = driver_write_span;
    algro_start_video(drv);
    algro_set_for_graphics(drv);
    if (di->depth == 8) {
      gotrealdac = 1;
      istruecolor = 0;
      fake_lut = 0;
      di->fake_lut = 0;
    }
    else {
      gotrealdac = 0;
      istruecolor = 1;
      fake_lut = 1;
      di->fake_lut = 1;
    }
    break;

  default:
    fprintf(stderr,"Bad mode %d\n", dotmode);
    goodmode = 0;
    exit(-1);
  } 
  if (dotmode !=0) {
/*    algro_read_palette(drv); This breaks it! */
    andcolor = colors-1;
    boxcount =0;
  }
}

/*----------------------------------------------------------------------
 *
 * algro_window --
 *
 *	Make the window.
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
algro_window(Driver *drv)
{

  /* set the graphics mode */
  set_palette(default_palette);
  algro_write_palette(drv); /* so the first call to algro_read_palette can
                          return a palette if a map=fname.map isn't present*/
#if 0
  set_gfx_mode(GFX_SAFE, 320, 200, 0, 0);

  if (!gfx_mode_select_ex(&di->gfx_mode, &di->winwidth, &di->winheight,
                          &di->depth)) {
     algro_terminate(drv);
  }
#endif
  algro_set_video_mode(drv, 0, 0, 0, 0);

  return;
}

static void
algro_move_cursor(Driver *drv, int row, int col)
{
/*
  fprintf(stderr, "algro_move_cursor(%d,%d)\n", row, col);
*/

  if (row != -1)
    textrow = row;
  if (col != -1)
    textcol = col;
}

static void
algro_hide_text_cursor(Driver *drv)
{
  /* erase cursor if currently drawn */
  /* not used, we don't have a text cursor */
/*
  fprintf(stderr, "algro_hide_text_cursor\n");
*/
}

static void
algro_set_attr(Driver *drv, int row, int col, int attr, int count)
{
  DIALGRO(drv);
  int i = col;
  int k;
  int r, c, s_r, s_c;
  int s_count = count;
  int foregnd = attr & 15;
  int backgnd = (attr >> 4) & 15;
  int tmp_attr;

#if 0
  fprintf(stderr, "algro_set_attr(%d,%d, %d): %d\n", row, col, count, attr);
#endif
  if (attr & BRIGHT && !(attr & INVERSE)) { /* bright */
    foregnd += 8;
  }
  if (attr & INVERSE) { /* inverse video */
    text_mode(palette_color[foregnd]);
    tmp_attr = backgnd;
  }
  else {
    text_mode(palette_color[backgnd]);
    tmp_attr = foregnd;
  }

  if (row != -1)
    textrow = row;
  if (col != -1)
    textcol = col;
  s_r = r = textrow + textrbase;
  s_c = c = textcol + textcbase;

  assert(count <= TEXT_WIDTH * TEXT_HEIGHT);
  while (count) {
    assert(r < TEXT_HEIGHT);
    assert(i < TEXT_WIDTH);
    di->text_attr[r][i] = attr;
    textout(di->txt,font,&di->text_screen[r][i],
            i<<di->txt_wt,r<<di->txt_ht,palette_color[tmp_attr]);
    if (++i == TEXT_WIDTH) {
      i = 0;
      r++;
    }
    count--;
  }
  /* refresh text */
  if (r == 0)
    r = 1;
  if (r > TEXT_HEIGHT - s_r)
    r = TEXT_HEIGHT - s_r;
  if (s_count > TEXT_WIDTH - s_c)
    s_count = TEXT_WIDTH - s_c;
  i = s_r<<di->txt_ht; /* reuse i for blit, above i is col, now it's row */
  k = s_c<<di->txt_wt;
  scare_mouse();
  acquire_screen();
  blit(di->txt,screen,k,i,k,i,s_count<<di->txt_wt,r<<di->txt_ht);
  release_screen();
  unscare_mouse();
}

static void
algro_scroll_up(Driver *drv, int top, int bot)
{
  DIALGRO(drv);
  int r, c, i;
  char *tmp;
/* assume the attributes are the same for all rows and columns */
  int foregnd = di->text_attr[top][5] & 15; /* column 5 is arbitrary */
  int backgnd = (di->text_attr[top][5] >> 4) & 15;
  int tmp_attr;

  if (di->text_attr[row][col] & BRIGHT) { /* bright */
    foregnd += 8;
  }
  if (di->text_attr[top][5] & INVERSE) { /* inverse video */
    text_mode(palette_color[foregnd]);
    tmp_attr = backgnd;
  }
  else {
    text_mode(palette_color[backgnd]);
    tmp_attr = foregnd;
  }

  assert(bot <= TEXT_HEIGHT);
  for (r = top; r < bot; r++) {
    for (c = 0; c < TEXT_WIDTH; c++) {
      di->text_attr[r][c] = di->text_attr[r+1][c];
      di->text_screen[r][c] = di->text_screen[r+1][c];
    }
    tmp = &di->text_screen[r][0];
    textout(di->txt,font,tmp,0,r<<di->txt_ht,palette_color[tmp_attr]);
  }
  for (c = 0; c < TEXT_WIDTH; c++) {
    di->text_attr[bot][c] = 0;
    di->text_screen[bot][c] = ' ';
  }
  tmp = &di->text_screen[bot][0];
  textout(di->txt,font,tmp,0,r<<di->txt_ht,palette_color[tmp_attr]);
  i = top << di->txt_ht;
  scare_mouse();
  acquire_screen();
  blit(di->txt,screen,0,i,0,i,TEXT_WIDTH<<di->txt_wt,(bot-top)<<di->txt_ht);
  release_screen();
  unscare_mouse();
  /* draw text */
#ifdef DEBUG
  fprintf(stderr, "algro_scroll_up(%d, %d)\n", top, bot);
#endif
}

static void
algro_stack_screen(Driver *drv)
{
  DIALGRO(drv);
  int r, c;
#if 0
  fprintf(stderr, "algro_stack_screen, %i screens stacked\n", di->text_modep+1);
#endif
/* since we double buffer, the screen is in di->bmp */
/* no need to clear the screen, the text routines do it */
  if (di->text_modep > 0) {
    for (r = 0; r < TEXT_HEIGHT; r++)
      for (c = 0; c < TEXT_WIDTH; c++) {
        di->stack_text_screen[r][c] = di->text_screen[r][c];
        di->stack_text_attr[r][c] = di->text_attr[r][c];
      }
    blit(di->txt,di->stack_txt,0,0,0,0,TEXT_WIDTH<<di->txt_wt,
         TEXT_HEIGHT<<di->txt_ht);
  }
  di->text_modep++;
}

static void
algro_unstack_screen(Driver *drv)
{
  DIALGRO(drv);
  int r, c;
#if 0
  fprintf(stderr, "algro_unstack_screen, %i screens stacked\n", di->text_modep);
#endif
  if (di->text_modep > 1) {
    erase_text_screen(di);
    set_palette(default_palette);
    for (r = 0; r < TEXT_HEIGHT; r++)
      for (c = 0; c < TEXT_WIDTH; c++) {
        di->text_screen[r][c] = di->stack_text_screen[r][c];
        di->text_attr[r][c] = di->stack_text_attr[r][c];
      }
    blit(di->stack_txt,di->txt,0,0,0,0,TEXT_WIDTH<<di->txt_wt,
         TEXT_HEIGHT<<di->txt_ht);
    scare_mouse();
    acquire_screen();
    blit(di->txt,screen,0,0,0,0,TEXT_WIDTH<<di->txt_wt,
         TEXT_HEIGHT<<di->txt_ht);
    release_screen();
    unscare_mouse();
  }
  else {
    set_palette(di->pal);
    scare_mouse();
    acquire_screen();
    blit(di->bmp,screen,0,0,0,0,di->winwidth,di->winheight);
    release_screen();
    unscare_mouse();
  }
  di->text_modep--;
}

static void
algro_discard_screen(Driver *drv)
{
  DIALGRO(drv);
/*  fprintf(stderr, "algro_discard_screen\n"); */
  clearAllegrowindow(drv);
  di->text_modep = 0;
}

static int
algro_init_fm(Driver *drv)
{
  return 0;
}

static void
algro_delay(Driver *drv, long time)
{
  rest(time); /* this must be used because of the Allegro timers */
}

static void
algro_buzzer(Driver *drv, int kind)
{
  if ((soundflag & 7) != 0) {
    printf("\007");
    fflush(stdout);
  }
  if (kind==0) {
    algro_redraw(drv);
  }
}

static int
algro_sound_on(Driver *drv, int freq)
{
  fprintf(stderr, "algro_sound_on(%d)\n", freq);
  return (0);
}

static void
algro_sound_off(Driver *drv)
{
  fprintf(stderr, "algro_sound_off\n");
}

static int
algro_diskp(Driver *drv)
{
  return 0;
}

/*
 * place this last in the file to avoid having to forward declare routines
 */
static DriverAlgro algro_driver_info = {
  STD_DRIVER_STRUCT(algro),
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
  NULL,                                 /* bmp */
  NULL,                                 /* txt */
  NULL,                                 /* stack_txt */
  0,					/* depth */
  NULL,					/* Xdata */
  0,					/* Xdscreen */
  GFX_AUTODETECT_WINDOWED,           /* gfx_mode */
  DEFX, DEFY,				/* winwidth, winheight */
  -1,					/* lastcolor */
  NULL,					/* pixbuf */
  { {0}, {0}, {0}, {0} },				/* cols */
  { {0}, {0}, {0} },                                /* pal */
  0,					/* XZoomWaiting */
  FONT,					/* x_font_name */
  0,					/* xbufkey */
  NULL,					/* fontPtr */
  3,                                    /* txt_ht */
  3,                                    /* txt_wt */
  { {0} },				/* text_screen */
  { {0} },				/* text_attr */
  { {0} },				/* stack_text_screen */
  { {0} },				/* stack_text_attr */
  NULL,					/* font_table */
  0,    				/* text_modep */
  0,					/* ctl_mode */
  0,					/* shift_mode */
  0,					/* button_num */
  0, 0,					/* last_x, last_y */
  0, 0					/* dx, dy */
};

Driver *algro_driver = &algro_driver_info.pub;
