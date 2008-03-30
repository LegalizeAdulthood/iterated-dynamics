#if !defined(MPMATH_H)
#define MPMATH_H

#include "cmplx.h"
#include "externs.h"
#include "prototyp.h"
#include "Formula.h"

extern void (*g_trig0_l)();
extern void (*g_trig1_l)();
extern void (*g_trig2_l)();
extern void (*g_trig3_l)();
extern void (*g_trig0_d)();
extern void (*g_trig1_d)();
extern void (*g_trig2_d)();
extern void (*g_trig3_d)();

extern ComplexD ComplexPower(ComplexD const &x, ComplexD const &y);
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

// --------------------------------------------------------------------
// The following inline functions allow the complex transcendental functions
// in parser.cpp to be used here thus avoiding duplicated code.
// --------------------------------------------------------------------

inline double CMPLXmod(ComplexD const &z)
{
	return sqr(z.real()) + sqr(z.imag());
}

inline void LCMPLXtrig0(ComplexL const &arg, ComplexL &out)
{ g_argument1->l = arg; g_trig0_l(); out = g_argument1->l; }
inline void LCMPLXtrig1(ComplexL const &arg, ComplexL &out)
{ g_argument1->l = arg; g_trig1_l(); out = g_argument1->l; }
inline void LCMPLXtrig2(ComplexL const &arg, ComplexL &out)
{ g_argument1->l = arg; g_trig2_l(); out = g_argument1->l; }
inline void LCMPLXtrig3(ComplexL const &arg, ComplexL &out)
{ g_argument1->l = arg; g_trig3_l(); out = g_argument1->l; }

inline void CMPLXtrig0(ComplexD const &arg, ComplexD &out)
{ g_argument1->d = arg; g_trig0_d(); out = g_argument1->d; }
inline void CMPLXtrig1(ComplexD const &arg, ComplexD &out)
{ g_argument1->d = arg; g_trig1_d(); out = g_argument1->d; }
inline void CMPLXtrig2(ComplexD const &arg, ComplexD &out)
{ g_argument1->d = arg; g_trig2_d(); out = g_argument1->d; }
inline void CMPLXtrig3(ComplexD const &arg, ComplexD &out)
{ g_argument1->d = arg; g_trig3_d(); out = g_argument1->d; }

inline void LCMPLXsin(ComplexL const &arg, ComplexL &out)
{ g_argument1->l = arg; lStkSin();  out = g_argument1->l; }
inline void LCMPLXcos(ComplexL const &arg, ComplexL &out)
{ g_argument1->l = arg; lStkCos();  out = g_argument1->l; }
inline void LCMPLXsinh(ComplexL const &arg, ComplexL &out)
{ g_argument1->l = arg; lStkSinh(); out = g_argument1->l; }
inline void LCMPLXcosh(ComplexL const &arg, ComplexL &out)
{ g_argument1->l = arg; lStkCosh(); out = g_argument1->l; }
inline void LCMPLXlog(ComplexL const &arg, ComplexL &out)
{ g_argument1->l = arg; lStkLog();  out = g_argument1->l; }
inline void LCMPLXexp(ComplexL const &arg, ComplexL &out)
{ g_argument1->l = arg; lStkExp();  out = g_argument1->l; }

inline void LCMPLXsqr(ComplexL const &arg, ComplexL &out)
{
	out.x = lsqr(arg.x) - lsqr(arg.y);
	out.y = multiply(arg.x, arg.y, g_bit_shift_minus_1);
}
inline void CMPLXsqr(ComplexD const &arg, ComplexD &out)
{
	out.x = sqr(arg.x) - sqr(arg.y);
	out.y = (arg.x+arg.x) * arg.y;
}

inline void LCMPLXsqr_old(ComplexL &out)
{
	out.y = multiply(g_old_z_l.real(), g_old_z_l.imag(), g_bit_shift_minus_1);
	out.x = g_temp_sqr_l.real() - g_temp_sqr_l.imag();
}
inline void CMPLXsqr_old(ComplexD &out)
{
	out.y = (g_old_z.real() + g_old_z.real())*g_old_z.imag();
	out.x = g_temp_sqr.real() - g_temp_sqr.imag();
}


inline void LCMPLXpwr(ComplexL const &arg1, ComplexL const &arg2, ComplexL &out)
{
	g_argument2->l = arg1;
	g_argument1->l = arg2;
	lStkPwr();
	g_argument1++;
	g_argument2++;
	out = g_argument2->l;
}

inline void LCMPLXmult(ComplexL const &arg1, ComplexL const &arg2, ComplexL &out)
{
	g_argument2->l = arg1;
	g_argument1->l = arg2;
	lStkMul();
	g_argument1++;
	g_argument2++;
	out = g_argument2->l;
}

inline void LCMPLXadd(ComplexL const &arg1, ComplexL const &arg2, ComplexL &out)
{
	out.x = arg1.x + arg2.x;
	out.y = arg1.y + arg2.y;
}

inline void LCMPLXsub(ComplexL const &arg1, ComplexL const &arg2, ComplexL &out)
{
	out.x = arg1.x - arg2.x;
	out.y = arg1.y - arg2.y;
}

inline void LCMPLXtimesreal(ComplexL const &arg, long real, ComplexL &out)
{
	out.x = multiply(arg.x, real, g_bit_shift);
	out.y = multiply(arg.y, real, g_bit_shift);
}

inline void LCMPLXrecip(ComplexL const &arg, ComplexL &out)
{
	long denom = lsqr(arg.x) + lsqr(arg.y);
	if (denom == 0L)
	{
		g_overflow = true;
	}
	else
	{
		out.x = divide(arg.x, denom, g_bit_shift);
		out.y = -divide(arg.y, denom, g_bit_shift);
	}
}

inline void CMPLXsin(ComplexD const &arg, ComplexD &out)
{
	g_argument1->d = arg;
	dStkSin();
	out = g_argument1->d;
}

inline void CMPLXcos(ComplexD const &arg, ComplexD &out)
{
	g_argument1->d = arg;
	dStkCos();
	out = g_argument1->d;
}

inline void CMPLXsinh(ComplexD const &arg, ComplexD &out)
{
	g_argument1->d = arg;
	dStkSinh();
	out = g_argument1->d;
}

inline void CMPLXcosh(ComplexD const &arg, ComplexD &out)
{
	g_argument1->d = arg;
	dStkCosh();
	out = g_argument1->d;
}

inline void CMPLXlog(ComplexD const &arg, ComplexD &out)
{
	g_argument1->d = arg;
	dStkLog();
	out = g_argument1->d;
}

inline void CMPLXexp(ComplexD const &arg, ComplexD &out)
{
	FPUcplxexp(&arg, &out);
}

inline void CMPLXpwr(ComplexD const &arg1, ComplexD const &arg2, ComplexD &out)
{
	out = ComplexPower(arg1, arg2);
}

inline void CMPLXmult1(ComplexD const &arg1, ComplexD const &arg2, ComplexD &out)
{
	g_argument2->d = arg1;
	g_argument1->d = arg2;
	dStkMul();
	g_argument1++;
	g_argument2++;
	out = g_argument2->d;
}

inline void CMPLXmult(ComplexD const &arg1, ComplexD const &arg2, ComplexD &out)
{
	ComplexD tmp;
	tmp.x = arg1.x*arg2.x - arg1.y*arg2.y;
	tmp.y = arg1.x*arg2.y + arg1.y*arg2.x;
	out = tmp;
}

inline void CMPLXadd(ComplexD const &arg1, ComplexD const &arg2, ComplexD &out)
{
	out.x = arg1.x + arg2.x;
	out.y = arg1.y + arg2.y;
}

inline void CMPLXsub(ComplexD const &arg1, ComplexD const &arg2, ComplexD &out)
{
	out.x = arg1.x - arg2.x;
	out.y = arg1.y - arg2.y;
}

inline void CMPLXtimesreal(ComplexD const &arg, double real, ComplexD &out)
{
	out.x = arg.x*real;
	out.y = arg.y*real;
}

inline void CMPLXrecip(ComplexD const &arg, ComplexD &out)
{
	double const denom = sqr(arg.x) + sqr(arg.y);
	if (denom == 0.0)
	{
		out.x = 1.0e10;
		out.y = 1.0e10;
	}
	else
	{
		out.x =  arg.x/denom;
		out.y = -arg.y/denom;
	}
}

inline void CMPLXneg(ComplexD const &arg, ComplexD &out)
{
	out.x = -arg.x;
	out.y = -arg.y;
}

#endif
