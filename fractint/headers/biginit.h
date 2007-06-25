/* biginit.h */
/* Used for fractint only. */
/* Many of these are redundant from big.h  */
/* but the fractint specific ones are not. */

#ifndef BIGINIT_H
#define BIGINIT_H

#define MATHBITS      32
#define MATHBYTES     (MATHBITS/8)
#define NUMVARS       30            /* room for this many on stack */
#define CURRENTREZ    1
#define MAXREZ        0


/* globals */
extern int g_step_bn;
extern int g_bn_length;
extern int g_int_length;
extern int g_r_length;
extern int g_padding;
extern int g_shift_factor;
extern int g_decimals;
extern int g_bf_length;
extern int g_rbf_length;
extern int g_bf_shift_factor;
extern int g_bf_decimals;

/* used internally by bignum.c routines */
extern bn_t bntmp1;
extern bn_t bntmp2;
extern bn_t bntmp3;
extern bn_t bntmp4;
extern bn_t bntmp5;
extern bn_t bntmp6;     /* g_r_length  */
extern bn_t bntmpcpy1;
extern bn_t bntmpcpy2;                               /* g_bn_length */

/* used by other routines */
extern bn_t bnxmin;
extern bn_t bnxmax;
extern bn_t bnymin;
extern bn_t bnymax;
extern bn_t bnx3rd;
extern bn_t bny3rd;     /* g_bn_length */
extern bn_t bnxdel;
extern bn_t bnydel;
extern bn_t bnxdel2;
extern bn_t bnydel2;
extern bn_t bnclosenuff;      /* g_bn_length */
extern bn_t bntmpsqrx;
extern bn_t bntmpsqry;
extern bn_t bntmp;                        /* g_r_length  */
extern ComplexBigNum bnold;
extern ComplexBigNum bnparm;
extern ComplexBigNum bnsaved;            /* g_bn_length */
extern ComplexBigNum bnnew;                                           /* g_r_length */
extern bn_t bn_pi;                                        /* TAKES NO SPACE */

extern bf_t bftmp1;
extern bf_t bftmp2;
extern bf_t bftmp3;
extern bf_t bftmp4;
extern bf_t bftmp5;
extern bf_t bftmp6;  /* g_rbf_length+2 */
extern bf_t bftmpcpy1;
extern bf_t bftmpcpy2;                            /* g_rbf_length+2 */
extern bf_t bfxdel;
extern bf_t bfydel;
extern bf_t bfxdel2;
extern bf_t bfydel2;
extern bf_t bfclosenuff;   /* g_rbf_length+2 */
extern bf_t bftmpsqrx;
extern bf_t bftmpsqry;                            /* g_rbf_length+2 */
extern ComplexBigFloat bfparm;
extern ComplexBigFloat bfsaved;         /* g_bf_length+2 */
extern ComplexBigFloat bfold;
extern ComplexBigFloat bfnew;                               /* g_rbf_length+2 */
extern bf_t bf_pi;                                        /* TAKES NO SPACE */
extern bf_t big_pi;                                           /* g_bf_length+2 */

/* for testing only */

/* used by other routines */
extern bf_t g_sx_min_bf;
extern bf_t g_sx_max_bf;
extern bf_t g_sy_min_bf;
extern bf_t g_sy_max_bf;
extern bf_t g_sx_3rd_bf;
extern bf_t g_sy_3rd_bf;/* g_bf_length+2 */
extern bf_t bfparms[10];                                 /* (g_bf_length+2)*10 */
extern bf_t bftmp;
extern bf_t bf10tmp;                                              /* dec+4 */

void free_bf_vars();
bn_t alloc_stack(size_t size);
int save_stack();
void restore_stack(int old_offset);
void init_bf_dec(int dec);
void init_bf_length(int bnl);
void init_big_pi();

#endif
