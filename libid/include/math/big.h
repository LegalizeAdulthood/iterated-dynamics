// SPDX-License-Identifier: GPL-3.0-only
//
// Wesley Loewer's Big Numbers.        (C) 1994, Wesley B. Loewer
#pragma once

#include "math/cmplx.h"
#include "misc/sized_types.h"

#include <config/port.h>

// Number of bytes to use for integer part for fixed decimal math,
// does not affect floating point math at all.
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
    BIG_NUM = 1,         // g_bf_math is being used with BigNum numbers
    BIG_FLT = 2          // g_bf_math is being used with BigFloat numbers
};

using Big = unsigned char *;
using BigNum = Big;  // for clarification purposes
using BigFloat = Big;
using BigFloat10 = Big;

using BFComplex = id::Complex<BigFloat>;
using BNComplex = id::Complex<BigNum>;

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
extern BigNum g_bn_tmp1;      // g_r_length
extern BigNum g_bn_tmp2;      // g_r_length
extern BigNum g_bn_tmp3;      // g_r_length
extern BigNum g_bn_tmp4;      // g_r_length
extern BigNum g_bn_tmp5;      // g_r_length
extern BigNum g_bn_tmp6;      // g_r_length
extern BigNum g_bn_tmp_copy1; // g_bn_length
extern BigNum g_bn_tmp_copy2; // g_bn_length
extern BigNum g_bn_pi;
extern BigNum g_bn_tmp;         // g_r_length
extern BigFloat g_bf_tmp1;      // g_r_bf_length+2
extern BigFloat g_bf_tmp2;      // g_r_bf_length+2
extern BigFloat g_bf_tmp3;      // g_r_bf_length+2
extern BigFloat g_bf_tmp4;      // g_r_bf_length+2
extern BigFloat g_bf_tmp5;      // g_r_bf_length+2
extern BigFloat g_bf_tmp6;      // g_r_bf_length+2
extern BigFloat g_bf_tmp_copy1; // g_r_bf_length+2
extern BigFloat g_bf_tmp_copy2; // g_r_bf_length+2
extern BigFloat g_bf_pi;
extern BigFloat g_bf_tmp;     // g_r_bf_length
extern BigFloat10 g_bf10_tmp; // dec+4
extern Big g_big_pi;

void calc_lengths();
BigNum clear_bn(BigNum r);
BigNum max_bn(BigNum r);
BigNum copy_bn(BigNum r, BigNum n);
int cmp_bn(BigNum n1, BigNum n2);
bool is_bn_neg(BigNum n);
bool is_bn_not_zero(BigNum n);
BigNum add_bn(BigNum r, BigNum n1, BigNum n2);
BigNum add_a_bn(BigNum r, BigNum n);
BigNum sub_bn(BigNum r, BigNum n1, BigNum n2);
BigNum sub_a_bn(BigNum r, BigNum n);
BigNum neg_bn(BigNum r, BigNum n);
BigNum neg_a_bn(BigNum r);
BigNum double_bn(BigNum r, BigNum n);
BigNum double_a_bn(BigNum r);
BigNum half_bn(BigNum r, BigNum n);
BigNum half_a_bn(BigNum r);
BigNum unsafe_full_mult_bn(BigNum r, BigNum n1, BigNum n2);
BigNum unsafe_mult_bn(BigNum r, BigNum n1, BigNum n2);
BigNum unsafe_full_square_bn(BigNum r, BigNum n);
BigNum unsafe_square_bn(BigNum r, BigNum n);
BigNum mult_bn_int(BigNum r, BigNum n, U16 u);
BigNum mult_a_bn_int(BigNum r, U16 u);
BigNum unsafe_div_bn_int(BigNum r, BigNum n, U16 u);
BigNum div_a_bn_int(BigNum r, U16 u);
BigFloat clear_bf(BigFloat r);
BigFloat copy_bf(BigFloat r, BigFloat n);
BigFloat float_to_bf(BigFloat r, LDouble f);
BigFloat float_to_bf1(BigFloat r, LDouble f);
LDouble bf_to_float(BigFloat n);
LDouble bn_to_float(BigNum n);
LDouble extract256(LDouble f, int *exp_ptr);
LDouble scale256(LDouble f, int n);

#ifdef ACCESS_BY_BYTE
// prototypes
U32 BIG_ACCESS32(Byte *addr);
U16 BIG_ACCESS16(Byte *addr);
S16 BIG_ACCESS_S16(S16 *addr);
U32 BIG_SET32(Byte *addr, U32 val);
U16 BIG_SET16(Byte *addr, U16 val);
S16 BIG_SET_S16(S16 *addr, S16 val);
#else
// equivalent defines
#define BIG_ACCESS32(addr)   (*(U32 *)(addr))
#define BIG_ACCESS16(addr)   (*(U16 *)(addr))
#define BIG_ACCESS_S16(addr)   (*(S16 *)(addr))
#define BIG_SET32(addr, val) (*(U32 *)(addr) = (U32)(val))
#define BIG_SET16(addr, val) (*(U16 *)(addr) = (U16)(val))
#define BIG_SET_S16(addr, val) (*(S16 *)(addr) = (S16)(val))
#endif
void bn_hex_dump(BigNum r);
BigNum str_to_bn(BigNum r, char *s);
int strlen_needed_bn();
char *unsafe_bn_to_str(char *s, int dec, BigNum r);
BigNum int_to_bn(BigNum r, long value);
long bn_to_int(BigNum n);
int  sign_bn(BigNum n);
BigNum abs_bn(BigNum r, BigNum n);
BigNum abs_a_bn(BigNum r);
BigNum unsafe_inv_bn(BigNum r, BigNum n);
BigNum unsafe_div_bn(BigNum r, BigNum n1, BigNum n2);
BigNum sqrt_bn(BigNum r, BigNum n);
BigNum exp_bn(BigNum r, BigNum n);
BigNum unsafe_ln_bn(BigNum r, BigNum n);
BigNum unsafe_sin_cos_bn(BigNum s, BigNum c, BigNum n);
BigNum unsafe_atan_bn(BigNum r, BigNum n);
BigNum unsafe_atan2_bn(BigNum r, BigNum ny, BigNum nx);
int convert_bn(BigNum new_num, BigNum old_num, int new_bn_len, int new_int_len, int old_bn_len, int old_int_len);
// "safe" versions
BigNum full_mult_bn(BigNum r, BigNum n1, BigNum n2);
BigNum mult_bn(BigNum r, BigNum n1, BigNum n2);
BigNum full_square_bn(BigNum r, BigNum n);
BigNum square_bn(BigNum r, BigNum n);
BigNum div_bn_int(BigNum r, BigNum n, U16 u);
char *bn_to_str(char *s, int dec, BigNum r);
BigNum inv_bn(BigNum r, BigNum n);
BigNum div_bn(BigNum r, BigNum n1, BigNum n2);
BigNum ln_bn(BigNum r, BigNum n);
BigNum sin_cos_bn(BigNum s, BigNum c, BigNum n);
BigNum atan_bn(BigNum r, BigNum n);
BigNum atan2_bn(BigNum r, BigNum ny, BigNum nx);
// misc
bool is_bn_zero(BigNum n);
BigNum float_to_bn(BigNum r, LDouble f);

void bf_hex_dump(BigFloat r);
BigFloat str_to_bf(BigFloat r, char const *s);
int strlen_needed_bf();
char *unsafe_bf_to_str(char *s, int dec, BigFloat r);
char *unsafe_bf_to_str_e(char *s, int dec, BigFloat r);
char *unsafe_bf_to_str_f(char *s, int dec, BigFloat r);
BigNum bf_to_bn(BigNum n, BigFloat f);
BigNum bn_to_bf(BigFloat f, BigNum n);
long bf_to_int(BigFloat f);
BigFloat int_to_bf(BigFloat r, long value);
int sign_bf(BigFloat n);
BigFloat abs_bf(BigFloat r, BigFloat n);
BigFloat abs_a_bf(BigFloat r);
BigFloat unsafe_inv_bf(BigFloat r, BigFloat n);
BigFloat unsafe_div_bf(BigFloat r, BigFloat n1, BigFloat n2);
BigFloat unsafe_sqrt_bf(BigFloat r, BigFloat n);
BigFloat exp_bf(BigFloat r, BigFloat n);
BigFloat unsafe_ln_bf(BigFloat r, BigFloat n);
BigFloat unsafe_sin_cos_bf(BigFloat s, BigFloat c, BigFloat n);
BigFloat unsafe_atan_bf(BigFloat r, BigFloat n);
BigFloat unsafe_atan2_bf(BigFloat r, BigFloat ny, BigFloat nx);
BigFloat add_bf(BigFloat r, BigFloat n1, BigFloat n2);
BigFloat add_a_bf(BigFloat r, BigFloat n);
BigFloat sub_bf(BigFloat r, BigFloat n1, BigFloat n2);
BigFloat sub_a_bf(BigFloat r, BigFloat n);
BigFloat full_mult_bf(BigFloat r, BigFloat n1, BigFloat n2);
BigFloat mult_bf(BigFloat r, BigFloat n1, BigFloat n2);
BigFloat full_square_bf(BigFloat r, BigFloat n);
BigFloat square_bf(BigFloat r, BigFloat n);
BigFloat mult_bf_int(BigFloat r, BigFloat n, U16 u);
BigFloat div_bf_int(BigFloat r, BigFloat n,  U16 u);
char *bf_to_str(char *s, int dec, BigFloat r);
char *bf_to_str_e(char *s, int dec, BigFloat r);
char *bf_to_str_f(char *s, int dec, BigFloat r);
BigFloat inv_bf(BigFloat r, BigFloat n);
BigFloat div_bf(BigFloat r, BigFloat n1, BigFloat n2);
BigFloat sqrt_bf(BigFloat r, BigFloat n);
BigFloat ln_bf(BigFloat r, BigFloat n);
BigFloat sin_cos_bf(BigFloat s, BigFloat c, BigFloat n);
BigFloat atan_bf(BigFloat r, BigFloat n);
BigFloat atan2_bf(BigFloat r, BigFloat ny, BigFloat nx);
bool is_bf_zero(BigFloat n);
int convert_bf(BigFloat new_num, BigFloat old_num, int new_bf_len, int old_bf_len);
LDouble extract_value(LDouble f, LDouble b, int *exp_ptr);
LDouble scale_value(LDouble f, LDouble b , int n);
LDouble extract_10(LDouble f, int *exp_ptr);
LDouble scale_10(LDouble f, int n);
BigFloat10 unsafe_bf_to_bf10(BigFloat10 r, int dec, BigFloat n);
BigFloat10 mult_a_bf10_int(BigFloat10 r, int dec, U16 n);
BigFloat10 div_a_bf10_int(BigFloat10 r, int dec, U16 n);
char  *bf10_to_str_e(char *s, int dec, BigFloat10 n);
char  *bf10_to_str_f(char *s, int dec, BigFloat10 n);

BigFloat norm_bf(BigFloat r);
void norm_sign_bf(BigFloat r, bool positive);
S16 adjust_bf_add(BigFloat n1, BigFloat n2);
BigFloat max_bf(BigFloat r);
int cmp_bf(BigFloat n1, BigFloat n2);
bool is_bf_neg(BigFloat n);
bool is_bf_not_zero(BigFloat n);
BigFloat unsafe_add_bf(BigFloat r, BigFloat n1, BigFloat n2);
BigFloat unsafe_add_a_bf(BigFloat r, BigFloat n);
BigFloat unsafe_sub_bf(BigFloat r, BigFloat n1, BigFloat n2);
BigFloat unsafe_sub_a_bf(BigFloat r, BigFloat n);
BigFloat neg_bf(BigFloat r, BigFloat n);
BigFloat neg_a_bf(BigFloat r);
BigFloat double_bf(BigFloat r, BigFloat n);
BigFloat double_a_bf(BigFloat r);
BigFloat half_bf(BigFloat r, BigFloat n);
BigFloat half_a_bf(BigFloat r);
BigFloat unsafe_full_mult_bf(BigFloat r, BigFloat n1, BigFloat n2);
BigFloat unsafe_mult_bf(BigFloat r, BigFloat n1, BigFloat n2);
BigFloat unsafe_full_square_bf(BigFloat r, BigFloat n);
BigFloat unsafe_square_bf(BigFloat r, BigFloat n);
BigFloat unsafe_mult_bf_int(BigFloat r, BigFloat n, U16 u);
BigFloat mult_a_bf_int(BigFloat r, U16 u);
BigFloat unsafe_div_bf_int(BigFloat r, BigFloat n,  U16 u);
BigFloat div_a_bf_int(BigFloat r, U16 u);

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
extern BigNum g_x_min_bn;
extern BigNum g_x_max_bn;
extern BigNum g_y_min_bn;
extern BigNum g_y_max_bn;
extern BigNum g_x_3rd_bn;
extern BigNum g_y_3rd_bn;
extern BigNum g_delta_x_bn;
extern BigNum g_delta_y_bn;
extern BigNum g_delta2_x_bn;
extern BigNum g_delta2_y_bn;
extern BigNum g_close_enough_bn;
// g_r_length
extern BigNum g_tmp_sqr_x_bn;
extern BigNum g_tmp_sqr_y_bn;
// g_bn_length
extern BNComplex g_old_z_bn;
extern BNComplex g_param_z_bn;
extern BNComplex g_saved_z_bn;
extern BNComplex g_new_z_bn; // g_r_length
// g_r_bf_length+2
extern BigFloat g_delta_x_bf;
extern BigFloat g_delta_y_bf;
extern BigFloat g_delta2_x_bf;
extern BigFloat g_delta2_y_bf;
extern BigFloat g_close_enough_bf;
extern BigFloat g_tmp_sqr_x_bf;
extern BigFloat g_tmp_sqr_y_bf;
extern BFComplex g_param_z_bf;
extern BFComplex g_saved_z_bf;
extern BFComplex g_old_z_bf;
extern BFComplex g_new_z_bf;

// for testing only
// used by other routines
// g_bf_length+2
extern BigFloat g_bf_x_min;
extern BigFloat g_bf_x_max;
extern BigFloat g_bf_y_min;
extern BigFloat g_bf_y_max;
extern BigFloat g_bf_x_3rd;
extern BigFloat g_bf_y_3rd;
extern BigFloat g_bf_save_x_min;
extern BigFloat g_bf_save_x_max;
extern BigFloat g_bf_save_y_min;
extern BigFloat g_bf_save_y_max;
extern BigFloat g_bf_save_x_3rd;
extern BigFloat g_bf_save_y_3rd;
extern BigFloat g_bf_params[10];                                 // (g_bf_length+2)*10
