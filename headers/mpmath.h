#ifndef MPMATH_H
#define MPMATH_H

#include "calcfrac.h"
#include "cmplx.h"
#include "fractals.h"
#include "id_data.h"

#include <vector>

#if !defined(XFRACT)
struct MP
{
    int Exp;
    unsigned long Mant;
};
#else
struct MP
{
    double val;
};
#endif

struct MPC
{
    MP x;
    MP y;
};

extern int                   g_distribution;
extern bool                  g_log_map_calculate;
extern std::vector<BYTE>     g_log_map_table;
extern long                  g_log_map_table_max_size;
extern int                   g_mp_overflow;
extern MPC                   g_mpc_one;
extern int                   g_slope;

/* Mark Peterson's expanded floating point operators. If
   the operation results in an overflow (result < 2**(2**14), or division
   by zero) the global 'g_mp_overflow' is set to one. */
extern int (*pMPcmp)(MP , MP);
extern MP  *(*pMPmul)(MP , MP);
extern MP  *(*pMPdiv)(MP , MP);
extern MP  *(*pMPadd)(MP , MP);
extern MP  *(*pMPsub)(MP , MP);
extern MP  *(*pd2MP)(double)                ;
extern double     *(*pMP2d)(MP)             ;

// ** Formula Declarations **
#if !defined(XFRACT)
enum MATH_TYPE { D_MATH, M_MATH, L_MATH };
#else
enum MATH_TYPE { D_MATH};
#endif

extern MATH_TYPE MathType;

#define fDiv(x, y, z) ((*(long*)&z) = RegDivFloat(*(long*)&x, *(long*)&y))
#define fMul16(x, y, z) ((*(long*)&z) = r16Mul(*(long*)&x, *(long*)&y))
#define fShift(x, Shift, z) ((*(long*)&z) = RegSftFloat(*(long*)&x, Shift))
#define Fg2Float(x, f, z) ((*(long*)&z) = RegFg2Float(x, f))
#define Float2Fg(x, f) RegFloat2Fg(*(long*)&x, f)
#define fLog14(x, z) ((*reinterpret_cast<long*>(&z)) = RegFg2Float(LogFloat14(*reinterpret_cast<long*>(&x)), 16))
#define fExp14(x, z) ((*(long*)&z) = ExpFloat14(*(long*)&x));
#define fSqrt14(x, z) fLog14(x, z); fShift(z, -1, z); fExp14(z, z)

// the following are declared 4 dimensional as an experiment
// changeing declarations to DComplex and LComplex restores the code
// to 2D
union Arg
{
    DComplex d;
    MPC m;
    LComplex l;
};

struct ConstArg
{
    char const *s;
    int len;
    Arg a;
};

extern Arg *Arg1;
extern Arg *Arg2;

extern void lStkSin();
extern void lStkCos();
extern void lStkSinh();
extern void lStkCosh();
extern void lStkLog();
extern void lStkExp();
extern void lStkSqr();
extern void dStkExp();
extern void dStkSqr();

// --------------------------------------------------------------------
// The following #defines allow the complex transcendental functions
// in parser.c to be used here thus avoiding duplicated code.
// --------------------------------------------------------------------
#if !defined(XFRACT)
#define CMPLXmod(z)       (sqr((z).x)+sqr((z).y))
#define CMPLXconj(z)    ((z).y =  -((z).y))
#define LCMPLXmod(z)       (lsqr((z).x)+lsqr((z).y))
#define LCMPLXconj(z)   ((z).y =  -((z).y))
#define LCMPLXtrig0(arg, out) Arg1->l = (arg); ltrig0(); (out) = Arg1->l
#define LCMPLXtrig1(arg, out) Arg1->l = (arg); ltrig1(); (out) = Arg1->l
#define LCMPLXtrig2(arg, out) Arg1->l = (arg); ltrig2(); (out) = Arg1->l
#define LCMPLXtrig3(arg, out) Arg1->l = (arg); ltrig3(); (out) = Arg1->l
#endif /* XFRACT */
#define  CMPLXtrig0(arg, out) Arg1->d = (arg); dtrig0(); (out) = Arg1->d
#define  CMPLXtrig1(arg, out) Arg1->d = (arg); dtrig1(); (out) = Arg1->d
#define  CMPLXtrig2(arg, out) Arg1->d = (arg); dtrig2(); (out) = Arg1->d
#define  CMPLXtrig3(arg, out) Arg1->d = (arg); dtrig3(); (out) = Arg1->d
#if !defined(XFRACT)
#define LCMPLXsin(arg, out)   Arg1->l = (arg); lStkSin();  (out) = Arg1->l
#define LCMPLXcos(arg, out)   Arg1->l = (arg); lStkCos();  (out) = Arg1->l
#define LCMPLXsinh(arg, out)  Arg1->l = (arg); lStkSinh(); (out) = Arg1->l
#define LCMPLXcosh(arg, out)  Arg1->l = (arg); lStkCosh(); (out) = Arg1->l
#define LCMPLXlog(arg, out)   Arg1->l = (arg); lStkLog();  (out) = Arg1->l
#define LCMPLXexp(arg, out)   Arg1->l = (arg); lStkExp();  (out) = Arg1->l
inline void LCMPLXsqr(const LComplex &arg, LComplex &out)
{
   out.x = lsqr(arg.x) - lsqr(arg.y);
   out.y = multiply(arg.x, arg.y, g_bit_shift_less_1);
}
#define LCMPLXsqr_old(out)       \
   (out).y = multiply(g_l_old_z.x, g_l_old_z.y, g_bit_shift_less_1);\
   (out).x = g_l_temp_sqr_x - g_l_temp_sqr_y
inline void LCMPLXpwr(const LComplex &arg1, const LComplex &arg2, LComplex &out)
{
    Arg2->l = arg1;
    Arg1->l = arg2;
    extern void lStkPwr();
    lStkPwr();
    Arg1++;
    Arg2++;
    out = Arg2->l;
}

inline void LCMPLXmult(const LComplex &arg1, const LComplex &arg2, LComplex &out)
{
    Arg1->l = arg1;
    Arg2->l = arg2;
    extern void lStkMul();
    lStkMul();
    Arg1++;
    Arg2++;
    out = Arg2->l;
}
inline void LCMPLXadd(const LComplex &arg1, const LComplex &arg2, LComplex &out)
{
    out = arg1 + arg2;
}
inline void LCMPLXsub(const LComplex &arg1, const LComplex &arg2, LComplex &out)
{
    out = arg1 - arg2;
}
inline void LCMPLXtimesreal(const LComplex &arg, long real, LComplex &out)
{
    out.x = multiply(arg.x, real, g_bit_shift);
    out.y = multiply(arg.y, real, g_bit_shift);
}
inline void LCMPLXrecip(const LComplex &arg, LComplex &out)
{
    const long denom = lsqr(arg.x) + lsqr(arg.y);
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
#endif /* XFRACT */
inline void CMPLXsin(const DComplex &arg, DComplex &out)
{
    Arg1->d = arg;
    extern void dStkSin();
    dStkSin();
    out = Arg1->d;
}
inline void CMPLXcos(const DComplex &arg, DComplex &out)
{
    Arg1->d = arg;
    extern void dStkCos();
    dStkCos();
    out = Arg1->d;
}
inline void CMPLXsinh(const DComplex &arg, DComplex &out)
{
    Arg1->d = arg;
    extern void dStkSinh();
    dStkSinh();
    out = Arg1->d;
}
inline void CMPLXcosh(const DComplex &arg, DComplex &out)
{
    Arg1->d = arg;
    extern void dStkCosh();
    dStkCosh();
    out = Arg1->d;
}
inline void CMPLXlog(const DComplex &arg, DComplex &out)
{
    Arg1->d = arg;
    extern void dStkLog();
    dStkLog();
    out = Arg1->d;
}
inline void CMPLXexp(const DComplex &arg, DComplex &out)
{
    extern void FPUcplxexp(const DComplex *, DComplex *);
    FPUcplxexp(&(arg), &(out));
}
inline void CMPLXsqr(const DComplex &arg, DComplex &out)
{
   out.x = sqr(arg.x) - sqr(arg.y);
   out.y = (arg.x + arg.x) * arg.y;
}
#define CMPLXsqr_old(out)       \
   (out).y = (g_old_z.x+g_old_z.x) * g_old_z.y;\
   (out).x = g_temp_sqr_x - g_temp_sqr_y
inline void CMPLXpwr(const DComplex &arg1, const DComplex &arg2, DComplex &out)
{
    extern DComplex ComplexPower(DComplex, DComplex);
    out = ComplexPower(arg1, arg2);
}
inline void CMPLXmult1(const DComplex &arg1, const DComplex &arg2, DComplex &out)
{
    extern void dStkMul();
    Arg2->d = arg1;
    Arg1->d = arg2;
    dStkMul();
    Arg1++;
    Arg2++;
    out = Arg2->d;
}
inline void CMPLXmult(const DComplex &arg1, const DComplex &arg2, DComplex &out)
{
    DComplex tmp;
    tmp.x = arg1.x*arg2.x - arg1.y*arg2.y;
    tmp.y = arg1.x*arg2.y + arg1.y*arg2.x;
    out = tmp;
}
inline void CMPLXadd(const DComplex &arg1, const DComplex &arg2, DComplex &out)
{
    out = arg1 + arg2;
}
inline void CMPLXsub(const DComplex &arg1, const DComplex &arg2, DComplex &out)
{
    out = arg1 - arg2;
}
inline void CMPLXtimesreal(const DComplex &arg, double real, DComplex &out)
{
    out = arg*real;
}
inline void CMPLXrecip(const DComplex &arg, DComplex &out)
{
    const double denom = sqr(arg.x) + sqr(arg.y);
    if (denom == 0.0)
    {
        out = {1.0e10, 1.0e10};
    }
    else
    {
        out.x = arg.x / denom;
        out.y = -arg.y / denom;
    }
}
inline void CMPLXneg(const DComplex &arg, DComplex &out)
{
    out.x = -arg.x;
    out.y = -arg.y;
}

#endif
