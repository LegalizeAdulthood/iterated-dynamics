#if !defined(FRA_MAIN_2_H)
#define FRA_MAIN_2_H

extern ApplicationStateType big_while_loop(bool &kbdmore, bool &screen_stacked, bool resume_flag);
extern int check_key();
extern int key_count(int);
#if !defined(XFRACT)
#if !defined(_WIN32)
extern int _cdecl _matherr(struct exception *);
#endif
#else
extern int XZoomWaiting;
#endif
extern int timer(int, int (*subrtn)(), ...);
extern void clear_zoom_box();
extern void flip_image(int kbdchar);
#ifndef WINFRACT
extern void reset_zoom_corners();
#endif

#endif
