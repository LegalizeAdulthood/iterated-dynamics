/*
 *  REMOVED FORMAL PARAMETERS FROM FUNCTION DEFINITIONS (1/4/92)
 */


#ifndef MPMATH_H
#define MPMATH_H

#ifndef _CMPLX_DEFINED
#include "cmplx.h"
#endif

#ifdef XFRACT
#define far
#endif


#if !defined(XFRACT)
struct MP
{
	int Exp;
	unsigned long Mant;
};
#else
struct MP {
   double val;
};
#endif

struct MPC
{
	struct MP x, y;
};

extern int g_overflow_mp;
extern int DivideOverflow;

/* Mark Peterson's expanded floating point operators.  Automatically uses
   either the 8086 or 80386 processor type specified in global 'g_cpu'. If
   the operation results in an overflow (result < 2**(2**14), or division
   by zero) the global 'MPoverflow' is set to one. */

/* function pointer support added by Tim Wegner 12/07/89 */
extern int         (*pMPcmp)(struct MP , struct MP );
extern struct MP  *(*pMPmul)(struct MP , struct MP );
extern struct MP  *(*pMPdiv)(struct MP , struct MP );
extern struct MP  *(*pMPadd)(struct MP , struct MP );
extern struct MP  *(*pMPsub)(struct MP , struct MP );
extern struct MP  *(*pd2MP)(double )                ;
extern double     *(*pMP2d)(struct MP )             ;


/*** Formula Declarations ***/
#if !defined(XFRACT)
enum MATH_TYPE { D_MATH, M_MATH, L_MATH };
#else
enum MATH_TYPE { D_MATH};
#endif
extern enum MATH_TYPE MathType;

#define fDiv(x, y, z) (void)((*(long*)&z) = RegDivFloat(*(long*)&x, *(long*)&y))
#define fMul16(x, y, z) (void)((*(long*)&z) = r16Mul(*(long*)&x, *(long*)&y))
#define fShift(x, Shift, z) (void)((*(long*)&z) = \
   RegSftFloat(*(long*)&x, Shift))
#define Fg2Float(x, f, z) (void)((*(long*)&z) = RegFg2Float(x, f))
#define Float2Fg(x, f) RegFloat2Fg(*(long*)&x, f)
#define fLog14(x, z) (void)((*(long*)&z) = \
        RegFg2Float(LogFloat14(*(long*)&x), 16))
#define fExp14(x, z) (void)((*(long*)&z) = ExpFloat14(*(long*)&x));
#define fSqrt14(x, z) fLog14(x, z); fShift(z, -1, z); fExp14(z, z)

/* the following are declared 4 dimensional as an experiment */
/* changeing declarations to _CMPLX and _LCMPLX restores the code */
/* to 2D */
union Arg
{
   _CMPLX     d;
   struct MPC m;
   _LCMPLX    l;
/*
   _DHCMPLX   dh;
   _LHCMPLX   lh; */
};

struct ConstArg
{
   char *s;
   int len;
   union Arg a;
};

extern union Arg *Arg1, *Arg2;

extern void lStkSin(void), lStkCos(void), lStkSinh(void), lStkCosh(void), lStkLog(void), lStkExp(void), lStkSqr(void);
extern void dStkSin(void), dStkCos(void), dStkSinh(void), dStkCosh(void), dStkLog(void), dStkExp(void), dStkSqr(void);

extern void (*g_trig0_l)(void);
extern void (*g_trig1_l)(void);
extern void (*g_trig2_l)(void);
extern void (*g_trig3_l)(void);
extern void (*g_trig0_d)(void);
extern void (*g_trig1_d)(void);
extern void (*g_trig2_d)(void);
extern void (*g_trig3_d)(void);

/* -------------------------------------------------------------------- */
/*   The following #defines allow the complex transcendental functions  */
/*   in parser.c to be used here thus avoiding duplicated code.         */
/* -------------------------------------------------------------------- */
#if !defined(XFRACT)

#define CMPLXmod(z)       (sqr((z).x) + sqr((z).y))
#define CMPLXconj(z)    ((z).y = -((z).y))
#define LCMPLXmod(z)       (lsqr((z).x)+lsqr((z).y))
#define LCMPLXconj(z)   ((z).y =  -((z).y))


#define LCMPLXtrig0(arg, out) do { Arg1->l = (arg); g_trig0_l(); (out)=Arg1->l; } while (0)
#define LCMPLXtrig1(arg, out) do { Arg1->l = (arg); g_trig1_l(); (out)=Arg1->l; } while (0)
#define LCMPLXtrig2(arg, out) do { Arg1->l = (arg); g_trig2_l(); (out)=Arg1->l; } while (0)
#define LCMPLXtrig3(arg, out) do { Arg1->l = (arg); g_trig3_l(); (out)=Arg1->l; } while (0)

#endif /* XFRACT */

#define  CMPLXtrig0(arg, out) do { Arg1->d = (arg); g_trig0_d(); (out)=Arg1->d; } while (0)
#define  CMPLXtrig1(arg, out) do { Arg1->d = (arg); g_trig1_d(); (out)=Arg1->d; } while (0)
#define  CMPLXtrig2(arg, out) do { Arg1->d = (arg); g_trig2_d(); (out)=Arg1->d; } while (0)
#define  CMPLXtrig3(arg, out) do { Arg1->d = (arg); g_trig3_d(); (out)=Arg1->d; } while (0)

#if !defined(XFRACT)

#define LCMPLXsin(arg, out)   do { Arg1->l = (arg); lStkSin();  (out) = Arg1->l; } while (0)
#define LCMPLXcos(arg, out)   do { Arg1->l = (arg); lStkCos();  (out) = Arg1->l; } while (0)
#define LCMPLXsinh(arg, out)  do { Arg1->l = (arg); lStkSinh(); (out) = Arg1->l; } while (0)
#define LCMPLXcosh(arg, out)  do { Arg1->l = (arg); lStkCosh(); (out) = Arg1->l; } while (0)
#define LCMPLXlog(arg, out)   do { Arg1->l = (arg); lStkLog();  (out) = Arg1->l; } while (0)
#define LCMPLXexp(arg, out)   do { Arg1->l = (arg); lStkExp();  (out) = Arg1->l; } while (0)

#define LCMPLXsqr(arg, out)											\
	do																\
	{																\
		(out).x = lsqr((arg).x) - lsqr((arg).y);					\
		(out).y = multiply((arg).x, (arg).y, g_bit_shift_minus_1);	\
	}																\
	while (0)

#define LCMPLXsqr_old(out)													\
	do																		\
	{																		\
		(out).y = multiply(g_old_z_l.x, g_old_z_l.y, g_bit_shift_minus_1);	\
		(out).x = g_temp_sqr_x_l - g_temp_sqr_y_l;							\
	}																		\
	while (0)

#define LCMPLXpwr(arg1, arg2, out)	\
	do								\
	{								\
		Arg2->l = (arg1);			\
		Arg1->l = (arg2);			\
		lStkPwr();					\
		Arg1++;						\
		Arg2++;						\
		(out) = Arg2->l;			\
	}								\
	while (0)

#define LCMPLXmult(arg1, arg2, out)	\
	do								\
	{								\
		Arg2->l = (arg1);			\
		Arg1->l = (arg2);			\
		lStkMul();					\
		Arg1++;						\
		Arg2++;						\
		(out) = Arg2->l;			\
	}								\
	while (0)

#define LCMPLXadd(arg1, arg2, out)		\
	do									\
	{									\
		(out).x = (arg1).x + (arg2).x;	\
		(out).y = (arg1).y + (arg2).y;	\
	}									\
	while (0)

#define LCMPLXsub(arg1, arg2, out)		\
	do									\
	{									\
		(out).x = (arg1).x - (arg2).x;	\
		(out).y = (arg1).y - (arg2).y;	\
	}									\
	while (0)

#define LCMPLXtimesreal(arg, real, out)						\
	do														\
	{														\
		(out).x = multiply((arg).x, (real), g_bit_shift);	\
		(out).y = multiply((arg).y, (real), g_bit_shift);	\
	}														\
	while (0)

#define LCMPLXrecip(arg, out)								\
	do														\
	{														\
		long denom;											\
		denom = lsqr((arg).x) + lsqr((arg).y);				\
		if (denom == 0L)									\
		{													\
			g_overflow = 1;									\
		}													\
		else												\
		{													\
			(out).x = divide((arg).x, denom, g_bit_shift);	\
			(out).y = -divide((arg).y, denom, g_bit_shift);	\
		}													\
	}														\
	while (0)

#endif /* XFRACT */

#define CMPLXsin(arg, out)	\
	do						\
	{						\
		Arg1->d = (arg);	\
		dStkSin();			\
		(out) = Arg1->d;	\
	}						\
	while (0)

#define CMPLXcos(arg, out)	\
	do						\
	{						\
		Arg1->d = (arg);	\
		dStkCos();			\
		(out) = Arg1->d;	\
	}						\
	while (0)

#define CMPLXsinh(arg, out)	\
	do						\
	{						\
		Arg1->d = (arg);	\
		dStkSinh();			\
		(out) = Arg1->d;	\
	}						\
	while (0)

#define CMPLXcosh(arg, out)	\
	do						\
	{						\
		Arg1->d = (arg);	\
		dStkCosh();			\
		(out) = Arg1->d;	\
	}						\
	while (0)

#define CMPLXlog(arg, out)	\
	do						\
	{						\
		Arg1->d = (arg);	\
		dStkLog();			\
		(out) = Arg1->d;	\
	}						\
	while (0)

#define CMPLXexp(arg, out)    (FPUcplxexp(&(arg), &(out)))
#define CMPLXsqr(arg, out)						\
	do											\
	{											\
		(out).x = sqr((arg).x) - sqr((arg).y);	\
		(out).y = ((arg).x+(arg).x) * (arg).y;	\
	}											\
	while (0)

#define CMPLXsqr_old(out)							\
	do												\
	{												\
		(out).y = (g_old_z.x+g_old_z.x) * g_old_z.y;\
		(out).x = g_temp_sqr_x - g_temp_sqr_y;		\
	}												\
	while (0)

#define CMPLXpwr(arg1, arg2, out)   (out)= ComplexPower((arg1), (arg2))

#define CMPLXmult1(arg1, arg2, out)	\
	do								\
	{								\
		Arg2->d = (arg1);			\
		Arg1->d = (arg2);			\
		dStkMul();					\
		Arg1++;						\
		Arg2++;						\
		(out) = Arg2->d;			\
	}								\
	while (0)

#define CMPLXmult(arg1, arg2, out)						\
	do													\
	{													\
		_CMPLX TmP;										\
		TmP.x = (arg1).x*(arg2).x - (arg1).y*(arg2).y;	\
		TmP.y = (arg1).x*(arg2).y + (arg1).y*(arg2).x;	\
		(out) = TmP;									\
	}													\
	while (0)

#define CMPLXadd(arg1, arg2, out)		\
	do									\
	{									\
		(out).x = (arg1).x + (arg2).x;	\
		(out).y = (arg1).y + (arg2).y;	\
	}									\
	while (0)

#define CMPLXsub(arg1, arg2, out)		\
	do									\
	{									\
		(out).x = (arg1).x - (arg2).x;	\
		(out).y = (arg1).y - (arg2).y;	\
	}									\
	while (0)

#define CMPLXtimesreal(arg, real, out)	\
	do									\
	{									\
		(out).x = (arg).x*(real);		\
		(out).y = (arg).y*(real);		\
	}									\
	while (0)

#define CMPLXrecip(arg, out)						\
	do												\
	{												\
		double denom = sqr((arg).x) + sqr((arg).y);	\
		if (denom == 0.0)							\
		{											\
			(out).x = 1.0e10;						\
			(out).y = 1.0e10;						\
		}											\
		else										\
		{											\
			(out).x =  (arg).x/denom;				\
			(out).y = -(arg).y/denom;				\
		}											\
	}												\
	while (0)

#define CMPLXneg(arg, out)  \
	do						\
	{						\
		(out).x = -(arg).x;	\
		(out).y = -(arg).y; \
	}						\
	while (0)

#endif
