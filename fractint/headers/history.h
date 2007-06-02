#if !defined(HISTORY_H)
#define HISTORY_H

void _fastcall history_restore_info();
void _fastcall history_save_info();
void history_allocate();
void history_free();
void history_back();
void history_forward();

#endif
