#pragma once

// stop_message() flags 
enum StopMessageFlag
{
	STOPMSG_NORMAL		= 0,
	STOPMSG_NO_STACK	= 1,
	STOPMSG_CANCEL		= 2,
	STOPMSG_NO_BUZZER	= 4,
	STOPMSG_FIXED_FONT	= 8,
	STOPMSG_INFO_ONLY	= 16
};

extern int stop_message(int flags, const std::string &message);
extern void blank_rows(int, int, int);
extern int text_temp_message(const char *message);
extern int text_temp_message(const std::string &message);
extern int show_temp_message(const char *message);
extern int show_temp_message(const std::string &message);
extern void clear_temp_message();
extern void help_title();
extern int put_string_center(int, int, int, int, const char *);
extern int put_string_center(int row, int col, int width, int attr, const std::string &msg);
#ifndef XFRACT // Unix should have this in string.h 
extern int strncasecmp(const char *, const char *, int);
#endif
extern int main_menu(bool full_menu);
extern int input_field(int, int, char *, int, int, int, int (*)(int));
extern int field_prompt(char *, char *, char *, int, int (*)(int));
extern int thinking(int, char *);
extern int check_video_mode_key(int);
extern int check_vidmode_keyname(char *);
extern void video_mode_key_name(int key, char *name);
extern std::string video_mode_key_name(int key);
extern void free_temp_message();
extern void load_video_table(int);
