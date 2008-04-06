#if !defined(REAL_DOS_H)
#define REAL_DOS_H

#include "StopMessage.h"
#include "strncasecmp.h"
#include "VideoModeKeyName.h"

extern int text_temp_message(char const *message);
extern int text_temp_message(const std::string &message);
extern int show_temp_message(char const *message);
extern int show_temp_message(const std::string &message);
extern void clear_temp_message();
extern void help_title();
extern int put_string_center(int row, int col, int width, int attr, char const *msg);
extern int put_string_center(int row, int col, int width, int attr, const std::string &msg);
extern int main_menu(bool full_menu);
extern int input_field(int options, int attribute, char *field, int len, int row, int col, int (*check_keystroke)(int key));
extern int field_prompt(std::string const &heading, std::string const &instructions, char *field, int len, int (*check_keystroke)(int key) = 0);
extern int field_prompt(std::string const &heading, char *field, int len, int (*check_keystroke)(int key) = 0);
extern int thinking(int options, char const *msg);
extern int check_video_mode_key(int key);
extern int check_vidmode_keyname(char const *name);
extern void free_temp_message();

#endif
