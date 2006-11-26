#include <string.h>
#include "port.h"
#include "cmplx.h"
#include "fractint.h"
#include "drivers.h"
#include "externs.h"
#include "prototyp.h"

extern int (*dotread)(int, int);			/* read-a-dot routine */
extern void (*dotwrite)(int, int, int);		/* write-a-dot routine */

int andcolor;
int overflow = 0;
int inside_help = 0;
SEGTYPE extraseg;
unsigned char dacbox[256][3];
int boxx[2304], boxy[1024];
int boxvalues[512];
char tstack[4096];
BYTE block[256];
int chkd_vvs = 0;
int color_dark = 0;		/* darkest color in palette */
int color_bright = 0;		/* brightest color in palette */
int color_medium = 0;		/* nearest to medbright grey in palette
				   Zoom-Box values (2K x 2K screens max) */
int cpu, fpu;                        /* cpu, fpu flags */
int daclearn = 0;
int dacnorm = 0;
int daccount = 0;
int diskflag = 0;
int disktarga = 0;
int DivideOverflow = 0;
int (*dotread) (int, int);	/* read-a-dot routine */
void (*dotwrite) (int, int, int); /* write-a-dot routine */
int fake_lut = 0;
int finishrow = 0;
int fm_attack = 0;
int fm_decay = 0;
int fm_release = 0;
int fm_sustain = 0;
int fm_vol = 0;
int fm_wavetype = 0;
int goodmode = 0;
int gotrealdac = 0;
int hi_atten = 0;
int istruecolor = 0;
void (*lineread) ();		/* read-a-line routine */
void (*linewrite) ();		/* write-a-line routine */
long linitx = 0;
long linity = 0;
int lookatmouse = 0;
int mode7text = 0;


long stackavail()
{
	return 8192;
}

/*
;
;       32-bit integer divide routine with an 'n'-bit shift.
;       Overflow condition returns 0x7fffh with overflow = 1;
;
;       z = divide(x,y,n);       z = x / y;
*/
long divide(long x, long y, int n)
{
    return (long) (((float) x) / ((float) y)*(float) (1 << n));
}

/*
;
;       32-bit integer multiply routine with an 'n'-bit shift.
;       Overflow condition returns 0x7fffh with overflow = 1;
;
;       long x, y, z, multiply();
;       int n;
;
;       z = multiply(x,y,n)
;
*/

/*
 * 32 bit integer multiply with n bit shift.
 * Note that we fake integer multiplication with floating point
 * multiplication.
 */
long multiply(long x, long y, int n)
{
    register long l;
    l = (long) (((float) x) * ((float) y)/(float) (1 << n));
    if (l == 0x7fffffff)
	{
		overflow = 1;
    }
    return l;
}

/*
; ****************** Function getakey() *****************************
; **************** Function keypressed() ****************************

;       'getakey()' gets a key from either a "normal" or an enhanced
;       keyboard.   Returns either the vanilla ASCII code for regular
;       keys, or 1000+(the scan code) for special keys (like F1, etc)
;       Use of this routine permits the Control-Up/Down arrow keys on
;       enhanced keyboards.
;
;       The concept for this routine was "borrowed" from the MSKermit
;       SCANCHEK utility
;
;       'keypressed()' returns a zero if no keypress is outstanding,
;       and the value that 'getakey()' will return if one is.  Note
;       that you must still call 'getakey()' to flush the character.
;       As a sidebar function, calls 'help()' if appropriate, or
;       'tab_display()' if appropriate.
;       Think of 'keypressed()' as a super-'kbhit()'.
*/
int keybuffer = 0;

/*
 * This is the low level key handling routine.
 * If block is set, we want to block before returning, since we are waiting
 * for a key press.
 * We also have to handle the slide file, etc.
 */

int getkeyint(int block)
{
    int ch;
    int curkey;
    if (keybuffer)
	{
		ch = keybuffer;
		keybuffer = 0;
		return ch;
    }
    curkey = driver_get_key(0);
    if (slides==1 && curkey == ESC)
	{
		stopslideshow();
		return 0;
    }

    if (curkey==0 && slides==1)
	{
		curkey = slideshw();
    }

    if (curkey==0 && block)
	{
		curkey = driver_get_key(1);
		if (slides==1 && curkey == ESC)
		{
			stopslideshow();
			return 0;
		}
    }

    if (curkey && slides==2)
	{
		recordshw(curkey);
    }

    return curkey;
}

/*
 * This routine returns the current key, or 0.
 */
int getkeynowait(void)
{
    return getkeyint(0);
}

/*
 * This routine returns a keypress
 */
int getakey(void)
{
    int ch;

    do
	{
		ch = getkeyint(1);
    }
	while (ch==0);
    return ch;
}

/*
 * This routine returns a key, ignoring F1
 */
int getakeynohelp(void)
{
	int ch;
	while (1)
	{
		ch = getakey();
		if (ch != F1)
			break;
	}
	return ch;
}

int keypressed(void)
{
    int ch;
    ch = getkeynowait();
    if (!ch)
		return 0;
    keybuffer = ch;
    if (ch==F1 && helpmode)
	{
		keybuffer = 0;
		inside_help = 1;
		help(0);
		inside_help = 0;
		return 0;
    }
	else if (ch==TAB && tabmode)
	{
		keybuffer = 0;
		tab_display();
		return 0;
    }
    return ch;
}

/* Wait for a key.
 * This should be used instead of:
 * while (!keypressed()) {}
 * If timeout=1, waitkeypressed will time out after .5 sec.
 */
int waitkeypressed(int timeout)
{
    while (!keybuffer)
	{
		keybuffer = getkeyint(1);
		if (timeout)
			break;
    }
    return keypressed();
}

/*
; **************** Function getcolor(xdot, ydot) *******************

;       Return the color on the screen at the (xdot,ydot) point
*/
int getcolor(int xdot, int ydot)
{
	int x1, y1;
	x1 = xdot + sxoffs;
	y1 = ydot + syoffs;
	if (x1 < 0 || y1 < 0 || x1 >= sxdots || y1 >= sydots)
		return 0;
	return dotread(x1, y1);
}

/*
; ************** Function putcolor_a(xdot, ydot, color) *******************

;       write the color on the screen at the (xdot,ydot) point
*/
void putcolor_a(int xdot, int ydot, int color)
{
	dotwrite(xdot + sxoffs, ydot + syoffs, color & andcolor);
}

void far_strcpy(char *to, char *from)
{
	strcpy(to, from);
}
int far_strlen(char *str)
{
	return (int) strlen(str);
}

int far_strcmp(char *a, char *b)
{
    return strcmp(a,b);
}

int far_stricmp(char *a, char *b)
{
   return stricmp(a,b);
}

int far_strnicmp(char *a, char *b, int n)
{
    return strnicmp(a,b,n);
}

void far_strcat(char *a, char *b)
{
    strcat(a,b);
}

void far_memset(VOIDFARPTR a, int c, unsigned int len)
{
    memset(a,c,len);
}

void far_memcpy(VOIDFARPTR a, VOIDFARPTR b, int len)
{
    memcpy(a,b,len);
}

int far_memcmp(VOIDFARPTR a, VOIDFARPTR b, int len)
{
    return memcmp(a,b,len);
}

void far_memicmp(VOIDFARPTR a, VOIDFARPTR b, int len)
{
    memicmp(a,b,len);
}

VOIDPTR farmemalloc(long len)
{
    return (VOIDPTR) malloc((unsigned) len);
}

void farmemfree(VOIDPTR addr)
{
    free((char *) addr);
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

int fr_findfirst(char *path)       /* Find 1st file (or subdir) meeting path/filespec */
{
	return -1;
}

int fr_findnext()
{
	return -1;
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
	if (startcol + sxoffs >= sxdots || row + syoffs >= sydots)
		return;
	lineread(row + syoffs, startcol + sxoffs, stopcol + sxoffs, pixels);
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
	if (startcol + sxoffs >= sxdots || row + syoffs > sydots)
		return;
	linewrite(row + syoffs, startcol + sxoffs, stopcol + sxoffs, pixels);
}

int get_sound_params(void)
{
	return(0);
}
