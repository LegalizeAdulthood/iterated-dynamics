#pragma once

extern bool                  historyflag;
extern int                   historyptr;
extern int                   g_max_image_history;

void history_init();
void restore_history_info(int i);
void save_history_info();
