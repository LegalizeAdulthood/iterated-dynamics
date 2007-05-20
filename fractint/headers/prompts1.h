#if !defined(PROMPTS_1_H)
#define PROMPTS_1_H

extern int full_screen_prompt(const char *heading, int num_prompts, const char **prompts,
	struct full_screen_values *values, int function_key_mask, char *footer);
extern long get_file_entry(int type, const char *title, char *fmask, char *filename, char *entryname);
extern int get_fractal_type();
extern int get_fractal_parameters(int);
extern int get_fractal_3d_parameters();
extern int get_3d_parameters();
extern int prompt_value_string(char *buf, struct full_screen_values *val);
extern void set_bail_out_formula(enum bailouts);
extern int find_extra_parameter(int);
extern void load_parameters(int g_fractal_type);
extern int check_orbit_name(char *);
extern int scan_entries(FILE *infile, struct entryinfo *choices, const char *itemname);
extern void set_default_parms();

#endif
