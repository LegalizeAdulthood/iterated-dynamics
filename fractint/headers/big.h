#ifndef _BIG_H
#define _BIG_H

/* big.h */

/* Wesley Loewer's Big Numbers.        (C) 1994, Wesley B. Loewer */

/* Number of bytes to use for integer part for fixed decimal math, */
/* does not effect floating point math at all. */
enum
{
	BN_INT_LENGTH = 4
};

/****************************************************************
 The rest is handled by the compiler
****************************************************************/

extern double const LOG10_256;
extern double const LOG_256;

/* values that g_bf_math can hold, */
/* 0 = g_bf_math is not being used */
/* 1 = g_bf_math is being used     */
enum
{
	BIGNUM = 1,  /* g_bf_math is being used with bn_t numbers */
	BIGFLT = 2  /* g_bf_math is being used with bf_t numbers */
};

typedef unsigned char *big_t;
#define bn_t   big_t  /* for clarification purposes */
#define bf_t   big_t
#define bf10_t big_t

#include "cmplx.h"

typedef ComplexT<bn_t> ComplexBigFloat;
typedef ComplexT<bn_t> ComplexBigNum;

extern int g_bf_math;

extern int g_step_bn;
extern int g_int_length;
extern int g_bn_length;
extern int g_r_length;
extern int g_padding;
extern int g_decimals;
extern int g_shift_factor;
extern int g_bf_length;
extern int g_rbf_length;
extern int g_bf_padding;
extern int g_bf_decimals;

extern bn_t bntmp1;
extern bn_t bntmp2;
extern bn_t bntmp3;
extern bn_t bntmp4;
extern bn_t bntmp5;
extern bn_t bntmp6;							/* g_r_length */
extern bn_t bntest1;
extern bn_t bntest2;
extern bn_t bntest3;						/* g_r_length */
extern bn_t bntmpcpy1;
extern bn_t bntmpcpy2;						/* g_bn_length */
extern bn_t bn_pi;
extern bn_t bntmp;							/* g_r_length  */

extern bf_t bftmp1;
extern bf_t bftmp2;
extern bf_t bftmp3;
extern bf_t bftmp4;
extern bf_t bftmp5;
extern bf_t bftmp6;							/* g_rbf_length+2 */
extern bf_t bftest1;
extern bf_t bftest2;
extern bf_t bftest3;						/* g_rbf_length+2 */
extern bf_t bftmpcpy1;
extern bf_t bftmpcpy2;						/* g_bf_length+2  */
extern bf_t bf_pi;
extern bf_t bftmp;							/* g_rbf_length  */

extern bf10_t bf10tmp;						/* dec+4 */
extern big_t big_pi;

void calculate_bignum_lengths();
void init_big_dec(int dec);
void init_big_length(int bnl);
void init_big_pi();

/* functions defined in bignuma.asm or bignumc.c */
extern bn_t clear_bn(bn_t r);
extern bn_t max_bn(bn_t r);
extern bn_t copy_bn(bn_t r, bn_t n);
extern int cmp_bn(bn_t n1, bn_t n2);
extern bool is_bn_neg(bn_t n);
extern bool is_bn_not_zero(bn_t n);
extern bn_t add_bn(bn_t r, bn_t n1, bn_t n2);
extern bn_t add_a_bn(bn_t r, bn_t n);
extern bn_t sub_bn(bn_t r, bn_t n1, bn_t n2);
extern bn_t sub_a_bn(bn_t r, bn_t n);
extern bn_t neg_bn(bn_t r, bn_t n);
extern bn_t neg_a_bn(bn_t r);
extern bn_t double_bn(bn_t r, bn_t n);
extern bn_t double_a_bn(bn_t r);
extern bn_t half_bn(bn_t r, bn_t n);
extern bn_t half_a_bn(bn_t r);
extern bn_t unsafe_full_mult_bn(bn_t r, bn_t n1, bn_t n2);
extern bn_t unsafe_mult_bn(bn_t r, bn_t n1, bn_t n2);
extern bn_t unsafe_full_square_bn(bn_t r, bn_t n);
extern bn_t unsafe_square_bn(bn_t r, bn_t n);
extern bn_t mult_bn_int(bn_t r, bn_t n, U16 u);
extern bn_t mult_a_bn_int(bn_t r, U16 u);
extern bn_t unsafe_div_bn_int(bn_t r, bn_t n, U16 u);
extern bn_t div_a_bn_int(bn_t r, U16 u);

/* used to be in bigflta.asm or bigfltc.c */
extern bf_t clear_bf(bf_t r);
extern bf_t copy_bf(bf_t r, bf_t n);
extern bf_t floattobf(bf_t r, LDBL f);
extern LDBL bftofloat(bf_t n);
extern LDBL bntofloat(bn_t n);
extern LDBL extract_256(LDBL f, int *exp_ptr);
extern LDBL scale_256( LDBL f, int n );

/* functions defined in bignum.c */
#ifdef ACCESS_BY_BYTE
/* prototypes */
extern U32 big_access32(BYTE *addr);
extern U16 big_access16(BYTE *addr);
extern S16 big_accessS16(S16 *addr);
extern U32 big_set32(BYTE *addr, U32 val);
extern U16 big_set16(BYTE *addr, U16 val);
extern S16 big_setS16(S16 *addr, S16 val);
#else
/* equivalent defines */
#define big_access32(addr)   (*(U32 *)(addr))
#define big_access16(addr)   (*(U16 *)(addr))
#define big_accessS16(addr)   (*(S16 *)(addr))
#define big_set32(addr, val) (*(U32 *)(addr) = (U32)(val))
#define big_set16(addr, val) (*(U16 *)(addr) = (U16)(val))
#define big_setS16(addr, val) (*(S16 *)(addr) = (S16)(val))
#endif

extern bn_t strtobn(bn_t r, char *s);
extern char *unsafe_bntostr(char *s, int dec, bn_t r);
extern bn_t inttobn(bn_t r, long longval);
extern long bntoint(bn_t n);

extern int  sign_bn(bn_t n);
extern bn_t abs_bn(bn_t r, bn_t n);
extern bn_t abs_a_bn(bn_t r);
extern bn_t unsafe_inv_bn(bn_t r, bn_t n);
extern bn_t unsafe_div_bn(bn_t r, bn_t n1, bn_t n2);
extern bn_t sqrt_bn(bn_t r, bn_t n);
extern bn_t exp_bn(bn_t r, bn_t n);
extern bn_t unsafe_ln_bn(bn_t r, bn_t n);
extern bn_t unsafe_sincos_bn(bn_t s, bn_t c, bn_t n);
extern bn_t unsafe_atan_bn(bn_t r, bn_t n);
extern bn_t unsafe_atan2_bn(bn_t r, bn_t ny, bn_t nx);
extern int convert_bn(bn_t result,bn_t old,int newbnlength,int newintlength,int oldbnlength,int oldintlength);

    /* "safe" versions */
extern bn_t full_mult_bn(bn_t r, bn_t n1, bn_t n2);
extern bn_t mult_bn(bn_t r, bn_t n1, bn_t n2);
extern bn_t full_square_bn(bn_t r, bn_t n);
extern bn_t square_bn(bn_t r, bn_t n);
extern bn_t div_bn_int(bn_t r, bn_t n, U16 u);
extern char *bntostr(char *s, int dec, bn_t r);
extern bn_t inv_bn(bn_t r, bn_t n);
extern bn_t div_bn(bn_t r, bn_t n1, bn_t n2);
extern bn_t ln_bn(bn_t r, bn_t n);
extern bn_t sincos_bn(bn_t s, bn_t c, bn_t n);
extern bn_t atan_bn(bn_t r, bn_t n);
extern bn_t atan2_bn(bn_t r, bn_t ny, bn_t nx);

    /* misc */
extern bool is_bn_zero(bn_t n);
extern bn_t floattobn(bn_t r, LDBL f);

/************/
/* bigflt.c */
extern bf_t strtobf(bf_t r, char *s);
extern int strlen_needed_bf();
extern char *unsafe_bftostr(char *s, int dec, bf_t r);
extern char *unsafe_bftostr_e(char *s, int dec, bf_t r);
extern char *unsafe_bftostr_f(char *s, int dec, bf_t r);
extern bn_t bftobn(bn_t n, bf_t f);
extern bn_t bntobf(bf_t f, bn_t n);
extern long bftoint(bf_t f);
extern bf_t inttobf(bf_t r, long longval);

extern int sign_bf(bf_t n);
extern bf_t abs_bf(bf_t r, bf_t n);
extern bf_t abs_a_bf(bf_t r);
extern bf_t unsafe_inv_bf(bf_t r, bf_t n);
extern bf_t unsafe_div_bf(bf_t r, bf_t n1, bf_t n2);
extern bf_t unsafe_sqrt_bf(bf_t r, bf_t n);
extern bf_t exp_bf(bf_t r, bf_t n);
extern bf_t unsafe_ln_bf(bf_t r, bf_t n);
extern bf_t unsafe_sincos_bf(bf_t s, bf_t c, bf_t n);
extern bf_t unsafe_atan_bf(bf_t r, bf_t n);
extern bf_t unsafe_atan2_bf(bf_t r, bf_t ny, bf_t nx);

extern bf_t add_bf(bf_t r, bf_t n1, bf_t n2);
extern bf_t add_a_bf(bf_t r, bf_t n);
extern bf_t sub_bf(bf_t r, bf_t n1, bf_t n2);
extern bf_t sub_a_bf(bf_t r, bf_t n);
extern bf_t full_mult_bf(bf_t r, bf_t n1, bf_t n2);
extern bf_t mult_bf(bf_t r, bf_t n1, bf_t n2);
extern bf_t full_square_bf(bf_t r, bf_t n);
extern bf_t square_bf(bf_t r, bf_t n);
extern bf_t mult_bf_int(bf_t r, bf_t n, U16 u);
extern bf_t div_bf_int(bf_t r, bf_t n,  U16 u);

extern char *bftostr(char *s, int dec, bf_t r);
extern char *bftostr_e(char *s, int dec, bf_t r);
extern char *bftostr_f(char *s, int dec, bf_t r);
extern bf_t inv_bf(bf_t r, bf_t n);
extern bf_t div_bf(bf_t r, bf_t n1, bf_t n2);
extern bf_t sqrt_bf(bf_t r, bf_t n);
extern bf_t ln_bf(bf_t r, bf_t n);
extern bf_t sincos_bf(bf_t s, bf_t c, bf_t n);
extern bf_t atan_bf(bf_t r, bf_t n);
extern bf_t atan2_bf(bf_t r, bf_t ny, bf_t nx);
extern bool is_bf_zero(bf_t n);
extern int convert_bf(bf_t result, bf_t old, int newbflength, int oldbflength);

extern LDBL extract_value(LDBL f, LDBL b, int *exp_ptr);
extern LDBL scale_value( LDBL f, LDBL b , int n );
extern LDBL extract_10(LDBL f, int *exp_ptr);
extern LDBL scale_10( LDBL f, int n );

extern bf10_t unsafe_bftobf10(bf10_t s, int dec, bf_t n);
extern bf10_t mult_a_bf10_int(bf10_t s, int dec, U16 n);
extern bf10_t div_a_bf10_int (bf10_t s, int dec, U16 n);
extern char  *bf10tostr_e(char *s, int dec, bf10_t n);
extern char  *bf10tostr_f(char *s, int dec, bf10_t n);

/* functions defined in bigfltc.c */
extern bf_t norm_bf(bf_t r);
extern void norm_sign_bf(bf_t r, int positive);
extern S16 adjust_bf_add(bf_t n1, bf_t n2);
extern bf_t max_bf(bf_t r);
extern int cmp_bf(bf_t n1, bf_t n2);
extern bool is_bf_neg(bf_t n);
extern bool is_bf_not_zero(bf_t n);
extern bf_t unsafe_add_bf(bf_t r, bf_t n1, bf_t n2);
extern bf_t unsafe_add_a_bf(bf_t r, bf_t n);
extern bf_t unsafe_sub_bf(bf_t r, bf_t n1, bf_t n2);
extern bf_t unsafe_sub_a_bf(bf_t r, bf_t n);
extern bf_t neg_bf(bf_t r, bf_t n);
extern bf_t neg_a_bf(bf_t r);
extern bf_t double_bf(bf_t r, bf_t n);
extern bf_t double_a_bf(bf_t r);
extern bf_t half_bf(bf_t r, bf_t n);
extern bf_t half_a_bf(bf_t r);
extern bf_t unsafe_full_mult_bf(bf_t r, bf_t n1, bf_t n2);
extern bf_t unsafe_mult_bf(bf_t r, bf_t n1, bf_t n2);
extern bf_t unsafe_full_square_bf(bf_t r, bf_t n);
extern bf_t unsafe_square_bf(bf_t r, bf_t n);
extern bf_t unsafe_mult_bf_int(bf_t r, bf_t n, U16 u);
extern bf_t mult_a_bf_int(bf_t r, U16 u);
extern bf_t unsafe_div_bf_int(bf_t r, bf_t n,  U16 u);
extern bf_t div_a_bf_int(bf_t r, U16 u);

/****************************/
/* bigcmplx.c */
extern ComplexD complex_bn_to_float(ComplexBigNum *s);
extern ComplexD complex_bf_to_float(ComplexBigFloat *s);
extern ComplexBigFloat *complex_log_bf(ComplexBigFloat *t, ComplexBigFloat *s);
extern ComplexBigFloat *cplxmul_bf( ComplexBigFloat *t, ComplexBigFloat *x, ComplexBigFloat *y);
extern ComplexBigFloat *ComplexPower_bf(ComplexBigFloat *t, ComplexBigFloat *xx, ComplexBigFloat *yy);
extern ComplexBigNum *complex_power_bn(ComplexBigNum *t, ComplexBigNum *xx, ComplexBigNum *yy);
extern ComplexBigNum *complex_log_bn(ComplexBigNum *t, ComplexBigNum *s);
extern ComplexBigNum *complex_multiply_bn( ComplexBigNum *t, ComplexBigNum *x, ComplexBigNum *y);

#include "biginit.h" /* fractint only */

#endif /* _BIG_H */
