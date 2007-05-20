#if !defined(CALC_FRAC_H)
#define CALC_FRAC_H

extern int calculate_fractal();
extern int calculate_mandelbrot();
extern int calculate_mandelbrot_fp();
extern int standard_fractal();
extern int find_alternate_math(int, int);

extern void (_fastcall *g_plot_color)(int,int,int);

#endif
