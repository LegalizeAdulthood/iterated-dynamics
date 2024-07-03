// Wesley Loewer's Big Numbers.        (C) 1994, Wesley B. Loewer
#pragma once

#include "cmplx.h"

// Number of bytes to use for integer part for fixed decimal math,
// does not effect floating point math at all.
#define BN_INT_LENGTH 4
/* #define CALCULATING_BIG_PI */ /* define for generating big_pi[] table */
/****************************************************************
 The rest is handled by the compiler
****************************************************************/
#define LOG10_256 2.4082399653118
#define LOG_256   5.5451774444795
// values that bf_math can hold,
// 0 = bf_math is not being used
// 1 = bf_math is being used
enum class bf_math_type
{
    // cppcheck-suppress variableHidingEnum
    NONE = 0,
    BIGNUM = 1,         // bf_math is being used with bn_t numbers
    BIGFLT = 2          // bf_math is being used with bf_t numbers
};

using big_t = unsigned char *;
using bn_t = big_t;  // for clarification purposes
using bf_t = big_t;
using bf10_t = big_t;

using BFComplex = id::Complex<bf_t>;
using BNComplex = id::Complex<bn_t>;

// globals
extern bf_math_type bf_math;
extern int g_bn_step;
extern int intlength;
extern int bnlength;
extern int rlength;
extern int padding;
extern int g_decimals;
extern int shiftfactor;
extern int bflength;
extern int rbflength;
extern int bfpadding;
extern int bfdecimals;
extern bn_t bntmp1;    // rlength
extern bn_t bntmp2;    // rlength
extern bn_t bntmp3;    // rlength
extern bn_t bntmp4;    // rlength
extern bn_t bntmp5;    // rlength
extern bn_t bntmp6;    // rlength
extern bn_t bntest1;   // rlength
extern bn_t bntest2;   // rlength
extern bn_t bntest3;   // rlength
extern bn_t bntmpcpy1; // bnlength
extern bn_t bntmpcpy2; // bnlength
extern bn_t bn_pi;
extern bn_t bntmp;     // rlength
extern bf_t bftmp1;    // rbflength+2
extern bf_t bftmp2;    // rbflength+2
extern bf_t bftmp3;    // rbflength+2
extern bf_t bftmp4;    // rbflength+2
extern bf_t bftmp5;    // rbflength+2
extern bf_t bftmp6;    // rbflength+2
extern bf_t bftest1;   // rbflength+2
extern bf_t bftest2;   // rbflength+2
extern bf_t bftest3;   // rbflength+2
extern bf_t bftmpcpy1; // rbflength+2
extern bf_t bftmpcpy2; // rbflength+2
extern bf_t bf_pi;
extern bf_t bftmp;     // rbflength
extern bf10_t bf10tmp; // dec+4
extern big_t big_pi;

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
bf_t floattobf(bf_t r, LDBL f);
LDBL bftofloat(bf_t n);
LDBL bntofloat(bn_t n);
LDBL extract_256(LDBL f, int *exp_ptr);
LDBL scale_256(LDBL f, int n);
// functions defined in bignum.c
#ifdef ACCESS_BY_BYTE
// prototypes
U32 big_access32(BYTE *addr);
U16 big_access16(BYTE *addr);
S16 big_accessS16(S16 *addr);
U32 big_set32(BYTE *addr, U32 val);
U16 big_set16(BYTE *addr, U16 val);
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
void bn_hexdump(bn_t r);
bn_t strtobn(bn_t r, char *s);
char *unsafe_bntostr(char *s, int dec, bn_t r);
bn_t inttobn(bn_t r, long longval);
long bntoint(bn_t n);
int  sign_bn(bn_t n);
bn_t abs_bn(bn_t r, bn_t n);
bn_t abs_a_bn(bn_t r);
bn_t unsafe_inv_bn(bn_t r, bn_t n);
bn_t unsafe_div_bn(bn_t r, bn_t n1, bn_t n2);
bn_t sqrt_bn(bn_t r, bn_t n);
bn_t exp_bn(bn_t r, bn_t n);
bn_t unsafe_ln_bn(bn_t r, bn_t n);
bn_t unsafe_sincos_bn(bn_t s, bn_t c, bn_t n);
bn_t unsafe_atan_bn(bn_t r, bn_t n);
bn_t unsafe_atan2_bn(bn_t r, bn_t ny, bn_t nx);
int convert_bn(bn_t new_n, bn_t old, int newbnlength, int newintlength, int oldbnlength, int oldintlength);
// "safe" versions
bn_t full_mult_bn(bn_t r, bn_t n1, bn_t n2);
bn_t mult_bn(bn_t r, bn_t n1, bn_t n2);
bn_t full_square_bn(bn_t r, bn_t n);
bn_t square_bn(bn_t r, bn_t n);
bn_t div_bn_int(bn_t r, bn_t n, U16 u);
char *bntostr(char *s, int dec, bn_t r);
bn_t inv_bn(bn_t r, bn_t n);
bn_t div_bn(bn_t r, bn_t n1, bn_t n2);
bn_t ln_bn(bn_t r, bn_t n);
bn_t sincos_bn(bn_t s, bn_t c, bn_t n);
bn_t atan_bn(bn_t r, bn_t n);
bn_t atan2_bn(bn_t r, bn_t ny, bn_t nx);
// misc
bool is_bn_zero(bn_t n);
bn_t floattobn(bn_t r, LDBL f);
/************/
// bigflt.c
void bf_hexdump(bf_t r);
bf_t strtobf(bf_t r, char const *s);
int strlen_needed_bf();
char *unsafe_bftostr(char *s, int dec, bf_t r);
char *unsafe_bftostr_e(char *s, int dec, bf_t r);
char *unsafe_bftostr_f(char *s, int dec, bf_t r);
bn_t bftobn(bn_t n, bf_t f);
bn_t bntobf(bf_t f, bn_t n);
long bftoint(bf_t f);
bf_t inttobf(bf_t r, long longval);
int sign_bf(bf_t n);
bf_t abs_bf(bf_t r, bf_t n);
bf_t abs_a_bf(bf_t r);
bf_t unsafe_inv_bf(bf_t r, bf_t n);
bf_t unsafe_div_bf(bf_t r, bf_t n1, bf_t n2);
bf_t unsafe_sqrt_bf(bf_t r, bf_t n);
bf_t exp_bf(bf_t r, bf_t n);
bf_t unsafe_ln_bf(bf_t r, bf_t n);
bf_t unsafe_sincos_bf(bf_t s, bf_t c, bf_t n);
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
char *bftostr(char *s, int dec, bf_t r);
char *bftostr_e(char *s, int dec, bf_t r);
char *bftostr_f(char *s, int dec, bf_t r);
bf_t inv_bf(bf_t r, bf_t n);
bf_t div_bf(bf_t r, bf_t n1, bf_t n2);
bf_t sqrt_bf(bf_t r, bf_t n);
bf_t ln_bf(bf_t r, bf_t n);
bf_t sincos_bf(bf_t s, bf_t c, bf_t n);
bf_t atan_bf(bf_t r, bf_t n);
bf_t atan2_bf(bf_t r, bf_t ny, bf_t nx);
bool is_bf_zero(bf_t n);
int convert_bf(bf_t new_n, bf_t old, int newbflength, int oldbflength);
LDBL extract_value(LDBL f, LDBL b, int *exp_ptr);
LDBL scale_value(LDBL f, LDBL b , int n);
LDBL extract_10(LDBL f, int *exp_ptr);
LDBL scale_10(LDBL f, int n);
bf10_t unsafe_bftobf10(bf10_t s, int dec, bf_t n);
bf10_t mult_a_bf10_int(bf10_t s, int dec, U16 n);
bf10_t div_a_bf10_int(bf10_t s, int dec, U16 n);
char  *bf10tostr_e(char *s, int dec, bf10_t n);
char  *bf10tostr_f(char *s, int dec, bf10_t n);
// functions defined in bigfltc.c
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

#define MATHBITS      32
#define MATHBYTES     (MATHBITS/8)
#define NUMVARS       30            // room for this many on stack
#define CURRENTREZ    1
#define MAXREZ        0

// used by other routines
// bnlength
extern bn_t bnxmin;
extern bn_t bnxmax;
extern bn_t bnymin;
extern bn_t bnymax;
extern bn_t bnx3rd;
extern bn_t bny3rd;
extern bn_t bnxdel;
extern bn_t bnydel;
extern bn_t bnxdel2;
extern bn_t bnydel2;
extern bn_t bnclosenuff;
// rlength
extern bn_t bntmpsqrx;
extern bn_t bntmpsqry;
// bnlength
extern BNComplex bnold;
extern BNComplex bnparm;
extern BNComplex bnsaved;
extern BNComplex bnnew;                                         // rlength
// rbflength+2
extern bf_t bfxdel;
extern bf_t bfydel;
extern bf_t bfxdel2;
extern bf_t bfydel2;
extern bf_t bfclosenuff;
extern bf_t bftmpsqrx;
extern bf_t bftmpsqry;
extern BFComplex bfparm;
extern BFComplex bfsaved;
extern BFComplex bfold;
extern BFComplex bfnew;

// for testing only
// used by other routines
// bflength+2
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
extern bf_t bfparms[10];                                 // (bflength+2)*10
