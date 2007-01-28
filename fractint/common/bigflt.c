/* bigflt.c - C routines for big floating point numbers */

/*
Wesley Loewer's Big Numbers.        (C) 1994-95, Wesley B. Loewer
*/

#include <string.h>
  /* see Fractint.c for a description of the "include"  hierarchy */
#include "port.h"
#include "big.h"
#ifndef BIG_ANSI_C
#include <malloc.h>
#endif

#define LOG10_256 2.4082399653118
#define LOG_256   5.5451774444795

/********************************************************************/
/* bf_hexdump() - for debugging, dumps to stdout                    */

void bf_hexdump(bf_t r)
    {
    int i;

    for (i=0; i<bflength; i++)
        printf("%02X ", *(r+i));
    printf(" e %04hX ", (S16)big_access16(r+bflength));
    printf("\n");
    return;
    }

/**********************************************************************/
/* strtobf() - converts a string into a bigfloat                       */
/*   r - pointer to a bigfloat                                        */
/*   s - string in the floating point format [+-][dig].[dig]e[+-][dig]*/
/*   note: the string may not be empty or have extra space.           */
/*         It may use scientific notation.                            */
/* USES: bftmp1                                                       */

bf_t strtobf(bf_t r, char *s)
    {
    BYTE onesbyte;
    int signflag=0;
    char *l, *d, *e; /* pointer to s, ".", "[eE]" */
    int powerten=0, keeplooping;

    clear_bf(r);

    if (s[0] == '+')    /* for + sign */
        {
        s++;
        }
    else if (s[0] == '-')    /* for neg sign */
        {
        signflag = 1;
        s++;
        }

    d = strchr(s, '.');
    e = strchr(s, 'e');
    if (e == NULL)
        e = strchr(s, 'E');
    if (e != NULL)
        {
        powerten = atoi(e+1);    /* read in the e (x10^) part */
        l = e - 1; /* just before e */
        }
    else
        l = s + strlen(s) - 1;  /* last digit */

    if (d != NULL) /* is there a decimal point? */
        {
        while (*l >= '0' && *l <= '9') /* while a digit */
            {
            onesbyte = (BYTE)(*(l--) - '0');
            inttobf(bftmp1,onesbyte);
            unsafe_add_a_bf(r, bftmp1);
            div_a_bf_int(r, 10);
            }

        if (*(l--) == '.') /* the digit was found */
            {
            keeplooping = *l >= '0' && *l <= '9' && l>=s;
            while (keeplooping) /* while a digit */
                {
                onesbyte = (BYTE)(*(l--) - '0');
                inttobf(bftmp1,onesbyte);
                unsafe_add_a_bf(r, bftmp1);
                keeplooping = *l >= '0' && *l <= '9' && l>=s;
                if (keeplooping)
                    {
                    div_a_bf_int(r, 10);
                    powerten++;    /* increase the power of ten */
                    }
                }
            }
        }
    else
        {
        keeplooping = *l >= '0' && *l <= '9' && l>=s;
        while (keeplooping) /* while a digit */
            {
            onesbyte = (BYTE)(*(l--) - '0');
            inttobf(bftmp1,onesbyte);
            unsafe_add_a_bf(r, bftmp1);
            keeplooping = *l >= '0' && *l <= '9' && l>=s;
            if (keeplooping)
                {
                div_a_bf_int(r, 10);
                powerten++;    /* increase the power of ten */
                }
            }
        }

    if (powerten > 0)
        {
        for (; powerten>0; powerten--)
            {
            mult_a_bf_int(r, 10);
            }
        }
    else if (powerten < 0)
        {
        for (; powerten<0; powerten++)
            {
            div_a_bf_int(r, 10);
            }
        }
    if (signflag)
        neg_a_bf(r);

    return r;
    }

/********************************************************************/
/* strlen_needed() - returns string length needed to hold bigfloat */

int strlen_needed_bf()
   {
   int length;

   /* first space for integer part */
    length = 1;
    length += decimals;  /* decimal part */
    length += 2;         /* decimal point and sign */
    length += 2;         /* e and sign */
    length += 4;         /* exponent */
    length += 4;         /* null and a little extra for safety */
    return(length);
    }

/********************************************************************/
/* bftostr() - converts a bigfloat into a scientific notation string */
/*   s - string, must be large enough to hold the number.           */
/* dec - decimal places, 0 for max                                  */
/*   r - bigfloat                                                   */
/*   will convert to a floating point notation                      */
/*   SIDE-EFFECT: the bigfloat, r, is destroyed.                    */
/*                Copy it first if necessary.                       */
/* USES: bftmp1 - bftmp2                                            */
/********************************************************************/

char *unsafe_bftostr(char *s, int dec, bf_t r)
    {
    LDBL value;
    int power;

    value = bftofloat(r);
    if (value == 0.0)
        {
        strcpy(s, "0.0");
        return s;
        }

    copy_bf(bftmp1, r);
    unsafe_bftobf10(bf10tmp, dec, bftmp1);
    power = (S16)big_access16(bf10tmp+dec+2); /* where the exponent is stored */
    if (power > -4 && power < 6) /* tinker with this */
        bf10tostr_f(s, dec, bf10tmp);
    else
        bf10tostr_e(s, dec, bf10tmp);
    return s;
    }


/********************************************************************/
/* the e version puts it in scientific notation, (like printf's %e) */
char *unsafe_bftostr_e(char *s, int dec, bf_t r)
    {
    LDBL value;

    value = bftofloat(r);
    if (value == 0.0)
        {
        strcpy(s, "0.0");
        return s;
        }

    copy_bf(bftmp1, r);
    unsafe_bftobf10(bf10tmp, dec, bftmp1);
    bf10tostr_e(s, dec, bf10tmp);
    return s;
    }

/********************************************************************/
/* the f version puts it in decimal notation, (like printf's %f) */
char *unsafe_bftostr_f(char *s, int dec, bf_t r)
    {
    LDBL value;

    value = bftofloat(r);
    if (value == 0.0)
        {
        strcpy(s, "0.0");
        return s;
        }

    copy_bf(bftmp1, r);
    unsafe_bftobf10(bf10tmp, dec, bftmp1);
    bf10tostr_f(s, dec, bf10tmp);
    return s;
    }

/*********************************************************************/
/*  bn = floor(bf)                                                   */
/*  Converts a bigfloat to a bignumber (integer)                     */
/*  bflength must be at least bnlength+2                             */
bn_t bftobn(bn_t n, bf_t f)
    {
    int fexp;
    int movebytes;
    BYTE hibyte;

    fexp = (S16)big_access16(f+bflength);
    if (fexp >= intlength)
        { /* if it's too big, use max value */
        max_bn(n);
        if (is_bf_neg(f))
            neg_a_bn(n);
        return n;
        }

    if (-fexp > bnlength - intlength) /* too small, return zero */
        {
        clear_bn(n);
        return n;
        }

    /* already checked for over/underflow, this should be ok */
    movebytes = bnlength - intlength + fexp + 1;
    memcpy(n, f+bflength-movebytes-1, movebytes);
    hibyte = *(f+bflength-1);
    memset(n+movebytes, hibyte, bnlength-movebytes); /* sign extends */
    return n;
    }

/*********************************************************************/
/*  bf = bn                                                          */
/*  Converts a bignumber (integer) to a bigfloat                     */
/*  bflength must be at least bnlength+2                             */
bf_t bntobf(bf_t f, bn_t n)
    {
    memcpy(f+bflength-bnlength-1, n, bnlength);
    memset(f, 0, bflength - bnlength - 1);
    *(f+bflength-1) = (BYTE)(is_bn_neg(n) ? 0xFF : 0x00); /* sign extend */
    big_set16(f+bflength, (S16)(intlength - 1)); /* exp */
    norm_bf(f);
    return f;
    }

/*********************************************************************/
/*  b = l                                                            */
/*  Converts a long to a bigfloat                                   */
bf_t inttobf(bf_t r, long longval)
    {
    clear_bf(r);
    big_set32(r+bflength-4, (S32)longval);
    big_set16(r+bflength, (S16)2);
    norm_bf(r);
    return r;
    }

/*********************************************************************/
/*  l = floor(b), floor rounds down                                  */
/*  Converts a bigfloat to a long                                    */
/*  note: a bf value of 2.999... will be return a value of 2, not 3  */
long bftoint(bf_t f)
    {
    int fexp;
    long longval;

    fexp = (S16)big_access16(f+bflength);
    if (fexp > 3)
        {
        longval = 0x7FFFFFFFL;
        if (is_bf_neg(f))
            longval = -longval;
        return longval;
        }
    longval = big_access32(f+bflength-5);
    longval >>= 8*(3-fexp);
    return longval;
    }

/********************************************************************/
/* sign(r)                                                          */
int sign_bf(bf_t n)
    {
    return is_bf_neg(n) ? -1 : is_bf_not_zero(n) ? 1 : 0;
    }

/********************************************************************/
/* r = |n|                                                          */
bf_t abs_bf(bf_t r, bf_t n)
    {
    copy_bf(r,n);
    if (is_bf_neg(r))
       {
       neg_a_bf(r);
       }
    return r;
    }

/********************************************************************/
/* r = |r|                                                          */
bf_t abs_a_bf(bf_t r)
    {
    if (is_bf_neg(r))
        neg_a_bf(r);
    return r;
    }

/********************************************************************/
/* r = 1/n                                                          */
/* uses bftmp1 - bftmp2 - global temp bigfloats                     */
/*  SIDE-EFFECTS:                                                   */
/*      n ends up as |n|/256^exp    Make copy first if necessary.   */
bf_t unsafe_inv_bf(bf_t r, bf_t n)
    {
    int signflag=0, i;
    int fexp, rexp;
    LDBL f;
    bf_t orig_r, orig_n; /* orig_bftmp1 not needed here */
    int  orig_bflength,
         orig_bnlength,
         orig_padding,
         orig_rlength,
         orig_shiftfactor,
         orig_rbflength;

    /* use Newton's recursive method for zeroing in on 1/n : r=r(2-rn) */

    if (is_bf_neg(n))
        { /* will be a lot easier to deal with just positives */
        signflag = 1;
        neg_a_bf(n);
        }

    fexp = (S16)big_access16(n+bflength);
    big_set16(n+bflength, (S16)0); /* put within LDBL range */

    f = bftofloat(n);
    if (f == 0) /* division by zero */
        {
        max_bf(r);
        return r;
        }
    f = 1/f; /* approximate inverse */

    /* With Newton's Method, there is no need to calculate all the digits */
    /* every time.  The precision approximately doubles each iteration.   */
    /* Save original values. */
    orig_bflength      = bflength;
    orig_bnlength      = bnlength;
    orig_padding       = padding;
    orig_rlength       = rlength;
    orig_shiftfactor   = shiftfactor;
    orig_rbflength     = rbflength;
    orig_r             = r;
    orig_n             = n;
    /* orig_bftmp1        = bftmp1; */

    /* calculate new starting values */
    bnlength = intlength + (int)(LDBL_DIG/LOG10_256) + 1; /* round up */
    if (bnlength > orig_bnlength)
        bnlength = orig_bnlength;
    calc_lengths();

    /* adjust pointers */
    r = orig_r + orig_bflength - bflength;
    n = orig_n + orig_bflength - bflength;
    /* bftmp1 = orig_bftmp1 + orig_bflength - bflength; */

    floattobf(r, f); /* start with approximate inverse */

    for (i=0; i<25; i++) /* safety net, this shouldn't ever be needed */
        {
        /* adjust lengths */
        bnlength <<= 1; /* double precision */
        if (bnlength > orig_bnlength)
           bnlength = orig_bnlength;
        calc_lengths();
        r = orig_r + orig_bflength - bflength;
        n = orig_n + orig_bflength - bflength;
        /* bftmp1 = orig_bftmp1 + orig_bflength - bflength; */

        unsafe_mult_bf(bftmp1, r, n); /* bftmp1=rn */
        inttobf(bftmp2, 1); /* will be used as 1.0 */

        /* There seems to very little difficulty getting bftmp1 to be EXACTLY 1 */
        if (bflength == orig_bflength && cmp_bf(bftmp1, bftmp2) == 0)
            break;

        inttobf(bftmp2, 2); /* will be used as 2.0 */
        unsafe_sub_a_bf(bftmp2, bftmp1); /* bftmp2=2-rn */
        unsafe_mult_bf(bftmp1, r, bftmp2); /* bftmp1=r(2-rn) */
        copy_bf(r, bftmp1); /* r = bftmp1 */
        }

    /* restore original values */
    bflength      = orig_bflength;
    bnlength      = orig_bnlength;
    padding       = orig_padding;
    rlength       = orig_rlength;
    shiftfactor   = orig_shiftfactor;
    rbflength     = orig_rbflength;
    r             = orig_r;
    n             = orig_n;
    /* bftmp1        = orig_bftmp1; */

    if (signflag)
        {
        neg_a_bf(r);
        }
    rexp = (S16)big_access16(r+bflength);
    rexp -= fexp;
    big_set16(r+bflength, (S16)rexp); /* adjust result exponent */
    return r;
    }

/********************************************************************/
/* r = n1/n2                                                        */
/*      r - result of length bflength                               */
/* uses bftmp1 - bftmp2 - global temp bigfloats                     */
/*  SIDE-EFFECTS:                                                   */
/*      n1, n2 end up as |n1|/256^x, |n2|/256^x                     */
/*      Make copies first if necessary.                             */
bf_t unsafe_div_bf(bf_t r, bf_t n1, bf_t n2)
    {
    int aexp, bexp, rexp;
    LDBL a, b;

    /* first, check for valid data */

    aexp = (S16)big_access16(n1+bflength);
    big_set16(n1+bflength, (S16)0); /* put within LDBL range */

    a = bftofloat(n1);
    if (a == 0) /* division into zero */
        {
        clear_bf(r); /* return 0 */
        return r;
        }

    bexp = (S16)big_access16(n2+bflength);
    big_set16(n2+bflength, (S16)0); /* put within LDBL range */

    b = bftofloat(n2);
    if (b == 0) /* division by zero */
        {
        max_bf(r);
        return r;
        }

    unsafe_inv_bf(r, n2);
    unsafe_mult_bf(bftmp1, n1, r);
    copy_bf(r, bftmp1); /* r = bftmp1 */

    rexp = (S16)big_access16(r+bflength);
    rexp += aexp - bexp;
    big_set16(r+bflength, (S16)rexp); /* adjust result exponent */

    return r;
    }

/********************************************************************/
/* sqrt(r)                                                          */
/* uses bftmp1 - bftmp3 - global temp bigfloats                    */
/*  SIDE-EFFECTS:                                                   */
/*      n ends up as |n|                                            */
bf_t unsafe_sqrt_bf(bf_t r, bf_t n)
    {
    int i, comp, almost_match=0;
    LDBL f;
    bf_t orig_r, orig_n;
    int  orig_bflength,
         orig_bnlength,
         orig_padding,
         orig_rlength,
         orig_shiftfactor,
         orig_rbflength;

/* use Newton's recursive method for zeroing in on sqrt(n): r=.5(r+n/r) */

    if (is_bf_neg(n))
        { /* sqrt of a neg, return 0 */
        clear_bf(r);
        return r;
        }

    f = bftofloat(n);
    if (f == 0) /* division by zero will occur */
        {
        clear_bf(r); /* sqrt(0) = 0 */
        return r;
        }
    f = sqrtl(f); /* approximate square root */
    /* no need to check overflow */

    /* With Newton's Method, there is no need to calculate all the digits */
    /* every time.  The precision approximately doubles each iteration.   */
    /* Save original values. */
    orig_bflength      = bflength;
    orig_bnlength      = bnlength;
    orig_padding       = padding;
    orig_rlength       = rlength;
    orig_shiftfactor   = shiftfactor;
    orig_rbflength     = rbflength;
    orig_r             = r;
    orig_n             = n;

    /* calculate new starting values */
    bnlength = intlength + (int)(LDBL_DIG/LOG10_256) + 1; /* round up */
    if (bnlength > orig_bnlength)
        bnlength = orig_bnlength;
    calc_lengths();

    /* adjust pointers */
    r = orig_r + orig_bflength - bflength;
    n = orig_n + orig_bflength - bflength;

    floattobf(r, f); /* start with approximate sqrt */

    for (i=0; i<25; i++) /* safety net, this shouldn't ever be needed */
        {
        /* adjust lengths */
        bnlength <<= 1; /* double precision */
        if (bnlength > orig_bnlength)
           bnlength = orig_bnlength;
        calc_lengths();
        r = orig_r + orig_bflength - bflength;
        n = orig_n + orig_bflength - bflength;

        unsafe_div_bf(bftmp3, n, r);
        unsafe_add_a_bf(r, bftmp3);
        half_a_bf(r);
        if (bflength == orig_bflength && (comp=abs(cmp_bf(r, bftmp3))) < 8 ) /* if match or almost match */
            {
            if (comp < 4  /* perfect or near perfect match */
                || almost_match == 1) /* close enough for 2nd time */
                break;
            else /* this is the first time they almost matched */
                almost_match++;
            }
        }

    /* restore original values */
    bflength      = orig_bflength;
    bnlength      = orig_bnlength;
    padding       = orig_padding;
    rlength       = orig_rlength;
    shiftfactor   = orig_shiftfactor;
    rbflength     = orig_rbflength;
    r             = orig_r;
    n             = orig_n;

    return r;
    }

/********************************************************************/
/* exp(r)                                                           */
/* uses bftmp1, bftmp2, bftmp3 - global temp bigfloats             */
bf_t exp_bf(bf_t r, bf_t n)
    {
    U16 fact=1;
    S16 BIGDIST * testexp, BIGDIST * rexp;

    testexp = (S16 BIGDIST *)(bftmp2+bflength);
    rexp = (S16 BIGDIST *)(r+bflength);

    if (is_bf_zero(n))
        {
        inttobf(r, 1);
        return r;
        }

/* use Taylor Series (very slow convergence) */
    inttobf(r, 1); /* start with r=1.0 */
    copy_bf(bftmp2, r);
    for (;;)
        {
        copy_bf(bftmp1, n);
        unsafe_mult_bf(bftmp3, bftmp2, bftmp1);
        unsafe_div_bf_int(bftmp2, bftmp3, fact);
        if (big_accessS16(testexp) < big_accessS16(rexp)-(bflength-2))
            break; /* too small to register */
        unsafe_add_a_bf(r, bftmp2);
        fact++;
        }

    return r;
    }

/********************************************************************/
/* ln(r)                                                            */
/* uses bftmp1 - bftmp6 - global temp bigfloats                     */
/*  SIDE-EFFECTS:                                                   */
/*      n ends up as |n|                                            */
bf_t unsafe_ln_bf(bf_t r, bf_t n)
    {
    int i, comp, almost_match=0;
    LDBL f;
    bf_t orig_r, orig_n, orig_bftmp5;
    int  orig_bflength,
         orig_bnlength,
         orig_padding,
         orig_rlength,
         orig_shiftfactor,
         orig_rbflength;

/* use Newton's recursive method for zeroing in on ln(n): r=r+n*exp(-r)-1 */

    if (is_bf_neg(n) || is_bf_zero(n))
        { /* error, return largest neg value */
        max_bf(r);
        neg_a_bf(r);
        return r;
        }

    f = bftofloat(n);
    f = logl(f); /* approximate ln(x) */
    /* no need to check overflow */
    /* appears to be ok, do ln */

    /* With Newton's Method, there is no need to calculate all the digits */
    /* every time.  The precision approximately doubles each iteration.   */
    /* Save original values. */
    orig_bflength      = bflength;
    orig_bnlength      = bnlength;
    orig_padding       = padding;
    orig_rlength       = rlength;
    orig_shiftfactor   = shiftfactor;
    orig_rbflength     = rbflength;
    orig_r             = r;
    orig_n             = n;
    orig_bftmp5        = bftmp5;

    /* calculate new starting values */
    bnlength = intlength + (int)(LDBL_DIG/LOG10_256) + 1; /* round up */
    if (bnlength > orig_bnlength)
        bnlength = orig_bnlength;
    calc_lengths();

    /* adjust pointers */
    r = orig_r + orig_bflength - bflength;
    n = orig_n + orig_bflength - bflength;
    bftmp5 = orig_bftmp5 + orig_bflength - bflength;

    floattobf(r, f); /* start with approximate ln */
    neg_a_bf(r); /* -r */
    copy_bf(bftmp5, r); /* -r */

    for (i=0; i<25; i++) /* safety net, this shouldn't ever be needed */
        {
        /* adjust lengths */
        bnlength <<= 1; /* double precision */
        if (bnlength > orig_bnlength)
           bnlength = orig_bnlength;
        calc_lengths();
        r = orig_r + orig_bflength - bflength;
        n = orig_n + orig_bflength - bflength;
        bftmp5 = orig_bftmp5 + orig_bflength - bflength;

        exp_bf(bftmp6, r);     /* exp(-r) */
        unsafe_mult_bf(bftmp2, bftmp6, n);  /* n*exp(-r) */
        inttobf(bftmp4, 1);
        unsafe_sub_a_bf(bftmp2, bftmp4);   /* n*exp(-r) - 1 */
        unsafe_sub_a_bf(r, bftmp2);        /* -r - (n*exp(-r) - 1) */
        if (bflength == orig_bflength && (comp=abs(cmp_bf(r, bftmp5))) < 8 ) /* if match or almost match */
            {
            if (comp < 4  /* perfect or near perfect match */
                || almost_match == 1) /* close enough for 2nd time */
                break;
            else /* this is the first time they almost matched */
                almost_match++;
            }
        copy_bf(bftmp5, r); /* -r */
        }

    /* restore original values */
    bflength      = orig_bflength;
    bnlength      = orig_bnlength;
    padding       = orig_padding;
    rlength       = orig_rlength;
    shiftfactor   = orig_shiftfactor;
    rbflength     = orig_rbflength;
    r             = orig_r;
    n             = orig_n;
    bftmp5        = orig_bftmp5;

    neg_a_bf(r); /* -(-r) */
    return r;
    }

/********************************************************************/
/* sincos_bf(r)                                                     */
/* uses bftmp1 - bftmp2 - global temp bigfloats                     */
/*  SIDE-EFFECTS:                                                   */
/*      n ends up as |n| mod (pi/4)                                 */
bf_t unsafe_sincos_bf(bf_t s, bf_t c, bf_t n)
    {
    U16 fact=2;
    int k=0, i, halves;
    int signcos=0, signsin=0, switch_sincos=0;
    int sin_done=0, cos_done=0;
    S16 BIGDIST * testexp, BIGDIST * cexp, BIGDIST * sexp;

    testexp = (S16 BIGDIST *)(bftmp1+bflength);
    cexp = (S16 BIGDIST *)(c+bflength);
    sexp = (S16 BIGDIST *)(s+bflength);

#ifndef CALCULATING_BIG_PI
    /* assure range 0 <= x < pi/4 */

    if (is_bf_zero(n))
        {
        clear_bf(s);    /* sin(0) = 0 */
        inttobf(c, 1);  /* cos(0) = 1 */
        return s;
        }

    if (is_bf_neg(n))
        {
        signsin = !signsin; /* sin(-x) = -sin(x), odd; cos(-x) = cos(x), even */
        neg_a_bf(n);
        }
    /* n >= 0 */

    double_bf(bftmp1, bf_pi); /* 2*pi */
    /* this could be done with remainders, but it would probably be slower */
    while (cmp_bf(n, bftmp1) >= 0) /* while n >= 2*pi */
        {
        copy_bf(bftmp2, bftmp1);
        unsafe_sub_a_bf(n, bftmp2);
        }
    /* 0 <= n < 2*pi */

    copy_bf(bftmp1, bf_pi); /* pi */
    if (cmp_bf(n, bftmp1) >= 0) /* if n >= pi */
        {
        unsafe_sub_a_bf(n, bftmp1);
        signsin = !signsin;
        signcos = !signcos;
        }
    /* 0 <= n < pi */

    half_bf(bftmp1, bf_pi); /* pi/2 */
    if (cmp_bf(n, bftmp1) > 0) /* if n > pi/2 */
        {
        copy_bf(bftmp2, bf_pi);
        unsafe_sub_bf(n, bftmp2, n);
        signcos = !signcos;
        }
    /* 0 <= n < pi/2 */

    half_bf(bftmp1, bf_pi); /* pi/2 */
    half_a_bf(bftmp1);      /* pi/4 */
    if (cmp_bf(n, bftmp1) > 0) /* if n > pi/4 */
        {
        copy_bf(bftmp2, n);
        half_bf(bftmp1, bf_pi); /* pi/2 */
        unsafe_sub_bf(n, bftmp1, bftmp2);  /* pi/2 - n */
        switch_sincos = !switch_sincos;
        }
    /* 0 <= n < pi/4 */

    /* this looks redundant, but n could now be zero when it wasn't before */
    if (is_bf_zero(n))
        {
        clear_bf(s);    /* sin(0) = 0 */
        inttobf(c, 1);  /* cos(0) = 1 */
        return s;
        }


/* at this point, the double angle trig identities could be used as many  */
/* times as desired to reduce the range to pi/8, pi/16, etc...  Each time */
/* the range is cut in half, the number of iterations required is reduced */
/* by "quite a bit."  It's just a matter of testing to see what gives the */
/* optimal results.                                                       */
    /* halves = bflength / 10; */ /* this is experimental */
    halves = 1;
    for (i = 0; i < halves; i++)
        half_a_bf(n);
#endif

/* use Taylor Series (very slow convergence) */
    copy_bf(s, n); /* start with s=n */
    inttobf(c, 1); /* start with c=1 */
    copy_bf(bftmp1, n); /* the current x^n/n! */
    do
        {
        /* even terms for cosine */
        copy_bf(bftmp2, bftmp1);
        unsafe_mult_bf(bftmp1, bftmp2, n);
        div_a_bf_int(bftmp1, fact++);
        if (!cos_done)
            {
            cos_done = (big_accessS16(testexp) < big_accessS16(cexp)-(bflength-2)); /* too small to register */
            if (!cos_done)
                {
                if (k) /* alternate between adding and subtracting */
                    unsafe_add_a_bf(c, bftmp1);
                else
                    unsafe_sub_a_bf(c, bftmp1);
                }
            }

        /* odd terms for sine */
        copy_bf(bftmp2, bftmp1);
        unsafe_mult_bf(bftmp1, bftmp2, n);
        div_a_bf_int(bftmp1, fact++);
        if (!sin_done)
            {
            sin_done = (big_accessS16(testexp) < big_accessS16(sexp)-(bflength-2)); /* too small to register */
            if (!sin_done)
                {
                if (k) /* alternate between adding and subtracting */
                    unsafe_add_a_bf(s, bftmp1);
                else
                    unsafe_sub_a_bf(s, bftmp1);
                }
            }
        k = !k; /* toggle */
#ifdef CALCULATING_BIG_PI
        printf("."); /* lets you know it's doing something */
#endif
        } while (!cos_done || !sin_done);

#ifndef CALCULATING_BIG_PI
        /* now need to undo what was done by cutting angles in half */
        for (i = 0; i < halves; i++)
            {
            unsafe_mult_bf(bftmp2, s, c); /* no need for safe mult */
            double_bf(s, bftmp2); /* sin(2x) = 2*sin(x)*cos(x) */
            unsafe_square_bf(bftmp2,c);
            double_a_bf(bftmp2);
            inttobf(bftmp1, 1);
            unsafe_sub_bf(c, bftmp2, bftmp1); /* cos(2x) = 2*cos(x)*cos(x) - 1 */
            }

        if (switch_sincos)
            {
            copy_bf(bftmp1, s);
            copy_bf(s, c);
            copy_bf(c, bftmp1);
            }
        if (signsin)
            neg_a_bf(s);
        if (signcos)
            neg_a_bf(c);
#endif

    return s; /* return sine I guess */
    }

/********************************************************************/
/* atan(r)                                                          */
/* uses bftmp1 - bftmp5 - global temp bigfloats                    */
/*  SIDE-EFFECTS:                                                   */
/*      n ends up as |n| or 1/|n|                                   */
bf_t unsafe_atan_bf(bf_t r, bf_t n)
    {
    int i, comp, almost_match=0, signflag=0;
    LDBL f;
    bf_t orig_r, orig_n, orig_bf_pi, orig_bftmp3;
    int  orig_bflength,
         orig_bnlength,
         orig_padding,
         orig_rlength,
         orig_shiftfactor,
         orig_rbflength;
    int large_arg;


/* use Newton's recursive method for zeroing in on atan(n): r=r-cos(r)(sin(r)-n*cos(r)) */

    if (is_bf_neg(n))
        {
        signflag = 1;
        neg_a_bf(n);
        }

/* If n is very large, atanl() won't give enough decimal places to be a */
/* good enough initial guess for Newton's Method.  If it is larger than */
/* say, 1, atan(n) = pi/2 - acot(n) = pi/2 - atan(1/n).                 */

    f = bftofloat(n);
    large_arg = f > 1.0;
    if (large_arg)
        {
        unsafe_inv_bf(bftmp3, n);
        copy_bf(n, bftmp3);
        f = bftofloat(n);
        }

    clear_bf(bftmp3); /* not really necessary, but makes things more consistent */

    /* With Newton's Method, there is no need to calculate all the digits */
    /* every time.  The precision approximately doubles each iteration.   */
    /* Save original values. */
    orig_bflength      = bflength;
    orig_bnlength      = bnlength;
    orig_padding       = padding;
    orig_rlength       = rlength;
    orig_shiftfactor   = shiftfactor;
    orig_rbflength     = rbflength;
    orig_bf_pi         = bf_pi;
    orig_r             = r;
    orig_n             = n;
    orig_bftmp3        = bftmp3;

    /* calculate new starting values */
    bnlength = intlength + (int)(LDBL_DIG/LOG10_256) + 1; /* round up */
    if (bnlength > orig_bnlength)
        bnlength = orig_bnlength;
    calc_lengths();

    /* adjust pointers */
    r = orig_r + orig_bflength - bflength;
    n = orig_n + orig_bflength - bflength;
    bf_pi = orig_bf_pi + orig_bflength - bflength;
    bftmp3 = orig_bftmp3 + orig_bflength - bflength;

    f = atanl(f); /* approximate arctangent */
    /* no need to check overflow */

    floattobf(r, f); /* start with approximate atan */
    copy_bf(bftmp3, r);

    for (i=0; i<25; i++) /* safety net, this shouldn't ever be needed */
        {
        /* adjust lengths */
        bnlength <<= 1; /* double precision */
        if (bnlength > orig_bnlength)
           bnlength = orig_bnlength;
        calc_lengths();
        r = orig_r + orig_bflength - bflength;
        n = orig_n + orig_bflength - bflength;
        bf_pi = orig_bf_pi + orig_bflength - bflength;
        bftmp3 = orig_bftmp3 + orig_bflength - bflength;

#ifdef CALCULATING_BIG_PI
        printf("\natan() loop #%i, bflength=%i\nsincos() loops\n", i, bflength);
#endif
        unsafe_sincos_bf(bftmp4, bftmp5, bftmp3);   /* sin(r), cos(r) */
        copy_bf(bftmp3, r); /* restore bftmp3 from sincos_bf() */
        copy_bf(bftmp1, bftmp5);
        unsafe_mult_bf(bftmp2, n, bftmp1);     /* n*cos(r) */
        unsafe_sub_a_bf(bftmp4, bftmp2); /* sin(r) - n*cos(r) */
        unsafe_mult_bf(bftmp1, bftmp5, bftmp4); /* cos(r) * (sin(r) - n*cos(r)) */
        copy_bf(bftmp3, r);
        unsafe_sub_a_bf(r, bftmp1); /* r - cos(r) * (sin(r) - n*cos(r)) */
#ifdef CALCULATING_BIG_PI
        putchar('\n');
        bf_hexdump(r);
#endif
        if (bflength == orig_bflength && (comp=abs(cmp_bf(r, bftmp3))) < 8 ) /* if match or almost match */
            {
#ifdef CALCULATING_BIG_PI
            printf("atan() loop comp=%i\n", comp);
#endif
            if (comp < 4  /* perfect or near perfect match */
                || almost_match == 1) /* close enough for 2nd time */
                break;
            else /* this is the first time they almost matched */
                almost_match++;
            }

#ifdef CALCULATING_BIG_PI
        if (bflength == orig_bflength && comp >= 8)
            printf("atan() loop comp=%i\n", comp);
#endif

        copy_bf(bftmp3, r); /* make a copy for later comparison */
        }

    /* restore original values */
    bflength      = orig_bflength;
    bnlength      = orig_bnlength;
    padding       = orig_padding;
    rlength       = orig_rlength;
    shiftfactor   = orig_shiftfactor;
    rbflength     = orig_rbflength;
    bf_pi         = orig_bf_pi;
    r             = orig_r;
    n             = orig_n;
    bftmp3        = orig_bftmp3;

    if (large_arg)
        {
        half_bf(bftmp3, bf_pi);  /* pi/2 */
        sub_a_bf(bftmp3, r);     /* pi/2 - atan(1/n) */
        copy_bf(r, bftmp3);
        }

    if (signflag)
        neg_a_bf(r);
    return r;
    }

/********************************************************************/
/* atan2(r,ny,nx)                                                     */
/* uses bftmp1 - bftmp6 - global temp bigfloats                     */
bf_t unsafe_atan2_bf(bf_t r, bf_t ny, bf_t nx)
   {
   int signx, signy;

   signx = sign_bf(nx);
   signy = sign_bf(ny);

   if (signy == 0)
      {
      if (signx < 0)
         copy_bf(r, bf_pi); /* negative x axis, 180 deg */
      else    /* signx >= 0    positive x axis, 0 */
         clear_bf(r);
      return(r);
      }
   if (signx == 0)
      {
      copy_bf(r, bf_pi); /* y axis */
      half_a_bf(r);      /* +90 deg */
      if (signy < 0)
         neg_a_bf(r);    /* -90 deg */
      return(r);
      }

   if (signy < 0)
      neg_a_bf(ny);
   if (signx < 0)
      neg_a_bf(nx);
   unsafe_div_bf(bftmp6,ny,nx);
   unsafe_atan_bf(r, bftmp6);
   if (signx < 0)
      sub_bf(r,bf_pi,r);
   if (signy < 0)
      neg_a_bf(r);
   return(r);
   }

/**********************************************************************/
/* The rest of the functions are "safe" versions of the routines that */
/* have side effects which alter the parameters.                      */
/* Most bf routines change values of parameters, not just the sign.   */
/**********************************************************************/

/**********************************************************************/
bf_t add_bf(bf_t r, bf_t n1, bf_t n2)
    {
    copy_bf(bftmpcpy1, n1);
    copy_bf(bftmpcpy2, n2);
    unsafe_add_bf(r, bftmpcpy1, bftmpcpy2);
    return r;
    }

/**********************************************************************/
bf_t add_a_bf(bf_t r, bf_t n)
    {
    copy_bf(bftmpcpy1, n);
    unsafe_add_a_bf(r, bftmpcpy1);
    return r;
    }

/**********************************************************************/
bf_t sub_bf(bf_t r, bf_t n1, bf_t n2)
    {
    copy_bf(bftmpcpy1, n1);
    copy_bf(bftmpcpy2, n2);
    unsafe_sub_bf(r, bftmpcpy1, bftmpcpy2);
    return r;
    }

/**********************************************************************/
bf_t sub_a_bf(bf_t r, bf_t n)
    {
    copy_bf(bftmpcpy1, n);
    unsafe_sub_a_bf(r, bftmpcpy1);
    return r;
    }

/**********************************************************************/
/* mult and div only change sign */
bf_t full_mult_bf(bf_t r, bf_t n1, bf_t n2)
    {
    copy_bf(bftmpcpy1, n1);
    copy_bf(bftmpcpy2, n2);
    unsafe_full_mult_bf(r, bftmpcpy1, bftmpcpy2);
    return r;
    }

/**********************************************************************/
bf_t mult_bf(bf_t r, bf_t n1, bf_t n2)
    {
    copy_bf(bftmpcpy1, n1);
    copy_bf(bftmpcpy2, n2);
    unsafe_mult_bf(r, bftmpcpy1, bftmpcpy2);
    return r;
    }

/**********************************************************************/
bf_t full_square_bf(bf_t r, bf_t n)
    {
    copy_bf(bftmpcpy1, n);
    unsafe_full_square_bf(r, bftmpcpy1);
    return r;
    }

/**********************************************************************/
bf_t square_bf(bf_t r, bf_t n)
    {
    copy_bf(bftmpcpy1, n);
    unsafe_square_bf(r, bftmpcpy1);
    return r;
    }

/**********************************************************************/
bf_t mult_bf_int(bf_t r, bf_t n, U16 u)
    {
    copy_bf(bftmpcpy1, n);
    unsafe_mult_bf_int(r, bftmpcpy1, u);
    return r;
    }

/**********************************************************************/
bf_t div_bf_int(bf_t r, bf_t n,  U16 u)
    {
    copy_bf(bftmpcpy1, n);
    unsafe_div_bf_int(r, bftmpcpy1, u);
    return r;
    }

/**********************************************************************/
char *bftostr(char *s, int dec, bf_t r)
    {
    copy_bf(bftmpcpy1, r);
    unsafe_bftostr(s, dec, bftmpcpy1);
    return s;
    }

/**********************************************************************/
char *bftostr_e(char *s, int dec, bf_t r)
    {
    copy_bf(bftmpcpy1, r);
    unsafe_bftostr_e(s, dec, bftmpcpy1);
    return s;
    }

/**********************************************************************/
char *bftostr_f(char *s, int dec, bf_t r)
    {
    copy_bf(bftmpcpy1, r);
    unsafe_bftostr_f(s, dec, bftmpcpy1);
    return s;
    }

/**********************************************************************/
bf_t inv_bf(bf_t r, bf_t n)
    {
    copy_bf(bftmpcpy1, n);
    unsafe_inv_bf(r, bftmpcpy1);
    return r;
    }

/**********************************************************************/
bf_t div_bf(bf_t r, bf_t n1, bf_t n2)
    {
    copy_bf(bftmpcpy1, n1);
    copy_bf(bftmpcpy2, n2);
    unsafe_div_bf(r, bftmpcpy1, bftmpcpy2);
    return r;
    }

/**********************************************************************/
bf_t sqrt_bf(bf_t r, bf_t n)
    {
    copy_bf(bftmpcpy1, n);
    unsafe_sqrt_bf(r, bftmpcpy1);
    return r;
    }

/**********************************************************************/
bf_t ln_bf(bf_t r, bf_t n)
    {
    copy_bf(bftmpcpy1, n);
    unsafe_ln_bf(r, bftmpcpy1);
    return r;
    }

/**********************************************************************/
bf_t sincos_bf(bf_t s, bf_t c, bf_t n)
    {
    copy_bf(bftmpcpy1, n);
    return unsafe_sincos_bf(s, c, bftmpcpy1);
    }

/**********************************************************************/
bf_t atan_bf(bf_t r, bf_t n)
    {
    copy_bf(bftmpcpy1, n);
    unsafe_atan_bf(r, bftmpcpy1);
    return r;
    }

/**********************************************************************/
bf_t atan2_bf(bf_t r, bf_t ny, bf_t nx)
   {
    copy_bf(bftmpcpy1, ny);
    copy_bf(bftmpcpy2, nx);
    unsafe_atan2_bf(r, bftmpcpy1, bftmpcpy2);
    return r;
    }

/**********************************************************************/
int is_bf_zero(bf_t n)
    {
    return !is_bf_not_zero(n);
    }

/************************************************************************/
/* convert_bf  -- convert bigfloat numbers from old to new lengths      */
int convert_bf(bf_t newnum, bf_t old, int newbflength, int oldbflength)
   {
   int savebflength;

   /* save lengths so not dependent on external environment */
   savebflength  = bflength;
   bflength      = newbflength;
   clear_bf(newnum);
   bflength      = savebflength;

   if (newbflength > oldbflength)
      memcpy(newnum+newbflength-oldbflength,old,oldbflength+2);
   else
      memcpy(newnum,old+oldbflength-newbflength,newbflength+2);
   return(0);
   }

/* The following used to be in bigfltc.c */
/********************************************************************/
/* normalize big float */
bf_t norm_bf(bf_t r)
    {
    int scale;
    BYTE hi_byte;
    S16 BIGDIST *rexp;

    rexp  = (S16 BIGDIST *)(r+bflength);

    /* check for overflow */
    hi_byte = r[bflength-1];
    if (hi_byte != 0x00 && hi_byte != 0xFF)
        {
        memmove(r, r+1, bflength-1);
        r[bflength-1] = (BYTE)(hi_byte & 0x80 ? 0xFF : 0x00);
        big_setS16(rexp,big_accessS16(rexp)+(S16)1);   /* exp */
        }

    /* check for underflow */
    else
        {
        for (scale = 2; scale < bflength && r[bflength-scale] == hi_byte; scale++)
            ; /* do nothing */
        if (scale == bflength && hi_byte == 0) /* zero */
            big_setS16(rexp,0);
        else
            {
            scale -= 2;
            if (scale > 0) /* it did underflow */
                {
                memmove(r+scale, r, bflength-scale-1);
                memset(r, 0, scale);
                big_setS16(rexp,big_accessS16(rexp)-(S16)scale);    /* exp */
                }
            }
        }

    return r;
    }

/********************************************************************/
/* normalize big float with forced sign */
/* positive = 1, force to be positive   */
/*          = 0, force to be negative   */
void norm_sign_bf(bf_t r, int positive)
    {
    norm_bf(r);
    r[bflength-1] = (BYTE)(positive ? 0x00 : 0xFF);
    }
/******************************************************/
/* adjust n1, n2 for before addition or subtraction   */
/* by forcing exp's to match.                         */
/* returns the value of the adjusted exponents        */
S16 adjust_bf_add(bf_t n1, bf_t n2)
    {
    int scale, fill_byte;
    S16 rexp;
    S16 BIGDIST *n1exp, BIGDIST *n2exp;

    /* scale n1 or n2 */
    /* compare exp's */
    n1exp = (S16 BIGDIST *)(n1+bflength);
    n2exp = (S16 BIGDIST *)(n2+bflength);
    if (big_accessS16(n1exp) > big_accessS16(n2exp))
        { /* scale n2 */
        scale = big_accessS16(n1exp) - big_accessS16(n2exp); /* n1exp - n2exp */
        if (scale < bflength)
            {
            fill_byte = is_bf_neg(n2) ? 0xFF : 0x00;
            memmove(n2, n2+scale, bflength-scale);
            memset(n2+bflength-scale, fill_byte, scale);
            }
        else
            clear_bf(n2);
        big_setS16(n2exp,big_accessS16(n1exp)); /* *n2exp = *n1exp; set exp's = */
        rexp = big_accessS16(n2exp);
        }
    else if (big_accessS16(n1exp) < big_accessS16(n2exp))
        { /* scale n1 */
        scale = big_accessS16(n2exp) - big_accessS16(n1exp);  /* n2exp - n1exp */
        if (scale < bflength)
            {
            fill_byte = is_bf_neg(n1) ? 0xFF : 0x00;
            memmove(n1, n1+scale, bflength-scale);
            memset(n1+bflength-scale, fill_byte, scale);
            }
        else
            clear_bf(n1);
        big_setS16(n1exp,big_accessS16(n2exp)); /* *n1exp = *n2exp; set exp's = */
        rexp = big_accessS16(n2exp);
        }
    else
        rexp = big_accessS16(n1exp);
    return rexp;
    }

/********************************************************************/
/* r = max positive value */
bf_t max_bf(bf_t r)
    {
    inttobf(r, 1);
    big_set16(r+bflength, (S16)(LDBL_MAX_EXP/8));
    return r;
    }

/****************************************************************************/
/* n1 != n2 ?                                                               */
/* RETURNS:                                                                 */
/*  if n1 == n2 returns 0                                                   */
/*  if n1 > n2 returns a positive (bytes left to go when mismatch occurred) */
/*  if n1 < n2 returns a negative (bytes left to go when mismatch occurred) */

int cmp_bf(bf_t n1, bf_t n2)
    {
    int i;
    int sign1, sign2;
    S16 BIGDIST *n1exp, BIGDIST *n2exp;
    U16 value1, value2;


    /* compare signs */
    sign1 = sign_bf(n1);
    sign2 = sign_bf(n2);
    if (sign1 > sign2)
        return bflength;
    else if (sign1 < sign2)
        return -bflength;
    /* signs are the same */

    /* compare exponents, using signed comparisons */
    n1exp = (S16 BIGDIST *)(n1+bflength);
    n2exp = (S16 BIGDIST *)(n2+bflength);
    if (big_accessS16(n1exp) > big_accessS16(n2exp))
        return sign1*(bflength);
    else if (big_accessS16(n1exp) < big_accessS16(n2exp))
        return -sign1*(bflength);

    /* To get to this point, the signs must match */
    /* so unsigned comparison is ok. */
    /* two bytes at a time */
    for (i=bflength-2; i>=0; i-=2)
        {
        if ( (value1=big_access16(n1+i)) > (value2=big_access16(n2+i)) )
            { /* now determine which of the two bytes was different */
            if ( (value1&0xFF00) > (value2&0xFF00) ) /* compare just high bytes */
                return (i+2); /* high byte was different */
            else
                return (i+1); /* low byte was different */
            }
        else if (value1 < value2)
            { /* now determine which of the two bytes was different */
            if ( (value1&0xFF00) < (value2&0xFF00) ) /* compare just high bytes */
                return -(i+2); /* high byte was different */
            else
                return -(i+1); /* low byte was different */
            }
        }
    return 0;
    }

/********************************************************************/
/* r < 0 ?                                      */
/* returns 1 if negative, 0 if positive or zero */
int is_bf_neg(bf_t n)
    {
    return (S8)n[bflength-1] < 0;
    }

/********************************************************************/
/* n != 0 ?                      */
/* RETURNS: if n != 0 returns 1  */
/*          else returns 0       */
int is_bf_not_zero(bf_t n)
    {
    int bnl;
    int retval;

    bnl = bnlength;
    bnlength = bflength;
    retval = is_bn_not_zero(n);
    bnlength = bnl;

    return retval;
    }

/********************************************************************/
/* r = n1 + n2 */
/* SIDE-EFFECTS: n1 and n2 can be "de-normalized" and lose precision */
bf_t unsafe_add_bf(bf_t r, bf_t n1, bf_t n2)
    {
    int bnl;
    S16 BIGDIST *rexp;

    if (is_bf_zero(n1))
        {
        copy_bf(r, n2);
        return r;
        }
    if (is_bf_zero(n2))
        {
        copy_bf(r, n1);
        return r;
        }

    rexp = (S16 BIGDIST *)(r+bflength);
    big_setS16(rexp,adjust_bf_add(n1, n2));

    bnl = bnlength;
    bnlength = bflength;
    add_bn(r, n1, n2);
    bnlength = bnl;

    norm_bf(r);
    return r;
    }

/********************************************************************/
/* r += n */
bf_t unsafe_add_a_bf(bf_t r, bf_t n)
    {
    int bnl;

    if (is_bf_zero(r))
        {
        copy_bf(r, n);
        return r;
        }
    if (is_bf_zero(n))
        {
        return r;
        }

    adjust_bf_add(r, n);

    bnl = bnlength;
    bnlength = bflength;
    add_a_bn(r, n);
    bnlength = bnl;

    norm_bf(r);

    return r;
    }

/********************************************************************/
/* r = n1 - n2 */
/* SIDE-EFFECTS: n1 and n2 can be "de-normalized" and lose precision */
bf_t unsafe_sub_bf(bf_t r, bf_t n1, bf_t n2)
    {
    int bnl;
    S16 BIGDIST *rexp;

    if (is_bf_zero(n1))
        {
        neg_bf(r, n2);
        return r;
        }
    if (is_bf_zero(n2))
        {
        copy_bf(r, n1);
        return r;
        }

    rexp = (S16 BIGDIST *)(r+bflength);
    big_setS16(rexp,adjust_bf_add(n1, n2));

    bnl = bnlength;
    bnlength = bflength;
    sub_bn(r, n1, n2);
    bnlength = bnl;

    norm_bf(r);
    return r;
    }

/********************************************************************/
/* r -= n */
bf_t unsafe_sub_a_bf(bf_t r, bf_t n)
    {
    int bnl;

    if (is_bf_zero(r))
        {
        neg_bf(r,n);
        return r;
        }
    if (is_bf_zero(n))
        {
        return r;
        }

    adjust_bf_add(r, n);

    bnl = bnlength;
    bnlength = bflength;
    sub_a_bn(r, n);
    bnlength = bnl;

    norm_bf(r);
    return r;
    }

/********************************************************************/
/* r = -n */
bf_t neg_bf(bf_t r, bf_t n)
    {
    int bnl;
    S16 BIGDIST *rexp, BIGDIST *nexp;

    rexp = (S16 BIGDIST *)(r+bflength);
    nexp = (S16 BIGDIST *)(n+bflength);
    big_setS16(rexp, big_accessS16(nexp)); /* *rexp = *nexp; */

    bnl = bnlength;
    bnlength = bflength;
    neg_bn(r, n);
    bnlength = bnl;

    norm_bf(r);
    return r;
    }

/********************************************************************/
/* r *= -1 */
bf_t neg_a_bf(bf_t r)
    {
    int bnl;

    bnl = bnlength;
    bnlength = bflength;
    neg_a_bn(r);
    bnlength = bnl;

    norm_bf(r);
    return r;
    }

/********************************************************************/
/* r = 2*n */
bf_t double_bf(bf_t r, bf_t n)
    {
    int bnl;
    S16 BIGDIST *rexp, BIGDIST *nexp;

    rexp = (S16 BIGDIST *)(r+bflength);
    nexp = (S16 BIGDIST *)(n+bflength);
    big_setS16(rexp, big_accessS16(nexp)); /* *rexp = *nexp; */

    bnl = bnlength;
    bnlength = bflength;
    double_bn(r, n);
    bnlength = bnl;

    norm_bf(r);
    return r;
    }

/********************************************************************/
/* r *= 2 */
bf_t double_a_bf(bf_t r)
    {
    int bnl;

    bnl = bnlength;
    bnlength = bflength;
    double_a_bn(r);
    bnlength = bnl;

    norm_bf(r);
    return r;
    }

/********************************************************************/
/* r = n/2 */
bf_t half_bf(bf_t r, bf_t n)
    {
    int bnl;
    S16 BIGDIST *rexp, BIGDIST *nexp;

    rexp = (S16 BIGDIST *)(r+bflength);
    nexp = (S16 BIGDIST *)(n+bflength);
    big_setS16(rexp, big_accessS16(nexp)); /* *rexp = *nexp; */

    bnl = bnlength;
    bnlength = bflength;
    half_bn(r, n);
    bnlength = bnl;

    norm_bf(r);
    return r;
    }

/********************************************************************/
/* r /= 2 */
bf_t half_a_bf(bf_t r)
    {
    int bnl;

    bnl = bnlength;
    bnlength = bflength;
    half_a_bn(r);
    bnlength = bnl;

    norm_bf(r);
    return r;
    }

/************************************************************************/
/* r = n1 * n2                                                          */
/* Note: r will be a double wide result, 2*bflength                     */
/*       n1 and n2 can be the same pointer                              */
/* SIDE-EFFECTS: n1 and n2 are changed to their absolute values         */
bf_t unsafe_full_mult_bf(bf_t r, bf_t n1, bf_t n2)
    {
    int bnl, dbfl;
    S16 BIGDIST *rexp, BIGDIST *n1exp, BIGDIST *n2exp;

    if (is_bf_zero(n1) || is_bf_zero(n2) )
        {
        bflength <<= 1;
        clear_bf(r);
        bflength >>= 1;
        return r;
        }

    dbfl = 2*bflength; /* double width bflength */
    rexp  = (S16 BIGDIST *)(r+dbfl); /* note: 2*bflength */
    n1exp = (S16 BIGDIST *)(n1+bflength);
    n2exp = (S16 BIGDIST *)(n2+bflength);
    /* add exp's */
    big_setS16(rexp, (S16)(big_accessS16(n1exp) + big_accessS16(n2exp)) );

    bnl = bnlength;
    bnlength = bflength;
    unsafe_full_mult_bn(r, n1, n2);
    bnlength = bnl;

    /* handle normalizing full mult on individual basis */

    return r;
    }

/************************************************************************/
/* r = n1 * n2 calculating only the top rlength bytes                   */
/* Note: r will be of length rlength                                    */
/*       2*bflength <= rlength < bflength                               */
/*       n1 and n2 can be the same pointer                              */
/* SIDE-EFFECTS: n1 and n2 are changed to their absolute values         */
bf_t unsafe_mult_bf(bf_t r, bf_t n1, bf_t n2)
    {
    int positive;
    int bnl, bfl, rl;
    int rexp;
    S16 BIGDIST *n1exp, BIGDIST *n2exp;

    if (is_bf_zero(n1) || is_bf_zero(n2) )
        {
        clear_bf(r);
        return r;
        }

    n1exp = (S16 BIGDIST *)(n1+bflength);
    n2exp = (S16 BIGDIST *)(n2+bflength);
    /* add exp's */
    rexp = big_accessS16(n1exp) + big_accessS16(n2exp);

    positive = is_bf_neg(n1) == is_bf_neg(n2); /* are they the same sign? */

    bnl = bnlength;
    bnlength = bflength;
    rl = rlength;
    rlength = rbflength;
    unsafe_mult_bn(r, n1, n2);
    bnlength = bnl;
    rlength = rl;

    bfl = bflength;
    bflength = rbflength;
    big_set16(r+bflength, (S16)(rexp+2)); /* adjust after mult */
    norm_sign_bf(r, positive);
    bflength = bfl;
    memmove(r, r+padding, bflength+2); /* shift back */

    return r;
    }

/************************************************************************/
/* r = n^2                                                              */
/*   because of the symmetry involved, n^2 is much faster than n*n      */
/*   for a bignumber of length l                                        */
/*      n*n takes l^2 multiplications                                   */
/*      n^2 takes (l^2+l)/2 multiplications                             */
/*          which is about 1/2 n*n as l gets large                      */
/*  uses the fact that (a+b+c+...)^2 = (a^2+b^2+c^2+...)+2(ab+ac+bc+...)*/
/*                                                                      */
/* SIDE-EFFECTS: n is changed to its absolute value                     */
bf_t unsafe_full_square_bf(bf_t r, bf_t n)
    {
    int bnl, dbfl;
    S16 BIGDIST *rexp, BIGDIST *nexp;

    if (is_bf_zero(n))
        {
        bflength <<= 1;
        clear_bf(r);
        bflength >>= 1;
        return r;
        }

    dbfl = 2*bflength; /* double width bflength */
    rexp  = (S16 BIGDIST *)(r+dbfl); /* note: 2*bflength */
    nexp = (S16 BIGDIST *)(n+bflength);
    big_setS16(rexp, 2 * big_accessS16(nexp));

    bnl = bnlength;
    bnlength = bflength;
    unsafe_full_square_bn(r, n);
    bnlength = bnl;

    /* handle normalizing full mult on individual basis */

    return r;
    }


/************************************************************************/
/* r = n^2                                                              */
/*   because of the symmetry involved, n^2 is much faster than n*n      */
/*   for a bignumber of length l                                        */
/*      n*n takes l^2 multiplications                                   */
/*      n^2 takes (l^2+l)/2 multiplications                             */
/*          which is about 1/2 n*n as l gets large                      */
/*  uses the fact that (a+b+c+...)^2 = (a^2+b^2+c^2+...)+2(ab+ac+bc+...)*/
/*                                                                      */
/* Note: r will be of length rlength                                    */
/*       2*bflength >= rlength > bflength                               */
/* SIDE-EFFECTS: n is changed to its absolute value                     */
bf_t unsafe_square_bf(bf_t r, bf_t n)
    {
    int bnl, bfl, rl;
    int rexp;
    S16 BIGDIST *nexp;

    if (is_bf_zero(n))
        {
        clear_bf(r);
        return r;
        }

    nexp = (S16 BIGDIST *)(n+bflength);
    rexp = (S16)(2 * big_accessS16(nexp));

    bnl = bnlength;
    bnlength = bflength;
    rl = rlength;
    rlength = rbflength;
    unsafe_square_bn(r, n);
    bnlength = bnl;
    rlength = rl;

    bfl = bflength;
    bflength = rbflength;
    big_set16(r+bflength, (S16)(rexp+2)); /* adjust after mult */

    norm_sign_bf(r, 1);
    bflength = bfl;
    memmove(r, r+padding, bflength+2); /* shift back */

    return r;
    }

/********************************************************************/
/* r = n * u  where u is an unsigned integer */
/* SIDE-EFFECTS: n can be "de-normalized" and lose precision */
bf_t unsafe_mult_bf_int(bf_t r, bf_t n, U16 u)
    {
    int positive;
    int bnl;
    S16 BIGDIST *rexp, BIGDIST *nexp;

    rexp = (S16 BIGDIST *)(r+bflength);
    nexp = (S16 BIGDIST *)(n+bflength);
    big_setS16(rexp, big_accessS16(nexp)); /* *rexp = *nexp; */

    positive = !is_bf_neg(n);

/*
if u > 0x00FF, then the integer part of the mantissa will overflow the
2 byte (16 bit) integer size.  Therefore, make adjustment before
multiplication is performed.
*/
    if (u > 0x00FF)
        { /* un-normalize n */
        memmove(n, n+1, bflength-1);  /* this sign extends as well */
        big_setS16(rexp,big_accessS16(rexp)+(S16)1);
        }

    bnl = bnlength;
    bnlength = bflength;
    mult_bn_int(r, n, u);
    bnlength = bnl;

    norm_sign_bf(r, positive);
    return r;
    }

/********************************************************************/
/* r *= u  where u is an unsigned integer */
bf_t mult_a_bf_int(bf_t r, U16 u)
    {
    int positive;
    int bnl;
    S16 BIGDIST *rexp;

    rexp = (S16 BIGDIST *)(r+bflength);
    positive = !is_bf_neg(r);

/*
if u > 0x00FF, then the integer part of the mantissa will overflow the
2 byte (16 bit) integer size.  Therefore, make adjustment before
multiplication is performed.
*/
    if (u > 0x00FF)
        { /* un-normalize n */
        memmove(r, r+1, bflength-1);  /* this sign extends as well */
        big_setS16(rexp,big_accessS16(rexp)+(S16)1);
        }

    bnl = bnlength;
    bnlength = bflength;
    mult_a_bn_int(r, u);
    bnlength = bnl;

    norm_sign_bf(r, positive);
    return r;
    }

/********************************************************************/
/* r = n / u  where u is an unsigned integer */
bf_t unsafe_div_bf_int(bf_t r, bf_t n,  U16 u)
    {
    int bnl;
    S16 BIGDIST *rexp, BIGDIST *nexp;

    if (u == 0) /* division by zero */
        {
        max_bf(r);
        if (is_bf_neg(n))
            neg_a_bf(r);
        return r;
        }

    rexp = (S16 BIGDIST *)(r+bflength);
    nexp = (S16 BIGDIST *)(n+bflength);
    big_setS16(rexp, big_accessS16(nexp)); /* *rexp = *nexp; */

    bnl = bnlength;
    bnlength = bflength;
    unsafe_div_bn_int(r, n, u);
    bnlength = bnl;

    norm_bf(r);
    return r;
    }

/********************************************************************/
/* r /= u  where u is an unsigned integer */
bf_t div_a_bf_int(bf_t r, U16 u)
    {
    int bnl;

    if (u == 0) /* division by zero */
        {
        if (is_bf_neg(r))
            {
            max_bf(r);
            neg_a_bf(r);
            }
        else
            {
            max_bf(r);
            }
        return r;
        }

    bnl = bnlength;
    bnlength = bflength;
    div_a_bn_int(r, u);
    bnlength = bnl;

    norm_bf(r);
    return r;
    }

/********************************************************************/
/* extracts the mantissa and exponent of f                          */
/* finds m and n such that 1<=|m|<b and f = m*b^n                   */
/* n is stored in *exp_ptr and m is returned, sort of like frexp()  */
LDBL extract_value(LDBL f, LDBL b, int *exp_ptr)
   {
   int n;
   LDBL af, ff, orig_b;
   LDBL value[15];
   unsigned powertwo;

   if (b <= 0 || f == 0)
      {
      *exp_ptr = 0;
      return 0;
      }

   orig_b = b;
   af = f >= 0 ? f: -f;     /* abs value */
   ff = af > 1 ? af : 1/af;
   n = 0;
   powertwo = 1;
   while (b < ff)
      {
      value[n] = b;
      n++;
      powertwo <<= 1;
      b *= b;
      }

   *exp_ptr = 0;
   for (; n > 0; n--)
      {
      powertwo >>= 1;
      if (value[n-1] < ff)
         {
         ff /= value[n-1];
         *exp_ptr += powertwo;
         }
      }
   if (f < 0)
      ff = -ff;
   if (af < 1)
      {
      ff = orig_b/ff;
      *exp_ptr = -*exp_ptr - 1;
      }

   return ff;
   }

/********************************************************************/
/* calculates and returns the value of f*b^n                        */
/* sort of like ldexp()                                             */
LDBL scale_value( LDBL f, LDBL b , int n )
   {
   LDBL total=1;
   int an;

   if (b == 0 || f == 0)
      return 0;

   if (n == 0)
      return f;

   an = abs(n);

   while (an != 0)
      {
      if (an & 0x0001)
         total *= b;
      b *= b;
      an >>= 1;
      }

   if (n > 0)
      f *= total;
   else /* n < 0 */
      f /= total;
   return f;
   }

/********************************************************************/
/* extracts the mantissa and exponent of f                          */
/* finds m and n such that 1<=|m|<10 and f = m*10^n                 */
/* n is stored in *exp_ptr and m is returned, sort of like frexp()  */
LDBL extract_10(LDBL f, int *exp_ptr)
   {
   return extract_value(f, 10, exp_ptr);
   }

/********************************************************************/
/* calculates and returns the value of f*10^n                       */
/* sort of like ldexp()                                             */
LDBL scale_10( LDBL f, int n )
   {
   return scale_value( f, 10, n );
   }



/* big10flt.c - C routines for base 10 big floating point numbers */

/**********************************************************
(Just when you thought it was safe to go back in the water.)
Just when you thought you seen every type of format possible,
16 bit integer, 32 bit integer, double, long double, mpmath,
bn_t, bf_t, I now give you bf10_t (big float base 10)!

Why, because this is the only way (I can think of) to properly do a
bftostr() without rounding errors.  With out this, then
   -1.9999999999( > LDBL_DIG of 9's)9999999123456789...
will round to -2.0.  The good news is that we only need to do two
mathematical operations: multiplication and division by integers

bf10_t format: (notice the position of the MSB and LSB)

MSB                                         LSB
  _  _  _  _  _  _  _  _  _  _  _  _ _ _ _ _
n <><------------- dec --------------><> <->
  1 byte pad            1 byte rounding   2 byte exponent.

  total length = dec + 4

***********************************************************/

/**********************************************************************/
/* unsafe_bftobf10() - converts a bigfloat into a bigfloat10          */
/*   n - pointer to a bigfloat                                        */
/*   r - result array of BYTE big enough to hold the bf10_t number    */
/* dec - number of decimals, not including the one extra for rounding */
/*  SIDE-EFFECTS: n is changed to |n|.  Make copy of n if necessary.  */

bf10_t unsafe_bftobf10(bf10_t r, int dec, bf_t n)
   {
   int d;
   int power256;
   int p;
   int bnl;
   bf_t onesbyte;
   bf10_t power10;

   if (is_bf_zero(n))
      { /* in scientific notation, the leading digit can't be zero */
      r[1] = (BYTE)0; /* unless the number is zero */
      return r;
      }

   onesbyte = n + bflength - 1;           /* really it's n+bflength-2 */
   power256 = (S16)big_access16(n + bflength) + 1; /* so adjust power256 by 1  */

   if (dec == 0)
       dec = decimals;
   dec++;  /* one extra byte for rounding */
   power10 = r + dec + 1;

   if (is_bf_neg(n))
      {
      neg_a_bf(n);
      r[0] = 1; /* sign flag */
      }
   else
      r[0] = 0;

   p = -1;  /* multiply by 10 right away */
   bnl = bnlength;
   bnlength = bflength;
   for (d=1; d<=dec; d++)
      {
      /* pretend it's a bn_t instead of a bf_t */
      /* this leaves n un-normalized, which is what we want here  */
      mult_a_bn_int(n, 10);

      r[d] = *onesbyte;
      if (d == 1 && r[d] == 0)
         {
         d = 0; /* back up a digit */
         p--; /* and decrease by a factor of 10 */
         }
      *onesbyte = 0;
      }
   bnlength = bnl;
   big_set16(power10, (U16)p); /* save power of ten */

   /* the digits are all read in, now scale it by 256^power256 */
   if (power256 > 0)
      for (d=0; d<power256; d++)
         mult_a_bf10_int(r, dec, 256);

   else if (power256 < 0)
      for (d=0; d>power256; d--)
         div_a_bf10_int(r, dec, 256);

   /* else power256 is zero, don't do anything */

   /* round the last digit */
   if (r[dec] >= 5)
      {
      d = dec-1;
      while (d > 0) /* stop before you get to the sign flag */
         {
         r[d]++;  /* round up */
         if (r[d] < 10)
            {
            d = -1; /* flag for below */
            break; /* finished rounding */
            }
         r[d] = 0;
         d--;
         }
      if (d == 0) /* rounding went back to the first digit and it overflowed */
         {
         r[1] = 0;
         memmove(r+2, r+1, dec-1);
         r[1] = 1;
         p = (S16)big_access16(power10);
         big_set16(power10, (U16)(p+1));
         }
      }
   r[dec] = 0; /* truncate the rounded digit */

   return r;
   }


/**********************************************************************/
/* mult_a_bf10_int()                                                  */
/* r *= n                                                             */
/* dec - number of decimals, including the one extra for rounding */

bf10_t mult_a_bf10_int(bf10_t r, int dec, U16 n)
   {
   int signflag;
   int d, p;
   unsigned value, overflow;
   bf10_t power10;

   if (r[1] == 0 || n == 0)
      {
      r[1] = 0;
      return r;
      }

   power10 = r + dec + 1;
   p = (S16)big_access16(power10);

   signflag = r[0];  /* r[0] to be used as a padding */
   overflow = 0;
   for (d = dec; d>0; d--)
      {
      value = r[d] * n + overflow;
      r[d] = (BYTE)(value % 10);
      overflow = value / 10;
      }
   while (overflow)
      {
      p++;
      memmove(r+2, r+1, dec-1);
      r[1] = (BYTE)(overflow % 10);
      overflow = overflow / 10;
      }
   big_set16(power10, (U16)p); /* save power of ten */
   r[0] = (BYTE)signflag; /* restore sign flag */
   return r;
   }

/**********************************************************************/
/* div_a_bf10_int()                                                   */
/* r /= n                                                             */
/* dec - number of decimals, including the one extra for rounding */

bf10_t div_a_bf10_int (bf10_t r, int dec, U16 n)
   {
   int src, dest, p;
   unsigned value, remainder;
   bf10_t power10;

   if (r[1] == 0 || n == 0)
      {
      r[1] = 0;
      return r;
      }

   power10 = r + dec + 1;
   p = (S16)big_access16(power10);

   remainder = 0;
   for (src=dest=1; src<=dec; dest++, src++)
      {
      value = 10*remainder + r[src];
      r[dest] = (BYTE)(value / n);
      remainder = value % n;
      if (dest == 1 && r[dest] == 0)
         {
         dest = 0; /* back up a digit */
         p--;      /* and decrease by a factor of 10 */
         }
      }
   for (; dest<=dec; dest++)
      {
      value = 10*remainder;
      r[dest] = (BYTE)(value / n);
      remainder = value % n;
      if (dest == 1 && r[dest] == 0)
         {
         dest = 0; /* back up a digit */
         p--;      /* and decrease by a factor of 10 */
         }
      }

   big_set16(power10, (U16)p); /* save power of ten */
   return r;
   }


/*************************************************************************/
/* bf10tostr_e()                                                         */
/* Takes a bf10 number and converts it to an ascii string, sci. notation */
/* dec - number of decimals, not including the one extra for rounding    */

char *bf10tostr_e(char *s, int dec, bf10_t n)
   {
   int d, p;
   bf10_t power10;

   if (n[1] == 0)
      {
      strcpy(s, "0.0");
      return s;
      }

   if (dec == 0)
       dec = decimals;
   dec++;  /* one extra byte for rounding */
   power10 = n + dec + 1;
   p = (S16)big_access16(power10);

   /* if p is negative, it is not necessary to show all the decimal places */
   if (p < 0 && dec > 8) /* 8 sounds like a reasonable value */
      {
      dec = dec + p;
      if (dec < 8) /* let's keep at least a few */
         dec = 8;
      }

   if (n[0] == 1) /* sign flag */
      *(s++) = '-';
   *(s++) = (char)(n[1] + '0');
   *(s++) = '.';
   for (d=2; d<=dec; d++)
      {
      *(s++) = (char)(n[d] + '0');
      }
   /* clean up trailing 0's */
   while (*(s-1) == '0')
      s--;
   if (*(s-1) == '.') /* put at least one 0 after the decimal */
      *(s++) = '0';
   sprintf(s, "e%d", p);
   return s;
   }

/****************************************************************************/
/* bf10tostr_f()                                                            */
/* Takes a bf10 number and converts it to an ascii string, decimal notation */

char *bf10tostr_f(char *s, int dec, bf10_t n)
   {
   int d, p;
   bf10_t power10;

   if (n[1] == 0)
      {
      strcpy(s, "0.0");
      return s;
      }

   if (dec == 0)
       dec = decimals;
   dec++;  /* one extra byte for rounding */
   power10 = n + dec + 1;
   p = (S16)big_access16(power10);

   /* if p is negative, it is not necessary to show all the decimal places */
   if (p < 0 && dec > 8) /* 8 sounds like a reasonable value */
      {
      dec = dec + p;
      if (dec < 8) /* let's keep at least a few */
         dec = 8;
      }

   if (n[0] == 1) /* sign flag */
      *(s++) = '-';
   if (p >= 0)
      {
      for (d=1; d<=p+1; d++)
         *(s++) = (char)(n[d] + '0');
      *(s++) = '.';
      for (; d<=dec; d++)
         *(s++) = (char)(n[d] + '0');
      }
   else
      {
      *(s++) = '0';
      *(s++) = '.';
      for (d=0; d>p+1; d--)
         *(s++) = '0';
      for (d=1; d<=dec; d++)
         *(s++) = (char)(n[d] + '0');
      }

   /* clean up trailing 0's */
   while (*(s-1) == '0')
      s--;
   if (*(s-1) == '.') /* put at least one 0 after the decimal */
      *(s++) = '0';
   *s = '\0'; /* terminating nul */
   return s;
   }
