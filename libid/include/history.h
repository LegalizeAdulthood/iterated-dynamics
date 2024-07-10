#pragma once

extern bool                  g_history_flag;
extern int                   g_history_ptr;
extern int                   g_max_image_history;

void history_init();
void restore_history_info(int i);
void save_history_info();
