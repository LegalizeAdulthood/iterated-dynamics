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
extern InitializedComplexD g_c_degree;
extern InitializedComplexD g_c_root;

namespace std {
extern ComplexD pow(ComplexD const &x, ComplexD const &y);
}
extern void SetupLogTable();
extern long logtablecalc(long);
extern long ExpFloat14(long);
extern int complex_basin();

// --------------------------------------------------------------------
// The following inline functions allow the complex transcendental functions
// in parser.cpp to be used here thus avoiding duplicated code.
// --------------------------------------------------------------------


inline void LCMPLXtrig0(ComplexL const &arg, ComplexL &out)
{ g_argument1->l = arg; g_trig0_l(); out = g_argument1->l; }
inline void LCMPLXtrig1(ComplexL const &arg, ComplexL &out)
{ g_argument1->l = arg; g_trig1_l(); out = g_argument1->l; }
inline void LCMPLXtrig2(ComplexL const &arg, ComplexL &out)
{ g_argument1->l = arg; g_trig2_l(); out = g_argument1->l; }
inline void LCMPLXtrig3(ComplexL const &arg, ComplexL &out)
{ g_argument1->l = arg; g_trig3_l(); out = g_argument1->l; }

inline ComplexD CMPLXtrig0(ComplexD const &arg)
{ g_argument1->d = arg; g_trig0_d(); return g_argument1->d; }
inline ComplexD CMPLXtrig1(ComplexD const &arg)
{ g_argument1->d = arg; g_trig1_d(); return g_argument1->d; }
inline ComplexD CMPLXtrig2(ComplexD const &arg)
{ g_argument1->d = arg; g_trig2_d(); return g_argument1->d; }
inline ComplexD CMPLXtrig3(ComplexD const &arg)
{ g_argument1->d = arg; g_trig3_d(); return g_argument1->d; }

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
	out.real(lsqr(arg.real()) - lsqr(arg.imag()));
	out.imag(multiply(arg.real(), arg.imag(), g_bit_shift_minus_1));
}
inline void CMPLXsqr(ComplexD const &arg, ComplexD &out)
{
	out.real(sqr(arg.real()) - sqr(arg.imag()));
	out.imag((arg.real()+arg.real()) * arg.imag());
}

inline void LCMPLXsqr_old(ComplexL &out)
{
	out.imag(multiply(g_old_z_l.real(), g_old_z_l.imag(), g_bit_shift_minus_1));
	out.real(g_temp_sqr_l.real() - g_temp_sqr_l.imag());
}
inline void CMPLXsqr_old(ComplexD &out)
{
	out.imag((g_old_z.real() + g_old_z.real())*g_old_z.imag());
	out.real(g_temp_sqr.real() - g_temp_sqr.imag());
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
	out.real(arg1.real() + arg2.real());
	out.imag(arg1.imag() + arg2.imag());
}

inline void LCMPLXsub(ComplexL const &arg1, ComplexL const &arg2, ComplexL &out)
{
	out.real(arg1.real() - arg2.real());
	out.imag(arg1.imag() - arg2.imag());
}

inline void LCMPLXtimesreal(ComplexL const &arg, long real, ComplexL &out)
{
	out.real(multiply(arg.real(), real, g_bit_shift));
	out.imag(multiply(arg.imag(), real, g_bit_shift));
}

inline void LCMPLXrecip(ComplexL const &arg, ComplexL &out)
{
	long denom = lsqr(arg.real()) + lsqr(arg.imag());
	if (denom == 0L)
	{
		g_overflow = true;
	}
	else
	{
		out.real(divide(arg.real(), denom, g_bit_shift));
		out.imag(-divide(arg.imag(), denom, g_bit_shift));
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
	out = std::pow(arg1, arg2);
}

inline void CMPLXrecip(ComplexD const &arg, ComplexD &out)
{
	double const denom = norm(arg);
	if (denom == 0.0)
	{
		out.real(1.0e10);
		out.imag(1.0e10);
	}
	else
	{
		out.real( arg.real()/denom);
		out.imag(-arg.imag()/denom);
	}
}

inline void CMPLXneg(ComplexD const &arg, ComplexD &out)
{
	out.real(-arg.real());
	out.imag(-arg.imag());
}

#endif
