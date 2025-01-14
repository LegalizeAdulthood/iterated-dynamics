// SPDX-License-Identifier: GPL-3.0-only
//
// Wesley Loewer's Big Numbers.        (C) 1994, Wesley B. Loewer
#pragma once

#include "math/cmplx.h"
#include "misc/sized_types.h"

#include <config/port.h>

// Number of bytes to use for integer part for fixed decimal math,
// does not effect floating point math at all.
enum
{
    BN_INT_LENGTH = 4
};

/* #define CALCULATING_BIG_PI */ /* define for generating g_big_pi[] table */
/****************************************************************
 The rest is handled by the compiler
****************************************************************/
#define LOG10_256 2.4082399653118
#define LOG_256   5.5451774444795
// values that g_bf_math can hold,
// 0 = g_bf_math is not being used
// 1 = g_bf_math is being used
enum class BFMathType
{
    // cppcheck-suppress variableHidingEnum
    NONE = 0,
    BIG_NUM = 1,         // g_bf_math is being used with bn_t numbers
    BIG_FLT = 2          // g_bf_math is being used with bf_t numbers
};

using big_t = unsigned char *;
using bn_t = big_t;  // for clarification purposes
using bf_t = big_t;
using bf10_t = big_t;

using BFComplex = id::Complex<bf_t>;
using BNComplex = id::Complex<bn_t>;

// globals
extern BFMathType g_bf_math;
extern int g_bn_step;
extern int g_int_length;
extern int g_bn_length;
extern int g_r_length;
extern int g_padding;
extern int g_decimals;
extern int g_shift_factor;
extern int g_bf_length;
extern int g_r_bf_length;
extern int g_bf_decimals;
extern bn_t g_bn_tmp1;    // g_r_length
extern bn_t g_bn_tmp2;    // g_r_length
extern bn_t g_bn_tmp3;    // g_r_length
extern bn_t g_bn_tmp4;    // g_r_length
extern bn_t g_bn_tmp5;    // g_r_length
extern bn_t g_bn_tmp6;    // g_r_length
extern bn_t g_bn_tmp_copy1; // g_bn_length
extern bn_t g_bn_tmp_copy2; // g_bn_length
extern bn_t g_bn_pi;
extern bn_t g_bn_tmp;     // g_r_length
extern bf_t g_bf_tmp1;    // g_r_bf_length+2
extern bf_t g_bf_tmp2;    // g_r_bf_length+2
extern bf_t g_bf_tmp3;    // g_r_bf_length+2
extern bf_t g_bf_tmp4;    // g_r_bf_length+2
extern bf_t g_bf_tmp5;    // g_r_bf_length+2
extern bf_t g_bf_tmp6;    // g_r_bf_length+2
extern bf_t g_bf_tmp_copy1; // g_r_bf_length+2
extern bf_t g_bf_tmp_copy2; // g_r_bf_length+2
extern bf_t g_bf_pi;
extern bf_t g_bf_tmp;     // g_r_bf_length
extern bf10_t g_bf10_tmp; // dec+4
extern big_t g_big_pi;

void calc_lengths();
void init_big_dec(int dec);
void init_big_length(int bnl);
void init_big_pi();
bn_t clear_bn(bn_t r);
bn_t max_bn(bn_t r);
bn_t copy_bn(bn_t r, bn_t n);
int cmp_bn(bn_t n1, bn_t n2);
bool is_bn_neg(bn_t n);
bool is_bn_not_zero(bn_t n);
bn_t add_bn(bn_t r, bn_t n1, bn_t n2);
bn_t add_a_bn(bn_t r, bn_t n);
bn_t sub_bn(bn_t r, bn_t n1, bn_t n2);
bn_t sub_a_bn(bn_t r, bn_t n);
bn_t neg_bn(bn_t r, bn_t n);
bn_t neg_a_bn(bn_t r);
bn_t double_bn(bn_t r, bn_t n);
bn_t double_a_bn(bn_t r);
bn_t half_bn(bn_t r, bn_t n);
bn_t half_a_bn(bn_t r);
bn_t unsafe_full_mult_bn(bn_t r, bn_t n1, bn_t n2);
bn_t unsafe_mult_bn(bn_t r, bn_t n1, bn_t n2);
bn_t unsafe_full_square_bn(bn_t r, bn_t n);
bn_t unsafe_square_bn(bn_t r, bn_t n);
bn_t mult_bn_int(bn_t r, bn_t n, U16 u);
bn_t mult_a_bn_int(bn_t r, U16 u);
bn_t unsafe_div_bn_int(bn_t r, bn_t n, U16 u);
bn_t div_a_bn_int(bn_t r, U16 u);
// used to be in bigflta.asm or bigfltc.c
bf_t clear_bf(bf_t r);
bf_t copy_bf(bf_t r, bf_t n);
bf_t float_to_bf(bf_t r, LDouble f);
LDouble bf_to_float(bf_t n);
LDouble bn_to_float(bn_t n);
LDouble extract256(LDouble f, int *exp_ptr);
LDouble scale256(LDouble f, int n);

#ifdef ACCESS_BY_BYTE
// prototypes
U32 big_access32(Byte *addr);
U16 big_access16(Byte *addr);
S16 big_accessS16(S16 *addr);
U32 big_set32(Byte *addr, U32 val);
U16 big_set16(Byte *addr, U16 val);
S16 big_setS16(S16 *addr, S16 val);
#else
// equivalent defines
#define big_access32(addr)   (*(U32 *)(addr))
#define big_access16(addr)   (*(U16 *)(addr))
#define big_accessS16(addr)   (*(S16 *)(addr))
#define big_set32(addr, val) (*(U32 *)(addr) = (U32)(val))
#define big_set16(addr, val) (*(U16 *)(addr) = (U16)(val))
#define big_setS16(addr, val) (*(S16 *)(addr) = (S16)(val))
#endif
void bn_hex_dump(bn_t r);
bn_t str_to_bn(bn_t r, char *s);
char *unsafe_bn_to_str(char *s, int dec, bn_t r);
bn_t int_to_bn(bn_t r, long value);
long bn_to_int(bn_t n);
int  sign_bn(bn_t n);
bn_t abs_bn(bn_t r, bn_t n);
bn_t abs_a_bn(bn_t r);
bn_t unsafe_inv_bn(bn_t r, bn_t n);
bn_t unsafe_div_bn(bn_t r, bn_t n1, bn_t n2);
bn_t sqrt_bn(bn_t r, bn_t n);
bn_t exp_bn(bn_t r, bn_t n);
bn_t unsafe_ln_bn(bn_t r, bn_t n);
bn_t unsafe_sin_cos_bn(bn_t s, bn_t c, bn_t n);
bn_t unsafe_atan_bn(bn_t r, bn_t n);
bn_t unsafe_atan2_bn(bn_t r, bn_t ny, bn_t nx);
int convert_bn(bn_t new_num, bn_t old_num, int new_bn_len, int new_int_len, int old_bn_len, int old_int_len);
// "safe" versions
bn_t full_mult_bn(bn_t r, bn_t n1, bn_t n2);
bn_t mult_bn(bn_t r, bn_t n1, bn_t n2);
bn_t full_square_bn(bn_t r, bn_t n);
bn_t square_bn(bn_t r, bn_t n);
bn_t div_bn_int(bn_t r, bn_t n, U16 u);
char *bn_to_str(char *s, int dec, bn_t r);
bn_t inv_bn(bn_t r, bn_t n);
bn_t div_bn(bn_t r, bn_t n1, bn_t n2);
bn_t ln_bn(bn_t r, bn_t n);
bn_t sin_cos_bn(bn_t s, bn_t c, bn_t n);
bn_t atan_bn(bn_t r, bn_t n);
bn_t atan2_bn(bn_t r, bn_t ny, bn_t nx);
// misc
bool is_bn_zero(bn_t n);
bn_t float_to_bn(bn_t r, LDouble f);

void bf_hex_dump(bf_t r);
bf_t str_to_bf(bf_t r, char const *s);
int strlen_needed_bf();
char *unsafe_bf_to_str(char *s, int dec, bf_t r);
char *unsafe_bf_to_str_e(char *s, int dec, bf_t r);
char *unsafe_bf_to_str_f(char *s, int dec, bf_t r);
bn_t bf_to_bn(bn_t n, bf_t f);
bn_t bn_to_bf(bf_t f, bn_t n);
long bf_to_int(bf_t f);
bf_t int_to_bf(bf_t r, long value);
int sign_bf(bf_t n);
bf_t abs_bf(bf_t r, bf_t n);
bf_t abs_a_bf(bf_t r);
bf_t unsafe_inv_bf(bf_t r, bf_t n);
bf_t unsafe_div_bf(bf_t r, bf_t n1, bf_t n2);
bf_t unsafe_sqrt_bf(bf_t r, bf_t n);
bf_t exp_bf(bf_t r, bf_t n);
bf_t unsafe_ln_bf(bf_t r, bf_t n);
bf_t unsafe_sin_cos_bf(bf_t s, bf_t c, bf_t n);
bf_t unsafe_atan_bf(bf_t r, bf_t n);
bf_t unsafe_atan2_bf(bf_t r, bf_t ny, bf_t nx);
bf_t add_bf(bf_t r, bf_t n1, bf_t n2);
bf_t add_a_bf(bf_t r, bf_t n);
bf_t sub_bf(bf_t r, bf_t n1, bf_t n2);
bf_t sub_a_bf(bf_t r, bf_t n);
bf_t full_mult_bf(bf_t r, bf_t n1, bf_t n2);
bf_t mult_bf(bf_t r, bf_t n1, bf_t n2);
bf_t full_square_bf(bf_t r, bf_t n);
bf_t square_bf(bf_t r, bf_t n);
bf_t mult_bf_int(bf_t r, bf_t n, U16 u);
bf_t div_bf_int(bf_t r, bf_t n,  U16 u);
char *bf_to_str(char *s, int dec, bf_t r);
char *bf_to_str_e(char *s, int dec, bf_t r);
char *bf_to_str_f(char *s, int dec, bf_t r);
bf_t inv_bf(bf_t r, bf_t n);
bf_t div_bf(bf_t r, bf_t n1, bf_t n2);
bf_t sqrt_bf(bf_t r, bf_t n);
bf_t ln_bf(bf_t r, bf_t n);
bf_t sin_cos_bf(bf_t s, bf_t c, bf_t n);
bf_t atan_bf(bf_t r, bf_t n);
bf_t atan2_bf(bf_t r, bf_t ny, bf_t nx);
bool is_bf_zero(bf_t n);
int convert_bf(bf_t new_num, bf_t old_num, int new_bf_len, int old_bf_len);
LDouble extract_value(LDouble f, LDouble b, int *exp_ptr);
LDouble scale_value(LDouble f, LDouble b , int n);
LDouble extract_10(LDouble f, int *exp_ptr);
LDouble scale_10(LDouble f, int n);
bf10_t unsafe_bf_to_bf10(bf10_t s, int dec, bf_t n);
bf10_t mult_a_bf10_int(bf10_t s, int dec, U16 n);
bf10_t div_a_bf10_int(bf10_t s, int dec, U16 n);
char  *bf10_to_str_e(char *s, int dec, bf10_t n);
char  *bf10_to_str_f(char *s, int dec, bf10_t n);

bf_t norm_bf(bf_t r);
void norm_sign_bf(bf_t r, bool positive);
S16 adjust_bf_add(bf_t n1, bf_t n2);
bf_t max_bf(bf_t r);
int cmp_bf(bf_t n1, bf_t n2);
bool is_bf_neg(bf_t n);
bool is_bf_not_zero(bf_t n);
bf_t unsafe_add_bf(bf_t r, bf_t n1, bf_t n2);
bf_t unsafe_add_a_bf(bf_t r, bf_t n);
bf_t unsafe_sub_bf(bf_t r, bf_t n1, bf_t n2);
bf_t unsafe_sub_a_bf(bf_t r, bf_t n);
bf_t neg_bf(bf_t r, bf_t n);
bf_t neg_a_bf(bf_t r);
bf_t double_bf(bf_t r, bf_t n);
bf_t double_a_bf(bf_t r);
bf_t half_bf(bf_t r, bf_t n);
bf_t half_a_bf(bf_t r);
bf_t unsafe_full_mult_bf(bf_t r, bf_t n1, bf_t n2);
bf_t unsafe_mult_bf(bf_t r, bf_t n1, bf_t n2);
bf_t unsafe_full_square_bf(bf_t r, bf_t n);
bf_t unsafe_square_bf(bf_t r, bf_t n);
bf_t unsafe_mult_bf_int(bf_t r, bf_t n, U16 u);
bf_t mult_a_bf_int(bf_t r, U16 u);
bf_t unsafe_div_bf_int(bf_t r, bf_t n,  U16 u);
bf_t div_a_bf_int(bf_t r, U16 u);

enum
{
    MATH_BITS = 32,
    MATH_BYTES = MATH_BITS / 8,
    NUM_VARS = 30, // room for this many on stack
};

// Request the precision needed to distinguish adjacent pixels at the
// maximum resolution of MAX_PIXELS by MAX_PIXELS or at current resolution
enum class ResolutionFlag
{
    MAX = 0,
    CURRENT = 1,
};

// used by other routines
// g_bn_length
extern bn_t g_x_min_bn;
extern bn_t g_x_max_bn;
extern bn_t g_y_min_bn;
extern bn_t g_y_max_bn;
extern bn_t g_x_3rd_bn;
extern bn_t g_y_3rd_bn;
extern bn_t g_delta_x_bn;
extern bn_t g_delta_y_bn;
extern bn_t g_delta2_x_bn;
extern bn_t g_delta2_y_bn;
extern bn_t g_close_enough_bn;
// g_r_length
extern bn_t g_tmp_sqr_x_bn;
extern bn_t g_tmp_sqr_y_bn;
// g_bn_length
extern BNComplex g_old_z_bn;
extern BNComplex g_param_z_bn;
extern BNComplex g_saved_z_bn;
extern BNComplex g_new_z_bn; // g_r_length
// g_r_bf_length+2
extern bf_t g_delta_x_bf;
extern bf_t g_delta_y_bf;
extern bf_t g_delta2_x_bf;
extern bf_t g_delta2_y_bf;
extern bf_t g_close_enough_bf;
extern bf_t g_tmp_sqr_x_bf;
extern bf_t g_tmp_sqr_y_bf;
extern BFComplex g_param_z_bf;
extern BFComplex g_saved_z_bf;
extern BFComplex g_old_z_bf;
extern BFComplex g_new_z_bf;

// for testing only
// used by other routines
// g_bf_length+2
extern bf_t g_bf_x_min;
extern bf_t g_bf_x_max;
extern bf_t g_bf_y_min;
extern bf_t g_bf_y_max;
extern bf_t g_bf_x_3rd;
extern bf_t g_bf_y_3rd;
extern bf_t g_bf_save_x_min;
extern bf_t g_bf_save_x_max;
extern bf_t g_bf_save_y_min;
extern bf_t g_bf_save_y_max;
extern bf_t g_bf_save_x_3rd;
extern bf_t g_bf_save_y_3rd;
extern bf_t g_bf_params[10];                                 // (g_bf_length+2)*10