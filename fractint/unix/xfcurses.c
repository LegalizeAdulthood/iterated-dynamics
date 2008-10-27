#ifndef NCURSES

#ifndef __XFCURSES_LOADED
#define __XFCURSES_LOADED	1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#ifdef _AIX
#include <sys/select.h>
#endif

#ifdef __hpux
#include <sys/file.h>
#endif

#include <fcntl.h>

#include "xfcurses.h"
#include "helpdefs.h"
#include "port.h"
#include "prototyp.h"

#define BRIGHT_INVERSE 0xC000
#define ATTRSIZE sizeof(short)

extern int ctrl_window;
extern int Xwinwidth ,Xwinheight;
extern int helpmode;
extern XImage *Ximage;
extern int screenctr;
extern int unixDisk;
extern int resize_flag;
extern void error_display();

static GC Xwcgc = None;
static XFontStruct * font, * fontbold;
static unsigned long black, white;
static XSetWindowAttributes Xwatt;

unsigned long pixel[48];
static char * xc[16] = {
  "#000000",    /* black */
  "#0000A8",    /* dark blue */
  "#00A800",    /* dark green */
  "#00A8A8",    /* dark cyan */
  "#A80000",    /* dark red */
  "#A800A8",    /* dark magenta */
  "#A85400",    /* brown */
  "#CCCCCC",    /* grey80 */
  "#333333",    /* grey20 */
  "#5454FC",    /* light blue */
  "#54FC54",    /* light green */
  "#54FCFC",    /* light cyan */
  "#FC5454",    /* light red */
  "#FC54FC",    /* light magenta */
  "#FCFC54",    /* light yellow */
  "#FFFFFF"     /* white */
};

int charx, chary;
static int charwidth, charheight, ascent, descent;
static int Xwcwidth, Xwcheight;
static Screen *Xwcsc;

int COLS = 80;
int LINES = 25 ;
int textmargin = 40;
WINDOW * curwin;

Window Xwc = None, Xwp = None;
char * Xmessage = NULL;
char * Xfontname = NULL;
char * Xfontnamebold = NULL;

void Open_XDisplay()
{
    if (Xdp != NULL) return;
  
    Xdp = XOpenDisplay(Xdisplay);

    if (Xdp == NULL) {
      error_display();
      exit(-1);
    }
    Xdscreen = XDefaultScreen(Xdp);
    Xdepth = DefaultDepth(Xdp, Xdscreen);
}


void cbreak(void)
{
    curwin = newwin(LINES, COLS, 0, 0);
}

void nocbreak(void)
{
}

void echo(void)
{
}

void noecho(void)
{
}

void clear(void)
{
  wclear(curwin);
}

int standout(void)
{
  if (screenctr)
     XSetFont(Xdp, Xwcgc, fontbold->fid);
  return 1;
}

int standend(void)
{
  if (screenctr)
     XSetFont(Xdp, Xwcgc, font->fid);
  return 1;
}

void endwin(void)
{
}

void delwin(WINDOW *win)
{
  if (curwin) free(curwin->_text);
  free(curwin);
  XClearWindow(Xdp, Xwc);
  standend();
}

void fill_rectangle(int x, int y, int n)
{
  int u, v;
  if (screenctr == 0)
     return;
  u = (y==0)?descent:0;
  v = (y==LINES-1)? descent+4:0;
  XFillRectangle(Xdp, Xwc, Xwcgc, 
	        charx + x * charwidth,
                chary + y * charheight - ascent - u - 1,
		charwidth * n, charheight + u + v);
}

void setcolor_bg(WINDOW *win, int j)
{
  int attr = (j<0)?win->_cur_attr : win->_attr[j];
  XSetForeground(Xdp, Xwcgc, pixel[(attr>>4) & 0xF]);
}

void setcolor_fg(WINDOW *win, int j)
{
  int attr = (j<0)?win->_cur_attr : win->_attr[j];
  XSetForeground(Xdp, Xwcgc, pixel[attr & 0xF]);
}

void waddch(WINDOW *win, const chtype ch)
{
  char str[4];
  int j;

  if (win->_cur_x<0 || win->_cur_x>=win->_num_x) return;
  if (win->_cur_y<0 || win->_cur_y>=win->_num_y) return;
  *str = (char) ch;
  j = win->_cur_y*win->_num_x + win->_cur_x;
  if (ch) 
     win->_text[j] = (char) ch;

  win->_attr[j] = (short) win->_cur_attr;
  
  if (win->_cur_attr & BRIGHT_INVERSE)
    standout();
  else
    standend();

  setcolor_bg(win, -1);
  fill_rectangle(win->_cur_x, win->_cur_y, 1);

  *str = win->_text[j];
  if (*str) {
     setcolor_fg(win, j);
     XDrawString(Xdp, Xwc, Xwcgc, 
                 charx + win->_cur_x * charwidth, 
                 chary + win->_cur_y * charheight, 
                 str, 1);
  }

  win->_cur_x += 1;
  if (win->_cur_x >= win->_num_x) {
     win->_cur_x = 0;
     win->_cur_y += 1;
  }
}

void waddstr(WINDOW *win, const char *str)
{
  int i, j, n = strlen(str);

  if (win->_cur_y<0 || win->_cur_y>=win->_num_y) return;
  for (i=0; i<n; i++) {
     j = win->_cur_y*win->_num_x + win->_cur_x + i;
     if (win->_cur_x+i<0 || win->_cur_x+i>=win->_num_x) continue;
     win->_text[j] = str[i];
     win->_attr[j] = (short) win->_cur_attr;
  }
  
  setcolor_bg(win, -1);
  fill_rectangle(win->_cur_x, win->_cur_y, n);

  if (win->_cur_attr & BRIGHT_INVERSE)
     standout();
  else
     standend();

  setcolor_fg(win, -1);
  XDrawString(Xdp, Xwc, Xwcgc, 
              charx + win->_cur_x * charwidth, 
              chary + win->_cur_y * charheight, 
	      str, n);

  win->_cur_x += n;
  if (win->_cur_x >= win->_num_x) {
    win->_cur_y += win->_cur_x / win->_num_x;
    win->_cur_x = win->_cur_x % win->_num_x;
  }
}  

void draw_caret(WINDOW *win, int y, int x)
{
  int j;
  char str[4];

  if (screenctr == 0) 
     goto caret_end;

  if (win->_car_x>=0 && win->_car_x<COLS && 
      win->_car_y>=0 && win->_car_y<LINES) {
     j = win->_car_y * win->_num_x + win->_car_x;
     *str = win->_text[j];
     if (win->_attr[j] & BRIGHT_INVERSE)
        standout();
     else
        standend();

     setcolor_bg(win, j);
     if (win->_car_x>=0 && win->_car_x<COLS &&
         win->_car_y>=0 && win->_car_y<LINES)
        XFillRectangle(Xdp, Xwc, Xwcgc, 
	            charx + win->_car_x * charwidth + 2,
                    chary + win->_car_y * charheight + 2 ,
		    charwidth-2, 2);
     if (*str) {
        setcolor_fg(win, j);
        XDrawString(Xdp, Xwc, Xwcgc, 
                    charx + win->_car_x * charwidth, 
                    chary + win->_car_y * charheight, 
                    str, 1);
     }
  }

  XSetForeground(Xdp, Xwcgc, pixel[4]);
  if (x>=0 && x<COLS && y>=0 && y<LINES)
     XFillRectangle(Xdp, Xwc, Xwcgc, 
	            charx + x * charwidth + 2,
                    chary + y * charheight + 2 ,
		    charwidth-2, 2);
  XFlush(Xdp);
caret_end:
  win->_car_y = y;
  win->_car_x = x;
}

void mvcur(int oldrow, int oldcol, int newrow, int newcol)
{
  curwin->_cur_y = newrow;
  curwin->_cur_x = newcol;
  draw_caret(curwin, newrow, newcol);
}

void wclear(WINDOW *win)
{
  int n;
  if (!win) return;
  n = (win->_num_x)*(win->_num_y);
  win->_cur_attr = 0;
  memset(win->_text, 0, n);
  memset(win->_attr, 0, n*sizeof(short));
  XClearWindow(Xdp, Xwc);
}

void wdeleteln(WINDOW *win)
{
  int j, k;
  if (win->_cur_y<0 || win->_cur_y>=win->_num_y) return;
  for (j=win->_cur_y; j <win->_num_y-1 ; j++) {
     k = j*win->_num_x;
     memcpy(&win->_text[k], &win->_text[k+win->_num_x], win->_num_x);
     memcpy(&win->_attr[k], &win->_attr[k+win->_num_x], win->_num_x*ATTRSIZE);
  }
  k = (win->_num_y-1)*win->_num_x;
  memset(&win->_text[k], 0, win->_num_x);
  memset(&win->_attr[k], 0, win->_num_x*ATTRSIZE);
}

void winsertln(WINDOW *win)
{
  int j, k;   
  if (win->_cur_y<0 || win->_cur_y>=win->_num_y) return;
  for (j = win->_num_y - 1; j> win->_cur_y; j--) {
     k = j*win->_num_x;
     memcpy(&win->_text[k], &win->_text[k-win->_num_x], win->_num_x);
     memcpy(&win->_attr[k], &win->_attr[k-win->_num_x], win->_num_x*ATTRSIZE);
  }
  k = win->_cur_y*win->_num_x;
  memset(&win->_text[k], 0, win->_num_x);
  memset(&win->_attr[k], 0, win->_num_x*ATTRSIZE);
  xrefresh(win, win->_cur_y, win->_num_y);
}

void wmove(WINDOW *win, int y, int x)
{
  draw_caret(win, y, x);
  win->_cur_y = y;
  win->_cur_x = x;
}

void wrefresh(WINDOW *win)
{
}

void refresh(int line1, int line2)
{
  xrefresh(curwin, line1, line2);
}


void xrefresh(WINDOW *win, int line1, int line2)
{
  char str[4];
  int x, y, j, topline;

  if (screenctr == 0 && !ctrl_window) {
    if (resize_flag & 2) {
       resize_flag &= ~2;
       ungetakey('d');
    } else
       XPutImage(Xdp, Xw, Xgc, Ximage, 0, 0, 0, 0, Xwinwidth, Xwinheight);
    return;
  }
 
  if (line1 < 0) 
    line1 = 0;
  if (line1 >= win->_num_y)
    line1 = win->_num_y;
  if (line2 >= win->_num_y)
    line2 = win->_num_y;
  if (line1 > line2) return;

  str[1] = '\0';
  for (y=line1; y<line2; y++) {
    for (x=0; x<win->_num_x; x++) {
       j = y*win->_num_x + x;
       setcolor_bg(win, j);
       fill_rectangle(x, y, 1);
       setcolor_fg(win, j);
       if (win->_attr[j] & BRIGHT_INVERSE) 
          standout();
       else
          standend();
       if (win->_text[j])
         str[0] = win->_text[j];
       else
	 str[0] = ' ';	 
       XDrawString(Xdp, Xwc, Xwcgc, 
                  charx + x * charwidth, 
                  chary + y * charheight, 
                  str, 1);
    }
  }
  XFlush(Xdp);
}

void touchwin(WINDOW *win)
{
}

void wtouchln(WINDOW *win, int y, int n, int changed)
{
}

void wstandout(WINDOW *win)
{
  standout();
}

void wstandend(WINDOW *win)
{
  standend();
}

WINDOW *newwin(int nlines, int ncols, int begin_y, int begin_x)
{
  WINDOW *win;
  int n;
  win = (WINDOW *) malloc(sizeof(WINDOW));
  if (!win) return NULL;
  win->_num_y = nlines;
  win->_num_x = ncols;
  win->_car_y = begin_y;
  win->_car_x = begin_x;
  win->_cur_x = 0;
  win->_cur_y = 0;
  win->_cur_attr = 0;
  n = 2*(ncols+1)*(nlines+1);
  win->_attr = (short *)malloc(n*ATTRSIZE); 
  win->_text = (char *)malloc(n);
  if (!win->_text || !win->_attr) {
     if (win->_text) free(win->_text);
     if (win->_attr) free(win->_attr);
     free(win);
     return NULL;
  }
  memset(win->_text, 0, n);
  memset(win->_attr, 0, n*ATTRSIZE);
  return win;
}

WINDOW *initscr(void)
{
  XGCValues Xgcvals;
  XColor xcolor, junk;
  int i, Xwinx = 0, Xwiny = 0;

  Open_XDisplay();
  Xwcsc = ScreenOfDisplay(Xdp, Xdscreen);

  if (Xfontname == NULL)
     Xfontname = "9x15";
  if (Xfontnamebold == NULL)
     Xfontnamebold = "9x15bold";

  Xwatt.background_pixel = BlackPixelOfScreen(Xwcsc);
  Xwatt.bit_gravity = StaticGravity;

  if (DoesBackingStore(Xwcsc)) {
    Xwatt.backing_store = Always;
  } else {
    Xwatt.backing_store = NotUseful;
  }

  Xroot = DefaultRootWindow(Xdp);

  font = XLoadQueryFont(Xdp, Xfontname);
  if (font == (XFontStruct *)NULL) {
     fprintf(stderr, "xfractint: can't open font `%s'\n", Xfontname);
     exit(-1);
  }
  fontbold = XLoadQueryFont(Xdp, Xfontnamebold);
  if (fontbold == (XFontStruct *)NULL) {
     fprintf(stderr, "xfractint: can't open font `%s', using `%s'\n", Xfontnamebold, Xfontname);
     /* no need to exit since at this point we know Xfontname is good */
     fontbold = XLoadQueryFont(Xdp, Xfontname);
  }

  ascent = font->max_bounds.ascent;
  descent = font->max_bounds.descent;
  charwidth = XTextWidth(font, "m", 1);
  charheight = ascent + descent + 2;
  Xwcwidth = (COLS*charwidth+3)&-4;
  Xwcheight = (LINES*charheight+2*(charheight/4)+3)&-4;

  if (!ctrl_window || unixDisk) {
    i = 2*textmargin;
    Xwcwidth += i;
    Xwcheight += i;
    if (Xgeometry) {
      XParseGeometry(Xgeometry, &Xwinx, &Xwiny, (unsigned int *) &Xwinwidth,
		     (unsigned int *) &Xwinheight);
    }
    Xwinwidth &= -4;
    Xwinheight &= -4;
    if (Xwinwidth > Xwcwidth) 
       Xwcwidth = Xwinwidth;
    if (Xwinheight > Xwcheight) 
       Xwcheight = Xwinheight;
    if (Xwcheight<(i=(3*Xwcwidth)/4)) Xwcheight = (i+3)&-4;
    if (Xwcwidth<(i=(4*Xwcheight)/3)) Xwcwidth = (i+3)&-4;
  }
  charx = (Xwcwidth - COLS*charwidth)/2;
  chary = (Xwcheight - (LINES*charheight+2*(charheight/4)))/2 
          + ascent + (charheight/4);
  if (Xwc == None)
     Xwc = XCreateWindow(Xdp, Xroot, Xwinx, Xwiny, Xwcwidth,
 		      Xwcheight, 0, Xdepth, InputOutput, CopyFromParent,
		      CWBackPixel | CWBitGravity | CWBackingStore, &Xwatt);
  XSelectInput(Xdp, Xwc, ExposureMask|StructureNotifyMask|
                         KeyPressMask|KeyReleaseMask|
		         ButtonPressMask|ButtonReleaseMask|PointerMotionMask);
  wm_protocols = XInternAtom(Xdp, "WM_PROTOCOLS", False);
  wm_delete_window = XInternAtom(Xdp, "WM_DELETE_WINDOW", False);   
  XSetWMProtocols(Xdp, Xwc, &wm_delete_window, 1);

  XStoreName(Xdp, Xwc, (ctrl_window)?"Xfractint controls":"Xfractint");

  white = WhitePixelOfScreen(Xwcsc);
  black = BlackPixelOfScreen(Xwcsc);

  Xcmap = DefaultColormapOfScreen(Xwcsc);

  for (i=0; i<16; i++)
  if (XAllocNamedColor(Xdp, Xcmap, xc[i], &xcolor, &junk))
     pixel[i] = xcolor.pixel;
  else
     pixel[i] = (i<=6)? black:white;

  Xgcvals.font = font->fid;
  Xgcvals.foreground = white;
  Xgcvals.background = black;
  Xwcgc = XCreateGC(Xdp, Xwc, GCForeground | GCBackground | GCFont, &Xgcvals);
  clear();
  if (ctrl_window)
     XMapRaised(Xdp, Xwc);
  standend();
  return NULL;
}

void set_margins(int width, int height)
{
  int i, j;
  i = width - ((COLS*charwidth+3)&-4);
  j = height - ((LINES*charheight+2*(charheight/4)+3)&-4);
  if (i<0 || j<0) 
     textmargin = 0;
  else
  if (i<j)
     textmargin = i/2;
  else
     textmargin = j/2;
  charx = (width - COLS*charwidth)/2;
  if (charx < 0) 
     charx = 0;
  j = ascent + charheight/4;
  chary = (height - (LINES*charheight+2*(charheight/4)))/2 + j;
  if (chary < j)
     chary = j; 
  if (screenctr) {
    j = chary-ascent; if (j<0) j = 0;
    if (j>0)
       XClearArea(Xdp, Xw, 0, 0, width, j, True);
    j = chary+LINES*charheight-ascent+descent+4;
    if (j<height)
       XClearArea(Xdp, Xw, 0, j, width, height-j, True);
    if (charx>0) {
       XClearArea(Xdp, Xw, 0, 0, charx, height, True);
       XClearArea(Xdp, Xw, width-charx, 0, charx, height, True);
    }
  }
}

void xpopup(char *str) {
  Window child;
  XSizeHints size_hints;
  char *ptr1, *ptr2;
  int x, y, j, n;
  unsigned int junk;

  if (!Xwc) return;

  /* if str==NULL refresh message */
  if (!str && Xwp && Xmessage) {
  display_text:
    ptr1 = str = strdup(Xmessage);
    XSetForeground(Xdp, Xwcgc, pixel[15]);
    n = 0;
  iter:
    if ((ptr2=strchr(ptr1, '\n')))
      *ptr2 = '\0';
    XDrawImageString(Xdp, Xwp, Xwcgc, 
	             charwidth/2, ascent + (charheight/4)+2+n*charheight,
		     ptr1, strlen(ptr1));
    XFlush(Xdp);
    usleep(100000);
    if (ptr2) {
      ptr1 = ptr2+1;
      n++;
      goto iter;
    }
    free(str);
    return;
  }

  /* otherwise create popup */
  if (Xwp)
    XDestroyWindow(Xdp, Xwp);

  if (Xmessage)
    free(Xmessage);

  Xmessage = strdup(str);

  j = 1;
  n = 1;
  ptr1 = str;
  while ((ptr2=strchr(ptr1, '\n'))) {
    ++n;
    if (ptr2-ptr1>j) j = (int)(ptr2-ptr1);
    ptr1 = ptr2+1;
  }
  if (ptr1 && strlen(ptr1)>j) j = strlen(ptr1);

  size_hints.flags = USPosition | USSize | PBaseSize | PResizeInc;
  size_hints.base_width = charwidth*(j+1);
  size_hints.base_height = charheight*n + 2*(charheight/4);
  size_hints.width_inc = 4;
  size_hints.height_inc = 4;
  XTranslateCoordinates(Xdp, Xw, Xroot, 0, 0, &x, &y, &child);
  size_hints.x = x+20;
  size_hints.y = y+20;
  Xwp = XCreateWindow(Xdp, Xroot, size_hints.x, size_hints.y,
                      size_hints.base_width, size_hints.base_height,
 		      0, Xdepth, InputOutput, CopyFromParent,
		      CWBackPixel | CWBitGravity | CWBackingStore, &Xwatt);
  XSelectInput(Xdp, Xwp, ExposureMask);
  XSetWMProtocols(Xdp, Xwp, &wm_delete_window, 1);
  XStoreName(Xdp, Xwp, "Xfractint message");

  XSetWMNormalHints(Xdp, Xwp, &size_hints);
  XMapRaised(Xdp, Xwp);
  XFlush(Xdp);
  usleep(10000);
  goto display_text;
}

#endif /* __XFCURSES_LOADED */
#endif /* not NCURSES */

