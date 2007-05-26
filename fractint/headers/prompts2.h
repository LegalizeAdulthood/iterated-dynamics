#if !defined(PROMPTS_2_H)
#define PROMPTS_2_H

extern int get_toggles();
extern int get_toggles2();
extern int passes_options();
extern int get_view_params();
extern int get_starfield_params();
extern int get_commands();
extern void goodbye();
extern void shell_sort(void *, int n, unsigned, int (__cdecl *fct)(VOIDPTR, VOIDPTR));
extern int get_browse_parameters();
extern int get_command_string();
extern int get_random_dot_stereogram_parameters();
extern int starfield();
extern int get_a_number(double *, double *);
extern int lccompare(VOIDPTR, VOIDPTR);
extern int integer_unsupported();

#endif
