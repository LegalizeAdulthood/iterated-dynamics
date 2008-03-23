#pragma once

#include "StopMessage.h"
#include "strncasecmp.h"
#include "VideoModeKeyName.h"

extern int text_temp_message(const char *message);
extern int text_temp_message(const std::string &message);
extern int show_temp_message(const char *message);
extern int show_temp_message(const std::string &message);
extern void clear_temp_message();
extern void help_title();
extern int put_string_center(int, int, int, int, const char *);
extern int put_string_center(int row, int col, int width, int attr, const std::string &msg);
extern int main_menu(bool full_menu);
extern int input_field(int, int, char *, int, int, int, int (*)(int));
extern int field_prompt(char *, char *, char *, int, int (*)(int));
extern int thinking(int, char *);
extern int check_video_mode_key(int key);
extern int check_vidmode_keyname(char const *name);
extern void free_temp_message();
extern void load_video_table(int);
