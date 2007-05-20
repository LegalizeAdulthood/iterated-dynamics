#if !defined(CALC_FRAC_H)
#define CALC_FRAC_H

extern int calculate_fractal();
extern int calculate_mandelbrot();
extern int calculate_mandelbrot_fp();
extern int standard_fractal();
extern int find_alternate_math(int, int);
extern void _fastcall plot_color_none(int, int, int);
extern void _fastcall plot_color_symmetry_pi(int, int, int);
extern void _fastcall plot_color_symmetry_pi_origin(int, int, int);
extern void _fastcall plot_color_symmetry_pi_xy_axis(int, int, int);
extern void _fastcall plot_color_symmetry_x_axis(int, int, int);
extern void _fastcall plot_color_symmetry_y_axis(int, int, int);
extern void _fastcall plot_color_symmetry_origin(int, int, int);
extern void _fastcall plot_color_symmetry_xy_axis(int, int, int);
extern void _fastcall plot_color_symmetry_x_axis_basin(int, int, int);
extern void _fastcall plot_color_symmetry_xy_axis_basin(int, int, int);

extern void (_fastcall *g_plot_color)(int,int,int);

#endif
