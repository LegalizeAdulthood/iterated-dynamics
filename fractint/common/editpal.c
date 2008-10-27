/*
 * editpal.c
 *
 * Edits VGA 256-color palettes.
 *
 * Key to initials:
 *
 *    EAN - Ethan Nagel [70022,2552]
 *
 *    JJB - Juan J. Buhler [jbuhler@gidef.edu.ar]
 *
 *    TIW - Tim Wegner
 *
 *    AMC - Andrew McCarthy [andrewmc@netsoc.ucd.ie]
 *
 * Revision History:
 *
 *   10-22-90 EAN     Initial release.
 *
 *   10-23-90 EAN     "Discovered" get_line/put_line functions, integrated
 *                      them in (instead of only getcolor/putcolor). Much
 *                      faster!
 *                    Redesigned color editors (now at top of palette) and
 *                      re-assigned some keys.
 *                    Added (A)uto option.
 *                    Fixed memory allocation problem.  Now uses shared
 *                      FRACTINT data area (strlocn).  Uses 6 bytes DS.
 *
 *   10-27-90 EAN     Added save to memory option - will save screen image to
 *                      memory, if enough mem avail.  (disk otherwise).
 *                    Added s(T)ripe mode - works like (S)hade except only
 *                      changes every n'th entry.
 *                    Added temporary palette save/restore.  (Can work like
 *                      an undo feature.)  Thanks to Pieter Branderhorst for
 *                      idea.
 *
 *   10-28-90 EAN     The (H)ide function now makes the palette invisible,
 *                      while allowing any other operations (except '\\' -
 *                      move/resize) to continue.
 *
 *   10-29-90 PB (in EAN's absence, <grin>)
 *                    Change 'c' to 'd' and 's' to '=' for below.
 *                    Add 'l' to load palette from .map, 's' to store .map.
 *                    Add 'c' to invoke color cycling.
 *                    Change cursor to use whatever colors it can from
 *                    the palette (not fixed 0 and 255).
 *                    Restore colors 0 and 255 to real values whenever
 *                    palette is not on display.
 *                    Rotate 255 colors instead of 254.
 *                    Reduce cursor blink rate.
 *
 *   11-15-90 EAN     Minor "bug" fixes.  Continuous rotation now at a fixed
 *                      rate - once every timer tick (18.2 sec);  Blanks out
 *                      color samples when rotating; Editors no longer rotate
 *                      with the colors in color rotation mode;  Eliminated
 *                      (Z)oom mode; other minor fixes.
 *
 *   01-05-91 PB      Add 'w' function to convert to greyscale.
 *
 *   01-16-91 PB      Change rotate function to use new cyclerange stuff.
 *
 *   01-29-91 EAN     Made all colors editable.  The two reserved colors are
 *                       X'ed out.  They can be edited but the color is not
 *                       visible.  (There is an X over the sample instead.)
 *                    Changed default reserved colors to 254 & 255.
 *                    Added 'v' command to set the reserved colors to those
 *                       under the editors.
 *                    Added 'o' command to set the rotate range to between
 *                      the two editors.
 *                    Modified 'w' function:
 *                      uses internal function to do conversion (not BIOS)
 *                      will convert only current color if in 'x' mode or
 *                        range between editors in 'y' mode or entire palette
 *                        if in "normal" mode.
 *
 *   02-08-91 EAN     Improved 16 color support.  In 16 color mode, colors
 *                      16-255 have a dot over them and are editable but not
 *                      visible (like the two reserved colors).
 *
 *   09-08-91 SWT     Added 'n' command to make a negative color palette:
 *                      will convert only current color if in 'x' mode or
 *                      range between editors in 'y' mode or entire palette
 *                      if in "normal" mode.
 *
 *   03-03-92 JJB     Added '!', '@' and '#' commands to swap RG, GB and
 *                      RB columns (sorry, I didn't find better keys)
 *
 *  10-03-92 TIW      Minor changes for Jiim support, primarily changing
 *                    variables from static to global.
 *
 *   2-11-93 EAN      Added full Undo ('U' key) and Redo ('E' key)
 *                      capability.  Works pretty much as you'd expect
 *                      it to.
 *
 *    3-6-93 EAN      Modified "freestyle" mode, written by RB, to integrate
 *                      into palette editor more completely and work with
 *                      undo logic.
 *                    Added status area under the "fractint" message to
 *                      display current state of editor.  A = Auto mode,
 *                      X, Y = exclusion modes, F = freesyle mode, T = stripe
 *                      mode is waiting for #.
 *
 *   03-21-97 AMC     Made '"' work the same as '@' and made 'œ' work like
 *                      '#' for those of us on this side of the Atlantic!
 *                    The original palette is now stored in the other slots
 *                      on startup, so you can 'undo all' if you haven't
 *                      overwritten them already. Undo could do this, but
 *                      this is handy.
 *   05-22-97 TIW     Replaced movedata with far_memcopy()
 */

#ifdef DEBUG_UNDO
#include "mdisp.h"   /* EAN 930211 *** Debug Only *** */
#endif

#include <string.h>

#ifndef USE_VARARGS
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#ifdef __TURBOC__
#   include <mem.h>   /* to get mem...() declarations */
#endif

  /* see Fractint.c for a description of the "include"  hierarchy */
#include "port.h"
#include "prototyp.h"

/*
 * misc. #defines
 */

#define FONT_DEPTH          8     /* font size */

#define CSIZE_MIN           8     /* csize cannot be smaller than this */

#define CURSOR_SIZE         5     /* length of one side of the x-hair cursor */

#ifndef XFRACT
#define CURSOR_BLINK_RATE   3     /* timer ticks between cursor blinks */
#else
#define CURSOR_BLINK_RATE   300   /* timer ticks between cursor blinks */
#endif

#define FAR_RESERVE     8192L     /* amount of far mem we will leave avail. */

#define MAX_WIDTH        1024     /* palette editor cannot be wider than this */

char scrnfile[] = "FRACTINT.$$1";  /* file where screen portion is */
                                   /* stored */
char undofile[] = "FRACTINT.$$2";  /* file where undo list is stored */
#define TITLE   "FRACTINT"

#define TITLE_LEN (8)


#define newx(size)     mem_alloc(size)
#define new(class)     (class *)(mem_alloc(sizeof(class)))
#define delete(block)  block=NULL  /* just for warning */

#ifdef XFRACT
int editpal_cursor = 0;
#endif


/*
 * Stuff from fractint
 */

#if 0
 /* declarations moved to PRORTOTYPE.H - this left for docs */
 BYTE dacbox[256][3];            /* DAC spindac() will use           */
 int         sxdots;             /* width of physical screen         */
 int         sydots;             /* depth of physical screen         */
 int         sxoffs;             /* start of logical screen          */
 int         syoffs;             /* start of logical screen          */
 int         lookatmouse;        /* mouse mode for getakey(), etc    */
 int         strlocn[];          /* 10K buffer to store classes in   */
 int         colors;             /* # colors avail.                  */
 int         color_bright;       /* brightest color in palette       */
 int         color_dark;         /* darkest color in palette         */
 int         color_medium;       /* nearest to medbright gray color  */
 int         rotate_lo, rotate_hi;
 int         debugflag;
#endif
int using_jiim = 0;

/*
 * basic data types
 */


typedef struct
   {
   BYTE red,
                 green,
                 blue;
   } PALENTRY;



/*
 * static data
 */


static BYTE far *font8x8 = NULL;
BYTE     *line_buff;   /* must be alloced!!! */
static BYTE       fg_color,
                          bg_color;
static BOOLEAN            reserve_colors;
static BOOLEAN            inverse;

static float    gamma_val = 1;


/*
 * Interface to FRACTINT's graphics stuff
 */


static void setpal(int pal, int r, int g, int b)
   {
   dacbox[pal][0] = (BYTE)r;
   dacbox[pal][1] = (BYTE)g;
   dacbox[pal][2] = (BYTE)b;
   spindac(0,1);
   }


static void setpalrange(int first, int how_many, PALENTRY *pal)
   {
   memmove(dacbox+first, pal, how_many*3);
   spindac(0,1);
   }


static void getpalrange(int first, int how_many, PALENTRY *pal)
   {
   memmove(pal, dacbox+first, how_many*3);
   }


static void rotatepal(PALENTRY *pal, int dir, int lo, int hi)
   {             /* rotate in either direction */
   PALENTRY hold;
   int      size;

   size  = 1 + (hi-lo);

   if ( dir > 0 )
      {
      while ( dir-- > 0 )
         {
         memmove(&hold, &pal[hi],  3);
         memmove(&pal[lo+1], &pal[lo], 3*(size-1));
         memmove(&pal[lo], &hold, 3);
         }
      }

   else if ( dir < 0 )
      {
      while ( dir++ < 0 )
         {
         memmove(&hold, &pal[lo], 3);
         memmove(&pal[lo], &pal[lo+1], 3*(size-1));
         memmove(&pal[hi], &hold,  3);
         }
      }
   }


static void clip_put_line(int row, int start, int stop, BYTE *pixels)
   {
   if ( row < 0 || row >= sydots || start > sxdots || stop < 0 )
      return ;

   if ( start < 0 )
      {
      pixels += -start;
      start = 0;
      }

   if ( stop >= sxdots )
      stop = sxdots - 1;

   if ( start > stop )
      return ;

   put_line(row, start, stop, pixels);
   }


static void clip_get_line(int row, int start, int stop, BYTE *pixels)
   {
   if ( row < 0 || row >= sydots || start > sxdots || stop < 0 )
      return ;

   if ( start < 0 )
      {
      pixels += -start;
      start = 0;
      }

   if ( stop >= sxdots )
      stop = sxdots - 1;

   if ( start > stop )
      return ;

   get_line(row, start, stop, pixels);
   }


void clip_putcolor(int x, int y, int color)
   {
   if ( x < 0 || y < 0 || x >= sxdots || y >= sydots )
      return ;

   putcolor(x, y, color);
   }


int clip_getcolor(int x, int y)
   {
   if ( x < 0 || y < 0 || x >= sxdots || y >= sydots )
      return (0);

   return ( getcolor(x, y) );
   }


static void hline(int x, int y, int width, int color)
   {
   memset(line_buff, color, width);
   clip_put_line(y, x, x+width-1, line_buff);
   }


static void vline(int x, int y, int depth, int color)
   {
   while (depth-- > 0)
      clip_putcolor(x, y++, color);
   }


void getrow(int x, int y, int width, char *buff)
   {
   clip_get_line(y, x, x+width-1, (BYTE *)buff);
   }


void putrow(int x, int y, int width, char *buff)
   {
   clip_put_line(y, x, x+width-1, (BYTE *)buff);
   }


static void vgetrow(int x, int y, int depth, char *buff)
   {
   while (depth-- > 0)
      *buff++ = (char)clip_getcolor(x, y++);
   }


static void vputrow(int x, int y, int depth, char *buff)
   {
   while (depth-- > 0)
      clip_putcolor(x, y++, (BYTE)(*buff++));
   }


static void fillrect(int x, int y, int width, int depth, int color)
   {
   while (depth-- > 0)
      hline(x, y++, width, color);
   }


static void rect(int x, int y, int width, int depth, int color)
   {
   hline(x, y, width, color);
   hline(x, y+depth-1, width, color);

   vline(x, y, depth, color);
   vline(x+width-1, y, depth, color);
   }


void displayc(int x, int y, int fg, int bg, int ch)
   {
   int                xc, yc;
   BYTE      t;
   BYTE far *ptr;

   if( font8x8 == NULL)
      if ( (font8x8 = findfont(0)) == NULL )
         return ;

   ptr = ((BYTE far *)font8x8) + ch*FONT_DEPTH;

   for (yc=0; yc<FONT_DEPTH; yc++, y++, ++ptr)
      {
      for (xc=0, t= *ptr; xc<8; xc++, t<<=1)
         line_buff[xc] = (BYTE)((t&0x80) ? fg : bg);
      putrow(x, y, 8, (char *)line_buff);
      }
   }


#ifndef USE_VARARGS
static void displayf(int x, int y, int fg, int bg, char *format, ...)
#else
static void displayf(va_alist)
va_dcl
#endif
   {
   char buff[81];
   int  ctr;

   va_list arg_list;

#ifndef USE_VARARGS
   va_start(arg_list, format);
#else
   int x,y,fg,bg;
   char *format;

   va_start(arg_list);
   x = va_arg(arg_list,int);
   y = va_arg(arg_list,int);
   fg = va_arg(arg_list,int);
   bg = va_arg(arg_list,int);
   format = va_arg(arg_list,char *);
#endif
   vsprintf(buff, format, arg_list);
   va_end(arg_list);

   for(ctr=0; buff[ctr]!='\0'; ctr++, x+=8)
      displayc(x, y, fg, bg, buff[ctr]);
   }


/*
 * create smooth shades between two colors
 */


static void mkpalrange(PALENTRY *p1, PALENTRY *p2, PALENTRY pal[], int num, int skip)
   {
   int    curr;
   double rm = (double)((int) p2->red   - (int) p1->red  ) / num,
          gm = (double)((int) p2->green - (int) p1->green) / num,
          bm = (double)((int) p2->blue  - (int) p1->blue ) / num;

   for (curr=0; curr<num; curr+=skip)
      {
      if (gamma_val == 1)
          {
          pal[curr].red   = (BYTE)((p1->red   == p2->red  ) ? p1->red   :
                  (int) p1->red   + (int) ( rm * curr ));
          pal[curr].green = (BYTE)((p1->green == p2->green) ? p1->green :
              (int) p1->green + (int) ( gm * curr ));
          pal[curr].blue  = (BYTE)((p1->blue  == p2->blue ) ? p1->blue  :
                  (int) p1->blue  + (int) ( bm * curr ));
          }
          else
          {
          pal[curr].red   = (BYTE)((p1->red   == p2->red  ) ? p1->red   :
                  (int) (p1->red   + pow(curr/(double)(num-1),gamma_val)*num*rm));
          pal[curr].green = (BYTE)((p1->green == p2->green) ? p1->green :
                  (int) (p1->green + pow(curr/(double)(num-1),gamma_val)*num*gm));
          pal[curr].blue  = (BYTE)((p1->blue  == p2->blue ) ? p1->blue  :
                  (int) (p1->blue  + pow(curr/(double)(num-1),gamma_val)*num*bm));
          }
      }
   }



/*  Swap RG GB & RB columns */

static void rotcolrg(PALENTRY pal[], int num)
   {
   int    curr;
   int    dummy;

    for (curr=0; curr<=num; curr++)
      {
      dummy = pal[curr].red;
      pal[curr].red = pal[curr].green;
      pal[curr].green = (BYTE)dummy;
      }
   }


static void rotcolgb(PALENTRY pal[], int num)
   {
   int    curr;
   int    dummy;

    for (curr=0; curr<=num; curr++)
      {
      dummy = pal[curr].green;
      pal[curr].green = pal[curr].blue;
      pal[curr].blue = (BYTE)dummy;
      }
   }

static void rotcolbr(PALENTRY pal[], int num)
   {
   int    curr;
   int    dummy;

    for (curr=0; curr<=num; curr++)
      {
      dummy = pal[curr].red;
      pal[curr].red = pal[curr].blue;
      pal[curr].blue = (BYTE)dummy;
      }
   }


/*
 * convert a range of colors to grey scale
 */


static void palrangetogrey(PALENTRY pal[], int first, int how_many)
   {
   PALENTRY      *curr;
   BYTE  val;


   for (curr = &pal[first]; how_many>0; how_many--, curr++)
      {
      val = (BYTE) ( ((int)curr->red*30 + (int)curr->green*59 + (int)curr->blue*11) / 100 );
      curr->red = curr->green = curr->blue = (BYTE)val;
      }
   }

/*
 * convert a range of colors to their inverse
 */


static void palrangetonegative(PALENTRY pal[], int first, int how_many)
   {
   PALENTRY      *curr;

   for (curr = &pal[first]; how_many>0; how_many--, curr++)
      {
      curr->red   = (BYTE)(63 - curr->red);
      curr->green = (BYTE)(63 - curr->green);
      curr->blue  = (BYTE)(63 - curr->blue);
      }
   }


/*
 * draw and horizontal/vertical dotted lines
 */


static void hdline(int x, int y, int width)
   {
   int ctr;
   BYTE *ptr;

   for (ctr=0, ptr=line_buff; ctr<width; ctr++, ptr++)
      *ptr = (BYTE)((ctr&2) ? bg_color : fg_color);

   putrow(x, y, width, (char *)line_buff);
   }


static void vdline(int x, int y, int depth)
   {
   int ctr;

   for (ctr=0; ctr<depth; ctr++, y++)
      clip_putcolor(x, y, (ctr&2) ? bg_color : fg_color);
   }


static void drect(int x, int y, int width, int depth)
   {
   hdline(x, y, width);
   hdline(x, y+depth-1, width);

   vdline(x, y, depth);
   vdline(x+width-1, y, depth);
   }


/*
 * A very simple memory "allocator".
 *
 * Each call to mem_alloc() returns size bytes from the array mem_block.
 *
 * Be sure to call mem_init() before using mem_alloc()!
 *
 */

static char     *mem_block;
static unsigned  mem_avail;


void mem_init(VOIDPTR block, unsigned size)
   {
   mem_block = (char *)block;
   mem_avail = size;
   }


VOIDPTR mem_alloc(unsigned size)
   {
   VOIDPTR block;

#ifndef XFRACT
   if (size & 1)
      ++size;   /* allocate even sizes */
#else
   size = (size+3)&~3; /* allocate word-aligned memory */
#endif

   if (mem_avail < size)   /* don't let this happen! */
      {
      static FCODE msg[] = "editpal.c: Out of memory!\n";

      stopmsg(0, msg);
      exit(1);
      }

   block = mem_block;
   mem_avail -= size;
   mem_block += size;

   return(block);
   }



/*
 * misc. routines
 *
 */


static BOOLEAN is_reserved(int color)
   {
   return ((BOOLEAN) ((reserve_colors && (color==(int)fg_color || color==(int)bg_color) ) ? TRUE : FALSE) );
   }



static BOOLEAN is_in_box(int x, int y, int bx, int by, int bw, int bd)
   {
   return ((BOOLEAN) ((x >= bx) && (y >= by) && (x < bx+bw) && (y < by+bd)) );
   }



static void draw_diamond(int x, int y, int color)
   {
   putcolor (x+2, y+0,    color);
   hline    (x+1, y+1, 3, color);
   hline    (x+0, y+2, 5, color);
   hline    (x+1, y+3, 3, color);
   putcolor (x+2, y+4,    color);
   }



/*
 * Class:     Cursor
 *
 * Purpose:   Draw the blinking cross-hair cursor.
 *
 * Note:      Only one Cursor can exist (referenced through the_cursor).
 *            IMPORTANT: Call Cursor_Construct before you use any other
 *            Cursor_ function!  Call Cursor_Destroy before exiting to
 *            deallocate memory.
 */

struct _Cursor
   {

   int     x, y;
   int     hidden;       /* >0 if mouse hidden */
   long    last_blink;
   BOOLEAN blink;
#if 0
   char    t[CURSOR_SIZE],        /* save line segments here */
           b[CURSOR_SIZE],
           l[CURSOR_SIZE],
           r[CURSOR_SIZE];
#endif
   char    t[CURSOR_SIZE];        /* save line segments here */
   char    b[CURSOR_SIZE];
   char    l[CURSOR_SIZE];
   char    r[CURSOR_SIZE];
   } ;

#define Cursor struct _Cursor

/* private: */

   static  void    Cursor__Draw      (void);
   static  void    Cursor__Save      (void);
   static  void    Cursor__Restore   (void);

/* public: */
#ifdef NOT_USED
   static  BOOLEAN Cursor_IsHidden  (void);
#endif



static Cursor *the_cursor = NULL;


BOOLEAN Cursor_Construct(void)
   {
   if (the_cursor != NULL)
      return(FALSE);

   the_cursor = new(Cursor);

   the_cursor->x          = sxdots/2;
   the_cursor->y          = sydots/2;
   the_cursor->hidden     = 1;
   the_cursor->blink      = FALSE;
   the_cursor->last_blink = 0;

   return (TRUE);
   }


void Cursor_Destroy(void)
   {
   if (the_cursor != NULL)
      delete(the_cursor);

   the_cursor = NULL;
   }



static void Cursor__Draw(void)
   {
   int color;

   find_special_colors();
   color = (the_cursor->blink) ? color_medium : color_dark;

   vline(the_cursor->x, the_cursor->y-CURSOR_SIZE-1, CURSOR_SIZE, color);
   vline(the_cursor->x, the_cursor->y+2,             CURSOR_SIZE, color);

   hline(the_cursor->x-CURSOR_SIZE-1, the_cursor->y, CURSOR_SIZE, color);
   hline(the_cursor->x+2,             the_cursor->y, CURSOR_SIZE, color);
   }


static void Cursor__Save(void)
   {
   vgetrow(the_cursor->x, the_cursor->y-CURSOR_SIZE-1, CURSOR_SIZE, the_cursor->t);
   vgetrow(the_cursor->x, the_cursor->y+2,             CURSOR_SIZE, the_cursor->b);

   getrow(the_cursor->x-CURSOR_SIZE-1, the_cursor->y,  CURSOR_SIZE, the_cursor->l);
   getrow(the_cursor->x+2,             the_cursor->y,  CURSOR_SIZE, the_cursor->r);
   }


static void Cursor__Restore(void)
   {
   vputrow(the_cursor->x, the_cursor->y-CURSOR_SIZE-1, CURSOR_SIZE, the_cursor->t);
   vputrow(the_cursor->x, the_cursor->y+2,             CURSOR_SIZE, the_cursor->b);

   putrow(the_cursor->x-CURSOR_SIZE-1, the_cursor->y,  CURSOR_SIZE, the_cursor->l);
   putrow(the_cursor->x+2,             the_cursor->y,  CURSOR_SIZE, the_cursor->r);
   }



void Cursor_SetPos(int x, int y)
   {
   if (!the_cursor->hidden)
      Cursor__Restore();

   the_cursor->x = x;
   the_cursor->y = y;

   if (!the_cursor->hidden)
      {
      Cursor__Save();
      Cursor__Draw();
      }
   }

#ifdef NOT_USED

static int Cursor_IsHidden(void)
   {
   return ( the_cursor->hidden );
   }


#endif


void Cursor_Move(int xoff, int yoff)
   {
   if ( !the_cursor->hidden )
      Cursor__Restore();

   the_cursor->x += xoff;
   the_cursor->y += yoff;

   if (the_cursor->x < 0)       the_cursor->x = 0;
   if (the_cursor->y < 0)       the_cursor->y = 0;
   if (the_cursor->x >= sxdots) the_cursor->x = sxdots-1;
   if (the_cursor->y >= sydots) the_cursor->y = sydots-1;

   if ( !the_cursor->hidden )
      {
      Cursor__Save();
      Cursor__Draw();
      }
   }


int Cursor_GetX(void)   { return(the_cursor->x); }

int Cursor_GetY(void)   { return(the_cursor->y); }


void Cursor_Hide(void)
   {
   if ( the_cursor->hidden++ == 0 )
      Cursor__Restore();
   }


void Cursor_Show(void)
   {
   if ( --the_cursor->hidden == 0)
      {
      Cursor__Save();
      Cursor__Draw();
      }
   }

#ifdef XFRACT
void Cursor_StartMouseTracking()
{
    editpal_cursor = 1;
}

void Cursor_EndMouseTracking()
{
    editpal_cursor = 0;
}
#endif

/* See if the cursor should blink yet, and blink it if so */
void Cursor_CheckBlink(void)
{
   long tick;
   tick = readticker();

   if ( (tick - the_cursor->last_blink) > CURSOR_BLINK_RATE )
      {
      the_cursor->blink = (BOOLEAN)((the_cursor->blink) ? FALSE : TRUE);
      the_cursor->last_blink = tick;
      if ( !the_cursor->hidden )
         Cursor__Draw();
      }
   else if ( tick < the_cursor->last_blink )
      the_cursor->last_blink = tick;
}

int Cursor_WaitKey(void)   /* blink cursor while waiting for a key */
   {

#ifndef XFRACT
   while ( !keypressed() ) {
       Cursor_CheckBlink();
   }
#else
   while ( !waitkeypressed(1) ) {
       Cursor_CheckBlink();
   }
#endif

   return( keypressed() );
   }



/*
 * Class:     MoveBox
 *
 * Purpose:   Handles the rectangular move/resize box.
 */

struct _MoveBox
   {
   int      x, y;
   int      base_width,
            base_depth;
   int      csize;
   BOOLEAN  moved;
   BOOLEAN  should_hide;
   char    *t, *b,
           *l, *r;
   } ;

#define MoveBox struct _MoveBox

/* private: */

   static void     MoveBox__Draw     (MoveBox *this);
   static void     MoveBox__Erase    (MoveBox *this);
   static void     MoveBox__Move     (MoveBox *this, int key);

/* public: */

   static MoveBox *MoveBox_Construct  (int x, int y, int csize, int base_width,
                                      int base_depth);
   static void     MoveBox_Destroy    (MoveBox *this);
   static BOOLEAN  MoveBox_Process    (MoveBox *this); /* returns FALSE if ESCAPED */
   static BOOLEAN  MoveBox_Moved      (MoveBox *this);
   static BOOLEAN  MoveBox_ShouldHide (MoveBox *this);
   static int      MoveBox_X          (MoveBox *this);
   static int      MoveBox_Y          (MoveBox *this);
   static int      MoveBox_CSize      (MoveBox *this);

   static void     MoveBox_SetPos     (MoveBox *this, int x, int y);
   static void     MoveBox_SetCSize   (MoveBox *this, int csize);



static MoveBox *MoveBox_Construct(int x, int y, int csize, int base_width, int base_depth)
   {
   MoveBox *this = new(MoveBox);

   this->x           = x;
   this->y           = y;
   this->csize       = csize;
   this->base_width  = base_width;
   this->base_depth  = base_depth;
   this->moved       = FALSE;
   this->should_hide = FALSE;
   this->t           = newx(sxdots);
   this->b           = newx(sxdots);
   this->l           = newx(sydots);
   this->r           = newx(sydots);

   return(this);
   }


static void MoveBox_Destroy(MoveBox *this)
   {
   delete(this->t);
   delete(this->b);
   delete(this->l);
   delete(this->r);
   delete(this);
   }


static BOOLEAN MoveBox_Moved(MoveBox *this) { return(this->moved); }

static BOOLEAN MoveBox_ShouldHide(MoveBox *this) { return(this->should_hide); }

static int MoveBox_X(MoveBox *this)      { return(this->x); }

static int MoveBox_Y(MoveBox *this)      { return(this->y); }

static int MoveBox_CSize(MoveBox *this)  { return(this->csize); }


static void MoveBox_SetPos(MoveBox *this, int x, int y)
   {
   this->x = x;
   this->y = y;
   }


static void MoveBox_SetCSize(MoveBox *this, int csize)
   {
   this->csize = csize;
   }


static void MoveBox__Draw(MoveBox *this)  /* private */
   {
   int width = this->base_width + this->csize*16+1,
       depth = this->base_depth + this->csize*16+1;
   int x     = this->x,
       y     = this->y;


   getrow (x, y,         width, this->t);
   getrow (x, y+depth-1, width, this->b);

   vgetrow(x,         y, depth, this->l);
   vgetrow(x+width-1, y, depth, this->r);

   hdline(x, y,         width);
   hdline(x, y+depth-1, width);

   vdline(x,         y, depth);
   vdline(x+width-1, y, depth);
   }


static void MoveBox__Erase(MoveBox *this)   /* private */
   {
   int width = this->base_width + this->csize*16+1,
       depth = this->base_depth + this->csize*16+1;

   vputrow(this->x,         this->y, depth, this->l);
   vputrow(this->x+width-1, this->y, depth, this->r);

   putrow(this->x, this->y,         width, this->t);
   putrow(this->x, this->y+depth-1, width, this->b);
   }


#define BOX_INC     1
#define CSIZE_INC   2

static void MoveBox__Move(MoveBox *this, int key)
   {
   BOOLEAN done  = FALSE;
   BOOLEAN first = TRUE;
   int     xoff  = 0,
           yoff  = 0;

   while ( !done )
      {
      switch(key)
         {
         case RIGHT_ARROW_2:     xoff += BOX_INC*4;   break;
         case RIGHT_ARROW:       xoff += BOX_INC;     break;
         case LEFT_ARROW_2:      xoff -= BOX_INC*4;   break;
         case LEFT_ARROW:        xoff -= BOX_INC;     break;
         case DOWN_ARROW_2:      yoff += BOX_INC*4;   break;
         case DOWN_ARROW:        yoff += BOX_INC;     break;
         case UP_ARROW_2:        yoff -= BOX_INC*4;   break;
         case UP_ARROW:          yoff -= BOX_INC;     break;

         default:
            done = TRUE;
         }

      if (!done)
         {
         if (!first)
            getakey();       /* delete key from buffer */
         else
            first = FALSE;
         key = keypressed();   /* peek at the next one... */
         }
      }

   xoff += this->x;
   yoff += this->y;   /* (xoff,yoff) = new position */

   if (xoff < 0) xoff = 0;
   if (yoff < 0) yoff = 0;

   if (xoff+this->base_width+this->csize*16+1 > sxdots)
       xoff = sxdots - (this->base_width+this->csize*16+1);

   if (yoff+this->base_depth+this->csize*16+1 > sydots)
      yoff = sydots - (this->base_depth+this->csize*16+1);

   if ( xoff!=this->x || yoff!=this->y )
      {
      MoveBox__Erase(this);
      this->y = yoff;
      this->x = xoff;
      MoveBox__Draw(this);
      }
   }


static BOOLEAN MoveBox_Process(MoveBox *this)
   {
   int     key;
   int     orig_x     = this->x,
           orig_y     = this->y,
           orig_csize = this->csize;

   MoveBox__Draw(this);

#ifdef XFRACT
   Cursor_StartMouseTracking();
#endif
   for(;;)
      {
      Cursor_WaitKey();
      key = getakey();

      if (key==ENTER || key==ENTER_2 || key==ESC || key=='H' || key=='h')
         {
         if (this->x != orig_x || this->y != orig_y || this->csize != orig_csize)
            this->moved = TRUE;
         else
           this->moved = FALSE;
         break;
         }

      switch(key)
         {
         case UP_ARROW:
         case DOWN_ARROW:
         case LEFT_ARROW:
         case RIGHT_ARROW:
         case UP_ARROW_2:
         case DOWN_ARROW_2:
         case LEFT_ARROW_2:
         case RIGHT_ARROW_2:
            MoveBox__Move(this, key);
            break;

         case PAGE_UP:   /* shrink */
            if (this->csize > CSIZE_MIN)
               {
               int t = this->csize - CSIZE_INC;
               int change;

               if (t < CSIZE_MIN)
                  t = CSIZE_MIN;

               MoveBox__Erase(this);

               change = this->csize - t;
               this->csize = t;
               this->x += (change*16) / 2;
               this->y += (change*16) / 2;
               MoveBox__Draw(this);
               }
            break;

         case PAGE_DOWN:   /* grow */
            {
            int max_width = min(sxdots, MAX_WIDTH);

            if (this->base_depth+(this->csize+CSIZE_INC)*16+1 < sydots  &&
                this->base_width+(this->csize+CSIZE_INC)*16+1 < max_width )
               {
               MoveBox__Erase(this);
               this->x -= (CSIZE_INC*16) / 2;
               this->y -= (CSIZE_INC*16) / 2;
               this->csize += CSIZE_INC;
               if (this->y+this->base_depth+this->csize*16+1 > sydots)
                  this->y = sydots - (this->base_depth+this->csize*16+1);
               if (this->x+this->base_width+this->csize*16+1 > max_width)
                  this->x = max_width - (this->base_width+this->csize*16+1);
               if (this->y < 0)
                  this->y = 0;
               if (this->x < 0)
                  this->x = 0;
               MoveBox__Draw(this);
               }
            }
            break;
         }
      }

#ifdef XFRACT
   Cursor_EndMouseTracking();
#endif

   MoveBox__Erase(this);

   this->should_hide = (BOOLEAN)((key == 'H' || key == 'h') ? TRUE : FALSE);

   return( (BOOLEAN)((key==ESC) ? FALSE : TRUE) );
   }



/*
 * Class:     CEditor
 *
 * Purpose:   Edits a single color component (R, G or B)
 *
 * Note:      Calls the "other_key" function to process keys it doesn't use.
 *            The "change" function is called whenever the value is changed
 *            by the CEditor.
 */

struct _CEditor
   {
   int       x, y;
   char      letter;
   int       val;
   BOOLEAN   done;
   BOOLEAN   hidden;
#ifndef XFRACT
   void    (*other_key)(int key, struct _CEditor *ce, VOIDPTR info);
   void    (*change)(struct _CEditor *ce, VOIDPTR info);
#else
   void    (*other_key)();
   void    (*change)();
#endif
   void     *info;

   } ;

#define CEditor struct _CEditor

/* public: */

#ifndef XFRACT
   static CEditor *CEditor_Construct( int x, int y, char letter,
                                      void (*other_key)(int,CEditor*,void*),
                                      void (*change)(CEditor*,void*), VOIDPTR info);
   static void CEditor_Destroy   (CEditor *this);
   static void CEditor_Draw      (CEditor *this);
   static void CEditor_SetPos    (CEditor *this, int x, int y);
   static void CEditor_SetVal    (CEditor *this, int val);
   static int  CEditor_GetVal    (CEditor *this);
   static void CEditor_SetDone   (CEditor *this, BOOLEAN done);
   static void CEditor_SetHidden (CEditor *this, BOOLEAN hidden);
   static int  CEditor_Edit      (CEditor *this);
#else
   static CEditor *CEditor_Construct( int , int , char ,
                                    void (*other_key)(),
                                    void (*change)(), VOIDPTR );
   static void CEditor_Destroy         (CEditor *);
   static void CEditor_Draw    (CEditor *);
   static void CEditor_SetPos  (CEditor *, int , int );
   static void CEditor_SetVal  (CEditor *, int );
   static int  CEditor_GetVal  (CEditor *);
   static void CEditor_SetDone         (CEditor *, BOOLEAN );
   static void CEditor_SetHidden (CEditor *, BOOLEAN );
   static int  CEditor_Edit    (CEditor *);
#endif

#define CEditor_WIDTH (8*3+4)
#define CEditor_DEPTH (8+4)



#ifndef XFRACT
static CEditor *CEditor_Construct( int x, int y, char letter,
                                   void (*other_key)(int,CEditor*,VOIDPTR),
                                   void (*change)(CEditor*, VOIDPTR), VOIDPTR info)
#else
static CEditor *CEditor_Construct( int x, int y, char letter,
                                   void (*other_key)(),
                                   void (*change)(), VOIDPTR info)
#endif
   {
   CEditor *this = new(CEditor);

   this->x         = x;
   this->y         = y;
   this->letter    = letter;
   this->val       = 0;
   this->other_key = other_key;
   this->hidden    = FALSE;
   this->change    = change;
   this->info      = info;

   return(this);
   }

#ifdef __TURBOC__
#   pragma argsused   /* kills "arg not used" warning */
#endif
#ifdef __CLINT__
#   pragma argsused   /* kills "arg not used" warning */
#endif

static void CEditor_Destroy(CEditor *this)
   {
   delete(this);
   }


static void CEditor_Draw(CEditor *this)
   {
   if (this->hidden)
      return;

   Cursor_Hide();
   displayf(this->x+2, this->y+2, fg_color, bg_color, "%c%02d", this->letter, this->val);
   Cursor_Show();
   }


static void CEditor_SetPos(CEditor *this, int x, int y)
   {
   this->x = x;
   this->y = y;
   }


static void CEditor_SetVal(CEditor *this, int val)
   {
   this->val = val;
   }


static int CEditor_GetVal(CEditor *this)
   {
   return(this->val);
   }


static void CEditor_SetDone(CEditor *this, BOOLEAN done)
   {
   this->done = done;
   }


static void CEditor_SetHidden(CEditor *this, BOOLEAN hidden)
   {
   this->hidden = hidden;
   }


static int CEditor_Edit(CEditor *this)
   {
   int key = 0;
   int diff;

   this->done = FALSE;

   if (!this->hidden)
      {
      Cursor_Hide();
      rect(this->x, this->y, CEditor_WIDTH, CEditor_DEPTH, fg_color);
      Cursor_Show();
      }

#ifdef XFRACT
   Cursor_StartMouseTracking();
#endif
   while ( !this->done )
      {
      Cursor_WaitKey();
      key = getakey();

      switch( key )
         {
         case PAGE_UP:
            if (this->val < 63)
               {
               this->val += 5;
               if (this->val > 63)
                  this->val = 63;
               CEditor_Draw(this);
               this->change(this, this->info);
               }
            break;

         case '+':
         case CTL_PLUS:        /*RB*/
            diff = 1;
            while ( keypressed() == key )
               {
               getakey();
               ++diff;
               }
            if (this->val < 63)
               {
               this->val += diff;
               if (this->val > 63)
                  this->val = 63;
               CEditor_Draw(this);
               this->change(this, this->info);
               }
            break;

         case PAGE_DOWN:
            if (this->val > 0)
               {
               this->val -= 5;
               if (this->val < 0)
                  this->val = 0;
               CEditor_Draw(this);
               this->change(this, this->info);
               } break;

         case '-':
         case CTL_MINUS:     /*RB*/
            diff = 1;
            while ( keypressed() == key )
               {
               getakey();
               ++diff;
               }
            if (this->val > 0)
               {
               this->val -= diff;
               if (this->val < 0)
                  this->val = 0;
               CEditor_Draw(this);
               this->change(this, this->info);
               }
            break;

         case '0':
         case '1':
         case '2':
         case '3':
         case '4':
         case '5':
         case '6':
         case '7':
         case '8':
         case '9':
            this->val = (key - '0') * 10;
            if (this->val > 63)
               this->val = 63;
            CEditor_Draw(this);
            this->change(this, this->info);
            break;

         default:
            this->other_key(key, this, this->info);
            break;
         } /* switch */
      } /* while */
#ifdef XFRACT
   Cursor_EndMouseTracking();
#endif

   if (!this->hidden)
      {
      Cursor_Hide();
      rect(this->x, this->y, CEditor_WIDTH, CEditor_DEPTH, bg_color);
      Cursor_Show();
      }

   return(key);
   }



/*
 * Class:     RGBEditor
 *
 * Purpose:   Edits a complete color using three CEditors for R, G and B
 */

struct _RGBEditor
   {
   int       x, y;            /* position */
   int       curr;            /* 0=r, 1=g, 2=b */
   int       pal;             /* palette number */
   BOOLEAN   done;
   BOOLEAN   hidden;
   CEditor  *color[3];        /* color editors 0=r, 1=g, 2=b */
#ifndef XFRACT
   void    (*other_key)(int key, struct _RGBEditor *e, VOIDPTR info);
   void    (*change)(struct _RGBEditor *e, VOIDPTR info);
#else
   void    (*other_key)();
   void    (*change)();
#endif
   void     *info;
   } ;

#define RGBEditor struct _RGBEditor

/* private: */

   static void      RGBEditor__other_key (int key, CEditor *ceditor, VOIDPTR info);
   static void      RGBEditor__change    (CEditor *ceditor, VOIDPTR info);

/* public: */

#ifndef XFRACT
   static RGBEditor *RGBEditor_Construct(int x, int y,
                     void (*other_key)(int,RGBEditor*,void*),
                     void (*change)(RGBEditor*,void*), VOIDPTR info);
#else
   static RGBEditor *RGBEditor_Construct(int x, int y,
                     void (*other_key)(),
                     void (*change)(), VOIDPTR info);
#endif

   static void     RGBEditor_Destroy  (RGBEditor *this);
   static void     RGBEditor_SetPos   (RGBEditor *this, int x, int y);
   static void     RGBEditor_SetDone  (RGBEditor *this, BOOLEAN done);
   static void     RGBEditor_SetHidden(RGBEditor *this, BOOLEAN hidden);
   static void     RGBEditor_BlankSampleBox(RGBEditor *this);
   static void     RGBEditor_Update   (RGBEditor *this);
   static void     RGBEditor_Draw     (RGBEditor *this);
   static int      RGBEditor_Edit     (RGBEditor *this);
   static void     RGBEditor_SetRGB   (RGBEditor *this, int pal, PALENTRY *rgb);
   static PALENTRY RGBEditor_GetRGB   (RGBEditor *this);

#define RGBEditor_WIDTH 62
#define RGBEditor_DEPTH (1+1+CEditor_DEPTH*3-2+2)

#define RGBEditor_BWIDTH ( RGBEditor_WIDTH - (2+CEditor_WIDTH+1 + 2) )
#define RGBEditor_BDEPTH ( RGBEditor_DEPTH - 4 )



#ifndef XFRACT
static RGBEditor *RGBEditor_Construct(int x, int y, void (*other_key)(int,RGBEditor*,void*),
                                      void (*change)(RGBEditor*,void*), VOIDPTR info)
#else
static RGBEditor *RGBEditor_Construct(int x, int y, void (*other_key)(),
                                      void (*change)(), VOIDPTR info)
#endif
   {
   RGBEditor      *this     = new(RGBEditor);
   static FCODE letter[] = "RGB";
   int             ctr;

   for (ctr=0; ctr<3; ctr++)
      this->color[ctr] = CEditor_Construct(0, 0, letter[ctr], RGBEditor__other_key,
                                           RGBEditor__change, this);

   RGBEditor_SetPos(this, x, y);
   this->curr      = 0;
   this->pal       = 1;
   this->hidden    = FALSE;
   this->other_key = other_key;
   this->change    = change;
   this->info      = info;

   return(this);
   }


static void RGBEditor_Destroy(RGBEditor *this)
   {
   CEditor_Destroy(this->color[0]);
   CEditor_Destroy(this->color[1]);
   CEditor_Destroy(this->color[2]);
   delete(this);
   }


static void RGBEditor_SetDone(RGBEditor *this, BOOLEAN done)
   {
   this->done = done;
   }


static void RGBEditor_SetHidden(RGBEditor *this, BOOLEAN hidden)
   {
   this->hidden = hidden;
   CEditor_SetHidden(this->color[0], hidden);
   CEditor_SetHidden(this->color[1], hidden);
   CEditor_SetHidden(this->color[2], hidden);
   }


static void RGBEditor__other_key(int key, CEditor *ceditor, VOIDPTR info) /* private */
   {
   RGBEditor *this = (RGBEditor *)info;

   switch( key )
      {
      case 'R':
      case 'r':
         if (this->curr != 0)
            {
            this->curr = 0;
            CEditor_SetDone(ceditor, TRUE);
            }
         break;

      case 'G':
      case 'g':
         if (this->curr != 1)
            {
            this->curr = 1;
            CEditor_SetDone(ceditor, TRUE);
            }
         break;

      case 'B':
      case 'b':
         if (this->curr != 2)
            {
            this->curr = 2;
            CEditor_SetDone(ceditor, TRUE);
            }
         break;

      case DELETE:   /* move to next CEditor */
      case CTL_ENTER_2:    /*double click rt mouse also! */
         if ( ++this->curr > 2)
            this->curr = 0;
         CEditor_SetDone(ceditor, TRUE);
         break;

      case INSERT:   /* move to prev CEditor */
         if ( --this->curr < 0)
            this->curr = 2;
         CEditor_SetDone(ceditor, TRUE);
         break;

      default:
         this->other_key(key, this, this->info);
         if (this->done)
            CEditor_SetDone(ceditor, TRUE);
         break;
      }
   }

#ifdef __TURBOC__
#   pragma argsused   /* kills "arg not used" warning */
#endif
#ifdef __CLINT__
#   pragma argsused   /* kills "arg not used" warning */
#endif

static void RGBEditor__change(CEditor *ceditor, VOIDPTR info) /* private */
   {
   RGBEditor *this = (RGBEditor *)info;

   ceditor = NULL; /* just for warning */
   if ( this->pal < colors && !is_reserved(this->pal) )
      setpal(this->pal, CEditor_GetVal(this->color[0]),
          CEditor_GetVal(this->color[1]), CEditor_GetVal(this->color[2]));

   this->change(this, this->info);
   }


static void RGBEditor_SetPos(RGBEditor *this, int x, int y)
   {
   this->x = x;
   this->y = y;

   CEditor_SetPos(this->color[0], x+2, y+2);
   CEditor_SetPos(this->color[1], x+2, y+2+CEditor_DEPTH-1);
   CEditor_SetPos(this->color[2], x+2, y+2+CEditor_DEPTH-1+CEditor_DEPTH-1);
   }


static void RGBEditor_BlankSampleBox(RGBEditor *this)
   {
   if (this->hidden)
      return ;

   Cursor_Hide();
   fillrect(this->x+2+CEditor_WIDTH+1+1, this->y+2+1, RGBEditor_BWIDTH-2, RGBEditor_BDEPTH-2, bg_color);
   Cursor_Show();
   }


static void RGBEditor_Update(RGBEditor *this)
   {
   int x1 = this->x+2+CEditor_WIDTH+1+1,
       y1 = this->y+2+1;

   if (this->hidden)
      return ;

   Cursor_Hide();

   if ( this->pal >= colors )
      {
      fillrect(x1, y1, RGBEditor_BWIDTH-2, RGBEditor_BDEPTH-2, bg_color);
      draw_diamond(x1+(RGBEditor_BWIDTH-5)/2, y1+(RGBEditor_BDEPTH-5)/2, fg_color);
      }

   else if ( is_reserved(this->pal) )
      {
      int x2 = x1+RGBEditor_BWIDTH-3,
          y2 = y1+RGBEditor_BDEPTH-3;

      fillrect(x1, y1, RGBEditor_BWIDTH-2, RGBEditor_BDEPTH-2, bg_color);
      draw_line(x1, y1, x2, y2, fg_color);
      draw_line(x1, y2, x2, y1, fg_color);
      }
   else
      fillrect(x1, y1, RGBEditor_BWIDTH-2, RGBEditor_BDEPTH-2, this->pal);

   CEditor_Draw(this->color[0]);
   CEditor_Draw(this->color[1]);
   CEditor_Draw(this->color[2]);
   Cursor_Show();
   }


static void RGBEditor_Draw(RGBEditor *this)
   {
   if (this->hidden)
      return ;

   Cursor_Hide();
   drect(this->x, this->y, RGBEditor_WIDTH, RGBEditor_DEPTH);
   fillrect(this->x+1, this->y+1, RGBEditor_WIDTH-2, RGBEditor_DEPTH-2, bg_color);
   rect(this->x+1+CEditor_WIDTH+2, this->y+2, RGBEditor_BWIDTH, RGBEditor_BDEPTH, fg_color);
   RGBEditor_Update(this);
   Cursor_Show();
   }


static int RGBEditor_Edit(RGBEditor *this)
   {
   int key = 0;

   this->done = FALSE;

   if (!this->hidden)
      {
      Cursor_Hide();
      rect(this->x, this->y, RGBEditor_WIDTH, RGBEditor_DEPTH, fg_color);
      Cursor_Show();
      }

   while ( !this->done )
      key = CEditor_Edit( this->color[this->curr] );

   if (!this->hidden)
      {
      Cursor_Hide();
      drect(this->x, this->y, RGBEditor_WIDTH, RGBEditor_DEPTH);
      Cursor_Show();
      }

   return (key);
   }


static void RGBEditor_SetRGB(RGBEditor *this, int pal, PALENTRY *rgb)
   {
   this->pal = pal;
   CEditor_SetVal(this->color[0], rgb->red);
   CEditor_SetVal(this->color[1], rgb->green);
   CEditor_SetVal(this->color[2], rgb->blue);
   }


static PALENTRY RGBEditor_GetRGB(RGBEditor *this)
   {
   PALENTRY pal;

   pal.red   = (BYTE)CEditor_GetVal(this->color[0]);
   pal.green = (BYTE)CEditor_GetVal(this->color[1]);
   pal.blue  = (BYTE)CEditor_GetVal(this->color[2]);

   return(pal);
   }



/*
 * Class:     PalTable
 *
 * Purpose:   This is where it all comes together.  Creates the two RGBEditors
 *            and the palette. Moves the cursor, hides/restores the screen,
 *            handles (S)hading, (C)opying, e(X)clude mode, the "Y" exclusion
 *            mode, (Z)oom option, (H)ide palette, rotation, etc.
 *
 */

/*
enum stored_at_values
   {
   NOWHERE,
   DISK,
   MEMORY
   } ;
*/

/*

Modes:
   Auto:          "A", " "
   Exclusion:     "X", "Y", " "
   Freestyle:     "F", " "
   S(t)ripe mode: "T", " "

*/



struct  _PalTable
   {
   int           x, y;
   int           csize;
   int           active;   /* which RGBEditor is active (0,1) */
   int           curr[2];
   RGBEditor    *rgb[2];
   MoveBox      *movebox;
   BOOLEAN       done;
   BOOLEAN       exclude;
   BOOLEAN       auto_select;
   PALENTRY      pal[256];
   FILE         *undo_file;
   BOOLEAN       curr_changed;
   int           num_redo;
   int           hidden;
   int           stored_at;
   FILE         *file;
   char far     *memory;

   PALENTRY far *save_pal[8];


   PALENTRY      fs_color;
   int           top,bottom; /* top and bottom colours of freestyle band */
   int           bandwidth; /*size of freestyle colour band */
   BOOLEAN       freestyle;
   } ;

#define PalTable struct _PalTable

/* private: */

   static void    PalTable__DrawStatus  (PalTable *this, BOOLEAN stripe_mode);
   static void    PalTable__HlPal       (PalTable *this, int pnum, int color);
   static void    PalTable__Draw        (PalTable *this);
   static BOOLEAN PalTable__SetCurr     (PalTable *this, int which, int curr);
   static BOOLEAN PalTable__MemoryAlloc (PalTable *this, long size);
   static void    PalTable__SaveRect    (PalTable *this);
   static void    PalTable__RestoreRect (PalTable *this);
   static void    PalTable__SetPos      (PalTable *this, int x, int y);
   static void    PalTable__SetCSize    (PalTable *this, int csize);
   static int     PalTable__GetCursorColor(PalTable *this);
   static void    PalTable__DoCurs      (PalTable *this, int key);
   static void    PalTable__Rotate      (PalTable *this, int dir, int lo, int hi);
   static void    PalTable__UpdateDAC   (PalTable *this);
   static void    PalTable__other_key   (int key, RGBEditor *rgb, VOIDPTR info);
   static void    PalTable__SaveUndoData(PalTable *this, int first, int last);
   static void    PalTable__SaveUndoRotate(PalTable *this, int dir, int first, int last);
   static void    PalTable__UndoProcess (PalTable *this, int delta);
   static void    PalTable__Undo        (PalTable *this);
   static void    PalTable__Redo        (PalTable *this);
   static void    PalTable__change      (RGBEditor *rgb, VOIDPTR info);

/* public: */

   static PalTable *PalTable_Construct (void);
   static void      PalTable_Destroy   (PalTable *this);
   static void      PalTable_Process   (PalTable *this);
   static void      PalTable_SetHidden (PalTable *this, BOOLEAN hidden);
   static void      PalTable_Hide      (PalTable *this, RGBEditor *rgb, BOOLEAN hidden);


#define PalTable_PALX (1)
#define PalTable_PALY (2+RGBEditor_DEPTH+2)

#define UNDO_DATA        (1)
#define UNDO_DATA_SINGLE (2)
#define UNDO_ROTATE      (3)

/*  - Freestyle code - */

static void PalTable__CalcTopBottom(PalTable *this)
   {
   if (this->curr[this->active] < this->bandwidth )
      this->bottom = 0;
   else
      this->bottom = (this->curr[this->active]) - this->bandwidth;

   if (this->curr[this->active] > (255-this->bandwidth) )
      this->top = 255;
   else
      this->top = (this->curr[this->active]) + this->bandwidth;
   }

static void PalTable__PutBand(PalTable *this, PALENTRY *pal)
   {
   int r,b,a;

  /* clip top and bottom values to stop them running off the end of the DAC */

   PalTable__CalcTopBottom(this);

  /* put bands either side of current colour */

   a = this->curr[this->active];
   b = this->bottom;
   r = this->top;

   pal[a] = this->fs_color;

   if (r != a && a != b)
      {
      mkpalrange(&pal[a], &pal[r], &pal[a], r-a, 1);
      mkpalrange(&pal[b], &pal[a], &pal[b], a-b, 1);
      }

   }


/* - Undo.Redo code - */


static void PalTable__SaveUndoData(PalTable *this, int first, int last)
   {
   int num;

   if ( this->undo_file == NULL )
      return ;

   num = (last - first) + 1;

#ifdef DEBUG_UNDO
   mprintf("%6ld Writing Undo DATA from %d to %d (%d)", ftell(this->undo_file), first, last, num);
#endif

   fseek(this->undo_file, 0, SEEK_CUR);
   if ( num == 1 )
      {
      putc(UNDO_DATA_SINGLE, this->undo_file);
      putc(first, this->undo_file);
      fwrite(this->pal+first, 3, 1, this->undo_file);
      putw( 1 + 1 + 3 + sizeof(int), this->undo_file);
      }
   else
      {
      putc(UNDO_DATA, this->undo_file);
      putc(first, this->undo_file);
      putc(last,  this->undo_file);
      fwrite(this->pal+first, 3, num, this->undo_file);
      putw(1 + 2 + (num*3) + sizeof(int), this->undo_file);
      }

   this->num_redo = 0;
   }


static void PalTable__SaveUndoRotate(PalTable *this, int dir, int first, int last)
   {
   if ( this->undo_file == NULL )
      return ;

#ifdef DEBUG_UNDO
   mprintf("%6ld Writing Undo ROTATE of %d from %d to %d", ftell(this->undo_file), dir, first, last);
#endif

   fseek(this->undo_file, 0, SEEK_CUR);
   putc(UNDO_ROTATE, this->undo_file);
   putc(first, this->undo_file);
   putc(last,  this->undo_file);
   putw(dir, this->undo_file);
   putw(1 + 2 + sizeof(int), this->undo_file);

   this->num_redo = 0;
   }


static void PalTable__UndoProcess(PalTable *this, int delta)   /* undo/redo common code */
   {              /* delta = -1 for undo, +1 for redo */
   int cmd = getc(this->undo_file);

   switch( cmd )
      {
      case UNDO_DATA:
      case UNDO_DATA_SINGLE:
         {
         int      first, last, num;
         PALENTRY temp[256];

         if ( cmd == UNDO_DATA )
            {
            first = (unsigned char)getc(this->undo_file);
            last  = (unsigned char)getc(this->undo_file);
            }
         else  /* UNDO_DATA_SINGLE */
            first = last = (unsigned char)getc(this->undo_file);

         num = (last - first) + 1;

#ifdef DEBUG_UNDO
         mprintf("          Reading DATA from %d to %d", first, last);
#endif

         fread(temp, 3, num, this->undo_file);

         fseek(this->undo_file, -(num*3), SEEK_CUR);  /* go to start of undo/redo data */
         fwrite(this->pal+first, 3, num, this->undo_file);  /* write redo/undo data */

         memmove(this->pal+first, temp, num*3);

         PalTable__UpdateDAC(this);

         RGBEditor_SetRGB(this->rgb[0], this->curr[0], &(this->pal[this->curr[0]]));
         RGBEditor_SetRGB(this->rgb[1], this->curr[1], &(this->pal[this->curr[1]]));
         RGBEditor_Update(this->rgb[0]);
         RGBEditor_Update(this->rgb[1]);
         break;
         }

      case UNDO_ROTATE:
         {
         int first = (unsigned char)getc(this->undo_file);
         int last  = (unsigned char)getc(this->undo_file);
         int dir   = getw(this->undo_file);

#ifdef DEBUG_UNDO
         mprintf("          Reading ROTATE of %d from %d to %d", dir, first, last);
#endif
         PalTable__Rotate(this, delta*dir, first, last);
         break;
         }

      default:
#ifdef DEBUG_UNDO
         mprintf("          Unknown command: %d", cmd);
#endif
         break;
      }

   fseek(this->undo_file, 0, SEEK_CUR);  /* to put us in read mode */
   getw(this->undo_file);  /* read size */
   }


static void PalTable__Undo(PalTable *this)
   {
   int  size;
   long pos;

   if ( ftell(this->undo_file) <= 0  )   /* at beginning of file? */
      {                                  /*   nothing to undo -- exit */
      return ;
      }

   fseek(this->undo_file, -(int)sizeof(int), SEEK_CUR);  /* go back to get size */

   size = getw(this->undo_file);
   fseek(this->undo_file, -size, SEEK_CUR);   /* go to start of undo */

#ifdef DEBUG_UNDO
   mprintf("%6ld Undo:", ftell(this->undo_file));
#endif

   pos = ftell(this->undo_file);

   PalTable__UndoProcess(this, -1);

   fseek(this->undo_file, pos, SEEK_SET);   /* go to start of this block */

   ++this->num_redo;
   }


static void PalTable__Redo(PalTable *this)
   {
   if ( this->num_redo <= 0 )
      return ;

#ifdef DEBUG_UNDO
   mprintf("%6ld Redo:", ftell(this->undo_file));
#endif

   fseek(this->undo_file, 0, SEEK_CUR);  /* to make sure we are in "read" mode */
   PalTable__UndoProcess(this, 1);

   --this->num_redo;
   }


/* - everything else - */


#define STATUS_LEN (4)

static void PalTable__DrawStatus(PalTable *this, BOOLEAN stripe_mode)
   {
   int color, hunds, tens, ones;
   int width = 1+(this->csize*16)+1+1;

   if ( !this->hidden && ( width - (RGBEditor_WIDTH*2+4) >= STATUS_LEN*8 ) )
      {
      int x = this->x + 2 + RGBEditor_WIDTH,
          y = this->y + PalTable_PALY - 10;
      color = PalTable__GetCursorColor(this);
      if (color < 0 || color >= colors) /* hmm, the border returns -1 */
         color = 0;
      Cursor_Hide();

      displayc(x+(0*8), y, fg_color, bg_color, (this->auto_select) ? 'A' : 254);
      displayc(x+(1*8), y, fg_color, bg_color, (this->exclude==1)  ? 'X' :
                                               (this->exclude==2)  ? 'Y' : 254);
      displayc(x+(2*8), y, fg_color, bg_color, (this->freestyle)   ? 'F' : 254);
      displayc(x+(3*8), y, fg_color, bg_color, (stripe_mode)       ? 'T' : 254);

      y = y - 10;
      hunds = (int)(color/100);
      displayc(x+(0*8), y, fg_color, bg_color, (char)(hunds) + '0'  );
      tens = (int)((color - (hunds * 100)) / 10);
      displayc(x+(1*8), y, fg_color, bg_color, (char)(tens) + '0'  );
      ones = (int)(color - (hunds * 100) - (tens * 10));
      displayc(x+(2*8), y, fg_color, bg_color, (char)(ones) + '0'  );

      Cursor_Show();
      }
   }


static void PalTable__HlPal(PalTable *this, int pnum, int color)
   {
   int x    = this->x + PalTable_PALX + (pnum%16)*this->csize,
       y    = this->y + PalTable_PALY + (pnum/16)*this->csize,
       size = this->csize;

   if (this->hidden)
      return ;

   Cursor_Hide();

   if (color < 0)
      drect(x, y, size+1, size+1);
   else
      rect(x, y, size+1, size+1, color);

   Cursor_Show();
   }


static void PalTable__Draw(PalTable *this)
   {
   int pal;
   int xoff, yoff;
   int width;

   if (this->hidden)
      return ;

   Cursor_Hide();

   width = 1+(this->csize*16)+1+1;

   rect(this->x, this->y, width, 2+RGBEditor_DEPTH+2+(this->csize*16)+1+1, fg_color);

   fillrect(this->x+1, this->y+1, width-2, 2+RGBEditor_DEPTH+2+(this->csize*16)+1+1-2, bg_color);

   hline(this->x, this->y+PalTable_PALY-1, width, fg_color);

   if ( width - (RGBEditor_WIDTH*2+4) >= TITLE_LEN*8 )
      {
      int center = (width - TITLE_LEN*8) / 2;

      displayf(this->x+center, this->y+RGBEditor_DEPTH/2-6, fg_color, bg_color, TITLE);
      }

   RGBEditor_Draw(this->rgb[0]);
   RGBEditor_Draw(this->rgb[1]);

   for (pal=0; pal<256; pal++)
      {
      xoff = PalTable_PALX + (pal%16) * this->csize;
      yoff = PalTable_PALY + (pal/16) * this->csize;

      if ( pal >= colors )
         {
         fillrect(this->x + xoff + 1, this->y + yoff + 1, this->csize-1, this->csize-1, bg_color);
         draw_diamond(this->x + xoff + this->csize/2 - 1, this->y + yoff + this->csize/2 - 1, fg_color);
         }

      else if ( is_reserved(pal) )
         {
         int x1 = this->x + xoff + 1,
             y1 = this->y + yoff + 1,
             x2 = x1 + this->csize - 2,
             y2 = y1 + this->csize - 2;
         fillrect(this->x + xoff + 1, this->y + yoff + 1, this->csize-1, this->csize-1, bg_color);
         draw_line(x1, y1, x2, y2, fg_color);
         draw_line(x1, y2, x2, y1, fg_color);
         }
      else
         fillrect(this->x + xoff + 1, this->y + yoff + 1, this->csize-1, this->csize-1, pal);

      }

   if (this->active == 0)
      {
      PalTable__HlPal(this, this->curr[1], -1);
      PalTable__HlPal(this, this->curr[0], fg_color);
      }
   else
      {
      PalTable__HlPal(this, this->curr[0], -1);
      PalTable__HlPal(this, this->curr[1], fg_color);
      }

   PalTable__DrawStatus(this, FALSE);

   Cursor_Show();
   }



static BOOLEAN PalTable__SetCurr(PalTable *this, int which, int curr)
   {
   BOOLEAN redraw = (BOOLEAN)((which < 0) ? TRUE : FALSE);

   if ( redraw )
      {
      which = this->active;
      curr = this->curr[which];
      }
   else
      if ( curr == this->curr[which] || curr < 0 )
         return (FALSE);

   Cursor_Hide();

   PalTable__HlPal(this, this->curr[0], bg_color);
   PalTable__HlPal(this, this->curr[1], bg_color);
   PalTable__HlPal(this, this->top,     bg_color);
   PalTable__HlPal(this, this->bottom,  bg_color);

   if ( this->freestyle )
      {
      this->curr[which] = curr;

      PalTable__CalcTopBottom(this);

      PalTable__HlPal(this, this->top,    -1);
      PalTable__HlPal(this, this->bottom, -1);
      PalTable__HlPal(this, this->curr[this->active], fg_color);

      RGBEditor_SetRGB(this->rgb[which], this->curr[which], &this->fs_color);
      RGBEditor_Update(this->rgb[which]);

      PalTable__UpdateDAC(this);

      Cursor_Show();

      return (TRUE);
      }

   this->curr[which] = curr;

   if (this->curr[0] != this->curr[1])
      PalTable__HlPal(this, this->curr[this->active==0?1:0], -1);
   PalTable__HlPal(this, this->curr[this->active], fg_color);

   RGBEditor_SetRGB(this->rgb[which], this->curr[which], &(this->pal[this->curr[which]]));

   if (redraw)
      {
      int other = (which==0) ? 1 : 0;
      RGBEditor_SetRGB(this->rgb[other], this->curr[other], &(this->pal[this->curr[other]]));
      RGBEditor_Update(this->rgb[0]);
      RGBEditor_Update(this->rgb[1]);
      }
   else
      RGBEditor_Update(this->rgb[which]);

   if (this->exclude)
      PalTable__UpdateDAC(this);

   Cursor_Show();

   this->curr_changed = FALSE;

   return(TRUE);
   }


static BOOLEAN PalTable__MemoryAlloc(PalTable *this, long size)
   {
   char far *temp;

   if (debugflag == 420)
      {
      this->stored_at = NOWHERE;
      return (FALSE);   /* can't do it */
      }
   temp = (char far *)farmemalloc(FAR_RESERVE);   /* minimum free space */

   if (temp == NULL)
      {
      this->stored_at = NOWHERE;
      return (FALSE);   /* can't do it */
      }

   this->memory = (char far *)farmemalloc( size );

   farmemfree(temp);

   if ( this->memory == NULL )
      {
      this->stored_at = NOWHERE;
      return (FALSE);
      }
   else
      {
      this->stored_at = FARMEM;
      return (TRUE);
      }
   }


static void PalTable__SaveRect(PalTable *this)
   {
   char buff[MAX_WIDTH];
   int  width = PalTable_PALX + this->csize*16 + 1 + 1,
        depth = PalTable_PALY + this->csize*16 + 1 + 1;
   int  yoff;


   /* first, do any de-allocationg */

   switch( this->stored_at )
      {
      case NOWHERE:
         break;

      case DISK:
         break;

      case FARMEM:
         if (this->memory != NULL)
            farmemfree(this->memory);
         this->memory = NULL;
         break;
      } ;

   /* allocate space and store the rect */

   if ( PalTable__MemoryAlloc(this, (long)width*depth) )
      {
      char far  *ptr = this->memory;
      char far  *bufptr = buff; /* MSC needs this indirection to get it right */

      Cursor_Hide();
      for (yoff=0; yoff<depth; yoff++)
         {
         getrow(this->x, this->y+yoff, width, buff);
         hline (this->x, this->y+yoff, width, bg_color);
         far_memcpy(ptr,bufptr, width);
         ptr = (char far *)normalize(ptr+width);
         }
      Cursor_Show();
      }

   else /* to disk */
      {
      this->stored_at = DISK;

      if ( this->file == NULL )
         {
         this->file = dir_fopen(tempdir,scrnfile, "w+b");
         if (this->file == NULL)
            {
            this->stored_at = NOWHERE;
            buzzer(3);
            return ;
            }
         }

      rewind(this->file);
      Cursor_Hide();
      for (yoff=0; yoff<depth; yoff++)
         {
         getrow(this->x, this->y+yoff, width, buff);
         hline (this->x, this->y+yoff, width, bg_color);
         if ( fwrite(buff, width, 1, this->file) != 1 )
            {
            buzzer(3);
            break;
            }
         }
      Cursor_Show();
      }

   }


static void PalTable__RestoreRect(PalTable *this)
   {
   char buff[MAX_WIDTH];
   int  width = PalTable_PALX + this->csize*16 + 1 + 1,
        depth = PalTable_PALY + this->csize*16 + 1 + 1;
   int  yoff;

   if (this->hidden)
      return;

   switch ( this->stored_at )
      {
      case DISK:
         rewind(this->file);
         Cursor_Hide();
         for (yoff=0; yoff<depth; yoff++)
            {
            if ( fread(buff, width, 1, this->file) != 1 )
               {
               buzzer(3);
               break;
               }
            putrow(this->x, this->y+yoff, width, buff);
            }
         Cursor_Show();
         break;

      case FARMEM:
         {
         char far  *ptr = this->memory;
         char far  *bufptr = buff; /* MSC needs this indirection to get it right */

         Cursor_Hide();
         for (yoff=0; yoff<depth; yoff++)
            {
            far_memcpy(bufptr, ptr, width);
            putrow(this->x, this->y+yoff, width, buff);
            ptr = (char far *)normalize(ptr+width);
            }
         Cursor_Show();
         break;
         }

      case NOWHERE:
         break;
      } /* switch */
   }


static void PalTable__SetPos(PalTable *this, int x, int y)
   {
   int width = PalTable_PALX + this->csize*16 + 1 + 1;

   this->x = x;
   this->y = y;

   RGBEditor_SetPos(this->rgb[0], x+2, y+2);
   RGBEditor_SetPos(this->rgb[1], x+width-2-RGBEditor_WIDTH, y+2);
   }


static void PalTable__SetCSize(PalTable *this, int csize)
   {
   this->csize = csize;
   PalTable__SetPos(this, this->x, this->y);
   }


static int PalTable__GetCursorColor(PalTable *this)
   {
   int x     = Cursor_GetX(),
       y     = Cursor_GetY(),
       size;
   int color = getcolor(x, y);

   if ( is_reserved(color) )
      {
      if ( is_in_box(x, y, this->x, this->y, 1+(this->csize*16)+1+1, 2+RGBEditor_DEPTH+2+(this->csize*16)+1+1) )
         {  /* is the cursor over the editor? */
         x -= this->x + PalTable_PALX;
         y -= this->y + PalTable_PALY;
         size = this->csize;

         if (x < 0 || y < 0 || x > size*16 || y > size*16)
            return (-1);

         if ( x == size*16 )
            --x;
         if ( y == size*16 )
            --y;

         return ( (y/size)*16 + x/size );
         }
      else
         return (color);
      }

   return (color);
   }



#define CURS_INC 1

static void PalTable__DoCurs(PalTable *this, int key)
   {
   BOOLEAN done  = FALSE;
   BOOLEAN first = TRUE;
   int     xoff  = 0,
           yoff  = 0;

   while ( !done )
      {
      switch(key)
         {
         case RIGHT_ARROW_2:     xoff += CURS_INC*4;   break;
         case RIGHT_ARROW:       xoff += CURS_INC;     break;
         case LEFT_ARROW_2:      xoff -= CURS_INC*4;   break;
         case LEFT_ARROW:        xoff -= CURS_INC;     break;
         case DOWN_ARROW_2:      yoff += CURS_INC*4;   break;
         case DOWN_ARROW:        yoff += CURS_INC;     break;
         case UP_ARROW_2:        yoff -= CURS_INC*4;   break;
         case UP_ARROW:          yoff -= CURS_INC;     break;

         default:
            done = TRUE;
         }

      if (!done)
         {
         if (!first)
            getakey();       /* delete key from buffer */
         else
            first = FALSE;
         key = keypressed();   /* peek at the next one... */
         }
      }

   Cursor_Move(xoff, yoff);

   if (this->auto_select)
      PalTable__SetCurr(this, this->active, PalTable__GetCursorColor(this));
   }


#ifdef __TURBOC__
#   pragma argsused
#endif
#ifdef __CLINT__
#   pragma argsused
#endif

static void PalTable__change(RGBEditor *rgb, VOIDPTR info)
   {
   PalTable *this = (PalTable *)info;
   int       pnum = this->curr[this->active];

   if ( this->freestyle )
      {
      this->fs_color = RGBEditor_GetRGB(rgb);
      PalTable__UpdateDAC(this);
      return ;
      }

   if ( !this->curr_changed )
      {
      PalTable__SaveUndoData(this, pnum, pnum);
      this->curr_changed = TRUE;
      }

   this->pal[pnum] = RGBEditor_GetRGB(rgb);

   if (this->curr[0] == this->curr[1])
      {
      int      other = this->active==0 ? 1 : 0;
      PALENTRY color;

      color = RGBEditor_GetRGB(this->rgb[this->active]);
      RGBEditor_SetRGB(this->rgb[other], this->curr[other], &color);

      Cursor_Hide();
      RGBEditor_Update(this->rgb[other]);
      Cursor_Show();
      }

   }


static void PalTable__UpdateDAC(PalTable *this)
   {
   if ( this->exclude )
      {
      memset(dacbox, 0, 256*3);
      if (this->exclude == 1)
         {
         int a = this->curr[this->active];
         memmove(dacbox[a], &this->pal[a], 3);
         }
      else
         {
         int a = this->curr[0],
             b = this->curr[1];

         if (a>b)
            {
            int t=a;
            a=b;
            b=t;
            }

         memmove(dacbox[a], &this->pal[a], 3*(1+(b-a)));
         }
      }
   else
      {
      memmove(dacbox[0], this->pal, 3*colors);

      if ( this->freestyle )
         PalTable__PutBand(this, (PALENTRY *)dacbox);   /* apply band to dacbox */
      }

   if ( !this->hidden )
      {
      if (inverse)
         {
         memset(dacbox[fg_color], 0, 3);         /* dacbox[fg] = (0,0,0) */
         memset(dacbox[bg_color], 48, 3);        /* dacbox[bg] = (48,48,48) */
         }
      else
         {
         memset(dacbox[bg_color], 0, 3);         /* dacbox[bg] = (0,0,0) */
         memset(dacbox[fg_color], 48, 3);        /* dacbox[fg] = (48,48,48) */
         }
      }

   spindac(0,1);
   }


static void PalTable__Rotate(PalTable *this, int dir, int lo, int hi)
   {

   rotatepal(this->pal, dir, lo, hi);

   Cursor_Hide();

   /* update the DAC.  */

   PalTable__UpdateDAC(this);

   /* update the editors. */

   RGBEditor_SetRGB(this->rgb[0], this->curr[0], &(this->pal[this->curr[0]]));
   RGBEditor_SetRGB(this->rgb[1], this->curr[1], &(this->pal[this->curr[1]]));
   RGBEditor_Update(this->rgb[0]);
   RGBEditor_Update(this->rgb[1]);

   Cursor_Show();
   }


static void PalTable__other_key(int key, RGBEditor *rgb, VOIDPTR info)
   {
   PalTable *this = (PalTable *)info;

   switch(key)
      {
      case '\\':    /* move/resize */
         {
         if (this->hidden)
            break;           /* cannot move a hidden pal */
         Cursor_Hide();
         PalTable__RestoreRect(this);
         MoveBox_SetPos(this->movebox, this->x, this->y);
         MoveBox_SetCSize(this->movebox, this->csize);
         if ( MoveBox_Process(this->movebox) )
            {
            if ( MoveBox_ShouldHide(this->movebox) )
               PalTable_SetHidden(this, TRUE);
            else if ( MoveBox_Moved(this->movebox) )
               {
               PalTable__SetPos(this, MoveBox_X(this->movebox), MoveBox_Y(this->movebox));
               PalTable__SetCSize(this, MoveBox_CSize(this->movebox));
               PalTable__SaveRect(this);
               }
            }
         PalTable__Draw(this);
         Cursor_Show();

         RGBEditor_SetDone(this->rgb[this->active], TRUE);

         if (this->auto_select)
            PalTable__SetCurr(this, this->active, PalTable__GetCursorColor(this));
         break;
         }

      case 'Y':    /* exclude range */
      case 'y':
         if ( this->exclude==2 )
            this->exclude = 0;
         else
            this->exclude = 2;
         PalTable__UpdateDAC(this);
         break;

      case 'X':
      case 'x':     /* exclude current entry */
         if ( this->exclude==1 )
            this->exclude = 0;
         else
            this->exclude = 1;
         PalTable__UpdateDAC(this);
         break;

      case RIGHT_ARROW:
      case LEFT_ARROW:
      case UP_ARROW:
      case DOWN_ARROW:
      case RIGHT_ARROW_2:
      case LEFT_ARROW_2:
      case UP_ARROW_2:
      case DOWN_ARROW_2:
         PalTable__DoCurs(this, key);
         break;

      case ESC:
         this->done = TRUE;
         RGBEditor_SetDone(rgb, TRUE);
         break;

      case ' ':     /* select the other palette register */
         this->active = (this->active==0) ? 1 : 0;
         if (this->auto_select)
            PalTable__SetCurr(this, this->active, PalTable__GetCursorColor(this));
          else
            PalTable__SetCurr(this, -1, 0);

         if (this->exclude || this->freestyle)
            PalTable__UpdateDAC(this);

         RGBEditor_SetDone(rgb, TRUE);
         break;

      case ENTER:    /* set register to color under cursor.  useful when not */
      case ENTER_2:  /* in auto_select mode */

         if ( this->freestyle )
            {
            PalTable__SaveUndoData(this, this->bottom, this->top);
            PalTable__PutBand(this, this->pal);
            }

         PalTable__SetCurr(this, this->active, PalTable__GetCursorColor(this));

         if (this->exclude || this->freestyle )
            PalTable__UpdateDAC(this);

         RGBEditor_SetDone(rgb, TRUE);
         break;

      case 'D':    /* copy (Duplicate?) color in inactive to color in active */
      case 'd':
         {
         int   a = this->active,
                  b = (a==0) ? 1 : 0;
         PALENTRY t;

         t = RGBEditor_GetRGB(this->rgb[b]);
         Cursor_Hide();

         RGBEditor_SetRGB(this->rgb[a], this->curr[a], &t);
         RGBEditor_Update(this->rgb[a]);
         PalTable__change(this->rgb[a], this);
         PalTable__UpdateDAC(this);

         Cursor_Show();
         colorstate = 1;
         break;
         }

      case '=':    /* create a shade range between the two entries */
         {
         int a = this->curr[0],
             b = this->curr[1];

         if (a > b)
            {
            int t = a;
            a = b;
            b = t;
            }

         PalTable__SaveUndoData(this, a, b);

         if (a != b)
            {
            mkpalrange(&this->pal[a], &this->pal[b], &this->pal[a], b-a, 1);
            PalTable__UpdateDAC(this);
            colorstate = 1;
            }

         break;
         }

      case '!':    /* swap r<->g */
         {
         int a = this->curr[0],
             b = this->curr[1];

         if (a > b)
            {
            int t = a;
            a = b;
            b = t;
            }

         PalTable__SaveUndoData(this, a, b);

         if (a != b)
            {
            rotcolrg(&this->pal[a], b-a);
            PalTable__UpdateDAC(this);
            colorstate = 1;
            }


         break;
         }

      case '@':    /* swap g<->b */
      case '"':    /* UK keyboards */
      case 151:    /* French keyboards */
         {
         int a = this->curr[0],
             b = this->curr[1];

         if (a > b)
            {
            int t = a;
            a = b;
            b = t;
            }

         PalTable__SaveUndoData(this, a, b);

         if (a != b)
            {
            rotcolgb(&this->pal[a], b-a);
            PalTable__UpdateDAC(this);
            colorstate = 1;
            }

         break;
         }

      case '#':    /* swap r<->b */
      case 156:    /* UK keyboards (pound sign) */
      case '$':    /* For French keyboards */
         {
         int a = this->curr[0],
             b = this->curr[1];

         if (a > b)
            {
            int t = a;
            a = b;
            b = t;
            }

         PalTable__SaveUndoData(this, a, b);

         if (a != b)
            {
            rotcolbr(&this->pal[a], b-a);
            PalTable__UpdateDAC(this);
            colorstate = 1;
            }

         break;
         }


      case 'T':
      case 't':   /* s(T)ripe mode */
         {
         int key;

         Cursor_Hide();
         PalTable__DrawStatus(this, TRUE);
         key = getakeynohelp();
         Cursor_Show();

         if (key >= '1' && key <= '9')
            {
            int a = this->curr[0],
                b = this->curr[1];

            if (a > b)
               {
               int t = a;
               a = b;
               b = t;
               }

            PalTable__SaveUndoData(this, a, b);

            if (a != b)
               {
               mkpalrange(&this->pal[a], &this->pal[b], &this->pal[a], b-a, key-'0');
               PalTable__UpdateDAC(this);
               colorstate = 1;
               }
            }

         break;
         }

      case 'M':   /* set gamma */
      case 'm':
          {
              static FCODE o_msg[] = {"Enter gamma value"};
              char msg[sizeof(o_msg)];
              int i;
              char buf[20];
              far_strcpy(msg,o_msg);
              sprintf(buf,"%.3f",1./gamma_val);
              stackscreen();
              i = field_prompt(0,msg,NULL,buf,20,NULL);
              unstackscreen();
              if (i != -1) {
                  sscanf(buf,"%f",&gamma_val);
                  if (gamma_val==0) {
                      gamma_val = (float)0.0000000001;
                  }
                  gamma_val = (float)(1./gamma_val);
              }
          }
          break;
      case 'A':   /* toggle auto-select mode */
      case 'a':
         this->auto_select = (BOOLEAN)((this->auto_select) ? FALSE : TRUE);
         if (this->auto_select)
            {
            PalTable__SetCurr(this, this->active, PalTable__GetCursorColor(this));
            if (this->exclude)
               PalTable__UpdateDAC(this);
            }
         break;

      case 'H':
      case 'h': /* toggle hide/display of palette editor */
         Cursor_Hide();
         PalTable_Hide(this, rgb, (BOOLEAN)((this->hidden) ? FALSE : TRUE));
         Cursor_Show();
         break;

      case '.':   /* rotate once */
      case ',':
         {
         int dir = (key=='.') ? 1 : -1;

         PalTable__SaveUndoRotate(this, dir, rotate_lo, rotate_hi);
         PalTable__Rotate(this, dir, rotate_lo, rotate_hi);
         if (colorstate == 0 || colorstate == 2)
            colorstate = 3;
         break;
         }

      case '>':   /* continuous rotation (until a key is pressed) */
      case '<':
         {
         int  dir;
         long tick;
         int  diff = 0;

         Cursor_Hide();

         if ( !this->hidden )
            {
            RGBEditor_BlankSampleBox(this->rgb[0]);
            RGBEditor_BlankSampleBox(this->rgb[1]);
            RGBEditor_SetHidden(this->rgb[0], TRUE);
            RGBEditor_SetHidden(this->rgb[1], TRUE);
            }

         do
            {
            dir = (key=='>') ? 1 : -1;

            while ( !keypressed() )
               {
               tick = readticker();
               PalTable__Rotate(this, dir, rotate_lo, rotate_hi);
               diff += dir;
               while (readticker() == tick) ;   /* wait until a tick passes */
               }

            key = getakey();
            }
         while (key=='<' || key=='>');

         if ( !this->hidden )
            {
            RGBEditor_SetHidden(this->rgb[0], FALSE);
            RGBEditor_SetHidden(this->rgb[1], FALSE);
            RGBEditor_Update(this->rgb[0]);
            RGBEditor_Update(this->rgb[1]);
            }

         if ( diff != 0 )
            {
            PalTable__SaveUndoRotate(this, diff, rotate_lo, rotate_hi);
            if (colorstate == 0 || colorstate == 2)
               colorstate = 3;
            }

         Cursor_Show();
         break;
         }

      case 'I':     /* invert the fg & bg colors */
      case 'i':
        inverse = (BOOLEAN)!inverse;
        PalTable__UpdateDAC(this);
        break;

      case 'V':
      case 'v':  /* set the reserved colors to the editor colors */
         if ( this->curr[0] >= colors || this->curr[1] >= colors ||
              this->curr[0] == this->curr[1] )
            {
            buzzer(2);
            break;
            }

         fg_color = (BYTE)this->curr[0];
         bg_color = (BYTE)this->curr[1];

         if ( !this->hidden )
            {
            Cursor_Hide();
            PalTable__UpdateDAC(this);
            PalTable__Draw(this);
            Cursor_Show();
            }

         RGBEditor_SetDone(this->rgb[this->active], TRUE);
         break;

      case 'O':    /* set rotate_lo and rotate_hi to editors */
      case 'o':
         if (this->curr[0] > this->curr[1])
            {
            rotate_lo = this->curr[1];
            rotate_hi = this->curr[0];
            }
         else
            {
            rotate_lo = this->curr[0];
            rotate_hi = this->curr[1];
            }
         break;

      case F2:    /* restore a palette */
      case F3:
      case F4:
      case F5:
      case F6:
      case F7:
      case F8:
      case F9:
         {
         int which = key - F2;

         if ( this->save_pal[which] != NULL )
            {
            Cursor_Hide();

            PalTable__SaveUndoData(this, 0, 255);
            far_memcpy(this->pal,this->save_pal[which],256*3);
            PalTable__UpdateDAC(this);

            PalTable__SetCurr(this, -1, 0);
            Cursor_Show();
            RGBEditor_SetDone(this->rgb[this->active], TRUE);
            colorstate = 1;
            }
         else
            buzzer(3);   /* error buzz */
         break;
         }

      case SF2:   /* save a palette */
      case SF3:
      case SF4:
      case SF5:
      case SF6:
      case SF7:
      case SF8:
      case SF9:
         {
         int which = key - SF2;

         if ( this->save_pal[which] != NULL )
            {
            far_memcpy(this->save_pal[which],this->pal,256*3);
            }
         else
            buzzer(3); /* oops! short on memory! */
         break;
         }

      case 'L':     /* load a .map palette */
      case 'l':
         {
         PalTable__SaveUndoData(this, 0, 255);

         load_palette();
#ifndef XFRACT
         getpalrange(0, colors, this->pal);
#else
         getpalrange(0, 256, this->pal);
#endif
         PalTable__UpdateDAC(this);
         RGBEditor_SetRGB(this->rgb[0], this->curr[0], &(this->pal[this->curr[0]]));
         RGBEditor_Update(this->rgb[0]);
         RGBEditor_SetRGB(this->rgb[1], this->curr[1], &(this->pal[this->curr[1]]));
         RGBEditor_Update(this->rgb[1]);
         break;
         }

      case 'S':     /* save a .map palette */
      case 's':
         {
#ifndef XFRACT
         setpalrange(0, colors, this->pal);
#else
         setpalrange(0, 256, this->pal);
#endif
         save_palette();
         PalTable__UpdateDAC(this);
         break;
         }

      case 'C':     /* color cycling sub-mode */
      case 'c':
         {
         BOOLEAN oldhidden = (BOOLEAN)this->hidden;

         PalTable__SaveUndoData(this, 0, 255);

         Cursor_Hide();
         if ( !oldhidden )
            PalTable_Hide(this, rgb, TRUE);
         setpalrange(0, colors, this->pal);
         rotate(0);
         getpalrange(0, colors, this->pal);
         PalTable__UpdateDAC(this);
         if ( !oldhidden )
            {
            RGBEditor_SetRGB(this->rgb[0], this->curr[0], &(this->pal[this->curr[0]]));
            RGBEditor_SetRGB(this->rgb[1], this->curr[1], &(this->pal[this->curr[1]]));
            PalTable_Hide(this, rgb, FALSE);
            }
         Cursor_Show();
         break;
         }

      case 'F':
      case 'f':    /* toggle freestyle palette edit mode */
         this->freestyle= (BOOLEAN)((this->freestyle) ? FALSE :TRUE);

         PalTable__SetCurr(this, -1, 0);

         if ( !this->freestyle )   /* if turning off... */
            PalTable__UpdateDAC(this);
         colorstate = 1;

         break;

      case CTL_DEL:  /* rt plus down */
         if (this->bandwidth >0 )
            this->bandwidth  --;
         else
            this->bandwidth=0;
         PalTable__SetCurr(this, -1, 0);
         break;

      case CTL_INSERT: /* rt plus up */
         if (this->bandwidth <255 )
           this->bandwidth ++;
         else
            this->bandwidth = 255;
         PalTable__SetCurr(this, -1, 0);
         break;

      case 'W':   /* convert to greyscale */
      case 'w':
         {
         switch ( this->exclude )
            {
            case 0:   /* normal mode.  convert all colors to grey scale */
               PalTable__SaveUndoData(this, 0, 255);
               palrangetogrey(this->pal, 0, 256);
               break;

            case 1:   /* 'x' mode. convert current color to grey scale.  */
               PalTable__SaveUndoData(this, this->curr[this->active], this->curr[this->active]);
               palrangetogrey(this->pal, this->curr[this->active], 1);
               break;

            case 2:  /* 'y' mode.  convert range between editors to grey. */
               {
               int a = this->curr[0],
                   b = this->curr[1];

               if (a > b)
                  {
                  int t = a;
                  a = b;
                  b = t;
                  }

               PalTable__SaveUndoData(this, a, b);
               palrangetogrey(this->pal, a, 1+(b-a));
               break;
               }
            }

         PalTable__UpdateDAC(this);
         RGBEditor_SetRGB(this->rgb[0], this->curr[0], &(this->pal[this->curr[0]]));
         RGBEditor_Update(this->rgb[0]);
         RGBEditor_SetRGB(this->rgb[1], this->curr[1], &(this->pal[this->curr[1]]));
         RGBEditor_Update(this->rgb[1]);
         colorstate = 1;
         break;
         }

      case 'N':   /* convert to negative color */
      case 'n':
         {
         switch ( this->exclude )
            {
            case 0:      /* normal mode.  convert all colors to grey scale */
               PalTable__SaveUndoData(this, 0, 255);
               palrangetonegative(this->pal, 0, 256);
               break;

            case 1:      /* 'x' mode. convert current color to grey scale.  */
               PalTable__SaveUndoData(this, this->curr[this->active], this->curr[this->active]);
               palrangetonegative(this->pal, this->curr[this->active], 1);
               break;

            case 2:  /* 'y' mode.  convert range between editors to grey. */
               {
               int a = this->curr[0],
                   b = this->curr[1];

               if (a > b)
                  {
                  int t = a;
                  a = b;
                  b = t;
                  }

               PalTable__SaveUndoData(this, a, b);
               palrangetonegative(this->pal, a, 1+(b-a));
               break;
               }
            }

         PalTable__UpdateDAC(this);
         RGBEditor_SetRGB(this->rgb[0], this->curr[0], &(this->pal[this->curr[0]]));
         RGBEditor_Update(this->rgb[0]);
         RGBEditor_SetRGB(this->rgb[1], this->curr[1], &(this->pal[this->curr[1]]));
         RGBEditor_Update(this->rgb[1]);
         colorstate = 1;
         break;
         }

      case 'U':     /* Undo */
      case 'u':
         PalTable__Undo(this);
         break;

      case 'e':    /* Redo */
      case 'E':
         PalTable__Redo(this);
         break;

      } /* switch */
      PalTable__DrawStatus(this, FALSE);
   }

static void PalTable__MkDefaultPalettes(PalTable *this)  /* creates default Fkey palettes */
{
   int i;
   for(i=0; i<8; i++) /* copy original palette to save areas */
   {
      if (this->save_pal[i] != NULL)
      {
         far_memcpy(this->save_pal[i], this->pal, 256*3);
      }
   }
}



static PalTable *PalTable_Construct(void)
   {
   PalTable     *this = new(PalTable);
   int           csize;
   int           ctr;
   PALENTRY far *mem_block;
   void far     *temp;

   temp = (void far *)farmemalloc(FAR_RESERVE);

   if ( temp != NULL )
      {
      mem_block = (PALENTRY far *)farmemalloc(256L*3 * 8);

      if ( mem_block == NULL )
         {
         for (ctr=0; ctr<8; ctr++)
            this->save_pal[ctr] = NULL;
         }
      else
         {
         for (ctr=0; ctr<8; ctr++)
            this->save_pal[ctr] = mem_block + (256*ctr);
         }
      farmemfree(temp);
      }

   this->rgb[0] = RGBEditor_Construct(0, 0, PalTable__other_key,
                  PalTable__change, this);
   this->rgb[1] = RGBEditor_Construct(0, 0, PalTable__other_key,
                  PalTable__change, this);

   this->movebox = MoveBox_Construct(0,0,0, PalTable_PALX+1, PalTable_PALY+1);

   this->active      = 0;
   this->curr[0]     = 1;
   this->curr[1]     = 1;
   this->auto_select = TRUE;
   this->exclude     = FALSE;
   this->hidden      = FALSE;
   this->stored_at   = NOWHERE;
   this->file        = NULL;
   this->memory      = NULL;

   this->fs_color.red   = 42;
   this->fs_color.green = 42;
   this->fs_color.blue  = 42;
   this->freestyle      = FALSE;
   this->bandwidth      = 15;
   this->top            = 255;
   this->bottom         = 0 ;

   this->undo_file    = dir_fopen(tempdir,undofile, "w+b");
   this->curr_changed = FALSE;
   this->num_redo     = 0;

   RGBEditor_SetRGB(this->rgb[0], this->curr[0], &this->pal[this->curr[0]]);
   RGBEditor_SetRGB(this->rgb[1], this->curr[1], &this->pal[this->curr[0]]);

   if (video_scroll) {
      PalTable__SetPos(this, video_startx, video_starty);
      csize = ( (vesa_yres-(PalTable_PALY+1+1)) / 2 ) / 16;
   } else {
      PalTable__SetPos(this, 0, 0);
      csize = ( (sydots-(PalTable_PALY+1+1)) / 2 ) / 16;
   }

   if (csize<CSIZE_MIN)
      csize = CSIZE_MIN;
   PalTable__SetCSize(this, csize);

   return(this);
   }


static void PalTable_SetHidden(PalTable *this, BOOLEAN hidden)
   {
   this->hidden = hidden;
   RGBEditor_SetHidden(this->rgb[0], hidden);
   RGBEditor_SetHidden(this->rgb[1], hidden);
   PalTable__UpdateDAC(this);
   }



static void PalTable_Hide(PalTable *this, RGBEditor *rgb, BOOLEAN hidden)
   {
   if (hidden)
      {
      PalTable__RestoreRect(this);
      PalTable_SetHidden(this, TRUE);
      reserve_colors = FALSE;
      if (this->auto_select)
         PalTable__SetCurr(this, this->active, PalTable__GetCursorColor(this));
      }
   else
      {
      PalTable_SetHidden(this, FALSE);
      reserve_colors = TRUE;
      if (this->stored_at == NOWHERE)  /* do we need to save screen? */
         PalTable__SaveRect(this);
      PalTable__Draw(this);
      if (this->auto_select)
         PalTable__SetCurr(this, this->active, PalTable__GetCursorColor(this));
      RGBEditor_SetDone(rgb, TRUE);
      }
   }


static void PalTable_Destroy(PalTable *this)
   {

   if (this->file != NULL)
      {
      fclose(this->file);
      dir_remove(tempdir,scrnfile);
      }

   if (this->undo_file != NULL)
      {
      fclose(this->undo_file);
      dir_remove(tempdir,undofile);
      }

   if (this->memory != NULL)
      farmemfree(this->memory);

   if (this->save_pal[0] != NULL)
      farmemfree((BYTE far *)this->save_pal[0]);

   RGBEditor_Destroy(this->rgb[0]);
   RGBEditor_Destroy(this->rgb[1]);
   MoveBox_Destroy(this->movebox);
   delete(this);
   }


static void PalTable_Process(PalTable *this)
   {
   int ctr;

   getpalrange(0, colors, this->pal);

   /* Make sure all palette entries are 0-63 */

   for(ctr=0; ctr<768; ctr++)
      ((char *)this->pal)[ctr] &= 63;

   PalTable__UpdateDAC(this);

   RGBEditor_SetRGB(this->rgb[0], this->curr[0], &this->pal[this->curr[0]]);
   RGBEditor_SetRGB(this->rgb[1], this->curr[1], &this->pal[this->curr[0]]);

   if (!this->hidden)
      {
      MoveBox_SetPos(this->movebox, this->x, this->y);
      MoveBox_SetCSize(this->movebox, this->csize);
      if ( !MoveBox_Process(this->movebox) )
         {
         setpalrange(0, colors, this->pal);
         return ;
         }

      PalTable__SetPos(this, MoveBox_X(this->movebox), MoveBox_Y(this->movebox));
      PalTable__SetCSize(this, MoveBox_CSize(this->movebox));

      if ( MoveBox_ShouldHide(this->movebox) )
         {
         PalTable_SetHidden(this, TRUE);
         reserve_colors = FALSE;   /* <EAN> */
         }
      else
         {
         reserve_colors = TRUE;    /* <EAN> */
         PalTable__SaveRect(this);
         PalTable__Draw(this);
         }
      }

   PalTable__SetCurr(this, this->active,          PalTable__GetCursorColor(this));
   PalTable__SetCurr(this, (this->active==1)?0:1, PalTable__GetCursorColor(this));
   Cursor_Show();
   PalTable__MkDefaultPalettes(this);
   this->done = FALSE;

   while ( !this->done )
      RGBEditor_Edit(this->rgb[this->active]);

   Cursor_Hide();
   PalTable__RestoreRect(this);
   setpalrange(0, colors, this->pal);
   }


/*
 * interface to FRACTINT
 */



void EditPalette(void)       /* called by fractint */
   {
   int       oldlookatmouse = lookatmouse;
   int       oldsxoffs      = sxoffs;
   int       oldsyoffs      = syoffs;
   PalTable *pt;

   mem_init(strlocn, 10*1024);

   if ( (font8x8 = findfont(0)) == NULL )
      return ;

   if (sxdots < 133 || sydots < 174)
      return; /* prevents crash when physical screen is too small */

   plot = putcolor;

   line_buff = newx(max(sxdots,sydots));

   lookatmouse = 3;
   sxoffs = syoffs = 0;

   reserve_colors = TRUE;
   inverse = FALSE;
   fg_color = (BYTE)(255%colors);
   bg_color = (BYTE)(fg_color-1);

   Cursor_Construct();
   pt = PalTable_Construct();
   PalTable_Process(pt);
   PalTable_Destroy(pt);
   Cursor_Destroy();

   lookatmouse = oldlookatmouse;
   sxoffs = oldsxoffs;
   syoffs = oldsyoffs;
   delete(line_buff);
   }
