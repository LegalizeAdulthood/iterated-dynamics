// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "big.h"
#include "cmplx.h"

#include <complex>

extern int                   g_basin;
extern int                   g_bit_shift_less_1;
extern int                   g_c_exponent;
extern double                g_cos_x;
extern int                   g_degree;
extern DComplex *            g_float_param;
extern long                  g_fudge_half;
extern long                  g_fudge_one;
extern long                  g_fudge_two;
extern LComplex              g_l_coefficient;
extern LComplex              g_l_init;
extern LComplex              g_l_old_z;
extern LComplex              g_l_new_z;
extern LComplex              g_l_param;
extern LComplex              g_l_param2;
extern LComplex              g_l_temp;
extern long                  g_l_temp_sqr_x;
extern long                  g_l_temp_sqr_y;
extern LComplex *            g_long_param;
extern DComplex              g_marks_coefficient;
extern int                   g_max_color;
extern DComplex              g_param_z1;
extern DComplex              g_param_z2;
extern DComplex              g_power_z;
extern double                g_quaternion_c;
extern double                g_quaternion_ci;
extern double                g_quaternion_cj;
extern double                g_quaternion_ck;
extern double                g_sin_x;
extern double                g_temp_sqr_x;
extern double                g_temp_sqr_y;
extern double                g_threshold;

void cpower(DComplex *, int, DComplex *);
int lcpower(LComplex *, int, LComplex *, int);
int lcomplex_mult(LComplex, LComplex, LComplex *, int);
int complex_mult(DComplex arg1, DComplex arg2, DComplex *pz);
int complex_div(DComplex numerator, DComplex denominator, DComplex *pout);
int MPCNewtonFractal();
int JuliaFractal();
int JuliafpFractal();
int longCmplxZpowerFractal();
int floatCmplxZpowerFractal();
int TrigPlusZsquaredFractal();
int TrigPlusZsquaredfpFractal();
int long_julia_per_pixel();
int long_mandel_per_pixel();
int julia_per_pixel();
int mandel_per_pixel();
int mandelfp_per_pixel();
void mandel_ref_pt(const std::complex<double> &center, std::complex<double> &z);
void mandel_ref_pt_bf(const BFComplex &center, BFComplex &z);
void mandel_perturb(
    const std::complex<double> &ref, std::complex<double> &delta_n, const std::complex<double> &delta0);
int juliafp_per_pixel();
int othermandelfp_per_pixel();
int otherjuliafp_per_pixel();
int dynamfloat(double *, double *, double*);
int mandelcloudfloat(double *, double *, double*);
int dynam2dfloat();
int fpMODbailout();
int fpREALbailout();
int fpIMAGbailout();
int fpORbailout();
int fpANDbailout();
int fpMANHbailout();
int fpMANRbailout();
int bnMODbailout();
int bnREALbailout();
int bnIMAGbailout();
int bnORbailout();
int bnANDbailout();
int bnMANHbailout();
int bnMANRbailout();
int bfMODbailout();
int bfREALbailout();
int bfIMAGbailout();
int bfORbailout();
int bfANDbailout();
int bfMANHbailout();
int bfMANRbailout();
void set_pixel_calc_functions();
