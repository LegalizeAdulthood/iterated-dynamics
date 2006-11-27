#include <curses.h>
#include <string.h>
#include "port.h"
#include "prototyp.h"
/*
 * This file contains Unix versions of the routines in video.asm
 * Copyright 1992 Ken Shirriff
 */

extern unsigned char *xgetfont (void);
extern int startdisk (void);
extern int waitkeypressed (int);

WINDOW *curwin;

int fake_lut = 0;
int istruecolor = 0;
int daclearn = 0;
int dacnorm = 0;
int daccount = 0;
int ShadowColors;
int goodmode = 0;		/* if non-zero, OK to read/write pixels */
void (*dotwrite) (int, int, int);
				/* write-a-dot routine */
int (*dotread) (int, int);	/* read-a-dot routine */
void (*linewrite) ();		/* write-a-line routine */
void (*lineread) ();		/* read-a-line routine */
int andcolor = 0;		/* "and" value used for color selection */
int diskflag = 0;		/* disk video active flag */

int videoflag = 0;		/* special "your-own-video" flag */

void (*swapsetup) (void) = NULL;	/* setfortext/graphics setup routine */
int color_dark = 0;		/* darkest color in palette */
int color_bright = 0;		/* brightest color in palette */
int color_medium = 0;		/* nearest to medbright grey in palette
				   Zoom-Box values (2K x 2K screens max) */
int boxcolor = 0;		/* Zoom-Box color */
int reallyega = 0;		/* 1 if its an EGA posing as a VGA */
int gotrealdac = 0;		/* 1 if loaddac has a dacbox */
int rowcount = 0;		/* row-counter for decoder and out_line */
int video_type = 0;		/* actual video adapter type:
				   0  = type not yet determined
				   1  = Hercules
				   2  = CGA (assumed if nothing else)
				   3  = EGA
				   4  = MCGA
				   5  = VGA
				   6  = VESA (not yet checked)
				   11  = 8514/A (not yet checked)
				   12  = TIGA   (not yet checked)
				   13  = TARGA  (not yet checked)
				   100 = x monochrome
				   101 = x 256 colors
				 */
int svga_type = 0;		/*  (forced) SVGA type
				   1 = ahead "A" type
				   2 = ATI
				   3 = C&T
				   4 = Everex
				   5 = Genoa
				   6 = Ncr
				   7 = Oak-Tech
				   8 = Paradise
				   9 = Trident
				   10 = Tseng 3000
				   11 = Tseng 4000
				   12 = Video-7
				   13 = ahead "B" type
				   14 = "null" type (for testing only) */
int mode7text = 0;		/* nonzero for egamono and hgc */
int textaddr = 0xb800;		/* b800 for mode 3, b000 for mode 7 */
int textsafe = 0;		/* 0 = default, runup chgs to 1
				   1 = yes
				   2 = no, use 640x200
				   3 = bios, yes plus use int 10h-1Ch
				   4 = save, save entire image */
int text_type = 1;		/* current mode's type of text:
				   0  = real text, mode 3 (or 7)
				   1  = 640x200x2, mode 6
				   2  = some other mode, graphics */
int textrow = 0;		/* for putstring(-1,...) */
int textcol = 0;		/* for putstring(..,-1,...) */
int textrbase = 0;		/* textrow is relative to this */
int textcbase = 0;		/* textcol is relative to this */

int vesa_detect = 1;		/* set to 0 to disable VESA-detection */
int virtual_screens = 0;                /* no virtual screens, it's a DOS thing */
int video_scroll = 0;
int video_startx = 0;
int video_starty = 0;
int vesa_xres;
int vesa_yres;
int chkd_vvs = 0;
int video_vram = 0;

/*

;		|--Adapter/Mode-Name------|-------Comments-----------|

;		|------INT 10H------|Dot-|--Resolution---|
;	    |key|--AX---BX---CX---DX|Mode|--X-|--Y-|Color|
*/

struct videoinfo videotable[MAXVIDEOTABLE] = {
  {"xfractint mode           ", "                         ",
   999, 0, 0, 0, 0, 19, 640, 480, 256},
  {"unused  mode             ", "                         ",
   0, 0, 0, 0, 0, 0, 0, 0, 0},
};

void setforgraphics ();

void
nullwrite (a, b, c)
     int a, b, c;
{
}

int
nullread (a, b)
     int a, b;
{
  return 0;
}

void
setnullvideo (void)
{
  dotwrite = nullwrite;
  dotread = nullread;
}

void normalineread ();
void normaline ();

void
putprompt (void)
{
  wclear (curwin);		/* ???? */
  putstring (0, 0, 0, "Press operation key, or <Esc> to return to Main Menu");
  wrefresh (curwin);
  return;
}

/*
; ********************** Function setvideotext() ************************

;       Sets video to text mode, using setvideomode to do the work.
*/
void
setvideotext (void)
{
  dotmode = 0;
  setvideomode (3, 0, 0, 0);
}

void
loaddac (void)
{
  readvideopalette ();
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
void
setvideomode (ax, bx, cx, dx)
     int ax, bx, cx, dx;
{
  if (diskflag)
    {
      enddisk ();
    }
  if (videoflag)
    {
      endvideo ();
      videoflag = 0;
    }
  goodmode = 1;
  switch (dotmode)
    {
    case 0:			/* text */
      clear ();
      /*
         touchwin(curwin);
       */
      wrefresh (curwin);
      break;
    case 11:
      startdisk ();
      dotwrite = writedisk;
      dotread = readdisk;
      lineread = normalineread;
      linewrite = normaline;
      break;
    case 19:			/* X window */
      putprompt ();
      dotwrite = writevideo;
      dotread = readvideo;
      lineread = readvideoline;
      linewrite = writevideoline;
      videoflag = 1;
      startvideo ();
      setforgraphics ();
      break;
    default:
      printf ("Bad mode %d\n", dotmode);
      exit (-1);
    }
  if (dotmode != 0)
    {
      loaddac ();
      andcolor = colors - 1;
      boxcount = 0;
    }
  vesa_xres = sxdots;
  vesa_yres = sydots;
}


/*
; **************** Function getcolor(xdot, ydot) *******************

;       Return the color on the screen at the (xdot,ydot) point
*/
int
getcolor (xdot, ydot)
     int xdot, ydot;
{
  int x1, y1;
  x1 = xdot + sxoffs;
  y1 = ydot + syoffs;
  if (x1 < 0 || y1 < 0 || x1 >= sxdots || y1 >= sydots)
    return 0;
  return dotread (x1, y1);
}

/*
; ************** Function putcolor_a(xdot, ydot, color) *******************

;       write the color on the screen at the (xdot,ydot) point
*/
void
putcolor_a (xdot, ydot, color)
     int xdot, ydot, color;
{
  dotwrite (xdot + sxoffs, ydot + syoffs, color & andcolor);
}

/*
; **************** Function movecursor(row, col)  **********************

;       Move the cursor (called before printfs)
*/
void
movecursor (row, col)
     int row, col;
{
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
  wmove (curwin, row, col);
}

/*
; **************** Function keycursor(row, col)  **********************

;       Subroutine to wait cx ticks, or till keystroke pending
*/
int
keycursor (row, col)
     int row, col;
{
  movecursor (row, col);
  wrefresh (curwin);
  waitkeypressed (0);
  return getakey ();
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
void
putstring (row, col, attr, msg)
     int row, col, attr;
     CHAR far *msg;
{
  int so = 0;

  if (row != -1)
    textrow = row;
  if (col != -1)
    textcol = col;

  if (attr & INVERSE || attr & BRIGHT)
    {
      wstandout (curwin);
      so = 1;
    }
  wmove (curwin, textrow + textrbase, textcol + textcbase);
  while (1)
    {
      if (*msg == '\0')
	break;
      if (*msg == '\n')
	{
	  textcol = 0;
	  textrow++;
	  wmove (curwin, textrow + textrbase, textcol + textcbase);
	}
      else
	{
	  char *ptr;
	  ptr = strchr (msg, '\n');
	  if (ptr == NULL)
	    {
	      waddstr (curwin, msg);
	      break;
	    }
	  else
	    {
	      waddch (curwin, *msg);
	    }
	}
      msg++;
    }
  if (so)
    {
      wstandend (curwin);
    }

  wrefresh (curwin);
  fflush (stdout);
  getyx (curwin, textrow, textcol);
  textrow -= textrbase;
  textcol -= textcbase;
}

/*
; setattr(row, col, attr, count) where
;         row, col = row and column to start printing.
;         attr = color attribute.
;         count = number of characters to set
;         This routine works only in real color text mode.
*/
void
setattr (row, col, attr, count)
     int row, col, attr, count;
{
  movecursor (row, col);
}

/*
; **************** Function home()  ********************************

;       Home the cursor (called before printfs)
*/
void
home (void)
{
  wmove (curwin, 0, 0);
  textrow = 0;
  textcol = 0;
}


/*
; ************* Function scrollup(toprow, botrow) ******************

;       Scroll the screen up (from toprow to botrow)
*/
void
scrollup (top, bot)
     int top, bot;
{
  wmove (curwin, top, 0);
  wdeleteln (curwin);
  wmove (curwin, bot, 0);
  winsertln (curwin);
  wrefresh (curwin);
}

/*
; *************** Function spindac(direction, rstep) ********************

;       Rotate the MCGA/VGA DAC in the (plus or minus) "direction"
;       in "rstep" increments - or, if "direction" is 0, just replace it.
*/
void
spindac (dir, inc)
     int dir, inc;
{
  int i, top;
  unsigned char tmp[3];
  unsigned char *dacbot;
  int len;
  if (colors < 16)
    return;
  if (istruecolor && truemode)
    return;
  if (dir != 0 && rotate_lo < colors && rotate_lo < rotate_hi)
    {
      top = rotate_hi > colors ? colors - 1 : rotate_hi;
      dacbot = (unsigned char *) dacbox + 3 * rotate_lo;
      len = (top - rotate_lo) * 3 * sizeof (unsigned char);
      if (dir > 0)
	{
	  for (i = 0; i < inc; i++)
	    {
	      bcopy (dacbot, tmp, 3 * sizeof (unsigned char));
	      bcopy (dacbot + 3 * sizeof (unsigned char), dacbot, len);
	      bcopy (tmp, dacbot + len, 3 * sizeof (unsigned char));
	    }
	}
      else
	{
	  for (i = 0; i < inc; i++)
	    {
	      bcopy (dacbot + len, tmp, 3 * sizeof (unsigned char));
	      bcopy (dacbot, dacbot + 3 * sizeof (unsigned char), len);
	      bcopy (tmp, dacbot, 3 * sizeof (unsigned char));
	    }
	}
    }
  writevideopalette ();
  delay (colors - daccount - 1);
}

/*
; ---- Help (Video) Support
; ********* Functions setfortext() and setforgraphics() ************

;       setfortext() resets the video for text mode and saves graphics data
;       setforgraphics() restores the graphics mode and data
;       setclear() clears the screen after setfortext()
*/
void
setfortext (void)
{
}

void
setclear (void)
{
  wclear (curwin);
  wrefresh (curwin);
}

void
setforgraphics ()
{
  startvideo ();
  spindac (0, 1);
}

unsigned char *fontTab = NULL;

/*
; ************** Function findfont(n) ******************************

;       findfont(0) returns far pointer to 8x8 font table if it can
;                   find it, NULL otherwise;
;                   nonzero parameter reserved for future use
*/
BYTE *
findfont (fontparm)
     int fontparm;
{
  if (fontTab == NULL)
    {
      fontTab = xgetfont ();
    }
  return (BYTE *) fontTab;
}

/*
; ******************** Zoombox functions **************************
*/

/*
 * The IBM method is that boxx[],boxy[] is a set of locations, and boxvalues
 * is the values in these locations.
 * Instead of using this box save/restore technique, we'll put the corners
 * in boxx[0],boxy[0],1,2,3 and then use xor.
 */

void
dispbox (void)
{
  if (boxcount)
    {
      setlinemode (1);
      drawline (boxx[0], boxy[0], boxx[1], boxy[1]);
      drawline (boxx[1], boxy[1], boxx[2], boxy[2]);
      drawline (boxx[2], boxy[2], boxx[3], boxy[3]);
      drawline (boxx[3], boxy[3], boxx[0], boxy[0]);
      setlinemode (0);
      xsync ();
    }
}

void
clearbox (void)
{
  dispbox ();
}

int
CheckForTPlus (void)
{
  return 0;
}

/*
; Passing this routine 0 turns off shadow, nonzero turns it on.
*/
int
ShadowVideo (on)
     int on;
{
  return 0;
}

int
SetupShadowVideo (void)
{
  return 0;
}

/*
; adapter_detect:
;       This routine performs a few quick checks on the type of
;       video adapter installed.
;       It sets variables video_type and textsafe,
;       and fills in a few bank-switching routines.
*/
int done_detect = 0;

void
adapter_detect (void)
{
  if (done_detect)
    return;
  done_detect = 1;
  textsafe = 2;
  if (colors == 2)
    {
      video_type = 100;
    }
  else
    {
      video_type = 101;
    }
}

/*
; **************** internal Read/Write-a-line routines *********************
;
;       These routines are called by out_line(), put_line() and get_line().
*/

void
normaline (y, x, lastx, pixels)
     int x, y, lastx;
     BYTE *pixels;
{
  int i, width;
  width = lastx - x + 1;
  for (i = 0; i < width; i++)
    {
      dotwrite (x + i, y, pixels[i]);
    }
}

void
normalineread (y, x, lastx, pixels)
     int x, y, lastx;
     BYTE *pixels;
{
  int i, width;
  width = lastx - x + 1;
  for (i = 0; i < width; i++)
    {
      pixels[i] = dotread (x + i, y);
    }
}

/*
; *************** Function find_special_colors ********************

;       Find the darkest and brightest colors in palette, and a medium
;       color which is reasonably bright and reasonably grey.
*/
void
find_special_colors (void)
{
  int maxb = 0;
  int minb = 9999;
  int med = 0;
  int maxgun, mingun;
  int brt;
  int i;

  color_dark = 0;
  color_medium = 7;
  color_bright = 15;

  if (colors == 2)
    {
      color_medium = 1;
      color_bright = 1;
      return;
    }

  if (!(gotrealdac || fake_lut))
    return;

  for (i = 0; i < colors; i++)
    {
      brt = (int) dacbox[i][0] + (int) dacbox[i][1] + (int) dacbox[i][2];
      if (brt > maxb)
	{
	  maxb = brt;
	  color_bright = i;
	}
      if (brt < minb)
	{
	  minb = brt;
	  color_dark = i;
	}
      if (brt < 150 && brt > 80)
	{
	  maxgun = mingun = (int) dacbox[i][0];
	  if ((int) dacbox[i][1] > (int) dacbox[i][0])
	    {
	      maxgun = (int) dacbox[i][1];
	    }
	  else
	    {
	      mingun = (int) dacbox[i][1];
	    }
	  if ((int) dacbox[i][2] > maxgun)
	    {
	      maxgun = (int) dacbox[i][2];
	    }
	  if ((int) dacbox[i][2] < mingun)
	    {
	      mingun = (int) dacbox[i][2];
	    }
	  if (brt - (maxgun - mingun) / 2 > med)
	    {
	      color_medium = i;
	      med = brt - (maxgun - mingun) / 2;
	    }
	}
    }
}

/*
; *************** Functions get_a_char, put_a_char ********************

;       Get and put character and attribute at cursor
;       Hi nybble=character, low nybble attribute. Text mode only
*/
char
get_a_char (void)
{
  return (char) getakey ();
}

void
put_a_char (ch)
     int ch;
{
}

/*
; ***Function get_line(int row,int startcol,int stopcol, unsigned char *pixels)

;       This routine is a 'line' analog of 'getcolor()', and gets a segment
;       of a line from the screen and stores it in pixels[] at one byte per
;       pixel
;       Called by the GIF decoder
*/

void
get_line (row, startcol, stopcol, pixels)
     int row, startcol, stopcol;
     BYTE *pixels;
{
  if (startcol + sxoffs >= sxdots || row + syoffs >= sydots)
    return;
  lineread (row + syoffs, startcol + sxoffs, stopcol + sxoffs, pixels);
}

/*
; ***Function put_line(int row,int startcol,int stopcol, unsigned char *pixels)

;       This routine is a 'line' analog of 'putcolor()', and puts a segment
;       of a line from the screen and stores it in pixels[] at one byte per
;       pixel
;       Called by the GIF decoder
*/

void
put_line (row, startcol, stopcol, pixels)
     int row, startcol, stopcol;
     BYTE *pixels;
{
  if (startcol + sxoffs >= sxdots || row + syoffs > sydots)
    return;
  linewrite (row + syoffs, startcol + sxoffs, stopcol + sxoffs, pixels);
}

/*
; ***************Function out_line(pixels,linelen) *********************

;       This routine is a 'line' analog of 'putcolor()', and sends an
;       entire line of pixels to the screen (0 <= xdot < xdots) at a clip
;       Called by the GIF decoder
*/
int
out_line (pixels, linelen)
     BYTE *pixels;
     int linelen;
{
  if (rowcount + syoffs >= sydots)
    return 0;
  linewrite (rowcount + syoffs, sxoffs, linelen + sxoffs - 1, pixels);
  rowcount++;
  return 0;
}

/*
; far move routine for savegraphics/restoregraphics
*/
void
movewords (len, fromptr, toptr)
     int len;
     BYTE *fromptr, *toptr;
{
  bcopy (fromptr, toptr, len);
}

void
swapnormread (void)
{
}
void
swapnormwrite (void)
{
}

/*
 * Implement stack and unstack window functions by using multiple curses
 * windows.
 */
void
savecurses (ptr)
     WINDOW **ptr;
{
  ptr[0] = curwin;
  curwin = newwin (0, 0, 0, 0);
  touchwin (curwin);
  wrefresh (curwin);
}

void
restorecurses (ptr)
     WINDOW **ptr;
{
  delwin (curwin);
  curwin = ptr[0];
  touchwin (curwin);
  wrefresh (curwin);
}

/* Moved the following from realdos.c to more cleanly separate the XFRACT
 * code.  Now curses.h is only required in the unix directory. JCO 11/26/2005
 */

/*
 * The stackscreen()/unstackscreen() functions were originally
 * ported to Xfractint. However, since Xfractint uses a separate
 * text and graphics window, I don't see what these functions
 * are good for, so I have commented them out.
 *
 * To restore the Xfractint versions of these functions,
 * uncomment next.
 * These functions are useful for switching between different text screens.
 * For example, looking at a parameter entry using F2.
 */

#define USE_XFRACT_STACK_FUNCTIONS

static int screenctr = 0;

#define MAXSCREENS 3

static BYTE far *savescreen[MAXSCREENS];
static int saverc[MAXSCREENS+1];

void stackscreen()
{
#ifdef USE_XFRACT_STACK_FUNCTIONS
   int i;
   BYTE far *ptr;
   saverc[screenctr+1] = textrow*80 + textcol;
   if (++screenctr) { /* already have some stacked */
         static char far msg[]={"stackscreen overflow"};
      if ((i = screenctr - 1) >= MAXSCREENS) { /* bug, missing unstack? */
         stopmsg(1,msg);
         exit(1);
         }
      if ((ptr = (savescreen[i] = (BYTE far *)farmemalloc(sizeof(int *)))))
         savecurses((WINDOW **)ptr);
      else {
         stopmsg(1,msg);
         exit(1);
        }
      setclear();
      }
   else
      setfortext();
#endif
}

void unstackscreen()
{
#ifdef USE_XFRACT_STACK_FUNCTIONS
   BYTE far *ptr;
   textrow = saverc[screenctr] / 80;
   textcol = saverc[screenctr] % 80;
   if (--screenctr >= 0) { /* unstack */
      ptr = savescreen[screenctr];
      restorecurses((WINDOW **)ptr);
      farmemfree(ptr);
      }
   else
      setforgraphics();
   movecursor(-1,-1);
#endif
}

void discardscreen()
{
   if (--screenctr >= 0) { /* unstack */
      if (savescreen[screenctr]) {
#ifdef USE_XFRACT_STACK_FUNCTIONS
         farmemfree(savescreen[screenctr]);
#endif
      }
   }
   else
      discardgraphics();
}
