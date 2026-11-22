// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
// Wesley Loewer's Big Numbers.        (C) 1994, Wesley B. Loewer
#pragma once

#include <math/cmplx.h>
#include <misc/sized_types.h>

#include <config/port.h>

#include <cstddef>
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

/* #define CALCULATING_BIG_PI */ /* define for generating the Pi table */
/****************************************************************
 The rest is handled by the compiler
****************************************************************/
#define LOG10_256 2.4082399653118
#define LOG_256   5.5451774444795
using Big = unsigned char *;
using BFPtrComplex = Complex<Byte *>;
using BFConstPtrComplex = Complex<const Byte *>;

// Precision lengths shared with the initialization layer.
extern int g_int_length;
extern int g_bn_length;
extern int g_r_length;
extern int g_padding;
extern int g_decimals;
extern int g_shift_factor;
extern int g_bf_length;
extern int g_r_bf_length;
void calc_lengths();

Byte *clear_bn(Byte *r);
Byte *max_bn(Byte *r);
Byte *copy_bn(Byte *r, const Byte *n);
bool is_bn_neg(const Byte *n);
Byte *add_bn(Byte *r, const Byte *n1, const Byte *n2);
Byte *add_a_bn(Byte *r, const Byte *n);
Byte *sub_bn(Byte *r, const Byte *n1, const Byte *n2);
Byte *sub_a_bn(Byte *r, const Byte *n);
Byte *neg_bn(Byte *r, const Byte *n);
Byte *neg_a_bn(Byte *r);
Byte *double_bn(Byte *r, const Byte *n);
Byte *double_a_bn(Byte *r);
Byte *half_bn(Byte *r, const Byte *n);
Byte *half_a_bn(Byte *r);
Byte *unsafe_mult_bn(Byte *r, Byte *n1, Byte *n2);
Byte *unsafe_square_bn(Byte *r, Byte *n);
Byte *mult_bn_int(Byte *r, const Byte *n, misc::U16 u);
Byte *mult_a_bn_int(Byte *r, misc::U16 u);
Byte *unsafe_div_bn_int(Byte *r, Byte *n, misc::U16 u);
Byte *div_a_bn_int(Byte *r, misc::U16 u);
Byte *clear_bf(Byte *r);
Byte *copy_bf(Byte *r, const Byte *n);
void reduce_bf_product(Byte *value);
void copy_bf_pi(Byte *destination);
Byte *float_to_bf(Byte *r, LDouble f);
LDouble bf_to_float(Byte *n);
LDouble bn_to_float(Byte *n);
LDouble extract256(LDouble f, int *exp_ptr);
LDouble scale256(LDouble f, int n);

#ifdef ACCESS_BY_BYTE
// prototypes
misc::U32 BIG_ACCESS32(const Byte *addr);
misc::U16 BIG_ACCESS16(const Byte *addr);
misc::U32 BIG_SET32(Byte *addr, misc::U32 val);
misc::U16 BIG_SET16(Byte *addr, misc::U16 val);
#else
// equivalent defines
#define BIG_ACCESS32(addr)   (*(U32 *)(addr))
#define BIG_ACCESS16(addr)   (*(U16 *)(addr))
#define BIG_SET32(addr, val) (*(U32 *)(addr) = (U32)(val))
#define BIG_SET16(addr, val) (*(U16 *)(addr) = (U16)(val))
#endif
Byte *binary_operation(Byte *result, const Byte *left, const Byte *right, Byte *copy1, Byte *copy2,
                       Byte *(*operation)(Byte *, Byte *, Byte *) );

// "safe" versions
// misc
Byte *float_to_bn(Byte *r, LDouble f);

void bf_hex_dump_snapshot(const Byte *r);
Byte *str_to_bf(Byte *r, std::string_view s, Byte *tmp1);
Byte *int_to_bf(Byte *r, long value);
int sign_bf(const Byte *n);
Byte *abs_bf(Byte *r, const Byte *n);
Byte *abs_a_bf(Byte *r);
Byte *unsafe_inv_bf(Byte *r, Byte *n, Byte *tmp1, Byte *tmp2);
Byte *unsafe_div_bf(Byte *r, Byte *n1, Byte *n2, Byte *tmp1, Byte *tmp2);
Byte *exp_bf(Byte *r, const Byte *n, Byte *tmp1, Byte *tmp2, Byte *tmp3);
Byte *unsafe_ln_bf(Byte *r, Byte *n, Byte *tmp1, Byte *tmp2, Byte *tmp3, Byte *tmp4, Byte *tmp5, Byte *tmp6);
Byte *unsafe_sin_cos_bf(Byte *s, Byte *c, Byte *n, const Byte *pi, Byte *tmp1, Byte *tmp2);
Byte *arctangent(
    Byte *r, Byte *n, const Byte *pi, Byte *tmp1, Byte *tmp2, Byte *tmp3, Byte *tmp4, Byte *tmp5, Byte *copy1);
Byte *unsafe_atan2_bf(Byte *r, Byte *ny, Byte *nx, const Byte *pi, Byte *tmp1, Byte *tmp2, Byte *tmp3, Byte *tmp4,
    Byte *tmp5, Byte *tmp6, Byte *copy1, Byte *copy2);
Byte *unsafe_bf_to_bf10(Byte *r, int dec, Byte *n);
std::string unsafe_bf_to_string(Byte *r, int dec, Byte *decimal);
std::string unsafe_bf_to_string_e(Byte *r, int dec, Byte *decimal);
std::string unsafe_bf_to_string_f(Byte *r, int dec, Byte *decimal);
Byte *mult_a_bf10_int(Byte *r, int dec, misc::U16 n);
Byte *div_a_bf10_int(Byte *r, int dec, misc::U16 n);
std::string bf10_to_string_e(const Byte *n, int dec);
std::string bf10_to_string_f(const Byte *n, int dec);
bool is_bf_zero(const Byte *n);
int convert_bf(Byte *new_num, const Byte *old_num, int new_bf_len, int old_bf_len);
// Decode a stored mantissa and exponent; return the next packed value.
const Byte *unpack_bf(Byte *destination, const Byte *source, int stored_length);
void migrate_bf(Byte *destination, const Byte *source, std::size_t blocks, int new_bf_length, int new_padding,
    int old_bf_length, int old_padding);
LDouble extract_value(LDouble f, LDouble b, int *exp_ptr);
LDouble scale_value(LDouble f, LDouble b , int n);
LDouble extract_10(LDouble f, int *exp_ptr);
LDouble scale_10(LDouble f, int n);

Byte *norm_bf(Byte *r);
void norm_sign_bf(Byte *r, bool positive);
Byte *max_bf(Byte *r);
int cmp_bf(const Byte *n1, const Byte *n2);
bool is_bf_neg(const Byte *n);
bool is_bf_not_zero(const Byte *n);
Byte *unsafe_add_bf(Byte *r, Byte *n1, Byte *n2);
Byte *unsafe_add_a_bf(Byte *r, Byte *n);
Byte *unsafe_sub_bf(Byte *r, Byte *n1, Byte *n2);
Byte *unsafe_sub_a_bf(Byte *r, Byte *n);
Byte *neg_bf(Byte *r, const Byte *n);
Byte *neg_a_bf(Byte *r);
Byte *double_bf(Byte *r, const Byte *n);
Byte *double_a_bf(Byte *r);
Byte *half_bf(Byte *r, const Byte *n);
Byte *half_a_bf(Byte *r);
// Wide product: g_r_bf_length mantissa bytes followed by the exponent.
Byte *multiply_wide(Byte *r, Byte *n1, Byte *n2);
Byte *square_wide(Byte *r, Byte *n);
// Canonical product bridges implemented in the value layer.
Byte *unsafe_mult_bf(Byte *r, Byte *n1, Byte *n2);
Byte *unsafe_square_bf(Byte *r, Byte *n);
Byte *unsafe_mult_bf_int(Byte *r, Byte *n, misc::U16 u);
Byte *mult_a_bf_int(Byte *r, misc::U16 u);
Byte *unsafe_div_bf_int(Byte *r, Byte *n, misc::U16 u);
Byte *div_a_bf_int(Byte *r, misc::U16 u);

enum
{
    MATH_BITS = 32,
    MATH_BYTES = MATH_BITS / 8,
};

} // namespace id::math
