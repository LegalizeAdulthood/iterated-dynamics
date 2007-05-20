#if !defined(CALC_FRAC_H)
#define CALC_FRAC_H

extern int calculate_fractal();
extern int calculate_mandelbrot();
extern int calculate_mandelbrot_fp();
extern int standard_fractal();
extern int find_alternate_math(int, int);
extern void _fastcall plot_color_none(int x, int y, int color);
extern void _fastcall plot_color_symmetry_x_axis(int x, int y, int color);
extern void _fastcall plot_color_symmetry_origin(int x, int y, int color);

extern void (_fastcall *g_plot_color)(int x, int y, int color);

#endif
