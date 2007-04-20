/*
 * editpal.c
 *
 * Edits VGA 256-color palettes.
 *
 * Key to initials:
 *
 *    EAN - Ethan Nagel [70022, 2552]
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
 *                      them in (instead of only getcolor/g_put_color). Much
 *                      faster!
 *                    Redesigned color editors (now at top of palette) and
 *                      re-assigned some keys.
 *                    Added (A)uto option.
 *                    Fixed memory allocation problem.  Now uses shared
 *                      FRACTINT data area (g_string_location).  Uses 6 bytes DS.
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
 *                    Change cursor to use whatever g_colors it can from
 *                    the palette (not fixed 0 and 255).
 *                    Restore g_colors 0 and 255 to real values whenever
 *                    palette is not on display.
 *                    Rotate 255 g_colors instead of 254.
 *                    Reduce cursor blink rate.
 *
 *   11-15-90 EAN     Minor "bug" fixes.  Continuous rotation now at a fixed
 *                      rate - once every timer tick (18.2 sec);  Blanks out
 *                      color samples when rotating; Editors no longer rotate
 *                      with the g_colors in color rotation mode;  Eliminated
 *                      (Z)oom mode; other minor fixes.
 *
 *   01-05-91 PB      Add 'w' function to convert to greyscale.
 *
 *   01-16-91 PB      Change rotate function to use new cyclerange stuff.
 *
 *   01-29-91 EAN     Made all g_colors editable.  The two reserved g_colors are
 *                       X'ed out.  They can be edited but the color is not
 *                       visible.  (There is an X over the sample instead.)
 *                    Changed default reserved g_colors to 254 & 255.
 *                    Added 'v' command to set the reserved g_colors to those
 *                       under the editors.
 *                    Added 'o' command to set the rotate range to between
 *                      the two editors.
 *                    Modified 'w' function:
 *                      uses internal function to do conversion (not BIOS)
 *                      will convert only current color if in 'x' mode or
 *                        range between editors in 'y' mode or entire palette
 *                        if in "normal" mode.
 *
 *   02-08-91 EAN     Improved 16 color support.  In 16 color mode, g_colors
 *                      16-255 have a dot over them and are editable but not
 *                      visible (like the two reserved g_colors).
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
#include "helpdefs.h"
#include "fihelp.h"

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
#define TITLE   "FRACTINT"
#define TITLE_LEN (8)
#define NEWC(class_)	((class_ *) malloc(sizeof(class_)))
#define DELETE(g_block)	(free(g_block), g_block = NULL)  /* just for warning */

#ifdef XFRACT
int g_edit_pal_cursor = 0;
#endif
char g_screen_file[] = "FRACTINT.$$1";  /* file where screen portion is stored */
BYTE     *g_line_buffer;   /* must be alloced!!! */
int g_using_jiim = 0;

static char s_undo_file[] = "FRACTINT.$$2";  /* file where undo list is stored */
static BYTE		s_fg_color,
				s_bg_color;
static BOOLEAN	s_reserve_colors;
static BOOLEAN	s_inverse;
static float    s_gamma_val = 1;

/*
 * basic data types
 */
typedef struct
{
	BYTE red, green, blue;
} PALENTRY;

/*
 * Interface to FRACTINT's graphics stuff
 */
static void set_pal(int pal, int r, int g, int b)
{
	g_dac_box[pal][0] = (BYTE) r;
	g_dac_box[pal][1] = (BYTE) g;
	g_dac_box[pal][2] = (BYTE) b;
	spindac(0, 1);
}

static void set_pal_range(int first, int how_many, PALENTRY *pal)
{
	memmove(g_dac_box + first, pal, how_many*3);
	spindac(0, 1);
}

static void get_pal_range(int first, int how_many, PALENTRY *pal)
{
	memmove(pal, g_dac_box + first, how_many*3);
}

static void rotate_pal(PALENTRY *pal, int dir, int lo, int hi)
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
	if (row < 0 || row >= g_screen_height || start > g_screen_width || stop < 0)
	{
		return ;
	}

	if (start < 0)
	{
		pixels += -start;
		start = 0;
	}

	if (stop >= g_screen_width)
	{
		stop = g_screen_width - 1;
	}

	if (start > stop)
	{
		return ;
	}

	put_line(row, start, stop, pixels);
}

static void clip_get_line(int row, int start, int stop, BYTE *pixels)
{
	if (row < 0 || row >= g_screen_height || start > g_screen_width || stop < 0)
	{
		return ;
	}

	if (start < 0)
	{
		pixels += -start;
		start = 0;
	}

	if (stop >= g_screen_width)
	{
		stop = g_screen_width - 1;
	}

	if (start > stop)
	{
		return ;
	}

	get_line(row, start, stop, pixels);
}

static void clip_put_color(int x, int y, int color)
{
	if (x < 0 || y < 0 || x >= g_screen_width || y >= g_screen_height)
	{
		return ;
	}

	g_put_color(x, y, color);
}

static int clip_get_color(int x, int y)
{
	if (x < 0 || y < 0 || x >= g_screen_width || y >= g_screen_height)
	{
		return 0;
	}

	return getcolor(x, y);
}

static void horizontal_line(int x, int y, int width, int color)
{
	memset(g_line_buffer, color, width);
	clip_put_line(y, x, x + width-1, g_line_buffer);
}

static void vertical_line(int x, int y, int depth, int color)
{
	while (depth-- > 0)
	{
		clip_put_color(x, y++, color);
	}
}

void get_row(int x, int y, int width, char *buff)
{
	clip_get_line(y, x, x + width-1, (BYTE *)buff);
}

void put_row(int x, int y, int width, char *buff)
{
	clip_put_line(y, x, x + width-1, (BYTE *)buff);
}

static void vertical_get_row(int x, int y, int depth, char *buff)
{
	while (depth-- > 0)
	{
		*buff++ = (char)clip_get_color(x, y++);
	}
}

static void vertical_put_row(int x, int y, int depth, char *buff)
{
	while (depth-- > 0)
	{
		clip_put_color(x, y++, (BYTE)(*buff++));
	}
}

static void fill_rectangle(int x, int y, int width, int depth, int color)
{
	while (depth-- > 0)
	{
		horizontal_line(x, y++, width, color);
	}
}

static void rectangle(int x, int y, int width, int depth, int color)
{
	horizontal_line(x, y, width, color);
	horizontal_line(x, y + depth-1, width, color);

	vertical_line(x, y, depth, color);
	vertical_line(x + width-1, y, depth, color);
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
	int x, y, fg, bg;
	char *format;

	va_start(arg_list);
	x = va_arg(arg_list, int);
	y = va_arg(arg_list, int);
	fg = va_arg(arg_list, int);
	bg = va_arg(arg_list, int);
	format = va_arg(arg_list, char *);
#endif
	vsprintf(buff, format, arg_list);
	va_end(arg_list);

	driver_display_string(x, y, fg, bg, buff);
}


/*
 * create smooth shades between two g_colors
 */
static void make_pal_range(PALENTRY *p1, PALENTRY *p2, PALENTRY pal[], int num, int skip)
{
	int    curr;
	double rm = (double)((int) p2->red   - (int) p1->red) / num,
			gm = (double)((int) p2->green - (int) p1->green) / num,
			bm = (double)((int) p2->blue  - (int) p1->blue) / num;

	for (curr = 0; curr < num; curr += skip)
	{
		if (s_gamma_val == 1)
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
				(int) (p1->red   + pow(curr/(double)(num-1), (double) s_gamma_val)*num*rm));
			pal[curr].green = (BYTE)((p1->green == p2->green) ? p1->green :
				(int) (p1->green + pow(curr/(double)(num-1), (double) s_gamma_val)*num*gm));
			pal[curr].blue  = (BYTE)((p1->blue  == p2->blue) ? p1->blue  :
				(int) (p1->blue  + pow(curr/(double)(num-1), (double) s_gamma_val)*num*bm));
		}
	}
}


/*  Swap RG GB & RB columns */
static void swap_columns_rg(PALENTRY pal[], int num)
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

static void swap_columns_gb(PALENTRY pal[], int num)
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

static void swap_columns_br(PALENTRY pal[], int num)
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
 * convert a range of g_colors to grey scale
 */
static void pal_range_to_grey(PALENTRY pal[], int first, int how_many)
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
 * convert a range of g_colors to their s_inverse
 */
static void pal_range_to_negative(PALENTRY pal[], int first, int how_many)
{
	PALENTRY      *curr;

	for (curr = &pal[first]; how_many > 0; how_many--, curr++)
	{
		curr->red   = (BYTE) (COLOR_CHANNEL_MAX - curr->red);
		curr->green = (BYTE) (COLOR_CHANNEL_MAX - curr->green);
		curr->blue  = (BYTE) (COLOR_CHANNEL_MAX - curr->blue);
	}
}

/*
 * draw and horizontal/vertical dotted lines
 */
static void horizontal_dotted_line(int x, int y, int width)
{
	int ctr;
	BYTE *ptr;

	for (ctr = 0, ptr = g_line_buffer; ctr < width; ctr++, ptr++)
	{
		*ptr = (BYTE)((ctr&2) ? s_bg_color : s_fg_color);
	}

	put_row(x, y, width, (char *)g_line_buffer);
}

static void vertical_dotted_line(int x, int y, int depth)
{
	int ctr;

	for (ctr = 0; ctr < depth; ctr++, y++)
	{
		clip_put_color(x, y, (ctr&2) ? s_bg_color : s_fg_color);
	}
}

static void dotted_rectangle(int x, int y, int width, int depth)
{
	horizontal_dotted_line(x, y, width);
	horizontal_dotted_line(x, y + depth-1, width);

	vertical_dotted_line(x, y, depth);
	vertical_dotted_line(x + width-1, y, depth);
}


/*
 * misc. routines
 *
 */
static BOOLEAN is_reserved(int color)
{
	return (BOOLEAN) ((s_reserve_colors && (color == (int)s_fg_color || color == (int)s_bg_color)) ? TRUE : FALSE);
}

static BOOLEAN is_in_box(int x, int y, int bx, int by, int bw, int bd)
{
	return (BOOLEAN) ((x >= bx) && (y >= by) && (x < bx + bw) && (y < by + bd));
}

static void draw_diamond(int x, int y, int color)
{
	g_put_color (x + 2, y + 0,    color);
	horizontal_line    (x + 1, y + 1, 3, color);
	horizontal_line    (x + 0, y + 2, 5, color);
	horizontal_line    (x + 1, y + 3, 3, color);
	g_put_color (x + 2, y + 4,    color);
}

/*
 * Class:     cursor
 *
 * Purpose:   Draw the blinking cross-hair cursor.
 *
 * Note:      Only one cursor can exist (referenced through s_the_cursor).
 *            IMPORTANT: Call cursor_new before you use any other
 *            Cursor_ function!  Call cursor_destroy before exiting to
 *            deallocate memory.
 */

struct tag_cursor
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
};
typedef struct tag_cursor cursor;

/* private: */
static void cursor_draw(void);
static void cursor_save(void);
static void cursor_restore(void);

static cursor *s_the_cursor = NULL;

BOOLEAN cursor_new(void)
{
	if (s_the_cursor != NULL)
	{
		return FALSE;
	}

	s_the_cursor = NEWC(cursor);

	s_the_cursor->x          = g_screen_width/2;
	s_the_cursor->y          = g_screen_height/2;
	s_the_cursor->hidden     = 1;
	s_the_cursor->blink      = FALSE;
	s_the_cursor->last_blink = 0;

	return TRUE;
}

void cursor_destroy(void)
{
	if (s_the_cursor != NULL)
	{
		DELETE(s_the_cursor);
	}

	s_the_cursor = NULL;
}

static void cursor_draw(void)
{
	int color;

	find_special_colors();
	color = (s_the_cursor->blink) ? g_color_medium : g_color_dark;

	vertical_line(s_the_cursor->x, s_the_cursor->y-CURSOR_SIZE-1, CURSOR_SIZE, color);
	vertical_line(s_the_cursor->x, s_the_cursor->y + 2,             CURSOR_SIZE, color);

	horizontal_line(s_the_cursor->x-CURSOR_SIZE-1, s_the_cursor->y, CURSOR_SIZE, color);
	horizontal_line(s_the_cursor->x + 2,             s_the_cursor->y, CURSOR_SIZE, color);
}

static void cursor_save(void)
{
	vertical_get_row(s_the_cursor->x, s_the_cursor->y-CURSOR_SIZE-1, CURSOR_SIZE, s_the_cursor->t);
	vertical_get_row(s_the_cursor->x, s_the_cursor->y + 2,             CURSOR_SIZE, s_the_cursor->b);

	get_row(s_the_cursor->x-CURSOR_SIZE-1, s_the_cursor->y,  CURSOR_SIZE, s_the_cursor->l);
	get_row(s_the_cursor->x + 2,             s_the_cursor->y,  CURSOR_SIZE, s_the_cursor->r);
}

static void cursor_restore(void)
{
	vertical_put_row(s_the_cursor->x, s_the_cursor->y-CURSOR_SIZE-1, CURSOR_SIZE, s_the_cursor->t);
	vertical_put_row(s_the_cursor->x, s_the_cursor->y + 2,             CURSOR_SIZE, s_the_cursor->b);

	put_row(s_the_cursor->x-CURSOR_SIZE-1, s_the_cursor->y,  CURSOR_SIZE, s_the_cursor->l);
	put_row(s_the_cursor->x + 2,             s_the_cursor->y,  CURSOR_SIZE, s_the_cursor->r);
}

void cursor_set_position(int x, int y)
{
	if (!s_the_cursor->hidden)
	{
		cursor_restore();
	}

	s_the_cursor->x = x;
	s_the_cursor->y = y;

	if (!s_the_cursor->hidden)
	{
		cursor_save();
		cursor_draw();
	}
}

void cursor_move(int xoff, int yoff)
{
	if (!s_the_cursor->hidden)
	{
		cursor_restore();
	}

	s_the_cursor->x += xoff;
	s_the_cursor->y += yoff;

	if (s_the_cursor->x < 0)
	{
		s_the_cursor->x = 0;
	}
	if (s_the_cursor->y < 0)
	{
		s_the_cursor->y = 0;
	}
	if (s_the_cursor->x >= g_screen_width)
	{
		s_the_cursor->x = g_screen_width-1;
	}
	if (s_the_cursor->y >= g_screen_height)
	{
		s_the_cursor->y = g_screen_height-1;
	}

	if (!s_the_cursor->hidden)
	{
		cursor_save();
		cursor_draw();
	}
}

int cursor_get_x(void)
{
	return s_the_cursor->x;
}

int cursor_get_y(void)
{
	return s_the_cursor->y;
}

void cursor_hide(void)
{
	if (s_the_cursor->hidden++ == 0)
	{
		cursor_restore();
	}
}

void cursor_show(void)
{
	if (--s_the_cursor->hidden == 0)
	{
		cursor_save();
		cursor_draw();
	}
}

#ifdef XFRACT
void cursor_start_mouse_tracking()
{
	g_edit_pal_cursor = 1;
}

void cursor_end_mouse_tracking()
{
	g_edit_pal_cursor = 0;
}
#endif

/* See if the cursor should blink yet, and blink it if so */
void cursor_check_blink(void)
{
	long tick;
	tick = readticker();

	if ((tick - s_the_cursor->last_blink) > CURSOR_BLINK_RATE)
	{
		s_the_cursor->blink = (BOOLEAN)((s_the_cursor->blink) ? FALSE : TRUE);
		s_the_cursor->last_blink = tick;
		if (!s_the_cursor->hidden)
		{
			cursor_draw();
		}
	}
	else if (tick < s_the_cursor->last_blink)
	{
		s_the_cursor->last_blink = tick;
	}
}

int cursor_wait_key(void)   /* blink cursor while waiting for a key */
{
	while (!driver_wait_key_pressed(1))
	{
		cursor_check_blink();
	}

	return driver_key_pressed();
}

/*
 * Class:     move_box
 *
 * Purpose:   Handles the rectangular move/resize box.
 */

struct tag_move_box
{
	int      x, y;
	int      base_width,
				base_depth;
	int      csize;
	BOOLEAN  moved;
	BOOLEAN  should_hide;
	char    *t, *b,
			*l, *r;
};
typedef struct tag_move_box move_box;

static void     move_box_draw     (move_box *me);
static void     move_box_erase    (move_box *me);
static void     move_box_move     (move_box *me, int key);
static move_box *move_box_new  (int x, int y, int csize, int base_width,
									int base_depth);
static void     move_box_destroy    (move_box *me);
static BOOLEAN  move_box_process    (move_box *me); /* returns FALSE if ESCAPED */
static BOOLEAN  move_box_moved      (move_box *me);
static BOOLEAN  move_box_should_hide (move_box *me);
static int      move_box_x          (move_box *me);
static int      move_box_y          (move_box *me);
static int      move_box_csize      (move_box *me);
static void     move_box_set_position     (move_box *me, int x, int y);
static void     move_box_set_csize   (move_box *me, int csize);

static move_box *move_box_new(int x, int y, int csize, int base_width, int base_depth)
{
	move_box *me = NEWC(move_box);

	me->x           = x;
	me->y           = y;
	me->csize       = csize;
	me->base_width  = base_width;
	me->base_depth  = base_depth;
	me->moved       = FALSE;
	me->should_hide = FALSE;
	me->t           = (char *) malloc(g_screen_width);
	me->b           = (char *) malloc(g_screen_width);
	me->l           = (char *) malloc(g_screen_height);
	me->r           = (char *) malloc(g_screen_height);

	return me;
}

static void move_box_destroy(move_box *me)
{
	DELETE(me->t);
	DELETE(me->b);
	DELETE(me->l);
	DELETE(me->r);
	DELETE(me);
}

static BOOLEAN move_box_moved(move_box *me)
{
	return me->moved;
}

static BOOLEAN move_box_should_hide(move_box *me)
{
	return me->should_hide;
}

static int move_box_x(move_box *me)
{
	return me->x;
}

static int move_box_y(move_box *me)
{
	return me->y;
}

static int move_box_csize(move_box *me)
{
	return me->csize;
}

static void move_box_set_position(move_box *me, int x, int y)
{
	me->x = x;
	me->y = y;
}

static void move_box_set_csize(move_box *me, int csize)
{
	me->csize = csize;
}

static void move_box_draw(move_box *me)  /* private */
{
	int width = me->base_width + me->csize*16 + 1,
		depth = me->base_depth + me->csize*16 + 1;
	int x     = me->x,
		y     = me->y;


	get_row (x, y,         width, me->t);
	get_row (x, y + depth-1, width, me->b);

	vertical_get_row(x,         y, depth, me->l);
	vertical_get_row(x + width-1, y, depth, me->r);

	horizontal_dotted_line(x, y,         width);
	horizontal_dotted_line(x, y + depth-1, width);

	vertical_dotted_line(x,         y, depth);
	vertical_dotted_line(x + width-1, y, depth);
}

static void move_box_erase(move_box *me)   /* private */
{
	int width = me->base_width + me->csize*16 + 1,
		depth = me->base_depth + me->csize*16 + 1;

	vertical_put_row(me->x,         me->y, depth, me->l);
	vertical_put_row(me->x + width-1, me->y, depth, me->r);

	put_row(me->x, me->y,         width, me->t);
	put_row(me->x, me->y + depth-1, width, me->b);
}

#define BOX_INC     1
#define CSIZE_INC   2

static void move_box_move(move_box *me, int key)
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
			{
				driver_get_key();       /* delete key from buffer */
			}
			else
			{
				first = FALSE;
			}
			key = driver_key_pressed();   /* peek at the next one... */
		}
	}

	xoff += me->x;
	yoff += me->y;   /* (xoff, yoff) = new position */

	if (xoff < 0)
	{
		xoff = 0;
	}
	if (yoff < 0)
	{
		yoff = 0;
	}

	if (xoff + me->base_width + me->csize*16 + 1 > g_screen_width)
	{
		xoff = g_screen_width - (me->base_width + me->csize*16 + 1);
	}

	if (yoff + me->base_depth + me->csize*16 + 1 > g_screen_height)
	{
		yoff = g_screen_height - (me->base_depth + me->csize*16 + 1);
	}

	if (xoff != me->x || yoff != me->y)
	{
		move_box_erase(me);
		me->y = yoff;
		me->x = xoff;
		move_box_draw(me);
	}
}

static BOOLEAN move_box_process(move_box *me)
{
	int     key;
	int     orig_x     = me->x,
			orig_y     = me->y,
			orig_csize = me->csize;

	move_box_draw(me);

#ifdef XFRACT
	cursor_start_mouse_tracking();
#endif
	while (1)
	{
		cursor_wait_key();
		key = driver_get_key();

		if (key == FIK_ENTER || key == FIK_ENTER_2 || key == FIK_ESC || key == 'H' || key == 'h')
		{
			me->moved = (me->x != orig_x || me->y != orig_y || me->csize != orig_csize)
				? TRUE : FALSE;
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
			move_box_move(me, key);
			break;

		case FIK_PAGE_UP:   /* shrink */
			if (me->csize > CSIZE_MIN)
			{
				int t = me->csize - CSIZE_INC;
				int change;

				if (t < CSIZE_MIN)
				{
					t = CSIZE_MIN;
				}

				move_box_erase(me);

				change = me->csize - t;
				me->csize = t;
				me->x += (change*16) / 2;
				me->y += (change*16) / 2;
				move_box_draw(me);
			}
			break;

		case FIK_PAGE_DOWN:   /* grow */
			{
				int max_width = min(g_screen_width, MAX_WIDTH);

				if (me->base_depth + (me->csize + CSIZE_INC)*16 + 1 < g_screen_height  &&
					me->base_width + (me->csize + CSIZE_INC)*16 + 1 < max_width)
				{
					move_box_erase(me);
					me->x -= (CSIZE_INC*16) / 2;
					me->y -= (CSIZE_INC*16) / 2;
					me->csize += CSIZE_INC;
					if (me->y + me->base_depth + me->csize*16 + 1 > g_screen_height)
					{
						me->y = g_screen_height - (me->base_depth + me->csize*16 + 1);
					}
					if (me->x + me->base_width + me->csize*16 + 1 > max_width)
					{
						me->x = max_width - (me->base_width + me->csize*16 + 1);
					}
					if (me->y < 0)
					{
						me->y = 0;
					}
					if (me->x < 0)
					{
						me->x = 0;
					}
					move_box_draw(me);
				}
			}
			break;
		}
	}

#ifdef XFRACT
	cursor_end_mouse_tracking();
#endif

	move_box_erase(me);

	me->should_hide = (BOOLEAN)((key == 'H' || key == 'h') ? TRUE : FALSE);

	return (BOOLEAN)((key == FIK_ESC) ? FALSE : TRUE);
}

/*
 * Class:     color_editor
 *
 * Purpose:   Edits a single color component (R, G or B)
 *
 * Note:      Calls the "other_key" function to process keys it doesn't use.
 *            The "change" function is called whenever the value is changed
 *            by the color_editor.
 */
typedef struct tag_color_editor color_editor;
struct tag_color_editor
{
	int       x, y;
	char      letter;
	int       val;
	BOOLEAN   done;
	BOOLEAN   hidden;
	void    (*other_key)(int key, color_editor *ce, VOIDPTR info);
	void    (*change)(color_editor *ce, VOIDPTR info);
	void     *info;

};

#ifndef XFRACT
static color_editor *color_editor_new(int x, int y, char letter,
									void (*other_key)(int, color_editor*, void*),
									void (*change)(color_editor*, void*), VOIDPTR info);
static void color_editor_destroy   (color_editor *me);
static void color_editor_draw      (color_editor *me);
static void color_editor_set_position    (color_editor *me, int x, int y);
static void color_editor_set_value    (color_editor *me, int val);
static int  color_editor_get_value    (color_editor *me);
static void color_editor_set_done   (color_editor *me, BOOLEAN done);
static void color_editor_set_hidden (color_editor *me, BOOLEAN hidden);
static int  color_editor_edit      (color_editor *me);
#else
static color_editor *color_editor_new(int , int , char ,
									void (*other_key)(),
									void (*change)(), VOIDPTR);
static void color_editor_destroy         (color_editor *);
static void color_editor_draw    (color_editor *);
static void color_editor_set_position  (color_editor *, int , int);
static void color_editor_set_value  (color_editor *, int);
static int  color_editor_get_value  (color_editor *);
static void color_editor_set_done         (color_editor *, BOOLEAN);
static void color_editor_set_hidden (color_editor *, BOOLEAN);
static int  color_editor_edit    (color_editor *);
#endif

#define COLOR_EDITOR_WIDTH (8*3 + 4)
#define COLOR_EDITOR_DEPTH (8 + 4)



#ifndef XFRACT
static color_editor *color_editor_new(int x, int y, char letter,
					void (*other_key)(int, color_editor*, VOIDPTR),
					void (*change)(color_editor*, VOIDPTR), VOIDPTR info)
#else
static color_editor *color_editor_new(int x, int y, char letter,
					void (*other_key)(),
					void (*change)(), VOIDPTR info)
#endif
{
	color_editor *me = NEWC(color_editor);

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

static void color_editor_destroy(color_editor *me)
{
	DELETE(me);
}

static void color_editor_draw(color_editor *me)
{
	if (me->hidden)
	{
		return;
	}

	cursor_hide();
	displayf(me->x + 2, me->y + 2, s_fg_color, s_bg_color, "%c%02d", me->letter, me->val);
	cursor_show();
}

static void color_editor_set_position(color_editor *me, int x, int y)
{
	me->x = x;
	me->y = y;
}

static void color_editor_set_value(color_editor *me, int val)
{
	me->val = val;
}

static int color_editor_get_value(color_editor *me)
{
	return me->val;
}

static void color_editor_set_done(color_editor *me, BOOLEAN done)
{
	me->done = done;
}

static void color_editor_set_hidden(color_editor *me, BOOLEAN hidden)
{
	me->hidden = hidden;
}

static int color_editor_edit(color_editor *me)
{
	int key = 0;
	int diff;

	me->done = FALSE;

	if (!me->hidden)
	{
		cursor_hide();
		rectangle(me->x, me->y, COLOR_EDITOR_WIDTH, COLOR_EDITOR_DEPTH, s_fg_color);
		cursor_show();
	}

#ifdef XFRACT
	cursor_start_mouse_tracking();
#endif
	while (!me->done)
	{
		cursor_wait_key();
		key = driver_get_key();

		switch (key)
		{
		case FIK_PAGE_UP:
			if (me->val < COLOR_CHANNEL_MAX)
			{
				me->val += 5;
				if (me->val > COLOR_CHANNEL_MAX)
				{
					me->val = COLOR_CHANNEL_MAX;
				}
				color_editor_draw(me);
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
			if (me->val < COLOR_CHANNEL_MAX)
			{
				me->val += diff;
				if (me->val > COLOR_CHANNEL_MAX)
				{
					me->val = COLOR_CHANNEL_MAX;
				}
				color_editor_draw(me);
				me->change(me, me->info);
			}
			break;

		case FIK_PAGE_DOWN:
			if (me->val > 0)
			{
				me->val -= 5;
				if (me->val < 0)
				{
					me->val = 0;
				}
				color_editor_draw(me);
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
				{
					me->val = 0;
				}
				color_editor_draw(me);
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
			if (me->val > COLOR_CHANNEL_MAX)
			{
				me->val = COLOR_CHANNEL_MAX;
			}
			color_editor_draw(me);
			me->change(me, me->info);
			break;

		default:
			me->other_key(key, me, me->info);
			break;
		} /* switch */
	} /* while */
#ifdef XFRACT
	cursor_end_mouse_tracking();
#endif

	if (!me->hidden)
	{
		cursor_hide();
		rectangle(me->x, me->y, COLOR_EDITOR_WIDTH, COLOR_EDITOR_DEPTH, s_bg_color);
		cursor_show();
	}

	return key;
}

/*
 * Class:     rgb_editor
 *
 * Purpose:   Edits a complete color using three CEditors for R, G and B
 */

struct tag_rgb_editor
{
	int       x, y;            /* position */
	int       curr;            /* 0 = r, 1 = g, 2 = b */
	int       pal;             /* palette number */
	BOOLEAN   done;
	BOOLEAN   hidden;
	color_editor  *color[3];        /* color editors 0 = r, 1 = g, 2 = b */
#ifndef XFRACT
	void    (*other_key)(int key, struct tag_rgb_editor *e, VOIDPTR info);
	void    (*change)(struct tag_rgb_editor *e, VOIDPTR info);
#else
	void    (*other_key)();
	void    (*change)();
#endif
	void     *info;
};
typedef struct tag_rgb_editor rgb_editor;

static void      rgb_editor_other_key (int key, color_editor *ceditor, VOIDPTR info);
static void      rgb_editor_change    (color_editor *ceditor, VOIDPTR info);
#ifndef XFRACT
static rgb_editor *rgb_editor_new(int x, int y,
						void (*other_key)(int, rgb_editor*, void*),
						void (*change)(rgb_editor*, void*), VOIDPTR info);
#else
static rgb_editor *rgb_editor_new(int x, int y,
						void (*other_key)(),
						void (*change)(), VOIDPTR info);
#endif
static void     rgb_editor_destroy  (rgb_editor *me);
static void     rgb_editor_set_position   (rgb_editor *me, int x, int y);
static void     rgb_editor_set_done  (rgb_editor *me, BOOLEAN done);
static void     rgb_editor_set_hidden(rgb_editor *me, BOOLEAN hidden);
static void     rgb_editor_blank_sample_box(rgb_editor *me);
static void     rgb_editor_update   (rgb_editor *me);
static void     rgb_editor_draw     (rgb_editor *me);
static int      rgb_editor_edit     (rgb_editor *me);
static void     rgb_editor_set_rgb   (rgb_editor *me, int pal, PALENTRY *rgb);
static PALENTRY rgb_editor_get_rgb   (rgb_editor *me);

#define RGB_EDITOR_WIDTH 62
#define RGB_EDITOR_DEPTH (1 + 1 + COLOR_EDITOR_DEPTH*3-2 + 2)
#define RGB_EDITOR_BOX_WIDTH (RGB_EDITOR_WIDTH - (2 + COLOR_EDITOR_WIDTH + 1 + 2))
#define RGB_EDITOR_BOX_DEPTH (RGB_EDITOR_DEPTH - 4)



#ifndef XFRACT
static rgb_editor *rgb_editor_new(int x, int y, void (*other_key)(int, rgb_editor*, void*),
									void (*change)(rgb_editor*, void*), VOIDPTR info)
#else
static rgb_editor *rgb_editor_new(int x, int y, void (*other_key)(),
									void (*change)(), VOIDPTR info)
#endif
{
	rgb_editor      *me     = NEWC(rgb_editor);
	static char letter[] = "RGB";
	int             ctr;

	for (ctr = 0; ctr < 3; ctr++)
	{
		me->color[ctr] = color_editor_new(0, 0, letter[ctr], rgb_editor_other_key,
											rgb_editor_change, me);
	}

	rgb_editor_set_position(me, x, y);
	me->curr      = 0;
	me->pal       = 1;
	me->hidden    = FALSE;
	me->other_key = other_key;
	me->change    = change;
	me->info      = info;

	return me;
}

static void rgb_editor_destroy(rgb_editor *me)
{
	color_editor_destroy(me->color[0]);
	color_editor_destroy(me->color[1]);
	color_editor_destroy(me->color[2]);
	DELETE(me);
}

static void rgb_editor_set_done(rgb_editor *me, BOOLEAN done)
{
	me->done = done;
}

static void rgb_editor_set_hidden(rgb_editor *me, BOOLEAN hidden)
{
	me->hidden = hidden;
	color_editor_set_hidden(me->color[0], hidden);
	color_editor_set_hidden(me->color[1], hidden);
	color_editor_set_hidden(me->color[2], hidden);
}

static void rgb_editor_other_key(int key, color_editor *ceditor, VOIDPTR info) /* private */
{
	rgb_editor *me = (rgb_editor *)info;

	switch (key)
	{
	case 'R':
	case 'r':
		if (me->curr != 0)
		{
			me->curr = 0;
			color_editor_set_done(ceditor, TRUE);
		}
		break;
	case 'G':
	case 'g':
		if (me->curr != 1)
		{
			me->curr = 1;
			color_editor_set_done(ceditor, TRUE);
		}
		break;

	case 'B':
	case 'b':
		if (me->curr != 2)
		{
			me->curr = 2;
			color_editor_set_done(ceditor, TRUE);
		}
		break;

	case FIK_DELETE:   /* move to next color_editor */
	case FIK_CTL_ENTER_2:    /*double click rt mouse also! */
		if (++me->curr > 2)
		{
			me->curr = 0;
		}
		color_editor_set_done(ceditor, TRUE);
		break;

	case FIK_INSERT:   /* move to prev color_editor */
		if (--me->curr < 0)
		{
			me->curr = 2;
		}
		color_editor_set_done(ceditor, TRUE);
		break;

	default:
		me->other_key(key, me, me->info);
		if (me->done)
		{
			color_editor_set_done(ceditor, TRUE);
		}
		break;
	}
}

#ifdef __CLINT__
#   pragma argsused   /* kills "arg not used" warning */
#endif

static void rgb_editor_change(color_editor *ceditor, VOIDPTR info) /* private */
{
	rgb_editor *me = (rgb_editor *)info;

	ceditor = NULL; /* just for warning */
	if (me->pal < g_colors && !is_reserved(me->pal))
	{
		set_pal(me->pal, color_editor_get_value(me->color[0]),
			color_editor_get_value(me->color[1]), color_editor_get_value(me->color[2]));
	}
	me->change(me, me->info);
}

static void rgb_editor_set_position(rgb_editor *me, int x, int y)
{
	me->x = x;
	me->y = y;

	color_editor_set_position(me->color[0], x + 2, y + 2);
	color_editor_set_position(me->color[1], x + 2, y + 2 + COLOR_EDITOR_DEPTH-1);
	color_editor_set_position(me->color[2], x + 2, y + 2 + COLOR_EDITOR_DEPTH-1 + COLOR_EDITOR_DEPTH-1);
}

static void rgb_editor_blank_sample_box(rgb_editor *me)
{
	if (me->hidden)
	{
		return ;
	}

	cursor_hide();
	fill_rectangle(me->x + 2 + COLOR_EDITOR_WIDTH + 1 + 1, me->y + 2 + 1, RGB_EDITOR_BOX_WIDTH-2, RGB_EDITOR_BOX_DEPTH-2, s_bg_color);
	cursor_show();
}

static void rgb_editor_update(rgb_editor *me)
{
	int x1 = me->x + 2 + COLOR_EDITOR_WIDTH + 1 + 1,
		y1 = me->y + 2 + 1;

	if (me->hidden)
	{
		return ;
	}

	cursor_hide();

	if (me->pal >= g_colors)
	{
		fill_rectangle(x1, y1, RGB_EDITOR_BOX_WIDTH-2, RGB_EDITOR_BOX_DEPTH-2, s_bg_color);
		draw_diamond(x1 + (RGB_EDITOR_BOX_WIDTH-5)/2, y1 + (RGB_EDITOR_BOX_DEPTH-5)/2, s_fg_color);
	}

	else if (is_reserved(me->pal))
	{
		int x2 = x1 + RGB_EDITOR_BOX_WIDTH-3,
			y2 = y1 + RGB_EDITOR_BOX_DEPTH-3;

		fill_rectangle(x1, y1, RGB_EDITOR_BOX_WIDTH-2, RGB_EDITOR_BOX_DEPTH-2, s_bg_color);
		driver_draw_line(x1, y1, x2, y2, s_fg_color);
		driver_draw_line(x1, y2, x2, y1, s_fg_color);
	}
	else
	{
		fill_rectangle(x1, y1, RGB_EDITOR_BOX_WIDTH-2, RGB_EDITOR_BOX_DEPTH-2, me->pal);
	}

	color_editor_draw(me->color[0]);
	color_editor_draw(me->color[1]);
	color_editor_draw(me->color[2]);
	cursor_show();
}

static void rgb_editor_draw(rgb_editor *me)
{
	if (me->hidden)
	{
		return ;
	}

	cursor_hide();
	dotted_rectangle(me->x, me->y, RGB_EDITOR_WIDTH, RGB_EDITOR_DEPTH);
	fill_rectangle(me->x + 1, me->y + 1, RGB_EDITOR_WIDTH-2, RGB_EDITOR_DEPTH-2, s_bg_color);
	rectangle(me->x + 1 + COLOR_EDITOR_WIDTH + 2, me->y + 2, RGB_EDITOR_BOX_WIDTH, RGB_EDITOR_BOX_DEPTH, s_fg_color);
	rgb_editor_update(me);
	cursor_show();
}

static int rgb_editor_edit(rgb_editor *me)
{
	int key = 0;

	me->done = FALSE;

	if (!me->hidden)
	{
		cursor_hide();
		rectangle(me->x, me->y, RGB_EDITOR_WIDTH, RGB_EDITOR_DEPTH, s_fg_color);
		cursor_show();
	}

	while (!me->done)
	{
		key = color_editor_edit(me->color[me->curr]);
	}

	if (!me->hidden)
	{
		cursor_hide();
		dotted_rectangle(me->x, me->y, RGB_EDITOR_WIDTH, RGB_EDITOR_DEPTH);
		cursor_show();
	}

	return key;
}

static void rgb_editor_set_rgb(rgb_editor *me, int pal, PALENTRY *rgb)
{
	me->pal = pal;
	color_editor_set_value(me->color[0], rgb->red);
	color_editor_set_value(me->color[1], rgb->green);
	color_editor_set_value(me->color[2], rgb->blue);
}

static PALENTRY rgb_editor_get_rgb(rgb_editor *me)
{
	PALENTRY pal;

	pal.red   = (BYTE)color_editor_get_value(me->color[0]);
	pal.green = (BYTE)color_editor_get_value(me->color[1]);
	pal.blue  = (BYTE)color_editor_get_value(me->color[2]);

	return pal;
}

/*
 * Class:     pal_table
 *
 * Purpose:   This is where it all comes together.  Creates the two RGBEditors
 *            and the palette. Moves the cursor, hides/restores the screen,
 *            handles (S)hading, (C)opying, e(X)clude mode, the "Y" exclusion
 *            mode, (Z)oom option, (H)ide palette, rotation, etc.
 *
 */
/*

Modes:
	Auto:          "A", " "
	Exclusion:     "X", "Y", " "
	Freestyle:     "F", " "
	S(t)ripe mode: "T", " "

*/
#define EXCLUDE_NONE	0
#define EXCLUDE_CURRENT	1
#define EXCLUDE_RANGE	2

struct tag_pal_table
{
	int           x, y;
	int           csize;
	int           active;   /* which rgb_editor is active (0, 1) */
	int           curr[2];
	rgb_editor    *rgb[2];
	move_box      *movebox;
	BOOLEAN       done;
	int exclude;
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
	int           top, bottom; /* top and bottom colours of freestyle band */
	int           bandwidth; /*size of freestyle colour band */
	BOOLEAN       freestyle;
};
typedef struct tag_pal_table pal_table;

static void    pal_table_draw_status  (pal_table *me, BOOLEAN stripe_mode);
static void    pal_table_highlight_pal       (pal_table *me, int pnum, int color);
static void    pal_table_draw        (pal_table *me);
static BOOLEAN pal_table_set_current     (pal_table *me, int which, int curr);
static BOOLEAN pal_table_memory_alloc (pal_table *me, long size);
static void    pal_table_save_rect    (pal_table *me);
static void    pal_table_restore_rect (pal_table *me);
static void    pal_table_set_position      (pal_table *me, int x, int y);
static void    pal_table_set_csize    (pal_table *me, int csize);
static int     pal_table_get_cursor_color(pal_table *me);
static void    pal_table_do_cursor      (pal_table *me, int key);
static void    pal_table_rotate      (pal_table *me, int dir, int lo, int hi);
static void    pal_table_update_dac   (pal_table *me);
static void    pal_table_other_key   (int key, rgb_editor *rgb, VOIDPTR info);
static void    pal_table_save_undo_data(pal_table *me, int first, int last);
static void    pal_table_save_undo_rotate(pal_table *me, int dir, int first, int last);
static void    pal_table_undo_process (pal_table *me, int delta);
static void    pal_table_undo        (pal_table *me);
static void    pal_table_redo        (pal_table *me);
static void    pal_table_change      (rgb_editor *rgb, VOIDPTR info);
static pal_table *pal_table_new(void);
static void      pal_table_destroy   (pal_table *me);
static void      pal_table_process   (pal_table *me);
static void      pal_table_set_hidden (pal_table *me, BOOLEAN hidden);
static void      pal_table_hide      (pal_table *me, rgb_editor *rgb, BOOLEAN hidden);

#define PALTABLE_PALX (1)
#define PALTABLE_PALY (2 + RGB_EDITOR_DEPTH + 2)
#define UNDO_DATA        (1)
#define UNDO_DATA_SINGLE (2)
#define UNDO_ROTATE      (3)

/*  - Freestyle code - */

static void pal_table_calc_top_bottom(pal_table *me)
{
	me->bottom = (me->curr[me->active] < me->bandwidth)
		? 0    : (me->curr[me->active]) - me->bandwidth;
	me->top    = (me->curr[me->active] > (255-me->bandwidth))
		? 255  : (me->curr[me->active]) + me->bandwidth;
}

static void pal_table_put_band(pal_table *me, PALENTRY *pal)
{
	int r, b, a;

	/* clip top and bottom values to stop them running off the end of the DAC */

	pal_table_calc_top_bottom(me);

	/* put bands either side of current colour */

	a = me->curr[me->active];
	b = me->bottom;
	r = me->top;

	pal[a] = me->fs_color;

	if (r != a && a != b)
	{
		make_pal_range(&pal[a], &pal[r], &pal[a], r-a, 1);
		make_pal_range(&pal[b], &pal[a], &pal[b], a-b, 1);
	}

}

/* - Undo.Redo code - */
static void pal_table_save_undo_data(pal_table *me, int first, int last)
{
	int num;

	if (me->undo_file == NULL)
	{
		return ;
	}

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

static void pal_table_save_undo_rotate(pal_table *me, int dir, int first, int last)
{
	if (me->undo_file == NULL)
	{
		return;
	}

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

static void pal_table_undo_process(pal_table *me, int delta)   /* undo/redo common code */
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
			{
				first = last = (unsigned char)getc(me->undo_file);
			}

			num = (last - first) + 1;

#ifdef DEBUG_UNDO
			mprintf("          Reading DATA from %d to %d", first, last);
#endif

			fread(temp, 3, num, me->undo_file);

			fseek(me->undo_file, -(num*3), SEEK_CUR);  /* go to start of undo/redo data */
			fwrite(me->pal + first, 3, num, me->undo_file);  /* write redo/undo data */

			memmove(me->pal + first, temp, num*3);

			pal_table_update_dac(me);

			rgb_editor_set_rgb(me->rgb[0], me->curr[0], &(me->pal[me->curr[0]]));
			rgb_editor_set_rgb(me->rgb[1], me->curr[1], &(me->pal[me->curr[1]]));
			rgb_editor_update(me->rgb[0]);
			rgb_editor_update(me->rgb[1]);
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
			pal_table_rotate(me, delta*dir, first, last);
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

static void pal_table_undo(pal_table *me)
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
	pal_table_undo_process(me, -1);
	fseek(me->undo_file, pos, SEEK_SET);   /* go to start of me g_block */
	++me->num_redo;
}

static void pal_table_redo(pal_table *me)
{
	if (me->num_redo <= 0)
	{
		return ;
	}

#ifdef DEBUG_UNDO
	mprintf("%6ld Redo:", ftell(me->undo_file));
#endif

	fseek(me->undo_file, 0, SEEK_CUR);  /* to make sure we are in "read" mode */
	pal_table_undo_process(me, 1);

	--me->num_redo;
}

#define STATUS_LEN (4)

static void pal_table_draw_status(pal_table *me, BOOLEAN stripe_mode)
{
	int color;
	int width = 1 + (me->csize*16) + 1 + 1;

	if (!me->hidden && (width - (RGB_EDITOR_WIDTH*2 + 4) >= STATUS_LEN*8))
	{
		int x = me->x + 2 + RGB_EDITOR_WIDTH,
			y = me->y + PALTABLE_PALY - 10;
		color = pal_table_get_cursor_color(me);
		if (color < 0 || color >= g_colors) /* hmm, the border returns -1 */
		{
			color = 0;
		}
		cursor_hide();

		{
			char buff[80];
			sprintf(buff, "%c%c%c%c",
				me->auto_select ? 'A' : ' ',
				(me->exclude == EXCLUDE_CURRENT)  ? 'X' : (me->exclude == EXCLUDE_RANGE) ? 'Y' : ' ',
				me->freestyle ? 'F' : ' ',
				stripe_mode ? 'T' : ' ');
			driver_display_string(x, y, s_fg_color, s_bg_color, buff);

			y = y - 10;
			sprintf(buff, "%d", color);
			driver_display_string(x, y, s_fg_color, s_bg_color, buff);
		}
		cursor_show();
	}
}

static void pal_table_highlight_pal(pal_table *me, int pnum, int color)
{
	int x    = me->x + PALTABLE_PALX + (pnum % 16)*me->csize,
		y    = me->y + PALTABLE_PALY + (pnum/16)*me->csize,
		size = me->csize;

	if (me->hidden)
	{
		return ;
	}

	cursor_hide();

	if (color < 0)
	{
		dotted_rectangle(x, y, size + 1, size + 1);
	}
	else
	{
		rectangle(x, y, size + 1, size + 1, color);
	}

	cursor_show();
}

static void pal_table_draw(pal_table *me)
{
	int pal;
	int xoff, yoff;
	int width;

	if (me->hidden)
	{
		return ;
	}

	cursor_hide();
	width = 1 + (me->csize*16) + 1 + 1;
	rectangle(me->x, me->y, width, 2 + RGB_EDITOR_DEPTH + 2 + (me->csize*16) + 1 + 1, s_fg_color);
	fill_rectangle(me->x + 1, me->y + 1, width-2, 2 + RGB_EDITOR_DEPTH + 2 + (me->csize*16) + 1 + 1-2, s_bg_color);
	horizontal_line(me->x, me->y + PALTABLE_PALY-1, width, s_fg_color);
	if (width - (RGB_EDITOR_WIDTH*2 + 4) >= TITLE_LEN*8)
	{
		int center = (width - TITLE_LEN*8) / 2;

		displayf(me->x + center, me->y + RGB_EDITOR_DEPTH/2-6, s_fg_color, s_bg_color, TITLE);
	}

	rgb_editor_draw(me->rgb[0]);
	rgb_editor_draw(me->rgb[1]);

	for (pal = 0; pal < 256; pal++)
	{
		xoff = PALTABLE_PALX + (pal % 16)*me->csize;
		yoff = PALTABLE_PALY + (pal/16)*me->csize;

		if (pal >= g_colors)
		{
			fill_rectangle(me->x + xoff + 1, me->y + yoff + 1, me->csize-1, me->csize-1, s_bg_color);
			draw_diamond(me->x + xoff + me->csize/2 - 1, me->y + yoff + me->csize/2 - 1, s_fg_color);
		}
		else if (is_reserved(pal))
		{
			int x1 = me->x + xoff + 1,
				y1 = me->y + yoff + 1,
				x2 = x1 + me->csize - 2,
				y2 = y1 + me->csize - 2;
			fill_rectangle(me->x + xoff + 1, me->y + yoff + 1, me->csize-1, me->csize-1, s_bg_color);
			driver_draw_line(x1, y1, x2, y2, s_fg_color);
			driver_draw_line(x1, y2, x2, y1, s_fg_color);
		}
		else
		{
			fill_rectangle(me->x + xoff + 1, me->y + yoff + 1, me->csize-1, me->csize-1, pal);
		}
	}

	if (me->active == 0)
	{
		pal_table_highlight_pal(me, me->curr[1], -1);
		pal_table_highlight_pal(me, me->curr[0], s_fg_color);
	}
	else
	{
		pal_table_highlight_pal(me, me->curr[0], -1);
		pal_table_highlight_pal(me, me->curr[1], s_fg_color);
	}

	pal_table_draw_status(me, FALSE);
	cursor_show();
}

static BOOLEAN pal_table_set_current(pal_table *me, int which, int curr)
{
	BOOLEAN redraw = (BOOLEAN)((which < 0) ? TRUE : FALSE);

	if (redraw)
	{
		which = me->active;
		curr = me->curr[which];
	}
	else if (curr == me->curr[which] || curr < 0)
	{
		return FALSE;
	}

	cursor_hide();

	pal_table_highlight_pal(me, me->curr[0], s_bg_color);
	pal_table_highlight_pal(me, me->curr[1], s_bg_color);
	pal_table_highlight_pal(me, me->top,     s_bg_color);
	pal_table_highlight_pal(me, me->bottom,  s_bg_color);

	if (me->freestyle)
	{
		me->curr[which] = curr;
		pal_table_calc_top_bottom(me);
		pal_table_highlight_pal(me, me->top,    -1);
		pal_table_highlight_pal(me, me->bottom, -1);
		pal_table_highlight_pal(me, me->curr[me->active], s_fg_color);
		rgb_editor_set_rgb(me->rgb[which], me->curr[which], &me->fs_color);
		rgb_editor_update(me->rgb[which]);
		pal_table_update_dac(me);
		cursor_show();
		return TRUE;
	}

	me->curr[which] = curr;

	if (me->curr[0] != me->curr[1])
	{
		pal_table_highlight_pal(me, me->curr[me->active == 0 ? 1 : 0], -1);
	}
	pal_table_highlight_pal(me, me->curr[me->active], s_fg_color);

	rgb_editor_set_rgb(me->rgb[which], me->curr[which], &(me->pal[me->curr[which]]));

	if (redraw)
	{
		int other = (which == 0) ? 1 : 0;
		rgb_editor_set_rgb(me->rgb[other], me->curr[other], &(me->pal[me->curr[other]]));
		rgb_editor_update(me->rgb[0]);
		rgb_editor_update(me->rgb[1]);
	}
	else
	{
		rgb_editor_update(me->rgb[which]);
	}

	if (me->exclude)
	{
		pal_table_update_dac(me);
	}

	cursor_show();
	me->curr_changed = FALSE;
	return TRUE;
}


static BOOLEAN pal_table_memory_alloc(pal_table *me, long size)
{
	char *temp;

	if (DEBUGFLAG_USE_DISK == g_debug_flag)
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


static void pal_table_save_rect(pal_table *me)
{
	char buff[MAX_WIDTH];
	int  width = PALTABLE_PALX + me->csize*16 + 1 + 1,
		depth = PALTABLE_PALY + me->csize*16 + 1 + 1;
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
		{
			free(me->memory);
		}
		me->memory = NULL;
		break;
	}

	/* allocate space and store the rectangle */

	if (pal_table_memory_alloc(me, (long)width*depth))
	{
		char  *ptr = me->memory;
		char  *bufptr = buff; /* MSC needs me indirection to get it right */

		cursor_hide();
		for (yoff = 0; yoff < depth; yoff++)
		{
			get_row(me->x, me->y + yoff, width, buff);
			horizontal_line (me->x, me->y + yoff, width, s_bg_color);
			memcpy(ptr, bufptr, width);
			ptr += width;
		}
		cursor_show();
	}
	else /* to disk */
	{
		me->stored_at = DISK;

		if (me->file == NULL)
		{
			me->file = dir_fopen(g_temp_dir, g_screen_file, "w + b");
			if (me->file == NULL)
			{
				me->stored_at = NOWHERE;
				driver_buzzer(BUZZER_ERROR);
				return ;
			}
		}

		rewind(me->file);
		cursor_hide();
		for (yoff = 0; yoff < depth; yoff++)
		{
			get_row(me->x, me->y + yoff, width, buff);
			horizontal_line (me->x, me->y + yoff, width, s_bg_color);
			if (fwrite(buff, width, 1, me->file) != 1)
			{
				driver_buzzer(BUZZER_ERROR);
				break;
			}
		}
		cursor_show();
	}
}


static void pal_table_restore_rect(pal_table *me)
{
	char buff[MAX_WIDTH];
	int  width = PALTABLE_PALX + me->csize*16 + 1 + 1,
		depth = PALTABLE_PALY + me->csize*16 + 1 + 1;
	int  yoff;

	if (me->hidden)
	{
		return;
	}

	switch (me->stored_at)
	{
	case DISK:
		rewind(me->file);
		cursor_hide();
		for (yoff = 0; yoff < depth; yoff++)
		{
			if (fread(buff, width, 1, me->file) != 1)
			{
				driver_buzzer(BUZZER_ERROR);
				break;
				}
			put_row(me->x, me->y + yoff, width, buff);
		}
		cursor_show();
		break;

	case MEMORY:
		{
			char  *ptr = me->memory;
			char  *bufptr = buff; /* MSC needs me indirection to get it right */

			cursor_hide();
			for (yoff = 0; yoff < depth; yoff++)
			{
				memcpy(bufptr, ptr, width);
				put_row(me->x, me->y + yoff, width, buff);
				ptr += width;
			}
			cursor_show();
			break;
		}

	case NOWHERE:
		break;
	} /* switch */
}


static void pal_table_set_position(pal_table *me, int x, int y)
{
	int width = PALTABLE_PALX + me->csize*16 + 1 + 1;

	me->x = x;
	me->y = y;

	rgb_editor_set_position(me->rgb[0], x + 2, y + 2);
	rgb_editor_set_position(me->rgb[1], x + width-2-RGB_EDITOR_WIDTH, y + 2);
}


static void pal_table_set_csize(pal_table *me, int csize)
{
	me->csize = csize;
	pal_table_set_position(me, me->x, me->y);
}


static int pal_table_get_cursor_color(pal_table *me)
{
	int x     = cursor_get_x(),
		y     = cursor_get_y(),
		size;
	int color = getcolor(x, y);

	if (is_reserved(color))
	{
		if (is_in_box(x, y, me->x, me->y, 1 + (me->csize*16) + 1 + 1, 2 + RGB_EDITOR_DEPTH + 2 + (me->csize*16) + 1 + 1))
		{  /* is the cursor over the editor? */
			x -= me->x + PALTABLE_PALX;
			y -= me->y + PALTABLE_PALY;
			size = me->csize;

			if (x < 0 || y < 0 || x > size*16 || y > size*16)
			{
				return -1;
			}

			if (x == size*16)
			{
				--x;
			}
			if (y == size*16)
			{
				--y;
			}

			return (y/size)*16 + x/size;
		}
		else
		{
			return color;
		}
	}

	return color;
}



#define CURS_INC 1

static void pal_table_do_cursor(pal_table *me, int key)
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
			{
				driver_get_key();       /* delete key from buffer */
			}
			else
			{
				first = FALSE;
			}
			key = driver_key_pressed();   /* peek at the next one... */
		}
	}

	cursor_move(xoff, yoff);

	if (me->auto_select)
	{
		pal_table_set_current(me, me->active, pal_table_get_cursor_color(me));
	}
}


#ifdef __CLINT__
#   pragma argsused
#endif

static void pal_table_change(rgb_editor *rgb, VOIDPTR info)
{
	pal_table *me = (pal_table *)info;
	int       pnum = me->curr[me->active];

	if (me->freestyle)
	{
		me->fs_color = rgb_editor_get_rgb(rgb);
		pal_table_update_dac(me);
		return ;
	}

	if (!me->curr_changed)
	{
		pal_table_save_undo_data(me, pnum, pnum);
		me->curr_changed = TRUE;
	}

	me->pal[pnum] = rgb_editor_get_rgb(rgb);

	if (me->curr[0] == me->curr[1])
		{
		int      other = me->active == 0 ? 1 : 0;
		PALENTRY color;

		color = rgb_editor_get_rgb(me->rgb[me->active]);
		rgb_editor_set_rgb(me->rgb[other], me->curr[other], &color);

		cursor_hide();
		rgb_editor_update(me->rgb[other]);
		cursor_show();
		}

}


static void pal_table_update_dac(pal_table *me)
{
	if (me->exclude)
	{
		memset(g_dac_box, 0, 256*3);
		if (me->exclude == EXCLUDE_CURRENT)
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
				int t = a;
				a = b;
				b = t;
			}

			memmove(g_dac_box[a], &me->pal[a], 3*(1 + (b-a)));
		}
	}
	else
	{
		memmove(g_dac_box[0], me->pal, 3*g_colors);

		if (me->freestyle)
		{
			pal_table_put_band(me, (PALENTRY *) g_dac_box);   /* apply band to g_dac_box */
		}
	}

	if (!me->hidden)
	{
		if (s_inverse)
		{
			memset(g_dac_box[s_fg_color], 0, 3);         /* g_dac_box[fg] = (0, 0, 0) */
			memset(g_dac_box[s_bg_color], 48, 3);        /* g_dac_box[bg] = (48, 48, 48) */
		}
		else
		{
			memset(g_dac_box[s_bg_color], 0, 3);         /* g_dac_box[bg] = (0, 0, 0) */
			memset(g_dac_box[s_fg_color], 48, 3);        /* g_dac_box[fg] = (48, 48, 48) */
		}
	}

	spindac(0, 1);
}


static void pal_table_rotate(pal_table *me, int dir, int lo, int hi)
{

	rotate_pal(me->pal, dir, lo, hi);

	cursor_hide();

	/* update the DAC.  */

	pal_table_update_dac(me);

	/* update the editors. */

	rgb_editor_set_rgb(me->rgb[0], me->curr[0], &(me->pal[me->curr[0]]));
	rgb_editor_set_rgb(me->rgb[1], me->curr[1], &(me->pal[me->curr[1]]));
	rgb_editor_update(me->rgb[0]);
	rgb_editor_update(me->rgb[1]);

	cursor_show();
}


static void pal_table_other_key(int key, rgb_editor *rgb, VOIDPTR info)
{
	pal_table *me = (pal_table *)info;

	switch (key)
	{
	case '\\':    /* move/resize */
		if (me->hidden)
		{
			break;           /* cannot move a hidden pal */
		}
		cursor_hide();
		pal_table_restore_rect(me);
		move_box_set_position(me->movebox, me->x, me->y);
		move_box_set_csize(me->movebox, me->csize);
		if (move_box_process(me->movebox))
		{
			if (move_box_should_hide(me->movebox))
			{
				pal_table_set_hidden(me, TRUE);
			}
			else if (move_box_moved(me->movebox))
			{
				pal_table_set_position(me, move_box_x(me->movebox), move_box_y(me->movebox));
				pal_table_set_csize(me, move_box_csize(me->movebox));
				pal_table_save_rect(me);
			}
		}
		pal_table_draw(me);
		cursor_show();

		rgb_editor_set_done(me->rgb[me->active], TRUE);

		if (me->auto_select)
		{
			pal_table_set_current(me, me->active, pal_table_get_cursor_color(me));
		}
		break;

	case 'Y':    /* exclude range */
	case 'y':
		me->exclude = (me->exclude == EXCLUDE_RANGE) ? EXCLUDE_NONE : EXCLUDE_RANGE;
		pal_table_update_dac(me);
		break;

	case 'X':
	case 'x':     /* exclude current entry */
		me->exclude = (me->exclude == EXCLUDE_CURRENT) ? EXCLUDE_NONE : EXCLUDE_CURRENT;
		pal_table_update_dac(me);
		break;

	case FIK_RIGHT_ARROW:
	case FIK_LEFT_ARROW:
	case FIK_UP_ARROW:
	case FIK_DOWN_ARROW:
	case FIK_CTL_RIGHT_ARROW:
	case FIK_CTL_LEFT_ARROW:
	case FIK_CTL_UP_ARROW:
	case FIK_CTL_DOWN_ARROW:
		pal_table_do_cursor(me, key);
		break;

	case FIK_ESC:
		me->done = TRUE;
		rgb_editor_set_done(rgb, TRUE);
		break;

	case ' ':     /* select the other palette register */
		me->active = (me->active == 0) ? 1 : 0;
		if (me->auto_select)
		{
			pal_table_set_current(me, me->active, pal_table_get_cursor_color(me));
		}
		else
		{
			pal_table_set_current(me, -1, 0);
		}
		if (me->exclude || me->freestyle)
		{
			pal_table_update_dac(me);
		}
		rgb_editor_set_done(rgb, TRUE);
		break;

	case FIK_ENTER:    /* set register to color under cursor.  useful when not */
	case FIK_ENTER_2:  /* in auto_select mode */
		if (me->freestyle)
		{
			pal_table_save_undo_data(me, me->bottom, me->top);
			pal_table_put_band(me, me->pal);
		}

		pal_table_set_current(me, me->active, pal_table_get_cursor_color(me));

		if (me->exclude || me->freestyle)
		{
			pal_table_update_dac(me);
		}

		rgb_editor_set_done(rgb, TRUE);
		break;

	case 'D':    /* copy (Duplicate?) color in inactive to color in active */
	case 'd':
		{
			int   a = me->active,
				b = (a == 0) ? 1 : 0;
			PALENTRY t;

			t = rgb_editor_get_rgb(me->rgb[b]);
			cursor_hide();

			rgb_editor_set_rgb(me->rgb[a], me->curr[a], &t);
			rgb_editor_update(me->rgb[a]);
			pal_table_change(me->rgb[a], me);
			pal_table_update_dac(me);

			cursor_show();
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

			pal_table_save_undo_data(me, a, b);

			if (a != b)
			{
				make_pal_range(&me->pal[a], &me->pal[b], &me->pal[a], b-a, 1);
				pal_table_update_dac(me);
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

			pal_table_save_undo_data(me, a, b);

			if (a != b)
			{
				swap_columns_rg(&me->pal[a], b-a);
				pal_table_update_dac(me);
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

			pal_table_save_undo_data(me, a, b);

			if (a != b)
			{
				swap_columns_gb(&me->pal[a], b-a);
				pal_table_update_dac(me);
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

			pal_table_save_undo_data(me, a, b);

			if (a != b)
			{
				swap_columns_br(&me->pal[a], b-a);
				pal_table_update_dac(me);
			}

			break;
		}

	case 'T':
	case 't':   /* s(T)ripe mode */
		{
			int key;

			cursor_hide();
			pal_table_draw_status(me, TRUE);
			key = getakeynohelp();
			cursor_show();

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

				pal_table_save_undo_data(me, a, b);

				if (a != b)
				{
					make_pal_range(&me->pal[a], &me->pal[b], &me->pal[a], b-a, key-'0');
					pal_table_update_dac(me);
				}
			}
			break;
		}

	case 'M':   /* set gamma */
	case 'm':
		{
			int i;
			char buf[20];
			sprintf(buf, "%.3f", 1./s_gamma_val);
			driver_stack_screen();
			i = field_prompt("Enter gamma value", NULL, buf, 20, NULL);
			driver_unstack_screen();
			if (i != -1)
			{
				sscanf(buf, "%f", &s_gamma_val);
				if (s_gamma_val == 0)
				{
					s_gamma_val = 0.0000000001f;
				}
				s_gamma_val = (float)(1./s_gamma_val);
			}
		}
		break;

	case 'A':   /* toggle auto-select mode */
	case 'a':
		me->auto_select = (BOOLEAN)((me->auto_select) ? FALSE : TRUE);
		if (me->auto_select)
		{
			pal_table_set_current(me, me->active, pal_table_get_cursor_color(me));
			if (me->exclude)
			{
				pal_table_update_dac(me);
			}
		}
		break;

	case 'H':
	case 'h': /* toggle hide/display of palette editor */
		cursor_hide();
		pal_table_hide(me, rgb, (BOOLEAN)((me->hidden) ? FALSE : TRUE));
		cursor_show();
		break;

	case '.':   /* rotate once */
	case ',':
		{
			int dir = (key == '.') ? 1 : -1;

			pal_table_save_undo_rotate(me, dir, g_rotate_lo, g_rotate_hi);
			pal_table_rotate(me, dir, g_rotate_lo, g_rotate_hi);
			break;
		}

	case '>':   /* continuous rotation (until a key is pressed) */
	case '<':
		{
			int  dir;
			long tick;
			int  diff = 0;

			cursor_hide();

			if (!me->hidden)
			{
				rgb_editor_blank_sample_box(me->rgb[0]);
				rgb_editor_blank_sample_box(me->rgb[1]);
				rgb_editor_set_hidden(me->rgb[0], TRUE);
				rgb_editor_set_hidden(me->rgb[1], TRUE);
			}

			do
			{
				dir = (key == '>') ? 1 : -1;

				while (!driver_key_pressed())
				{
					tick = readticker();
					pal_table_rotate(me, dir, g_rotate_lo, g_rotate_hi);
					diff += dir;
					while (readticker() == tick)   /* wait until a tick passes */
					{
					}
				}

				key = driver_get_key();
			}
			while (key == '<' || key == '>');

			if (!me->hidden)
			{
				rgb_editor_set_hidden(me->rgb[0], FALSE);
				rgb_editor_set_hidden(me->rgb[1], FALSE);
				rgb_editor_update(me->rgb[0]);
				rgb_editor_update(me->rgb[1]);
			}

			if (diff != 0)
			{
				pal_table_save_undo_rotate(me, diff, g_rotate_lo, g_rotate_hi);
			}

			cursor_show();
			break;
		}

	case 'I':     /* invert the fg & bg g_colors */
	case 'i':
		s_inverse = (BOOLEAN)!s_inverse;
		pal_table_update_dac(me);
		break;

	case 'V':
	case 'v':  /* set the reserved g_colors to the editor g_colors */
		if (me->curr[0] >= g_colors || me->curr[1] >= g_colors ||
			me->curr[0] == me->curr[1])
		{
			driver_buzzer(BUZZER_ERROR);
			break;
		}

		s_fg_color = (BYTE)me->curr[0];
		s_bg_color = (BYTE)me->curr[1];

		if (!me->hidden)
		{
			cursor_hide();
			pal_table_update_dac(me);
			pal_table_draw(me);
			cursor_show();
		}

		rgb_editor_set_done(me->rgb[me->active], TRUE);
		break;

	case 'O':    /* set rotate_lo and rotate_hi to editors */
	case 'o':
		if (me->curr[0] > me->curr[1])
		{
			g_rotate_lo = me->curr[1];
			g_rotate_hi = me->curr[0];
		}
		else
		{
			g_rotate_lo = me->curr[0];
			g_rotate_hi = me->curr[1];
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
				cursor_hide();

				pal_table_save_undo_data(me, 0, 255);
				memcpy(me->pal, me->save_pal[which], 256*3);
				pal_table_update_dac(me);

				pal_table_set_current(me, -1, 0);
				cursor_show();
				rgb_editor_set_done(me->rgb[me->active], TRUE);
			}
			else
			{
				driver_buzzer(BUZZER_ERROR);   /* error buzz */
			}
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
				memcpy(me->save_pal[which], me->pal, 256*3);
			}
			else
			{
				driver_buzzer(BUZZER_ERROR); /* oops! short on memory! */
			}
			break;
		}

	case 'L':     /* load a .map palette */
	case 'l':
		pal_table_save_undo_data(me, 0, 255);
		load_palette();
#ifndef XFRACT
		get_pal_range(0, g_colors, me->pal);
#else
		get_pal_range(0, 256, me->pal);
#endif
		pal_table_update_dac(me);
		rgb_editor_set_rgb(me->rgb[0], me->curr[0], &(me->pal[me->curr[0]]));
		rgb_editor_update(me->rgb[0]);
		rgb_editor_set_rgb(me->rgb[1], me->curr[1], &(me->pal[me->curr[1]]));
		rgb_editor_update(me->rgb[1]);
		break;

	case 'S':     /* save a .map palette */
	case 's':
#ifndef XFRACT
		set_pal_range(0, g_colors, me->pal);
#else
		set_pal_range(0, 256, me->pal);
#endif
		save_palette();
		pal_table_update_dac(me);
		break;

	case 'C':     /* color cycling sub-mode */
	case 'c':
		{
			BOOLEAN oldhidden = (BOOLEAN)me->hidden;

			pal_table_save_undo_data(me, 0, 255);

			cursor_hide();
			if (!oldhidden)
			{
				pal_table_hide(me, rgb, TRUE);
			}
			set_pal_range(0, g_colors, me->pal);
			rotate(0);
			get_pal_range(0, g_colors, me->pal);
			pal_table_update_dac(me);
			if (!oldhidden)
			{
				rgb_editor_set_rgb(me->rgb[0], me->curr[0], &(me->pal[me->curr[0]]));
				rgb_editor_set_rgb(me->rgb[1], me->curr[1], &(me->pal[me->curr[1]]));
				pal_table_hide(me, rgb, FALSE);
			}
			cursor_show();
			break;
		}

	case 'F':
	case 'f':    /* toggle freestyle palette edit mode */
		me->freestyle = (BOOLEAN)((me->freestyle) ? FALSE :TRUE);
		pal_table_set_current(me, -1, 0);
		if (!me->freestyle)   /* if turning off... */
		{
			pal_table_update_dac(me);
		}
		break;

	case FIK_CTL_DEL:  /* rt plus down */
		if (me->bandwidth >0)
		{
			me->bandwidth--;
		}
		else
		{
			me->bandwidth = 0;
		}
		pal_table_set_current(me, -1, 0);
		break;

	case FIK_CTL_INSERT: /* rt plus up */
		if (me->bandwidth <255)
		{
			me->bandwidth ++;
		}
		else
		{
			me->bandwidth = 255;
		}
		pal_table_set_current(me, -1, 0);
		break;

	case 'W':   /* convert to greyscale */
	case 'w':
		switch (me->exclude)
		{
		case EXCLUDE_NONE:   /* normal mode.  convert all g_colors to grey scale */
			pal_table_save_undo_data(me, 0, 255);
			pal_range_to_grey(me->pal, 0, 256);
			break;

		case EXCLUDE_CURRENT:   /* 'x' mode. convert current color to grey scale.  */
			pal_table_save_undo_data(me, me->curr[me->active], me->curr[me->active]);
			pal_range_to_grey(me->pal, me->curr[me->active], 1);
			break;

		case EXCLUDE_RANGE:  /* 'y' mode.  convert range between editors to grey. */
			{
				int a = me->curr[0],
					b = me->curr[1];

				if (a > b)
				{
					int t = a;
					a = b;
					b = t;
				}

				pal_table_save_undo_data(me, a, b);
				pal_range_to_grey(me->pal, a, 1 + (b-a));
				break;
			}
		}

		pal_table_update_dac(me);
		rgb_editor_set_rgb(me->rgb[0], me->curr[0], &(me->pal[me->curr[0]]));
		rgb_editor_update(me->rgb[0]);
		rgb_editor_set_rgb(me->rgb[1], me->curr[1], &(me->pal[me->curr[1]]));
		rgb_editor_update(me->rgb[1]);
		break;

	case 'N':   /* convert to negative color */
	case 'n':
		switch (me->exclude)
		{
		case EXCLUDE_NONE:      /* normal mode.  convert all g_colors to grey scale */
			pal_table_save_undo_data(me, 0, 255);
			pal_range_to_negative(me->pal, 0, 256);
			break;

		case EXCLUDE_CURRENT:      /* 'x' mode. convert current color to grey scale.  */
			pal_table_save_undo_data(me, me->curr[me->active], me->curr[me->active]);
			pal_range_to_negative(me->pal, me->curr[me->active], 1);
			break;

		case EXCLUDE_RANGE:  /* 'y' mode.  convert range between editors to grey. */
			{
				int a = me->curr[0],
				b = me->curr[1];

				if (a > b)
				{
					int t = a;
					a = b;
					b = t;
				}

				pal_table_save_undo_data(me, a, b);
				pal_range_to_negative(me->pal, a, 1 + (b-a));
				break;
			}
		}

		pal_table_update_dac(me);
		rgb_editor_set_rgb(me->rgb[0], me->curr[0], &(me->pal[me->curr[0]]));
		rgb_editor_update(me->rgb[0]);
		rgb_editor_set_rgb(me->rgb[1], me->curr[1], &(me->pal[me->curr[1]]));
		rgb_editor_update(me->rgb[1]);
		break;

	case 'U':     /* Undo */
	case 'u':
		pal_table_undo(me);
		break;

	case 'e':    /* Redo */
	case 'E':
		pal_table_redo(me);
		break;
	} /* switch */
	pal_table_draw_status(me, FALSE);
}

static void pal_table_make_default_palettes(pal_table *me)  /* creates default Fkey palettes */
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



static pal_table *pal_table_new(void)
{
	pal_table     *me = NEWC(pal_table);
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
			{
				me->save_pal[ctr] = NULL;
			}
		}
		else
		{
			for (ctr = 0; ctr < 8; ctr++)
			{
				me->save_pal[ctr] = mem_block + (256*ctr);
			}
		}
		free(temp);
	}

	me->rgb[0] = rgb_editor_new(0, 0, pal_table_other_key,
						pal_table_change, me);
	me->rgb[1] = rgb_editor_new(0, 0, pal_table_other_key,
						pal_table_change, me);

	me->movebox = move_box_new(0, 0, 0, PALTABLE_PALX + 1, PALTABLE_PALY + 1);

	me->active      = 0;
	me->curr[0]     = 1;
	me->curr[1]     = 1;
	me->auto_select = TRUE;
	me->exclude     = EXCLUDE_NONE;
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

	me->undo_file    = dir_fopen(g_temp_dir, s_undo_file, "w+b");
	me->curr_changed = FALSE;
	me->num_redo     = 0;

	rgb_editor_set_rgb(me->rgb[0], me->curr[0], &me->pal[me->curr[0]]);
	rgb_editor_set_rgb(me->rgb[1], me->curr[1], &me->pal[me->curr[0]]);

	pal_table_set_position(me, 0, 0);
	csize = ((g_screen_height-(PALTABLE_PALY + 1 + 1)) / 2) / 16;

	if (csize < CSIZE_MIN)
	{
		csize = CSIZE_MIN;
	}
	pal_table_set_csize(me, csize);

	return me;
}


static void pal_table_set_hidden(pal_table *me, BOOLEAN hidden)
{
	me->hidden = hidden;
	rgb_editor_set_hidden(me->rgb[0], hidden);
	rgb_editor_set_hidden(me->rgb[1], hidden);
	pal_table_update_dac(me);
}



static void pal_table_hide(pal_table *me, rgb_editor *rgb, BOOLEAN hidden)
{
	if (hidden)
	{
		pal_table_restore_rect(me);
		pal_table_set_hidden(me, TRUE);
		s_reserve_colors = FALSE;
		if (me->auto_select)
		{
			pal_table_set_current(me, me->active, pal_table_get_cursor_color(me));
		}
	}
	else
	{
		pal_table_set_hidden(me, FALSE);
		s_reserve_colors = TRUE;
		if (me->stored_at == NOWHERE)  /* do we need to save screen? */
		{
			pal_table_save_rect(me);
		}
		pal_table_draw(me);
		if (me->auto_select)
		{
			pal_table_set_current(me, me->active, pal_table_get_cursor_color(me));
		}
		rgb_editor_set_done(rgb, TRUE);
	}
}


static void pal_table_destroy(pal_table *me)
	{

	if (me->file != NULL)
		{
		fclose(me->file);
		dir_remove(g_temp_dir, g_screen_file);
		}

	if (me->undo_file != NULL)
		{
		fclose(me->undo_file);
		dir_remove(g_temp_dir, s_undo_file);
		}

	if (me->memory != NULL)
	{
		free(me->memory);
	}

	if (me->save_pal[0] != NULL)
	{
		free((BYTE *)me->save_pal[0]);
	}

	rgb_editor_destroy(me->rgb[0]);
	rgb_editor_destroy(me->rgb[1]);
	move_box_destroy(me->movebox);
	DELETE(me);
	}


static void pal_table_process(pal_table *me)
{
	int ctr;

	get_pal_range(0, g_colors, me->pal);

	/* Make sure all palette entries are 0-COLOR_CHANNEL_MAX */

	for (ctr = 0; ctr < 768; ctr++)
	{
		((char *)me->pal)[ctr] &= COLOR_CHANNEL_MAX;
	}

	pal_table_update_dac(me);

	rgb_editor_set_rgb(me->rgb[0], me->curr[0], &me->pal[me->curr[0]]);
	rgb_editor_set_rgb(me->rgb[1], me->curr[1], &me->pal[me->curr[0]]);

	if (!me->hidden)
	{
		move_box_set_position(me->movebox, me->x, me->y);
		move_box_set_csize(me->movebox, me->csize);
		if (!move_box_process(me->movebox))
		{
			set_pal_range(0, g_colors, me->pal);
			return ;
		}

		pal_table_set_position(me, move_box_x(me->movebox), move_box_y(me->movebox));
		pal_table_set_csize(me, move_box_csize(me->movebox));

		if (move_box_should_hide(me->movebox))
		{
			pal_table_set_hidden(me, TRUE);
			s_reserve_colors = FALSE;   /* <EAN> */
		}
		else
		{
			s_reserve_colors = TRUE;    /* <EAN> */
			pal_table_save_rect(me);
			pal_table_draw(me);
		}
	}

	pal_table_set_current(me, me->active,          pal_table_get_cursor_color(me));
	pal_table_set_current(me, (me->active == 1) ? 0 : 1, pal_table_get_cursor_color(me));
	cursor_show();
	pal_table_make_default_palettes(me);
	me->done = FALSE;

	while (!me->done)
	{
		rgb_editor_edit(me->rgb[me->active]);
	}

	cursor_hide();
	pal_table_restore_rect(me);
	set_pal_range(0, g_colors, me->pal);
}


/*
 * interface to FRACTINT
 */



void palette_edit(void)       /* called by fractint */
{
	int       oldlookatmouse = g_look_at_mouse;
	int       oldsxoffs      = g_sx_offset;
	int       oldsyoffs      = g_sy_offset;
	pal_table *pt;

	if (g_screen_width < 133 || g_screen_height < 174)
	{
		return; /* prevents crash when physical screen is too small */
	}

	push_help_mode(HELPXHAIR);

	g_plot_color = g_put_color;

	g_line_buffer = (BYTE *) malloc(max(g_screen_width, g_screen_height));

	g_look_at_mouse = LOOK_MOUSE_ZOOM_BOX;
	g_sx_offset = g_sy_offset = 0;

	s_reserve_colors = TRUE;
	s_inverse = FALSE;
	s_fg_color = (BYTE)(255 % g_colors);
	s_bg_color = (BYTE)(s_fg_color-1);

	cursor_new();
	pt = pal_table_new();
	pal_table_process(pt);
	pal_table_destroy(pt);
	cursor_destroy();

	g_look_at_mouse = oldlookatmouse;
	g_sx_offset = oldsxoffs;
	g_sy_offset = oldsyoffs;
	DELETE(g_line_buffer);

	pop_help_mode();
}
