#ifndef MPMATH_H
#define MPMATH_H
#ifndef CMPLX_H_DEFINED
#include "cmplx.h"
#endif
#if !defined(XFRACT)
struct MP {
    int Exp;
    unsigned long Mant;
};
#else
struct MP {
    double val;
};
#endif
struct MPC {
    struct MP x, y;
};
extern int MPOverflow;
/* Mark Peterson's expanded floating point operators. If
   the operation results in an overflow (result < 2**(2**14), or division
   by zero) the global 'MPoverflow' is set to one. */
extern int (*pMPcmp)(struct MP , struct MP);
extern struct MP  *(*pMPmul)(struct MP , struct MP);
extern struct MP  *(*pMPdiv)(struct MP , struct MP);
extern struct MP  *(*pMPadd)(struct MP , struct MP);
extern struct MP  *(*pMPsub)(struct MP , struct MP);
extern struct MP  *(*pd2MP)(double)                ;
extern double     *(*pMP2d)(struct MP)             ;
// ** Formula Declarations **
#if !defined(XFRACT)
enum MATH_TYPE { D_MATH, M_MATH, L_MATH };
#else
enum MATH_TYPE { D_MATH};
#endif
extern enum MATH_TYPE MathType;
#define fDiv(x, y, z) ((*(long*)&z) = RegDivFloat(*(long*)&x, *(long*)&y))
#define fMul16(x, y, z) ((*(long*)&z) = r16Mul(*(long*)&x, *(long*)&y))
#define fShift(x, Shift, z) ((*(long*)&z) = \
   RegSftFloat(*(long*)&x, Shift))
#define Fg2Float(x, f, z) ((*(long*)&z) = RegFg2Float(x, f))
#define Float2Fg(x, f) RegFloat2Fg(*(long*)&x, f)
#define fLog14(x, z) ((*(long*)&z) = \
        RegFg2Float(LogFloat14(*(long*)&x), 16))
#define fExp14(x, z) ((*(long*)&z) = ExpFloat14(*(long*)&x));
#define fSqrt14(x, z) fLog14(x, z); fShift(z, -1, z); fExp14(z, z)
// the following are declared 4 dimensional as an experiment
// changeing declarations to DComplex and LComplex restores the code
// to 2D
union Arg {
    DComplex     d;
    struct MPC m;
    LComplex    l;
};
struct ConstArg {
    const char *s;
    int len;
    union Arg a;
};
extern union Arg *Arg1,*Arg2;
extern void lStkSin(),lStkCos(),lStkSinh(),lStkCosh(),lStkLog(),lStkExp(),lStkSqr();
extern void dStkSin(),dStkCos(),dStkSinh(),dStkCosh(),dStkLog(),dStkExp(),dStkSqr();
extern void (*ltrig0)();
extern void (*ltrig1)();
extern void (*ltrig2)();
extern void (*ltrig3)();
extern void (*dtrig0)();
extern void (*dtrig1)();
extern void (*dtrig2)();
extern void (*dtrig3)();
// --------------------------------------------------------------------
// The following #defines allow the complex transcendental functions
// in parser.c to be used here thus avoiding duplicated code.
// --------------------------------------------------------------------
#if !defined(XFRACT)
#define CMPLXmod(z)       (sqr((z).x)+sqr((z).y))
#define CMPLXconj(z)    ((z).y =  -((z).y))
#define LCMPLXmod(z)       (lsqr((z).x)+lsqr((z).y))
#define LCMPLXconj(z)   ((z).y =  -((z).y))
#define LCMPLXtrig0(arg,out) Arg1->l = (arg); ltrig0(); (out)=Arg1->l
#define LCMPLXtrig1(arg,out) Arg1->l = (arg); ltrig1(); (out)=Arg1->l
#define LCMPLXtrig2(arg,out) Arg1->l = (arg); ltrig2(); (out)=Arg1->l
#define LCMPLXtrig3(arg,out) Arg1->l = (arg); ltrig3(); (out)=Arg1->l
#endif /* XFRACT */
#define  CMPLXtrig0(arg,out) Arg1->d = (arg); dtrig0(); (out)=Arg1->d
#define  CMPLXtrig1(arg,out) Arg1->d = (arg); dtrig1(); (out)=Arg1->d
#define  CMPLXtrig2(arg,out) Arg1->d = (arg); dtrig2(); (out)=Arg1->d
#define  CMPLXtrig3(arg,out) Arg1->d = (arg); dtrig3(); (out)=Arg1->d
#if !defined(XFRACT)
#define LCMPLXsin(arg,out)   Arg1->l = (arg); lStkSin();  (out) = Arg1->l
#define LCMPLXcos(arg,out)   Arg1->l = (arg); lStkCos();  (out) = Arg1->l
#define LCMPLXsinh(arg,out)  Arg1->l = (arg); lStkSinh(); (out) = Arg1->l
#define LCMPLXcosh(arg,out)  Arg1->l = (arg); lStkCosh(); (out) = Arg1->l
#define LCMPLXlog(arg,out)   Arg1->l = (arg); lStkLog();  (out) = Arg1->l
#define LCMPLXexp(arg,out)   Arg1->l = (arg); lStkExp();  (out) = Arg1->l
/*
#define LCMPLXsqr(arg,out)   Arg1->l = (arg); lStkSqr();  (out) = Arg1->l
*/
#define LCMPLXsqr(arg,out)   \
   (out).x = lsqr((arg).x) - lsqr((arg).y);\
   (out).y = multiply((arg).x, (arg).y, bitshiftless1)
#define LCMPLXsqr_old(out)       \
   (out).y = multiply(lold.x, lold.y, bitshiftless1);\
   (out).x = ltempsqrx - ltempsqry
#define LCMPLXpwr(arg1,arg2,out)    Arg2->l = (arg1); Arg1->l = (arg2);\
         lStkPwr(); Arg1++; Arg2++; (out) = Arg2->l
#define LCMPLXmult(arg1,arg2,out)    Arg2->l = (arg1); Arg1->l = (arg2);\
         lStkMul(); Arg1++; Arg2++; (out) = Arg2->l
#define LCMPLXadd(arg1,arg2,out)    \
    (out).x = (arg1).x + (arg2).x; (out).y = (arg1).y + (arg2).y
#define LCMPLXsub(arg1,arg2,out)    \
    (out).x = (arg1).x - (arg2).x; (out).y = (arg1).y - (arg2).y
#define LCMPLXtimesreal(arg,real,out)   \
    (out).x = multiply((arg).x,(real),bitshift);\
    (out).y = multiply((arg).y,(real),bitshift)
#define LCMPLXrecip(arg,out)                            \
    {                                                   \
        long denom = lsqr((arg).x) + lsqr((arg).y);     \
        if (denom==0L)                                  \
            overflow = true;                            \
        else                                            \
        {                                               \
            (out).x = divide((arg).x,denom,bitshift);   \
            (out).y = -divide((arg).y,denom,bitshift);  \
        }                                               \
    }
#endif /* XFRACT */
#define CMPLXsin(arg,out)    Arg1->d = (arg); dStkSin();  (out) = Arg1->d
#define CMPLXcos(arg,out)    Arg1->d = (arg); dStkCos();  (out) = Arg1->d
#define CMPLXsinh(arg,out)   Arg1->d = (arg); dStkSinh(); (out) = Arg1->d
#define CMPLXcosh(arg,out)   Arg1->d = (arg); dStkCosh(); (out) = Arg1->d
#define CMPLXlog(arg,out)    Arg1->d = (arg); dStkLog();  (out) = Arg1->d
#define CMPLXexp(arg,out)    FPUcplxexp(&(arg), &(out))
/*
#define CMPLXsqr(arg,out)    Arg1->d = (arg); dStkSqr();  (out) = Arg1->d
*/
#define CMPLXsqr(arg,out)    \
   (out).x = sqr((arg).x) - sqr((arg).y);\
   (out).y = ((arg).x+(arg).x) * (arg).y
#define CMPLXsqr_old(out)       \
   (out).y = (old.x+old.x) * old.y;\
   (out).x = tempsqrx - tempsqry
#define CMPLXpwr(arg1,arg2,out)   (out)= ComplexPower((arg1), (arg2))
#define CMPLXmult1(arg1,arg2,out)    Arg2->d = (arg1); Arg1->d = (arg2);\
         dStkMul(); Arg1++; Arg2++; (out) = Arg2->d
#define CMPLXmult(arg1,arg2,out)  \
        {\
           DComplex TmP;\
           TmP.x = (arg1).x*(arg2).x - (arg1).y*(arg2).y;\
           TmP.y = (arg1).x*(arg2).y + (arg1).y*(arg2).x;\
           (out) = TmP;\
         }
#define CMPLXadd(arg1,arg2,out)    \
    (out).x = (arg1).x + (arg2).x; (out).y = (arg1).y + (arg2).y
#define CMPLXsub(arg1,arg2,out)    \
    (out).x = (arg1).x - (arg2).x; (out).y = (arg1).y - (arg2).y
#define CMPLXtimesreal(arg,real,out)   \
    (out).x = (arg).x*(real);\
    (out).y = (arg).y*(real)
#define CMPLXrecip(arg,out)    \
   { double denom; denom = sqr((arg).x) + sqr((arg).y);\
     if(denom==0.0) {(out).x = 1.0e10;(out).y = 1.0e10;}else\
    { (out).x =  (arg).x/denom;\
     (out).y = -(arg).y/denom;}}
#define CMPLXneg(arg,out)  (out).x = -(arg).x; (out).y = -(arg).y
#endif