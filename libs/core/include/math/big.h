// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
// Wesley Loewer's Big Numbers.        (C) 1994, Wesley B. Loewer
#pragma once

#include <math/cmplx.h>
#include <misc/sized_types.h>

#include <config/port.h>

#include <string>
#include <string_view>

namespace id::math
{

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
using BigNumPtr = Big; // for clarification purposes
using BigFloatPtr = Big;

using BFComplex = Complex<BigFloatPtr>;
using BNComplex = Complex<BigNumPtr>;

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
extern BigNumPtr g_bn_tmp1;        // g_r_length
extern BigNumPtr g_bn_tmp2;        // g_r_length
extern BigNumPtr g_bn_tmp3;        // g_r_length
extern BigNumPtr g_bn_tmp4;        // g_r_length
extern BigNumPtr g_bn_tmp5;        // g_r_length
extern BigNumPtr g_bn_tmp6;        // g_r_length
extern BigNumPtr g_bn_tmp_copy1;   // g_bn_length
extern BigNumPtr g_bn_tmp_copy2;   // g_bn_length
extern BigNumPtr g_bn_pi;
extern BigNumPtr g_bn_tmp;         // g_r_length
extern BigFloatPtr g_bf_tmp1;      // g_r_bf_length+2
extern BigFloatPtr g_bf_tmp2;      // g_r_bf_length+2
extern BigFloatPtr g_bf_tmp3;      // g_r_bf_length+2
extern BigFloatPtr g_bf_tmp4;      // g_r_bf_length+2
extern BigFloatPtr g_bf_tmp5;      // g_r_bf_length+2
extern BigFloatPtr g_bf_tmp6;      // g_r_bf_length+2
extern BigFloatPtr g_bf_tmp_copy1; // g_r_bf_length+2
extern BigFloatPtr g_bf_tmp_copy2; // g_r_bf_length+2
extern BigFloatPtr g_bf_pi;
extern BigFloatPtr g_bf_tmp;       // g_r_bf_length
extern Big g_big_pi;

void calc_lengths();
BigNumPtr clear_bn(BigNumPtr r);
BigNumPtr max_bn(BigNumPtr r);
BigNumPtr copy_bn(BigNumPtr r, BigNumPtr n);
int cmp_bn(BigNumPtr n1, BigNumPtr n2);
bool is_bn_neg(BigNumPtr n);
bool is_bn_not_zero(BigNumPtr n);
BigNumPtr add_bn(BigNumPtr r, BigNumPtr n1, BigNumPtr n2);
BigNumPtr add_a_bn(BigNumPtr r, BigNumPtr n);
BigNumPtr sub_bn(BigNumPtr r, BigNumPtr n1, BigNumPtr n2);
BigNumPtr sub_a_bn(BigNumPtr r, BigNumPtr n);
BigNumPtr neg_bn(BigNumPtr r, BigNumPtr n);
BigNumPtr neg_a_bn(BigNumPtr r);
BigNumPtr double_bn(BigNumPtr r, BigNumPtr n);
BigNumPtr double_a_bn(BigNumPtr r);
BigNumPtr half_bn(BigNumPtr r, BigNumPtr n);
BigNumPtr half_a_bn(BigNumPtr r);
BigNumPtr unsafe_full_mult_bn(BigNumPtr r, BigNumPtr n1, BigNumPtr n2);
BigNumPtr unsafe_mult_bn(BigNumPtr r, BigNumPtr n1, BigNumPtr n2);
BigNumPtr unsafe_full_square_bn(BigNumPtr r, BigNumPtr n);
BigNumPtr unsafe_square_bn(BigNumPtr r, BigNumPtr n);
BigNumPtr mult_bn_int(BigNumPtr r, BigNumPtr n, misc::U16 u);
BigNumPtr mult_a_bn_int(BigNumPtr r, misc::U16 u);
BigNumPtr unsafe_div_bn_int(BigNumPtr r, BigNumPtr n, misc::U16 u);
BigNumPtr div_a_bn_int(BigNumPtr r, misc::U16 u);
BigFloatPtr clear_bf(BigFloatPtr r);
BigFloatPtr copy_bf(BigFloatPtr r, BigFloatPtr n);
BigFloatPtr float_to_bf(BigFloatPtr r, LDouble f);
BigFloatPtr float_to_bf1(BigFloatPtr r, LDouble f);
LDouble bf_to_float(BigFloatPtr n);
LDouble bn_to_float(BigNumPtr n);
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
void bn_hex_dump(BigNumPtr r);
BigNumPtr str_to_bn(BigNumPtr r, std::string_view s);
int strlen_needed_bn();
BigNumPtr int_to_bn(BigNumPtr r, long value);
long bn_to_int(BigNumPtr n);
int sign_bn(BigNumPtr n);
BigNumPtr abs_bn(BigNumPtr r, BigNumPtr n);
BigNumPtr abs_a_bn(BigNumPtr r);
BigNumPtr unsafe_inv_bn(BigNumPtr r, BigNumPtr n);
BigNumPtr unsafe_div_bn(BigNumPtr r, BigNumPtr n1, BigNumPtr n2);
BigNumPtr sqrt_bn(BigNumPtr r, BigNumPtr n);
BigNumPtr exp_bn(BigNumPtr r, BigNumPtr n);
BigNumPtr unsafe_ln_bn(BigNumPtr r, BigNumPtr n);
BigNumPtr unsafe_sin_cos_bn(BigNumPtr s, BigNumPtr c, BigNumPtr n);
BigNumPtr unsafe_atan_bn(BigNumPtr r, BigNumPtr n);
BigNumPtr unsafe_atan2_bn(BigNumPtr r, BigNumPtr ny, BigNumPtr nx);
int convert_bn(BigNumPtr new_num, BigNumPtr old_num, int new_bn_len, int new_int_len, int old_bn_len, int old_int_len);
// "safe" versions
BigNumPtr full_mult_bn(BigNumPtr r, BigNumPtr n1, BigNumPtr n2);
BigNumPtr mult_bn(BigNumPtr r, BigNumPtr n1, BigNumPtr n2);
BigNumPtr full_square_bn(BigNumPtr r, BigNumPtr n);
BigNumPtr square_bn(BigNumPtr r, BigNumPtr n);
BigNumPtr div_bn_int(BigNumPtr r, BigNumPtr n, misc::U16 u);
std::string bn_to_string(BigNumPtr r, int dec);
BigNumPtr inv_bn(BigNumPtr r, BigNumPtr n);
BigNumPtr div_bn(BigNumPtr r, BigNumPtr n1, BigNumPtr n2);
BigNumPtr ln_bn(BigNumPtr r, BigNumPtr n);
BigNumPtr sin_cos_bn(BigNumPtr s, BigNumPtr c, BigNumPtr n);
BigNumPtr atan_bn(BigNumPtr r, BigNumPtr n);
BigNumPtr atan2_bn(BigNumPtr r, BigNumPtr ny, BigNumPtr nx);
// misc
bool is_bn_zero(BigNumPtr n);
BigNumPtr float_to_bn(BigNumPtr r, LDouble f);

void bf_hex_dump(BigFloatPtr r);
BigFloatPtr str_to_bf(BigFloatPtr r, std::string_view s);
int strlen_needed_bf();
BigNumPtr bf_to_bn(BigNumPtr n, BigFloatPtr f);
BigNumPtr bn_to_bf(BigFloatPtr f, BigNumPtr n);
long bf_to_int(BigFloatPtr f);
BigFloatPtr int_to_bf(BigFloatPtr r, long value);
int sign_bf(BigFloatPtr n);
BigFloatPtr abs_bf(BigFloatPtr r, BigFloatPtr n);
BigFloatPtr abs_a_bf(BigFloatPtr r);
BigFloatPtr unsafe_inv_bf(BigFloatPtr r, BigFloatPtr n);
BigFloatPtr unsafe_div_bf(BigFloatPtr r, BigFloatPtr n1, BigFloatPtr n2);
BigFloatPtr unsafe_sqrt_bf(BigFloatPtr r, BigFloatPtr n);
BigFloatPtr exp_bf(BigFloatPtr r, BigFloatPtr n);
BigFloatPtr unsafe_ln_bf(BigFloatPtr r, BigFloatPtr n);
BigFloatPtr unsafe_sin_cos_bf(BigFloatPtr s, BigFloatPtr c, BigFloatPtr n);
BigFloatPtr unsafe_atan_bf(BigFloatPtr r, BigFloatPtr n);
BigFloatPtr unsafe_atan2_bf(BigFloatPtr r, BigFloatPtr ny, BigFloatPtr nx);
BigFloatPtr add_bf(BigFloatPtr r, BigFloatPtr n1, BigFloatPtr n2);
BigFloatPtr add_a_bf(BigFloatPtr r, BigFloatPtr n);
BigFloatPtr sub_bf(BigFloatPtr r, BigFloatPtr n1, BigFloatPtr n2);
BigFloatPtr sub_a_bf(BigFloatPtr r, BigFloatPtr n);
BigFloatPtr full_mult_bf(BigFloatPtr r, BigFloatPtr n1, BigFloatPtr n2);
BigFloatPtr mult_bf(BigFloatPtr r, BigFloatPtr n1, BigFloatPtr n2);
BigFloatPtr full_square_bf(BigFloatPtr r, BigFloatPtr n);
BigFloatPtr square_bf(BigFloatPtr r, BigFloatPtr n);
BigFloatPtr mult_bf_int(BigFloatPtr r, BigFloatPtr n, misc::U16 u);
BigFloatPtr div_bf_int(BigFloatPtr r, BigFloatPtr n, misc::U16 u);
std::string bf_to_string(BigFloatPtr r, int dec);
std::string bf_to_string_e(BigFloatPtr r, int dec);
std::string bf_to_string_f(BigFloatPtr r, int dec);
BigFloatPtr inv_bf(BigFloatPtr r, BigFloatPtr n);
BigFloatPtr div_bf(BigFloatPtr r, BigFloatPtr n1, BigFloatPtr n2);
BigFloatPtr sqrt_bf(BigFloatPtr r, BigFloatPtr n);
BigFloatPtr ln_bf(BigFloatPtr r, BigFloatPtr n);
BigFloatPtr sin_cos_bf(BigFloatPtr s, BigFloatPtr c, BigFloatPtr n);
BigFloatPtr atan_bf(BigFloatPtr r, BigFloatPtr n);
BigFloatPtr atan2_bf(BigFloatPtr r, BigFloatPtr ny, BigFloatPtr nx);
bool is_bf_zero(BigFloatPtr n);
int convert_bf(BigFloatPtr new_num, BigFloatPtr old_num, int new_bf_len, int old_bf_len);
LDouble extract_value(LDouble f, LDouble b, int *exp_ptr);
LDouble scale_value(LDouble f, LDouble b , int n);
LDouble extract_10(LDouble f, int *exp_ptr);
LDouble scale_10(LDouble f, int n);

BigFloatPtr norm_bf(BigFloatPtr r);
void norm_sign_bf(BigFloatPtr r, bool positive);
misc::S16 adjust_bf_add(BigFloatPtr n1, BigFloatPtr n2);
BigFloatPtr max_bf(BigFloatPtr r);
int cmp_bf(BigFloatPtr n1, BigFloatPtr n2);
bool is_bf_neg(BigFloatPtr n);
bool is_bf_not_zero(BigFloatPtr n);
BigFloatPtr unsafe_add_bf(BigFloatPtr r, BigFloatPtr n1, BigFloatPtr n2);
BigFloatPtr unsafe_add_a_bf(BigFloatPtr r, BigFloatPtr n);
BigFloatPtr unsafe_sub_bf(BigFloatPtr r, BigFloatPtr n1, BigFloatPtr n2);
BigFloatPtr unsafe_sub_a_bf(BigFloatPtr r, BigFloatPtr n);
BigFloatPtr neg_bf(BigFloatPtr r, BigFloatPtr n);
BigFloatPtr neg_a_bf(BigFloatPtr r);
BigFloatPtr double_bf(BigFloatPtr r, BigFloatPtr n);
BigFloatPtr double_a_bf(BigFloatPtr r);
BigFloatPtr half_bf(BigFloatPtr r, BigFloatPtr n);
BigFloatPtr half_a_bf(BigFloatPtr r);
BigFloatPtr unsafe_full_mult_bf(BigFloatPtr r, BigFloatPtr n1, BigFloatPtr n2);
BigFloatPtr unsafe_mult_bf(BigFloatPtr r, BigFloatPtr n1, BigFloatPtr n2);
BigFloatPtr unsafe_full_square_bf(BigFloatPtr r, BigFloatPtr n);
BigFloatPtr unsafe_square_bf(BigFloatPtr r, BigFloatPtr n);
BigFloatPtr unsafe_mult_bf_int(BigFloatPtr r, BigFloatPtr n, misc::U16 u);
BigFloatPtr mult_a_bf_int(BigFloatPtr r, misc::U16 u);
BigFloatPtr unsafe_div_bf_int(BigFloatPtr r, BigFloatPtr n, misc::U16 u);
BigFloatPtr div_a_bf_int(BigFloatPtr r, misc::U16 u);

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
extern BigNumPtr g_x_min_bn;
extern BigNumPtr g_x_max_bn;
extern BigNumPtr g_y_min_bn;
extern BigNumPtr g_y_max_bn;
extern BigNumPtr g_x_3rd_bn;
extern BigNumPtr g_y_3rd_bn;
extern BigNumPtr g_delta_x_bn;
extern BigNumPtr g_delta_y_bn;
extern BigNumPtr g_delta2_x_bn;
extern BigNumPtr g_delta2_y_bn;
extern BigNumPtr g_close_enough_bn;
// g_r_length
extern BigNumPtr g_tmp_sqr_x_bn;
extern BigNumPtr g_tmp_sqr_y_bn;
// g_bn_length
extern BNComplex g_old_z_bn;
extern BNComplex g_param_z_bn;
extern BNComplex g_saved_z_bn;
extern BNComplex g_new_z_bn; // g_r_length
// g_r_bf_length+2
extern BigFloatPtr g_delta_x_bf;
extern BigFloatPtr g_delta_y_bf;
extern BigFloatPtr g_delta2_x_bf;
extern BigFloatPtr g_delta2_y_bf;
extern BigFloatPtr g_close_enough_bf;
extern BigFloatPtr g_tmp_sqr_x_bf;
extern BigFloatPtr g_tmp_sqr_y_bf;
extern BFComplex g_param_z_bf;
extern BFComplex g_saved_z_bf;
extern BFComplex g_old_z_bf;
extern BFComplex g_new_z_bf;

// for testing only
// used by other routines
// g_bf_length+2
extern BigFloatPtr g_bf_x_min;
extern BigFloatPtr g_bf_x_max;
extern BigFloatPtr g_bf_y_min;
extern BigFloatPtr g_bf_y_max;
extern BigFloatPtr g_bf_x_3rd;
extern BigFloatPtr g_bf_y_3rd;
extern BigFloatPtr g_bf_save_x_min;
extern BigFloatPtr g_bf_save_x_max;
extern BigFloatPtr g_bf_save_y_min;
extern BigFloatPtr g_bf_save_y_max;
extern BigFloatPtr g_bf_save_x_3rd;
extern BigFloatPtr g_bf_save_y_3rd;
extern BigFloatPtr g_bf_params[10]; // (g_bf_length+2)*10

} // namespace id::math
