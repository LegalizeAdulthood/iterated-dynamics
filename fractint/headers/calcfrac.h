#if !defined(CALC_FRAC_H)
#define CALC_FRAC_H

extern int calculate_fractal();
extern int calculate_mandelbrot();
extern int calculate_mandelbrot_fp();
extern int standard_fractal();
extern int test();
extern int plasma();
extern int diffusion();
extern int bifurcation();
extern int bifurcation_lambda();
extern int bifurcation_set_trig_pi_fp();
extern int bifurcation_set_trig_pi();
extern int bifurcation_add_trig_pi_fp();
extern int bifurcation_add_trig_pi();
extern int bifurcation_may_fp();
extern int bifurcation_may_setup();
extern int bifurcation_may();
extern int bifurcation_lambda_trig_fp();
extern int bifurcation_lambda_trig();
extern int bifurcation_verhulst_trig_fp();
extern int bifurcation_verhulst_trig();
extern int bifurcation_stewart_trig_fp();
extern int bifurcation_stewart_trig();
extern int popcorn();
extern int lyapunov();
extern int lyapunov_setup();
extern int cellular();
extern int cellular_setup();
extern int froth_calc();
extern int froth_per_pixel();
extern int froth_per_orbit();
extern int froth_setup();
extern int logtable_in_extra_ok();
extern int find_alternate_math(int, int);

#endif
