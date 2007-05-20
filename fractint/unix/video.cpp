#include <curses.h>
#include <string.h>

#include "port.h"
#include "prototyp.h"
#include "externs.h"
#include "drivers.h"

/*
 * This file contains Unix versions of the routines in video.asm
 * Copyright 1992 Ken Shirriff
 */
WINDOW *g_current_window;

extern unsigned char *x_get_font();
extern int disk_start();
extern int wait_key_pressed(int);
extern int get_a_key();

int g_fake_lut = 0;
int g_is_true_color = 0;
int g_dac_learn = 0;
int g_dac_count = 0;
int g_good_mode = 0;		/* if non-zero, OK to read/write pixels */
void (*g_dot_write)(int, int, int);
				/* write-a-dot routine */
int (*g_dot_read)(int, int);	/* read-a-dot routine */
void (*g_line_write)(int, int, int, BYTE *);		/* write-a-line routine */
void (*g_line_read)(int, int, int, BYTE *);		/* read-a-line routine */
int g_and_color = 0;		/* "and" value used for color selection */
bool g_disk_flag = 0;		/* disk video active flag */

int videoflag = 0;		/* special "your-own-video" flag */

void (*swapsetup) () = NULL;	/* setfortext/graphics setup routine */
int g_color_dark = 0;		/* darkest color in palette */
int g_color_bright = 0;		/* brightest color in palette */
int g_color_medium = 0;		/* nearest to medbright grey in palette
				   Zoom-Box values (2K x 2K screens max) */
bool g_got_real_dac = false;		/* 1 if load_dac has a dacbox */
int g_row_count = 0;		/* row-counter for decoder and out_line */
int g_text_row = 0;		/* for driver_put_string(-1,...) */
int g_text_col = 0;		/* for driver_put_string(..,-1,...) */
int g_text_rbase = 0;		/* g_text_row is relative to this */
int g_text_cbase = 0;		/* g_text_col is relative to this */

int g_vesa_detect = 1;		/* set to 0 to disable VESA-detection */
int g_video_scroll = 0;
int g_video_start_x = 0;
int g_video_start_y = 0;
int g_vesa_x_res;
int g_vesa_y_res;

VIDEOINFO x11_video_table[] =
{
	{
		"xfractint mode           ", "                         ",
		999, 19, 640, 480, 256
	}
};

void setforgraphics();

static void null_write(int a, int b, int c)
{
}

static int null_read(int a, int b)
{
	return 0;
}

void set_null_video()
{
	g_dot_write = null_write;
	g_dot_read = null_read;
}

static void normal_line_read(int, int, int, BYTE *);
static void normal_line_write(int, int, int, BYTE *);

void put_prompt()
{
	wclear(g_current_window);		/* ???? */
	driver_put_string(0, 0, 0, "Press operation key, or <Esc> to return to Main Menu");
	wrefresh(g_current_window);
	return;
}

/*
; ********************** Function set_video_text() ************************

;       Sets video to text mode, using setvideomode to do the work.
*/
void set_video_text()
{
	g_dot_mode = 0;
	setvideomode(3, 0, 0, 0);
}

void load_dac()
{
	readvideopalette();
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

; Unix: We ignore ax,bx,cx,dx.  g_dot_mode is the "mode" field in the video
; table.  We use mode 19 for the X window.
*/
void setvideomode(int ax, int bx, int cx, int dx)
{
	if (g_disk_flag)
	{
		disk_end();
	}
	if (videoflag)
	{
		endvideo();
		videoflag = 0;
	}
	g_good_mode = 1;
	switch (g_dot_mode)
	{
	case 0:			/* text */
		clear();
		wrefresh(g_current_window);
		break;
	case 11:
		disk_start();
		g_dot_write = disk_write;
		g_dot_read = disk_read;
		g_line_read = normal_line_read;
		g_line_write = normal_line_write;
		break;
	case 19:			/* X window */
		put_prompt();
		g_dot_write = writevideo;
		g_dot_read = readvideo;
		g_line_read = readvideoline;
		g_line_write = writevideoline;
		videoflag = 1;
		startvideo();
		setforgraphics();
		break;
	default:
		printf("Bad mode %d\n", g_dot_mode);
		exit(-1);
	}
	if (g_dot_mode != 0)
	{
		load_dac();
		g_and_color = g_colors - 1;
		g_box_count = 0;
	}
	g_vesa_x_res = g_screen_width;
	g_vesa_y_res = g_screen_height;
}


/*
; **************** Function getcolor(xdot, ydot) *******************

;       Return the color on the screen at the (xdot,ydot) point
*/
int getcolor(int xdot, int ydot)
{
	int x1, y1;
	x1 = xdot + g_sx_offset;
	y1 = ydot + g_sy_offset;
	if (x1 < 0 || y1 < 0 || x1 >= g_screen_width || y1 >= g_screen_height)
	{
		return 0;
	}
	return g_dot_read(x1, y1);
}

/*
; ************** Function putcolor_a(xdot, ydot, color) *******************

;       write the color on the screen at the (xdot,ydot) point
*/
void putcolor_a(int xdot, int ydot, int color)
{
	g_dot_write(xdot + g_sx_offset, ydot + g_sy_offset, color & g_and_color);
}

/*
; **************** Function move_cursor(row, col)  **********************

;       Move the cursor (called before printfs)
*/
void move_cursor(int row, int col)
{
	if (row == -1)
	{
		row = g_text_row;
	}
	else
	{
		g_text_row = row;
	}
	if (col == -1)
	{
		col = g_text_col;
	}
	else
	{
		g_text_col = col;
	}
	wmove(g_current_window, row, col);
}

/*
; **************** Function keycursor(row, col)  **********************

;       Subroutine to wait cx ticks, or till keystroke pending
*/
int keycursor(int row, int col)
{
	move_cursor(row, col);
	wrefresh(g_current_window);
	wait_key_pressed(0);
	return get_a_key();
}

/*
; PUTSTR.asm puts a string directly to video display memory. Called from C by:
;    driver_put_string(row, col, attr, string) where
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
void put_string(int row, int col, int attr, char *msg)
{
	int so = 0;

	if (row != -1)
	{
		g_text_row = row;
	}
	if (col != -1)
	{
		g_text_col = col;
	}

	if (attr & INVERSE || attr & BRIGHT)
	{
		wstandout(g_current_window);
		so = 1;
	}
	wmove(g_current_window, g_text_row + g_text_rbase, g_text_col + g_text_cbase);
	while (1)
	{
		if (*msg == '\0')
		{
			break;
		}
		if (*msg == '\n')
		{
			g_text_col = 0;
			g_text_row++;
			wmove(g_current_window, g_text_row + g_text_rbase, g_text_col + g_text_cbase);
		}
		else
		{
			char *ptr;
			ptr = strchr(msg, '\n');
			if (ptr == NULL)
			{
				waddstr(g_current_window, msg);
				break;
			}
			else
			{
				waddch(g_current_window, *msg);
			}
		}
		msg++;
	}
	if (so)
	{
		wstandend(g_current_window);
	}

	wrefresh(g_current_window);
	fflush(stdout);
	getyx(g_current_window, g_text_row, g_text_col);
	g_text_row -= g_text_rbase;
	g_text_col -= g_text_cbase;
}

/*
; set_attribute(row, col, attr, count) where
;         row, col = row and column to start printing.
;         attr = color attribute.
;         count = number of characters to set
;         This routine works only in real color text mode.
*/
void set_attribute(int row, int col, int attr, int count)
{
	move_cursor(row, col);
}

/*
; **************** Function home()  ********************************

;       Home the cursor (called before printfs)
*/
void home()
{
	wmove(g_current_window, 0, 0);
	g_text_row = 0;
	g_text_col = 0;
}


/*
; ************* Function scrollup(toprow, botrow) ******************

;       Scroll the screen up (from toprow to botrow)
*/
void scrollup(int top, int bot)
{
	wmove(g_current_window, top, 0);
	wdeleteln(g_current_window);
	wmove(g_current_window, bot, 0);
	winsertln(g_current_window);
	wrefresh(g_current_window);
}

/*
; *************** Function spindac(direction, rstep) ********************

;       Rotate the MCGA/VGA DAC in the (plus or minus) "direction"
;       in "rstep" increments - or, if "direction" is 0, just replace it.
*/
void spindac(int dir, int inc)
{
	int i, top;
	unsigned char tmp[3];
	unsigned char *dacbot;
	int len;
	if (g_colors < 16)
	{
		return;
	}
	if (g_is_true_color && g_true_mode)
	{
		return;
	}
	if (dir != 0 && g_rotate_lo < g_colors && g_rotate_lo < g_rotate_hi)
	{
		top = g_rotate_hi > g_colors ? g_colors - 1 : g_rotate_hi;
		dacbot = (unsigned char *) g_dac_box + 3 * g_rotate_lo;
		len = (top - g_rotate_lo) * 3 * sizeof(unsigned char);
		if (dir > 0)
		{
			for (i = 0; i < inc; i++)
			{
				bcopy(dacbot, tmp, 3 * sizeof(unsigned char));
				bcopy(dacbot + 3 * sizeof(unsigned char), dacbot, len);
				bcopy(tmp, dacbot + len, 3 * sizeof(unsigned char));
			}
		}
		else
		{
			for (i = 0; i < inc; i++)
			{
				bcopy(dacbot + len, tmp, 3 * sizeof(unsigned char));
				bcopy(dacbot, dacbot + 3 * sizeof(unsigned char), len);
				bcopy(tmp, dacbot, 3 * sizeof(unsigned char));
			}
		}
	}
	writevideopalette();
	driver_delay(g_colors - g_dac_count - 1);
}

/*
; ---- Help (Video) Support
; ********* Functions setfortext() and setforgraphics() ************

;       setfortext() resets the video for text mode and saves graphics data
;       setforgraphics() restores the graphics mode and data
;       setclear() clears the screen after setfortext()
*/
void setfortext()
{
}

void setclear()
{
	wclear(g_current_window);
	wrefresh(g_current_window);
}

void setforgraphics()
{
	startvideo();
	spindac(0, 1);
}

unsigned char *fontTab = NULL;

/*
; ************** Function findfont(n) ******************************

;       findfont(0) returns pointer to 8x8 font table if it can
;                   find it, NULL otherwise;
;                   nonzero parameter reserved for future use
*/
BYTE *findfont(int fontparm)
{
	if (fontTab == NULL)
	{
		fontTab = x_get_font();
	}
	return (BYTE *) fontTab;
}

/*
; ******************** Zoombox functions **************************
*/

/*
 * The IBM method is that g_box_x[],g_box_y[] is a set of locations, and boxvalues
 * is the values in these locations.
 * Instead of using this box save/restore technique, we'll put the corners
 * in g_box_x[0],g_box_y[0],1,2,3 and then use xor.
 */
void display_box()
{
	if (g_box_count)
	{
		setlinemode(1);
		drawline(g_box_x[0], g_box_y[0], g_box_x[1], g_box_y[1]);
		drawline(g_box_x[1], g_box_y[1], g_box_x[2], g_box_y[2]);
		drawline(g_box_x[2], g_box_y[2], g_box_x[3], g_box_y[3]);
		drawline(g_box_x[3], g_box_y[3], g_box_x[0], g_box_y[0]);
		setlinemode(0);
		xsync();
	}
}

void clear_box()
{
	display_box();
}

int CheckForTPlus()
{
	return 0;
}

/*
; Passing this routine 0 turns off shadow, nonzero turns it on.
*/
int ShadowVideo(int on)
{
	return 0;
}

int SetupShadowVideo()
{
	return 0;
}

/*
; **************** internal Read/Write-a-line routines *********************
;
;       These routines are called by out_line(), put_line() and get_line().
*/
static void normal_line_write(int y, int x, int lastx, BYTE *pixels)
{
	int i, width;
	width = lastx - x + 1;
	for (i = 0; i < width; i++)
	{
		g_dot_write(x + i, y, pixels[i]);
	}
}

static void normal_line_read(int y, int x, int lastx, BYTE *pixels)
{
	int i, width;
	width = lastx - x + 1;
	for (i = 0; i < width; i++)
	{
		pixels[i] = g_dot_read(x + i, y);
	}
}

/*
; *************** Function find_special_colors ********************

;       Find the darkest and brightest colors in palette, and a medium
;       color which is reasonably bright and reasonably grey.
*/
void find_special_colors()
{
	int maxb = 0;
	int minb = 9999;
	int med = 0;
	int maxgun, mingun;
	int brt;
	int i;

	g_color_dark = 0;
	g_color_medium = 7;
	g_color_bright = 15;

	if (g_colors == 2)
	{
		g_color_medium = 1;
		g_color_bright = 1;
		return;
	}

	if (!(g_got_real_dac || g_fake_lut))
	{
		return;
	}

	for (i = 0; i < g_colors; i++)
	{
		brt = (int) g_dac_box[i][0] + (int) g_dac_box[i][1] + (int) g_dac_box[i][2];
		if (brt > maxb)
		{
			maxb = brt;
			g_color_bright = i;
		}
		if (brt < minb)
		{
			minb = brt;
			g_color_dark = i;
		}
		if (brt < 150 && brt > 80)
		{
			maxgun = mingun = (int) g_dac_box[i][0];
			if ((int) g_dac_box[i][1] > (int) g_dac_box[i][0])
			{
				maxgun = (int) g_dac_box[i][1];
			}
			else
			{
				mingun = (int) g_dac_box[i][1];
			}
			if ((int) g_dac_box[i][2] > maxgun)
			{
				maxgun = (int) g_dac_box[i][2];
			}
			if ((int) g_dac_box[i][2] < mingun)
			{
				mingun = (int) g_dac_box[i][2];
			}
			if (brt - (maxgun - mingun) / 2 > med)
			{
				g_color_medium = i;
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
char get_a_char()
{
	return (char) get_a_key();
}

void put_a_char(int ch)
{
}

/*
; ***Function get_line(int row,int startcol,int stopcol, unsigned char *pixels)

;       This routine is a 'line' analog of 'getcolor()', and gets a segment
;       of a line from the screen and stores it in pixels[] at one byte per
;       pixel
;       Called by the GIF decoder
*/

void get_line(int row, int startcol, int stopcol, BYTE *pixels)
{
	if (startcol + g_sx_offset >= g_screen_width
		|| row + g_sy_offset >= g_screen_height)
	{
		return;
	}
	g_line_read(row + g_sy_offset, startcol + g_sx_offset,
		stopcol + g_sx_offset, pixels);
}

/*
; ***Function put_line(int row,int startcol,int stopcol, unsigned char *pixels)

;       This routine is a 'line' analog of 'putcolor()', and puts a segment
;       of a line from the screen and stores it in pixels[] at one byte per
;       pixel
;       Called by the GIF decoder
*/
void put_line(int row, int startcol, int stopcol, BYTE *pixels)
{
	if (startcol + g_sx_offset >= g_screen_width
		|| row + g_sy_offset > g_screen_height)
	{
		return;
	}
	g_line_write(row + g_sy_offset, startcol + g_sx_offset,
		stopcol + g_sx_offset, pixels);
}

/*
; ***************Function out_line(pixels,linelen) *********************

;       This routine is a 'line' analog of 'putcolor()', and sends an
;       entire line of pixels to the screen (0 <= xdot < xdots) at a clip
;       Called by the GIF decoder
*/
int out_line(BYTE *pixels, int linelen)
{
	if (g_row_count + g_sy_offset >= g_screen_height)
	{
		return 0;
	}
	g_line_write(g_row_count + g_sy_offset, g_sx_offset,
		linelen + g_sx_offset - 1, pixels);
	g_row_count++;
	return 0;
}

/*
; move routine for savegraphics/restoregraphics
*/
void movewords(int len, BYTE *fromptr, BYTE *toptr)
{
	bcopy(fromptr, toptr, len);
}

void swapnormread()
{
}

void swapnormwrite()
{
}

/*
 * Implement stack and unstack window functions by using multiple curses
 * windows.
 */
void savecurses(WINDOW **ptr)
{
	ptr[0] = g_current_window;
	g_current_window = newwin(0, 0, 0, 0);
	touchwin(g_current_window);
	wrefresh(g_current_window);
}

void restorecurses(WINDOW **ptr)
{
	delwin(g_current_window);
	g_current_window = ptr[0];
	touchwin(g_current_window);
	wrefresh(g_current_window);
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

static BYTE *savescreen[MAXSCREENS];
static int saverc[MAXSCREENS+1];

void stackscreen()
{
#ifdef USE_XFRACT_STACK_FUNCTIONS
	int i;
	BYTE *ptr;
	saverc[screenctr+1] = g_text_row*80 + g_text_col;
	if (++screenctr) /* already have some stacked */
	{
		static char *msg = "stackscreen overflow";
		if ((i = screenctr - 1) >= MAXSCREENS)
		{ /* bug, missing unstack? */
			stop_message(1, msg);
			exit(1);
		}

		savescreen[i] = (BYTE *) malloc(sizeof(int *));
		ptr = savescreen[i];
		if (ptr)
		{
			savecurses((WINDOW **) ptr);
		}
		else
		{
			stop_message(1, msg);
			exit(1);
		}
		setclear();
	}
	else
	{
		setfortext();
	}
#endif
}

void unstackscreen()
{
#ifdef USE_XFRACT_STACK_FUNCTIONS
	BYTE *ptr;
	g_text_row = saverc[screenctr] / 80;
	g_text_col = saverc[screenctr] % 80;
	if (--screenctr >= 0) /* unstack */
	{
		ptr = savescreen[screenctr];
		restorecurses((WINDOW **)ptr);
		free(ptr);
	}
	else
	{
		setforgraphics();
	}
	move_cursor(-1, -1);
#endif
}

void discardscreen()
{
	if (--screenctr >= 0) /* unstack */
	{
		if (savescreen[screenctr])
		{
#ifdef USE_XFRACT_STACK_FUNCTIONS
			free(savescreen[screenctr]);
#endif
		}
	}
}
