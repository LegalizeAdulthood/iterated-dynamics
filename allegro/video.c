#include <string.h>
#include "port.h"
#include "prototyp.h"

/*
 * This file contains Unix versions of the routines in video.asm
 * Copyright 1992 Ken Shirriff
 */

extern int waitkeypressed (int);

int fake_lut = 0;
int istruecolor = 0;
int daclearn = 0;
int dacnorm = 0;
int daccount = 0;
int ShadowColors;
int goodmode = 0;               /* if non-zero, OK to read/write pixels */
void (*dotwrite) (int, int, int);
                                /* write-a-dot routine */
int (*dotread) (int, int);      /* read-a-dot routine */
void (*linewrite) ();           /* write-a-line routine */
void (*lineread) ();            /* read-a-line routine */
int andcolor = 0;               /* "and" value used for color selection */
int diskflag = 0;               /* disk video active flag */

void (*swapsetup) (void) = NULL;        /* setfortext/graphics setup routine */
int color_dark = 0;             /* darkest color in palette */
int color_bright = 0;           /* brightest color in palette */
int color_medium = 0;           /* nearest to medbright grey in palette
                                   Zoom-Box values (2K x 2K screens max) */
int boxcolor = 0;               /* Zoom-Box color */
int reallyega = 0;              /* 1 if its an EGA posing as a VGA */
int gotrealdac = 0;             /* 1 if loaddac has a dacbox */
int rowcount = 0;               /* row-counter for decoder and out_line */
int video_type = 0;             /* actual video adapter type:
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
int svga_type = 0;              /*  (forced) SVGA type
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
int mode7text = 0;              /* nonzero for egamono and hgc */
int textaddr = 0xb800;          /* b800 for mode 3, b000 for mode 7 */
int textsafe = 0;               /* 0 = default, runup chgs to 1
                                   1 = yes
                                   2 = no, use 640x200
                                   3 = bios, yes plus use int 10h-1Ch
                                   4 = save, save entire image */
int text_type = 1;              /* current mode's type of text:
                                   0  = real text, mode 3 (or 7)
                                   1  = 640x200x2, mode 6
                                   2  = some other mode, graphics */
int textrow = 0;                /* for driver_put_string(-1,...) */
int textcol = 0;                /* for driver_put_string(..,-1,...) */
int textrbase = 0;              /* textrow is relative to this */
int textcbase = 0;              /* textcol is relative to this */

int vesa_detect = 1;            /* set to 0 to disable VESA-detection */
int virtual = 0;                /* no virtual screens, it's a DOS thing */
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

VIDEOINFO videotable[MAXVIDEOTABLE];
/*
 = {
  {"unused  mode             ","                         ",
   0, 0,	 0,   0,   0,	0, 0, 0,  0, 0 },
};
*/


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
setnullvideo ()
{
  dotwrite = nullwrite;
  dotread = nullread;
}

void normalineread ();
void normaline ();

void
loaddac ()
{
  driver_read_palette ();
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
; **************** Function keycursor(row, col)  **********************

;       Subroutine to wait cx ticks, or till keystroke pending
*/
int
keycursor (row, col)
{
  driver_move_cursor (row, col);
  driver_flush ();
  waitkeypressed (0);
  return getakey ();
}

/*
; **************** Function home()  ********************************

;       Home the cursor (called before printfs)
*/
void
home (void)
{
  textrow = 0;
  textcol = 0;
  driver_move_cursor (0, 0);
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
  driver_write_palette ();
/*    driver_delay(colors-daccount-1);  This kills us when starting new images w/ bpp>8.  JCO */
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
dispbox ()
{
  if (boxcount)
    {
      driver_set_line_mode (1);
      driver_draw_line (boxx[0], boxy[0], boxx[1], boxy[1]);
      driver_draw_line (boxx[1], boxy[1], boxx[2], boxy[2]);
      driver_draw_line (boxx[2], boxy[2], boxx[3], boxy[3]);
      driver_draw_line (boxx[3], boxy[3], boxx[0], boxy[0]);
      driver_set_line_mode (0);
      driver_flush ();
    }
  vesa_xres = sxdots;
  vesa_yres = sydots;
}

void
clearbox ()
{
  dispbox ();
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
SetupShadowVideo ()
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

void
swapnormread ()
{
}
void
swapnormwrite ()
{
}
