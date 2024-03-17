#pragma once

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
extern bool                  g_from_text;
extern bool                  g_have_evolve_info;
extern char                  g_old_std_calc_mode;
extern void                (*g_out_line_cleanup)();
extern bool                  g_virtual_screens;

main_state big_while_loop(bool *kbdmore, bool *stacked, bool resume_flag);
void clear_zoombox();
int cmp_line(BYTE *pixels, int linelen);
int key_count(int keynum);
main_state main_menu_switch(int *kbdchar, bool *frommandel, bool *kbdmore, bool *stacked);
void move_zoombox(int keynum);
void reset_zoom_corners();
