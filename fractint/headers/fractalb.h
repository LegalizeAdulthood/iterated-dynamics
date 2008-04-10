#if !defined(FRACTAL_B_H)
#define FRACTAL_B_H

#include "big.h"

extern void compare_values(char *, LDBL, bn_t);
extern void compare_values_bf(char *, LDBL, bf_t);
extern void show_var_bf(char *s, bf_t n);
extern void show_two_bf(char *, bf_t, char *, bf_t, int);
extern void corners_bf_to_float();
extern void show_corners_dbl(char *);
extern bool mandelbrot_setup_bn();
extern int mandelbrot_per_pixel_bn();
extern int julia_per_pixel_bn();
extern int julia_orbit_bn();
extern int julia_z_power_orbit_bn();
extern void complex_log_bn(ComplexBigNum &t, ComplexBigNum const &s);
extern void complex_multiply_bn(ComplexBigNum &t, ComplexBigNum const &x, ComplexBigNum const &y);
extern void complex_power_bn(ComplexBigNum &t, ComplexBigNum const &xx, ComplexBigNum const &yy);
extern bool mandelbrot_setup_bf();
extern int mandelbrot_per_pixel_bf();
extern int julia_per_pixel_bf();
extern int julia_orbit_bf();
extern int julia_z_power_orbit_bf();
extern void complex_log_bf(ComplexBigFloat &t, ComplexBigFloat const &s);
extern void cplxmul_bf(ComplexBigFloat &t, ComplexBigFloat const &x, ComplexBigFloat const &y);
extern void ComplexPower_bf(ComplexBigFloat &t, ComplexBigFloat const &xx, ComplexBigFloat const &yy);

#endif
