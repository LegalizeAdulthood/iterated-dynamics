#pragma once
#if !defined(FRAMAIN2_H)
#define FRAMAIN2_H

struct EVOLUTION_INFO;

enum class main_state
{
    NOTHING = 0,
    RESTART,
    IMAGE_START,
    RESTORE_START,
    CONTINUE
};

extern EVOLUTION_INFO        g_evolve_info;
extern int                   g_finish_row;
extern bool                  g_have_evolve_info;
extern int                   g_max_image_history;
extern char                  g_old_std_calc_mode;
extern void                (*g_out_line_cleanup)();
extern bool                  g_virtual_screens;

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
