#if !defined(CALC_FRAC_H)
#define CALC_FRAC_H

#define DEM_BAILOUT 535.5  /* (pb: not sure if this is special or arbitrary) */

extern int calculate_fractal();
extern int calculate_mandelbrot();
extern int calculate_mandelbrot_fp();
extern int standard_fractal();
extern void _fastcall plot_color_none(int x, int y, int color);
extern void _fastcall plot_color_symmetry_x_axis(int x, int y, int color);
extern void _fastcall plot_color_symmetry_origin(int x, int y, int color);

extern void (_fastcall *g_plot_color)(int x, int y, int color);
extern void sym_fill_line(int row, int left, int right, BYTE *str);

extern int g_ix_start;
extern int g_iy_start;
extern int g_work_pass;
extern int g_work_sym;
extern int g_and_color;					/* AND mask for iteration to get color index */

#endif
