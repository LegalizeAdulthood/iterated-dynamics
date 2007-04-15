#if !defined(FRACTINT_HELP_H)
#define FRACTINT_HELP_H

extern void set_help_mode(int new_mode);
extern void push_help_mode(int new_mode);
extern void pop_help_mode(void);
extern int get_help_mode(void);
extern int full_screen_prompt_help(int help_mode, char *hdg, int numprompts, char **prompts,
	struct full_screen_values *values, int fkeymask, char *extrainfo);
extern int field_prompt_help(int help_mode,
	char *hdg, char *instr, char *fld, int len, int (*checkkey)(int));
extern long get_file_entry_help(int help_mode,
	int type, char *title, char *fmask, char *filename, char *entryname);
extern int get_a_filename_help(int help_mode, char *hdg, char *file_template, char *flname);

#endif
