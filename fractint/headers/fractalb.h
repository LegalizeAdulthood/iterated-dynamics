#if !defined(FRACTAL_B_H)
#define FRACTAL_B_H

extern ComplexD complex_bn_to_float(ComplexBigNum *);
extern ComplexD complex_bf_to_float(ComplexBigFloat *);
extern void compare_values(char *, LDBL, bn_t);
extern void compare_values_bf(char *, LDBL, bf_t);
extern void show_var_bf(char *s, bf_t n);
extern void show_two_bf(char *, bf_t, char *, bf_t, int);
extern void corners_bf_to_float();
extern void show_corners_dbl(char *);
extern int mandelbrot_setup_bn();
extern int mandelbrot_per_pixel_bn();
extern int julia_per_pixel_bn();
extern int julia_orbit_bn();
extern int julia_z_power_orbit_bn();
extern ComplexBigNum *complex_log_bn(ComplexBigNum *t, ComplexBigNum *s);
extern ComplexBigNum *complex_multiply_bn(ComplexBigNum *t, ComplexBigNum *x, ComplexBigNum *y);
extern ComplexBigNum *complex_power_bn(ComplexBigNum *t, ComplexBigNum *xx, ComplexBigNum *yy);
extern int mandelbrot_setup_bf();
extern int mandelbrot_per_pixel_bf();
extern int julia_per_pixel_bf();
extern int julia_orbit_bf();
extern int julia_z_power_orbit_bf();
extern ComplexBigFloat *complex_log_bf(ComplexBigFloat *t, ComplexBigFloat *s);
extern ComplexBigFloat *cplxmul_bf(ComplexBigFloat *t, ComplexBigFloat *x, ComplexBigFloat *y);
extern ComplexBigFloat *ComplexPower_bf(ComplexBigFloat *t, ComplexBigFloat *xx, ComplexBigFloat *yy);

#endif
