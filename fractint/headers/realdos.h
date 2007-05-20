#if !defined(REAL_DOS_H)
#define REAL_DOS_H

extern int show_vid_length();
extern int stop_message(int, const char *);
extern void blank_rows(int, int, int);
extern int text_temp_message(char *);
extern int full_screen_choice(int options, const char *hdg, char *hdg2,
	char *instr, int numchoices, char **choices, int *attributes,
	int boxwidth, int boxdepth, int colwidth, int current,
	void (*formatitem)(int, char *), char *speedstring,
	int (*speedprompt)(int, int, int, char *, int),
	int (*checkkey)(int, int));
extern int full_screen_choice_help(int help_mode, int options,
	const char *hdg, char *hdg2, char *instr, int numchoices,
	char **choices, int *attributes, int boxwidth, int boxdepth,
	int colwidth, int current, void (*formatitem)(int, char *),
	char *speedstring, int (*speedprompt)(int, int, int, char *, int),
	int (*checkkey)(int, int));
extern int show_temp_message(char *);
extern void clear_temp_message();
extern void help_title();
extern int put_string_center(int, int, int, int, const char *);
#ifndef XFRACT /* Unix should have this in string.h */
extern int strncasecmp(char *, char *, int);
#endif
extern int main_menu(bool full_menu);
extern int input_field(int, int, char *, int, int, int, int (*)(int));
extern int field_prompt(char *, char *, char *, int, int (*)(int));
extern int thinking(int, char *);
extern void load_fractint_config();
extern int check_video_mode_key(int, int);
extern int check_vidmode_keyname(char *);
extern void video_mode_key_name(int, char *);
extern void free_temp_message();
extern void load_video_table(int);
extern void bad_fractint_cfg_msg();

#endif
