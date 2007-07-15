#if !defined(JB_H)
#define JB_H

extern int julibrot_setup();
extern int julibrot_setup_fp();
extern int julibrot_per_pixel();
extern int julibrot_per_pixel_fp();
extern int standard_4d_fractal();
extern int standard_4d_fractal_fp();
extern void standard_4d_fractal_set_orbit_calc(
	int (*orbit_function)(void), int (*complex_orbit_function)(void));

#endif
