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

  /* see Fractint.c for a description of the "include"  hierarchy */
#include "port.h"
#include "prototyp.h"
#include "drivers.h"

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

#define FAR_RESERVE     8192L     /* amount of mem we will leave avail. */

#define MAX_WIDTH        1024     /* palette editor cannot be wider than this */

char scrnfile[] = "FRACTINT.$$1";  /* file where screen portion is */
                                   /* stored */
char undofile[] = "FRACTINT.$$2";  /* file where undo list is stored */
#define TITLE   "FRACTINT"

#define TITLE_LEN (8)


#define NEWX(size)     malloc(size)
#define NEWC(class_)     ((class_ *) malloc(sizeof(class_)))
#define DELETE(block)  (free(block), block=NULL)  /* just for warning */

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
 int         lookatmouse;        /* mouse mode for driver_get_key(), etc    */
 int         strlocn[];          /* 10K buffer to store classes in   */
 int         colors;             /* # colors avail.                  */
 int         g_color_bright;       /* brightest color in palette       */
 int         g_color_dark;         /* darkest color in palette         */
 int         g_color_medium;       /* nearest to medbright gray color  */
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
	g_dac_box[pal][0] = (BYTE)r;
	g_dac_box[pal][1] = (BYTE)g;
	g_dac_box[pal][2] = (BYTE)b;
	spindac(0,1);
	}


static void setpalrange(int first, int how_many, PALENTRY *pal)
	{
	memmove(g_dac_box + first, pal, how_many*3);
	spindac(0,1);
	}


static void getpalrange(int first, int how_many, PALENTRY *pal)
	{
	memmove(pal, g_dac_box + first, how_many*3);
	}


static void rotatepal(PALENTRY *pal, int dir, int lo, int hi)
	{             /* rotate in either direction */
	PALENTRY hold;
	int      size;

	size  = 1 + (hi-lo);

	if (dir > 0)
		{
		while (dir-- > 0)
			{
			memmove(&hold, &pal[hi],  3);
			memmove(&pal[lo + 1], &pal[lo], 3*(size-1));
			memmove(&pal[lo], &hold, 3);
			}
		}

	else if (dir < 0)
		{
		while (dir++ < 0)
			{
			memmove(&hold, &pal[lo], 3);
			memmove(&pal[lo], &pal[lo + 1], 3*(size-1));
			memmove(&pal[hi], &hold,  3);
			}
		}
	}


static void clip_put_line(int row, int start, int stop, BYTE *pixels)
	{
	if (row < 0 || row >= sydots || start > sxdots || stop < 0)
		return ;

	if (start < 0)
		{
		pixels += -start;
		start = 0;
		}

	if (stop >= sxdots)
		stop = sxdots - 1;

	if (start > stop)
		return ;

	put_line(row, start, stop, pixels);
	}


static void clip_get_line(int row, int start, int stop, BYTE *pixels)
	{
	if (row < 0 || row >= sydots || start > sxdots || stop < 0)
		return ;

	if (start < 0)
		{
		pixels += -start;
		start = 0;
		}

	if (stop >= sxdots)
		stop = sxdots - 1;

	if (start > stop)
		return ;

	get_line(row, start, stop, pixels);
	}


void clip_putcolor(int x, int y, int color)
	{
	if (x < 0 || y < 0 || x >= sxdots || y >= sydots)
		return ;

	putcolor(x, y, color);
	}


int clip_getcolor(int x, int y)
	{
	if (x < 0 || y < 0 || x >= sxdots || y >= sydots)
		return 0;

	return getcolor(x, y);
	}


static void hline(int x, int y, int width, int color)
	{
	memset(line_buff, color, width);
	clip_put_line(y, x, x + width-1, line_buff);
	}


static void vline(int x, int y, int depth, int color)
	{
	while (depth-- > 0)
		clip_putcolor(x, y++, color);
	}


void getrow(int x, int y, int width, char *buff)
	{
	clip_get_line(y, x, x + width-1, (BYTE *)buff);
	}


void putrow(int x, int y, int width, char *buff)
	{
	clip_put_line(y, x, x + width-1, (BYTE *)buff);
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
	hline(x, y + depth-1, width, color);

	vline(x, y, depth, color);
	vline(x + width-1, y, depth, color);
	}


#ifndef USE_VARARGS
static void displayf(int x, int y, int fg, int bg, char *format, ...)
#else
static void displayf(va_alist)
va_dcl
#endif
	{
	char buff[81];

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

	driver_display_string(x, y, fg, bg, buff);
}


/*
 * create smooth shades between two colors
 */


static void mkpalrange(PALENTRY *p1, PALENTRY *p2, PALENTRY pal[], int num, int skip)
	{
	int    curr;
	double rm = (double)((int) p2->red   - (int) p1->red) / num,
          gm = (double)((int) p2->green - (int) p1->green) / num,
          bm = (double)((int) p2->blue  - (int) p1->blue) / num;

	for (curr = 0; curr < num; curr += skip)
		{
		if (gamma_val == 1)
          {
          pal[curr].red   = (BYTE)((p1->red   == p2->red) ? p1->red   :
						(int) p1->red   + (int) (rm*curr));
          pal[curr].green = (BYTE)((p1->green == p2->green) ? p1->green :
              (int) p1->green + (int) (gm*curr));
          pal[curr].blue  = (BYTE)((p1->blue  == p2->blue) ? p1->blue  :
						(int) p1->blue  + (int) (bm*curr));
          }
          else
          {
          pal[curr].red   = (BYTE)((p1->red   == p2->red) ? p1->red   :
						(int) (p1->red   + pow(curr/(double)(num-1),gamma_val)*num*rm));
          pal[curr].green = (BYTE)((p1->green == p2->green) ? p1->green :
						(int) (p1->green + pow(curr/(double)(num-1),gamma_val)*num*gm));
          pal[curr].blue  = (BYTE)((p1->blue  == p2->blue) ? p1->blue  :
						(int) (p1->blue  + pow(curr/(double)(num-1),gamma_val)*num*bm));
          }
		}
	}



/*  Swap RG GB & RB columns */

static void rotcolrg(PALENTRY pal[], int num)
	{
	int    curr;
	int    dummy;

	for (curr = 0; curr <= num; curr++)
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

	for (curr = 0; curr <= num; curr++)
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

	for (curr = 0; curr <= num; curr++)
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


	for (curr = &pal[first]; how_many > 0; how_many--, curr++)
		{
		val = (BYTE) (((int)curr->red*30 + (int)curr->green*59 + (int)curr->blue*11) / 100);
		curr->red = curr->green = curr->blue = (BYTE)val;
		}
	}

/*
 * convert a range of colors to their inverse
 */


static void palrangetonegative(PALENTRY pal[], int first, int how_many)
	{
	PALENTRY      *curr;

	for (curr = &pal[first]; how_many > 0; how_many--, curr++)
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

	for (ctr = 0, ptr=line_buff; ctr < width; ctr++, ptr++)
		*ptr = (BYTE)((ctr&2) ? bg_color : fg_color);

	putrow(x, y, width, (char *)line_buff);
	}


static void vdline(int x, int y, int depth)
	{
	int ctr;

	for (ctr = 0; ctr < depth; ctr++, y++)
		clip_putcolor(x, y, (ctr&2) ? bg_color : fg_color);
	}


static void drect(int x, int y, int width, int depth)
	{
	hdline(x, y, width);
	hdline(x, y + depth-1, width);

	vdline(x, y, depth);
	vdline(x + width-1, y, depth);
	}


/*
 * misc. routines
 *
 */


static BOOLEAN is_reserved(int color)
	{
	return (BOOLEAN) ((reserve_colors && (color == (int)fg_color || color == (int)bg_color)) ? TRUE : FALSE);
	}



static BOOLEAN is_in_box(int x, int y, int bx, int by, int bw, int bd)
	{
	return (BOOLEAN) ((x >= bx) && (y >= by) && (x < bx + bw) && (y < by + bd));
	}



static void draw_diamond(int x, int y, int color)
	{
	putcolor (x + 2, y + 0,    color);
	hline    (x + 1, y + 1, 3, color);
	hline    (x + 0, y + 2, 5, color);
	hline    (x + 1, y + 3, 3, color);
	putcolor (x + 2, y + 4,    color);
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
		return FALSE;

	the_cursor = NEWC(Cursor);

	the_cursor->x          = sxdots/2;
	the_cursor->y          = sydots/2;
	the_cursor->hidden     = 1;
	the_cursor->blink      = FALSE;
	the_cursor->last_blink = 0;

	return TRUE;
	}


void Cursor_Destroy(void)
	{
	if (the_cursor != NULL)
		DELETE(the_cursor);

	the_cursor = NULL;
	}



static void Cursor__Draw(void)
	{
	int color;

	find_special_colors();
	color = (the_cursor->blink) ? g_color_medium : g_color_dark;

	vline(the_cursor->x, the_cursor->y-CURSOR_SIZE-1, CURSOR_SIZE, color);
	vline(the_cursor->x, the_cursor->y + 2,             CURSOR_SIZE, color);

	hline(the_cursor->x-CURSOR_SIZE-1, the_cursor->y, CURSOR_SIZE, color);
	hline(the_cursor->x + 2,             the_cursor->y, CURSOR_SIZE, color);
	}


static void Cursor__Save(void)
	{
	vgetrow(the_cursor->x, the_cursor->y-CURSOR_SIZE-1, CURSOR_SIZE, the_cursor->t);
	vgetrow(the_cursor->x, the_cursor->y + 2,             CURSOR_SIZE, the_cursor->b);

	getrow(the_cursor->x-CURSOR_SIZE-1, the_cursor->y,  CURSOR_SIZE, the_cursor->l);
	getrow(the_cursor->x + 2,             the_cursor->y,  CURSOR_SIZE, the_cursor->r);
	}


static void Cursor__Restore(void)
	{
	vputrow(the_cursor->x, the_cursor->y-CURSOR_SIZE-1, CURSOR_SIZE, the_cursor->t);
	vputrow(the_cursor->x, the_cursor->y + 2,             CURSOR_SIZE, the_cursor->b);

	putrow(the_cursor->x-CURSOR_SIZE-1, the_cursor->y,  CURSOR_SIZE, the_cursor->l);
	putrow(the_cursor->x + 2,             the_cursor->y,  CURSOR_SIZE, the_cursor->r);
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
	return the_cursor->hidden;
	}


#endif


void Cursor_Move(int xoff, int yoff)
	{
	if (!the_cursor->hidden)
		Cursor__Restore();

	the_cursor->x += xoff;
	the_cursor->y += yoff;

	if (the_cursor->x < 0)       the_cursor->x = 0;
	if (the_cursor->y < 0)       the_cursor->y = 0;
	if (the_cursor->x >= sxdots) the_cursor->x = sxdots-1;
	if (the_cursor->y >= sydots) the_cursor->y = sydots-1;

	if (!the_cursor->hidden)
		{
		Cursor__Save();
		Cursor__Draw();
		}
	}


int Cursor_GetX(void)   
{ return the_cursor->x; }

int Cursor_GetY(void)   
{ return the_cursor->y; }


void Cursor_Hide(void)
	{
	if (the_cursor->hidden++ == 0)
		Cursor__Restore();
	}


void Cursor_Show(void)
	{
	if (--the_cursor->hidden == 0)
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

	if ((tick - the_cursor->last_blink) > CURSOR_BLINK_RATE)
		{
		the_cursor->blink = (BOOLEAN)((the_cursor->blink) ? FALSE : TRUE);
		the_cursor->last_blink = tick;
		if (!the_cursor->hidden)
			Cursor__Draw();
		}
	else if (tick < the_cursor->last_blink)
		the_cursor->last_blink = tick;
}

int Cursor_WaitKey(void)   /* blink cursor while waiting for a key */
	{

	while (!driver_wait_key_pressed(1)) 
	{
       Cursor_CheckBlink();
	}

	return driver_key_pressed();
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

	static void     MoveBox__Draw     (MoveBox *me);
	static void     MoveBox__Erase    (MoveBox *me);
	static void     MoveBox__Move     (MoveBox *me, int key);

/* public: */

	static MoveBox *MoveBox_Construct  (int x, int y, int csize, int base_width,
                                      int base_depth);
	static void     MoveBox_Destroy    (MoveBox *me);
	static BOOLEAN  MoveBox_Process    (MoveBox *me); /* returns FALSE if ESCAPED */
	static BOOLEAN  MoveBox_Moved      (MoveBox *me);
	static BOOLEAN  MoveBox_ShouldHide (MoveBox *me);
	static int      MoveBox_X          (MoveBox *me);
	static int      MoveBox_Y          (MoveBox *me);
	static int      MoveBox_CSize      (MoveBox *me);

	static void     MoveBox_SetPos     (MoveBox *me, int x, int y);
	static void     MoveBox_SetCSize   (MoveBox *me, int csize);



static MoveBox *MoveBox_Construct(int x, int y, int csize, int base_width, int base_depth)
	{
	MoveBox *me = NEWC(MoveBox);

	me->x           = x;
	me->y           = y;
	me->csize       = csize;
	me->base_width  = base_width;
	me->base_depth  = base_depth;
	me->moved       = FALSE;
	me->should_hide = FALSE;
	me->t           = NEWX(sxdots);
	me->b           = NEWX(sxdots);
	me->l           = NEWX(sydots);
	me->r           = NEWX(sydots);

	return me;
	}


static void MoveBox_Destroy(MoveBox *me)
	{
	DELETE(me->t);
	DELETE(me->b);
	DELETE(me->l);
	DELETE(me->r);
	DELETE(me);
	}


static BOOLEAN MoveBox_Moved(MoveBox *me) 
{ return me->moved; }

static BOOLEAN MoveBox_ShouldHide(MoveBox *me) 
{ return me->should_hide; }

static int MoveBox_X(MoveBox *me)      
{ return me->x; }

static int MoveBox_Y(MoveBox *me)      
{ return me->y; }

static int MoveBox_CSize(MoveBox *me)  
{ return me->csize; }


static void MoveBox_SetPos(MoveBox *me, int x, int y)
	{
	me->x = x;
	me->y = y;
	}


static void MoveBox_SetCSize(MoveBox *me, int csize)
	{
	me->csize = csize;
	}


static void MoveBox__Draw(MoveBox *me)  /* private */
	{
	int width = me->base_width + me->csize*16 + 1,
       depth = me->base_depth + me->csize*16 + 1;
	int x     = me->x,
       y     = me->y;


	getrow (x, y,         width, me->t);
	getrow (x, y + depth-1, width, me->b);

	vgetrow(x,         y, depth, me->l);
	vgetrow(x + width-1, y, depth, me->r);

	hdline(x, y,         width);
	hdline(x, y + depth-1, width);

	vdline(x,         y, depth);
	vdline(x + width-1, y, depth);
	}


static void MoveBox__Erase(MoveBox *me)   /* private */
	{
	int width = me->base_width + me->csize*16 + 1,
       depth = me->base_depth + me->csize*16 + 1;

	vputrow(me->x,         me->y, depth, me->l);
	vputrow(me->x + width-1, me->y, depth, me->r);

	putrow(me->x, me->y,         width, me->t);
	putrow(me->x, me->y + depth-1, width, me->b);
	}


#define BOX_INC     1
#define CSIZE_INC   2

static void MoveBox__Move(MoveBox *me, int key)
	{
	BOOLEAN done  = FALSE;
	BOOLEAN first = TRUE;
	int     xoff  = 0,
           yoff  = 0;

	while (!done)
		{
		switch (key)
			{
			case FIK_CTL_RIGHT_ARROW:     xoff += BOX_INC*4;   break;
			case FIK_RIGHT_ARROW:       xoff += BOX_INC;     break;
			case FIK_CTL_LEFT_ARROW:      xoff -= BOX_INC*4;   break;
			case FIK_LEFT_ARROW:        xoff -= BOX_INC;     break;
			case FIK_CTL_DOWN_ARROW:      yoff += BOX_INC*4;   break;
			case FIK_DOWN_ARROW:        yoff += BOX_INC;     break;
			case FIK_CTL_UP_ARROW:        yoff -= BOX_INC*4;   break;
			case FIK_UP_ARROW:          yoff -= BOX_INC;     break;

			default:
				done = TRUE;
			}

		if (!done)
			{
			if (!first)
				driver_get_key();       /* delete key from buffer */
			else
				first = FALSE;
			key = driver_key_pressed();   /* peek at the next one... */
			}
		}

	xoff += me->x;
	yoff += me->y;   /* (xoff,yoff) = new position */

	if (xoff < 0) xoff = 0;
	if (yoff < 0) yoff = 0;

	if (xoff + me->base_width + me->csize*16 + 1 > sxdots)
       xoff = sxdots - (me->base_width + me->csize*16 + 1);

	if (yoff + me->base_depth + me->csize*16 + 1 > sydots)
		yoff = sydots - (me->base_depth + me->csize*16 + 1);

	if (xoff != me->x || yoff != me->y)
		{
		MoveBox__Erase(me);
		me->y = yoff;
		me->x = xoff;
		MoveBox__Draw(me);
		}
	}


static BOOLEAN MoveBox_Process(MoveBox *me)
	{
	int     key;
	int     orig_x     = me->x,
           orig_y     = me->y,
           orig_csize = me->csize;

	MoveBox__Draw(me);

#ifdef XFRACT
	Cursor_StartMouseTracking();
#endif
	while (1)
		{
		Cursor_WaitKey();
		key = driver_get_key();

		if (key == FIK_ENTER || key == FIK_ENTER_2 || key == FIK_ESC || key == 'H' || key == 'h')
			{
			if (me->x != orig_x || me->y != orig_y || me->csize != orig_csize)
				me->moved = TRUE;
			else
           me->moved = FALSE;
			break;
			}

		switch (key)
			{
			case FIK_UP_ARROW:
			case FIK_DOWN_ARROW:
			case FIK_LEFT_ARROW:
			case FIK_RIGHT_ARROW:
			case FIK_CTL_UP_ARROW:
			case FIK_CTL_DOWN_ARROW:
			case FIK_CTL_LEFT_ARROW:
			case FIK_CTL_RIGHT_ARROW:
				MoveBox__Move(me, key);
				break;

			case FIK_PAGE_UP:   /* shrink */
				if (me->csize > CSIZE_MIN)
					{
					int t = me->csize - CSIZE_INC;
					int change;

					if (t < CSIZE_MIN)
						t = CSIZE_MIN;

					MoveBox__Erase(me);

					change = me->csize - t;
					me->csize = t;
					me->x += (change*16) / 2;
					me->y += (change*16) / 2;
					MoveBox__Draw(me);
					}
				break;

			case FIK_PAGE_DOWN:   /* grow */
				{
				int max_width = min(sxdots, MAX_WIDTH);

				if (me->base_depth + (me->csize + CSIZE_INC)*16 + 1 < sydots  &&
                me->base_width + (me->csize + CSIZE_INC)*16 + 1 < max_width)
					{
					MoveBox__Erase(me);
					me->x -= (CSIZE_INC*16) / 2;
					me->y -= (CSIZE_INC*16) / 2;
					me->csize += CSIZE_INC;
					if (me->y + me->base_depth + me->csize*16 + 1 > sydots)
						me->y = sydots - (me->base_depth + me->csize*16 + 1);
					if (me->x + me->base_width + me->csize*16 + 1 > max_width)
						me->x = max_width - (me->base_width + me->csize*16 + 1);
					if (me->y < 0)
						me->y = 0;
					if (me->x < 0)
						me->x = 0;
					MoveBox__Draw(me);
					}
				}
				break;
			}
		}

#ifdef XFRACT
	Cursor_EndMouseTracking();
#endif

	MoveBox__Erase(me);

	me->should_hide = (BOOLEAN)((key == 'H' || key == 'h') ? TRUE : FALSE);

	return (BOOLEAN)((key == FIK_ESC) ? FALSE : TRUE);
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
	static CEditor *CEditor_Construct(int x, int y, char letter,
                                      void (*other_key)(int,CEditor*,void*),
                                      void (*change)(CEditor*,void*), VOIDPTR info);
	static void CEditor_Destroy   (CEditor *me);
	static void CEditor_Draw      (CEditor *me);
	static void CEditor_SetPos    (CEditor *me, int x, int y);
	static void CEditor_SetVal    (CEditor *me, int val);
	static int  CEditor_GetVal    (CEditor *me);
	static void CEditor_SetDone   (CEditor *me, BOOLEAN done);
	static void CEditor_SetHidden (CEditor *me, BOOLEAN hidden);
	static int  CEditor_Edit      (CEditor *me);
#else
	static CEditor *CEditor_Construct(int , int , char ,
                                    void (*other_key)(),
                                    void (*change)(), VOIDPTR);
	static void CEditor_Destroy         (CEditor *);
	static void CEditor_Draw    (CEditor *);
	static void CEditor_SetPos  (CEditor *, int , int);
	static void CEditor_SetVal  (CEditor *, int);
	static int  CEditor_GetVal  (CEditor *);
	static void CEditor_SetDone         (CEditor *, BOOLEAN);
	static void CEditor_SetHidden (CEditor *, BOOLEAN);
	static int  CEditor_Edit    (CEditor *);
#endif

#define CEditor_WIDTH (8*3 + 4)
#define CEditor_DEPTH (8 + 4)



#ifndef XFRACT
static CEditor *CEditor_Construct(int x, int y, char letter,
                                   void (*other_key)(int,CEditor*,VOIDPTR),
                                   void (*change)(CEditor*, VOIDPTR), VOIDPTR info)
#else
static CEditor *CEditor_Construct(int x, int y, char letter,
                                   void (*other_key)(),
                                   void (*change)(), VOIDPTR info)
#endif
	{
	CEditor *me = NEWC(CEditor);

	me->x         = x;
	me->y         = y;
	me->letter    = letter;
	me->val       = 0;
	me->other_key = other_key;
	me->hidden    = FALSE;
	me->change    = change;
	me->info      = info;

	return me;
	}

#ifdef __CLINT__
#   pragma argsused   /* kills "arg not used" warning */
#endif

static void CEditor_Destroy(CEditor *me)
	{
	DELETE(me);
	}


static void CEditor_Draw(CEditor *me)
	{
	if (me->hidden)
		return;

	Cursor_Hide();
	displayf(me->x + 2, me->y + 2, fg_color, bg_color, "%c%02d", me->letter, me->val);
	Cursor_Show();
	}


static void CEditor_SetPos(CEditor *me, int x, int y)
	{
	me->x = x;
	me->y = y;
	}


static void CEditor_SetVal(CEditor *me, int val)
	{
	me->val = val;
	}


static int CEditor_GetVal(CEditor *me)
	{
	return me->val;
	}


static void CEditor_SetDone(CEditor *me, BOOLEAN done)
	{
	me->done = done;
	}


static void CEditor_SetHidden(CEditor *me, BOOLEAN hidden)
	{
	me->hidden = hidden;
	}


static int CEditor_Edit(CEditor *me)
	{
	int key = 0;
	int diff;

	me->done = FALSE;

	if (!me->hidden)
		{
		Cursor_Hide();
		rect(me->x, me->y, CEditor_WIDTH, CEditor_DEPTH, fg_color);
		Cursor_Show();
		}

#ifdef XFRACT
	Cursor_StartMouseTracking();
#endif
	while (!me->done)
		{
		Cursor_WaitKey();
		key = driver_get_key();

		switch (key)
			{
			case FIK_PAGE_UP:
				if (me->val < 63)
					{
					me->val += 5;
					if (me->val > 63)
						me->val = 63;
					CEditor_Draw(me);
					me->change(me, me->info);
					}
				break;

			case '+':
			case FIK_CTL_PLUS:        /*RB*/
				diff = 1;
				while (driver_key_pressed() == key)
					{
					driver_get_key();
					++diff;
					}
				if (me->val < 63)
					{
					me->val += diff;
					if (me->val > 63)
						me->val = 63;
					CEditor_Draw(me);
					me->change(me, me->info);
					}
				break;

			case FIK_PAGE_DOWN:
				if (me->val > 0)
					{
					me->val -= 5;
					if (me->val < 0)
						me->val = 0;
					CEditor_Draw(me);
					me->change(me, me->info);
					}
					break;

			case '-':
			case FIK_CTL_MINUS:     /*RB*/
				diff = 1;
				while (driver_key_pressed() == key)
					{
					driver_get_key();
					++diff;
					}
				if (me->val > 0)
					{
					me->val -= diff;
					if (me->val < 0)
						me->val = 0;
					CEditor_Draw(me);
					me->change(me, me->info);
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
				me->val = (key - '0')*10;
				if (me->val > 63)
					me->val = 63;
				CEditor_Draw(me);
				me->change(me, me->info);
				break;

			default:
				me->other_key(key, me, me->info);
				break;
			} /* switch */
		} /* while */
#ifdef XFRACT
	Cursor_EndMouseTracking();
#endif

	if (!me->hidden)
		{
		Cursor_Hide();
		rect(me->x, me->y, CEditor_WIDTH, CEditor_DEPTH, bg_color);
		Cursor_Show();
		}

	return key;
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

	static void     RGBEditor_Destroy  (RGBEditor *me);
	static void     RGBEditor_SetPos   (RGBEditor *me, int x, int y);
	static void     RGBEditor_SetDone  (RGBEditor *me, BOOLEAN done);
	static void     RGBEditor_SetHidden(RGBEditor *me, BOOLEAN hidden);
	static void     RGBEditor_BlankSampleBox(RGBEditor *me);
	static void     RGBEditor_Update   (RGBEditor *me);
	static void     RGBEditor_Draw     (RGBEditor *me);
	static int      RGBEditor_Edit     (RGBEditor *me);
	static void     RGBEditor_SetRGB   (RGBEditor *me, int pal, PALENTRY *rgb);
	static PALENTRY RGBEditor_GetRGB   (RGBEditor *me);

#define RGBEditor_WIDTH 62
#define RGBEditor_DEPTH (1 + 1 + CEditor_DEPTH*3-2 + 2)

#define RGBEditor_BWIDTH (RGBEditor_WIDTH - (2 + CEditor_WIDTH + 1 + 2))
#define RGBEditor_BDEPTH (RGBEditor_DEPTH - 4)



#ifndef XFRACT
static RGBEditor *RGBEditor_Construct(int x, int y, void (*other_key)(int,RGBEditor*,void*),
                                      void (*change)(RGBEditor*,void*), VOIDPTR info)
#else
static RGBEditor *RGBEditor_Construct(int x, int y, void (*other_key)(),
                                      void (*change)(), VOIDPTR info)
#endif
	{
	RGBEditor      *me     = NEWC(RGBEditor);
	static char letter[] = "RGB";
	int             ctr;

	for (ctr = 0; ctr < 3; ctr++)
		me->color[ctr] = CEditor_Construct(0, 0, letter[ctr], RGBEditor__other_key,
                                           RGBEditor__change, me);

	RGBEditor_SetPos(me, x, y);
	me->curr      = 0;
	me->pal       = 1;
	me->hidden    = FALSE;
	me->other_key = other_key;
	me->change    = change;
	me->info      = info;

	return me;
	}


static void RGBEditor_Destroy(RGBEditor *me)
	{
	CEditor_Destroy(me->color[0]);
	CEditor_Destroy(me->color[1]);
	CEditor_Destroy(me->color[2]);
	DELETE(me);
	}


static void RGBEditor_SetDone(RGBEditor *me, BOOLEAN done)
	{
	me->done = done;
	}


static void RGBEditor_SetHidden(RGBEditor *me, BOOLEAN hidden)
	{
	me->hidden = hidden;
	CEditor_SetHidden(me->color[0], hidden);
	CEditor_SetHidden(me->color[1], hidden);
	CEditor_SetHidden(me->color[2], hidden);
	}


static void RGBEditor__other_key(int key, CEditor *ceditor, VOIDPTR info) /* private */
	{
	RGBEditor *me = (RGBEditor *)info;

	switch (key)
		{
		case 'R':
		case 'r':
			if (me->curr != 0)
				{
				me->curr = 0;
				CEditor_SetDone(ceditor, TRUE);
				}
			break;

		case 'G':
		case 'g':
			if (me->curr != 1)
				{
				me->curr = 1;
				CEditor_SetDone(ceditor, TRUE);
				}
			break;

		case 'B':
		case 'b':
			if (me->curr != 2)
				{
				me->curr = 2;
				CEditor_SetDone(ceditor, TRUE);
				}
			break;

		case FIK_DELETE:   /* move to next CEditor */
		case FIK_CTL_ENTER_2:    /*double click rt mouse also! */
			if (++me->curr > 2)
				me->curr = 0;
			CEditor_SetDone(ceditor, TRUE);
			break;

		case FIK_INSERT:   /* move to prev CEditor */
			if (--me->curr < 0)
				me->curr = 2;
			CEditor_SetDone(ceditor, TRUE);
			break;

		default:
			me->other_key(key, me, me->info);
			if (me->done)
				CEditor_SetDone(ceditor, TRUE);
			break;
		}
	}

#ifdef __CLINT__
#   pragma argsused   /* kills "arg not used" warning */
#endif

static void RGBEditor__change(CEditor *ceditor, VOIDPTR info) /* private */
	{
	RGBEditor *me = (RGBEditor *)info;

	ceditor = NULL; /* just for warning */
	if (me->pal < colors && !is_reserved(me->pal))
		setpal(me->pal, CEditor_GetVal(me->color[0]),
          CEditor_GetVal(me->color[1]), CEditor_GetVal(me->color[2]));

	me->change(me, me->info);
	}


static void RGBEditor_SetPos(RGBEditor *me, int x, int y)
	{
	me->x = x;
	me->y = y;

	CEditor_SetPos(me->color[0], x + 2, y + 2);
	CEditor_SetPos(me->color[1], x + 2, y + 2 + CEditor_DEPTH-1);
	CEditor_SetPos(me->color[2], x + 2, y + 2 + CEditor_DEPTH-1 + CEditor_DEPTH-1);
	}


static void RGBEditor_BlankSampleBox(RGBEditor *me)
	{
	if (me->hidden)
		return ;

	Cursor_Hide();
	fillrect(me->x + 2 + CEditor_WIDTH + 1 + 1, me->y + 2 + 1, RGBEditor_BWIDTH-2, RGBEditor_BDEPTH-2, bg_color);
	Cursor_Show();
	}


static void RGBEditor_Update(RGBEditor *me)
	{
	int x1 = me->x + 2 + CEditor_WIDTH + 1 + 1,
       y1 = me->y + 2 + 1;

	if (me->hidden)
		return ;

	Cursor_Hide();

	if (me->pal >= colors)
		{
		fillrect(x1, y1, RGBEditor_BWIDTH-2, RGBEditor_BDEPTH-2, bg_color);
		draw_diamond(x1 + (RGBEditor_BWIDTH-5)/2, y1 + (RGBEditor_BDEPTH-5)/2, fg_color);
		}

	else if (is_reserved(me->pal))
		{
		int x2 = x1 + RGBEditor_BWIDTH-3,
          y2 = y1 + RGBEditor_BDEPTH-3;

		fillrect(x1, y1, RGBEditor_BWIDTH-2, RGBEditor_BDEPTH-2, bg_color);
		driver_draw_line(x1, y1, x2, y2, fg_color);
		driver_draw_line(x1, y2, x2, y1, fg_color);
		}
	else
		fillrect(x1, y1, RGBEditor_BWIDTH-2, RGBEditor_BDEPTH-2, me->pal);

	CEditor_Draw(me->color[0]);
	CEditor_Draw(me->color[1]);
	CEditor_Draw(me->color[2]);
	Cursor_Show();
	}


static void RGBEditor_Draw(RGBEditor *me)
	{
	if (me->hidden)
		return ;

	Cursor_Hide();
	drect(me->x, me->y, RGBEditor_WIDTH, RGBEditor_DEPTH);
	fillrect(me->x + 1, me->y + 1, RGBEditor_WIDTH-2, RGBEditor_DEPTH-2, bg_color);
	rect(me->x + 1 + CEditor_WIDTH + 2, me->y + 2, RGBEditor_BWIDTH, RGBEditor_BDEPTH, fg_color);
	RGBEditor_Update(me);
	Cursor_Show();
	}


static int RGBEditor_Edit(RGBEditor *me)
	{
	int key = 0;

	me->done = FALSE;

	if (!me->hidden)
		{
		Cursor_Hide();
		rect(me->x, me->y, RGBEditor_WIDTH, RGBEditor_DEPTH, fg_color);
		Cursor_Show();
		}

	while (!me->done)
		key = CEditor_Edit(me->color[me->curr]);

	if (!me->hidden)
		{
		Cursor_Hide();
		drect(me->x, me->y, RGBEditor_WIDTH, RGBEditor_DEPTH);
		Cursor_Show();
		}

	return key;
	}


static void RGBEditor_SetRGB(RGBEditor *me, int pal, PALENTRY *rgb)
	{
	me->pal = pal;
	CEditor_SetVal(me->color[0], rgb->red);
	CEditor_SetVal(me->color[1], rgb->green);
	CEditor_SetVal(me->color[2], rgb->blue);
	}


static PALENTRY RGBEditor_GetRGB(RGBEditor *me)
	{
	PALENTRY pal;

	pal.red   = (BYTE)CEditor_GetVal(me->color[0]);
	pal.green = (BYTE)CEditor_GetVal(me->color[1]);
	pal.blue  = (BYTE)CEditor_GetVal(me->color[2]);

	return pal;
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
	char     *memory;

	PALENTRY *save_pal[8];


	PALENTRY      fs_color;
	int           top,bottom; /* top and bottom colours of freestyle band */
	int           bandwidth; /*size of freestyle colour band */
	BOOLEAN       freestyle;
	} ;

#define PalTable struct _PalTable

/* private: */

	static void    PalTable__DrawStatus  (PalTable *me, BOOLEAN stripe_mode);
	static void    PalTable__HlPal       (PalTable *me, int pnum, int color);
	static void    PalTable__Draw        (PalTable *me);
	static BOOLEAN PalTable__SetCurr     (PalTable *me, int which, int curr);
	static BOOLEAN PalTable__MemoryAlloc (PalTable *me, long size);
	static void    PalTable__SaveRect    (PalTable *me);
	static void    PalTable__RestoreRect (PalTable *me);
	static void    PalTable__SetPos      (PalTable *me, int x, int y);
	static void    PalTable__SetCSize    (PalTable *me, int csize);
	static int     PalTable__GetCursorColor(PalTable *me);
	static void    PalTable__DoCurs      (PalTable *me, int key);
	static void    PalTable__Rotate      (PalTable *me, int dir, int lo, int hi);
	static void    PalTable__UpdateDAC   (PalTable *me);
	static void    PalTable__other_key   (int key, RGBEditor *rgb, VOIDPTR info);
	static void    PalTable__SaveUndoData(PalTable *me, int first, int last);
	static void    PalTable__SaveUndoRotate(PalTable *me, int dir, int first, int last);
	static void    PalTable__UndoProcess (PalTable *me, int delta);
	static void    PalTable__Undo        (PalTable *me);
	static void    PalTable__Redo        (PalTable *me);
	static void    PalTable__change      (RGBEditor *rgb, VOIDPTR info);

/* public: */

	static PalTable *PalTable_Construct (void);
	static void      PalTable_Destroy   (PalTable *me);
	static void      PalTable_Process   (PalTable *me);
	static void      PalTable_SetHidden (PalTable *me, BOOLEAN hidden);
	static void      PalTable_Hide      (PalTable *me, RGBEditor *rgb, BOOLEAN hidden);


#define PalTable_PALX (1)
#define PalTable_PALY (2 + RGBEditor_DEPTH + 2)

#define UNDO_DATA        (1)
#define UNDO_DATA_SINGLE (2)
#define UNDO_ROTATE      (3)

/*  - Freestyle code - */

static void PalTable__CalcTopBottom(PalTable *me)
	{
	if (me->curr[me->active] < me->bandwidth)
		me->bottom = 0;
	else
		me->bottom = (me->curr[me->active]) - me->bandwidth;

	if (me->curr[me->active] > (255-me->bandwidth))
		me->top = 255;
	else
		me->top = (me->curr[me->active]) + me->bandwidth;
	}

static void PalTable__PutBand(PalTable *me, PALENTRY *pal)
	{
	int r,b,a;

  /* clip top and bottom values to stop them running off the end of the DAC */

	PalTable__CalcTopBottom(me);

  /* put bands either side of current colour */

	a = me->curr[me->active];
	b = me->bottom;
	r = me->top;

	pal[a] = me->fs_color;

	if (r != a && a != b)
		{
		mkpalrange(&pal[a], &pal[r], &pal[a], r-a, 1);
		mkpalrange(&pal[b], &pal[a], &pal[b], a-b, 1);
		}

	}


/* - Undo.Redo code - */


static void PalTable__SaveUndoData(PalTable *me, int first, int last)
	{
	int num;

	if (me->undo_file == NULL)
		return ;

	num = (last - first) + 1;

#ifdef DEBUG_UNDO
	mprintf("%6ld Writing Undo DATA from %d to %d (%d)", ftell(me->undo_file), first, last, num);
#endif

	fseek(me->undo_file, 0, SEEK_CUR);
	if (num == 1)
		{
		putc(UNDO_DATA_SINGLE, me->undo_file);
		putc(first, me->undo_file);
		fwrite(me->pal + first, 3, 1, me->undo_file);
		putw(1 + 1 + 3 + sizeof(int), me->undo_file);
		}
	else
		{
		putc(UNDO_DATA, me->undo_file);
		putc(first, me->undo_file);
		putc(last,  me->undo_file);
		fwrite(me->pal + first, 3, num, me->undo_file);
		putw(1 + 2 + (num*3) + sizeof(int), me->undo_file);
		}

	me->num_redo = 0;
	}


static void PalTable__SaveUndoRotate(PalTable *me, int dir, int first, int last)
	{
	if (me->undo_file == NULL)
		return ;

#ifdef DEBUG_UNDO
	mprintf("%6ld Writing Undo ROTATE of %d from %d to %d", ftell(me->undo_file), dir, first, last);
#endif

	fseek(me->undo_file, 0, SEEK_CUR);
	putc(UNDO_ROTATE, me->undo_file);
	putc(first, me->undo_file);
	putc(last,  me->undo_file);
	putw(dir, me->undo_file);
	putw(1 + 2 + sizeof(int), me->undo_file);

	me->num_redo = 0;
	}


static void PalTable__UndoProcess(PalTable *me, int delta)   /* undo/redo common code */
	{              /* delta = -1 for undo, +1 for redo */
	int cmd = getc(me->undo_file);

	switch (cmd)
		{
		case UNDO_DATA:
		case UNDO_DATA_SINGLE:
			{
			int      first, last, num;
			PALENTRY temp[256];

			if (cmd == UNDO_DATA)
				{
				first = (unsigned char)getc(me->undo_file);
				last  = (unsigned char)getc(me->undo_file);
				}
			else  /* UNDO_DATA_SINGLE */
				first = last = (unsigned char)getc(me->undo_file);

			num = (last - first) + 1;

#ifdef DEBUG_UNDO
			mprintf("          Reading DATA from %d to %d", first, last);
#endif

			fread(temp, 3, num, me->undo_file);

			fseek(me->undo_file, -(num*3), SEEK_CUR);  /* go to start of undo/redo data */
			fwrite(me->pal + first, 3, num, me->undo_file);  /* write redo/undo data */

			memmove(me->pal + first, temp, num*3);

			PalTable__UpdateDAC(me);

			RGBEditor_SetRGB(me->rgb[0], me->curr[0], &(me->pal[me->curr[0]]));
			RGBEditor_SetRGB(me->rgb[1], me->curr[1], &(me->pal[me->curr[1]]));
			RGBEditor_Update(me->rgb[0]);
			RGBEditor_Update(me->rgb[1]);
			break;
			}

		case UNDO_ROTATE:
			{
			int first = (unsigned char)getc(me->undo_file);
			int last  = (unsigned char)getc(me->undo_file);
			int dir   = getw(me->undo_file);

#ifdef DEBUG_UNDO
			mprintf("          Reading ROTATE of %d from %d to %d", dir, first, last);
#endif
			PalTable__Rotate(me, delta*dir, first, last);
			break;
			}

		default:
#ifdef DEBUG_UNDO
			mprintf("          Unknown command: %d", cmd);
#endif
			break;
		}

	fseek(me->undo_file, 0, SEEK_CUR);  /* to put us in read mode */
	getw(me->undo_file);  /* read size */
	}


static void PalTable__Undo(PalTable *me)
	{
	int  size;
	long pos;

	if (ftell(me->undo_file) <= 0)   /* at beginning of file? */
		{                                  /*   nothing to undo -- exit */
		return ;
		}

	fseek(me->undo_file, -(int)sizeof(int), SEEK_CUR);  /* go back to get size */

	size = getw(me->undo_file);
	fseek(me->undo_file, -size, SEEK_CUR);   /* go to start of undo */

#ifdef DEBUG_UNDO
	mprintf("%6ld Undo:", ftell(me->undo_file));
#endif

	pos = ftell(me->undo_file);

	PalTable__UndoProcess(me, -1);

	fseek(me->undo_file, pos, SEEK_SET);   /* go to start of me block */

	++me->num_redo;
	}


static void PalTable__Redo(PalTable *me)
	{
	if (me->num_redo <= 0)
		return ;

#ifdef DEBUG_UNDO
	mprintf("%6ld Redo:", ftell(me->undo_file));
#endif

	fseek(me->undo_file, 0, SEEK_CUR);  /* to make sure we are in "read" mode */
	PalTable__UndoProcess(me, 1);

	--me->num_redo;
	}


/* - everything else - */


#define STATUS_LEN (4)

static void PalTable__DrawStatus(PalTable *me, BOOLEAN stripe_mode)
	{
	int color;
	int width = 1 + (me->csize*16) + 1 + 1;

	if (!me->hidden && (width - (RGBEditor_WIDTH*2 + 4) >= STATUS_LEN*8))
		{
		int x = me->x + 2 + RGBEditor_WIDTH,
          y = me->y + PalTable_PALY - 10;
		color = PalTable__GetCursorColor(me);
		if (color < 0 || color >= colors) /* hmm, the border returns -1 */
			color = 0;
		Cursor_Hide();

		{
			char buff[80];
			sprintf(buff, "%c%c%c%c",
				me->auto_select ? 'A' : ' ',
				(me->exclude == 1)  ? 'X' : (me->exclude == 2) ? 'Y' : ' ',
				me->freestyle ? 'F' : ' ',
				stripe_mode ? 'T' : ' ');
			driver_display_string(x, y, fg_color, bg_color, buff);

			y = y - 10;
			sprintf(buff, "%d", color);
			driver_display_string(x, y, fg_color, bg_color, buff);
		}
		Cursor_Show();
		}
	}


static void PalTable__HlPal(PalTable *me, int pnum, int color)
	{
	int x    = me->x + PalTable_PALX + (pnum%16)*me->csize,
       y    = me->y + PalTable_PALY + (pnum/16)*me->csize,
       size = me->csize;

	if (me->hidden)
		return ;

	Cursor_Hide();

	if (color < 0)
		drect(x, y, size + 1, size + 1);
	else
		rect(x, y, size + 1, size + 1, color);

	Cursor_Show();
	}


static void PalTable__Draw(PalTable *me)
	{
	int pal;
	int xoff, yoff;
	int width;

	if (me->hidden)
		return ;

	Cursor_Hide();

	width = 1 + (me->csize*16) + 1 + 1;

	rect(me->x, me->y, width, 2 + RGBEditor_DEPTH + 2 + (me->csize*16) + 1 + 1, fg_color);

	fillrect(me->x + 1, me->y + 1, width-2, 2 + RGBEditor_DEPTH + 2 + (me->csize*16) + 1 + 1-2, bg_color);

	hline(me->x, me->y + PalTable_PALY-1, width, fg_color);

	if (width - (RGBEditor_WIDTH*2 + 4) >= TITLE_LEN*8)
		{
		int center = (width - TITLE_LEN*8) / 2;

		displayf(me->x + center, me->y + RGBEditor_DEPTH/2-6, fg_color, bg_color, TITLE);
		}

	RGBEditor_Draw(me->rgb[0]);
	RGBEditor_Draw(me->rgb[1]);

	for (pal = 0; pal < 256; pal++)
		{
		xoff = PalTable_PALX + (pal%16)*me->csize;
		yoff = PalTable_PALY + (pal/16)*me->csize;

		if (pal >= colors)
			{
			fillrect(me->x + xoff + 1, me->y + yoff + 1, me->csize-1, me->csize-1, bg_color);
			draw_diamond(me->x + xoff + me->csize/2 - 1, me->y + yoff + me->csize/2 - 1, fg_color);
			}

		else if (is_reserved(pal))
			{
			int x1 = me->x + xoff + 1,
             y1 = me->y + yoff + 1,
             x2 = x1 + me->csize - 2,
             y2 = y1 + me->csize - 2;
			fillrect(me->x + xoff + 1, me->y + yoff + 1, me->csize-1, me->csize-1, bg_color);
			driver_draw_line(x1, y1, x2, y2, fg_color);
			driver_draw_line(x1, y2, x2, y1, fg_color);
			}
		else
			fillrect(me->x + xoff + 1, me->y + yoff + 1, me->csize-1, me->csize-1, pal);

		}

	if (me->active == 0)
		{
		PalTable__HlPal(me, me->curr[1], -1);
		PalTable__HlPal(me, me->curr[0], fg_color);
		}
	else
		{
		PalTable__HlPal(me, me->curr[0], -1);
		PalTable__HlPal(me, me->curr[1], fg_color);
		}

	PalTable__DrawStatus(me, FALSE);

	Cursor_Show();
	}



static BOOLEAN PalTable__SetCurr(PalTable *me, int which, int curr)
	{
	BOOLEAN redraw = (BOOLEAN)((which < 0) ? TRUE : FALSE);

	if (redraw)
		{
		which = me->active;
		curr = me->curr[which];
		}
	else
		if (curr == me->curr[which] || curr < 0)
			return FALSE;

	Cursor_Hide();

	PalTable__HlPal(me, me->curr[0], bg_color);
	PalTable__HlPal(me, me->curr[1], bg_color);
	PalTable__HlPal(me, me->top,     bg_color);
	PalTable__HlPal(me, me->bottom,  bg_color);

	if (me->freestyle)
		{
		me->curr[which] = curr;

		PalTable__CalcTopBottom(me);

		PalTable__HlPal(me, me->top,    -1);
		PalTable__HlPal(me, me->bottom, -1);
		PalTable__HlPal(me, me->curr[me->active], fg_color);

		RGBEditor_SetRGB(me->rgb[which], me->curr[which], &me->fs_color);
		RGBEditor_Update(me->rgb[which]);

		PalTable__UpdateDAC(me);

		Cursor_Show();

		return TRUE;
		}

	me->curr[which] = curr;

	if (me->curr[0] != me->curr[1])
		PalTable__HlPal(me, me->curr[me->active == 0?1:0], -1);
	PalTable__HlPal(me, me->curr[me->active], fg_color);

	RGBEditor_SetRGB(me->rgb[which], me->curr[which], &(me->pal[me->curr[which]]));

	if (redraw)
		{
		int other = (which == 0) ? 1 : 0;
		RGBEditor_SetRGB(me->rgb[other], me->curr[other], &(me->pal[me->curr[other]]));
		RGBEditor_Update(me->rgb[0]);
		RGBEditor_Update(me->rgb[1]);
		}
	else
		RGBEditor_Update(me->rgb[which]);

	if (me->exclude)
		PalTable__UpdateDAC(me);

	Cursor_Show();

	me->curr_changed = FALSE;

	return TRUE;
	}


static BOOLEAN PalTable__MemoryAlloc(PalTable *me, long size)
	{
	char *temp;

	if (debugflag == 420)
		{
		me->stored_at = NOWHERE;
		return FALSE;   /* can't do it */
		}
	temp = (char *)malloc(FAR_RESERVE);   /* minimum free space */

	if (temp == NULL)
		{
		me->stored_at = NOWHERE;
		return FALSE;   /* can't do it */
		}

	me->memory = (char *)malloc(size);

	free(temp);

	if (me->memory == NULL)
		{
		me->stored_at = NOWHERE;
		return FALSE;
		}
	else
		{
		me->stored_at = MEMORY;
		return TRUE;
		}
	}


static void PalTable__SaveRect(PalTable *me)
	{
	char buff[MAX_WIDTH];
	int  width = PalTable_PALX + me->csize*16 + 1 + 1,
		depth = PalTable_PALY + me->csize*16 + 1 + 1;
	int  yoff;


	/* first, do any de-allocationg */

	switch (me->stored_at)
		{
		case NOWHERE:
			break;

		case DISK:
			break;

		case MEMORY:
			if (me->memory != NULL)
				free(me->memory);
			me->memory = NULL;
			break;
		} ;

	/* allocate space and store the rect */

	if (PalTable__MemoryAlloc(me, (long)width*depth))
		{
		char  *ptr = me->memory;
		char  *bufptr = buff; /* MSC needs me indirection to get it right */

		Cursor_Hide();
		for (yoff = 0; yoff < depth; yoff++)
			{
			getrow(me->x, me->y + yoff, width, buff);
			hline (me->x, me->y + yoff, width, bg_color);
			memcpy(ptr,bufptr, width);
			ptr += width;
			}
		Cursor_Show();
		}

	else /* to disk */
		{
		me->stored_at = DISK;

		if (me->file == NULL)
			{
			me->file = dir_fopen(tempdir,scrnfile, "w + b");
			if (me->file == NULL)
				{
				me->stored_at = NOWHERE;
				driver_buzzer(BUZZER_ERROR);
				return ;
				}
			}

		rewind(me->file);
		Cursor_Hide();
		for (yoff = 0; yoff < depth; yoff++)
			{
			getrow(me->x, me->y + yoff, width, buff);
			hline (me->x, me->y + yoff, width, bg_color);
			if (fwrite(buff, width, 1, me->file) != 1)
				{
				driver_buzzer(BUZZER_ERROR);
				break;
				}
			}
		Cursor_Show();
		}

	}


static void PalTable__RestoreRect(PalTable *me)
	{
	char buff[MAX_WIDTH];
	int  width = PalTable_PALX + me->csize*16 + 1 + 1,
		depth = PalTable_PALY + me->csize*16 + 1 + 1;
	int  yoff;

	if (me->hidden)
		return;

	switch (me->stored_at)
		{
		case DISK:
			rewind(me->file);
			Cursor_Hide();
			for (yoff = 0; yoff < depth; yoff++)
				{
				if (fread(buff, width, 1, me->file) != 1)
					{
					driver_buzzer(BUZZER_ERROR);
					break;
					}
				putrow(me->x, me->y + yoff, width, buff);
				}
			Cursor_Show();
			break;

		case MEMORY:
			{
			char  *ptr = me->memory;
			char  *bufptr = buff; /* MSC needs me indirection to get it right */

			Cursor_Hide();
			for (yoff = 0; yoff < depth; yoff++)
				{
				memcpy(bufptr, ptr, width);
				putrow(me->x, me->y + yoff, width, buff);
				ptr += width;
				}
			Cursor_Show();
			break;
			}

		case NOWHERE:
			break;
		} /* switch */
	}


static void PalTable__SetPos(PalTable *me, int x, int y)
	{
	int width = PalTable_PALX + me->csize*16 + 1 + 1;

	me->x = x;
	me->y = y;

	RGBEditor_SetPos(me->rgb[0], x + 2, y + 2);
	RGBEditor_SetPos(me->rgb[1], x + width-2-RGBEditor_WIDTH, y + 2);
	}


static void PalTable__SetCSize(PalTable *me, int csize)
	{
	me->csize = csize;
	PalTable__SetPos(me, me->x, me->y);
	}


static int PalTable__GetCursorColor(PalTable *me)
	{
	int x     = Cursor_GetX(),
       y     = Cursor_GetY(),
       size;
	int color = getcolor(x, y);

	if (is_reserved(color))
		{
		if (is_in_box(x, y, me->x, me->y, 1 + (me->csize*16) + 1 + 1, 2 + RGBEditor_DEPTH + 2 + (me->csize*16) + 1 + 1))
			{  /* is the cursor over the editor? */
			x -= me->x + PalTable_PALX;
			y -= me->y + PalTable_PALY;
			size = me->csize;

			if (x < 0 || y < 0 || x > size*16 || y > size*16)
				return -1;

			if (x == size*16)
				--x;
			if (y == size*16)
				--y;

			return (y/size)*16 + x/size;
			}
		else
			return color;
		}

	return color;
	}



#define CURS_INC 1

static void PalTable__DoCurs(PalTable *me, int key)
	{
	BOOLEAN done  = FALSE;
	BOOLEAN first = TRUE;
	int     xoff  = 0,
           yoff  = 0;

	while (!done)
		{
		switch (key)
			{
			case FIK_CTL_RIGHT_ARROW:     xoff += CURS_INC*4;   break;
			case FIK_RIGHT_ARROW:       xoff += CURS_INC;     break;
			case FIK_CTL_LEFT_ARROW:      xoff -= CURS_INC*4;   break;
			case FIK_LEFT_ARROW:        xoff -= CURS_INC;     break;
			case FIK_CTL_DOWN_ARROW:      yoff += CURS_INC*4;   break;
			case FIK_DOWN_ARROW:        yoff += CURS_INC;     break;
			case FIK_CTL_UP_ARROW:        yoff -= CURS_INC*4;   break;
			case FIK_UP_ARROW:          yoff -= CURS_INC;     break;

			default:
				done = TRUE;
			}

		if (!done)
			{
			if (!first)
				driver_get_key();       /* delete key from buffer */
			else
				first = FALSE;
			key = driver_key_pressed();   /* peek at the next one... */
			}
		}

	Cursor_Move(xoff, yoff);

	if (me->auto_select)
		PalTable__SetCurr(me, me->active, PalTable__GetCursorColor(me));
	}


#ifdef __CLINT__
#   pragma argsused
#endif

static void PalTable__change(RGBEditor *rgb, VOIDPTR info)
	{
	PalTable *me = (PalTable *)info;
	int       pnum = me->curr[me->active];

	if (me->freestyle)
		{
		me->fs_color = RGBEditor_GetRGB(rgb);
		PalTable__UpdateDAC(me);
		return ;
		}

	if (!me->curr_changed)
		{
		PalTable__SaveUndoData(me, pnum, pnum);
		me->curr_changed = TRUE;
		}

	me->pal[pnum] = RGBEditor_GetRGB(rgb);

	if (me->curr[0] == me->curr[1])
		{
		int      other = me->active == 0 ? 1 : 0;
		PALENTRY color;

		color = RGBEditor_GetRGB(me->rgb[me->active]);
		RGBEditor_SetRGB(me->rgb[other], me->curr[other], &color);

		Cursor_Hide();
		RGBEditor_Update(me->rgb[other]);
		Cursor_Show();
		}

	}


static void PalTable__UpdateDAC(PalTable *me)
	{
	if (me->exclude)
		{
		memset(g_dac_box, 0, 256*3);
		if (me->exclude == 1)
			{
			int a = me->curr[me->active];
			memmove(g_dac_box[a], &me->pal[a], 3);
			}
		else
			{
			int a = me->curr[0],
             b = me->curr[1];

			if (a > b)
				{
				int t=a;
				a=b;
				b=t;
				}

			memmove(g_dac_box[a], &me->pal[a], 3*(1 + (b-a)));
			}
		}
	else
		{
		memmove(g_dac_box[0], me->pal, 3*colors);

		if (me->freestyle)
			PalTable__PutBand(me, (PALENTRY *)g_dac_box);   /* apply band to g_dac_box */
		}

	if (!me->hidden)
		{
		if (inverse)
			{
			memset(g_dac_box[fg_color], 0, 3);         /* g_dac_box[fg] = (0,0,0) */
			memset(g_dac_box[bg_color], 48, 3);        /* g_dac_box[bg] = (48,48,48) */
			}
		else
			{
			memset(g_dac_box[bg_color], 0, 3);         /* g_dac_box[bg] = (0,0,0) */
			memset(g_dac_box[fg_color], 48, 3);        /* g_dac_box[fg] = (48,48,48) */
			}
		}

	spindac(0,1);
	}


static void PalTable__Rotate(PalTable *me, int dir, int lo, int hi)
	{

	rotatepal(me->pal, dir, lo, hi);

	Cursor_Hide();

	/* update the DAC.  */

	PalTable__UpdateDAC(me);

	/* update the editors. */

	RGBEditor_SetRGB(me->rgb[0], me->curr[0], &(me->pal[me->curr[0]]));
	RGBEditor_SetRGB(me->rgb[1], me->curr[1], &(me->pal[me->curr[1]]));
	RGBEditor_Update(me->rgb[0]);
	RGBEditor_Update(me->rgb[1]);

	Cursor_Show();
	}


static void PalTable__other_key(int key, RGBEditor *rgb, VOIDPTR info)
	{
	PalTable *me = (PalTable *)info;

	switch (key)
		{
		case '\\':    /* move/resize */
			{
			if (me->hidden)
				break;           /* cannot move a hidden pal */
			Cursor_Hide();
			PalTable__RestoreRect(me);
			MoveBox_SetPos(me->movebox, me->x, me->y);
			MoveBox_SetCSize(me->movebox, me->csize);
			if (MoveBox_Process(me->movebox))
				{
				if (MoveBox_ShouldHide(me->movebox))
					PalTable_SetHidden(me, TRUE);
				else if (MoveBox_Moved(me->movebox))
					{
					PalTable__SetPos(me, MoveBox_X(me->movebox), MoveBox_Y(me->movebox));
					PalTable__SetCSize(me, MoveBox_CSize(me->movebox));
					PalTable__SaveRect(me);
					}
				}
			PalTable__Draw(me);
			Cursor_Show();

			RGBEditor_SetDone(me->rgb[me->active], TRUE);

			if (me->auto_select)
				PalTable__SetCurr(me, me->active, PalTable__GetCursorColor(me));
			break;
			}

		case 'Y':    /* exclude range */
		case 'y':
			if (me->exclude == 2)
				me->exclude = 0;
			else
				me->exclude = 2;
			PalTable__UpdateDAC(me);
			break;

		case 'X':
		case 'x':     /* exclude current entry */
			if (me->exclude == 1)
				me->exclude = 0;
			else
				me->exclude = 1;
			PalTable__UpdateDAC(me);
			break;

		case FIK_RIGHT_ARROW:
		case FIK_LEFT_ARROW:
		case FIK_UP_ARROW:
		case FIK_DOWN_ARROW:
		case FIK_CTL_RIGHT_ARROW:
		case FIK_CTL_LEFT_ARROW:
		case FIK_CTL_UP_ARROW:
		case FIK_CTL_DOWN_ARROW:
			PalTable__DoCurs(me, key);
			break;

		case FIK_ESC:
			me->done = TRUE;
			RGBEditor_SetDone(rgb, TRUE);
			break;

		case ' ':     /* select the other palette register */
			me->active = (me->active == 0) ? 1 : 0;
			if (me->auto_select)
				PalTable__SetCurr(me, me->active, PalTable__GetCursorColor(me));
          else
				PalTable__SetCurr(me, -1, 0);

			if (me->exclude || me->freestyle)
				PalTable__UpdateDAC(me);

			RGBEditor_SetDone(rgb, TRUE);
			break;

		case FIK_ENTER:    /* set register to color under cursor.  useful when not */
		case FIK_ENTER_2:  /* in auto_select mode */

			if (me->freestyle)
				{
				PalTable__SaveUndoData(me, me->bottom, me->top);
				PalTable__PutBand(me, me->pal);
				}

			PalTable__SetCurr(me, me->active, PalTable__GetCursorColor(me));

			if (me->exclude || me->freestyle)
				PalTable__UpdateDAC(me);

			RGBEditor_SetDone(rgb, TRUE);
			break;

		case 'D':    /* copy (Duplicate?) color in inactive to color in active */
		case 'd':
			{
			int   a = me->active,
						b = (a == 0) ? 1 : 0;
			PALENTRY t;

			t = RGBEditor_GetRGB(me->rgb[b]);
			Cursor_Hide();

			RGBEditor_SetRGB(me->rgb[a], me->curr[a], &t);
			RGBEditor_Update(me->rgb[a]);
			PalTable__change(me->rgb[a], me);
			PalTable__UpdateDAC(me);

			Cursor_Show();
			break;
			}

		case '=':    /* create a shade range between the two entries */
			{
			int a = me->curr[0],
             b = me->curr[1];

			if (a > b)
				{
				int t = a;
				a = b;
				b = t;
				}

			PalTable__SaveUndoData(me, a, b);

			if (a != b)
				{
				mkpalrange(&me->pal[a], &me->pal[b], &me->pal[a], b-a, 1);
				PalTable__UpdateDAC(me);
				}

			break;
			}

		case '!':    /* swap r<->g */
			{
			int a = me->curr[0],
             b = me->curr[1];

			if (a > b)
				{
				int t = a;
				a = b;
				b = t;
				}

			PalTable__SaveUndoData(me, a, b);

			if (a != b)
				{
				rotcolrg(&me->pal[a], b-a);
				PalTable__UpdateDAC(me);
				}


			break;
			}

		case '@':    /* swap g<->b */
		case '"':    /* UK keyboards */
		case 151:    /* French keyboards */
			{
			int a = me->curr[0],
             b = me->curr[1];

			if (a > b)
				{
				int t = a;
				a = b;
				b = t;
				}

			PalTable__SaveUndoData(me, a, b);

			if (a != b)
				{
				rotcolgb(&me->pal[a], b-a);
				PalTable__UpdateDAC(me);
				}

			break;
			}

		case '#':    /* swap r<->b */
		case 156:    /* UK keyboards (pound sign) */
		case '$':    /* For French keyboards */
			{
			int a = me->curr[0],
             b = me->curr[1];

			if (a > b)
				{
				int t = a;
				a = b;
				b = t;
				}

			PalTable__SaveUndoData(me, a, b);

			if (a != b)
				{
				rotcolbr(&me->pal[a], b-a);
				PalTable__UpdateDAC(me);
				}

			break;
			}


		case 'T':
		case 't':   /* s(T)ripe mode */
			{
			int key;

			Cursor_Hide();
			PalTable__DrawStatus(me, TRUE);
			key = getakeynohelp();
			Cursor_Show();

			if (key >= '1' && key <= '9')
				{
				int a = me->curr[0],
                b = me->curr[1];

				if (a > b)
					{
					int t = a;
					a = b;
					b = t;
					}

				PalTable__SaveUndoData(me, a, b);

				if (a != b)
					{
					mkpalrange(&me->pal[a], &me->pal[b], &me->pal[a], b-a, key-'0');
					PalTable__UpdateDAC(me);
					}
				}

			break;
			}

		case 'M':   /* set gamma */
		case 'm':
          {
              int i;
              char buf[20];
              sprintf(buf,"%.3f",1./gamma_val);
              driver_stack_screen();
              i = field_prompt("Enter gamma value",NULL,buf,20,NULL);
              driver_unstack_screen();
              if (i != -1) 
              {
						sscanf(buf,"%f",&gamma_val);
						if (gamma_val == 0) 
						{
                      gamma_val = 0.0000000001f;
						}
						gamma_val = (float)(1./gamma_val);
              }
          }
          break;
		case 'A':   /* toggle auto-select mode */
		case 'a':
			me->auto_select = (BOOLEAN)((me->auto_select) ? FALSE : TRUE);
			if (me->auto_select)
				{
				PalTable__SetCurr(me, me->active, PalTable__GetCursorColor(me));
				if (me->exclude)
					PalTable__UpdateDAC(me);
				}
			break;

		case 'H':
		case 'h': /* toggle hide/display of palette editor */
			Cursor_Hide();
			PalTable_Hide(me, rgb, (BOOLEAN)((me->hidden) ? FALSE : TRUE));
			Cursor_Show();
			break;

		case '.':   /* rotate once */
		case ',':
			{
			int dir = (key == '.') ? 1 : -1;

			PalTable__SaveUndoRotate(me, dir, rotate_lo, rotate_hi);
			PalTable__Rotate(me, dir, rotate_lo, rotate_hi);
			break;
			}

		case '>':   /* continuous rotation (until a key is pressed) */
		case '<':
			{
			int  dir;
			long tick;
			int  diff = 0;

			Cursor_Hide();

			if (!me->hidden)
				{
				RGBEditor_BlankSampleBox(me->rgb[0]);
				RGBEditor_BlankSampleBox(me->rgb[1]);
				RGBEditor_SetHidden(me->rgb[0], TRUE);
				RGBEditor_SetHidden(me->rgb[1], TRUE);
				}

			do
				{
				dir = (key == '>') ? 1 : -1;

				while (!driver_key_pressed())
					{
					tick = readticker();
					PalTable__Rotate(me, dir, rotate_lo, rotate_hi);
					diff += dir;
					while (readticker() == tick) ;   /* wait until a tick passes */
					}

				key = driver_get_key();
				}
			while (key == '<' || key == '>');

			if (!me->hidden)
				{
				RGBEditor_SetHidden(me->rgb[0], FALSE);
				RGBEditor_SetHidden(me->rgb[1], FALSE);
				RGBEditor_Update(me->rgb[0]);
				RGBEditor_Update(me->rgb[1]);
				}

			if (diff != 0)
				PalTable__SaveUndoRotate(me, diff, rotate_lo, rotate_hi);

			Cursor_Show();
			break;
			}

		case 'I':     /* invert the fg & bg colors */
		case 'i':
		inverse = (BOOLEAN)!inverse;
		PalTable__UpdateDAC(me);
		break;

		case 'V':
		case 'v':  /* set the reserved colors to the editor colors */
			if (me->curr[0] >= colors || me->curr[1] >= colors ||
              me->curr[0] == me->curr[1])
				{
				driver_buzzer(BUZZER_ERROR);
				break;
				}

			fg_color = (BYTE)me->curr[0];
			bg_color = (BYTE)me->curr[1];

			if (!me->hidden)
				{
				Cursor_Hide();
				PalTable__UpdateDAC(me);
				PalTable__Draw(me);
				Cursor_Show();
				}

			RGBEditor_SetDone(me->rgb[me->active], TRUE);
			break;

		case 'O':    /* set rotate_lo and rotate_hi to editors */
		case 'o':
			if (me->curr[0] > me->curr[1])
				{
				rotate_lo = me->curr[1];
				rotate_hi = me->curr[0];
				}
			else
				{
				rotate_lo = me->curr[0];
				rotate_hi = me->curr[1];
				}
			break;

		case FIK_F2:    /* restore a palette */
		case FIK_F3:
		case FIK_F4:
		case FIK_F5:
		case FIK_F6:
		case FIK_F7:
		case FIK_F8:
		case FIK_F9:
			{
			int which = key - FIK_F2;

			if (me->save_pal[which] != NULL)
				{
				Cursor_Hide();

				PalTable__SaveUndoData(me, 0, 255);
				memcpy(me->pal,me->save_pal[which],256*3);
				PalTable__UpdateDAC(me);

				PalTable__SetCurr(me, -1, 0);
				Cursor_Show();
				RGBEditor_SetDone(me->rgb[me->active], TRUE);
				}
			else
				driver_buzzer(BUZZER_ERROR);   /* error buzz */
			break;
			}

		case FIK_SF2:   /* save a palette */
		case FIK_SF3:
		case FIK_SF4:
		case FIK_SF5:
		case FIK_SF6:
		case FIK_SF7:
		case FIK_SF8:
		case FIK_SF9:
			{
			int which = key - FIK_SF2;

			if (me->save_pal[which] != NULL)
				{
				memcpy(me->save_pal[which],me->pal,256*3);
				}
			else
				driver_buzzer(BUZZER_ERROR); /* oops! short on memory! */
			break;
			}

		case 'L':     /* load a .map palette */
		case 'l':
			{
			PalTable__SaveUndoData(me, 0, 255);

			load_palette();
#ifndef XFRACT
			getpalrange(0, colors, me->pal);
#else
			getpalrange(0, 256, me->pal);
#endif
			PalTable__UpdateDAC(me);
			RGBEditor_SetRGB(me->rgb[0], me->curr[0], &(me->pal[me->curr[0]]));
			RGBEditor_Update(me->rgb[0]);
			RGBEditor_SetRGB(me->rgb[1], me->curr[1], &(me->pal[me->curr[1]]));
			RGBEditor_Update(me->rgb[1]);
			break;
			}

		case 'S':     /* save a .map palette */
		case 's':
			{
#ifndef XFRACT
			setpalrange(0, colors, me->pal);
#else
			setpalrange(0, 256, me->pal);
#endif
			save_palette();
			PalTable__UpdateDAC(me);
			break;
			}

		case 'C':     /* color cycling sub-mode */
		case 'c':
			{
			BOOLEAN oldhidden = (BOOLEAN)me->hidden;

			PalTable__SaveUndoData(me, 0, 255);

			Cursor_Hide();
			if (!oldhidden)
				PalTable_Hide(me, rgb, TRUE);
			setpalrange(0, colors, me->pal);
			rotate(0);
			getpalrange(0, colors, me->pal);
			PalTable__UpdateDAC(me);
			if (!oldhidden)
				{
				RGBEditor_SetRGB(me->rgb[0], me->curr[0], &(me->pal[me->curr[0]]));
				RGBEditor_SetRGB(me->rgb[1], me->curr[1], &(me->pal[me->curr[1]]));
				PalTable_Hide(me, rgb, FALSE);
				}
			Cursor_Show();
			break;
			}

		case 'F':
		case 'f':    /* toggle freestyle palette edit mode */
			me->freestyle= (BOOLEAN)((me->freestyle) ? FALSE :TRUE);

			PalTable__SetCurr(me, -1, 0);

			if (!me->freestyle)   /* if turning off... */
				PalTable__UpdateDAC(me);

			break;

		case FIK_CTL_DEL:  /* rt plus down */
			if (me->bandwidth >0)
				me->bandwidth  --;
			else
				me->bandwidth = 0;
			PalTable__SetCurr(me, -1, 0);
			break;

		case FIK_CTL_INSERT: /* rt plus up */
			if (me->bandwidth <255)
           me->bandwidth ++;
			else
				me->bandwidth = 255;
			PalTable__SetCurr(me, -1, 0);
			break;

		case 'W':   /* convert to greyscale */
		case 'w':
			{
			switch (me->exclude)
				{
				case 0:   /* normal mode.  convert all colors to grey scale */
					PalTable__SaveUndoData(me, 0, 255);
					palrangetogrey(me->pal, 0, 256);
					break;

				case 1:   /* 'x' mode. convert current color to grey scale.  */
					PalTable__SaveUndoData(me, me->curr[me->active], me->curr[me->active]);
					palrangetogrey(me->pal, me->curr[me->active], 1);
					break;

				case 2:  /* 'y' mode.  convert range between editors to grey. */
					{
					int a = me->curr[0],
                   b = me->curr[1];

					if (a > b)
						{
						int t = a;
						a = b;
						b = t;
						}

					PalTable__SaveUndoData(me, a, b);
					palrangetogrey(me->pal, a, 1 + (b-a));
					break;
					}
				}

			PalTable__UpdateDAC(me);
			RGBEditor_SetRGB(me->rgb[0], me->curr[0], &(me->pal[me->curr[0]]));
			RGBEditor_Update(me->rgb[0]);
			RGBEditor_SetRGB(me->rgb[1], me->curr[1], &(me->pal[me->curr[1]]));
			RGBEditor_Update(me->rgb[1]);
			break;
			}

		case 'N':   /* convert to negative color */
		case 'n':
			{
			switch (me->exclude)
				{
				case 0:      /* normal mode.  convert all colors to grey scale */
					PalTable__SaveUndoData(me, 0, 255);
					palrangetonegative(me->pal, 0, 256);
					break;

				case 1:      /* 'x' mode. convert current color to grey scale.  */
					PalTable__SaveUndoData(me, me->curr[me->active], me->curr[me->active]);
					palrangetonegative(me->pal, me->curr[me->active], 1);
					break;

				case 2:  /* 'y' mode.  convert range between editors to grey. */
					{
					int a = me->curr[0],
                   b = me->curr[1];

					if (a > b)
						{
						int t = a;
						a = b;
						b = t;
						}

					PalTable__SaveUndoData(me, a, b);
					palrangetonegative(me->pal, a, 1 + (b-a));
					break;
					}
				}

			PalTable__UpdateDAC(me);
			RGBEditor_SetRGB(me->rgb[0], me->curr[0], &(me->pal[me->curr[0]]));
			RGBEditor_Update(me->rgb[0]);
			RGBEditor_SetRGB(me->rgb[1], me->curr[1], &(me->pal[me->curr[1]]));
			RGBEditor_Update(me->rgb[1]);
			break;
			}

		case 'U':     /* Undo */
		case 'u':
			PalTable__Undo(me);
			break;

		case 'e':    /* Redo */
		case 'E':
			PalTable__Redo(me);
			break;

		} /* switch */
		PalTable__DrawStatus(me, FALSE);
	}

static void PalTable__MkDefaultPalettes(PalTable *me)  /* creates default Fkey palettes */
{
	int i;
	for (i = 0; i < 8; i++) /* copy original palette to save areas */
	{
		if (me->save_pal[i] != NULL)
		{
			memcpy(me->save_pal[i], me->pal, 256*3);
		}
	}
}



static PalTable *PalTable_Construct(void)
	{
	PalTable     *me = NEWC(PalTable);
	int           csize;
	int           ctr;
	PALENTRY *mem_block;
	void     *temp;

	temp = (void *)malloc(FAR_RESERVE);

	if (temp != NULL)
		{
		mem_block = (PALENTRY *)malloc(256L*3*8);

		if (mem_block == NULL)
			{
			for (ctr = 0; ctr < 8; ctr++)
				me->save_pal[ctr] = NULL;
			}
		else
			{
			for (ctr = 0; ctr < 8; ctr++)
				me->save_pal[ctr] = mem_block + (256*ctr);
			}
		free(temp);
		}

	me->rgb[0] = RGBEditor_Construct(0, 0, PalTable__other_key,
						PalTable__change, me);
	me->rgb[1] = RGBEditor_Construct(0, 0, PalTable__other_key,
						PalTable__change, me);

	me->movebox = MoveBox_Construct(0,0,0, PalTable_PALX + 1, PalTable_PALY + 1);

	me->active      = 0;
	me->curr[0]     = 1;
	me->curr[1]     = 1;
	me->auto_select = TRUE;
	me->exclude     = FALSE;
	me->hidden      = FALSE;
	me->stored_at   = NOWHERE;
	me->file        = NULL;
	me->memory      = NULL;

	me->fs_color.red   = 42;
	me->fs_color.green = 42;
	me->fs_color.blue  = 42;
	me->freestyle      = FALSE;
	me->bandwidth      = 15;
	me->top            = 255;
	me->bottom         = 0 ;

	me->undo_file    = dir_fopen(tempdir,undofile, "w+b");
	me->curr_changed = FALSE;
	me->num_redo     = 0;

	RGBEditor_SetRGB(me->rgb[0], me->curr[0], &me->pal[me->curr[0]]);
	RGBEditor_SetRGB(me->rgb[1], me->curr[1], &me->pal[me->curr[0]]);

	if (g_video_scroll) 
	{
		PalTable__SetPos(me, g_video_start_x, g_video_start_y);
		csize = ((g_vesa_y_res-(PalTable_PALY + 1 + 1)) / 2) / 16;
	}
	else 
	{
		PalTable__SetPos(me, 0, 0);
		csize = ((sydots-(PalTable_PALY + 1 + 1)) / 2) / 16;
	}

	if (csize < CSIZE_MIN)
		csize = CSIZE_MIN;
	PalTable__SetCSize(me, csize);

	return me;
	}


static void PalTable_SetHidden(PalTable *me, BOOLEAN hidden)
	{
	me->hidden = hidden;
	RGBEditor_SetHidden(me->rgb[0], hidden);
	RGBEditor_SetHidden(me->rgb[1], hidden);
	PalTable__UpdateDAC(me);
	}



static void PalTable_Hide(PalTable *me, RGBEditor *rgb, BOOLEAN hidden)
	{
	if (hidden)
		{
		PalTable__RestoreRect(me);
		PalTable_SetHidden(me, TRUE);
		reserve_colors = FALSE;
		if (me->auto_select)
			PalTable__SetCurr(me, me->active, PalTable__GetCursorColor(me));
		}
	else
		{
		PalTable_SetHidden(me, FALSE);
		reserve_colors = TRUE;
		if (me->stored_at == NOWHERE)  /* do we need to save screen? */
			PalTable__SaveRect(me);
		PalTable__Draw(me);
		if (me->auto_select)
			PalTable__SetCurr(me, me->active, PalTable__GetCursorColor(me));
		RGBEditor_SetDone(rgb, TRUE);
		}
	}


static void PalTable_Destroy(PalTable *me)
	{

	if (me->file != NULL)
		{
		fclose(me->file);
		dir_remove(tempdir,scrnfile);
		}

	if (me->undo_file != NULL)
		{
		fclose(me->undo_file);
		dir_remove(tempdir,undofile);
		}

	if (me->memory != NULL)
		free(me->memory);

	if (me->save_pal[0] != NULL)
		free((BYTE *)me->save_pal[0]);

	RGBEditor_Destroy(me->rgb[0]);
	RGBEditor_Destroy(me->rgb[1]);
	MoveBox_Destroy(me->movebox);
	DELETE(me);
	}


static void PalTable_Process(PalTable *me)
	{
	int ctr;

	getpalrange(0, colors, me->pal);

	/* Make sure all palette entries are 0-63 */

	for (ctr = 0; ctr < 768; ctr++)
		((char *)me->pal)[ctr] &= 63;

	PalTable__UpdateDAC(me);

	RGBEditor_SetRGB(me->rgb[0], me->curr[0], &me->pal[me->curr[0]]);
	RGBEditor_SetRGB(me->rgb[1], me->curr[1], &me->pal[me->curr[0]]);

	if (!me->hidden)
		{
		MoveBox_SetPos(me->movebox, me->x, me->y);
		MoveBox_SetCSize(me->movebox, me->csize);
		if (!MoveBox_Process(me->movebox))
			{
			setpalrange(0, colors, me->pal);
			return ;
			}

		PalTable__SetPos(me, MoveBox_X(me->movebox), MoveBox_Y(me->movebox));
		PalTable__SetCSize(me, MoveBox_CSize(me->movebox));

		if (MoveBox_ShouldHide(me->movebox))
			{
			PalTable_SetHidden(me, TRUE);
			reserve_colors = FALSE;   /* <EAN> */
			}
		else
			{
			reserve_colors = TRUE;    /* <EAN> */
			PalTable__SaveRect(me);
			PalTable__Draw(me);
			}
		}

	PalTable__SetCurr(me, me->active,          PalTable__GetCursorColor(me));
	PalTable__SetCurr(me, (me->active == 1)?0:1, PalTable__GetCursorColor(me));
	Cursor_Show();
	PalTable__MkDefaultPalettes(me);
	me->done = FALSE;

	while (!me->done)
		RGBEditor_Edit(me->rgb[me->active]);

	Cursor_Hide();
	PalTable__RestoreRect(me);
	setpalrange(0, colors, me->pal);
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

	if (sxdots < 133 || sydots < 174)
		return; /* prevents crash when physical screen is too small */

	plot = putcolor;

	line_buff = NEWX(max(sxdots,sydots));

	lookatmouse = LOOK_MOUSE_ZOOM_BOX;
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
	DELETE(line_buff);
	}
