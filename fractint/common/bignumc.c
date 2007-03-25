/* bignumc.c - C routines equivalent to ASM routines in bignuma.asm */

/*
Wesley Loewer's Big Numbers.        (C) 1994-95, Wesley B. Loewer
*/

#include <memory.h>
/* see Fractint.c for a description of the "include"  hierarchy */
#include "port.h"
#include "big.h"

/********************************************************************
 The following code contains the C versions of the routines from the
 file BIGNUMA.ASM.  It is provided here for portibility and for clarity.
*********************************************************************/

/********************************************************************
 Note:  The C code must be able to detect over/underflow.  Therefore
 32 bit integers must be used when doing 16 bit math.  All we really
 need is one more bit, such as is provided in asm with the carry bit.
 Functions that don't need the test for over/underflow, such as cmp_bn()
 and is_bn_not_zero(), can use 32 bit integers as as long as bnstep
 is set to 4.

 The 16/32 bit compination of integer sizes could be increased to
 32/64 bit to improve efficiency, but since many compilers don't offer
 64 bit integers, this option was not included.

*********************************************************************/

/********************************************************************/
/* r = 0 */
bn_t clear_bn(bn_t r)
{
#ifdef BIG_BASED
	_fmemset(r, 0, bnlength); /* set array to zero */
#else
#ifdef BIG_FAR
	_fmemset(r, 0, bnlength); /* set array to zero */
#else
	memset(r, 0, bnlength); /* set array to zero */
#endif
#endif
	return r;
}

/********************************************************************/
/* r = max positive value */
bn_t max_bn(bn_t r)
{
#ifdef BIG_BASED
	_fmemset(r, 0xFF, bnlength-1); /* set to max values */
#else
#ifdef BIG_FAR
	_fmemset(r, 0xFF, bnlength-1); /* set to max values */
#else
	memset(r, 0xFF, bnlength-1); /* set to max values */
#endif
#endif
	r[bnlength-1] = 0x7F;  /* turn off the sign bit */
	return r;
}

/********************************************************************/
/* r = n */
bn_t copy_bn(bn_t r, bn_t n)
{
#ifdef BIG_BASED
	_fmemcpy(r, n, bnlength);
#else
#ifdef BIG_FAR
	_fmemcpy(r, n, bnlength);
#else
	memcpy(r, n, bnlength);
#endif
#endif
	return r;
}

/***************************************************************************/
/* n1 != n2 ?                                                              */
/* RETURNS:                                                                */
/*  if n1 == n2 returns 0                                                  */
/*  if n1 > n2 returns a positive (bytes left to go when mismatch occured) */
/*  if n1 < n2 returns a negative (bytes left to go when mismatch occured) */
int cmp_bn(bn_t n1, bn_t n2)
{
	int i;
	S16 Svalue1, Svalue2;
	U16 value1, value2;

	/* two bytes at a time */
	/* signed comparison for msb */
	if ((Svalue1=big_accessS16((S16 BIGDIST *)(n1 + bnlength-2))) >
			(Svalue2=big_accessS16((S16 BIGDIST *)(n2 + bnlength-2))))
	{ /* now determine which of the two bytes was different */
		if ((S16)(Svalue1&0xFF00) > (S16)(Svalue2&0xFF00)) /* compare just high bytes */
		{
			return bnlength; /* high byte was different */
		}
		else
		{
			return bnlength-1; /* low byte was different */
		}
	}
	else if (Svalue1 < Svalue2)
	{ /* now determine which of the two bytes was different */
		if ((S16)(Svalue1&0xFF00) < (S16)(Svalue2&0xFF00)) /* compare just high bytes */
		{
			return -(bnlength); /* high byte was different */
		}
		else
		{
			return -(bnlength-1); /* low byte was different */
		}
	}

	/* unsigned comparison for the rest */
	for (i=bnlength-4; i >= 0; i -= 2)
	{
		value1 = big_access16(n1 + i);
		value2 = big_access16(n2 + i);
		if (value1 > value2)
		{ /* now determine which of the two bytes was different */
			if ((value1&0xFF00) > (value2&0xFF00)) /* compare just high bytes */
			{
				return i + 2; /* high byte was different */
			}
			else
			{
				return i + 1; /* low byte was different */
			}
		}
		else if (value1 < value2)
		{ /* now determine which of the two bytes was different */
			if ((value1&0xFF00) < (value2&0xFF00)) /* compare just high bytes */
			{
				return -(i + 2); /* high byte was different */
			}
			else
			{
				return -(i + 1); /* low byte was different */
			}
		}
	}
	return 0;
}

/********************************************************************/
/* r < 0 ?                                      */
/* returns 1 if negative, 0 if positive or zero */
int is_bn_neg(bn_t n)
{
	return (S8)n[bnlength-1] < 0;
}

/********************************************************************/
/* n != 0 ?                      */
/* RETURNS: if n != 0 returns 1  */
/*          else returns 0       */
int is_bn_not_zero(bn_t n)
{
	int i;

	/* two bytes at a time */
	for (i = 0; i < bnlength; i += 2)
		if (big_access16(n + i) != 0)
		{
			return 1;
		}
	return 0;
}

/********************************************************************/
/* r = n1 + n2 */
bn_t add_bn(bn_t r, bn_t n1, bn_t n2)
{
	int i;
	U32 sum = 0;

	/* two bytes at a time */
	for (i = 0; i < bnlength; i += 2)
	{
		sum += (U32)big_access16(n1 + i) + (U32)big_access16(n2 + i); /* add 'em up */
		big_set16(r + i, (U16)sum);   /* store the lower 2 bytes */
		sum >>= 16; /* shift the overflow for next time */
	}
	return r;
}

/********************************************************************/
/* r += n */
bn_t add_a_bn(bn_t r, bn_t n)
{
	int i;
	U32 sum = 0;

	/* two bytes at a time */
	for (i = 0; i < bnlength; i += 2)
	{
		sum += (U32)big_access16(r + i) + (U32)big_access16(n + i); /* add 'em up */
		big_set16(r + i, (U16)sum);   /* store the lower 2 bytes */
		sum >>= 16; /* shift the overflow for next time */
	}
	return r;
}

/********************************************************************/
/* r = n1 - n2 */
bn_t sub_bn(bn_t r, bn_t n1, bn_t n2)
{
	int i;
	U32 diff = 0;

	/* two bytes at a time */
	for (i = 0; i < bnlength; i += 2)
	{
		diff = (U32)big_access16(n1 + i) - ((U32)big_access16(n2 + i)-(S32)(S16)diff); /* subtract with borrow */
		big_set16(r + i, (U16)diff);   /* store the lower 2 bytes */
		diff >>= 16; /* shift the underflow for next time */
	}
	return r;
}

/********************************************************************/
/* r -= n */
bn_t sub_a_bn(bn_t r, bn_t n)
{
	int i;
	U32 diff = 0;

	/* two bytes at a time */
	for (i = 0; i < bnlength; i += 2)
	{
		diff = (U32)big_access16(r + i) - ((U32)big_access16(n + i)-(S32)(S16)diff); /* subtract with borrow */
		big_set16(r + i, (U16)diff);   /* store the lower 2 bytes */
		diff >>= 16; /* shift the underflow for next time */
	}
	return r;
}

/********************************************************************/
/* r = -n */
bn_t neg_bn(bn_t r, bn_t n)
{
	int i;
	U16 t_short;
	U32 neg = 1; /* to get the 2's complement started */

	/* two bytes at a time */
	for (i = 0; neg != 0 && i < bnlength; i += 2)
	{
		t_short = ~big_access16(n + i);
		neg += ((U32)t_short); /* two's complement */
		big_set16(r + i, (U16)neg);   /* store the lower 2 bytes */
		neg >>= 16; /* shift the sign bit for next time */
	}
	/* if neg was 0, then just "not" the rest */
	for (; i < bnlength; i += 2)
	{ /* notice that big_access16() and big_set16() are not needed here */
		*(U16 BIGDIST *)(r + i) = ~*(U16 BIGDIST *)(n + i); /* toggle all the bits */
	}
	return r;
}

/********************************************************************/
/* r *= -1 */
bn_t neg_a_bn(bn_t r)
{
	int i;
	U16 t_short;
	U32 neg = 1; /* to get the 2's complement started */

	/* two bytes at a time */
	for (i = 0; neg != 0 && i < bnlength; i += 2)
	{
		t_short = ~big_access16(r + i);
		neg += ((U32)t_short); /* two's complement */
		big_set16(r + i, (U16)neg);   /* store the lower 2 bytes */
		neg >>= 16; /* shift the sign bit for next time */
	}
	/* if neg was 0, then just "not" the rest */
	for (; i < bnlength; i += 2)
	{ /* notice that big_access16() and big_set16() are not needed here */
		*(U16 BIGDIST *)(r + i) = ~*(U16 BIGDIST *)(r + i); /* toggle all the bits */
	}
	return r;
}

/********************************************************************/
/* r = 2*n */
bn_t double_bn(bn_t r, bn_t n)
{
	int i;
	U32 prod = 0;

	/* two bytes at a time */
	for (i = 0; i < bnlength; i += 2)
	{
		prod += (U32)big_access16(n + i) << 1 ; /* double it */
		big_set16(r + i, (U16)prod);   /* store the lower 2 bytes */
		prod >>= 16; /* shift the overflow for next time */
	}
	return r;
}

/********************************************************************/
/* r *= 2 */
bn_t double_a_bn(bn_t r)
{
	int i;
	U32 prod = 0;

	/* two bytes at a time */
	for (i = 0; i < bnlength; i += 2)
	{
		prod += (U32)big_access16(r + i) << 1 ; /* double it */
		big_set16(r + i, (U16)prod);   /* store the lower 2 bytes */
		prod >>= 16; /* shift the overflow for next time */
	}
	return r;
}

/********************************************************************/
/* r = n/2 */
bn_t half_bn(bn_t r, bn_t n)
{
	int i;
	U32 quot = 0;

	/* two bytes at a time */

	/* start with an arithmetic shift */
	i=bnlength-2;
	quot += (U32)(((S32)(S16)big_access16(n + i) << 16) >> 1) ; /* shift to upper 2 bytes and half it */
	big_set16(r + i, (U16)(quot >> 16));   /* store the upper 2 bytes */
	quot <<= 16; /* shift the underflow for next time */

	for (i=bnlength-4; i >= 0; i -= 2)
	{
		/* looks wierd, but properly sign extends argument */
		quot += (U32)(((U32)big_access16(n + i) << 16) >> 1) ; /* shift to upper 2 bytes and half it */
		big_set16(r + i, (U16)(quot >> 16));   /* store the upper 2 bytes */
		quot <<= 16; /* shift the underflow for next time */
	}

	return r;
}

/********************************************************************/
/* r /= 2 */
bn_t half_a_bn(bn_t r)
{
	int i;
	U32 quot = 0;

	/* two bytes at a time */

	/* start with an arithmetic shift */
	i=bnlength-2;
	quot += (U32)(((S32)(S16)big_access16(r + i) << 16) >> 1) ; /* shift to upper 2 bytes and half it */
	big_set16(r + i, (U16)(quot >> 16));   /* store the upper 2 bytes */
	quot <<= 16; /* shift the underflow for next time */

	for (i=bnlength-4; i >= 0; i -= 2)
	{
		/* looks wierd, but properly sign extends argument */
		quot += (U32)(((U32)(U16)big_access16(r + i) << 16) >> 1) ; /* shift to upper 2 bytes and half it */
		big_set16(r + i, (U16)(quot >> 16));   /* store the upper 2 bytes */
		quot <<= 16; /* shift the underflow for next time */
	}
	return r;
}

/************************************************************************/
/* r = n1*n2                                                          */
/* Note: r will be a double wide result, 2*bnlength                     */
/*       n1 and n2 can be the same pointer                              */
/* SIDE-EFFECTS: n1 and n2 are changed to their absolute values         */
bn_t unsafe_full_mult_bn(bn_t r, bn_t n1, bn_t n2)
{
	int sign1, sign2 = 0, samevar;
	int i, j, k, steps, doublesteps, carry_steps;
	bn_t n1p, n2p;      /* pointers for n1, n2 */
	bn_t rp1, rp2, rp3; /* pointers for r */
	U32 prod, sum;

	sign1 = is_bn_neg(n1);
	if (sign1 != 0) /* =, not == */
	{
		neg_a_bn(n1);
	}
	samevar = (n1 == n2);
	if (!samevar) /* check to see if they're the same pointer */
	{
		sign2 = is_bn_neg(n2);
		if (sign2 != 0) /* =, not == */
		{
			neg_a_bn(n2);
		}
	}

	n1p = n1;
	steps = bnlength >> 1; /* two bytes at a time */
	carry_steps = doublesteps = (steps << 1) - 2;
	bnlength <<= 1;
	clear_bn(r);        /* double width */
	bnlength >>= 1;
	rp1 = rp2 = r;
	for (i = 0; i < steps; i++)
	{
		n2p = n2;
		for (j = 0; j < steps; j++)
		{
			prod = (U32)big_access16(n1p)*(U32)big_access16(n2p); /* U16*U16=U32 */
			sum = (U32)big_access16(rp2) + prod; /* add to previous, including overflow */
			big_set16(rp2, (U16)sum); /* save the lower 2 bytes */
			sum >>= 16;             /* keep just the upper 2 bytes */
			rp3 = rp2 + 2;          /* move over 2 bytes */
			sum += big_access16(rp3);     /* add what was the upper two bytes */
			big_set16(rp3 ,(U16)sum); /* save what was the upper two bytes */
			sum >>= 16;             /* keep just the overflow */
			for (k = 0; sum != 0 && k < carry_steps; k++)
			{
				rp3 += 2;               /* move over 2 bytes */
				sum += big_access16(rp3);     /* add to what was the overflow */
				big_set16(rp3, (U16)sum); /* save what was the overflow */
				sum >>= 16;             /* keep just the new overflow */
			}
			n2p += 2;       /* to next word */
			rp2 += 2;
			carry_steps--;  /* use one less step */
		}
		n1p += 2;           /* to next word */
		rp2 = rp1 += 2;
		carry_steps = --doublesteps; /* decrease doubles steps and reset carry_steps */
	}

	/* if they were the same or same sign, the product must be positive */
	if (!samevar && sign1 != sign2)
	{
		bnlength <<= 1;         /* for a double wide number */
		neg_a_bn(r);
		bnlength >>= 1; /* restore bnlength */
	}
	return r;
}

/************************************************************************/
/* r = n1*n2 calculating only the top rlength bytes                   */
/* Note: r will be of length rlength                                    */
/*       2*bnlength <= rlength < bnlength                               */
/*       n1 and n2 can be the same pointer                              */
/* SIDE-EFFECTS: n1 and n2 are changed to their absolute values         */
bn_t unsafe_mult_bn(bn_t r, bn_t n1, bn_t n2)
{
	int sign1, sign2 = 0, samevar;
	int i, j, k, steps, doublesteps, carry_steps, skips;
	bn_t n1p, n2p;      /* pointers for n1, n2 */
	bn_t rp1, rp2, rp3; /* pointers for r */
	U32 prod, sum;
	int bnl; /* temp bnlength holder */

	bnl = bnlength;
	sign1 = is_bn_neg(n1);
	if (sign1 != 0) /* =, not == */
	{
		neg_a_bn(n1);
	}
	samevar = (n1 == n2);
	if (!samevar) /* check to see if they're the same pointer */
	{
		sign2 = is_bn_neg(n2);
		if (sign2 != 0) /* =, not == */
		{
			neg_a_bn(n2);
		}
	}
	n1p = n1;
	n2 += (bnlength << 1) - rlength;  /* shift n2 over to where it is needed */

	bnlength = rlength;
	clear_bn(r);        /* zero out r, rlength width */
	bnlength = bnl;

	steps = (rlength-bnlength) >> 1;
	skips = (bnlength >> 1) - steps;
	carry_steps = doublesteps = (rlength >> 1)-2;
	rp2 = rp1 = r;
	for (i=bnlength >> 1; i > 0; i--)
	{
		n2p = n2;
		for (j = 0; j < steps; j++)
		{
			prod = (U32)big_access16(n1p)*(U32)big_access16(n2p); /* U16*U16=U32 */
			sum = (U32)big_access16(rp2) + prod; /* add to previous, including overflow */
			big_set16(rp2, (U16)sum); /* save the lower 2 bytes */
			sum >>= 16;             /* keep just the upper 2 bytes */
			rp3 = rp2 + 2;          /* move over 2 bytes */
			sum += big_access16(rp3);     /* add what was the upper two bytes */
			big_set16(rp3, (U16)sum); /* save what was the upper two bytes */
			sum >>= 16;             /* keep just the overflow */
			for (k = 0; sum != 0 && k < carry_steps; k++)
			{
				rp3 += 2;               /* move over 2 bytes */
				sum += big_access16(rp3);     /* add to what was the overflow */
				big_set16(rp3, (U16)sum); /* save what was the overflow */
				sum >>= 16;             /* keep just the new overflow */
			}
			n2p += 2;                   /* increase by two bytes */
			rp2 += 2;
			carry_steps--;
		}
		n1p += 2;   /* increase by two bytes */

		if (skips != 0)
		{
			n2 -= 2;    /* shift n2 back a word */
			steps++;    /* one more step this time */
			skips--;    /* keep track of how many times we've done this */
		}
		else
		{
			rp1 += 2;           /* shift forward a word */
			doublesteps--;      /* reduce the carry steps needed next time */
		}
		rp2 = rp1;
		carry_steps = doublesteps;
	}

	/* if they were the same or same sign, the product must be positive */
	if (!samevar && sign1 != sign2)
	{
		bnlength = rlength;
		neg_a_bn(r);            /* wider bignumber */
		bnlength = bnl;
	}
	return r;
}

/************************************************************************/
/* r = n^2                                                              */
/*   because of the symetry involved, n^2 is much faster than n*n       */
/*   for a bignumber of length l                                        */
/*      n*n takes l^2 multiplications                                   */
/*      n^2 takes (l^2 + l)/2 multiplications                             */
/*          which is about 1/2 n*n as l gets large                      */
/*  uses the fact that (a + b + c + ...)^2 = (a^2 + b^2 + c^2 + ...) + 2(ab + ac + bc + ...)*/
/*                                                                      */
/* SIDE-EFFECTS: n is changed to its absolute value                     */
bn_t unsafe_full_square_bn(bn_t r, bn_t n)
{
	int i, j, k, steps, doublesteps, carry_steps;
	bn_t n1p, n2p;
	bn_t rp1, rp2, rp3;
	U32 prod, sum;

	if (is_bn_neg(n))  /* don't need to keep track of sign since the */
	{
		neg_a_bn(n);   /* answer must be positive. */
	}

	bnlength <<= 1;
	clear_bn(r);        /* zero out r, double width */
	bnlength >>= 1;

	steps = (bnlength >> 1)-1;
	carry_steps = doublesteps = (steps << 1) - 1;
	rp2 = rp1 = r + 2;  /* start with second two-byte word */
	n1p = n;
	if (steps != 0) /* if zero, then skip all the middle term calculations */
	{
		for (i=steps; i > 0; i--) /* steps gets altered, count backwards */
		{
			n2p = n1p + 2;  /* set n2p pointer to 1 step beyond n1p */
			for (j = 0; j < steps; j++)
			{
				prod = (U32)big_access16(n1p)*(U32)big_access16(n2p); /* U16*U16=U32 */
				sum = (U32)big_access16(rp2) + prod; /* add to previous, including overflow */
				big_set16(rp2, (U16)sum); /* save the lower 2 bytes */
				sum >>= 16;             /* keep just the upper 2 bytes */
				rp3 = rp2 + 2;          /* move over 2 bytes */
				sum += big_access16(rp3);     /* add what was the upper two bytes */
				big_set16(rp3, (U16)sum); /* save what was the upper two bytes */
				sum >>= 16;             /* keep just the overflow */
				for (k = 0; sum != 0 && k < carry_steps; k++)
				{
					rp3 += 2;               /* move over 2 bytes */
					sum += big_access16(rp3);     /* add to what was the overflow */
					big_set16(rp3, (U16)sum); /* save what was the overflow */
					sum >>= 16;             /* keep just the new overflow */
				}
				n2p += 2;       /* increase by two bytes */
				rp2 += 2;
				carry_steps--;
			}
			n1p += 2;           /* increase by two bytes */
			rp2 = rp1 += 4;     /* increase by 2*two bytes */
			carry_steps = doublesteps -= 2;   /* reduce the carry steps needed */
			steps--;
		}
		/* All the middle terms have been multiplied.  Now double it. */
		bnlength <<= 1;     /* double wide bignumber */
		double_a_bn(r);
		bnlength >>= 1;
		/* finished with middle terms */
	}

	/* Now go back and add in the squared terms. */
	n1p = n;
	steps = (bnlength >> 1);
	carry_steps = doublesteps = (steps << 1) - 2;
	rp1 = r;
	for (i = 0; i < steps; i++)
	{
		/* square it */
		prod = (U32)big_access16(n1p)*(U32)big_access16(n1p); /* U16*U16=U32 */
		sum = (U32)big_access16(rp1) + prod; /* add to previous, including overflow */
		big_set16(rp1, (U16)sum); /* save the lower 2 bytes */
		sum >>= 16;             /* keep just the upper 2 bytes */
		rp3 = rp1 + 2;          /* move over 2 bytes */
		sum += big_access16(rp3);     /* add what was the upper two bytes */
		big_set16(rp3, (U16)sum); /* save what was the upper two bytes */
		sum >>= 16;             /* keep just the overflow */
		for (k = 0; sum != 0 && k < carry_steps; k++)
		{
			rp3 += 2;               /* move over 2 bytes */
			sum += big_access16(rp3);     /* add to what was the overflow */
			big_set16(rp3, (U16)sum); /* save what was the overflow */
			sum >>= 16;             /* keep just the new overflow */
		}
		n1p += 2;       /* increase by 2 bytes */
		rp1 += 4;       /* increase by 4 bytes */
		carry_steps = doublesteps -= 2;
	}
	return r;
}


/************************************************************************/
/* r = n^2                                                              */
/*   because of the symetry involved, n^2 is much faster than n*n       */
/*   for a bignumber of length l                                        */
/*      n*n takes l^2 multiplications                                   */
/*      n^2 takes (l^2 + l)/2 multiplications                             */
/*          which is about 1/2 n*n as l gets large                      */
/*  uses the fact that (a + b + c + ...)^2 = (a^2 + b^2 + c^2 + ...) + 2(ab + ac + bc + ...)*/
/*                                                                      */
/* Note: r will be of length rlength                                    */
/*       2*bnlength >= rlength > bnlength                               */
/* SIDE-EFFECTS: n is changed to its absolute value                     */
bn_t unsafe_square_bn(bn_t r, bn_t n)
{
	int i, j, k, steps, doublesteps, carry_steps;
	int skips, rodd;
	bn_t n1p, n2p, n3p;
	bn_t rp1, rp2, rp3;
	U32 prod, sum;
	int bnl;

	/* This whole procedure would be a great deal simpler if we could assume that */
	/* rlength < 2*bnlength (that is, not =).  Therefore, we will take the        */
	/* easy way out and call full_square_bn() if it is.                           */
	if (rlength == (bnlength << 1)) /* rlength == 2*bnlength */
	{
		return unsafe_full_square_bn(r, n);    /* call full_square_bn() and quit */
	}

	if (is_bn_neg(n))  /* don't need to keep track of sign since the */
	{
		neg_a_bn(n);   /* answer must be positive. */
	}

	bnl = bnlength;
	bnlength = rlength;
	clear_bn(r);        /* zero out r, of width rlength */
	bnlength = bnl;

	/* determine whether r is on an odd or even two-byte word in the number */
	rodd = (U16)(((bnlength << 1)-rlength) >> 1) & 0x0001;
	i = (bnlength >> 1)-1;
	steps = (rlength-bnlength) >> 1;
	carry_steps = doublesteps = (bnlength >> 1) + steps-2;
	skips = (i - steps) >> 1;     /* how long to skip over pointer shifts */
	rp2 = rp1 = r;
	n1p = n;
	n3p = n2p = n1p + (((bnlength >> 1)-steps) << 1);    /* n2p = n1p + 2*(bnlength/2 - steps) */
	if (i != 0) /* if zero, skip middle term calculations */
	{
		/* i is already set */
		for (; i > 0; i--)
		{
			for (j = 0; j < steps; j++)
			{
				prod = (U32)big_access16(n1p)*(U32)big_access16(n2p); /* U16*U16=U32 */
				sum = (U32)big_access16(rp2) + prod; /* add to previous, including overflow */
				big_set16(rp2, (U16)sum); /* save the lower 2 bytes */
				sum >>= 16;             /* keep just the upper 2 bytes */
				rp3 = rp2 + 2;          /* move over 2 bytes */
				sum += big_access16(rp3);     /* add what was the upper two bytes */
				big_set16(rp3, (U16)sum); /* save what was the upper two bytes */
				sum >>= 16;             /* keep just the overflow */
				for (k = 0; sum != 0 && k < carry_steps; k++)
				{
					rp3 += 2;               /* move over 2 bytes */
					sum += big_access16(rp3);     /* add to what was the overflow */
					big_set16(rp3, (U16)sum); /* save what was the overflow */
					sum >>= 16;             /* keep just the new overflow */
				}
				n2p += 2;       /* increase by 2-byte word size */
				rp2 += 2;
				carry_steps--;
			}
			n1p += 2;       /* increase by 2-byte word size */
			if (skips > 0)
			{
				n2p = n3p -= 2;
				steps++;
				skips--;
			}
			else if (skips == 0)    /* only gets executed once */
			{
				steps -= rodd;  /* rodd is 1 or 0 */
				doublesteps -= rodd + 1;
				rp1 += (rodd + 1) << 1;
				n2p = n1p + 2;
				skips--;
			}
			else /* skips < 0 */
			{
				steps--;
				doublesteps -= 2;
				rp1 += 4;           /* add two 2-byte words */
				n2p = n1p + 2;
			}
			rp2 = rp1;
			carry_steps = doublesteps;
		}
		/* All the middle terms have been multiplied.  Now double it. */
		bnlength = rlength;
		double_a_bn(r);
		bnlength = bnl;
	}
	/* Now go back and add in the squared terms. */

	/* be careful, the next dozen or so lines are confusing!       */
	/* determine whether r is on an odd or even word in the number */
	/* using i as a temporary variable here */
	i = (bnlength << 1)-rlength;
	rp1 = r + ((U16)i & (U16)0x0002);
	i = (U16)((i >> 1) + 1) & (U16)0xFFFE;
	n1p = n + i;
	/* i here is no longer a temp var., but will be used as a loop counter */
	i = (bnlength - i) >> 1;
	carry_steps = doublesteps = (i << 1)-2;
	/* i is already set */
	for (; i > 0; i--)
	{
		/* square it */
		prod = (U32)big_access16(n1p)*(U32)big_access16(n1p); /* U16*U16=U32 */
		sum = (U32)big_access16(rp1) + prod; /* add to previous, including overflow */
		big_set16(rp1, (U16)sum); /* save the lower 2 bytes */
		sum >>= 16;             /* keep just the upper 2 bytes */
		rp3 = rp1 + 2;          /* move over 2 bytes */
		sum += big_access16(rp3);     /* add what was the upper two bytes */
		big_set16(rp3, (U16)sum); /* save what was the upper two bytes */
		sum >>= 16;             /* keep just the overflow */
		for (k = 0; sum != 0 && k < carry_steps; k++)
		{
			rp3 += 2;               /* move over 2 bytes */
			sum += big_access16(rp3);     /* add to what was the overflow */
			big_set16(rp3, (U16)sum); /* save what was the overflow */
			sum >>= 16;             /* keep just the new overflow */
		}
		n1p += 2;
		rp1 += 4;
		carry_steps = doublesteps -= 2;
	}
	return r;
}

/********************************************************************/
/* r = n*u  where u is an unsigned integer */
bn_t mult_bn_int(bn_t r, bn_t n, U16 u)
{
	int i;
	U32 prod = 0;

	/* two bytes at a time */
	for (i = 0; i < bnlength; i += 2)
	{
		prod += (U32)big_access16(n + i)*u ; /* n*u */
		big_set16(r + i, (U16)prod);   /* store the lower 2 bytes */
		prod >>= 16; /* shift the overflow for next time */
	}
	return r;
}

/********************************************************************/
/* r *= u  where u is an unsigned integer */
bn_t mult_a_bn_int(bn_t r, U16 u)
{
	int i;
	U32 prod = 0;

	/* two bytes at a time */
	for (i = 0; i < bnlength; i += 2)
	{
		prod += (U32)big_access16(r + i)*u ; /* r*u */
		big_set16(r + i, (U16)prod);   /* store the lower 2 bytes */
		prod >>= 16; /* shift the overflow for next time */
	}
	return r;
}

/********************************************************************/
/* r = n / u  where u is an unsigned integer */
bn_t unsafe_div_bn_int(bn_t r, bn_t n,  U16 u)
{
	int i, sign;
	U32 full_number;
	U16 quot, rem = 0;

	sign = is_bn_neg(n);
	if (sign)
	{
		neg_a_bn(n);
	}

	if (u == 0) /* division by zero */
	{
		max_bn(r);
		if (sign)
		{
			neg_a_bn(r);
		}
		return r;
	}

	/* two bytes at a time */
	for (i=bnlength-2; i >= 0; i -= 2)
	{
		full_number = ((U32)rem << 16) + (U32)big_access16(n + i);
		quot = (U16)(full_number / u);
		rem  = (U16)(full_number % u);
		big_set16(r + i, quot);
	}

	if (sign)
	{
		neg_a_bn(r);
	}
	return r;
}

/********************************************************************/
/* r /= u  where u is an unsigned integer */
bn_t div_a_bn_int(bn_t r, U16 u)
{
	int i, sign;
	U32 full_number;
	U16 quot, rem = 0;

	sign = is_bn_neg(r);
	if (sign)
	{
		neg_a_bn(r);
	}

	if (u == 0) /* division by zero */
	{
		max_bn(r);
		if (sign)
		{
			neg_a_bn(r);
		}
		return r;
	}

	/* two bytes at a time */
	for (i=bnlength-2; i >= 0; i -= 2)
	{
		full_number = ((U32)rem << 16) + (U32)big_access16(r + i);
		quot = (U16)(full_number / u);
		rem  = (U16)(full_number % u);
		big_set16(r + i, quot);
	}

	if (sign)
	{
		neg_a_bn(r);
	}
	return r;
}

/*********************************************************************/
/*  f = b                                                            */
/*  Converts a bignumber to a double                                 */
LDBL bntofloat(bn_t n)
{
	int i;
	int signflag = 0;
	int expon;
	bn_t getbyte;
	LDBL f = 0;

	if (is_bn_neg(n))
	{
		signflag = 1;
		neg_a_bn(n);
	}

	expon = intlength - 1;
	getbyte = n + bnlength - 1;
	while (*getbyte == 0 && getbyte >= n)
	{
		getbyte--;
		expon--;
	}

	/* There is no need to use all bnlength bytes.  To get the full */
	/* precision of LDBL, all you need is LDBL_MANT_DIG/8 + 1.        */
	for (i = 0; i < (LDBL_MANT_DIG/8 + 1) && getbyte >= n; i++, getbyte--)
	{
		f += scale_256(*getbyte,-i);
	}

	f = scale_256(f,expon);

	if (signflag)
	{
		f = -f;
		neg_a_bn(n);
	}
	return f;
}

/*****************************************/
/* the following used to be in bigfltc.c */

/********************************************************************/
/* r = 0 */
bf_t clear_bf(bf_t r)
{
	memset(r, 0, bflength + 2); /* set array to zero */
	return r;
}

/********************************************************************/
/* r = n */
bf_t copy_bf(bf_t r, bf_t n)
{
	memcpy(r, n, bflength + 2);
	return r;
}

/*********************************************************************/
/*  b = f                                                            */
/*  Converts a double to a bigfloat                                  */
bf_t floattobf(bf_t r, LDBL f)
{
	int power;
	int bnl, il;
	if (f == 0)
	{
		clear_bf(r);
		return r;
	}

	/* remove the exp part */
	f = extract_256(f, &power);

	bnl = bnlength;
	bnlength = bflength;
	il = intlength;
	intlength = 2;
	floattobn(r, f);
	bnlength = bnl;
	intlength = il;

	big_set16(r + bflength, (S16)power); /* exp */

	return r;
}

/*********************************************************************/
/*  b = f                                                            */
/*  Converts a double to a bigfloat                                  */
bf_t floattobf1(bf_t r, LDBL f)
{
	char msg[80];
#ifdef USE_LONG_DOUBLE
	sprintf(msg,"%-.22Le", f);
#else
	sprintf(msg,"%-.22le", f);
#endif
	strtobf(r,msg);
	return r;
}

/*********************************************************************/
/*  f = b                                                            */
/*  Converts a bigfloat to a double                                 */
LDBL bftofloat(bf_t n)
{
	int power;
	int bnl, il;
	LDBL f;

	bnl = bnlength;
	bnlength = bflength;
	il = intlength;
	intlength = 2;
	f = bntofloat(n);
	bnlength = bnl;
	intlength = il;

	power = (S16)big_access16(n + bflength);
	f = scale_256(f,power);

	return f;
}

/********************************************************************/
/* extracts the mantissa and exponent of f                          */
/* finds m and n such that 1 <= |m| < 256 and f = m*256^n               */
/* n is stored in *exp_ptr and m is returned, sort of like frexp()  */
LDBL extract_256(LDBL f, int *exp_ptr)
{
	return extract_value(f, 256, exp_ptr);
}

/********************************************************************/
/* calculates and returns the value of f*256^n                      */
/* sort of like ldexp()                                             */
/*                                                                  */
/* n must be in the range -2^12 <= n < 2^12 (2^12=4096),            */
/* which should not be a problem                                    */
LDBL scale_256(LDBL f, int n)
{
	return scale_value(f, 256, n);
}
