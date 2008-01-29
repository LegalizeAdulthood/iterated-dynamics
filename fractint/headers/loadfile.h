#if !defined(LOAD_FILE_H)
#define LOAD_FILE_H

extern int read_overlay();
extern void set_if_old_bif();
extern void set_function_parm_defaults();
extern void backwards_v18();
extern void backwards_v19();
extern void backwards_v20();
extern bool check_back();
int find_fractal_info(const char *gif_file, fractal_info *,
	resume_info_extension_block *,
	formula_info_extension_block *,
	ranges_info_extension_block *,
	multiple_precision_info_extension_block *,
	evolver_info_extension_block *,
	orbits_info_extension_block *);

#endif
