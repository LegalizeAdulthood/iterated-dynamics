#ifndef FMATH_H
#define FMATH_H

/* FMath.h (C) 1989, Mark C. Peterson, CompuServe [70441,3353]
     All rights reserved.

   Code may be used in any program provided the author is credited
     either during program execution or in the documentation.  Source
     code may be distributed only in combination with public domain or
     shareware source code.  Source code may be modified provided the
     copyright notice and this message is left unchanged and all
     modifications are clearly documented.

     I would appreciate a copy of any work which incorporates this code,
     however this is optional.

     Mark C. Peterson
     128 Hamden Ave., F
     Waterbury, CT 06704
     (203) 754-1162

     Notes below document changes to Mark's original file:

     Date       Change                                    Changer
     ============================================================
     07-16-89 - Added sqrt define per Mark's suggestion   TIW
     07-26-89 - Added documentation and complex support   MCP
*/

/*****************
 * Documentation *
 *****************

   #include "fmath.h"
   float x, y, z;
   int Pot, Fudge;

                   23-bit accuracy (limit of type float)
     Regular Implementation               Fast Math Implementation
 --------------------------------------------------------------------
         z = x + y;                          fAdd(x, y, z);
         z = x * y;                          fMul(x, y, z);
         z = x * x;                          fSqr(x, z);
         z = x / y;                          fDiv(x, y, z);
         z = x * 2;                          fShift(x, 1, z);
         z = x * 16;                         fShift(x, 4, z);
         z = x / 32;                         fShift(x, -5, z);
         z = x / (pow(2.0, (double)Pot));    fShift(x, -Pot, z);
         z = (float)Pot * (1L << Fudge);     Fg2Float(Pot, Fudge, z);
         Pot = (int)(z / (1L << Fudge));     Pot = Float2Fg(z, Fudge);

                  Complex numbers using fComplex structures
         z = x**2                            fSqrZ(&x, &z);      mod updated
         z.mod = (z.x*z.x)+(z.y*z.y)         fModZ(&z);          mod updated
         z = 1 / x                           fInvZ(&x, &z);      mod updated
         z = x * y                           fMulZ(&x, &y, &z);  mod updated
         z = x / y                           fDivZ(&x, &y, &z);  mod updated

                            16-bit accuracy
     Regular Implementation               Fast Math Implementation
 --------------------------------------------------------------------
         z = x * y;                          fMul16(x, y, z);
         z = x * x;                          fSqr16(x, z);

                            14-bit accuracy
     Regular Implementation               Fast Math Implementation
 --------------------------------------------------------------------
         z = log(x);                         fLog14(x, z);
         z = exp(x);                         fExp14(x, z);
         z = pow(x, y);                      fPow14(x, y, z);

                            12-bit accuracy
     Regular Implementation               Fast Math Implementation
 --------------------------------------------------------------------
         z = sin(x);                         fSin12(x, z);
         z = cos(x);                         fCos12(x, z);
         z = sinh(x);                        fSinh12(x, z);
         z = cosh(x);                        fCosh12(x, z);

                  Complex numbers using fComplex structures
         z = sin(x)                          fSinZ(&x, &z);
         z = cos(x)                          fCosZ(&x, &z);
         z = tan(x)                          fTagZ(&x, &z);
         z = sinh(x)                         fSinhZ(&x, &z);
         z = cosh(x)                         fCoshZ(&x, &z);
         z = tanh(x)                         fCoshZ(&x, &z);

Just be sure to declare x, y, and z as type floats instead of type double.
*/

long
#ifndef XFRACT
   RegFg2Float(long x, char FudgeFact),
   RegSftFloat(long x, char Shift),
#else
   RegFg2Float(long x, int FudgeFact),
   RegSftFloat(long x, int Shift),
#endif
   RegFloat2Fg(long x, int Fudge),
   RegAddFloat(long x, long y),
   RegDivFloat(long x, long y),
   RegMulFloat(long x, long y),
   RegSqrFloat(long x),
   RegSubFloat(long x, long y);
long
   r16Mul(long x, long y),
   r16Sqr(long x);
int
        sin13(long x),
        cos13(long x),
        FastCosine(int x),
        FastSine(int x);
long
        FastHypCosine(int x),
        FastHypSine(int x),
   sinh13(long x),
   cosh13(long x);
long LogFudged(unsigned long x, int Fudge);
long LogFloat14(unsigned long x);
unsigned long ExpFudged(long x, int Fudge);
long ExpFloat14(long x);

#define fAdd(x, y, z) (void)((*(long*)&z) = RegAddFloat(*(long*)&x, *(long*)&y))
#define fMul(x, y, z) (void)((*(long*)&z) = RegMulFloat(*(long*)&x, *(long*)&y))
#define fDiv(x, y, z) (void)((*(long*)&z) = RegDivFloat(*(long*)&x, *(long*)&y))
#define fSub(x, y, z) (void)((*(long*)&z) = RegSubFloat(*(long*)&x, *(long*)&y))
#define fMul16(x, y, z) (void)((*(long*)&z) = r16Mul(*(long*)&x, *(long*)&y))
#define fSqr16(x, z) (void)((*(long*)&z) = r16Sqr(*(long*)&x))
#define fSqr(x, z) (void)((*(long*)&z) = RegSqrFloat(*(long*)&x))
#define fShift(x, Shift, z) (void)((*(long*)&z) = \
   RegSftFloat(*(long*)&x, Shift))
#define Fg2Float(x, f, z) (void)((*(long*)&z) = RegFg2Float(x, f))
#define Float2Fg(x, f) RegFloat2Fg(*(long*)&x, f)
#define fSin12(x, z) (void)((*(long*)&z) = \
   RegFg2Float((long)sin13(Float2Fg(x, 13)), 13))
#define fCos12(x, z) (void)((*(long*)&z) = \
   RegFg2Float((long)cos13(Float2Fg(x, 13)), 13))
#define fSinh12(x, z) (void)((*(long*)&z) = \
   RegFg2Float(sinh13(Float2Fg(x, 13)), 13))
#define fCosh12(x, z) (void)((*(long*)&z) = \
   RegFg2Float(cosh13(Float2Fg(x, 13)), 13))
#define fLog14(x, z) (void)((*(long*)&z) = \
        RegFg2Float(LogFloat14(*(long*)&x), 16))
#define fExp14(x, z) (void)((*(long*)&z) = ExpFloat14(*(long*)&x));
#define fPow14(x, y, z)		\
	do						\
	{						\
		fLog14(x, z);		\
		fMul16(z, y, z);	\
		fExp14(z, z);		\
	} while (0)
#define fSqrt14(x, z)		\
	do						\
	{						\
		fLog14(x, z);		\
		fShift(z, -1, z);	\
		fExp14(z, z);		\
	}						\
	while (0)

struct fComplex {
   float x, y, mod;
};

void
   fSqrZ(struct fComplex *x, struct fComplex *z),
   fMod(struct fComplex *x),
   fInvZ(struct fComplex *x, struct fComplex *z),
   fMulZ(struct fComplex *x, struct fComplex *y, struct fComplex *z),
   fDivZ(struct fComplex *x, struct fComplex *y, struct fComplex *z),
   fSinZ(struct fComplex *x, struct fComplex *z),
   fCosZ(struct fComplex *x, struct fComplex *z),
   fTanZ(struct fComplex *x, struct fComplex *z),
   fSinhZ(struct fComplex *x, struct fComplex *z),
   fCoshZ(struct fComplex *x, struct fComplex *z),
   fTanhZ(struct fComplex *x, struct fComplex *z);

#endif
