#if !defined(EDIT_PAL_H)
#define EDIT_PAL_H

extern void palette_edit();
void put_row(int x, int y, int width, char *buff);
void get_row(int x, int y, int width, char *buff);
int cursor_wait_key();
void cursor_check_blink();
#ifdef XFRACT
void cursor_start_mouse_tracking();
void cursor_end_mouse_tracking();
#endif
bool cursor_new();
void cursor_destroy();
void cursor_set_position(int x, int y);
void cursor_move(int xoff, int yoff);
int cursor_get_x();
int cursor_get_y();
void cursor_hide();
void cursor_show();
extern void displayc(int, int, int, int, int);

#endif
