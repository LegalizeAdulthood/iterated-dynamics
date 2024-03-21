#pragma once

#include "main_state.h"

struct EVOLUTION_INFO;

extern EVOLUTION_INFO        g_evolve_info;
extern int                   g_finish_row;
extern bool                  g_from_text;
extern bool                  g_have_evolve_info;
extern char                  g_old_std_calc_mode;
extern void                (*g_out_line_cleanup)();
extern bool                  g_virtual_screens;

main_state big_while_loop(bool *kbdmore, bool *stacked, bool resume_flag);
void clear_zoombox();
int key_count(int keynum);
void move_zoombox(int keynum);
void reset_zoom_corners();
