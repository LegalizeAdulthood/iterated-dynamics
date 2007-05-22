#if !defined(FRA_MAIN_2_H)
#define FRA_MAIN_2_H

extern ApplicationStateType big_while_loop(int &kbdmore, bool &screen_stacked, bool resume_flag);
extern int check_key();
extern int out_line_compare(BYTE *, int);
extern int key_count(int);
#if !defined(XFRACT)
#if !defined(_WIN32)
extern int _cdecl _matherr(struct exception *);
#endif
#else
extern int XZoomWaiting;
#endif
#ifndef USE_VARARGS
extern int timer(int, int (*subrtn)(), ...);
#else
extern int timer();
#endif

extern void clear_zoom_box();
extern void flip_image(int kbdchar);
#ifndef WINFRACT
extern void reset_zoom_corners();
#endif

#endif
