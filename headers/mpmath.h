#ifndef MPMATH_H
#define MPMATH_H

#include "calcfrac.h"
#include "cmplx.h"
#include "fpu087.h"
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
extern long ExpFloat14(long);

// ** Formula Declarations **
#if !defined(XFRACT)
enum MATH_TYPE { D_MATH, M_MATH, L_MATH };
#else
enum MATH_TYPE { D_MATH};
#endif

extern MATH_TYPE MathType;

inline void fDiv(float x, float y, float &z)
{
    *(long*)&z = RegDivFloat(*(long*)&x, *(long*)&y);
}
inline void fMul16(float x, float y, float &z)
{
    *(long*)&z = r16Mul(*(long*)&x, *(long*)&y);
}
inline void fShift(float x, int shift, float &z)
{
    *(long*)&z = RegSftFloat(*(long*)&x, shift);
}
inline void Fg2Float(int x, long f, float &z)
{
    *(long*)&z = RegFg2Float(x, f);
}
inline long Float2Fg(float x, int f)
{
    return RegFloat2Fg(*(long*)&x, f);
}
inline void fLog14(float x, float &z)
{
    *reinterpret_cast<long*>(&z) = RegFg2Float(LogFloat14(*reinterpret_cast<long*>(&x)), 16);
}
inline void fExp14(float x, float &z)
{
    *(long*)&z = ExpFloat14(*(long*)&x);
}
inline void fSqrt14(float x, float &z)
{
    fLog14(x, z);
    fShift(z, -1, z);
    fExp14(z, z);
}

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

// --------------------------------------------------------------------
// The following #defines allow the complex transcendental functions
// in parser.c to be used here thus avoiding duplicated code.
// --------------------------------------------------------------------
#if !defined(XFRACT)
inline long LCMPLXmod(const LComplex &z)
{
    return lsqr(z.x) + lsqr(z.y);
}
inline void LCMPLXtrig0(const LComplex &arg, LComplex &out)
{
    Arg1->l = arg;
    extern void (*ltrig0)();
    ltrig0();
    out = Arg1->l;
}
#define LCMPLXtrig1(arg, out) Arg1->l = (arg); ltrig1(); (out) = Arg1->l
#define LCMPLXtrig2(arg, out) Arg1->l = (arg); ltrig2(); (out) = Arg1->l
#define LCMPLXtrig3(arg, out) Arg1->l = (arg); ltrig3(); (out) = Arg1->l
#endif /* XFRACT */
#define  CMPLXtrig0(arg, out) Arg1->d = (arg); dtrig0(); (out) = Arg1->d
#define  CMPLXtrig1(arg, out) Arg1->d = (arg); dtrig1(); (out) = Arg1->d
#define  CMPLXtrig2(arg, out) Arg1->d = (arg); dtrig2(); (out) = Arg1->d
#define  CMPLXtrig3(arg, out) Arg1->d = (arg); dtrig3(); (out) = Arg1->d
#if !defined(XFRACT)
inline void LCMPLXsin(const LComplex &arg, LComplex &out)
{
    Arg1->l = arg;
    extern void lStkSin();
    lStkSin();
    (out) = Arg1->l;
}
inline void LCMPLXcos(const LComplex &arg, LComplex &out)
{
    Arg1->l = arg;
    extern void lStkCos();
    lStkCos();
    out = Arg1->l;
}
inline void LCMPLXsinh(const LComplex &arg, LComplex &out)
{
    Arg1->l = arg;
    extern void lStkSinh();
    lStkSinh();
    out = Arg1->l;
}
inline void LCMPLXcosh(const LComplex &arg, LComplex &out)
{
    Arg1->l = arg;
    extern void lStkCosh();
    lStkCosh();
    out = Arg1->l;
}
inline void LCMPLXlog(const LComplex &arg, LComplex &out)
{
    Arg1->l = arg;
    extern void lStkLog();
    lStkLog();
    out = Arg1->l;
}
inline void LCMPLXexp(const LComplex &arg, LComplex &out)
{
    Arg1->l = arg;
    extern void lStkExp();
    lStkExp();
    out = Arg1->l;
}
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
inline double CMPLXmod(const DComplex &z)
{
    return sqr(z.x) + sqr(z.y);
}
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
