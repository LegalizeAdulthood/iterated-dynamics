#if !defined(PROMPTS_1_H)
#define PROMPTS_1_H

extern const std::string GLASSES1_MAP;
extern const std::string GLASSES2_MAP;

extern const char *g_jiim_left_right[2];
extern const char *g_jiim_method[3];

extern int full_screen_prompt(const char *heading, int num_prompts, const char **prompts,
	full_screen_values *values, int function_key_mask, char *footer);
extern int full_screen_prompt_help(int help_mode, const char *heading, int num_prompts, const char **prompts,
	full_screen_values *values, int function_key_mask, char *footer);
extern long get_file_entry(int type, const char *title, char *fmask,
					char *filename, char *entryname);
extern int get_fractal_type();
extern int get_fractal_parameters(bool type_specific);
extern int get_fractal_3d_parameters();
extern int get_3d_parameters();
extern int prompt_value_string(char *buf, full_screen_values *val);
extern void set_bail_out_formula(enum bailouts);
extern int find_extra_parameter(int);
extern void load_parameters(int g_fractal_type);
extern int check_orbit_name(char *);
extern int scan_entries(std::ifstream &infile, entry_info *choices, const char *itemname);
extern void set_default_parms();

#endif
