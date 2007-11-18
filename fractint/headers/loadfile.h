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
	ext_blk_resume_info *,
	ext_blk_formula_info *,
	ext_blk_ranges_info *,
	ext_blk_mp_info *,
	ext_blk_evolver_info *,
	ext_blk_orbits_info *);

#endif
