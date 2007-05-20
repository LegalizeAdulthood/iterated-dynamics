#if !defined(PROMPTS_2_H)
#define PROMPTS_2_H

extern int get_toggles();
extern int get_toggles2();
extern int passes_options();
extern int get_view_params();
extern int get_starfield_params();
extern int get_commands();
extern void goodbye();
extern bool is_a_directory(char *s);
extern int get_a_filename(char *, char *, char *);
extern int split_path(const char *file_template, char *drive, char *dir, char *fname, char *ext);
extern int make_path(char *file_template, char *drive, char *dir, char *fname, char *ext);
extern int fr_find_first(char *path);
extern int fr_find_next();
extern void shell_sort(void *, int n, unsigned, int (__cdecl *fct)(VOIDPTR, VOIDPTR));
extern void fix_dir_name(char *dirname);
extern int merge_path_names(char *, char *, int);
extern int get_browse_parameters();
extern int get_command_string();
extern int get_random_dot_stereogram_parameters();
extern int starfield();
extern int get_a_number(double *, double *);
extern int lccompare(VOIDPTR, VOIDPTR);
extern int dir_remove(char *, char *);
extern FILE *dir_fopen(const char *, const char *, const char *);
extern void extract_filename(char *, char *);
extern char *has_extension(char *source);
extern int integer_unsupported();

#endif
