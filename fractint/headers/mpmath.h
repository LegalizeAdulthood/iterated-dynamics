/*
 *  REMOVED FORMAL PARAMETERS FROM FUNCTION DEFINITIONS (1/4/92)
 */


#ifndef MPMATH_H
#define MPMATH_H

#ifndef _CMPLX_DEFINED
#include "cmplx.h"
#endif

extern int g_overflow_mp;
extern int DivideOverflow;

/*** Formula Declarations ***/
enum MathType
{
	FLOATING_POINT_MATH,
	FIXED_POINT_MATH
};

#define fDiv(x, y, z)		((*(long *) &z) = RegDivFloat(*(long *) &x, *(long *) &y))
#define fMul16(x, y, z)		((*(long *) &z) = r16Mul(*(long *) &x, *(long *) &y))
#define fShift(x, Shift, z)	((*(long *) &z) = RegSftFloat(*(long *) &x, Shift))
#define Fg2Float(x, f, z)	((*(long *) &z) = RegFg2Float(x, f))
#define Float2Fg(x, f)		RegFloat2Fg(*(long *) &x, f)
#define fLog14(x, z)		((*(long *) &z) = RegFg2Float(LogFloat14(*(long *) &x), 16))
#define fExp14(x, z)		((*(long *) &z) = ExpFloat14(*(long *) &x));
#define fSqrt14(x, z)		\
	do						\
	{						\
		fLog14(x, z);		\
		fShift(z, -1, z);	\
		fExp14(z, z);		\
	}						\
	while (0)

// the following are declared 4 dimensional as an experiment
// changing declarations to ComplexD and ComplexL restores the code
// to 2D
union Arg
{
   ComplexD d;
   ComplexL l;
};

struct ConstArg
{
   const char *name;
   int name_length;
   union Arg argument;
};

extern Arg *g_argument1;
extern Arg *g_argument2;

extern void lStkSin();
extern void lStkCos();
extern void lStkSinh();
extern void lStkCosh();
extern void lStkLog();
extern void lStkExp();
extern void lStkSqr();
extern void dStkSin();
extern void dStkCos();
extern void dStkSinh();
extern void dStkCosh();
extern void dStkLog();
extern void dStkExp();
extern void dStkSqr();

extern void (*g_trig0_l)();
extern void (*g_trig1_l)();
extern void (*g_trig2_l)();
extern void (*g_trig3_l)();
extern void (*g_trig0_d)();
extern void (*g_trig1_d)();
extern void (*g_trig2_d)();
extern void (*g_trig3_d)();

// --------------------------------------------------------------------
// The following #defines allow the complex transcendental functions
// in parser.c to be used here thus avoiding duplicated code.
// --------------------------------------------------------------------

#define CMPLXmod(z)		(sqr((z).real()) + sqr((z).imag()))
#define CMPLXconj(z)	((z).imag(-(z).imag()))
#define LCMPLXmod(z)	(lsqr((z).real())+lsqr((z).imag()))
#define LCMPLXconj(z)	((z).imag(-(z).imag()))


#define LCMPLXtrig0(arg, out) do { g_argument1->l = (arg); g_trig0_l(); (out)=g_argument1->l; } while (0)
#define LCMPLXtrig1(arg, out) do { g_argument1->l = (arg); g_trig1_l(); (out)=g_argument1->l; } while (0)
#define LCMPLXtrig2(arg, out) do { g_argument1->l = (arg); g_trig2_l(); (out)=g_argument1->l; } while (0)
#define LCMPLXtrig3(arg, out) do { g_argument1->l = (arg); g_trig3_l(); (out)=g_argument1->l; } while (0)

#define  CMPLXtrig0(arg, out) do { g_argument1->d = (arg); g_trig0_d(); (out)=g_argument1->d; } while (0)
#define  CMPLXtrig1(arg, out) do { g_argument1->d = (arg); g_trig1_d(); (out)=g_argument1->d; } while (0)
#define  CMPLXtrig2(arg, out) do { g_argument1->d = (arg); g_trig2_d(); (out)=g_argument1->d; } while (0)
#define  CMPLXtrig3(arg, out) do { g_argument1->d = (arg); g_trig3_d(); (out)=g_argument1->d; } while (0)

#define LCMPLXsin(arg, out)   do { g_argument1->l = (arg); lStkSin();  (out) = g_argument1->l; } while (0)
#define LCMPLXcos(arg, out)   do { g_argument1->l = (arg); lStkCos();  (out) = g_argument1->l; } while (0)
#define LCMPLXsinh(arg, out)  do { g_argument1->l = (arg); lStkSinh(); (out) = g_argument1->l; } while (0)
#define LCMPLXcosh(arg, out)  do { g_argument1->l = (arg); lStkCosh(); (out) = g_argument1->l; } while (0)
#define LCMPLXlog(arg, out)   do { g_argument1->l = (arg); lStkLog();  (out) = g_argument1->l; } while (0)
#define LCMPLXexp(arg, out)   do { g_argument1->l = (arg); lStkExp();  (out) = g_argument1->l; } while (0)

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
		(out).y = multiply(g_old_z_l.real(), g_old_z_l.imag(), g_bit_shift_minus_1);	\
		(out).x = g_temp_sqr_x_l - g_temp_sqr_y_l;							\
	}																		\
	while (0)

#define LCMPLXpwr(arg1, arg2, out)	\
	do								\
	{								\
		g_argument2->l = (arg1);			\
		g_argument1->l = (arg2);			\
		lStkPwr();					\
		g_argument1++;						\
		g_argument2++;						\
		(out) = g_argument2->l;			\
	}								\
	while (0)

#define LCMPLXmult(arg1, arg2, out)	\
	do								\
	{								\
		g_argument2->l = (arg1);			\
		g_argument1->l = (arg2);			\
		lStkMul();					\
		g_argument1++;						\
		g_argument2++;						\
		(out) = g_argument2->l;			\
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
			g_overflow = true;								\
		}													\
		else												\
		{													\
			(out).x = divide((arg).x, denom, g_bit_shift);	\
			(out).y = -divide((arg).y, denom, g_bit_shift);	\
		}													\
	}														\
	while (0)

#define CMPLXsin(arg, out)	\
	do						\
	{						\
		g_argument1->d = (arg);	\
		dStkSin();			\
		(out) = g_argument1->d;	\
	}						\
	while (0)

#define CMPLXcos(arg, out)	\
	do						\
	{						\
		g_argument1->d = (arg);	\
		dStkCos();			\
		(out) = g_argument1->d;	\
	}						\
	while (0)

#define CMPLXsinh(arg, out)	\
	do						\
	{						\
		g_argument1->d = (arg);	\
		dStkSinh();			\
		(out) = g_argument1->d;	\
	}						\
	while (0)

#define CMPLXcosh(arg, out)	\
	do						\
	{						\
		g_argument1->d = (arg);	\
		dStkCosh();			\
		(out) = g_argument1->d;	\
	}						\
	while (0)

#define CMPLXlog(arg, out)	\
	do						\
	{						\
		g_argument1->d = (arg);	\
		dStkLog();			\
		(out) = g_argument1->d;	\
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
		(out).y = (g_old_z.real()+g_old_z.real()) * g_old_z.imag();\
		(out).x = g_temp_sqr_x - g_temp_sqr_y;		\
	}												\
	while (0)

#define CMPLXpwr(arg1, arg2, out)   (out)= ComplexPower((arg1), (arg2))

#define CMPLXmult1(arg1, arg2, out)	\
	do								\
	{								\
		g_argument2->d = (arg1);			\
		g_argument1->d = (arg2);			\
		dStkMul();					\
		g_argument1++;						\
		g_argument2++;						\
		(out) = g_argument2->d;			\
	}								\
	while (0)

#define CMPLXmult(arg1, arg2, out)						\
	do													\
	{													\
		ComplexD TmP;										\
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

extern ComplexD ComplexPower(ComplexD, ComplexD);
extern void SetupLogTable();
extern long logtablecalc(long);
extern long ExpFloat14(long);
extern int complex_basin();
extern void Arcsinz(ComplexD z, ComplexD *rz);
extern void Arccosz(ComplexD z, ComplexD *rz);
extern void Arcsinhz(ComplexD z, ComplexD *rz);
extern void Arccoshz(ComplexD z, ComplexD *rz);
extern void Arctanhz(ComplexD z, ComplexD *rz);
extern void Arctanz(ComplexD z, ComplexD *rz);

#endif
