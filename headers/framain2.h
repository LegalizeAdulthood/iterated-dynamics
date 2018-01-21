#pragma once
#if !defined(FRAMAIN2_H)
#define FRAMAIN2_H

extern main_state big_while_loop(bool *kbdmore, bool *stacked, bool resume_flag);
extern bool check_key();
extern int cmp_line(BYTE *, int);
extern int key_count(int);
extern main_state main_menu_switch(int *kbdchar, bool *frommandel, bool *kbdmore, bool *stacked);
extern int pot_line(BYTE *, int);
extern int sound_line(BYTE *, int);
extern int timer(int, int (*subrtn)(), ...);
extern void clear_zoombox();
extern void flip_image(int kbdchar);
extern void reset_zoom_corners();
extern void history_init();

#endif
