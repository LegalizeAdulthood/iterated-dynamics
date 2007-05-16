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
extern int bnstep, bnlength, intlength, rlength, padding, shiftfactor;
extern int g_decimals, bflength, rbflength, bfshiftfactor, bfdecimals;

/* used internally by bignum.c routines */
extern bn_t bntmp1, bntmp2, bntmp3, bntmp4, bntmp5, bntmp6;     /* rlength  */
extern bn_t bntmpcpy1, bntmpcpy2;                               /* bnlength */

/* used by other routines */
extern bn_t bnxmin, bnxmax, bnymin, bnymax, bnx3rd, bny3rd;     /* bnlength */
extern bn_t bnxdel, bnydel, bnxdel2, bnydel2, bnclosenuff;      /* bnlength */
extern bn_t bntmpsqrx, bntmpsqry, bntmp;                        /* rlength  */
extern ComplexBigNum bnold, /* bnnew, */ bnparm, bnsaved;            /* bnlength */
extern ComplexBigNum bnnew;                                           /* rlength */
extern bn_t bn_pi;                                        /* TAKES NO SPACE */

extern bf_t bftmp1, bftmp2, bftmp3, bftmp4, bftmp5, bftmp6;  /* rbflength+2 */
extern bf_t bftmpcpy1, bftmpcpy2;                            /* rbflength+2 */
extern bf_t bfxdel, bfydel, bfxdel2, bfydel2, bfclosenuff;   /* rbflength+2 */
extern bf_t bftmpsqrx, bftmpsqry;                            /* rbflength+2 */
extern ComplexBigFloat /* bfold,  bfnew, */ bfparm, bfsaved;         /* bflength+2 */
extern ComplexBigFloat bfold,  bfnew;                               /* rbflength+2 */
extern bf_t bf_pi;                                        /* TAKES NO SPACE */
extern bf_t big_pi;                                           /* bflength+2 */

/* for testing only */

/* used by other routines */
extern bf_t bfsxmin, bfsxmax, bfsymin,bfsymax,bfsx3rd,bfsy3rd;/* bflength+2 */
extern bf_t bfparms[10];                                 /* (bflength+2)*10 */
extern bf_t bftmp;
extern bf_t bf10tmp;                                              /* dec+4 */

#endif
