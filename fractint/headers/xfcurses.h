#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

struct	_win_st
{
	int	_cur_y, _cur_x;
	int	_car_y, _car_x;
	int	_num_y, _num_x;
	int	_cur_attr;
	char	*_text;
        short   *_attr;
};

#define	WINDOW struct _win_st
#define stdscr NULL

typedef unsigned chtype;

extern Display *Xdp;
extern Window Xw;
extern Window Xwc;
extern Window Xwp;
extern Window Xroot;
extern GC Xgc;
extern Visual *Xvi;
extern Screen *Xsc;
extern Colormap	Xcmap;
extern int Xdscreen;
extern int Xdepth;
extern char *Xmessage;
extern char *Xdisplay;
extern char *Xgeometry;
extern char *Xfontname;
extern char *Xfontnamebold;
extern Atom wm_delete_window, wm_protocols;

extern int COLS;
extern int LINES;

extern void Open_XDisplay();

extern void cbreak(void);
extern void nocbreak(void);
extern void echo(void);
extern void noecho(void);
extern void clear(void);
extern int  standout(void);
extern int  standend(void);
extern void endwin(void);
extern void refresh(int line1, int line2);
extern void xpopup(char *str);
extern void mvcur(int oldrow, int oldcol, int newrow, int newcol);

extern void delwin(WINDOW *win);
extern void waddch(WINDOW *win, const chtype ch);
extern void waddstr(WINDOW *win, char *str);
extern void wclear(WINDOW *win);
extern void wdeleteln(WINDOW *win);
extern void winsertln(WINDOW *win);
extern void wmove(WINDOW *win, int y, int x);
extern void wrefresh(WINDOW *win);
extern void xrefresh(WINDOW *win, int line1, int line2);
extern void touchwin(WINDOW *win);
extern void wtouchln(WINDOW *win, int y, int n, int changed);
extern void wstandout(WINDOW *win);
extern void wstandend(WINDOW *win);

extern WINDOW *newwin(int nlines, int ncols, int begin_y, int begin_x);
extern WINDOW *initscr(void);
extern void set_margins(int width, int height);

#define getyx(win,y,x)	 (y = (win)->_cur_y, x = (win)->_cur_x)


