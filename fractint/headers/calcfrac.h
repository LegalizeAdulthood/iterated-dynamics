#if !defined(CALC_FRAC_H)
#define CALC_FRAC_H

#define DEM_BAILOUT 535.5  /* (pb: not sure if this is special or arbitrary) */

enum CalculationMode
{
	CALCMODE_SINGLE_PASS = '1',
	CALCMODE_DUAL_PASS = '2',
	CALCMODE_TRIPLE_PASS = '3',
	CALCMODE_SOLID_GUESS = 'g',
	CALCMODE_BOUNDARY_TRACE = 'b',
	CALCMODE_DIFFUSION = 'd',
	CALCMODE_TESSERAL = 't',
	CALCMODE_SYNCHRONOUS_ORBITS = 's',
	CALCMODE_ORBITS = 'o'
};

extern CalculationMode g_user_standard_calculation_mode;
extern CalculationMode g_standard_calculation_mode_old;
extern CalculationMode g_standard_calculation_mode;

extern int calculate_fractal();
extern int calculate_mandelbrot_l();
extern int calculate_mandelbrot_fp();
extern int standard_fractal();
extern void plot_color_none(int x, int y, int color);
extern void plot_color_symmetry_x_axis(int x, int y, int color);
extern void plot_color_symmetry_origin(int x, int y, int color);

extern void (*g_plot_color)(int x, int y, int color);
extern void sym_fill_line(int row, int left, int right, BYTE *str);

extern int g_ix_start;
extern int g_iy_start;
extern int g_work_pass;
extern int g_work_sym;
extern int g_and_color;					/* AND mask for iteration to get color index */

#endif
